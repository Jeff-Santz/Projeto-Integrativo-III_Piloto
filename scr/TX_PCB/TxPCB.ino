// TX
/*
* Transmissor com timer e destinatário específico  NAO MUDARRRRRRRRRRRR
*/

#include <RH_ASK.h>
#include <SPI.h> // Necessário para compilar 
#define ARDUINOJSON_ENABLE_NAN 1
#include <ArduinoJson.h>

#define LED_INT 2
#define us_para_s 1000000
#define tTimer 5 // Tempo desejado em segundos

#define PIN_TX 21 // Pino TX
#define PIN_RX 18 // Pino RX
#define SM 4 // Saída PWM com sinal modulado 
#define LDR 2 // Sensor LDR

// Parte Wi-Fi e RTC
#include <time.h>
#include <WiFi.h>

#define BRT 3

// const char* ssid     = "Isadora :)";
// const char* password = "dufa8704";
const char* ssid     = "Jeff";
const char* password = "Jefferson10";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 60*60*(-BRT);
const int   daylightOffset_sec = 0; // 3600 para horário de verão

RTC_DATA_ATTR int contWiFi;

char jsonBuffer[64];

// Pinos e variáveis diversas 
int leitura_ldr = 0, tentativa = 0, recebido = 0, tentativa_rec=0, contador=0;
// 1 (início) + 8 (data) + 1 (separador) + 6 (horário) + 1 (separador) + 4 (dado) + 1 (fim) + 9 bytes (biblioteca) 
// char LIXO[35] = "APENAS PARA TESTE";
char dado[5];
char dataHoraFormatada[64];

uint8_t idTx = 0xFF; 
uint8_t idDestino = 0xFF;

// Instanciar o driver com a taxa de 2000 bps -> 500 us
RH_ASK driver(2000, PIN_RX, PIN_TX, -1);

//Funções nossas----------------------------------------------------
void inicializaWiFi(){
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {ESP.restart();}
    i++;
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Espera até obter a hora corretamente
  struct tm tmstruct;
  int tentativas = 0;
  while (!getLocalTime(&tmstruct) && tentativas < 10) {
    Serial.println("Aguardando sincronização NTP...");
    delay(1000);
    tentativas++;
  }

  if (tentativas >= 10) {
    Serial.println("Erro: Não foi possível sincronizar a hora.");
  } else {
    Serial.println("Hora sincronizada com sucesso.");
  }

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  contWiFi = 1;
}

void dataHora(){
  struct tm tmstruct;
  if(!getLocalTime(&tmstruct)){
    Serial.println("Falha ao acessar os dados.");
    return;
  }

  // Formata a data e hora com segurança
  strftime(dataHoraFormatada, sizeof(dataHoraFormatada), "%Y-%m-%dT%H:%M:%S-03:00", &tmstruct);
  // Serial.println(dataHoraFormatada);

  // Opcional: imprimir o formato por extenso: "%A, %B %d %Y %H:%M:%S" ou "Date: %Y/%m/%d - Time: %H:%M:%S"
}

void manipulaDado(){
  dataHora();
  leitura_ldr = analogRead(LDR);
  StaticJsonDocument<64> doc;

  // itoa(leitura_ldr, dado, 10);
  // Serial.print("LDR: ");
  // Serial.println(dado);

  // Monta a mensagem completa (data|hora|dado)
  // snprintf(msg, sizeof(msg), "%s|%s", dado, dataHoraFormatada);
  doc["d"] = dataHoraFormatada;
  doc["l"] = leitura_ldr;

  serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));

  Serial.println("Mensagem montada:");
  Serial.println(jsonBuffer);
}

//Funções originais----------------------------------------------------
void setup(){
  // Corrige fuso para UTC-3 (Brasília): 
  setenv("TZ", "GMT+3", 1);  // Isso define corretamente UTC-3
  tzset();

  driver.setThisAddress(idTx);
  driver.setHeaderFrom(idTx);
  driver.setHeaderTo(idDestino);

  Serial.begin(115200);
  if (!driver.init()) {
      Serial.println("Falha ao inicializar RX");
  } else {
      Serial.println("RX inicializado");
  }

  pinMode(4, OUTPUT); // saída PWM
  pinMode(LED_INT, OUTPUT);
  pinMode(LDR, INPUT); 
  delay(500); // tempo para abrir o monitor serial

  if(contWiFi != 1){
    inicializaWiFi();
  }

  // Configura o deep sleep 
  esp_sleep_enable_timer_wakeup(us_para_s * tTimer); 
}

void loop(){
  digitalWrite(LED_INT, !(digitalRead(LED_INT)));
 
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  uint8_t from = 0x24; //0x24

  manipulaDado();
  Serial.println("Dado Enviado");
  driver.send((uint8_t*)jsonBuffer, strlen(jsonBuffer));
  driver.waitPacketSent();
  
  delay(1000);
}
