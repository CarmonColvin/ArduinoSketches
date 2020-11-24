#include <max6675.h>

const bool DEBUG = false;
const bool ECHO_PACKAGE = false;
const bool PLOTTER_PACKAGE = true;
const String versionString = "v0.45";

// Define LED Pins
const byte RED = 12;
const byte GREEN = LED_BUILTIN;

// Number of cycles to blink green status light
const byte blinkFrequency = 25;

// Pins for MAX6675 Thermocouple board (A)
const byte A_thermo_sck_pin = 52;
const byte A_thermo_cs_pin  = 50;
const byte A_thermo_so_pin  = 48;
const byte A_thermo_vcc_pin = 46;
const byte A_thermo_gnd_pin = 44;

// Pins for MAX6675 Thermocouple board (B)
const byte B_thermo_sck_pin = 40;
const byte B_thermo_cs_pin  = 38;
const byte B_thermo_so_pin  = 36;
const byte B_thermo_vcc_pin = 34;
const byte B_thermo_gnd_pin = 32;

// Pins for synchronizing LED light and button switch
const byte BLUE = 5;
const byte synchroButton = 4;

// Pins for joystick
const byte SW_pin = 2; // digital pin connected to switch output
const byte X_pin = 0; // analog pin connected to X output
const byte Y_pin = 7; // analog pin connected to Y output
const float MM_calib = 251.0;// rough calibration of 1mm of travel on joystick.
int Static_X = 0;
int Static_Y = 0;

MAX6675 A_thermocouple(A_thermo_sck_pin, A_thermo_cs_pin, A_thermo_so_pin);
MAX6675 B_thermocouple(B_thermo_sck_pin, B_thermo_cs_pin, B_thermo_so_pin);

const unsigned long updateAllFrequency = 4999;
unsigned long lastUpdateAllMillis = 0;
unsigned long now = millis();
unsigned long lastNow = 0;

int consecutiveErrorCount = 0;
int consecutiveErrorCountLimit = 25;
unsigned long loopCount = 0;

void setup() {
  // Setup and turn on the LED lights
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  analogWrite(RED, 100);
  analogWrite(GREEN, 100);

  // Setup serial connection with ESP8266
  Serial1.begin(115200);

  // Setup serial connection for debugging
  Serial.begin(9600);

  // Setup synchro LED and button
  pinMode(BLUE, OUTPUT);
  pinMode(synchroButton, INPUT_PULLUP);

  if (!PLOTTER_PACKAGE){
    // Splash!
    Serial.println("Datum Garage NBP Streamer " + versionString);
  }

  // Setup the max6675 thermocuple module as "A"
  pinMode(A_thermo_gnd_pin, OUTPUT);
  digitalWrite(A_thermo_gnd_pin, LOW);
  pinMode(A_thermo_vcc_pin, OUTPUT);
  digitalWrite(A_thermo_vcc_pin, HIGH);

  // Setup the max6675 thermocuple module as "B"
  pinMode(B_thermo_gnd_pin, OUTPUT);
  digitalWrite(B_thermo_gnd_pin, LOW);
  pinMode(B_thermo_vcc_pin, OUTPUT);
  digitalWrite(B_thermo_vcc_pin, HIGH);

  // Set the joystick calculations to center
  sampleStaticJoystick();

  // Setup the AT communication.
  sendDataUntil("AT+RST\r\n", 1250, "ready", "AT Reset");
  // sendDataUntil("AT+UART_CUR=9600,8,1,0,0", 1250, "OK", "Set Baud 9600"); // set ESP8266 baud rate to 9600
  // sendDataUntil("AT+UART_CUR=115200,8,1,0,0", 1250, "OK", "Set Baud 115200"); // set ESP8266 baud rate to 115200
  // sendDataUntil("AT+CIFSR\r\n", 1250, "OK", "ip-address"); // query local IP address
  sendDataUntil("AT+CIPMUX=1\r\n", 1250, "OK", "Set Multi Mode"); // set multi mode
  sendDataUntil("AT+CIPSERVER=1,80\r\n", 1250, "OK", "Setup Server"); // setup server

  // Turn off the LED lights
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);

  if (!PLOTTER_PACKAGE) {
    Serial.println("Server configuration complete.");
  }

  waitForConnection();
}

void waitForConnection()
{
  if (DEBUG) {
    Serial.println("Waiting for connection...");
  }

  String response = "";
  bool keepTrying = true;
  bool foundConnect = false;
  bool foundIpd = false;
  byte ledBrightness = 0;

  while (keepTrying) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read();
      response += c;
      foundConnect = response.endsWith(",CONNECT");
      foundIpd = response.endsWith("+IPD");
      keepTrying = !foundConnect && !foundIpd;
    }

    analogWrite(RED, ledBrightness / 4);
    ledBrightness += 8;
    delay(50);
  }

  analogWrite(RED, LOW);

  if (DEBUG) {
    Serial.println("Connection Established");
  }
}

// Use these variables to count the number of broadcasts made each second.
byte FrequencyIndex = 0;
float FrequencySum = 0;
const int FrequencyWindowSize = 25;
float Frequencies[FrequencyWindowSize];
float CurrentAverageFrequency = 0.0;

void broadcastFrequencyPing() {
  float currentFrequency = 1000.0 / (now - lastNow); // Calculate the current value;
  FrequencySum -= Frequencies[FrequencyIndex]; // Remove oldest from sum;
  Frequencies[FrequencyIndex] = currentFrequency; // Add newest reading to the window;
  FrequencySum += currentFrequency; // Add newest frequency to sum;
  FrequencyIndex = (FrequencyIndex + 1) % FrequencyWindowSize; // Increment index, wrap to 0;
  CurrentAverageFrequency = FrequencySum / FrequencyWindowSize; // calculate the average
  lastNow = now;
}

float getBroadcastFrequency() {
  return CurrentAverageFrequency;
}

bool isSynchroLedOn = false;

void checkSynchroLedStatus() {
  // Is the button pushed?
  if (digitalRead(synchroButton) == LOW) {
    isSynchroLedOn = true;
    digitalWrite(BLUE, HIGH);
  } else {
    digitalWrite(BLUE, LOW);
    isSynchroLedOn = false;
  }
}

void loop() {
  now = millis();
  loopCount++;

  broadcastFrequencyPing();

  if (consecutiveErrorCount >= consecutiveErrorCountLimit) {
    waitForConnection();
  }

  if (loopCount % blinkFrequency == 0) {
    analogWrite(GREEN, 30);
  }

  checkSynchroLedStatus();

  // Send the UpdateAll package on the specified frequency
  // Send the Update package on all other loops
  if ((now - lastUpdateAllMillis) >= updateAllFrequency) {
    sendNbpPackage(GetUpdateNbpPackage(true));
    lastUpdateAllMillis += updateAllFrequency;
  } else {
    sendNbpPackage(GetUpdateNbpPackage(false));
  }

  if (loopCount % blinkFrequency == 0) {
    if (DEBUG) { // Blink differently when in debug mode
      analogWrite(GREEN, 200);
      analogWrite(RED, 200);
      delay(50);
    }
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
  }
}

float lastTempA;
float lastTempB;
float lastX;
float lastY;
bool lastSyncFlag;

String GetUpdateNbpPackage(bool updateAll) {
  float currentTempA = GetTemperatureA();
  float currentTempB = GetTemperatureB();
  float currentX = (analogRead(X_pin) - Static_X) / MM_calib;
  float currentY = (analogRead(Y_pin) - Static_Y) / MM_calib;
  bool currentSyncFlag = isSynchroLedOn;
  float currentBroadcastFrequency = getBroadcastFrequency();

  String toReturn = "*NBP1,UPDATE" + String(updateAll ? "ALL" : "") + "," + String(now / 1000.0, 3) + "\n";
  
  if (updateAll || lastTempA != currentTempA) {
    lastTempA = currentTempA;
    toReturn += "\"Temp A\",\"C\":" + String(currentTempA, 2) + "\n";
  }

  if (updateAll || lastTempB != currentTempB) {
    lastTempB = currentTempB;
    toReturn += "\"Temp B\",\"C\":" + String(currentTempB, 2) + "\n";
  }

  if (updateAll || lastX != currentX) {
    lastX = currentX;
    toReturn += "\"Movement X\",\"MM\":" + String(currentX, 3) + "\n";
  }

  if (updateAll || lastY != currentY) {
    lastY = currentY;
    toReturn += "\"Movement Y\",\"MM\":" + String(currentY, 3) + "\n";
  }

  if (updateAll || lastSyncFlag != currentSyncFlag) {
    lastSyncFlag = currentSyncFlag;  
    toReturn += "\"Sync Flag\",\"Y/N\":" + String((currentSyncFlag ? "1" : "0")) + "\n";
  }

  toReturn += "\"Bcst. Freq.\",\"Hz\":" + String(currentBroadcastFrequency, 1) + "\n";
  toReturn += "\"Pkg. Count\",\"EA\":" + String(loopCount) + "\n";
  toReturn += "#\n";  

  if (updateAll){
    toReturn += "@NAME:Datum Garage\n";
    toReturn += "@VERSION:" + versionString + "\n";
  }

  if (PLOTTER_PACKAGE) {
    Serial.println("TEMP_A,TEMP_B,X,Y,SYNC,FREQ,,,,,,,,,,");
    Serial.print(currentTempA);
    Serial.print(",");
    Serial.print(currentTempB);
    Serial.print(",");
    Serial.print(currentX);
    Serial.print(",");
    Serial.print(currentY);
    Serial.print(",");
    Serial.print((currentSyncFlag ? "1" : "0"));
    Serial.print(",");
    Serial.println(currentBroadcastFrequency);
  }

  return toReturn;
}

/* Send the nbpPackage to the wi-fi serial using AT commands */
void sendNbpPackage(String nbpPackage) {
  byte atTimeout = 200;
  String atCipSendCommand = "AT+CIPSEND=0," + String(nbpPackage.length()) + "\r\n";

  Serial1.print(atCipSendCommand); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();
  bool keepTrying = true;
  bool foundExpected = false;
  bool foundError = false;

  while (keepTrying && (time + atTimeout) > millis()) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read();
      response += c;
      foundExpected = response.endsWith(">");
      foundError = response.endsWith("ERROR");
      keepTrying = !foundExpected && !foundError;
    }
  }

  if (DEBUG) {
    Serial.println("sendNbpPackage-cmd | " + String(millis() - time) + "ms | " + (foundExpected ? "SUCCESS" : "")  + (foundError ? "ERROR" : ""));
  }

  if (ECHO_PACKAGE) {
    Serial.println(nbpPackage);
  }

  if (foundError) {
    digitalWrite(RED, HIGH);
    consecutiveErrorCount++;
  } else if (foundExpected) {
    Serial1.print(nbpPackage);

    consecutiveErrorCount = 0;

    response = "";
    time = millis();
    keepTrying = true;
    foundExpected = false;
    foundError = false;

    while (keepTrying && (time + atTimeout) > millis()) {
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
    if (DEBUG) {
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
    analogWrite(RED, 200);
  }

  if (DEBUG){
    Serial.println(tracer + " | " + String(millis() - time) + "ms | " + (foundExpected ? "SUCCESS" : "")  + (foundError ? "ERROR" : ""));
  }
}

unsigned long lastTempAReadingTime = 0;
byte max6675ReadFrequency = 250;
float lastTempAReading = -99.99;

float GetTemperatureA()
{
  if (millis() - lastTempAReadingTime >= max6675ReadFrequency ) {
    lastTempAReading = A_thermocouple.readCelsius();
    // lastTempAReading = A_thermocouple.readFahrenheit();
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
    // lastTempBReading = B_thermocouple.readFahrenheit();
    lastTempBReadingTime += max6675ReadFrequency;
  }
  return lastTempBReading;
}


// Sample the neutral value of the joystick for relative movement calculation
void sampleStaticJoystick() {
  delay(150);
  Static_X = analogRead(X_pin);
  Static_Y = analogRead(Y_pin);
}
