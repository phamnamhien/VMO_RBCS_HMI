/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESP_TICKS_H
#define ESP_TICKS_H

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timer mode: one-shot or periodic
 */
typedef enum {
    TICK_ONCE = 0,      /*!< One-shot timer mode */
    TICK_PERIODIC = 1   /*!< Periodic timer mode */
} tick_type_t;

/**
 * @brief Opaque timer handle structure
 */
typedef struct tick_s *tick_handle_t;

/**
 * @brief Timer callback function type
 * 
 * @param arg User argument passed to callback
 */
typedef void (*tick_callback_t)(void *arg);

#ifndef CONFIG_TICKS_MAX_TIMERS
#define CONFIG_TICKS_MAX_TIMERS 8  /*!< Maximum concurrent timers */
#endif

/**
 * @brief Initialize the ticks timer system
 * 
 * This function must be called before any other ticks API.
 * It initializes the ESP32 timer and starts the tick processing.
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NO_MEM: Failed to allocate memory
 *     - ESP_FAIL: Timer initialization failed
 */
esp_err_t ticks_init(void);

/**
 * @brief Deinitialize the ticks timer system
 * 
 * Stops all timers and releases resources.
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Deinitialization failed
 */
esp_err_t ticks_deinit(void);

/**
 * @brief Create a software timer
 * 
 * @param[out] out_handle Pointer to receive the timer handle
 * @param callback Function to call on timer expiry
 * @param type Timer type (TICK_ONCE or TICK_PERIODIC)
 * @param arg User argument to pass to callback
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid parameter
 *     - ESP_ERR_NO_MEM: No free timer slots available
 */
esp_err_t ticks_create(tick_handle_t *out_handle, 
                       tick_callback_t callback,
                       tick_type_t type, 
                       void *arg);

/**
 * @brief Start or restart a timer
 * 
 * @param handle Timer handle
 * @param timeout_ms Timeout in milliseconds
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid parameter
 */
esp_err_t ticks_start(tick_handle_t handle, uint32_t timeout_ms);

/**
 * @brief Stop a running timer
 * 
 * @param handle Timer handle
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid parameter
 */
esp_err_t ticks_stop(tick_handle_t handle);

/**
 * @brief Delete a timer
 * 
 * @param handle Timer handle
 * 
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid parameter
 */
esp_err_t ticks_delete(tick_handle_t handle);

/**
 * @brief Get current system tick count in milliseconds
 * 
 * @return Current tick count since ticks_init()
 */
uint32_t ticks_get(void);

/**
 * @brief Check if a timer is active
 * 
 * @param handle Timer handle
 * 
 * @return true if timer is active, false otherwise
 */
bool ticks_is_active(tick_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ESP_TICKS_H */
