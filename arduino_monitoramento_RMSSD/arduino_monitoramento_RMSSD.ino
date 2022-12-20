#define USE_ARDUINO_INTERRUPTS true    // Habilita interrupções de baixo nível para um funcionamento mais preciso do Sensor de monitoramento cardíaco

//INCLUSÃO DAS BIBLIOTECAS
#include <dht.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <PulseSensorPlayground.h>
#include <Wire.h>
#include <BH1750.h>

//DEFINIÇÃO DE PINOS
#define pinSensor 7             //Pino utilizado para ler o valor do Sensor DHT22
const int PulseWire = A0;       // Pino utilizado para medir receber a saída do Sensor monitor cardíaco
const int LED13 = 13;          // Determina que o Pin 13 que corresponde ao LED  imbuitido no arduino pisque quando detectar um batimento
const int botao=4;

//CRIANDO VARIAVEIS E INSTANCIANDO OBJETOS
BH1750 luximetro;
dht sensorDHT;
PulseSensorPlayground pulseSensor;

float Temperatura = 0;
float Umidade = 0;
float Iluminancia = 0;
double RMSSD = 0;
int Threshold = 520;           // Determina a partir de qual valor uma leitura será considerada como uma onda R (início de um novo batimento)

unsigned long tempoInicio = 0;

//Declarando funções
int Medir_Ibi();
void calculo_RMSSD();
void ler_Temp_Umi();
void enviar_dados_serial();

//Cria uma Serial para comunicação do Arduino com a ESP, onde 5=Rx & 6=Tx
SoftwareSerial serial_com(5, 6);


void setup()
{
  // INICIANDO COMUNICAÇÕES
  Serial.begin(9600); //Serial ARD-PC
  serial_com.begin(9600); //Serial ARD-Node
  Wire.begin(); //Comunicação I2C

  pinMode(botao, INPUT_PULLUP);

  //Parametrização do objeto PulseSensor
  pulseSensor.analogInput(PulseWire);     //Define o pino de entrada do objeto
  pulseSensor.blinkOnPulse(LED13);       //Faz o Led piscar automaticamente quando um batimento for detectado
  pulseSensor.setThreshold(Threshold);   //Define o limite mínimo para que um pulso seja considerado um batimento
  
  luximetro.begin();

  delay(500);
    
}

void loop()
{

  if(digitalRead(botao)==LOW){
    Serial.println("Aguarde o fim das medições e o envio das informações");
    //LEITURA DOS DADOS
    if (pulseSensor.begin()) {
      Serial.println("Sensor de monitoramento cardíaco funcionando normalmente");
      calculo_RMSSD();
    }
    
    ler_Temp_Umi_Ilu();
    enviar_dados_serial();
    delay(500);
  }

}

void enviar_dados_serial(){

  StaticJsonDocument<1000> dados;
  
  //Grava os dados medidos no JSON
  dados["Umidade"] = Umidade;
  dados["Temperatura"] = Temperatura;
  dados["RMSSD"] = RMSSD;
  dados["Iluminancia"] = Iluminancia;
  
  //Envia os dados para a NodeMCU e exibe o JSON na serial
  serializeJson(dados, serial_com);
  serializeJson(dados, Serial);
  Serial.println();
  
}

void calculo_RMSSD(){

  int cont=0;
  tempoInicio = millis();
  int intervalos[300]; //Como no tempo determinado é impossível ocorrerem 300 batimento, este valor é mais do que suficiente para realização do cálculo
  double numerador = 0; //Variável que irá armazenar o numerador da fórmula do RMSSD
  
  for(int i=0; millis() - tempoInicio < 10000; i++){ //Faz a medição por 10 segundos
  
      int leitura = Medir_Ibi();
      
      if(leitura>0){ //Como a Medir_Ibi() retorna -1 quando não detecta nenhum batimento estes valores não serão armazenados, nem considerados parte da amostra
        intervalos[cont]=leitura;
        cont++;
      }
      
  }
  
  for(int i=0; i<cont; i++){

    numerador += pow(double(intervalos[i]) - double(intervalos[i+1]), 2);
    
  }

  RMSSD = sqrt(numerador/double(cont-1.0));
  
}

void ler_Temp_Umi_Ilu(){
  
      int chk = sensorDHT.read22(pinSensor);
      Umidade = sensorDHT.humidity;
      Temperatura = sensorDHT.temperature;
      Iluminancia = luximetro.readLightLevel();
      
}

int Medir_Ibi(){                            //Função para medir e retornar o intervalo entre os batimentos (Inter Beat Interval - IBI) em ms

   pulseSensor.sawNewSample();
   
   if(pulseSensor.sawStartOfBeat()){            //Verifica a ocorrência de batimentos
    int IBI = pulseSensor.getInterBeatIntervalMs();
    return IBI;
  }
  else return -1; //quando não for detectado nenhum batimento a função irá retornar -1
  
}
