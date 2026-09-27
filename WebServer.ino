// ========================================================================================
//      Webserver
//      Port 80: Steuerseite "/", Setup-Seite "/setup" und die API "/api/..."
//      Port 81: MJPEG Videostream "/stream" (eigener Server, damit der Stream
//               die Steuerbefehle nicht blockiert)
// ========================================================================================

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t _httpServer = NULL;
httpd_handle_t _streamServer = NULL;

// FPS Messung (zaehlt die tatsaechlich an den Client gesendeten Bilder)
volatile uint32_t _fpsFrames = 0;
volatile uint32_t _fpsStart = 0;
volatile uint8_t  _streamClients = 0;

// Verfuegbare Aufloesungen fuer die ComboBox
struct FrameSizeOption { uint8_t value; const char *name; };
static const FrameSizeOption FRAME_SIZES[] = {
  { FRAMESIZE_QQVGA,  "QQVGA 160x120" },
  { FRAMESIZE_QCIF,   "QCIF 176x144" },
  { FRAMESIZE_HQVGA,  "HQVGA 240x176" },
  { FRAMESIZE_240X240,"240x240" },
  { FRAMESIZE_QVGA,   "QVGA 320x240" },
  { FRAMESIZE_CIF,    "CIF 400x296" },
  { FRAMESIZE_HVGA,   "HVGA 480x320" },
  { FRAMESIZE_VGA,    "VGA 640x480" },
  { FRAMESIZE_SVGA,   "SVGA 800x600" },
  { FRAMESIZE_XGA,    "XGA 1024x768" },
  { FRAMESIZE_HD,     "HD 1280x720" },
  { FRAMESIZE_SXGA,   "SXGA 1280x1024" },
  { FRAMESIZE_UXGA,   "UXGA 1600x1200" },
};

// ---------------------------------------------------------------- Hilfsfunktionen
static bool getQueryParam(httpd_req_t *req, const char *key, char *value, size_t len) {
  size_t qlen = httpd_req_get_url_query_len(req) + 1;
  if (qlen <= 1 || qlen > 512) return false;
  char query[512];
  if (httpd_req_get_url_query_str(req, query, qlen) != ESP_OK) return false;
  return httpd_query_key_value(query, key, value, len) == ESP_OK;
}

static esp_err_t sendJson(httpd_req_t *req, const char *json) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t sendConfigJson(httpd_req_t *req, const char *message) {
  char json[1400];
  int n = snprintf(json, sizeof(json),
    "{\"thrMin\":%d,\"thrCenter\":%d,\"thrMax\":%d,\"thrInvert\":%d,"
    "\"strMin\":%d,\"strCenter\":%d,\"strMax\":%d,\"strInvert\":%d,"
    "\"lightOff\":%d,\"lightOn\":%d,"
    "\"hornFreq\":%u,\"hornDuty\":%u,\"hornDuration\":%u,"
    "\"frameSize\":%u,\"jpegQuality\":%u,\"light\":%d,"
    "\"storage\":\"%s\",\"message\":\"%s\",\"psram\":%d,\"frameSizes\":[",
    _settings.throttle.minUs, _settings.throttle.centerUs, _settings.throttle.maxUs, _settings.throttle.invert,
    _settings.steering.minUs, _settings.steering.centerUs, _settings.steering.maxUs, _settings.steering.invert,
    _settings.lightOffUs, _settings.lightOnUs,
    _settings.hornFreqHz, _settings.hornDuty, _settings.hornDurationMs,
    _settings.frameSize, _settings.jpegQuality, _control.light,
    _settingsStorage, message ? message : "", psramFound() ? 1 : 0);

  size_t count = sizeof(FRAME_SIZES) / sizeof(FRAME_SIZES[0]);
  for (size_t i = 0; i < count && n < (int)sizeof(json) - 64; i++) {
    n += snprintf(json + n, sizeof(json) - n, "%s{\"v\":%u,\"n\":\"%s\"}",
                  i ? "," : "", FRAME_SIZES[i].value, FRAME_SIZES[i].name);
  }
  snprintf(json + n, sizeof(json) - n, "]}");
  return sendJson(req, json);
}

// ---------------------------------------------------------------- Seiten
static esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, DRIVE_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t setupHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, SETUP_PAGE, HTTPD_RESP_USE_STRLEN);
}

// ---------------------------------------------------------------- Steuerung
// /api/control?t=-1..1&s=-1..1   (wird ca. alle 50 ms gesendet, dient auch als Heartbeat)
static esp_err_t controlHandler(httpd_req_t *req) {
  char value[16];
  if (getQueryParam(req, "t", value, sizeof(value))) _control.throttle = constrain(atof(value), -1.0, 1.0);
  if (getQueryParam(req, "s", value, sizeof(value))) _control.steering = constrain(atof(value), -1.0, 1.0);
  _control.lastUpdate = millis();

  char json[48];
  snprintf(json, sizeof(json), "{\"light\":%d}", _control.light);
  return sendJson(req, json);
}

// /api/light?on=0|1   (ohne Parameter: umschalten)
static esp_err_t lightHandler(httpd_req_t *req) {
  char value[4];
  if (getQueryParam(req, "on", value, sizeof(value))) _control.light = atoi(value) != 0;
  else _control.light = !_control.light;

  char json[32];
  snprintf(json, sizeof(json), "{\"light\":%d}", _control.light);
  return sendJson(req, json);
}

// /api/horn   (Hupe fuer die eingestellte Dauer, Standard 500 ms)
static esp_err_t hornHandler(httpd_req_t *req) {
  outputsHorn(_control, _settings);
  return sendJson(req, "{\"horn\":1}");
}

// ---------------------------------------------------------------- Setup API
// /api/config            -> aktuelle Einstellungen
static esp_err_t configHandler(httpd_req_t *req) {
  return sendConfigJson(req, NULL);
}

// /api/set?key=value&... -> Einstellungen im RAM uebernehmen (sofort wirksam, noch nicht gespeichert)
static esp_err_t setHandler(httpd_req_t *req) {
  static const char *KEYS[] = {
    "thrMin", "thrCenter", "thrMax", "thrInvert",
    "strMin", "strCenter", "strMax", "strInvert",
    "lightOff", "lightOn", "hornFreq", "hornDuty", "hornDuration",
    "frameSize", "jpegQuality"
  };

  uint8_t oldFrameSize = _settings.frameSize;
  uint8_t oldQuality = _settings.jpegQuality;

  char value[16];
  for (size_t i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); i++) {
    if (getQueryParam(req, KEYS[i], value, sizeof(value))) {
      settingsApplyKeyValue(_settings, String(KEYS[i]), atol(value));
    }
  }
  settingsSanitize(_settings);

  const char *message = "uebernommen (nicht gespeichert)";
  if (_settings.frameSize != oldFrameSize && !cameraSetFrameSize(_settings.frameSize)) {
    _settings.frameSize = oldFrameSize;
    message = "Aufloesung wird nicht unterstuetzt";
  }
  if (_settings.jpegQuality != oldQuality) cameraSetQuality(_settings.jpegQuality);

  return sendConfigJson(req, message);
}

// /api/save              -> dauerhaft speichern (NVS, sonst SD-Karte)
static esp_err_t saveHandler(httpd_req_t *req) {
  const char *where = settingsSave(_settings);
  if (where) {
    _settingsStorage = where;
    return sendConfigJson(req, where[0] == 'N' ? "gespeichert (NVS)" : "gespeichert (SD-Karte)");
  }
  return sendConfigJson(req, "FEHLER: Speichern fehlgeschlagen");
}

// /api/defaults          -> Standardwerte laden (noch nicht gespeichert)
static esp_err_t defaultsHandler(httpd_req_t *req) {
  settingsSetDefaults(_settings);
  cameraSetFrameSize(_settings.frameSize);
  cameraSetQuality(_settings.jpegQuality);
  return sendConfigJson(req, "Standardwerte geladen (nicht gespeichert)");
}

// /api/fps/start         -> FPS Messung starten
// /api/fps               -> Ergebnis seit Start
static esp_err_t fpsStartHandler(httpd_req_t *req) {
  _fpsFrames = 0;
  _fpsStart = millis();
  return sendJson(req, "{\"started\":1}");
}

static esp_err_t fpsHandler(httpd_req_t *req) {
  uint32_t elapsed = millis() - _fpsStart;
  float fps = elapsed > 0 ? _fpsFrames * 1000.0f / elapsed : 0;
  char json[96];
  snprintf(json, sizeof(json), "{\"frames\":%u,\"ms\":%u,\"fps\":%.1f,\"clients\":%u}",
           (unsigned)_fpsFrames, (unsigned)elapsed, fps, (unsigned)_streamClients);
  return sendJson(req, json);
}

// ---------------------------------------------------------------- Videostream
static esp_err_t streamHandler(httpd_req_t *req) {
  if (!_cameraReady) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "camera not ready");
    return ESP_FAIL;
  }

  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  _streamClients = _streamClients + 1;
  char header[64];

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      delay(5);
      continue;
    }

    size_t hlen = snprintf(header, sizeof(header), STREAM_PART, (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, header, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (res != ESP_OK) break;   // Client hat die Verbindung getrennt
    _fpsFrames = _fpsFrames + 1;
  }

  _streamClients = _streamClients - 1;
  return res;
}

// ---------------------------------------------------------------- Start
static void registerHandler(httpd_handle_t server, const char *uri, esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t def = {};
  def.uri = uri;
  def.method = HTTP_GET;
  def.handler = handler;
  def.user_ctx = NULL;
  httpd_register_uri_handler(server, &def);
}

void webServerStart() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = HTTP_PORT;
  config.ctrl_port = 32768;
  config.max_uri_handlers = 16;
  config.lru_purge_enable = true;

  if (httpd_start(&_httpServer, &config) == ESP_OK) {
    registerHandler(_httpServer, "/",             indexHandler);
    registerHandler(_httpServer, "/setup",        setupHandler);
    registerHandler(_httpServer, "/api/control",  controlHandler);
    registerHandler(_httpServer, "/api/light",    lightHandler);
    registerHandler(_httpServer, "/api/horn",     hornHandler);
    registerHandler(_httpServer, "/api/config",   configHandler);
    registerHandler(_httpServer, "/api/set",      setHandler);
    registerHandler(_httpServer, "/api/save",     saveHandler);
    registerHandler(_httpServer, "/api/defaults", defaultsHandler);
    registerHandler(_httpServer, "/api/fps/start",fpsStartHandler);
    registerHandler(_httpServer, "/api/fps",      fpsHandler);
    Serial.printf("Webserver laeuft auf Port %d\n", HTTP_PORT);
  } else {
    Serial.println("Webserver konnte nicht gestartet werden");
  }

  httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();
  streamConfig.server_port = STREAM_PORT;
  streamConfig.ctrl_port = 32769;
  streamConfig.max_uri_handlers = 2;
  streamConfig.lru_purge_enable = true;
  streamConfig.stack_size = 8192;

  if (httpd_start(&_streamServer, &streamConfig) == ESP_OK) {
    registerHandler(_streamServer, "/stream", streamHandler);
    Serial.printf("Videostream laeuft auf Port %d\n", STREAM_PORT);
  } else {
    Serial.println("Streamserver konnte nicht gestartet werden");
  }
}
