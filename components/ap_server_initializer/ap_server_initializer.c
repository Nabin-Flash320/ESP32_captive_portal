
#include "esp_wifi.h"
#include "esp_event.h"
#include "ap_server_initializer.h"
#include "trace.h"
#include "string.h"
#include "dns_hijacking.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "uri_handler.h"

static esp_netif_t *ap_netif = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void start_dns_server();

void configure_wifi_ap(const char *wifi_ap_ssid, const char *wifi_ap_pass)
{

    // Check the return type, may be important.
    ap_netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_init_configurations = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_configurations));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cred_configurations = {
        .ap = {
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_cred_configurations.ap.ssid, wifi_ap_ssid, sizeof(wifi_cred_configurations.ap.ssid));
    strncpy((char *)wifi_cred_configurations.ap.password, wifi_ap_pass, sizeof(wifi_cred_configurations.ap.password));

    TRACE_B("Setting wifi mode to AP.");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    TRACE_B("Setting wifi AP credential configurations.");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cred_configurations));
    TRACE_I("Wifi credentials set complete with channel: %d SSID: %s and PASS: %s", wifi_cred_configurations.ap.channel, wifi_cred_configurations.ap.ssid, wifi_cred_configurations.ap.password);

    TRACE_B("Starting wifi AP");
    ESP_ERROR_CHECK(esp_wifi_start());
    TRACE_B("Wifi AP started!!");
}

void deconfigure_wifi_ap()
{
    stop_ap_server();
    TRACE_E("HTTP server stopped.");
    dns_hijack_srv_stop();
    TRACE_E("DNS hijack server stopped.");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler));
    esp_netif_destroy_default_wifi(ap_netif);
    ESP_ERROR_CHECK(esp_wifi_stop());
    TRACE_E("Wifi stopped.");
    ESP_ERROR_CHECK(esp_wifi_deinit());
    TRACE_E("Wifi resources deinitialized.");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_AP_START:
    {
        TRACE_B("Wifi AP started.");
        TRACE_B("Starting DNS server.");
        start_dns_server();
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        TRACE_B("Station connected to AP.");
        TRACE_B("station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        TRACE_E("Station disconnected from AP");
        TRACE_E("station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STOP:
    {
        TRACE_E("Wifi AP stopped.");
        break;
    }
    default:
    {
        TRACE_E("Unknown event ID received.(event: %d)", event_id);
        break;
    }
    }
}

static void start_dns_server()
{
    ip4_addr_t resolve_ip;
    inet_pton(AF_INET, "192.168.4.1", &resolve_ip);

    if (dns_hijack_srv_start(resolve_ip) == ESP_OK)
    {
        TRACE_B("DNS hijack server started");
    }
    else
    {
        TRACE_E("DNS hijack server has not started");
    }
}
