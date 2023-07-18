

#ifndef __NVS_COMPONENT_H__
#define __NVS_COMPONENT_H__

#include "nvs_flash.h"
#include "esp_err.h"

esp_err_t initialize_nvs_partition();
esp_err_t set_wifi_cred(const char *wifi_cred);
size_t get_wifi_detail(char *wifi_cred);
#endif // __NVS_COMPONENT_H__


