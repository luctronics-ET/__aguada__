idf_component_register(
    SRCS 
        "main.cpp"
        "ultra.cpp"
        "packet.cpp"
        "wifi.cpp"
        "ios.cpp"
        "heartbeat.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino
)
