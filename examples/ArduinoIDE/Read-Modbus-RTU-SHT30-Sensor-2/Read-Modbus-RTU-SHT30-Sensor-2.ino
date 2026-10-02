/*
  Reading a SHT30 Modbus RTU Temperature & Humidity Sensor

  This sketch shows you how to interact with a SHT30 Modbus RTU temperature & humidity
  sensor module. It reads the temperature & humidity values every 2 seconds
  and outputs them to the serial monitor.

  IMPORTANT:
  The 'TX-ENABLE' solder bridge on the YB-ESP32-S3-RS485 board must be set to 'AUTO' !
  GPIO21 is not used.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The SHT30 sensor module by default communicates with 9600Bd, 8 data bits, no parity
  and 1 stop bit.

  No Modbus Library is used here. All Modbus comm stuff is done 'by hand'.

  Last updated 2026-10-02, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>

// setting Modbus parameters for the SHT30 sensor module
const byte slaveAddr = 0x01;              // address of modbus slave device
const byte funcCode = 0x03;               // function code for reading holding registers
const byte addressHigh = 0x00;            // register starting address, high/low byte
const byte addressLow = 0x00;
const byte countHigh = 0x00;              // number of registers to read, high/low byte
const byte countLow = 0x02;

byte request[8], response[9];             // will keep transmit/receive data

uint16_t calculateCRC(byte *buffer, byte length) {
  uint16_t crc = 0xFFFF;
  for (byte i = 0; i < length; i++) {
    crc ^= buffer[i];
    for (byte j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void sendRequest() {
  while (Serial1.available()) Serial1.read();      // clean RS-485 input buffer before sending new request
  for (byte i = 0; i < sizeof(request); i++) {
    Serial1.write(request[i]);
  }
}

bool readResponse() {
  uint32_t start = millis();

  for (byte i = 0; i < sizeof(response);) {
    if (Serial1.available())
      response[i++] = Serial1.read();
    else
      if ((millis() - start) > 500) return false;  // response timeout
  }
  return true;
}

bool verifyCRC(byte *buffer, byte length) {
  uint16_t receivedCRC = (buffer[length - 1] << 8) | buffer[length - 2];

  return calculateCRC(buffer, length - 2) == receivedCRC;  // compare calculated & received CRC
}

void setup() {
  Serial.begin(115200);                                    // serial output
  Serial1.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD);   // RS-485 communication

  Serial.println("running example \"Reading SHT30 Modbus RTU Temperature/Humidity Sensor\":");

  request[0] = slaveAddr;
  request[1] = funcCode;
  request[2] = addressHigh;
  request[3] = addressLow;
  request[4] = countHigh;
  request[5] = countLow;

  uint16_t crc = calculateCRC(request, sizeof(request) - 2);
  request[6] = crc & 0xFF;                         // append calculated CRC, low byte
  request[7] = (crc >> 8) & 0xFF;                  // append calculated CRC, high byte  
}

void loop() {
  sendRequest();
  if (!readResponse()) {
    Serial.println("No response.");
  }
  else if (!verifyCRC(response, sizeof(response))) {
    Serial.println("CRC error.");
  }
  else if (request[0] != response[0]) {
    Serial.println("Address error.");
  }
  else if (request[1] != response[1]) {
    Serial.println("Function error.");
  }
  else {
    // valid response received
    uint16_t humidity = (response[3] << 8) | response[4],
             temperature = (response[5] << 8) | response[6];

    Serial.printf("Temperature: %.1f °C, Humidity: %.1f %%RH\n",
                  (float)(temperature / 10.0), (float)(humidity / 10.0));
  }

  delay(2000);
}

