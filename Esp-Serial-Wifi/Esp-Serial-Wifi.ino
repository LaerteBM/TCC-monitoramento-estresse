/*
    This sketch shows how to use multiple WiFi networks.
    It demonstrates:
    - Fast connect to previous WiFi network at startup
    - Registering multiple networks (at least 1)
    - Connect to WiFi with strongest signal (RSSI)
    - Fall back to connect to next WiFi when a connection failed or lost
    - Fall back to connect to hidden SSID's which are not reported by WiFi scan
*/

//INCLUSÃO DE BIBLIOTECAS
//Cógido NODEMCU
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

//Declaração de objetos
ESP8266WiFiMulti WiFiMulti;
HTTPClient https;

//D6 = 12 = Rx & D5 = 14 = Tx
SoftwareSerial serial_com(12, 14);

//DECLARAÇÃO DE VARIÁVEIS DE CONEXÃO
#ifndef STASSID
#define STASSID "brisa-1209088"
#define STAPSK  "yhiwtgzt"
#endif
 
// DECLARAÇÃO DE VARIÁVEIS PARA WIFI
const char* ssid = STASSID;
const char* password = STAPSK;

// DECLARAÇÃO DE VARIÁVEIS DA PÁGINA WEB
String url = "https://burdenlite-tcc-laerte.laertebernardo.repl.co/gravacaoDadosDash";
const char fingerprint[] PROGMEM = "7B 88 90 87 DF BA 18 94 C7 1E 39 60 B3 F8 42 BD 0D 4D 00 FE"; //DEFINIÇÃO DO CERTIFICADO SSL/TLS PARA COMUINICAÇÂO HTTPS

// DECLARAÇÃO DE VARIAVEIS PARA CONTROLE DO TEMPO DE ENVIO DAS INFORMAÇÕES
int periodo = 10000;
unsigned long timeNow = 0;


void setup() {
  //INICIALIZAÇÃO DA SERIAL E INÍCIO DA CONEXÃO WIFI
  Serial.begin(9600);
  serial_com.begin(9600);
  while(!Serial) continue;
  for (uint8_t n = 4; n > 0; n--) {
    Serial.printf("[SETUP] WAIT %d...\n", n);
    Serial.flush();
    delay(1000);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(ssid, password);
  }
}

//LOOP
void loop() {
  
  timeNow = millis();

  if ((WiFiMulti.run() == WL_CONNECTED)) {
    //Criação de um canal de comunicação wi-fi seguro para envio dos dados
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(fingerprint);

    if (https.begin(*client, url)) {
      postAmbiente(); 
      https.end();
    }
  }
}

void postAmbiente() {
  //ALIMENTAÇÃO DO ARQUIVO JSON ATRAVÉS DA COMUNICAÇÃO SERIAL
  StaticJsonDocument<1000> dados;
  deserializeJson(dados, serial_com);
  JsonObject json = dados.as<JsonObject>();
  
  if(json.isNull()); // VERIFICA SE O JSON RECEBIDO POSSUI DADOS
  else{
    
    String jsonOutput;
    serializeJson(dados, jsonOutput);
    
    //DEFINIÇÃO DOS DADOS DO HEADER DA REQUISIÇÃO
    https.setURL(url);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("User-Agent", "ESP8266");
    Serial.print("Dados enviado: ");
    Serial.println(jsonOutput);

    //Resultados da requisição
    int resCode = https.POST(jsonOutput);
    Serial.print("Código de resposta HTTP: ");
    Serial.println(resCode); 
  }
  
  
}
