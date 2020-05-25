/*
	Codigo desenvolvido como parte do projeto Smart Home
	Github: https://github.com/Smart-Home-SD
	NodeMCU detecta movimento a partir de um HC-SR04 e envia os dados para broker MQTT
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WifiUdp.h>

#define wifi_ssid "WIFI_SSID"
#define wifi_password "PASSWORD"
#define mqtt_server "localhost"  // MQTT Cloud address

#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "a.st1.ntp.br"

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

const int trigPin = 5;  //D1
const int echoPin = 4;  //D2
const int ledWifiPin = 2; //D4
const int ledMotionPin = 0; //D3

long duration, lastMsg = 0;
int distance, previousDistance;
bool motionDetected = false;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledWifiPin, OUTPUT);
  pinMode(ledMotionPin, OUTPUT);

  Serial.begin(9600);

  digitalWrite(ledWifiPin, LOW);
  digitalWrite(ledMotionPin, LOW);

  previousDistance = initDistance();
  
  setup_wifi();
  timeClient.begin();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
    delay(10);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("nodeMcuMotion")) {
            Serial.println("connected");
            digitalWrite(ledWifiPin, HIGH);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            digitalWrite(ledWifiPin, LOW);
            delay(5000);
        }
    }
}

int initDistance() {
  int duration1;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration1 = pulseIn(echoPin, HIGH);
  return duration1*0.034/2;
}

void loop() {

  if (!client.connected()) {
        reconnect();
  }
  timeClient.update();
  client.loop();

  long now;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance= duration*0.034/2;
  
  if ( abs(previousDistance - distance) > 50) {
    motionDetected = true;
    digitalWrite(ledMotionPin, HIGH);
  }

  // publishes every 5 seconds whether or not in that timeframe
  // motion was detected
  now = millis();
  
  if (now - lastMsg > 5000) {
    lastMsg = now;

    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JSONencoder["id"] = 2;
    JSONencoder["active"] = motionDetected == true ? 1 : 0;
    JSONencoder["timestamp"] = timeClient.getEpochTime();

    char JSONmessageBuffer[100];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    Serial.println(JSONmessageBuffer);
    
    if (client.publish("smarthome/motion", JSONmessageBuffer) == true) {
      Serial.println("Success sending message");
    } else {
      Serial.println("Error sending message");
    }
    motionDetected = false;
    digitalWrite(ledMotionPin, LOW);
  }
  delay(200);
}
