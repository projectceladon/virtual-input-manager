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

int vInputManager::checkDeviceExist(string devName)
{
    for (int i = 0; i < MAX_DEV; i++) {
        char fileName[70] = " ";
        string str;
        ifstream infile;

        snprintf(fileName, sizeof(fileName),
            "/sys/class/input/event%d/device/name", i);
        infile.open(fileName);
        if (!infile.is_open())
            continue;

        getline(infile, str);
        if (!strncmp(devName.c_str(), str.c_str(), devName.size() + 1))
            return 0;

        infile.close();
    }

    return -1;
}

void vInputManager::processInputDevice(uint16_t keyCode)
{
    vInputDevice vD;

    if (vD.createInputDevice(keyCode) < 0) {
        cout << "processInputDevice: Failed to create input device" << endl;
        exit(0);
    }

    if (vD.createSymLink() < 0) {
        cout << "Failed to create softlink device" << endl;
        exit(0);
    }

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

int main()
{
    string devName[2] = {"Power Button vm0", "Volume Button vm0"};
    uint16_t keyCode[2] = {KEY_POWER, KEY_VOLUMEUP};
    vInputManager vM;

    vM.devCnt = 2;
    for (int i = 0; i < vM.devCnt; i++) {
        if (!vM.checkDeviceExist(devName[i])) {
            cout << "device already exists" << endl;
            continue;
        }
        vM.processInputDevice(keyCode[i]);
    }

    return 0;
}
