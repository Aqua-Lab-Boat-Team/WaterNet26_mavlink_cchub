#include "Particle.h"
#if __has_include("C:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23Vehicle _mavlink/lib/SdFat/src/SdFat.h")
#include "C:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23Vehicle _mavlink/lib/SdFat/src/SdFat.h"
#elif __has_include("../lib/SdFat/src/SdFat.h")
#include "../lib/SdFat/src/SdFat.h"
#endif
#if __has_include("C:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23Vehicle _mavlink/lib/SdFat/src/sdios.h")
#include "C:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23Vehicle _mavlink/lib/SdFat/src/sdios.h"
#elif __has_include("../lib/SdFat/src/sdios.h")
#include "../lib/SdFat/src/sdios.h"
#endif

#include <math.h>
#include <string.h>

// SD card setup aligned with the main WaterNet vehicle firmware.
#define chipSelect D8
SdFat sd((SPIClass*)&SPI);
File logFile;
char logFilename[32];

bool sdReady = false;

SYSTEM_MODE(MANUAL);

void logByteStream(const uint8_t* data, uint16_t len, const char* direction) {
    if (!sdReady) {
        return;
    }

    if (!logFile.isOpen()) {
        logFile = sd.open(logFilename, O_RDWR | O_CREAT | O_AT_END);
    }

    if (logFile) {
        logFile.printf("[%lu][%s] ", millis(), direction);
        for (uint16_t i = 0; i < len; i++) {
            logFile.printf("%02X ", data[i]);
        }
        logFile.println();
        logFile.flush();
    }
}

void forwardUsbToWireless(uint8_t rxbyte) {
    Serial1.write(rxbyte);
   // logByteStream(&rxbyte, 1, "USB->WLS");
}

void forwardWirelessToUsb(uint8_t rxbyte) {
    Serial.write(rxbyte);
   // logByteStream(&rxbyte, 1, "WLS->USB");
}

void setup() {
    Serial.begin(115200);   // USB/Jetson serial path.
    Serial1.begin(115200);  // Wireless serial path.

    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%02d%02d%04d%02d%02d%02d",
        Time.month(), Time.day(), Time.year(), Time.hour(), Time.minute(), Time.second());
    strcpy(logFilename, "MAVBRIDGE_");
    strcat(logFilename, timestamp);
    strcat(logFilename, ".txt");

    if (sd.begin(chipSelect, SD_SCK_MHZ(8))) {
        sdReady = true;
        logFile = sd.open(logFilename, O_RDWR | O_CREAT | O_AT_END);
        if (logFile) {
            logFile.println("MAVLink Bridge Log Start");
            logFile.flush();
        }
    } else {
        Serial.println("MAVLink bridge: SD card init failed");
    }

    RGB.control(true);
    RGB.color(0, 0, 0);
}

void loop() {
    while (Serial.available()) {
        uint8_t rxbyte = Serial.read();
        forwardUsbToWireless(rxbyte);
    }

    while (Serial1.available()) {
        uint8_t rxbyte = Serial1.read();
        forwardWirelessToUsb(rxbyte);
    }
}