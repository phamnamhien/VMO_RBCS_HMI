#include "app_states.h"

static const char* TAG = "HSM";

static hsm_event_t app_state_loading_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_common_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_slot_1_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_slot_2_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_slot_3_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_slot_4_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_slot_5_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data);


static void esp32_timer_callback(TimerHandle_t xTimer);
static void* esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat);
static void esp32_timer_stop(void* timer_handle);
static uint32_t esp32_timer_get_ms(void);

/* States */
static hsm_state_t app_state_loading;
static hsm_state_t app_state_main_common;
static hsm_state_t app_state_main;
static hsm_state_t app_state_main_slot_1;
static hsm_state_t app_state_main_slot_2;
static hsm_state_t app_state_main_slot_3;
static hsm_state_t app_state_main_slot_4;
static hsm_state_t app_state_main_slot_5;
static hsm_state_t app_state_setting;

/* Timers */
static hsm_timer_t *timer_loading;

void
app_state_hsm_init(app_state_hsm_t* me) {
    /* Create states */
    hsm_state_create(&app_state_loading, "s_loading", app_state_loading_handler, NULL);
    hsm_state_create(&app_state_main_common, "s_main_com", app_state_main_common_handler, NULL);
    hsm_state_create(&app_state_main, "s_main", app_state_main_handler, &app_state_main_common);
    hsm_state_create(&app_state_main_slot_1, "s_main_sl1", app_state_main_slot_1_handler, &app_state_main_common);
    hsm_state_create(&app_state_main_slot_2, "s_main_sl2", app_state_main_slot_2_handler, &app_state_main_common);
    hsm_state_create(&app_state_main_slot_3, "s_main_sl3", app_state_main_slot_3_handler, &app_state_main_common);
    hsm_state_create(&app_state_main_slot_4, "s_main_sl4", app_state_main_slot_4_handler, &app_state_main_common);
    hsm_state_create(&app_state_main_slot_5, "s_main_sl5", app_state_main_slot_5_handler, &app_state_main_common);
    hsm_state_create(&app_state_setting, "s_setting", app_state_setting_handler, NULL);

    static const hsm_timer_if_t esp32_timer_if = {
        .start = esp32_timer_start,
        .stop = esp32_timer_stop,
        .get_ms = esp32_timer_get_ms
    };

    /* Init HSM */
    hsm_init((hsm_t *)me, "app", &app_state_loading, &esp32_timer_if);
}

static hsm_event_t
app_state_loading_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    static uint8_t loading_count = 0;
    
    switch (event) {
        case HSM_EVENT_ENTRY: 
            loading_count = 0; 
            hsm_timer_create(&timer_loading, hsm, HEVT_TIMER_LOADING, LOADING_1PERCENT_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_loading);
            ESP_LOGI(TAG, "Loading: ENTRY");
            break;
            
        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "Loading: EXIT");
            loading_count = 0;
            break;
            
        case HEVT_TIMER_LOADING:
            // if (timer_loading == NULL) {
            //     ESP_LOGW(TAG, "Timer callback after delete - IGNORED");
            //     return HSM_EVENT_NONE;
            // }
            // if (!hsm_is_in_state(hsm, &app_state_loading)) {
            //     ESP_LOGW(TAG, "Timer fired in wrong state - IGNORED");
            //     return HSM_EVENT_NONE;
            // }
            loading_count += 4;
            if (loading_count > 100) {
                ESP_LOGI(TAG, "Loading Done -> Main State");
                hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            } else {
                if (ui_lock(-1)) {
                    lv_bar_set_value(ui_barSplashLoading, loading_count, LV_ANIM_OFF);
                    ui_unlock();
                }
            }
            return HSM_EVENT_NONE;  // ← HANDLED
            
        default: 
            return event;
    }
    return HSM_EVENT_NONE;
}

static hsm_event_t
app_state_main_common_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: break;
        case HSM_EVENT_EXIT: break;
        case HEVT_MODBUS_GET_STATION_STATE_DATA:
            // Handle station state data update if needed
            break;
        case HEVT_MODBUS_GET_SLOT_DATA: 
            ui_update_all_slots_display(me); 
            break;
        case HEVT_CHANGE_SCR_MAIN_TO_SETTING: 
            hsm_transition((hsm_t *)me, &app_state_setting, NULL, NULL); 
            break;
        case HEVT_MODBUS_CONNECTED: 
            ui_show_main_not_connect(false); 
            break;
        case HEVT_MODBUS_NOTCONNECTED: 
            ui_show_main_not_connect(true); 
            break;
        default: 
            return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            ui_load_screen(ui_scrMain);
            ui_show_slot_serial_detail(0);
            ui_show_slot_detail_panel(false);
            ESP_LOGI(TAG, "Entered Main State");
            break;
        case HSM_EVENT_EXIT: break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_1, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_2, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_3, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_4, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_5, NULL, NULL); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_slot_1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            // ui_set_button_color(ui_btMainSlot1, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(1);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 1 Clicked");
            break;
        case HSM_EVENT_EXIT:
            // ui_set_button_color(ui_btMainSlot1, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            //ui_show_slot_detail_panel(false);
            break;
        case HEVT_MODBUS_GET_SLOT_1_DATA: ui_update_all_slot_details(me, IDX_SLOT_1); break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_2, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_3, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_4, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_5, NULL, NULL); break;
        case HEVT_MAIN_MANUAL_SWAP_CLICKED: modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1000, 1); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_slot_2_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            // ui_set_button_color(ui_btMainSlot2, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(2);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 2 Clicked");
            break;
        case HSM_EVENT_EXIT:
            // ui_set_button_color(ui_btMainSlot2, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HEVT_MODBUS_GET_SLOT_2_DATA: ui_update_all_slot_details(me, IDX_SLOT_2); break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_1, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_3, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_4, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_5, NULL, NULL); break;
        case HEVT_MAIN_MANUAL_SWAP_CLICKED: modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1001, 1); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_slot_3_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            // ui_set_button_color(ui_btMainSlot3, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(3);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 3 Clicked");
            break;
        case HSM_EVENT_EXIT:
            // ui_set_button_color(ui_btMainSlot3, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HEVT_MODBUS_GET_SLOT_3_DATA: ui_update_all_slot_details(me, IDX_SLOT_3); break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_1, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_2, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_4, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_5, NULL, NULL); break;
        case HEVT_MAIN_MANUAL_SWAP_CLICKED: modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1002, 1); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_slot_4_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            // ui_set_button_color(ui_btMainSlot4, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(4);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 4 Clicked");
            break;
        case HSM_EVENT_EXIT:
            // ui_set_button_color(ui_btMainSlot4, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HEVT_MODBUS_GET_SLOT_4_DATA: ui_update_all_slot_details(me, IDX_SLOT_4); break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_1, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_2, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_3, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_5, NULL, NULL); break;
        case HEVT_MAIN_MANUAL_SWAP_CLICKED: modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1003, 1); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_main_slot_5_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            // ui_set_button_color(ui_btMainSlot5, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(5);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 5 Clicked");
            break;
        case HSM_EVENT_EXIT:
            // ui_set_button_color(ui_btMainSlot5, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HEVT_MODBUS_GET_SLOT_5_DATA: ui_update_all_slot_details(me, IDX_SLOT_5); break;
        case HEVT_MAIN_SLOT_1_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_1, NULL, NULL); break;
        case HEVT_MAIN_SLOT_2_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_2, NULL, NULL); break;
        case HEVT_MAIN_SLOT_3_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_3, NULL, NULL); break;
        case HEVT_MAIN_SLOT_4_CLICKED: hsm_transition((hsm_t *)me, &app_state_main_slot_4, NULL, NULL); break;
        case HEVT_MAIN_SLOT_5_CLICKED: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        case HEVT_MAIN_MANUAL_SWAP_CLICKED: modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1004, 1); break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            ui_load_screen(ui_scrSetting);
            ui_show_slot_detail_panel(false);
            ESP_LOGI(TAG, "Entered Setting State");
            break;
        case HSM_EVENT_EXIT: break;
        case HEVT_CHANGE_SCR_SETTING_TO_MAIN: hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL); break;
        default: return event;
    }
    return 0;
}


static void 
esp32_timer_callback(TimerHandle_t xTimer) {
    esp32_timer_t* timer = (esp32_timer_t*)pvTimerGetTimerID(xTimer);
    if (timer == NULL) return;
    
    // ✅ KHÔNG take mutex - chỉ check state
    if (timer->state != TIMER_STATE_ACTIVE) {
        return;
    }
    
    // ✅ GỌI CALLBACK TRỰC TIẾP (không qua mutex)
    if (timer->callback) {
        timer->callback(timer->arg);
    }
}

static void* 
esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    esp32_timer_t* timer = malloc(sizeof(esp32_timer_t));
    if (!timer) return NULL;
    timer->mutex = xSemaphoreCreateMutex();
    if (!timer->mutex) {
        free(timer);
        return NULL;
    }
    timer->callback = callback;
    timer->arg = arg;
    timer->state = TIMER_STATE_ACTIVE;
    
    timer->handle = xTimerCreate("hsm", pdMS_TO_TICKS(period_ms),
                                  repeat ? pdTRUE : pdFALSE, 
                                  timer, esp32_timer_callback);

    if (!timer->handle || xTimerStart(timer->handle, 0) != pdPASS) {
        if (timer->handle) xTimerDelete(timer->handle, 0);
        vSemaphoreDelete(timer->mutex);
        free(timer);
        return NULL;
    }
    
    ESP_LOGD(TAG, "Timer started: %p", timer);
    return timer;
}

static void 
esp32_timer_stop(void* timer_handle) {
    esp32_timer_t* timer = (esp32_timer_t*)timer_handle;
    if (timer == NULL) return;
    
    ESP_LOGD(TAG, "Timer stopping: %p", timer);
    
    // ✅ ĐÁNH DẤU DELETING TRƯỚC
    timer->state = TIMER_STATE_DELETING;
    timer->callback = NULL;  // ✅ NULL ngay để callback không chạy
    
    // ✅ STOP timer
    if (timer->handle) {
        xTimerStop(timer->handle, pdMS_TO_TICKS(100));
    }
    
    // ✅ ĐỢI callback chạy xong (nếu đang chạy)
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // ✅ XÓA TIMER
    if (timer->handle) {
        xTimerDelete(timer->handle, pdMS_TO_TICKS(100));
        timer->handle = NULL;
    }
    
    // ✅ XÓA MUTEX và FREE
    if (timer->mutex) {
        vSemaphoreDelete(timer->mutex);
    }
    free(timer);
    
    ESP_LOGD(TAG, "Timer deleted: %p", timer);
}

static uint32_t 
esp32_timer_get_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}