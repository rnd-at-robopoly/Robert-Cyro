
#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>

// 4 servos configuration
const int NUM_SERVOS = 4;
const int servoPins[NUM_SERVOS] = {13, 14, 27, 26};
const char *servoNames[NUM_SERVOS] = {
    "Servo 1 - Base (GPIO 13)", "Servo 2 - Shoulder (GPIO 14)",
    "Servo 3 - Elbow (GPIO 27)", "Servo 4 - Wrist (GPIO 26)"};

Servo servos[NUM_SERVOS];
int servoPos[NUM_SERVOS] = {90, 90, 90, 90}; // Default center position

// ESP32 creates its own Wi-Fi network
const char *ssid = "RobertCyro-ESP32";
const char *password = "12345678";

// set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// timeout handling
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void handleServoRequest(const String &reqHeader) {
  int lineEnd = reqHeader.indexOf('\n');
  String requestLine =
      (lineEnd >= 0) ? reqHeader.substring(0, lineEnd) : reqHeader;

  int servoIdxPos = requestLine.indexOf("servo=");
  int valuePos = requestLine.indexOf("value=");

  if (servoIdxPos >= 0 && valuePos >= 0) {
    int ampPos = requestLine.indexOf('&', servoIdxPos);
    int endValPos = requestLine.indexOf('&', valuePos);
    if (endValPos < 0) {
      endValPos = requestLine.indexOf(' ', valuePos);
    }

    if (ampPos > servoIdxPos && endValPos > valuePos) {
      int servoNum = requestLine.substring(servoIdxPos + 6, ampPos).toInt();
      int angle = requestLine.substring(valuePos + 6, endValPos).toInt();

      if (servoNum >= 1 && servoNum <= NUM_SERVOS) {
        angle = constrain(angle, 0, 180);
        servoPos[servoNum - 1] = angle;
        servos[servoNum - 1].write(angle);
        Serial.printf("%s -> %d deg\n", servoNames[servoNum - 1], angle);
      }
    }
  }
}

void sendHtmlPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, "
                 "initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<style>");
  client.println("body { text-align: center; font-family: 'Trebuchet MS', "
                 "Arial, sans-serif; margin: 20px auto; max-width: 400px; }");
  client.println(".card { border: 1px solid #ddd; border-radius: 8px; padding: "
                 "12px; margin: 12px 0; }");
  client.println(".slider { width: 100%; }");
  client.println("</style></head>");
  client.println("<body><h1>Robert Cyro - 4 Servos</h1>");

  // create one slider block per servo
  for (int i = 0; i < NUM_SERVOS; i++) {
    int id = i + 1;
    client.println("<div class=\"card\">");
    client.printf("<h3>%s</h3>\n", servoNames[i]);
    client.printf("<p>Position: <span id=\"servoPos%d\">%d</span>&deg;</p>\n",
                  id, servoPos[i]);
    client.printf("<input type=\"range\" min=\"0\" max=\"180\" "
                  "class=\"slider\" id=\"servoSlider%d\" value=\"%d\" "
                  "oninput=\"updateLabel(%d, this.value)\" "
                  "onchange=\"moveServo(%d, this.value)\"/>\n",
                  id, servoPos[i], id, id);
    client.println("</div>");
  }

  client.println("<script>");
  client.println("function updateLabel(id, val) { "
                 "document.getElementById('servoPos' + id).innerHTML = val; }");
  client.println("function moveServo(id, pos) { fetch('/?servo=' + id + "
                 "'&value=' + pos + '&'); }");
  client.println("</script>");
  client.println("</body></html>");
  client.println();
}

void setup() {
  Serial.begin(115200);

  // attach all 4 servos with MG90S pulse widths (500us =0deg, 2400us = 180 deg)
  for (int i = 0; i < NUM_SERVOS; i++) {
    ESP32PWM::allocateTimer(i);
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 500, 2400);
    servos[i].write(servoPos[i]);
  }

  // start wifi
  Serial.print("Starting Access Point: ");
  Serial.println(ssid);
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed!");
    while (true) {
      delay(1000);
    }
  }

  // print ip
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.accept(); // listen for incoming clients

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (header.length() < 2048) {
          header += c;
        }

        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (header.indexOf("GET /?servo=") >= 0) {
              handleServoRequest(header);
            }
            sendHtmlPage(client);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
