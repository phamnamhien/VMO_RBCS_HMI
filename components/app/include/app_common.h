#ifndef APP_IO_H
#define APP_IO_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h> 


#ifdef __cplusplus
extern "C" {
#endif

#define APP_IO_UART_NUM         UART_NUM_2  
#define APP_IO_UART_TX_PIN      16  // RS485_TX
#define APP_IO_UART_RX_PIN      15  // RS485_RX
#define APP_IO_UART_RTS_PIN     -1  // Auto direction switching
#define APP_MODBUS_SLAVE_ID     1   // Modbus slave ID for BMS
// ============================================
// Hardware Pin Definitions
// ============================================

// I2C Touch Controller
#define I2C_MASTER_SCL_IO           9
#define I2C_MASTER_SDA_IO           8
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4             4
#define GPIO_INPUT_PIN_SEL          (1ULL << GPIO_INPUT_IO_4)

// LCD RGB Interface
#define LCD_PIXEL_CLOCK_HZ          (18 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL       1
#define LCD_BK_LIGHT_OFF_LEVEL      !LCD_BK_LIGHT_ON_LEVEL
#define LCD_PIN_NUM_BK_LIGHT        -1
#define LCD_PIN_NUM_HSYNC           46
#define LCD_PIN_NUM_VSYNC           3
#define LCD_PIN_NUM_DE              5
#define LCD_PIN_NUM_PCLK            7
#define LCD_PIN_NUM_DATA0           14  // B3
#define LCD_PIN_NUM_DATA1           38  // B4
#define LCD_PIN_NUM_DATA2           18  // B5
#define LCD_PIN_NUM_DATA3           17  // B6
#define LCD_PIN_NUM_DATA4           10  // B7
#define LCD_PIN_NUM_DATA5           39  // G2
#define LCD_PIN_NUM_DATA6           0   // G3
#define LCD_PIN_NUM_DATA7           45  // G4
#define LCD_PIN_NUM_DATA8           48  // G5
#define LCD_PIN_NUM_DATA9           47  // G6
#define LCD_PIN_NUM_DATA10          21  // G7
#define LCD_PIN_NUM_DATA11          1   // R3
#define LCD_PIN_NUM_DATA12          2   // R4
#define LCD_PIN_NUM_DATA13          42  // R5
#define LCD_PIN_NUM_DATA14          41  // R6
#define LCD_PIN_NUM_DATA15          40  // R7
#define LCD_PIN_NUM_DISP_EN         -1


#ifdef __cplusplus
}
#endif

#endif // APP_IO_H

