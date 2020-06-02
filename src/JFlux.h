#ifndef JFlux_h
#define JFlux_h

// Interrupt if IR sensor SG-2BC detects the paddle wheel
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile uint16_t paddleWheelPluse;
volatile uint16_t paddleWheelPluseDsp;
void IRAM_ATTR irqPaddleWheel() {
  portENTER_CRITICAL_ISR(&mux);
  paddleWheelPluse++;
  portEXIT_CRITICAL_ISR(&mux);
}
// irq
volatile uint16_t magnetHallPluse;
int valCnt; 
void IRAM_ATTR irqMagnetHall() {
  portENTER_CRITICAL_ISR(&mux);
  valCnt=digitalRead(34);
  if (valCnt==HIGH) {
    magnetHallPluse++;
    paddleWheelPluseDsp = paddleWheelPluse;
    paddleWheelPluse=0;
  }
  portEXIT_CRITICAL_ISR(&mux);
}


class JFlux {
public:

  JFlux(int p, int q) {
    interrupPinL = p;
    pinPaddleWheel = q;
  }

  // Better than irq at 6-7 l/m irq signal is like:
  // _-------_--------_----- 1,6sec. Low and 7sec. High
  // New irq AND time > 0.5 sec
  uint8_t counterLo = 0;
  void loop() {
     unsigned long now = millis();
    //  valCnt = digitalRead(interrupPinL);
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
     for (int i = 0; i < 4; i++)
       timeMs[i] = millis();
     // Serial.printf("Flux.setup interruptCounter=%lu at:%lu ms\n\r", interruptCounter, timeMs[0]);
     // Test irq
     magnetHallPluse = 0;
     paddleWheelPluse = 0;
     // pinMode(interrupPinL, INPUT);
     pinMode(interrupPinL, INPUT_PULLUP);
     attachInterrupt(digitalPinToInterrupt(interrupPinL), irqMagnetHall, CHANGE);
     // pinMode(pinPaddleWheel, INPUT);
     pinMode(pinPaddleWheel, INPUT_PULLUP);
     attachInterrupt(digitalPinToInterrupt(pinPaddleWheel), irqPaddleWheel, FALLING);
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
 // int      valCnt;

private :
  int interrupPinL; // pin Hall detector
  int pinPaddleWheel; // pin IR detector
  unsigned long timeMs[4];
  boolean lastState = false;
  uint64_t logCounter = 0; // Last implusionPerLitre at every seconds
};

#endif
