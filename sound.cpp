#include "sound.h"
#include "config.h"
#include "driver/i2s.h"
#include <math.h>

static const int I2S_PORT = I2S_NUM_0;
static const uint32_t SAMPLE_RATE = 22050;
static const int16_t AMP = 10000;

struct SynthTone {
  bool active = false;
  float freq = 440;
  float phase = 0;
  uint32_t startMs = 0;
  uint32_t endMs = 0;
  WaveType wave = WAVE_SINE;
};

static SynthTone synth;

void soundInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;

  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num = I2S_LRC;
  pins.data_out_num = I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  i2s_driver_install((i2s_port_t)I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin((i2s_port_t)I2S_PORT, &pins);
  i2s_zero_dma_buffer((i2s_port_t)I2S_PORT);
}

static void startTone(float freq, uint32_t durMs, WaveType w) {
  synth.active = true;
  synth.freq = freq;
  synth.phase = 0;
  synth.startMs = millis();
  synth.endMs = synth.startMs + durMs;
  synth.wave = w;
}

void playSound(SoundEvent ev) {
  switch (ev) {
    case SND_PET: startTone(1200, 50, WAVE_SQUARE); break;
    case SND_FEED: startTone(700, 80, WAVE_SAW); break;
    case SND_DRINK: startTone(900, 60, WAVE_SQUARE); break;
    case SND_PLAY: startTone(1500, 120, WAVE_SQUARE); break;
    case SND_WASH: startTone(500, 100, WAVE_SINE); break;
    case SND_SLEEP: startTone(280, 200, WAVE_SINE); break;
    case SND_WAKE: startTone(900, 80, WAVE_SAW); break;
    case SND_HAPPY: startTone(1800, 90, WAVE_SQUARE); break;
    case SND_SAD: startTone(350, 160, WAVE_SINE); break;
    case SND_ANGRY: startTone(200, 180, WAVE_SAW); break;
    case SND_SICK: startTone(260, 220, WAVE_SINE); break;
    case SND_HATCH: startTone(1200, 150, WAVE_SINE); break;
    default: break;
  }
}

static float envelope(uint32_t now, uint32_t start, uint32_t end) {
  const float attackMs = 5.0f;
  const float releaseMs = 20.0f;

  if (now < start + attackMs) {
    return (now - start) / attackMs;  // Attack
  }
  if (now > end - releaseMs) {
    return (end - now) / releaseMs;  // Release
  }
  return 1.0f;
}

static inline float osc(float phase, WaveType w) {
  switch (w) {
    case WAVE_SQUARE:
      return (sinf(phase) >= 0) ? 1.0f : -1.0f;

    case WAVE_SAW:
      return (phase / M_PI) - 1.0f;

    default:
      return sinf(phase);
  }
}

void soundTick() {
  if (!synth.active) return;

  uint32_t now = millis();
  if (now > synth.endMs) {
    synth.active = false;
    return;
  }

  static int16_t buf[512];  // stereo
  float inc = 2.0f * M_PI * synth.freq / SAMPLE_RATE;

  float env = envelope(now, synth.startMs, synth.endMs);

  for (int i = 0; i < 256; i++) {
    float v = osc(synth.phase, synth.wave) * env;
    int16_t s = (int16_t)(v * AMP);

    buf[i * 2] = s;
    buf[i * 2 + 1] = s;

    synth.phase += inc;
    if (synth.phase > 2 * M_PI) synth.phase -= 2 * M_PI;
  }

  size_t written;
  i2s_write((i2s_port_t)I2S_PORT, buf, sizeof(buf), &written, 0);
}
