#ifndef PAGE_INDEX_H
#define PAGE_INDEX_H

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>PulseBand ESP32 Dashboard</title>
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin />
  <link href="https://fonts.googleapis.com/css2?family=Sora:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet" />
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --bg-light: #f5f5f7;
      --bg-dark: #000000;
      --surface-light: #ffffff;
      --surface-dark: #1d1d1f;
      --text-light: #1d1d1f;
      --text-dark: #f5f5f7;
      --text-secondary-light: #656566;
      --text-secondary-dark: #a1a1a6;
      --accent: #0071e3;
      --accent-light: #0077ed;
      --danger: #ff3b30;
      --ok: #34c759;
      --shadow-sm: 0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.24);
      --shadow-md: 0 3px 6px rgba(0, 0, 0, 0.15), 0 2px 4px rgba(0, 0, 0, 0.12);
      --shadow-lg: 0 10px 40px rgba(0, 0, 0, 0.16);
    }

    html.dark-mode {
      color-scheme: dark;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      font-family: "Sora", -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      color: var(--text-light);
      background: var(--bg-light);
      transition: background-color 0.5s ease, color 0.5s ease;
      line-height: 1.6;
    }

    html.dark-mode body {
      color: var(--text-dark);
      background: var(--bg-dark);
    }

    .wrap {
      width: min(1280px, 95vw);
      margin: 0 auto;
      padding: 60px 20px;
      display: grid;
      gap: 40px;
    }

    .hero {
      display: grid;
      gap: 20px;
      text-align: center;
      padding: 80px 20px;
      margin-bottom: 40px;
    }

    .hero h1 {
      margin: 0;
      font-size: clamp(2rem, 7vw, 3.5rem);
      font-weight: 700;
      line-height: 1.1;
      letter-spacing: -0.02em;
    }

    .hero .sub {
      margin: 0;
      color: var(--text-secondary-light);
      max-width: 600px;
      margin: 0 auto;
      font-size: 1.125rem;
      line-height: 1.6;
    }

    html.dark-mode .hero .sub {
      color: var(--text-secondary-dark);
    }

    .tag {
      width: fit-content;
      margin: 0 auto;
      font-family: "JetBrains Mono", monospace;
      font-size: 0.875rem;
      font-weight: 500;
      letter-spacing: 0.05em;
      color: var(--accent);
      background: transparent;
      border: none;
      padding: 0;
    }

    .grid {
      display: grid;
      gap: 32px;
      grid-template-columns: repeat(12, 1fr);
      margin-bottom: 40px;
    }

    .panel {
      background: var(--surface-light);
      border: 1px solid rgba(0, 0, 0, 0.06);
      border-radius: 20px;
      padding: 40px;
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    }

    html.dark-mode .panel {
      background: var(--surface-dark);
      border-color: rgba(255, 255, 255, 0.1);
    }

    .panel:hover {
      border-color: rgba(0, 0, 0, 0.1);
      box-shadow: var(--shadow-md);
    }

    html.dark-mode .panel:hover {
      border-color: rgba(255, 255, 255, 0.15);
    }

    .live {
      grid-column: span 7;
      display: grid;
      gap: 32px;
    }

    .about {
      grid-column: span 5;
      display: grid;
      gap: 24px;
      align-content: start;
    }

    .about h2 {
      margin: 0 0 16px 0;
      font-size: 1.5rem;
      font-weight: 600;
      letter-spacing: -0.015em;
    }

    .stats {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 16px;
    }

    .kpi {
      background: rgba(0, 0, 0, 0.02);
      border: none;
      border-radius: 16px;
      padding: 20px;
      transition: all 0.3s ease;
    }

    html.dark-mode .kpi {
      background: rgba(255, 255, 255, 0.08);
    }

    .kpi:hover {
      background: rgba(0, 0, 0, 0.04);
    }

    html.dark-mode .kpi:hover {
      background: rgba(255, 255, 255, 0.12);
    }

    .kpi .name {
      color: var(--text-secondary-light);
      font-size: 0.875rem;
      font-weight: 500;
      margin-bottom: 8px;
    }

    html.dark-mode .kpi .name {
      color: var(--text-secondary-dark);
    }

    .kpi .value {
      font-family: "JetBrains Mono", monospace;
      font-size: 1.75rem;
      font-weight: 600;
      color: var(--accent);
      letter-spacing: -0.02em;
    }

    .pulse-shell {
      display: flex;
      gap: 40px;
      align-items: center;
      flex-wrap: wrap;
      padding: 20px 0;
    }

    .meter {
      width: 160px;
      height: 160px;
      border-radius: 50%;
      display: grid;
      place-items: center;
      border: 12px solid rgba(0, 0, 0, 0.08);
      background: conic-gradient(var(--accent) 0deg, var(--accent) 0deg, rgba(0, 0, 0, 0.08) 0deg);
      position: relative;
      transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
      box-shadow: 0 8px 24px rgba(0, 113, 227, 0.15);
    }

    html.dark-mode .meter {
      border-color: rgba(255, 255, 255, 0.1);
      box-shadow: 0 8px 24px rgba(0, 113, 227, 0.25);
    }

    .meter::after {
      content: "";
      position: absolute;
      inset: 18px;
      background: var(--surface-light);
      border-radius: 50%;
      z-index: 0;
    }

    html.dark-mode .meter::after {
      background: var(--surface-dark);
    }

    .meter-value {
      position: relative;
      z-index: 1;
      text-align: center;
      font-family: "JetBrains Mono", monospace;
    }

    .meter-value b {
      display: block;
      font-size: 2.25rem;
      font-weight: 600;
      line-height: 1;
      color: var(--text-light);
      letter-spacing: -0.02em;
    }

    html.dark-mode .meter-value b {
      color: var(--text-dark);
    }

    .meter-value span {
      color: var(--text-secondary-light);
      font-size: 0.875rem;
      font-weight: 500;
      margin-top: 4px;
      display: block;
    }

    html.dark-mode .meter-value span {
      color: var(--text-secondary-dark);
    }

    .controls {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      margin-top: 20px;
    }

    button {
      border: none;
      border-radius: 12px;
      padding: 10px 20px;
      font: inherit;
      font-weight: 600;
      font-size: 0.9375rem;
      cursor: pointer;
      transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
      outline: none;
    }

    .btn-primary {
      background: var(--accent);
      color: white;
      box-shadow: 0 4px 12px rgba(0, 113, 227, 0.3);
    }

    .btn-primary:hover {
      background: var(--accent-light);
      box-shadow: 0 6px 16px rgba(0, 113, 227, 0.4);
      transform: translateY(-1px);
    }

    .btn-primary:active {
      transform: translateY(0);
    }

    .btn-secondary {
      background: rgba(0, 0, 0, 0.08);
      color: var(--text-light);
    }

    html.dark-mode .btn-secondary {
      background: rgba(255, 255, 255, 0.1);
      color: var(--text-dark);
    }

    .btn-secondary:hover {
      background: rgba(0, 0, 0, 0.12);
      transform: translateY(-1px);
    }

    html.dark-mode .btn-secondary:hover {
      background: rgba(255, 255, 255, 0.15);
    }

    .btn-secondary:active {
      transform: translateY(0);
    }

    .btn-neutral {
      background: rgba(0, 0, 0, 0.04);
      color: var(--accent);
      border: 1px solid rgba(0, 113, 227, 0.2);
    }

    html.dark-mode .btn-neutral {
      background: rgba(255, 255, 255, 0.05);
      border-color: rgba(0, 113, 227, 0.3);
    }

    .btn-neutral:hover {
      background: rgba(0, 0, 0, 0.08);
      border-color: rgba(0, 113, 227, 0.3);
      transform: translateY(-1px);
    }

    html.dark-mode .btn-neutral:hover {
      background: rgba(255, 255, 255, 0.1);
    }

    .chart-wrap {
      border: 1px solid rgba(0, 0, 0, 0.06);
      border-radius: 16px;
      padding: 20px;
      background: rgba(0, 0, 0, 0.01);
      overflow: hidden;
    }

    html.dark-mode .chart-wrap {
      border-color: rgba(255, 255, 255, 0.1);
      background: rgba(255, 255, 255, 0.03);
    }

    .log {
      grid-column: span 12;
      display: grid;
      gap: 16px;
    }

    .log h2 {
      margin: 0;
      font-size: 1.5rem;
      font-weight: 600;
      letter-spacing: -0.015em;
    }

    textarea {
      width: 100%;
      min-height: 200px;
      resize: vertical;
      border: 1px solid rgba(0, 0, 0, 0.06);
      border-radius: 16px;
      background: rgba(0, 0, 0, 0.01);
      color: var(--text-light);
      font-family: "JetBrains Mono", monospace;
      font-size: 0.875rem;
      padding: 16px;
      transition: all 0.3s ease;
    }

    html.dark-mode textarea {
      border-color: rgba(255, 255, 255, 0.1);
      background: rgba(255, 255, 255, 0.03);
      color: var(--text-dark);
    }

    textarea:focus {
      outline: none;
      border-color: var(--accent);
      background: rgba(0, 113, 227, 0.02);
    }

    html.dark-mode textarea:focus {
      background: rgba(0, 113, 227, 0.08);
    }

    .status-row {
      display: flex;
      gap: 16px;
      flex-wrap: wrap;
      align-items: center;
      color: var(--text-secondary-light);
      font-size: 0.9375rem;
      font-weight: 500;
    }

    html.dark-mode .status-row {
      color: var(--text-secondary-dark);
    }

    .dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      display: inline-block;
      margin-right: 8px;
      background: #ffaa00;
      box-shadow: 0 0 0 8px rgba(255, 170, 0, 0.15);
      transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    }

    .dot.live {
      background: var(--ok);
      box-shadow: 0 0 0 8px rgba(52, 199, 89, 0.15);
    }

    .flow {
      margin: 0;
      padding-left: 24px;
      line-height: 1.8;
      color: var(--text-secondary-light);
    }

    html.dark-mode .flow {
      color: var(--text-secondary-dark);
    }

    .flow li {
      margin-bottom: 12px;
    }

    .mono {
      font-family: "JetBrains Mono", monospace;
      color: var(--accent);
      font-weight: 500;
      background: rgba(0, 113, 227, 0.08);
      padding: 2px 6px;
      border-radius: 4px;
    }

    .dark-mode-toggle {
      position: fixed;
      top: 24px;
      right: 24px;
      background: var(--surface-light);
      border: 1px solid rgba(0, 0, 0, 0.06);
      border-radius: 50%;
      width: 44px;
      height: 44px;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      font-size: 1.25rem;
      transition: all 0.3s ease;
      z-index: 100;
    }

    html.dark-mode .dark-mode-toggle {
      background: var(--surface-dark);
      border-color: rgba(255, 255, 255, 0.1);
    }

    .dark-mode-toggle:hover {
      box-shadow: var(--shadow-md);
      transform: scale(1.05);
    }

    @keyframes fadeIn {
      from {
        opacity: 0;
        transform: translateY(20px);
      }
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }

    main {
      animation: fadeIn 0.6s cubic-bezier(0.4, 0, 0.2, 1);
    }

    @media (max-width: 1024px) {
      .live,
      .about,
      .log {
        grid-column: span 12;
      }

      .stats {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }

      .pulse-shell {
        gap: 24px;
      }
    }

    @media (max-width: 640px) {
      .wrap {
        padding: 40px 16px;
        gap: 24px;
      }

      .panel {
        padding: 24px;
      }

      .hero {
        padding: 60px 16px;
      }

      .hero h1 {
        font-size: 1.75rem;
      }

      .stats {
        grid-template-columns: 1fr;
      }

      .meter {
        width: 140px;
        height: 140px;
      }

      .pulse-shell {
        gap: 20px;
        flex-direction: column;
      }

      .dark-mode-toggle {
        top: 16px;
        right: 16px;
        width: 40px;
        height: 40px;
      }
    }
  </style>
</head>
<body>
  <button class="dark-mode-toggle" id="themeToggle" title="Toggle dark mode">🌙</button>
  <main class="wrap">
    <section class="hero">
      <div class="tag">IOT + BIOSIGNAL</div>
      <h1>PulseBand ESP32 Heart Rate Monitor</h1>
      <p class="sub">A wearable pulse sensor sends analog heartbeat data to an ESP32, calculates BPM in real time, renders it on OLED, and serves this live web dashboard over Wi-Fi.</p>
    </section>

    <section class="grid">
      <article class="panel live">
        <div class="pulse-shell">
          <div class="meter" id="meter">
            <div class="meter-value">
              <b id="bpm">0</b>
              <span>BPM</span>
            </div>
          </div>

          <div>
            <div class="status-row">
              <span><i class="dot" id="dot"></i><span id="streamState">Waiting for data...</span></span>
              <span>IP: <span class="mono" id="ipAddress">-</span></span>
            </div>

            <div class="controls">
              <button class="btn-primary" id="startBtn">Start Measurement</button>
              <button class="btn-secondary" id="stopBtn">Stop</button>
              <button class="btn-neutral" id="resetBtn">Reset Log</button>
              <button class="btn-neutral" id="downloadBtn">Download CSV</button>
            </div>
          </div>
        </div>

        <div class="stats">
          <div class="kpi">
            <div class="name">Average BPM</div>
            <div class="value" id="avgBpm">0</div>
          </div>
          <div class="kpi">
            <div class="name">Signal</div>
            <div class="value" id="signal">0</div>
          </div>
          <div class="kpi">
            <div class="name">Threshold</div>
            <div class="value" id="threshold">0</div>
          </div>
        </div>

        <div class="chart-wrap">
          <canvas id="pulseChart" height="110"></canvas>
        </div>
      </article>

      <article class="panel about">
        <h2>Project Functionality</h2>
        <ol class="flow">
          <li>Pulse sensor reads blood-volume variations as analog values.</li>
          <li>ESP32 filters noise and detects valid beat peaks.</li>
          <li>Heartbeat interval is converted using <span class="mono">BPM = 60000 / IBI</span>.</li>
          <li>BPM is averaged for stability, then shown on OLED and webpage.</li>
          <li>Data points are logged and can be downloaded as CSV for analysis.</li>
        </ol>
        <p class="sub">This dashboard is served directly by the ESP32 web server. You can open it from any phone or laptop on the same Wi-Fi.</p>
      </article>

      <article class="panel log">
        <h2>Live Log</h2>
        <textarea id="logBox" readonly aria-label="Live pulse log"></textarea>
      </article>
    </section>
  </main>

  <script>
    // Dark mode setup
    const themeToggle = document.getElementById("themeToggle");
    const isDarkMode = () => document.documentElement.classList.contains("dark-mode");
    
    const initTheme = () => {
      const saved = localStorage.getItem("pulseBandTheme");
      if (saved === "dark") {
        document.documentElement.classList.add("dark-mode");
        themeToggle.textContent = "☀️";
      } else if (saved === "light") {
        document.documentElement.classList.remove("dark-mode");
        themeToggle.textContent = "🌙";
      } else if (window.matchMedia("(prefers-color-scheme: dark)").matches) {
        document.documentElement.classList.add("dark-mode");
        themeToggle.textContent = "☀️";
      }
    };

    themeToggle.addEventListener("click", () => {
      const dark = isDarkMode();
      if (dark) {
        document.documentElement.classList.remove("dark-mode");
        localStorage.setItem("pulseBandTheme", "light");
        themeToggle.textContent = "🌙";
      } else {
        document.documentElement.classList.add("dark-mode");
        localStorage.setItem("pulseBandTheme", "dark");
        themeToggle.textContent = "☀️";
      }
      updateChartTheme();
    });

    initTheme();

    const bpmEl = document.getElementById("bpm");
    const avgBpmEl = document.getElementById("avgBpm");
    const signalEl = document.getElementById("signal");
    const thresholdEl = document.getElementById("threshold");
    const ipAddressEl = document.getElementById("ipAddress");
    const streamStateEl = document.getElementById("streamState");
    const dotEl = document.getElementById("dot");
    const meterEl = document.getElementById("meter");
    const logBox = document.getElementById("logBox");

    const chartCtx = document.getElementById("pulseChart");
    const labels = [];
    const dataSeries = [];

    const getChartColors = () => {
      const dark = isDarkMode();
      return {
        line: dark ? "#0071e3" : "#0071e3",
        gridColor: dark ? "rgba(255,255,255,0.1)" : "rgba(0,0,0,0.05)",
        textColor: dark ? "#f5f5f7" : "#1d1d1f",
      };
    };

    const createChart = () => {
      const colors = getChartColors();
      return new Chart(chartCtx, {
        type: "line",
        data: {
          labels,
          datasets: [{
            label: "Filtered Signal",
            data: dataSeries,
            borderColor: colors.line,
            borderWidth: 2.5,
            pointRadius: 0,
            tension: 0.3,
            fill: false,
            backgroundColor: "transparent"
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            legend: { display: false },
            filler: { propagate: false }
          },
          scales: {
            x: {
              ticks: { display: false },
              grid: { color: colors.gridColor, drawBorder: false }
            },
            y: {
              grid: { color: colors.gridColor, drawBorder: false },
              ticks: { color: colors.textColor },
              suggestedMin: 0,
              suggestedMax: 4095
            }
          }
        }
      });
    };

    let chart = createChart();

    const updateChartTheme = () => {
      chart.destroy();
      chart = createChart();
    };

    let pointCount = 0;

    function clamp(value, min, max) {
      return Math.max(min, Math.min(max, value));
    }

    function setMeter(bpm) {
      const safe = clamp(Number(bpm) || 0, 0, 200);
      const deg = (safe / 200) * 360;
      const accentColor = "#0071e3";
      const bgColor = "rgba(0, 0, 0, 0.08)";
      meterEl.style.background = `conic-gradient(${accentColor} ${deg}deg, ${bgColor} ${deg}deg)`;
    }

    function appendChart(signal) {
      pointCount += 1;
      labels.push(pointCount);
      dataSeries.push(signal);

      if (labels.length > 90) {
        labels.shift();
        dataSeries.shift();
      }

      chart.update("none");
    }

    function appendLog(line) {
      const next = `${line}\n`;
      logBox.value += next;
      if (logBox.value.length > 5000) {
        logBox.value = logBox.value.slice(-5000);
      }
      logBox.scrollTop = logBox.scrollHeight;
    }

    function updateView(payload) {
      const bpm = payload?.bpm?.current ?? 0;
      const avg = payload?.bpm?.average ?? 0;
      const signal = payload?.sensor?.filtered ?? 0;
      const threshold = payload?.sensor?.threshold ?? 0;
      const ip = payload?.wifi?.ip ?? "-";
      const measuring = Boolean(payload?.state?.measuring);

      bpmEl.textContent = bpm;
      avgBpmEl.textContent = avg;
      signalEl.textContent = signal;
      thresholdEl.textContent = threshold;
      ipAddressEl.textContent = ip;
      streamStateEl.textContent = measuring ? "Measuring" : "Idle";
      dotEl.classList.toggle("live", measuring);

      setMeter(bpm);
      appendChart(signal);
      appendLog(`${new Date().toLocaleTimeString()} | BPM=${bpm}, Signal=${signal}, Thr=${threshold}`);
    }

    async function fetchState() {
      const res = await fetch("/api/state");
      const json = await res.json();
      updateView(json);
    }

    document.getElementById("startBtn").addEventListener("click", async () => {
      if (isHardwareConnected) {
        await fetch("/api/measure?enabled=1");
        await fetchState();
      } else {
        mockMeasuring = true;
        streamStateEl.textContent = "Demo Mode - Measuring";
        dotEl.classList.add("live");
      }
    });

    document.getElementById("stopBtn").addEventListener("click", async () => {
      if (isHardwareConnected) {
        await fetch("/api/measure?enabled=0");
        await fetchState();
      } else {
        mockMeasuring = false;
        streamStateEl.textContent = "Demo Mode - Stopped";
        dotEl.classList.remove("live");
      }
    });

    document.getElementById("resetBtn").addEventListener("click", async () => {
      if (isHardwareConnected) {
        await fetch("/api/reset");
        logBox.value = "";
        await fetchState();
      } else {
        logBox.value = "";
        pointCount = 0;
        labels.length = 0;
        dataSeries.length = 0;
        chart.update();
        mockMeasuring = false;
        streamStateEl.textContent = "Demo Mode - Stopped";
        dotEl.classList.remove("live");
      }
    });

    document.getElementById("downloadBtn").addEventListener("click", () => {
      if (isHardwareConnected) {
        window.location.href = "/api/log.csv";
      } else {
        alert("CSV download not available in demo mode.");
      }
    });

    let isHardwareConnected = false;
    let mockBpmPhase = 0;
    let mockSignalPhase = 0;
    let mockIntervalId = null;
    let mockMeasuring = false;

    function generateMockData() {
      // Simulate realistic pulse waveform with occasional BPM variation
      mockSignalPhase += Math.random() * 0.08;
      mockBpmPhase += 0.015;

      const baseBpm = 72 + Math.sin(mockBpmPhase * 0.5) * 12;
      const currentBpm = Math.round(baseBpm + (Math.random() - 0.5) * 3);
      
      const pulseWave = Math.sin(mockSignalPhase) * 800 + 1500;
      const noise = (Math.random() - 0.5) * 200;
      const filteredSignal = Math.max(0, Math.round(pulseWave + noise));

      return {
        bpm: currentBpm,
        average: 72,
        signal: filteredSignal,
        threshold: 1800,
        ip: "192.168.1.100",
        measuring: mockMeasuring
      };
    }

    function simulateMeasurement() {
      if (!mockMeasuring) return;
      const mock = generateMockData();
      updateView({
        bpm: { current: mock.bpm, average: mock.average },
        sensor: { filtered: mock.signal, threshold: mock.threshold },
        wifi: { ip: mock.ip },
        state: { measuring: mock.measuring }
      });
    }

    function startLiveFeed() {
      if (!isHardwareConnected) {
        // Run mock data simulation
        if (mockIntervalId === null) {
          mockIntervalId = setInterval(simulateMeasurement, 200);
          mockMeasuring = true;
          streamStateEl.textContent = "Demo Mode (No Hardware)";
          dotEl.classList.add("live");
        }
        return;
      }

      if (typeof EventSource === "undefined") {
        setInterval(fetchState, 1000);
        return;
      }

      const source = new EventSource("/events");
      source.onmessage = (event) => {
        try {
          updateView(JSON.parse(event.data));
        } catch (_) {
          fetchState();
        }
      };

      source.onerror = () => {
        source.close();
        setTimeout(startLiveFeed, 1500);
      };
    }

    fetchState().then(() => {
      isHardwareConnected = true;
      startLiveFeed();
    }).catch(() => {
      isHardwareConnected = false;
      streamStateEl.textContent = "Demo Mode (No Hardware)";
      startLiveFeed();
    });
  </script>
</body>
</html>
)rawliteral";

#endif
