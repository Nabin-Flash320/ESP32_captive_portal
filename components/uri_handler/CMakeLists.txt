


idf_component_register(SRCS "uri_handler.c"
                        INCLUDE_DIRS "."
                        REQUIRES trace esp_http_server nvs_flash json spiffs dns_hijacking nvs_component)

