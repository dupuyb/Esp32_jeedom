#ifndef JFlux_h
#define JFlux_h

#define ANTIBOUNCEH 1000 //  _-------_--------_----- 1,6sec. Low and 7sec. High for 1 liter
#define ANTIBOUNCEI 20   // 20ms => 50 Pluses/sec => 490 P/liter

class JFlux {
public:

  JFlux(int p, int q) {
    pinHallSensor = p;
    pinPaddleWheel = q;
  }

  // _-------_--------_----- 1,6sec. Low and 7sec. High Max 7l/m
  // New irq AND time > 0.5 sec
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
    magnetHallPluse = (uint64_t)(totWaterM3 * 1000.0 * (float)implusionPerLitre); // last value recorded in jeedom
    magnetHallPulseOld = magnetHallPluse;
  }

  // _-------_--------__----- 1,6-1,7 sec Low 7-9 sec High
  boolean isChanged(struct tm *time, float implusionPerLitre) {
    if (magnetHallPulseOld != magnetHallPluse) { // Hall sensor
      magnetHallPulseOld = magnetHallPluse;
    } 
    if (paddleWheelPulseOld != paddleWheelPulse) { // PaddleWheel
      unsigned long ts = millis();
      // compute l/m ((v0-v1)/100) * ((60000/t0-t1)) 
      float dv = (float)(paddleWheelPulse - paddleWheelPulseOld); // Difference en litre
      float dt = 60000.0/(float)(ts - to); // milli-seconde 1000-61000= 1 minute
      literPerMinute = dt * dv;
      paddleWheelPulseOld = paddleWheelPulse;
      to = ts;
      state = true;
      cnt=0;
    } else {
      if (cnt>5) { // fix more than 5 sec
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
    vTaskDelay(5/portTICK_RATE_MS); // 5ms Filter 200Hz
    if (gpio_get_level((gpio_num_t)io_num) != v ) // If value changed 
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
 
  uint64_t getMagnetHallPluse() { return magnetHallPluse;}
  void setMagnetHallPluse(uint64_t v) { magnetHallPluse = v;}

  uint64_t getPaddleWheelPulse() { return paddleWheelPulse;}

 private :
  float    literPerMinute=0;
  uint64_t paddleWheelPulse=0;
  uint64_t magnetHallPluse=0;
  boolean  state ;
  int pinHallSensor; // pin Hall detector
  int pinPaddleWheel; // pin IR detector
  boolean lastState = false;
  uint64_t magnetHallPulseOld=0; // Last implusionPerLitre at every seconds
  uint64_t paddleWheelPulseOld=0;
  // Add Isr
  unsigned long timeHall;
  unsigned long timeIR;
  int hallLast, hallCrt;
  int irLast, irCrt;
  unsigned long to;
  uint8_t cnt=0;
};
#endif