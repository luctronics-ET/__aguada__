idf_component_register(
    SRCS "aguada_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES json
    PRIV_REQUIRES esp_common
)

# Compiler warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror
)

# Version info
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    AGUADA_PROTOCOL_VERSION=2
)
