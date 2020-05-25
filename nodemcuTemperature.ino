/*
	Codigo desenvolvido como parte do projeto Smart Home
	Github: https://github.com/Smart-Home-SD
	NodeMCU le temperatura de um LM35 e envia os dados para broker MQTT
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

const int ledWifiPin = 5; //D1

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

int outputpin= A0;

void setup() {
    pinMode(ledWifiPin, OUTPUT);
    
    Serial.begin(115200);
    
    digitalWrite(ledWifiPin, LOW);
    
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
        if (client.connect("nodeMcuTemperature")) {
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

bool checkBound(float newValue, float prevValue, float maxDiff) {
    return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

float temp = 0.0;
float diff = 0.01;

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    timeClient.update();
    client.loop();
        
    int analogValue = analogRead(outputpin);
    float millivolts = (analogValue/1024.0) * 3300; //3300 is the voltage provided by NodeMCU
    float newTemp = millivolts/10;
        
    if (checkBound(newTemp, temp, diff)) {
            temp = newTemp;
            
            StaticJsonBuffer<300> JSONbuffer;
            JsonObject& JSONencoder = JSONbuffer.createObject();
            JSONencoder["id"] = 1;
            JSONencoder["temperature"] = temp;
            JSONencoder["timestamp"] = timeClient.getEpochTime();

            char JSONmessageBuffer[100];
            JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
            
            Serial.print("New temperature:");
            Serial.println(JSONmessageBuffer);

            if (client.publish("smarthome/temperature", JSONmessageBuffer) == true) {
              Serial.println("Success sending message");
            } else {
                Serial.println("Error sending message");
            }
    }
    delay(5000);
}
