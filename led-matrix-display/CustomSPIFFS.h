// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifdef ESP32
#ifndef _CUSTOM_SPIFFS_H_
#define _CUSTOM_SPIFFS_H_

#include "FS.h"

class CustomSPIFFSFS : public FS
{
public:
    CustomSPIFFSFS();
    bool begin(bool formatOnFail=false, const char * partition="user", const char * basePath="/user", uint8_t maxOpenFiles=10);
    bool format();
    size_t totalBytes();
    size_t usedBytes();
    void end();

private:
    const char* _partition;
};

#endif

#endif