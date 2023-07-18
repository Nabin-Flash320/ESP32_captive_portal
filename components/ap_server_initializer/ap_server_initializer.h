


#ifndef __AP_SERVER_INITIALIZER_H__
#define __AP_SERVER_INITIALIZER_H__

#include "esp_event.h"

void configure_wifi_ap(const char *wifi_ap_ssid, const char *wifi_ap_pass);
void deconfigure_wifi_ap();

#endif // __AP_SERVER_INITIALIZER_H__

