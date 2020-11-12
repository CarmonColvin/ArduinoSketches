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

#define DEBUG true
String versionString = "v0.16";
int sendDataDelay = 115; // 115 is lower limit. AT errors any faster
unsigned long metadataFrequency = 5000;
unsigned long lastMetadataMillis = 0;
unsigned long now = millis();

void setup() {
 Serial.begin(9600);
 Serial1.begin(115200);

 randomSeed(analogRead(0));
 
 Serial.println("Datum Garage " + versionString);
 Serial.println("Send Delay: " + String(sendDataDelay));

   sendData("AT+RST\r\n", 2000, DEBUG); // rst
   sendData("AT+CIFSR\r\n", 2000, DEBUG);
   sendData("AT+CIPMUX=1\r\n", 2000, DEBUG); // set multi mode
   sendData("AT+CIPSERVER=1,80\r\n", 2000, DEBUG); // setup server
}

void loop() {
    now = millis();

    float batteryV = random(1200,1350) / 100.0;
    float brakePct = random(0,1000) / 10.0;
  
    String toBroadcast = "*NBP1,UPDATEALL," + String(now / 1000.0, 3) + "\n";
           toBroadcast += "\"Battery\",\"V\":";
           toBroadcast += String(batteryV, 2);
           toBroadcast += "\n";
           toBroadcast += "\"Brake Pedal\",\"%\":";
           toBroadcast += String(brakePct, 2);
           toBroadcast += "\n";
           toBroadcast += "#\n";

   // Append the metadata if enough time has passed.
   if ((now - metadataFrequency) >= lastMetadataMillis)
   {
      lastMetadataMillis = now;
      toBroadcast += "@NAME:Datum Garage\n";      
      toBroadcast += "@VERSION:" + versionString + "\n";      
   }

     sendData("AT+CIPSEND=0," + String(toBroadcast.length()) + "\r\n", sendDataDelay, DEBUG);
     sendData(toBroadcast, sendDataDelay, DEBUG);

//  while (Serial.available()) { 
//      char ch = Serial.read();
//      Serial1.print(ch);
//    }

//  String response ="";
  
//  while (Serial1.available()) { 
//      char ch1 = Serial1.read();
//      response += ch1;
//    }

//     Serial.print(response);
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
