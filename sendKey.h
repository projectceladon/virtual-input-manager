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

#ifndef SENDKEY_H_
#define SENDKEY_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define POWER_BUTTON_FILE_PATH "/tmp/pwr-key"

#define VOLUME_BUTTON_FILE_PATH "/tmp/vol-key"

struct mQData {
    long type;
    unsigned bCtrl;
};

enum button {POWER, VOLUME};
enum volumeButtonControl {UP, DOWN};
#endif  /*SENDKEY_H_*/
