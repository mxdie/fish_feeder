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
#include "servo_180.h"

static bool g_switchIntState = false;

int32_t HIMCUL_PRF_SwitchOnPutHandler(uint8_t siid, uint8_t ciid, const uint8_t *data,
    uint32_t len, HIMCUL_Buffer *buf)
{
    bool switchIntState = false;
    int32_t ret = HIMCUL_ProfileCharToBool(data, len, &switchIntState);
    if (ret != 0) {
        HIMCUL_LOGE("invalid bool data, ret=%d", ret);
        return ret;
    }
    HIMCUL_LOGN("put switchIntState, state=%d=>%d", g_switchIntState, switchIntState);
    g_switchIntState = switchIntState;
    
    if (g_switchIntState) {
        servo_180_to_180();
    } else {
        servo_180_to_0();
    }
    
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchIntState, buf);
}

int32_t HIMCUL_PRF_SwitchOnGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("get switchIntState, state=%d", g_switchIntState);
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchIntState, buf);
}

int32_t HIMCUL_PRF_SwitchOnRptHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf)
{
    HIMCUL_LOGN("report switchIntState, state=%d", g_switchIntState);
    return HIMCUL_ProfileCharAddBool(siid, ciid, g_switchIntState, buf);
}
