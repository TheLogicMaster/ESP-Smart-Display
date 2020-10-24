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

#include "vfs_api.h"

extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
}

#include "CustomSPIFFS.h"

using namespace fs;

class CustomSPIFFSImpl : public VFSImpl
{
public:
    CustomSPIFFSImpl();
    virtual ~CustomSPIFFSImpl() { }
    virtual bool exists(const char* path);
};

CustomSPIFFSImpl::CustomSPIFFSImpl()
{
}

bool CustomSPIFFSImpl::exists(const char* path)
{
    File f = open(path, "r");
    return (f == true) && !f.isDirectory();
}

CustomSPIFFSFS::CustomSPIFFSFS() : FS(FSImplPtr(new CustomSPIFFSImpl()))
{
this->_partition = NULL;
}

bool CustomSPIFFSFS::begin(bool formatOnFail, const char * partition, const char * basePath, uint8_t maxOpenFiles)
{
    this->_partition = partition;
    
    if(esp_spiffs_mounted(_partition)){
        log_w("SPIFFS Already Mounted!");
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
      .base_path = basePath,
      .partition_label = _partition,
      .max_files = maxOpenFiles,
      .format_if_mount_failed = false
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if(err == ESP_FAIL && formatOnFail){
        if(format()){
            err = esp_vfs_spiffs_register(&conf);
        }
    }
    if(err != ESP_OK){
        log_e("Mounting SPIFFS failed! Error: %d", err);
        return false;
    }
    _impl->mountpoint(basePath);
    return true;
}

void CustomSPIFFSFS::end()
{
    if(esp_spiffs_mounted(_partition)){
        esp_err_t err = esp_vfs_spiffs_unregister(_partition);
        if(err){
            log_e("Unmounting SPIFFS failed! Error: %d", err);
            return;
        }
        _impl->mountpoint(_partition);
    }
}

bool CustomSPIFFSFS::format()
{
    disableCore0WDT();
    esp_err_t err = esp_spiffs_format(_partition);
    enableCore0WDT();
    if(err){
        log_e("Formatting SPIFFS failed! Error: %d", err);
        return false;
    }
    return true;
}

size_t CustomSPIFFSFS::totalBytes()
{
    size_t total,used;
    if(esp_spiffs_info(_partition, &total, &used)){
        return 0;
    }
    return total;
}

size_t CustomSPIFFSFS::usedBytes()
{
    size_t total,used;
    if(esp_spiffs_info(_partition, &total, &used)){
        return 0;
    }
    return used;
}
#endif