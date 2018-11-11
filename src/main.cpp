#include <Arduino.h>
// File FS (file system), added to pltformio.ini lib_deps
#include <FS.h>
#include <SPIFFS.h>
// JSon install ArduinoJson by Benoit Blanchon, added to pltformio.ini lib_deps
#include <ArduinoJson.h>
// OTA need WiFiUdp
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// WiFimanager need WiFi WebServer WiFimanager
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
 //https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>
// WebSocket
#include <WebSocketsServer.h>
// mDNS
#include "ESPmDNS.h"

#define EspLedBlue 2

// variables
long previousMillis = 0;
#define  BUF_SIZE  60       // Taille max des notification ( nb de caractéres max )
char Notif[BUF_SIZE];
String hostname;
bool   OTAerreur = false;
bool   raz       = false;

// Config file Voici l’URL = http://#IP_JEEDOM#/jeedom/core/api/jeeApi.php?apikey=#APIKEY#&type=cmd&id=#ID#
//Init JSON
DynamicJsonBuffer jsonBuffer(500);
JsonObject& JSONRoot   = jsonBuffer.createObject();
JsonObject& JSONSystem = JSONRoot.createNestedObject("System");
JsonObject& JSONJeedom = JSONRoot.createNestedObject("Jeedom");
struct Config {
  char HostName[20];
  char JeedomIp[20];
};
const char *filename = "/config/config.json";
Config config;

// services WEB
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String JsonConfig() {
  String configjson;
  // Use https://arduinojson.org/assistant/ to compute the capacity.
  StaticJsonBuffer<1600> jsonBuffercfg;
  // Parse the root object
  JsonObject &rootcfg = jsonBuffercfg.createObject();
  // Set the values
  rootcfg["JeedomIp"] = config.JeedomIp;
  // Transform to string
  rootcfg.printTo(configjson);
  return configjson;
}

// Read config file
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File file = SPIFFS.open(filename, "r");
  if (!file)
    Serial.println(" Config file is absent.");
  size_t size = file.size();
  if (size > 1024)
    Serial.println(" Fichier config trop grand.");
  // allocate buffer for loading config
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  StaticJsonBuffer<1600> jsonBufferConfig;
  JsonObject& rootcfg = jsonBufferConfig.parseObject(buf.get());
  if (!rootcfg.success())
    Serial.println(" Erreur lecture fichier config - Valeur par defaut -");
  // Set config or defaults
  strlcpy(config.HostName, rootcfg["HostName"] | "ESP32-Dudu",sizeof(config.HostName));
  strlcpy(config.JeedomIp, rootcfg["JeedomIp"] | "", sizeof(config.JeedomIp));
}

// Save config file
void saveConfiguration(const char *filename, const Config &config) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println("Can't write in Config file.");
    return;
  }
  file.print(JsonConfig());
  file.close();
}

//  configModeCallback callback when entering into AP mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Choisir AP..");
  delay(3000);
  Serial.println(WiFi.softAPIP().toString());
  delay(3000);
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

// Setup WiFiManager
void setupWifiManager() {
  WiFiManager wifiManager;
  //Si besoin de fixer une adresse IP
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  //Forcer à effacer les donnees WIFI dans l'eprom , permet de changer d'AP à chaque demmarrage ou effacer les infos d'une AP dans la memoire ( a valider , lors du premier lancement  )
  if (raz) wifiManager.resetSettings();
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //Recupere les identifiants   ssid et Mot de passe dans l'eprom  et essayes de se connecter
  //Si pas de connexion possible , il demarre un nouveau point d'accés avec comme nom , celui definit dans la commande autoconnect ( ici : AutoconnectAP )
  wifiManager.autoConnect(config.HostName);
  //Si rien indiqué le nom par defaut est  ESP + ChipID
  //wifiManager.autoConnect();
  // if(!wifiManager.autoConnect("WIFI-ESP32")) {
  //   Serial.println("erreur AP..resart..");
  //   delay(2000);
  //   //reset and try again, or maybe put it to deep sleep
  //   ESP.restart();
  //   delay(2000);
  // }
}

// Setup OTA
void setupOTA(){
  // ArduinoOTA.setPort(8266); default is 8266
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
    else  type = "SPIFFS";
    if ((type)=="SPIFFS") SPIFFS.end();   // NOTE: demontage disque SPIFFS pour mise a jour over the Air
    delay(500);
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("Reboot ...");
    delay(500);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    sprintf(Notif,"Upload  %u%%", (progress / (total / 100)));
    Serial.println(Notif);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    delay(500);
    OTAerreur=true;
    Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();
}

// Web socket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  Serial.printf("[%u] get Message: %s\r\n", num, payload);
  switch(type) {
    case WStype_DISCONNECTED:
    break;
    case WStype_CONNECTED:  {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    }
    break;
    case WStype_TEXT:  {
      String _payload = String((char *) &payload[0]);
      Serial.println(_payload);
    }
    break;
    case WStype_BIN:
    break;
    default:
    break;
  }
}
// Handle Setup web  server
void handleRoot() {
  server.send(200, "text/plain", "hello from esp32!");
}
void handleNotFound(){
  String message = "Page Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void setupWebServer(){
  server.on("/", handleRoot);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
}
// Set up mDNS
void setupmDNS() {
  String  mdnsName = config.HostName;
  // - first argument is the domain name, in this example   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise   we send our IP address on the WiFi network
  if (!MDNS.begin(mdnsName.c_str())) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  //mdns.register("fishtank", { description="Top Fishtank", service="http", port=80, location='Living Room' })
  MDNS.addService("http",  "tcp", 80);
  MDNS.addService("ws",    "tcp", 81);
  MDNS.addService("esp32", "tcp", 8888); // Announce esp32 service port 8888 TCP
}
// Arduino core
void setup() {
  Serial.begin(115200);
  // Set MAC addess & AP IP
  byte new_mac[8] = {0x30,0xAE,0xA4,0x90,0xFD,0xC8}; // Pif
  //  byte new_mac[8] = {0x8C,0x29,0x37,0x43,0xCD,0x46}; // Dudu
  esp_base_mac_addr_set(new_mac); // Wifi_STA=mac  wifi_AP=mac+1  BT=mac+2
  // Set pin mode
  pinMode(EspLedBlue, OUTPUT);
  delay(5000);
  Serial.println("Setup started.");
  loadConfiguration(filename, config);
  setupWifiManager();
  // Wait for connection
  Serial.println("Wait wifi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  // Websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  // Web serveur
  setupWebServer();
  // nDNS
  setupmDNS();
  Serial.println("Setup finished.");
}

void loop() {
  //  handle server Web & WebSocket
  server.handleClient();
  webSocket.loop();
  // handle OTA request
  ArduinoOTA.handle();
  if (OTAerreur) ESP.restart();
  // Is alive
  if ( millis() - previousMillis > 1000L) {
    previousMillis = millis();
    digitalWrite(EspLedBlue, !digitalRead(EspLedBlue));
  }
}
