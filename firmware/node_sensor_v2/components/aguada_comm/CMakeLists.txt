idf_component_register(
    SRCS "aguada_comm.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash aguada_protocol
    PRIV_REQUIRES freertos
)

# Compiler warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror
)
