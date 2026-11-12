#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <cmath>

// Arm dimensions from Martin's kinematics (in mm)
const float L1 = 130.0; // longueur premier bras (mm)
const float L2 = 120.0; // longueur deuxieme bras (mm)

struct Position {
  float x, y, z;
  float yaw, pitch, roll;
};

// Forward Kinematics calculation from Martin's code
Position calculateArmPosition(float alpha, float beta, float gamma, float phi) {
  Position pos;
  float alphaRad = alpha * DEG_TO_RAD;
  float betaRad = beta * DEG_TO_RAD;
  float gammaMinusBetaRad = (gamma - beta) * DEG_TO_RAD;

  pos.x = (L1 * sin(betaRad) + L2 * sin(gammaMinusBetaRad)) * cos(alphaRad);
  pos.y = (L1 * sin(betaRad) + L2 * sin(gammaMinusBetaRad)) * sin(alphaRad);
  pos.z = L1 * cos(betaRad) - L2 * cos(gammaMinusBetaRad);
  pos.yaw = alpha;
  pos.pitch = gamma - beta - 90.0;
  pos.roll = phi;
  return pos;
}

// 4 servos configuration
// GPIOs: 13 (Base), 14 (Shoulder), 27 (Elbow), 26 (Wrist)
const int NUM_SERVOS = 4;
const int servoPins[NUM_SERVOS] = {13, 14, 27, 26};
const char *servoNames[NUM_SERVOS] = {
    "Base / Alpha (GPIO 13)", "Shoulder / Beta (GPIO 14)",
    "Elbow / Gamma (GPIO 27)", "Wrist / Phi (GPIO 26)"};

Servo servos[NUM_SERVOS];
int servoPos[NUM_SERVOS] = {90, 90, 90, 90}; // Default 90 degrees

// ESP32 Access Point credentials
const char *ssid = "RobertCyro-ESP32";
const char *password = "12345678";

// Web server on port 80
WiFiServer server(80);

// Request tracking
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

const char HTML_HEAD_CSS[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; max-width: 380px; margin: 24px auto; padding: 0 16px; color: #111; }
    h2 { margin: 0 0 2px 0; font-size: 20px; }
    .sub { color: #666; font-size: 13px; margin: 0 0 16px 0; }
    .kin { padding: 12px 0; border-top: 1px solid #e5e5e5; border-bottom: 1px solid #e5e5e5; margin-bottom: 20px; }
    .row { display: flex; justify-content: space-between; margin: 4px 0; font-size: 14px; }
    .val { font-weight: 600; }
    .joint { margin: 16px 0; }
    .header { display: flex; justify-content: space-between; font-size: 14px; margin-bottom: 2px; }
    input[type=range] { width: 100%; margin: 6px 0; accent-color: #111; }
  </style>
</head>
<body>
  <h2>Robert Cyro</h2>
)rawliteral";

const char HTML_JS[] PROGMEM = R"rawliteral(
  <script>
    var pending = {};
    var busy = false;

    function moveServo(id, val) {
      document.getElementById('v' + id).innerText = val;
      pending[id] = val;
      sendNext();
    }

    function sendNext() {
      if (busy) return;
      var keys = Object.keys(pending);
      if (keys.length === 0) return;

      var id = keys[0];
      var val = pending[id];
      delete pending[id];

      busy = true;
      fetch('/?servo=' + id + '&value=' + val + '&')
        .then(function(r) { return r.json(); })
        .then(function(d) {
          document.getElementById('pos_x').innerText = d.x.toFixed(2);
          document.getElementById('pos_y').innerText = d.y.toFixed(2);
          document.getElementById('pos_z').innerText = d.z.toFixed(2);
          document.getElementById('pos_yaw').innerText = d.yaw.toFixed(1);
          document.getElementById('pos_pitch').innerText = d.pitch.toFixed(1);
          document.getElementById('pos_roll').innerText = d.roll.toFixed(1);
        })
        .catch(function() {})
        .finally(function() {
          busy = false;
          if (Object.keys(pending).length > 0) sendNext();
        });
    }
  </script>
</body>
</html>
)rawliteral";

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

        Position p = calculateArmPosition(servoPos[0], servoPos[1], servoPos[2],
                                          servoPos[3]);
        Serial.printf("%s -> %d deg | Arm: X=%.1f mm, Y=%.1f mm, Z=%.1f mm\n",
                      servoNames[servoNum - 1], angle, p.x, p.y, p.z);
      }
    }
  }
}

void sendKinematicsJson(WiFiClient &client) {
  Position p =
      calculateArmPosition(servoPos[0], servoPos[1], servoPos[2], servoPos[3]);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: application/json");
  client.println("Connection: close");
  client.println();
  client.printf("{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"yaw\":%.1f,\"pitch\":%.1f,"
                "\"roll\":%.1f}\n",
                p.x, p.y, p.z, p.yaw, p.pitch, p.roll);
}

// Sends HTML page on initial visit or page refresh
void sendHtmlPage(WiFiClient &client) {
  Position p =
      calculateArmPosition(servoPos[0], servoPos[1], servoPos[2], servoPos[3]);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: text/html");
  client.println("Connection: close");
  client.println();

  // static html head and CSS from Flash
  client.print(HTML_HEAD_CSS);

  // simple kinematics block
  client.println("<div class=\"kin\">");
  client.println("  <div class=\"row\">");
  client.printf(
      "    <div>X: <span id=\"pos_x\" class=\"val\">%.2f</span> mm</div>\n",
      p.x);
  client.printf(
      "    <div>Y: <span id=\"pos_y\" class=\"val\">%.2f</span> mm</div>\n",
      p.y);
  client.printf(
      "    <div>Z: <span id=\"pos_z\" class=\"val\">%.2f</span> mm</div>\n",
      p.z);
  client.println("  </div>");
  client.println("  <div class=\"row\" style=\"color: #666; font-size: 13px; "
                 "margin-top: 6px;\">");
  client.printf("    <div>Yaw: <span id=\"pos_yaw\">%.1f</span>&deg;</div>\n",
                p.yaw);
  client.printf(
      "    <div>Pitch: <span id=\"pos_pitch\">%.1f</span>&deg;</div>\n",
      p.pitch);
  client.printf("    <div>Roll: <span id=\"pos_roll\">%.1f</span>&deg;</div>\n",
                p.roll);
  client.println("  </div>");
  client.println("</div>");

  // Sliders for 4 servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    int id = i + 1;
    client.println("<div class=\"joint\">");
    client.printf("  <div class=\"header\"><span>%s</span><span><span "
                  "id=\"v%d\" class=\"val\">%d</span>&deg;</span></div>\n",
                  servoNames[i], id, servoPos[i]);
    client.printf("  <input type=\"range\" min=\"0\" max=\"180\" value=\"%d\" "
                  "oninput=\"moveServo(%d, this.value)\">\n",
                  servoPos[i], id);
    client.println("</div>");
  }

  // JavaScript from Flash
  client.print(HTML_JS);
}

void setup() {
  Serial.begin(115200);

  // Attach all 4 servos with MG90S pulse widths (500us - 2400us)
  for (int i = 0; i < NUM_SERVOS; i++) {
    ESP32PWM::allocateTimer(i);
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 500, 2400);
    servos[i].write(servoPos[i]);
  }

  // Start Wi-Fi Access Point
  Serial.print("Starting Access Point: ");
  Serial.println(ssid);
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.accept();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        if (header.length() < 1024) {
          header += c;
        }

        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (header.indexOf("GET /?servo=") >= 0) {
              handleServoRequest(header);
              sendKinematicsJson(client); // Fast, compact response
            } else {
              sendHtmlPage(client); // Initial full page load
            }
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
  }
}
