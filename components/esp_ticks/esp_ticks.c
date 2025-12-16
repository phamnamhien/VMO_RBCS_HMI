/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_ticks.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "ticks";

/**
 * @brief Internal timer structure
 */
typedef struct tick_s {
    tick_callback_t callback;   /*!< Callback function */
    tick_type_t type;           /*!< Timer type */
    void *arg;                  /*!< User argument */
    uint32_t period_ms;         /*!< Period in milliseconds */
    uint32_t countdown_ms;      /*!< Remaining milliseconds */
    bool is_active;             /*!< Active flag */
} tick_t;

/**
 * @brief Ticks manager structure
 */
typedef struct {
    tick_t timers[CONFIG_TICKS_MAX_TIMERS];
    esp_timer_handle_t esp_timer;
    volatile uint32_t system_ticks;
    SemaphoreHandle_t mutex;
    bool initialized;
} ticks_manager_t;

static ticks_manager_t s_ticks_mgr = {0};

/**
 * @brief Process all active timers (called every 1ms)
 */
static void ticks_process_timers(void)
{
    if (xSemaphoreTake(s_ticks_mgr.mutex, 0) != pdTRUE) {
        return;  // Skip if mutex is busy
    }

    s_ticks_mgr.system_ticks++;

    for (uint8_t i = 0; i < CONFIG_TICKS_MAX_TIMERS; i++) {
        tick_t *timer = &s_ticks_mgr.timers[i];
        
        if (timer->is_active && timer->callback) {
            if (--timer->countdown_ms == 0) {
                // Timer expired
                if (timer->type == TICK_ONCE) {
                    timer->is_active = false;
                }
                
                // Call callback
                timer->callback(timer->arg);
                
                // Reload for periodic timers
                if (timer->type == TICK_PERIODIC) {
                    timer->countdown_ms = timer->period_ms;
                }
            }
        }
    }

    xSemaphoreGive(s_ticks_mgr.mutex);
}

/**
 * @brief ESP timer callback (1ms periodic)
 */
static void ticks_esp_timer_callback(void *arg)
{
    ticks_process_timers();
}

esp_err_t ticks_init(void)
{
    if (s_ticks_mgr.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Clear timer array
    memset(&s_ticks_mgr, 0, sizeof(ticks_manager_t));

    // Create mutex
    s_ticks_mgr.mutex = xSemaphoreCreateMutex();
    if (s_ticks_mgr.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Create ESP timer (1ms periodic)
    const esp_timer_create_args_t timer_args = {
        .callback = ticks_esp_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ticks_timer",
        .skip_unhandled_events = true
    };

    esp_err_t err = esp_timer_create(&timer_args, &s_ticks_mgr.esp_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(err));
        vSemaphoreDelete(s_ticks_mgr.mutex);
        return err;
    }

    // Start timer (1000 us = 1 ms)
    err = esp_timer_start_periodic(s_ticks_mgr.esp_timer, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        esp_timer_delete(s_ticks_mgr.esp_timer);
        vSemaphoreDelete(s_ticks_mgr.mutex);
        return err;
    }

    s_ticks_mgr.initialized = true;
    ESP_LOGI(TAG, "Initialized successfully");
    
    return ESP_OK;
}

esp_err_t ticks_deinit(void)
{
    if (!s_ticks_mgr.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Stop and delete timer
    if (s_ticks_mgr.esp_timer) {
        esp_timer_stop(s_ticks_mgr.esp_timer);
        esp_timer_delete(s_ticks_mgr.esp_timer);
        s_ticks_mgr.esp_timer = NULL;
    }

    // Delete mutex
    if (s_ticks_mgr.mutex) {
        vSemaphoreDelete(s_ticks_mgr.mutex);
        s_ticks_mgr.mutex = NULL;
    }

    s_ticks_mgr.initialized = false;
    ESP_LOGI(TAG, "Deinitialized");
    
    return ESP_OK;
}

esp_err_t ticks_create(tick_handle_t *out_handle, 
                       tick_callback_t callback,
                       tick_type_t type, 
                       void *arg)
{
    if (!s_ticks_mgr.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (out_handle == NULL || callback == NULL) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_ticks_mgr.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }

    // Find free slot
    tick_handle_t handle = NULL;
    for (uint8_t i = 0; i < CONFIG_TICKS_MAX_TIMERS; i++) {
        if (s_ticks_mgr.timers[i].callback == NULL) {
            handle = &s_ticks_mgr.timers[i];
            handle->callback = callback;
            handle->type = type;
            handle->arg = arg;
            handle->period_ms = 0;
            handle->countdown_ms = 0;
            handle->is_active = false;
            break;
        }
    }

    xSemaphoreGive(s_ticks_mgr.mutex);

    if (handle == NULL) {
        ESP_LOGE(TAG, "No free timer slots");
        return ESP_ERR_NO_MEM;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t ticks_start(tick_handle_t handle, uint32_t timeout_ms)
{
    if (!s_ticks_mgr.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (handle == NULL || timeout_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_ticks_mgr.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }

    handle->period_ms = timeout_ms;
    handle->countdown_ms = timeout_ms;
    handle->is_active = true;

    xSemaphoreGive(s_ticks_mgr.mutex);
    
    return ESP_OK;
}

esp_err_t ticks_stop(tick_handle_t handle)
{
    if (!s_ticks_mgr.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_ticks_mgr.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }

    handle->is_active = false;

    xSemaphoreGive(s_ticks_mgr.mutex);
    
    return ESP_OK;
}

esp_err_t ticks_delete(tick_handle_t handle)
{
    if (!s_ticks_mgr.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_ticks_mgr.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }

    handle->is_active = false;
    handle->callback = NULL;
    handle->arg = NULL;
    handle->period_ms = 0;
    handle->countdown_ms = 0;
    handle->type = TICK_ONCE;

    xSemaphoreGive(s_ticks_mgr.mutex);
    
    return ESP_OK;
}

uint32_t ticks_get(void)
{
    return s_ticks_mgr.system_ticks;
}

bool ticks_is_active(tick_handle_t handle)
{
    if (!s_ticks_mgr.initialized || handle == NULL) {
        return false;
    }
    return handle->is_active;
}
