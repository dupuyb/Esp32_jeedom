
#include <U8g2lib.h>
// #define DEBUG_FRAME
// Frame wifi
#include "FrameWeb.h"
FrameWeb frame;

#include "HLog.h"
HLog hlog(100);

#include <time.h>
#include "Jeedom.h"
#include "JFlame.h"
#include "JFlux.h"
#include "JKeyLedBuz.h"
// Reset Reason 
#include <rom/rtc.h>

// LOGGER update 120 chars Max
#define LOG(format, ...) { \
  char temp[120];\
  snprintf(temp, 120, format, __VA_ARGS__); \
  hlog.append(temp); \
}  

const char VERSION[] ="2.8.0";
// Debug macro 
// #define DEBUG_MAIN

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

// Define Icons
const uint8_t wifi_1  [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0xf9, 0x27, 0x0d, 0x2c, 0xe3, 0x31, 0x19, 0x26, 0xc5, 0x28, 0x31, 0x23, 0x09, 0x24, 0xc1, 0x20, 0xe1, 0x21, 0xe1, 0x21, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t wifi_0  [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0xc1, 0x20, 0xe1, 0x21, 0xe1, 0x21, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t jeedom_0[] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0xc5, 0x2c, 0xed, 0x2d, 0x19, 0x2b, 0xb1, 0x26, 0x6d, 0x2d, 0xd5, 0x2a, 0xa9, 0x25, 0x79, 0x23, 0xe9, 0x26, 0xe9, 0x2d, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t jeedom_1[] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0xc1, 0x2c, 0xe1, 0x2d, 0x31, 0x2b, 0xd9, 0x26, 0xed, 0x2d, 0xf5, 0x2b, 0xe9, 0x27, 0xf9, 0x27, 0xe9, 0x27, 0xe9, 0x23, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t flame_1 [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0x81, 0x21, 0xc1, 0x20, 0xc1, 0x2d, 0x99, 0x2d, 0xb9, 0x27, 0xf1, 0x23, 0xf9, 0x26, 0x4d, 0x2c, 0x9d, 0x26, 0x39, 0x23, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t flame_0 [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0x01, 0x20, 0x99, 0x2d, 0x55, 0x22, 0xd5, 0x26, 0x55, 0x22, 0x55, 0x22, 0x4d, 0x22, 0x01, 0x20, 0x01, 0x20, 0xa9, 0x2a, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t valve_1 [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0x81, 0x2e, 0xc1, 0x23, 0x41, 0x21, 0xe1, 0x23, 0x1d, 0x2c, 0x03, 0x30, 0x01, 0x20, 0x03, 0x30, 0x3d, 0x2e, 0xc1, 0x21, 0x01, 0x20, 0xfe, 0x1f };
const uint8_t valve_0 [] PROGMEM = { 0xfe, 0x1f, 0x01, 0x20, 0x81, 0x2e, 0xc1, 0x23, 0xc1, 0x21, 0xe1, 0x23, 0xfd, 0x2f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xfd, 0x2f, 0xc1, 0x21, 0x01, 0x20, 0xfe, 0x1f };

// Serial command
int8_t cmd;
int8_t wifiLost = 0;

// Time facilities
const long gmtOffset_sec     = 3600;
const int daylightOffset_sec = 3600; // heure d'ete 3600
struct tm timeinfo;            // time struct
const char* ntpServer        = "pool.ntp.org";

// Time HH:MM.ss
String getTime() {
  static char temp[10];
  snprintf(temp, 10, "%02d:%02d:%02d", timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
  return String(temp);
}

String rebootTime;
String recordTime;
// Date as europeen format
String getDate(int sh = -1){
  static char temp[20];
  switch (sh) {
  case 0: 
    snprintf(temp, 20, "%02d/%02d/%04d", timeinfo.tm_mday, (timeinfo.tm_mon+1), (1900+timeinfo.tm_year) );
    break;
  case 1:
    snprintf(temp, 20, "%02d/%02d/%02d %02d:%02d", timeinfo.tm_mday, (timeinfo.tm_mon+1), (timeinfo.tm_year-100),  timeinfo.tm_hour,timeinfo.tm_min );
    break;
  default:
    snprintf(temp, 20, "%02d/%02d/%04d %02d:%02d:%02d", timeinfo.tm_mday, (timeinfo.tm_mon+1), (1900+timeinfo.tm_year),  timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
    break;
  }
  return String(temp);
}

bool isNight() {
  if (timeinfo.tm_hour>21 && timeinfo.tm_hour<7 ) return true;
  return false;
}

// Boiler flame
#define pinFlameAo 36 // ADC1_0 A
JFlame flame(pinFlameAo);

// FLux detector
int fluxStart = -1;
float tmpLiterPerMinute;          // On change on liter per minute
uint16_t cntFluxBadSec = 0;       // Count how many seconds the flux is flowing
boolean isValveClosed  = false;   // Valve must be closed or opened

String getMMSS() {
  static char temp[10];
  int mm = (int)cntFluxBadSec/60;
  int ss = (int)cntFluxBadSec - (mm*60);
  snprintf(temp, 10, "%dm%02ds", mm, ss);
  return String(temp);
}

// IRQ
#define irqPinHall 34
#define irqPinIR 13
JFlux flux(irqPinHall, irqPinIR);

// Relay Valve Button LEd pins valve 5Nn 300sec
#define pinSS 21 // SoldState Relay
#define pinVD 14 // Valve Direction
#define pinBz 18 // Buzzer
#define pinU  05 // Button Up
#define pinD  17 // Button Down

#define pinR 16 // Led Red
#define pinG 04 // Led Green
#define pinB 15 // Led blue
JKeyLedBuz keyLedBuz(pinR, pinG, pinB, pinU, pinD, pinVD, pinBz, pinSS);
uint8_t buttonPressed = 0; // 1=BtUp 2=BtDown 3=Both

// Internal led
#define EspLedBlue 2
long previousMillis = 0;

// Jeedom
Jeedom jeedom("/cfJeedom.json");
bool saveConfigJeedom = false;
// Devices from virtual jeedom
const int idTemp   = 1784; //! Not used
const int idHumi   = 1785; //! Not used
const int idFlam   = 1786; // On Off Flam
const int idPower  = 1816; // Percent Flam
const int idDebitA = 1811; // l/m
const int idDebitT = 1812; // Total m3
const int idDebitD = 1823; // Flux On/Off
const int idValve  = 1825; // Open Closed
int onChanged = 2;

// #define JEEDOM_DISLABED
#ifdef JEEDOM_DISLABED
#define SEND2JEEDOM(na,wc,rj,id,va) 
#else
#define SEND2JEEDOM(na,wc,rj,id,va) { \
  if (wc == WL_CONNECTED && rj == HTTP_CODE_OK) { \
    rj = jeedom.sendVirtual(id, va); \
    if (rj != HTTP_CODE_OK && Serial.availableForWrite() ) { \
      LOG("%s %s Jeedom(id:%d) error (%s)\n\r", getDate().c_str(), na, id, frame.httpStatus(rj) ); \
    }\
  }\
}
#endif

// Frame option
void saveConfigCallback() {}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {}

// Action from JEEDOM
void actionOpen() {
  LOG("%s -Action OPEN valve.", getDate().c_str());
  isValveClosed = false;
  onChanged = 2;
}
void actionClose() {
  LOG("%s -Action CLOSE valve.", getDate().c_str());
  isValveClosed = true;
  onChanged = 2;
}
void actionReset() {
  LOG("%s -Action RESET valve.", getDate().c_str());
  cntFluxBadSec = 0;
  isValveClosed = false;
  onChanged = 2;
}
void actionSetTotal(uint64_t val) {
  LOG("%s -Action SET total at %llu m3", getDate().c_str(), val);
  flux.setMagnetHallPluse( val );
  onChanged = 2;
}

// -------- Web transformation into Get Set functions ------------- 
#include "eau.h"

int retJeedom = HTTP_CODE_OK;
void setDsp() {
  if (isNight()) {
      u8g2.setContrast(0);
      u8g2.clear();
  } else {
    u8g2.setContrast(0xFF);
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
    // Symbole
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    // Text
    u8g2.setFont(u8g2_font_7x14_tf);
    OLEDF( 73,14, "%s", getTime().c_str());
    switch (cntOled) {
      case 0: /* OLEDF( 0, 31, "Temp:%2.0f°C Hum:%2.0f%%", temperatureDHT, humidityDHT); break; */
      case 1: OLEDF( 0, 31, "Eau: %.3f m3", jeedom.config.waterM3); break;
      case 2: OLEDF( 0, 31, "Vanne: %s %s", (isValveClosed)?("OFF"):("ON"), getMMSS().c_str() ); break;
      case 3: OLEDF( 0, 31, "Flamme: %s", (flame.state)?("Allumée"):("Eteinte") ); break;
      case 4: OLEDF( 0, 31, "Wifi:%s", WiFi.localIP().toString().c_str() ); break;
      case 5: OLEDF( 0, 31, "Mac:%s", WiFi.macAddress().c_str() ); break;
      case 6: OLEDF( 0, 31, "Jeedom: %s", ((retJeedom == HTTP_CODE_OK)?("est OK"):("Erreurs")) ); break;
      case 7: OLEDF( 0, 31, "Date: %s", getDate(0).c_str() ); break;
      case 8: OLEDF( 0, 31, "Version: %s", VERSION ); break;
      case 9: OLEDF( 0, 31, "Rec:%s", recordTime.c_str() ); break;
      case 10:OLEDF( 0, 31, "Raz:%s", rebootTime.c_str() ); break;
      default: cntOled=0; break; 
    }
    OLEDS();
    if ( (timeinfo.tm_sec % 3)==0 ) {
       cntOled++;
    }
  }
}

// WatchDog
uint32_t wdCounter = 0;
void watchdog(void *pvParameter) {
  while (1) {
    vTaskDelay(5000/portTICK_RATE_MS); // Wait 5 sec
    wdCounter++;
    if (wdCounter > 400) { // 
      // We have a problem no connection if crash or waitting 
      if (wdCounter == 401 ) {
        LOG("%s -WatchDog Wifi:%s after:%d sec. -> REBOOT.", getDate().c_str(), frame.wifiStatus(WiFi.status()), (wdCounter*5) );
        hlog.flush();
        jeedom.saveConfigurationJeedom();
      } else {
        // Perhapse force ??? WiFi.begin(ssid, password);
        ESP.restart(); // Restart after 5sec * 180 => 15min
        delay(2000);
      }
    }
  }
}

//  configModeCallback callback when entering into AP mode
void configModeCallback (WiFiManager *myWiFiManager) {
  // Clear OLED 
  OLEDC();
  u8g2.setFont(u8g2_font_7x14_tf);
  OLEDF( 0, 10, "AP: %s", myWiFiManager->getConfigPortalSSID().c_str()); 
  OLEDF( 0, 31, "IP: %s", WiFi.softAPIP().toString().c_str()); 
  OLEDS();
  LOG("%s -ACCESS POINT actived MAC:%s IP:%s", getDate().c_str(), myWiFiManager->getConfigPortalSSID().c_str() ,  WiFi.softAPIP().toString().c_str() );
}

// Interrupt Sevice Routine for flux ------------------------
static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR irq0(){
  uint32_t gpio_num = irqPinIR;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void IRAM_ATTR irq1(){
  uint32_t gpio_num = irqPinHall;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void taskIsrFLux(void* arg) {
  uint32_t io_num;
  for(;;) {
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      flux.irq(io_num, gpio_get_level((gpio_num_t)io_num));
    }
  }
}

void initIsrFlux(){
  //create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  //start gpio task
  xTaskCreate(taskIsrFLux, "taskIsrFLux", 2048, NULL, 10, NULL);
  //set isr on InfraRed paddle wheel counter
  pinMode(irqPinIR, INPUT_PULLUP);
 // pinMode(irqPinIR, INPUT);  
  attachInterrupt(digitalPinToInterrupt(irqPinIR), irq0, RISING);
  //Set isr on Magnet hall detector 
  //pinMode(GPIO_INPUT_IO_1, INPUT_PULLUP);
  pinMode(irqPinHall, INPUT);  
  attachInterrupt(digitalPinToInterrupt(irqPinHall), irq1, CHANGE);
}

// setup -------------------------------------------------------------------------
void setup() {
#ifdef DEBUG_MAIN
  Serial.begin(115200);
  Serial.printf("Start setup Ver:%s\n\r",VERSION);
#endif
  // Start Oled 128x32
  u8g2.begin();
  OLEDC();
  u8g2.setFont(u8g2_font_10x20_tf);
  OLEDF( 0, 14, "Survey:%s", VERSION);
  OLEDS();
  // Set pin mode  I/O Directions
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
   // Start my WatchDog olso used to reset AP evey 15m (Some time after general cut off Wifi host is started after Eps)
  xTaskCreate(&watchdog, "wd task", 2048, NULL, 5, NULL);
  keyLedBuz.rgb = 0xFFFFFF; // Wait Wifi
  // Start framework
  frame.setup();
  keyLedBuz.rgb = 0x777777;  // WifiOK
  // Start jeedom_ok ---> Jedom command Return jeedom_ok
  jeedom.setup();
  // Append /wwm access html 
  frame.server.on("/eau", [](){
    frame.server.send(HTTP_CODE_OK, "text/html", sentHtmlEau());
  });
  frame.server.on("/tail",  [](){
    frame.server.send(HTTP_CODE_OK, "text/plain", hlog.getTail());
  });
  frame.externalHtmlTools="Specific home page is visible at :<a class='button' href='/eau'>Eau Page</a>";
  // Init time
  configTime(gmtOffset_sec, jeedom.config.daylightoffset, ntpServer); //init and get the time
  wifiLost = 0;
  // IRQ
  flux.setup(jeedom.config.waterM3, jeedom.config.fluxReference);
  initIsrFlux();
  isValveClosed = !jeedom.config.valveOpen;
  keyLedBuz.initValve(isValveClosed);
  // Start time
  getLocalTime(&timeinfo);
  // Wait get time delay
  delay(2000);
  rebootTime = getDate(1);
  // Get Reset Reason 
  RESET_REASON rr = rtc_get_reset_reason(0);
  LOG("%s -CPU REBOOT(%s)  IP:%s MAC:%s", getDate().c_str(), frame.resetReason((int)rr), WiFi.localIP().toString().c_str() , WiFi.macAddress().c_str() );
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
        if (cmd=='h') { Serial.println(); Serial.println("- Help info: r=reboot i=myip s=saveConfig");}
        else if (cmd=='r') { ESP.restart(); }
        else if (cmd=='i') { Serial.printf("Heap:%u IP:%s \n\r",ESP.getFreeHeap(), WiFi.localIP().toString().c_str() ); }
        else if (cmd=='s') { Serial.println("Mode save config."); jeedom.saveConfigurationJeedom(); cmd=' ';}
        else { Serial.printf("Stop serial: %s \n\r",VERSION); }
      }
    }
  }
#endif
  // Call flux loop
  flux.loop();
  // Call frame loop
  frame.loop();
  // Call key led RGB animation Key repeat = 200ms
  buttonPressed = keyLedBuz.getKey(200);
  if (buttonPressed != 0) {
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
    retJeedom = HTTP_CODE_OK;
    int wifistat = WiFi.status();
    // if wifi is down, try reconnecting every 60 seconds
    if (wifistat != WL_CONNECTED) {
      wifiLost++;
      if (wifiLost==10) {
       // LOG("%s -WiFi Lost:%s wifiLost:%d sec. jeeErrCnt:%d localIP:%s", getDate().c_str(), frame.wifiStatus(wifistat), wifiLost, jeedom.getErrorCounter(), WiFi.localIP().toString().c_str() );
      }
      if (wifiLost == 50) {
       // LOG("%s -WiFi disconnect OK after 50s (%s).",getDate().c_str(), frame.wifiStatus(wifistat));
        saveConfigJeedom = true;
        WiFi.disconnect();
      }
      if (wifiLost == 60) {
        if (WiFi.reconnect()) {
          LOG("%s -WiFi reconnect OK after 60s (%s).",getDate().c_str(), frame.wifiStatus(wifistat));
          wifistat = WL_CONNECTED;
        }
      }
    } else {
      wdCounter = 0;
      wifiLost = 0;
    }
    // Valve state analysis
    if (isValveClosed==false) {
      if (flux.isChanged(&timeinfo, jeedom.config.fluxReference)) {
        onChanged = 2;
      }
      if (flux.getState()) {
        if (fluxStart==-1) fluxStart=flux.getMagnetHallPluse();
        cntOled = 2; // Force Dsp Vanne
        if ( ((float)cntFluxBadSec / 60.0) < jeedom.config.openDelay) {
          cntFluxBadSec++;
          keyLedBuz.rgb = 0x0000FF;
        } else {
          keyLedBuz.rgb = 0xFF0000;
          isValveClosed = true;   // Close valve
        }
      } else {
        if (cntFluxBadSec>0) {
          if (keyLedBuz.rgb==0x0000FF) {
            // DEB£UG Config at 0
           //  int litre = (flux.getMagnetHallPluse() - fluxStart ) * jeedom.config.fluxReference;
            fluxStart = -1;
            // if (litre>0) { LOG("%s -Flux durant:%s pour %dl", getDate().c_str(),  getMMSS().c_str(), litre ); }
            // else { LOG("%s -Flux durant:%s" , getDate().c_str(),  getMMSS().c_str() ); }
          }
          cntFluxBadSec--;
        }
        keyLedBuz.rgb = 0x007700;
      }
    } else {
      // valve is closed Red is selected
      keyLedBuz.rgb = 0x770000;
    }

    // Action USER valve And Jeedom notify
    if (keyLedBuz.setValve(isValveClosed)) {
      SEND2JEEDOM("Set valve", wifistat, retJeedom, idValve, !isValveClosed);
      onChanged = 2;
    }
    // Boiler is changed
    if (flame.isChanged(&timeinfo, 1000)) { // hysteresis = 1000/10 * 2
      SEND2JEEDOM("JFlame.isChanged", wifistat, retJeedom, idPower, flame.flamePerCent);
      SEND2JEEDOM("JFlame.isChanged", wifistat, retJeedom, idFlam,  flame.state       );
    }
    if (flame.state)  {
      keyLedBuz.rgb2 = 0xFF7700; // change flash by red
    } else {
      keyLedBuz.rgb2 = 0x0;
    }
    if (flux.getLiterPerMinute()!=tmpLiterPerMinute) {
      tmpLiterPerMinute = flux.getLiterPerMinute();
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitA, flux.getLiterPerMinute());
    }
    // every day. update Gaz power & Water counter & record jeedom config if changed
    boolean newday = ( (timeinfo.tm_hour == 23) && (timeinfo.tm_min == 59) && (timeinfo.tm_sec == 55));
    jeedom.config.waterM3 = ((float)flux.getMagnetHallPluse()/(jeedom.config.fluxReference * 1000.0) );
    jeedom.config.valveOpen = !isValveClosed;
    if ( onChanged>0 || newday ) {
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitA, flux.getLiterPerMinute()   );
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitD, !flux.getState()           );
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idValve , jeedom.config.valveOpen );
      SEND2JEEDOM("JFlux.isChanged", wifistat, retJeedom, idDebitT, jeedom.config.waterM3 );
      if ( newday && onChanged==1 /* && jeedom.isCcrChanged()*/ )  {
        saveConfigJeedom = true;
      }
      onChanged--;
    }
    // Optional action
    if (saveConfigJeedom ) {
      if (jeedom.saveConfigurationJeedom())
        recordTime = getDate(1);
      hlog.flush();
      saveConfigJeedom = false;
    }
#ifdef DEBUG_MAIN
    Serial.printf("%s onChanged:%d isValveClosed=%d \n\r",getDate().c_str(), onChanged,isValveClosed);
#endif
    // oled display
    setDsp();
  } // End second
}