const express = require("express");
const path = require("path");
require("dotenv").config();
const { createClient } = require("@supabase/supabase-js");

const app = express();
const port = Number(process.env.PORT || 3000);

app.use(express.json({ limit: "200kb" }));

const LOG_CAPACITY = 900;
const MIN_IBI_MS = 280;
const MAX_IBI_MS = 1800;
const DEFAULT_ESP32_BASE_URL = process.env.ESP32_BASE_URL || "http://192.168.1.100";
const DEFAULT_EMERGENCY_SETTINGS = {
  enabled: true,
  minBpm: 45,
  maxBpm: 120,
  sustainSamples: 6,
  cooldownMs: 120000,
  autoDialWebhookUrl: process.env.EMERGENCY_WEBHOOK_URL || ""
};

const SUPABASE_URL = process.env.SUPABASE_URL || "";
const SUPABASE_SERVICE_KEY = process.env.SUPABASE_SERVICE_KEY || process.env.SUPABASE_SERVICE_ROLE_KEY || "";

if (!SUPABASE_URL || !SUPABASE_SERVICE_KEY) {
  console.warn("Supabase environment variables are missing. Set SUPABASE_URL and SUPABASE_SERVICE_KEY before starting the backend.");
}

const supabase = createClient(SUPABASE_URL || "http://localhost", SUPABASE_SERVICE_KEY || "local-key", {
  auth: {
    persistSession: false,
    autoRefreshToken: false
  }
});

let clients = [];

const state = {
  startedAt: Date.now(),
  sourceMode: (process.env.DATA_SOURCE || "sim").toLowerCase() === "esp32" ? "esp32" : "sim",
  esp32BaseUrl: DEFAULT_ESP32_BASE_URL,
  currentSessionId: null,
  lastEsp32SyncAt: 0,
  sim: {
    lastBeatMs: 0,
    phase: 0,
    bpmPhase: 0,
    bpmWindow: []
  },
  snapshot: {
    measuring: false,
    measuringSince: 0,
    rawSignal: 0,
    filteredSignal: 0,
    threshold: 1800,
    currentBpm: 0,
    avgBpm: 0,
    wifiIp: "127.0.0.1",
    wifiSsid: "PulseBand-Local",
    wifiRssi: -45
  },
  log: [],
  emergency: {
    ...DEFAULT_EMERGENCY_SETTINGS,
    status: "idle",
    abnormalSamples: 0,
    lastAlertAt: 0,
    lastAlertReason: "",
    lastAlertMessage: "",
    lastWebhookStatus: "",
    activeAlertId: null,
    contacts: [],
    alerts: []
  }
};

function uptimeMs() {
  return Date.now() - state.startedAt;
}

function sanitizeBaseUrl(value) {
  return String(value || "").trim().replace(/\/+$/, "");
}

function isValidHttpUrl(value) {
  try {
    const parsed = new URL(value);
    return parsed.protocol === "http:" || parsed.protocol === "https:";
  } catch (_) {
    return false;
  }
}

function clampInteger(value, min, max, fallback) {
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) {
    return fallback;
  }

  return Math.max(min, Math.min(max, parsed));
}

function normalizePhone(phone) {
  return String(phone || "").trim();
}

function normalizeContactRow(contact) {
  return {
    id: contact.id,
    name: String(contact.name || "Unnamed contact"),
    phone: String(contact.phone || ""),
    priority: Number(contact.priority ?? 0),
    createdAt: contact.created_at,
    updatedAt: contact.updated_at
  };
}

function normalizeAlertRow(alert) {
  return {
    id: alert.id,
    sessionId: alert.session_id,
    level: alert.level,
    reason: alert.reason,
    bpm: alert.bpm,
    message: alert.message,
    webhookStatus: alert.webhook_status,
    createdAt: alert.created_at,
    acknowledgedAt: alert.acknowledged_at
  };
}

function normalizeSessionRow(session) {
  return {
    id: session.id,
    startedAt: session.started_at,
    sourceMode: session.source_mode,
    sourceBaseUrl: session.source_base_url
  };
}

function emergencySnapshot() {
  return {
    enabled: Boolean(state.emergency.enabled),
    status: state.emergency.status,
    minBpm: state.emergency.minBpm,
    maxBpm: state.emergency.maxBpm,
    sustainSamples: state.emergency.sustainSamples,
    cooldownMs: state.emergency.cooldownMs,
    abnormalSamples: state.emergency.abnormalSamples,
    lastAlertAt: state.emergency.lastAlertAt,
    lastAlertReason: state.emergency.lastAlertReason,
    lastAlertMessage: state.emergency.lastAlertMessage,
    lastWebhookStatus: state.emergency.lastWebhookStatus,
    activeAlertId: state.emergency.activeAlertId,
    contactsCount: state.emergency.contacts.length,
    alertCount: state.emergency.alerts.length
  };
}

async function fetchJson(url, timeoutMs = 2500) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);

  try {
    const response = await fetch(url, { signal: controller.signal });
    if (!response.ok) {
      throw new Error(`Request failed with status ${response.status}`);
    }
    return await response.json();
  } finally {
    clearTimeout(timer);
  }
}

function normalizePayload(payload) {
  return {
    wifi: {
      ssid: String(payload?.wifi?.ssid || "PulseBand"),
      ip: String(payload?.wifi?.ip || "127.0.0.1"),
      rssi: Number(payload?.wifi?.rssi ?? -45)
    },
    sensor: {
      raw: Number(payload?.sensor?.raw ?? payload?.sensor?.filtered ?? 0),
      filtered: Number(payload?.sensor?.filtered ?? 0),
      threshold: Number(payload?.sensor?.threshold ?? 1800)
    },
    bpm: {
      current: Number(payload?.bpm?.current ?? 0),
      average: Number(payload?.bpm?.average ?? payload?.bpm?.current ?? 0)
    },
    state: {
      measuring: Boolean(payload?.state?.measuring)
    }
  };
}

function setSnapshotFromPayload(payload) {
  const normalized = normalizePayload(payload);

  state.snapshot.wifiSsid = normalized.wifi.ssid;
  state.snapshot.wifiIp = normalized.wifi.ip;
  state.snapshot.wifiRssi = normalized.wifi.rssi;

  state.snapshot.rawSignal = normalized.sensor.raw;
  state.snapshot.filteredSignal = normalized.sensor.filtered;
  state.snapshot.threshold = normalized.sensor.threshold;

  state.snapshot.currentBpm = normalized.bpm.current;
  state.snapshot.avgBpm = normalized.bpm.average;
  if (normalized.state.measuring && !state.snapshot.measuringSince) {
    state.snapshot.measuringSince = Date.now();
  }
  if (!normalized.state.measuring) {
    state.snapshot.measuringSince = 0;
  }
  state.snapshot.measuring = normalized.state.measuring;
}

function buildPayload() {
  return {
    uptimeMs: uptimeMs(),
    wifi: {
      ssid: state.snapshot.wifiSsid,
      ip: state.snapshot.wifiIp,
      rssi: state.snapshot.wifiRssi
    },
    sensor: {
      raw: state.snapshot.rawSignal,
      filtered: state.snapshot.filteredSignal,
      threshold: state.snapshot.threshold
    },
    bpm: {
      current: state.snapshot.currentBpm,
      average: state.snapshot.avgBpm
    },
    state: {
      measuring: state.snapshot.measuring,
      samplesLogged: state.log.length
    },
    emergency: emergencySnapshot(),
    source: {
      mode: state.sourceMode,
      esp32BaseUrl: state.esp32BaseUrl
    }
  };
}

async function dbQuery(builder) {
  const { data, error } = await builder;
  if (error) {
    throw error;
  }
  return data;
}

async function loadEmergencySettings() {
  let settingsRow = await dbQuery(
    supabase.from("emergency_settings").select("*").eq("id", 1).maybeSingle()
  );

  if (!settingsRow) {
    settingsRow = await dbQuery(
      supabase
        .from("emergency_settings")
        .upsert(
          {
            id: 1,
            enabled: DEFAULT_EMERGENCY_SETTINGS.enabled,
            min_bpm: DEFAULT_EMERGENCY_SETTINGS.minBpm,
            max_bpm: DEFAULT_EMERGENCY_SETTINGS.maxBpm,
            sustain_samples: DEFAULT_EMERGENCY_SETTINGS.sustainSamples,
            cooldown_ms: DEFAULT_EMERGENCY_SETTINGS.cooldownMs,
            auto_dial_webhook_url: DEFAULT_EMERGENCY_SETTINGS.autoDialWebhookUrl,
            updated_at: new Date().toISOString()
          },
          { onConflict: "id" }
        )
        .select("*")
        .single()
    );
  }

  state.emergency.enabled = Boolean(settingsRow.enabled);
  state.emergency.minBpm = Number(settingsRow.min_bpm ?? DEFAULT_EMERGENCY_SETTINGS.minBpm);
  state.emergency.maxBpm = Number(settingsRow.max_bpm ?? DEFAULT_EMERGENCY_SETTINGS.maxBpm);
  state.emergency.sustainSamples = Number(settingsRow.sustain_samples ?? DEFAULT_EMERGENCY_SETTINGS.sustainSamples);
  state.emergency.cooldownMs = Number(settingsRow.cooldown_ms ?? DEFAULT_EMERGENCY_SETTINGS.cooldownMs);
  state.emergency.autoDialWebhookUrl = String(settingsRow.auto_dial_webhook_url || DEFAULT_EMERGENCY_SETTINGS.autoDialWebhookUrl || "");
}

async function persistEmergencySettings() {
  await dbQuery(
    supabase
      .from("emergency_settings")
      .upsert(
        {
          id: 1,
          enabled: Boolean(state.emergency.enabled),
          min_bpm: state.emergency.minBpm,
          max_bpm: state.emergency.maxBpm,
          sustain_samples: state.emergency.sustainSamples,
          cooldown_ms: state.emergency.cooldownMs,
          auto_dial_webhook_url: state.emergency.autoDialWebhookUrl,
          updated_at: new Date().toISOString()
        },
        { onConflict: "id" }
      )
      .select("*")
      .single()
  );
}

async function refreshEmergencyContacts() {
  const contacts = await dbQuery(
    supabase.from("emergency_contacts").select("*").order("priority", { ascending: true }).order("id", { ascending: true })
  );
  state.emergency.contacts = contacts.map(normalizeContactRow);
}

async function refreshEmergencyAlerts(limit = 25) {
  const alerts = await dbQuery(
    supabase.from("emergency_alerts").select("*").order("id", { ascending: false }).limit(limit)
  );
  state.emergency.alerts = alerts.map(normalizeAlertRow);
}

async function createSession() {
  const session = await dbQuery(
    supabase
      .from("sessions")
      .insert({
        started_at: new Date().toISOString(),
        source_mode: state.sourceMode,
        source_base_url: state.sourceMode === "esp32" ? state.esp32BaseUrl : null
      })
      .select("id")
      .single()
  );

  state.currentSessionId = session.id;
}

function pushBpmToWindow(value) {
  state.sim.bpmWindow.push(value);
  if (state.sim.bpmWindow.length > 8) {
    state.sim.bpmWindow.shift();
  }

  if (state.sim.bpmWindow.length === 0) {
    state.snapshot.avgBpm = 0;
    return;
  }

  const sum = state.sim.bpmWindow.reduce((acc, item) => acc + item, 0);
  state.snapshot.avgBpm = Math.round(sum / state.sim.bpmWindow.length);
}

function appendLogEntry() {
  const entry = {
    elapsedMs: uptimeMs(),
    bpm: state.snapshot.currentBpm,
    filtered: state.snapshot.filteredSignal,
    recordedAt: new Date().toISOString()
  };

  state.log.push(entry);
  if (state.log.length > LOG_CAPACITY) {
    state.log.shift();
  }

  if (state.currentSessionId !== null) {
    dbQuery(
      supabase.from("samples").insert({
        session_id: state.currentSessionId,
        elapsed_ms: entry.elapsedMs,
        bpm: entry.bpm,
        filtered_signal: entry.filtered,
        recorded_at: entry.recordedAt
      })
    ).catch((error) => {
      console.error("Failed to persist sample:", error.message);
    });
  }
}

function updateMeasuringState(nextValue) {
  state.snapshot.measuring = nextValue;
  state.snapshot.measuringSince = nextValue ? state.snapshot.measuringSince || Date.now() : 0;
}

function buildEmergencyMessage(reason, bpm) {
  if (reason === "no-signal") {
    return `PulseBand detected no reliable heartbeat signal for ${state.emergency.sustainSamples} consecutive samples.`;
  }

  if (reason === "high-bpm") {
    return `PulseBand detected a sustained high heart rate of ${bpm} BPM above ${state.emergency.maxBpm} BPM.`;
  }

  if (reason === "low-bpm") {
    return `PulseBand detected a sustained low heart rate of ${bpm} BPM below ${state.emergency.minBpm} BPM.`;
  }

  return `PulseBand detected an abnormal heart rate condition (${reason}).`;
}

function classifyEmergencyReason() {
  const bpm = state.snapshot.currentBpm;

  if (!state.snapshot.measuring) {
    return null;
  }

  if (Date.now() - state.snapshot.measuringSince < 5000) {
    return null;
  }

  if (bpm <= 0) {
    return "no-signal";
  }

  if (bpm > state.emergency.maxBpm) {
    return "high-bpm";
  }

  if (bpm < state.emergency.minBpm) {
    return "low-bpm";
  }

  return null;
}

async function sendEmergencyWebhook(alertPayload) {
  const webhookUrl = String(state.emergency.autoDialWebhookUrl || "").trim();
  if (!webhookUrl) {
    return "not-configured";
  }

  if (!isValidHttpUrl(webhookUrl)) {
    return "invalid-url";
  }

  try {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 5000);

    try {
      const response = await fetch(webhookUrl, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(alertPayload),
        signal: controller.signal
      });

      return response.ok ? `delivered:${response.status}` : `failed:${response.status}`;
    } finally {
      clearTimeout(timer);
    }
  } catch (error) {
    return `error:${error.message}`;
  }
}

async function raiseEmergencyAlert(reason) {
  const nowIso = new Date().toISOString();
  const bpm = state.snapshot.currentBpm;
  const message = buildEmergencyMessage(reason, bpm);
  const alertPayload = {
    sessionId: state.currentSessionId,
    reason,
    bpm,
    message,
    contacts: state.emergency.contacts,
    source: state.sourceMode,
    createdAt: nowIso
  };

  const webhookStatus = await sendEmergencyWebhook(alertPayload);
  const alert = await dbQuery(
    supabase
      .from("emergency_alerts")
      .insert({
        session_id: state.currentSessionId,
        level: "critical",
        reason,
        bpm,
        message,
        webhook_status: webhookStatus,
        created_at: nowIso
      })
      .select("id")
      .single()
  );

  state.emergency.status = "active";
  state.emergency.activeAlertId = alert.id;
  state.emergency.lastAlertAt = Date.now();
  state.emergency.lastAlertReason = reason;
  state.emergency.lastAlertMessage = message;
  state.emergency.lastWebhookStatus = webhookStatus;
  state.emergency.abnormalSamples = 0;

  await refreshEmergencyAlerts();
  sendEvent({ ...buildPayload(), emergencyEvent: { type: "alert", ...emergencySnapshot(), message } });
}

async function acknowledgeEmergencyAlert() {
  if (!state.emergency.activeAlertId) {
    return false;
  }

  await dbQuery(
    supabase
      .from("emergency_alerts")
      .update({ acknowledged_at: new Date().toISOString() })
      .eq("id", state.emergency.activeAlertId)
  );

  state.emergency.status = "acknowledged";
  state.emergency.activeAlertId = null;
  state.emergency.lastAlertMessage = state.emergency.lastAlertMessage || "";
  await refreshEmergencyAlerts();
  return true;
}

async function evaluateEmergencyState() {
  if (!state.emergency.enabled) {
    state.emergency.status = "idle";
    state.emergency.abnormalSamples = 0;
    return;
  }

  const reason = classifyEmergencyReason();
  if (!reason) {
    if (!state.emergency.activeAlertId) {
      state.emergency.status = state.snapshot.measuring ? "watching" : "idle";
    }
    state.emergency.abnormalSamples = 0;
    return;
  }

  state.emergency.abnormalSamples += 1;
  state.emergency.status = "watching";

  if (state.emergency.activeAlertId) {
    return;
  }

  if (state.emergency.abnormalSamples >= state.emergency.sustainSamples) {
    const canAlert = Date.now() - state.emergency.lastAlertAt >= state.emergency.cooldownMs;
    if (canAlert) {
      await raiseEmergencyAlert(reason);
    }
  }
}

function buildCsv() {
  const rows = ["ElapsedMs,BPM,FilteredSignal"];
  for (const entry of state.log) {
    rows.push(`${entry.elapsedMs},${entry.bpm},${entry.filtered}`);
  }
  return `${rows.join("\n")}\n`;
}

function sendEvent(payload) {
  const body = `id: ${Date.now()}\nevent: message\ndata: ${JSON.stringify(payload)}\n\n`;
  clients = clients.filter((client) => {
    try {
      client.write(body);
      return true;
    } catch (_) {
      return false;
    }
  });
}

function resetRuntimeState() {
  state.snapshot.currentBpm = 0;
  state.snapshot.avgBpm = 0;
  state.snapshot.rawSignal = 0;
  state.snapshot.filteredSignal = 0;
  state.snapshot.threshold = 1800;
  updateMeasuringState(false);

  state.sim.lastBeatMs = 0;
  state.sim.phase = 0;
  state.sim.bpmPhase = 0;
  state.sim.bpmWindow = [];

  state.log = [];
  state.emergency.status = "idle";
  state.emergency.abnormalSamples = 0;
  state.emergency.activeAlertId = null;
  state.emergency.lastAlertReason = "";
  state.emergency.lastAlertMessage = "";
  state.emergency.lastWebhookStatus = "";
}

function simulateSignalTick() {
  state.sim.phase += 0.13;
  state.sim.bpmPhase += 0.015;

  const base = 1800 + Math.sin(state.sim.phase) * 700;
  const beatBoost = Math.max(0, Math.sin(state.sim.phase * 2.2)) * 450;
  const noise = (Math.random() - 0.5) * 120;
  const raw = Math.max(0, Math.round(base + beatBoost + noise));
  const filtered = Math.round(state.snapshot.filteredSignal * 0.82 + raw * 0.18);

  state.snapshot.rawSignal = raw;
  state.snapshot.filteredSignal = filtered;

  const dynamicAmplitude = 600 + Math.sin(state.sim.phase * 0.45) * 120;
  state.snapshot.threshold = Math.round(1500 + dynamicAmplitude * 0.45);

  if (!state.snapshot.measuring) {
    state.snapshot.currentBpm = 0;
    return;
  }

  const now = Date.now();
  const crossing = filtered > state.snapshot.threshold;

  if (crossing && now - state.sim.lastBeatMs > MIN_IBI_MS) {
    if (state.sim.lastBeatMs > 0) {
      const ibi = now - state.sim.lastBeatMs;
      if (ibi >= MIN_IBI_MS && ibi <= MAX_IBI_MS) {
        const bpm = Math.round(60000 / ibi);
        state.snapshot.currentBpm = bpm;
        pushBpmToWindow(bpm);
      }
    }
    state.sim.lastBeatMs = now;
  }

  if (state.snapshot.currentBpm === 0) {
    const baseline = 74 + Math.sin(state.sim.bpmPhase) * 8 + (Math.random() - 0.5) * 2;
    const fallbackBpm = Math.max(52, Math.min(130, Math.round(baseline)));
    state.snapshot.currentBpm = fallbackBpm;
    pushBpmToWindow(fallbackBpm);
  }

  evaluateEmergencyState().catch((error) => {
    console.error("Emergency evaluation failed:", error.message);
  });
}

async function pullEsp32State() {
  const payload = await fetchJson(`${state.esp32BaseUrl}/api/state`, 2500);
  setSnapshotFromPayload(payload);
  state.lastEsp32SyncAt = Date.now();
  await evaluateEmergencyState();
  return buildPayload();
}

async function setSourceMode(mode, baseUrl) {
  const normalizedMode = mode === "esp32" ? "esp32" : "sim";
  let nextBaseUrl = state.esp32BaseUrl;
  let initialPayload;

  if (normalizedMode === "esp32") {
    if (baseUrl) {
      const candidateUrl = sanitizeBaseUrl(baseUrl);
      if (!isValidHttpUrl(candidateUrl)) {
        throw new Error("Invalid ESP32 base URL.");
      }
      nextBaseUrl = candidateUrl;
    }

    initialPayload = await fetchJson(`${nextBaseUrl}/api/state`, 3000);
  }

  state.sourceMode = normalizedMode;
  state.esp32BaseUrl = nextBaseUrl;
  resetRuntimeState();
  await createSession();

  if (state.sourceMode === "esp32") {
    setSnapshotFromPayload(initialPayload);
    state.lastEsp32SyncAt = Date.now();
  } else {
    state.snapshot.wifiSsid = "PulseBand-Local";
    state.snapshot.wifiIp = "127.0.0.1";
    state.snapshot.wifiRssi = -45;
  }
}

setInterval(() => {
  if (state.sourceMode === "sim") {
    simulateSignalTick();
  }
}, 200);

setInterval(async () => {
  try {
    if (state.sourceMode === "esp32") {
      await pullEsp32State();
    }

    if (state.snapshot.measuring) {
      appendLogEntry();
    }

    sendEvent(buildPayload());
  } catch (error) {
    sendEvent({
      ...buildPayload(),
      error: `Source update failed: ${error.message}`
    });
  }
}, 1000);

app.get("/api/state", async (_req, res) => {
  try {
    if (state.sourceMode === "esp32" && Date.now() - state.lastEsp32SyncAt > 800) {
      await pullEsp32State();
    }
    res.json(buildPayload());
  } catch (error) {
    res.status(502).json({ ok: false, error: error.message, payload: buildPayload() });
  }
});

app.get("/api/source", (_req, res) => {
  res.json({ ok: true, source: { mode: state.sourceMode, esp32BaseUrl: state.esp32BaseUrl } });
});

app.post("/api/source", async (req, res) => {
  const mode = String(req.body?.mode || "sim").toLowerCase();
  const baseUrl = req.body?.baseUrl;

  try {
    await setSourceMode(mode, baseUrl);
    res.json({ ok: true, source: { mode: state.sourceMode, esp32BaseUrl: state.esp32BaseUrl } });
  } catch (error) {
    res.status(400).json({ ok: false, error: error.message });
  }
});

app.get("/api/measure", async (req, res) => {
  const enabled = String(req.query.enabled || "").toLowerCase();
  const measuring = enabled === "1" || enabled === "true" || enabled === "on";

  try {
    if (state.sourceMode === "esp32") {
      await fetchJson(`${state.esp32BaseUrl}/api/measure?enabled=${measuring ? "1" : "0"}`, 3000);
      await pullEsp32State();
    } else {
      updateMeasuringState(measuring);
      if (!measuring) {
        state.snapshot.currentBpm = 0;
      }
    }

    res.json({ ok: true, measuring: state.snapshot.measuring, sourceMode: state.sourceMode });
  } catch (error) {
    res.status(502).json({ ok: false, error: error.message });
  }
});

app.get("/api/reset", async (_req, res) => {
  try {
    if (state.sourceMode === "esp32") {
      await fetchJson(`${state.esp32BaseUrl}/api/reset`, 3000);
    }

    resetRuntimeState();
    await createSession();

    if (state.sourceMode === "esp32") {
      await pullEsp32State();
    }

    res.json({ ok: true, sourceMode: state.sourceMode, sessionId: state.currentSessionId });
  } catch (error) {
    res.status(502).json({ ok: false, error: error.message });
  }
});

app.get("/api/emergency/settings", (_req, res) => {
  res.json({ ok: true, emergency: emergencySnapshot(), webhookUrlConfigured: Boolean(state.emergency.autoDialWebhookUrl) });
});

app.post("/api/emergency/settings", async (req, res) => {
  const nextEnabled = req.body?.enabled;
  const nextMinBpm = req.body?.minBpm;
  const nextMaxBpm = req.body?.maxBpm;
  const nextSustainSamples = req.body?.sustainSamples;
  const nextCooldownMs = req.body?.cooldownMs;
  const nextWebhookUrl = req.body?.autoDialWebhookUrl;

  if (nextEnabled !== undefined) {
    state.emergency.enabled = Boolean(nextEnabled);
  }

  if (nextMinBpm !== undefined) {
    state.emergency.minBpm = clampInteger(nextMinBpm, 20, 150, state.emergency.minBpm);
  }

  if (nextMaxBpm !== undefined) {
    state.emergency.maxBpm = clampInteger(nextMaxBpm, 30, 220, state.emergency.maxBpm);
  }

  if (nextSustainSamples !== undefined) {
    state.emergency.sustainSamples = clampInteger(nextSustainSamples, 1, 20, state.emergency.sustainSamples);
  }

  if (nextCooldownMs !== undefined) {
    state.emergency.cooldownMs = clampInteger(nextCooldownMs, 30000, 15 * 60 * 1000, state.emergency.cooldownMs);
  }

  if (nextWebhookUrl !== undefined) {
    state.emergency.autoDialWebhookUrl = String(nextWebhookUrl || "").trim();
  }

  await persistEmergencySettings();
  res.json({ ok: true, emergency: emergencySnapshot(), webhookUrlConfigured: Boolean(state.emergency.autoDialWebhookUrl) });
});

app.get("/api/emergency/contacts", async (_req, res) => {
  await refreshEmergencyContacts();
  res.json({ ok: true, contacts: state.emergency.contacts });
});

app.post("/api/emergency/contacts", async (req, res) => {
  const name = String(req.body?.name || "").trim();
  const phone = normalizePhone(req.body?.phone);
  const priority = clampInteger(req.body?.priority ?? 0, 0, 99, 0);

  if (!name || !phone) {
    res.status(400).json({ ok: false, error: "name and phone are required" });
    return;
  }

  const nowIso = new Date().toISOString();
  const contact = await dbQuery(
    supabase
      .from("emergency_contacts")
      .insert({ name, phone, priority, created_at: nowIso, updated_at: nowIso })
      .select("*")
      .single()
  );

  await refreshEmergencyContacts();
  res.json({ ok: true, contact: normalizeContactRow(contact) });
});

app.put("/api/emergency/contacts/:id", async (req, res) => {
  const contactId = Number(req.params.id);
  if (!Number.isInteger(contactId) || contactId <= 0) {
    res.status(400).json({ ok: false, error: "Invalid contact id" });
    return;
  }

  const name = req.body?.name !== undefined ? String(req.body.name).trim() : null;
  const phone = req.body?.phone !== undefined ? normalizePhone(req.body.phone) : null;
  const priority = req.body?.priority !== undefined ? clampInteger(req.body.priority, 0, 99, 0) : null;

  const existing = await dbQuery(supabase.from("emergency_contacts").select("id").eq("id", contactId).maybeSingle());
  if (!existing) {
    res.status(404).json({ ok: false, error: "Contact not found" });
    return;
  }

  await dbQuery(
    supabase
      .from("emergency_contacts")
      .update({
        ...(name !== null ? { name } : {}),
        ...(phone !== null ? { phone } : {}),
        ...(priority !== null ? { priority } : {}),
        updated_at: new Date().toISOString()
      })
      .eq("id", contactId)
  );

  await refreshEmergencyContacts();
  res.json({ ok: true, contacts: state.emergency.contacts });
});

app.delete("/api/emergency/contacts/:id", async (req, res) => {
  const contactId = Number(req.params.id);
  if (!Number.isInteger(contactId) || contactId <= 0) {
    res.status(400).json({ ok: false, error: "Invalid contact id" });
    return;
  }

  await dbQuery(supabase.from("emergency_contacts").delete().eq("id", contactId));
  await refreshEmergencyContacts();
  res.json({ ok: true, contacts: state.emergency.contacts });
});

app.get("/api/emergency/alerts", async (req, res) => {
  const limit = Math.max(1, Math.min(50, Number(req.query.limit || 20)));
  const alerts = await dbQuery(
    supabase.from("emergency_alerts").select("*").order("id", { ascending: false }).limit(limit)
  );

  res.json({ ok: true, alerts: alerts.map(normalizeAlertRow) });
});

app.post("/api/emergency/test", async (_req, res) => {
  await raiseEmergencyAlert("manual-test");
  res.json({ ok: true, emergency: emergencySnapshot() });
});

app.post("/api/emergency/acknowledge", async (_req, res) => {
  const acknowledged = await acknowledgeEmergencyAlert();
  res.json({ ok: true, acknowledged, emergency: emergencySnapshot() });
});

app.get("/api/log.csv", (_req, res) => {
  res.setHeader("Content-Type", "text/csv");
  res.setHeader("Content-Disposition", "attachment; filename=PulseBandLog.csv");
  res.send(buildCsv());
});

app.get("/api/history", async (req, res) => {
  const limit = Math.max(1, Math.min(100, Number(req.query.limit || 20)));
  const sessions = await dbQuery(
    supabase.from("sessions").select("*").order("id", { ascending: false }).limit(limit)
  );

  const histories = await Promise.all(
    sessions.map(async (session) => {
      const samples = await dbQuery(
        supabase.from("samples").select("bpm").eq("session_id", session.id)
      );

      const sampleCount = samples.length;
      const avgBpm = sampleCount
        ? Math.round((samples.reduce((sum, sample) => sum + Number(sample.bpm || 0), 0) / sampleCount) * 10) / 10
        : null;

      return {
        ...normalizeSessionRow(session),
        sampleCount,
        avgBpm
      };
    })
  );

  res.json({ ok: true, sessions: histories });
});

app.get("/api/history/:sessionId", async (req, res) => {
  const sessionId = Number(req.params.sessionId);
  const limit = Math.max(1, Math.min(2000, Number(req.query.limit || 900)));

  if (!Number.isInteger(sessionId) || sessionId <= 0) {
    res.status(400).json({ ok: false, error: "Invalid sessionId" });
    return;
  }

  const session = await dbQuery(
    supabase.from("sessions").select("*").eq("id", sessionId).maybeSingle()
  );

  if (!session) {
    res.status(404).json({ ok: false, error: "Session not found" });
    return;
  }

  const samples = await dbQuery(
    supabase
      .from("samples")
      .select("elapsed_ms, bpm, filtered_signal, recorded_at")
      .eq("session_id", sessionId)
      .order("id", { ascending: false })
      .limit(limit)
  );

  res.json({
    ok: true,
    session: normalizeSessionRow(session),
    samples: samples.reverse().map((sample) => ({
      elapsedMs: sample.elapsed_ms,
      bpm: sample.bpm,
      filtered: sample.filtered_signal,
      recordedAt: sample.recorded_at
    }))
  });
});

app.get("/events", (req, res) => {
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache");
  res.setHeader("Connection", "keep-alive");
  res.flushHeaders();

  clients.push(res);
  res.write(`id: ${Date.now()}\nevent: message\ndata: ${JSON.stringify(buildPayload())}\n\n`);

  req.on("close", () => {
    clients = clients.filter((client) => client !== res);
  });
});

const websiteDir = path.join(__dirname, "..", "website");
app.use(express.static(websiteDir));

app.get("/", (_req, res) => {
  res.sendFile(path.join(websiteDir, "index.html"));
});

async function start() {
  if (!SUPABASE_URL || !SUPABASE_SERVICE_KEY) {
    throw new Error("Missing Supabase configuration. Set SUPABASE_URL and SUPABASE_SERVICE_KEY in backend/.env.");
  }

  await loadEmergencySettings();
  await refreshEmergencyContacts();
  await refreshEmergencyAlerts();
  await createSession();

  if (state.sourceMode === "esp32") {
    try {
      await pullEsp32State();
    } catch (error) {
      console.warn(`ESP32 source unavailable on startup (${error.message}). Falling back to latest cached state.`);
    }
  }

  app.listen(port, () => {
    console.log(`PulseBand backend running at http://localhost:${port}`);
    console.log(`Source mode: ${state.sourceMode}`);
  });
}

start().catch((error) => {
  console.error("Failed to start backend:", error);
  process.exit(1);
});
