#include "app_states.h"

static const char* TAG = "UI_HELPERS";

// ============================================
// ĐỊNH NGHĨA MÀU SẮC
// ============================================
#define COLOR_STANDBY 0xFFFFFF // Màu den cho Standby
#define COLOR_LOAD    0x00BFFF // Màu xanh dương cho Load
#define COLOR_CHARGE  0x00FF00 // Màu xanh lá cho Charge (đang phóng điện)
#define COLOR_ERROR   0xFF0000 // Màu đỏ cho Error
#define COLOR_LOW_SOC 0xFF0000 // Màu đỏ cho SOC < 10%
#define COLOR_OTHER   0x808080

/*--------------------------------------------------------------------*/
/* OPTIMIZED BATCH UPDATE - SINGLE LOCK */
/*--------------------------------------------------------------------*/


// Hàm cập nhật dữ liệu cho container
#include "lvgl.h"
#include "esp_log.h"

#define TAG "UI"

void scrmainbatslotscontainer_update(
    const bool has_slot[5],
    const float voltages[5],
    const float percents[5])
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char summaryText[256];
    summaryText[0] = '\0';

    for (int i = 0; i < 5; i++) {
        char line[64];

        if (has_slot[i]) {
            snprintf(line, sizeof(line),
                     "%.1fV\n%.1f%%",
                     voltages[i], percents[i]);

            int value = (int)percents[i];
            switch (i) {
                case 0: lv_bar_set_value(ui_scrmainbatslot1bar, value, LV_ANIM_OFF); break;
                case 1: lv_bar_set_value(ui_scrmainbatslot2bar, value, LV_ANIM_OFF); break;
                case 2: lv_bar_set_value(ui_scrmainbatslot3bar, value, LV_ANIM_OFF); break;
                case 3: lv_bar_set_value(ui_scrmainbatslot4bar, value, LV_ANIM_OFF); break;
                case 4: lv_bar_set_value(ui_scrmainbatslot5bar, value, LV_ANIM_OFF); break;
            }
        } else {
            snprintf(line, sizeof(line), "-.-V\n-.-%%");

            switch (i) {
                case 0: lv_bar_set_value(ui_scrmainbatslot1bar, 0, LV_ANIM_OFF); break;
                case 1: lv_bar_set_value(ui_scrmainbatslot2bar, 0, LV_ANIM_OFF); break;
                case 2: lv_bar_set_value(ui_scrmainbatslot3bar, 0, LV_ANIM_OFF); break;
                case 3: lv_bar_set_value(ui_scrmainbatslot4bar, 0, LV_ANIM_OFF); break;
                case 4: lv_bar_set_value(ui_scrmainbatslot5bar, 0, LV_ANIM_OFF); break;
            }
        }

        strcat(summaryText, line);
        if (i < 4) strcat(summaryText, "\n\n");
    }

    lv_label_set_text(ui_scrmainbatslotslabel, summaryText);

    ui_unlock();
}

void scrmainlasttimelabel_update(uint16_t seconds)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char timeText[32];
    uint16_t hours = seconds / 3600;
    uint16_t minutes = (seconds % 3600) / 60;
    uint16_t secs = seconds % 60;

    if (hours > 0) {
        snprintf(timeText, sizeof(timeText), "%uh%um%us", hours, minutes, secs);
    } else if (minutes > 0) {
        snprintf(timeText, sizeof(timeText), "%um%us", minutes, secs);
    } else {
        snprintf(timeText, sizeof(timeText), "%us", secs);
    }

    lv_label_set_text(ui_scrmainlasttimelabel, timeText);

    ui_unlock();
}

void scrmainstateofchargervalue_update(BMS_Swap_State_t state)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    const char* stateText;
    
    switch (state) {
        case SWAP_STATE_STANDBY:
            stateText = "Standby";
            break;
        case SWAP_STATE_ROBOT_REQUEST:
            stateText = "Requesting";
            break;
        case SWAP_STATE_ROBOT_POSTION:
            stateText = "Positioning";
            break;
        case SWAP_STATE_REMOVE_EMPTY_BATTERY:
            stateText = "Removing";
            break;
        case SWAP_STATE_STORE_EMPTY_BATTERY:
            stateText = "Storing";
            break;
        case SWAP_STATE_RETRIEVES_FULL_BATTERY:
            stateText = "Retrieving";
            break;
        case SWAP_STATE_INSTALL_FULL_BATTERY:
            stateText = "Installing";
            break;
        case SWAP_STATE_CHARGING_COMPLETE:
            stateText = "Complete";
            break;
        case SWAP_STATE_MOTOR_CABLID:
            stateText = "Motor Calib";
            break;
        case SWAP_STATE_FAULT:
            stateText = "Fault";
            break;
        default:
            stateText = "----";
            break;
    }

    lv_label_set_text(ui_scrmainstateofchargervalue, stateText);

    ui_unlock();
}


void scrdetaildataslottitlelabel_update(SlotIndex_t index)
{
    if (index < 0 || index >= TOTAL_SLOT) {
        ESP_LOGE(TAG, "Invalid slot index: %d", index);
        return;
    }

    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char buf[16]; // đủ để chứa "SLOT X"
    snprintf(buf, sizeof(buf), "SLOT %d", index + 1);

    lv_label_set_text(ui_scrdetaildataslottitlelabel, buf);

    ui_unlock();
}
void scrdetailslotssttcontainer_update(
    const BMS_Slot_State_t state[TOTAL_SLOT],
    const BMS_Data_t data[TOTAL_SLOT],
    uint16_t current_slot)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    lv_obj_t* panels[5] = {
        ui_scrdetailslotssttpanel1,
        ui_scrdetailslotssttpanel2,
        ui_scrdetailslotssttpanel3,
        ui_scrdetailslotssttpanel4,
        ui_scrdetailslotssttpanel5
    };

    for (int i = 0; i < 5; i++) {
        lv_color_t color;
        int width, height;

        if (state[i] == BMS_SLOT_CONNECTED) {
            if (data[i].faults != 0) {
                color = lv_color_hex(0xEE3A29); // Đỏ - lỗi
            } else {
                color = lv_color_hex(0x46A279); // Xanh - bình thường
            }
        } else if (state[i] == MBS_SLOT_DISCONNECTED) {
            color = lv_color_hex(0xFF8C00); // Cam - disconnected
        } else {
            color = lv_color_hex(0xFFFFFF); // Trắng - empty
        }

        if (current_slot == i) {
            width = height = 25;
        } else {
            width = height = 20;
        }

        lv_obj_set_style_bg_color(panels[i], color, LV_PART_MAIN);
        lv_obj_set_size(panels[i], width, height);
    }

    ui_unlock();
}

void scrdetaildataslotvalue_update(const BMS_Data_t* data, BMS_Slot_State_t state)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char col1[512], col2[512], col3[512];
    
    if (state == BMS_SLOT_EMPTY) {
        snprintf(col1, sizeof(col1),
            "State: -\n"
            "CtrlReq: -\n"
            "CtrlRsp: -\n"
            "FETCtrl: -\n"
            "FETStat: -\n"
            "Alarm: -\n"
            "Faults: -\n"
            "PackV: -.-V\n"
            "StackV: -.-V\n"
            "LoadV: -.-V\n"
            "Curr: -.-A\n"
            "Cap: -mAh\n"
            "SOC: -\n"
            "SOH: -mAh\n"
        );
        
        snprintf(col2, sizeof(col2),
            "Temp1: -.-C\n"
            "Temp2: -.-C\n"
            "Temp3: -.-C\n"
            "PinPct: -\n"
            "TgtPct: -\n"
            "SafeA: -\n"
            "SafeB: -\n"
            "SafeC: -\n"
            "Resist: -mOhm\n"
            "S/P: -\n"
            "AccInt: -\n"
            "AccFrac: -\n"
            "AccTime: -\n"
        );
        
        snprintf(col3, sizeof(col3),
            "C1: -.-V\n"
            "C2: -.-V\n"
            "C3: -.-V\n"
            "C4: -.-V\n"
            "C5: -.-V\n"
            "C6: -.-V\n"
            "C7: -.-V\n"
            "C8: -.-V\n"
            "C9: -.-V\n"
            "C10: -.-V\n"
            "C11: -.-V\n"
            "C12: -.-V\n"
            "C13: -.-V\n"
        );
    } else {
        const char* state_str;
        switch(data->bms_state) {
            case 2: state_str = "STBY"; break;
            case 3: state_str = "LOAD"; break;
            case 4: state_str = "CHRG"; break;
            case 5: state_str = "ERR"; break;
            default: state_str = "UNK"; break;
        }
        
        snprintf(col1, sizeof(col1),
            "State: %s\n"
            "CtrlReq: %u\n"
            "CtrlRsp: %u\n"
            "FETCtrl: %u\n"
            "FETStat: 0x%02X\n"
            "Alarm: 0x%04X\n"
            "Faults: 0x%02X\n"
            "PackV: %.1fV\n"
            "StackV: %.1fV\n"
            "LoadV: %.1fV\n"
            "Curr: %.1fA\n"
            "Cap: %umAh\n"
            "SOC: %u\n"
            "SOH: %umAh\n",
            state_str,
            data->ctrl_request,
            data->ctrl_response,
            data->fet_ctrl_pin,
            data->fet_status,
            data->alarm_bits,
            data->faults,
            data->pack_volt / 1000.0f,
            data->stack_volt / 1000.0f,
            data->ld_volt / 1000.0f,
            data->pack_current / 1000.0f,
            data->capacity,
            data->soc_percent,
            data->soh_value
        );
        
        snprintf(col2, sizeof(col2),
            "Temp1: %.1fC\n"
            "Temp2: %.1fC\n"
            "Temp3: %.1fC\n"
            "PinPct: %u\n"
            "TgtPct: %u\n"
            "SafeA: 0x%04X\n"
            "SafeB: 0x%04X\n"
            "SafeC: 0x%04X\n"
            "Resist: %umOhm\n"
            "S/P: %u\n"
            "AccInt: %lu\n"
            "AccFrac: %lu\n"
            "AccTime: %lu\n",
            data->temp1 / 10.0f,
            data->temp2 / 10.0f,
            data->temp3 / 10.0f,
            data->pin_percent,
            data->percent_target,
            data->safety_a,
            data->safety_b,
            data->safety_c,
            data->cell_resistance,
            data->single_parallel,
            (unsigned long)data->accu_int,
            (unsigned long)data->accu_frac,
            (unsigned long)data->accu_time
        );
        
        snprintf(col3, sizeof(col3),
            "C1: %.1fV\n"
            "C2: %.1fV\n"
            "C3: %.1fV\n"
            "C4: %.1fV\n"
            "C5: %.1fV\n"
            "C6: %.1fV\n"
            "C7: %.1fV\n"
            "C8: %.1fV\n"
            "C9: %.1fV\n"
            "C10: %.1fV\n"
            "C11: %.1fV\n"
            "C12: %.1fV\n"
            "C13: %.1fV\n",
            data->cell_volt[0] / 1000.0f,
            data->cell_volt[1] / 1000.0f,
            data->cell_volt[2] / 1000.0f,
            data->cell_volt[3] / 1000.0f,
            data->cell_volt[4] / 1000.0f,
            data->cell_volt[5] / 1000.0f,
            data->cell_volt[6] / 1000.0f,
            data->cell_volt[7] / 1000.0f,
            data->cell_volt[8] / 1000.0f,
            data->cell_volt[9] / 1000.0f,
            data->cell_volt[10] / 1000.0f,
            data->cell_volt[11] / 1000.0f,
            data->cell_volt[12] / 1000.0f
        );
    }

    lv_label_set_text(ui_scrdetaildataslotvalue1, col1);
    lv_label_set_text(ui_scrdetaildataslotvalue2, col2);
    lv_label_set_text(ui_scrdetaildataslotvalue3, col3);

    ui_unlock();
}

void scrmanual2slotinfolabel_update(
    const bool has_slot[5],
    const float voltages[5],
    const float percents[5])
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char summaryText[256];
    summaryText[0] = '\0';

    for (int i = 0; i < 5; i++) {
        char line[64];

        if (has_slot[i]) {
            snprintf(line, sizeof(line),
                     "%.1fV\n%.1f%%",
                     voltages[i], percents[i]);
        } else {
            snprintf(line, sizeof(line), "-.-V\n-.-%%");
        }

        strcat(summaryText, line);
        if (i < 4) strcat(summaryText, "\n\n");
    }

    lv_label_set_text(ui_scrmanual2slotinfolabel, summaryText);

    ui_unlock();
}



void scrprocessslotssttcontainer_update(
    const BMS_Slot_State_t state[TOTAL_SLOT],
    const BMS_Data_t data[TOTAL_SLOT])
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    lv_obj_t* panels[5] = {
        ui_scrprocessslotssttslot1panel,
        ui_scrprocessslotssttslot2panel,
        ui_scrprocessslotssttslot3panel,
        ui_scrprocessslotssttslot4panel,
        ui_scrprocessslotssttslot5panel
    };

    for (int i = 0; i < 5; i++) {
        lv_color_t color;

        if (state[i] == BMS_SLOT_CONNECTED) {
            if (data[i].faults != 0) {
                color = lv_color_hex(0xEE3A29);
            } else {
                color = lv_color_hex(0x46A279);
            }
        } else if (state[i] == MBS_SLOT_DISCONNECTED) {
            color = lv_color_hex(0xFF8C00);
        } else {
            color = lv_color_hex(0xFFFFFF);
        }

        lv_obj_set_style_bg_color(panels[i], color, LV_PART_MAIN);
    }

    ui_unlock();
}
void scrprocessruntimevalue_update(uint16_t seconds)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    char timeText[32];
    uint16_t hours = seconds / 3600;
    uint16_t minutes = (seconds % 3600) / 60;
    uint16_t secs = seconds % 60;

    if (hours > 0) {
        snprintf(timeText, sizeof(timeText), "%uh%um%us", hours, minutes, secs);
    } else if (minutes > 0) {
        snprintf(timeText, sizeof(timeText), "%um%us", minutes, secs);
    } else {
        snprintf(timeText, sizeof(timeText), "%us", secs);
    }

    lv_label_set_text(ui_scrprocessruntimevalue, timeText);

    ui_unlock();
}

void scrprocessstatevalue_update(BMS_Swap_State_t state)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }

    const char* stateText;
    
    switch (state) {
        case SWAP_STATE_STANDBY:
            stateText = "Standby";
            break;
        case SWAP_STATE_ROBOT_REQUEST:
            stateText = "Requesting";
            break;
        case SWAP_STATE_ROBOT_POSTION:
            stateText = "Positioning";
            break;
        case SWAP_STATE_REMOVE_EMPTY_BATTERY:
            stateText = "Removing";
            break;
        case SWAP_STATE_STORE_EMPTY_BATTERY:
            stateText = "Storing";
            break;
        case SWAP_STATE_RETRIEVES_FULL_BATTERY:
            stateText = "Retrieving";
            break;
        case SWAP_STATE_INSTALL_FULL_BATTERY:
            stateText = "Installing";
            break;
        case SWAP_STATE_CHARGING_COMPLETE:
            stateText = "Complete";
            break;
        case SWAP_STATE_MOTOR_CABLID:
            stateText = "Motor Calib";
            break;
        case SWAP_STATE_FAULT:
            stateText = "Fault";
            break;
        default:
            stateText = "----";
            break;
    }

    lv_label_set_text(ui_scrprocessstatevalue, stateText);

    ui_unlock();
}