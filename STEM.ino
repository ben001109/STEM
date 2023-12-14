#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// 函数声明
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void broadcast(const String &message);

// 定义LED和按键状态布尔值
bool buttonDown = false;
bool ledOn = false;

// 宏定义LED和按键引脚，根据自己的开发板原理图修改
#ifdef ESP32C3
  // 适配合宙esp32C3
  #define STATUS_LED 12
  #define STATUS_BUTTON 9
#elif ESP32
  // 适配esp32
  #define STATUS_LED 2
  #define STATUS_BUTTON 0
#endif

// 格式化MAC地址
// Formats MAC Address
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

//  接收到数据时的回调函数
// Called when data is received
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
  // 消息最长250个字符，加上一个空字符
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  //确保以空字符结尾
  // Make sure we are null terminated
  buffer[msgLen] = 0;

  // 格式化MAC地址
  // Format the MAC address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);

  // 发送调试日志消息到串口
  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);

  // Dapenson 以下便可添加数据处理或逻辑处理代码
  // 比较2个字符串是否相等，相等则返回0
  // 如果消息是“on”，则打开LED
  if (strcmp("on", buffer) == 0)
  {
    ledOn = true;
  }
  else
  {
    ledOn = false;
  }
  digitalWrite(STATUS_LED, ledOn);
}

// 消息发送后的回调函数，用以判断对方是否成功收到消息等
// Called when data is sent
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// 广播消息 ***
// Emulates a broadcast
void broadcast(const String &message)
{
  // 将消息广播到每个在范围内的设备
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  // 把broadcastAddress添加为消息对等体
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }

  // 发送消息给所有范围内的设备
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  // 将发送结果打印到串口
  // Print results to serial monitor
  Serial.printf(esp_err_to_name(result));
  Serial.println();
}

void setup()
{

  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");

  // 初始化espnow，如果失败则打印错误信息并退出重启
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    // 注册接受消息的回调函数
    esp_now_register_recv_cb(receiveCallback);
    // 注册消息发送的回调函数
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    // 重启esp设备
    ESP.restart();
  }

  pinMode(STATUS_LED, OUTPUT);
}
void loop()
{
  if (digitalRead(STATUS_BUTTON))
  {
    // 检测从低到高的转换
    // Detect the transition from low to high
    if (!buttonDown)
    {
      buttonDown = true;
      
      // 翻转LED状态
      // Toggle the LED state
      ledOn = !ledOn;
      digitalWrite(STATUS_LED, ledOn);
      
      // 发送消息给所有范围内的设备，用以同步LED状态
      // Send a message to all devices
      if (ledOn)
      {
        broadcast("on");
      }
      else
      {
        broadcast("off");
      }
    }
    
    // 延时以避免抖动
    // Delay to avoid bouncing
    delay(500);
  }
  else
  {
    // 重置按钮状态
    // Reset the button state
    buttonDown = false;
  }
}