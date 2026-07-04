#include "DisplayPanel.h"

#include "AppConfig.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <lvgl.h>

static Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED, 42,
    2, 1, GFX_NOT_DEFINED);

static Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40, 39, 38, 41,
    46, 3, 8, 18, 17,
    14, 13, 12, 11, 10, 9,
    5, 45, 48, 47, 21,
    1, 10, 8, 50,
    1, 10, 8, 20,
    0, 6000000);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    SCREEN_W, SCREEN_H, rgbpanel, DISPLAY_ROTATION, true,
    bus, GFX_NOT_DEFINED,
    st7701_type1_init_operations,
    sizeof(st7701_type1_init_operations));

static lv_disp_draw_buf_t draw_buf;
static lv_disp_t *display_handle = nullptr;
static volatile bool display_reinitializing = false;
static constexpr uint8_t EXPANDER_CONFIG_ON = 0x3a;
static bool backlight_enabled = true;

static void writeExpander(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(IO_EXPANDER_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

static void enablePanelPower()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  // Keep the V4.0 onboard controller in the same output state as Waveshare's examples.
  writeExpander(0x02, 0xdf); // bit5=0 to keep buzzer (EXIO5/BEE_EN) off
  writeExpander(0x03, EXPANDER_CONFIG_ON);
  delay(120);
}

static void lvTick(void *)
{
  lv_tick_inc(LVGL_TICK_MS);
}

static void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  if (display_reinitializing) {
    lv_disp_flush_ready(disp);
    return;
  }

  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

bool DisplayPanel::begin()
{
  enablePanelPower();
  if (!gfx->begin()) return false;

  lv_init();

  const uint32_t buf_pixels = SCREEN_W * 60;
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
  if (!buf1 || !buf2) return false;

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = displayFlush;
  disp_drv.draw_buf = &draw_buf;
  display_handle = lv_disp_drv_register(&disp_drv);

  const esp_timer_create_args_t tick_args = {
      .callback = &lvTick,
      .name = "lvgl_tick"};
  esp_timer_handle_t tick_timer = nullptr;
  esp_timer_create(&tick_args, &tick_timer);
  esp_timer_start_periodic(tick_timer, LVGL_TICK_MS * 1000);
  return true;
}

void DisplayPanel::reinitialize()
{
  display_reinitializing = true;
  lv_timer_handler();
  delay(20);

  enablePanelPower();
  gfx->begin();
  gfx->fillScreen(0x0000);
  delay(20);

  display_reinitializing = false;
  lv_obj_invalidate(lv_scr_act());
  if (display_handle != nullptr) {
    lv_refr_now(display_handle);
  }
}

void DisplayPanel::setBacklight(bool enabled)
{
  backlight_enabled = enabled;
}

bool DisplayPanel::backlightEnabled()
{
  return backlight_enabled;
}
