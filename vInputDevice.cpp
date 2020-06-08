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

int vInputDevice::createVolumeDevice()
{
    struct uinput_setup usetup = {};
    struct input_id uid  = {};
    const char btnName[] =  "Virtual-Volume-Button";

    openUinputDev();
    snprintf(devName, sizeof(devName), btnName);
    uInputName = devName;
    type = VOLUME;
    memset(&uid, 0, sizeof(struct input_id));
    memset(&usetup, 0, sizeof(usetup));
    usetup.id = uid;
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, btnName);
    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_EVBIT, EV_MSC);
    ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
    ioctl(ufd, UI_SET_KEYBIT, KEY_VOLUMEUP);
    ioctl(ufd, UI_SET_MSCBIT, MSC_SCAN);
    ioctl(ufd, UI_DEV_SETUP, &usetup);
    ioctl(ufd, UI_DEV_CREATE);

    return 0;
}

int vInputDevice::configureUinputDev()
{
    ssize_t bits_size = 0;
    unsigned setbittype = 0, nosetbittype = 0;
    int res = 0;
    uint8_t *bits = NULL;

    for (int i = EV_KEY; i <= EV_MAX; i++) {
        while (true) {
            res = ioctl(fd[0], EVIOCGBIT(i, bits_size), bits);
            if (res < bits_size)
                break;
            bits_size = res + 16;
            bits = reinterpret_cast<uint8_t *>(realloc(bits, bits_size * 2));
            if (bits == NULL)
                err(1, "failed to allocate buffer of size %d\n",
                                   static_cast<int>(bits_size));
        }

        switch (i) {
        case EV_KEY:
            ioctl(fd[0], EVIOCGKEY(res), bits + bits_size);
            setbittype = UI_SET_KEYBIT;
            break;
        case EV_REL:
            setbittype = UI_SET_RELBIT;
            break;
        case EV_ABS:
            setbittype = UI_SET_ABSBIT;
            break;
        case EV_MSC:
            setbittype = UI_SET_MSCBIT;
            break;
        case EV_LED:
            ioctl(fd[0], EVIOCGLED(res), bits + bits_size);
            setbittype = UI_SET_LEDBIT;
            break;
        case EV_SND:
            ioctl(fd[0], EVIOCGSND(res), bits + bits_size);
            setbittype = UI_SET_SNDBIT;
            break;
        case EV_SW:
            ioctl(fd[0], EVIOCGSW(bits_size), bits + bits_size);
            setbittype = UI_SET_SWBIT;
            break;
        case EV_REP:
            nosetbittype = 1;
            break;
        case EV_FF:
            setbittype = UI_SET_FFBIT;
            break;
        case EV_PWR:
            nosetbittype = 1;
            break;
        case EV_FF_STATUS:
            setbittype = UI_SET_FFBIT;
            break;
        }

        if (nosetbittype)
            continue;

        int flag = 0;
        for (int j = 0; j < res; j++) {
            for (int k = 0; k < 8; k++)
                if (bits[j] & 1 << k) {
                    if (!flag) {
                        ioctl(ufd, UI_SET_EVBIT, EV_FF_STATUS);
                        flag = 1;
                    }

                    ioctl(ufd, UI_SET_EVBIT, i);
                    ioctl(ufd, setbittype, (j*8+k));
                    if (i == EV_ABS) {
                        struct input_absinfo abs = {};
                        if (ioctl(fd[0], EVIOCGABS(j * 8 + k), &abs) == 0) {
                            //Fix, Add ABS info
                        }
                    }
                }
        }
        nosetbittype = 0;
    }

    free(bits);

    return 0;
}

int vInputDevice::openSourceDev()
{
    for (int i = 0; i < sourceDev->count; i++) {
        const char *devPath = sourceDev->path[i].c_str();
        if ((fd[i] = open(devPath, O_RDONLY | O_CLOEXEC)) < 0)
            return -1;
    }

    return 0;
}

int vInputDevice::openUinputDev()
{
    if ((ufd = open("/dev/uinput", O_WRONLY)) < 0) {
        cout << "Failed to open uinput device" << endl;
        return -1;
    }

    return 0;
}

int vInputDevice::setDevName()
{
    devName[sizeof(devName) - 1] = '\0';
    if (ioctl(fd[0], EVIOCGNAME(sizeof(devName) - 1), &devName) < 1) {
        devName[0] = '\0';
        return -1;
    }

    return 0;
}

int vInputDevice::createInputDevice(const char *vmDev)
{
    struct input_id uid = {};
    struct uinput_setup usetup = {};

    if (openUinputDev() < 0) {
        cout << "Failed to open uinput device" << endl;
        return -1;
    }

    if (openSourceDev() < 0) {
        cout << "Failed to open source device" << endl;
        return -1;
    }

    setDevName();
    if (configureUinputDev() != 0) {
        cout << "Failed to set bits" << endl;
        return -1;
    }

    memset(&uid, 0, sizeof(struct input_id));
    memset(&usetup, 0, sizeof(usetup));
    usetup.id = uid;
    uInputName = devName;
    uInputName.append("-");
    uInputName.append(vmDev);
    type = POWER;  /*Fix Hard code value*/
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "%s", uInputName.c_str());
    ioctl(ufd, UI_DEV_SETUP, &usetup);
    ioctl(ufd, UI_DEV_SETUP, &usetup);
    ioctl(ufd, UI_DEV_CREATE);
    return 0;
}

int vInputDevice::createSymLink()
{
    for (int i = 0; i < 20; i++) {
        char name[100] = " ";
        string str;
        ifstream infile;
        snprintf(name, sizeof(name), "/sys/class/input/event%d/device/name",
                                                                        i);
        infile.open(name);
        if (!infile.is_open())
            continue;

        getline(infile, str);
        if (!strncmp(str.c_str(), uInputName.c_str(), str.size() + 1)) {
            char cmd[150];

            sLinkPath = "/dev/input/by-id/";
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", sLinkPath.c_str());
            system(cmd);
            sLinkPath.append(uInputName);
            std::replace(sLinkPath.begin(), sLinkPath.end(), ' ', '-');
            snprintf(cmd, sizeof(cmd), "ln -sf /dev/input/event%d %s",
                                            i, sLinkPath.c_str());
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
    int mqId = -1;

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

    return mqId;
}

static void sendKeyThread(vInputDevice *vD)
{
    struct mQData data = {};
    int mqId = vD->getMsgQ();

    while (true) {
        msgrcv(mqId, &data, sizeof(struct mQData), 1, 0);
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
    struct input_event event = {};
    int cnt = vD->sourceDev->count;
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
                res = read(pfd[i].fd, &event, sizeof(event));
                if (res < static_cast<int>(sizeof(event))) {
                    cout << "could not get event" << endl;
                    continue;
                }

                vD->sendEvent(event.type, event.code, event.value);
            }
    }
}

void vInputDevice::pollAndPushEvents()
{
    thread sK(sendKeyThread, this);
    thread rD(realDeviceThread, this);
    sK.join();
    rD.join();
}
