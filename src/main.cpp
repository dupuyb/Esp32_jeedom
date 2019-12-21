#include <U8g2lib.h>
//#define DEBUG_FRAME
#include "Frame.h"
#include <Adafruit_Sensor.h>
#include <time.h>
#include "DHT.h"
#include "Jeedom.h"
#include "JFlame.h"
#include "JFlux.h"
#include "JKeyLedBuz.h"

const char VERSION[] = "2.1.3";
// Debug macro
//#define DEBUG_MAIN
#ifdef DEBUG_MAIN
#define DBXM(...) {Serial.print("[M]");Serial.print(__VA_ARGS__);}
#define DBXMLN(...) {Serial.print("[M]");Serial.println(__VA_ARGS__);}
#define DBXMF(...) {Serial.print("[M]");Serial.printf(__VA_ARGS__);}
#else
#define DBXM(...) {}
#define DBXMLN(...) {}
#define DBXMF(...) {}
#endif

// Oled 128x32 on my i2c
int cntOled = 0;
#define pinSDA  23
#define pinSCL  22
#define i2cADD  0x3c
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ pinSCL, /* data=*/ pinSDA);   // pin remapping with ESP8266 HW I2C
#define OLEDC() u8g2.clearBuffer(); 
#define OLEDS() u8g2.sendBuffer();
#define OLEDF(x,y,format, ...) { \
  char temp[80];\
  snprintf(temp, 80, format, __VA_ARGS__); \
  u8g2.setFontMode(1);	\
  u8g2.setFontDirection(0); \
  u8g2.drawUTF8(x, y, temp); \
}
#define OLEDI(x,y,format, ...) { \
  OLEDC(); \
  u8g2.setFont(u8g2_font_10x20_tf); \
  OLEDF( x, y,format,__VA_ARGS__); \
  OLEDS(); \
}

const uint8_t wifi_1[] = {
   0xfe, 0x1f, 0x01, 0x20, 0xf9, 0x27, 0x0d, 0x2c, 0xe3, 0x31, 0x19, 0x26,
   0xc5, 0x28, 0x31, 0x23, 0x09, 0x24, 0xc1, 0x20, 0xe1, 0x21, 0xe1, 0x21,
   0x01, 0x20, 0xfe, 0x1f};
const uint8_t wifi_0[] = {
   0xfe, 0x1f, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20,
   0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0xc1, 0x20, 0xe1, 0x21, 0xe1, 0x21,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t jeedom_0[] = {
     0xfe, 0x1f, 0x01, 0x20, 0xc5, 0x2c, 0xed, 0x2d, 0x19, 0x2b, 0xb1, 0x26,
   0x6d, 0x2d, 0xd5, 0x2a, 0xa9, 0x25, 0x79, 0x23, 0xe9, 0x26, 0xe9, 0x2d,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t jeedom_1[] = {
0xfe, 0x1f, 0x01, 0x20, 0xc1, 0x2c, 0xe1, 0x2d, 0x31, 0x2b, 0xd9, 0x26,
   0xed, 0x2d, 0xf5, 0x2b, 0xe9, 0x27, 0xf9, 0x27, 0xe9, 0x27, 0xe9, 0x23,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t flame_1[] = {  
   0xfe, 0x1f, 0x01, 0x20, 0x81, 0x21, 0xc1, 0x20, 0xc1, 0x2d, 0x99, 0x2d,
   0xb9, 0x27, 0xf1, 0x23, 0xf9, 0x26, 0x4d, 0x2c, 0x9d, 0x26, 0x39, 0x23,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t flame_0[] = {  
   0xfe, 0x1f, 0x01, 0x20, 0x01, 0x20, 0x99, 0x2d, 0x55, 0x22, 0xd5, 0x26,
   0x55, 0x22, 0x55, 0x22, 0x4d, 0x22, 0x01, 0x20, 0x01, 0x20, 0xa9, 0x2a,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t valve_1[] = {  
   0xfe, 0x1f, 0x01, 0x20, 0x81, 0x2e, 0xc1, 0x23, 0x41, 0x21, 0xe1, 0x23,
   0x1d, 0x2c, 0x03, 0x30, 0x01, 0x20, 0x03, 0x30, 0x3d, 0x2e, 0xc1, 0x21,
   0x01, 0x20, 0xfe, 0x1f };
const uint8_t valve_0[] = {  
      0xfe, 0x1f, 0x01, 0x20, 0x81, 0x2e, 0xc1, 0x23, 0xc1, 0x21, 0xe1, 0x23,
   0xfd, 0x2f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xfd, 0x2f, 0xc1, 0x21,
   0x01, 0x20, 0xfe, 0x1f };

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

String rebootTime;
// Date as europeen format
String getDate(bool sh = false){
  char temp[20];
  if (sh)
    snprintf(temp, 20, "%02d/%02d/%04d",
           timeinfo.tm_mday, (timeinfo.tm_mon+1), (1900+timeinfo.tm_year) );
  else
    snprintf(temp, 20, "%02d/%02d/%04d %02d:%02d:%02d",
           timeinfo.tm_mday, (timeinfo.tm_mon+1), (1900+timeinfo.tm_year),  timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
  return String(temp);
}

// DHT22 pin 13
#define pinDHT     13 // GPIO13
DHT dht(pinDHT, DHT22, 20);
float temperatureDHT = -1;
float humidityDHT = -1;

// Boiler flame
#define pinFlameAo 36 // ADC1_0 A
JFlame flame(pinFlameAo);

// FLux detector
float tmpLiterPerMinute;          // On change on liter per minute
uint16_t cntFluxBadSec = 0;       // Count how many seconds the flux is flowing
boolean isValveClosed  = false;   // Valve must be closed or opened
// IRQ
#define interruptPin 34
JFlux flux(interruptPin);

// Relay Valve Button LEd pins
#define pinVD 14 // Valve Direction
#define pinBz 18 // Buzzer
#define pinU 5  // Button Up
#define pinD 17 // Button Down

#define pinR 16 // Led Red
#define pinG 04 // Led Green
#define pinB 15 // Led blue
JKeyLedBuz keyLedBuz(pinR, pinG, pinB, pinU, pinD, pinVD, pinBz);
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
#define JEEDOM_DISLABED
#ifdef JEEDOM_DISLABED
#define SEND2JEEDOM(na,wc,rj,id,va) 
#else
#define SEND2JEEDOM(na,wc,rj,id,va) { \
  if (wc == WL_CONNECTED && rj == HTTP_CODE_OK) { \
    rj = jeedom.sendVirtual(id, va); \
    if (rj != HTTP_CODE_OK) { \
      DBXMF("%s %s Jeedom(id:%d) error (%s)  \n\r", getDate().c_str(), na, id, httpStatus(rj)); \
    }\
  }\
}
#endif

// Test webscoket
uint32_t value;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      DBXMF("[%u] Disconnected!", num);
      break;
    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      DBXMF("[%u] Connected from %d.%d.%d.%d url: [%s]", num, ip[0], ip[1], ip[2], ip[3], payload);
      String ReponseHTML = String(value);
      webSocket.sendTXT(num, ReponseHTML);
    }
      break;
    case WStype_TEXT:
    {
      DBXMF("[%u] get Text: %s", num, payload);
      String _payload = String((char *) &payload[0]);
      if (payload[0] == '#')
        value = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode sendValue
      String ReponseHTML = String(value);
      webSocket.sendTXT(num, ReponseHTML);
    }
      break;
    case WStype_BIN:
      DBXMF("[WSc] get binary length: %u", length);
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
    DBXMLN(F("Mode jeeDom actionOpen."));
  isValveClosed = false;
  onChanged = true;
}
void actionClose() {
  if (cmd=='d')
    DBXMLN(F("Mode jeeDom actionClose."));
  isValveClosed = true;
  onChanged = true;

}
void actionReset() {
  if (cmd=='d')
    DBXMLN(F("Mode jeeDom actionReset."));
  cntFluxBadSec = 0;
  isValveClosed = false;
  onChanged = true;

}
void actionSetTotal(uint64_t val) {
  if (cmd=='d')
    DBXMLN(F("Mode jeeDom actionSetTotal="+val));
  flux.interruptCounter = val;
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
    else if (srvcmd=="total") { // Msg cmd=total value=m3 in float   http://192.168.1.19/jeedom?cmd=total&value=33.967
      if (srvval!="") actionSetTotal( (uint64_t) (srvval.toFloat()*1000.0) );
      else ret = String( (int)flux.interruptCounter);
    }
    else if (srvcmd=="action" ) { // Msg cmd=action
      if (srvval=="open") actionOpen();
      else if (srvval=="close") actionClose();
      else if (srvval == "reset") actionReset();
    }
    if (jeedom.isCcrChanged()) saveConfigJeedom = true;
    if (cmd=='d' || cmd=='f')
      DBXMF("%s Jeedom srvcmd:%s srvval:%s fluxRef.=%.1f oDelay=%.1f \n\r",((srvval!="")?("Set"):("Get")), srvcmd.c_str(), srvval.c_str(), jeedom.config.fluxReference, jeedom.config.openDelay);
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
    DBXMF("%s DHT ERROR I2C RET:%d \n\r", getDate().c_str(), ret);
    return ret;
  }
  if (temperatureDHT!=t) ret = true;
  temperatureDHT=t;
  return ret;
}

// WatchDog:  wdCounter is set to 0 at (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15)
//            otherwise after 15 minutes ESP is restarted
uint32_t wdCounter = 0;
/*
void watchdog(void *pvParameter) {
  while (1) {
    vTaskDelay(5000/portTICK_RATE_MS);
    wdCounter++;
    if (wdCounter > 180) {
      // We have a problem no connection if crash ....
      if (wdCounter == 181 ) {
        DBXMF("%s Wifi.Status=%s       \r\n",  getDate().c_str(), wifiStatus(WiFi.status()) );
        DBXMF("%s wdCounter:%d REBOOT...\n\r", getDate().c_str(), wdCounter);
      } else {
        // Perhapse force ??? WiFi.begin(ssid, password);
        ESP.restart(); // Restart after 5sec * 180 => 15min
        delay(2000);
      }
    }
  }
}
*/

//  configModeCallback callback when entering into AP mode
void myConfigModeCallback (WiFiManager *myWiFiManager) {
  // Clear OLED 
  OLEDC();
  u8g2.setFont(u8g2_font_7x14_tf);
  OLEDF( 0, 10, "AP: %s", myWiFiManager->getConfigPortalSSID().c_str()); 
  OLEDF( 0, 31, "IP: %s", WiFi.softAPIP().toString().c_str()); 
  OLEDS();
}

void setup() {
  Serial.begin(115200);
  DBXMF("Start setup Ver:%s",VERSION);
  // Start Oled 128x32
  u8g2.begin();
  OLEDC();
  u8g2.setFont(u8g2_font_10x20_tf);
  OLEDF( 0, 14, "Survey:%s", VERSION);
  OLEDS();
  delay(2000);
  // Start my WatchDog
  // xTaskCreate(&watchdog, "wd task", 2048, NULL, 5, NULL);
  // Set pin mode  I/O Directions
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  keyLedBuz.rgb = 0xFFFFFF; // Wait Wifi
  // Start framework
  frame_setup(   myConfigModeCallback );
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
  isValveClosed = !jeedom.config.valveOpen;
  // Start
  getLocalTime(&timeinfo);
  // test 
  DBXMF("Heap:%u IP:%s MAC:%s \n\r",ESP.getFreeHeap(), WiFi.localIP().toString().c_str() , WiFi.macAddress().c_str());

  rebootTime = getDate(true);
}

// Main loop -----------------------------------------------------------------
void loop() {
#ifdef DEBUG_MAIN
  while (Serial.available() > 0) {
    uint8_t c = (uint8_t)Serial.read();
    if (c != 13 && c != 10 ) {
      cmd = c;
    } else {
      if (c==13) {
        if (cmd=='h') { Serial.println(); Serial.println("- Help info: r=reboot i=myip d=debug t=traceChgt o=OneShot s=saveConfig");}
        else if (cmd=='r') { ESP.restart(); }
        else if (cmd=='i') { Serial.printf("Heap:%u IP:%s \n\r",ESP.getFreeHeap(), WiFi.localIP().toString().c_str() ); }
        else if (cmd=='d') { Serial.println("Mode debug active."); }
        else if (cmd=='t') { Serial.println("Mode trace onCahnge active."); }
        else if (cmd=='o') { Serial.println("Mode One shot display."); }
        else if (cmd=='s') { Serial.println("Mode save config."); jeedom.saveConfigurationJeedom(); cmd=' ';}
        else { Serial.printf("Stop serial: %s \n\r",VERSION); }
      }
    }
  }
#endif
  // Call flux loop
  flux.loop();
  // Call frame loop
  frame_loop();
  // Call key led RGB animation Key repeat = 200ms
  buttonPressed = keyLedBuz.getKey(200);
  if (buttonPressed != 0) {
    if (cmd == 'd') {
      DBXMF("Button ID:%d detected\n\r",  buttonPressed);
    }
    // Action on keyboard
    if (buttonPressed == 1) { // UP
      cntFluxBadSec=0;
      isValveClosed = false;
    } else if (buttonPressed==2) { // Down
      cntFluxBadSec=0;
      isValveClosed = true;
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
      if (wifiLost==1) {
        DBXMF("%s WiFi connection is lost(%s). wifiCnt:%d jeeErrCnt:%d localIP:%s\n\r",getDate().c_str(), wifiStatus(wifistat), wifiLost, jeedom.getErrorCounter(), WiFi.localIP().toString().c_str());
      }
      else { 
        DBXMF("."); 
      }
      if (wifiLost == 30 ) {
        saveConfigJeedom = true;
      }
      // if (wifiLost == 50)   WiFi.disconnect();
      if (wifiLost == 60) {
        if (WiFi.reconnect()) {
          DBXMF("%s WiFi reconnect OK after 60s (%s). \n\r",getDate().c_str(), wifiStatus(wifistat));
          wifistat = WL_CONNECTED;
        }
        wifiLost = 0;
      }
    } else {
      if (wifiLost>0) {
        DBXMF("\n\r");
      }
      wifiLost = 0;
    }
    // Valve state analysis
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
      if (cmd=='t') {
         DBXMF("%s JFlame.isChanged(Fl_D:%s Fl_A:%u Fl_100:%.1f%%) JeeDom:%s \n\r", 
                        getDate().c_str(), 
                        ((flame.state==1)?("On "):("Off")), 
                        flame.value, 
                        flame.flamePerCent, 
                        httpStatus(retJeedom));
      }
    }
    if (flame.state)  {
      keyLedBuz.rgb2 = 0xFF7700; // change flash by red
    }
    else keyLedBuz.rgb2 = 0x0;
    if (flux.literPerMinute!=tmpLiterPerMinute) {
      tmpLiterPerMinute=flux.literPerMinute;
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitA, flux.literPerMinute);
    }
    // every 3 hours. update Gaz power & Water counter & record jeedom config if changed
    boolean quater = ( (timeinfo.tm_hour % 3 == 0) && (timeinfo.tm_min == 0) && (timeinfo.tm_sec == 0));
    if ( onChanged || quater ) {
      jeedom.config.waterM3 = ((float)flux.interruptCounter/(jeedom.config.fluxReference * 1000) );
      jeedom.config.valveOpen = !isValveClosed;
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitA, flux.literPerMinute);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitD, !flux.state);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idValve,  !isValveClosed);
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitT, jeedom.config.waterM3);
      if (onChanged && cmd=='t') {
        DBXMF("%s JFlux.isChanged(%s)->irqCount:%lu  JeeDom:%s \n\r",getDate().c_str(), ((flux.state)?("On  "):("Off ")) ,flux.interruptCounter, httpStatus(retJeedom));
      }
      if ( quater && jeedom.isCcrChanged())  {
        saveConfigJeedom = true;
      }
      if (cmd=='d') {
        DBXMF("%s 3h_|_OnCh l/m:%.1f Eau:%s valve:%s Fx:%.3fm3 onCh:%s JeeDom:%s \n\r",
                        getDate().c_str(),
                        flux.literPerMinute,
                        (flux.state)?("On "):("Off"),
                        (isValveClosed)?("Close"):("Open "),
                        jeedom.config.waterM3,
                        (onChanged)?("True "):("False"),
                        httpStatus(retJeedom));
      }
      onChanged = false;
    }
    // Every 5 minutes record T and H
    if ( (timeinfo.tm_min % 5==0) && (timeinfo.tm_sec == 15) ) {
      wdCounter = 0; // Reset WD
      if (getDHTTemperature()) {
        SEND2JEEDOM("DHT.Temp.", wifistat, retJeedom, idTemp, temperatureDHT);
      }
      if (retJeedom == HTTP_CODE_OK && getDHTHumidity()) {
        SEND2JEEDOM("DHT.Hum.", wifistat, retJeedom, idHumi, humidityDHT);
      }
      if (cmd=='d') {
        Serial.printf("%s Opt Heap:%u Jee:%d Temp:%.1f°C Hum:%.1f%% \n\r",getDate().c_str(), ESP.getFreeHeap(), retJeedom, temperatureDHT, humidityDHT);
      }
      if ( retJeedom != HTTP_CODE_OK ) {
        saveConfigJeedom = true;
      }
    }
    // one shot display
    if ( cmd=='o' ) {
      cmd = ' ';
      DBXMF("%s OneShot Wifi:%d Flux Ref:%.1f%% l/m:%.1f Eau:%s valve:%s Fx:%.3fm3 bad:%dsec. Flame:%s rgb:0x%X rgb2:0x%X wdCounter:%d \n\r",
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
      if ( (cmd=='d') && scjd) {
        DBXMF("%s Configuration Jeedom file has been saved. \n\r", getDate().c_str());
      }
    }

    // Set Oled dsp
    OLEDC();
    // Icons
    if (wifiLost==0) u8g2.drawXBMP(0,0,14,14,wifi_1);
    else u8g2.drawXBMP(0,0,14,14,wifi_0);
    if (retJeedom == HTTP_CODE_OK) u8g2.drawXBMP(16,0,14,14,jeedom_1);
    else u8g2.drawXBMP(16,0,14,14,jeedom_0);
    if (flame.state) u8g2.drawXBMP(32,0,14,14,flame_1);
    else u8g2.drawXBMP(32,0,14,14,flame_0);
    if (isValveClosed) u8g2.drawXBMP(48,0,14,14,valve_0);
    else u8g2.drawXBMP(48,0,14,14,valve_1);  
    // Text
    u8g2.setFont(u8g2_font_7x14_tf);
    OLEDF( 73,14, "%s", getTime().c_str());
    switch (cntOled) {
      case 0: OLEDF( 0, 31, "Temp:%2.0f°C Hum:%2.0f%%", temperatureDHT,humidityDHT); break;
      case 1: OLEDF( 0, 31, "Eau: %.3f m3", jeedom.config.waterM3); break;
      case 2: OLEDF( 0, 31, "Vanne: %s", (isValveClosed)?("Fermé"):("Ouverte") ); break;
      case 3: OLEDF( 0, 31, "Flamme: %s", (flame.state)?("Allumé"):("Eteinte") ); break;
      case 4: OLEDF( 0, 31, "Wifi:%s", WiFi.localIP().toString().c_str() ); break;
      case 5: OLEDF( 0, 31, "Mac: %s", WiFi.macAddress().c_str() ); break;
      case 6: OLEDF( 0, 31, "Jeedom: %s", ((retJeedom == HTTP_CODE_OK)?("est OK"):("Erreur")) ); break;
      case 7: OLEDF( 0, 31, "Date: %s", getDate(true).c_str() ); break;
      case 8: OLEDF( 0, 31, "Version: %s", VERSION ); break;
      case 9: OLEDF( 0, 31, "Reboot: %s", rebootTime.c_str() ); break;
      default: cntOled=0; break; 
    }
    OLEDS();
    if ( (timeinfo.tm_sec % 3)==0 ) {
       cntOled++;
    }
  } // End second
}
