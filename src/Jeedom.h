#ifndef Jeedom_h
#define Jeedom_h

#include <HTTPClient.h>

class Jeedom
{
  public:
    Jeedom(const char* h, int p, const char* k) {
      host = h; port = p; apiKey = k;
      virtualbaseurl = "/core/api/jeeApi.php?apikey=";
      virtualbaseurl += apiKey;
      virtualbaseurl += "&type=virtual&id=";
    };

    int sendVirtual(String id, float val) {
      String url = virtualbaseurl + id;
      url += url + "&value="; url += String(val);
    	http.begin(host,port,url);
    	int httpCode = http.GET();
    	http.end();
    	return httpCode;
    }

  private:
    HTTPClient http;
    const char* host;
    int   port;
    const char* apiKey;
    String virtualbaseurl;
};

#endif
