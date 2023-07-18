

#ifndef __URI_HANDLER_H__
#define __URI_HANDLER_H__

#include "esp_err.h"
#include "esp_http_server.h"

void begin_ap_server();
void stop_ap_server();
bool is_wifi_cred_set();

#endif // __URI_HANDLER_H__
