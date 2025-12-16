#include "modbus_master_manager.h"
#include "esp_modbus_master.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "MODBUS_MASTER";

// Internal state
static struct {
    void *master_handle;
    modbus_master_config_t config;
    modbus_master_data_callback_t callback;
    SemaphoreHandle_t mutex;
    bool initialized;
    bool running;
} modbus_master_ctx = {0};

// Private functions
static esp_err_t modbus_master_lock(void)
{
    if (!modbus_master_ctx.mutex) return ESP_ERR_INVALID_STATE;
    return (xSemaphoreTake(modbus_master_ctx.mutex, pdMS_TO_TICKS(5000)) == pdTRUE) 
           ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void modbus_master_unlock(void)
{
    if (modbus_master_ctx.mutex) {
        xSemaphoreGive(modbus_master_ctx.mutex);
    }
}

// Public API implementation
esp_err_t modbus_master_init(const modbus_master_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (modbus_master_ctx.initialized) {
        ESP_LOGW(TAG, "Modbus Master already initialized");
        return ESP_OK;
    }

    // Create mutex
    modbus_master_ctx.mutex = xSemaphoreCreateMutex();
    if (!modbus_master_ctx.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Save configuration
    memcpy(&modbus_master_ctx.config, config, sizeof(modbus_master_config_t));

    // Configure Modbus communication
    mb_communication_info_t comm_info = {0};
    comm_info.ser_opts.mode = MB_RTU;
    comm_info.ser_opts.port = config->uart_port;
    comm_info.ser_opts.baudrate = config->baudrate;
    comm_info.ser_opts.data_bits = UART_DATA_8_BITS;
    comm_info.ser_opts.parity = MB_PARITY_NONE;
    comm_info.ser_opts.stop_bits = UART_STOP_BITS_1;
    comm_info.ser_opts.uid = 0;  // Unused for master

    // ✅ BƯỚC 1: Create Modbus Master
    esp_err_t err = mbc_master_create_serial(&comm_info, &modbus_master_ctx.master_handle);
    if (err != ESP_OK || modbus_master_ctx.master_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create Modbus Master: %s", esp_err_to_name(err));
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // ✅ BƯỚC 2: Set UART pins
    int tx = (config->tx_pin == -1) ? UART_PIN_NO_CHANGE : config->tx_pin;
    int rx = (config->rx_pin == -1) ? UART_PIN_NO_CHANGE : config->rx_pin;
    int rts = (config->rts_pin == -1) ? UART_PIN_NO_CHANGE : config->rts_pin;
    
    err = uart_set_pin(config->uart_port, tx, rx, rts, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // ✅ BƯỚC 3: Set RS485 Half Duplex mode (QUAN TRỌNG!)
    err = uart_set_mode(config->uart_port, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set RS485 mode: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    // ✅ BƯỚC 4: Delay nhỏ
    vTaskDelay(pdMS_TO_TICKS(5));

    // ✅ BƯỚC 5: Start Modbus stack
    err = mbc_master_start(modbus_master_ctx.master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Modbus: %s", esp_err_to_name(err));
        mbc_master_delete(modbus_master_ctx.master_handle);
        vSemaphoreDelete(modbus_master_ctx.mutex);
        return err;
    }

    modbus_master_ctx.initialized = true;
    modbus_master_ctx.running = true;

    ESP_LOGI(TAG, "Modbus Master initialized successfully");
    ESP_LOGI(TAG, "  Port: UART%d, Baudrate: %lu", config->uart_port, config->baudrate);
    ESP_LOGI(TAG, "  TX: GPIO%d, RX: GPIO%d, RTS: GPIO%d", 
             config->tx_pin, config->rx_pin, config->rts_pin);

    return ESP_OK;
}

esp_err_t modbus_master_deinit(void)
{
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

    ESP_LOGI(TAG, "Modbus Master stopped");
    return ESP_OK;
}

void modbus_master_register_callback(modbus_master_data_callback_t callback)
{
    modbus_master_ctx.callback = callback;
}

esp_err_t modbus_master_read_holding_registers(uint8_t slave_addr, uint16_t reg_addr, 
                                               uint16_t reg_count, uint16_t *data)
{
    if (!modbus_master_ctx.initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x03,
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

esp_err_t modbus_master_read_input_registers(uint8_t slave_addr, uint16_t reg_addr, 
                                             uint16_t reg_count, uint16_t *data)
{
    if (!modbus_master_ctx.initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x04,
        .reg_start = reg_addr,
        .reg_size = reg_count
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, data);
    
    if (err == ESP_OK && modbus_master_ctx.callback) {
        modbus_master_ctx.callback(slave_addr, 0x04, reg_addr, data, reg_count);
    }

    modbus_master_unlock();
    return err;
}

esp_err_t modbus_master_write_single_register(uint8_t slave_addr, uint16_t reg_addr, 
                                              uint16_t value)
{
    if (!modbus_master_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x06,
        .reg_start = reg_addr,
        .reg_size = 1
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, &value);
    
    modbus_master_unlock();
    return err;
}

esp_err_t modbus_master_write_multiple_registers(uint8_t slave_addr, uint16_t reg_addr,
                                                 uint16_t reg_count, uint16_t *data)
{
    if (!modbus_master_ctx.initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x10,
        .reg_start = reg_addr,
        .reg_size = reg_count
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, data);
    
    modbus_master_unlock();
    return err;
}

esp_err_t modbus_master_read_coils(uint8_t slave_addr, uint16_t coil_addr,
                                   uint16_t coil_count, uint8_t *data)
{
    if (!modbus_master_ctx.initialized || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x01,
        .reg_start = coil_addr,
        .reg_size = coil_count
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, data);
    
    modbus_master_unlock();
    return err;
}

esp_err_t modbus_master_write_single_coil(uint8_t slave_addr, uint16_t coil_addr,
                                         bool value)
{
    if (!modbus_master_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = modbus_master_lock();
    if (err != ESP_OK) return err;

    uint16_t coil_value = value ? 0xFF00 : 0x0000;
    
    mb_param_request_t request = {
        .slave_addr = slave_addr,
        .command = 0x05,
        .reg_start = coil_addr,
        .reg_size = 1
    };

    err = mbc_master_send_request(modbus_master_ctx.master_handle, &request, &coil_value);
    
    modbus_master_unlock();
    return err;
}

bool modbus_master_is_running(void)
{
    return modbus_master_ctx.running;
}

