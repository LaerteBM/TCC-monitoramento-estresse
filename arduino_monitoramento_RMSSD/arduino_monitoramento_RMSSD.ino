#define USE_ARDUINO_INTERRUPTS true    // Habilita interrupções de baixo nível para um funcionamento mais preciso do Sensor de monitoramento cardíaco

//INCLUSÃO DAS BIBLIOTECAS
#include <dht.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <PulseSensorPlayground.h>
#include <Wire.h>
#include <BH1750.h>

//DEFINIÇÃO DE PINOS
#define pinInterrupcao 2
#define pinSensor 7             //Pino utilizado para ler o valor do Sensor DHT22
const int PulseWire = A0;       // Pino utilizado para medir receber a saída do Sensor monitor cardíaco
const int LED13 = 13;          // Determina que o Pin 13 que corresponde ao LED  imbuitido no arduino pisque quando detectar um batimento

//INTERVALO DE LEITURA
#define intervalo 10000


//CRIANDO VARIAVEIS E INSTANCIANDO OBJETOS
BH1750 luximetro;
dht sensorDHT;
PulseSensorPlayground pulseSensor;

float Temperatura = 0;
float Umidade = 0;
float Iluminancia = 0;
double RMSSD = 0;
int Threshold = 520;           // Determina a partir de qual valor uma leitura será considerada como uma onda R (início de um novo batimento)
bool interrupcao=0; 

unsigned long delayIntervalo = 0;
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

  //Parametrização do objeto PulseSensor
  pulseSensor.analogInput(PulseWire);     //Define o pino de entrada do objeto
  pulseSensor.blinkOnPulse(LED13);       //Faz o Led piscar automaticamente quando um batimento for detectado
  pulseSensor.setThreshold(Threshold);   //Define o limite mínimo para que um pulso seja considerado um batimento

  if (pulseSensor.begin()) {
    Serial.println("Sensor de monitoramento cardíaco funcionando normalmente");
  }
  
  luximetro.begin();

  delay(10000);
  calculo_RMSSD();

  pinMode(pinInterrupcao, INPUT);
  //attachInterrupt(digitalPinToInterrupt(pinInterrupcao), calculo_RMSSD, RISING);  //declara a interrupção externa para o pino 2 quando houver alteração no estado da porta de LOW para HIGH
    
}

void loop()
{

  if ( (millis() - delayIntervalo) > intervalo ) {
    //LEITURA DOS DADOS
    ler_Temp_Umi_Ilu();
    enviar_dados_serial();
  };

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

  Serial.print("Nº de amostras:");
  Serial.println(cont);

  for(int i=0; i<cont; i++){
    Serial.print("IBI ");
    Serial.print(i+1);
    Serial.print(": ");
    Serial.println(intervalos[i]);
    delay(50);
  }
  
  for(int i=0; i<cont; i++){

    numerador += pow(double(intervalos[i]) - double(intervalos[i+1]), 2);
    Serial.print(double(intervalos[i]));
    Serial.print("-");
    Serial.print(double(intervalos[i+1]));
    Serial.print("=");
    Serial.println(double(intervalos[i]) - double(intervalos[i+1]));
    
  }

  RMSSD = sqrt(numerador/double(cont-1.0));
  Serial.print("Num: ");
  Serial.println(numerador);
  Serial.println("RMSSD: ");
  Serial.println(RMSSD);

  
}

void ler_Temp_Umi_Ilu(){
  
      int chk = sensorDHT.read22(pinSensor);
      Umidade = sensorDHT.humidity;
      Temperatura = sensorDHT.temperature;
      Iluminancia = luximetro.readLightLevel();
      delayIntervalo = millis();
}

int Medir_Ibi(){                            //Função para medir e retornar o intervalo entre os batimentos (Inter Beat Interval - IBI) em ms

   pulseSensor.sawNewSample();
   
   if(pulseSensor.sawStartOfBeat()){            //Verifica a ocorrência de batimentos
    int IBI = pulseSensor.getInterBeatIntervalMs();
    return IBI;
  }
  else return -1; //quando não for detectado nenhum batimento a função irá retornar -1
  
}
