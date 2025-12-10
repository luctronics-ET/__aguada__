idf_component_register(
    SRCS "aguada_sensor.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer
    PRIV_REQUIRES freertos
)

# Compiler warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror
)
