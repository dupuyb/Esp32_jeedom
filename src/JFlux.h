#ifndef JFlux_h
#define JFlux_h

// Debug macro
#define DEBUG_FLUX

class JFlux {
public:

  JFlux(int p) {
    interrupPin = p;
  }

  // Better than irq at 6-7 l/m irq signal is like:
  // _-------_--------_----- 1,6sec. Low and 7sec. High
  // New irq AND time > 0.5 sec
  void loop(String dt) {
    unsigned long now = millis();
    int val = digitalRead(interrupPin);
    // Evaluation every 0.5 second
    if ( abs(now-logTimeMs[1]) > 500 && interruptVal != val ) {
      if (val==0) {
        interruptCounter++;
        logTimeMs[0] = logTimeMs[1];
        logTimeMs[1] = now;
#ifdef DEBUG_FLUX
        Serial.printf("JFlux.loop %s --> irq:%llu\n\r", dt.c_str(), interruptCounter);
#endif
      }
      interruptVal = val;
    }
  }

  void setup(float totWaterM3, float implusionPerLitre) {
    // Default total counter
    interruptCounter = (uint64_t)(totWaterM3 * 1000.0 * (float)implusionPerLitre); // last value recorded in jeedom
    logCounter = interruptCounter;
    for (int i=0;i<2;i++)
       logTimeMs[i] = millis();
#ifdef DEBUG_FLUX
    Serial.printf("JFlux.setup(tot:%.3f, ipl:%f)->irq:%llu \n\r", totWaterM3, implusionPerLitre, interruptCounter);
#endif
    // We don't use IRQ more stabilitie
    // pinMode(interrupPin, INPUT);
    // pinMode(interrupPin, INPUT_PULLUP);
    interruptVal = digitalRead(interrupPin);
  }

  // _-------_--------__----- 1,6-1,7 sec Low 7-9 sec High
  boolean isChanged(struct tm *time, float implusionPerLitre) {
    unsigned long intervalNow = mktime(time) - logTimeEpoc;
    if (logCounter != interruptCounter) {
      logCounter = interruptCounter;
      logTimeEpoc = mktime(time);
      long ecartMs = abs(logTimeMs[1] - logTimeMs[0]);
      if ( ecartMs > 0 ) literPerMinute = 60000.0 / (float)ecartMs;
      state = true;
    } else {
      if ( intervalNow > 30 ) {   // 30 seconds without any changing
        literPerMinute = 0.0;
        state = false;
      }
    }
    boolean ret = ((state==lastState)?(false):(true));
    lastState = state;
    return ret;
  }

  boolean  state = false;
  float    literPerMinute = 0;
  uint64_t interruptCounter = 0;
private :
  int interrupPin; // pin
  int interruptVal; // value
  boolean lastState = false;
  unsigned long logTimeMs[2];
  uint64_t logCounter = 0; // Last implusionPerLitre at every seconds
  unsigned long logTimeEpoc = 0;
};

#endif
