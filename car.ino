#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Motor control pins
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D5
#define ENB D6

// Ultrasonic sensor pins
#define TRIG D7
#define ECHO D8

ESP8266WebServer server(80);

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP8266 Car Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #282c36; color: white; padding: 20px; }
    h1 { margin-bottom: 10px; }
    .grid-container { display: grid; grid-template-columns: repeat(3, 100px); gap: 10px; justify-content: center; }
    button {
      width: 100px; height: 100px; font-size: 18px; color: white;
      background: #007bff; border: none; border-radius: 10px; cursor: pointer;
      transition: 0.1s; box-shadow: 2px 2px 8px rgba(0,0,0,0.2);
    }
    button:active { transform: scale(0.95); }
    .stop { background: #dc3545 !important; grid-column: span 3; }
    .speed-container, .info-box, .toggle-container { margin-top: 20px; }
    input[type="range"] { width: 80%; }
    .switch { position: relative; display: inline-block; width: 60px; height: 34px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
      background-color: #ccc; transition: 0.4s; border-radius: 34px;
    }
    .slider:before {
      position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px;
      background-color: white; transition: 0.4s; border-radius: 50%;
    }
    input:checked + .slider { background-color: #4CAF50; }
    input:checked + .slider:before { transform: translateX(26px); }
  </style>
  <script>
    function sendData(dir) { fetch(`/move?direction=${dir}`).catch(console.error); }
    function updateSpeed(v) { document.getElementById('speed-value').innerText = v; fetch(`/speed?value=${v}`).catch(console.error); }
    function updateDistance() {
      fetch('/getDistance').then(res => res.json()).then(data => {
        document.getElementById('distance-value').innerText = data.distance;
      }).catch(console.error);
    }
    function toggleAutoMode(el) {
      fetch(`/toggleAuto?state=${el.checked ? 1 : 0}`).catch(console.error);
    }
    setInterval(updateDistance, 200);
  </script>
</head>
<body>
  <h1>ESP8266 Car Control</h1>
  <div class="grid-container">
    <button onclick="sendData('forward')">Forward</button>
    <button onclick="sendData('left')">Left</button>
    <button onclick="sendData('right')">Right</button>
    <button onclick="sendData('backward')">Backward</button>
    <button class="stop" onclick="sendData('stop')">STOP</button>
  </div>
  <div class="speed-container">
    <label>Speed: <span id="speed-value">512</span></label><br>
    <input type="range" min="200" max="1023" value="512" id="speed" oninput="updateSpeed(this.value)">
  </div>
  <div class="info-box">Distance: <span id="distance-value">---</span> cm</div>
  <div class="toggle-container">
    <label>Auto Navigation:</label>
    <label class="switch">
      <input type="checkbox" id="auto-toggle" onchange="toggleAutoMode(this)">
      <span class="slider"></span>
    </label>
  </div>
</body>
</html>
)rawliteral";

int speedValue = 512;
unsigned long lastObstacleCheck = 0;
bool autoMode = true; // Enable automatic obstacle avoidance

void setup() {
  Serial.begin(115200);
  setupMotorPins();
  setupUltrasonicPins();

  WiFi.softAP("ESP_Car", "12345678");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/speed", handleSpeed);
  server.on("/getDistance", handleGetDistance);
  server.on("/toggleAuto", handleToggleAuto);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  if (autoMode && millis() - lastObstacleCheck > 100) {  
    lastObstacleCheck = millis();
    handleObstacleAvoidance();
  }
}

void handleRoot() { server.send(200, "text/html", MAIN_page); }
void handleSpeed() { speedValue = server.arg("value").toInt(); server.send(200, "text/plain", "Speed updated"); }
void handleMove() { moveCar(server.arg("direction")); server.send(200, "text/plain", "OK"); }
void handleGetDistance() {
  String jsonResponse = "{\"distance\": " + String(measureDistance()) + "}";
  server.send(200, "application/json", jsonResponse);
}
void handleToggleAuto() {
  autoMode = server.arg("state").toInt() == 1;
  server.send(200, "text/plain", autoMode ? "Auto mode ON" : "Auto mode OFF");
}

void moveCar(String direction) {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  if (direction == "left") { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); delay(150); stopCar(); }
  else if (direction == "right") { digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); delay(150); stopCar(); }
  else if (direction == "backward") { digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
  else if (direction == "forward") { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); }
  else stopCar();
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void handleObstacleAvoidance() {
  float distance = measureDistance();
  if (distance > 0 && distance < 15) { 
    stopCar();
    delay(200);

    moveCar("backward");
    delay(500);
    stopCar();

    float leftDist, rightDist;

    moveCar("left");
    delay(200);
    leftDist = measureDistance();

    moveCar("right");
    delay(400);
    rightDist = measureDistance();

    moveCar(leftDist > rightDist ? "left" : "right");
    delay(300);
    moveCar("forward");
  }
}

float measureDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  float duration = pulseIn(ECHO, HIGH, 20000); 
  float distance = (duration * 0.0343) / 2;
  return (distance > 2 && distance < 300) ? distance : 300;
}

void setupMotorPins() { pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT); }
void setupUltrasonicPins() { pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); }
