

#include "esp_err.h"
#include "trace.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_spiffs.h"
#include "cJSON.h"

#include "nvs_component.h"
#include "ap_server_initializer.h"
#include "sta_initializer.h"
#include "uri_handler.h"

static const char *wifi_ap_ssid = "Captive WiFi SSID";
static const char *wifi_ap_pass = "pass12345";

static bool init_wifi_ap = false;
static bool init_wifi_sta = true;

static void wifi_cred_received_task(void *params)
{
    while (1)
    {
        if(init_wifi_sta)
        {
            esp_err_t error = wifi_sta_connect();
            if(ESP_OK == error)
            {
                break;
            }
            else 
            {
                init_wifi_sta = false;
                init_wifi_ap = true;
            }
        }        
        if (init_wifi_ap)
        {
            TRACE_B("Initializing WiFi AP.");
            configure_wifi_ap(wifi_ap_ssid, wifi_ap_pass);
            begin_ap_server();
            init_wifi_ap = false;
            init_wifi_sta = false;
        }
        if (is_wifi_cred_set())
        {
            deconfigure_wifi_ap();
            init_wifi_ap = false;
            init_wifi_sta = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    TRACE_B("Deleting task.");
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialize_nvs_partition();
    TRACE_B("NVS flash initialized.");

    xTaskCreate(wifi_cred_received_task, "wifi_cred_received_task", 2048 * 2, NULL, 0, NULL);
}
