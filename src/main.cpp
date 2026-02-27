#include <Arduino.h>
#include <cstring>

#include "command/dispatcher.h"
#include "command/i114.h"
#include "command/i999.h"
#include "command/types.h"
#include "configuration.h"
#include "protocol_decoder.h"

namespace {

char g_cmdRxBuffer[CMD_BUFFER_SIZE];
size_t g_cmdWriteIdx = 0;
bool g_cmdReady = false;
bool g_cmdOverflow = false;
bool g_hostSeen = false;

SamplePacket g_sampleRing[SAMPLE_RING_CAPACITY];
uint16_t g_sampleHead = 0;
uint16_t g_sampleTail = 0;

hw_timer_t *g_timer = nullptr;
portMUX_TYPE g_timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool g_sampleTick = false;

DeviceState g_state {
  DEFAULT_ANALOG_PIN,
  DEFAULT_SAMPLE_HZ,
  DEFAULT_ANALOG_PIN,
  DEFAULT_SAMPLE_HZ,
  0,
  0,
  false,
  false,
  RunState::Idle
};

bool ringPushSample(const SamplePacket &pkt) {
  uint16_t nextHead = static_cast<uint16_t>((g_sampleHead + 1U) % SAMPLE_RING_CAPACITY);
  if (nextHead == g_sampleTail) {
    return false;
  }
  g_sampleRing[g_sampleHead] = pkt;
  g_sampleHead = nextHead;
  return true;
}

bool ringPopSample(SamplePacket &pkt) {
  if (g_sampleTail == g_sampleHead) {
    return false;
  }
  pkt = g_sampleRing[g_sampleTail];
  g_sampleTail = static_cast<uint16_t>((g_sampleTail + 1U) % SAMPLE_RING_CAPACITY);
  return true;
}

void clearSampleRing() {
  portENTER_CRITICAL(&g_timerMux);
  g_sampleHead = 0;
  g_sampleTail = 0;
  portEXIT_CRITICAL(&g_timerMux);
}

void IRAM_ATTR onSampleTimer() {
  g_sampleTick = true;
}

void ensureTimerCreated() {
  if (g_timer == nullptr) {
    // Arduino-ESP32 core 2.x timer API:
    // timer index = 0, divider = 80 => 1MHz (1us per tick), count up.
    g_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(g_timer, &onSampleTimer, true);
  }
}

void startSamplingTimer(uint16_t sampleHz) {
  ensureTimerCreated();
  uint32_t periodUs = 1000000UL / sampleHz;
  if (periodUs == 0) {
    periodUs = 1;
  }
  timerWrite(g_timer, 0);
  timerAlarmWrite(g_timer, periodUs, true);
  timerAlarmEnable(g_timer);
  timerStart(g_timer);
}

void stopSamplingTimer() {
  if (g_timer != nullptr) {
    timerAlarmDisable(g_timer);
    timerStop(g_timer);
  }
  g_sampleTick = false;
}

uint16_t readVoltageMilliVolt(uint8_t pin) {
  uint16_t raw = analogRead(pin);
  return static_cast<uint16_t>((static_cast<uint32_t>(raw) * 3300U) / 4095U);
}

void emitError(const char *msg) {
  Serial.println(msg);
}

I114StatusSnapshot buildI114Snapshot() {
  uint16_t head = 0;
  uint16_t tail = 0;
  portENTER_CRITICAL(&g_timerMux);
  head = g_sampleHead;
  tail = g_sampleTail;
  portEXIT_CRITICAL(&g_timerMux);

  uint16_t used = (head >= tail)
                    ? static_cast<uint16_t>(head - tail)
                    : static_cast<uint16_t>(SAMPLE_RING_CAPACITY - tail + head);
  uint16_t freeSpace = static_cast<uint16_t>(SAMPLE_RING_CAPACITY - 1U - used);

  uint32_t workMs = 0;
  if (g_state.runState == RunState::Sampling) {
    workMs = millis() - g_state.samplingStartMs;
  }

  uint16_t pinValueMv = g_state.lastVoltageMv;
  if (g_state.runState != RunState::Sampling) {
    pinValueMv = readVoltageMilliVolt(g_state.activePin);
  }

  return {
    SERIAL_BAUD,
    CMD_BUFFER_SIZE,
    g_cmdWriteIdx,
    g_cmdOverflow,
    SAMPLE_RING_CAPACITY,
    used,
    freeSpace,
    g_state.sampleOverflow,
    g_state.activePin,
    g_state.sampleHz,
    g_state.hasLastConfig,
    g_state.lastConfigPin,
    g_state.lastConfigHz,
    g_state.runState,
    pinValueMv,
    workMs
  };
}

void sendSampleBinary(const SamplePacket &pkt) {
  // [0xAA][0x55][timestamp_ms 4B LE][voltage_mv 2B LE][0x0D][0x0A]
  uint8_t frame[10];
  frame[0] = 0xAA;
  frame[1] = 0x55;
  frame[2] = static_cast<uint8_t>(pkt.timestamp_ms & 0xFFU);
  frame[3] = static_cast<uint8_t>((pkt.timestamp_ms >> 8) & 0xFFU);
  frame[4] = static_cast<uint8_t>((pkt.timestamp_ms >> 16) & 0xFFU);
  frame[5] = static_cast<uint8_t>((pkt.timestamp_ms >> 24) & 0xFFU);
  frame[6] = static_cast<uint8_t>(pkt.voltage_mv & 0xFFU);
  frame[7] = static_cast<uint8_t>((pkt.voltage_mv >> 8) & 0xFFU);
  frame[8] = 0x0D;
  frame[9] = 0x0A;
  Serial.write(frame, sizeof(frame));
}

void serialRxPollToBuffer() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (!g_hostSeen) {
      g_hostSeen = true;
      Serial.println(MSG_ONLINE);
    }
    if (g_cmdReady) {
      continue;
    }
    if (c == '\r' || c == '\n') {
      if (g_cmdWriteIdx > 0) {
        g_cmdRxBuffer[g_cmdWriteIdx] = '\0';
        g_cmdReady = true;
        g_cmdWriteIdx = 0;
      }
      continue;
    }
    if (g_cmdWriteIdx >= (CMD_BUFFER_SIZE - 1)) {
      g_cmdOverflow = true;
      g_cmdWriteIdx = 0;
      continue;
    }
    g_cmdRxBuffer[g_cmdWriteIdx++] = c;
  }
}

void applyCommandResult(const CommandResult &result) {
  if (result.errorMsg != nullptr) {
    emitError(result.errorMsg);
  }

  if (result.shouldStopSampling) {
    stopSamplingTimer();
  }

  if (result.shouldClearBuffer) {
    clearSampleRing();
  }

  if (result.shouldStartSampling) {
    pinMode(g_state.activePin, INPUT);
    if (result.shouldResetTimestamp) {
      g_state.samplingStartMs = millis();
    }
    startSamplingTimer(g_state.sampleHz);
  }

  if (result.shouldReportStatus) {
    emitI114StatusReport(buildI114Snapshot());
  }

  if (result.shouldReboot) {
    executeI999Reboot();
  }
}

void handleSamplingTick() {
  if (g_state.runState != RunState::Sampling) {
    return;
  }
  if (!g_sampleTick) {
    return;
  }
  g_sampleTick = false;

  SamplePacket pkt {};
  pkt.timestamp_ms = millis() - g_state.samplingStartMs;
  pkt.voltage_mv = readVoltageMilliVolt(g_state.activePin);
  g_state.lastVoltageMv = pkt.voltage_mv;

  portENTER_CRITICAL(&g_timerMux);
  bool ok = ringPushSample(pkt);
  if (!ok) {
    g_state.sampleOverflow = true;
  }
  portEXIT_CRITICAL(&g_timerMux);
}

void streamSamplesToHost() {
  if (g_state.runState == RunState::Idle) {
    return;
  }

  // Limit packets per loop so command RX stays responsive at high sample rates.
  constexpr uint16_t MAX_PACKETS_PER_LOOP = 32;
  uint16_t sentCount = 0;
  SamplePacket pkt {};
  while (sentCount < MAX_PACKETS_PER_LOOP) {
    portENTER_CRITICAL(&g_timerMux);
    bool hasData = ringPopSample(pkt);
    portEXIT_CRITICAL(&g_timerMux);
    if (!hasData) {
      break;
    }
    sendSampleBinary(pkt);
    sentCount++;
  }

  if (g_state.runState == RunState::Stopping) {
    portENTER_CRITICAL(&g_timerMux);
    bool empty = (g_sampleHead == g_sampleTail);
    portEXIT_CRITICAL(&g_timerMux);
    if (empty) {
      g_state.runState = RunState::Idle;
    }
  }
}

void reportBufferOverflowIfAny() {
  if (!g_state.sampleOverflow) {
    return;
  }
  g_state.sampleOverflow = false;
  stopSamplingTimer();
  g_state.runState = RunState::Idle;
  emitError(ERR_OVERFLOW);
}

} // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  analogReadResolution(12);
  delay(100);
  Serial.println(MSG_ONLINE);
}

void loop() {
  serialRxPollToBuffer();

  if (g_cmdOverflow) {
    g_cmdOverflow = false;
  }

  if (g_cmdReady) {
    char cmdLocal[CMD_BUFFER_SIZE];
    strncpy(cmdLocal, g_cmdRxBuffer, CMD_BUFFER_SIZE);
    cmdLocal[CMD_BUFFER_SIZE - 1] = '\0';
    g_cmdReady = false;

    DecodedCommand decoded = decodeCommandLine(cmdLocal);
    CommandResult result = dispatchCommand(decoded, g_state);
    applyCommandResult(result);
  }

  handleSamplingTick();
  streamSamplesToHost();
  reportBufferOverflowIfAny();
}
