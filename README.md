# ESP32 MJPEG Multiclient Streaming Server

This is a simple MJPEG streaming webserver implemented for AI-Thinker ESP32-CAM or ESP-EYE modules. 

This is tested to work with **VLC** and **Blynk** video widget. 



**This version uses FreeRTOS tasks to enable streaming to up to 10 connected clients**

## Supported Camera Models

- AI-Thinker ESP32-CAM (default)
- ESP-EYE
- M5STACK PSRAM
- M5STACK WIDE
- WROVER KIT
- GOOUUU ESP32-S3-CAM

## 关于PSRAM的设置说明

### ESP32-S3 PSRAM配置

ESP32-S3系列开发板通常配备OPI PSRAM（8线接口），与ESP32的QSPI PSRAM（4线接口）不同。为确保PSRAM正常工作，请按照以下步骤操作：

1. **Arduino IDE设置**
   - 开发板选择: "ESP32S3 Dev Module"
   - PSRAM设置: "OPI PSRAM"（重要）
   - Flash模式: "QIO 80MHz"
   - Flash大小: 与您的开发板匹配（通常为"8MB"）

2. **识别PSRAM类型**
   - ESP32-S3模块通常标记为N8R8、N16R8等，其中"R8"表示8MB PSRAM
   - 若使用ESP32，则选择"QSPI PSRAM"选项

3. **开发板特殊注意事项**
   - GOOUUU ESP32-S3-CAM: 确保选择OPI PSRAM模式
   - AI-Thinker ESP32-CAM: 使用QSPI PSRAM模式

4. **故障排查**
   - 如果出现PSRAM初始化失败，尝试更新Arduino ESP32核心库到最新版本
   - 某些旧版本ESP32-S3开发板可能需要烧录特定eFuse配置

### PSRAM对相机性能的影响

PSRAM是高清视频流的关键组件：
- 有PSRAM: 可使用更高分辨率(VGA/SVGA)和更高帧率
- 无PSRAM: 需降低分辨率至QVGA以避免内存不足错误

### Arduino IDE中的PSRAM和Flash设置详解

要在Arduino IDE中正确配置ESP32/ESP32-S3的PSRAM和Flash设置，请按照以下步骤操作：

#### 1. 安装ESP32开发板支持

首先确保您已经安装了最新版本的ESP32开发板支持包：

1. 打开Arduino IDE
2. 点击 **文件 > 首选项**
3. 在"附加开发板管理器URL"中添加：`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
4. 点击 **工具 > 开发板 > 开发板管理器**
5. 搜索"esp32"并安装最新版本（推荐2.0.5或更高版本）

#### 2. ESP32-S3开发板设置

对于搭载有OPI PSRAM的ESP32-S3开发板（如GOOUUU ESP32-S3-CAM）：

1. 点击 **工具 > 开发板**，选择 `ESP32S3 Dev Module`
2. 在工具菜单中进行以下设置：
   - **USB CDC On Boot**: `Enabled`
   - **CPU Frequency**: `240MHz (WiFi)`
   - **USB DFU On Boot**: `Disabled`
   - **Flash Mode**: `QIO 80MHz`
   - **Flash Size**: `8MB (64Mb)`
   - **USB Mode**: `Hardware CDC and JTAG`
   - **PSRAM**: `OPI PSRAM` ← **关键设置**
   - **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)`
   - **Upload Speed**: `921600`

Arduino IDE中ESP32-S3开发板的PSRAM设置位置如下所示：

```
工具 → 开发板 → ESP32S3 Dev Module
     → PSRAM → OPI PSRAM
```

#### ESP32-S3的PSRAM模式选择

| ESP32-S3开发板类型 | 推荐PSRAM设置 | 说明 |
|--------------------|--------------|------|
| 带有8MB OPI PSRAM的版本 (如GOOUUU ESP32-S3-CAM) | OPI PSRAM | 大多数ESP32-S3-WROOM-1-N8R8模块 |
| 带有QSPI PSRAM的特殊版本 | QSPI PSRAM | 少数特殊型号，请查看开发板规格 |
| 无PSRAM的版本 | Disabled | 如ESP32-S3-WROOM-1-N8模块 |

#### Flash模式选择说明

| 设置项 | 建议值 | 说明 |
|--------|--------|------|
| Flash Mode | QIO 80MHz | 提供最佳性能，大多数模块支持 |
| Flash Mode | QIO 40MHz | 如果80MHz不稳定，可尝试此选项 |
| Flash Mode | DIO 40MHz | 兼容性最好，但性能较低 |

所有ESP32-S3开发板的Flash和PSRAM共享同一时钟，因此它们的频率设置需要匹配。

#### 3. ESP32开发板设置（AI-Thinker ESP32-CAM等）

对于搭载有QSPI PSRAM的原始ESP32开发板：

1. 点击 **工具 > 开发板**，选择 `ESP32 Dev Module` 或 `AI Thinker ESP32-CAM`
2. 在工具菜单中进行以下设置：
   - **CPU Frequency**: `240MHz (WiFi)`
   - **Flash Mode**: `QIO 80MHz`
   - **Flash Size**: `4MB (32Mb)`
   - **PSRAM**: `Enabled` ← **关键设置**
   - **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)`
   - **Upload Speed**: `921600`

#### 4. 验证PSRAM设置

编译并上传以下代码片段来验证PSRAM是否正确启用：

```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if(psramFound()) {
    Serial.printf("PSRAM 已启用! 大小: %d MB\n", ESP.getPsramSize() / 1024 / 1024);
  } else {
    Serial.println("未找到PSRAM");
  }
}

void loop() {
  delay(1000);
}
```

#### 5. 常见问题解决

- **如果确认开发板有PSRAM但检测不到**:
  
  对于ESP32-S3，尝试在Arduino IDE中更改PSRAM设置，切换到`OPI PSRAM`
  
- **如果无法编译或上传**:
  
  确认您使用的Arduino-ESP32核心版本与开发板兼容，ESP32-S3需要2.0.3或更高版本
  
- **上传问题**:
  
  上传前按住BOOT按钮，看到"Connecting..."时松开

- **Flash频率相关的启动问题**:
  
  如果开发板无法启动，尝试降低Flash频率到`40MHz`或`20MHz`

#### 6. ESP32-S3高级配置

##### 分区表配置

ESP32-S3可以使用不同的分区表配置，根据您的应用需求选择：

| 分区表选项 | 适用场景 | App空间 | 备注 |
|------------|----------|---------|------|
| Huge APP (3MB No OTA/1MB SPIFFS) | 图像流应用 | 3MB | 推荐用于相机应用 |
| 16M Flash (2MB APP/12.5MB FATFS) | 需要大存储 | 2MB | 当需要存储大量图片时 |
| Default | 一般应用 | 1.3MB | 支持OTA更新 |

##### 运行时优化建议

为获得ESP32-S3上最佳的相机流性能：

1. **内存优化**
   - 在代码中使用`MALLOC_CAP_SPIRAM`标志分配大型缓冲区
   - 例如：`uint8_t* buffer = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);`

2. **PSRAM使用**
   - 检查是否可用：`if (psramFound()) { ... }`
   - 对于图像处理，将主要数据结构放在PSRAM中
   - 相机帧缓冲区设置：`config.fb_location = CAMERA_FB_IN_PSRAM;`

3. **编译优化**
   - Arduino IDE中，选择`Optimize: Fastest (-O3)`
   - 如果使用PlatformIO，在配置文件中添加：
     ```
     build_flags = 
         -O3
         -DBOARD_HAS_PSRAM
     ```

##### 验证相机与PSRAM配合工作

以下代码可用于验证相机是否正确使用PSRAM：

```cpp
#include "esp_camera.h"
#include "esp_heap_caps.h"

// 初始化后检查内存使用情况
void printMemoryStats() {
  Serial.printf("总内存: %d字节\n", ESP.getHeapSize());
  Serial.printf("空闲内存: %d字节\n", ESP.getFreeHeap());
  
  if (psramFound()) {
    Serial.printf("PSRAM总计: %d字节\n", ESP.getPsramSize());
    Serial.printf("PSRAM可用: %d字节\n", ESP.getFreePsram());
    
    // 检查相机帧缓冲区位置
    size_t psram_used_by_fb = heap_caps_get_allocated_size(MALLOC_CAP_SPIRAM);
    Serial.printf("PSRAM用于帧缓冲区: %d字节\n", psram_used_by_fb);
  }
}
```

Inspired by and based on this Instructable: [$9 RTSP Video Streamer Using the ESP32-CAM Board](https://www.instructables.com/id/9-RTSP-Video-Streamer-Using-the-ESP32-CAM-Board/)

Full story: https://www.hackster.io/anatoli-arkhipenko/multi-client-mjpeg-streaming-from-esp32-47768f

------

## 结语

通过正确配置ESP32/ESP32-S3的PSRAM和Flash设置，您可以充分发挥开发板的性能，获得更高质量的视频流。请记住以下关键点：

1. ESP32-S3通常使用OPI PSRAM，而ESP32使用QSPI PSRAM
2. 根据开发板型号选择正确的PSRAM类型设置
3. 合理利用PSRAM来存储相机帧缓冲区和处理大型数据
4. 如果遇到启动问题，可能需要调整Flash频率或模式

希望本文档对您的ESP32相机项目有所帮助！

##### Other repositories that may be of interest

###### ESP32 MJPEG streaming server servicing a single client:

https://github.com/arkhipenko/esp32-cam-mjpeg



###### ESP32 MJPEG streaming server servicing multiple clients (FreeRTOS based):

https://github.com/arkhipenko/esp32-cam-mjpeg-multiclient



###### ESP32 MJPEG streaming server servicing multiple clients (FreeRTOS based) with the latest camera drivers from espressif.

https://github.com/arkhipenko/esp32-mjpeg-multiclient-espcam-drivers



###### Cooperative multitasking library:

https://github.com/arkhipenko/TaskScheduler

