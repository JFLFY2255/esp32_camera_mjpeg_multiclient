#include "Arduino.h"
#include "esp_camera.h"
#include "OV2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

// ESP-IDF特定头文件
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
// 添加格式转换支持
#include "img_converters.h"

// 定义CPU核心
#define APP_CPU 1
#define PRO_CPU 0

// 选择摄像头模型
#define CAMERA_MODEL CAMERA_MODEL_GOOUUU_ESP32S3_CAM

// WiFi凭据
#include "home_wifi_multi.h"

// 日志标签
static const char* TAG = "ESP32_CAM";

OV2640 cam;
WebServer server(80);

// ===== RTOS任务句柄 =========================
TaskHandle_t tMjpeg;   // 处理到webserver的客户端连接
TaskHandle_t tCam;     // 处理从摄像头获取图片帧并本地存储
TaskHandle_t tStream;  // 实际向所有连接的客户端流式传输帧

// frameSync信号量用于防止在更换下一帧时流式传输缓冲区
SemaphoreHandle_t frameSync = NULL;

// 队列存储当前连接的正在流式传输的客户端
QueueHandle_t streamingClients;

// 我们将尝试实现25 FPS的帧率
const int FPS = 14;

// 我们将每50毫秒（20赫兹）处理一次web客户端请求
const int WSINTERVAL = 100;

// 常用变量：
volatile size_t camSize;    // 当前帧的大小，字节
volatile char* camBuf;      // 指向当前帧的指针

// 前向声明
void camCB(void* pvParameters);
void streamCB(void* pvParameters);
void handleJPGSstream(void);
void handleJPG(void);
void handleNotFound(void);

// ==== 内存分配器，如果存在PSRAM则利用它 =======================
char* allocateMemory(char* aPtr, size_t aSize) {
  //  由于当前缓冲区太小，请释放它
  if (aPtr != NULL) free(aPtr);

  size_t freeHeap = ESP.getFreeHeap();
  char* ptr = NULL;

  // 如果请求的内存超过当前可用堆的2/3，立即尝试PSRAM
  if (aSize > freeHeap * 2 / 3) {
    if (psramFound() && ESP.getFreePsram() > aSize) {
      ptr = (char*) ps_malloc(aSize);
    }
  }
  else {
    // 足够的空闲堆 - 让我们尝试分配快速RAM作为缓冲区
    ptr = (char*) malloc(aSize);

    // 如果堆上的分配失败，再给PSRAM一次机会：
    if (ptr == NULL && psramFound() && ESP.getFreePsram() > aSize) {
      ptr = (char*) ps_malloc(aSize);
    }
  }

  // 最后，如果内存指针为NULL，我们无法分配任何内存，这是一个终端条件。
  if (ptr == NULL) {
    ESP.restart();
  }
  return ptr;
}

// ======== 服务器连接处理任务 ==========================
void mjpegCB(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(WSINTERVAL);

  // 创建帧同步信号量并初始化它
  frameSync = xSemaphoreCreateBinary();
  xSemaphoreGive(frameSync);

  // 创建一个队列来跟踪所有连接的客户端
  streamingClients = xQueueCreate(10, sizeof(WiFiClient*));

  //=== 设置部分 ==================

  // 创建用于从摄像头抓取帧的RTOS任务
  xTaskCreatePinnedToCore(
    camCB,       // 回调
    "cam",       // 名称
    4096,        // 堆栈大小
    NULL,        // 参数
    2,           // 优先级
    &tCam,       // RTOS任务句柄
    APP_CPU);    // 核心

  // 创建任务将流推送到所有连接的客户端
  xTaskCreatePinnedToCore(
    streamCB,
    "strmCB",
    4 * 1024,
    NULL,
    2,
    &tStream,
    APP_CPU);

  // 注册webserver处理例程
  server.on("/mjpeg/1", HTTP_GET, handleJPGSstream);
  server.on("/jpg", HTTP_GET, handleJPG);
  server.onNotFound(handleNotFound);

  // 启动webserver
  server.begin();

  //=== 循环部分 ===================
  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    server.handleClient();

    // 在每个服务器客户端处理请求之后，我们让其他任务运行，然后暂停
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ==== RTOS任务从摄像头抓取帧 =========================
void camCB(void* pvParameters) {
  TickType_t xLastWakeTime;

  // 与当前所需帧率相关联的运行间隔
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / FPS);

  // 用于在活动帧周围切换关键部分的互斥锁
  portMUX_TYPE xSemaphore = portMUX_INITIALIZER_UNLOCKED;

  // 指向2个帧的指针，它们各自的大小和当前帧的索引
  char* fbs[2] = { NULL, NULL };
  size_t fSize[2] = { 0, 0 };
  int ifb = 0;

  //=== 循环部分 ===================
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // 从摄像头抓取一帧并查询其大小
    cam.run();
    size_t s = cam.getSize();

    // 如果帧大小比我们之前分配的更多 - 请求当前帧空间的125%
    if (s > fSize[ifb]) {
      fSize[ifb] = s * 4 / 3;
      fbs[ifb] = allocateMemory(fbs[ifb], fSize[ifb]);
    }

    // 将当前帧复制到本地缓冲区
    char* b = (char*)cam.getfb();
    memcpy(fbs[ifb], b, s);

    // 让其他任务运行并等到当前帧率间隔结束（如果有剩余时间）
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    // 仅当没有帧当前正在流式传输到客户端时才切换帧
    // 等待信号量直到客户端操作完成
    xSemaphoreTake(frameSync, portMAX_DELAY);

    // 在切换当前帧时不允许中断
    portENTER_CRITICAL(&xSemaphore);
    camBuf = fbs[ifb];
    camSize = s;
    ifb++;
    ifb &= 1;  // 这应该产生1, 0, 1, 0, 1 ...序列
    portEXIT_CRITICAL(&xSemaphore);

    // 让任何等待帧的人知道帧已准备好
    xSemaphoreGive(frameSync);

    // 技术上只需要一次：让流式传输任务知道我们至少有一个帧
    // 它可以开始向客户端发送帧（如果有的话）
    xTaskNotifyGive(tStream);

    // 立即让其他（流式传输）任务运行
    taskYIELD();

    // 如果流式传输任务已挂起自己（没有要流式传输的活动客户端）
    // 则无需从摄像头抓取帧。我们可以通过挂起任务来节省一些功耗
    if (eTaskGetState(tStream) == eSuspended) {
      vTaskSuspend(NULL);  // 传递NULL表示"挂起自己"
    }
  }
}

// ==== STREAMING ======================================================
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                    "Access-Control-Allow-Origin: *\r\n" \
                    "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

// ==== 处理来自客户端的连接请求 ===============================
void handleJPGSstream(void) {
  // 只能容纳10个客户端。限制是WiFi连接的默认值
  if (!uxQueueSpacesAvailable(streamingClients)) return;

  // 创建一个新的WiFi客户端对象来跟踪这个
  WiFiClient* client = new WiFiClient();
  *client = server.client();

  // 立即向此客户端发送标头
  client->write(HEADER, hdrLen);
  client->write(BOUNDARY, bdrLen);

  // 将客户端推到流队列
  xQueueSend(streamingClients, (void*)&client, 0);

  // 唤醒流任务，如果它们之前被挂起：
  if (eTaskGetState(tCam) == eSuspended) vTaskResume(tCam);
  if (eTaskGetState(tStream) == eSuspended) vTaskResume(tStream);
}

// ==== 实际向所有连接的客户端流式传输内容 ========================
void streamCB(void* pvParameters) {
  char buf[16];
  TickType_t xLastWakeTime;
  TickType_t xFrequency;

  // 等待捕获第一帧并有东西发送
  // 给客户端
  ulTaskNotifyTake(pdTRUE,         /* 退出前清除通知值。 */
                portMAX_DELAY);   /* 无限期阻塞。 */

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    // 默认假设我们按照FPS运行
    xFrequency = pdMS_TO_TICKS(1000 / FPS);

    // 只有当有人观看时才费心发送任何东西
    UBaseType_t activeClients = uxQueueMessagesWaiting(streamingClients);
    if (activeClients) {
      // 根据连接的客户端数量调整周期
      xFrequency /= activeClients;

      // 由于我们向每个人发送相同的帧，
      // 从队列前面弹出一个客户端
      WiFiClient* client;
      xQueueReceive(streamingClients, (void*)&client, 0);

      // 检查此客户端是否仍然连接。
      if (!client->connected()) {
        // 如果他/她已断开连接，请删除此客户端引用
        // 不要再把它放回队列了。再见！
        delete client;
      }
      else {
        // 好的。这是一个积极连接的客户端。
        // 让我们抓取一个信号量来防止帧变化，而我们
        // 正在提供这个帧
        xSemaphoreTake(frameSync, portMAX_DELAY);

        size_t jpgSize = 0;
        uint8_t* jpgBuf = NULL;
        
        // 检查是否需要进行格式转换
        camera_fb_t *fb = cam.getFrameBuffer();
        if (fb && fb->format != PIXFORMAT_JPEG && camSize > 0) {
          bool convert_ok = fmt2jpg((uint8_t*)camBuf, camSize, fb->width, fb->height, 
                               fb->format, 30, &jpgBuf, &jpgSize);
          
          if (convert_ok && jpgSize > 0) {
            // 发送转换后的JPEG数据
            client->write(CTNTTYPE, cntLen);
            sprintf(buf, "%u\r\n\r\n", jpgSize);
            client->write(buf, strlen(buf));
            client->write((char*)jpgBuf, jpgSize);
            
            ESP_LOGI(TAG, "格式转换为JPEG成功: %u -> %u bytes", camSize, jpgSize);
            
            // 释放转换后的JPEG缓冲区
            free(jpgBuf);
          } else {
            ESP_LOGE(TAG, "格式转换失败，丢弃图片");
          }
        } else {
          // 已经是JPEG格式或数据无效，直接发送
          client->write(CTNTTYPE, cntLen);
          sprintf(buf, "%u\r\n\r\n", camSize);
          client->write(buf, strlen(buf));
          client->write((char*)camBuf, (size_t)camSize);
        }
        
        client->write(BOUNDARY, bdrLen);

        // 由于此客户端仍然连接，请将其推到末尾
        // 队列以进行进一步处理
        xQueueSend(streamingClients, (void*)&client, 0);

        // 帧已提供。释放信号量并让其他任务运行。
        // 如果帧切换准备好了，现在将在帧之间发生
        xSemaphoreGive(frameSync);
        taskYIELD();
      }
    }
    else {
      // 由于没有连接的客户端，没有理由浪费电池运行
      vTaskSuspend(NULL);
    }
    // 在服务每个客户端后让其他任务运行
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                     "Content-disposition: inline; filename=capture.jpg\r\n" \
                     "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

// ==== 提供一个JPEG帧 =============================================
void handleJPG(void) {
  WiFiClient client = server.client();

  if (!client.connected()) return;
  cam.run();
  client.write(JHEADER, jhdLen);
  client.write((char*)cam.getfb(), cam.getSize());
}

// ==== 处理无效的URL请求 ============================================
void handleNotFound() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

// ESP-IDF应用程序入口点
extern "C" void app_main() {
  // 初始化Arduino库
  initArduino();
  
  // 设置串行连接：
  Serial.begin(115200);
  delay(1000); // 等待一秒钟，让Serial连接
  
  // 打印内存信息
  Serial.printf("开始前，可用内存: %lu bytes\n", ESP.getFreeHeap());
  if(psramFound()) {
    Serial.printf("PSRAM大小: %lu bytes, 可用: %lu bytes\n", 
                ESP.getPsramSize(), ESP.getFreePsram());
  }
  
  // 相机初始化前短暂延迟以稳定电源
  delay(500);
  
  // 直接从OV2640类获取相机配置
  camera_config_t config = cam.get_config(CAMERA_MODEL);
  
  // 检查PSRAM
  if(psramFound()) {
    Serial.println("PSRAM 已找到，使用PSRAM存储帧缓冲区");
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("未找到PSRAM，使用DRAM");
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  
  #if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  #endif
  
  if (cam.init(config) != ESP_OK) {
    Serial.println("相机初始化错误");
    delay(3000);
    ESP.restart();
  }
  Serial.println("相机初始化成功");
  
  // 配置并连接到WiFi
  IPAddress ip;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);
  Serial.print("正在连接到WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  ip = WiFi.localIP();
  Serial.println(F("WiFi已连接"));
  Serial.println("");
  Serial.print("流链接: http://");
  Serial.print(ip);
  Serial.println("/mjpeg/1");
  
  // 启动主流RTOS任务
  xTaskCreatePinnedToCore(
    mjpegCB,
    "mjpeg",
    4 * 1024,
    NULL,
    2,
    &tMjpeg,
    APP_CPU);
    
  // ESP-IDF主循环
  while(1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
} 