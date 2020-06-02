#ifndef HLOG_H
#define HLOG_H

#include <vector>
#include <assert.h>
// File FS SPI Flash File System
#include "eth_phy/phy.h"
#include <FS.h>
#include <SPIFFS.h>

#define filenamelog0 "/running.log" // file log
#define filenamelog1 "/previous.log" // file log

class HLog {
private:
  size_t   nbrLineMax; // size maximun in record
  size_t   fileSizeMx;
  bool     cpyOnSPIFS;
  
protected:
    std::vector<String> record;

public:
  HLog(int szIt=80, int szFs=8496, bool cpyFs=true) { nbrLineMax=szIt; fileSizeMx=szFs; cpyOnSPIFS=cpyFs; }

  ~HLog() { clear(); }

  void append (const char *s){
      append(String(s));
  } 

  void append (String s) {
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
        // Keep only two files
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