#ifndef JKeyLedBuz_h
#define JKeyLedBuz_h

// Handles local controls: 2 buttons, RGB status LED, buzzer and valve relays.
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

    pinMode(pinBtUp, INPUT_PULLUP); // Button input (active low)
    pinMode(pinBtDn, INPUT_PULLUP); // Button input (active low)
    pinMode(pinValve, OUTPUT);      // Relay Valve
    digitalWrite(pinValve, HIGH);   // Initial direction state
    pinMode(pinBuzzer, OUTPUT);     // Buzzer output
    pinMode(pinSolidS, OUTPUT);     // Solid-state relay output
    digitalWrite(pinSolidS, HIGH);
  }

  void initValve(boolean state){
    // State mirrors software logic (true = closed request, false = open request).
    valveStat = state;
  } 

  // Apply valve state and return true when state changed.
  boolean setValve(boolean state) {
    boolean ret = (state != valveStat);
    // Stop drive relay while changing valve state.
    if (ret)
      digitalWrite(pinSolidS, LOW);   // SSR off during transition
    else
      digitalWrite(pinSolidS, HIGH);  // SSR on in steady state
    // Update valve direction pin.
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
    // Buttons are wired active-low.
    if ( digitalRead(pinBtUp) == 0 ) pressed = 1;
    if ( digitalRead(pinBtDn) == 0 ) pressed = (pressed==1)?(3):(2);
    long tnow = millis();
    // Key repeat gate
    if ( (tnow - repeatKey) > repeatMs) {
      repeatKey = tnow;
      if (lastKey==pressed) repeated++;
      else repeated = 0;
      lastKey = pressed;
    } else {
      pressed = 0;
    }
    // Trigger buzzer feedback
    if (pressed != 0 ) {
      timeBip = tnow;
      buzzer = true;
    }
    if ( buzzer && tnow-timeBip > 1000  ) {
       buzzer = false;
    }
    setBuzzer();
    // LED animation tick
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
    // Brightness modulation
    if (flip) dimmer++;
    else dimmer--;
      // Reduce LED intensity according to dimmer phase.
    if (dimmer==7 || dimmer==0)  flip = !flip;
    // rgb2 is a transient overlay color used for alerts/highlights.
    if ( rgb2!=0 && dimmer<2 ) {
      ledcWrite(0, ( rgb2 & 0x0F0000)>>(16+dimmer));
      ledcWrite(1, ( rgb2 & 0x000F00)>>(8+dimmer));
      ledcWrite(2, ( rgb2 & 0x00000F)>>dimmer);
    } else {
      // Smooth RGB variation
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
