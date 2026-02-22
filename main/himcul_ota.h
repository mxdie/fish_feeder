/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef HIMCU_LITE_OTA_H
#define HIMCU_LITE_OTA_H

#include "himcul_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       TODO: OTA初始化，准备传输固件
 *
 * @param[in]   size: 待升级固件大小
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaInit(uint32_t size);

/**
 * @brief       TODO: OTA固件分片写入
 *
 * @param[in]   data: 固件数据分片
 * @param[in]   len: 固件数据分片长度
 * @param[in]   offset: 固件数据分片偏移
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaWrite(const uint8_t *data, uint32_t len, uint32_t offset);

/**
 * @brief       TODO: OTA固件分片读取， 使能 HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT 后有效 \n
 *                    通过读取写入flash的固件计算CRC，与模组下发的CRC比较，判断固件写入是否成功 \n
 *                    固件自身有完整性校验机制可关闭该功能
 *
 * @param[in,out]   buffer: 固件数据分片缓冲区
 * @param[in]   len: 固件数据分片缓冲区长度
 * @param[in]   offset: 固件数据分片读取偏移
 * @return
 *              - > 0: 读取到的数据长度
 *              - <= 0: 失败
 */
int32_t HIMCUL_PROD_OtaRead(uint8_t *buffer, uint32_t len, uint32_t offset);

/**
 * @brief       TODO: OTA固件传输结束，开发者需根据type清理资源占用
 *
 * @param[in]   type: 结束类型
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaDeinit(HIMCUL_OtaEndType type);

/**
 * @brief       TODO: 固件CRC校验， 关闭 HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT 后有效 \n
 *                    由产品自身判断写入flash的固件CRC是否一致
 *
 * @param[in]   crc: 固件CRC
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaCheckCrc(uint32_t crc);

/**
 * @brief       TODO: OTA升级重启
 *
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaReboot(void);

/**
 * @brief       TODO: 获取当前能否进行OTA升级，开发者可以使用该接口实现凌晨自动升级
 *
 * @param[in]   type: 升级类型
 * @return
 *              - HIMCUL_OTA_REBOOT_FLAG_ENABLE: 可升级
 *              - HIMCUL_OTA_REBOOT_FLAG_DISABLE: 不可升级
 */
HIMCUL_OtaEnableFlag HIMCUL_PROD_OtaGetEnableFlag(HIMCUL_OtaStartType type);

/**
 * @brief       TODO: 计算CRC32，可适配到硬件或查表实现，需在每次读取完后喂狗，避免循环读取固件时狗复位
 *
 * @param[in,out]   crc: 待更新的crc计算结果
 * @param[in]   data: 计算数据
 * @param[in]   len: 计算长度
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_OtaUpdateCrc(uint32_t *crc, const uint8_t *data, uint32_t len);

/**
 * @brief       TODO: 获取最大升级区域大小
 *
 * @return
 *              - 0: 失败
 *              - 其他: 成功
 */
uint32_t HIMCUL_PROD_GetOtaSize(void);

#ifdef __cplusplus
}
#endif

#endif /* HIMCU_LITE_OTA_H */