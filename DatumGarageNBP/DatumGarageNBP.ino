/*
  1. Upload this code and open the serial monitor
  2. On the serial monitor enter the command AT
  3. If the response is OK, your esp8266 serial baud rate is 115200.
    Close this sketch and run the example code.
    Then on the void setup make this change: espSerial.begin(115200); and upload the code.

    If you want to change the speed to 9600 use the command AT+UART_CUR=9600,8,1,0,0

  4. Otherwise if you can't see anything on the serial display reset the ESP8266 module
    change the esp8266 serial baud rate to 9600 or other, until you get a response.
    Then enter the command AT+UART_CUR=115200,8,1,0,0 to change the speed to 115200

*/

// AT                             Command to display the OK message
// AT+GMR                         Command to display esp8266 version info
// AT+UART_CUR=9600,8,1,0,0       Command to change the ESP8266 serial baud rate  to 9600
// AT Command to change the ESP8266 serial baud rate  to 115200   AT+UART_CUR=115200,8,1,0,0

#include <max6675.h>

const bool DEBUG = false;
const String versionString = "v0.25";

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

void setup() {
  Serial1.begin(115200);
  Serial.begin(9600);

  // Setup the max6675 thermocuple module
  pinMode(A_thermo_vcc_pin, OUTPUT);
  pinMode(A_thermo_gnd_pin, OUTPUT);
  digitalWrite(A_thermo_vcc_pin, HIGH);
  digitalWrite(A_thermo_gnd_pin, LOW);

  sampleStaticJoystick();

    // Announce yourself!
    Serial.println("Datum Garage NBP Streamer " + versionString);

  // Setup the AT communication. Is all of this even necessary?
  sendData("AT+RST\r\n", 1250); // reset
//  sendData("AT+CIFSR\r\n", 1250); // query local IP address
  sendData("AT+CIPMUX=1\r\n", 1250); // set multi mode
  sendData("AT+CIPSERVER=1,80\r\n", 1250); // setup server
}

// Use these variables to count the number of broadcasts made each second.
float broadcastFrequency = 0.0;
unsigned int broadcastFrequencyCounter = 0;
unsigned long lastBroadcastFrequencyTime = 0;

void loop() {
  now = millis();

  // This is not / by 0 safe. Could happen
  // todo KEEP track of last few cycles and average
  broadcastFrequency = 1000.0 / (now - lastNow);
  lastNow = now;
  
  // Build a string in the NBP v1.0 format
  String toBroadcast = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
  toBroadcast += "\"Ambient Temp\",\"C\":" + String(GetTemperatureA(), 2) + "\n";
  toBroadcast += "\"Movement X\",\"MM\":" + String((analogRead(X_pin) - Static_X) / MM_calib, 3) + "\n";
  toBroadcast += "\"Movement Y\",\"MM\":" + String((analogRead(Y_pin) - Static_Y) / MM_calib, 3) + "\n";
  toBroadcast += "\"Broadcast Freq.\",\"Hz\":" + String(broadcastFrequency, 2) + "\n";
  toBroadcast += "#\n";

  // Append the metadata if enough time has passed. Target is every 5 seconds.
  if ((now - metadataFrequency) >= lastMetadataMillis)
  {
    lastMetadataMillis = now;
    toBroadcast += "@NAME:Datum Garage\n";
    toBroadcast += "@VERSION:" + versionString + "\n";
  }

    // Build the AT CIPSEND command line here.
    String atCipSendCommand = "AT+CIPSEND=0," + String(toBroadcast.length()) + "\r\n";

    // Send the command.
    sendDataUntil(atCipSendCommand, 250, ">", "SEND-COMMAND");

    // Send the string to broadcast.
    sendDataUntil(toBroadcast, 250, "SEND OK", "SEND-PAYLOAD");
}

void sendDataUntil(String command, const int timeout, String expected, String tracer){
  Serial1.print(command); // send the command to the esp8266

  String response = "";
  unsigned long time = millis();
  bool keepTrying = true;

  while (keepTrying && (time + timeout) > millis()) {
    while (keepTrying && Serial1.available()) {
      char c = Serial1.read(); // read the next character.
      response += c;
      keepTrying = !response.endsWith(expected) || response.endsWith("ERROR");
    }
  }
  if (DEBUG || response.indexOf("ERROR") > 0) {
    Serial.println(tracer + " || " + response + " ||");
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
