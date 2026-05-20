#ifndef JFlame_h
#define JFlame_h

// Boiler flame monitor based on analog IR sensor input.
// State transitions are filtered with hysteresis and used to estimate ON duty cycle.
class JFlame {

public:

  JFlame(int pinA) {
    pinFlameAo = pinA;
    // Analog channel is sampled in software filter (no interrupt required).
    pinMode(pinFlameAo, INPUT);
  }

  // Call once per second with simple debounce/hysteresis filtering.
  bool isChanged(struct tm *time, uint32_t limit) {
    bool changed = false;
    // Low-pass filter to reduce noise before threshold comparison.
    value = ((value * 2) + analogRead(pinFlameAo)) / 3; // 33% new sample, 66% previous value (12-bit ADC)
    if (value < limit - (limit/10)) crtFlame = true; // Flame ON
    if (value > limit + (limit/10)) crtFlame = false; // Flame OFF
    if (crtFlame != state) changed = true;
    state = crtFlame;
    // Recompute duty ratio only when state changes.
    if (changed) {
      if (state) {
        // ---D0____U0----D1____U1--
        upTime[0] = upTime[1];
        upTime[1] = mktime(time);
        float up = (float)(dnTime[1]-upTime[0]);
        float tt = (float)(upTime[1]-upTime[0]);
        flamePerCent = (up /  tt ) * 100.0 ;
      } else {
        // ___U0----D0____U1-----D1__
        dnTime[0] = dnTime[1];
        dnTime[1] = mktime(time);
        float up = (float)(dnTime[1]-upTime[1]);
        float tt = (float)(dnTime[1]-dnTime[0]);
        flamePerCent = (up / tt ) * 100.0 ;
      }
      if (flamePerCent < 0) flamePerCent = 0;
      if (flamePerCent > 100.0) flamePerCent = 100.0;
    }
    return changed;
  }

  // Exposed state values
  bool state = false; // Flame state
  uint32_t value;     // IR analog value
  float flamePerCent = 0; // Flame ON duty cycle percentage

private:
  int pinFlameAo;     // Analog input pin
  bool crtFlame = false;
  // Edge timestamps used to estimate ON duty ratio.
  unsigned long upTime[2];
  unsigned long dnTime[2];
};

#endif
