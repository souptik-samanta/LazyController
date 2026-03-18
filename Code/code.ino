/*
  LazyController — 4 Relay WiFi Controller
  MCU: ESP32-C3-WROOM-02
  Transistors: BC547 (NPN, active HIGH)
  
  Relay GPIOs — adjust to match your PCB traces:
  Relay 1 → GPIO2
  Relay 2 → GPIO3
  Relay 3 → GPIO4
  Relay 4 → GPIO5
*/

#include <WiFi.h>
#include <WebServer.h>

// ─── Config ───────────────────────────────────────────────
const char* SSID     = "your_wifi_ssid";
const char* PASSWORD = "your_wifi_password";

const int RELAY_PINS[4] = {2, 3, 4, 5};  // adjust if needed
bool relayState[4]      = {false, false, false, false};

WebServer server(80);

// ─── HTML UI ──────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LazyController</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: monospace;
      background: #0f0f0f;
      color: #e0e0e0;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      padding: 40px 16px;
    }
    h1 { font-size: 1.4rem; letter-spacing: 0.15em; color: #fff; margin-bottom: 6px; }
    p.sub { font-size: 0.75rem; color: #555; margin-bottom: 40px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; width: 100%; max-width: 380px; }
    .card {
      background: #1a1a1a;
      border: 1px solid #2a2a2a;
      border-radius: 12px;
      padding: 24px 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 14px;
    }
    .label { font-size: 0.7rem; color: #777; letter-spacing: 0.1em; text-transform: uppercase; }
    .dot {
      width: 12px; height: 12px;
      border-radius: 50%;
      background: #2a2a2a;
      box-shadow: 0 0 0 3px #1a1a1a;
      transition: background 0.2s, box-shadow 0.2s;
    }
    .dot.on { background: #00e676; box-shadow: 0 0 8px #00e676aa; }
    .btn {
      width: 100%;
      padding: 10px;
      border: none;
      border-radius: 8px;
      background: #222;
      color: #ccc;
      font-family: monospace;
      font-size: 0.8rem;
      cursor: pointer;
      transition: background 0.15s, color 0.15s;
      letter-spacing: 0.08em;
    }
    .btn:hover { background: #2e2e2e; color: #fff; }
    .btn.on { background: #00c853; color: #000; }
    .btn.on:hover { background: #00e676; }
    .all-row { display: flex; gap: 10px; width: 100%; max-width: 380px; margin-top: 24px; }
    .all-row .btn { flex: 1; }
  </style>
</head>
<body>
  <h1>LazyController</h1>
  <p class="sub">ESP32-C3 · 4 relay · WiFi</p>

  <div class="grid" id="grid"></div>

  <div class="all-row">
    <button class="btn" onclick="allRelays(true)">ALL ON</button>
    <button class="btn" onclick="allRelays(false)">ALL OFF</button>
  </div>

  <script>
    let states = [false, false, false, false];

    function buildUI() {
      const grid = document.getElementById('grid');
      grid.innerHTML = '';
      states.forEach((on, i) => {
        grid.innerHTML += `
          <div class="card">
            <div class="dot ${on ? 'on' : ''}" id="dot${i}"></div>
            <span class="label">Relay ${i + 1}</span>
            <button class="btn ${on ? 'on' : ''}" onclick="toggle(${i})">
              ${on ? 'ON' : 'OFF'}
            </button>
          </div>`;
      });
    }

    async function fetchStates() {
      const res = await fetch('/state');
      states = await res.json();
      buildUI();
    }

    async function toggle(i) {
      await fetch(`/relay/${i + 1}/${states[i] ? 'off' : 'on'}`);
      fetchStates();
    }

    async function allRelays(on) {
      await fetch(`/all/${on ? 'on' : 'off'}`);
      fetchStates();
    }

    fetchStates();
    setInterval(fetchStates, 3000);
  </script>
</body>
</html>
)rawliteral";

// ─── Handlers ─────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleState() {
  String json = "[";
  for (int i = 0; i < 4; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 3) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleRelay() {
  // URL: /relay/{1-4}/{on|off}
  String uri = server.uri();  // e.g. /relay/2/on
  int relayNum = uri.charAt(7) - '0';  // char at index 7 = relay number
  bool turnOn  = uri.endsWith("on");

  if (relayNum >= 1 && relayNum <= 4) {
    int idx = relayNum - 1;
    relayState[idx] = turnOn;
    digitalWrite(RELAY_PINS[idx], turnOn ? HIGH : LOW);
    server.send(200, "text/plain", turnOn ? "on" : "off");
  } else {
    server.send(400, "text/plain", "bad relay number");
  }
}

void handleAll() {
  bool turnOn = server.uri().endsWith("on");
  for (int i = 0; i < 4; i++) {
    relayState[i] = turnOn;
    digitalWrite(RELAY_PINS[i], turnOn ? HIGH : LOW);
  }
  server.send(200, "text/plain", "ok");
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Init relay pins
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);  // relays OFF on boot
  }

  // Connect WiFi
  Serial.printf("\nConnecting to %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Routes
  server.on("/",          HTTP_GET, handleRoot);
  server.on("/state",     HTTP_GET, handleState);
  server.onNotFound([&]() {
    String uri = server.uri();
    if (uri.startsWith("/relay/")) handleRelay();
    else if (uri.startsWith("/all/"))  handleAll();
    else server.send(404, "text/plain", "not found");
  });

  server.begin();
  Serial.println("HTTP server started.");
}

// ─── Loop ─────────────────────────────────────────────────
void loop() {
  server.handleClient();
}