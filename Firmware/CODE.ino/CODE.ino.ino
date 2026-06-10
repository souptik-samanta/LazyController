/*
  ============================================================
  XIAO ESP32-S3 — 4-Relay Web Controller
  ============================================================
  Hardware:
    GP1 → 1kΩ → BC547 Base → Collector → Relay 1 coil → VCC
    GP2 → 1kΩ → BC547 Base → Collector → Relay 2 coil → VCC
    GP3 → 1kΩ → BC547 Base → Collector → Relay 3 coil → VCC
    GP4 → 1kΩ → BC547 Base → Collector → Relay 4 coil → VCC
    All BC547 Emitters → GND
    Put a flyback diode (1N4007) across each relay coil (cathode to VCC)

  Usage:
    1. Set your WiFi credentials below
    2. Flash to XIAO ESP32-S3
    3. Open Serial Monitor (115200 baud) to see assigned IP
    4. Open that IP in browser on same network
  ============================================================
*/

#include <WiFi.h>
#include <WebServer.h>

// ── WiFi Credentials ─────────────────────────────────────
const char* WIFI_SSID     = "Souptik";
const char* WIFI_PASSWORD = "26596204";

// ── Relay GPIO Pins ───────────────────────────────────────
const int RELAY_PINS[4] = {1, 2, 3, 4};   // GP1, GP2, GP3, GP4
bool relayState[4]      = {false, false, false, false};

const char* RELAY_NAMES[4] = {"Relay 1", "Relay 2", "Relay 3", "Relay 4"};

WebServer server(80);

// ══════════════════════════════════════════════════════════
//  HTML Page (served from XIAO)
// ══════════════════════════════════════════════════════════
String buildPage() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>XIAO Relay Control</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700;900&display=swap');

  :root {
    --bg: #0a0e1a;
    --panel: #0f1628;
    --border: #1a2a4a;
    --accent: #00d4ff;
    --accent2: #ff6b35;
    --on: #00ff88;
    --off: #1a2a4a;
    --text: #c8d8f0;
    --dim: #4a6080;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Share Tech Mono', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 30px 16px;
    background-image:
      radial-gradient(ellipse at 20% 20%, rgba(0,212,255,0.04) 0%, transparent 60%),
      radial-gradient(ellipse at 80% 80%, rgba(255,107,53,0.04) 0%, transparent 60%),
      linear-gradient(rgba(0,212,255,0.015) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0,212,255,0.015) 1px, transparent 1px);
    background-size: 100% 100%, 100% 100%, 40px 40px, 40px 40px;
  }

  header {
    text-align: center;
    margin-bottom: 40px;
    animation: fadeDown 0.6s ease;
  }

  .logo {
    font-family: 'Orbitron', monospace;
    font-size: clamp(1.4rem, 5vw, 2.2rem);
    font-weight: 900;
    letter-spacing: 0.15em;
    color: var(--accent);
    text-shadow: 0 0 20px rgba(0,212,255,0.5), 0 0 40px rgba(0,212,255,0.2);
  }

  .subtitle {
    font-size: 0.75rem;
    color: var(--dim);
    letter-spacing: 0.3em;
    margin-top: 6px;
    text-transform: uppercase;
  }

  .status-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 8px 16px;
    margin-top: 14px;
    font-size: 0.72rem;
    color: var(--dim);
  }

  .dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--on);
    box-shadow: 0 0 8px var(--on);
    animation: pulse 2s infinite;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 16px;
    width: 100%;
    max-width: 480px;
  }

  .card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 24px 20px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    position: relative;
    overflow: hidden;
    transition: border-color 0.3s, box-shadow 0.3s;
    animation: fadeUp 0.5s ease both;
  }

  .card:nth-child(1) { animation-delay: 0.1s; }
  .card:nth-child(2) { animation-delay: 0.2s; }
  .card:nth-child(3) { animation-delay: 0.3s; }
  .card:nth-child(4) { animation-delay: 0.4s; }

  .card.on {
    border-color: var(--on);
    box-shadow: 0 0 20px rgba(0,255,136,0.15), inset 0 0 30px rgba(0,255,136,0.04);
  }

  .card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
    background: linear-gradient(90deg, transparent, var(--accent), transparent);
    opacity: 0;
    transition: opacity 0.3s;
  }

  .card.on::before { opacity: 1; background: linear-gradient(90deg, transparent, var(--on), transparent); }

  .relay-id {
    font-family: 'Orbitron', monospace;
    font-size: 0.65rem;
    letter-spacing: 0.2em;
    color: var(--dim);
    text-transform: uppercase;
  }

  .relay-icon {
    width: 52px; height: 52px;
    border-radius: 50%;
    border: 2px solid var(--border);
    display: flex; align-items: center; justify-content: center;
    font-size: 1.4rem;
    transition: all 0.3s;
    background: rgba(0,0,0,0.3);
  }

  .card.on .relay-icon {
    border-color: var(--on);
    box-shadow: 0 0 16px rgba(0,255,136,0.4);
    background: rgba(0,255,136,0.08);
  }

  .relay-name {
    font-family: 'Orbitron', monospace;
    font-size: 0.8rem;
    font-weight: 700;
    letter-spacing: 0.05em;
    color: var(--text);
  }

  .state-label {
    font-size: 0.65rem;
    letter-spacing: 0.25em;
    padding: 3px 10px;
    border-radius: 2px;
    transition: all 0.3s;
  }

  .card.on  .state-label { color: var(--on);  background: rgba(0,255,136,0.1); }
  .card.off .state-label { color: var(--dim); background: rgba(255,255,255,0.04); }

  .toggle-btn {
    width: 100%;
    padding: 10px;
    border-radius: 4px;
    border: 1px solid var(--border);
    background: rgba(0,212,255,0.06);
    color: var(--accent);
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.78rem;
    letter-spacing: 0.15em;
    cursor: pointer;
    text-transform: uppercase;
    transition: all 0.2s;
  }

  .toggle-btn:hover {
    background: rgba(0,212,255,0.12);
    border-color: var(--accent);
    box-shadow: 0 0 12px rgba(0,212,255,0.2);
  }

  .toggle-btn:active { transform: scale(0.97); }

  .card.on .toggle-btn {
    border-color: rgba(0,255,136,0.4);
    background: rgba(0,255,136,0.08);
    color: var(--on);
  }

  .card.on .toggle-btn:hover {
    background: rgba(0,255,136,0.15);
    box-shadow: 0 0 12px rgba(0,255,136,0.2);
  }

  .all-controls {
    display: flex;
    gap: 10px;
    width: 100%;
    max-width: 480px;
    margin-top: 20px;
    animation: fadeUp 0.5s 0.5s ease both;
  }

  .all-btn {
    flex: 1;
    padding: 12px;
    border-radius: 4px;
    border: 1px solid var(--border);
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.72rem;
    letter-spacing: 0.15em;
    text-transform: uppercase;
    cursor: pointer;
    transition: all 0.2s;
  }

  .all-on  { background: rgba(0,255,136,0.08); color: var(--on);  border-color: rgba(0,255,136,0.3); }
  .all-off { background: rgba(255,107,53,0.08); color: var(--accent2); border-color: rgba(255,107,53,0.3); }

  .all-on:hover  { background: rgba(0,255,136,0.15); box-shadow: 0 0 14px rgba(0,255,136,0.2); }
  .all-off:hover { background: rgba(255,107,53,0.15); box-shadow: 0 0 14px rgba(255,107,53,0.2); }

  footer {
    margin-top: 30px;
    font-size: 0.65rem;
    color: var(--dim);
    letter-spacing: 0.15em;
    animation: fadeUp 0.5s 0.6s ease both;
  }

  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }
  @keyframes fadeDown { from{opacity:0;transform:translateY(-16px)} to{opacity:1;transform:none} }
  @keyframes fadeUp   { from{opacity:0;transform:translateY(16px)}  to{opacity:1;transform:none} }
</style>
</head>
<body>

<header>
  <div class="logo">⚡ RELAY CTRL</div>
  <div class="subtitle">XIAO ESP32-S3 · GPIO Control Panel</div>
  <div class="status-bar">
    <div class="dot"></div>
    <span>CONNECTED · )rawhtml";

  html += WiFi.localIP().toString();

  html += R"rawhtml(</span>
  </div>
</header>

<div class="grid" id="relayGrid">
)rawhtml";

  // Build 4 relay cards
  for (int i = 0; i < 4; i++) {
    String onClass = relayState[i] ? "on" : "off";
    String stateText = relayState[i] ? "ON" : "OFF";
    String btnText   = relayState[i] ? "TURN OFF" : "TURN ON";
    String icons[4] = {"💡", "🔌", "⚙️", "🔋"};

    html += "<div class='card " + onClass + "' id='card" + String(i+1) + "'>";
    html += "<div class='relay-id'>GP" + String(i+1) + "</div>";
    html += "<div class='relay-icon'>" + icons[i] + "</div>";
    html += "<div class='relay-name'>RELAY " + String(i+1) + "</div>";
    html += "<div class='state-label'>" + stateText + "</div>";
    html += "<button class='toggle-btn' onclick=\"toggle(" + String(i+1) + ")\">" + btnText + "</button>";
    html += "</div>\n";
  }

  html += R"rawhtml(
</div>

<div class="all-controls">
  <button class="all-btn all-on"  onclick="allOn()">◼ ALL ON</button>
  <button class="all-btn all-off" onclick="allOff()">◻ ALL OFF</button>
</div>

<footer>XIAO ESP32-S3 · BC547 DRIVER · 4-CH RELAY</footer>

<script>
async function toggle(relay) {
  const res = await fetch('/toggle?relay=' + relay);
  const data = await res.json();
  updateCard(relay, data.state);
}

async function allOn() {
  const res = await fetch('/all?state=1');
  const data = await res.json();
  data.states.forEach((s, i) => updateCard(i+1, s));
}

async function allOff() {
  const res = await fetch('/all?state=0');
  const data = await res.json();
  data.states.forEach((s, i) => updateCard(i+1, s));
}

function updateCard(relay, state) {
  const card = document.getElementById('card' + relay);
  const label = card.querySelector('.state-label');
  const btn   = card.querySelector('.toggle-btn');
  if (state) {
    card.className = 'card on';
    label.textContent = 'ON';
    btn.textContent = 'TURN OFF';
  } else {
    card.className = 'card off';
    label.textContent = 'OFF';
    btn.textContent = 'TURN ON';
  }
}
</script>

</body>
</html>
)rawhtml";

  return html;
}

// ══════════════════════════════════════════════════════════
//  Route Handlers
// ══════════════════════════════════════════════════════════

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleToggle() {
  if (!server.hasArg("relay")) {
    server.send(400, "application/json", "{\"error\":\"missing relay param\"}");
    return;
  }
  int relay = server.arg("relay").toInt();
  if (relay < 1 || relay > 4) {
    server.send(400, "application/json", "{\"error\":\"relay must be 1-4\"}");
    return;
  }
  int idx = relay - 1;
  relayState[idx] = !relayState[idx];
  digitalWrite(RELAY_PINS[idx], relayState[idx] ? HIGH : LOW);

  String json = "{\"relay\":" + String(relay) +
                ",\"state\":" + (relayState[idx] ? "1" : "0") + "}";
  server.send(200, "application/json", json);

  Serial.printf("Relay %d → %s\n", relay, relayState[idx] ? "ON" : "OFF");
}

void handleAll() {
  if (!server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"missing state param\"}");
    return;
  }
  bool on = (server.arg("state") == "1");
  for (int i = 0; i < 4; i++) {
    relayState[i] = on;
    digitalWrite(RELAY_PINS[i], on ? HIGH : LOW);
  }
  String json = "{\"states\":[";
  for (int i = 0; i < 4; i++) {
    json += (relayState[i] ? "1" : "0");
    if (i < 3) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
  Serial.printf("All relays → %s\n", on ? "ON" : "OFF");
}

void handleStatus() {
  String json = "{\"relays\":[";
  for (int i = 0; i < 4; i++) {
    json += "{\"id\":" + String(i+1) + ",\"pin\":" + String(RELAY_PINS[i]) +
            ",\"state\":" + (relayState[i] ? "1" : "0") + "}";
    if (i < 3) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ══════════════════════════════════════════════════════════
//  Setup & Loop
// ══════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  // Init relay pins — all OFF at start
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  // Connect to WiFi
  Serial.printf("\nConnecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ Connected!");
    Serial.print("► Open browser at: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi failed. Check credentials and reset.");
    return;
  }

  // Register routes
  server.on("/",        handleRoot);
  server.on("/toggle",  handleToggle);
  server.on("/all",     handleAll);
  server.on("/status",  handleStatus);

  server.begin();
  Serial.println("► Web server started on port 80");
}

void loop() {
  server.handleClient();
}