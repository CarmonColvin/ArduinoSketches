#include <max6675.h>

const bool DEBUG = false;
const String versionString = "v0.38";

// Define LED Pins
const byte RED = 5;
const byte GREEN = 6;

const byte blinkFrequency = 25; // number of cycles to blink green light

// Arduino pins for MAX6675 Thermocouple board (A)
const byte A_thermo_gnd_pin = 44;
const byte A_thermo_vcc_pin = 46;
const byte A_thermo_so_pin  = 48;
const byte A_thermo_cs_pin  = 50;
const byte A_thermo_sck_pin = 52;

// Arduino pins for MAX6675 Thermocouple board (B)
const byte B_thermo_gnd_pin = 32;
const byte B_thermo_vcc_pin = 34;
const byte B_thermo_so_pin  = 36;
const byte B_thermo_cs_pin  = 38;
const byte B_thermo_sck_pin = 40;

// Arduino pins for joystick
const byte SW_pin = 2; // digital pin connected to switch output
const byte X_pin = 0; // analog pin connected to X output
const byte Y_pin = 1; // analog pin connected to Y output
const float MM_calib = 251.0;// rough calibration of 1mm of travel on joystick.
int Static_X = 0;
int Static_Y = 0;

MAX6675 A_thermocouple(A_thermo_sck_pin, A_thermo_cs_pin, A_thermo_so_pin);
MAX6675 B_thermocouple(B_thermo_sck_pin, B_thermo_cs_pin, B_thermo_so_pin);

const unsigned long updateAllFrequency = 4000; // Milliseconds 1000 = 1sec.
unsigned long lastUpdateAllMillis = 0;
unsigned long now = millis();
unsigned long lastNow = 0;

unsigned long loopCount = 0;

void setup() {
  // Setup and turn on the LED lights
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial1.begin(115200);
  Serial.begin(9600);

  // Setup the max6675 thermocuple module.
  pinMode(A_thermo_vcc_pin, OUTPUT);
  pinMode(A_thermo_gnd_pin, OUTPUT);
  digitalWrite(A_thermo_vcc_pin, HIGH);
  digitalWrite(A_thermo_gnd_pin, LOW);

  Serial.println("Datum Garage NBP Streamer " + versionString);

  sampleStaticJoystick();

  // Setup the AT communication.
  sendDataUntil("AT+RST\r\n", 3000, "ready", "reset");
  // sendData("AT+UART_CUR=9600,8,1,0,0", 1250); // set ESP8266 baud rate to 9600
  // sendData("AT+UART_CUR=115200,8,1,0,0", 1250); // set ESP8266 baud rate to 115200
  // sendDataUntil("AT+CIFSR\r\n", 3000, "OK", "ip-address"); // query local IP address
  sendDataUntil("AT+CIPMUX=1\r\n", 3000, "OK", "multi-mode"); // set multi mode
  sendDataUntil("AT+CIPSERVER=1,80\r\n", 3000, "OK", "cip-server"); // setup server

  Serial.println("Server configuration complete.");

  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
}

// Use these variables to count the number of broadcasts made each second.
byte FrequencyIndex = 0;
float FrequencySum = 0;
const int FrequencyWindowSize = 40;
float Frequencies[FrequencyWindowSize];

// TODO: Make this a ping and read situation so it works more like a sensor would.
float calculateFrequency() {
  float currentFrequency = 1000.0 / (now - lastNow); // Calculate the current value;
  FrequencySum -= Frequencies[FrequencyIndex]; // Remove oldest from sum;
  Frequencies[FrequencyIndex] = currentFrequency; // Add newest reading to the window;
  FrequencySum += currentFrequency; // Add newest frequency to sum;
  FrequencyIndex = (FrequencyIndex+1) % FrequencyWindowSize; // Increment index, wrap to 0;
  return FrequencySum / FrequencyWindowSize; // return the average
}

void loop() {
  now = millis();
  loopCount++;

  if (loopCount % blinkFrequency == 0) {
    analogWrite(GREEN, 30);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  float broadcastFrequency = calculateFrequency();
  lastNow = now;

  // Send the UpdateAll package on the specified frequency
  // Send the Update package on all other loops
  if ((now - lastUpdateAllMillis) >= updateAllFrequency) {
    sendNbpPackage(GetUpdateAllNbpPackage(broadcastFrequency));
    lastUpdateAllMillis += updateAllFrequency;
  } else {
    sendNbpPackage(GetUpdateNbpPackage(broadcastFrequency));
  }

  if (loopCount % blinkFrequency == 0) {
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

String GetUpdateAllNbpPackage(float broadcastFrequency) {
  String toReturn = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
  toReturn += "\"Temp A\",\"C\":" + String(GetTemperatureA(), 2) + "\n";
  toReturn += "\"Temp B\",\"C\":" + String(GetTemperatureB(), 2) + "\n";
  toReturn += "\"Movement X\",\"MM\":" + String((analogRead(X_pin) - Static_X) / MM_calib, 3) + "\n";
  toReturn += "\"Movement Y\",\"MM\":" + String((analogRead(Y_pin) - Static_Y) / MM_calib, 3) + "\n";
  toReturn += "\"Bcst. Freq.\",\"Hz\":" + String(broadcastFrequency, 1) + "\n";
  toReturn += "\"Pkg. Count\",\"EA\":" + String(loopCount) + "\n";
  toReturn += "#\n";
  toReturn += "@NAME:Datum Garage\n";
  toReturn += "@VERSION:" + versionString + "\n";
  return toReturn;
}

float lastTempA;
float lastTempB;
float lastX;
float lastY;

String GetUpdateNbpPackage(float broadcastFrequency) {
  float currentTempA = GetTemperatureA();
  float currentTempB = GetTemperatureB();
  float currentX = (analogRead(X_pin) - Static_X) / MM_calib;
  float currentY = (analogRead(Y_pin) - Static_Y) / MM_calib;

  String toReturn = "*NBP1,UPDATE," + String(now / 1000.0, 3) + "\n";

  if (lastTempA != currentTempA) {
    lastTempA = currentTempA;
    toReturn += "\"Temp A\",\"C\":" + String(currentTempA, 2) + "\n";
  }

  if (lastTempB != currentTempB) {
    lastTempB = currentTempB;
    toReturn += "\"Temp B\",\"C\":" + String(currentTempA, 2) + "\n";
  }

  if (lastX != currentX) {
    lastX = currentX;
    toReturn += "\"Movement X\",\"MM\":" + String(currentX, 3) + "\n";
  }

  if (lastY != currentY) {
    lastY = currentY;
    toReturn += "\"Movement Y\",\"MM\":" + String(currentY, 3) + "\n";
  }

  toReturn += "\"Bcst. Freq.\",\"Hz\":" + String(broadcastFrequency, 1) + "\n";
  toReturn += "\"Pkg. Count\",\"EA\":" + String(loopCount) + "\n";
  toReturn += "#\n";

  return toReturn;
}

void sendNbpPackage(String nbpPackage) {
  String atCipSendCommand = "AT+CIPSEND=0," + String(nbpPackage.length()) + "\r\n";

  Serial1.print(atCipSendCommand); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();
  bool keepTrying = true;
  bool foundExpected = false;
  bool foundError = false;

  while (keepTrying && (time + 200) > millis()) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read();
      response += c;
      foundExpected = response.endsWith(">");
      foundError = response.endsWith("ERROR");
      keepTrying = !foundExpected && !foundError;
    }
  }

  if (DEBUG || foundError) {
    Serial.println("sendNbpPackage-cmd | " + String(millis() - time) + "ms | " + (foundExpected ? "SUCCESS" : "")  + (foundError ? "ERROR" : ""));
  }

  if (foundError) {
    digitalWrite(RED, HIGH);
    delay(300);
  } else if (foundExpected) {
    Serial1.print(nbpPackage);

    response = "";
    time = millis();
    keepTrying = true;
    foundExpected = false;
    foundError = false;

    while (keepTrying && (time + 200) > millis()) {
      while (keepTrying && Serial1.available()) {
        char c = Serial1.read();
        response += c;
        foundExpected = response.endsWith("SEND OK");
        foundError = response.endsWith("ERROR");
        keepTrying = !foundExpected && !foundError;
      }
    }

    if (foundError) {
      digitalWrite(RED, HIGH);
    }
    if (DEBUG || foundError) {
      Serial.println("sendNbpPackage-pkg | " + String(millis() - time) + "ms | " + (foundExpected ? "SUCCESS" : "")  + (foundError ? "ERROR" : ""));
    }
  }
}

void sendDataUntil(String command, const int timeout, String expected, String tracer) {
  Serial1.print(command); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();
  bool keepTrying = true;
  bool foundExpected = false;
  bool foundError = false;

  while (keepTrying && (time + timeout) > millis()) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read();
      response += c;
      foundExpected = response.endsWith(expected);
      foundError = response.endsWith("ERROR");
      keepTrying = !foundExpected && !foundError;
    }
  }

  if (foundError) {
    digitalWrite(RED, HIGH);
  }
  if (DEBUG || foundError) {
    Serial.println(tracer + " | " + String(millis() - time) + "ms | " + (foundExpected ? "SUCCESS" : "")  + (foundError ? "ERROR" : ""));
  }
}

unsigned long lastTempAReadingTime = 0;
byte max6675ReadFrequency = 220;
float lastTempAReading = -99.99;

float GetTemperatureA()
{
  if (millis() - lastTempAReadingTime >= max6675ReadFrequency ) {
    lastTempAReading = A_thermocouple.readCelsius();
    //lastTempAreading = A_thermocouple.readFahrenheit();
    lastTempAReadingTime += max6675ReadFrequency;
  }
  return lastTempAReading;
}

unsigned long lastTempBReadingTime = 0;
float lastTempBReading = -99.99;

float GetTemperatureB()
{
  if (millis() - lastTempBReadingTime >= max6675ReadFrequency ) {
    lastTempBReading = B_thermocouple.readCelsius();
    //lastTempBreading = B_thermocouple.readFahrenheit();
    lastTempBReadingTime += max6675ReadFrequency;
  }
  return lastTempBReading;
}

/* 
  Sample the neutral value of the joystick for relative movement calculation 
*/
void sampleStaticJoystick() {
  delay(250);
  Static_X = analogRead(X_pin);
  Static_Y = analogRead(Y_pin);
}
