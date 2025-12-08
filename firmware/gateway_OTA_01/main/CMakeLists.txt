idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash esp_system esp_driver_gpio esp_timer esp_http_client esp_https_ota app_update bootloader_support
)
