/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sta_initializer.h"
#include "nvs_component.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_MAXIMUM_RETRY 5
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *sta_netif = NULL;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";
static const char *is_wifi_cred = "is_wifi_cred";
static const char *sta_ssid_key = "SSID";
static const char *sta_pass_key = "PSWD";
static char *wifi_sta_ssid = NULL;
static char *wifi_sta_pass = NULL;
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_sta(const char *wifi_sta_ssid, const char *wifi_sta_pass)
{
    esp_err_t error = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();

    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, wifi_sta_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, wifi_sta_pass, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        error = ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        esp_netif_destroy_default_wifi(sta_netif);
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        error = ESP_FAIL;
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        error = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    return error;
}

esp_err_t wifi_sta_connect()
{
    esp_err_t ret = ESP_OK;

    char *wifi_cred = (char *)malloc(256);
    memset(wifi_cred, 0, 256);
    size_t length = get_wifi_detail(wifi_cred);
    // TRACE_B("Length is %d, buffer is %s", length, wifi_cred);
    if (length > 0)
    {
        cJSON *cj_wifi_cred = cJSON_ParseWithLength(wifi_cred, length);
        TRACE_B("%s", cJSON_Print(cj_wifi_cred));
        cJSON *o_item = cJSON_GetObjectItem(cj_wifi_cred, is_wifi_cred);
        if (true == o_item->valueint)
        {
            wifi_sta_ssid = (char *)malloc(256);
            wifi_sta_pass = (char *)malloc(256);
            if ((NULL != wifi_sta_ssid) && (NULL != wifi_sta_pass))
            {
                memset(wifi_sta_ssid, 0, 256);
                memset(wifi_sta_pass, 0, 256);

                o_item = cJSON_GetObjectItem(cj_wifi_cred, sta_ssid_key);
                wifi_sta_ssid = o_item->valuestring;
                o_item = cJSON_GetObjectItem(cj_wifi_cred, sta_pass_key);
                wifi_sta_pass = o_item->valuestring;

                TRACE_B("SSID: %s, Password: %s", wifi_sta_ssid, wifi_sta_pass);

                if (ESP_OK != wifi_init_sta(wifi_sta_ssid, wifi_sta_pass))
                {
                    TRACE_E("Wifi STA connection error.");
                    ret = ESP_FAIL;
                }
                else
                {
                    TRACE_B("Wifi STA successfully connected.");
                    ret = ESP_OK;
                }
                free(wifi_sta_ssid);
                free(wifi_sta_pass);
            }
            else
            {
                TRACE_E("Couldn't provice memory to SSID and password.");
                ret = ESP_FAIL;
            }
        }
        else
        {
            TRACE_E("Wifi credentials not available.");
            ret = ESP_FAIL;
        }
    }
    else
    {
        TRACE_B("Wifi credentials not available.")
        ret = ESP_FAIL;
    }
    free(wifi_cred);
    return ret;
}
