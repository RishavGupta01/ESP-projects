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
    body {
      font-family: Arial, sans-serif;
      background: rgba(24, 24, 24, 0.9);
      backdrop-filter: blur(10px);
      color: #f8f8f8;
      text-align: center;
      padding: 20px;
    }
    h2 { margin-bottom: 10px; }
    .mode-container {
      display: flex;
      justify-content: center;
      gap: 15px;
      margin-bottom: 20px;
      padding: 15px;
      background: rgba(34, 34, 34, 0.5);
      border-radius: 15px;
      box-shadow: 0 4px 15px rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(10px);
    }
    .mode-btn {
      padding: 12px 24px;
      border: none;
      border-radius: 25px;
      background: rgb(168, 33, 33);
      color: white;
      cursor: pointer;
      transition: 0.3s;
      font-weight: bold;
      backdrop-filter: blur(10px);
    }
    .mode-btn.active {
      background: rgba(80, 213, 84, 0.925);
      box-shadow: 0 0 15px rgba(91, 215, 95, 0.5);
    }
    .manual-control {
      display: none;
      margin-top: 20px;
    }
    .manual-control.active { display: block; }
    .control-grid {
      display: grid;
      grid-template-columns: 90px 90px 90px;
      grid-template-rows: 90px 90px 90px;
      grid-template-areas: ". forward ." "left . right" ". backward .";
      gap: 15px;
      justify-content: center;
      align-items: center;
      margin-top: 15px;
    }
    .control-btn {
      width: 90px;
      height: 90px;
      font-size: 16px;
      border-radius: 15px;
      background: rgba(255, 26, 26, 0.668);
      color: white;
      border: 2px solid rgba(255, 89, 89, 0.771);
      cursor: pointer;
      transition: 0.2s;
      backdrop-filter: blur(10px);
    }
    .control-btn:hover { background: rgba(4, 132, 243, 0.908); border: 2px solid rgba(89, 186, 255, 0.771);}
    .control-btn:active { transform: scale(0.9); }
    .forward { grid-area: forward; }
    .left    { grid-area: left; }
    .right   { grid-area: right; }
    .backward { grid-area: backward; }
    .stop-btn {
      margin-top: 30px;
      width: 140px;
      height: 140px;
      font-size: 24px;
      font-weight: bold;
      background: rgba(255, 68, 68, 0.914);
      color: white;
      border: 3px solid rgba(255, 119, 119, 0.421);
      border-radius: 30px;
      box-shadow: 0 0 30px rgba(255, 0, 0, 0.596);
      cursor: pointer;
      animation: pulse 1.5s infinite;
      backdrop-filter: blur(10px);
    }
    @keyframes pulse {
      0% { transform: scale(1); }
      50% { transform: scale(1.05); }
      100% { transform: scale(1); }
    }
    .speed-slider {
      margin-top: 15px;
      width: 80%;
      -webkit-appearance: none;
      height: 10px;
      background: rgba(68, 68, 68, 0.5);
      border-radius: 5px;
      backdrop-filter: blur(10px);
    }
    .speed-slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      background: rgb(255, 37, 37);
      border: 1.5px solid rgba(255, 89, 89, 0.771);
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 4px 15px rgba(255, 46, 46, 0.524);

    }
    .distance-display {
      font-size: 1.5em;
      margin-top: 15px;
      background: rgba(37, 37, 37, 0.5);
      padding: 10px;
      box-shadow: 0 4px 15px rgba(255, 255, 255, 0.1);

      border-radius: 10px;
      transition: background 0.5s;
      backdrop-filter: blur(10px);
    }
  </style>
</head>
<body>
  <h2>Car Control</h2>
  <div class="mode-container">
    <button class="mode-btn active" id="autoBtn" onclick="setMode('auto')">Auto</button>
    <button class="mode-btn" id="manualBtn" onclick="setMode('manual')">Manual</button>
  </div>
  <div class="distance-display" id="distanceDisplay">Distance: -- cm</div>
  <div class="manual-control" id="manualControls">
    <div class="control-grid">
      <button class="control-btn forward" onclick="sendCommand('forward')">Forward</button>
      <button class="control-btn left" onclick="sendCommand('left')">Left</button>
      <button class="control-btn right" onclick="sendCommand('right')">Right</button>
      <button class="control-btn backward" onclick="sendCommand('backward')">Backward</button>
    </div>
    <button class="stop-btn" onclick="sendCommand('stop')">STOP</button>
    <input type="range" class="speed-slider" id="speedControl" min="110" max="1023" step="10" value="512" oninput="setSpeed(this.value)">
    <p>Speed: <span id="speedValue">512</span></p>
  </div>
  <script>
    function setMode(mode) {
      fetch(`/toggleAuto?state=${mode === 'auto' ? 1 : 0}`)
        .then(res => res.text()).then(data => console.log("Mode:", data));
      document.querySelectorAll('.mode-btn').forEach(btn => btn.classList.toggle('active', btn.id === mode + 'Btn'));
      document.getElementById('manualControls').classList.toggle('active', mode === 'manual');
    }

    function sendCommand(dir) {
      fetch(`/move?direction=${dir}`).catch(console.error);
    }

    function setSpeed(value) {
      document.getElementById('speedValue').innerText = value;
      fetch(`/setSpeed?value=${value}`).catch(console.error);
    }

    function updateDistance() {
      fetch('/getDistance')
        .then(res => res.json())
        .then(data => {
          document.getElementById('distanceDisplay').innerText = `Distance: ${data.distance} cm`;
        })
        .catch(console.error);
    }

    setInterval(updateDistance, 500);
  </script>
</body>
</html>

)rawliteral";

int speedValue = 512;
unsigned long lastObstacleCheck = 0;
bool autoMode = true;

void setup() {
  Serial.begin(115200);
  setupMotorPins();
  setupUltrasonicPins();
  WiFi.softAP("Rishav", "12345678");
  Serial.println("IP Address: " + WiFi.softAPIP().toString());

  server.on("/", []() { server.send_P(200, "text/html", MAIN_page); });
  server.on("/move", []() { moveCar(server.arg("direction")); server.send(200, "text/plain", "OK"); });
  
  server.on("/setSpeed", []() {
  if (server.hasArg("value")) {
    speedValue = constrain(server.arg("value").toInt(), 100, 1023);
    server.send(200, "text/plain", "Speed updated");
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
  });

  
  server.on("/toggleAuto", []() { 
    if (server.hasArg("state")) {
      autoMode = server.arg("state").toInt() == 1;
      Serial.println(autoMode ? "Auto mode ON" : "Manual mode ON");
      server.send(200, "text/plain", autoMode ? "Auto mode ON" : "Manual mode ON");
    } else server.send(400, "text/plain", "Missing state parameter");
  });
  server.on("/getDistance", []() { 
    server.send(200, "application/json", "{\"distance\": " + String(measureDistance()) + "}");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (autoMode && millis() - lastObstacleCheck > 200) {
    lastObstacleCheck = millis();
    handleObstacleAvoidance();
  }
}

void moveCar(String direction) {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  if (direction == "forward") setMotorState(LOW, HIGH, LOW, HIGH);
  else if (direction == "backward") setMotorState(HIGH, LOW, HIGH, LOW);
  else if (direction == "left") turnCar(LOW, HIGH, HIGH, LOW);
  else if (direction == "right") turnCar(HIGH, LOW, LOW, HIGH);
  else stopCar();
}

void turnCar(int a, int b, int c, int d) {
  setMotorState(a, b, c, d);
  delay(150);
  stopCar();
}

void stopCar() { setMotorState(LOW, LOW, LOW, LOW); }

void handleObstacleAvoidance() {
    float distance = measureDistance();

    if (distance > 0 && distance < 15) {  
        smoothStop();
        moveCar("backward");
        delay(500);
        smoothStop();

        float leftDist, rightDist, frontLeft, frontRight;
        int attempt = 0;

        while (attempt < 3) {  // Allow up to 3 retries for best path
            // Scan different angles
            turnCar(LOW, HIGH, HIGH, LOW);  // Slight left turn
            delay(250);
            frontLeft = measureDistance();

            turnCar(HIGH, LOW, LOW, HIGH);  // Slight right turn
            delay(500);
            frontRight = measureDistance();

            turnCar(LOW, HIGH, HIGH, LOW);  // Full left turn
            delay(400);
            leftDist = measureDistance();

            turnCar(HIGH, LOW, LOW, HIGH);  // Full right turn
            delay(500);
            rightDist = measureDistance();

            // Decision Making: Choose the most open space
            if (leftDist > rightDist && leftDist > 20) {
                moveCar("left");
                delay(map(leftDist, 20, 100, 400, 700));  // Adaptive turn timing
                moveCar("forward");
                break;
            } else if (rightDist > leftDist && rightDist > 20) {
                moveCar("right");
                delay(map(rightDist, 20, 100, 400, 700));
                moveCar("forward");
                break;
            } else if (frontLeft > 20) {
                moveCar("left");
                delay(400);
                moveCar("forward");
                break;
            } else if (frontRight > 20) {
                moveCar("right");
                delay(400);
                moveCar("forward");
                break;
            } else {
                // If both left and right are blocked, try rotating fully
                moveCar("backward");
                delay(300);
                moveCar("left");
                delay(600); // Rotate 90 degrees
                moveCar("forward");
                attempt++;
            }
        }

        // If still stuck, perform a full rotation scan
        if (attempt >= 3) {
            moveCar("backward");
            delay(500);
            moveCar("left");
            delay(800); // 180-degree rotation
            moveCar("forward");
        }
    }
}

void smoothStop() {
    analogWrite(ENA, 120);  
    analogWrite(ENB, 120);
    delay(150);
    stopCar();
}

float measureDistance() {
    const int numSamples = 5;
    float distances[numSamples];

    for (int i = 0; i < numSamples; i++) {
        digitalWrite(TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);
        
        float duration = pulseIn(ECHO, HIGH, 20000);
        distances[i] = (duration * 0.0343) / 2;
        delay(10);
    }

    sortArray(distances, numSamples);
    float distance = distances[numSamples / 2];

    return (distance > 2 && distance < 300) ? distance : 300;
}

void sortArray(float arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                float temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}


void setMotorState(int in1, int in2, int in3, int in4) {
  digitalWrite(IN1, in1);
  digitalWrite(IN2, in2);
  digitalWrite(IN3, in3);
  digitalWrite(IN4, in4);
}

void setupMotorPins() { for (int pin : {IN1, IN2, IN3, IN4, ENA, ENB}) pinMode(pin, OUTPUT); }
void setupUltrasonicPins() { pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); }
