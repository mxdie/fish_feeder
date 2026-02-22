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
#include "himcul_profile.h"

/** @brief 模拟开关状态 */
static bool g_switchOnState = false;
/** @brief 模拟亮度状态 */
static int32_t g_brightnessBrightnessState = 30;
/** @brief 模拟色温状态 */
static int32_t g_cctColorTemperatureState = 3000;

int32_t HIMCUL_PRF_SwitchOnPutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf)
{
    bool switchOnState = false;
    /* 解析bool型属性 */
    int32_t ret = HIMCUL_ProfileCharToBool(data, len, &switchOnState);
    if (ret != 0) {
        HIMCUL_LOGE("invalid bool data, ret=%d", ret);
        return ret;
    }
    HIMCUL_LOGN("put switchOnState, state=%d=>%d", g_switchOnState, switchOnState);
    /* 修改属性状态 */
    g_switchOnState = switchOnState;
    /* 填充响应 */
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchOnState, buf);
}

int32_t HIMCUL_PRF_SwitchOnGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("get switchOnState, state=%d", g_switchOnState);
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchOnState, buf);
}

int32_t HIMCUL_PRF_SwitchOnRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("report switchOnState, state=%d", g_switchOnState);
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchOnState, buf);
}

int32_t HIMCUL_PRF_BrightnessBrightnessPutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf)
{
    int32_t brightnessBrightnessState = 0;
    /* 解析int型属性 */
    int32_t ret = HIMCUL_ProfileCharToInt(data, len, &brightnessBrightnessState);
    if (ret != 0) {
        HIMCUL_LOGE("invalid int data, ret=%d", ret);
        return ret;
    }
    HIMCUL_LOGN("put brightnessBrightnessState, state=%d=>%d", g_brightnessBrightnessState, brightnessBrightnessState);
    /* 修改属性状态 */
    g_brightnessBrightnessState = brightnessBrightnessState;
    /* 填充响应 */
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_brightnessBrightnessState, buf);
}

int32_t HIMCUL_PRF_BrightnessBrightnessGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("get brightnessBrightnessState, state=%d", g_brightnessBrightnessState);
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_brightnessBrightnessState, buf);
}

int32_t HIMCUL_PRF_BrightnessBrightnessRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("report brightnessBrightnessState, state=%d", g_brightnessBrightnessState);
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_brightnessBrightnessState, buf);
}

int32_t HIMCUL_PRF_CctColorTemperaturePutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf)
{
    int32_t cctColorTemperatureState = 0;
    /* 解析bool型属性 */
    int32_t ret = HIMCUL_ProfileCharToInt(data, len, &cctColorTemperatureState);
    if (ret != 0) {
        HIMCUL_LOGE("invalid int data, ret=%d", ret);
        return ret;
    }
    HIMCUL_LOGN("put cctColorTemperatureState, state=%d=>%d", g_cctColorTemperatureState, cctColorTemperatureState);
    /* 修改属性状态 */
    g_cctColorTemperatureState = cctColorTemperatureState;
    /* 填充响应 */
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_cctColorTemperatureState, buf);
}

int32_t HIMCUL_PRF_CctColorTemperatureGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("get cctColorTemperatureState, state=%d", g_cctColorTemperatureState);
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_cctColorTemperatureState, buf);
}

int32_t HIMCUL_PRF_CctColorTemperatureRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("report cctColorTemperatureState, state=%d", g_cctColorTemperatureState);
    return HIMCUL_ProfileCharAddInt(siid, ciid, g_cctColorTemperatureState, buf);
}
