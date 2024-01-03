#include <LoRa.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_TEMPLATE_ID "TMPL6oTC0xrt5"
#define BLYNK_DEVICE_NAME "Drowning Detecting"

#define ss 10
#define rst 9
#define dio0 7

const char* ssid = "你的WiFi名稱";
const char* password = "你的WiFi密碼";


BlynkTimer timer;

void setup() {
    Serial.begin(9600);
    LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(868E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    Serial.println("LoRa Initialized");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    Blynk.begin(BLYNK_TEMPLATE_ID, BLYNK_DEVICE_NAME, ssid, password);
}

void loop() {
    Blynk.run();
    timer.run();
    
    // 檢查是否有來自LoRa的數據
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String receivedData = "";
        while (LoRa.available()) {
            receivedData += (char)LoRa.read();
        }
        Serial.println("Received: " + receivedData);
        processReceivedData(receivedData);
    }
}

void processReceivedData(String data) {
    if (data.startsWith("Drowning Alert!")) {
        Serial.println("Drowning alert received!");
        Blynk.notify("Drowning alert!");
    } else {
        // 處理常規的位置數據
        int latIndex = data.indexOf("LAT:");
        int lonIndex = data.indexOf(",LONG:");
        if (latIndex != -1 && lonIndex != -1) {
            double lat = data.substring(latIndex + 4, lonIndex).toDouble();
            double lon = data.substring(lonIndex + 6).toDouble();
            Blynk.virtualWrite(V1, lat, lon, "GPS Location");
        }
    }
}
