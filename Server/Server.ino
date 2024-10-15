#include <WiFi.h>            /* WiFi library for ESP32 */
#include <Wire.h>
#include <PubSubClient.h>
#include <MQUnifiedsensor.h>

// MQ 135 Setup
#define LED_PIN 2
#define placa "ESP 32"
#define Voltage_Resolution 5
#define pin 34 //Analog input 0 of your arduino
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 12 // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm  

double CO2 = (0);
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

// WiFi Setup
#define wifi_ssid "random_ass_network"
#define wifi_password "Black#111"
#define mqtt_server "192.168.0.149"

#define sensor_topic "esp32/sensor"

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  pinMode(LED_PIN,OUTPUT);
  
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.862);

  client.setServer(mqtt_server, 1883);

  MQ135.init(); 
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); 
    while(1);
  }
  if(calcR0 == 0) {
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); 
    while(1);
  }
  MQ135.serialDebug(true);

}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  digitalWrite(LED_PIN,LOW);

  delay(5000);

  MQ135.update();
  CO2 = MQ135.readSensor();
  Serial.print("CO2 = ");
  Serial.println(CO2);

  String payload = "{\"co2\":" + String(CO2) + "}";
  Serial.print("Publishing payload: ");
  Serial.println(payload);

  client.publish(sensor_topic, payload.c_str(), true);
}
