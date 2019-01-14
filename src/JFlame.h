#ifndef JFlame_h
#define JFlame_h

class JFlame {

public:

  JFlame(int pinA) {
    pinFlameAo = pinA;
  }

  // Call every seconds anti-rebond
  bool isChanged(struct tm *time, uint32_t limit) {
    bool changed = false;
    value = ((value * 2) + analogRead(pinFlameAo)) / 3; // new value 33% old value 66% ADC 12 bits
    if (value < limit - (limit/10)) crtFlame = true; // Flame ON
    if (value > limit + (limit/10)) crtFlame = false; // Flame OFF
    if (crtFlame != state) changed = true;
    state = crtFlame;
    // record changed only
    if (changed) {
      if (state) {
        upTime[0] = upTime[1];
        upTime[1] = mktime(time);
        float up = (float)(dnTime[1]-upTime[0]);
        float ud = (float)(upTime[1]-upTime[0]);
        flamePerCent = (up /  ud ) * 100.0 ;
      //  Serial.printf("On  up[1]=%lu up[0]=%lu flamePerCent=%f \r\n", upTime[1], upTime[0], flamePerCent );
      } else {
        dnTime[0] = dnTime[1];
        dnTime[1] = mktime(time);
        float up = (float)(dnTime[1]-upTime[1]);
        float ud = (float)(dnTime[1]-dnTime[0]);
        flamePerCent = (up / ud ) * 100.0 ;
      //  Serial.printf("Off dn[1]=%lu dn[0]=%lu flamePerCent=%f \r\n", dnTime[1], dnTime[0], flamePerCent );
      }
      if (flamePerCent < 0) flamePerCent = 0;
      if (flamePerCent > 100.0) flamePerCent = 100.0;
    }
    return changed;
  }

  // Final values
  bool state = false; // Flame status
  uint32_t value;     // IR analog value
  float flamePerCent = 0; // Percent ON
  int pinFlameAo;     // Analog input pin

private:
  bool crtFlame = false;
  unsigned long upTime[2];
  unsigned long dnTime[2];
};

#endif
