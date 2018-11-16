#include <Arduino.h>
#define DEBUG_FRAME
#include "Frame.h"

#define EspLedBlue 2

long previousMillis  = 0;       // Use in loop
// Test webscoket
uint32_t value;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[%u] Disconnected!\n", num);
			break;
		case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: [%s]\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      // String ReponseHTML = String(value);
      // webSocket.sendTXT(num, ReponseHTML);
    }
		break;
		case WStype_TEXT:
    {
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String _payload = String((char *) &payload[0]);
      if (payload[0] == '#') {            // we get #
          value = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode sendValue
      }
      String ReponseHTML = String(value);
      webSocket.sendTXT(num, ReponseHTML);
    }
    break;
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
			// send data to server
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

//void sendWebSocket() {
  //  String str = String(value);
  //  webSocket.broadcastTXT(str);
//}

void showAH(){
	String m = "Nbr of args:";	m+=server.args();	m+="\n";
	for(int i = 0; i < server.args(); i++) {
		m+="Arg["+(String)i+"]=";		m+=server.argName(i)+":";	m+=server.arg(i)+"\n";
	}
	Serial.print(m);
	String mm = "Nbr of heders:";	mm+=server.headers();	mm+="\n";
	for(int i = 0; i < server.headers(); i++) {
		mm+="Header["+(String)i+"]=";		mm+=server.headerName(i)+":";	mm+=server.header(i)+"\n";
	}
	Serial.print(mm);
	Serial.print("hostHeader:");Serial.println(server.hostHeader());
}

// Arduino core

void setup() {
  Serial.begin(115200);
  // Set pin mode
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  delay(5000);                     // If stay BLUE after 5 sec mode AccessPoint
  // Start framework
  frame_setup();
}

// Main loop -----------------------------------------------------------------
void loop() {
  // Call frame loop
  frame_loop();
  // Is alive
  if ( millis() - previousMillis > 1000L) {
  //  sendWebSocket();
    previousMillis = millis();
    digitalWrite(EspLedBlue, !digitalRead(EspLedBlue));
  }
}
