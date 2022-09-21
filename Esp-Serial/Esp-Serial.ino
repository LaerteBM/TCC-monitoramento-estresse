//Sending Data from Arduino to NodeMCU Via Serial Communication
//NodeMCU code
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

//D6 = 12 = Rx & D5 = 14 = Tx
SoftwareSerial serial_com(12, 14);
float Umidade = 0;
float Temperatura = 0;


void setup() {
  // Initialize Serial port
  Serial.begin(9600);
  serial_com.begin(9600);
  while(!Serial) continue;
}

void loop() {
  
  StaticJsonDocument<1000> dados;
  deserializeJson(dados, serial_com);
  JsonObject json_teste = dados.as<JsonObject>();
  if(json_teste.isNull());
  else{
    serializeJson(dados, Serial);
    Serial.println();
    Umidade = dados["Umidade"];
    Temperatura = dados["Temperatura"];
  }
  
}
