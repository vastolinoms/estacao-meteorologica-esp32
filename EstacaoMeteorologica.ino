// Estação Metereológica - Versao 2S/2024

// Credenciais do Blynk

#define BLYNK_TEMPLATE_ID "TMPL2_Qqltquc"
#define BLYNK_TEMPLATE_NAME "EstacaoMeteorologica"
#define BLYNK_AUTH_TOKEN "zaITYljeDaTaWU4mEfHxA1Py5QFHWOwj"
#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;

// Inclui bibliotecas
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <DHT.h>
#include <AS5600.h>
#include <SimpleTimer.h>

//Configuracao do Wi-Fi
#define WIFI_SSID "Vasto"
#define WIFI_PASS "p4ngu1nh4123"

//Define do pino do ESP para o sensor DHT11 (Umidade)
#define DHT_DATA_PIN 32
#define DHTTYPE DHT11
DHT dht(DHT_DATA_PIN, DHTTYPE);

// Define o pino do encoder (Rotacao)
#define EncPIN 35

// Sensor de pressao e temperatura (BMP280)
Adafruit_BMP280 bmp;

// Sensor de luminosidade (BH1750)
BH1750 lightMeter;

// Sensor de direcao do vento (Efeito Hall AS5600)
AS5600 as5600;

// Define os objetos timer para acionar as interrupções
SimpleTimer TimerLeituras;  // Define o intervalo entre leituras
SimpleTimer TimerEncoder;   // Define intervalo para ler contador

// Define a variável para armazenar o intervalo de amostragem (em milisegundos)
int IntLeituras = 7000;  // Faz uma leitura dos sensores a cada 7 segundos
int IntEncoder = 2000;   // Faz a totalizacao dos pulsos a cada 2 segundos

// Define as variáveis para armazenar os valores lidos
int Umidade = 0;
int Luminosidade = 0;
int Pressao = 0;
int Temperatura = 0;
int Velocidade = 0;
int Rotacao = 0;
int Direcao = 0;

int lastTemperatura = 0;
int lastUmidade = 0;
int lastLuminosidade = 0;
int lastPressao = 0;
int lastDirecao = 0;
int lastRotacao = 0;

// Variável para ler o status do encoder
bool EncSt = LOW;
bool EncAnt = LOW;

// Variável para armazenar a contagem de pulsos
int Cont = 0;

//Variável com o diametro do anemômetro
int DIAMETRO_ROTOR 0.1

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Configura o pino do encoder como entrada
  pinMode(EncPIN, INPUT);

  // Configura a interrupção para ser acionada a cada intervalo de amostragem
  TimerLeituras.setInterval(IntLeituras, RunLeituras);
  TimerEncoder.setInterval(IntEncoder, ContaPulsos);

  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startTime = millis();

  // Tenta conectar ao Wi-Fi com um limite de tempo de 1 minuto
  while (WiFi.status() != WL_CONNECTED) {
    // Verifica se já se passaram 60 segundos
    if (millis() - startTime >= 60000) {
      Serial.println("\n[WIFI] Timeout! Falha ao conectar.");
      WiFi.disconnect();
      return;  // Cancela a tentativa de conexão
    }
    Serial.print(".");
    delay(5000);  // Aguarda 5 segundos antes de tentar novamente
  }

  Serial.println("\n[WIFI] Conectado com sucesso!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Inicia DHT11
  dht.begin();

  // Inicia BH1750
  lightMeter.begin();

  // Inicia BMP280
  bmp.begin(0x76);

  // Inicia AS5600
  as5600.begin(4);                         //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.

  // Mensagem de conexão do Blynk
  Blynk.begin(auth, WIFI_SSID, WIFI_PASS);
}

void loop() {
  // Executa as tarefas programadas pelo timer
  TimerLeituras.run();
  TimerEncoder.run();
  Blynk.run();

  // Realiza a contagem de pulsos
  EncSt = digitalRead(EncPIN);
  if (EncSt == HIGH && EncAnt == LOW) {
    Cont++;
    EncAnt = HIGH;
  }
  if (EncSt == LOW && EncAnt == HIGH) {
    EncAnt = LOW;
  }
}

void RunLeituras() {
  Umidade = dht.readHumidity();
  Luminosidade = lightMeter.readLightLevel();
  Pressao = bmp.readPressure() / 100;   // Pressão em hPa
  Temperatura = dht.readTemperature();  // Temperatura do DHT

  // Lê o valor do sensor AS5600 e ajusta para que a posição inicial seja 0º
  Direcao = abs(((as5600.rawAngle() * 360.0 / 4095.0) - 85));  // Subtrai 85 para ajustar a posição inicial

  // Se a direção estiver negativa, ajusta para um valor positivo (0-360)
  if (Direcao < 0) {
    Direcao += 360;
  }

  // Exibe os valores no serial monitor
  Serial.print("Temperatura: ");
  Serial.print(Temperatura);
  Serial.println(" ºC");
  Serial.print("Umidade: ");
  Serial.print(Umidade);
  Serial.println(" %");
  Serial.print("Pressão: ");
  Serial.print(Pressao);
  Serial.println(" hPa");
  Serial.print("Luminosidade: ");
  Serial.print(Luminosidade);
  Serial.println(" lux");
  Serial.print("Velocidade: ");
  Serial.print(Velocidade);
  Serial.println(" km/h");
  Serial.print("Direção: ");
  Serial.print(Direcao);
  Serial.println(" graus");

  sendBlynk(Temperatura, lastTemperatura, V0);
  sendBlynk(Umidade, lastUmidade, V1);
  sendBlynk(Pressao, lastPressao, V2);
  sendBlynk(Luminosidade, lastLuminosidade, V3);
  sendBlynk(Velocidade, lastRotacao, V4);  
  sendBlynk(Direcao, lastDirecao, V5);
}


// Rotina para integralizar os pulsos e converter para rpm
void ContaPulsos() {
  Rotacao = ((Cont * 6)) * 1000.00 / IntEncoder;
  float circunferencia = PI * DIAMETRO_ROTOR;  // Circunferência em metros
  Velocidade = (Rotacao * circunferencia * 60) / 1000;  // Velocidade em km/h
  Cont = 0;
}


void sendBlynk(int data, int &lastData, int pin) {
  if (abs(data - lastData) > 2) {  // Ignora pequenas mudanças
    Blynk.virtualWrite(pin, data);
    lastData = data;
  }
}

