#ifndef EAU_h
#define EAU_h

//---- Start Generated from src/eau.html file --- 2023-02-07 19:31:16.619213
const char HTTP_EAU[] PROGMEM = "<!DOCTYPE html><html><style>.l12 {text-align: right;width: 10em;display: inline-block;}.c12 {text-align: center;width: 10em;display: inline-block;}.r12 {text-align: left;width: 15em;display: inline-block;}.myButton {font-weight: bold;text-align: center;}.myButton:hover {background: linear-gradient(to bottom, #60ec03 5%, #a7c61f 100%);background-color: #60ff05;}.myButton:active {position: relative;top: 1px;}</style><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><title>%%TITLE%%</title></head><body><center><h3>Module de surveillance Eau et Chaudière</h3><div style=' width: 40em; display: inline-block; text-align: left; padding:20px 5px; '><fieldset><legend> Valeurs actuelles : </legend><ul><li>En date du %%DATE%% . </li><li>Le métrage d'eau est de <b> %%MT%% m<sup>3</sup></b>. </li><li>La flamme de la chaudière est <b>%%FL%%</b>. </li><li>La vanne d’alimentation général d'eau est <b>%%Cmd%%</b>. </li><li>Log data visible <a href='/tail'>Last changed</a> or open <a href='/running.log'>Current</a> or file <a href='/previous.log'>Previous</a>.</li><li>Valeur (Irq_Hall, IR_Wheel): %%Tst%%</li></ul></fieldset></div><div style=' width: 40em; display: block; text-align: left; '><form action='/eau' method='post'><fieldset><legend> Paramétres modifiable : </legend><div class='l12'><label for='IPL'>Impulsion par litre :</label></div><div class='c12'><input type='number' name='IPL' maxlength='5' value='%%IPL%%'></div><div class='r12'><label>i.p.l (dépend du coupteur)</label></div><div class='l12'><label for='DFE'>Si l'eau coule plus de :</label></div><div class='c12'><input type='number' name='DFE' value='%%DFE%%' maxlength='7'></div><div class='r12'><label>minutes, alors fermer la vanne</label></div><div class='l12'><label for='TOE'>Totalisateur eau :</label></div><div class='c12'><input type='number' name='TOE' value='%%TOE%%' maxlength='9'></div><div class='r12'><label>IRQ (pour nouvelle initalisation)</label></div><div class='myButton'><button type='submit'><b>Envoyer les nouvelles valeurs</b></button></div></fieldset></form><fieldset><legend> Action : </legend>Commande immediate :<button><a href=\"/eau?Cmd=open\" class=\"myButton\">Ouverture Vanne</a> </button><button><a href=\"/eau?Cmd=close\" class=\"myButton\">Fermeture Vanne</a> </button><button><a href=\"/eau?Cmd=reset\" class=\"myButton\">RAZ Surveillance</a> </button></fieldset><!-- Add command Open CLose Reset--></div><div style=' width: 40em; display: block; text-align: left; '><fieldset><legend> Parameters du contrôleur : </legend><ul><li>Logiciel version : <b>%%VL%%</b></li><li>Dérnier redémarrage le : <b>%%RB%%</b></li><li>Nom du contrôleur : <b>%%HOS%%</b></li><li>Adresse matériel mac : <b>%%MAC%%</b></li><li>Adresse internet IP : <b>%%IP%%</b></li><li>Disque Total/Utilisé : <b>%%MFREE%%</b> Kbytes</li></ul></fieldset></div></center></body></html>";
//---- len : 2918 bytes
//---- End Generated 

// -------- Web tranlat wrapper Get Set  -------------
char strtmp [20];
char* getRB()  {  return (char*) rebootTime.c_str();}
char* getCmd() {  return (char*)((isValveClosed)?("Fermé"):("Ouverte"));}
void  setCmd(String s) { 
   if (s=="open") actionOpen(); 
   else if (s=="close") actionClose(); 
   else if (s=="reset") actionReset(); 
}
char* getTOE() { snprintf(strtmp, sizeof(strtmp), "%llu", flux.getMagnetHallPluse()); return strtmp;}
void  setTOE(String s) { actionSetTotal( (uint64_t) s.toFloat() ); }
char* getMFREE() { 
  int a = SPIFFS.totalBytes()/1024; int b = SPIFFS.usedBytes()/1024;
  snprintf(strtmp, sizeof(strtmp), "%d/%d",  a, b);
  return strtmp;
}
char* getIPL() { snprintf(strtmp, sizeof(strtmp), "%.1f", jeedom.config.fluxReference); return strtmp; }
void  setIPL(String s) { jeedom.config.fluxReference=s.toFloat() ;}
char* getVL() { return (char*) VERSION; }
char* getHOS() { return frame.config.HostName;}
char* getIP() { snprintf(strtmp, sizeof(strtmp), "%s", WiFi.localIP().toString().c_str()); return strtmp;}
char* getTITLE() { snprintf(strtmp, sizeof(strtmp), "%s",frame.config.HostName); return strtmp;}
char* getMAC() { snprintf(strtmp, sizeof(strtmp), "%s",WiFi.macAddress().c_str()); return strtmp;}
char* getFL() {  return (char*) ((flame.state)?("Allumée"):("Eteinte")) ;}
char* getDFE() { snprintf(strtmp, sizeof(strtmp), "%.0f", jeedom.config.openDelay); return strtmp;}
void  setDFE(String s) { jeedom.config.openDelay = s.toFloat(); ;}
char* getDATE() { snprintf(strtmp, sizeof(strtmp), "%s",getDate().c_str()); return strtmp;}
char* getMT() { snprintf(strtmp, sizeof(strtmp), "%.3f", jeedom.config.waterM3); return strtmp;}
void  setMT(String s) { actionSetTotal( (uint64_t) s.toFloat()*1000.0) ;}
char* getTst() {snprintf(strtmp, sizeof(strtmp), "(%llu, %llu)", flux.getMagnetHallPluse(), flux.getPaddleWheelPulse()); return strtmp;}

struct Equiv {
  char key[10];
  char* (*get_ptr)(void);
  void  (*set_ptr)(String);
};

#define NBRITEMINDICO 15

Equiv dico[] ={
  {"%%RB%%"   , &getRB   ,  NULL   },
  {"%%Cmd%%"  , &getCmd  , &setCmd },
  {"%%TOE%%"  , &getTOE  , &setTOE },
  {"%%MFREE%%", &getMFREE,  NULL   },
  {"%%IPL%%"  , &getIPL  , &setIPL },
  {"%%VL%%"   , &getVL   ,  NULL   },
  {"%%HOS%%"  , &getHOS  ,  NULL   },
  {"%%IP%%"   , &getIP   ,  NULL   },
  {"%%TITLE%%", &getTITLE,  NULL   },
  {"%%MAC%%"  , &getMAC  ,  NULL   },
  {"%%FL%%"   , &getFL   ,  NULL   },
  {"%%DFE%%"  , &getDFE  , &setDFE },
  {"%%DATE%%" , &getDATE ,  NULL   },
  {"%%MT%%"   , &getMT   , &setMT  },
  {"%%Tst%%"  , &getTst  , NULL    },
};

String getKey(int i) {
  return  "%%"+frame.server.argName(i)+"%%";
}

void callbackSetWwm(int i){
  for (int idx=0; idx<NBRITEMINDICO; idx++) {
    if (getKey(i)==dico[idx].key  && dico[idx].set_ptr != NULL) {
      (*dico[idx].set_ptr)(frame.server.arg(i));
      return;
    }
  }
}

String callbackGetWwm(int i){
  for (int idx=0; idx<NBRITEMINDICO; idx++) {
    if (getKey(i)==dico[idx].key  && dico[idx].get_ptr!=NULL) {
      return (*dico[idx].get_ptr)();
    }
  }
  return "";
}

String sentHtmlEau(){
  // Http get or post action
  if (frame.server.method() == HTTP_POST || frame.server.method() == HTTP_GET) {
    for (uint8_t i=0; i<frame.server.args(); i++) { // Scan if Post or Get contain %%key%%
#ifdef DEBUG_MAIN
      Serial.printf("sentHtmlEau(Post or Get)  Arg->[%s]:[%s]\n\r", frame.server.argName(i).c_str(), frame.server.arg(i).c_str() );
#endif   
      if (frame.server.arg(i).isEmpty()) // Received a Get like "Cmd=" wihtout Arg will return "Ouvert" As text
        return callbackGetWwm(i); // Return immediate value only
      else
        callbackSetWwm(i);
    }
    // Save if changed
    if (jeedom.isCcrChanged()) saveConfigJeedom = true;
  }  
  // Return html page with substitution 
  String rt = HTTP_EAU; 
  for (int idx=0; idx<NBRITEMINDICO; idx++){ 
    if (dico[idx].get_ptr!=NULL) rt.replace(dico[idx].key, (*dico[idx].get_ptr)() ); 
  } 
  return rt; 
}

#endif
