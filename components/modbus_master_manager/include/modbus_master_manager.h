#ifndef MODBUS_MASTER_MANAGER_H
#define MODBUS_MASTER_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h> 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modbus Master configuration structure
 */
typedef struct {
    int uart_port;          // UART port (UART_NUM_1, UART_NUM_2)
    int tx_pin;             // TX GPIO pin
    int rx_pin;             // RX GPIO pin
    int rts_pin;            // RTS GPIO pin (DE/RE for RS485)
    uint32_t baudrate;      // Baudrate (9600, 19200, 115200...)
} modbus_master_config_t;

/**
 * @brief Callback when new Modbus data is received
 * 
 * @param slave_addr Slave address
 * @param reg_type Register type (0x03=Holding, 0x04=Input)
 * @param reg_addr Starting register address
 * @param data Pointer to data
 * @param length Number of registers
 */
typedef void (*modbus_master_data_callback_t)(uint8_t slave_addr, uint8_t reg_type, 
                                              uint16_t reg_addr, uint16_t *data, 
                                              uint16_t length);

/**
 * @brief Initialize Modbus Master Manager
 * 
 * @param config Pointer to configuration
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_init(const modbus_master_config_t *config);

/**
 * @brief Deinitialize Modbus Master Manager
 * 
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_deinit(void);

/**
 * @brief Register callback for data reception
 * 
 * @param callback Callback function
 */
void modbus_master_register_callback(modbus_master_data_callback_t callback);

/**
 * @brief Read Holding Registers (FC 0x03)
 * 
 * @param slave_addr Slave address
 * @param reg_addr Starting register address
 * @param reg_count Number of registers
 * @param data Buffer to store read data
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_read_holding_registers(uint8_t slave_addr, uint16_t reg_addr, 
                                               uint16_t reg_count, uint16_t *data);

/**
 * @brief Read Input Registers (FC 0x04)
 * 
 * @param slave_addr Slave address
 * @param reg_addr Starting register address
 * @param reg_count Number of registers
 * @param data Buffer to store read data
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_read_input_registers(uint8_t slave_addr, uint16_t reg_addr, 
                                             uint16_t reg_count, uint16_t *data);

/**
 * @brief Write Single Register (FC 0x06)
 * 
 * @param slave_addr Slave address
 * @param reg_addr Register address
 * @param value Value to write
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_write_single_register(uint8_t slave_addr, uint16_t reg_addr, 
                                              uint16_t value);

/**
 * @brief Write Multiple Registers (FC 0x10)
 * 
 * @param slave_addr Slave address
 * @param reg_addr Starting register address
 * @param reg_count Number of registers
 * @param data Data to write
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_write_multiple_registers(uint8_t slave_addr, uint16_t reg_addr,
                                                 uint16_t reg_count, uint16_t *data);

/**
 * @brief Read Coils (FC 0x01)
 * 
 * @param slave_addr Slave address
 * @param coil_addr Starting coil address
 * @param coil_count Number of coils
 * @param data Buffer to store status (bit packed)
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_read_coils(uint8_t slave_addr, uint16_t coil_addr,
                                   uint16_t coil_count, uint8_t *data);

/**
 * @brief Write Single Coil (FC 0x05)
 * 
 * @param slave_addr Slave address
 * @param coil_addr Coil address
 * @param value true=ON, false=OFF
 * @return ESP_OK if successful
 */
esp_err_t modbus_master_write_single_coil(uint8_t slave_addr, uint16_t coil_addr,
                                         bool value);

/**
 * @brief Check if Modbus Master Manager is running
 * 
 * @return true if running
 */
bool modbus_master_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // MODBUS_MASTER_MANAGER_H

