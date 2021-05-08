#include <PubSubClient.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "MartinRouterKing"
#define STAPSK  "Sesamsemmel2$"
#endif

const char* SSID = STASSID;
const char* PSK = STAPSK;
const char* MQTT_BROKER = "192.168.178.54";
 
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
const int doorOpenPin = 14;
const int bellExt = 5;
const int bellInt = 4;
volatile boolean bellExtFlag = false;
volatile boolean bellIntFlag = false;
 
void setup() {
    pinMode(bellExt, INPUT_PULLUP);
    pinMode(bellInt, INPUT_PULLUP);
    pinMode(doorOpenPin, OUTPUT);
    digitalWrite(doorOpenPin, HIGH);
    Serial.begin(115200);
    attachInterrupt(digitalPinToInterrupt(bellExt), setBellExtFlag, RISING);
    attachInterrupt(digitalPinToInterrupt(bellInt), setBellIntFlag, FALLING);
    interrupts();
    setup_wifi();
    ArduinoOTA.setHostname("ESP_Doorbell");
    ArduinoOTA.setPassword("Th!s1sL0ud");
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_FS
        type = "filesystem";
      }
  
      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);
}
 
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
 
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void openDoor() {
    //Serial.println("set low");
    digitalWrite(doorOpenPin, LOW);
    delay(200);
    //Serial.println("set high");
    digitalWrite(doorOpenPin, HIGH);
    
}

ICACHE_RAM_ATTR void setBellExtFlag() {
    
    Serial.print("Ext Bell!");
    bellExtFlag = true;
    //delay(10);
}

ICACHE_RAM_ATTR void setBellIntFlag() {
    
    Serial.print("Int Bell!");
    bellIntFlag = true;
    //delay(10);
}
 
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    Serial.println();
 
    msg[length] = '&#92;&#48;';
    Serial.println(msg);
 
    if(strcmp(msg,"openDoor;")==0){
      openDoor();
    }
}

 
void reconnect() {
    while (!client.connected()) {
        Serial.println("Reconnecting MQTT...");
        if (!client.connect("ESP8266Client")) {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
    client.subscribe("/home/doorbell");
    Serial.println("MQTT Connected...");
}
 
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    ArduinoOTA.handle();
    client.loop();
    if (bellExtFlag) {
      client.publish("/home/doorbell", "bellExt");
      bellExtFlag = false;
    }
    if (bellIntFlag) {
      client.publish("/home/doorbell", "bellInt");
      bellIntFlag = false;
    }
    
    delay(100);
}
