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

#ifndef VINPUTDEVICE_H_
#define VINPUTDEVICE_H_

#include <string>

#define MAX_DEV 20

using namespace std;

struct device {
    int count;
    string path[MAX_DEV];
};

class vInputDevice{
 public:
    int createInputDevice(const char *);
    int createSymLink();
    void pollAndPushEvents();
    int createVolumeDevice();
    void sendEvent(uint16_t, uint16_t, int32_t);
    struct device *sourceDev;
    int getMsgQ();
    int type;
    int fd[MAX_DEV];  /*fd for source /dev/input/eventX file*/

 private:
    char devName[30];  /*source device name*/
    string uInputName;  /*virtual input device name*/
    string sLinkPath;  /*softlink path*/
    int ufd;  /*fd for uinput device*/
    int openSourceDev();
    int openUinputDev();
    int setDevName();
    int configureUinputDev();
};
#endif  /*VINPUTDEVICE_H_*/
