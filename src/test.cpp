
#if false

#include <Arduino.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd;

void setup() {

  Serial.begin(115200);
  lcd.begin(20, 4);
  lcd.clear();

}

void loop() {

  if(Serial.available()) {
    int d = Serial.parseInt();
    for(int i = 0; i < 4; i++) {
      lcd.noBacklight();
      delay(d);
      lcd.backlight();
      delay(d);
    }
  }

}

#endif

#if false

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#define WIFI_SSID "WLAN_18E1D0"
#define WIFI_PASS "R7gaQ3P25Nf8"

AsyncWebServer web_server(80);
AsyncWebSocket ws("/esc");


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.println((char*)data);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
    
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}




void setup() {

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED);
  Serial.println("Connected");

  web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", "Pozdrav svijete", NULL);
  });

  MDNS.begin("esc");
  MDNS.addService("http", "tcp", 80);

  ws.onEvent(onEvent);
  web_server.addHandler(&ws);

  web_server.begin();

}

int64_t t1, t2;

void loop() {

  t2 = millis();
  if(t2 - t1 >= 4000) {
    t1 = t2;

    ws.textAll("Dobar dan, kako si ti meni? Ja sam super! ;)");
  }

}

#endif

// working
#if false

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1); // The default android DNS
DNSServer dnsServer;
WiFiServer server(80);

String responseHTML = ""
  "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
  "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
  "be redirected here.</p></body></html>";

void setup() { 

  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-DNSServer");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);
  MDNS.begin("esc");
  MDNS.addService("http", "tcp", 80);

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(responseHTML);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}

#endif


#if false

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

#define AP_WIFI_SSID "ESP32 WiFi TEST"

AsyncWebServer web_server(80);
DNSServer dns_server;

void on_ap_start(WiFiEvent_t event, WiFiEventInfo_t info) {

}

void setup() {

  Serial.begin(115200);

  //WiFi.onEvent(on_ap_start, SYSTEM_EVENT_AP_STACONNECTED);

  web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", "Pozdrav svijete", NULL);
  });

  WiFi.softAP(AP_WIFI_SSID, NULL);
  web_server.begin();

  dns_server.start(53, "*", IPAddress(192, 168, 1, 1));
  MDNS.begin("esp32");
  MDNS.addService("http", "tcp", 80);

  IPAddress local_ip(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(local_ip, local_ip, subnet);

}

void loop() {
  dns_server.processNextRequest();

}

#endif

#if false

#include <Arduino.h>

void setup() {

  Serial.begin(115200);

}

void loop() {

    if(Serial.available()) {

        String data = Serial.readStringUntil('\n');

        int i = data.indexOf('=');
        String d1 = data.substring(0, i - 1);
        int d2 = data.substring(i + 2).toInt();

        if(d1 == "led") {
            Serial.printf("Set LED brightness to %d\n", d2);
        }else if(d1 == "sample avg") {
            Serial.printf("Set sample average to %d\n", d2);
        }else if(d1 == "led mode") {
            Serial.printf("Set LED mode to %d\n", d2);
        }else if(d1 == "sample rate") {
            Serial.printf("Set sample rate to %d\n", d2);
        }else if(d1 == "pulse width") {
            Serial.printf("Set pulse width to %d\n", d2);
        }else if(d1 == "adc range") {
            Serial.printf("Set ADC range to %d\n", d2);
        }


    }
    /* 
    byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
  
    */

  Serial.print(" R[");
  Serial.print(sensor.getRed());
  Serial.print("] IR[");
  Serial.print(sensor.getIR());
  Serial.print("] G[");
  Serial.print(sensor.getGreen());
  Serial.print("]");

  Serial.println();

}

#endif

#if false

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // SDA 18
  // SCL 19

  Wire1.setPins(18, 19);
  // Initialize sensor
  if (!particleSensor.begin(Wire1, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();
}
#endif
