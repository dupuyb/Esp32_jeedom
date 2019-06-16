#include <Arduino.h>
// #define DEBUG_FRAME
#include "Frame.h"
#include <Adafruit_Sensor.h>
#include <time.h>
#include "DHT.h"
#include "Jeedom.h"
#define DEBUG_FLAME
#include "JFlame.h"
#define DEBUG_FLUX
#include "JFlux.h"
#include "JKeyLedBuz.h"

const char VERSION[] = "Ver:0.7.2";
// Debug macro
#ifdef DEBUG_MAIN
  #define DBXM(...) Serial.print(__VA_ARGS__)
  #define DBXMLN(...) Serial.println(__VA_ARGS__)
#else
  #define DBXM(...)
  #define DBXMLN(...)
#endif

// Serial command
int8_t cmd;
int8_t wifiLost = 0;

// Time facilities
const long gmtOffset_sec     = 3600;
const int daylightOffset_sec = 3600;
struct tm timeinfo;            // time struct
const char* ntpServer        = "pool.ntp.org";

// Time HH:MM.ss
String getTime() {
  char temp[10];
  snprintf(temp, 20, "%02d:%02d:%02d", timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
  return String(temp);
}

// Date as europeen format
String getDate(){
  char temp[20];
  snprintf(temp, 20, "%02d/%02d/%04d %02d:%02d:%02d",
          timeinfo.tm_mday, (timeinfo.tm_mon+1), (1900+timeinfo.tm_year),  timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
  return String(temp);
}

// DHT22 pin 13
#define pinDHT     13 // GPIO13
DHT dht(pinDHT, DHT11, 20);
float temperatureDHT = -1;
float humidityDHT = -1;

// Boiler flame
#define pinFlameAo 36 // ADC1_0
JFlame flame(pinFlameAo);

// FLux detector
uint16_t cntFluxBadSec = 0;       // Count how many seconds the flux is flowing
boolean isValveClosed  = false;   // Valve must be closed or opened
// IRQ
#define interruptPin 34
JFlux flux(interruptPin);

// Relay Valve Button LEd pins
#define pinVD 12 // Valve Direction
#define pinVP 14 // valve Power
#define pinBz 18 // Buzzer
#define pinU 5  // Button Up
#define pinD 17 // Button Down

#define pinR 16 // Led Red
#define pinG 04 // Led Green
#define pinB 15 // Led blue
JKeyLedBuz keyLedBuz(pinR, pinG, pinB, pinU, pinD, pinVD, pinVP, pinBz);
uint8_t buttonPressed = 0; // 1=BtUp 2=BtDown 3=Both

// Internal led
#define EspLedBlue 2
long previousMillis = 0;

// Jeedom
Jeedom jeedom("/cfJeedom.json");
bool saveConfigJeedom = false;
// Devices from virtual jeedom
const int idTemp = 1784;
const int idHumi = 1785;
const int idFlam = 1786;
const int idPower = 1816;
const int idDebitA = 1811;
const int idDebitT = 1812;
const int idDebitD = 1823;
const int idValve = 1825;
bool onChanged = true;

// Test webscoket
uint32_t value;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			DBXMLN("[%u] Disconnected!", num);
			break;
		case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      DBXMLN("[%u] Connected from %d.%d.%d.%d url: [%s]", num, ip[0], ip[1], ip[2], ip[3], payload);
    	String ReponseHTML = String(value);
	    webSocket.sendTXT(num, ReponseHTML);
    }
		break;
		case WStype_TEXT:
    {
      DBXMLN("[%u] get Text: %s", num, payload);
      String _payload = String((char *) &payload[0]);
      if (payload[0] == '#')
          value = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode sendValue
      String ReponseHTML = String(value);
      webSocket.sendTXT(num, ReponseHTML);
    }
    break;
		case WStype_BIN:
			DBXMLN("[WSc] get binary length: %u", length);
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

// Action from JEEDOM
void actionOpen() {
  if (cmd=='d')
    Serial.println("Mode jeeDom actionOpen.");
  isValveClosed = false;
  onChanged = true;
}
void actionClose() {
  if (cmd=='d')
    Serial.println("Mode jeeDom actionClose.");
  isValveClosed = true;
  onChanged = true;

}
void actionReset() {
  if (cmd=='d')
    Serial.println("Mode jeeDom actionReset.");
  cntFluxBadSec = 0;
  isValveClosed = false;
  onChanged = true;

}
void actionResetTotal() {
  if (cmd=='d')
    Serial.println("Mode jeeDom actionResetTotal.");
  //flux.interruptCounter = 0;
  onChanged = true;
}

// received argument from Jeedom script via virutal
String handleJeedom() {
  String ret ="jeedom_ok";
  String srvcmd = server.arg("cmd");
  String srvval = server.arg("value");
  if (srvcmd!="") {
    if (srvcmd=="reference" ) { // Msg cmd=reference
      if (srvval!="") jeedom.config.fluxReference = srvval.toFloat(); // SET
      else ret = String(jeedom.config.fluxReference); // No value tag value => GET
    }
    else if (srvcmd=="delay" ) { // Msg cmd=delay
      if (srvval!="") jeedom.config.openDelay = srvval.toFloat(); // SET
      else ret = String(jeedom.config.openDelay);
    }
    else if (srvcmd=="action" ) { // Msg cmd=action
      if (srvval=="open") actionOpen();
      else if (srvval=="close") actionClose();
      else if (srvval == "reset") actionReset();
      else if (srvval=="resetTotal") actionResetTotal();
    }
    if (jeedom.isCcrChanged()) saveConfigJeedom = true;
    if (cmd=='d' || cmd=='f')
      Serial.printf("%s Jeedom srvcmd:%s srvval:%s fluxRef.=%.1f oDelay=%.1f \n\r",((srvval!="")?("Set"):("Get")), srvcmd.c_str(), srvval.c_str(), jeedom.config.fluxReference, jeedom.config.openDelay);
  }
  return ret;
}

bool getDHTHumidity(){
  bool ret = false;
  float h = dht.readHumidity();
  if (isnan(h)) return ret;
  if (humidityDHT!=h) ret = true;
  humidityDHT=h;
  return ret;
}

bool getDHTTemperature(){
  bool ret = false;
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.printf("%s DHT ERROR I2C RET:%d \n\r", getDate().c_str(), ret);
    return ret;
  }
  if (temperatureDHT!=t) ret = true;
  temperatureDHT=t;
  return ret;
}

// WatchDog:  wdCounter is set to 0 at (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15)
//            otherwise after 15 minutes ESP is restarted
uint32_t wdCounter = 0;
void watchdog(void *pvParameter) {
  while (1) {
    vTaskDelay(5000/portTICK_RATE_MS);
    wdCounter++;
    if (wdCounter > 180) {
      if (wdCounter == 181 ) Serial.printf("%s wdCounter:%d REBOOT...\n\r", getDate().c_str(), wdCounter);
      else ESP.restart(); // Restart after 5sec * 180 => 15min
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("Version:"); Serial.println(VERSION);
  // Start my WatchDog
  xTaskCreate(&watchdog, "wd task", 2048, NULL, 5, NULL);
  // Set pin mode  I/O Directions
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  keyLedBuz.rgb = 0xFFFFFF; // Wait Wifi
  // Start framework
  frame_setup();
  keyLedBuz.rgb = 0x777777;  // WifiOK
  // Start jeedom_ok
  jeedom.setup();
  server.on("/jeedom", [](){
     server.send(HTTP_CODE_OK, "text/plain", handleJeedom());
  });
	// Start DHT22
	dht.begin();
	// Init time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time
  wifiLost = 0;
  // IRQ
  flux.setup(jeedom.config.waterM3, jeedom.config.fluxReference);
  // Start
  getLocalTime(&timeinfo);
  Serial.printf("%s  Running...\r\n", getDate().c_str());
}

// Main loop -----------------------------------------------------------------
void loop() {
	while (Serial.available() > 0) {
	  uint8_t c = (uint8_t)Serial.read();
	  if (c != 13 && c != 10 ) {
      cmd = c;
    } else {
      if (c==13) {
        if (cmd=='h') { Serial.println(); Serial.println("- Help info: r=reboot i=myip d=debug j=jeedom s=save");}
			  else if (cmd=='r') { ESP.restart(); }
        else if (cmd=='i') { Serial.printf("Heap:%u IP:%s \n\r",ESP.getFreeHeap(), WiFi.localIP().toString().c_str() ); }
        else if (cmd=='d') { Serial.println("Mode debug active."); }
        else if (cmd=='j') { Serial.println("Mode jeeDom active."); }
        else if (cmd=='s') { Serial.println("Send jeeDom Widget."); }
        else { Serial.printf("Stop serial: %s \n\r",VERSION); }
      }
		}
  }

  // Call flux loop
  flux.loop();

  // Call frame loop
  frame_loop();

  // Call key led RGB animation Key repeat = 200ms
  buttonPressed = keyLedBuz.getKey(200);

  if (buttonPressed!=0) {
    if (cmd=='d')
      Serial.printf("Button ID:%d detected\n\r",  buttonPressed);
    // Action on keyboard
    if (buttonPressed==1) { // UP
      cntFluxBadSec=0;
      isValveClosed=false;
    } else if (buttonPressed==2) { // Down
      cntFluxBadSec=0;
      isValveClosed=true;
    }
  }

  // Is alive executed every 1 sec.
  if ( millis() - previousMillis > 1000L) {
    previousMillis = millis();
		getLocalTime(&timeinfo);
    digitalWrite(EspLedBlue, !digitalRead(EspLedBlue));
    int  retJeedom = 0;

    // Boiler is changed
    if (flame.isChanged(&timeinfo, 1000)) { // hysteresis = 1000/10 * 2
      retJeedom = jeedom.sendVirtual(idPower, flame.flamePerCent);
      retJeedom = jeedom.sendVirtual(idFlam,  flame.state);
      if (retJeedom!=HTTP_CODE_OK)
        Serial.printf("%s Flame ERROR Jeedom return:%d \n\r", getDate().c_str(), retJeedom);
      if (cmd=='j')
        Serial.printf("%s Flame Jee:%d Fl_D:%s Fl_A:%u Fl_100:%.1f%% \n\r", getDate().c_str(), retJeedom, ((flame.state==1)?("On "):("Off")), flame.value, flame.flamePerCent);
    }
    if (isValveClosed==false) {
      if (flux.isChanged(&timeinfo, jeedom.config.fluxReference)) {
        onChanged = true;
      }
      if (flux.state == true) {
        if ( (cntFluxBadSec / 60) < jeedom.config.openDelay) {
          cntFluxBadSec++;
          keyLedBuz.rgb = 0x0000FF;
        } else {
          keyLedBuz.rgb = 0xFF0000;
          isValveClosed = true;   // Close valve
        }
        onChanged = true;
      } else {
        keyLedBuz.rgb = 0x007700;
        if (cntFluxBadSec>0) cntFluxBadSec--;
      }
    } else {
      // valve is closed Red is selected
      keyLedBuz.rgb = 0x770000;
    }

    if (flame.state)  keyLedBuz.rgb2 = 0xFF7700; // change flash by red
    else keyLedBuz.rgb2 = 0x0;

    if ( cmd=='d' ) {
      Serial.printf("%s Flux Ref:%.1f%% l/m:%.1f Eau:%s valve:%s Fx:%.3fm3 bad:%dsec. Flame:%s rgb:0x%X rgb2:0x%X wdCounter:%d \n\r",
                    getDate().c_str(),
                    jeedom.config.fluxReference,
                    flux.literPerMinute,
                    (flux.state)?("On "):("Off"),
                    (isValveClosed)?("Close"):("Open "),
                    ((float)flux.interruptCounter/(jeedom.config.fluxReference * 1000)),
                    cntFluxBadSec,
                    (flame.state)?("On "):("Off"),
                    keyLedBuz.rgb,
                    keyLedBuz.rgb2,
                    wdCounter
                   );
    }

    // Action valve And notify
    if (keyLedBuz.setValve(isValveClosed)) {
      retJeedom = jeedom.sendVirtual(idValve, !isValveClosed);
      if (retJeedom!=HTTP_CODE_OK)
        Serial.printf("%s Action valve ERROR Jeedom return:%d \n\r", getDate().c_str(), retJeedom);
      onChanged = true;
    }

    if (cmd=='s') onChanged = true;
    // every 3 hours. update Gaz power & Water counter & record jeedom config if changed
    boolean quater = ( (timeinfo.tm_hour % 3 == 0) && (timeinfo.tm_min == 0) && (timeinfo.tm_sec == 0));
    if ( onChanged || quater ) {
      jeedom.sendVirtual(idDebitA, flux.literPerMinute);
      jeedom.sendVirtual(idDebitD, !flux.state);
      jeedom.sendVirtual(idValve,  !isValveClosed);
      // Probleme a voir il y a des entree en BD tous les 3 heures A TESTER aavec vannes fermé pendant + 3 heures
      jeedom.config.waterM3 = ((float)flux.interruptCounter/(jeedom.config.fluxReference * 1000) );
      jeedom.sendVirtual(idDebitT, jeedom.config.waterM3);
      if ( quater && jeedom.isCcrChanged()) saveConfigJeedom = true;
      if (cmd=='j' || cmd=='s') {
        Serial.printf("%s 3h or OnCh l/m:%.1f Eau:%s valve:%s Fx:%.3fm3 onCh:%s \n\r",
                                     getDate().c_str(),
                                     flux.literPerMinute,
                                     (flux.state)?("On "):("Off"),
                                     (isValveClosed)?("Close"):("Open "),
                                     jeedom.config.waterM3,
                                     (onChanged)?("True "):("False")
                                     );
      }
      onChanged = false;
    }

    // DHT read every 120 seconds
		if ( (timeinfo.tm_min % 2==0) && timeinfo.tm_sec == 30) {
      // if wifi is down, try reconnecting every 60 seconds
      if ((WiFi.status() != WL_CONNECTED) ) {
        wifiLost++;
        if (wifiLost == 2 ) {
          Serial.printf("%s WiFi connection is lost. cnt:%d jeeErrCnt:%d\n\r",getDate().c_str(), wifiLost, jeedom.getErrorCounter());
          saveConfigJeedom = true;
        }
        if (wifiLost == 4) {
          if (WiFi.reconnect()) {
            Serial.printf("%s WiFi reconnect OK (%d). \n\r",getDate().c_str(), wifiLost);
            wifiLost = 0;
          }
        }
      } else {
        // Test if Jeedom is Connected
        if(  jeedom.getErrorCounter()!= 0 ) {
          Serial.printf("%s WiFi connected but jeedom error jeeErrCnt:%d\n\r",getDate().c_str(), jeedom.getErrorCounter());
        }
      }
	  }

    // Every 5 minutes record T and H
    if ( (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15) ) {
      wdCounter = 0; // Reset WD
      if (getDHTTemperature())
        retJeedom = jeedom.sendVirtual(idTemp, temperatureDHT);
      if (retJeedom==HTTP_CODE_OK && getDHTHumidity())
        retJeedom = jeedom.sendVirtual(idHumi, humidityDHT);
      if (cmd=='d' || cmd =='j' || cmd =='i') {
        Serial.printf("%s Opt Heap:%u Jee:%d Temp:%.1f°C Hum:%.1f%% \n\r",getDate().c_str(), ESP.getFreeHeap(), retJeedom, temperatureDHT, humidityDHT);
      }
      if ( (retJeedom > 0) && (retJeedom != HTTP_CODE_OK) ) {
        saveConfigJeedom = true;
      }
    }

    // Optional action
    if (saveConfigJeedom ) {
      boolean scjd = jeedom.saveConfigurationJeedom();
      saveConfigJeedom = false;
      if ( (cmd=='d') && scjd) Serial.printf("%s Configuration Jeedom file has been saved. \n\r", getDate().c_str());
    }

  } // End second
}
