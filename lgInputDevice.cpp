/*
 * Copyright (c) 2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <thread>
#include <errno.h>

#include "sendKey.h"
#include "lgInputDevice.h"

lgInputDevice::lgInputDevice()
{
    type = 0;
    ufd = 0;
    mqId = 0;
    virt = false;
    sourceDev.count = 0;
}

void lgInputDevice::setVirtMode(bool mode)
{
    virt = mode;
}

bool lgInputDevice::getVirtMode()
{
    return virt;
}

lgInputDevice::~lgInputDevice()
{
    if (ufd > 0)
        close(ufd);

    for (int i = 0; i < sourceDev.count; i++)
        if (fd[i] > 0)
            close(fd[i]);

    if (mqId > 0) {
        msgctl(mqId, IPC_RMID, NULL);
    }
}

int lgInputDevice::getDevices(uint16_t keyCode, struct device *dev)
{
    const ssize_t keyBitsSize = (KEY_MAX / 8) + 1;

    dev->count = 0;
    if((keyCode / 8) >= keyBitsSize)
        return dev->count;

    for (int i = 0; i < MAX_DEV; i++) {
        int fd = 0, res = 0;
        unsigned int k = 0;
        char devPath[70] = " ";

        snprintf(devPath, sizeof(devPath), "/dev/input/event%d", i);
        fd = open(devPath, O_RDONLY);
        if (fd < 0)
            continue;

        res = ioctl(fd, EVIOCGBIT(0, sizeof(k)), &k);
        if (res < 0) {
            close(fd);
            continue;
        }

        if (k & (1 << EV_KEY)) {
            uint8_t keyBits[keyBitsSize] = {};

            res = ioctl(fd, EVIOCGBIT(EV_KEY, keyBitsSize), keyBits);
            if (res < 0) {
                close(fd);
                continue;
            }

            if (keyBits[keyCode/8] & (1 << (keyCode % 8))) {
                dev->path[dev->count] = "/dev/input/event" + to_string(i);
                dev->count++;
            }
        }
        close(fd);
    }

    return dev->count;
}

int lgInputDevice::openSourceDev()
{
    for (int i = 0; i < sourceDev.count; i++) {
        const char *devPath = sourceDev.path[i].c_str();
        if ((fd[i] = open(devPath, O_RDONLY | O_CLOEXEC)) < 0)
            return -1;
    }

    return 0;
}

int lgInputDevice::openUinputDev()
{
    if ((ufd = open("/dev/uinput", O_WRONLY | O_SYNC)) < 0) {
        cout << "Failed to open uinput device" << endl;
        return -1;
    }

    return 0;
}

int lgInputDevice::createInputDevice(uint16_t keyCode, bool gvtdMode)
{
    struct input_id uid = {};
    struct uinput_setup usetup = {};

    if (gvtdMode == true) {
        if (!getDevices(keyCode, &sourceDev))
            setVirtMode(true);
        else
            setVirtMode(false);
    } else {
       setVirtMode(true);
    }

    if (openUinputDev() < 0) {
        cout << "Failed to open uinput device" << endl;
        return -1;
    }

    char str[100];

    sprintf(str, "input-looking-glass%d", mouseType);
    uInputName = str;
    struct uinput_user_dev dev;
    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_EVBIT, EV_ABS);
    ioctl(ufd, UI_SET_EVBIT, EV_SYN);
    ioctl(ufd, UI_SET_ABSBIT, ABS_X);
    ioctl(ufd, UI_SET_ABSBIT, ABS_Y);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TOOL_TYPE);
    ioctl(ufd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOUCH);

    memset(&dev, 0, sizeof(dev));
    snprintf(dev.name, sizeof(dev.name), "IntelvTouch");
    sprintf(dev.name, "input-looking-glass%d", mouseType);
    dev.id.bustype = BUS_USB;
    dev.id.vendor  = 0x1;
    dev.id.product = 0x1;
    dev.id.version = 1;

    signed int touch_xres = 600;
    signed int touch_yres = 960;

    /* single touch inputs */
    dev.absmax[ABS_MT_SLOT] = 0;

    dev.absmin[ABS_X] = 0;
    dev.absmax[ABS_X] = touch_xres;

    dev.absmin[ABS_Y] = 0;
    dev.absmax[ABS_Y] = touch_yres;

    dev.absmin[ABS_MT_POSITION_X] = 0;
    dev.absmax[ABS_MT_POSITION_X] = touch_xres;

    dev.absmin[ABS_MT_POSITION_Y] = 0;
    dev.absmax[ABS_MT_POSITION_Y] = touch_yres;

    dev.absmin[ABS_PRESSURE] = 0;
    dev.absmax[ABS_PRESSURE] = 0xff;

    dev.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    dev.absmax[ABS_MT_TOUCH_MAJOR] = 0xff;

    dev.absmin[ABS_MT_PRESSURE] = 0;
    dev.absmax[ABS_MT_PRESSURE] = 0xff;

    write(ufd, &dev, sizeof(dev));
    ioctl(ufd, UI_DEV_CREATE);

    type = MOUSE;

    return 0;
}

int lgInputDevice::createSymLink()
{
    for (int i = 0; i < MAX_DEV; i++) {
        char name[100] = " ";
        string str;
        ifstream infile;

        snprintf(name, sizeof(name), "/sys/class/input/event%d/device/name", i);
        infile.open(name);
        if (!infile.is_open())
            continue;

        getline(infile, str);
        if (!strncmp(str.c_str(), uInputName.c_str(), str.size() + 1)) {
            char cmd[150];

            softLinkPath = "/dev/input/by-id/";
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", softLinkPath.c_str());
            system(cmd);
            softLinkPath.append(uInputName);
            std::replace(softLinkPath.begin(), softLinkPath.end(), ' ', '-');
            snprintf(cmd, sizeof(cmd), "ln -sf /dev/input/event%d %s",
                                            i, softLinkPath.c_str());
            system(cmd);
            return 0;
        }
        infile.close();
    }

    return -1;
}

void lgInputDevice::sendEvent(uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev = {};

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(ufd, &ev, sizeof(ev));
}

int lgInputDevice::getMsgQ()
{
    key_t key = 0;
    char path[100];
    char cmd[100];
    sprintf(path, "%s%d", MOUSE_BUTTON_FILE_PATH,mouseType);
    sprintf(cmd, "touch %s",path);
    system(cmd);
    key = ftok(path, 99);
    mqId = msgget(key, 0666 | IPC_CREAT);
    if (mqId < 0) {
	cout << "Failed to get msgq id" << endl;
	sprintf(cmd, "rm %s",path);
	system(cmd);
	return -1;
    }

    /*
     * Clean up message queue buffer
     */
    struct input_event ev = {};
    int n = 1;
    while (n > 0) {
        n = msgrcv(mqId, &ev, sizeof(struct input_event), 1, IPC_NOWAIT);
    }

    return mqId;
}

static void sendKeyThread(lgInputDevice *vDev)
{
   struct mQData1 mD = {};
    lgInputDevice *vD = vDev;
    vD->type = MOUSE;
    int mqId = vD->getMsgQ();
    while (true) {
            if (msgrcv(mqId, &mD, sizeof(struct mQData1), 1, 0) < 0) {
                printf("error no: %s mousetype %d\n", strerror(errno), vD->mouseType);
                continue;
	    }
	    vD->sendEvent(mD.ev.type, mD.ev.code, mD.ev.value);
    }
}

static void realDeviceThread(lgInputDevice *vD)
{
    struct pollfd pfd[MAX_DEV] = {};
    struct input_event evBuf[10] = {};
    int cnt = vD->sourceDev.count;
    int res = 0;

    for (int i = 0; i < cnt; i++) {
        pfd[i].fd = vD->fd[i];
        pfd[i].events = POLLIN;
    }

    while (true) {
        if (poll(pfd, cnt, -1) < 0) {
            cout << "Failed to poll in input device" << endl;
            return;
        }

        for (int i = 0; i < cnt; i++)
            if (pfd[i].revents & POLLIN) {
                int j = 0;
                while (true) {
                    res = read(pfd[i].fd, &evBuf[j], sizeof(input_event));
                    if (res < 0) {
                        cout << "could not get event" << endl;
                        break;
                    }
                    if ((!evBuf[j].type) && (!evBuf[j].code) &&
                                        (!evBuf[i].value)) {
                         if ((evBuf[0].code == 116) && (evBuf[0].type == 1) &&
                                                      (evBuf[0].value == 1)) {
                             system("sudo python3 ./wakeup.py");
                             usleep(800000); /* 800ms delay */
                         }
                         for (int k = 0; k <= j; k++) {
                            vD->sendEvent(evBuf[k].type, evBuf[k].code,
                                                    evBuf[k].value);
                         }
                         j = 0;
                         continue;
                    }
                    j++;
                }
            }
    }
}

void lgInputDevice::pollAndPushEvents()
{
    if (getVirtMode() == true) {
        sendKeyThread(this);
    } else {
        thread sK(sendKeyThread, this);
        thread rD(realDeviceThread, this);
        sK.join();
        rD.join();
    }
}
