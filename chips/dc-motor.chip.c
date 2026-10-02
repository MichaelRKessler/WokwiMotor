// Brushed DC motor - Wokwi custom chip
//
// Drive = M1 minus M2 (each read as 0/1), time-averaged over each display frame,
// so PWM duty cycle sets the speed and swapping polarity reverses direction.
// Speed follows the drive through a first-order lag (rotor inertia).
//
// Attributes:
//   maxRpm  - speed at 100% drive (default 60, kept low so rotation is visible)
//   tau     - mechanical time constant in seconds (default 0.25)

#include "wokwi-api.h"
#include <math.h>
#include <stdlib.h>

#define FRAME_US 33333  // ~30 fps
#define CX (chip->width / 2)
#define CY (chip->height / 2 - 6)

#define RGBA(r, g, b) ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | 0xff000000u)
#define COLOR_BG      RGBA(0x20, 0x20, 0x20)
#define COLOR_HOUSING RGBA(0x70, 0x70, 0x78)
#define COLOR_ROTOR   RGBA(0xb0, 0x8a, 0x40)
#define COLOR_SPOKE   RGBA(0xe0, 0xe0, 0xe0)
#define COLOR_MARK    RGBA(0xe0, 0x30, 0x30)
#define COLOR_HUB     RGBA(0x30, 0x30, 0x30)
#define COLOR_TRACK   RGBA(0x40, 0x40, 0x40)
#define COLOR_FWD     RGBA(0x30, 0xc0, 0x50)
#define COLOR_REV     RGBA(0xe0, 0x50, 0x30)

typedef struct {
  pin_t m_pos;
  pin_t m_neg;
  int drive;            // current instantaneous drive: -1, 0, +1
  uint64_t last_edge_ns;
  double drive_acc_ns;  // integral of drive over the current frame
  uint64_t frame_start_ns;

  float max_rpm;
  float tau;
  float rpm;
  float angle;          // degrees

  buffer_t fb;
  uint32_t width;
  uint32_t height;
  uint32_t *pixels;
} chip_state_t;

static int read_drive(chip_state_t *chip) {
  return (int)pin_read(chip->m_pos) - (int)pin_read(chip->m_neg);
}

static void accumulate(chip_state_t *chip, uint64_t now) {
  chip->drive_acc_ns += (double)chip->drive * (double)(now - chip->last_edge_ns);
  chip->last_edge_ns = now;
}

static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = user_data;
  accumulate(chip, get_sim_nanos());
  chip->drive = read_drive(chip);
}

static void render(chip_state_t *chip) {
  const float r_housing = CY - 3;
  const float r_rotor = r_housing - 5;
  const float r_hub = 5;
  const float spoke_half_width = 2.5f;
  const float rad = chip->angle * (float)M_PI / 180.0f;

  const int bar_y0 = chip->height - 11;
  const int bar_y1 = chip->height - 4;
  const int bar_half = chip->width / 2 - 6;
  const int bar_len = (int)(fabsf(chip->rpm) / chip->max_rpm * bar_half + 0.5f);

  for (uint32_t y = 0; y < chip->height; y++) {
    for (uint32_t x = 0; x < chip->width; x++) {
      uint32_t color = COLOR_BG;
      float dx = (float)x - CX + 0.5f;
      float dy = (float)y - CY + 0.5f;
      float d = sqrtf(dx * dx + dy * dy);

      if (d <= r_hub) {
        color = COLOR_HUB;
      } else if (d <= r_rotor) {
        color = COLOR_ROTOR;
        // Three spokes 120 degrees apart; spoke 0 is marked red so direction is obvious.
        for (int s = 0; s < 3; s++) {
          float a = rad + s * 2.0f * (float)M_PI / 3.0f;
          float ux = cosf(a), uy = sinf(a);
          float along = dx * ux + dy * uy;
          float across = fabsf(-dx * uy + dy * ux);
          if (along > 0 && across <= spoke_half_width) {
            color = s == 0 ? COLOR_MARK : COLOR_SPOKE;
          }
        }
      } else if (d <= r_housing) {
        color = COLOR_HOUSING;
      } else if ((int)y >= bar_y0 && (int)y < bar_y1) {
        // Speed bar: grows right (green) for forward, left (red) for reverse.
        int off = (int)x - (int)(chip->width / 2);
        if (abs(off) <= bar_half) {
          color = COLOR_TRACK;
          if (chip->rpm > 0 && off >= 0 && off < bar_len) color = COLOR_FWD;
          if (chip->rpm < 0 && off < 0 && -off <= bar_len) color = COLOR_REV;
        }
      }
      chip->pixels[y * chip->width + x] = color;
    }
  }
  buffer_write(chip->fb, 0, chip->pixels, chip->width * chip->height * sizeof(uint32_t));
}

static void on_frame(void *user_data) {
  chip_state_t *chip = user_data;
  uint64_t now = get_sim_nanos();
  accumulate(chip, now);

  double frame_ns = (double)(now - chip->frame_start_ns);
  float avg_drive = frame_ns > 0 ? (float)(chip->drive_acc_ns / frame_ns) : 0.0f;
  float dt = (float)(frame_ns * 1e-9);
  chip->drive_acc_ns = 0;
  chip->frame_start_ns = now;

  float target = avg_drive * chip->max_rpm;
  chip->rpm += (target - chip->rpm) * (1.0f - expf(-dt / chip->tau));
  if (fabsf(chip->rpm) < 0.01f) chip->rpm = 0;
  chip->angle = fmodf(chip->angle + chip->rpm * 6.0f * dt, 360.0f);  // rpm * 360/60

  render(chip);
}

void chip_init(void) {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  // Pull-downs: when the driver outputs are high-Z (enable low) both terminals
  // read LOW, so the motor coasts instead of seeing undefined levels.
  chip->m_pos = pin_init("M1", INPUT_PULLDOWN);
  chip->m_neg = pin_init("M2", INPUT_PULLDOWN);
  chip->max_rpm = attr_read_float(attr_init_float("maxRpm", 60.0f));
  chip->tau = attr_read_float(attr_init_float("tau", 0.25f));
  if (chip->max_rpm <= 0) chip->max_rpm = 60.0f;
  if (chip->tau <= 0) chip->tau = 0.25f;

  chip->fb = framebuffer_init(&chip->width, &chip->height);
  chip->pixels = malloc(chip->width * chip->height * sizeof(uint32_t));

  uint64_t now = get_sim_nanos();
  chip->last_edge_ns = now;
  chip->frame_start_ns = now;
  chip->drive = read_drive(chip);

  const pin_watch_config_t watch = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = chip,
  };
  pin_watch(chip->m_pos, &watch);
  pin_watch(chip->m_neg, &watch);

  const timer_config_t timer = {
    .callback = on_frame,
    .user_data = chip,
  };
  timer_start(timer_init(&timer), FRAME_US, true);

  render(chip);
}
