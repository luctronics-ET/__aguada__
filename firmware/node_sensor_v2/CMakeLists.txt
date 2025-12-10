# CMakeLists.txt - AGUADA Node Sensor v2.0
cmake_minimum_required(VERSION 3.16)

# Include extra components from ./components directory
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(aguada_node_v2)
