#ifndef JFlux_h
#define JFlux_h

#define ANTIBOUNCEH 1000 // Hall debounce in ms (sensor period roughly 1.6s low + 7s high per liter)
#define ANTIBOUNCEI 20   // IR debounce in ms (up to ~50 pulses/s)

// Flow helper:
// - Hall input tracks totalized volume (stable long period signal)
// - IR paddle input tracks instantaneous flow
class JFlux {
public:

  JFlux(int p, int q) {
    pinHallSensor = p;
    pinPaddleWheel = q;
  }

  // Poll debounced ISR shadow values and update pulse counters.
  void loop() {
    unsigned long now = millis();
    if ( (now - timeHall) > ANTIBOUNCEH && hallCrt!=hallLast) {
      hallCrt=hallLast;
      if (hallCrt==HIGH) {
        magnetHallPluse++;
      } 
    }
    if ( (now - timeIR) > ANTIBOUNCEI && irCrt!=irLast) {
      irCrt=irLast;
      paddleWheelPulse++;
    }
  }

  void setup(float totWaterM3, float implusionPerLitre) {
    // Restore pulse counters from persisted m3 value.
    magnetHallPluse = (uint64_t)(totWaterM3 * 1000.0 * (float)implusionPerLitre); // Last persisted counter from Jeedom config
    magnetHallPulseOld = magnetHallPluse;
  }

  // Detect flow state transitions and compute liters per minute.
  boolean isChanged(struct tm *time, float implusionPerLitre) {
    if (magnetHallPulseOld != magnetHallPluse) { // Hall sensor
      magnetHallPulseOld = magnetHallPluse;
    } 
    if (paddleWheelPulseOld != paddleWheelPulse) { // PaddleWheel
      unsigned long ts = millis();
      // Compute liters/min from pulse delta and elapsed time.
      float dv = (float)(paddleWheelPulse - paddleWheelPulseOld); // Pulse delta
      float dt = 60000.0/(float)(ts - to); // Scale to one minute
      literPerMinute = dt * dv;
      paddleWheelPulseOld = paddleWheelPulse;
      to = ts;
      state = true;
      cnt=0;
    } else {
      if (cnt>5) { // No pulse for more than 5 seconds
        literPerMinute = 0.0;
        state = false;
      } else {
        cnt++;
      }
    }
    boolean ret = ((state==lastState)?(false):(true));
    lastState = state;
    return ret;
  }

  void irq(uint32_t io_num, int v) {
    // This method is called by a worker task fed by ISR queue, not directly from hardware ISR.
    vTaskDelay(5/portTICK_RATE_MS); // 5 ms ISR-side settle filter
    if (gpio_get_level((gpio_num_t)io_num) != v ) // Ignore unstable edge
      return;
    if (io_num==pinHallSensor) {
      if (v!=hallLast) timeHall = millis();
      hallLast=v;
    }
    if (io_num==pinPaddleWheel) {
      if (v!=irLast) timeIR = millis();
      irLast=v;
    }
  }

  float getLiterPerMinute() { return literPerMinute; }

  boolean getState() { return state; }
 
  // Totalized pulse count from hall sensor (persistent water counter source).
  uint64_t getMagnetHallPluse() { return magnetHallPluse;}
  void setMagnetHallPluse(uint64_t v) { magnetHallPluse = v;}

  // Instant flow pulse count from paddle wheel.
  uint64_t getPaddleWheelPulse() { return paddleWheelPulse;}

 private :
  float    literPerMinute=0;
  uint64_t paddleWheelPulse=0;
  uint64_t magnetHallPluse=0;
  boolean  state ;
  int pinHallSensor; // Hall sensor GPIO
  int pinPaddleWheel; // IR paddle wheel GPIO
  boolean lastState = false;
  uint64_t magnetHallPulseOld=0; // Previous hall counter snapshot
  uint64_t paddleWheelPulseOld=0;
  // ISR shadow timing and edge tracking
  unsigned long timeHall;
  unsigned long timeIR;
  int hallLast, hallCrt;
  int irLast, irCrt;
  unsigned long to;
  uint8_t cnt=0;
};
#endif