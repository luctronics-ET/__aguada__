idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES aguada_protocol aguada_sensor aguada_comm esp_timer
)
