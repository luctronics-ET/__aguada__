/**
 * @file aguada_sensor.c
 * @brief AGUADA Sensor Interface - Implementation
 */

#include "aguada_sensor.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "AGUADA_SENSOR";

static aguada_sensor_config_t s_config;
static bool s_initialized = false;

/* Comparator for qsort */
static int compare_int32(const void *a, const void *b)
{
    return (*(int32_t *)a - *(int32_t *)b);
}

aguada_sensor_config_t aguada_sensor_get_default_config(void)
{
    aguada_sensor_config_t config = {
        .trig_pin = SENSOR_TRIG_PIN,
        .echo_pin = SENSOR_ECHO_PIN,
        .valve_in_pin = SENSOR_VALVE_IN,
        .valve_out_pin = SENSOR_VALVE_OUT,
        .sound_in_pin = SENSOR_SOUND_IN,
        .led_pin = SENSOR_LED_STATUS,
        .timeout_us = SENSOR_TIMEOUT_US,
        .deadband_cm = SENSOR_DEADBAND_CM};
    return config;
}

esp_err_t aguada_sensor_init(const aguada_sensor_config_t *config)
{
    if (s_initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    if (config)
    {
        memcpy(&s_config, config, sizeof(aguada_sensor_config_t));
    }
    else
    {
        s_config = aguada_sensor_get_default_config();
    }

    ESP_LOGI(TAG, "Initializing sensors...");

    // Configure ultrasonic TRIG pin (output)
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << s_config.trig_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&trig_conf));
    gpio_set_level(s_config.trig_pin, 0);

    // Configure ultrasonic ECHO pin (input)
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << s_config.echo_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&echo_conf));

    // Configure digital inputs (valves, sound)
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << s_config.valve_in_pin) |
                        (1ULL << s_config.valve_out_pin) |
                        (1ULL << s_config.sound_in_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable pull-up for digital inputs
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&input_conf));

    // Configure LED pin (output)
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << s_config.led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&led_conf));
    gpio_set_level(s_config.led_pin, 0);

    s_initialized = true;
    ESP_LOGI(TAG, "Sensors initialized (TRIG=%d, ECHO=%d, VALVE_IN=%d, VALVE_OUT=%d, SOUND=%d, LED=%d)",
             s_config.trig_pin, s_config.echo_pin, s_config.valve_in_pin,
             s_config.valve_out_pin, s_config.sound_in_pin, s_config.led_pin);

    return ESP_OK;
}

int32_t aguada_sensor_read_distance(void)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Sensor not initialized");
        return -1;
    }

    // Send 10μs trigger pulse
    gpio_set_level(s_config.trig_pin, 0);
    ets_delay_us(2);
    gpio_set_level(s_config.trig_pin, 1);
    ets_delay_us(10);
    gpio_set_level(s_config.trig_pin, 0);

    // Wait for echo pulse to start (timeout protection)
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(s_config.echo_pin) == 0)
    {
        if ((esp_timer_get_time() - start_time) > s_config.timeout_us)
        {
            ESP_LOGW(TAG, "Ultrasonic timeout (no echo start)");
            return -1; // Timeout - sensor didn't respond
        }
    }

    // Measure echo pulse duration
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(s_config.echo_pin) == 1)
    {
        if ((esp_timer_get_time() - pulse_start) > s_config.timeout_us)
        {
            ESP_LOGW(TAG, "Ultrasonic timeout (echo too long)");
            return -1; // Timeout
        }
    }
    int64_t pulse_end = esp_timer_get_time();

    uint32_t duration_us = (uint32_t)(pulse_end - pulse_start);

    // Calculate distance in cm × 100
    // Speed of sound: 343 m/s = 0.0343 cm/μs
    // Distance = (duration × 0.0343) / 2
    // Simplified: (duration × 343) / 20000
    int32_t distance_cm_x100 = (int32_t)((duration_us * 343) / 20000);

    // Validate range
    if (distance_cm_x100 < (SENSOR_MIN_DISTANCE_CM * SENSOR_VALUE_MULTIPLIER) ||
        distance_cm_x100 > (SENSOR_MAX_DISTANCE_CM * SENSOR_VALUE_MULTIPLIER))
    {
        ESP_LOGW(TAG, "Distance out of range: %ld.%02ld cm",
                 distance_cm_x100 / 100, distance_cm_x100 % 100);
        return -2; // Out of range
    }

    ESP_LOGD(TAG, "Distance: %ld.%02ld cm", distance_cm_x100 / 100, distance_cm_x100 % 100);
    return distance_cm_x100;
}

int32_t aguada_sensor_read_distance_filtered(void)
{
    int32_t samples[SENSOR_MEDIAN_SAMPLES];
    int valid_count = 0;

    ESP_LOGD(TAG, "Taking %d samples for median filter...", SENSOR_MEDIAN_SAMPLES);

    for (int i = 0; i < SENSOR_MEDIAN_SAMPLES; i++)
    {
        int32_t dist = aguada_sensor_read_distance();
        if (dist > 0)
        {
            samples[valid_count++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_SAMPLE_INTERVAL_MS));
    }

    if (valid_count < 5)
    {
        ESP_LOGE(TAG, "Not enough valid samples (%d/%d)", valid_count, SENSOR_MEDIAN_SAMPLES);
        return -1;
    }

    // Sort samples and return median
    qsort(samples, valid_count, sizeof(int32_t), compare_int32);
    int32_t median = samples[valid_count / 2];

    ESP_LOGI(TAG, "Median distance: %ld.%02ld cm (from %d samples)",
             median / 100, median % 100, valid_count);

    return median;
}

uint8_t aguada_sensor_read_digital(gpio_num_t pin)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Sensor not initialized");
        return 0;
    }

    return gpio_get_level(pin);
}

esp_err_t aguada_sensor_read_all(aguada_sensor_data_t *data)
{
    if (!data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Read all sensors
    data->distance_cm_x100 = aguada_sensor_read_distance_filtered();
    data->valve_in = aguada_sensor_read_digital(s_config.valve_in_pin);
    data->valve_out = aguada_sensor_read_digital(s_config.valve_out_pin);
    data->sound_in = aguada_sensor_read_digital(s_config.sound_in_pin);
    data->timestamp_us = esp_timer_get_time();

    ESP_LOGI(TAG, "Sensors: distance=%ld.%02ld cm, valve_in=%d, valve_out=%d, sound=%d",
             data->distance_cm_x100 / 100, data->distance_cm_x100 % 100,
             data->valve_in, data->valve_out, data->sound_in);

    return ESP_OK;
}

void aguada_sensor_set_led(uint8_t state)
{
    if (s_initialized)
    {
        gpio_set_level(s_config.led_pin, state ? 1 : 0);
    }
}

void aguada_sensor_blink_led(uint8_t count, uint32_t on_ms, uint32_t off_ms)
{
    if (!s_initialized)
    {
        return;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        aguada_sensor_set_led(1);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        aguada_sensor_set_led(0);
        if (i < count - 1)
        { // Don't delay after last blink
            vTaskDelay(pdMS_TO_TICKS(off_ms));
        }
    }
}
