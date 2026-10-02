// L293D quadruple half-H driver - Wokwi custom chip
//
// Each channel: Y = A while its enable pin is HIGH, otherwise Y is high-impedance.
// Channels 1/2 are enabled by EN12, channels 3/4 by EN34.
// Inputs are watched on both edges, so PWM on EN or A passes straight through.

#include "wokwi-api.h"
#include <stdlib.h>

#define CHANNELS 4

typedef struct {
  pin_t en12;
  pin_t en34;
  pin_t in[CHANNELS];
  pin_t out[CHANNELS];
  bool driving[CHANNELS];
} chip_state_t;

static void update_outputs(chip_state_t *chip) {
  bool en12 = pin_read(chip->en12);
  bool en34 = pin_read(chip->en34);
  for (int i = 0; i < CHANNELS; i++) {
    bool enabled = i < 2 ? en12 : en34;
    if (enabled) {
      bool value = pin_read(chip->in[i]);
      if (chip->driving[i]) {
        pin_write(chip->out[i], value);
      } else {
        // Set mode and level together so a stale level is never driven.
        pin_mode(chip->out[i], value ? OUTPUT_HIGH : OUTPUT_LOW);
        chip->driving[i] = true;
      }
    } else if (chip->driving[i]) {
      pin_mode(chip->out[i], INPUT);  // high-Z
      chip->driving[i] = false;
    }
  }
}

static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  update_outputs((chip_state_t *)user_data);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->en12 = pin_init("EN12", INPUT);
  chip->en34 = pin_init("EN34", INPUT);
  chip->in[0] = pin_init("1A", INPUT);
  chip->in[1] = pin_init("2A", INPUT);
  chip->in[2] = pin_init("3A", INPUT);
  chip->in[3] = pin_init("4A", INPUT);
  chip->out[0] = pin_init("1Y", INPUT);
  chip->out[1] = pin_init("2Y", INPUT);
  chip->out[2] = pin_init("3Y", INPUT);
  chip->out[3] = pin_init("4Y", INPUT);
  for (int i = 0; i < CHANNELS; i++) {
    chip->driving[i] = false;
  }

  // Power pins exist for realistic wiring; they are not simulated electrically.
  pin_init("VCC1", INPUT);
  pin_init("VCC2", INPUT);
  pin_init("GND.1", INPUT);
  pin_init("GND.2", INPUT);
  pin_init("GND.3", INPUT);
  pin_init("GND.4", INPUT);

  const pin_watch_config_t watch = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = chip,
  };
  pin_watch(chip->en12, &watch);
  pin_watch(chip->en34, &watch);
  for (int i = 0; i < CHANNELS; i++) {
    pin_watch(chip->in[i], &watch);
  }

  update_outputs(chip);
}
