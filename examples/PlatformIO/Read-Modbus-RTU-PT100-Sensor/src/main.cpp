/*
  Reading a PT100 Modbus RTU Temperature Sensor

  This sketch shows you how to interact with a PT100 Modbus RTU temperature sensor
  transmitter module. It reads the temperature value every second and outputs it
  to the serial monitor.

  If the 'TX-ENABLE' solder bridge on the YB-ESP32-S3-RS485 board is set to 'AUTO'
  then functions preTransmission()/postTransmission() are not used. In that case
  you can uncomment '#define TX_ENABLE_GPIO21'.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The PT100 temperature sensor transmitter module by default communicates with
  9600Bd, 8 data bits, no parity and 1 stop bit.

  Last updated 2026-09-26, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <ModbusMaster.h>

//#define TX_ENABLE_GPIO21  // GPIO21 used to switch between RS-485 RX/TX

ModbusMaster pt100;

#ifdef TX_ENABLE_GPIO21
void preTransmission() {
  digitalWrite(RS485_TX_ENABLE, HIGH);
}
void postTransmission() {
  digitalWrite(RS485_TX_ENABLE, LOW);
}
#endif

void setup() {
  Serial.begin(115200);                                  // serial output
  Serial1.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD); // RS-485 communication

  Serial.println("running example \"Reading PT100 Modbus RTU Temperature Sensor\":");

  pt100.begin(1, Serial1);                               // slave ID=1, comm via Serial1
#ifdef TX_ENABLE_GPIO21
  pinMode(RS485_TX_ENABLE, OUTPUT);
  digitalWrite(RS485_TX_ENABLE, LOW);
  pt100.preTransmission(preTransmission);
  pt100.postTransmission(postTransmission);
#endif
}

void loop() {
  uint8_t result=0;
  uint16_t data[16];

  pt100.clearResponseBuffer();
  result = pt100.readHoldingRegisters(0, 1);             // function code 0x03

  if (result == pt100.ku8MBSuccess) {
    Serial.printf("Temp: %.1f °C\n", (float)(pt100.getResponseBuffer(0) / 10.0));
  }
  else {
    Serial.printf("Error: 0x%02X (%d)\n", result, result);
  }
  delay(1000);
}
