#ifndef APP_STATES_H
#define APP_STATES_H

#include <stdio.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "modbus_master_manager.h"

#include "app_common.h"
#include "esp_timer.h"
#include "hsm.h"
#include "ui_support.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_gt911.h"
#include "lvgl.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMS_TIMEOUT_MAX_COUNT           3

#define LOADING_1PERCENT_MS             100     
#define UPDATE_SCREEN_VALUE_MS          1000     


#define BMS_RUN_TIMEOUT                 (60*3)

typedef enum {
    IDX_SLOT_1 = 0,
    IDX_SLOT_2,
    IDX_SLOT_3,
    IDX_SLOT_4,
    IDX_SLOT_5,
    TOTAL_SLOT,
} SlotIndex_t;

typedef enum {
    BMS_SLOT_EMPTY = 0,
    BMS_SLOT_CONNECTED,
    MBS_SLOT_DISCONNECTED,
} BMS_Slot_State_t;

typedef enum {
    SWAP_STATE_STANDBY = 0,
    SWAP_STATE_ROBOT_REQUEST,
    SWAP_STATE_ROBOT_POSTION,
    SWAP_STATE_REMOVE_EMPTY_BATTERY,
    SWAP_STATE_STORE_EMPTY_BATTERY,
    SWAP_STATE_RETRIEVES_FULL_BATTERY,
    SWAP_STATE_INSTALL_FULL_BATTERY,
    SWAP_STATE_CHARGING_COMPLETE,
    SWAP_STATE_MOTOR_CABLID,
    
    SWAP_STATE_FAULT,
    // ... add more states if needed
    TOTAL_SWAP_STATE,
} BMS_Swap_State_t;

typedef enum {
    TIMER_STATE_IDLE = 0,
    TIMER_STATE_ACTIVE,
    TIMER_STATE_DELETING,
} timer_state_t;

typedef struct {
    esp_timer_handle_t handle;
    void (*callback)(void*);
    void* arg;
    volatile uint8_t active;
} esp32_timer_t;

typedef struct {
    // BMS State Machine
    uint8_t bms_state;     // BMS state: 2=standby, 3=load, 4=charge, 5=error
    uint8_t ctrl_request;  // Control request from host
    uint8_t ctrl_response; // Control response from BMS
    uint8_t fet_ctrl_pin;  // FET control pin status
    uint8_t fet_status;    // FET status bits: PDSG|DSG|PCHG|CHG
    uint16_t alarm_bits;   // Alarm status bits
    uint8_t faults;        // Fault flags: OCC|UV|OV|SCD|OCD

    // Voltage Measurements
    uint16_t pack_volt;     // Pack voltage in mV
    uint16_t stack_volt;    // Total stack voltage in mV
    uint16_t cell_volt[13]; // Individual cell voltages in mV (13 cells)
    uint16_t ld_volt;       // Load voltage in mV

    // Current Measurement
    int32_t pack_current; // Pack current in mA (signed)

    // Temperature Sensors
    int32_t temp1; // Temperature sensor 1 in 0.1°C
    int32_t temp2; // Temperature sensor 2 in 0.1°C
    int32_t temp3; // Temperature sensor 3 in 0.1°C

    // Capacity & State of Charge
    uint16_t capacity;      // Battery capacity in mAh
    uint8_t soc_percent;    // State of Charge in %
    uint16_t soh_value;     // State of Health value in mAh
    uint8_t pin_percent;    // Current percentage indicator
    uint8_t percent_target; // Target percentage

    // Safety Status
    uint16_t safety_a; // Safety status register A
    uint16_t safety_b; // Safety status register B
    uint16_t safety_c; // Safety status register C

    // Accumulator Values
    uint32_t accu_int;  // Accumulator integer part
    uint32_t accu_frac; // Accumulator fractional part
    uint32_t accu_time; // Accumulator time counter

    // Additional Parameters
    uint8_t cell_resistance; // Cell internal resistance in mΩ
    uint8_t single_parallel; // Battery configuration: single/parallel

} BMS_Data_t;

typedef struct {
    BMS_Slot_State_t slot_state[TOTAL_SLOT];
    BMS_Swap_State_t swap_state;
    uint16_t manual_swap_request;
    uint16_t complete_swap;
} BMS_Information_t;

typedef enum {
    HEVT_LOOP = HSM_EVENT_USER,

    HEVT_LOADING_DONE,

    HEVT_MODBUS_GET_SLOT_1_DATA,
    HEVT_MODBUS_GET_SLOT_2_DATA,
    HEVT_MODBUS_GET_SLOT_3_DATA,
    HEVT_MODBUS_GET_SLOT_4_DATA,
    HEVT_MODBUS_GET_SLOT_5_DATA,
    HEVT_MODBUS_GET_SLOT_DATA,
    HEVT_MODBUS_GET_STATION_STATE_DATA,
    HEVT_MODBUS_CONNECTED,
    HEVT_MODBUS_NOTCONNECTED,

    HEVT_TRANS_MAIN_TO_DETAIL,
    HEVT_TRANS_MAIN_TO_MANUAL1,

    HEVT_TRANS_DETAIL_TO_MAIN,
    HEVT_TRANS_DETAIL_TO_MANUAL1,

    HEVT_TRANS_MANUAL1_TO_MANUAL2,
    
    HEVT_TRANS_BACK_TO_MAIN,

    HEVT_MANUAL1_SELECT_BAT1,
    HEVT_MANUAL1_SELECT_BAT2,
    HEVT_MANUAL2_SELECT_SLOT1,
    HEVT_MANUAL2_SELECT_SLOT2,
    HEVT_MANUAL2_SELECT_SLOT3,
    HEVT_MANUAL2_SELECT_SLOT4,
    HEVT_MANUAL2_SELECT_SLOT5,

    HEVT_PROCESS_PR_BUTTON_CLICKED,
    HEVT_PROCESS_ST_BUTTON_CLICKED,
    
    HEVT_TIMER_LOADING,
    HEVT_TIMER_UPDATE,
    HEVT_TIMER_CLOCK,
} app_events_t;



typedef struct {
    hsm_t parent;

    BMS_Data_t bms_data[TOTAL_SLOT];
    BMS_Information_t bms_info;
    
    uint16_t time_run;
    uint16_t last_time_run;

    uint16_t present_slot_display;

    uint8_t manual_robot_bat_select;

    uint8_t is_bms_not_connected;
} app_state_hsm_t;

void app_state_hsm_init(app_state_hsm_t* me);


// UI main screen
void scrmainbatslotscontainer_update(const bool has_slot[5], const float voltages[5], const float percents[5]);
void scrmainlasttimelabel_update(uint16_t seconds);
void scrmainstateofchargervalue_update(BMS_Swap_State_t state);
// UI detail screen
void scrdetaildataslottitlelabel_update(SlotIndex_t index);
void scrdetailslotssttcontainer_update(const BMS_Slot_State_t state[TOTAL_SLOT], const BMS_Data_t data[TOTAL_SLOT], uint16_t current_slot);
void scrdetaildataslotvalue_update(const BMS_Data_t* data, BMS_Slot_State_t state);
// UI manual 2 screen
void scrmanual2slotinfolabel_update(const bool has_slot[5], const float voltages[5], const float percents[5]);

// UI Process screen
void scrprocessslotssttcontainer_update(const BMS_Slot_State_t state[TOTAL_SLOT], const BMS_Data_t data[TOTAL_SLOT]);
void scrprocessruntimevalue_update(uint16_t seconds);
void scrprocessstatevalue_update(BMS_Swap_State_t state);


#ifdef __cplusplus
}

#endif

#endif // APP_STATES_H
