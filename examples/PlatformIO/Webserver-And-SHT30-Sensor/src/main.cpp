/*
  This example reads a SHT30 ModbusRTU Temperature & Humidity Sensor and presents
  the readings on a local web page.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The SHT30 sensor module by default communicates with 9600Bd, 8 data bits, no parity
  and 1 stop bit.

  Last updated 2026-11-03, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ModbusMaster.h>

//#define TX_ENABLE_GPIO21  // GPIO21 switches between RS-485 RX/TX

const char ssid[] = "MySSID";
const char pass[] = "MyPASSWORD";

ModbusMaster sht30;
WiFiServer server(80);

#ifdef TX_ENABLE_GPIO21
void preTransmission() {
  digitalWrite(RS485_TX_ENABLE, HIGH);
}
void postTransmission() {
  digitalWrite(RS485_TX_ENABLE, LOW);
}
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);                                   // serial output
  Serial1.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD);  // RS-485 communication
  Serial.println("running example \"SHT30 Sensor & Webserver\":");

  // connecting to local WiFi network
  Serial.printf("connecting to WiFi network \"%s\"\n", ssid);
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }
  Serial.printf("\nconnected successfully to \"%s\". IP address: %s\n",
                  ssid, WiFi.localIP().toString().c_str());
  digitalWrite(LED_BUILTIN, HIGH);

  server.begin();                             // starts the local webserver
  sht30.begin(1, Serial1);                    // slave ID=1, comm via Serial1

#ifdef TX_ENABLE_GPIO21
  pinMode(RS485_TX_ENABLE, OUTPUT);
  digitalWrite(RS485_TX_ENABLE, LOW);
  sht30.preTransmission(preTransmission);
  sht30.postTransmission(postTransmission);
#endif
}

void loop() {
  uint8_t result = 0;
  char str[70];
  float temp, humidity;

  result = sht30.readHoldingRegisters(0, 2);  // function code 0x03

  if (result == sht30.ku8MBSuccess) {
    temp = (float)(sht30.getResponseBuffer(1) / 10.0);
    humidity = (float)(sht30.getResponseBuffer(0) / 10.0);
    snprintf(str, sizeof(str), "Temperature: %.1f &deg;C, Relative Humidity: %.1f %%", temp, humidity);
    Serial.printf("Temp: %.1f °C, Humidity: %.1f %%\n", temp, humidity);
  }
  else {
    snprintf(str, sizeof(str), "Error: 0x%02X (%d)", result, result);
    Serial.println(str);
  }

  // listen for incoming clients
  WiFiClient client = server.accept();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // last incoming line was empty, now send response
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // meta tag makes browser refresh page every 5 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          // simple page
          client.println("<style>html {font-family: Helvetica; font-size: 30px; text-align: center;}</style>");
          client.print("Actual SHT30 Sensor Readings: ");
          client.println("<br />");
          client.print(str);
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // starts a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // character on actual line detected
          currentLineIsBlank = false;
        }
      }
    }
    delay(10);
    client.stop();
    Serial.println("client has disconnected");
  }

  delay(1000);
}
