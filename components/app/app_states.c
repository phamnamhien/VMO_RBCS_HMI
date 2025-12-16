#include "app_states.h"


static const char *TAG = "HSM";

static HSM_EVENT app_state_loading_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_common_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_slot_1_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_slot_2_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_slot_3_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_slot_4_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_main_slot_5_handler(HSM *This, HSM_EVENT event, void *param);
static HSM_EVENT app_state_setting_handler(HSM *This, HSM_EVENT event, void *param);

static void tm_loading_callback(void *arg);

static void blink_1s_timer_callback(void *arg);


static HSM_STATE app_state_loading;
static HSM_STATE app_state_main_common;
static HSM_STATE app_state_main;
static HSM_STATE app_state_main_slot_1;
static HSM_STATE app_state_main_slot_2;
static HSM_STATE app_state_main_slot_3;
static HSM_STATE app_state_main_slot_4;
static HSM_STATE app_state_main_slot_5;
static HSM_STATE app_state_setting;

tick_handle_t tm_loading;

tick_handle_t blink_1s_timer;


void
app_state_hsm_init(DeviceHSM_t *me) {

    ESP_ERROR_CHECK(ticks_create(&blink_1s_timer, 
                                 blink_1s_timer_callback,
                                 TICK_PERIODIC, 
                                 me));
    ESP_ERROR_CHECK(ticks_create(&tm_loading, 
                                 tm_loading_callback,
                                 TICK_PERIODIC, 
                                 me));

    HSM_STATE_Create(&app_state_loading, "s_loading", app_state_loading_handler, NULL);

    HSM_STATE_Create(&app_state_main_common, "s_main_com", app_state_main_common_handler, NULL);
    HSM_STATE_Create(&app_state_main, "s_main", app_state_main_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_main_slot_1, "s_main_sl1", app_state_main_slot_1_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_main_slot_2, "s_main_sl2", app_state_main_slot_2_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_main_slot_3, "s_main_sl3", app_state_main_slot_3_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_main_slot_4, "s_main_sl4", app_state_main_slot_4_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_main_slot_5, "s_main_sl5", app_state_main_slot_5_handler, &app_state_main_common);
    HSM_STATE_Create(&app_state_setting, "s_setting", app_state_setting_handler, NULL);

    HSM_Create((HSM *)me, "app", &app_state_loading);
}    




static HSM_EVENT 
app_state_loading_handler(HSM *This, HSM_EVENT event, void *param) {
    static uint8_t loading_count = 0;
    switch (event) {
        case HSME_ENTRY:
            ticks_start(tm_loading, 100);
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            loading_count = 0;
            ticks_stop(tm_loading);
            ticks_stop(blink_1s_timer);
            break;
        case HSME_LOADING_COUNT_TIMER:
            loading_count+=10;
            if (loading_count > 100) {
                HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
                ESP_LOGI(TAG, "Loading Done, transitioning to Idle State");
            } else {
                if (ui_lock(-1)) {
                    lv_bar_set_value(ui_barSplashLoading, loading_count, LV_ANIM_OFF);
                    ui_unlock();
                }
            }
            break;
        default:
            return event;
    }
    return 0;
}

static HSM_EVENT 
app_state_main_common_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:

            break;
        case HSME_INIT:

            break;
        case HSME_EXIT:

            break;
        case HSME_MODBUS_GET_STATION_STATE_DATA:
            // Handle station state data update if needed
            break;
        case HSME_MODBUS_GET_SLOT_DATA:
            ui_update_all_slots_display((DeviceHSM_t *)This);
            break;
        case HSME_CHANGE_SCR_MAIN_TO_SETTING:
            HSM_Tran((HSM *)This, &app_state_setting, NULL, NULL);
            break;
        case HSME_MODBUS_CONNECTED:
            ui_show_main_not_connect(false);
            break;    
        case HSME_MODBUS_NOTCONNECTED:
            ui_show_main_not_connect(true);
            break;
        default:
            return event;
    }
    return 0;
}   
static HSM_EVENT 
app_state_main_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_load_screen(ui_scrMain);
            ui_show_slot_serial_detail(0);
            ui_show_slot_detail_panel(false);

            // ✅ IN RA ĐỊA CHỈ CÁC UI LABELS
            ESP_LOGI(TAG, "===========================================");
            ESP_LOGI(TAG, "  📺 UI LABEL ADDRESSES");
            ESP_LOGI(TAG, "===========================================");
            ESP_LOGI(TAG, "  ui_lbMainVoltageSlot1 @ %p", ui_lbMainVoltageSlot1);
            ESP_LOGI(TAG, "  ui_lbMainVoltageSlot2 @ %p", ui_lbMainVoltageSlot2);
            ESP_LOGI(TAG, "  ui_lbMainVoltageSlot3 @ %p", ui_lbMainVoltageSlot3);
            ESP_LOGI(TAG, "  ui_lbMainVoltageSlot4 @ %p", ui_lbMainVoltageSlot4);
            ESP_LOGI(TAG, "  ui_lbMainVoltageSlot5 @ %p", ui_lbMainVoltageSlot5);
            ESP_LOGI(TAG, "===========================================");

            ESP_LOGI(TAG, "Entered Main State");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_1, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_2, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_3, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_4, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_5, NULL, NULL);
            break;
        default:
            return event;
    }
    return 0;
}

static HSM_EVENT 
app_state_main_slot_1_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_set_button_color(ui_btMainSlot1, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(1);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 1 Clicked");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            ui_set_button_color(ui_btMainSlot1, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            //ui_show_slot_detail_panel(false);
            break;
        case HSME_MODBUS_GET_SLOT_1_DATA:
            ui_update_all_slot_details((DeviceHSM_t *)This, IDX_SLOT_1);
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_2, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_3, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_4, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_5, NULL, NULL);
            break;
        case HSME_MAIN_MANUAL_SWAP_CLICKED:
            modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1000, 1);
            break;
        default:
            return event;
    }
    return 0;
}
static HSM_EVENT 
app_state_main_slot_2_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_set_button_color(ui_btMainSlot2, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(2);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 2 Clicked");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            ui_set_button_color(ui_btMainSlot2, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HSME_MODBUS_GET_SLOT_2_DATA:
            ui_update_all_slot_details((DeviceHSM_t *)This, IDX_SLOT_2);
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_1, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_3, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_4, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_5, NULL, NULL);
            break;
        case HSME_MAIN_MANUAL_SWAP_CLICKED:
            modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1001, 1);
            break;
        default:
            return event;
    }
    return 0;
}
static HSM_EVENT 
app_state_main_slot_3_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_set_button_color(ui_btMainSlot3, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(3);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 3 Clicked");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            ui_set_button_color(ui_btMainSlot3, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HSME_MODBUS_GET_SLOT_3_DATA:
            ui_update_all_slot_details((DeviceHSM_t *)This, IDX_SLOT_3);
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_1, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_2, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_4, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_5, NULL, NULL);
            break;
        case HSME_MAIN_MANUAL_SWAP_CLICKED:
            modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1002, 1);
            break;
        default:
            return event;
    }
    return 0;
}
static HSM_EVENT 
app_state_main_slot_4_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_set_button_color(ui_btMainSlot4, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(4);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 4 Clicked");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            ui_set_button_color(ui_btMainSlot4, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HSME_MODBUS_GET_SLOT_4_DATA:
            ui_update_all_slot_details((DeviceHSM_t *)This, IDX_SLOT_4);
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_1, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_2, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_3, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_5, NULL, NULL);
            break;
        case HSME_MAIN_MANUAL_SWAP_CLICKED:
            modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1003, 1);
            break;
        default:
            return event;
    }
    return 0;
}
static HSM_EVENT 
app_state_main_slot_5_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_set_button_color(ui_btMainSlot5, BTN_COLOR_ACTIVE);
            ui_show_slot_serial_detail(5);
            ui_clear_all_slot_details();
            ui_show_slot_detail_panel(true);
            ESP_LOGI(TAG, "Main Slot 5 Clicked");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            ui_set_button_color(ui_btMainSlot5, BTN_COLOR_NORMAL);
            ui_show_slot_serial_detail(0);
            // ui_show_slot_detail_panel(false);
            break;
        case HSME_MODBUS_GET_SLOT_5_DATA:
            ui_update_all_slot_details((DeviceHSM_t *)This, IDX_SLOT_5);
            break;
        case HSME_MAIN_SLOT_1_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_1, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_2_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_2, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_3_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_3, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_4_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main_slot_4, NULL, NULL);
            break;
        case HSME_MAIN_SLOT_5_CLICKED:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        case HSME_MAIN_MANUAL_SWAP_CLICKED:
            modbus_master_write_single_register(APP_MODBUS_SLAVE_ID, 1004, 1);
            break;
        default:
            return event;
    }
    return 0;
}

static HSM_EVENT 
app_state_setting_handler(HSM *This, HSM_EVENT event, void *param) {
    switch (event) {
        case HSME_ENTRY:
            ui_load_screen(ui_scrSetting);
            ui_show_slot_detail_panel(false);
            ESP_LOGI(TAG, "Entered Setting State");
            break;
        case HSME_INIT:
            break;
        case HSME_EXIT:
            break;
        case HSME_CHANGE_SCR_SETTING_TO_MAIN:
            HSM_Tran((HSM *)This, &app_state_main, NULL, NULL);
            break;
        default:
            return event;
    }
    return 0;
}





static void tm_loading_callback(void *arg)
{
    HSM *This = (HSM *)arg;
    HSM_Run(This, HSME_LOADING_COUNT_TIMER, NULL);
}

static void blink_1s_timer_callback(void *arg)
{
    HSM *This = (HSM *)arg;
    HSM_Run(This, HSME_BLINK_1S_TIMER, NULL);
}

