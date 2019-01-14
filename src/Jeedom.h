#ifndef Jeedom_h
#define Jeedom_h

#include <HTTPClient.h>

// configuration file jeedom
struct ConfigJeedom {
  char  host[20];
  int   port;
  char  apiKey[40];
  float fluxReference; // impulsion per liter default 2
  float openDelay; // Delay of flow is continus
  float waterM3; // total counter initial value default 0
};

class Jeedom {
public:

  boolean isCcrChanged(){
     return (myCrc8((uint8_t*)&config, sizeof(config) - 1) != ccrConfig);
  }

  boolean saveConfigurationJeedom() {
    File file = SPIFFS.open(fileconfigjeedom, "w");
    if (!file) {
      // Serial.println(F("Can't write in Configuration Jeedom file."));
       return false;
    }
    String cfJeedomjson;
    StaticJsonBuffer<500> jsonBuffercfg;
    JsonObject &rootcfg = jsonBuffercfg.createObject();
    rootcfg["host"] = config.host;
    rootcfg["port"] = config.port;
    rootcfg["apiKey"] = config.apiKey;
    rootcfg["fluxReference"] = config.fluxReference;
    rootcfg["openDelay"] = config.openDelay;
    rootcfg["waterM3"] = config.waterM3;
    rootcfg.printTo(cfJeedomjson);
    file.print(cfJeedomjson);
    file.close();
    ccrConfig = getCcrConfig();
    return true;
  //  Serial.println(F("Configuration Jeedom file has been saved."));
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
      StaticJsonBuffer<600> jsonBufferConfigJeedom;
      JsonObject& rootcfg = jsonBufferConfigJeedom.parseObject(buf.get());
      strlcpy(config.host, rootcfg["host"] | "192.168.1.117", sizeof(config.host));
      config.port = rootcfg["port"] | 80;
      strlcpy(config.apiKey, rootcfg["apiKey"] | "unknown", sizeof(config.apiKey));
      config.fluxReference = rootcfg["fluxReference"] | 1.0; // default 2 pulse per liter
      config.openDelay = rootcfg["openDelay"] | 15.0; // default 15 minutes
      config.waterM3 = rootcfg["waterM3"] | 0.000; // Read local counter
      if (!rootcfg.success()) saveConfigurationJeedom();
    }
    ccrConfig = getCcrConfig();
  }

  Jeedom(const char * filename) {
    fileconfigjeedom = filename;
  };

  void setup() {
    loadConfigurationJeedom();
    virtualbaseurl = "/core/api/jeeApi.php?apikey=";
    virtualbaseurl += config.apiKey;
    virtualbaseurl += "&type=virtual&id=";
  }

  int sendVirtual(int id, float val) {
    char temp[20]; // force for m3 in liter
    snprintf(temp, 20, "%.3f", val);
    String url = virtualbaseurl + String(id);
    url += url + "&value="; url += String(temp);
    http.begin(config.host,config.port, url);
    int httpCode = http.GET();
    http.end();
    return httpCode;
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

};

#endif
