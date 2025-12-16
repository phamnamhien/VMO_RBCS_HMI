  #include "ui_support.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "UI_SUPPORT";
static SemaphoreHandle_t lvgl_mux = NULL;

// ============================================
// Initialization
// ============================================

bool ui_support_init(void *lvgl_mutex)
{
    if (lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "LVGL mutex is NULL");
        return false;
    }
    
    lvgl_mux = (SemaphoreHandle_t)lvgl_mutex;
    ESP_LOGI(TAG, "UI Support initialized");
    return true;
}

// ============================================
// LVGL Thread Safety
// ============================================

bool ui_lock(int timeout_ms)
{
    if (lvgl_mux == NULL) {
        ESP_LOGE(TAG, "UI Support not initialized! Call ui_support_init() first");
        return false;
    }
    
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void ui_unlock(void)
{
    if (lvgl_mux == NULL) {
        ESP_LOGE(TAG, "UI Support not initialized!");
        return;
    }
    
    xSemaphoreGiveRecursive(lvgl_mux);
}

// ============================================
// Screen Management
// ============================================

// ui_support.c - Sửa hàm ui_load_screen
bool ui_load_screen(lv_obj_t *screen)
{
    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);  // ← Tắt animation
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_load_screen_fade(lv_obj_t *screen, uint32_t fade_time, uint32_t delay)
{
    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, fade_time, delay, false);
        ui_unlock();
        return true;
    }
    
    return false;
}
bool ui_load_screen_slide(lv_obj_t *screen, lv_scr_load_anim_t anim_type, 
                          uint32_t time, uint32_t delay)
{
    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_scr_load_anim(screen, anim_type, time, delay, false);
        ui_unlock();
        return true;
    }
    
    return false;
}
lv_obj_t* ui_get_current_screen(void)
{
    lv_obj_t *screen = NULL;
    
    if (ui_lock(-1)) {
        screen = lv_scr_act();
        ui_unlock();
    }
    
    return screen;
}

// ============================================
// UI Update Helpers
// ============================================

bool ui_label_set_text(lv_obj_t *label, const char *text)
{
    if (label == NULL || text == NULL) {
        ESP_LOGE(TAG, "Label or text is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_label_set_text(label, text);
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_label_set_text_fmt(lv_obj_t *label, const char *fmt, ...)
{
    if (label == NULL || fmt == NULL) {
        ESP_LOGE(TAG, "Label or format is NULL");
        return false;
    }
    
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (ui_lock(-1)) {
        lv_label_set_text(label, buffer);
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_image_set_src(lv_obj_t *img, const void *src)
{
    if (img == NULL || src == NULL) {
        ESP_LOGE(TAG, "Image or source is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_img_set_src(img, src);
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_arc_set_value(lv_obj_t *arc, int16_t value)
{
    if (arc == NULL) {
        ESP_LOGE(TAG, "Arc is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_arc_set_value(arc, value);
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_bar_set_value(lv_obj_t *bar, int32_t value, bool anim)
{
    if (bar == NULL) {
        ESP_LOGE(TAG, "Bar is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_bar_set_value(bar, value, anim ? LV_ANIM_ON : LV_ANIM_OFF);
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_slider_set_value(lv_obj_t *slider, int32_t value, bool anim)
{
    if (slider == NULL) {
        ESP_LOGE(TAG, "Slider is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        lv_slider_set_value(slider, value, anim ? LV_ANIM_ON : LV_ANIM_OFF);
        ui_unlock();
        return true;
    }
    
    return false;
}

// ============================================
// Object Visibility
// ============================================

bool ui_object_set_visible(lv_obj_t *obj, bool visible)
{
    if (obj == NULL) {
        ESP_LOGE(TAG, "Object is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        if (visible) {
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        ui_unlock();
        return true;
    }
    
    return false;
}

bool ui_object_set_enabled(lv_obj_t *obj, bool enabled)
{
    if (obj == NULL) {
        ESP_LOGE(TAG, "Object is NULL");
        return false;
    }
    
    if (ui_lock(-1)) {
        if (enabled) {
            lv_obj_clear_state(obj, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(obj, LV_STATE_DISABLED);
        }
        ui_unlock();
        return true;
    }
    
    return false;
}

// ============================================
// Deferred Callback (Advanced)
// ============================================

typedef struct {
    ui_callback_t callback;
    void *user_data;
} ui_callback_data_t;

static void ui_timer_callback_wrapper(lv_timer_t *timer)
{
    ui_callback_data_t *data = (ui_callback_data_t *)timer->user_data;
    
    if (data && data->callback) {
        data->callback(data->user_data);
    }
    
    // One-shot timer - delete after execution
    lv_timer_del(timer);
    free(data);
}

bool ui_execute_callback(ui_callback_t callback, void *user_data)
{
    if (callback == NULL) {
        ESP_LOGE(TAG, "Callback is NULL");
        return false;
    }
    
    ui_callback_data_t *data = malloc(sizeof(ui_callback_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate callback data");
        return false;
    }
    
    data->callback = callback;
    data->user_data = user_data;
    
    if (ui_lock(-1)) {
        lv_timer_t *timer = lv_timer_create(ui_timer_callback_wrapper, 10, data);
        lv_timer_set_repeat_count(timer, 1);
        ui_unlock();
        return true;
    }
    
    free(data);
    return false;
}

void ui_set_button_color(lv_obj_t *button, uint32_t color)
{
    if (button == NULL) {
        ESP_LOGE(TAG, "Button is NULL");
        return;
    }
    
    if (ui_lock(-1)) {
        lv_obj_set_style_bg_color(button, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(button, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_unlock();
    }
}

