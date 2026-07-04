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
  lv_color_hex(0x02070b), lv_color_hex(0x07131d), lv_color_hex(0x071018),
  lv_color_hex(0x09141d), lv_color_hex(0x030a10), lv_color_hex(0x1c5062),
  lv_color_hex(0x12313d), lv_color_hex(0xf2fbff), lv_color_hex(0x8ab8c8),
  lv_color_hex(0x4f7a88), lv_color_hex(0x00e5ff), lv_color_hex(0xff3fb4),
  lv_color_hex(0x00ff8a), lv_color_hex(0xffd24a), lv_color_hex(0x00ff8a),
  lv_color_hex(0xffd24a), lv_color_hex(0x6c7cff), lv_color_hex(0xff3d62),
  lv_color_hex(0x49ff9a), lv_color_hex(0x10212c),
};

static const ThemePalette *theme = &NIGHT;
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
static lv_obj_t *clients_panel;
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
static lv_obj_t *clients_title_label;
static lv_obj_t *clients_value_label;
static lv_obj_t *clients_sub_label;
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

// Per-chart context for the draw callback
struct ChartCtx {
  const float *rx_hist;
  const float *tx_hist;
  lv_chart_series_t *rx_ser;
  lv_chart_series_t *tx_ser;
  float chart_max;
};
static ChartCtx eth0_ctx = {eth0_rx_window, eth0_tx_window, nullptr, nullptr, 1024.0f};
static ChartCtx eth1_ctx = {eth1_rx_window, eth1_tx_window, nullptr, nullptr, 1024.0f};

// Map speed (kbps) to a color: green(slow) → yellow(mid) → red(fast)
static lv_color_t speedColor(float kbps)
{
  if (kbps <= 0.0f) return lv_color_hex(0x00ff8a);
  if (kbps >= 5120.0f) return lv_color_hex(0xff3d62);

  uint8_t r, g, b;
  if (kbps < 1024.0f) {
    float t = kbps / 1024.0f;
    r = (uint8_t)(0x00 + t * (0xff - 0x00));
    g = (uint8_t)(0xff + t * (0xd2 - 0xff));
    b = (uint8_t)(0x8a + t * (0x4a - 0x8a));
  } else {
    float t = (kbps - 1024.0f) / (5120.0f - 1024.0f);
    if (t > 1.0f) t = 1.0f;
    r = 0xff;
    g = (uint8_t)(0xd2 + t * (0x3d - 0xd2));
    b = (uint8_t)(0x4a + t * (0x62 - 0x4a));
  }
  return lv_color_make(r, g, b);
}

// Custom draw callback: color each line segment by its speed value
static void chartDrawCb(lv_event_t *e)
{
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part != LV_PART_ITEMS) return;
  if (dsc->type != LV_CHART_DRAW_PART_LINE_AND_POINT) return;

  ChartCtx *ctx = (ChartCtx *)lv_event_get_user_data(e);
  if (!ctx || !dsc->sub_part_ptr) return;

  uint32_t idx = dsc->id;
  if (idx >= CHART_POINTS - 1) return;

  // Determine which series this segment belongs to
  const float *hist = nullptr;
  if (dsc->sub_part_ptr == ctx->rx_ser) {
    hist = ctx->rx_hist;
  } else if (dsc->sub_part_ptr == ctx->tx_ser) {
    hist = ctx->tx_hist;
  } else {
    return;
  }

  float avg = (hist[idx] + hist[idx + 1]) / 2.0f;
  dsc->line_dsc->color = speedColor(avg);
}

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
  float mbps = kbps / 1024.0f;
  if (mbps < 10.0f) return String(mbps, 2) + "M";
  if (mbps < 100.0f) return String(mbps, 1) + "M";
  return String(mbps, 0) + "M";
}

static String formatSpeedCompact(float kbps)
{
  float mbps = kbps / 1024.0f;
  if (mbps < 10.0f) return String(mbps, 2) + "M";
  return String(mbps, 1) + "M";
}

static void pushHistory(float *history, float value)
{
  if (value < 0.0f) value = 0.0f;
  for (uint16_t i = 0; i < CHART_POINTS - 1; i++) history[i] = history[i + 1];
  history[CHART_POINTS - 1] = value;
}

static float autoChartMax(const float *rx_history, const float *tx_history)
{
  float peak = 0.0f;
  for (uint16_t i = 0; i < CHART_POINTS; i++) {
    if (rx_history[i] > peak) peak = rx_history[i];
    if (tx_history[i] > peak) peak = tx_history[i];
  }
  if (peak < 1024.0f) return 1024;
  return peak;
}

static int chartValue(float kbps, float chart_max)
{
  if (kbps < 0) kbps = 0;
  if (chart_max < 1.0f) chart_max = 1.0f;
  int value = (int)((kbps * 1000.0f / chart_max) + 0.5f);
  if (value < 0) value = 0;
  if (value > 1000) value = 1000;
  return value;
}

static void updateChartScale(lv_obj_t *chart, lv_obj_t *scale_label, float chart_max)
{
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
  lv_label_set_text_fmt(scale_label, "%s", formatSpeedCompact(chart_max).c_str());
}

// Smooth a single value using Catmull-Rom-like weighted average of neighbors
static float smoothVal(const float *hist, uint16_t idx)
{
  if (idx == 0) return hist[0];
  if (idx >= CHART_POINTS - 1) return hist[CHART_POINTS - 1];
  // Weighted: 25% prev + 50% current + 25% next
  return hist[idx - 1] * 0.25f + hist[idx] * 0.5f + hist[idx + 1] * 0.25f;
}

static void redrawChart(lv_obj_t *chart, lv_chart_series_t *rx_series, lv_chart_series_t *tx_series,
                        const float *rx_history, const float *tx_history, float chart_max)
{
  for (uint16_t i = 0; i < CHART_POINTS; i++) {
    float rx_s = smoothVal(rx_history, i);
    float tx_s = smoothVal(tx_history, i);
    lv_chart_set_value_by_id(chart, rx_series, i, chartValue(rx_s, chart_max));
    lv_chart_set_value_by_id(chart, tx_series, i, chartValue(tx_s, chart_max));
  }
  lv_chart_refresh(chart);
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
                               lv_chart_series_t **rx_series, lv_chart_series_t **tx_series,
                               ChartCtx *ctx)
{
  lv_obj_t *obj = lv_chart_create(parent);
  lv_obj_set_size(obj, 320, 108);
  lv_chart_set_type(obj, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(obj, CHART_POINTS);
  lv_chart_set_range(obj, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
  lv_chart_set_update_mode(obj, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_div_line_count(obj, 3, 5);
  lv_obj_set_style_bg_color(obj, theme->chart_bg, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_line_color(obj, theme->grid, LV_PART_MAIN);
  lv_obj_set_style_line_width(obj, 1, LV_PART_MAIN);
  lv_obj_set_style_size(obj, 0, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(obj, 2, LV_PART_ITEMS);
  *rx_series = lv_chart_add_series(obj, rx_color, LV_CHART_AXIS_PRIMARY_Y);
  *tx_series = lv_chart_add_series(obj, tx_color, LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_all_value(obj, *rx_series, 0);
  lv_chart_set_all_value(obj, *tx_series, 0);

  // Store series pointers in ctx and register draw callback
  if (ctx) {
    ctx->rx_ser = *rx_series;
    ctx->tx_ser = *tx_series;
    lv_obj_add_event_cb(obj, chartDrawCb, LV_EVENT_DRAW_PART_BEGIN, ctx);
  }
  return obj;
}

static lv_obj_t *makeEthPanel(lv_obj_t *parent, const char *name, lv_color_t rx_color, lv_color_t tx_color,
                              lv_obj_t **rx_label, lv_obj_t **tx_label, lv_obj_t **sum_label,
                              lv_obj_t **scale_label,
                              lv_obj_t **chart_out, lv_chart_series_t **rx_series,
                              lv_chart_series_t **tx_series, ChartCtx *ctx)
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
  *rx_label = makeLabel(panel, "0.00M", rx_color, &lv_font_montserrat_28);
  lv_obj_set_width(*rx_label, 140);
  lv_label_set_long_mode(*rx_label, LV_LABEL_LONG_CLIP);
  lv_obj_align(*rx_label, LV_ALIGN_TOP_LEFT, 20, 50);

  lv_obj_t *tx_tag = makeLabel(panel, "TX", tx_color, &lv_font_montserrat_12);
  if (strcmp(name, "eth0") == 0) eth0_tx_tag = tx_tag;
  else eth1_tx_tag = tx_tag;
  lv_obj_align(tx_tag, LV_ALIGN_TOP_LEFT, 0, 98);
  *tx_label = makeLabel(panel, "0.00M", tx_color, &lv_font_montserrat_28);
  lv_obj_set_width(*tx_label, 140);
  lv_label_set_long_mode(*tx_label, LV_LABEL_LONG_CLIP);
  lv_obj_align(*tx_label, LV_ALIGN_TOP_LEFT, 20, 88);

  *chart_out = makeLineChart(panel, rx_color, tx_color, rx_series, tx_series, ctx);
  lv_obj_align(*chart_out, LV_ALIGN_RIGHT_MID, 0, 8);
  return panel;
}

static lv_obj_t *makeCoreBlock(lv_obj_t *parent, int idx)
{
  lv_obj_t *block = lv_obj_create(parent);
  lv_obj_set_size(block, 36, 88);
  lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(block, 0, 0);
  lv_obj_set_style_pad_all(block, 0, 0);

  core_label[idx] = makeLabel(block, "C0\n0%", theme->sub, &lv_font_montserrat_12);
  lv_obj_align(core_label[idx], LV_ALIGN_TOP_MID, 0, 0);

  core_bar[idx] = lv_bar_create(block);
  lv_obj_set_size(core_bar[idx], 12, 52);
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
                                      &eth0_chart, &eth0_rx_series, &eth0_tx_series, &eth0_ctx);
  lv_obj_align(eth0_panel, LV_ALIGN_TOP_MID, 0, 34);

  eth1_panel = makeEthPanel(main_root, "eth1", theme->eth1_rx, theme->eth1_tx,
                                      &eth1_rx_label, &eth1_tx_label, &eth1_sum_label, &eth1_scale_label,
                                      &eth1_chart, &eth1_rx_series, &eth1_tx_series, &eth1_ctx);
  lv_obj_align(eth1_panel, LV_ALIGN_TOP_MID, 0, 186);

  cpu_panel = lv_obj_create(main_root);
  lv_obj_set_size(cpu_panel, 168, 126);
  lv_obj_align(cpu_panel, LV_ALIGN_BOTTOM_LEFT, 18, -18);
  styleCard(cpu_panel, theme->panel_2);

  cpu_title_label = makeLabel(cpu_panel, "CPU", theme->text, &lv_font_montserrat_16);
  lv_obj_align(cpu_title_label, LV_ALIGN_TOP_LEFT, 0, -1);
  temp_label = makeLabel(cpu_panel, "--C", theme->amber, &lv_font_montserrat_16);
  lv_obj_align(temp_label, LV_ALIGN_TOP_RIGHT, 0, -1);

  lv_obj_t *core_row = lv_obj_create(cpu_panel);
  lv_obj_set_size(core_row, 154, 88);
  lv_obj_align(core_row, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_clear_flag(core_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(core_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(core_row, 0, 0);
  lv_obj_set_style_pad_all(core_row, 0, 0);
  lv_obj_set_style_pad_column(core_row, 3, 0);
  lv_obj_set_flex_flow(core_row, LV_FLEX_FLOW_ROW);
  for (int i = 0; i < 4; i++) makeCoreBlock(core_row, i);

  clients_panel = lv_obj_create(main_root);
  lv_obj_set_size(clients_panel, 116, 126);
  lv_obj_align_to(clients_panel, cpu_panel, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  styleCard(clients_panel, theme->panel_2);

  clients_title_label = makeLabel(clients_panel, "CLIENTS", theme->text, &lv_font_montserrat_14);
  lv_obj_align(clients_title_label, LV_ALIGN_TOP_LEFT, 0, -1);
  clients_value_label = makeLabel(clients_panel, "--", theme->blue, &lv_font_montserrat_40);
  lv_obj_set_width(clients_value_label, 98);
  lv_label_set_long_mode(clients_value_label, LV_LABEL_LONG_CLIP);
  lv_obj_align(clients_value_label, LV_ALIGN_CENTER, 0, -4);
  clients_sub_label = makeLabel(clients_panel, "active", theme->sub, &lv_font_montserrat_14);
  lv_obj_align(clients_sub_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  sys_panel = lv_obj_create(main_root);
  lv_obj_set_size(sys_panel, 148, 126);
  lv_obj_align(sys_panel, LV_ALIGN_BOTTOM_RIGHT, -18, -18);
  styleCard(sys_panel, theme->panel_2);

  sys_title_label = makeLabel(sys_panel, "SYSTEM", theme->text, &lv_font_montserrat_16);
  lv_obj_align(sys_title_label, LV_ALIGN_TOP_LEFT, 0, -1);

  ram_label = makeLabel(sys_panel, "RAM 0%", theme->mint, &lv_font_montserrat_22);
  lv_obj_align(ram_label, LV_ALIGN_TOP_LEFT, 0, 24);
  ram_bar = lv_bar_create(sys_panel);
  lv_obj_set_size(ram_bar, 124, 9);
  lv_obj_align(ram_bar, LV_ALIGN_TOP_LEFT, 0, 54);
  lv_bar_set_range(ram_bar, 0, 100);
  lv_obj_set_style_bg_color(ram_bar, theme->bar_bg, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ram_bar, theme->mint, LV_PART_INDICATOR);
  lv_obj_set_style_radius(ram_bar, 5, LV_PART_MAIN);
  lv_obj_set_style_radius(ram_bar, 5, LV_PART_INDICATOR);

  sys_line_1 = makeLabel(sys_panel, "conn --", theme->sub, &lv_font_montserrat_16);
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
    lv_label_set_text_fmt(connect_wifi_label, "WiFi: %s   CH %d   %d dBm",
                          wifiStatusText(wifi_status), WiFi.channel(), WiFi.RSSI());
    lv_label_set_text_fmt(connect_ip_label, "IP: %s   MAC %s",
                          WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());
    lv_label_set_text_fmt(connect_route_label, "Gateway: %s   BSSID %s",
                          WiFi.gatewayIP().toString().c_str(), WiFi.BSSIDstr().c_str());
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
    lv_label_set_text_fmt(connect_error_label, "%s", stats.error.c_str());
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
  setMainVisible(stats.online || stats.last_ok_ms != 0);

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

  float eth0_max = autoChartMax(eth0_rx_window, eth0_tx_window);
  float eth1_max = autoChartMax(eth1_rx_window, eth1_tx_window);
  updateChartScale(eth0_chart, eth0_scale_label, eth0_max);
  updateChartScale(eth1_chart, eth1_scale_label, eth1_max);
  eth0_ctx.chart_max = eth0_max;
  eth1_ctx.chart_max = eth1_max;
  redrawChart(eth0_chart, eth0_rx_series, eth0_tx_series, eth0_rx_window, eth0_tx_window, eth0_max);
  redrawChart(eth1_chart, eth1_rx_series, eth1_tx_series, eth1_rx_window, eth1_tx_window, eth1_max);

  // Dynamic color: faster speed → redder (labels only, chart lines stay fixed)
  lv_obj_set_style_text_color(eth0_rx_label, speedColor(stats.eth0_rx), 0);
  lv_obj_set_style_text_color(eth0_tx_label, speedColor(stats.eth0_tx), 0);
  lv_obj_set_style_text_color(eth1_rx_label, speedColor(stats.eth1_rx), 0);
  lv_obj_set_style_text_color(eth1_tx_label, speedColor(stats.eth1_tx), 0);

  for (int i = 0; i < 4; i++) {
    lv_label_set_text_fmt(core_label[i], "C%d\n%d%%", i, stats.cores[i]);
    lv_obj_set_style_text_color(core_label[i], stats.cores[i] > 80 ? theme->red : theme->sub, 0);
    lv_bar_set_value(core_bar[i], stats.cores[i], LV_ANIM_OFF);
  }

  lv_label_set_text_fmt(temp_label, "%dC", stats.temp);
  lv_label_set_text_fmt(clients_value_label, "%d", stats.clients);
  lv_label_set_text_fmt(clients_sub_label, "conn %d", stats.conn);
  lv_label_set_text_fmt(ram_label, "RAM %d%%", stats.mem);
  lv_bar_set_value(ram_bar, stats.mem, LV_ANIM_OFF);
  // lv_label_set_text_fmt(sys_line_1, "conn %d", stats.conn);
  lv_label_set_text_fmt(sys_line_2, "T %s", stats.uptime.substring(0, 11).c_str());

  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text(status_label, stats.online ? "ONLINE" : stats.error.c_str());
    lv_obj_set_style_text_color(status_label, stats.online ? theme->ok : theme->red, 0);
  } else {
    lv_label_set_text(status_label, "WIFI LOST");
    lv_obj_set_style_text_color(status_label, theme->red, 0);
  }
}
