#include <SPI.h>
#include <mcp2515.h>
#include "string.h"
MCP2515 mcp2515(10);
uint8_t messageCounter = 0x0;

int rpm = 0;
int speed_kmh = 0;
int temperatureCelsius = 0;

bool checkEngine = false;

struct can_frame canMsg;

void setup() {
    Serial.begin(115200);
    mcp2515.reset();
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();    
    delay(500);
    selfCheck();    
    
 }

void selfCheck() {
  int timer = millis();  
  if (timer < 2000) {
    speed_kmh = 260;
    rpm = 8500;
    sendSpeed();
    sendTaho();
    selfCheck();
  } else {
    speed_kmh = 0;
    rpm = 0;
    temperatureCelsius = 0;
    sendSpeed();
    sendTaho();
  }
} 

// calculate checksum method, counter required
uint8_t calculateChecksum(const uint8_t* data, size_t length, uint8_t counter) {
    uint8_t checksum = 0x0;
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        checksum = (checksum - ((byte >> 4) & 0xF)) & 0xF;
        checksum = (checksum - (byte & 0xF)) & 0xF;
    }
    checksum = (checksum - (counter & 0xF)) & 0xF;
    return checksum;
}

void sendSpeed() {
    uint16_t scaledSpeed = speed_kmh * 93.5; 
    canMsg.can_id = 0xC8;
    canMsg.can_dlc = 8;
    canMsg.data[0] = 0x00;
    canMsg.data[1] = 0x00;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = (scaledSpeed >> 8) & 0xFF;
    canMsg.data[5] = scaledSpeed & 0xFF;
    canMsg.data[6] = 0x00;
    uint8_t counter = messageCounter & 0xF;
    uint8_t checksum = calculateChecksum(canMsg.data, 7, counter);
    canMsg.data[7] = ((counter & 0xF) << 4) | (checksum & 0xF);
    messageCounter = (messageCounter + 1) & 0xF;
    mcp2515.sendMessage(&canMsg);
}

void sendTaho() {
  uint16_t temperatureValue = temperatureCelsius * 447.16;
  canMsg.can_id = 0x12C;
  canMsg.can_dlc = 8;
  canMsg.data[0] = (temperatureValue >> 8) & 0xFF;
  canMsg.data[1] = temperatureValue & 0xFF;
  canMsg.data[2] = 0x00;
  canMsg.data[3] = 0x00;
  canMsg.data[4] = (rpm >> 8) & 0xFF;
  canMsg.data[5] = rpm & 0xFF;
  canMsg.data[6] = checkEngine ? 0xD0 : 0x00;
  uint8_t counter = messageCounter & 0xF;
  uint8_t checksum = calculateChecksum(canMsg.data, 7, counter);
  canMsg.data[7] = ((counter & 0xF) << 4) | (checksum & 0xF);
  messageCounter = (messageCounter + 1) & 0xF;
  mcp2515.sendMessage(&canMsg);
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("R")) {
      rpm = command.substring(1).toInt();
    } else if (command.startsWith("S")) {
      float speedFloat = command.substring(1).toFloat(); 
      speed_kmh = round(speedFloat); 
    }
  }
  
  sendSpeed();
  sendTaho();
}
