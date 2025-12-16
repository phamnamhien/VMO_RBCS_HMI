#include "app_states.h"

static const char *TAG = "UI_HELPERS";

// ============================================
// ĐỊNH NGHĨA MÀU SẮC
// ============================================
#define COLOR_STANDBY   0xFFFFFF  // Màu den cho Standby
#define COLOR_LOAD      0x00BFFF  // Màu xanh dương cho Load
#define COLOR_CHARGE    0x00FF00  // Màu xanh lá cho Charge (đang phóng điện)
#define COLOR_ERROR     0xFF0000  // Màu đỏ cho Error
#define COLOR_LOW_SOC   0xFF0000  // Màu đỏ cho SOC < 10%
#define COLOR_OTHER     0x808080
/*--------------------------------------------------------------------*/
/* OPTIMIZED BATCH UPDATE - SINGLE LOCK */
/*--------------------------------------------------------------------*/

void ui_update_all_slot_details(DeviceHSM_t *me, uint8_t slot_index)
{
    if (slot_index >= TOTAL_SLOT) {
        ESP_LOGE(TAG, "Invalid slot index: %d", slot_index);
        return;
    }
    
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }
    
    // ✅ CHỈ LOCK 1 LẦN CHO TẤT CẢ UPDATE
    BMS_Data_t *bms = &me->bms_data[slot_index];
    char buf[32];
    
    // --- BMS State ---
    const char *state_text = "UNKNOWN";
    switch (bms->bms_state) {
        case 2: state_text = "STANDBY"; break;
        case 3: state_text = "LOAD"; break;
        case 4: state_text = "CHARGE"; break;
        case 5: state_text = "ERROR"; break;
    }
    lv_label_set_text(ui_lbMainSlotBMSStateValue, state_text);
    
    // --- Control Values ---
    snprintf(buf, sizeof(buf), "%d", bms->ctrl_request);
    lv_label_set_text(ui_lbMainSlotCtrlRqValue, buf);
    
    snprintf(buf, sizeof(buf), "%d", bms->ctrl_response);
    lv_label_set_text(ui_lbMainSlotCtrlRpValue, buf);
    
    snprintf(buf, sizeof(buf), "%d", bms->fet_ctrl_pin);
    lv_label_set_text(ui_lbMainSlotFETCtrlPValue, buf);
    
    // --- FET Status ---
    uint8_t fet = bms->fet_status;
    bool chg_on = (fet & 0x01), pchg_on = (fet & 0x02);
    bool dsg_on = (fet & 0x04), pdsg_on = (fet & 0x08);
    
    const char *fet_text = (!chg_on && !dsg_on) ? "IDLE" :
                          (chg_on && !dsg_on) ? (pchg_on ? "PCHG" : "CHG") :
                          (!chg_on && dsg_on) ? (pdsg_on ? "PDSG" : "DSG") : "ERROR";
    lv_label_set_text(ui_lbMainSlotFETSttValue, fet_text);
    
    // --- Alarm & Faults ---
    snprintf(buf, sizeof(buf), "%d", bms->alarm_bits);
    lv_label_set_text(ui_lbMainSlotAlarmBitsValue, buf);
    
    uint8_t faults = bms->faults;
    const char *fault_text = (faults == 0) ? "NONE" :
                            (faults & 0x02) ? "SCD" :
                            (faults & 0x01) ? "OCD" :
                            (faults & 0x10) ? "OCC" :
                            (faults & 0x04) ? "OV" :
                            (faults & 0x08) ? "UV" : "FAULT";
    lv_label_set_text(ui_lbMainSlotFaultValue, fault_text);
    
    // --- Voltages ---
    snprintf(buf, sizeof(buf), "%.3fV", bms->pack_volt / 1000.0f);
    lv_label_set_text(ui_lbMainSlotPackVoltValue, buf);
    
    snprintf(buf, sizeof(buf), "%.3fV", bms->stack_volt / 1000.0f);
    lv_label_set_text(ui_lbMainSlotStackVoltValue, buf);
    
    snprintf(buf, sizeof(buf), "%.3fV", bms->ld_volt / 1000.0f);
    lv_label_set_text(ui_lbMainSlotIdVoltValue, buf);
    
    // --- Current ---
    snprintf(buf, sizeof(buf), "%.3fA", bms->pack_current / 1000.0f);
    lv_label_set_text(ui_lbMainSlotPackCurValue, buf);
    
    // --- Percentages ---
    snprintf(buf, sizeof(buf), "%d%%", bms->pin_percent);
    lv_label_set_text(ui_lbMainSlotPinPercentValue, buf);
    
    snprintf(buf, sizeof(buf), "%d%%", bms->percent_target);
    lv_label_set_text(ui_lbMainSlotTgPercentValue, buf);
    
    snprintf(buf, sizeof(buf), "%d%%", bms->soc_percent);
    lv_label_set_text(ui_lbMainSlotSocPerValue, buf);
    
    // --- Battery Health ---
    snprintf(buf, sizeof(buf), "%dmR", bms->cell_resistance);
    lv_label_set_text(ui_lbMainSlotCelResValue, buf);
    
    snprintf(buf, sizeof(buf), "%d", bms->soh_value);
    lv_label_set_text(ui_lbMainSlotSOHValue, buf);
    
    lv_label_set_text(ui_lbMainSlotSinParValue, 
                     bms->single_parallel == 0 ? "SINGLE" : "PARALLEL");
    
    // --- Temperatures ---
    snprintf(buf, sizeof(buf), "%.1f°C", bms->temp1 / 10.0f);
    lv_label_set_text(ui_lbMainSlotTemp1Value, buf);
    
    snprintf(buf, sizeof(buf), "%.1f°C", bms->temp2 / 10.0f);
    lv_label_set_text(ui_lbMainSlotTemp2Value, buf);
    
    snprintf(buf, sizeof(buf), "%.1f°C", bms->temp3 / 10.0f);
    lv_label_set_text(ui_lbMainSlotTemp3Value, buf);
    
    // --- Cell Voltages (13 cells) ---
    lv_obj_t *cells[] = {
        ui_lbMainSlotCell1Value,  ui_lbMainSlotCell2Value,  ui_lbMainSlotCell3Value,
        ui_lbMainSlotCell4Value,  ui_lbMainSlotCell5Value,  ui_lbMainSlotCell6Value,
        ui_lbMainSlotCell7Value,  ui_lbMainSlotCell8Value,  ui_lbMainSlotCell9Value,
        ui_lbMainSlotCell10Value, ui_lbMainSlotCell11Value, ui_lbMainSlotCell12Value,
        ui_lbMainSlotCell13Value
    };
    for (uint8_t i = 0; i < 13; i++) {
        snprintf(buf, sizeof(buf), "%dmV", bms->cell_volt[i]);
        lv_label_set_text(cells[i], buf);
    }
    
    // --- Accumulator Values ---
    snprintf(buf, sizeof(buf), "%lu", bms->accu_int);
    lv_label_set_text(ui_lbMainSlotACCUIntValue, buf);
    
    snprintf(buf, sizeof(buf), "%lu", bms->accu_frac);
    lv_label_set_text(ui_lbMainSlotACCUFracValue, buf);
    
    snprintf(buf, sizeof(buf), "%lu", bms->accu_time);
    lv_label_set_text(ui_lbMainSlotACCUTimeValue, buf);
    
    // --- Safety Status ---
    snprintf(buf, sizeof(buf), "0x%04X", bms->safety_a);
    lv_label_set_text(ui_lbMainSlotSafetyAValue, buf);
    
    snprintf(buf, sizeof(buf), "0x%04X", bms->safety_b);
    lv_label_set_text(ui_lbMainSlotSafetyBValue, buf);
    
    snprintf(buf, sizeof(buf), "0x%04X", bms->safety_c);
    lv_label_set_text(ui_lbMainSlotSafetyCValue, buf);
    
    ui_unlock();  // ✅ CHỈ UNLOCK 1 LẦN
}

void ui_clear_all_slot_details(void)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }
    
    // ✅ CHỈ LOCK 1 LẦN CHO TẤT CẢ CLEAR
    const char *no_data = "-";
    
    // --- BMS State & Control ---
    lv_label_set_text(ui_lbMainSlotBMSStateValue, no_data);
    lv_label_set_text(ui_lbMainSlotCtrlRqValue, no_data);
    lv_label_set_text(ui_lbMainSlotCtrlRpValue, no_data);
    lv_label_set_text(ui_lbMainSlotFETCtrlPValue, no_data);
    lv_label_set_text(ui_lbMainSlotFETSttValue, no_data);
    lv_label_set_text(ui_lbMainSlotAlarmBitsValue, no_data);
    lv_label_set_text(ui_lbMainSlotFaultValue, no_data);
    
    // --- Voltages ---
    lv_label_set_text(ui_lbMainSlotPackVoltValue, no_data);
    lv_label_set_text(ui_lbMainSlotStackVoltValue, no_data);
    lv_label_set_text(ui_lbMainSlotIdVoltValue, no_data);
    
    // --- Current ---
    lv_label_set_text(ui_lbMainSlotPackCurValue, no_data);
    
    // --- Percentages ---
    lv_label_set_text(ui_lbMainSlotPinPercentValue, no_data);
    lv_label_set_text(ui_lbMainSlotTgPercentValue, no_data);
    lv_label_set_text(ui_lbMainSlotSocPerValue, no_data);
    
    // --- Battery Health ---
    lv_label_set_text(ui_lbMainSlotCelResValue, no_data);
    lv_label_set_text(ui_lbMainSlotSOHValue, no_data);
    lv_label_set_text(ui_lbMainSlotSinParValue, no_data);
    
    // --- Temperatures ---
    lv_label_set_text(ui_lbMainSlotTemp1Value, no_data);
    lv_label_set_text(ui_lbMainSlotTemp2Value, no_data);
    lv_label_set_text(ui_lbMainSlotTemp3Value, no_data);
    
    // --- Cell Voltages (13 cells) ---
    lv_obj_t *cells[] = {
        ui_lbMainSlotCell1Value,  ui_lbMainSlotCell2Value,  ui_lbMainSlotCell3Value,
        ui_lbMainSlotCell4Value,  ui_lbMainSlotCell5Value,  ui_lbMainSlotCell6Value,
        ui_lbMainSlotCell7Value,  ui_lbMainSlotCell8Value,  ui_lbMainSlotCell9Value,
        ui_lbMainSlotCell10Value, ui_lbMainSlotCell11Value, ui_lbMainSlotCell12Value,
        ui_lbMainSlotCell13Value
    };
    for (uint8_t i = 0; i < 13; i++) {
        lv_label_set_text(cells[i], no_data);
    }
    
    // --- Accumulator Values ---
    lv_label_set_text(ui_lbMainSlotACCUIntValue, no_data);
    lv_label_set_text(ui_lbMainSlotACCUFracValue, no_data);
    lv_label_set_text(ui_lbMainSlotACCUTimeValue, no_data);
    
    // --- Safety Status ---
    lv_label_set_text(ui_lbMainSlotSafetyAValue, no_data);
    lv_label_set_text(ui_lbMainSlotSafetyBValue, no_data);
    lv_label_set_text(ui_lbMainSlotSafetyCValue, no_data);
    
    ui_unlock();  // ✅ CHỈ UNLOCK 1 LẦN
}

/*--------------------------------------------------------------------*/
/* MAIN SCREEN BATTERY UPDATES */
/*--------------------------------------------------------------------*/

void ui_update_main_slot_voltage(DeviceHSM_t *me, int8_t slot_index)
{
    if (slot_index >= TOTAL_SLOT) return;
    
    lv_obj_t *labels[] = {
        ui_lbMainVoltageSlot1, ui_lbMainVoltageSlot2, ui_lbMainVoltageSlot3,
        ui_lbMainVoltageSlot4, ui_lbMainVoltageSlot5
    };
    
    if (!ui_lock(-1)) return;
    
    // ❌ Kiểm tra mất kết nối BMS
    if (me->is_bms_not_connected) {
        lv_obj_add_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        ui_unlock();
        return;
    }
    
    // ✅ Có kết nối → Kiểm tra trạng thái slot
    BMS_Slot_State_t slot_state = me->bms_info.slot_state[slot_index];
    
    if (slot_state == BMS_SLOT_EMPTY || slot_state == MBS_SLOT_DISCONNECTED) {
        // Pin empty hoặc disconnected → Hiển thị "-V"
        lv_obj_clear_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(labels[slot_index], "-V");
    } else {
        // Pin connected → Hiển thị giá trị thực
        char buf[16];
        snprintf(buf, sizeof(buf), "%.3fV", me->bms_data[slot_index].stack_volt / 1000.0f);
        lv_obj_clear_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(labels[slot_index], buf);
    }
    
    ui_unlock();
}

void ui_update_main_battery_percent(DeviceHSM_t *me, uint8_t slot_index)
{
    if (slot_index >= TOTAL_SLOT) return;
    
    lv_obj_t *bars[] = {
        ui_barMainBatPercent1, ui_barMainBatPercent2, ui_barMainBatPercent3,
        ui_barMainBatPercent4, ui_barMainBatPercent5
    };
    
    lv_obj_t *soc_labels[] = {
        ui_lbMainSlot1SOC, ui_lbMainSlot2SOC, ui_lbMainSlot3SOC,
        ui_lbMainSlot4SOC, ui_lbMainSlot5SOC
    };
    
    lv_obj_t *warn_imgs[] = {
        ui_imgMainBatWarn1, ui_imgMainBatWarn2, ui_imgMainBatWarn3,
        ui_imgMainBatWarn4, ui_imgMainBatWarn5
    };
    
    if (!ui_lock(-1)) return;
    
    // ❌ TRƯỜNG HỢP 1: MẤT KẾT NỐI BMS
    if (me->is_bms_not_connected) {
        // Ẩn bar
        lv_obj_add_flag(bars[slot_index], LV_OBJ_FLAG_HIDDEN);
        // Ẩn SOC
        lv_obj_add_flag(soc_labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        // Hiện cảnh báo
        lv_obj_clear_flag(warn_imgs[slot_index], LV_OBJ_FLAG_HIDDEN);
        ui_unlock();
        return;
    }
    
    // ✅ CÓ KẾT NỐI → Kiểm tra trạng thái slot
    BMS_Slot_State_t slot_state = me->bms_info.slot_state[slot_index];
    uint8_t soc = me->bms_data[slot_index].soc_percent;
    uint8_t bms_state = me->bms_data[slot_index].bms_state;
    
    if (soc > 100) soc = 100;
    
    // ❌ TRƯỜNG HỢP 2: SLOT EMPTY
    if (slot_state == BMS_SLOT_EMPTY) {
        // Ẩn bar
        lv_obj_add_flag(bars[slot_index], LV_OBJ_FLAG_HIDDEN);
        // Hiện SOC = "-%"
        lv_obj_clear_flag(soc_labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(soc_labels[slot_index], "-%");
        // Ẩn cảnh báo
        lv_obj_add_flag(warn_imgs[slot_index], LV_OBJ_FLAG_HIDDEN);
        ui_unlock();
        return;
    }
    
    // ❌ TRƯỜNG HỢP 3: SLOT DISCONNECTED
    if (slot_state == MBS_SLOT_DISCONNECTED) {
        // Ẩn bar
        lv_obj_add_flag(bars[slot_index], LV_OBJ_FLAG_HIDDEN);
        // Hiện SOC = "-%"
        lv_obj_clear_flag(soc_labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(soc_labels[slot_index], "-%");
        // Hiện cảnh báo
        lv_obj_clear_flag(warn_imgs[slot_index], LV_OBJ_FLAG_HIDDEN);
        ui_unlock();
        return;
    }
    
    // ✅ TRƯỜNG HỢP 4: SLOT CONNECTED
    if (slot_state == BMS_SLOT_CONNECTED) {
        // ❌ SUB-CASE: BMS ERROR (state = 5)
        if (bms_state == 5) {
            // Ẩn bar
            lv_obj_add_flag(bars[slot_index], LV_OBJ_FLAG_HIDDEN);
            // Hiện SOC với màu đỏ
            lv_obj_clear_flag(soc_labels[slot_index], LV_OBJ_FLAG_HIDDEN);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d%%", soc);
            lv_label_set_text(soc_labels[slot_index], buf);
            lv_obj_set_style_text_color(soc_labels[slot_index], 
                                       lv_color_hex(COLOR_ERROR), 
                                       LV_PART_MAIN | LV_STATE_DEFAULT);
            // Hiện cảnh báo
            lv_obj_clear_flag(warn_imgs[slot_index], LV_OBJ_FLAG_HIDDEN);
            ui_unlock();
            return;
        }
        
        // ✅ SUB-CASE: BMS NORMAL (state = 2, 3, 4)
        // Ẩn cảnh báo
        lv_obj_add_flag(warn_imgs[slot_index], LV_OBJ_FLAG_HIDDEN);
        
        // Hiện bar
        lv_obj_clear_flag(bars[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(bars[slot_index], soc, LV_ANIM_OFF);
        
        // Hiện SOC
        lv_obj_clear_flag(soc_labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", soc);
        lv_label_set_text(soc_labels[slot_index], buf);
        
        // 🎨 XÁC ĐỊNH MÀU SẮC
        uint32_t color;
        
        // Ưu tiên: SOC < 10% → Đỏ
        if (soc < 10) {
            color = COLOR_LOW_SOC;
        } 
        // Nếu SOC >= 10% → Xét theo bms_state
        else {
            switch (bms_state) {
                case 2:  // Standby
                    color = COLOR_STANDBY;
                    break;
                case 3:  // Load (đang phóng điện)
                    color = COLOR_LOAD;
                    break;
                case 4:  // Charge (đang sạc)
                    color = COLOR_CHARGE;
                    break;
                default:
                    color = COLOR_STANDBY;
                    break;
            }
        }
        
        // ✅ ÁP DỤNG MÀU CHO BAR (indicator)
        lv_obj_set_style_bg_color(bars[slot_index], 
                                  lv_color_hex(color), 
                                  LV_PART_INDICATOR | LV_STATE_DEFAULT);
        
        // ✅ ÁP DỤNG MÀU CHO PERCENT LABEL
        lv_obj_set_style_text_color(soc_labels[slot_index], 
                                    lv_color_hex(color), 
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    ui_unlock();
}

void ui_update_main_slot_capacity(DeviceHSM_t *me, int8_t slot_index)
{
    if (slot_index >= TOTAL_SLOT) return;
    
    lv_obj_t *labels[] = {
        ui_lbMainCapSlot1, ui_lbMainCapSlot2, ui_lbMainCapSlot3,
        ui_lbMainCapSlot4, ui_lbMainCapSlot5
    };
    
    if (!ui_lock(-1)) return;
    
    // ❌ Kiểm tra mất kết nối BMS
    if (me->is_bms_not_connected) {
        lv_obj_add_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        ui_unlock();
        return;
    }
    
    // ✅ Có kết nối → Kiểm tra trạng thái slot
    BMS_Slot_State_t slot_state = me->bms_info.slot_state[slot_index];
    
    if (slot_state == BMS_SLOT_EMPTY || slot_state == MBS_SLOT_DISCONNECTED) {
        // Pin empty hoặc disconnected → Hiển thị "-mAh"
        lv_obj_clear_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(labels[slot_index], "-mAh");
    } else {
        // Pin connected → Hiển thị giá trị thực
        char buf[16];
        snprintf(buf, sizeof(buf), "%dmAh", me->bms_data[slot_index].capacity);
        lv_obj_clear_flag(labels[slot_index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(labels[slot_index], buf);
    }
    
    ui_unlock();
}
/*--------------------------------------------------------------------*/
/* SLOT DETAIL PANEL VISIBILITY */
/*--------------------------------------------------------------------*/

void ui_show_slot_serial_detail(uint8_t slot_num)
{
    static uint8_t last_slot = 0;
    if (slot_num == last_slot) return;
    
    if (!ui_lock(-1)) return;
    
    lv_obj_t *slots[] = {
        ui_imgMainSlotSerialDetail1, ui_imgMainSlotSerialDetail2,
        ui_imgMainSlotSerialDetail3, ui_imgMainSlotSerialDetail4,
        ui_imgMainSlotSerialDetail5
    };
    
    // Hide old slot
    if (last_slot > 0 && last_slot <= 5) {
        lv_obj_add_flag(slots[last_slot - 1], LV_OBJ_FLAG_HIDDEN);
    }
    
    // Show new slot
    if (slot_num > 0 && slot_num <= 5) {
        lv_obj_clear_flag(slots[slot_num - 1], LV_OBJ_FLAG_HIDDEN);
    }
    
    last_slot = slot_num;
    ui_unlock();
}

void ui_show_slot_detail_panel(bool show)
{
    static bool last_state = false;
    if (show == last_state) return;
    
    if (!ui_lock(-1)) return;
    
    if (show) {
        lv_obj_clear_flag(ui_imgMainSlotDetalBg, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_imgMainSlotDetalBg, LV_OBJ_FLAG_HIDDEN);
    }
    
    last_state = show;
    ui_unlock();
}





void ui_show_main_not_connect(bool show)
{
    static bool last_state = false;
    if (show == last_state) return;  // Skip nếu không đổi
    
    if (ui_lock(-1)) {
        if (show) {
            lv_obj_clear_flag(ui_imgMainNotConnect, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_imgMainNotConnect, LV_OBJ_FLAG_HIDDEN);
        }
        last_state = show;
        ui_unlock();
    }
}

/*--------------------------------------------------------------------*/
/* OPTIMIZED BATCH UPDATE - ALL SLOTS AT ONCE */
/*--------------------------------------------------------------------*/
void ui_update_all_slots_display(DeviceHSM_t *me)
{
    if (!ui_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock UI");
        return;
    }
    
    // ✅ CHỈ LOCK 1 LẦN CHO TẤT CẢ 5 SLOTS!
    
    // Arrays of UI objects (pre-defined for speed)
    lv_obj_t *voltage_labels[] = {
        ui_lbMainVoltageSlot1, ui_lbMainVoltageSlot2, ui_lbMainVoltageSlot3,
        ui_lbMainVoltageSlot4, ui_lbMainVoltageSlot5
    };
    
    lv_obj_t *capacity_labels[] = {
        ui_lbMainCapSlot1, ui_lbMainCapSlot2, ui_lbMainCapSlot3,
        ui_lbMainCapSlot4, ui_lbMainCapSlot5
    };
    
    lv_obj_t *battery_bars[] = {
        ui_barMainBatPercent1, ui_barMainBatPercent2, ui_barMainBatPercent3,
        ui_barMainBatPercent4, ui_barMainBatPercent5
    };
    
    lv_obj_t *soc_labels[] = {
        ui_lbMainSlot1SOC, ui_lbMainSlot2SOC, ui_lbMainSlot3SOC,
        ui_lbMainSlot4SOC, ui_lbMainSlot5SOC
    };
    
    lv_obj_t *warn_images[] = {
        ui_imgMainBatWarn1, ui_imgMainBatWarn2, ui_imgMainBatWarn3,
        ui_imgMainBatWarn4, ui_imgMainBatWarn5
    };
    
    // Buffer tái sử dụng (giảm malloc)
    char buf[16];
    
    // ✅ LOOP QUA TẤT CẢ 5 SLOTS
    for (uint8_t i = 0; i < TOTAL_SLOT; i++) {
        BMS_Data_t *bms = &me->bms_data[i];
        BMS_Slot_State_t slot_state = me->bms_info.slot_state[i];
        uint8_t soc = bms->soc_percent;
        if (soc > 100) soc = 100;
        
        // ============================================
        // CASE 1: MẤT KẾT NỐI BMS
        // ============================================
        if (me->is_bms_not_connected) {
            lv_obj_add_flag(voltage_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(capacity_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(battery_bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(soc_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(warn_images[i], LV_OBJ_FLAG_HIDDEN);
            continue;  // Skip to next slot
        }
        
        // ============================================
        // CASE 2: SLOT EMPTY
        // ============================================
        if (slot_state == BMS_SLOT_EMPTY) {
            // Voltage
            lv_obj_clear_flag(voltage_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(voltage_labels[i], "-V");
            
            // Capacity
            lv_obj_clear_flag(capacity_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(capacity_labels[i], "-mAh");
            
            // Bar & SOC
            lv_obj_add_flag(battery_bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(soc_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(soc_labels[i], "-%");
            
            // Warning
            lv_obj_add_flag(warn_images[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        
        // ============================================
        // CASE 3: SLOT DISCONNECTED
        // ============================================
        if (slot_state == MBS_SLOT_DISCONNECTED) {
            // Voltage
            lv_obj_clear_flag(voltage_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(voltage_labels[i], "-V");
            
            // Capacity
            lv_obj_clear_flag(capacity_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(capacity_labels[i], "-mAh");
            
            // Bar & SOC
            lv_obj_add_flag(battery_bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(soc_labels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(soc_labels[i], "-%");
            
            // Warning
            lv_obj_clear_flag(warn_images[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        
        // ============================================
        // CASE 4: SLOT CONNECTED
        // ============================================
        if (slot_state == BMS_SLOT_CONNECTED) {
            // --- Update Voltage ---
            lv_obj_clear_flag(voltage_labels[i], LV_OBJ_FLAG_HIDDEN);
            snprintf(buf, sizeof(buf), "%.3fV", bms->stack_volt / 1000.0f);
            lv_label_set_text(voltage_labels[i], buf);
            
            // --- Update Capacity ---
            lv_obj_clear_flag(capacity_labels[i], LV_OBJ_FLAG_HIDDEN);
            snprintf(buf, sizeof(buf), "%dmAh", bms->capacity);
            lv_label_set_text(capacity_labels[i], buf);
            
            // --- Check BMS State ---
            uint8_t bms_state = bms->bms_state;
            
            // SUB-CASE: ERROR STATE
            if (bms_state == 5) {
                lv_obj_add_flag(battery_bars[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(soc_labels[i], LV_OBJ_FLAG_HIDDEN);
                snprintf(buf, sizeof(buf), "%d%%", soc);
                lv_label_set_text(soc_labels[i], buf);
                lv_obj_set_style_text_color(soc_labels[i], 
                                           lv_color_hex(COLOR_ERROR),  // Red
                                           LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_clear_flag(warn_images[i], LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            
            // SUB-CASE: NORMAL STATE (2=Standby, 3=Load, 4=Charge)
            lv_obj_add_flag(warn_images[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(battery_bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(battery_bars[i], soc, LV_ANIM_OFF);
            
            lv_obj_clear_flag(soc_labels[i], LV_OBJ_FLAG_HIDDEN);
            snprintf(buf, sizeof(buf), "%d%%", soc);
            lv_label_set_text(soc_labels[i], buf);
            
            // --- Determine Color ---
            uint32_t color;
            
            if (soc < 10) {
                // Priority: Low SOC → Red
                color = COLOR_LOW_SOC;
            } else {
                // Based on BMS state
                switch (bms_state) {
                    case 2:  // Standby → Gray
                        color = COLOR_STANDBY;
                        break;
                    case 3:  // Load → Green
                        color = COLOR_LOAD;
                        break;
                    case 4:  // Charge → Blue
                        color = COLOR_CHARGE;
                        break;
                    default:
                        color = COLOR_OTHER;
                        break;
                }
            }
            
            // Apply color to bar indicator
            lv_obj_set_style_bg_color(battery_bars[i], 
                                      lv_color_hex(color), 
                                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
            
            // Apply color to SOC label
            lv_obj_set_style_text_color(soc_labels[i], 
                                        lv_color_hex(color), 
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    ui_unlock();  // ✅ CHỈ UNLOCK 1 LẦN!
}
