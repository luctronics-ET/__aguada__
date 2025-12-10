/**
 * @file main.c
 * @brief AGUADA Node Sensor v2.0 - Main application
 *
 * Architecture: Component-based with FreeRTOS tasks
 * - sensor_task: Reads ultrasonic and GPIO sensors
 * - comm_task: Handles ESP-NOW transmission
 * - health_task: Monitors system health metrics
 * - watchdog_task: Resets watchdog timer
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "aguada_protocol.h"
#include "aguada_sensor.h"
#include "aguada_comm.h"

static const char *TAG = "AGUADA_MAIN";

/* Task parameters */
#define TASK_SENSOR_STACK 4096
#define TASK_COMM_STACK 4096
#define TASK_HEALTH_STACK 2048
#define TASK_WATCHDOG_STACK 2048

#define TASK_SENSOR_PRIO 5
#define TASK_COMM_PRIO 4
#define TASK_HEALTH_PRIO 3
#define TASK_WATCHDOG_PRIO 6

/* Timing */
#define SENSOR_READ_INTERVAL_MS 30000 // 30 seconds
#define HEALTH_INTERVAL_MS 60000      // 60 seconds
#define WATCHDOG_TIMEOUT_SEC 60       // 60 seconds

/* Event bits */
#define EVENT_SENSOR_READY BIT0
#define EVENT_COMM_READY BIT1

/* Global state */
static EventGroupHandle_t s_event_group;
static aguada_sensor_data_t s_last_sensor_data = {0};
static aguada_health_t s_health = {0};
static uint8_t s_node_mac[6];
static char s_node_mac_str[18];
static bool s_watchdog_enabled = false;

/* Last transmitted values (for deadband logic) */
static int32_t s_last_distance = -1;
static uint8_t s_last_valve_in = 255;
static uint8_t s_last_valve_out = 255;
static uint8_t s_last_sound_in = 255;

/**
 * @brief Check if value changed beyond deadband
 */
static bool value_changed(int32_t new_val, int32_t old_val, int32_t deadband)
{
    if (old_val < 0)
        return true; // First reading
    return (abs(new_val - old_val) >= deadband);
}

/**
 * @brief Send individual telemetry variable (AGUADA protocol)
 */
static void send_telemetry(const char *type, int32_t value)
{
    aguada_packet_v1_t packet;

    strcpy(packet.mac, s_node_mac_str);
    strncpy(packet.type, type, sizeof(packet.type) - 1);
    packet.value = value;
    packet.battery = 5000;                          // 5V DC source
    packet.uptime = esp_timer_get_time() / 1000000; // Convert μs to seconds
    packet.rssi = -50;                              // TODO: Read actual RSSI

    esp_err_t ret = aguada_comm_send_v1(&packet);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send %s: %s", type, esp_err_to_name(ret));
    }
}

/**
 * @brief Sensor task - Reads sensors and checks for changes
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started");

    // Wait for comm to be ready
    xEventGroupWaitBits(s_event_group, EVENT_COMM_READY, false, true, portMAX_DELAY);

    // Signal that sensor is ready
    xEventGroupSetBits(s_event_group, EVENT_SENSOR_READY);

    ESP_LOGI(TAG, "Starting sensor readings (interval: %d ms)", SENSOR_READ_INTERVAL_MS);

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        // Blink LED to show activity
        aguada_sensor_blink_led(1, 100, 0);

        // Read all sensors
        esp_err_t ret = aguada_sensor_read_all(&s_last_sensor_data);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read sensors: %s", esp_err_to_name(ret));
            goto next_iteration;
        }

        // Check distance (deadband: ±2cm = ±200 in cm×100)
        if (s_last_sensor_data.distance_cm_x100 > 0)
        {
            if (value_changed(s_last_sensor_data.distance_cm_x100, s_last_distance, 200))
            {
                send_telemetry("distance_cm", s_last_sensor_data.distance_cm_x100);
                s_last_distance = s_last_sensor_data.distance_cm_x100;
            }
        }
        else if (s_last_sensor_data.distance_cm_x100 == -1)
        {
            // Timeout - send 0
            if (s_last_distance != 0)
            {
                send_telemetry("distance_cm", 0);
                s_last_distance = 0;
            }
        }
        else if (s_last_sensor_data.distance_cm_x100 == -2)
        {
            // Out of range - send 1
            if (s_last_distance != 1)
            {
                send_telemetry("distance_cm", 1);
                s_last_distance = 1;
            }
        }

        // Check valve_in (send on any state change)
        if (s_last_valve_in == 255 || s_last_sensor_data.valve_in != s_last_valve_in)
        {
            send_telemetry("valve_in", s_last_sensor_data.valve_in);
            s_last_valve_in = s_last_sensor_data.valve_in;
        }

        // Check valve_out
        if (s_last_valve_out == 255 || s_last_sensor_data.valve_out != s_last_valve_out)
        {
            send_telemetry("valve_out", s_last_sensor_data.valve_out);
            s_last_valve_out = s_last_sensor_data.valve_out;
        }

        // Check sound_in
        if (s_last_sound_in == 255 || s_last_sensor_data.sound_in != s_last_sound_in)
        {
            send_telemetry("sound_in", s_last_sensor_data.sound_in);
            s_last_sound_in = s_last_sensor_data.sound_in;
        }

        // Reset watchdog
        if (s_watchdog_enabled)
        {
            esp_task_wdt_reset();
        }

    next_iteration:
        // Wait for next interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

/**
 * @brief Communication task - Manages ESP-NOW (future enhancements)
 */
static void comm_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Communication task started");

    // Signal that comm is ready
    xEventGroupSetBits(s_event_group, EVENT_COMM_READY);

    while (1)
    {
        // Future: Handle queued transmissions, retries, etc.
        // For now, just sleep
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Reset watchdog
        if (s_watchdog_enabled)
        {
            esp_task_wdt_reset();
        }
    }
}

/**
 * @brief Health monitoring task
 */
static void health_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Health task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        // Update health metrics
        s_health.uptime_sec = esp_timer_get_time() / 1000000;
        s_health.free_heap = esp_get_free_heap_size();
        // s_health.temperature = read_internal_temperature();  // Future
        s_health.reboot_reason = esp_reset_reason();

        aguada_comm_stats_t comm_stats;
        if (aguada_comm_get_stats(&comm_stats) == ESP_OK)
        {
            s_health.packets_sent = comm_stats.packets_sent;
            s_health.packets_failed = comm_stats.packets_failed;
        }

        ESP_LOGI(TAG, "Health: uptime=%lu s, heap=%lu B, sent=%lu, failed=%lu",
                 s_health.uptime_sec, s_health.free_heap,
                 s_health.packets_sent, s_health.packets_failed);

        // Reset watchdog
        if (s_watchdog_enabled)
        {
            esp_task_wdt_reset();
        }

        // Wait for next interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HEALTH_INTERVAL_MS));
    }
}

/**
 * @brief Watchdog task - Ensures system responsiveness
 */
static void watchdog_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Watchdog task started");

    // Initialize task watchdog
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true};
    ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(xTaskGetCurrentTaskHandle()));

    s_watchdog_enabled = true;

    while (1)
    {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Reset every 10 seconds
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════");
    ESP_LOGI(TAG, "   AGUADA Node Sensor v2.0");
    ESP_LOGI(TAG, "   Component-based architecture with FreeRTOS");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════");

    // Create event group
    s_event_group = xEventGroupCreate();
    if (!s_event_group)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }

    // Initialize communication (WiFi + ESP-NOW)
    ESP_LOGI(TAG, "Initializing communication...");
    esp_err_t ret = aguada_comm_init(NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize communication: %s", esp_err_to_name(ret));
        return;
    }

    // Get node MAC address
    aguada_comm_get_mac(s_node_mac);
    aguada_mac_to_string(s_node_mac, s_node_mac_str);
    ESP_LOGI(TAG, "Node MAC: %s", s_node_mac_str);

    // Initialize sensors
    ESP_LOGI(TAG, "Initializing sensors...");
    ret = aguada_sensor_init(NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize sensors: %s", esp_err_to_name(ret));
        return;
    }

    // Blink LED 3 times to show initialization success
    aguada_sensor_blink_led(3, 200, 200);

    // Create tasks
    ESP_LOGI(TAG, "Creating tasks...");

    xTaskCreate(sensor_task, "sensor", TASK_SENSOR_STACK, NULL,
                TASK_SENSOR_PRIO, NULL);

    xTaskCreate(comm_task, "comm", TASK_COMM_STACK, NULL,
                TASK_COMM_PRIO, NULL);

    xTaskCreate(health_task, "health", TASK_HEALTH_STACK, NULL,
                TASK_HEALTH_PRIO, NULL);

    xTaskCreate(watchdog_task, "watchdog", TASK_WATCHDOG_STACK, NULL,
                TASK_WATCHDOG_PRIO, NULL);

    // Wait for subsystems to be ready
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           EVENT_SENSOR_READY | EVENT_COMM_READY,
                                           false, true, pdMS_TO_TICKS(5000));

    if ((bits & (EVENT_SENSOR_READY | EVENT_COMM_READY)) ==
        (EVENT_SENSOR_READY | EVENT_COMM_READY))
    {
        ESP_LOGI(TAG, "✓ All subsystems ready");
        ESP_LOGI(TAG, "✓ Starting normal operation");
    }
    else
    {
        ESP_LOGE(TAG, "✗ Timeout waiting for subsystems");
    }
}
