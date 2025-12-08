# AGUADA Gateway OTA v1.0
# ESP32 DevKit v1 - Gateway/Repetidor com OTA
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(aguada_gateway_ota)
