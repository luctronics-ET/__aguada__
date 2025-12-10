/**
 * @file aguada_sensor.h
 * @brief AGUADA Sensor Interface - Ultrasonic and GPIO management
 */

#ifndef AGUADA_SENSOR_H
#define AGUADA_SENSOR_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO Pin definitions for ESP32-C3 SuperMini */
#define SENSOR_TRIG_PIN     GPIO_NUM_1
#define SENSOR_ECHO_PIN     GPIO_NUM_0
#define SENSOR_VALVE_IN     GPIO_NUM_2
#define SENSOR_VALVE_OUT    GPIO_NUM_3
#define SENSOR_SOUND_IN     GPIO_NUM_5
#define SENSOR_LED_STATUS   GPIO_NUM_8

/* Ultrasonic sensor parameters */
#define SENSOR_TIMEOUT_US       30000    // 30ms timeout
#define SENSOR_MIN_DISTANCE_CM  20       // AJ-SR04M min range
#define SENSOR_MAX_DISTANCE_CM  450      // AJ-SR04M max range
#define SENSOR_DEADBAND_CM      2        // ±2cm deadband

/* Median filter configuration */
#define SENSOR_MEDIAN_SAMPLES   11       // Number of samples for median
#define SENSOR_SAMPLE_INTERVAL_MS 200    // Interval between samples

/* Value multiplier for integer transmission */
#define SENSOR_VALUE_MULTIPLIER 100      // cm → cm×100

/**
 * @brief Sensor configuration
 */
typedef struct {
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
    gpio_num_t valve_in_pin;
    gpio_num_t valve_out_pin;
    gpio_num_t sound_in_pin;
    gpio_num_t led_pin;
    uint32_t timeout_us;
    uint16_t deadband_cm;
} aguada_sensor_config_t;

/**
 * @brief Sensor readings
 */
typedef struct {
    int32_t distance_cm_x100;   // Distance in cm × 100
    uint8_t valve_in;           // Valve input state (0/1)
    uint8_t valve_out;          // Valve output state (0/1)
    uint8_t sound_in;           // Sound detector state (0/1)
    int64_t timestamp_us;       // Timestamp in microseconds
} aguada_sensor_data_t;

/**
 * @brief Initialize sensor subsystem
 * 
 * Configures GPIO pins for ultrasonic sensor and digital inputs
 * 
 * @param config Sensor configuration (NULL for defaults)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_sensor_init(const aguada_sensor_config_t *config);

/**
 * @brief Read ultrasonic distance (single measurement)
 * 
 * @return int32_t Distance in cm×100, or negative on error:
 *         -1: timeout (sensor didn't respond)
 *         -2: out of range (< 20cm or > 450cm)
 */
int32_t aguada_sensor_read_distance(void);

/**
 * @brief Read ultrasonic distance with median filter
 * 
 * Takes multiple samples and returns median value to filter noise
 * 
 * @return int32_t Filtered distance in cm×100, or negative on error
 */
int32_t aguada_sensor_read_distance_filtered(void);

/**
 * @brief Read digital input (valve or sound detector)
 * 
 * @param pin GPIO pin number
 * @return uint8_t Pin state (0 or 1)
 */
uint8_t aguada_sensor_read_digital(gpio_num_t pin);

/**
 * @brief Read all sensors at once
 * 
 * @param data Output sensor data structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_sensor_read_all(aguada_sensor_data_t *data);

/**
 * @brief Set LED status
 * 
 * @param state LED state (0=off, 1=on)
 */
void aguada_sensor_set_led(uint8_t state);

/**
 * @brief Blink LED with specified pattern
 * 
 * @param count Number of blinks
 * @param on_ms On duration in milliseconds
 * @param off_ms Off duration in milliseconds
 */
void aguada_sensor_blink_led(uint8_t count, uint32_t on_ms, uint32_t off_ms);

/**
 * @brief Get default sensor configuration
 * 
 * @return aguada_sensor_config_t Default configuration
 */
aguada_sensor_config_t aguada_sensor_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif // AGUADA_SENSOR_H
