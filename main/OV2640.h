#ifndef OV2640_H_
#define OV2640_H_

#include "Arduino.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <stdio.h>

// 定义支持的相机模型枚举
enum CameraModel {
    CAMERA_MODEL_ESP32CAM_DEVBOARD,  // 默认ESP32-CAM
    CAMERA_MODEL_AI_THINKER,         // AI Thinker ESP32-CAM
    CAMERA_MODEL_TTGO_TCAMERA,       // TTGO T-Camera
    CAMERA_MODEL_GOOUUU_ESP32S3_CAM, // GOOUUU ESP32-S3-CAM
    CAMERA_MODEL_WROVER_KIT,         // ESP32 WROVER KIT
    CAMERA_MODEL_ESP_EYE,            // ESP-EYE
    CAMERA_MODEL_M5STACK_PSRAM,      // M5Stack Camera with PSRAM
    CAMERA_MODEL_M5STACK_WIDE        // M5Stack Camera Wide
};

class OV2640
{
public:
    OV2640(){
        fb = NULL;
    };
    ~OV2640(){
    };
    
    // 获取特定模型的相机配置
    camera_config_t get_config(CameraModel model);
    
    // 初始化相机
    esp_err_t init(camera_config_t config);
    
    // 获取新帧
    void run(void);
    
    // 获取帧信息
    size_t getSize(void);
    uint8_t *getfb(void);
    int getWidth(void);
    int getHeight(void);
    framesize_t getFrameSize(void);
    pixformat_t getPixelFormat(void);
    
    // 获取内部帧缓冲指针，用于释放等操作
    camera_fb_t* getFrameBuffer(void) { return fb; }

    // 设置帧属性
    void setFrameSize(framesize_t size);
    void setPixelFormat(pixformat_t format);

private:
    void runIfNeeded(); // 如果我们还没有帧，则抓取一帧

    // camera_framesize_t _frame_size;
    // camera_pixelformat_t _pixel_format;
    camera_config_t _cam_config;

    camera_fb_t *fb;
};

#endif //OV2640_H_ 