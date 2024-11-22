#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>  // Biblioteca para o sensor DHT22

// Configurações de WiFi
const char *SSID = "Wokwi-GUEST";  // Substitua por sua rede WiFi
const char *PASSWORD = "";  // Se necessário, coloque a senha

// Configurações de MQTT
const char *BROKER_MQTT = "test.mosquitto.org";
const int BROKER_PORT = 1883;
const char *ID_MQTT = "esp32_iot";
const char *TOPIC_PUBLISH_TEMP = "iot/temp";
const char *TOPIC_PUBLISH_HUM = "iot/hum";
const char *TOPIC_PUBLISH_ENERGY = "iot/energy";
const char *TOPIC_PUBLISH_SIMULATED = "iot/devices";

// Configuração do DHT22
#define PIN_DHT 12  // Pino de leitura do DHT22
DHTesp dht;

// Configuração do SCT013 (sensor de corrente)
#define PIN_SENSOR_CURRENT A0  // Pino analógico para o sensor de corrente

// Configuração do tempo de publicação
#define PUBLISH_DELAY 5000

// Variáveis globais
WiFiClient espClient;
PubSubClient MQTT(espClient);
unsigned long lastPublishTime = 0;

// Valores simulados de consumo de dispositivos
float fridgeConsumption = 100.0;   // Consumo da geladeira (em Watts)
float acConsumption = 1500.0;      // Consumo do ar-condicionado (em Watts)
float showerConsumption = 4000.0; // Consumo do chuveiro (em Watts)
float socketConsumption = 200.0;  // Consumo de uma tomada (em Watts)

// Protótipos de funções
void initWiFi();
void initMQTT();
void checkWiFiAndMQTT();
void reconnectMQTT();
float readCurrentSensor();
void publishSensorData();
void publishSimulatedConsumption();

void setup() {
  Serial.begin(115200);

  // Configuração dos pinos e do sensor
  dht.setup(PIN_DHT, DHTesp::DHT22);
  pinMode(PIN_SENSOR_CURRENT, INPUT);

  // Inicializa WiFi e MQTT
  initWiFi();
  initMQTT();
}

void loop() {
  checkWiFiAndMQTT();
  MQTT.loop();

  // Publica dados dos sensores periodicamente
  if (millis() - lastPublishTime >= PUBLISH_DELAY) {
    lastPublishTime = millis();
    publishSensorData();
    publishSimulatedConsumption();
  }
}

void initWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

void checkWiFiAndMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    initWiFi();
  }
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.println("Conectando ao MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado ao broker MQTT!");
    } else {
      Serial.println("Falha ao conectar. Tentando novamente em 2 segundos...");
      delay(2000);
    }
  }
}

float readCurrentSensor() {
  int sensorValue = analogRead(PIN_SENSOR_CURRENT);
  float voltage = sensorValue * (5.0 / 1023.0);  // Conversão para tensão (valores simulados)
  float current = (voltage - 2.5) / 0.066;       // Conversão para corrente (Exemplo com SCT013)
  return current;
}

void publishSensorData() {
  // Obtém os valores dos sensores simulados (temperatura e umidade)
  TempAndHumidity tempHum = dht.getTempAndHumidity();
  float current = readCurrentSensor();

  // Publica a temperatura simulada
  if (!isnan(tempHum.temperature)) {
    String tempPayload = String(tempHum.temperature);
    MQTT.publish(TOPIC_PUBLISH_TEMP, tempPayload.c_str());
    Serial.println("Temperatura publicada: " + tempPayload + " C");
  }

  // Publica a umidade simulada
  if (!isnan(tempHum.humidity)) {
    String humPayload = String(tempHum.humidity);
    MQTT.publish(TOPIC_PUBLISH_HUM, humPayload.c_str());
    Serial.println("Umidade publicada: " + humPayload + " %");
  }

  // Publica a corrente elétrica simulada
  String energyPayload = String(current);
  MQTT.publish(TOPIC_PUBLISH_ENERGY, energyPayload.c_str());
  Serial.println("Corrente elétrica publicada: " + energyPayload + " A");
}

void publishSimulatedConsumption() {
  // Monta uma mensagem com o consumo simulado dos dispositivos
  String simulatedPayload = 
    "Fridge:" + String(fridgeConsumption) + "W," +
    "AirConditioner:" + String(acConsumption) + "W," +
    "Shower:" + String(showerConsumption) + "W," +
    "Socket:" + String(socketConsumption) + "W";

  // Publica o consumo simulado
  MQTT.publish(TOPIC_PUBLISH_SIMULATED, simulatedPayload.c_str());
  Serial.println("Consumo simulado publicado:");
  Serial.println(simulatedPayload);
}
