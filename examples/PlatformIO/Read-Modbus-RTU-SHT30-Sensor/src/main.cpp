/*
  Reading a SHT30 Modbus RTU Temperature & Humidity Sensor

  This sketch shows you how to interact with a SHT30 Modbus RTU temperature & humidity
  sensor module. It reads the temperature & humidity values every 2 seconds and outputs
  them to the serial monitor.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The SHT30 sensor module by default communicates with 9600Bd, 8 data bits, no parity
  and 1 stop bit.

  Last updated 2026-09-30, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <ModbusMaster.h>

#define TX_ENABLE_GPIO21  // GPIO21 used to switch between RS-485 RX/TX

ModbusMaster sht30;

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

  Serial.println("running example \"Reading SHT30 Modbus RTU Temperature/Humidity Sensor\":");

  sht30.begin(1, Serial1);                               // slave ID=1, comm via Serial1
#ifdef TX_ENABLE_GPIO21
  pinMode(RS485_TX_ENABLE, OUTPUT);
  digitalWrite(RS485_TX_ENABLE, LOW);
  sht30.preTransmission(preTransmission);
  sht30.postTransmission(postTransmission);
#endif
}

void loop() {
  uint8_t result;

  result = sht30.readHoldingRegisters(0, 2);             // function code 0x03

  if (result == sht30.ku8MBSuccess) {
    Serial.printf("Temp: %.1f °C, Humidity: %.1f %%\n",
                  (float)(sht30.getResponseBuffer(1) / 10.0),
                  (float)(sht30.getResponseBuffer(0) / 10.0));
  }
  else {
    Serial.printf("Error: 0x%02X (%d)\n", result, result);
  }
  delay(2000);
}
