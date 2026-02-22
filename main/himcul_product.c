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
#include "himcul_product.h"
#include <stdlib.h>
#include <string.h>
#if HIMCUL_CONF_LOG_SUPPORT
#include <stdio.h>
#endif
#include "himcul_profile.h"
#include "himcul_ota.h"
#include "himcul_framework.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define UART_NUM UART_NUM_1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 256
#define UART_EVENT_QUEUE_SIZE 20

static SemaphoreHandle_t uart_mutex = NULL;
static QueueHandle_t uart_event_queue = NULL;
static bool uart_initialized = false;

/**
 * @brief       TODO: 按产品实际信息修改配置
 *
 */
static const uint8_t g_version[4]   = {1, 0, 0, 0};
static const char *g_prodId         = "2GVI";
static const char *g_model          = "SH-WDQ-LGT101";
static const char *g_devTypeId      = "005";
static const char *g_devTypeName    = "SmartSwitch";
static const char *g_manuId         = "0f6";
static const char *g_manuName       = "DNAKE";
#if HIMCUL_CONF_CUSTOM_PROTOCOL_ENABLE
static const uint8_t g_protType     = HIMCUL_CONF_CUSTOM_PROTOCOL_TYPE;
#endif
static const uint8_t g_netcfgMode   = HIMCUL_NETCFG_TYPE_SOFTAP;
#if HIMCU_CONF_NEAR_DISCOVERY_ENABLE
static const uint8_t g_nearPower    = HIMCU_CONF_NEAR_DISCOVERY_POWER;
#endif
#if HIMCUL_CONF_MCU_SN_SUPPORT
/* SN如果写在MCU侧，需要实现SN的读取赋值 */
static const char *g_sn             = "1234567890";
#endif
static const uint8_t g_heartbeat    = HIMCUL_CONF_HEARTBEAT_SUPPORT;
static const uint8_t g_retrans      = HIMCUL_CONF_RETRANS_SUPPORT;
static const uint16_t g_frameSize   = HIMCUL_CONF_TRANS_FRAME_MAX_SIZE;
#if HIMCUL_CONF_OTA_SUPPORT
static const uint16_t g_otaPacket   = HIMCUL_CONF_TRANS_FRAME_MAX_SIZE;
#endif

static const uint8_t g_acKey[48]    = {};

#if HIMCUL_CONF_LOG_SUPPORT
void HIMCUL_PROD_LogOutput(uint8_t level, const char *tag, const char *fmt, va_list arg)
{
    if (tag == NULL || fmt == NULL) {
        return;
    }
    if (level == 0) {
        level = HIMCUL_LOG_LEVEL_FATAL;
    } else if (level >= HIMCUL_LOG_LEVEL_MAX) {
        level = HIMCUL_LOG_LEVEL_DEBUG;
    }
    const char *levelTag[HIMCUL_LOG_LEVEL_DEBUG] = {"F", "E", "W", "N", "I", "D"};
    printf("HIMCUL:%s:", levelTag[level - 1]);
    printf("%s", tag);
    vprintf(fmt, arg);
    printf("\r\n");
}
#endif

int32_t HIMCUL_PROD_TransInit(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_mutex = xSemaphoreCreateMutex();
    if (uart_mutex == NULL) {
        return HIMCUL_ERROR;
    }

    uart_event_queue = xQueueCreate(UART_EVENT_QUEUE_SIZE, sizeof(uart_event_t));
    if (uart_event_queue == NULL) {
        return HIMCUL_ERROR;
    }

    uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 
                       UART_EVENT_QUEUE_SIZE, &uart_event_queue, 0);

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_initialized = true;
    return HIMCUL_OK;
}

#if !HIMCUL_CONF_UART_RING_BUFFER_SUPPORT
int32_t HIMCUL_PROD_TransRecvHandler(uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{
    HIMCUL_CHECK_RETURN_LOGW(buf != NULL && len != 0, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    
    uart_event_t event;
    if (xQueueReceive(uart_event_queue, (void *)&event, pdMS_TO_TICKS(timeoutMs))) {
        if (event.type == UART_DATA) {
            int32_t recv_len = uart_read_bytes(UART_NUM, buf, len, 0);
            return recv_len;
        }
    }
    
    return 0;
}
#endif

int32_t HIMCUL_PROD_TransSendHandler(const uint8_t *data, uint32_t len)
{
    HIMCUL_CHECK_RETURN_LOGW(data != NULL && len != 0, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    
    if (uart_mutex != NULL) {
        xSemaphoreTake(uart_mutex, portMAX_DELAY);
    }
    
    int32_t sent_len = uart_write_bytes(UART_NUM, (const char *)data, len);
    
    if (uart_mutex != NULL) {
        xSemaphoreGive(uart_mutex);
    }
    
    return sent_len;
}

void HIMCUL_PROD_EventProcess(uint32_t event)
{
    return;
}

int32_t HIMCUL_PROD_Strtol(const char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}

uint32_t HIMCUL_PROD_Strlen(const char *str)
{
    return strlen(str);
}

uint32_t HIMCUL_PROD_Strnlen(const char *str, uint32_t size)
{
    return strnlen(str, size);
}

typedef struct {
    const char *cid;
    const char *ct;
    uint8_t ciid;
    int32_t (*putFunc)(uint8_t siid, uint8_t ciid, const uint8_t *data, uint32_t len, HIMCUL_Buffer *buf);
    int32_t (*getFunc)(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);
    int32_t (*rptFunc)(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);
} ProfileCharItem;

typedef struct {
    const char *sid;
    const char *st;
    uint8_t siid;
    uint8_t charNum;
    const ProfileCharItem *chars;
} ProfileSvcItem;

static const ProfileCharItem g_charSwitch[] = {
    {
        .cid = "on",
        .ct = "bool",
        .ciid = 0,
        .putFunc = HIMCUL_PRF_SwitchOnPutHandler,
        .getFunc = HIMCUL_PRF_SwitchOnGetHandler,
        .rptFunc = HIMCUL_PRF_SwitchOnRptHandler,
    }
};

/**
 * @brief       TODO: 产品Profile服务信息
 *
 */
static const ProfileSvcItem g_profile[] = {
    {
        .sid = "switch",
        .st = "switch",
        .siid = 0,
        .charNum = HIMCUL_ARRAY_SIZE(g_charSwitch),
        .chars = g_charSwitch,
    }
};

static const ProfileCharItem *GetProfileCharItem(uint8_t siid, uint8_t ciid)
{
    for (uint32_t i = 0; i < HIMCUL_ARRAY_SIZE(g_profile); ++i) {
        if (g_profile[i].siid != siid || g_profile[i].chars == NULL) {
            continue;
        }
        for (uint8_t j = 0; j < g_profile[i].charNum; ++j) {
            if (g_profile[i].chars[j].ciid != ciid) {
                continue;
            }
            return g_profile[i].chars + j;
        }
    }
    return NULL;
}

int32_t HIMCUL_PROD_BaseProfilePutHandler(uint8_t siid, uint8_t ciid,
    const uint8_t *data, uint32_t len, HIMCUL_Buffer *buffer)
{
    HIMCUL_CHECK_RETURN_LOGW(buffer != NULL && data != NULL && len != 0, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    const ProfileCharItem *charItem = GetProfileCharItem(siid, ciid);
    if (charItem == NULL || charItem->putFunc == NULL) {
        HIMCUL_LOGE("unsupported put cmd, siid=%u ciid=%u", siid, ciid);
        return HIMCUL_ERR_PROFILE_SVC_NOT_FIND;
    }

    return charItem->putFunc(siid, ciid, data, len, buffer);
}

int32_t HIMCUL_PROD_BaseProfileGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buffer)
{
    HIMCUL_CHECK_RETURN_LOGW(buffer != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    const ProfileCharItem *charItem = GetProfileCharItem(siid, ciid);
    if (charItem == NULL || charItem->getFunc == NULL) {
        HIMCUL_LOGE("unsupported get cmd, siid=%u ciid=%u", siid, ciid);
        return HIMCUL_ERR_PROFILE_SVC_NOT_FIND;
    }

    return charItem->getFunc(siid, ciid, buffer);
}

int32_t HIMCUL_PROD_BaseProfileReportHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buffer)
{
    HIMCUL_CHECK_RETURN_LOGW(buffer != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    const ProfileCharItem *charItem = GetProfileCharItem(siid, ciid);
    if (charItem == NULL || charItem->rptFunc == NULL) {
        HIMCUL_LOGE("unsupported report cmd, siid=%u ciid=%u", siid, ciid);
        return HIMCUL_ERR_PROFILE_SVC_NOT_FIND;
    }

    return charItem->rptFunc(siid, ciid, buffer);
}

const char *GetCharType(const char *ct)
{
    if (strcmp(ct, "enum") == 0) {
        return "int";
    } else if (strcmp(ct, "array") == 0) {
        return "obj";
    }
    return ct;
}

int32_t HIMCUL_PROD_BuildBaseConfigProfile(HIMCUL_Buffer *buffer)
{
    HIMCUL_CHECK_RETURN_LOGW(buffer != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = HIMCUL_UtilsAddFormatData(buffer, "[");
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("build format date error, ret=%d", ret);
        return ret;
    }

    for (uint32_t i = 0; i < HIMCUL_ARRAY_SIZE(g_profile); ++i) {
        const ProfileSvcItem *curSvc = g_profile + i;
        ret = HIMCUL_UtilsAddFormatData(buffer, "{\"sid\":\"%s\",\"st\":\"%s\", \"siid\":%u,\"chars\":[",
            curSvc->sid, curSvc->st, curSvc->siid);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("build format date error, ret=%d", ret);
            return ret;
        }
        for (uint8_t j = 0; j < curSvc->charNum && curSvc->chars != NULL; j++) {
            const ProfileCharItem *curChar = curSvc->chars + j;
            ret = HIMCUL_UtilsAddFormatData(buffer, "{\"cid\":\"%s\",\"ct\":\"%s\", \"ciid\":%u}",
                curChar->cid, GetCharType(curChar->ct), curChar->ciid);
            if (ret != HIMCUL_OK) {
                HIMCUL_LOGE("build format date error, ret=%d", ret);
                return ret;
            }
            if (j == curSvc->charNum - 1) {
                continue;
            }
            ret = HIMCUL_UtilsAddFormatData(buffer, ",");
            if (ret != HIMCUL_OK) {
                HIMCUL_LOGE("build format date error, ret=%d", ret);
                return ret;
            }
        }
        ret = HIMCUL_UtilsAddFormatData(buffer, "]}");
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("build format date error, ret=%d", ret);
            return ret;
        }
        if (i == HIMCUL_ARRAY_SIZE(g_profile) - 1) {
            continue;
        }
        ret = HIMCUL_UtilsAddFormatData(buffer, ",");
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("build format date error, ret=%d", ret);
            return ret;
        }
    }
    return HIMCUL_UtilsAddFormatData(buffer, "]");
}

int32_t HIMCUL_PROD_BuildBaseConfig(HIMCUL_Buffer *buf)
{
    HIMCUL_CHECK_RETURN_LOGW(buf != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    /* 大小端转换 */
#ifdef HIMCUL_CONF_OTA_SUPPORT
    uint32_t otaSize = HIMCUL_UtilsHtonl(HIMCUL_PROD_GetOtaSize());
    uint16_t otaFrameSize = HIMCUL_UtilsHtons(g_otaPacket);
#endif
    uint16_t frameSize = HIMCUL_UtilsHtons(g_frameSize);

    struct ConfigItem {
        uint8_t tag;
        const uint8_t *data;
        uint32_t len;
    } configItems[] = {
        {HIMCUL_TAG_CONFIG_MCU_VERSION, (const uint8_t *)g_version, sizeof(g_version)},
        {HIMCUL_TAG_CONFIG_PRODUCT_ID, (const uint8_t *)g_prodId, HIMCUL_PROD_Strlen(g_prodId)},
        {HIMCUL_TAG_CONFIG_MODEL, (const uint8_t *)g_model, HIMCUL_PROD_Strlen(g_model)},
        {HIMCUL_TAG_CONFIG_DEVICE_TYPE_ID, (const uint8_t *)g_devTypeId, HIMCUL_PROD_Strlen(g_devTypeId)},
        {HIMCUL_TAG_CONFIG_DEVICE_TYPE_NAME, (const uint8_t *)g_devTypeName, HIMCUL_PROD_Strlen(g_devTypeName)},
        {HIMCUL_TAG_CONFIG_MANU_ID, (const uint8_t *)g_manuId, HIMCUL_PROD_Strlen(g_manuId)},
        {HIMCUL_TAG_CONFIG_MANU_NAME, (const uint8_t *)g_manuName, HIMCUL_PROD_Strlen(g_manuName)},
#if HIMCUL_CONF_CUSTOM_PROTOCOL_ENABLE
        {HIMCUL_TAG_CONFIG_PROT_TYPE, &g_protType, sizeof(g_protType)},
#endif
#if HIMCUL_CONF_MCU_SN_SUPPORT
        {HIMCUL_TAG_CONFIG_SN, (const uint8_t *)g_sn, HIMCUL_PROD_Strlen(g_sn)},
#endif
        {HIMCUL_TAG_CONFIG_AC_KEY, g_acKey, sizeof(g_acKey)},
        {HIMCUL_TAG_CONFIG_HEARTBEAT, &g_heartbeat, sizeof(g_heartbeat)},
        {HIMCUL_TAG_CONFIG_RETRANS, &g_retrans, sizeof(g_retrans)},
#if HIMCUL_CONF_OTA_SUPPORT
        {HIMCUL_TAG_CONFIG_OTA_MAX_SIZE, (const uint8_t *)&otaSize, sizeof(otaSize)},
        {HIMCUL_TAG_CONFIG_OTA_PACKET_SIZE, (const uint8_t *)&otaFrameSize, sizeof(otaFrameSize)},
#endif
        {HIMCUL_TAG_CONFIG_FRAME_SIZE, (const uint8_t *)&frameSize, sizeof(frameSize)},
        {HIMCUL_TAG_CONFIG_NET_CONFIG, &g_netcfgMode, sizeof(g_netcfgMode)},
#if HIMCU_CONF_NEAR_DISCOVERY_ENABLE
        {HIMCUL_TAG_CONFIG_NEAR_DISCOVERY_POWER, &g_nearPower, sizeof(g_nearPower)},
#endif
    };

    int32_t ret;
    for (uint32_t i = 0; i < HIMCUL_ARRAY_SIZE(configItems); i++) {
        ret = HIMCUL_UtilsAddTlv(buf, configItems[i].tag, configItems[i].data, configItems[i].len);
        if (ret != 0) {
            HIMCUL_LOGE("add config error, index=%u tag=%02x", i, configItems[i].tag);
            return ret;
        }
    }
    return HIMCUL_OK;
}

int32_t HIMCUL_PROD_BaseProfileReportAllHandler(HIMCUL_Buffer *buffer)
{
    int32_t ret;
    uint32_t cnt = 0;
    for (uint32_t i = 0; i < HIMCUL_ARRAY_SIZE(g_profile); ++i) {
        const ProfileSvcItem *curSvc = g_profile + i;
        if (curSvc->charNum == 0 || curSvc->chars == NULL) {
            continue;
        }
        for (uint8_t j = 0; j < curSvc->charNum; ++j) {
            const ProfileCharItem *curChar = curSvc->chars + j;
            if (curChar->rptFunc == NULL) {
                continue;
            }
            ret = curChar->rptFunc(curSvc->siid, curChar->ciid, buffer);
            if (ret != HIMCUL_OK) {
                HIMCUL_LOGE("report get error, ret=%d siid=%u ciid=%u", ret, curSvc->siid, curChar->ciid);
            } else {
                ++cnt;
            }
        }
    }

    /* 部分成功可以上报 */
    return cnt > 0 ? HIMCUL_OK : HIMCUL_ERR_PROFILE_REPORT;
}
