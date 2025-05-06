# ESP-IDF项目的CMakeLists.txt文件
cmake_minimum_required(VERSION 3.16)

# 设置项目特定配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_camera_mjpeg_multiclient) 