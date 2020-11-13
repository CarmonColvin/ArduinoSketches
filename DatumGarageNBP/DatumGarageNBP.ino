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

#define DEBUG false
const String versionString = "v0.21";

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

const int sendDataDelay = 115; // 115 is lower limit. Triggers AT errors when shorter delay is used.
const unsigned long metadataFrequency = 4999;
unsigned long lastMetadataMillis = 0;
unsigned long now = millis();

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);

  // Setup the max6675 thermocuple module
  pinMode(A_thermo_vcc_pin, OUTPUT);
  pinMode(A_thermo_gnd_pin, OUTPUT);
  digitalWrite(A_thermo_vcc_pin, HIGH);
  digitalWrite(A_thermo_gnd_pin, LOW);

  sampleStaticJoystick();

  // Announce yourself!
  Serial.println("Datum Garage " + versionString);
  Serial.println("Send Delay: " + String(sendDataDelay));

  // Setup the AT communication. Is all of this even necessary?
  sendData("AT+RST\r\n", 1500, DEBUG); // reset
  sendData("AT+CIFSR\r\n", 1500, DEBUG); // query local IP address
  sendData("AT+CIPMUX=1\r\n", 1500, DEBUG); // set multi mode
  sendData("AT+CIPSERVER=1,80\r\n", 1500, DEBUG); // setup server
}

void loop() {
  now = millis();

  if (DEBUG){
    Serial.println("loop(" + String(now) + ")");
  }
  
  float a_temp = GetTemperatureA(); // read the temp from the K-thermo
  
  int relativeX = analogRead(X_pin) - Static_X;
  int relativeY = analogRead(Y_pin) - Static_Y;

  float calibX = relativeX / MM_calib;
  float calibY = relativeY / MM_calib;

  // Build a string in the NBP v1.0 format
  String toBroadcast = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
  toBroadcast += "\"Ambient Temp\",\"C\":";
  toBroadcast += String(a_temp, 2);
  toBroadcast += "\n";
  toBroadcast += "\"Movement X\",\"MM\":";
  toBroadcast += String(calibX, 3);
  toBroadcast += "\n";
  toBroadcast += "\"Movement Y\",\"MM\":";
  toBroadcast += String(calibY, 3);
  toBroadcast += "\n";
  toBroadcast += "#\n";

  // Append the metadata if enough time has passed. Target is every 5 seconds.
  if ((now - metadataFrequency) >= lastMetadataMillis)
  {
    lastMetadataMillis = now;
    toBroadcast += "@NAME:Datum Garage\n";
    // toBroadcast += "@VERSION:" + versionString + "\n";
  }

  atCipSend(toBroadcast, DEBUG);
}

String sendData(String command, const int timeout, boolean debug) {
  if (debug) {
    Serial.print(command);
  }

  String response = "";

  Serial1.print(command); // send the read character to the esp8266

  long int time = millis();
  while ( (time + timeout) > millis()) {
    while (Serial1.available()) {
      // output to the serial window
      char c = Serial1.read(); // read the next character.
      response += c;
    }
  }
  if (debug) {
    Serial.print(response);
  }
  return response;
}

void atCipSend(String payload, boolean debug) {
  String atCipSendCommand = "AT+CIPSEND=0," + String(payload.length()) + "\r\n";

  Serial1.print(atCipSendCommand);

  if (debug) {
    Serial.println("Sent: " + atCipSendCommand);
  }

  if (Serial1.available()) {

    if (Serial1.find(">")) {
      if (debug) {
        Serial.println("Found >");
      }

      Serial1.print(payload);

      if (debug) {
        Serial.println("Sent: " + payload);
      }
    } else {
      Serial1.println("Not found!");
    }
  }
}

unsigned long lastTempAReadingTime = 0;
int minTempMilli = 250;
float lastTempAReading;

float GetTemperatureA()
{
  if (millis() - minTempMilli >= lastTempAReadingTime){
    lastTempAReadingTime = millis();
    lastTempAReading = A_thermocouple.readCelsius();
    //lastTempAreading = A_thermocouple.readFahrenheit();

    if (DEBUG){
    Serial.println("GetTemperatureA(): " + String(lastTempAReading, 2));
    }
  }

  return lastTempAReading;
}

void sampleStaticJoystick(){
  delay(300);
  Static_X = analogRead(X_pin);
  Static_Y = analogRead(Y_pin);
}
