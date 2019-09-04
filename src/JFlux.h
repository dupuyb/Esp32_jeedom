#ifndef JFlux_h
#define JFlux_h

// Struct flux Stat Stop
struct FluxData {
  unsigned long logTimeMs;
};

class JFlux {
public:

  JFlux(int p) {
    interrupPin = p;
    pinMode(interrupPin, INPUT);
  }

  // Better than irq at 6-7 l/m irq signal is like:
  // _-------_--------_----- 1,6sec. Low and 7sec. High
  // New irq AND time > 0.5 sec
  void loop() {
    unsigned long now = millis();
    int val = digitalRead(interrupPin);
    // Evaluation every 500 milli-seconds
    if ( abs(now-data[1].logTimeMs) > 500 && interruptVal != val ) {
      if (val==0) {
        interruptCounter++;
        data[0].logTimeMs = data[1].logTimeMs;
        data[1].logTimeMs = now;
      }
      interruptVal = val;
    }
  }

  void setup(float totWaterM3, float implusionPerLitre) {
    // Default total counter
    interruptCounter = (uint64_t)(totWaterM3 * 1000.0 * (float)implusionPerLitre); // last value recorded in jeedom
    logCounter = interruptCounter;
    for (int i=0;i<2;i++)
       data[i].logTimeMs = millis();
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
      long ecartMs = abs(data[1].logTimeMs - data[0].logTimeMs);
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
  FluxData data[2];
  boolean lastState = false;
  uint64_t logCounter = 0; // Last implusionPerLitre at every seconds
  unsigned long logTimeEpoc = 0;
};

#endif
