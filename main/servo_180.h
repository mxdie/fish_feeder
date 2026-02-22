#ifndef SERVO_180_H
#define SERVO_180_H

#include "esp_err.h"

/**
 * @brief 初始化180度舵机
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_init(void);

/**
 * @brief 设置舵机到指定角度
 * @param angle 角度值 (0-180)
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_set_angle(int angle);

/**
 * @brief 转动舵机到目标角度
 * @param target_angle 目标角度 (0-180)
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_rotate_to(int target_angle);

/**
 * @brief 平滑转动舵机（带步进延迟）
 * @param from_angle 起始角度 (0-180)
 * @param to_angle 目标角度 (0-180)
 * @param step_delay_ms 每步延迟时间 (毫秒)
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_rotate_smooth(int from_angle, int to_angle, int step_delay_ms);

/**
 * @brief 获取舵机当前角度
 * @return 当前角度值 (0-180)
 */
int servo_180_get_current_angle(void);

/**
 * @brief 转动舵机到0度
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_to_0(void);

/**
 * @brief 转动舵机到180度
 * @return ESP_OK 成功, 其他值失败
 */
esp_err_t servo_180_to_180(void);

#endif
