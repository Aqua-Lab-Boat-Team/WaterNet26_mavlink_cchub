#include "Particle.h"
#include "SdFat.h"
#include <vector>
#include <mutex>

// System configuration
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// SD card setup
#define chipSelect D8
SdFat sd((SPIClass*)&SPI1);
File logFile;
char logFilename[32];

// Serial communication timing
#define SERIAL1_IDLE_TIMEOUT_MS 5000
uint32_t lastSerial1RxTime = 0;

// LTE message queue
std::vector<String> pendingLteMessages;
std::mutex lteMessageMutex;


/**
 * @brief Configure XBee modules for bypass mode
 * Sends bypass mode characters to XBee modules with integrated microcontrollers
 */
void setupXBee() {
    Serial1.printf("\n");    // First character to set Bypass mode
    delay(20);               // Wait before sending next character
    Serial1.printf("B");     // Second character to set Bypass mode
    delay(20);
}

/**
 * @brief LTE data handler for incoming messages from water bots
 * @param event Event name (unused)
 * @param data Command string received from bot over LTE
 * 
 * Interrupt handler called anytime a message is received from a Bot/CCHUB over LTE.
 * Adds the message to the queue for processing in the main loop.
 */
void dataLTEHandler(const char *event, const char *data) {
    if (data == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(lteMessageMutex);
    pendingLteMessages.emplace_back(data);
}


void setup() {
    // Initialize serial ports
    Serial.begin(115200);   // USB serial
    Serial1.begin(115200);  // Wireless serial (XBee, UART, etc.)
    setupXBee();
    
    // Subscribe to LTE messages from bots
    Particle.subscribe("Bot1dat", dataLTEHandler);
    
    // Initialize SD card logging
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%02d%02d%04d%02d%02d%02d",
        Time.month(), Time.day(), Time.year(), Time.hour(), Time.minute(), Time.second());
    
    strcpy(logFilename, "BRIDGE_");
    strcat(logFilename, timestamp);
    strcat(logFilename, ".txt");

    if (sd.begin(chipSelect, SD_SCK_MHZ(8))) {
        logFile = sd.open(logFilename, O_RDWR | O_CREAT | O_AT_END);
        if (logFile) {
            logFile.println("Serial Bridge Log Start");
            logFile.flush();
        }
    }
    
    // Initialize LED
    RGB.control(true);
    RGB.color(0, 0, 0);
    
    lastSerial1RxTime = millis();
}
void loop() {
    // Forward messages from USB to wireless, or publish to LTE if wireless is idle
    String outboundData;
    while (Serial.available()) {
        uint8_t rxbyte = Serial.read();
        
        // Check if wireless link is idle
        if ((millis() - lastSerial1RxTime) > SERIAL1_IDLE_TIMEOUT_MS) {
            // Wireless is idle, buffer for LTE publish
            outboundData.concat((char)rxbyte);
            RGB.color(0, 250, 0);  // Green
        } else {
            // Wireless is active, forward directly
            Serial1.write(rxbyte);
            RGB.color(255, 0, 0);  // Red
        }
    }

    // Publish buffered data to LTE if any
    if (outboundData.length() > 0) {
        Particle.publish("CCHub", outboundData, PRIVATE);
    }
    
    RGB.color(0, 0, 0);  // LED off

    // Forward messages from wireless to USB
    while (Serial1.available()) {
        uint8_t rxbyte = Serial1.read();
        Serial.write(rxbyte);
        RGB.color(0, 255, 0);  // Green
        lastSerial1RxTime = millis();
    }

    // Process pending LTE messages
    {
        std::vector<String> messagesToSend;
        {
            std::lock_guard<std::mutex> lock(lteMessageMutex);
            if (!pendingLteMessages.empty()) {
                messagesToSend.swap(pendingLteMessages);
            }
        }

        for (const auto &message : messagesToSend) {
            if (message.length() > 0) {
                Serial.println(message);
            }
        }
    }
}