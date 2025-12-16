/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ticks_example.c
 * @brief Example usage of ticks library
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ticks.h"

static const char *TAG = "ticks_example";

// User data structures
typedef struct {
    int counter;
    const char *name;
} timer_context_t;

// Periodic timer callback
static void periodic_timer_callback(void *arg)
{
    timer_context_t *ctx = (timer_context_t *)arg;
    ctx->counter++;
    ESP_LOGI(TAG, "[%s] Periodic callback #%d", ctx->name, ctx->counter);
}

// One-shot timer callback
static void oneshot_timer_callback(void *arg)
{
    const char *msg = (const char *)arg;
    ESP_LOGI(TAG, "One-shot timer expired: %s", msg);
}

// LED blink simulation
static void led_blink_callback(void *arg)
{
    static bool led_state = false;
    led_state = !led_state;
    ESP_LOGI(TAG, "LED: %s", led_state ? "ON" : "OFF");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Ticks Example");

    // Initialize ticks system
    ESP_ERROR_CHECK(ticks_init());
    ESP_LOGI(TAG, "Ticks initialized");

    // Example 1: Create periodic timer (1 second)
    timer_context_t periodic_ctx = {
        .counter = 0,
        .name = "Periodic-1s"
    };
    tick_handle_t periodic_timer;
    ESP_ERROR_CHECK(ticks_create(&periodic_timer, 
                                 periodic_timer_callback,
                                 TICK_PERIODIC, 
                                 &periodic_ctx));
    ESP_ERROR_CHECK(ticks_start(periodic_timer, 1000));
    ESP_LOGI(TAG, "Created periodic timer (1s)");

    // Example 2: Create one-shot timer (5 seconds)
    tick_handle_t oneshot_timer;
    ESP_ERROR_CHECK(ticks_create(&oneshot_timer, 
                                 oneshot_timer_callback,
                                 TICK_ONCE, 
                                 (void *)"5 seconds elapsed!"));
    ESP_ERROR_CHECK(ticks_start(oneshot_timer, 5000));
    ESP_LOGI(TAG, "Created one-shot timer (5s)");

    // Example 3: LED blink timer (500ms)
    tick_handle_t led_timer;
    ESP_ERROR_CHECK(ticks_create(&led_timer, 
                                 led_blink_callback,
                                 TICK_PERIODIC, 
                                 NULL));
    ESP_ERROR_CHECK(ticks_start(led_timer, 500));
    ESP_LOGI(TAG, "Created LED blink timer (500ms)");

    // Example 4: Stop timer after 10 seconds
    vTaskDelay(pdMS_TO_TICKS(10000));
    ESP_LOGI(TAG, "Stopping periodic timer...");
    ESP_ERROR_CHECK(ticks_stop(periodic_timer));

    // Example 5: Get system uptime
    uint32_t uptime = ticks_get();
    ESP_LOGI(TAG, "System uptime: %lu ms", uptime);

    // Example 6: Check timer status
    if (ticks_is_active(led_timer)) {
        ESP_LOGI(TAG, "LED timer is still active");
    }

    // Example 7: Delete timers
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Deleting timers...");
    ticks_delete(periodic_timer);
    ticks_delete(oneshot_timer);
    ticks_delete(led_timer);

    // Example 8: Multiple timers
    tick_handle_t multi_timers[3];
    timer_context_t multi_ctx[3];
    
    for (int i = 0; i < 3; i++) {
        multi_ctx[i].counter = 0;
        char name_buf[32];
        snprintf(name_buf, sizeof(name_buf), "Timer-%d", i);
        multi_ctx[i].name = strdup(name_buf);
        
        ESP_ERROR_CHECK(ticks_create(&multi_timers[i], 
                                     periodic_timer_callback,
                                     TICK_PERIODIC, 
                                     &multi_ctx[i]));
        ESP_ERROR_CHECK(ticks_start(multi_timers[i], (i + 1) * 1000));
    }
    ESP_LOGI(TAG, "Created 3 timers with different periods");

    // Keep running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Main loop - uptime: %lu ms", ticks_get());
    }
}
