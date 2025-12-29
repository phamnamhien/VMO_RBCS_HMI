#include "app_states.h"
#include "esp_timer.h"

static const char* TAG = "HSM";

static hsm_event_t app_state_loading_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_common_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_main_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_detail_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_manual1_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_manual2_handler(hsm_t* hsm, hsm_event_t event, void* data);
static hsm_event_t app_state_process_handler(hsm_t* hsm, hsm_event_t event, void* data);

static hsm_event_t app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data);


static void timer_loading_callback(void* arg);
static void timer_update_callback(void* arg);
static void timer_clock_callback(void* arg);

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

esp_timer_handle_t timer_loading;
esp_timer_handle_t timer_update;
esp_timer_handle_t timer_clock;

void
app_state_hsm_init(app_state_hsm_t* me) {
    /* Create timer */
    const esp_timer_create_args_t timer_loading_args = {
        .callback = &timer_loading_callback,
        .arg = me,
        .name = "t_loading"
    };
    const esp_timer_create_args_t timer_update_args = {
        .callback = &timer_update_callback,
        .arg = me,
        .name = "t_update"
    };
    const esp_timer_create_args_t timer_clock_args = {
        .callback = &timer_clock_callback,
        .arg = me,
        .name = "t_clock"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_loading_args, &timer_loading));
    ESP_ERROR_CHECK(esp_timer_create(&timer_update_args, &timer_update));
    ESP_ERROR_CHECK(esp_timer_create(&timer_clock_args, &timer_clock));

    /* Create states */
    hsm_state_create(&app_state_loading, "s_loading", app_state_loading_handler, NULL);
    hsm_state_create(&app_state_main_common, "s_main_com", app_state_main_common_handler, NULL);
    hsm_state_create(&app_state_main, "s_main", app_state_main_handler, &app_state_main_common);
    hsm_state_create(&app_state_detail, "s_detail", app_state_detail_handler, &app_state_main_common);
    hsm_state_create(&app_state_manual1, "s_manual1", app_state_manual1_handler, &app_state_main_common);
    hsm_state_create(&app_state_manual2, "s_manual2", app_state_manual2_handler, &app_state_main_common);
    hsm_state_create(&app_state_process, "s_process", app_state_process_handler, &app_state_main_common);
    hsm_state_create(&app_state_setting, "s_setting", app_state_setting_handler, NULL);

    /* Init HSM */
    hsm_init((hsm_t *)me, "app", &app_state_loading);
}

static hsm_event_t
app_state_loading_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    static uint8_t loading_count = 0;
    
    switch (event) {
        case HSM_EVENT_ENTRY: 
            loading_count = 0; 
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_loading, LOADING_1PERCENT_MS*1000));
            ESP_LOGI(TAG, "Loading: ENTRY");
            break;
            
        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "Loading: EXIT");
            loading_count = 0;
            ESP_ERROR_CHECK(esp_timer_stop(timer_loading));
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
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_update, UPDATE_SCREEN_VALUE_MS*1000));
            me->last_time_run = me->time_run;
            me->time_run = 0;
            ESP_LOGI(TAG, "Entered Main State");
            break;
        case HSM_EVENT_EXIT: 
            ESP_ERROR_CHECK(esp_timer_stop(timer_update));
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
                me->bms_data[0].stack_volt/1000.0, 
                me->bms_data[1].stack_volt/1000.0, 
                me->bms_data[2].stack_volt/1000.0, 
                me->bms_data[3].stack_volt/1000.0, 
                me->bms_data[4].stack_volt/1000.0
            };
            float percents[5] = {
                me->bms_data[0].pin_percent, 
                me->bms_data[1].pin_percent, 
                me->bms_data[2].pin_percent, 
                me->bms_data[3].pin_percent, 
                me->bms_data[4].pin_percent
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
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
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
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_update, UPDATE_SCREEN_VALUE_MS*1000));
            ESP_LOGI(TAG, "Entered Detail State");
            break;
        case HSM_EVENT_EXIT: 
            ESP_ERROR_CHECK(esp_timer_stop(timer_update));
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
        case HEVT_TRANS_DETAIL_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            break;
        case HEVT_TRANS_DETAIL_TO_MANUAL1:
            hsm_transition((hsm_t *)me, &app_state_manual1, NULL, NULL);
            break;
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            break;
        default: 
            return event;
    }
    return 0;
}

static hsm_event_t
app_state_manual1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY: 
        ESP_LOGI(TAG, "Entered Manual1 State");
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
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
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
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_update, UPDATE_SCREEN_VALUE_MS*1000));
            ESP_LOGI(TAG, "Entered Manual2 State");
            break;
        case HSM_EVENT_EXIT:    
            ESP_ERROR_CHECK(esp_timer_stop(timer_update));
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
                me->bms_data[0].stack_volt/1000.0, 
                me->bms_data[1].stack_volt/1000.0, 
                me->bms_data[2].stack_volt/1000.0, 
                me->bms_data[3].stack_volt/1000.0, 
                me->bms_data[4].stack_volt/1000.0
            };
            float percents[5] = {
                me->bms_data[0].pin_percent, 
                me->bms_data[1].pin_percent, 
                me->bms_data[2].pin_percent, 
                me->bms_data[3].pin_percent, 
                me->bms_data[4].pin_percent
            };
            scrmanual2slotinfolabel_update(slots, voltages, percents);
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
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            break;
        default: 
            return event;
    }
    return 0;
}

static hsm_event_t app_state_process_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    static bool is_paused = false;
    switch (event) {
        case HSM_EVENT_ENTRY: 
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_update, UPDATE_SCREEN_VALUE_MS*1000));
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_clock, 1000*1000));
            break;
        case HSM_EVENT_EXIT: 
            ESP_ERROR_CHECK(esp_timer_stop(timer_update));
            ESP_ERROR_CHECK(esp_timer_stop(timer_clock));
            is_paused = false;
            break;
        case HEVT_TIMER_UPDATE:
            scrprocessslotssttcontainer_update(me->bms_info.slot_state, me->bms_data);
            scrprocessruntimevalue_update(me->time_run);
            scrprocessstatevalue_update(me->bms_info.swap_state);

            if(me->bms_info.complete_swap) {
                me->bms_info.complete_swap = 0;
                modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_COMPLETE_SWAP_REG, 
                        me->bms_info.complete_swap);
                hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            }
            break;
        case HEVT_TIMER_CLOCK:
            me->time_run++;
            ESP_LOGD(TAG, "Process Time Run: %d seconds", me->time_run);
            if(me->time_run > BMS_RUN_TIMEOUT) {
                // Fault
            }
            break;
        case HEVT_PROCESS_PR_BUTTON_CLICKED:
            lv_obj_t* button = ui_comp_get_child(ui_scrprocessprcontainer, UI_COMP_BUTTONCONTAINER_BUTTON);
            lv_obj_t* label = ui_comp_get_child(ui_scrprocessprcontainer, UI_COMP_BUTTONCONTAINER_BUTONLABEL);
            if(!is_paused) {
                // Pause
                is_paused = true;
                lv_obj_set_style_bg_color(button, lv_color_hex(0x1A6538), LV_PART_MAIN);
                lv_label_set_text(label, "resume");
                ESP_ERROR_CHECK(esp_timer_stop(timer_clock));
            } else {
                // Resume
                is_paused = false;
                lv_obj_set_style_bg_color(button, lv_color_hex(0x2095F6), LV_PART_MAIN);
                lv_label_set_text(label, "pause");
                ESP_ERROR_CHECK(esp_timer_start_periodic(timer_clock, 1000*1000));
            }

            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_PAUSE_RESUME_REG, 
                        is_paused);
            break;
        case HEVT_PROCESS_ST_BUTTON_CLICKED:
            modbus_master_write_single_register(
                        APP_MODBUS_SLAVE_ID, 
                        MB_COMMON_E_STOP_REG, 
                        1);
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);            
            break;
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            break;
        default: 
            return event;
    }
    return 0;
}
static hsm_event_t
app_state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_state_hsm_t* me = (app_state_hsm_t *)hsm;
    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "Entered Setting State");
            break;
        case HSM_EVENT_EXIT: 
            break;
        case HEVT_TRANS_BACK_TO_MAIN:
            hsm_transition((hsm_t *)me, &app_state_main, NULL, NULL);
            break;    
        default: return event;
    }
    return 0;
}


// Timer callback function
static void timer_loading_callback(void* arg) {
    hsm_t* me = (hsm_t *)arg;
    hsm_dispatch(me, HEVT_TIMER_LOADING, NULL);
}
static void timer_update_callback(void* arg) {
    hsm_t* me = (hsm_t *)arg;
    hsm_dispatch(me, HEVT_TIMER_UPDATE, NULL);
}
static void timer_clock_callback(void* arg) {
    hsm_t* me = (hsm_t *)arg;
    hsm_dispatch(me, HEVT_TIMER_CLOCK, NULL);
}
