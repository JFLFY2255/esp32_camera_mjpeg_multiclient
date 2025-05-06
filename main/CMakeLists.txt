idf_component_register(
    SRCS 
        "main.cpp"
        "OV2640.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "arduino-esp32"
        "esp32-camera"
)

# 设置C++标准，与Arduino ESP32兼容
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=gnu++17") 