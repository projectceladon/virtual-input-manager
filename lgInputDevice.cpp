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

    sprintf(str, "input-looking-glass%d", deviceType);
    uInputName = str;
    struct uinput_user_dev dev;

    /*Event type bits*/
    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_EVBIT, EV_ABS);
    ioctl(ufd, UI_SET_EVBIT, EV_SYN);
    ioctl(ufd, UI_SET_EVBIT, EV_MSC);
    ioctl(ufd, UI_SET_EVBIT, EV_LED);

    /*ABS bits code*/
    ioctl(ufd, UI_SET_ABSBIT, ABS_X);
    ioctl(ufd, UI_SET_ABSBIT, ABS_Y);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(ufd, UI_SET_ABSBIT, ABS_MT_TOOL_TYPE);
    ioctl(ufd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    /*miscellaneous bits code*/
    ioctl(ufd, UI_SET_MSCBIT, MSC_SCAN);

    /*LED bits code*/
    ioctl(ufd, UI_SET_LEDBIT, LED_NUML);
    ioctl(ufd, UI_SET_LEDBIT, LED_CAPSL);
    ioctl(ufd, UI_SET_LEDBIT, LED_SCROLLL);
    ioctl(ufd, UI_SET_LEDBIT, LED_COMPOSE);
    ioctl(ufd, UI_SET_LEDBIT, LED_KANA);

    /*key bits code*/
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOUCH);

    ioctl(ufd, UI_SET_KEYBIT, KEY_RESERVED);
    ioctl(ufd, UI_SET_KEYBIT, KEY_ESC);
    ioctl(ufd, UI_SET_KEYBIT, KEY_1);
    ioctl(ufd, UI_SET_KEYBIT, KEY_2);
    ioctl(ufd, UI_SET_KEYBIT, KEY_3);
    ioctl(ufd, UI_SET_KEYBIT, KEY_4);
    ioctl(ufd, UI_SET_KEYBIT, KEY_5);
    ioctl(ufd, UI_SET_KEYBIT, KEY_6);
    ioctl(ufd, UI_SET_KEYBIT, KEY_7);
    ioctl(ufd, UI_SET_KEYBIT, KEY_8);
    ioctl(ufd, UI_SET_KEYBIT, KEY_9);
    ioctl(ufd, UI_SET_KEYBIT, KEY_0);
    ioctl(ufd, UI_SET_KEYBIT, KEY_MINUS);
    ioctl(ufd, UI_SET_KEYBIT, KEY_EQUAL);
    ioctl(ufd, UI_SET_KEYBIT, KEY_BACKSPACE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_TAB);
    ioctl(ufd, UI_SET_KEYBIT, KEY_Q);
    ioctl(ufd, UI_SET_KEYBIT, KEY_W);
    ioctl(ufd, UI_SET_KEYBIT, KEY_E);
    ioctl(ufd, UI_SET_KEYBIT, KEY_R);
    ioctl(ufd, UI_SET_KEYBIT, KEY_T);
    ioctl(ufd, UI_SET_KEYBIT, KEY_Y);
    ioctl(ufd, UI_SET_KEYBIT, KEY_U);
    ioctl(ufd, UI_SET_KEYBIT, KEY_I);
    ioctl(ufd, UI_SET_KEYBIT, KEY_O);
    ioctl(ufd, UI_SET_KEYBIT, KEY_P);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFTBRACE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHTBRACE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_ENTER);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(ufd, UI_SET_KEYBIT, KEY_A);
    ioctl(ufd, UI_SET_KEYBIT, KEY_S);
    ioctl(ufd, UI_SET_KEYBIT, KEY_D);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F);
    ioctl(ufd, UI_SET_KEYBIT, KEY_G);
    ioctl(ufd, UI_SET_KEYBIT, KEY_H);
    ioctl(ufd, UI_SET_KEYBIT, KEY_J);
    ioctl(ufd, UI_SET_KEYBIT, KEY_K);
    ioctl(ufd, UI_SET_KEYBIT, KEY_L);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SEMICOLON);
    ioctl(ufd, UI_SET_KEYBIT, KEY_APOSTROPHE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_GRAVE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFTSHIFT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_BACKSLASH);
    ioctl(ufd, UI_SET_KEYBIT, KEY_Z);
    ioctl(ufd, UI_SET_KEYBIT, KEY_X);
    ioctl(ufd, UI_SET_KEYBIT, KEY_C);
    ioctl(ufd, UI_SET_KEYBIT, KEY_V);
    ioctl(ufd, UI_SET_KEYBIT, KEY_B);
    ioctl(ufd, UI_SET_KEYBIT, KEY_N);
    ioctl(ufd, UI_SET_KEYBIT, KEY_M);
    ioctl(ufd, UI_SET_KEYBIT, KEY_COMMA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_DOT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SLASH);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHTSHIFT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPASTERISK);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFTALT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SPACE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_CAPSLOCK);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F1);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F2);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F3);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F4);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F5);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F6);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F7);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F8);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F9);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F10);
    ioctl(ufd, UI_SET_KEYBIT, KEY_NUMLOCK);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SCROLLLOCK);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP7);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP8);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP9);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPMINUS);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP4);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP5);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP6);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPPLUS);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP1);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP2);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP3);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KP0);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPDOT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_ZENKAKUHANKAKU);
    ioctl(ufd, UI_SET_KEYBIT, KEY_102ND);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F11);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F12);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RO);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KATAKANA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HIRAGANA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HENKAN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KATAKANAHIRAGANA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_MUHENKAN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPJPCOMMA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPENTER);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHTCTRL);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPSLASH);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SYSRQ);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHTALT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LINEFEED);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HOME);
    ioctl(ufd, UI_SET_KEYBIT, KEY_UP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PAGEUP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_END);
    ioctl(ufd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PAGEDOWN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_INSERT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_DELETE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_MACRO);
    ioctl(ufd, UI_SET_KEYBIT, KEY_MUTE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEUP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_POWER);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPEQUAL);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPPLUSMINUS);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PAUSE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SCALE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPCOMMA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HANGEUL);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HANJA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_YEN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_LEFTMETA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_RIGHTMETA);
    ioctl(ufd, UI_SET_KEYBIT, KEY_COMPOSE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_STOP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_AGAIN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PROPS);
    ioctl(ufd, UI_SET_KEYBIT, KEY_UNDO);
    ioctl(ufd, UI_SET_KEYBIT, KEY_FRONT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_COPY);
    ioctl(ufd, UI_SET_KEYBIT, KEY_OPEN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PASTE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_FIND);
    ioctl(ufd, UI_SET_KEYBIT, KEY_CUT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_HELP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_CALC);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SLEEP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_WWW);
    ioctl(ufd, UI_SET_KEYBIT, KEY_COFFEE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_BACK);
    ioctl(ufd, UI_SET_KEYBIT, KEY_FORWARD);
    ioctl(ufd, UI_SET_KEYBIT, KEY_EJECTCD);
    ioctl(ufd, UI_SET_KEYBIT, KEY_NEXTSONG);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PLAYPAUSE);
    ioctl(ufd, UI_SET_KEYBIT, KEY_PREVIOUSSONG);
    ioctl(ufd, UI_SET_KEYBIT, KEY_STOPCD);
    ioctl(ufd, UI_SET_KEYBIT, KEY_REFRESH);
    ioctl(ufd, UI_SET_KEYBIT, KEY_EDIT);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SCROLLUP);
    ioctl(ufd, UI_SET_KEYBIT, KEY_SCROLLDOWN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPLEFTPAREN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_KPRIGHTPAREN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F13);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F14);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F15);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F16);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F17);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F18);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F19);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F20);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F21);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F22);
    ioctl(ufd, UI_SET_KEYBIT, KEY_F23);

    memset(&dev, 0, sizeof(dev));
    snprintf(dev.name, sizeof(dev.name), "IntelvTouch");
    sprintf(dev.name, "input-looking-glass%d", deviceType);
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
    sprintf(path, "%s%d", INPUT_DEVICE_FILE_PATH ,deviceType);
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
                printf("error no: %s mousetype %d\n", strerror(errno), vD->deviceType);
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
