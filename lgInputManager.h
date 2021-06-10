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

#ifndef LGINPUTMANAGER_H_
#define LGINPUTMANAGER_H_

#include "lgInputDevice.h"

class lgInputManager {
 public:
    int checkDeviceExist(string);
    void processInputDevice(uint16_t);
    void getDevInfo(const char *, struct device *);
    int getDevices(unsigned, struct device *);
    void setGvtdMode(bool mode);
    bool getGvtdMode();
    int devCnt;

 private:
    bool bGvtdMode;
};
#endif  /*LGINPUTMANAGER_H_*/
