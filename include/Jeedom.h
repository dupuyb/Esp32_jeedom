#ifndef Jeedom_h
#define Jeedom_h

#include <HTTPClient.h>

const char  JeedomVersion[] PROGMEM = "1.1.1";

// Jeedom runtime configuration loaded from SPIFFS
struct ConfigJeedom {
  char  host[20];
  int   port;
  char  apiKey[40];
  float fluxReference; // Pulses per liter
  float openDelay; // Max continuous flow duration in minutes
  float waterM3; // Total water counter in m3
  bool  valveOpen; // true when valve is open
  int   daylightoffset; // daylightOffset_sec
};

class Jeedom {
public:

  // Compare current config checksum with last saved checksum.
  // This is a lightweight change detector (not a cryptographic CRC).
  boolean isCcrChanged(){
     return (myCrc8((uint8_t*)&config, sizeof(config) - 1) != ccrConfig);
  }

  boolean saveConfigurationJeedom() {
    File file = SPIFFS.open(fileconfigjeedom, "w");
    if (!file) {
       return false;
    }
    // Preserve legacy rule: avoid writing an all-zero counter snapshot.
     if (config.waterM3==0) { // Keep existing behavior from legacy implementation
      file.close();
      return false;
    }
    String cfJeedomjson;
    // ArduinoJson 6
    JsonDocument rootcfg;
    rootcfg["host"] = config.host;
    rootcfg["port"] = config.port;
    rootcfg["apiKey"] = config.apiKey;
    rootcfg["fluxReference"] = config.fluxReference;
    rootcfg["openDelay"] = config.openDelay;
    rootcfg["waterM3"] = config.waterM3;
    rootcfg["valveOpen"] = config.valveOpen; 
    rootcfg["daylightOffset"] = config.daylightoffset;
    // ArduinoJson 6
    serializeJson(rootcfg, cfJeedomjson);
    file.print(cfJeedomjson);
    file.close();
    ccrConfig = getCcrConfig();
    return true;
  }

  void loadConfigurationJeedom() {
    // Open file for reading configuration
    File file = SPIFFS.open(fileconfigjeedom, "r");
    if (!file) {
      Serial.println(F("Configuration Jeedom file is absent."));
    } else {
      size_t size = file.size();
      std::unique_ptr<char[]> buf(new char[size]);
      file.readBytes(buf.get(), size);
      // ArduinoJson 7
      JsonDocument rootcfg;
      /*auto error = */ deserializeJson(rootcfg, buf.get());
      strlcpy(config.host, rootcfg["host"] | "192.168.2.254", sizeof(config.host));
      config.port = rootcfg["port"] | 80;
      strlcpy(config.apiKey, rootcfg["apiKey"] | "unknown", sizeof(config.apiKey));
      config.fluxReference = rootcfg["fluxReference"] | 1.0; // Default pulses per liter
      config.openDelay = rootcfg["openDelay"] | 15.0; // default 15 minutes
      config.waterM3 = rootcfg["waterM3"] | 0.000; // Restore local persisted counter
      config.valveOpen = rootcfg["valveOpen"] | true; // Restore valve state
      config.daylightoffset = rootcfg["daylightOffset"] | 0; // Summer=3600, winter=0
      // if (error)  saveConfigurationJeedom();
    }
    ccrConfig = getCcrConfig();
  }

  Jeedom(const char * filename) {
    fileconfigjeedom = filename;
  };

  void setup() {
    jeeComErr = 0;
    loadConfigurationJeedom();
    // Build once, then append id/value for each virtual command push.
    virtualbaseurl = "/core/api/jeeApi.php?apikey=";
    virtualbaseurl += config.apiKey;
    virtualbaseurl += "&type=event&plugin=virtual&id=";
  }

  int sendVirtual(int id, float val) {
    // String formatting is normalized to 3 decimals for Jeedom virtual commands.
    char temp[20]; // force for m3 in liter
    snprintf(temp, 20, "%.3f", val);
    // Send one Jeedom virtual update through HTTP GET.
    String url = virtualbaseurl + String(id);
    url += url + "&value="; url += String(temp);
    http.begin(config.host,config.port, url);
    int httpCode = http.GET();
    if (httpCode!=HTTP_CODE_OK) jeeComErr++;
    else jeeComErr = 0;
    http.end();
    return httpCode;
  }

  int getErrorCounter() {
    return jeeComErr;
  }

private :
  uint8_t myCrc8(uint8_t * data, uint8_t count) {
    uint8_t result = 0xDF;
    while (count--) {
      result ^= *data;
      data++;
    }
    return result;
  }

  uint8_t getCcrConfig() {
    return myCrc8((uint8_t*)&config, sizeof(config) - 1);
  }

public:
  uint8_t ccrConfig;
  ConfigJeedom config;

private:
  const char * fileconfigjeedom;
  HTTPClient http;
  String virtualbaseurl;
  int jeeComErr;

};

#endif
