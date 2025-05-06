#include "OV2640.h"

#define TAG "OV2640"

// 相机配置函数 - 返回特定型号的配置
camera_config_t OV2640::get_config(CameraModel model) {
    camera_config_t config;
    
    // 初始化所有属性的通用默认值
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12; // 0-63，数字越小质量越高
    config.fb_count = 2;      // 如果超过1个，i2s以连续模式运行。仅用于jpeg
    
    // 根据不同型号配置特定的引脚
    switch (model) {
        case CAMERA_MODEL_ESP32CAM_DEVBOARD:
            // 适用于ESP32-CAM开发板（和大多数克隆版）的定义
            config.pin_pwdn = -1;
            config.pin_reset = 15;
            config.pin_xclk = 27;
            config.pin_sscb_sda = 25;
            config.pin_sscb_scl = 23;
            config.pin_d7 = 19;
            config.pin_d6 = 36;
            config.pin_d5 = 18;
            config.pin_d4 = 39;
            config.pin_d3 = 5;
            config.pin_d2 = 34;
            config.pin_d1 = 35;
            config.pin_d0 = 17;
            config.pin_vsync = 22;
            config.pin_href = 26;
            config.pin_pclk = 21;
            break;
        
        case CAMERA_MODEL_AI_THINKER:
            config.pin_pwdn = 32;
            config.pin_reset = -1;
            config.pin_xclk = 0;
            config.pin_sscb_sda = 26;
            config.pin_sscb_scl = 27;
            config.pin_d7 = 35;
            config.pin_d6 = 34;
            config.pin_d5 = 39;
            config.pin_d4 = 36;
            config.pin_d3 = 21;
            config.pin_d2 = 19;
            config.pin_d1 = 18;
            config.pin_d0 = 5;
            config.pin_vsync = 25;
            config.pin_href = 23;
            config.pin_pclk = 22;
            config.ledc_timer = LEDC_TIMER_1;
            config.ledc_channel = LEDC_CHANNEL_1;
            break;
        
        case CAMERA_MODEL_TTGO_TCAMERA:
            config.pin_pwdn = 26;
            config.pin_reset = -1;
            config.pin_xclk = 32;
            config.pin_sscb_sda = 13;
            config.pin_sscb_scl = 12;
            config.pin_d7 = 39;
            config.pin_d6 = 36;
            config.pin_d5 = 23;
            config.pin_d4 = 18;
            config.pin_d3 = 15;
            config.pin_d2 = 4;
            config.pin_d1 = 14;
            config.pin_d0 = 5;
            config.pin_vsync = 27;
            config.pin_href = 25;
            config.pin_pclk = 19;
            break;
        
        case CAMERA_MODEL_GOOUUU_ESP32S3_CAM:
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 15;
            config.pin_sscb_sda = 4;
            config.pin_sscb_scl = 5;
            config.pin_d7 = 16;
            config.pin_d6 = 17;
            config.pin_d5 = 18;
            config.pin_d4 = 12;
            config.pin_d3 = 10;
            config.pin_d2 = 8;
            config.pin_d1 = 9;
            config.pin_d0 = 11;
            config.pin_vsync = 6;
            config.pin_href = 7;
            config.pin_pclk = 13;
            break;

        case CAMERA_MODEL_WROVER_KIT:
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 21;
            config.pin_sscb_sda = 26;
            config.pin_sscb_scl = 27;
            config.pin_d7 = 35;
            config.pin_d6 = 34;
            config.pin_d5 = 39;
            config.pin_d4 = 36;
            config.pin_d3 = 19;
            config.pin_d2 = 18;
            config.pin_d1 = 5;
            config.pin_d0 = 4;
            config.pin_vsync = 25;
            config.pin_href = 23;
            config.pin_pclk = 22;
            break;

        case CAMERA_MODEL_ESP_EYE:
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 4;
            config.pin_sscb_sda = 18;
            config.pin_sscb_scl = 23;
            config.pin_d7 = 36;
            config.pin_d6 = 37;
            config.pin_d5 = 38;
            config.pin_d4 = 39;
            config.pin_d3 = 35;
            config.pin_d2 = 14;
            config.pin_d1 = 13;
            config.pin_d0 = 34;
            config.pin_vsync = 5;
            config.pin_href = 27;
            config.pin_pclk = 25;
            break;

        case CAMERA_MODEL_M5STACK_PSRAM:
            config.pin_pwdn = -1;
            config.pin_reset = 15;
            config.pin_xclk = 27;
            config.pin_sscb_sda = 25;
            config.pin_sscb_scl = 23;
            config.pin_d7 = 19;
            config.pin_d6 = 36;
            config.pin_d5 = 18;
            config.pin_d4 = 39;
            config.pin_d3 = 5;
            config.pin_d2 = 34;
            config.pin_d1 = 35;
            config.pin_d0 = 32;
            config.pin_vsync = 22;
            config.pin_href = 26;
            config.pin_pclk = 21;
            break;

        case CAMERA_MODEL_M5STACK_WIDE:
            config.pin_pwdn = -1;
            config.pin_reset = 15;
            config.pin_xclk = 27;
            config.pin_sscb_sda = 22;
            config.pin_sscb_scl = 23;
            config.pin_d7 = 19;
            config.pin_d6 = 36;
            config.pin_d5 = 18;
            config.pin_d4 = 39;
            config.pin_d3 = 5;
            config.pin_d2 = 34;
            config.pin_d1 = 35;
            config.pin_d0 = 32;
            config.pin_vsync = 25;
            config.pin_href = 26;
            config.pin_pclk = 21;
            break;
    }
    
    return config;
}

void OV2640::run(void)
{
    if (fb)
        //return the frame buffer back to the driver for reuse
        esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();
}

void OV2640::runIfNeeded(void)
{
    if (!fb)
        run();
}

int OV2640::getWidth(void)
{
    runIfNeeded();
    return fb->width;
}

int OV2640::getHeight(void)
{
    runIfNeeded();
    return fb->height;
}

size_t OV2640::getSize(void)
{
    runIfNeeded();
    if (!fb)
        return 0; // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?
    return fb->len;
}

uint8_t *OV2640::getfb(void)
{
    runIfNeeded();
    if (!fb)
        return NULL; // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?

    return fb->buf;
}

framesize_t OV2640::getFrameSize(void)
{
    return _cam_config.frame_size;
}

void OV2640::setFrameSize(framesize_t size)
{
    _cam_config.frame_size = size;
}

pixformat_t OV2640::getPixelFormat(void)
{
    return _cam_config.pixel_format;
}

void OV2640::setPixelFormat(pixformat_t format)
{
    switch (format)
    {
    case PIXFORMAT_RGB565:
    case PIXFORMAT_YUV422:
    case PIXFORMAT_GRAYSCALE:
    case PIXFORMAT_JPEG:
        _cam_config.pixel_format = format;
        break;
    default:
        _cam_config.pixel_format = PIXFORMAT_GRAYSCALE;
        break;
    }
}

esp_err_t OV2640::init(camera_config_t config)
{
    memset(&_cam_config, 0, sizeof(_cam_config));
    memcpy(&_cam_config, &config, sizeof(config));

    esp_err_t err = esp_camera_init(&_cam_config);
    if (err != ESP_OK)
    {
        printf("Camera probe failed with error 0x%x", err);
        return err;
    }
    // ESP_ERROR_CHECK(gpio_install_isr_service(0));

    return ESP_OK;
}
