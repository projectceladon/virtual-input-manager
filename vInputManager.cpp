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

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <err.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <memory>

#include "vInputManager.h"

void vInputManager::getDevInfo(const char name[], struct device *dev)
{
    dev->count = 0;
    for (int i = 0; i < 20; i++) {
        char sysPath[70] = " ";
        string str;
        ifstream infile;

        snprintf(sysPath, sizeof(sysPath),
            "/sys/class/input/event%d/device/name", i);
        infile.open(sysPath);
        if (!infile.is_open())
            continue;

        getline(infile, str);
        if (!strncmp(str.c_str(), name, str.size() + 1)) {
            dev->path[dev->count] = "/dev/input/event" + to_string(i);
            dev->count++;
        }
    }
}

int vInputManager::checkDeviceExist(const char *devName)
{
    if (!devName)
        return -1;

    for (int i = 0; i < 20; i++) {
        char fileName[70] = " ";
        string str;
        ifstream infile;

        snprintf(fileName, sizeof(fileName),
            "/sys/class/input/event%d/device/name", i);
        infile.open(fileName);
        if (!infile.is_open())
            continue;

        getline(infile, str);
        if (!strncmp(str.c_str(), devName, str.size() + 1))
            return 0;

        infile.close();
    }

    return -1;
}

void vInputManager::processInputDevice(int index)
{
    vInputDevice vD;

    vD.sourceDev = &dev[index];
    if (vD.createInputDevice("vm0") < 0) {
        cout << "processInputDevice: Failed to create input device" << endl;
        exit(0);
    }

    vD.createSymLink();
    int pid = fork();
    if (!pid) {
        vD.pollAndPushEvents();
    } else if (pid > 0) {
        cout << "Started-process" << endl;
    } else {
        cout << "Failed to create a process to transfer events" << endl;
        exit(0);
    }
}

void vInputManager::processVirtualInputDevice()
{
    if (!checkDeviceExist("Virtual-Volume-Button")) {
        cout << "device exists" << endl;
        return;
    }

    vInputDevice vD;
    vD.createVolumeDevice();
    vD.createSymLink();
    int pid = fork();
    if (!pid) {
        vD.pollAndPushEvents();
    } else if (pid > 0) {
        cout << "Created process" << endl;
    } else {
        cout << "Failed to create a process to transfer events" << endl;
    }
}

int main()
{
    vInputManager vM;

    vM.devCnt = 1;  /*Supporting only Power device.Fix hard code value*/
    vM.dev = new struct device;
    vM.getDevInfo("Power Button", vM.dev);
    for (int i = 0; i < vM.devCnt; i++) {
        if (!vM.checkDeviceExist("Power Button-vm0")) {  //Fix hard code string
            cout << "device already exists" << endl;
            continue;
        }
        vM.processInputDevice(i);
    }

    vM.processVirtualInputDevice();
    delete vM.dev;

    return 0;
}
