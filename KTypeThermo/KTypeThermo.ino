#include <max6675.h>


// ThermoCouple
int A_thermo_gnd_pin = 31;
int A_thermo_vcc_pin = 33;
int A_thermo_so_pin  = 35;
int A_thermo_cs_pin  = 37;
int A_thermo_sck_pin = 39;

int B_thermo_gnd_pin = 45;
int B_thermo_vcc_pin = 47;
int B_thermo_so_pin  = 49;
int B_thermo_cs_pin  = 51;
int B_thermo_sck_pin = 53;
  
MAX6675 A_thermocouple(A_thermo_sck_pin, A_thermo_cs_pin, A_thermo_so_pin);
MAX6675 B_thermocouple(B_thermo_sck_pin, B_thermo_cs_pin, B_thermo_so_pin);
  
void setup() {
  Serial.begin(9600);

  pinMode(A_thermo_vcc_pin, OUTPUT); 
  pinMode(A_thermo_gnd_pin, OUTPUT); 
  digitalWrite(A_thermo_vcc_pin, HIGH);
  digitalWrite(A_thermo_gnd_pin, LOW);
  
  pinMode(B_thermo_vcc_pin, OUTPUT); 
  pinMode(B_thermo_gnd_pin, OUTPUT); 
  digitalWrite(B_thermo_vcc_pin, HIGH);
  digitalWrite(B_thermo_gnd_pin, LOW);
}

void loop() {
   Serial.print("A = "); 
   Serial.println(A_thermocouple.readCelsius());
   Serial.print("B = "); 
   Serial.println(B_thermocouple.readCelsius());
     
   delay(255);
}
