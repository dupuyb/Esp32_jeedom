#ifndef JKeyLedBuz_h
#define JKeyLedBuz_h

class JKeyLedBuz {

public:
  JKeyLedBuz(int pr, int pg, int pb, int bu, int bd, int pv, int pbz, int pss){
    pinBtUp  = bu;
    pinBtDn  = bd;
    pinRed   = pr;
    pinGreen = pg;
    pinBlue  = pb;
    pinValve = pv;
    pinBuzzer = pbz;
    pinSolidS = pss;
    ledcSetup(0, 1500, 8); // Led channel 0 Red
    ledcSetup(1, 1500, 8); // Led channel 1 Green
    ledcSetup(2, 1500, 8); // Led channel 2 Blue

    // Attach channel to GPIO
    ledcAttachPin(pinRed,   0);
    ledcAttachPin(pinGreen, 1);
    ledcAttachPin(pinBlue,  2);

    pinMode(pinBtUp, INPUT_PULLUP); // set button pin as input
    pinMode(pinBtDn, INPUT_PULLUP); // set button pin as input#ifndef JFlux_h
    pinMode(pinValve, OUTPUT);      // Relay Valve
    digitalWrite(pinValve, HIGH);    // initial state
    pinMode(pinBuzzer, OUTPUT); // Buzzer
    pinMode(pinSolidS, OUTPUT); // initial value solidstate
    digitalWrite(pinSolidS, HIGH);
  }

  void initValve(boolean state){
    valveStat = state;
  } 

  // Set Valve and return true if state changed
  boolean setValve(boolean state) {
    boolean ret = (state != valveStat);
    // if  stat changing Stop valve
    if (ret) // Stop valve
      digitalWrite(pinSolidS, LOW);  // OFF SOLIDSTATERELAY
    else
      digitalWrite(pinSolidS, HIGH);  // OFF SOLIDSTATERELAY
    // Put other state
    if (state) {
      digitalWrite (pinValve, LOW); // DIR on
    } else {
      digitalWrite(pinValve, HIGH); // DIR off
    }
    valveStat = state;
    return ret;
  }

  uint8_t getKey(uint16_t repeatMs){
    uint8_t pressed = 0;
    if ( digitalRead(pinBtUp) == 0 ) pressed = 1;
    if ( digitalRead(pinBtDn) == 0 ) pressed = (pressed==1)?(3):(2);
    long tnow = millis();
    // Key repeat
    if ( (tnow - repeatKey) > repeatMs) {
      repeatKey = tnow;
      if (lastKey==pressed) repeated++;
      else repeated = 0;
      lastKey = pressed;
    } else {
      pressed = 0;
    }
    // bip
    if (pressed != 0 ) {
      timeBip = tnow;
      buzzer = true;
    }
    if ( buzzer && tnow-timeBip > 1000  ) {
       buzzer = false;
    }
    setBuzzer();
    // Led control
    if ( (tnow - prevTime) > 111) {
      actionLed();
      prevTime = tnow;
    }
    return pressed;
  }

  uint32_t rgb = 0;
  uint32_t rgb2 = 0;
  uint16_t repeated = 0;
  boolean buzzer = false;

  private:

  void actionLed() {
    // dimmer variation
    if (flip) dimmer++;
    else dimmer--;
      // Reduce LED Intensity FF become 0F And dimmer from 8 to 4
    if (dimmer==7 || dimmer==0)  flip = !flip;
    if ( rgb2!=0 && dimmer<2 ) {
      ledcWrite(0, ( rgb2 & 0x0F0000)>>(16+dimmer));
      ledcWrite(1, ( rgb2 & 0x000F00)>>(8+dimmer));
      ledcWrite(2, ( rgb2 & 0x00000F)>>dimmer);
    } else {
      // Led RGB smoth variation
      ledcWrite(0, (rgb & 0x0F0000)>>(16+dimmer));
      ledcWrite(1, (rgb & 0x000F00)>>(8+dimmer));
      ledcWrite(2, (rgb & 0x00000F)>>dimmer);
    }
  }

  void setBuzzer() {
    if (buzzer) {
      digitalWrite(pinBuzzer, HIGH);
    } else {
      digitalWrite(pinBuzzer, LOW);
    }
  }

  uint8_t pinBtUp;
  uint8_t pinBtDn;
  uint8_t pinRed;
  uint8_t pinGreen;
  uint8_t pinBlue;
  uint8_t pinValve;
  uint8_t pinBuzzer;
  uint8_t pinSolidS;
  boolean valveStat = false;
  boolean flip = true;
  uint8_t lastKey = 0;
  long repeatKey;
  long timeBip = 0;
  long prevTime;
  int dimmer = 0;
  uint8_t bip = 0;
};

#endif
