// Projeto dedicado a Global Solution 2025.2 baseado em ESP32 com sensor DHT22, display LCD I2C, buzzer e LED, comunicando via MQT
// Intuito do projeto é monitorar temperatura e umidade, exibindo mensagens recebidas via MQTT no display LCD, a fim de promover um melhor ambiente de trabalho.
// Integrantes:
/*
Enzo Amá Fatobene - RM: 562138

*/

// Bibliotecas necessárias: WiFi, PubSubClient, DHT, LiquidCrystal_I2C
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// Definições de rede e MQTT
const char* SSID = "Wokwi-GUEST";
const char* password = "";
const char* mqttBroker = "68.154.50.209";
const int port = 1883;
const char* topico_subs = "work/in";
const char* topico_mqtt = "work/out";

// Configuração LED com millis
#define led 4
unsigned long tempo = 0;
bool ledAceso = false;

// Configuração Buzzer com millis
#define buzzpin 19
bool buzzerAtivo = false;
unsigned long tempoBuzzer = 0;

// Inicialização dos objetos de cliente
WiFiClient espClient;
PubSubClient MQTT(espClient);

// Delcaração das variáveis relativas ao sensor DHT22
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Delcaração das variáveis relativas ao painel LCD I2Ct
#define LCDADDR 0x27
#define LCDColunas 20
#define LCDFileiras 4
LiquidCrystal_I2C lcd(LCDADDR, LCDColunas, LCDFileiras);

void setup() {
  // Setup das configurações iniciais, incluindo conexões WiFi e MQTT
  Serial.begin(115200);
  conectWifi(); // Função de conexão WiFi
  MQTT.setServer(mqttBroker, port);
  MQTT.setCallback(mqtt_callback);
  initAll(); // Função de inicialização dos componentes
}

void loop() {
  if(!MQTT.connected()){ //Caso a conexão MQTT seja perdida, tenta reconectar
    conectMqtt();
  }
  VerificaConexoesWiFIEMQTT(); // Verifica conexões WiFi e MQTT
  MQTT.loop(); // Mantém a conexão MQTT ativa
  publish(); // Publica os dados do sensor DHT22
  delay(2000); // Aguarda 2 segundos entre as publicações
  unsigned long tempo_atual = millis(); // Gerenciamento do LED e Buzzer com millis

  if(!ledAceso && (tempo_atual - tempo >= 30000)){ // Acende o LED a cada 30 segundos
    digitalWrite(led, HIGH);
    ledAceso = true;
    tempo = tempo_atual;

    tone(buzzpin, 1000); // Ativa o buzzer por 1 segundo
    buzzerAtivo = true;
    tempoBuzzer = tempo_atual;
  }
  else if(ledAceso && (tempo_atual - tempo >= 5000)){ // Apaga o LED após 5 segundos
    digitalWrite(led, LOW);
    ledAceso = false;
    tempo = tempo_atual;
  }

  if(buzzerAtivo && (tempo_atual - tempoBuzzer >= 1000)){ // Desativa o buzzer após 1 segundo
    noTone(buzzpin);
    buzzerAtivo = false;
  }
}

void initAll(){ //Função de inicialização dos componentes
  dht.begin(); // Inicializa o sensor DHT22
  lcd.init(); // Inicializa o display LCD I2C
  lcd.backlight(); // Ativa a luz de fundo do LCD
  pinMode(led, OUTPUT); // Configura o pino do LED como saída
}

void conectWifi(){ // Função de conexão WiFi
  if (WiFi.status() == WL_CONNECTED) // Verifica se já está conectado, caso positivo retorna
        return;
    WiFi.begin(SSID, password); // Inicia a conexão WiFi
    while (WiFi.status() != WL_CONNECTED) { // Aguarda até conectar
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Conectado com êxito a rede"); // Mensagem de sucesso
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

void conectMqtt(){ // Função de conexão MQTT
  while (!MQTT.connected()) { // Tenta conectar enquanto não estiver conectado
    Serial.println("Tentando conectar ao Broker...");
    if (MQTT.connect("ESP32Client")) {
      Serial.println("Conectado ao Broker"); // Sucesso na Reconexão
      MQTT.subscribe(topico_subs); // Tópico de recepção de mensagens
    } else {
      Serial.print("Falha, rc= ");
      Serial.println(MQTT.state()); // Mensagem da falha
      delay(2000);
    }
  }
}

void VerificaConexoesWiFIEMQTT() { // Função para verificar conexões WiFi e MQTT
    if (!MQTT.connected()){ // Verifica conexão MQTT
      conectMqtt(); // Conecta ao Borker noovamente
      conectWifi();
    }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) { // Função de callback para mensagens MQTT recebidas
  Serial.println("Mensagem do tópico: ");
  Serial.println(topic); // Confirmação do tópico recebido
  String msg; 
  for (int i = 0; i < length; i++) { // Converte o payload para array de caracteres
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);
  handleLCD(msg); // Chama a função para exibir a mensagem no LCD
}

void publish(){ // Função para publicar os dados do sensor DHT22 via MQTT
  float umidade = dht.readHumidity(); // Lê a umidade
  float temperatura = dht.readTemperature(); // Lê a temperatura em Celsius
  if (isnan(umidade) || isnan(temperatura)) { // Verifica se a leitura falhou
    Serial.println("Falha ao ler o sensor DHT22!");
    return;
  }

  String envio = "{"; // Formata os dados em JSON para envio via MQTT
  envio += "\"temperatura\": " + String(temperatura) + ",";
  envio += "\"umidade\": " + String(umidade) + ",";

  MQTT.publish(topico_mqtt, envio.c_str()); // Publica os dados no tópico MQTT
  Serial.println("Dados enviados: " + envio); // Confirmação dos dados enviados via Serial
}

void handleLCD(String mensagem) { // Função para exibir mensagens recebidas no display LCD I2C
  lcd.clear(); // Limpa o display antes de exibir a nova mensagem

  // Define o número máximo de colunas e linhas do LCD
  const int MAX_COLS = 20;
  const int MAX_ROWS = 4;

  // Divide a mensagem em partes que cabem no display e centraliza cada linha
  int start = 0;
  int linha = 0;

  while (start < mensagem.length() && linha < MAX_ROWS) { // Enquanto houver mensagem e linhas disponíveis

    String parte = mensagem.substring(start, start + MAX_COLS);

    parte.trim(); // Remove espaços em branco extras

    int espacos = (MAX_COLS - parte.length()) / 2;
    if (espacos < 0) espacos = 0;

    lcd.setCursor(espacos, linha); // Centraliza a parte da mensagem na linha atual
    lcd.print(parte); // Exibe a parte da mensagem no LCD

    start += MAX_COLS;
    linha++;
  }
}