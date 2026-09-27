// ========================================================================================
//      PWM Ausgaenge
//      D0 Vorwaerts/Rueckwaerts, D1 Lenkung, D2 Licht: Servo-PWM 50 Hz (1000..2000 us)
//      D3 Hupe: Rechtecksignal fuer Piezo-Summer (Standard 2,7 kHz, 50 %)
// ========================================================================================

#define SERVO_PERIOD_US (1000000UL / SERVO_FREQ_HZ)
#define SERVO_MAX_DUTY  ((1UL << SERVO_RES_BITS) - 1)
#define HORN_MAX_DUTY   ((1UL << HORN_RES_BITS) - 1)

bool _hornActive = false;

// Arduino-ESP32 Core 3.x und 2.x haben unterschiedliche LEDC-APIs
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define PWM_ATTACH(pin, ch, freq, res) ledcAttachChannel(pin, freq, res, ch)
  #define PWM_WRITE(pin, ch, duty)       ledcWrite(pin, duty)
  #define PWM_TONE(pin, ch, freq)        ledcChangeFrequency(pin, freq, HORN_RES_BITS)
#else
  #define PWM_ATTACH(pin, ch, freq, res) do { ledcSetup(ch, freq, res); ledcAttachPin(pin, ch); } while (0)
  #define PWM_WRITE(pin, ch, duty)       ledcWrite(ch, duty)
  #define PWM_TONE(pin, ch, freq)        ledcChangeFrequency(ch, freq, HORN_RES_BITS)
#endif

uint32_t usToDuty(int us) {
  return (uint32_t)((uint64_t)us * (SERVO_MAX_DUTY + 1) / SERVO_PERIOD_US);
}

// Wert -1..+1 auf Pulsbreite umrechnen. Mitte (Offset) und Endpunkte getrennt,
// damit eine verschobene Mitte die Endanschlaege nicht veraendert.
int valueToUs(float value, const ServoChannel &c) {
  value = constrain(value, -1.0f, 1.0f);
  if (c.invert) value = -value;
  if (value >= 0) return c.centerUs + (int)((c.maxUs - c.centerUs) * value);
  return c.centerUs + (int)((c.centerUs - c.minUs) * value);
}

void outputsInit(const Settings &s) {
  PWM_ATTACH(PIN_THROTTLE, LEDC_CH_THROTTLE, SERVO_FREQ_HZ, SERVO_RES_BITS);
  PWM_ATTACH(PIN_STEERING, LEDC_CH_STEERING, SERVO_FREQ_HZ, SERVO_RES_BITS);
  PWM_ATTACH(PIN_LIGHT,    LEDC_CH_LIGHT,    SERVO_FREQ_HZ, SERVO_RES_BITS);
  PWM_ATTACH(PIN_HORN,     LEDC_CH_HORN,     s.hornFreqHz,  HORN_RES_BITS);
  PWM_WRITE(PIN_HORN, LEDC_CH_HORN, 0);
  _hornActive = false;
}

// Wird zyklisch aus loop() aufgerufen
void outputsUpdate(ControlState &st, const Settings &s) {
  uint32_t now = millis();

  // Failsafe: keine Verbindung mehr -> Neutral
  if (now - st.lastUpdate > CONTROL_TIMEOUT_MS) {
    st.throttle = 0;
    st.steering = 0;
  }

  PWM_WRITE(PIN_THROTTLE, LEDC_CH_THROTTLE, usToDuty(valueToUs(st.throttle, s.throttle)));
  PWM_WRITE(PIN_STEERING, LEDC_CH_STEERING, usToDuty(valueToUs(st.steering, s.steering)));
  PWM_WRITE(PIN_LIGHT,    LEDC_CH_LIGHT,    usToDuty(st.light ? s.lightOnUs : s.lightOffUs));

  bool hornShouldRun = st.hornUntil != 0 && (int32_t)(st.hornUntil - now) > 0;
  if (hornShouldRun && !_hornActive) {
    PWM_TONE(PIN_HORN, LEDC_CH_HORN, s.hornFreqHz);
    PWM_WRITE(PIN_HORN, LEDC_CH_HORN, HORN_MAX_DUTY * s.hornDuty / 100);
    _hornActive = true;
  } else if (!hornShouldRun && _hornActive) {
    PWM_WRITE(PIN_HORN, LEDC_CH_HORN, 0);
    _hornActive = false;
    st.hornUntil = 0;
  }
}

void outputsHorn(ControlState &st, const Settings &s) {
  uint32_t until = millis() + s.hornDurationMs;
  if (until == 0) until = 1;
  st.hornUntil = until;
  _hornActive = false;  // bei erneutem Druck Frequenz/Tastverhaeltnis neu setzen
}
