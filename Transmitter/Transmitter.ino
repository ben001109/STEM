#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <LoRa.h>

// 定義ESP32-WROOM-DA的LoRa連接引腳
#define ss 5    // 定義 SS 引腳為 GPIO5
#define rst 14  // 定義 RST 引腳為 GPIO14
#define dio0 2  // 定義 DIO0 引腳為 GPIO2

String receivedLat, receivedLon, receivedStatus;
WebServer server(80);

void setup() {
  Serial.begin(115200);
  // 設置ESP32為WiFi接入點
  WiFi.softAP("LoRa_Receiver", "12345678");
  Serial.println("WiFi AP Started");
  Serial.println(WiFi.softAPIP());

  // 初始化LoRa
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized...");

  // 設置Web服務器的路由
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, sendData);
  server.begin();
}

void loop() {
  // 處理LoRa接收
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    Serial.println("Received: " + receivedData);
    parseReceivedData(receivedData);
  }
  // 處理HTTP請求
  server.handleClient();
}

void parseReceivedData(String data) {
  // 解析從LoRa收到的數據
  int latIndex = data.indexOf("LAT:");
  int lonIndex = data.indexOf(",LONG:");
  int statusIndex = data.indexOf(",STATUS:");
 
  if (latIndex != -1 && lonIndex != -1 && statusIndex != -1) {
    receivedLat = data.substring(latIndex + 4, lonIndex);
    receivedLon = data.substring(lonIndex + 6, statusIndex);
    receivedStatus = data.substring(statusIndex + 8);
  }
}

void handleRoot() {
  // 生成並發送HTML頁面
  String html = R"html(
    <html>
        <head>
            // 省略的樣式和JavaScript代碼
        </head>
        <body onload="fetchData()">
            <h1>LoRa Geo Fencing</h1>
            <div class="data-box" id="latitude">Latitude: </div>
            <div class="data-box" id="longitude">Longitude: </div>
            <div class="data-box" id="status">Status: </div>
        </body>
    </html>
    )html";
 
  server.send(200, "text/html", html);
}

void sendData() {
  // 發送收到的LoRa數據作為JSON
  String payload = "{\"lat\":\"" + receivedLat + "\",\"lon\":\"" + receivedLon + "\",\"status\":\"" + receivedStatus + "\"}";
  server.send(200, "application/json", payload);
}
