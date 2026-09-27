// ========================================================================================
//      HTML Seiten (werden direkt aus dem Flash ausgeliefert)
// ========================================================================================
#pragma once

// ----------------------------------------------------------------------------------------
// Steuerseite (Querformat)
//   links oben:   Setup
//   rechts oben:  Licht An/Aus, Hupe
//   links unten:  Gas vorwaerts/rueckwaerts (stufenlos, federt auf 0 zurueck)
//   rechts unten: Lenkung links/rechts      (stufenlos, federt auf Mitte zurueck)
//   Hintergrund:  Videostream
// ----------------------------------------------------------------------------------------
static const char DRIVE_PAGE[] = R"rawliteral(<!DOCTYPE html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>FPV Driver</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none}
html,body{margin:0;height:100%;overflow:hidden;background:#000;font-family:system-ui,sans-serif;color:#fff;touch-action:none;overscroll-behavior:none}
#video{position:fixed;inset:0;width:100%;height:100%;object-fit:cover;z-index:0;pointer-events:none}
body.fit #video{object-fit:contain}
#nostream{position:fixed;inset:0;display:flex;align-items:center;justify-content:center;color:#888;font-size:14px;z-index:0}
.btn{position:fixed;z-index:2;min-width:64px;height:48px;padding:0 16px;border-radius:12px;border:2px solid rgba(255,255,255,.6);
 background:rgba(0,0,0,.45);color:#fff;font-size:16px;font-weight:600;display:flex;align-items:center;justify-content:center;
 backdrop-filter:blur(4px);-webkit-backdrop-filter:blur(4px);text-decoration:none;cursor:pointer}
.btn:active,.btn.active{background:rgba(255,200,0,.75);color:#000;border-color:#ffc800}
.bar{position:fixed;z-index:2;top:max(12px,env(safe-area-inset-top));display:flex;gap:10px}
.bar .btn{position:static}
#left{left:max(12px,env(safe-area-inset-left))}
#right{right:max(12px,env(safe-area-inset-right))}
#fit{min-width:48px;padding:0 10px;font-size:14px}
#video.off{visibility:hidden}
.track{position:fixed;z-index:2;background:rgba(0,0,0,.35);border:2px solid rgba(255,255,255,.5);border-radius:40px;touch-action:none}
.track::before{content:"";position:absolute;background:rgba(255,255,255,.4)}
#thr{left:max(24px,env(safe-area-inset-left));bottom:max(20px,env(safe-area-inset-bottom));width:84px;height:min(62vh,300px)}
#thr::before{left:10px;right:10px;top:50%;height:2px}
#str{right:max(24px,env(safe-area-inset-right));bottom:max(20px,env(safe-area-inset-bottom));width:min(42vw,340px);height:84px}
#str::before{top:10px;bottom:10px;left:50%;width:2px}
.fill{position:absolute;background:rgba(255,200,0,.35);border-radius:36px;pointer-events:none}
.knob{position:absolute;width:68px;height:68px;border-radius:50%;background:rgba(255,255,255,.85);border:3px solid #ffc800;
 box-shadow:0 2px 10px rgba(0,0,0,.6);pointer-events:none;left:6px;top:6px}
.label{position:absolute;font-size:12px;color:rgba(255,255,255,.8);pointer-events:none;white-space:nowrap}
#hud{position:fixed;z-index:2;top:max(16px,env(safe-area-inset-top));left:50%;transform:translateX(-50%);font-size:13px;
 background:rgba(0,0,0,.45);padding:4px 10px;border-radius:8px;white-space:nowrap}
#hud.lost{background:rgba(200,0,0,.8)}
#rotate{display:none;position:fixed;inset:0;z-index:10;background:#000;color:#fff;align-items:center;justify-content:center;
 text-align:center;font-size:20px;padding:24px}
@media (orientation:portrait){#rotate{display:flex}}
</style></head>
<body>
<div id="nostream">Videostream wird geladen ...</div>
<img id="video" class="off" alt="">
<div id="left" class="bar">
  <a id="setup" class="btn" href="/setup">&#9881; Setup</a>
  <div id="fit" class="btn" title="Bild einpassen/fuellen">&#x26F6;</div>
</div>
<div id="right" class="bar">
  <div id="light" class="btn">&#128161; Licht</div>
  <div id="horn" class="btn">&#128227; Hupe</div>
</div>
<div id="hud">verbinde ...</div>

<div id="thr" class="track">
  <div class="fill"></div><div class="knob"></div>
  <span class="label" style="left:50%;top:-20px;transform:translateX(-50%)">Vor</span>
  <span class="label" style="left:50%;bottom:-18px;transform:translateX(-50%)">Zur&uuml;ck</span>
</div>
<div id="str" class="track">
  <div class="fill"></div><div class="knob"></div>
  <span class="label" style="left:8px;top:-20px">Links</span>
  <span class="label" style="right:8px;top:-20px">Rechts</span>
</div>

<div id="rotate">&#x21bb;<br>Bitte das Smartphone ins Querformat drehen.</div>

<script>
(function(){
  var state = { t: 0, s: 0, light: false };
  var lastSent = { t: null, s: null, time: 0 };
  var busy = false, okTime = Date.now();
  var hud = document.getElementById('hud');

  // ---------------- Videostream
  var video = document.getElementById('video');
  var nostream = document.getElementById('nostream');
  function startStream(){ video.src = 'http://' + location.hostname + ':81/stream?t=' + Date.now(); }
  video.onload = function(){ nostream.style.display = 'none'; video.classList.remove('off'); };
  video.onerror = function(){ video.classList.add('off'); nostream.style.display = 'flex'; nostream.textContent = 'Kein Videostream - neuer Versuch ...'; setTimeout(startStream, 1500); };
  startStream();

  // Bild fuellen / einpassen (wird im Browser gemerkt)
  try { if (localStorage.getItem('fit') === '1') document.body.classList.add('fit'); } catch(e){}
  document.getElementById('fit').addEventListener('click', function(){
    var on = document.body.classList.toggle('fit');
    try { localStorage.setItem('fit', on ? '1' : '0'); } catch(e){}
  });

  // ---------------- Vollbild + Querformat (sofern vom Browser erlaubt)
  function goFullscreen(){
    var el = document.documentElement;
    if (!document.fullscreenElement && el.requestFullscreen) {
      el.requestFullscreen().then(function(){
        if (screen.orientation && screen.orientation.lock) screen.orientation.lock('landscape').catch(function(){});
      }).catch(function(){});
    }
  }
  document.addEventListener('pointerdown', goFullscreen, { once: true });

  // ---------------- Slider (Multitouch faehig ueber Pointer Events)
  function makeSlider(id, vertical, onChange){
    var el = document.getElementById(id);
    var knob = el.querySelector('.knob');
    var fill = el.querySelector('.fill');
    var pointer = null;

    function render(v){
      var r = el.getBoundingClientRect();
      var k = 68, pad = 6;
      if (vertical) {
        var range = (r.height - 4 - k - 2 * pad) / 2;
        var cy = (r.height - 4) / 2 - k / 2;
        var y = cy - v * range;
        knob.style.top = y + 'px';
        var mid = (r.height - 4) / 2, pos = y + k / 2;
        fill.style.left = '6px'; fill.style.right = '6px';
        fill.style.top = Math.min(mid, pos) + 'px';
        fill.style.height = Math.abs(pos - mid) + 'px';
      } else {
        var rangeX = (r.width - 4 - k - 2 * pad) / 2;
        var cx = (r.width - 4) / 2 - k / 2;
        var x = cx + v * rangeX;
        knob.style.left = x + 'px';
        var midX = (r.width - 4) / 2, posX = x + k / 2;
        fill.style.top = '6px'; fill.style.bottom = '6px';
        fill.style.left = Math.min(midX, posX) + 'px';
        fill.style.width = Math.abs(posX - midX) + 'px';
      }
    }
    function valueFrom(e){
      var r = el.getBoundingClientRect();
      var k = 68, pad = 6, v;
      if (vertical) {
        var range = (r.height - k - 2 * pad) / 2;
        v = ((r.top + r.height / 2) - e.clientY) / range;
      } else {
        var rangeX = (r.width - k - 2 * pad) / 2;
        v = (e.clientX - (r.left + r.width / 2)) / rangeX;
      }
      v = Math.max(-1, Math.min(1, v));
      if (Math.abs(v) < 0.03) v = 0;   // kleine Totzone in der Mitte
      return Math.round(v * 1000) / 1000;
    }
    function set(v){ render(v); onChange(v); }

    el.addEventListener('pointerdown', function(e){
      e.preventDefault();
      if (pointer !== null) return;
      pointer = e.pointerId;
      try { el.setPointerCapture(e.pointerId); } catch(err){}
      set(valueFrom(e));
    });
    el.addEventListener('pointermove', function(e){
      if (e.pointerId !== pointer) return;
      e.preventDefault();
      set(valueFrom(e));
    });
    function release(e){
      if (e.pointerId !== pointer) return;
      pointer = null;
      set(0);   // Feder zurueck in Neutralstellung
    }
    el.addEventListener('pointerup', release);
    el.addEventListener('pointercancel', release);
    el.addEventListener('lostpointercapture', release);
    window.addEventListener('resize', function(){ render(vertical ? state.t : state.s); });
    render(0);
  }

  makeSlider('thr', true,  function(v){ state.t = v; send(); });
  makeSlider('str', false, function(v){ state.s = v; send(); });

  // ---------------- Steuerdaten senden (max. ca. 20/s, Heartbeat alle 200 ms)
  function send(force){
    var now = Date.now();
    var changed = state.t !== lastSent.t || state.s !== lastSent.s;
    if (busy) return;
    if (!force && !changed) return;
    if (!force && now - lastSent.time < 40) return;
    busy = true;
    lastSent.t = state.t; lastSent.s = state.s; lastSent.time = now;
    var ctrl = new AbortController();
    var to = setTimeout(function(){ ctrl.abort(); }, 800);
    fetch('/api/control?t=' + state.t + '&s=' + state.s, { cache: 'no-store', signal: ctrl.signal })
      .then(function(r){ return r.json(); })
      .then(function(j){ okTime = Date.now(); setLight(!!j.light); })
      .catch(function(){})
      .then(function(){ clearTimeout(to); busy = false; if (state.t !== lastSent.t || state.s !== lastSent.s) send(); });
  }
  setInterval(function(){
    send(Date.now() - lastSent.time > 200);
    var lost = Date.now() - okTime > 1000;
    hud.classList.toggle('lost', lost);
    hud.textContent = lost ? 'Verbindung verloren!' :
      ('Gas ' + Math.round(state.t * 100) + ' %  |  Lenkung ' + Math.round(state.s * 100) + ' %');
  }, 50);

  // Seite verlassen / im Hintergrund -> Neutral
  document.addEventListener('visibilitychange', function(){
    if (document.hidden) { state.t = 0; state.s = 0; fetch('/api/control?t=0&s=0', { keepalive: true }).catch(function(){}); }
  });

  // ---------------- Licht + Hupe
  var lightBtn = document.getElementById('light');
  function setLight(on){ state.light = on; lightBtn.classList.toggle('active', on); }
  lightBtn.addEventListener('click', function(){
    fetch('/api/light?on=' + (state.light ? 0 : 1), { cache: 'no-store' })
      .then(function(r){ return r.json(); }).then(function(j){ setLight(!!j.light); }).catch(function(){});
  });
  var hornBtn = document.getElementById('horn');
  hornBtn.addEventListener('pointerdown', function(e){
    e.preventDefault();
    hornBtn.classList.add('active');
    setTimeout(function(){ hornBtn.classList.remove('active'); }, 500);
    fetch('/api/horn', { cache: 'no-store' }).catch(function(){});
  });
})();
</script>
</body></html>
)rawliteral";


// ----------------------------------------------------------------------------------------
// Setup-Seite
// ----------------------------------------------------------------------------------------
static const char SETUP_PAGE[] = R"rawliteral(<!DOCTYPE html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>FPV Driver - Setup</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:#15171a;color:#eee;font-family:system-ui,sans-serif;font-size:15px}
header{position:sticky;top:0;z-index:5;display:flex;gap:8px;align-items:center;flex-wrap:wrap;padding:10px 16px;background:#202328;border-bottom:1px solid #333}
header h1{font-size:18px;margin:0 auto 0 0}
button,.btn,select,input{font:inherit}
button,.btn{background:#2d3138;color:#eee;border:1px solid #555;border-radius:8px;padding:8px 14px;cursor:pointer;text-decoration:none}
button.primary{background:#ffc800;color:#000;border-color:#ffc800;font-weight:600}
button:active{filter:brightness(1.3)}
main{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px;padding:14px 16px;max-width:1200px;margin:0 auto}
section{background:#202328;border:1px solid #333;border-radius:12px;padding:12px 14px}
section h2{font-size:16px;margin:0 0 10px;color:#ffc800}
.row{display:grid;grid-template-columns:120px 1fr 72px;gap:8px;align-items:center;margin:6px 0}
.row input[type=range]{width:100%}
.row input[type=number]{width:72px;background:#15171a;color:#eee;border:1px solid #555;border-radius:6px;padding:4px}
.check{display:flex;gap:8px;align-items:center;margin:8px 0}
.cam{display:flex;gap:10px;align-items:center;flex-wrap:nowrap}
select{background:#15171a;color:#eee;border:1px solid #555;border-radius:6px;padding:6px}
#fps{min-width:90px;font-weight:600;color:#7fdc7f;white-space:nowrap}
#preview.off{display:none}
#preview{width:100%;max-height:220px;object-fit:contain;background:#000;border-radius:8px;margin-top:10px}
#msg{font-size:13px;color:#aaa}
.hint{font-size:12px;color:#999;margin:4px 0 0}
</style></head>
<body>
<header>
  <h1>&#9881; Setup</h1>
  <span id="msg"></span>
  <button id="defaults">Standardwerte</button>
  <button id="save" class="primary">Speichern</button>
  <a class="btn" href="/">&#10132; Zur Steuerung</a>
</header>
<main>
  <section>
    <h2>Vorw&auml;rts / R&uuml;ckw&auml;rts (D0)</h2>
    <div id="thr"></div>
    <label class="check"><input type="checkbox" data-key="thrInvert"> Richtung umkehren</label>
    <p class="hint">Pulsbreite in &micro;s. &quot;Mitte&quot; ist der Offset f&uuml;r Neutral (Regler steht).</p>
  </section>
  <section>
    <h2>Lenkung Links / Rechts (D1)</h2>
    <div id="str"></div>
    <label class="check"><input type="checkbox" data-key="strInvert"> Richtung umkehren</label>
    <p class="hint">&quot;Mitte&quot; ist der Offset f&uuml;r Geradeauslauf.</p>
  </section>
  <section>
    <h2>Licht (D2)</h2>
    <div id="light"></div>
    <button id="lightTest">Licht umschalten</button> <span id="lightState"></span>
  </section>
  <section>
    <h2>Hupe (D3, Piezo)</h2>
    <div id="horn"></div>
    <button id="hornTest">Hupe testen</button>
    <p class="hint">Piezo-Summer sind meist bei 2&ndash;4 kHz am lautesten (Standard 2700 Hz, 50 %).</p>
  </section>
  <section>
    <h2>Kamera</h2>
    <div class="cam">
      <label for="frameSize">Aufl&ouml;sung</label>
      <select id="frameSize"></select>
      <span id="fps">&ndash;</span>
    </div>
    <div id="cam"></div>
    <img id="preview" class="off" alt="">
    <p class="hint">Nach dem &Auml;ndern der Aufl&ouml;sung wird der Videostream 5 Sekunden lang gemessen.</p>
  </section>
</main>
<script>
(function(){
  var FIELDS = {
    thr:   [['thrMin','Min (R&uuml;ckw.)',500,2500], ['thrCenter','Mitte',500,2500], ['thrMax','Max (Vorw.)',500,2500]],
    str:   [['strMin','Min (Links)',500,2500], ['strCenter','Mitte',500,2500], ['strMax','Max (Rechts)',500,2500]],
    light: [['lightOff','Aus [&micro;s]',500,2500], ['lightOn','An [&micro;s]',500,2500]],
    horn:  [['hornFreq','Frequenz [Hz]',100,10000], ['hornDuty','Tastverh. [%]',1,99], ['hornDuration','Dauer [ms]',50,5000]],
    cam:   [['jpegQuality','JPEG Qualit&auml;t',4,63]]
  };
  var msg = document.getElementById('msg');
  var cfg = {};
  var timers = {};

  function api(path){
    return fetch(path, { cache: 'no-store' }).then(function(r){ return r.json(); });
  }
  function show(j){
    cfg = j;
    Object.keys(FIELDS).forEach(function(g){
      FIELDS[g].forEach(function(f){
        var k = f[0];
        document.querySelectorAll('[data-key="' + k + '"]').forEach(function(el){
          if (document.activeElement !== el) el.value = j[k];
        });
      });
    });
    document.querySelectorAll('input[type=checkbox][data-key]').forEach(function(el){ el.checked = !!j[el.dataset.key]; });
    var sel = document.getElementById('frameSize');
    if (!sel.options.length) {
      j.frameSizes.forEach(function(o){
        var opt = document.createElement('option');
        opt.value = o.v; opt.textContent = o.n; sel.appendChild(opt);
      });
    }
    sel.value = j.frameSize;
    document.getElementById('lightState').textContent = j.light ? 'An' : 'Aus';
    msg.textContent = (j.message ? j.message + ' | ' : '') + 'Speicher: ' + j.storage + (j.psram ? '' : ' | kein PSRAM!');
  }

  // Zeilen mit Schieberegler + Zahlenfeld aufbauen
  Object.keys(FIELDS).forEach(function(g){
    var box = document.getElementById(g);
    FIELDS[g].forEach(function(f){
      var row = document.createElement('div');
      row.className = 'row';
      row.innerHTML = '<label>' + f[1] + '</label>' +
        '<input type="range" min="' + f[2] + '" max="' + f[3] + '" data-key="' + f[0] + '">' +
        '<input type="number" min="' + f[2] + '" max="' + f[3] + '" data-key="' + f[0] + '">';
      box.appendChild(row);
    });
  });

  // Aenderungen sofort (verzoegert) an den ESP senden -> live testbar
  function setValue(key, value){
    document.querySelectorAll('[data-key="' + key + '"]').forEach(function(el){
      if (el.type === 'checkbox') el.checked = !!value; else el.value = value;
    });
    clearTimeout(timers[key]);
    timers[key] = setTimeout(function(){
      api('/api/set?' + key + '=' + encodeURIComponent(value)).then(show).catch(function(){ msg.textContent = 'Verbindungsfehler'; });
    }, 150);
  }
  document.querySelectorAll('[data-key]').forEach(function(el){
    // Zahlenfelder erst nach der Eingabe senden, sonst springen Servos bei Zwischenwerten
    el.addEventListener(el.type === 'number' ? 'change' : 'input', function(){
      setValue(el.dataset.key, el.type === 'checkbox' ? (el.checked ? 1 : 0) : el.value);
    });
  });

  document.getElementById('save').addEventListener('click', function(){
    api('/api/save').then(show).catch(function(){ msg.textContent = 'Verbindungsfehler'; });
  });
  document.getElementById('defaults').addEventListener('click', function(){
    if (confirm('Alle Werte auf Standard zuruecksetzen? (Erst mit "Speichern" dauerhaft)'))
      api('/api/defaults').then(function(j){ show(j); measureFps(); });
  });
  document.getElementById('lightTest').addEventListener('click', function(){
    api('/api/light').then(function(j){ document.getElementById('lightState').textContent = j.light ? 'An' : 'Aus'; });
  });
  document.getElementById('hornTest').addEventListener('click', function(){ fetch('/api/horn'); });

  // ---------------- Videostream-Vorschau (wird fuer die FPS Messung benoetigt)
  var preview = document.getElementById('preview');
  function startStream(){ preview.src = 'http://' + location.hostname + ':81/stream?t=' + Date.now(); }
  preview.onload = function(){ preview.classList.remove('off'); };
  preview.onerror = function(){ preview.classList.add('off'); setTimeout(startStream, 1500); };
  startStream();

  // ---------------- Aufloesung + FPS Messung (5 Sekunden)
  var fpsEl = document.getElementById('fps');
  var measuring = 0;
  function measureFps(){
    var id = ++measuring;
    var left = 5;
    fpsEl.style.color = '#ccc';
    fpsEl.textContent = 'messe ... ' + left + ' s';
    // kurz warten, bis die neue Aufloesung im Stream ankommt
    setTimeout(function(){
      if (id !== measuring) return;
      api('/api/fps/start').then(function(){
        var iv = setInterval(function(){
          if (id !== measuring) { clearInterval(iv); return; }
          left--;
          if (left > 0) { fpsEl.textContent = 'messe ... ' + left + ' s'; return; }
          clearInterval(iv);
          api('/api/fps').then(function(j){
            if (id !== measuring) return;
            fpsEl.style.color = j.fps >= 20 ? '#7fdc7f' : (j.fps >= 10 ? '#ffc800' : '#ff6b6b');
            fpsEl.textContent = j.clients ? (j.fps.toFixed(1) + ' FPS') : 'kein Stream aktiv';
          });
        }, 1000);
      });
    }, 700);
  }
  document.getElementById('frameSize').addEventListener('change', function(e){
    api('/api/set?frameSize=' + e.target.value).then(function(j){ show(j); measureFps(); })
      .catch(function(){ msg.textContent = 'Verbindungsfehler'; });
  });

  api('/api/config').then(show).catch(function(){ msg.textContent = 'Verbindungsfehler'; });
})();
</script>
</body></html>
)rawliteral";
