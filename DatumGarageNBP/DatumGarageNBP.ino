#include <max6675.h>

const bool DEBUG = false;
const String versionString = "v0.29";

// Define LED Pins
const int RED = 5;
const int GREEN = 6;

// Arduino pins for MAX6675 Thermocouple board
const int A_thermo_gnd_pin = 45;
const int A_thermo_vcc_pin = 47;
const int A_thermo_so_pin  = 49;
const int A_thermo_cs_pin  = 51;
const int A_thermo_sck_pin = 53;

// Arduino pins for joystick
const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = 0; // analog pin connected to X output
const int Y_pin = 1; // analog pin connected to Y output
const float MM_calib = 251.0;// rough calibration of 1mm of travel on joystick.
int Static_X = 0;
int Static_Y = 0;

MAX6675 A_thermocouple(A_thermo_sck_pin, A_thermo_cs_pin, A_thermo_so_pin);

const unsigned long metadataFrequency = 5000;
unsigned long lastMetadataMillis = 0;
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
float broadcastFrequency = 0.0;
unsigned int broadcastFrequencyCounter = 0;
unsigned long lastBroadcastFrequencyTime = 0;

const int frequencyHistoryDepth = 40;
float frequencyHistory[frequencyHistoryDepth];

float calculateFrequency() {
    float currentFrequency = 1000.0 / (now - lastNow);
    float frequencyHistoryTotal = currentFrequency;

      for (int idx = frequencyHistoryDepth - 1; idx > 0; idx--){
        frequencyHistory[idx] = frequencyHistory[idx-1];
        frequencyHistoryTotal += frequencyHistory[idx-1];
      }
      frequencyHistory[0] = currentFrequency;
      float averageFreq = frequencyHistoryTotal / frequencyHistoryDepth;
      return averageFreq;
}

void loop() {
  now = millis();
  loopCount++;

  if (loopCount % 25 == 0) {
    analogWrite(GREEN, 30);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  float broadcastFrequency = calculateFrequency();
  
  lastNow = now;
  
  // Build a string in the NBP v1.0 format
  String toBroadcast = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
  toBroadcast += "\"Ambient Temp\",\"C\":" + String(GetTemperatureA(), 2) + "\n";
  toBroadcast += "\"Movement X\",\"MM\":" + String((analogRead(X_pin) - Static_X) / MM_calib, 3) + "\n";
  toBroadcast += "\"Movement Y\",\"MM\":" + String((analogRead(Y_pin) - Static_Y) / MM_calib, 3) + "\n";
  toBroadcast += "\"Broadcast Freq.\",\"Hz\":" + String(broadcastFrequency, 1) + "\n";
  toBroadcast += "\"Package Count\",\"EA\":" + String(loopCount) + "\n";
  toBroadcast += "#\n";

  // Append the metadata if enough time has passed. Target is every 5 seconds.
  if ((now - metadataFrequency) >= lastMetadataMillis)
  {
    lastMetadataMillis = now;
    toBroadcast += "@NAME:Datum Garage\n";
    toBroadcast += "@VERSION:" + versionString + "\n";
  }

    // Build the AT CIPSEND command line.
    String atCipSendCommand = "AT+CIPSEND=0," + String(toBroadcast.length()) + "\r\n";

    // Send the command.
    sendDataUntil(atCipSendCommand, 200, ">", "SEND-COMMAND");

    // Send the string to broadcast.
    sendDataUntil(toBroadcast, 200, "SEND OK", "SEND-PAYLOAD");

  if (loopCount % 25 == 0) {
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void sendDataUntil(String command, const int timeout, String expected, String tracer){
  Serial1.print(command); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();
  bool keepTrying = true;

  while (keepTrying && (time + timeout) > millis()) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read();
      response += c;
      keepTrying = !response.endsWith(expected) || response.endsWith("ERROR");
    }
  }
  bool foundError = response.indexOf("ERROR") > 0;

  if (foundError) {
    digitalWrite(RED, HIGH);
  }
  if (DEBUG || foundError) {
    Serial.println(tracer + " || " + String(millis()-time) + "ms || " + response.endsWith(expected) + " || " + response + " ||");
  }
}

void sendData(String command, const int timeout) {
  Serial1.print(command); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();

  while ( (time + timeout) > millis()) {
    while (Serial1.available()) {
      char c = Serial1.read(); // read the next character.
      response += c;
    }
  }
  if (DEBUG) {
    Serial.print(response);
  }
}

unsigned long lastTempAReadingTime = 0;
int minTempMilli = 220;
float lastTempAReading;

float GetTemperatureA()
{
  if (millis() - minTempMilli >= lastTempAReadingTime){
    lastTempAReadingTime = millis();
    lastTempAReading = A_thermocouple.readCelsius();
    //lastTempAreading = A_thermocouple.readFahrenheit();
  }

  return lastTempAReading;
}

void sampleStaticJoystick(){
  delay(250);
  Static_X = analogRead(X_pin);
  Static_Y = analogRead(Y_pin);
}
