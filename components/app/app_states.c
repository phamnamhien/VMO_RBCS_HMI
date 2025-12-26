#include "app_states.h"

static const char* TAG = "HSM";

static hsm_event_t app_state_loading_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_common_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_detail_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_manual1_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_manual2_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_process_handler(hsm_t* hsm, hsm_event_t event, void* data);

static hsm_event_t app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data);


static void esp32_timer_callback(TimerHandle_t xTimer);
static void* esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat);
static void esp32_timer_stop(void* timer_handle);
static uint32_t esp32_timer_get_ms(void);

/* States */
static hsm_state_t app_state_loading;
static hsm_state_t app_state_main_common;
static hsm_state_t app_state_main;
static hsm_state_t app_state_detail;
static hsm_state_t app_state_manual1;
static hsm_state_t app_state_manual2;
static hsm_state_t app_state_process;

static hsm_state_t app_state_setting;

/* Timers */
static hsm_timer_t *timer_loading;
static hsm_timer_t *timer_update;
static hsm_timer_t *timer_clock;

void
app_state_hsm_init(app_state_hsm_t* me) {
    /* Create states */
    hsm_state_create(&app_state_loading, "s_loading", app_state_loading_handler, NULL);
    hsm_state_create(&app_state_main_common, "s_main_com", app_state_main_common_handler, NULL);
    hsm_state_create(&app_state_main, "s_main", app_state_main_handler, &app_state_main_common);
    hsm_state_create(&app_state_detail, "s_detail", app_state_detail_handler, &app_state_main_common);
    hsm_state_create(&app_state_manual1, "s_manual1", app_state_manual1_handler, &app_state_main_common);
    hsm_state_create(&app_state_manual2, "s_manual2", app_state_manual2_handler, &app_state_main_common);
    hsm_state_create(&app_state_process, "s_process", app_state_process_handler, &app_state_main_common);
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
            loading_count += 4;
            if (loading_count > 100) {
                ESP_LOGI(TAG, "Loading Done -> Main State");
                hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            } else {
                if (ui_lock(-1)) {
                    lv_bar_set_value(ui_scrsplashloadingbar, loading_count, LV_ANIM_OFF);
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
    // app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: break;
        case HSM_EVENT_EXIT: break;
        case HEVT_MODBUS_GET_STATION_STATE_DATA:
            // Handle station state data update if needed
            break;
        case HEVT_MODBUS_GET_SLOT_DATA: 

            break;
        case HEVT_MODBUS_CONNECTED: 

            break;
        case HEVT_MODBUS_NOTCONNECTED: 

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
            hsm_timer_create(&timer_update, hsm, HEVT_TIMER_UPDATE, UPDATE_SCREEN_VALUE_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_update);
            me->last_time_run = me->time_run;
            me->time_run = 0;
            ESP_LOGI(TAG, "Entered Main State");
            break;
        case HSM_EVENT_EXIT: 

            break;
        case HEVT_TIMER_UPDATE:
            bool slots[5] = {
                me->bms_info.slot_state[0], 
                me->bms_info.slot_state[1], 
                me->bms_info.slot_state[2], 
                me->bms_info.slot_state[3], 
                me->bms_info.slot_state[4]
            };
            float voltages[5] = {
                me->bms_data[0].stack_volt/10.0, 
                me->bms_data[1].stack_volt/10.0, 
                me->bms_data[2].stack_volt/10.0, 
                me->bms_data[3].stack_volt/10.0, 
                me->bms_data[4].stack_volt/10.0
            };
            float percents[5] = {
                me->bms_data[0].pin_percent/10.0, 
                me->bms_data[1].pin_percent/10.0, 
                me->bms_data[2].pin_percent/10.0, 
                me->bms_data[3].pin_percent/10.0, 
                me->bms_data[4].pin_percent/10.0
            };
            scrmainbatslotscontainer_update(slots, voltages, percents);
            scrmainlasttimelabel_update(me->last_time_run);
            scrmainstateofchargervalue_update(me->bms_info.swap_state);
            break;
        case HEVT_TRANS_MAIN_TO_DETAIL:
            hsm_transition((hsm_t *)me, &app_state_detail, NULL, NULL);
            break;
        case HEVT_TRANS_MAIN_TO_MANUAL1:
            hsm_transition((hsm_t *)me, &app_state_manual1, NULL, NULL);
            break;
        default: 
            return event;
    }
    return 0;
}
static hsm_event_t app_state_detail_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    
    switch (event) {
        case HSM_EVENT_ENTRY:
            hsm_timer_create(&timer_update, hsm, HEVT_TIMER_UPDATE, UPDATE_SCREEN_VALUE_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_update);
            ESP_LOGI(TAG, "Entered Detail State");
            break;
        case HSM_EVENT_EXIT: 

            break;
        case HEVT_TIMER_UPDATE:
            scrdetaildataslottitlelabel_update(me->present_slot_display);
            scrdetaildataslotvalue_update(
                        &me->bms_data[me->present_slot_display],
                        me->bms_info.slot_state[me->present_slot_display]);
            scrdetailslotssttcontainer_update(
                        me->bms_info.slot_state,
                        me->bms_data,
                        me->present_slot_display);
            break;
        default: return event;
    }
    return 0;
}

static hsm_event_t
app_state_manual1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: 
            break;
        case HSM_EVENT_EXIT: 
            break;
        case HEVT_MANUAL1_SELECT_BAT1:
            me->manual_robot_bat_select = 1;
            hsm_transition((hsm_t *)me, &app_state_manual2, NULL, NULL);
            break;
        case HEVT_MANUAL1_SELECT_BAT2:
            me->manual_robot_bat_select = 2;
            hsm_transition((hsm_t *)me, &app_state_manual2, NULL, NULL);
            break;    
        default: 
            return event;
    }
    return 0;
}

static hsm_event_t
app_state_manual2_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: 
            break;
        case HSM_EVENT_EXIT: 
            break;
        case HEVT_MANUAL2_SELECT_SLOT1:
            me->bms_info.manual_swap_request = (me->manual_robot_bat_select - 1)*5 + 1;
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_MANUAL_CONTROL_REG, 
                        me->bms_info.manual_swap_request);
            me->manual_robot_bat_select = 0;
            hsm_transition((hsm_t *)me, &app_state_process, NULL, NULL);
            break;
        case HEVT_MANUAL2_SELECT_SLOT2:
            me->bms_info.manual_swap_request = (me->manual_robot_bat_select - 1)*5 + 2;
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_MANUAL_CONTROL_REG, 
                        me->bms_info.manual_swap_request);
            me->manual_robot_bat_select = 0;
            hsm_transition((hsm_t *)me, &app_state_process, NULL, NULL);
            break;
        case HEVT_MANUAL2_SELECT_SLOT3:
            me->bms_info.manual_swap_request = (me->manual_robot_bat_select - 1)*5 + 3;
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_MANUAL_CONTROL_REG, 
                        me->bms_info.manual_swap_request);
            me->manual_robot_bat_select = 0;
            hsm_transition((hsm_t *)me, &app_state_process, NULL, NULL);
            break;
        case HEVT_MANUAL2_SELECT_SLOT4:
            me->bms_info.manual_swap_request = (me->manual_robot_bat_select - 1)*5 + 4;
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_MANUAL_CONTROL_REG, 
                        me->bms_info.manual_swap_request);
            me->manual_robot_bat_select = 0;
            hsm_transition((hsm_t *)me, &app_state_process, NULL, NULL);
            break;
        case HEVT_MANUAL2_SELECT_SLOT5: 
            me->bms_info.manual_swap_request = (me->manual_robot_bat_select - 1)*5 + 5;
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_MANUAL_CONTROL_REG, 
                        me->bms_info.manual_swap_request);
            me->manual_robot_bat_select = 0;
            hsm_transition((hsm_t *)me, &app_state_process, NULL, NULL);
            break;
        default: 
            return event;
    }
    return 0;
}

static hsm_event_t app_state_process_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: 
            hsm_timer_create(&timer_update, hsm, HEVT_TIMER_UPDATE, UPDATE_SCREEN_VALUE_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_update);
            hsm_timer_create(&timer_clock, hsm, HEVT_TIMER_CLOCK, 1000, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_clock);
            break;
        case HSM_EVENT_EXIT: 

            break;
        case HEVT_TIMER_UPDATE:
            scrprocessslotssttcontainer_update(me->bms_info.slot_state, me->bms_data);
            scrprocessruntimevalue_update(me->time_run);
            scrprocessstatevalue_update(me->bms_info.swap_state);

            if(me->bms_info.swap_state == SWAP_STATE_CHARGING_COMPLETE
            || me->bms_info.swap_state == SWAP_STATE_MOTOR_CABLID
            || me->bms_info.swap_state == SWAP_STATE_STANDBY) {
                hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            }
            break;
        case HEVT_TIMER_CLOCK:
            me->time_run++;
            if(me->time_run > BMS_RUN_TIMEOUT) {
                // Fault
            }
            break;
        default: 
            return event;
    }
    return 0;
}
static hsm_event_t
app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    // app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "Entered Setting State");
            break;
        case HSM_EVENT_EXIT: break;
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