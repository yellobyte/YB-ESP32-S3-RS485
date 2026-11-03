/*
  DDS661 Modbus RTU Power Meter and Webserver

  This sketch reads a widely used Chinese DDS661 Modbus RTU power meter every
  5 secondes and presents the readings on a local web page.

  Solder bridge 'TX-ENABLE' on the YB-ESP32-S3-RS485 board is set to 'AUTO' by
  default. If you want to use GPIO21 for switching between RS-485 transmit/receive
  you need to change the solder bridge to 'GPIO21' and uncomment code line 24
  '#define TX_ENABLE_GPIO21'.

  'Serial' is used for serial/debug output, 'Serial1' for RS-485 communication.

  The DDS661 power meter by default communicates with 9600Bd, 8 data bits,
  even parity and 1 stop bit.

  Last updated 2026-11-03, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ModbusMaster.h>

//#define TX_ENABLE_GPIO21  // GPIO21 switches between RS-485 RX/TX

const char ssid[] = "MySSID";
const char pass[] = "MyPASSWORD";

ModbusMaster node;
WiFiServer server(80);
String voltage, current, frequency, powerFactor, power, energy;

#ifdef TX_ENABLE_GPIO21
void preTransmission() {
  digitalWrite(RS485_TX_ENABLE, HIGH);
}
void postTransmission() {
  digitalWrite(RS485_TX_ENABLE, LOW);
}
#endif

bool readDDS661(char *what, int address, char *unit, String *value) {
  uint16_t rcvd[2];
  byte     converted[4];
  char     buf[10];
  uint8_t  result = node.readInputRegisters(address, 2);

  if (result == node.ku8MBSuccess) {
    rcvd[0] = node.getResponseBuffer(0);
    rcvd[1] = node.getResponseBuffer(1);

    converted[0] = rcvd[1] & 0xFF;
    converted[1] = rcvd[1] >> 8;
    converted[2] = rcvd[0] & 0xFF;
    converted[3] = rcvd[0] >> 8;

    snprintf(buf, sizeof(buf), "%.2f", *(float *)converted);
    *value = buf;

    Serial.printf("%s: raw data: 0x%04.4x 0x%04.4x --> value: %s %s\n",
                  what, rcvd[0], rcvd[1], value->c_str(), unit);
    return true;
  }

  Serial.printf("%s: error reading value from meter: 0x%02x (%d)\n", what, result, result);
  *value = "";

  return false;
}

void backgroundTask(void *parameter) {
  while(true) {
    readDDS661("Voltage", 0x0000, "V", &voltage);
    delay(50);
    readDDS661("Current", 0x0008, "A", &current);
    delay(50);
    readDDS661("Frequency", 0x0036, "Hz", &frequency);
    delay(50);
    readDDS661("Power factor", 0x002A, "cos φ", &powerFactor);
    delay(50);
    readDDS661("Power", 0x0012, "W", &power);
    delay(50);
    readDDS661("Energy", 0x0100, "kWh", &energy);
    Serial.println();
    delay(5000);
  }
  vTaskDelete(NULL); // will never get here
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);                                   // serial output
  Serial1.begin(9600, SERIAL_8E1, RS485_RXD, RS485_TXD);  // RS-485 communication
  Serial.println("running example \"DDS661 Modbus RTU Power Meter & Webserver\":");

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

  server.begin();                                        // starts local webserver
  node.begin(1, Serial1);                                // slave ID=1, comm via Serial1
#ifdef TX_ENABLE_GPIO21
  pinMode(RS485_TX_ENABLE, OUTPUT);
  postTransmission();
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
#endif

  xTaskCreate(backgroundTask, "bgTask", 4096, NULL, 1, NULL);
}

void loop() {
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
          // meta tag requests browser to refresh page every 5 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          // simple page
          client.println("<style>html {font-family: Helvetica; font-size: 40px; text-align: center;}</style>");
          client.print("<h3>Actual DDS661 Power Meter Readings:</h3>");
          client.printf("Voltage: %s V<br>", voltage.c_str());
          client.printf("Current: %s A<br>", current.c_str());
          client.printf("Frequency: %s Hz<br>", frequency.c_str());
          client.printf("Power Factor: %s cos &phi;<br>", powerFactor.c_str());
          client.printf("Power: %s W<br>", power.c_str());
          client.printf("Energy: %s kWh<br>", energy.c_str());

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

  delay(500);
}


