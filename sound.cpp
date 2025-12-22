#include "sound.h"
#include "config.h"
#include "driver/i2s.h"
#include <math.h>

static const int I2S_PORT = I2S_NUM_0;
static const uint32_t SAMPLE_RATE = 22050;
static const int16_t AMP = 6000;

struct SynthTone {
  bool active = false;
  float freq = 440;
  float phase = 0;
  uint32_t endMs = 0;
};

static SynthTone synth;

void soundInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;

  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num  = I2S_LRC;
  pins.data_out_num = I2S_DOUT;
  pins.data_in_num  = I2S_PIN_NO_CHANGE;

  i2s_driver_install((i2s_port_t)I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin((i2s_port_t)I2S_PORT, &pins);
  i2s_zero_dma_buffer((i2s_port_t)I2S_PORT);
}

static void startTone(float freq, uint32_t durMs) {
  synth.active = true;
  synth.freq = freq;
  synth.phase = 0;
  synth.endMs = millis() + durMs;
}

void playSound(SoundEvent ev) {
  switch (ev) {
    case SND_PET:    startTone(1200, 60); break;
    case SND_FEED:   startTone(700, 80);  break;
    case SND_DRINK:  startTone(900, 80);  break;
    case SND_PLAY:   startTone(1400, 120);break;
    case SND_WASH:   startTone(500, 100); break;
    case SND_SLEEP:  startTone(300, 200); break;
    case SND_WAKE:   startTone(800, 120); break;
    case SND_HAPPY:  startTone(1600, 90); break;
    case SND_SAD:    startTone(350, 160); break;
    case SND_ANGRY:  startTone(220, 180); break;
    case SND_SICK:   startTone(260, 220); break;
    default: break;
  }
}

void soundTick() {
  if (!synth.active) return;

  if (millis() > synth.endMs) {
    synth.active = false;
    return;
  }

  static int16_t buf[256];
  float inc = 2.0f * M_PI * synth.freq / SAMPLE_RATE;

  for (int i = 0; i < 256; i++) {
    buf[i] = (int16_t)(sinf(synth.phase) * AMP);
    synth.phase += inc;
    if (synth.phase > 2 * M_PI) synth.phase -= 2 * M_PI;
  }

  size_t written;
  i2s_write((i2s_port_t)I2S_PORT, buf, sizeof(buf), &written, 0);
}
