/**
 * @file ui_support.h
 * @brief UI Support Library for LVGL operations
 * 
 * Provides thread-safe UI operations and helper functions for
 * SquareLine Studio UI integration
 */

#ifndef UI_SUPPORT_H
#define UI_SUPPORT_H

#include <stdbool.h>
#include "lvgl.h"
#include "ui_support.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>

#include "ui.h"


#ifdef __cplusplus
extern "C" {
#endif

// LCD Configuration
#define LCD_NUM_FB                  1  // Double buffer for maximum speed
#define LCD_H_RES                   800
#define LCD_V_RES                   480

// LVGL Task Configuration
#define LCD_LVGL_TICK_PERIOD_MS     1
#define LCD_LVGL_TASK_MAX_DELAY_MS  500
#define LCD_LVGL_TASK_MIN_DELAY_MS  1
#define LCD_LVGL_TASK_STACK_SIZE    (4 * 1024)
#define LCD_LVGL_TASK_PRIORITY      2

#define BTN_COLOR_NORMAL 0xECECEC
#define BTN_COLOR_ACTIVE 0xD5FFCD
// ============================================
// LVGL Thread Safety
// ============================================

/**
 * @brief Lock LVGL mutex for thread-safe operations
 * 
 * Must be called before any LVGL API calls from tasks/ISR.
 * Always pair with ui_unlock()
 * 
 * @param timeout_ms Timeout in milliseconds (-1 for infinite wait)
 * @return true if lock acquired, false on timeout
 * 
 * @example
 * if (ui_lock(-1)) {
 *     lv_label_set_text(label, "Hello");
 *     ui_unlock();
 * }
 */
bool ui_lock(int timeout_ms);

/**
 * @brief Unlock LVGL mutex
 * 
 * Must be called after ui_lock() to release the mutex
 */
void ui_unlock(void);

// ============================================
// Screen Management
// ============================================

/**
 * @brief Load screen with thread-safe wrapper
 * 
 * @param screen Pointer to LVGL screen object
 * @return true if successful, false otherwise
 * 
 * @example
 * ui_load_screen(ui_Screen1);
 */
bool ui_load_screen(lv_obj_t *screen);

/**
 * @brief Load screen with fade animation
 * 
 * @param screen Pointer to LVGL screen object
 * @param fade_time Animation duration in milliseconds
 * @param delay Delay before animation starts (ms)
 * @return true if successful, false otherwise
 */
bool ui_load_screen_fade(lv_obj_t *screen, uint32_t fade_time, uint32_t delay);

/**
 * @brief Load screen with slide animation
 */
bool ui_load_screen_slide(lv_obj_t *screen, lv_scr_load_anim_t anim_type, 
                          uint32_t time, uint32_t delay);
                          
/**
 * @brief Get current active screen
 * 
 * @return Pointer to current screen, NULL if error
 */
lv_obj_t* ui_get_current_screen(void);

// ============================================
// UI Update Helpers
// ============================================

/**
 * @brief Update label text safely
 * 
 * @param label Pointer to label object
 * @param text New text string
 * @return true if successful, false otherwise
 */
bool ui_label_set_text(lv_obj_t *label, const char *text);

/**
 * @brief Update label with formatted text
 * 
 * @param label Pointer to label object
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 * @return true if successful, false otherwise
 * 
 * @example
 * ui_label_set_text_fmt(ui_Label1, "Temp: %d°C", temperature);
 */
bool ui_label_set_text_fmt(lv_obj_t *label, const char *fmt, ...);

/**
 * @brief Update image source safely
 * 
 * @param img Pointer to image object
 * @param src Image source (pointer or path)
 * @return true if successful, false otherwise
 */
bool ui_image_set_src(lv_obj_t *img, const void *src);

/**
 * @brief Update arc value safely
 * 
 * @param arc Pointer to arc object
 * @param value New value
 * @return true if successful, false otherwise
 */
bool ui_arc_set_value(lv_obj_t *arc, int16_t value);

/**
 * @brief Update bar value safely
 * 
 * @param bar Pointer to bar object
 * @param value New value
 * @param anim Enable animation (true/false)
 * @return true if successful, false otherwise
 */
bool ui_bar_set_value(lv_obj_t *bar, int32_t value, bool anim);

/**
 * @brief Update slider value safely
 * 
 * @param slider Pointer to slider object
 * @param value New value
 * @param anim Enable animation (true/false)
 * @return true if successful, false otherwise
 */
bool ui_slider_set_value(lv_obj_t *slider, int32_t value, bool anim);

// ============================================
// Object Visibility
// ============================================

/**
 * @brief Show/hide object safely
 * 
 * @param obj Pointer to object
 * @param visible true to show, false to hide
 * @return true if successful, false otherwise
 */
bool ui_object_set_visible(lv_obj_t *obj, bool visible);

/**
 * @brief Enable/disable object safely
 * 
 * @param obj Pointer to object
 * @param enabled true to enable, false to disable
 * @return true if successful, false otherwise
 */
bool ui_object_set_enabled(lv_obj_t *obj, bool enabled);

// ============================================
// Event Callbacks (Run in LVGL task)
// ============================================

/**
 * @brief Callback function type for deferred UI operations
 * 
 * @param user_data User-provided data pointer
 */
typedef void (*ui_callback_t)(void *user_data);

/**
 * @brief Execute callback in LVGL task context
 * 
 * Safe way to update UI from other tasks. The callback will be
 * executed in the LVGL task with proper locking.
 * 
 * @param callback Function to execute
 * @param user_data Data to pass to callback
 * @return true if queued successfully, false otherwise
 * 
 * @example
 * void update_ui(void *data) {
 *     int temp = *(int*)data;
 *     lv_label_set_text_fmt(ui_Label1, "Temp: %d", temp);
 * }
 * 
 * int temperature = 25;
 * ui_execute_callback(update_ui, &temperature);
 */
bool ui_execute_callback(ui_callback_t callback, void *user_data);

// ============================================
// Initialization
// ============================================

/**
 * @brief Initialize UI support library
 * 
 * Must be called ONCE after LVGL initialization and before
 * creating LVGL task. Usually called from main.c
 * 
 * @param lvgl_mutex Pointer to LVGL mutex (SemaphoreHandle_t)
 * @return true if successful, false otherwise
 */
bool ui_support_init(void *lvgl_mutex);

/**
 * @brief Set background color cho button bất kỳ
 * 
 * @param button Pointer đến button object (ui_btMainSlot1 -> ui_btMainSlot5)
 * @param color Mã màu HEX (ví dụ: 0xFF0000 cho đỏ)
 */
void ui_set_button_color(lv_obj_t *button, uint32_t color);

/**
 * @brief Hiển thị slot serial detail
 * 
 * @param slot_number 0 = tắt tất cả, 1-5 = hiển thị slot tương ứng (ẩn các slot khác)
 */


#ifdef __cplusplus
}
#endif

#endif // UI_SUPPORT_H