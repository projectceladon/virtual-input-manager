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
#include <iostream>
#include <cstdint>

#include "sendKey.h"

using namespace std;

static string helpMsg =
    "Usage: \n./sendkey --vm <0, 1, 2….n> --volume <up/down>\n"\
    "./sendkey --vm <0, 1, 2….n> --power <delay in seconds>\n"\
    "Example:\n\t./sendkey --vm 0 --volume up\n"\
    "\t./sendkey --vm 0 --volume down\n"\
    "\t./sendkey --vm 0 --power 0";

static void usage()
{
    cout << helpMsg << endl;
    exit(0);
}

static int initMsgQ(const char file[])
{
    key_t key = 0;
    int mqId = 0;

    if ((key = ftok(file, 99)) < 0) {
        cout << "Failed to get msgq key" << endl;
        return -1;
    }

    if ((mqId = msgget(key, 0666)) < 0) {
        cout << "Failed to get msgq id" << endl;
        return -1;
    }

    return mqId;
}

static void sendPowerEvent(int32_t ctrl)
{
    struct mQData buf = {1, 0};
    int mqId = 0;

    if ((mqId = initMsgQ(POWER_BUTTON_FILE_PATH)) < 0)
        exit(0);

    buf.bCtrl = static_cast<uint32_t>(ctrl);
    msgsnd(mqId, &buf, sizeof(buf) - sizeof(long), 0);
}

static void sendVolumeEvent(int32_t ctrl)
{
    struct mQData buf = {1, 0};
    int mqId = 0;

    if ((mqId = initMsgQ(VOLUME_BUTTON_FILE_PATH)) < 0)
        exit(0);

    switch (ctrl) {
    case UP:
        buf.bCtrl = UP;
        msgsnd(mqId, &buf, sizeof(buf) - sizeof(long), 0);
        break;
    case DOWN:
        buf.bCtrl = DOWN;
        msgsnd(mqId, &buf, sizeof(buf) - sizeof(long), 0);
        break;
    default:
        cout << "Invalid Volume control option" << endl;
        usage();
    }
}

static void parseArgs(uint32_t *button, int32_t *bCtrl, char **argv)
{
    if (!strncmp(reinterpret_cast<const char *>(argv[3]), "--volume", 9)) {
        *button = VOLUME;
        if (!strncmp(argv[4], "up", 3))
            *bCtrl = UP;
        else if (!strncmp(argv[4], "down", 5))
            *bCtrl = DOWN;
        else
            usage();
    } else if (!strncmp(reinterpret_cast<const char *>(argv[3]),
                                                "--power", 8)) {
        *button = POWER;
        string intS(argv[4]);
        *bCtrl = stoi(intS);
        if (*bCtrl < 0 || *bCtrl > 10) {
            cout << "Power button delay limit is 0 to 10 seconds" << endl;
            usage();
        }
    } else {
        usage();
    }
}

int main(int nargs, char **argv)
{
    uint32_t button = 0;
    int32_t bCtrl = 0;

    if ((nargs != 5) || strncmp(argv[1], "--vm", 5)) {
        cout << "--vm not found" << endl;
        usage();
        return 0;
    }

    parseArgs(&button, &bCtrl, argv);
    switch (button) {
    case POWER:
        cout << "sending power button" << endl;
        sendPowerEvent(bCtrl);
        break;
    case VOLUME:
        cout << "sending volume button" << endl;
        sendVolumeEvent(bCtrl);
        break;
    }

    return 0;
}
