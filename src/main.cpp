#include <Arduino.h>
// #define DEBUG_FRAME
#include "Frame.h"
#include <Adafruit_Sensor.h>
#include <time.h>
#include "DHT.h"
#include "Jeedom.h"
#include "JFlame.h"
#include "JFlux.h"
#include "JKeyLedBuz.h"

const char VERSION[] = "1.0.4";
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
    Serial.println(F("Mode jeeDom actionOpen."));
  isValveClosed = false;
  onChanged = true;
}
void actionClose() {
  if (cmd=='d')
    Serial.println(F("Mode jeeDom actionClose."));
  isValveClosed = true;
  onChanged = true;

}
void actionReset() {
  if (cmd=='d')
    Serial.println(F("Mode jeeDom actionReset."));
  cntFluxBadSec = 0;
  isValveClosed = false;
  onChanged = true;

}
void actionResetTotal() {
  if (cmd=='d')
    Serial.println(F("Mode jeeDom actionResetTotal."));
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

const char* wifiStatus(int err) {
  switch (err) {
    case 0: return "WL_IDLE_STATUS"; break;
    case 1: return "WL_NO_SSID_AVAIL"; break;
    case 2: return "WL_SCAN_COMPLETED"; break;
    case 3: return "WL_CONNECTED"; break;
    case 4: return "WL_CONNECT_FAILED"; break;
    case 5: return "WL_CONNECTION_LOST"; break;
    case 6: return "WL_DISCONNECTED"; break;
  }
  String ret = "wifi_Err="+String(err);
  return ret.c_str();
}

const char* httpStatus(int err) {
  switch (err) {
    case -1  : return "CONNECTION_REFUSED"; break;
    case -2  : return "SEND_HEADER_FAILED"; break;
    case -3  : return "SEND_PAYLOAD_FAILED"; break;
    case -4  : return "NOT_CONNECTED"; break;
    case -5  : return "CONNECTION_LOST"; break;
    case -11 : return "READ_TIMEOUT"; break;
    case 200 : return "OK"; break;
  }
  String ret = "http_Code="+String(err);
  return ret.c_str();
}

// WatchDog:  wdCounter is set to 0 at (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15)
//            otherwise after 15 minutes ESP is restarted
uint32_t wdCounter = 0;
void watchdog(void *pvParameter) {
  while (1) {
    vTaskDelay(5000/portTICK_RATE_MS);
    wdCounter++;
    if (wdCounter > 180) {
      // We have a problem no connection if crash ....
      if (wdCounter == 181 ) {
        Serial.printf("%s Wifi.Status=%s       \r\n",  getDate().c_str(), wifiStatus(WiFi.status()) );
        Serial.printf("%s wdCounter:%d REBOOT...\n\r", getDate().c_str(), wdCounter);
      } else {
        // Perhapse force ??? WiFi.begin(ssid, password);
        ESP.restart(); // Restart after 5sec * 180 => 15min
        delay(2000);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("Start setup Ver:"); Serial.println(VERSION);
  // Start my WatchDog
  xTaskCreate(&watchdog, "wd task", 2048, NULL, 5, NULL);
  // Set pin mode  I/O Directions
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  keyLedBuz.rgb = 0xFFFFFF; // Wait Wifi
  // Start framework
  Serial.print("Start "); Serial.println(FrameVersion);
  frame_setup();
  keyLedBuz.rgb = 0x777777;  // WifiOK
  // Start jeedom_ok
  Serial.print("Start "); Serial.println(JeedomVersion);
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
  Serial.printf("%s  End setup. Running...\r\n", getDate().c_str());
}

#define SEND2JEEDOM(na,wc,rj,id,va) { if (wc == WL_CONNECTED && rj == HTTP_CODE_OK) { rj = jeedom.sendVirtual(id, va); if (rj != HTTP_CODE_OK) { Serial.printf("%s %s Jeedom(id:%d) error (%s)  \n\r", getDate().c_str(), na, id, httpStatus(rj)); }}}

// Main loop -----------------------------------------------------------------
void loop() {
  while (Serial.available() > 0) {
    uint8_t c = (uint8_t)Serial.read();
    if (c != 13 && c != 10 ) {
      cmd = c;
    } else {
      if (c==13) {
        if (cmd=='h') { Serial.println(); Serial.println("- Help info: r=reboot i=myip d=debug o=OneShot");}
        else if (cmd=='r') { ESP.restart(); }
        else if (cmd=='i') { Serial.printf("Heap:%u IP:%s \n\r",ESP.getFreeHeap(), WiFi.localIP().toString().c_str() ); }
        else if (cmd=='d') { Serial.println("Mode debug active."); }
        else if (cmd=='o') { Serial.println("Mode One shot display."); }
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
    int retJeedom = HTTP_CODE_OK;
    int wifistat = WiFi.status();

    // if wifi is down, try reconnecting every 60 seconds
    if (wifistat != WL_CONNECTED) {
      wifiLost++;
      if (wifiLost==1) Serial.printf("%s WiFi connection is lost(%s). wifiCnt:%d jeeErrCnt:%d\n\r",getDate().c_str(), wifiStatus(wifistat), wifiLost, jeedom.getErrorCounter());
      else Serial.printf(".");
      if (wifiLost == 30 ) {
        saveConfigJeedom = true;
      }
      if (wifiLost == 60) {
        if (WiFi.reconnect()) {
          Serial.printf("%s WiFi reconnect OK after 60s (%s). \n\r",getDate().c_str(), wifiStatus(wifiLost));
        }
        wifiLost = 0;
      }
    } else {
      if (wifiLost>0) Serial.printf("\n\r");
      wifiLost = 0;
    }

    // Valve state analsis
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
      } else {
        keyLedBuz.rgb = 0x007700;
        if (cntFluxBadSec>0) cntFluxBadSec--;
      }
    } else {
      // valve is closed Red is selected
      keyLedBuz.rgb = 0x770000;
    }

    // Action USER valve And Jeedom notify
    if (keyLedBuz.setValve(isValveClosed)) {
      SEND2JEEDOM("Set valve", wifistat, retJeedom, idValve, !isValveClosed);
      onChanged = true;
    }

    // Boiler is changed
    if (flame.isChanged(&timeinfo, 1000)) { // hysteresis = 1000/10 * 2
      SEND2JEEDOM("JFlame.isChanged", wifistat, retJeedom, idPower, flame.flamePerCent);
      SEND2JEEDOM("JFlame.isChanged", wifistat, retJeedom, idFlam,  flame.state);
      Serial.printf("%s JFlame.isChanged(Fl_D:%s Fl_A:%u Fl_100:%.1f%%) JeeDom:%s \n\r", getDate().c_str(), ((flame.state==1)?("On "):("Off")), flame.value, flame.flamePerCent, httpStatus(retJeedom));
    }

    if (flame.state)  keyLedBuz.rgb2 = 0xFF7700; // change flash by red
    else keyLedBuz.rgb2 = 0x0;

    // every 3 hours. update Gaz power & Water counter & record jeedom config if changed
    boolean quater = ( (timeinfo.tm_hour % 3 == 0) && (timeinfo.tm_min == 0) && (timeinfo.tm_sec == 0));
    if ( onChanged || quater ) {
      jeedom.config.waterM3 = ((float)flux.interruptCounter/(jeedom.config.fluxReference * 1000) );
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitA, flux.literPerMinute);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitD, !flux.state);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idValve,  !isValveClosed);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitT, jeedom.config.waterM3);
      if (onChanged)
        Serial.printf("%s JFlux.isChanged(%s)->irqCount:%llu  JeeDom:%s \n\r",getDate().c_str(), ((flux.state)?("On  "):("Off ")) ,flux.interruptCounter, httpStatus(retJeedom));
      if ( quater && jeedom.isCcrChanged()) saveConfigJeedom = true;
      if (cmd=='d') {
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

    // Every 5 minutes record T and H
    if ( (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15) ) {
      wdCounter = 0; // Reset WD
        if (getDHTTemperature())
          SEND2JEEDOM("DHT.Temp.", wifistat, retJeedom, idTemp, temperatureDHT);
        if (retJeedom == HTTP_CODE_OK && getDHTHumidity())
          SEND2JEEDOM("DHT.Hum.", wifistat, retJeedom, idHumi, humidityDHT);
      if (cmd=='d') {
        Serial.printf("%s Opt Heap:%u Jee:%d Temp:%.1f°C Hum:%.1f%% \n\r",getDate().c_str(), ESP.getFreeHeap(), retJeedom, temperatureDHT, humidityDHT);
      }
      if (  retJeedom != HTTP_CODE_OK ) {
        saveConfigJeedom = true;
      }
    }

    if ( cmd=='o' ) {
      cmd = ' ';
      Serial.printf("%s OneShot Wifi:%d Flux Ref:%.1f%% l/m:%.1f Eau:%s valve:%s Fx:%.3fm3 bad:%dsec. Flame:%s rgb:0x%X rgb2:0x%X wdCounter:%d \n\r",
                    getDate().c_str(), wifistat,
                    jeedom.config.fluxReference,
                    flux.literPerMinute,
                    (flux.state)?("On "):("Off"),
                    (isValveClosed)?("Close"):("Open "),
                    ((float)flux.interruptCounter/(jeedom.config.fluxReference * 1000)),
                    cntFluxBadSec,
                    (flame.state)?("On "):("Off"),
                    keyLedBuz.rgb,
                    keyLedBuz.rgb2,
                    wdCounter);
    }

    // Optional action
    if (saveConfigJeedom ) {
      boolean scjd = jeedom.saveConfigurationJeedom();
      saveConfigJeedom = false;
      if ( (cmd=='d') && scjd) Serial.printf("%s Configuration Jeedom file has been saved. \n\r", getDate().c_str());
    }

  } // End second
}
