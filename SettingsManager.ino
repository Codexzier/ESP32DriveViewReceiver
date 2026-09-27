// ========================================================================================
//      Einstellungen laden / speichern
//      Primaer:  NVS (Preferences) - das "EEPROM" des ESP32
//      Fallback: Textdatei auf der SD-Karte
// ========================================================================================

#define PREF_NAMESPACE  "fpvdrive"
#define PREF_KEY        "settings"
#define PREF_VERSION_KEY "version"
#define SETTINGS_VERSION 1

Preferences _prefs;
bool _sdAvailable = false;

void settingsSetDefaults(Settings &s) {
  s.throttle = { 1000, 1500, 2000, false };
  s.steering = { 1000, 1500, 2000, false };
  s.lightOffUs = 1000;
  s.lightOnUs = 2000;
  s.hornFreqHz = 2700;          // typische Resonanzfrequenz von Piezo-Summern
  s.hornDuty = 50;
  s.hornDurationMs = 500;
  s.frameSize = FRAMESIZE_VGA;
  s.jpegQuality = 12;
}

// Wertebereiche absichern, damit nie ungueltige Pulse an Servo/Regler gehen
void settingsSanitize(Settings &s) {
  ServoChannel *chs[2] = { &s.throttle, &s.steering };
  for (int i = 0; i < 2; i++) {
    ServoChannel *c = chs[i];
    c->minUs    = constrain(c->minUs,    500, 2500);
    c->maxUs    = constrain(c->maxUs,    500, 2500);
    c->centerUs = constrain(c->centerUs, 500, 2500);
  }
  s.lightOffUs     = constrain(s.lightOffUs, 500, 2500);
  s.lightOnUs      = constrain(s.lightOnUs,  500, 2500);
  s.hornFreqHz     = constrain(s.hornFreqHz, 100, 10000);
  s.hornDuty       = constrain(s.hornDuty, 1, 99);
  s.hornDurationMs = constrain(s.hornDurationMs, 50, 5000);
  if (s.frameSize > FRAMESIZE_UXGA) s.frameSize = FRAMESIZE_VGA;
  s.jpegQuality    = constrain(s.jpegQuality, 4, 63);
}

// ---------------------------------------------------------------- SD-Karte
bool sdInit() {
  if (_sdAvailable) return true;
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD-Karte nicht verfuegbar");
    return false;
  }
  _sdAvailable = true;
  Serial.println("SD-Karte bereit");
  return true;
}

bool sdSaveSettings(const Settings &s) {
  if (!sdInit()) return false;
  File f = SD.open(SD_CONFIG_FILE, FILE_WRITE);
  if (!f) return false;
  f.printf("version=%d\n", SETTINGS_VERSION);
  f.printf("thrMin=%d\nthrCenter=%d\nthrMax=%d\nthrInvert=%d\n",
           s.throttle.minUs, s.throttle.centerUs, s.throttle.maxUs, s.throttle.invert);
  f.printf("strMin=%d\nstrCenter=%d\nstrMax=%d\nstrInvert=%d\n",
           s.steering.minUs, s.steering.centerUs, s.steering.maxUs, s.steering.invert);
  f.printf("lightOff=%d\nlightOn=%d\n", s.lightOffUs, s.lightOnUs);
  f.printf("hornFreq=%u\nhornDuty=%u\nhornDuration=%u\n", s.hornFreqHz, s.hornDuty, s.hornDurationMs);
  f.printf("frameSize=%u\njpegQuality=%u\n", s.frameSize, s.jpegQuality);
  f.close();
  return true;
}

bool sdLoadSettings(Settings &s) {
  if (!sdInit()) return false;
  File f = SD.open(SD_CONFIG_FILE, FILE_READ);
  if (!f) return false;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    settingsApplyKeyValue(s, line.substring(0, eq), line.substring(eq + 1).toInt());
  }
  f.close();
  return true;
}

// Gemeinsame Zuordnung Schluessel -> Feld (fuer SD-Datei und Web-API)
bool settingsApplyKeyValue(Settings &s, const String &key, long v) {
  if      (key == "thrMin")       s.throttle.minUs = v;
  else if (key == "thrCenter")    s.throttle.centerUs = v;
  else if (key == "thrMax")       s.throttle.maxUs = v;
  else if (key == "thrInvert")    s.throttle.invert = v != 0;
  else if (key == "strMin")       s.steering.minUs = v;
  else if (key == "strCenter")    s.steering.centerUs = v;
  else if (key == "strMax")       s.steering.maxUs = v;
  else if (key == "strInvert")    s.steering.invert = v != 0;
  else if (key == "lightOff")     s.lightOffUs = v;
  else if (key == "lightOn")      s.lightOnUs = v;
  else if (key == "hornFreq")     s.hornFreqHz = v;
  else if (key == "hornDuty")     s.hornDuty = v;
  else if (key == "hornDuration") s.hornDurationMs = v;
  else if (key == "frameSize")    s.frameSize = v;
  else if (key == "jpegQuality")  s.jpegQuality = v;
  else return false;
  return true;
}

// ---------------------------------------------------------------- Oeffentliche Funktionen
// Rueckgabe: Speicherort als Text ("NVS", "SD", "Standard")
const char *settingsLoad(Settings &s) {
  settingsSetDefaults(s);

  if (_prefs.begin(PREF_NAMESPACE, true)) {
    bool ok = _prefs.getUChar(PREF_VERSION_KEY, 0) == SETTINGS_VERSION
           && _prefs.getBytesLength(PREF_KEY) == sizeof(Settings)
           && _prefs.getBytes(PREF_KEY, &s, sizeof(Settings)) == sizeof(Settings);
    _prefs.end();
    if (ok) {
      settingsSanitize(s);
      return "NVS";
    }
    settingsSetDefaults(s);
  }

  if (sdLoadSettings(s)) {
    settingsSanitize(s);
    return "SD";
  }

  settingsSetDefaults(s);
  return "Standard";
}

const char *settingsSave(const Settings &s) {
  if (_prefs.begin(PREF_NAMESPACE, false)) {
    bool ok = _prefs.putBytes(PREF_KEY, &s, sizeof(Settings)) == sizeof(Settings)
           && _prefs.putUChar(PREF_VERSION_KEY, SETTINGS_VERSION) == 1;
    _prefs.end();
    if (ok) return "NVS";
  }
  Serial.println("NVS speichern fehlgeschlagen - versuche SD-Karte");
  if (sdSaveSettings(s)) return "SD";
  return NULL;
}
