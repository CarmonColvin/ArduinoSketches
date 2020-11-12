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
const String versionString = "v0.17";

const int A_thermo_gnd_pin = 45;
const int A_thermo_vcc_pin = 47;
const int A_thermo_so_pin  = 49;
const int A_thermo_cs_pin  = 51;
const int A_thermo_sck_pin = 53;

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

  // Random used to fake telemetry data
  randomSeed(analogRead(0));

  // Announce yourself!
  Serial.println("Datum Garage " + versionString);
  Serial.println("Send Delay: " + String(sendDataDelay));

  // Setup the AT communication. Is all of this even necessary?
  sendData("AT+RST\r\n", 2000, DEBUG); // reset
  sendData("AT+CIFSR\r\n", 2000, DEBUG); // query local IP address
  sendData("AT+CIPMUX=1\r\n", 2000, DEBUG); // set multi mode
  sendData("AT+CIPSERVER=1,80\r\n", 2000, DEBUG); // setup server
}

void loop() {
    now = millis();

    float batteryV = random(1200,1350) / 100.0; // faked telemerty data
    float brakePct = random(0,1000) / 10.0; // faked telemerty data
    float a_tempF = A_thermocouple.readFahrenheit(); // read the temp from the K-thermo

    // Build a string in the NBP v1.0 format
    String toBroadcast = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
           toBroadcast += "\"Battery\",\"V\":";
           toBroadcast += String(batteryV, 2);
           toBroadcast += "\n";
           toBroadcast += "\"Brake Pedal\",\"%\":";
           toBroadcast += String(brakePct, 2);
           toBroadcast += "\n";
           toBroadcast += "\"Ambient Temp\",\"F\":";
           toBroadcast += String(a_tempF, 2);
           toBroadcast += "\n";
           toBroadcast += "#\n";

   // Append the metadata if enough time has passed. Target is every 5 seconds.
   if ((now - metadataFrequency) >= lastMetadataMillis)
   {
      lastMetadataMillis = now;
      toBroadcast += "@NAME:Datum Garage\n";      
      toBroadcast += "@VERSION:" + versionString + "\n";      
   }

     sendData("AT+CIPSEND=0," + String(toBroadcast.length()) + "\r\n", sendDataDelay - 10, DEBUG);
     sendData(toBroadcast, sendDataDelay, DEBUG);
}

String sendData(String command, const int timeout, boolean debug) {
  if (debug)
  {
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
