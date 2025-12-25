#include "modbus_master_manager.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "mbcontroller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char* TAG = "MODBUS_MASTER";

static struct {
    void* master_handle;  // ✅ Context handle
    modbus_master_config_t config;
    modbus_master_data_callback_t callback;
    SemaphoreHandle_t mutex;
    bool initialized;
    bool running;
} modbus_master_ctx = {0};

static esp_err_t
modbus_master_lock(void) {
    if (!modbus_master_ctx.mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    return (xSemaphoreTake(modbus_master_ctx.mutex, pdMS_TO_TICKS(5000)) == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void
modbus_master_unlock(void) {
    if (modbus_master_ctx.mutex) {
        xSemaphoreGive(modbus_master_ctx.mutex);
    }
}

esp_err_t
modbus_master_init(const modbus_master_config_t* config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (modbus_master_ctx.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    modbus_master_ctx.mutex = xSemaphoreCreateMutex();
    if (!modbus_master_ctx.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    memcpy(&modbus_master_ctx.config, config, sizeof(modbus_master_config_t));

    // ✅ KHAI BÁO VÀ GÁN DESCRIPTOR
    static mb_parameter_descriptor_t device_params = {0};
    device_params.cid = 0;
    device_params.param_key = "dummy";
    device_params.param_units = "";
    device_params.mb_slave_addr = 1;
    device_params.mb_param_type = MB_PARAM_HOLDING;
    device_params.mb_reg_start = 0;
    device_params.mb_size = 1;
    device_params.param_offset = 0;
    device_params.param_type = PARAM_TYPE_U16;
    device_params.param_size = 2;

    // Configure communication
    mb_communication_info_t comm_info = {0};
    comm_info.ser_opts.mode = MB_RTU;
    comm_info.ser_opts.port = config->uart_port;
    comm_info.ser_opts.baudrate = config->baudrate;
    comm_info.ser_opts.data_bits = UART_DATA_8_BITS;
    comm_info.ser_opts.parity = UART_PARITY_DISABLE;
    comm_info.ser_opts.stop_bits = UART_STOP_BITS_1;
    comm_info.ser_opts.uid = 0;

    // Create master
    esp_err_t err = mbc_master_create_serial(&comm_info, &modbus_master_ctx.master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_master_create_serial failed: %s", esp_err_to_name(err));
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // Set UART pins
    err = uart_set_pin(config->uart_port,
                       config->tx_pin,
                       config->rx_pin,
                       config->rts_pin,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // RS485 mode
    err = uart_set_mode(config->uart_port, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_mode failed: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    // Set descriptor
    err = mbc_master_set_descriptor(modbus_master_ctx.master_handle, &device_params, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_master_set_descriptor failed: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // Start
    err = mbc_master_start(modbus_master_ctx.master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_master_start failed: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    modbus_master_ctx.initialized = true;
    modbus_master_ctx.running = true;

    ESP_LOGI(TAG, "✅ Modbus Master initialized");
    ESP_LOGI(TAG, "   UART%d @ %lu baud", config->uart_port, config->baudrate);
    ESP_LOGI(TAG, "   TX=%d RX=%d RTS=%d", config->tx_pin, config->rx_pin, config->rts_pin);

    return ESP_OK;
}

esp_err_t
modbus_master_deinit(void) {
    if (!modbus_master_ctx.initialized) {
        return ESP_OK;
    }

    modbus_master_lock();
    modbus_master_ctx.running = false;

    if (modbus_master_ctx.master_handle) {
        mbc_master_stop(modbus_master_ctx.master_handle);
        mbc_master_delete(modbus_master_ctx.master_handle);
        modbus_master_ctx.master_handle = NULL;
    }

    modbus_master_ctx.initialized = false;
    modbus_master_ctx.callback = NULL;
    modbus_master_unlock();

    if (modbus_master_ctx.mutex) {
        vSemaphoreDelete(modbus_master_ctx.mutex);
        modbus_master_ctx.mutex = NULL;
    }

    ESP_LOGI(TAG, "Modbus stopped");
    return ESP_OK;
}

void
modbus_master_register_callback(modbus_master_data_callback_t callback) {
    modbus_master_ctx.callback = callback;
}

esp_err_t
modbus_master_read_holding_registers(uint8_t slave_addr, uint16_t reg_addr, uint16_t reg_count, uint16_t* data) {
    if (!modbus_master_ctx.initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) {
        return err;
    }

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x03,  // ✅ Function code 3
        .reg_start = reg_addr,
        .reg_size = reg_count
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, data);

    if (err == ESP_OK && modbus_master_ctx.callback) {
        modbus_master_ctx.callback(slave_addr, 0x03, reg_addr, data, reg_count);
    }

    modbus_master_unlock();
    return err;
}

esp_err_t
modbus_master_write_single_register(uint8_t slave_addr, uint16_t reg_addr, uint16_t value) {
    if (!modbus_master_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) {
        return err;
    }

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x06,  // ✅ Function code 6
        .reg_start = reg_addr,
        .reg_size = 1
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, &value);

    modbus_master_unlock();
    return err;
}

bool
modbus_master_is_running(void) {
    return modbus_master_ctx.running;
}