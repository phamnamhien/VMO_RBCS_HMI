#include "app_states.h"
#include "modbus_master_manager.h"
#include "ui.h"
#include "ui_support.h"

static const char* TAG = "RBCS_HMI";

// ============================================
// Configuration from Kconfig
// ============================================
#if CONFIG_HMI_DOUBLE_FB
#define HMI_LCD_NUM_FB 2
#else
#define HMI_LCD_NUM_FB 1
#endif

// ============================================
// Modbus & HSM Variables
// ============================================
app_state_hsm_t device;

// ============================================
// Global Variables
// ============================================
static SemaphoreHandle_t lvgl_mux = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

// ✅ Semaphores for VSYNC synchronization (Waveshare style)
#if CONFIG_HMI_AVOID_TEAR_EFFECT_WITH_SEM
static SemaphoreHandle_t sem_vsync_end;
static SemaphoreHandle_t sem_gui_ready;
#endif

// ============================================
// SquareLine UI Event Handlers
// ============================================

// ============================================
// Modbus Callbacks & Task
// ============================================// Callback nhận dữ liệu Modbus
void
modbus_data_received(uint8_t slave_addr, uint8_t reg_type, uint16_t reg_addr, uint16_t* data, uint16_t length) {
    // ESP_LOGI(TAG, "Received data from slave %d, type 0x%02X, addr %d:", slave_addr, reg_type, reg_addr);
    // for (int i = 0; i < length; i++) {
    //     ESP_LOGI(TAG, "  [%d] = %d", i, data[i]);
    // }
}

static void
modbus_battery_sync_data(app_state_hsm_t* me, uint16_t* dat, uint8_t slot_index) {
    // ✅ THÊM LOG ĐỂ KIỂM TRA
    ESP_LOGI(TAG, "📥 Syncing data TO slot_index=%d", slot_index);
    ESP_LOGI(TAG, "   Raw data[0]=%d, data[8]=%d", dat[0], dat[8]);

    // ✅ KIỂM TRA THỨ TỰ: Slot 1 phải có volt thấp nhất
    uint16_t expected_min = (slot_index + 1) * 100; // 100, 200, 300, 400, 500
    if (dat[8] != expected_min) {
        ESP_LOGW(TAG, "⚠️  Data mismatch! Expected ~%d but got %d for slot %d", expected_min, dat[8], slot_index + 1);
    }

    me->bms_data[slot_index].bms_state = dat[0];
    me->bms_data[slot_index].ctrl_request = dat[1];
    me->bms_data[slot_index].ctrl_response = dat[2];
    me->bms_data[slot_index].fet_ctrl_pin = dat[3];
    me->bms_data[slot_index].fet_status = dat[4];
    me->bms_data[slot_index].alarm_bits = dat[5];
    me->bms_data[slot_index].faults = dat[6];

    me->bms_data[slot_index].pack_volt = dat[7];
    me->bms_data[slot_index].stack_volt = dat[8];
    me->bms_data[slot_index].cell_volt[0] = dat[18];
    me->bms_data[slot_index].cell_volt[1] = dat[19];
    me->bms_data[slot_index].cell_volt[2] = dat[20];
    me->bms_data[slot_index].cell_volt[3] = dat[21];
    me->bms_data[slot_index].cell_volt[4] = dat[22];
    me->bms_data[slot_index].cell_volt[5] = dat[23];
    me->bms_data[slot_index].cell_volt[6] = dat[24];
    me->bms_data[slot_index].cell_volt[7] = dat[25];
    me->bms_data[slot_index].cell_volt[8] = dat[26];
    me->bms_data[slot_index].cell_volt[9] = dat[27];
    me->bms_data[slot_index].cell_volt[10] = dat[28];
    me->bms_data[slot_index].cell_volt[11] = dat[29];
    me->bms_data[slot_index].cell_volt[12] = dat[33];

    me->bms_data[slot_index].ld_volt = dat[11];
    me->bms_data[slot_index].pack_current = dat[9] << 16 | dat[10];
    me->bms_data[slot_index].temp1 = dat[12] << 16 | dat[13];
    me->bms_data[slot_index].temp2 = dat[12] << 16 | dat[13];
    me->bms_data[slot_index].temp3 = dat[14] << 16 | dat[15];

    me->bms_data[slot_index].capacity = dat[48];
    me->bms_data[slot_index].soc_percent = dat[46];
    me->bms_data[slot_index].soh_value = dat[47];
    me->bms_data[slot_index].pin_percent = dat[43];
    me->bms_data[slot_index].percent_target = dat[44];
    me->bms_data[slot_index].safety_a = dat[34];
    me->bms_data[slot_index].safety_b = dat[35];
    me->bms_data[slot_index].safety_c = dat[36];

    me->bms_data[slot_index].accu_int = dat[37] << 16 | dat[38];
    me->bms_data[slot_index].accu_frac = dat[39] << 16 | dat[40];
    me->bms_data[slot_index].accu_time = dat[41] << 16 | dat[42];

    me->bms_data[slot_index].cell_resistance = dat[45];
    me->bms_data[slot_index].single_parallel = dat[49];

    // ✅ IN RA SAU KHI SYNC
    ESP_LOGI(TAG, "   Stored in bms_data[%d].stack_volt = %d", slot_index, me->bms_data[slot_index].stack_volt);

    // ✅ SAU ĐÓ MỚI VALIDATION
    if (dat[8] == 0) {
        ESP_LOGW(TAG, "⚠️  stack_volt = 0 for slot %d", slot_index);
    }
}

static void
modbus_bms_information_sync_data(app_state_hsm_t* me, uint16_t* dat) {
    me->bms_info.slot_state[IDX_SLOT_1] = dat[0];
    me->bms_info.slot_state[IDX_SLOT_2] = dat[1];
    me->bms_info.slot_state[IDX_SLOT_3] = dat[2];
    me->bms_info.slot_state[IDX_SLOT_4] = dat[3];
    me->bms_info.slot_state[IDX_SLOT_5] = dat[4];

    me->bms_info.swap_state = dat[5];
    me->bms_info.manual_swap_request= dat[6];
    me->bms_info.complete_swap = dat[7];
}


#define USE_MODBUS_MASTER_DEBUG 0
void
modbus_poll_task(void* arg) {
    // ✅ MỖI SLOT CÓ BUFFER RIÊNG
    uint16_t slot1_regs[60];
    uint16_t slot2_regs[60];
    uint16_t slot3_regs[60];
    uint16_t slot4_regs[60];
    uint16_t slot5_regs[60];
    uint16_t station_regs[60];
    
    uint8_t consecutive_errors = 0;
    bool need_reset = false;

    while (1) {
        // ===== KIỂM TRA CẦN RESET =====
        if (need_reset) {
            // ESP_LOGW(TAG, "🔄 Resetting Modbus stack...");
            modbus_master_reset();
            need_reset = false;
            consecutive_errors = 0;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // ===== SLOT 1 =====
        esp_err_t err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_SLOT1_START_REG, MB_SLOT1_NUMBER_OF_REGS, slot1_regs);
        if (err == ESP_OK) {
            modbus_battery_sync_data(&device, slot1_regs, IDX_SLOT_1);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_1_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== SLOT 2 =====
        err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_SLOT2_START_REG, MB_SLOT2_NUMBER_OF_REGS, slot2_regs);
        if (err == ESP_OK) {
            modbus_battery_sync_data(&device, slot2_regs, IDX_SLOT_2);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_2_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== SLOT 3 =====
        err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_SLOT3_START_REG, MB_SLOT3_NUMBER_OF_REGS, slot3_regs);
        if (err == ESP_OK) {
            modbus_battery_sync_data(&device, slot3_regs, IDX_SLOT_3);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_3_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== SLOT 4 =====
        err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_SLOT4_START_REG, MB_SLOT4_NUMBER_OF_REGS, slot4_regs);
        if (err == ESP_OK) {
            modbus_battery_sync_data(&device, slot4_regs, IDX_SLOT_4);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_4_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== SLOT 5 =====
        err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_SLOT5_START_REG, MB_SLOT5_NUMBER_OF_REGS, slot5_regs);
        if (err == ESP_OK) {
            modbus_battery_sync_data(&device, slot5_regs, IDX_SLOT_5);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_5_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== STATION STATE =====
        err = modbus_master_read_holding_registers(APP_MODBUS_SLAVE_ID, MB_COMMON_START_REG, MB_COMMON_NUMBER_OF_REGS, station_regs);
        if (err == ESP_OK) {
            modbus_bms_information_sync_data(&device, station_regs);
            consecutive_errors = 0;
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_STATION_STATE_DATA, NULL);
        } else {
            consecutive_errors++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // ===== XỬ LÝ LỖI =====
        if (consecutive_errors >= 10) {
            // ESP_LOGE(TAG, "⚠️  Too many errors (%d) - will reset stack", consecutive_errors);
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_NOTCONNECTED, NULL);
            need_reset = true;
        } else if (consecutive_errors > 0) {
            hsm_dispatch((hsm_t *)&device, HEVT_MODBUS_GET_SLOT_DATA, NULL);
        }
    }
}

// ============================================
// LCD & LVGL Callbacks (Waveshare Style)
// ============================================
static bool
lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* event_data, void* user_data) {
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_HMI_AVOID_TEAR_EFFECT_WITH_SEM
    // ✅ CHỈ give sem_vsync_end KHI gui_ready đã được set
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

static void
lcd_lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsety1 = area->y1;
    int offsetx2 = area->x2;
    int offsety2 = area->y2;

#if CONFIG_HMI_AVOID_TEAR_EFFECT_WITH_SEM
    // ✅ BÁO GUI đã sẵn sàng
    xSemaphoreGive(sem_gui_ready);
    // ✅ ĐỢI VSYNC xác nhận
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
#endif

    // Vẽ bitmap
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

    lv_disp_flush_ready(drv);
}

static void
lcd_increase_lvgl_tick(void* arg) {
    lv_tick_inc(LCD_LVGL_TICK_PERIOD_MS);
}

static void 
lvgl_touch_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    esp_lcd_touch_point_data_t points[1];   // struct chứa dữ liệu cảm ứng
    uint8_t touchpad_cnt = 0;

    // Đọc dữ liệu từ driver
    esp_lcd_touch_read_data(drv->user_data);

    // Lấy dữ liệu cảm ứng
    esp_err_t ret = esp_lcd_touch_get_data(drv->user_data, points, &touchpad_cnt, 1);

    if (ret == ESP_OK && touchpad_cnt > 0) {
        data->point.x = points[0].x;
        data->point.y = points[0].y;
        data->state   = LV_INDEV_STATE_PR;   // đang chạm
    } else {
        data->state   = LV_INDEV_STATE_REL;  // không chạm
    }
}



// ============================================
// LVGL Port Task
// ============================================
static void
lcd_lvgl_port_task(void* arg) {
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = LCD_LVGL_TASK_MAX_DELAY_MS;

    while (1) {
        if (ui_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            ui_unlock();
        }

        if (task_delay_ms > LCD_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LCD_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LCD_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LCD_LVGL_TASK_MIN_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// ============================================
// I2C Initialization
// ============================================
static esp_err_t
i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// ============================================
// Touch Controller Initialization
// ============================================
static esp_err_t
touch_controller_init(esp_lcd_touch_handle_t* tp_out) {
    ESP_LOGI(TAG, "Initializing GT911 touch controller...");

    // 1. Init GPIO 4 for INT control
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_INPUT_IO_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
    };
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. Enable I/O expander 0x24
    uint8_t write_buf = 0x01;
    esp_err_t ret =
        i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I/O expander 0x24 not responding: %s", esp_err_to_name(ret));
    }

    // 3. GT911 reset sequence via 0x38
    write_buf = 0x2C;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Touch reset controller 0x38 not responding: %s", esp_err_to_name(ret));
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(GPIO_INPUT_IO_4, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    write_buf = 0x2E;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(200));

    // 4. Init touch controller
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create touch panel I/O: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_V_RES,
        .y_max = LCD_H_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .flags =
            {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
    };

    esp_lcd_touch_handle_t tp = NULL;
    ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init GT911: %s", esp_err_to_name(ret));
        return ret;
    }

    *tp_out = tp;
    ESP_LOGI(TAG, "GT911 initialized successfully");
    return ESP_OK;
}

// ============================================
// Main Application
// ============================================
void
app_main(void) {
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  📊 INITIAL MEMORY STATE");
    ESP_LOGI(TAG, "===========================================");

    // Total vs Free PSRAM
    size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t used_psram = total_psram - free_psram;

    ESP_LOGI(TAG, "  PSRAM Total:      %u KB", total_psram / 1024);
    ESP_LOGI(TAG, "  PSRAM Used:       %u KB (%.1f%%)", used_psram / 1024, (used_psram * 100.0) / total_psram);
    ESP_LOGI(TAG, "  PSRAM Free:       %u KB (%.1f%%)", free_psram / 1024, (free_psram * 100.0) / total_psram);

    // Largest free block
    size_t largest_free = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "  Largest block:    %u KB", largest_free / 1024);

    // Internal RAM
    ESP_LOGI(TAG, "  Internal RAM:     %u KB", esp_get_free_heap_size() / 1024);
    ESP_LOGI(TAG, "===========================================");

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  RBCS HMI - Battery Charging Station");
    ESP_LOGI(TAG, "===========================================");

#if CONFIG_HMI_AVOID_TEAR_EFFECT_WITH_SEM
    // ========================================
    // STEP 1: Create Semaphores
    // ========================================
    ESP_LOGI(TAG, "[1/9] Creating synchronization semaphores...");
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);
    ESP_LOGI(TAG, "      Semaphores created for tear effect avoidance");
#else
    ESP_LOGI(TAG, "[1/9] Skipping semaphores (tear effect avoidance disabled)");
#endif

#if LCD_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << LCD_PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    // ========================================
    // STEP 2: Install RGB LCD Panel
    // ========================================
    ESP_LOGI(TAG, "[2/9] Installing RGB LCD panel driver...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16,
        .psram_trans_align = 64,
        .num_fbs = HMI_LCD_NUM_FB,
#if CONFIG_HMI_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = LCD_PIN_NUM_DISP_EN,
        .pclk_gpio_num = LCD_PIN_NUM_PCLK,
        .vsync_gpio_num = LCD_PIN_NUM_VSYNC,
        .hsync_gpio_num = LCD_PIN_NUM_HSYNC,
        .de_gpio_num = LCD_PIN_NUM_DE,
        .data_gpio_nums =
            {
                LCD_PIN_NUM_DATA0,
                LCD_PIN_NUM_DATA1,
                LCD_PIN_NUM_DATA2,
                LCD_PIN_NUM_DATA3,
                LCD_PIN_NUM_DATA4,
                LCD_PIN_NUM_DATA5,
                LCD_PIN_NUM_DATA6,
                LCD_PIN_NUM_DATA7,
                LCD_PIN_NUM_DATA8,
                LCD_PIN_NUM_DATA9,
                LCD_PIN_NUM_DATA10,
                LCD_PIN_NUM_DATA11,
                LCD_PIN_NUM_DATA12,
                LCD_PIN_NUM_DATA13,
                LCD_PIN_NUM_DATA14,
                LCD_PIN_NUM_DATA15,
            },
        .timings =
            {
                .pclk_hz = LCD_PIXEL_CLOCK_HZ,
                .h_res = LCD_H_RES,
                .v_res = LCD_V_RES,
                .hsync_back_porch = 8,
                .hsync_front_porch = 8,
                .hsync_pulse_width = 4,
                .vsync_back_porch = 16,
                .vsync_front_porch = 16,
                .vsync_pulse_width = 4,
                .flags.pclk_active_neg = true,
            },
        .flags.fb_in_psram = true,
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_LOGI(TAG, "      RGB panel created (num_fbs=%d)", HMI_LCD_NUM_FB);

#if LCD_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(LCD_PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
#endif
    // ========================================
    // STEP 3: Initialize I2C
    // ========================================
    ESP_LOGI(TAG, "[3/9] Initializing I2C...");
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "      I2C initialized");

    // ========================================
    // STEP 4: Register VSYNC Callback
    // ========================================
    ESP_LOGI(TAG, "[4/9] Registering VSYNC event callbacks...");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = lcd_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));
    ESP_LOGI(TAG, "      VSYNC callback registered");

    // ========================================
    // STEP 5: Initialize RGB LCD Panel
    // ========================================
    ESP_LOGI(TAG, "[5/9] Initializing RGB LCD panel...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_LOGI(TAG, "      RGB LCD panel initialized");

    // ========================================
    // STEP 6: Initialize LVGL
    // ========================================
    ESP_LOGI(TAG, "[6/9] Initializing LVGL library...");
    lv_init();

    void* buf1 = NULL;
    void* buf2 = NULL;

#if CONFIG_HMI_DOUBLE_FB
    ESP_LOGI(TAG, "      Using frame buffers as LVGL draw buffers");
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LCD_V_RES);
#else
    ESP_LOGI(TAG, "      Allocating separate LVGL draw buffers from PSRAM");
    buf1 = heap_caps_malloc(LCD_H_RES * 100 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 100);
#endif
    ESP_LOGI(TAG, "      LVGL initialized");

    // ========================================
    // STEP 7: Register Display Driver
    // ========================================
    ESP_LOGI(TAG, "[7/9] Registering display driver to LVGL...");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lcd_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
#if CONFIG_HMI_DOUBLE_FB
    disp_drv.full_refresh = true;
    ESP_LOGI(TAG, "      Full refresh mode enable (double buffer)");
#endif
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
    ESP_LOGI(TAG, "      Display driver registered (%dx%d)", LCD_H_RES, LCD_V_RES);

    // ========================================
    // STEP 8: Initialize Touch Controller
    // ========================================
    ESP_LOGI(TAG, "[8/9] Initializing touch controller...");
    esp_err_t touch_ret = touch_controller_init(&touch_handle);

    if (touch_ret == ESP_OK) {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.disp = disp;
        indev_drv.read_cb = lvgl_touch_cb;
        indev_drv.user_data = touch_handle;
        lv_indev_drv_register(&indev_drv);
        ESP_LOGI(TAG, "      Touch controller initialized");
    } else {
        ESP_LOGW(TAG, "      Touch controller disabled");
    }

    // ========================================
    // STEP 9: Create LVGL Infrastructure
    // ========================================
    ESP_LOGI(TAG, "[9/9] Creating LVGL infrastructure...");

    // LVGL tick timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &lcd_increase_lvgl_tick, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LCD_LVGL_TICK_PERIOD_MS * 1000));

    // LVGL mutex
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    ui_support_init(lvgl_mux);

    // LVGL task
    xTaskCreate(lcd_lvgl_port_task, "LVGL", LCD_LVGL_TASK_STACK_SIZE, NULL, LCD_LVGL_TASK_PRIORITY, NULL);
    ESP_LOGI(TAG, "      LVGL infrastructure created");

    // ========================================
    // Initialize SquareLine UI
    // ========================================
    ESP_LOGI(TAG, "Initializing SquareLine UI...");
    if (ui_lock(-1)) {
        ui_init();

        // // Register event callbacks
        // lv_obj_add_event_cb(ui_ibtMainToSetting, fnMainSetting, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_ibtSettingBackToMain, fnSettingBackToMain, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_btMainSlot1, fnMainSlot1, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_btMainSlot2, fnMainSlot2, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_btMainSlot3, fnMainSlot3, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_btMainSlot4, fnMainSlot4, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_btMainSlot5, fnMainSlot5, LV_EVENT_CLICKED, NULL);
        // lv_obj_add_event_cb(ui_ibtMainManualSwap, fnMainManualSwap, LV_EVENT_CLICKED, NULL);

        ui_unlock();
    }
    ESP_LOGI(TAG, "      SquareLine UI initialized");
    // ========================================
    // Initialize HSM (State Machine)
    // ========================================
    ESP_LOGI(TAG, "Initializing HSM...");
    memset(&device, 0, sizeof(app_state_hsm_t));
    app_state_hsm_init(&device);
    ESP_LOGI(TAG, "      HSM initialized");

    // ========================================
    // Initialize Modbus RTU Master
    // ========================================
    ESP_LOGI(TAG, "Initializing Modbus RTU Master...");
    // ✅ IN RA GIÁ TRỊ ENUM ĐỂ KIỂM TRA
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  📋 ENUM VALUES CHECK");
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  IDX_SLOT_1 = %d", IDX_SLOT_1);
    ESP_LOGI(TAG, "  IDX_SLOT_2 = %d", IDX_SLOT_2);
    ESP_LOGI(TAG, "  IDX_SLOT_3 = %d", IDX_SLOT_3);
    ESP_LOGI(TAG, "  IDX_SLOT_4 = %d", IDX_SLOT_4);
    ESP_LOGI(TAG, "  IDX_SLOT_5 = %d", IDX_SLOT_5);
    ESP_LOGI(TAG, "  TOTAL_SLOT = %d", TOTAL_SLOT);
    ESP_LOGI(TAG, "===========================================");

    // ✅ IN RA ĐỊA CHỈ BMS_DATA ARRAY
    modbus_master_config_t modbus_cfg = {
        .uart_port = APP_IO_UART_NUM,
        .tx_pin = APP_IO_UART_TX_PIN,
        .rx_pin = APP_IO_UART_RX_PIN,
        .rts_pin = APP_IO_UART_RTS_PIN,
        .baudrate = 115200,
    };

    ESP_LOGI(TAG, "  📦 BMS_DATA ARRAY ADDRESSES");
    ESP_LOGI(TAG, "===========================================");
    for (int i = 0; i < TOTAL_SLOT; i++) {
        ESP_LOGI(TAG, "  bms_data[%d] @ %p", i, &device.bms_data[i]);
    }
    ESP_LOGI(TAG, "===========================================");

    esp_err_t modbus_ret = modbus_master_init(&modbus_cfg);
    if (modbus_ret == ESP_OK) {
        modbus_master_register_callback(modbus_data_received);
        ESP_LOGI(TAG, "      Modbus initialized");
        
        vTaskDelay(pdMS_TO_TICKS(500)); // ✅ ĐỢI modbus stack ready
        
        xTaskCreate(modbus_poll_task, "modbus_poll", 4096, NULL, 4, NULL);
        ESP_LOGI(TAG, "      Modbus task created");
    } else {
        ESP_LOGE(TAG, "      Modbus FAILED: %s", esp_err_to_name(modbus_ret));
    }
    // ========================================
    // System Startup Complete
    // ========================================
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  ✓ System Startup Completed!");
    ESP_LOGI(TAG, "===========================================");

#if CONFIG_HMI_DOUBLE_FB
    ESP_LOGI(TAG, "  LCD:    %dx%d RGB (Double Buffer)", LCD_H_RES, LCD_V_RES);
#else
    ESP_LOGI(TAG, "  LCD:    %dx%d RGB (Single Buffer)", LCD_H_RES, LCD_V_RES);
#endif

    ESP_LOGI(TAG, "  Touch:  %s", touch_ret == ESP_OK ? "GT911 Active" : "Disabled");
    ESP_LOGI(TAG, "  Modbus: %s", modbus_ret == ESP_OK ? "Active" : "Disabled");
    ESP_LOGI(TAG, "  HSM:    Running");

#if CONFIG_HMI_AVOID_TEAR_EFFECT_WITH_SEM
    ESP_LOGI(TAG, "  Sync:   Semaphore-based VSYNC");
#endif
#if CONFIG_HMI_USE_BOUNCE_BUFFER
    ESP_LOGI(TAG, "  Buffer: Bounce buffer enabled");
#endif

    ESP_LOGI(TAG, "-------------------------------------------");
    ESP_LOGI(TAG, "  Free heap:  %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Free PSRAM: %lu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "===========================================");
}

void fnbacktomainbutton(lv_event_t * e) {
    ESP_LOGI(TAG, "Back To Main Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_BACK_TO_MAIN, NULL);
}

// Main Screen
void fnscrmainbatterybuttonclicked(lv_event_t * e) {
    ESP_LOGI(TAG, "Main Goto Detail Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_MAIN_TO_DETAIL, NULL);
}

void fnscrmainmanualbuttonclicked(lv_event_t * e) {
    ESP_LOGI(TAG, "Main Goto Manual 1 Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_MAIN_TO_MANUAL1, NULL);
}    

// Detail Screen
void fnscrdetailbatterybuttonclicked(lv_event_t * e) {
    ESP_LOGI(TAG, "Detail Goto Main Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_DETAIL_TO_MAIN, NULL);
}

void fnscrdetailmanualbuttonclicked(lv_event_t * e) {
    ESP_LOGI(TAG, "Detail Goto Manual 1 Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_DETAIL_TO_MANUAL1, NULL);
}    

void fnscrdetailbacktomainbuttonclicked(lv_event_t * e) {
    ESP_LOGI(TAG, "Detail Goto Main Screen");
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_DETAIL_TO_MAIN, NULL);
}    
void fnscrdetailnextslotgasture(lv_event_t * e) {
    ESP_LOGI(TAG, "Detail Next Slot Data");
    if(++device.present_slot_display > TOTAL_SLOT - 1) {
        device.present_slot_display = IDX_SLOT_1;
    }
    lv_obj_set_style_opa(ui_scrdetaildataslottitlelabel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue3, LV_OPA_TRANSP, 0);

    scrdetaildataslottitlelabel_update(device.present_slot_display);
    scrdetaildataslotvalue_update(
                        &device.bms_data[device.present_slot_display],
                        device.bms_info.slot_state[device.present_slot_display]);
    scrdetailslotssttcontainer_update(
                        device.bms_info.slot_state,
                        device.bms_data,
                        device.present_slot_display);
}
void fnscrdetailbackslotgasture(lv_event_t * e) {
    ESP_LOGI(TAG, "Detail Back Slot Data");
    if((int8_t)--device.present_slot_display < IDX_SLOT_1) {
        device.present_slot_display = TOTAL_SLOT - 1;
    }
    lv_obj_set_style_opa(ui_scrdetaildataslottitlelabel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(ui_scrdetaildataslotvalue3, LV_OPA_TRANSP, 0);

    scrdetaildataslottitlelabel_update(device.present_slot_display);
    scrdetaildataslotvalue_update(
                        &device.bms_data[device.present_slot_display],
                        device.bms_info.slot_state[device.present_slot_display]);
    scrdetailslotssttcontainer_update(
                        device.bms_info.slot_state,
                        device.bms_data,
                        device.present_slot_display);
}

// Manual1 Screen
void fnscrmanual1selectbat1(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL1_SELECT_BAT1, NULL);
}
void fnscrmanual1selectbat2(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL1_SELECT_BAT1, NULL);
}
void fnscrmanual1selectslot1(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL2_SELECT_SLOT1, NULL);
}
void fnscrmanual1selectslot2(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL2_SELECT_SLOT2, NULL);
}
void fnscrmanual1selectslot3(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL2_SELECT_SLOT3, NULL);
}
void fnscrmanual1selectslot4(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL2_SELECT_SLOT4, NULL);
}
void fnscrmanual1selectslot5(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_MANUAL2_SELECT_SLOT5, NULL);
}


void backtomainscrevt(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_TRANS_BACK_TO_MAIN, NULL);
}    

void scrprocessprbuttonclicked(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_PROCESS_PR_BUTTON_CLICKED, NULL);
}    
void scrprocessstbuttonclicked(lv_event_t * e) {
    hsm_dispatch((hsm_t *)&device, HEVT_PROCESS_ST_BUTTON_CLICKED, NULL);
}