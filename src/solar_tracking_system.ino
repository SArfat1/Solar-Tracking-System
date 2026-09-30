/*
 * Solar Tracking System - ESP32
 * --------------------------------
 * Features:
 * 1. Dual-LDR solar tracking
 * 2. Moving-average sensor filtering
 * 3. Configurable deadband / threshold
 * 4. Safe servo angle limits
 * 5. 16x2 I2C LCD monitoring
 * 6. Serial monitoring
 * 7. ESP32 Wi-Fi local dashboard
 *
 * Hardware:
 *   Left LDR  -> GPIO 34
 *   Right LDR -> GPIO 35
 *   Servo     -> GPIO 18
 *   I2C SDA   -> GPIO 21
 *   I2C SCL   -> GPIO 22
 *
 * IMPORTANT:
 * Copy secrets.example.h to secrets.h and add your Wi-Fi
 * credentials before uploading if Wi-Fi dashboard is required.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------------- PIN CONFIGURATION ----------------
#define LDR_LEFT   34
#define LDR_RIGHT  35
#define SERVO_PIN  18

#define I2C_SDA    21
#define I2C_SCL    22

// ---------------- LCD CONFIGURATION ----------------
#define LCD_ADDRESS 0x3F
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

// ---------------- SERVO CONFIGURATION ----------------
Servo trackerServo;

int angle = 90;
int previousAngle = 90;

const int MIN_ANGLE = 10;
const int MAX_ANGLE = 170;
const int SERVO_STEP = 3;

// ---------------- TRACKING CONFIGURATION ----------------
const int THRESHOLD = 250;

// Number of readings used for smoothing.
const int SAMPLE_COUNT = 8;

int leftReadings[SAMPLE_COUNT];
int rightReadings[SAMPLE_COUNT];
int sampleIndex = 0;

int ldrLeft = 0;
int ldrRight = 0;
int lightDifference = 0;

// ---------------- TIMING ----------------
unsigned long lastSensorUpdate = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastSerialUpdate = 0;

const unsigned long SENSOR_INTERVAL = 80;
const unsigned long LCD_INTERVAL = 250;
const unsigned long SERIAL_INTERVAL = 500;

// ---------------- WIFI ----------------
// Create secrets.h from secrets.example.h.
// If Wi-Fi credentials are unavailable, the tracker still works
// normally without the dashboard.
#include "secrets.h"

WebServer server(80);
bool wifiConnected = false;

// ---------------- HTML DASHBOARD ----------------
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Solar Tracker</title>
<style>
*{box-sizing:border-box}
body{
  margin:0;
  font-family:Arial,sans-serif;
  background:#07131f;
  color:#f4f8fb;
}
.container{max-width:900px;margin:auto;padding:24px}
.header{
  padding:28px;
  border:1px solid #234256;
  border-radius:18px;
  background:#0d2232;
  margin-bottom:18px;
}
.header h1{margin:0 0 8px}
.header p{margin:0;color:#9db0bd}
.grid{
  display:grid;
  grid-template-columns:repeat(2,1fr);
  gap:16px;
}
.card{
  padding:22px;
  border:1px solid #234256;
  border-radius:16px;
  background:#0d2232;
}
.label{color:#9db0bd;font-size:13px;text-transform:uppercase}
.value{font-size:32px;font-weight:700;margin-top:8px}
.status{
  display:inline-block;
  margin-top:10px;
  padding:6px 10px;
  border-radius:8px;
  background:#123c35;
  color:#54e0bd;
}
.bar{
  height:10px;
  background:#162f40;
  border-radius:20px;
  overflow:hidden;
  margin-top:14px;
}
.fill{
  height:100%;
  width:0%;
  background:#42e8c4;
  transition:width .25s ease;
}
@media(max-width:650px){
  .grid{grid-template-columns:1fr}
  .container{padding:14px}
}
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>☀ ESP32 Solar Tracker</h1>
    <p>Live local monitoring dashboard</p>
    <span class="status" id="status">Connecting...</span>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Left LDR</div>
      <div class="value" id="left">--</div>
      <div class="bar"><div class="fill" id="leftBar"></div></div>
    </div>

    <div class="card">
      <div class="label">Right LDR</div>
      <div class="value" id="right">--</div>
      <div class="bar"><div class="fill" id="rightBar"></div></div>
    </div>

    <div class="card">
      <div class="label">Light Difference</div>
      <div class="value" id="difference">--</div>
    </div>

    <div class="card">
      <div class="label">Servo Angle</div>
      <div class="value"><span id="angle">--</span>°</div>
    </div>
  </div>
</div>

<script>
async function updateData(){
  try{
    const response = await fetch('/data');
    const data = await response.json();

    document.getElementById('left').textContent = data.left;
    document.getElementById('right').textContent = data.right;
    document.getElementById('difference').textContent = data.difference;
    document.getElementById('angle').textContent = data.angle;

    document.getElementById('leftBar').style.width =
      Math.min((data.left / 4095) * 100, 100) + '%';

    document.getElementById('rightBar').style.width =
      Math.min((data.right / 4095) * 100, 100) + '%';

    document.getElementById('status').textContent = 'ESP32 Online';
  }catch(error){
    document.getElementById('status').textContent = 'Connection Lost';
  }
}

updateData();
setInterval(updateData, 500);
</script>
</body>
</html>
)rawliteral";

// ---------------- SENSOR FUNCTIONS ----------------

int readAverage(int pin, int readings[]) {
  readings[sampleIndex] = analogRead(pin);

  long total = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    total += readings[i];
  }

  return total / SAMPLE_COUNT;
}

void updateSensors() {
  ldrLeft = readAverage(LDR_LEFT, leftReadings);
  ldrRight = readAverage(LDR_RIGHT, rightReadings);

  lightDifference = ldrLeft - ldrRight;

  sampleIndex++;
  if (sampleIndex >= SAMPLE_COUNT) {
    sampleIndex = 0;
  }
}

// ---------------- TRACKING FUNCTION ----------------

void updateTracking() {

  if (lightDifference > THRESHOLD) {
    angle -= SERVO_STEP;
  }
  else if (lightDifference < -THRESHOLD) {
    angle += SERVO_STEP;
  }

  angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);

  if (angle != previousAngle) {
    trackerServo.write(angle);
    previousAngle = angle;
  }
}

// ---------------- LCD FUNCTION ----------------

void updateLCD() {

  lcd.setCursor(0, 0);
  lcd.print("L:");
  lcd.print(ldrLeft);
  lcd.print(" R:");
  lcd.print(ldrRight);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print("Angle:");
  lcd.print(angle);
  lcd.print(" D:");
  lcd.print(lightDifference);
  lcd.print("  ");
}

// ---------------- SERIAL FUNCTION ----------------

void updateSerial() {

  Serial.print("Left=");
  Serial.print(ldrLeft);

  Serial.print(" | Right=");
  Serial.print(ldrRight);

  Serial.print(" | Difference=");
  Serial.print(lightDifference);

  Serial.print(" | Angle=");
  Serial.println(angle);
}

// ---------------- WIFI FUNCTIONS ----------------

void handleRoot() {
  server.send(200, "text/html", DASHBOARD_HTML);
}

void handleData() {

  String json = "{";
  json += "\"left\":" + String(ldrLeft) + ",";
  json += "\"right\":" + String(ldrRight) + ",";
  json += "\"difference\":" + String(lightDifference) + ",";
  json += "\"angle\":" + String(angle);
  json += "}";

  server.send(200, "application/json", json);
}

void connectWiFi() {

  if (strlen(WIFI_SSID) == 0) {
    Serial.println("Wi-Fi disabled: add credentials to secrets.h");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 10000) {

    delay(300);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    wifiConnected = true;

    Serial.print("Wi-Fi connected. Dashboard: http://");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/data", handleData);

    server.begin();

    Serial.println("Web server started.");
  }
  else {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Tracker will continue without dashboard.");
  }
}

// ---------------- SETUP ----------------

void setup() {

  Serial.begin(115200);

  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" Solar Tracking ");
  lcd.setCursor(0, 1);
  lcd.print(" Starting...     ");

  // Servo
  trackerServo.setPeriodHertz(50);
  trackerServo.attach(SERVO_PIN, 500, 2400);
  trackerServo.write(angle);

  // Initialize filter buffers.
  // Starting all buffers at 0 would temporarily distort readings,
  // so populate them with real sensor values.
  int initialLeft = analogRead(LDR_LEFT);
  int initialRight = analogRead(LDR_RIGHT);

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    leftReadings[i] = initialLeft;
    rightReadings[i] = initialRight;
  }

  ldrLeft = initialLeft;
  ldrRight = initialRight;

  delay(1000);
  lcd.clear();

  // Optional local IoT dashboard
  connectWiFi();
}

// ---------------- MAIN LOOP ----------------

void loop() {

  unsigned long now = millis();

  if (now - lastSensorUpdate >= SENSOR_INTERVAL) {

    lastSensorUpdate = now;

    updateSensors();
    updateTracking();
  }

  if (now - lastLCDUpdate >= LCD_INTERVAL) {

    lastLCDUpdate = now;

    updateLCD();
  }

  if (now - lastSerialUpdate >= SERIAL_INTERVAL) {

    lastSerialUpdate = now;

    updateSerial();
  }

  if (wifiConnected) {
    server.handleClient();
  }
}
