#ifndef JKeyLedBuz_h
#define JKeyLedBuz_h

class JKeyLedBuz {

public:
  JKeyLedBuz(int pr, int pg, int pb, int bu, int bd, int pv, int pp, int pbz){
    pinBtUp  = bu;
    pinBtDn  = bd;
    pinRed   = pr;
    pinGreen = pg;
    pinBlue  = pb;
    pinValve = pv;
    pinPower = pp;
    pinBuzzer = pbz;
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
    pinMode(pinPower, OUTPUT);
    digitalWrite(pinValve, HIGH);    // initial state
    digitalWrite(pinPower, HIGH);
    pinMode(pinBuzzer, OUTPUT); // Buzzer

  }

  // Set Valve and return true if state changed
  boolean setValve(boolean state) {
    boolean ret = (state != valveStat);
    if (state) {
      digitalWrite (pinValve, LOW); // ON
    } else {
      digitalWrite(pinValve, HIGH);
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
    if (dimmer==8 || dimmer==0)  flip = !flip;
    if ( rgb2!=0 && dimmer<2 ) {
      ledcWrite(0, ( rgb2 & 0xFF0000)>>(16+dimmer));
      ledcWrite(1, ( rgb2 & 0x00FF00)>>(8+dimmer));
      ledcWrite(2, ( rgb2 & 0x0000FF)>>dimmer);
    } else {
      // Led RGB smoth variation
      ledcWrite(0, (rgb & 0xFF0000)>>(16+dimmer));
      ledcWrite(1, (rgb & 0x00FF00)>>(8+dimmer));
      ledcWrite(2, (rgb & 0x0000FF)>>dimmer);
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
  uint8_t pinPower;
  uint8_t pinBuzzer;
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
