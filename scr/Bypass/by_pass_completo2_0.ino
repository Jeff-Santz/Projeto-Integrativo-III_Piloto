#include <RH_ASK.h>
#include <SPI.h> 

// Parte Wi-Fi e RTC
#include <time.h>
#include <WiFi.h>

#define us_para_s 1000000
#define tTimer 2 // Tempo desejado em segundos
#define BRT -3

#define PIN_RX 19 // Pin RX
#define PIN_TX 21 // Pin TX

// Configuracoes iniciais de cabeçalho
uint8_t thisAddress = 0x24; // end. oficial do ByPass
uint8_t idDestino = 0xFF;

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t buflen = sizeof(buf);

uint8_t fromTx = 0x23; 
uint8_t fromRx = 0x25; //0x24 para receptor e 0x23 para bypass

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
RTC_DATA_ATTR time_t horas[5];
int leitura_ldr = 0;

float mediaFinal = 0;
float tempoReenvio = 2; // Tempo necessário para os 3 reenvios + pequena espera

char dataHoraFormatada[64];
float medias[4];

char msg[64];
char dado[5];

// Instanciar o driver com a taxa de 20 00 bps -> 500 us
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
    int tentativa = 0, recebido = 0, tentativa_rec=0, contador=0;
    if (driver.recv(buf, &buflen)) {
      Serial.print("Recebi algo de: ");
      Serial.println(driver.headerFrom());
      // Delimita rementente
      if(driver.headerFrom() == fromTx){
        dataHora(); // Printa hora

        // Confere se dado é igual ao último recebido. Se for, descarta.
        if(strcmp((char*)buf, (char*)verificacao) != 0){
          strncpy((char*)verificacao, (char*)buf, buflen);
          verificacao[buflen] = '\0';
          // Senão,
          if(horasPegas<0){
            // Tempo necessário para a recepção estabilizar
            horasPegas++;
            podeDormir = false;
          } else if(horasPegas<5){
            // Define intervalo de recepção + inicia deepSleep
            time(&horas[horasPegas]);
            Serial.print("Tempo: ");
            Serial.println(horas[horasPegas]);
            horasPegas++;
          } else if(horasPegas==5){
            mediaFinal=0;
            for(int i = 0; i < 4; i++){
              medias[i] = difftime(horas[i+1], horas[i]);
            }

            for(int i = 0; i < 4; i++){
              mediaFinal = mediaFinal + medias[i];
            }

            mediaFinal = mediaFinal/4;
            Serial.print("Media Final: ");
            Serial.println(mediaFinal);
            Serial.println("");

            horasPegas++;
            podeDormir = true;
            
            // INICIA O DEEPSLEEP
            Durma();
          } 
          Serial.print("Horas pegas: ");
          Serial.println(horasPegas);

          buf[buflen] = '\0';
          Serial.print("Recebido2: ");
          Serial.println((char*)buf);

          driver.setHeaderFrom(thisAddress);
          driver.setHeaderTo(idDestino);
          while(recebido == 0 && tentativa < 3){
          // Enviando dado
          Serial.print("Enviando... -> tentativa : ");
          Serial.println(tentativa+1);

          driver.send(verificacao, strlen((char *)verificacao));
          driver.waitPacketSent();
          tentativa_rec=1;

          for (contador=0; contador <50 && tentativa_rec == 1; contador++){
            if (driver.recv(buf, &buflen)) {
              Serial.print ("Recebi algo de: ");
              Serial.println (driver.headerFrom());
              if(driver.headerFrom() == fromRx){
                buf[buflen] = '\0';
                Serial.print("Dado enviado com sucesso - ");
                Serial.println((char*)buf);

                driver.setHeaderFrom(thisAddress);

                recebido = 1;
                tentativa_rec=0;
                break;
                }
              }
            }
            if (recebido == 1) {
              break;
            }
            delay (200);
            tentativa++;
          }
            if(recebido == 1){
              Serial.println("Mensagem enviada com sucesso");
            } else{
              Serial.println("Envio nao confirmado");
            }
        } else{
          Serial.println("Dado repetido.");
        }
      }
    }


    if(podeDormir){
      horasPegas=5;
      podeDormir=false;
      esp_deep_sleep_start();
    }
}    
