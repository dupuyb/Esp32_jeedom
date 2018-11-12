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

//Init JSON
DynamicJsonBuffer jsonBuffer(500);
JsonObject& JSONRoot   = jsonBuffer.createObject();
JsonObject& JSONSystem = JSONRoot.createNestedObject("System");
JsonObject& JSONJeedom = JSONRoot.createNestedObject("Jeedom");
struct Config {            //
  char HostName[20];       // Default esp32dudu
  byte MacAddress[6];      // Default 0x30,0xAE,0xA4,0x90,0xFD,0xC8
  bool ResetWifi;          // Default false WiFimanager reconnect with last data
  char OTAHostName[20];    // Default esp32dudu
  char OTAPassword[20];    // Default empty
  char UploadName[20];     // Default login admin
  char UploadPassword[20]; // Default password admin
  bool UploadLocal;        // True if simpleUpload must be called in case of not Upload.html
  char JeedomIp[20];
};
// variables Global
long previousMillis = 0;
#define  BUF_SIZE  60       // Taille max des notification ( nb de caractéres max )
char   message[BUF_SIZE];   // Message global
bool   OTAerreur = false;   // Error OTA
const char *filename = "/config.json";
Config config;
File fsUploadFile;          // a File variable to temporarily store the received file

// services WEB
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String JsonConfig() {
  String configjson;
  StaticJsonBuffer<1600> jsonBuffercfg; // Use https://arduinojson.org/assistant/ to compute the capacity.
  JsonObject &rootcfg = jsonBuffercfg.createObject(); // Parse the root object
  // Set the values
  rootcfg["HostName"] = config.HostName;
  JsonArray& mac = rootcfg.createNestedArray("MacAddress");
  for (int i=0; i<6; i++)
    mac.add(config.MacAddress[i]);
  rootcfg["ResetWifi"]      = config.ResetWifi;
  rootcfg["OTAHostName"]    = config.OTAHostName;
  rootcfg["OTAPassword"]    = config.OTAPassword;
  rootcfg["UploadName"]     = config.UploadName;
  rootcfg["UploadPassword"] = config.UploadPassword;
  rootcfg["UploadLocal"]    = config.UploadLocal;
  rootcfg["JeedomIp"]       = config.JeedomIp;
  // Transform to string
  rootcfg.printTo(configjson);
  return configjson;
}
//  Directory list
void listDir(String& ret, fs::FS &fs, const char * dirname, uint8_t levels) {
  ret += "Listing directory: ";
  ret += dirname; ret += "\n";
  File root = fs.open(dirname);
  if (!root) { ret += "Failed to open directory\n";  return;  }
  if (!root.isDirectory()) { ret += "Not a directory\n"; return; }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      ret += "  DIR : "; ret += file.name(); ret += "\n";
      if (levels)  listDir(ret, fs, file.name(), levels - 1);
    } else {
      ret += ("  FILE: "); ret += (file.name());
      ret += ("  SIZE: "); ret += (file.size()); ret += "\n";
    }
    file = root.openNextFile();
  }
  return;
}
String getContentType(String filename) {
  if (server.hasArg("download"))        { return "application/octet-stream";
  } else if (filename.endsWith(".htm")) { return "text/html";
  } else if (filename.endsWith(".html")){ return "text/html";
  } else if (filename.endsWith(".css")) { return "text/css";
  } else if (filename.endsWith(".js"))  { return "application/javascript";
  } else if (filename.endsWith(".png")) { return "image/png";
  } else if (filename.endsWith(".gif")) { return "image/gif";
  } else if (filename.endsWith(".jpg")) { return "image/jpeg";
  } else if (filename.endsWith(".ico")) { return "image/x-icon";
  } else if (filename.endsWith(".xml")) { return "text/xml";
  } else if (filename.endsWith(".pdf")) { return "application/x-pdf";
  } else if (filename.endsWith(".zip")) { return "application/x-zip";
  } else if (filename.endsWith(".gz"))  { return "application/x-gzip"; }
  return "text/plain";
}
// Save config file
String saveConfiguration(const char *filename, const Config &config) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
  //  Serial.println("Can't write in Config file.");
    return "Can't write in Config file.";
  }
  file.print(JsonConfig());
  file.close();
  return "Config file was been saved.";
}
// Start SPIFFS & Read config file
void startSPIFFS() {
  if (SPIFFS.begin()==false){
    Serial.println("SPIFFS was not formatted.");
    SPIFFS.format();
    SPIFFS.begin();
  }
  String ls;
  listDir(ls, SPIFFS, "/", 0);
  Serial.print(ls);
}
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading configuration
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
  // Set config or defaults
  strlcpy(config.HostName, rootcfg["HostName"] | "esp32dudu",sizeof(config.HostName));
  byte new_mac[8] = {0x30,0xAE,0xA4,0x90,0xFD,0xC8};
  JsonArray& mac = rootcfg["MacAddress"];
  for (int i=0; i<6; i++)
    config.MacAddress[i] = mac[i] | new_mac[i];
  config.ResetWifi = rootcfg["ResetWifi"] | false;
  strlcpy(config.OTAHostName, rootcfg["OTAHostName"] | "esp32dudu",sizeof(config.OTAHostName));
  strlcpy(config.OTAPassword, rootcfg["OTAPassword"] | "empty",sizeof(config.OTAPassword));
  strlcpy(config.UploadName, rootcfg["UploadName"] | "admin",sizeof(config.UploadName));
  strlcpy(config.UploadPassword, rootcfg["UploadPassword"] | "admin",sizeof(config.UploadPassword));
  config.UploadLocal = rootcfg["UploadLocal"] | true;
  strlcpy(config.JeedomIp, rootcfg["JeedomIp"] | "", sizeof(config.JeedomIp));
  if (!rootcfg.success()) {
    Serial.println("Erreur lecture fichier config.");
    Serial.println(saveConfiguration(filename, config));
  }
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

// Start WiFiManager
void startWifiManager() {
  WiFiManager wifiManager;
  esp_base_mac_addr_set(config.MacAddress); // Wifi_STA=mac  wifi_AP=mac+1  BT=mac+2
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  //Forcer à effacer les donnees WIFI dans l'eprom , permet de changer d'AP à chaque demmarrage ou effacer les infos d'une AP dans la memoire ( a valider , lors du premier lancement  )
  if (config.ResetWifi) wifiManager.resetSettings();
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //Recupere les identifiants   ssid et Mot de passe dans l'eprom  et essayes de se connecter
  //Si pas de connexion possible , il demarre un nouveau point d'accés avec comme nom , celui definit dans la commande autoconnect ( ici : AutoconnectAP )
  wifiManager.autoConnect(config.HostName);
  // Wait for connection
  Serial.println("Wait wifi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
}

// Start OTA
void startOTA(){
  // ArduinoOTA.setPort(8266); default is 8266
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(config.OTAHostName);
  ArduinoOTA.setPassword(config.OTAPassword);
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
    else  type = "SPIFFS";
    if ((type)=="SPIFFS") SPIFFS.end();   // NOTE: demontage disque SPIFFS pour mise a jour over the Air
    delay(500);
    Serial.println("Start updating:" + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("Reboot ...");
    delay(500);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    sprintf(message,"Upload  %u%%", (progress / (total / 100)));
    Serial.println(message);
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
// Start webSocket
void startWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
// Handle Web server
bool handleFileRead(String path) {                         // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";            // If a folder is requested, send the index file
  String contentType = getContentType(path);               // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {  // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                       // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    server.streamFile(file, contentType);                  // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);     // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){                                // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");                // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
String textNotFound(){
  String message = "404: File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  return message;
}
void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", textNotFound());
  }
}
String simpleUpload(){
  String message = "<!DOCTYPE html><html><head>";
  message +="<title>HTML Uploader</title>";
  message +="<meta content='width=device-width' name='viewport'>";
  message +="</head><body><center><header><h1>HTML Uploader</h1></header>";
  message +="<div><p style=\"text-align: center\">Use this page to upload new files to the ESP32.<br/>You can use compressed (.gz) files.</p>";
  message +="<form method=\"post\" enctype=\"multipart/form-data\" style=\"margin: 0px auto 8px auto\" >";
  message +="<input type=\"file\" name=\"Choose file\" accept=\".gz,.html,.ico,.js,.css,.png,.gif,.jpg,.xml,.pdf,.htm\">";
  message +="<input class=\"button\" type=\"submit\" value=\"Upload\" name=\"submit\">";
  message +="</form></div></center></body></html>";
  return message;
}
// Start web server
void startWebServer(){
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
     server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well"); // Just for some test
  });
  server.on("/ls", [](){                      // Get list of file in FS
    String ls;
    listDir(ls, SPIFFS, "/", 0);
    server.send(200, "text/plain", ls);
  });
  server.on("/saveconfig", [](){               // Force saveconfig
    server.send(200, "text/plain", saveConfiguration(filename, config));
  });
  // server upload file
  server.on("/Upload", HTTP_GET, []() {        // Upload
    if (!server.authenticate(config.UploadName, config.UploadPassword)) {
      return server.requestAuthentication();
    }
    if (!handleFileRead("/upload.html")) {    // upload.html exist on FS
      if (config.UploadLocal) server.send(200, "text/html", simpleUpload() ); // If not upload.html in FS send light
      else server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.on("/Upload", HTTP_POST, []() {       // Back after selection
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.onNotFound(handleNotFound);         // Not found page
  server.begin();
}
// Start MDNS
void startMDNS() {
  // - first argument is the domain name, in this example   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise   we send our IP address on the WiFi network
  if (!MDNS.begin(config.HostName))
    Serial.println("Error setting up MDNS responder!");
  sprintf(message, "mDNS responder started: %s", config.HostName);
  Serial.println(message);
  MDNS.addService("http",  "tcp", 80);
  MDNS.addService("ws",    "tcp", 81);
  MDNS.addService("esp32", "tcp", 8888); // Announce esp32 service port 8888 TCP
}
// Arduino core -------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Set pin mode
  pinMode(EspLedBlue, OUTPUT);     // Led is BLUE at statup
  digitalWrite(EspLedBlue, HIGH);  // After 5 seconds blinking indicate WiFI ids OK
  delay(5000);                     // If stay BLUE after 5 sec mode AccessPoint
  Serial.println("Setup started.");
  startSPIFFS();                   // Start FS (list all contents)
  loadConfiguration(filename, config); // Read config file
  startWifiManager();              // Start a Wi-Fi access point, and try to connect
  startOTA();                      // Start the OTA service
  startWebSocket();                // Start a WebSocket server
  startWebServer();                // Start a HTTP server with a file read handler and an upload handler
  startMDNS();                     // Start the mDNS responder
  Serial.print("Setup finished IP:");Serial.println(WiFi.localIP());;
}
// Main loop -----------------------------------------------------------------
void loop() {
  server.handleClient();         // constantly check for events
  webSocket.loop();              // constantly check for websocket events
  ArduinoOTA.handle();           // listen for OTA events
  if (OTAerreur) ESP.restart();  // Restart in case of error
  // Is alive
  if ( millis() - previousMillis > 1000L) {
    previousMillis = millis();
    digitalWrite(EspLedBlue, !digitalRead(EspLedBlue));
  }
}
