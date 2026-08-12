/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23CCHub_mavlink/src/WaterNetMavlink_test.ino"
#include "Particle.h"
#include "SdFat.h"
#include "C:\Users\anuja\Desktop\Anuja_WorkSpace\AquaticRobot\WaterNet23-main\WaterNet23-main\WaterNet23CCHub_MAVLINK\mavlink_c_v2\common\mavlink.h"
#include "math.h"

// SD card setup (reuse from WaterNet23CCHub.ino)
void logMavlinkMessage(const mavlink_message_t* msg, const char* direction);
void setupXBee();
void MVL_Transmit_Message(mavlink_message_t* mvl_msg_ptr, Stream& port);
void handle_qgc_message(mavlink_message_t* msg);
void handle_wireless_message(mavlink_message_t* msg);
void setup();
void loop();
#line 7 "c:/Users/anuja/Desktop/Anuja_WorkSpace/AquaticRobot/WaterNet23-main/WaterNet23-main/WaterNet23CCHub_mavlink/src/WaterNetMavlink_test.ino"
#define chipSelect D8
SdFat sd((SPIClass*)&SPI1);
File logFile;
char logFilename[32];
SYSTEM_MODE(MANUAL);

// MAVLink setup
mavlink_message_t mvl_tx_message;
mavlink_message_t mvl_rx_message_usb;
mavlink_message_t mvl_rx_message_wireless;
mavlink_status_t mvl_rx_status_usb;
mavlink_status_t mvl_rx_status_wireless;

const uint8_t mvl_compid = 1;
const uint8_t mvl_sysid = 1;
const uint8_t mvl_chan = MAVLINK_COMM_1;

const uint32_t hb_interval = 1000;
uint32_t t_last_hb = 0;

// Exception handling state
unsigned long lastRequestTime = 0;
bool waitingForReply = false;
uint8_t lastSentMsgId = 0;
const unsigned long MAVLINK_REPLY_TIMEOUT = 1000; // 1 second

// Log a MAVLink message to SD card (raw bytes)
void logMavlinkMessage(const mavlink_message_t* msg, const char* direction) {
    if (!logFile.isOpen()) logFile.open(logFilename, O_RDWR | O_CREAT | O_AT_END);
    if (logFile) {
        uint8_t buffer[300];
        uint16_t len = mavlink_msg_to_send_buffer(buffer, msg);
        logFile.printf("[%lu][%s] ", millis(), direction);
        for (uint16_t i = 0; i < len; i++) {
            logFile.printf("%02X ", buffer[i]);
        }
        logFile.println();
        logFile.flush();
    }
}
void setupXBee(){           //Function to send the bypass characters to XBee modules that have the integrated microcontrollers - done on startup
    Serial1.printf("\n");    //First character to set Bypass mode
    delay(20);              //Wait some time before sending next character
    Serial1.printf("B");     //Second character to set Bypass mode
    delay(20);
}
// Transmit MAVLink message on a given serial port
void MVL_Transmit_Message(mavlink_message_t* mvl_msg_ptr, Stream& port) {
    uint8_t tx_byte_buffer[512] = {0};
    uint16_t tx_buflen = mavlink_msg_to_send_buffer(tx_byte_buffer, mvl_msg_ptr);
    port.write(tx_byte_buffer, tx_buflen);
}

// Handle incoming MAVLink messages from QGC (USB)
void handle_qgc_message(mavlink_message_t* msg) {
    MVL_Transmit_Message(msg, Serial1);
    logMavlinkMessage(msg, "USB->WLS");
    // Start waiting for reply if this is a message that expects a response
    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG || msg->msgid == MAVLINK_MSG_ID_PARAM_REQUEST_LIST) {
        waitingForReply = true;
        lastRequestTime = millis();
        lastSentMsgId = msg->msgid;
    }
}

// Handle incoming MAVLink messages from wireless (remote vehicle)
void handle_wireless_message(mavlink_message_t* msg) {
    MVL_Transmit_Message(msg, Serial);
    logMavlinkMessage(msg, "WLS->USB");
    // If this is a reply to our request, clear the waiting flag
    if (waitingForReply && (msg->msgid == MAVLINK_MSG_ID_COMMAND_ACK || msg->msgid == MAVLINK_MSG_ID_PARAM_VALUE)) {
        waitingForReply = false;
    }
}

void setup() {
    // Serial ports
    Serial.begin(115200);   // USB serial for QGC
    Serial1.begin(115200);  // Wireless serial (e.g., XBee, UART to remote B404)
    setupXBee();    
    // SD card initialization (reuse from WaterNet23CCHub.ino)
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%02d%02d%04d%02d%02d%02d",
        Time.month(), Time.day(), Time.year(), Time.hour(), Time.minute(), Time.second());
    strcpy(logFilename, "MAVBRIDGE_");
    strcat(logFilename, timestamp);
    strcat(logFilename, ".txt");

    if (!sd.begin(chipSelect, SD_SCK_MHZ(8))) {
        // SD card error handling (optional: blink LED, print error, etc.)
    } else {
        logFile = sd.open(logFilename, O_RDWR | O_CREAT | O_AT_END);
        if (logFile) {
            logFile.println("MAVLink Bridge Log Start");
            logFile.flush();
        }
    }
    RGB.control(true); // Take control of onboard LED
    RGB.color(0, 0, 0);
}
int i=0;
void loop() {
    // 1. Forward messages from QGC (USB) to wireless
    while (Serial.available()) {
        uint8_t rxbyte = Serial.read();
        Serial1.write(rxbyte); // Optional: echo received byte   for debugging
        //Serial1.write(i);
        //RGB.color(255, 0, 0); // Red
        RGB.color(255, 0, 0); // Red
        if (mavlink_parse_char(mvl_chan, rxbyte, &mvl_rx_message_usb, &mvl_rx_status_usb)) {
            i++;
           // handle_qgc_message(&mvl_rx_message_usb);
        }
    }
    RGB.color(255, 0, 0); //Off

    // 2. Forward messages from wireless to QGC (USB)
    while (Serial1.available()) {
        uint8_t rxbyte = Serial1.read();
        Serial.write(rxbyte); // Optional: echo received byte for debugging
        RGB.color(0, 255, 0); // Green
        /*if (mavlink_parse_char(mvl_chan, rxbyte, &mvl_rx_message_wireless, &mvl_rx_status_wireless)) {
            handle_wireless_message(&mvl_rx_message_wireless);
        }*/
    }

    // 3. Periodic heartbeat to QGC (optional, can also forward from vehicle)
    /*if ((millis() - t_last_hb) > hb_interval) {
        mavlink_heartbeat_t mvl_hb;
        mvl_hb.type = MAV_TYPE_SUBMARINE;
        mvl_hb.autopilot = MAV_AUTOPILOT_GENERIC;
        mvl_hb.system_status = MAV_STATE_ACTIVE;
        mvl_hb.base_mode = MAV_MODE_MANUAL_DISARMED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        mvl_hb.custom_mode = 0xABBA;
        mavlink_msg_heartbeat_encode_chan(mvl_sysid, mvl_compid, mvl_chan, &mvl_tx_message, &mvl_hb);
        MVL_Transmit_Message(&mvl_tx_message, Serial);
        logMavlinkMessage(&mvl_tx_message, "BRIDGE->USB");
        t_last_hb = millis();
    }*/

    // 4. Exception handling: check for reply timeout
    /*if (waitingForReply && (millis() - lastRequestTime > MAVLINK_REPLY_TIMEOUT)) {
        // Blink LED to indicate error
        RGB.color(255, 0, 0); // Red
        delay(100);
        RGB.color(0, 0, 0);
        delay(100);

        // Send STATUSTEXT to QGC
        mavlink_statustext_t statustext;
        statustext.severity = MAV_SEVERITY_ERROR;
        snprintf((char*)statustext.text, sizeof(statustext.text), "No reply from remote for msgid %d", lastSentMsgId);
        mavlink_msg_statustext_encode_chan(mvl_sysid, mvl_compid, mvl_chan, &mvl_tx_message, &statustext);
        MVL_Transmit_Message(&mvl_tx_message, Serial);
        logMavlinkMessage(&mvl_tx_message, "BRIDGE->USB");

        waitingForReply = false; // Reset flag so we don't repeat
    }*/
}