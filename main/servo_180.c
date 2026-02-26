#include "servo_180.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SERVO_GPIO_PIN 2
#define SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define SERVO_LEDC_TIMER LEDC_TIMER_0
#define SERVO_LEDC_MODE LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_RESOLUTION LEDC_TIMER_13_BIT
#define SERVO_FREQUENCY_HZ 50

#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500
#define SERVO_PERIOD_US 20000

static bool servo_initialized = false;
static int current_angle = 0;

static uint32_t angle_to_duty(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    uint32_t pulse_us = SERVO_MIN_PULSE_US + (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180);
    uint32_t duty = (pulse_us * ((1 << SERVO_LEDC_RESOLUTION) - 1)) / SERVO_PERIOD_US;
    
    return duty;
}

esp_err_t servo_180_init(void)
{
    if (servo_initialized) {
        return ESP_OK;
    }

    ledc_timer_config_t timer_conf = {
        .speed_mode = SERVO_LEDC_MODE,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        return ret;
    }

    ledc_channel_config_t channel_conf = {
        .gpio_num = SERVO_GPIO_PIN,
        .speed_mode = SERVO_LEDC_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        return ret;
    }

    current_angle = 0;
    servo_initialized = true;
    return ESP_OK;
}

esp_err_t servo_180_set_angle(int angle)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    printf("set angle, %u=>%u\n", current_angle, angle);
    uint32_t duty = angle_to_duty(angle);
    esp_err_t ret = ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
        if (ret == ESP_OK) {
            current_angle = angle;
        }
    }
    return ret;
}

esp_err_t servo_180_rotate_to(int target_angle)
{
    return servo_180_set_angle(target_angle);
}

esp_err_t servo_180_rotate_smooth(int from_angle, int to_angle, int step_delay_ms)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (from_angle < 0) from_angle = 0;
    if (from_angle > 180) from_angle = 180;
    if (to_angle < 0) to_angle = 0;
    if (to_angle > 180) to_angle = 180;
    if (step_delay_ms < 1) step_delay_ms = 10;

    int step = (to_angle > from_angle) ? 1 : -1;
    
    for (int angle = from_angle; angle != to_angle; angle += step) {
        esp_err_t ret = servo_180_set_angle(angle);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
    }
    
    return servo_180_set_angle(to_angle);
}

int servo_180_get_current_angle(void)
{
    return current_angle;
}

esp_err_t servo_180_to_0(void)
{
    return servo_180_rotate_to(0);
}

esp_err_t servo_180_to_180(void)
{
    return servo_180_rotate_to(180);
}
