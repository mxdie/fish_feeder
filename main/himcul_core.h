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
#ifndef HIMCU_LITE_CORE_H
#define HIMCU_LITE_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include "himcul_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HIMCUL_LOG_LEVEL_MIN    0
#define HIMCUL_LOG_LEVEL_FATAL  1
#define HIMCUL_LOG_LEVEL_ERROR  2
#define HIMCUL_LOG_LEVEL_WARN   3
#define HIMCUL_LOG_LEVEL_NOTICE 4
#define HIMCUL_LOG_LEVEL_INFO   5
#define HIMCUL_LOG_LEVEL_DEBUG  6
#define HIMCUL_LOG_LEVEL_MAX    7

#define HIMCUL_MAJOR_VERSION 1
#define HIMCUL_MINOR_VERSION 0
#define HIMCUL_PATCH_VERSION 0

/* 重传间隔 */
#define HIMCUL_RETRANS_INTERVAL_MS   1000
#define HIMCUL_RETRANS_MAX_CNT       5

/* 最大心跳间隔 */
#define HIMCUL_HEARTBEAT_MAX_INTERVAL_MS    (60 * 1000)

typedef enum {
    HIMCUL_NETCFG_TYPE_INVALID = -1,
    HIMCUL_NETCFG_TYPE_NONE = 0,
    /* softap配网 */
    HIMCUL_NETCFG_TYPE_SOFTAP,
    /* ble 辅助配网 */
    HIMCUL_NETCFG_TYPE_BLE_SUP,
    /* ble+sle 辅助配网 */
    HIMCUL_NETCFG_TYPE_BLE_SLE_SUP,
    /* ble 双连双控 */
    HIMCUL_NETCFG_TYPE_BLE_DUL_CONN,
    /* ble+sle 双连双控 */
    HIMCUL_NETCFG_TYPE_BLE_SLE_DUL_CONN,
} HIMCUL_NetcfgType;

typedef enum {
    HIMCUL_TRANS_OK                      = 0,
    HIMCUL_TRANS_ERR_INNER               = 1,
    HIMCUL_TRANS_ERR_PARAM_INVALID       = 2,
    HIMCUL_TRANS_ERR_SOURCE_NOT_ENOUGH   = 3,
    HIMCUL_TRANS_ERR_BUSY                = 4,
    HIMCUL_TRANS_ERR_CHECKSUM_NOT_MATCH  = 5,
    HIMCUL_TRANS_ERR_CONTROL             = 100,
    HIMCUL_TRANS_ERR_CONTROL_PART_OK     = 101,
    HIMCUL_TRANS_ERR_CONTROL_ASYNC       = 102,
    HIMCUL_TRANS_ERR_QUERY               = 103,
    HIMCUL_TRANS_ERR_QUERY_PART_OK       = 104,
    HIMCUL_TRANS_ERR_CONF_PART_OK        = 105,
    HIMCUL_TRANS_ERR_OTA                 = 200,
    HIMCUL_TRANS_ERR_OTA_NOT_READY       = 201,
    HIMCUL_TRANS_ERR_OTA_SIZE_INVALID    = 202,
    HIMCUL_TRANS_ERR_OTA_DATA_INCOMPLETE = 203,
    HIMCUL_TRANS_ERR_OTA_CRC_NOT_MATCH   = 204,
    HIMCUL_TRANS_ERR_FACTORY             = 300,
} HIMCUL_TransErrcode;

typedef enum {
    HIMCUL_ERR_REPORT_QUEUE_FULL = -500,
    HIMCUL_ERR_REPORT_CTX_INVALID,

    HIMCUL_ERR_OTA_SIZE_INVALID = -400,
    HIMCUL_ERR_OTA_INIT,
    HIMCUL_ERR_OTA_WRITE,
    HIMCUL_ERR_OTA_READ,

    HIMCUL_ERR_TRANSPORT_SEND = -300,
    HIMCUL_ERR_TRANSPORT_RECV,
    HIMCUL_ERR_TRANSPORT_TOO_LONG,

    HIMCUL_ERR_PROFILE_DISPATCH = -200,
    HIMCUL_ERR_PROFILE_PUT,
    HIMCUL_ERR_PROFILE_GET,
    HIMCUL_ERR_PROFILE_SVC_NOT_FIND,
    HIMCUL_ERR_PROFILE_REPORT,

    HIMCUL_ERR_PARAM_INVALID = -100,
    HIMCUL_ERR_SECUREC_MEMCPY,
    HIMCUL_ERR_SECUREC_VSPRINTF,
    HIMCUL_ERR_SECUREC_STRNCPY,
    HIMCUL_ERR_SECUREC_SPRINTF,
    HIMCUL_ERR_ADAPTER_STRTOL,
    HIMCUL_ERR_NOT_SUPPORT,
    HIMCUL_ERR_CTX_INVALID,
    HIMCUL_ERR_BUFFER_NOT_ENOUGH,

    HIMCUL_ERROR = -1,
    HIMCUL_OK = 0,
} HIMCUL_Errcode;

typedef enum {
    HIMCUL_OTA_RESULT_OK = 0x00,
    HIMCUL_OTA_RESULT_ERR_INNER = 0x01,
    HIMCUL_OTA_RESULT_ERR_TIMEOUT = 0x02,
    HIMCUL_OTA_RESULT_ERR_CRC_NOT_MATCH = 0x03,
} HIMCUL_OtaResult;

typedef enum {
    HIMCUL_OTA_REBOOT_FLAG_ENABLE = 0,
    HIMCUL_OTA_REBOOT_FLAG_DISABLE,
} HIMCUL_OtaEnableFlag;

typedef enum {
    HIMCUL_OTA_START_TYPE_MANUAL = 0,
    HIMCUL_OTA_START_TYPE_AUTO,
    HIMCUL_OTA_START_TYPE_MAX,
} HIMCUL_OtaStartType;

typedef enum {
    HIMCUL_OTA_END_TYPE_SUCCESS = 0,
    HIMCUL_OTA_END_TYPE_FAILED,
} HIMCUL_OtaEndType;

typedef enum {
    HIMCUL_EVENT_OFFLINE = 0,
    HIMCUL_EVENT_ONLINE,
    HIMCUL_EVENT_OFFLINE_LONG_TIME,
    HIMCUL_EVENT_OFFLINE_LONG_TIME_REBOOT,
    HIMCUL_EVENT_NOT_INIT,
    HIMCUL_EVENT_MODULE_UNDER_NETCFG,
    HIMCUL_EVENT_OFFLINE_10_MINUTES,
    HIMCUL_EVENT_ROUTER_CONNECTING,
    HIMCUL_EVENT_ROUTER_CONNECTED,
    HIMCUL_EVENT_CLOUD_CONNECTING,
    HIMCUL_EVENT_ROUTER_DISCONNECTED,
    HIMCUL_EVENT_REGISTERED,
    HIMCUL_EVENT_REVOKE_FLAG_SET,
    HIMCUL_EVENT_REGISTER_FAILED,
    HIMCUL_EVENT_ROUTER_CONNECT_FAILED,
    HIMCUL_EVENT_MODULE_NEED_CONFIG = 101,
    HIMCUL_EVENT_MODULE_RESTART,
} HIMCUL_Event;

#define HIMCUL_CMD_BASE_INFO_CONFIG              0x81
#define HIMCUL_CMD_BASE_SVC_REPORT               0x82
#define HIMCUL_CMD_BASE_INFO_QUERY               0x83
#define HIMCUL_CMD_BASE_RESTORE_FACTORY          0x84
#define HIMCUL_CMD_BASE_DIAGNOSE_REPORT          0x85
#define HIMCUL_CMD_BASE_REBOOT                   0x86
#define HIMCUL_CMD_BASE_CLEAR_CONFIG             0x87
#define HIMCUL_CMD_BASE_SVC_PUT                  0x01
#define HIMCUL_CMD_BASE_SVC_GET                  0x02
#define HIMCUL_CMD_BASE_EVENT_NOTIFY             0x03
#define HIMCUL_CMD_BASE_HEARTBEAT                0x04
#define HIMCUL_CMD_UPGRADE_START                 0x41
#define HIMCUL_CMD_UPGRADE_TRANS_FINISH          0x42
#define HIMCUL_CMD_UPGRADE_TRANS_ERROR           0x43
#define HIMCUL_CMD_UPGRADE_SEND_PACKET           0x44
#define HIMCUL_CMD_UPGRADE_CHECKSUM              0x45
#define HIMCUL_CMD_UPGRADE_RESULT_REPORT         0xC1
#define HIMCUL_CMD_FACTORY_ENTRY                 0xD1
#define HIMCUL_CMD_FACTORY_SCAN_WIFI             0xD2
#define HIMCUL_CMD_FACTORY_CONNECT_WIFI          0xD3
#define HIMCUL_CMD_FACTORY_OPEN_SOFTAP           0xD4
#define HIMCUL_CMD_FACTORY_REPORT_WIFI_LIST      0x51
#define HIMCUL_CMD_FACTORY_REPORT_WIFI_INFO      0x52
#define HIMCUL_CMD_FACTORY_REPORT_SOFTAP_RESULT  0x53

#define HIMCUL_TAG_CONFIG_MCU_VERSION            0x01
#define HIMCUL_TAG_CONFIG_PRODUCT_ID             0x10
#define HIMCUL_TAG_CONFIG_SUB_PRODUCT_ID         0x11
#define HIMCUL_TAG_CONFIG_MODEL                  0x12
#define HIMCUL_TAG_CONFIG_DEVICE_TYPE_ID         0x13
#define HIMCUL_TAG_CONFIG_DEVICE_TYPE_NAME       0x14
#define HIMCUL_TAG_CONFIG_MANU_ID                0x15
#define HIMCUL_TAG_CONFIG_MANU_NAME              0x16
#define HIMCUL_TAG_CONFIG_PROT_TYPE              0x17
#define HIMCUL_TAG_CONFIG_SN                     0x18
#define HIMCUL_TAG_CONFIG_SVC_INFO               0x20
#define HIMCUL_TAG_CONFIG_AC_KEY                 0x30
#define HIMCUL_TAG_CONFIG_PINCODE                0x31
#define HIMCUL_TAG_CONFIG_HEARTBEAT              0x40
#define HIMCUL_TAG_CONFIG_RETRANS                0x41
#define HIMCUL_TAG_CONFIG_OTA_MAX_SIZE           0x42
#define HIMCUL_TAG_CONFIG_OTA_PACKET_SIZE        0x43
#define HIMCUL_TAG_CONFIG_FRAME_SIZE             0x44
#define HIMCUL_TAG_CONFIG_NET_CONFIG             0x50
#define HIMCUL_TAG_CONFIG_NEAR_DISCOVERY_POWER   0x51

#define HIMCUL_CONFIG_ENABLE                     0x01
#define HIMCUL_CONFIG_DISABLE                    0x00

#define HIMCUL_QUERY_INFO_TIMEINFO_LEN           10

#define HIMCUL_TAG_QUERY_TIME_LOCAL              0x01
#define HIMCUL_TAG_QUERY_TIME_UTC                0x02
#define HIMCUL_TAG_QUERY_WIFI_SSID               0x03
#define HIMCUL_TAG_QUERY_WIFI_IP                 0x11
#define HIMCUL_TAG_QUERY_WIFI_RSSI               0x12
#define HIMCUL_TAG_QUERY_WIFI_BSSID              0x13
#define HIMCUL_TAG_QUERY_WIFI_CONNECT_STATUS     0x14
#define HIMCUL_TAG_QUERY_SN                      0x20
#define HIMCUL_TAG_QUERY_MAC                     0x21
#define HIMCUL_TAG_QUERY_SDK_VERSION             0x22
#define HIMCUL_TAG_QUERY_REG_STATUS              0x30
#define HIMCUL_TAG_QUERY_ONLINE_STATUS           0x31
#define HIMCUL_TAG_QUERY_NET_STATUS              0x32

#define HIMCUL_TAG_FACTORY_WIFI_INFO_SSID        0X01
#define HIMCUL_TAG_FACTORY_WIFI_INFO_PWD         0X02

#define HIMCUL_BASE_CONFIG_INTERVAL_MS           HIMCUL_SEC_TO_MS(10)
#define HIMCUL_HEARTBEAT_INTERVAL_MS             HIMCUL_SEC_TO_MS(15)
#define HIMCUL_HEARTBEAT_MAX_CNT                 4

#define HIMCUL_ARRAY_SINGLE_NUM 1
#define HIMCUL_ARRAY_SIZE(_x) (sizeof(_x) / sizeof((_x)[0]))
#define HIMCUL_BIT(_n) (1 << (_n))
#define HIMCUL_BIT_CLR(data)       ((data) = 0)
#define HIMCUL_BIT_SET(data, n)    ((data) |= HIMCUL_BIT(n))
#define HIMCUL_BIT_RESET(data, n)  ((data) &= (~HIMCUL_BIT(n)))
#define HIMCUL_IS_BIT_SET(_data, _n) (((_data) & HIMCUL_BIT(_n)) != 0)
#define HIMCUL_NOT_USED(a) ((void)(a))
#define HIMCUL_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define HIMCUL_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define HIMCUL_IS_MCU_TO_MODULE_REQUEST(cmd) ((((uint8_t)(cmd) >> 7) & 0xFF) == 1)
#define HIMCUL_IS_MODULE_TO_MCU_REQUEST(cmd) ((((uint8_t)(cmd) >> 7) & 0xFF) == 0)
#define HIMCUL_IS_MCU_TO_MODULE_RESPONSE(cmd) HIMCUL_IS_MODULE_TO_MCU_REQUEST(cmd)
#define HIMCUL_IS_MODULE_TO_MCU_RESPONSE(cmd) HIMCUL_IS_MCU_TO_MODULE_REQUEST(cmd)

typedef struct HIMCUL_Fsm HIMCUL_Fsm;

typedef struct {
    int32_t state;
    int32_t (*handler)(void *param, int32_t cur);
} HIMCUL_FsmStateNode;

struct HIMCUL_Fsm {
    const char *name;
    int32_t cur;
    void (*onChange)(HIMCUL_Fsm *fsm, int32_t before, int32_t next);
    const HIMCUL_FsmStateNode *tbl;
    uint32_t num;
};

typedef enum {
    HIMCUL_FRAME_FLAG_RETRANS = 0,
} HIMCUL_FrameFlag;

typedef struct HIMCUL_Context HIMCUL_Context;

typedef struct {
    uint16_t magic;
    uint8_t ver;
    uint8_t cmd;
    uint16_t id;
    uint16_t len;
    const uint8_t *data;
    uint8_t checkSum;
    uint8_t flag;
} HIMCUL_TransFrame;

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t len;
} HIMCUL_Buffer;

typedef struct {
    uint8_t cmd;
    int32_t (*handler)(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame);
} HIMCUL_TransEndpoint;

typedef int32_t (*HIMCUL_TransSendHandler)(const uint8_t *data, uint32_t len);
typedef int32_t (*HIMCUL_TransRecvHandler)(uint8_t *buf, uint32_t len, uint32_t timeoutMs);
typedef int32_t (*HIMCUL_BaseConfigBuildHandler)(HIMCUL_Buffer *buf);
typedef int32_t (*HIMCUL_BaseProfilePutHandler)(uint8_t siid, uint8_t ciid,
    const uint8_t *data, uint32_t len, HIMCUL_Buffer *buf);
typedef int32_t (*HIMCUL_BaseProfileGetHandler)(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buf);

struct HIMCUL_Context {
    HIMCUL_TransSendHandler txHandler;
    HIMCUL_TransRecvHandler rxHandler;
    HIMCUL_BaseProfilePutHandler putHandler;
    HIMCUL_BaseProfileGetHandler getHandler;
    const HIMCUL_TransEndpoint *endpoints;
    uint32_t endpointNum;
    uint8_t *recvBuf;
    uint32_t recvBufSize;
    uint32_t curMs;
    struct {
        bool init;
        uint32_t timeout;
        uint16_t recvSeq;
        uint16_t sendSeq;
        uint32_t recvLen;
        HIMCUL_TransFrame frame;
        HIMCUL_Fsm fsm;
    } private;
};

typedef struct {
    int32_t (*strtolCb)(const char *nptr, char **endptr, int base);
    void (*logOutput)(uint8_t level, const char *tag, const char *fmt, va_list arg);
} HIMCUL_BaseCallback;

typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    uint8_t *buf;
} HIMCUL_RingBuffer;

void HIMCUL_SetLogLevel(uint8_t level);
void HIMCUL_LogOutputImpl(uint8_t level, const char *funcName, uint32_t line, const char *fmt, ...);
#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_DEBUG)
#define HIMCUL_LOGD(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_DEBUG, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGD(...)
#endif

#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_INFO)
#define HIMCUL_LOGI(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_INFO, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGI(...)
#endif

#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_NOTICE)
#define HIMCUL_LOGN(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_NOTICE, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGN(...)
#endif

#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_WARN)
#define HIMCUL_LOGW(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_WARN, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGW(...)
#endif

#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_ERROR)
#define HIMCUL_LOGE(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_ERROR, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGE(...)
#endif

#if (HIMCUL_CONF_LOG_SUPPORT) && (HIMCUL_CONF_LOG_BUILD_LEVEL >= HIMCUL_LOG_LEVEL_FATAL)
#define HIMCUL_LOGF(...) HIMCUL_LogOutputImpl(HIMCUL_LOG_LEVEL_FATAL, \
    __func__, __LINE__, __VA_ARGS__)
#else
#define HIMCUL_LOGF(...)
#endif

#define HIMCUL_LOGE_FRAME(frame, fmt, ...) HIMCUL_LOGE("[0x%02x|0x%04x|%u]" fmt, \
    (frame)->cmd, (frame)->id, (frame)->len, ##__VA_ARGS__)
#define HIMCUL_LOGW_FRAME(frame, fmt, ...) HIMCUL_LOGW("[0x%02x|0x%04x|%u]" fmt, \
    (frame)->cmd, (frame)->id, (frame)->len, ##__VA_ARGS__)
#define HIMCUL_LOGN_FRAME(frame, fmt, ...) HIMCUL_LOGN("[0x%02x|0x%04x|%u]" fmt, \
    (frame)->cmd, (frame)->id, (frame)->len, ##__VA_ARGS__)
#define HIMCUL_LOGI_FRAME(frame, fmt, ...) HIMCUL_LOGI("[0x%02x|0x%04x|%u]" fmt, \
    (frame)->cmd, (frame)->id, (frame)->len, ##__VA_ARGS__)
#define HIMCUL_LOGD_FRAME(frame, fmt, ...) HIMCUL_LOGD("[0x%02x|0x%04x|%u]" fmt, \
    (frame)->cmd, (frame)->id, (frame)->len, ##__VA_ARGS__)
#define HIMCUL_LOGE_MEMCPY(arg1, arg2, arg3, arg4) \
    HIMCUL_LOGE("memcpy error, arg1=%d arg2=%u arg3=%d arg4=%u", (arg1) != NULL, (arg2), (arg3) != NULL, (arg4))
#define HIMCUL_LOGE_MEMMOVE(arg1, arg2, arg3, arg4) \
    HIMCUL_LOGE("memmove error, arg1>arg3=%d delta=%u arg2=%u arg4=%u", \
        (arg1) > (arg3), (arg1) > (arg3) ? (arg1) - (arg3) : (arg3) - (arg1), (arg2), (arg4))
#define HIMCUL_LOGE_STRNCPY(arg1, arg2, arg3, arg4) \
    HIMCUL_LOGE("strcpy error, arg1=%d arg2=%u arg3=%d arg4=%u", (arg1) != NULL, (arg2), (arg3) != NULL, (arg4))
#define HIMCUL_LOGE_MALLOC(size) \
    HIMCUL_LOGE("malloc error, size=%u", size)
#define HIMCUL_LOGE_CALLOC(num, size) \
    HIMCUL_LOGE("calloc error, num=%u size=%u", num, size)

#define HIMCUL_CHECK_RETURN(cond, ret) do { \
    if (!(cond)) { \
        return (ret); \
    } \
} while (0)

#define HIMCUL_CHECK_RETURN_LOGW(cond, ret, ...) do { \
    if (!(cond)) { \
        HIMCUL_LOGW(__VA_ARGS__); \
        return (ret); \
    } \
} while (0)

#define HIMCUL_CHECK_RETURN_LOGE(cond, ret, ...) do { \
    if (!(cond)) { \
        HIMCUL_LOGE(__VA_ARGS__); \
        return (ret); \
    } \
} while (0)

#define HIMCUL_CHECK_V_RETURN(cond) do { \
    if (!(cond)) { \
        return; \
    } \
} while (0)

#define HIMCUL_CHECK_V_RETURN_LOGW(cond, ...) do { \
    if (!(cond)) { \
        HIMCUL_LOGW(__VA_ARGS__); \
        return; \
    } \
} while (0)

#define HIMCUL_CHECK_V_RETURN_LOGE(cond, ...) do { \
    if (!(cond)) { \
        HIMCUL_LOGE(__VA_ARGS__); \
        return; \
    } \
} while (0)

#define HIMCUL_UNREACHABLE_BRANCH() HIMCUL_LOGF("entry unreachable branch!!!")

/**
 * @brief       注册基础回调函数
 *
 * @param[in]   cb: 回调函数指针，常量指针
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_BaseRegisterCallback(const HIMCUL_BaseCallback *cb);

/**
 * @brief       IO轮询入口
 *
 * @param[in]   ctx: 上下文
 * @param[in]   timeoutMs: 最大超时时间
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_TransIoLoop(HIMCUL_Context *ctx, uint32_t curMs, uint32_t timeoutMs);

/**
 * @brief       发送请求
 *
 * @param[in]   ctx: 上下文
 * @param[in]   cmd: 请求指令
 * @param[in,out]   id: 请求的id，可以为NULL，非NULL：0不指定id，出参携带himcul自动分配的id，非0则指定id用于重传
 * @param[in]   data: 请求数据，可以为NULL
 * @param[in]   len: 请求数据长度
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_TransSendRequest(HIMCUL_Context *ctx, uint8_t cmd, uint16_t *id, const uint8_t *data, uint32_t len);

/**
 * @brief       发送响应
 *
 * @param[in]   ctx: 上下文
 * @param[in]   cmd: 请求指令
 * @param[in]   id: 响应的id，从请求帧获取
 * @param[in]   data: 响应数据，可以为NULL
 * @param[in]   len: 响应数据长度
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_TransSendResponse(HIMCUL_Context *ctx, uint8_t cmd, uint16_t id, const uint8_t *data, uint32_t len);

/**
 * @brief       发送响应码
 *
 * @param[in]   ctx: 上下文
 * @param[in]   cmd: 请求指令
 * @param[in]   id: 响应的id，从请求帧获取
 * @param[in]   errcode: 响应码
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_TransSendResponseCode(HIMCUL_Context *ctx, uint8_t cmd, uint16_t id, uint32_t errcode);

/**
 * @brief       发送基础配置信息
 *
 * @param[in]   ctx: 上下文
 * @param[out]  id: 请求的id，可以为NULL
 * @param[in]   tag: 基础配置tag
 * @param[in]   data: 请求数据，可以为NULL
 * @param[in]   len: 请求数据长度
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_TransSendBaseConfig(HIMCUL_Context *ctx, uint16_t *id, uint8_t tag, const uint8_t *data, uint32_t len);

/**
 * @brief       分发profile指令
 *
 * @param[in]   ctx: 上下文
 * @param[in]   frame: 数据帧
 * @param[in,out]   buf: 构造响应的buffer
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_ProfileDispatch(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame, HIMCUL_Buffer *buf);

/** @brief 将profile数据帧中的ascii数据转换为真实数据类型 */
int32_t HIMCUL_ProfileCharToBool(const uint8_t *data, uint32_t len, bool *value);
int32_t HIMCUL_ProfileCharToInt(const uint8_t *data, uint32_t len, int32_t *value);

/** @brief 在数据帧中添加profile数据 */
int32_t HIMCUL_ProfileCharAddBool(uint8_t siid, uint8_t ciid, bool value, HIMCUL_Buffer *buf);
int32_t HIMCUL_ProfileCharAddInt(uint8_t siid, uint8_t ciid, int32_t value, HIMCUL_Buffer *buf);
int32_t HIMCUL_ProfileCharAddStr(uint8_t siid, uint8_t ciid, const char *str, uint32_t len, HIMCUL_Buffer *buf);

/** @brief 网络大端序与主机序转换 */
uint16_t HIMCUL_UtilsHtons(uint16_t hs);
uint32_t HIMCUL_UtilsHtonl(uint32_t hl);
uint16_t HIMCUL_UtilsNtohs(uint16_t ns);
uint32_t HIMCUL_UtilsNtohl(uint32_t nl);

/** @brief buffer中以网络大端序添加数据 */
int32_t HIMCUL_UtilsAddTlv(HIMCUL_Buffer *buf, uint8_t type, const uint8_t *value, uint32_t valueLen);
int32_t HIMCUL_UtilsAddData(HIMCUL_Buffer *buf, const uint8_t *data, uint32_t dataLen);
int32_t HIMCUL_UtilsAddUint8(HIMCUL_Buffer *buf, uint32_t num);
int32_t HIMCUL_UtilsAddUint16BigEnd(HIMCUL_Buffer *buf, uint32_t num);
int32_t HIMCUL_UtilsAddUint32BigEnd(HIMCUL_Buffer *buf, uint32_t num);
int32_t HIMCUL_UtilsAddFormatData(HIMCUL_Buffer *buffer, const char *format, ...);

/** @brief 环形缓冲区，仅支持单CPU单读单写 */
int32_t HIMCUL_RingBufferInit(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t size);
void HIMCUL_RingBufferClear(HIMCUL_RingBuffer *rb);
uint32_t HIMCUL_RingBufferGetDataLen(HIMCUL_RingBuffer *rb);
int32_t HIMCUL_RingBufferRead(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t len);
uint32_t HIMCUL_RingBufferGetBufLen(HIMCUL_RingBuffer *rb);
int32_t HIMCUL_RingBufferWrite(HIMCUL_RingBuffer *rb, const uint8_t *data, uint32_t len);

uint32_t HIMCUL_DeltaTime(uint32_t timeNew, uint32_t timeOld);

#ifdef __cplusplus
}
#endif

#endif /* HIMCU_LITE_CORE_H */