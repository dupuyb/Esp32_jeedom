#include <Arduino.h>
#define DEBUG_FRAME
#include "Frame.h"
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "Jeedom.h"
// DHT 11
DHT dht(13, DHT11);
// Internal led
#define EspLedBlue 2
long previousMillis  = 0;       // Use in loop

// Test webscoket
uint32_t value;

// Jeedom
Jeedom jeedom("192.168.1.117", 80, "tRcZTq9ceHKm7hJqqgY92wcF6di3EMuR");
// Devices
const char* idTemp = "1784";
const char* idHumi = "1785";


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[%u] Disconnected!\n\r", num);
			break;
		case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: [%s]\n\r", num, ip[0], ip[1], ip[2], ip[3], payload);
      // String ReponseHTML = String(value);
      // webSocket.sendTXT(num, ReponseHTML);
    	String ReponseHTML = String(value);
	    webSocket.sendTXT(num, ReponseHTML);
    }
		break;
		case WStype_TEXT:
    {
      Serial.printf("[%u] get Text: %s\n\r", num, payload);
      String _payload = String((char *) &payload[0]);
      if (payload[0] == '#')
          value = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode sendValue
      String ReponseHTML = String(value);
      webSocket.sendTXT(num, ReponseHTML);
    }
    break;
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n\r", length);
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}
}

void setup() {
  Serial.begin(115200);
  // Set pin mode
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  delay(5000);                     // If stay BLUE after 5 sec mode AccessPoint
  // Start framework
  frame_setup();
	// Start DHT11
	dht.begin();
}

// Main loop -----------------------------------------------------------------
uint32_t lc = 0;
void loop() {
	while (Serial.available() > 0) {
	  uint8_t c = (uint8_t)Serial.read();
	  if (c != 13) {
			if (c == 'r') ESP.restart();
      if (c == 'i') {Serial.print("IP:"); Serial.println( WiFi.localIP() ); }
		}
  }
  // Call frame loop
  frame_loop();
  // Is alive
  if ( millis() - previousMillis > 1000L) {
	  lc++;
    previousMillis = millis();
    digitalWrite(EspLedBlue, !digitalRead(EspLedBlue));
    // DHT read
		if ((lc & 0xF) == 0) {
		  float h = dht.readHumidity();
      float t = dht.readTemperature();
			if(!isnan(t) && !isnan(h)) {
			  int jr = jeedom.sendVirtual(idTemp, t);
			  if (jr==HTTP_CODE_OK) jr = jeedom.sendVirtual(idHumi, h);
			  Serial.printf("[%u] Heap:%u IP:%s Jee:%d Temp:%.1f°C Hum:%.1f%% \n\r", lc, ESP.getFreeHeap(), WiFi.localIP().toString().c_str(), jr, t, h);
		  }
	  }
  }
}
