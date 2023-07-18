

#include "cJSON.h"
#include "trace.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "nvs_component.h"
#include "string.h"
#include "stdlib.h"

static nvs_handle_t wifi_ap_nvs_handle = 0;
static const char *nvs_wifi_cred = "wifi_cred";
static char *wifi_details = NULL;

esp_err_t initialize_nvs_partition()
{
    esp_err_t error = ESP_OK;
    
    TRACE_B("Initializing NVS flash");
    error = nvs_flash_init();
    if ((ESP_ERR_NVS_NO_FREE_PAGES == error) || (ESP_ERR_NVS_NEW_VERSION_FOUND == error))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_open(nvs_wifi_cred, NVS_READWRITE, &wifi_ap_nvs_handle));
    return error;
}

esp_err_t set_wifi_cred(const char *wifi_cred)
{
    esp_err_t error = ESP_OK;
    ESP_ERROR_CHECK(nvs_set_str(wifi_ap_nvs_handle, nvs_wifi_cred, wifi_cred));
    return error;
}

size_t get_wifi_detail(char *wifi_cred)
{
    size_t length = 0;
    esp_err_t error = nvs_get_str(wifi_ap_nvs_handle, nvs_wifi_cred, NULL, &length);
    if (ESP_ERR_NVS_NOT_FOUND == error)
    {
        cJSON *cj_empty_cred = cJSON_CreateObject();
        cJSON_AddBoolToObject(cj_empty_cred, "is_wifi_cred", false);
        nvs_set_str(wifi_ap_nvs_handle, nvs_wifi_cred, cJSON_Print(cj_empty_cred));
    }
    else
    {
        wifi_details = (char*)malloc(sizeof(char) * 256);
        error = nvs_get_str(wifi_ap_nvs_handle, nvs_wifi_cred, wifi_details, &length);
    }
    strncpy(wifi_cred, wifi_details, length);
    return length;
}