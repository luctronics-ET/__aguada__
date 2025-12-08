# AGUADA v1.2 - Dual Sensor Node (IE01 + IE02)
# 2 sensores ultrassônicos em um ESP32-C3
# IE01: MAC real | IE02: MAC virtual (AA:BB:CC:DD:1E:02)

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(aguada_node21)
