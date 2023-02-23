/*
 * Copyright (c) 2020 Intel Corporation
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

#include "sendKey.h"
#include "vInputDevice.h"

vInputDevice::vInputDevice()
{
    type = 0;
    ufd = 0;
    mqId = 0;
    virt = false;
    sourceDev.count = 0;
}

void vInputDevice::setVirtMode(bool mode)
{
    virt = mode;
}

bool vInputDevice::getVirtMode()
{
    return virt;
}

vInputDevice::~vInputDevice()
{
    if (ufd > 0)
        close(ufd);

    for (int i = 0; i < sourceDev.count; i++)
        if (fd[i] > 0)
            close(fd[i]);

    if (mqId > 0) {
        msgctl(mqId, IPC_RMID, NULL);
        if (type == POWER)
            system("rm " POWER_BUTTON_FILE_PATH);
        else if (type == VOLUME)
            system("rm " VOLUME_BUTTON_FILE_PATH);
    }
}

int vInputDevice::getDevices(uint16_t keyCode, struct device *dev)
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

int vInputDevice::openSourceDev()
{
    for (int i = 0; i < sourceDev.count; i++) {
        const char *devPath = sourceDev.path[i].c_str();
        if ((fd[i] = open(devPath, O_RDONLY | O_CLOEXEC)) < 0)
            return -1;
    }

    return 0;
}

int vInputDevice::openUinputDev()
{
    if ((ufd = open("/dev/uinput", O_WRONLY | O_SYNC)) < 0) {
        cout << "Failed to open uinput device" << endl;
        return -1;
    }

    return 0;
}

int vInputDevice::createInputDevice(uint16_t keyCode, bool gvtdMode)
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

    if (getVirtMode() == false) {
        if (openSourceDev() < 0) {
            cout << "Failed to open source device" << endl;
            return -1;
        }
    }

    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    switch (keyCode) {
    case KEY_POWER:
        uInputName = "Power Button vm0";
        type = POWER;
        ioctl(ufd, UI_SET_KEYBIT, KEY_POWER);
        break;
    case KEY_VOLUMEDOWN:
    case KEY_VOLUMEUP:
        uInputName = "Volume Button vm0";
        type = VOLUME;
        ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
        ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEUP);
        break;
    default:
        return -1;
    }

    memset(&uid, 0, sizeof(struct input_id));
    memset(&usetup, 0, sizeof(usetup));
    usetup.id = uid;
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "%s", uInputName.c_str());
    ioctl(ufd, UI_DEV_SETUP, &usetup);
    ioctl(ufd, UI_DEV_CREATE);

    return 0;
}

int vInputDevice::createSymLink()
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

void vInputDevice::sendEvent(uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev = {};

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(ufd, &ev, sizeof(ev));
}

int vInputDevice::getMsgQ()
{
    key_t key = 0;

    if (type == POWER) {
        system("touch " POWER_BUTTON_FILE_PATH);
        key = ftok(POWER_BUTTON_FILE_PATH, 99);
        mqId = msgget(key, 0666 | IPC_CREAT);
        if (mqId < 0) {
            cout << "Failed to get msgq id" << endl;
            system("rm " POWER_BUTTON_FILE_PATH);
            return -1;
        }
    } else if (type == VOLUME) {
        system("touch " VOLUME_BUTTON_FILE_PATH);
        key = ftok(VOLUME_BUTTON_FILE_PATH, 99);
        mqId = msgget(key, 0666 | IPC_CREAT);
        if (mqId < 0) {
            cout << "Failed to get msgq id" << endl;
            system("rm " VOLUME_BUTTON_FILE_PATH);
            return -1;
        }
    }

    /*
     * Clean up message queue buffer
     */
    struct mQData data = {};
    int n = 1;
    while (n > 0) {
        n = msgrcv(mqId, &data, sizeof(struct mQData) - sizeof(long), 1, IPC_NOWAIT);
    }

    return mqId;
}

static void sendKeyThread(vInputDevice *vDev)
{
    struct mQData data = {};
    vInputDevice *vD = vDev;
    int mqId = vD->getMsgQ();

    if (mqId < 0) {
            cout << "Failed to get msgq id" << endl;
            return ;
    }

    while (true) {
        msgrcv(mqId, &data, sizeof(struct mQData) - sizeof(long), 1, 0);
        if (vD->type == VOLUME) {
            switch (data.bCtrl) {
            case UP:
                vD->sendEvent(0x04, 0x04, 0xc4);
                vD->sendEvent(0x01, KEY_VOLUMEUP, 0x01);
                vD->sendEvent(0x00, 0x00, 0x00);
                usleep(50000);
                vD->sendEvent(0x01, KEY_VOLUMEUP, 0x00);
                vD->sendEvent(0x00, 0x00, 0x00);
                break;
            case DOWN:
                vD->sendEvent(0x04, 0x04, 0xc6);
                vD->sendEvent(0x01, KEY_VOLUMEDOWN, 0x01);
                vD->sendEvent(0x00, 0x00, 0x00);
                usleep(50000);
                vD->sendEvent(0x01, KEY_VOLUMEDOWN, 0x00);
                vD->sendEvent(0x00, 0x00, 0x00);
                break;
            default:
                cout << "Invalid Ctrl option" << endl;
            }
        } else if (vD->type == POWER) {
            system("sudo python3 ./wakeup.py");
            usleep(800000); /* 800ms delay */
            vD->sendEvent(0x01, 0x074, 0x01);
            vD->sendEvent(0x00, 0x00, 0x00);
            if (data.bCtrl)
                sleep(data.bCtrl);
            else
                usleep(50000);

            vD->sendEvent(0x01, 0x074, 0x00);
            vD->sendEvent(0x00, 0x00, 0x00);
        }
    }
}

static void realDeviceThread(vInputDevice *vD)
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

void vInputDevice::pollAndPushEvents()
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
