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
#ifndef HIMCU_LITE_PROFILE_H
#define HIMCU_LITE_PROFILE_H

#include "himcul_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       switch-on 服务put处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   data: 服务数据
 * @param[in]   len: 服务数据长度
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_SwitchOnPutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf);

/**
 * @brief       switch-on 服务get处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_SwitchOnGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

/**
 * @brief       switch-on 服务report处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_SwitchOnRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

/**
 * @brief       brightness-brightness 服务put处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   data: 服务数据
 * @param[in]   len: 服务数据长度
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_BrightnessBrightnessPutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf);

/**
 * @brief       brightness-brightness 服务get处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_BrightnessBrightnessGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

/**
 * @brief       brightness-brightness 服务report处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_BrightnessBrightnessRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

/**
 * @brief       cct-colorTemperature 服务put处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   data: 服务数据
 * @param[in]   len: 服务数据长度
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_CctColorTemperaturePutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf);

/**
 * @brief       cct-colorTemperature 服务get处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_CctColorTemperatureGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

/**
 * @brief       cct-colorTemperature 服务report处理函数
 * @param[in]   siid: 服务 siid ，定义在g_profile中
 * @param[in]   ciid: 属性 ciid ，定义在g_profile中
 * @param[in]   buf: 响应数据回填buffer
 * @return
 *              - 0: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PRF_CctColorTemperatureRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* HIMCU_LITE_PROFILE_H */