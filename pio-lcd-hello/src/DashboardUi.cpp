#include "DashboardUi.h"

#include "AppConfig.h"

#include <WiFi.h>
#include <lvgl.h>
#include <cstring>
#include <time.h>

struct ThemePalette {
  lv_color_t bg;
  lv_color_t bg_grad;
  lv_color_t panel;
  lv_color_t panel_2;
  lv_color_t chart_bg;
  lv_color_t border;
  lv_color_t grid;
  lv_color_t text;
  lv_color_t sub;
  lv_color_t dim;
  lv_color_t eth0_rx;
  lv_color_t eth0_tx;
  lv_color_t eth1_rx;
  lv_color_t eth1_tx;
  lv_color_t mint;
  lv_color_t amber;
  lv_color_t blue;
  lv_color_t red;
  lv_color_t ok;
  lv_color_t bar_bg;
};

static const ThemePalette NIGHT = {
  lv_color_hex(0x070b10), lv_color_hex(0x101824), lv_color_hex(0x101822),
  lv_color_hex(0x0c121a), lv_color_hex(0x090e14), lv_color_hex(0x2b4052),
  lv_color_hex(0x203140), lv_color_hex(0xf4f8fb), lv_color_hex(0x8ea4b5),
  lv_color_hex(0x5a7182), lv_color_hex(0x38dff8), lv_color_hex(0xff66c7),
  lv_color_hex(0x63ee95), lv_color_hex(0xffd36b), lv_color_hex(0x63ee95),
  lv_color_hex(0xffd36b), lv_color_hex(0x6aa8ff), lv_color_hex(0xff6478),
  lv_color_hex(0x72e895), lv_color_hex(0x1e2a36),
};

static const ThemePalette DAY = {
  lv_color_hex(0xf5f8fb), lv_color_hex(0xe5edf4), lv_color_hex(0xffffff),
  lv_color_hex(0xf7fafc), lv_color_hex(0xf0f5f8), lv_color_hex(0xc3d1dc),
  lv_color_hex(0xcdd9e2), lv_color_hex(0x17212b), lv_color_hex(0x536879),
  lv_color_hex(0x7d909e), lv_color_hex(0x008da3), lv_color_hex(0xd92882),
  lv_color_hex(0x0a9d52), lv_color_hex(0xb27a00), lv_color_hex(0x0a9d52),
  lv_color_hex(0xb27a00), lv_color_hex(0x1f6fd1), lv_color_hex(0xc92a42),
  lv_color_hex(0x168947), lv_color_hex(0xdce5ec),
};

static const ThemePalette *theme = &NIGHT;
static bool is_day_theme = false;
static bool main_visible = false;

static lv_obj_t *screen_obj;
static lv_obj_t *main_root;
static lv_obj_t *connect_root;
static lv_obj_t *connect_title_label;
static lv_obj_t *connect_wifi_label;
static lv_obj_t *connect_ip_label;
static lv_obj_t *connect_route_label;
static lv_obj_t *connect_target_label;
static lv_obj_t *connect_error_label;
static lv_obj_t *connect_retry_label;
static lv_obj_t *status_label;
static lv_obj_t *ip_label;
static lv_obj_t *clock_label;
static lv_obj_t *eth0_panel;
static lv_obj_t *eth1_panel;
static lv_obj_t *cpu_panel;
static lv_obj_t *sys_panel;
static lv_obj_t *eth0_name_label;
static lv_obj_t *eth1_name_label;
static lv_obj_t *eth0_rx_label;
static lv_obj_t *eth0_tx_label;
static lv_obj_t *eth0_sum_label;
static lv_obj_t *eth0_scale_label;
static lv_obj_t *eth0_rx_tag;
static lv_obj_t *eth0_tx_tag;
static lv_obj_t *eth1_rx_label;
static lv_obj_t *eth1_tx_label;
static lv_obj_t *eth1_sum_label;
static lv_obj_t *eth1_scale_label;
static lv_obj_t *eth1_rx_tag;
static lv_obj_t *eth1_tx_tag;
static lv_obj_t *eth0_chart;
static lv_obj_t *eth1_chart;
static lv_chart_series_t *eth0_rx_series;
static lv_chart_series_t *eth0_tx_series;
static lv_chart_series_t *eth1_rx_series;
static lv_chart_series_t *eth1_tx_series;
static lv_obj_t *core_bar[4];
static lv_obj_t *core_label[4];
static lv_obj_t *cpu_title_label;
static lv_obj_t *sys_title_label;
static lv_obj_t *ram_bar;
static lv_obj_t *ram_label;
static lv_obj_t *sys_line_1;
static lv_obj_t *sys_line_2;
static lv_obj_t *temp_label;
static float eth0_rx_window[CHART_POINTS];
static float eth0_tx_window[CHART_POINTS];
static float eth1_rx_window[CHART_POINTS];
static float eth1_tx_window[CHART_POINTS];

static void setTextColor(lv_obj_t *obj, lv_color_t color)
{
  if (obj) lv_obj_set_style_text_color(obj, color, 0);
}

static void setPanelColors(lv_obj_t *obj, lv_color_t bg)
{
  if (!obj) return;
  lv_obj_set_style_bg_color(obj, bg, 0);
  lv_obj_set_style_border_color(obj, theme->border, 0);
}

static void applyTheme(bool day)
{
  if (day == is_day_theme && screen_obj) return;
  is_day_theme = day;
  theme = day ? &DAY : &NIGHT;
  if (!screen_obj) return;

  lv_obj_set_style_bg_color(screen_obj, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(screen_obj, theme->bg_grad, 0);
  lv_obj_set_style_bg_color(main_root, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(main_root, theme->bg_grad, 0);
  lv_obj_set_style_bg_color(connect_root, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(connect_root, theme->bg_grad, 0);
  setPanelColors(eth0_panel, theme->panel);
  setPanelColors(eth1_panel, theme->panel);
  setPanelColors(cpu_panel, theme->panel_2);
  setPanelColors(sys_panel, theme->panel_2);

  lv_obj_set_style_bg_color(eth0_chart, theme->chart_bg, 0);
  lv_obj_set_style_bg_color(eth1_chart, theme->chart_bg, 0);
  lv_obj_set_style_line_color(eth0_chart, theme->grid, LV_PART_MAIN);
  lv_obj_set_style_line_color(eth1_chart, theme->grid, LV_PART_MAIN);
  lv_chart_set_series_color(eth0_chart, eth0_rx_series, theme->eth0_rx);
  lv_chart_set_series_color(eth0_chart, eth0_tx_series, theme->eth0_tx);
  lv_chart_set_series_color(eth1_chart, eth1_rx_series, theme->eth1_rx);
  lv_chart_set_series_color(eth1_chart, eth1_tx_series, theme->eth1_tx);

  setTextColor(clock_label, theme->dim);
  setTextColor(ip_label, theme->dim);
  setTextColor(eth0_name_label, theme->text);
  setTextColor(eth1_name_label, theme->text);
  setTextColor(eth0_sum_label, theme->sub);
  setTextColor(eth1_sum_label, theme->sub);
  setTextColor(eth0_scale_label, theme->dim);
  setTextColor(eth1_scale_label, theme->dim);
  setTextColor(eth0_rx_tag, theme->eth0_rx);
  setTextColor(eth0_rx_label, theme->eth0_rx);
  setTextColor(eth0_tx_tag, theme->eth0_tx);
  setTextColor(eth0_tx_label, theme->eth0_tx);
  setTextColor(eth1_rx_tag, theme->eth1_rx);
  setTextColor(eth1_rx_label, theme->eth1_rx);
  setTextColor(eth1_tx_tag, theme->eth1_tx);
  setTextColor(eth1_tx_label, theme->eth1_tx);
  setTextColor(cpu_title_label, theme->text);
  setTextColor(sys_title_label, theme->text);
  setTextColor(temp_label, theme->amber);
  setTextColor(ram_label, theme->mint);
  setTextColor(sys_line_1, theme->sub);
  setTextColor(sys_line_2, theme->dim);
  setTextColor(connect_title_label, theme->text);
  setTextColor(connect_wifi_label, theme->sub);
  setTextColor(connect_ip_label, theme->sub);
  setTextColor(connect_route_label, theme->sub);
  setTextColor(connect_target_label, theme->sub);
  setTextColor(connect_error_label, theme->red);
  setTextColor(connect_retry_label, theme->dim);

  for (int i = 0; i < 4; i++) {
    lv_obj_set_style_bg_color(core_bar[i], theme->bar_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_color(core_bar[i], i % 2 ? theme->blue : theme->amber, LV_PART_INDICATOR);
  }
  lv_obj_set_style_bg_color(ram_bar, theme->bar_bg, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ram_bar, theme->mint, LV_PART_INDICATOR);
}

static void setMainVisible(bool visible)
{
  if (main_visible == visible) return;
  main_visible = visible;
  if (visible) {
    lv_obj_clear_flag(main_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(connect_root, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(main_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(connect_root, LV_OBJ_FLAG_HIDDEN);
  }
}

static const char *wifiStatusText(wl_status_t status)
{
  switch (status) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO SSID";
    case WL_SCAN_COMPLETED: return "SCAN DONE";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT FAILED";
    case WL_CONNECTION_LOST: return "LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

static String formatSpeed(float kbps)
{
  if (kbps < 100.0f) return String(kbps, 1) + "K";
  if (kbps < 1024.0f) return String((int)kbps) + "K";
  if (kbps < 1024.0f * 100.0f) return String(kbps / 1024.0f, 1) + "M";
  return String(kbps / 1024.0f, 0) + "M";
}

static String formatSpeedCompact(float kbps)
{
  if (kbps < 100.0f) return String(kbps, 1) + "K";
  if (kbps < 1024.0f) return String((int)kbps) + "K";
  return String(kbps / 1024.0f, 1) + "M";
}

static void pushHistory(float *history, float value)
{
  if (value < 0.0f) value = 0.0f;
  if (value > CHART_MAX_KBPS) value = CHART_MAX_KBPS;
  for (uint16_t i = 0; i < CHART_POINTS - 1; i++) history[i] = history[i + 1];
  history[CHART_POINTS - 1] = value;
}

static int autoChartMax(const float *rx_history, const float *tx_history)
{
  float peak = 0.0f;
  for (uint16_t i = 0; i < CHART_POINTS; i++) {
    if (rx_history[i] > peak) peak = rx_history[i];
    if (tx_history[i] > peak) peak = tx_history[i];
  }
  if (peak < 500.0f) return 500;
  if (peak > CHART_MAX_KBPS) return CHART_MAX_KBPS;
  return (int)(peak + 0.999f);
}

static int chartValue(float kbps)
{
  if (kbps < 0) kbps = 0;
  if (kbps > CHART_MAX_KBPS) kbps = CHART_MAX_KBPS;
  return (int)kbps;
}

static void updateChartScale(lv_obj_t *chart, lv_obj_t *scale_label, int chart_max)
{
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, chart_max);
  lv_label_set_text_fmt(scale_label, "%s", formatSpeedCompact(chart_max).c_str());
}

static lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, lv_color_t color, const lv_font_t *font)
{
  lv_obj_t *obj = lv_label_create(parent);
  lv_label_set_text(obj, text);
  lv_obj_set_style_text_color(obj, color, 0);
  lv_obj_set_style_text_font(obj, font, 0);
  return obj;
}

static void styleCard(lv_obj_t *obj, lv_color_t bg)
{
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(obj, bg, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(obj, theme->border, 0);
  lv_obj_set_style_border_opa(obj, LV_OPA_70, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 10, 0);
  lv_obj_set_style_pad_all(obj, 9, 0);
}

static lv_obj_t *makeLineChart(lv_obj_t *parent, lv_color_t rx_color, lv_color_t tx_color,
                               lv_chart_series_t **rx_series, lv_chart_series_t **tx_series)
{
  lv_obj_t *obj = lv_chart_create(parent);
  lv_obj_set_size(obj, 320, 108);
  lv_chart_set_type(obj, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(obj, CHART_POINTS);
  lv_chart_set_range(obj, LV_CHART_AXIS_PRIMARY_Y, 0, CHART_MAX_KBPS);
  lv_chart_set_update_mode(obj, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_div_line_count(obj, 3, 5);
  lv_obj_set_style_bg_color(obj, theme->chart_bg, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_line_color(obj, theme->grid, LV_PART_MAIN);
  lv_obj_set_style_line_width(obj, 1, LV_PART_MAIN);
  lv_obj_set_style_size(obj, 0, LV_PART_INDICATOR);
  *rx_series = lv_chart_add_series(obj, rx_color, LV_CHART_AXIS_PRIMARY_Y);
  *tx_series = lv_chart_add_series(obj, tx_color, LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_all_value(obj, *rx_series, 0);
  lv_chart_set_all_value(obj, *tx_series, 0);
  return obj;
}

static lv_obj_t *makeEthPanel(lv_obj_t *parent, const char *name, lv_color_t rx_color, lv_color_t tx_color,
                              lv_obj_t **rx_label, lv_obj_t **tx_label, lv_obj_t **sum_label,
                              lv_obj_t **scale_label,
                              lv_obj_t **chart_out, lv_chart_series_t **rx_series,
                              lv_chart_series_t **tx_series)
{
  lv_obj_t *panel = lv_obj_create(parent);
  lv_obj_set_size(panel, 444, 146);
  styleCard(panel, theme->panel);

  lv_obj_t *name_label = makeLabel(panel, name, theme->text, &lv_font_montserrat_16);
  lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, -1);
  if (strcmp(name, "eth0") == 0) eth0_name_label = name_label;
  else eth1_name_label = name_label;

  *sum_label = makeLabel(panel, "0.0K / 0.0K", theme->sub, &lv_font_montserrat_12);
  lv_obj_align(*sum_label, LV_ALIGN_TOP_LEFT, 0, 25);

  *scale_label = makeLabel(panel, "500K", theme->dim, &lv_font_montserrat_12);
  lv_obj_align(*scale_label, LV_ALIGN_TOP_LEFT, 0, 43);

  lv_obj_t *rx_tag = makeLabel(panel, "RX", rx_color, &lv_font_montserrat_12);
  if (strcmp(name, "eth0") == 0) eth0_rx_tag = rx_tag;
  else eth1_rx_tag = rx_tag;
  lv_obj_align(rx_tag, LV_ALIGN_TOP_LEFT, 0, 66);
  *rx_label = makeLabel(panel, "0.0K", rx_color, &lv_font_montserrat_24);
  lv_obj_set_width(*rx_label, 110);
  lv_label_set_long_mode(*rx_label, LV_LABEL_LONG_CLIP);
  lv_obj_align(*rx_label, LV_ALIGN_TOP_LEFT, 20, 54);

  lv_obj_t *tx_tag = makeLabel(panel, "TX", tx_color, &lv_font_montserrat_12);
  if (strcmp(name, "eth0") == 0) eth0_tx_tag = tx_tag;
  else eth1_tx_tag = tx_tag;
  lv_obj_align(tx_tag, LV_ALIGN_TOP_LEFT, 0, 98);
  *tx_label = makeLabel(panel, "0.0K", tx_color, &lv_font_montserrat_24);
  lv_obj_set_width(*tx_label, 110);
  lv_label_set_long_mode(*tx_label, LV_LABEL_LONG_CLIP);
  lv_obj_align(*tx_label, LV_ALIGN_TOP_LEFT, 20, 86);

  *chart_out = makeLineChart(panel, rx_color, tx_color, rx_series, tx_series);
  lv_obj_align(*chart_out, LV_ALIGN_RIGHT_MID, 0, 8);
  return panel;
}

static lv_obj_t *makeCoreBlock(lv_obj_t *parent, int idx)
{
  lv_obj_t *block = lv_obj_create(parent);
  lv_obj_set_size(block, 50, 94);
  lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(block, 0, 0);
  lv_obj_set_style_pad_all(block, 0, 0);

  core_label[idx] = makeLabel(block, "C0\n0%", theme->sub, &lv_font_montserrat_14);
  lv_obj_align(core_label[idx], LV_ALIGN_TOP_MID, 0, 0);

  core_bar[idx] = lv_bar_create(block);
  lv_obj_set_size(core_bar[idx], 14, 54);
  lv_obj_align(core_bar[idx], LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_bar_set_range(core_bar[idx], 0, 100);
  lv_bar_set_value(core_bar[idx], 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(core_bar[idx], theme->bar_bg, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(core_bar[idx], LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(core_bar[idx], idx % 2 ? theme->blue : theme->amber, LV_PART_INDICATOR);
  lv_obj_set_style_radius(core_bar[idx], 6, LV_PART_MAIN);
  lv_obj_set_style_radius(core_bar[idx], 6, LV_PART_INDICATOR);
  return block;
}

void DashboardUi::build()
{
  lv_obj_t *screen = lv_scr_act();
  screen_obj = screen;
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(screen, theme->bg_grad, 0);
  lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

  main_root = lv_obj_create(screen);
  lv_obj_set_size(main_root, SCREEN_W, SCREEN_H);
  lv_obj_align(main_root, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(main_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(main_root, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(main_root, theme->bg_grad, 0);
  lv_obj_set_style_bg_grad_dir(main_root, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(main_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(main_root, 0, 0);
  lv_obj_set_style_pad_all(main_root, 0, 0);

  connect_root = lv_obj_create(screen);
  lv_obj_set_size(connect_root, SCREEN_W, SCREEN_H);
  lv_obj_align(connect_root, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(connect_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(connect_root, theme->bg, 0);
  lv_obj_set_style_bg_grad_color(connect_root, theme->bg_grad, 0);
  lv_obj_set_style_bg_grad_dir(connect_root, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(connect_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(connect_root, 0, 0);
  lv_obj_set_style_pad_all(connect_root, 0, 0);

  connect_title_label = makeLabel(connect_root, "NETWORK LINK", theme->text, &lv_font_montserrat_24);
  lv_obj_align(connect_title_label, LV_ALIGN_TOP_LEFT, 34, 42);
  connect_wifi_label = makeLabel(connect_root, "WiFi: boot", theme->sub, &lv_font_montserrat_18);
  lv_obj_align(connect_wifi_label, LV_ALIGN_TOP_LEFT, 36, 96);
  connect_ip_label = makeLabel(connect_root, "IP: --", theme->sub, &lv_font_montserrat_18);
  lv_obj_align(connect_ip_label, LV_ALIGN_TOP_LEFT, 36, 136);
  connect_route_label = makeLabel(connect_root, "Gateway: --", theme->sub, &lv_font_montserrat_18);
  lv_obj_align(connect_route_label, LV_ALIGN_TOP_LEFT, 36, 176);
  connect_target_label = makeLabel(connect_root, "Stats: --", theme->sub, &lv_font_montserrat_18);
  lv_obj_align(connect_target_label, LV_ALIGN_TOP_LEFT, 36, 216);
  connect_error_label = makeLabel(connect_root, "Waiting for WiFi", theme->red, &lv_font_montserrat_22);
  lv_obj_align(connect_error_label, LV_ALIGN_TOP_LEFT, 36, 282);
  connect_retry_label = makeLabel(connect_root, "reconnects 0", theme->dim, &lv_font_montserrat_16);
  lv_obj_align(connect_retry_label, LV_ALIGN_BOTTOM_LEFT, 36, -38);

  status_label = makeLabel(main_root, "CONNECTING", theme->amber, &lv_font_montserrat_14);
  lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 20, 10);

  clock_label = makeLabel(main_root, "--:--", theme->dim, &lv_font_montserrat_14);
  lv_obj_align(clock_label, LV_ALIGN_TOP_RIGHT, -20, 10);

  ip_label = makeLabel(main_root, "IP --", theme->dim, &lv_font_montserrat_14);
  lv_obj_align_to(ip_label, clock_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);

  eth0_panel = makeEthPanel(main_root, "eth0", theme->eth0_rx, theme->eth0_tx,
                                      &eth0_rx_label, &eth0_tx_label, &eth0_sum_label, &eth0_scale_label,
                                      &eth0_chart, &eth0_rx_series, &eth0_tx_series);
  lv_obj_align(eth0_panel, LV_ALIGN_TOP_MID, 0, 34);

  eth1_panel = makeEthPanel(main_root, "eth1", theme->eth1_rx, theme->eth1_tx,
                                      &eth1_rx_label, &eth1_tx_label, &eth1_sum_label, &eth1_scale_label,
                                      &eth1_chart, &eth1_rx_series, &eth1_tx_series);
  lv_obj_align(eth1_panel, LV_ALIGN_TOP_MID, 0, 186);

  cpu_panel = lv_obj_create(main_root);
  lv_obj_set_size(cpu_panel, 218, 126);
  lv_obj_align(cpu_panel, LV_ALIGN_BOTTOM_LEFT, 18, -18);
  styleCard(cpu_panel, theme->panel_2);

  cpu_title_label = makeLabel(cpu_panel, "CPU CORES", theme->text, &lv_font_montserrat_16);
  lv_obj_align(cpu_title_label, LV_ALIGN_TOP_LEFT, 0, -1);
  temp_label = makeLabel(cpu_panel, "--C", theme->amber, &lv_font_montserrat_16);
  lv_obj_align(temp_label, LV_ALIGN_TOP_RIGHT, 0, -1);

  lv_obj_t *core_row = lv_obj_create(cpu_panel);
  lv_obj_set_size(core_row, 206, 92);
  lv_obj_align(core_row, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_clear_flag(core_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(core_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(core_row, 0, 0);
  lv_obj_set_style_pad_all(core_row, 0, 0);
  lv_obj_set_style_pad_column(core_row, 2, 0);
  lv_obj_set_flex_flow(core_row, LV_FLEX_FLOW_ROW);
  for (int i = 0; i < 4; i++) makeCoreBlock(core_row, i);

  sys_panel = lv_obj_create(main_root);
  lv_obj_set_size(sys_panel, 218, 126);
  lv_obj_align(sys_panel, LV_ALIGN_BOTTOM_RIGHT, -18, -18);
  styleCard(sys_panel, theme->panel_2);

  sys_title_label = makeLabel(sys_panel, "SYSTEM", theme->text, &lv_font_montserrat_16);
  lv_obj_align(sys_title_label, LV_ALIGN_TOP_LEFT, 0, -1);

  ram_label = makeLabel(sys_panel, "RAM 0%", theme->mint, &lv_font_montserrat_22);
  lv_obj_align(ram_label, LV_ALIGN_TOP_LEFT, 0, 24);
  ram_bar = lv_bar_create(sys_panel);
  lv_obj_set_size(ram_bar, 194, 9);
  lv_obj_align(ram_bar, LV_ALIGN_TOP_LEFT, 0, 54);
  lv_bar_set_range(ram_bar, 0, 100);
  lv_obj_set_style_bg_color(ram_bar, theme->bar_bg, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ram_bar, theme->mint, LV_PART_INDICATOR);
  lv_obj_set_style_radius(ram_bar, 5, LV_PART_MAIN);
  lv_obj_set_style_radius(ram_bar, 5, LV_PART_INDICATOR);

  sys_line_1 = makeLabel(sys_panel, "conn --   clients --", theme->sub, &lv_font_montserrat_16);
  lv_obj_align(sys_line_1, LV_ALIGN_TOP_LEFT, 0, 72);
  sys_line_2 = makeLabel(sys_panel, "uptime --", theme->dim, &lv_font_montserrat_14);
  lv_obj_align(sys_line_2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  setMainVisible(false);
}

void DashboardUi::updateClock()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5)) {
    lv_label_set_text(clock_label, "---- -- -- --:--");
    return;
  }
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &timeinfo);
  lv_label_set_text(clock_label, buf);
  applyTheme(timeinfo.tm_hour >= 7 && timeinfo.tm_hour < 19);
}

void DashboardUi::updateThemeFromClock()
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5)) {
    applyTheme(timeinfo.tm_hour >= 7 && timeinfo.tm_hour < 19);
  }
}

void DashboardUi::updateNetworkIdentity()
{
  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text_fmt(ip_label, "IP %s", WiFi.localIP().toString().c_str());
  } else {
    lv_label_set_text(ip_label, "IP --");
  }
  lv_obj_align_to(ip_label, clock_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);
}

void DashboardUi::updateConnectionView(const Stats &stats, uint32_t reconnect_count)
{
  wl_status_t wifi_status = WiFi.status();
  if (wifi_status == WL_CONNECTED) {
    lv_label_set_text_fmt(connect_wifi_label, "WiFi: %s   RSSI %d dBm",
                          wifiStatusText(wifi_status), WiFi.RSSI());
    lv_label_set_text_fmt(connect_ip_label, "IP: %s   MAC %s",
                          WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());
    lv_label_set_text_fmt(connect_route_label, "Gateway: %s   DNS %s",
                          WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
    lv_label_set_text_fmt(connect_target_label, "Stats: %s", statsTargetUrl().c_str());
  } else {
    lv_label_set_text_fmt(connect_wifi_label, "WiFi: %s   SSID %s",
                          wifiStatusText(wifi_status), WIFI_SSID);
    lv_label_set_text(connect_ip_label, "IP: --");
    lv_label_set_text(connect_route_label, "Gateway: --");
    lv_label_set_text_fmt(connect_target_label, "Stats: %s", statsTargetUrl().c_str());
  }

  if (stats.online) {
    lv_label_set_text(connect_error_label, "Stats link OK");
    lv_obj_set_style_text_color(connect_error_label, theme->ok, 0);
  } else if (wifi_status == WL_CONNECTED) {
    lv_label_set_text_fmt(connect_error_label, "Router stats: %s", stats.error.c_str());
    lv_obj_set_style_text_color(connect_error_label, theme->red, 0);
  } else {
    lv_label_set_text(connect_error_label, "Connecting WiFi...");
    lv_obj_set_style_text_color(connect_error_label, theme->amber, 0);
  }
  lv_label_set_text_fmt(connect_retry_label, "reconnects %lu   uptime %lus",
                        (unsigned long)reconnect_count, (unsigned long)(millis() / 1000));
}

void DashboardUi::updateStats(const Stats &stats)
{
  setMainVisible(stats.online);

  pushHistory(eth0_rx_window, stats.eth0_rx);
  pushHistory(eth0_tx_window, stats.eth0_tx);
  pushHistory(eth1_rx_window, stats.eth1_rx);
  pushHistory(eth1_tx_window, stats.eth1_tx);

  lv_label_set_text(eth0_rx_label, formatSpeed(stats.eth0_rx).c_str());
  lv_label_set_text(eth0_tx_label, formatSpeed(stats.eth0_tx).c_str());
  lv_label_set_text_fmt(eth0_sum_label, "%s / %s",
                        formatSpeedCompact(stats.eth0_rx).c_str(),
                        formatSpeedCompact(stats.eth0_tx).c_str());

  lv_label_set_text(eth1_rx_label, formatSpeed(stats.eth1_rx).c_str());
  lv_label_set_text(eth1_tx_label, formatSpeed(stats.eth1_tx).c_str());
  lv_label_set_text_fmt(eth1_sum_label, "%s / %s",
                        formatSpeedCompact(stats.eth1_rx).c_str(),
                        formatSpeedCompact(stats.eth1_tx).c_str());

  int eth0_max = autoChartMax(eth0_rx_window, eth0_tx_window);
  int eth1_max = autoChartMax(eth1_rx_window, eth1_tx_window);
  updateChartScale(eth0_chart, eth0_scale_label, eth0_max);
  updateChartScale(eth1_chart, eth1_scale_label, eth1_max);

  lv_chart_set_next_value(eth0_chart, eth0_rx_series, chartValue(stats.eth0_rx));
  lv_chart_set_next_value(eth0_chart, eth0_tx_series, chartValue(stats.eth0_tx));
  lv_chart_set_next_value(eth1_chart, eth1_rx_series, chartValue(stats.eth1_rx));
  lv_chart_set_next_value(eth1_chart, eth1_tx_series, chartValue(stats.eth1_tx));

  for (int i = 0; i < 4; i++) {
    lv_label_set_text_fmt(core_label[i], "C%d\n%d%%", i, stats.cores[i]);
    lv_obj_set_style_text_color(core_label[i], stats.cores[i] > 80 ? theme->red : theme->sub, 0);
    lv_bar_set_value(core_bar[i], stats.cores[i], LV_ANIM_OFF);
  }

  lv_label_set_text_fmt(temp_label, "%dC", stats.temp);
  lv_label_set_text_fmt(ram_label, "RAM %d%%", stats.mem);
  lv_bar_set_value(ram_bar, stats.mem, LV_ANIM_OFF);
  lv_label_set_text_fmt(sys_line_1, "conn %d   clients %d", stats.conn, stats.clients);
  lv_label_set_text_fmt(sys_line_2, "uptime %s", stats.uptime.substring(0, 11).c_str());

  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text(status_label, stats.online ? "ONLINE" : stats.error.c_str());
    lv_obj_set_style_text_color(status_label, stats.online ? theme->ok : theme->red, 0);
  } else {
    lv_label_set_text(status_label, "WIFI LOST");
    lv_obj_set_style_text_color(status_label, theme->red, 0);
  }
}
