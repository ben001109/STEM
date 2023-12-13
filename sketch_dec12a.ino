#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <TinyGPSPlus.h>

// 環境建置(定義函數)
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void broadcast(const String &message);

// 環境建置(代碼庫關聯(GPS))
TinyGPSPlus gps;

// 定義LED及按鈕開關狀態
bool buttonDown = false;
bool ledOn = false;

// 定義LED和按鍵引脚，根据自己的開發版原理圖修改
#ifdef ESP32
  // 配合esp32
  #define STATUS_LED 2
  #define STATUS_BUTTON 0
#endif

// 格式化MAC地址
// Formats MAC Address
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

//  觸發訊息接收函數
// Called when data is received
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
  // 訊息上限250字符+1空字符結尾
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  // 確保以空字符结尾
  // Make sure we are null terminated
  buffer[msgLen] = 0;

  // 格式化MAC位址
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

// 檢測是否收到信息(函數)
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

// 模擬廣播信號 ***
// Emulates a broadcast
void broadcast(const String &message)
{
  // 發送廣播信號到範圍內的ESP32
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  // 把broadcastAddress新增為廣播信號(定義觸發用位址)
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }

  // 發送訊號給範圍內的ESP32
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  // 將發送結果回傳至序列埠
  // Print results to serial monitor
  Serial.printf(esp_err_to_name(result));
  Serial.println();
}

void setup()
{
  //Serial(輸出顯示),Serial2(GPS模組資料I/O)
  Serial.begin(115200);
  Serial2.begin(115200);

  delay(1000);
  //WiFi初始化設定
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");

  // 初始化espnow，如果失敗則報錯並重啟
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    // 定義接收函數
    esp_now_register_recv_cb(receiveCallback);
    // 定義發送函數
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    // 重啟ESP
    ESP.restart();
  }

  pinMode(STATUS_LED, OUTPUT);
}

//設定GPS模組資料交互
void updateSerial()
{
  delay(500);

  while (Serial.available()) {
    Serial2.write(Serial.read()); // 回傳GPS模組資料
  }
  while (Serial2.available()) {
    Serial.write(Serial2.read()); // 回傳Console資料
  }
}

//顯示目前GPS迴傳資料
void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid()){
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
}

void loop()
{
    //updateSerial();
  while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
      displayInfo();
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
  if (digitalRead(STATUS_BUTTON))
  {
    // 檢測電位高低(按鈕動作)
    // Detect the transition from low to high
    if (!buttonDown)
    {
      buttonDown = true;

      // 切換LED狀態
      // Toggle the LED state
      ledOn = !ledOn;
      digitalWrite(STATUS_LED, ledOn);

      // 同步LED狀態
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

    // 加延遲避免高低電位變化影響信號控制
    // Delay to avoid bouncing
    delay(500);
  }
  else
  {
    // 重設按鈕狀態
    // Reset the button state
    buttonDown = false;
  }
}