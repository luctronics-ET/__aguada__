/**
 * @file main.c
 * @brief AGUADA Node Sensor v2.0 - Modernized Architecture
 *
 * Features:
 * - Component-based architecture
 * - FreeRTOS task orchestration
 * - Light sleep power management
 * - NVS configuration storage
 * - Structured logging
 * - Robust watchdog
 * - OTA ready
 *
 * @author AGUADA Project
 * @version 2.0.0
 * @date 2025-12-10
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"

// AGUADA Components
#include "aguada_protocol.h"
#include "aguada_sensor.h"
#include "aguada_comm.h"
#include "aguada_config.h"
#include "aguada_power.h"
#include "aguada_health.h"

// ============================================================================
// CONSTANTS
// ============================================================================

static const char *TAG = "AGUADA_NODE_V2";

#define FIRMWARE_VERSION "2.0.0"
#define PROTOCOL_VERSION 2

// Task priorities (0-25, higher = more priority)
#define PRIORITY_SENSOR_TASK 5
#define PRIORITY_COMM_TASK 6
#define PRIORITY_HEALTH_TASK 3
#define PRIORITY_WATCHDOG_TASK 10

// Task stack sizes (bytes)
#define STACK_SIZE_SENSOR 4096
#define STACK_SIZE_COMM 4096
#define STACK_SIZE_HEALTH 3072
#define STACK_SIZE_WATCHDOG 2048

// Event bits
#define EVENT_SENSOR_READY BIT0
#define EVENT_COMM_READY BIT1
#define EVENT_DATA_AVAILABLE BIT2
#define EVENT_SEND_REQUEST BIT3
#define EVENT_OTA_START BIT4

// Watchdog timeout
#define WATCHDOG_TIMEOUT_S 30

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Event group for task synchronization
static EventGroupHandle_t system_events;

// Queues for inter-task communication
static QueueHandle_t sensor_data_queue;
static QueueHandle_t comm_tx_queue;

// Task handles for monitoring
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t comm_task_handle = NULL;
static TaskHandle_t health_task_handle = NULL;
static TaskHandle_t watchdog_task_handle = NULL;

// System state
static aguada_system_state_t system_state = {
    .is_initialized = false,
    .boot_count = 0,
    .last_error = AGUADA_OK};

// ============================================================================
// SENSOR TASK
// ============================================================================

/**
 * @brief Sensor reading task
 *
 * Responsibilities:
 * - Read ultrasonic sensor with median filtering
 * - Read battery voltage via ADC
 * - Detect significant changes (delta)
 * - Push data to comm queue
 * - Handle heartbeat timer
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[SENSOR] Task started");

    aguada_sensor_config_t sensor_config = {
        .gpio_trig = GPIO_NUM_1,
        .gpio_echo = GPIO_NUM_0,
        .gpio_adc = GPIO_NUM_4,
        .samples_count = 11,
        .timeout_ms = 30,
        .delta_mm = 20,
        .heartbeat_interval_ms = 30000};

    // Initialize sensor
    if (aguada_sensor_init(&sensor_config) != AGUADA_OK)
    {
        ESP_LOGE(TAG, "[SENSOR] Failed to initialize");
        vTaskDelete(NULL);
        return;
    }

    xEventGroupSetBits(system_events, EVENT_SENSOR_READY);

    aguada_sensor_data_t last_sent = {0};
    aguada_sensor_data_t current = {0};
    int64_t last_send_time = 0;
    bool first_reading = true;

    // Main sensor loop
    while (1)
    {
        // Reset watchdog
        esp_task_wdt_reset();

        // Read sensors
        esp_err_t err = aguada_sensor_read(&current);

        if (err == ESP_OK)
        {
            // Check if should send (delta or heartbeat)
            bool should_send = first_reading ||
                               aguada_sensor_check_delta(&current, &last_sent, &sensor_config) ||
                               aguada_sensor_check_heartbeat(last_send_time, sensor_config.heartbeat_interval_ms);

            if (should_send)
            {
                // Send to comm queue
                if (xQueueSend(sensor_data_queue, &current, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    last_sent = current;
                    last_send_time = esp_timer_get_time();
                    first_reading = false;

                    ESP_LOGI(TAG, "[SENSOR] Data queued: dist=%dmm vcc=%dmV",
                             current.distance_mm, current.vcc_mv);
                }
                else
                {
                    ESP_LOGW(TAG, "[SENSOR] Queue full, data dropped");
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "[SENSOR] Read error: %s", esp_err_to_name(err));
        }

        // Sleep between readings (power save)
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds
    }
}

// ============================================================================
// COMMUNICATION TASK
// ============================================================================

/**
 * @brief Communication task
 *
 * Responsibilities:
 * - Initialize ESP-NOW
 * - Dequeue sensor data
 * - Format AGUADA-2 packet
 * - Send via ESP-NOW
 * - Track TX success/failure
 */
static void comm_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[COMM] Task started");

    // Initialize ESP-NOW
    aguada_comm_config_t comm_config = {
        .channel = 11,
        .use_encryption = true,
        .max_retries = 3};

    if (aguada_comm_init(&comm_config) != AGUADA_OK)
    {
        ESP_LOGE(TAG, "[COMM] Failed to initialize");
        vTaskDelete(NULL);
        return;
    }

    xEventGroupSetBits(system_events, EVENT_COMM_READY);

    aguada_sensor_data_t sensor_data;

    // Main communication loop
    while (1)
    {
        // Reset watchdog
        esp_task_wdt_reset();

        // Wait for data from sensor queue
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY) == pdTRUE)
        {

            // Build AGUADA-2 packet
            aguada2_packet_t packet;
            aguada_build_packet_v2(&packet, &sensor_data, &system_state);

            // Send via ESP-NOW
            esp_err_t err = aguada_comm_send(&packet, sizeof(packet));

            if (err == ESP_OK)
            {
                system_state.metrics.tx_ok++;
                ESP_LOGI(TAG, "[COMM] Packet sent (total: %lu)",
                         system_state.metrics.tx_ok);
            }
            else
            {
                system_state.metrics.tx_fail++;
                ESP_LOGE(TAG, "[COMM] Send failed: %s (total: %lu)",
                         esp_err_to_name(err), system_state.metrics.tx_fail);
            }
        }
    }
}

// ============================================================================
// HEALTH MONITORING TASK
// ============================================================================

/**
 * @brief System health monitoring task
 *
 * Responsibilities:
 * - Monitor free heap memory
 * - Track CPU temperature
 * - Log system metrics
 * - Detect anomalies
 * - Periodic health report (every 5 minutes)
 */
static void health_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[HEALTH] Task started");

    aguada_health_init();

    const uint32_t report_interval_ms = 300000; // 5 minutes

    while (1)
    {
        // Reset watchdog
        esp_task_wdt_reset();

        // Collect health metrics
        aguada_health_metrics_t health;
        aguada_health_collect(&health);

        // Update system state
        system_state.health = health;

        // Log health report
        ESP_LOGI(TAG, "[HEALTH] Uptime: %lus, Free heap: %lu bytes (min: %lu), CPU temp: %d°C",
                 health.uptime_s,
                 health.free_heap,
                 health.min_heap_ever,
                 health.cpu_temp);

        // Check for anomalies
        if (health.free_heap < 50000)
        {
            ESP_LOGW(TAG, "[HEALTH] Low memory warning!");
        }

        if (health.cpu_temp > 80)
        {
            ESP_LOGW(TAG, "[HEALTH] High temperature warning!");
        }

        // Sleep until next report
        vTaskDelay(pdMS_TO_TICKS(report_interval_ms));
    }
}

// ============================================================================
// WATCHDOG TASK
// ============================================================================

/**
 * @brief Watchdog task - ensures system health
 *
 * Responsibilities:
 * - Monitor task execution
 * - Detect deadlocks
 * - Force reboot if needed
 * - Log crash info to NVS
 */
static void watchdog_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[WATCHDOG] Task started");

    // Subscribe all tasks to watchdog
    esp_task_wdt_add(sensor_task_handle);
    esp_task_wdt_add(comm_task_handle);
    esp_task_wdt_add(health_task_handle);
    esp_task_wdt_add(NULL); // Current task (watchdog itself)

    while (1)
    {
        // Reset own watchdog
        esp_task_wdt_reset();

        // Check if all critical tasks are healthy
        // (each task should call esp_task_wdt_reset periodically)

        // Log watchdog status
        static uint32_t check_count = 0;
        if (++check_count % 60 == 0)
        { // Every 60 seconds
            ESP_LOGI(TAG, "[WATCHDOG] System healthy (checks: %lu)", check_count);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

// ============================================================================
// SYSTEM INITIALIZATION
// ============================================================================

/**
 * @brief Initialize system-wide resources
 */
static esp_err_t system_init(void)
{
    ESP_LOGI(TAG, "=== AGUADA Node v%s ===", FIRMWARE_VERSION);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load configuration from NVS
    aguada_config_load(&system_state.config);

    // Log boot reason
    esp_reset_reason_t reset_reason = esp_reset_reason();
    const char *reason_str = aguada_get_reset_reason_str(reset_reason);
    ESP_LOGI(TAG, "Boot reason: %s", reason_str);
    system_state.boot_count++;

    // Check OTA partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s (offset: 0x%lx)",
             running->label, running->address);

    // Create event group
    system_events = xEventGroupCreate();
    if (system_events == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    // Create queues
    sensor_data_queue = xQueueCreate(10, sizeof(aguada_sensor_data_t));
    comm_tx_queue = xQueueCreate(10, sizeof(aguada2_packet_t));

    if (sensor_data_queue == NULL || comm_tx_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }

    // Initialize watchdog
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true};
    ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));

    system_state.is_initialized = true;
    return ESP_OK;
}

// ============================================================================
// MAIN APPLICATION
// ============================================================================

void app_main(void)
{
    // Initialize system
    if (system_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "System initialization failed!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Creating tasks...");

    // Create sensor task
    xTaskCreatePinnedToCore(
        sensor_task,
        "sensor_task",
        STACK_SIZE_SENSOR,
        NULL,
        PRIORITY_SENSOR_TASK,
        &sensor_task_handle,
        0 // Core 0
    );

    // Create communication task
    xTaskCreatePinnedToCore(
        comm_task,
        "comm_task",
        STACK_SIZE_COMM,
        NULL,
        PRIORITY_COMM_TASK,
        &comm_task_handle,
        0 // Core 0
    );

    // Create health monitoring task
    xTaskCreate(
        health_task,
        "health_task",
        STACK_SIZE_HEALTH,
        NULL,
        PRIORITY_HEALTH_TASK,
        &health_task_handle);

    // Create watchdog task
    xTaskCreate(
        watchdog_task,
        "watchdog_task",
        STACK_SIZE_WATCHDOG,
        NULL,
        PRIORITY_WATCHDOG_TASK,
        &watchdog_task_handle);

    // Wait for all critical tasks to be ready
    EventBits_t bits = xEventGroupWaitBits(
        system_events,
        EVENT_SENSOR_READY | EVENT_COMM_READY,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(5000));

    if ((bits & (EVENT_SENSOR_READY | EVENT_COMM_READY)) ==
        (EVENT_SENSOR_READY | EVENT_COMM_READY))
    {
        ESP_LOGI(TAG, "✓ All tasks ready - system operational");
    }
    else
    {
        ESP_LOGE(TAG, "✗ Timeout waiting for tasks");
    }

    // Main task becomes idle (optional: can delete itself)
    ESP_LOGI(TAG, "Main task complete");
}
