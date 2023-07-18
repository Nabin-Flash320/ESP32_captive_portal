

idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES trace nvs_flash json esp_http_server nvs_component spiffs ap_server_initializer sta_initializer uri_handler)

spiffs_create_partition_image(storage ../content FLASH_IN_PROJECT)