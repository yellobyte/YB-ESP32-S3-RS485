/*
  Test-Reading DDS661 Modbus RTU Power Meter

  This sketch reads a widely used Chinese DDS661 Modbus RTU power meter.
  It reads the actual values for voltage, current, frequency, power & energy
  every 5 seconds from the meter and outputs them to the serial monitor.

  If the 'TX-ENABLE' solder bridge on the YB-ESP32-S3-RS485 board is set to 'AUTO'
  then functions preTransmission()/postTransmission() are not used. In that case
  you can uncomment '#define TX_ENABLE_GPIO21'.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The DDS661 power meter by default communicates with 9600Bd, 8 data bits, 
  even parity and 1 stop bit.

  Last updated 2026-09-25, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <ModbusMaster.h>

//#define TX_ENABLE_GPIO21  // GPIO21 used to switch between RS-485 RX/TX

ModbusMaster node;

#ifdef TX_ENABLE_GPIO21
void preTransmission() {
  digitalWrite(RS485_TX_ENABLE, HIGH);
}
void postTransmission() {
  digitalWrite(RS485_TX_ENABLE, LOW);
}
#endif

bool readDDS661(int address, char *unit) {
  uint16_t rcvd[2];
  byte     converted[4];
  float    value;
  uint8_t  result = node.readInputRegisters(address, 2);

  if (result == node.ku8MBSuccess) {
    rcvd[0] = node.getResponseBuffer(0);
    rcvd[1] = node.getResponseBuffer(1);

    converted[0] = rcvd[1] & 0xFF;
    converted[1] = rcvd[1] >> 8;
    converted[2] = rcvd[0] & 0xFF;
    converted[3] = rcvd[0] >> 8;

    memcpy(&value, converted, 4);    

    Serial.printf("raw data: 0x%04.4x 0x%04.4x --> value: %.2f %s\n", 
                  rcvd[0], rcvd[1], value, unit);    
    return true;
  }

  Serial.printf("error reading value from meter: 0x%02x\n", result);
  return false;
}

void setup() {
  Serial.begin(115200);                                  // serial output
  Serial1.begin(9600, SERIAL_8E1, RS485_RXD, RS485_TXD); // RS-485 communication

  Serial.println("running example \"Reading DDS661 Modbus RTU Power Meter\":");
  
  node.begin(1, Serial1);                                // slave ID=1, comm via Serial1
#ifdef TX_ENABLE_GPIO21
  pinMode(RS485_TX_ENABLE, OUTPUT);
  postTransmission();
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
#endif
}

void loop() {
  Serial.print("Voltage: ");
  readDDS661(0x0000, "V");
  delay(50);

  Serial.print("Current: ");
  readDDS661(0x0008, "A");
  delay(50);

  Serial.print("Frequency: ");
  readDDS661(0x0036, "Hz");
  delay(50);

  Serial.print("Power factor: ");
  readDDS661(0x002A, "cos φ");
  delay(50);

  Serial.print("Power: ");
  readDDS661(0x0012, "W");
  delay(50);

  Serial.print("Energy: ");
  readDDS661(0x0100, "kWh");
  Serial.println();

  delay(5000);
}


