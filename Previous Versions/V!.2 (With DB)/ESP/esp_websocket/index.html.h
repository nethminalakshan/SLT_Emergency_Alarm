// index.html.h — Lift Dashboard HTML stored in PROGMEM
// Auto-generated companion to esp_websocket.ino
// ============================================================
// To regenerate: run the Python helper or paste index.html
// contents between the R"==( ... )==" delimiters below.
// ============================================================

#pragma once

const char INDEX_HTML[] PROGMEM = R"==(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Lift Alarm Dashboard — SLTMobitel</title>
  <meta name="description" content="Real-time lift alarm monitoring dashboard for SLTMobitel building management. Live status across 3 zones via WebSocket." />

  <!-- Tailwind CSS CDN -->
  <script src="https://cdn.tailwindcss.com"></script>
  <script>
    tailwind.config = {
      theme: {
        extend: {
          fontFamily: { sans: ['Inter', 'sans-serif'] },
          animation: {
            'pulse-red':  'pulseRed 1.2s ease-in-out infinite',
            'spin-slow':  'spin 3s linear infinite',
          },
          keyframes: {
            pulseRed: {
              '0%,100%': { boxShadow: '0 0 0 0 rgba(239,68,68,0.7)'  },
              '50%':     { boxShadow: '0 0 0 18px rgba(239,68,68,0)'  },
            }
          }
        }
      }
    }
  </script>

  <!-- Google Fonts -->
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;700;900&display=swap" rel="stylesheet" />

  <style>
    /* ── Global reset & base ── */
    *, *::before, *::after { box-sizing: border-box; }
    body { margin: 0; background: #0a0f1e; color: #e2e8f0; font-family: 'Inter', sans-serif; }

    /* ── Glassmorphism card ── */
    .glass {
      background: rgba(255,255,255,0.05);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: 1.25rem;
      box-shadow: 0 8px 32px rgba(0,0,0,0.4);
    }

    /* ── Lift indicator circle ── */
    .lift-circle {
      width: 88px; height: 88px;
      border-radius: 50%;
      border: 3px solid rgba(255,255,255,0.18);
      transition: background-color 0.4s ease, box-shadow 0.4s ease;
    }
    .lift-circle.ok   { background: #22c55e; box-shadow: 0 0 14px 2px rgba(34,197,94,0.5); }
    .lift-circle.alarm {
      background: #ef4444;
      animation: pulseRed 1.2s ease-in-out infinite;
    }

    /* ── UNO badge ── */
    .uno-badge {
      display: inline-flex; align-items: center; gap: 6px;
      font-size: 0.72rem; font-weight: 600; letter-spacing: 0.05em;
      padding: 3px 10px; border-radius: 9999px;
      transition: background 0.3s, color 0.3s;
    }
    .uno-badge.active   { background: rgba(34,197,94,0.18); color: #4ade80; border: 1px solid #4ade80; }
    .uno-badge.inactive { background: rgba(239,68,68,0.18);  color: #f87171; border: 1px solid #f87171; }
    .uno-dot {
      width: 7px; height: 7px; border-radius: 50%;
      transition: background 0.3s;
    }
    .uno-badge.active   .uno-dot { background: #4ade80; }
    .uno-badge.inactive .uno-dot { background: #f87171; }

    /* ── WS connection banner ── */
    #ws-status { transition: opacity 0.4s; }

    /* ── Reset button ── */
    .reset-btn {
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.15);
      color: #cbd5e1;
      padding: 5px 18px;
      border-radius: 8px;
      font-size: 0.78rem;
      font-weight: 600;
      letter-spacing: 0.05em;
      cursor: pointer;
      transition: background 0.2s, transform 0.1s;
    }
    .reset-btn:hover  { background: rgba(255,255,255,0.16); }
    .reset-btn:active { transform: scale(0.93); }

    /* ── Glowing header text ── */
    .glow-text {
      text-shadow: 0 0 24px rgba(99,179,237,0.7), 0 0 48px rgba(99,179,237,0.3);
    }

    @keyframes pulseRed {
      0%,100% { box-shadow: 0 0 0 0 rgba(239,68,68,0.7); }
      50%     { box-shadow: 0 0 0 18px rgba(239,68,68,0); }
    }

    /* ── History log ── */
    #event-log { max-height: 220px; overflow-y: auto; scrollbar-width: thin; }
    #event-log::-webkit-scrollbar { width: 4px; }
    #event-log::-webkit-scrollbar-track { background: transparent; }
    #event-log::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.2); border-radius: 4px; }
  </style>
</head>
<body class="min-h-screen flex flex-col">

  <!-- ══════════════ HEADER ══════════════ -->
  <header class="flex items-center justify-between px-6 py-4 border-b border-white/10">
    <div class="flex items-center gap-4">
      <img src="https://upload.wikimedia.org/wikipedia/commons/e/ed/SLTMobitel_Logo.svg"
           alt="SLTMobitel" class="h-9 drop-shadow-lg" />
      <div>
        <h1 class="text-xl font-bold glow-text text-blue-300">Lift Alarm Dashboard</h1>
        <p class="text-xs text-slate-400">Real-time WebSocket Monitoring System</p>
      </div>
    </div>

    <!-- WS connection status badge -->
    <div id="ws-status"
         class="flex items-center gap-2 text-xs font-semibold px-3 py-1.5 rounded-full border
                border-yellow-400/40 text-yellow-300 bg-yellow-400/10">
      <span id="ws-dot" class="w-2 h-2 rounded-full bg-yellow-400 animate-pulse"></span>
      <span id="ws-label">Connecting…</span>
    </div>
  </header>

  <!-- ══════════════ ZONE GRID ══════════════ -->
  <main class="flex-1 px-4 py-6">
    <div id="zone-grid"
         class="grid gap-5 mx-auto max-w-6xl"
         style="grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));">

      <!-- Zone cards injected by JS -->
    </div>

    <!-- ══════════════ EVENT LOG ══════════════ -->
    <div class="glass mx-auto max-w-6xl mt-6 p-5">
      <h2 class="text-sm font-semibold text-slate-300 mb-3 uppercase tracking-widest">
        ⚡ Live Event Log
      </h2>
      <div id="event-log" class="space-y-1.5 text-xs font-mono text-slate-400">
        <p class="text-slate-600 italic">Waiting for events…</p>
      </div>
    </div>
  </main>

  <!-- ══════════════ FOOTER ══════════════ -->
  <footer class="text-center text-xs text-slate-600 py-3 border-t border-white/5">
    SLTMobitel Lift Monitoring — ESP32 WebSocket v2.0
  </footer>

  <!-- ══════════════ JAVASCRIPT ══════════════ -->
  <script>
  // ──────────────────────────────────────────
  //  CONFIGURATION
  // ──────────────────────────────────────────
  const ZONES = [
    { name: 'Lotus Side', id: 0 },
    { name: 'Duke Side',  id: 1 },
    { name: 'OTS',        id: 2 },
  ];
  // liftLabels[zone][slot] = display label
  const LIFT_LABELS = [
    ['Lift 1', 'Lift 2'],
    ['Lift 3', 'Lift 4'],
    ['Lift 5', 'Lift 6'],
  ];

  // State mirrors
  const liftState = [[false,false],[false,false],[false,false]];
  const unoActive = [false, false, false];

  // ──────────────────────────────────────────
  //  BUILD ZONE CARDS  (called once on load)
  // ──────────────────────────────────────────
  function buildUI() {
    const grid = document.getElementById('zone-grid');
    grid.innerHTML = '';
    ZONES.forEach(zone => {
      const card = document.createElement('div');
      card.className = 'glass p-5 flex flex-col gap-4';
      card.innerHTML = `
        <div class="flex items-center justify-between">
          <h2 class="font-bold text-base text-white/90">${zone.name}</h2>
          <span id="uno-badge-${zone.id}" class="uno-badge inactive">
            <span class="uno-dot"></span>UNO ${zone.id+1}
          </span>
        </div>
        <div class="flex gap-4 justify-center">
          ${[0,1].map(slot => `
            <div class="flex flex-col items-center gap-2">
              <div id="circle-${zone.id}-${slot}" class="lift-circle ok"></div>
              <span class="text-xs font-semibold text-slate-300">
                ${LIFT_LABELS[zone.id][slot]}
              </span>
              <div id="state-label-${zone.id}-${slot}"
                   class="text-xs font-bold text-green-400">
                OK
              </div>
              <button class="reset-btn"
                      id="btn-${zone.id}-${slot}"
                      onclick="resetLift(${zone.id},${slot})">
                RESET
              </button>
            </div>
          `).join('')}
        </div>`;
      grid.appendChild(card);
    });
  }

  // ──────────────────────────────────────────
  //  DOM UPDATERS
  // ──────────────────────────────────────────
  function setLift(zone, slot, isAlarm) {
    liftState[zone][slot] = isAlarm;
    const circle = document.getElementById(`circle-${zone}-${slot}`);
    const label  = document.getElementById(`state-label-${zone}-${slot}`);
    if (!circle) return;
    if (isAlarm) {
      circle.className = 'lift-circle alarm';
      label.textContent = 'ALARM';
      label.className = 'text-xs font-bold text-red-400';
    } else {
      circle.className = 'lift-circle ok';
      label.textContent = 'OK';
      label.className = 'text-xs font-bold text-green-400';
    }
  }

  function setUno(zone, active) {
    unoActive[zone] = active;
    const badge = document.getElementById(`uno-badge-${zone}`);
    if (!badge) return;
    badge.className = `uno-badge ${active ? 'active' : 'inactive'}`;
  }

  // ──────────────────────────────────────────
  //  EVENT LOG
  // ──────────────────────────────────────────
  const MAX_LOG = 50;
  function logEvent(msg, type = 'info') {
    const log   = document.getElementById('event-log');
    const empty = log.querySelector('p.italic');
    if (empty) empty.remove();

    const ts  = new Date().toLocaleTimeString();
    const row = document.createElement('div');
    row.className = 'flex gap-2';
    const colors = { alarm:'text-red-400', ok:'text-green-400', info:'text-blue-400', warn:'text-yellow-400' };
    row.innerHTML = `
      <span class="text-slate-600 shrink-0">${ts}</span>
      <span class="${colors[type] || 'text-slate-400'}">${msg}</span>`;
    log.prepend(row);
    // Keep log bounded
    while (log.children.length > MAX_LOG) log.lastChild.remove();
  }

  // ──────────────────────────────────────────
  //  RESET via WebSocket command
  // ──────────────────────────────────────────
  function resetLift(zone, slot) {
    if (!liftState[zone][slot]) return; // nothing to reset
    const cmd = JSON.stringify({ cmd: 'reset', zone, slot });
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(cmd);
      logEvent(`Reset command → ${LIFT_LABELS[zone][slot]} (zone ${zone})`, 'info');
    }
  }

  // ──────────────────────────────────────────
  //  WS STATUS BANNER
  // ──────────────────────────────────────────
  function setWsBanner(state) {
    const dot   = document.getElementById('ws-dot');
    const label = document.getElementById('ws-label');
    const el    = document.getElementById('ws-status');
    const styles = {
      connecting: ['border-yellow-400/40 text-yellow-300 bg-yellow-400/10', 'bg-yellow-400 animate-pulse', 'Connecting…'],
      open:       ['border-green-400/40  text-green-300  bg-green-400/10',  'bg-green-400',                'Live'],
      closed:     ['border-red-400/40    text-red-300    bg-red-400/10',    'bg-red-400 animate-pulse',    'Reconnecting…'],
    };
    const [elCls, dotCls, text] = styles[state];
    el.className = `flex items-center gap-2 text-xs font-semibold px-3 py-1.5 rounded-full border ${elCls}`;
    dot.className = `w-2 h-2 rounded-full ${dotCls}`;
    label.textContent = text;
  }

  // ──────────────────────────────────────────
  //  WEBSOCKET CLIENT — auto-reconnect
  // ──────────────────────────────────────────
  let ws;
  let reconnectDelay = 1000; // ms, grows on each failure

  function connectWS() {
    const url = `ws://${window.location.hostname}/ws`;
    ws = new WebSocket(url);
    setWsBanner('connecting');

    ws.onopen = () => {
      reconnectDelay = 1000;
      setWsBanner('open');
      logEvent('WebSocket connected to ESP32', 'info');
    };

    ws.onmessage = (event) => {
      let data;
      try { data = JSON.parse(event.data); } catch { return; }

      if (data.type === 'full') {
        // ── Initial full-state sync ──
        for (let z = 0; z < 3; z++) {
          setUno(z, data.uno[z] === 1);
          for (let s = 0; s < 2; s++) {
            setLift(z, s, data.lift[z][s] === 1);
          }
        }
        logEvent('Full state sync received', 'info');

      } else if (data.type === 'lift') {
        // ── Incremental lift event ──
        const isAlarm = data.status === 'ON';
        setLift(data.zone, data.slot, isAlarm);
        const name = LIFT_LABELS[data.zone][data.slot];
        logEvent(
          isAlarm
            ? `🔴 ALARM — ${name} (${ZONES[data.zone].name})`
            : `✅ CLEARED — ${name} (${ZONES[data.zone].name})`,
          isAlarm ? 'alarm' : 'ok'
        );

      } else if (data.type === 'uno') {
        // ── UNO health change ──
        setUno(data.zone, data.active === 1);
        logEvent(
          `UNO ${data.zone+1} (${ZONES[data.zone].name}) → ${data.active ? 'ONLINE' : 'OFFLINE'}`,
          data.active ? 'ok' : 'warn'
        );
      }
    };

    ws.onclose = () => {
      setWsBanner('closed');
      logEvent(`WebSocket closed. Reconnecting in ${reconnectDelay/1000}s…`, 'warn');
      setTimeout(() => {
        reconnectDelay = Math.min(reconnectDelay * 1.5, 15000);
        connectWS();
      }, reconnectDelay);
    };

    ws.onerror = (err) => {
      console.error('[WS] error', err);
      ws.close(); // triggers onclose → reconnect
    };
  }

  // ──────────────────────────────────────────
  //  INIT
  // ──────────────────────────────────────────
  buildUI();
  connectWS();
  </script>
</body>
</html>
)==";
