#ifndef JFlux_h
#define JFlux_h

class JFlux {
public:

  JFlux(int p) {
    interrupPin = p;
    pinMode(interrupPin, INPUT);
  }

  // Better than irq at 6-7 l/m irq signal is like:
  // _-------_--------_----- 1,6sec. Low and 7sec. High
  // New irq AND time > 0.5 sec
  uint8_t counterLo = 0;
  void loop() {
     unsigned long now = millis();
     valCnt = digitalRead(interrupPin);
     // Irq correct if low more than 1 sec
     if (valCnt==1) {
       counterLo = 0;
       timeMs[0] = now;
     } else {
       // Detect edge 1->0
       if (timeMs[0]==0) return;
       counterLo ++;
       timeMs[1] = now;
       unsigned long ecartMs = abs(timeMs[1] - timeMs[0]);
       if ( ecartMs > 1000 && counterLo > 5) {
          // Serial.printf("Flux.loop interruptCounter=%lu at:%lu ms\n\r", interruptCounter, now);
          interruptCounter++;
          timeMs[0] = 0;
          counterLo = 0;
       }
     }
   }

  void setup(float totWaterM3, float implusionPerLitre) {
    // Default total counter
    interruptCounter = (uint64_t)(totWaterM3 * 1000.0 * (float)implusionPerLitre); // last value recorded in jeedom
    logCounter = interruptCounter;
    for (int i=0;i<4;i++)
      timeMs[i] = millis();
    // Serial.printf("Flux.setup interruptCounter=%lu at:%lu ms\n\r", interruptCounter, timeMs[0]);
  }

  // _-------_--------__----- 1,6-1,7 sec Low 7-9 sec High
  boolean isChanged(struct tm *time, float implusionPerLitre) {
    unsigned long now = millis();
    unsigned long intervalNow = abs( now - timeMs[1] );
    if (logCounter != interruptCounter) { 
      logCounter = interruptCounter;
      // Compute liter.minute
      timeMs[2] = timeMs[3];
      timeMs[3] = now;
      unsigned long ecartMs = abs(timeMs[3] - timeMs[2]);
      if (ecartMs>0)
        literPerMinute = 60000.0 / (float)ecartMs;    
    //  Serial.printf("Flux.isChanged interruptCounter=%lu at:%lu ms liter=%f l.m ecartMs=%lu ms timeMs[2]=%lu timeMs[3]=%lu \n\r", interruptCounter, now, literPerMinute, ecartMs,timeMs[2],timeMs[3]);
      state = true;
    } else {
      if ( intervalNow > 30000 ) {   // 30 seconds without any changing
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
  unsigned long interruptCounter;
  int      valCnt;

private :
  int interrupPin; // pin
  unsigned long timeMs[4];
  boolean lastState = false;
  uint64_t logCounter = 0; // Last implusionPerLitre at every seconds
};

#endif
