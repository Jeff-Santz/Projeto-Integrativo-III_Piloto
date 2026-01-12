#include <RH_ASK.h>
#include <SPI.h> 
#include <ArduinoJson.h>

// Parte Wi-Fi e RTC
#include <time.h>
#include <WiFi.h>

#define us_para_s 1000000
#define tTimer 2 // Tempo desejado em segundos
#define BRT -3

#define PIN_RX 19 // Pin RX
#define PIN_TX 21 // Pin TX

// Configuracoes iniciais de cabeçalho
uint8_t thisAddress = 0xFF; // end. oficial do RX
uint8_t idDestino = 0xFF;

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t buflen = sizeof(buf);

uint8_t from = 0xFF; //0x24 (ByPass) para receptor e 0x23 (TX) para bypass

// Configurações para o Wi-Fi
// const char* ssid     = "Isadora :)";
// const char* password = "dufa8704";
const char* ssid     = "Jeff";
const char* password = "Jefferson10";

// Uso para o RTC
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 60*60*BRT;
const int   daylightOffset_sec = 0; // 3600 para horário de verão

// Variáveis do RTC
RTC_DATA_ATTR int contWiFi;
RTC_DATA_ATTR bool podeDormir;
RTC_DATA_ATTR int horasPegas = 0;
RTC_DATA_ATTR uint8_t verificacao[64]; 

float mediaFinal = 0;
float tempoReenvio = 2; // Tempo necessário para os 3 reenvios + pequena espera
char dataHoraFormatada[64];
float medias[4];
RTC_DATA_ATTR time_t horas[5];

// Instanciar o driver com a taxa de 2000 bps -> 500 us
RH_ASK driver(2000, PIN_RX, PIN_TX, -1);

//Funções nossas----------------------------------------------------
void Durma() {
  // Cria o timer
  esp_sleep_enable_timer_wakeup((mediaFinal - 2) * us_para_s);
  Serial.print("Dormindo por:");
  Serial.println(mediaFinal - tempoReenvio);
  delay(5);
}

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
  strftime(dataHoraFormatada, sizeof(dataHoraFormatada), "%Y%m%d|%H%M%S", &tmstruct);
  Serial.println(dataHoraFormatada);
  // Opcional: imprimir o formato por extenso: "%A, %B %d %Y %H:%M:%S" ou "Date: %Y/%m/%d - Time: %H:%M:%S"
}

//Funções originais----------------------------------------------------
void setup() {
  driver.setThisAddress(thisAddress);
  driver.setHeaderTo(idDestino);

  // Corrige fuso para UTC-3 (Brasília)
    setenv("TZ", "GMT+3", 1);
    tzset();

    Serial.begin(115200);
    while(!Serial){}

    if (!driver.init()) {
        Serial.println("Falha ao inicializar RX");
    } else {
        Serial.println("RX inicializado");
    }
 
    if(contWiFi != 1){
      inicializaWiFi();
    }
}

void loop() {
    //Recebimento de informações
    if (driver.recv(buf, &buflen)) {
      buf[buflen] = '\0';
      StaticJsonDocument<64> doc;
      DeserializationError err = deserializeJson(doc, buf);

      if(!err){
        const char* data = doc["d"];
        int ldr = doc["l"]; 

        Serial.print("Data/Hora: "); Serial.println(data);
        Serial.print("Luminosidade: "); Serial.println(ldr);
        } else {
        Serial.print("Erro ao decodificar JSON: ");
        Serial.println(err.c_str());
        }

      }

}    