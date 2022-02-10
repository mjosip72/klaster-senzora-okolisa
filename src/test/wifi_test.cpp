
#if false

#include "Test.h"
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

void _setup() {

	WiFi.disconnect(true);
	WiFi.begin("WLAN_18E1D0", "R7gaQ3P25Nf8");
  while(WiFi.isConnected() == false) delay(20);

  configTime(100, 200, "error");

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    /*String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);*/

    request->send(200, "text/plain", "OK");
  });

  server.begin();

}

void _loop() {

}

#endif
