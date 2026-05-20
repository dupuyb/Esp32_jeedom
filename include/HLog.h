#ifndef HLOG_H
#define HLOG_H

#include <vector>
#include <assert.h>
// SPI flash file system support
//#include "eth_phy/phy.h"
#include <FS.h>
#include <SPIFFS.h>

#define filenamelog0 "/running.log"   // Active log file
#define filenamelog1 "/previous.log"  // Rotated backup log file

// Lightweight logger with RAM buffering and periodic SPIFFS persistence.
// When the active file exceeds fileSizeMx, logs are rotated to filenamelog1.
class HLog {
private:
  size_t   nbrLineMax; // Maximum number of in-memory log lines
  size_t   fileSizeMx;
  bool     cpyOnSPIFS; // Reserved compatibility flag (kept for existing constructor API)
  
protected:
    std::vector<String> record;

public:
  HLog(int szIt=80, int szFs=8496, bool cpyFs=true) { nbrLineMax=szIt; fileSizeMx=szFs; cpyOnSPIFS=cpyFs; }

  ~HLog() { clear(); }

  void append (const char *s){
      append(String(s));
  } 

  void append (String s) {
    // Buffer first, write later to reduce flash writes.
    record.push_back(s);
    if (record.size()>nbrLineMax) 
        flush();
  } 

  String getTail() {
    String ret = "Logger Last record:"+String(record.size())+" line(s).\n";
    for (auto &i : record) {  
      ret += i; ret +='\n';
    }
    return ret;
  }

  void clear(){ 
    for (int i=0; i<record.size(); i++) 
      record.erase(record.begin()+0); 
    record.clear();
  }

  String getHLog(int r) { 
      assert(r < record.size());
      return record.at(r);
  }

  bool flush(){
    fs::File logFS;
    if (SPIFFS.exists(filenamelog0)) {
      logFS = SPIFFS.open(filenamelog0, "a");
      if (logFS.size() > fileSizeMx) {
        //Serial.printf("logfs:%d\n\r", logFS.size());
        logFS.close();
        // Keep only two log files (current + previous)
        if (SPIFFS.exists(filenamelog1)) 
          SPIFFS.remove(filenamelog1);
        SPIFFS.rename(filenamelog0, filenamelog1);
        logFS = SPIFFS.open(filenamelog0, "w");
      }
    } else {
      logFS = SPIFFS.open(filenamelog0, "w");
    }
    if (!logFS) {
        Serial.println(F("Log file open failed"));
        return false;
    }
    // Persist buffered lines in insertion order.
    for (auto &i : record) {  
      logFS.println(i);
      logFS.flush();
    }
    record.clear();
    logFS.close();
    return  true;
  } 
};
#endif
