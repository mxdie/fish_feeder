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
#include "himcul_ota.h"
#include "himcul_product.h"

#if HIMCUL_CONF_OTA_SUPPORT
/* TODO: CRC32算法，可适配到硬件或查表实现 */
int32_t HIMCUL_PROD_OtaUpdateCrc(uint32_t *crc, const uint8_t *data, uint32_t len)
{
    HIMCUL_CHECK_RETURN_LOGW(crc != NULL && data != NULL && len != 0, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    uint32_t curCrc = *crc;
    for (uint32_t i = 0; i < len; ++i) {
        curCrc ^= data[i];
        for (uint32_t j = 0; j < 8; ++j) {
            if ((curCrc & 1u) != 0) {
                curCrc >>= 1;
                curCrc ^= 0xEDB88320;
            } else {
                curCrc >>= 1;
            }
        }
    }
    *crc = curCrc;
    return HIMCUL_OK;
}

int32_t HIMCUL_PROD_OtaInit(uint32_t size)
{
    HIMCUL_CHECK_RETURN_LOGW(size <= HIMCUL_PROD_GetOtaSize(), HIMCUL_ERR_OTA_SIZE_INVALID,
        "ota size invalid, size=%u limit=%u", size, HIMCUL_PROD_GetOtaSize());
    return HIMCUL_OK;
}

int32_t HIMCUL_PROD_OtaWrite(const uint8_t *data, uint32_t len, uint32_t offset)
{
    return HIMCUL_OK;
}

#if HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT
int32_t HIMCUL_PROD_OtaRead(uint8_t *buffer, uint32_t len, uint32_t offset)
{
    return HIMCUL_OK;
}
#else
int32_t HIMCUL_PROD_OtaCheckCrc(uint32_t crc)
{
    return HIMCUL_OK;
}
#endif

int32_t HIMCUL_PROD_OtaDeinit(HIMCUL_OtaEndType type)
{
    return HIMCUL_OK;
}

int32_t HIMCUL_PROD_OtaReboot(void)
{
    return HIMCUL_OK;
}

HIMCUL_OtaEnableFlag HIMCUL_PROD_OtaGetEnableFlag(HIMCUL_OtaStartType type)
{
    return HIMCUL_OTA_REBOOT_FLAG_ENABLE;
}

uint32_t HIMCUL_PROD_GetOtaSize(void)
{
    return HIMCUL_CONF_OTA_MAX_SIZE;
}
#endif