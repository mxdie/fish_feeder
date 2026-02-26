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
#include "himcul_framework.h"
#include "himcul_core.h"
#include "himcul_product.h"
#include "himcul_ota.h"
#include "securec.h"

/**
 * @brief       HIMCUL运行过程上下文
 *
 */
static HIMCUL_Context g_himcuCtx = {0};
#if HIMCUL_CONF_UART_RING_BUFFER_SUPPORT
/* 环形缓冲区，串口中断过来的数据会先写入环形缓冲区 */
static uint8_t g_ringBufRaw[HIMCUL_CONF_TRANS_FRAME_MAX_SIZE] = {0};
static HIMCUL_RingBuffer g_ringBuffer;
#endif
/* 接收缓冲区，待接收完整帧成后处理 */
static uint8_t g_recvBufRaw[HIMCUL_CONF_TRANS_FRAME_MAX_SIZE] = {0};
/* 发送缓冲区 */
static uint8_t g_sendBufRaw[HIMCUL_CONF_TRANS_FRAME_MAX_SIZE] = {0};
/* 发送缓冲区使用前先重置长度 */
static HIMCUL_Buffer g_sendBuffer = {g_sendBufRaw, sizeof(g_sendBufRaw), 0};

/**
 * @brief       DEMO产品运行过程上下文
 *
 */
#if HIMCUL_CONF_OTA_SUPPORT
typedef enum {
    OTA_FLAG_RUNNING = 0,
    OTA_FLAG_FINISH,
    OTA_FLAG_REBOOT,
} OtaFlagBit;

static struct OtaCtx {
    uint32_t flag;
    uint32_t ts;
    uint32_t crc;
    uint32_t nextOffset;
    uint32_t otaSize;
} g_otaCtx;
#endif

typedef enum {
    REPORT_TYPE_RESTORE = 0,
    REPORT_TYPE_REBOOT,
    REPORT_TYPE_PROFILE,
    REPORT_TYPE_CLEAR_CONFIG,
    REPORT_TYPE_DIAG,
    REPORT_TYPE_BASE_CONFIG,
    REPORT_TYPE_BASE_CONFIG_PROFILE,
    REPORT_TYPE_OTA_RESULT,
    REPORT_TYPE_MAX,
} DemoReportType;

typedef struct {
    uint8_t type;
    uint8_t cnt;
    uint16_t id;
    uint32_t ts;
    union {
        struct {
            uint8_t num;
            uint8_t siid[HIMCUL_CONF_REPORT_SVC_MAX_NUM];
            uint8_t ciid[HIMCUL_CONF_REPORT_SVC_MAX_NUM];
        } profile;
        struct {
            uint32_t code;
            char info[HIMCUL_CONF_DIAG_INFO_MAX_LEN];
        } diag;
#if HIMCUL_CONF_OTA_SUPPORT
        uint8_t otaResult;
#endif
    } ctx;
} DEMO_ReportItem;

static struct ReportContext {
    uint8_t index;
    uint8_t cnt;
    DEMO_ReportItem item[HIMCUL_CONF_REPORT_QUEUE_MAX_NUM];
} g_reportCtx;

static struct ConnectContext {
    uint32_t watchDogInterval;
    uint32_t beforeTs;
    bool conn;
} g_connCtx;


static void ClearSendBuffer(void)
{
    g_sendBuffer.len = 0;
}

static void ReportCtxDequeue(void)
{
    g_reportCtx.index = (g_reportCtx.index + 1) % HIMCUL_CONF_REPORT_QUEUE_MAX_NUM;
    g_reportCtx.cnt = g_reportCtx.cnt > 0 ? g_reportCtx.cnt - 1 : 0;
}

static void ReportAckDequeue(uint16_t id)
{
    if (g_reportCtx.index >= HIMCUL_CONF_REPORT_QUEUE_MAX_NUM) {
        HIMCUL_LOGE("invalid ctx index, index=%u", g_reportCtx.index);
        return;
    }
    if (g_reportCtx.item[g_reportCtx.index].id != id) {
        HIMCUL_LOGI("ack not expect, exp=%u recv=%u", g_reportCtx.item[g_reportCtx.index].id, id);
        return;
    }

    HIMCUL_LOGN("ack recv, type=%u id=%u cnt=%u", g_reportCtx.item[g_reportCtx.index].type,
        g_reportCtx.item[g_reportCtx.index].id,
        g_reportCtx.item[g_reportCtx.index].cnt);
    ReportCtxDequeue();
}

#if HIMCUL_CONF_UART_RING_BUFFER_SUPPORT
int32_t HIMCUL_FWK_WriteDataRingBuffer(const uint8_t *data, uint32_t len)
{
    /* 串口过来的数据写入环形缓冲区 */
    return HIMCUL_RingBufferWrite(&g_ringBuffer, data, len);
}

static int32_t RingBufferRecvHandler(uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{
    return HIMCUL_RingBufferRead(&g_ringBuffer, buf, len);
}
#endif

static int32_t BaseCmdPutAndGetReqHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    HIMCUL_LOGN_FRAME(frame, "recv profile cmd");
    ClearSendBuffer();
    /* 填充错误码 */
    int32_t ret = HIMCUL_UtilsAddUint32BigEnd(&g_sendBuffer, HIMCUL_TRANS_OK);
    if (ret != 0) {
        HIMCUL_LOGE("add errcode error, ret=%d", ret);
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_CONTROL);
    }

    ret = HIMCUL_ProfileDispatch(ctx, frame, &g_sendBuffer);
    if (ret != 0) {
        HIMCUL_LOGE("dispatch profile cmd error, ret=%d", ret);
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_CONTROL);
    }
    return HIMCUL_TransSendResponse(ctx, frame->cmd, frame->id, g_sendBuffer.buffer, g_sendBuffer.len);
}

static int32_t BaseCmdEventReqHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL && frame->data != NULL && frame->len >= sizeof(uint32_t),
        HIMCUL_ERR_PARAM_INVALID, "param invalid");
    uint32_t event;
    int32_t ret = memcpy_s(&event, sizeof(event), frame->data, sizeof(uint32_t));
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&event, sizeof(event), frame->data, sizeof(uint32_t));
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    event = HIMCUL_UtilsNtohl(event);
    HIMCUL_LOGN_FRAME(frame, "recv event, event=%u", event);
    switch (event) {
        case HIMCUL_EVENT_MODULE_NEED_CONFIG:
            /* 静态配置上报  */
            ret = HIMCUL_FWK_BaseConfigReport();
            if (ret != 0) {
                HIMCUL_LOGE("base conf report error, ret=%d", ret);
            }
            ret = HIMCUL_FWK_BaseConfigProfileReport();
            if (ret != 0) {
                HIMCUL_LOGE("profile conf report error, ret=%d", ret);
            }
            break;
        case HIMCUL_EVENT_MODULE_RESTART:
            /* 模组重启后应上报所有可上报服务 */
            ret = HIMCUL_FWK_ProfileReportAll();
            if (ret != 0) {
                HIMCUL_LOGE("report all error, ret=%d", ret);
            }
            break;
        default:
            break;
    }
    HIMCUL_PROD_EventProcess(event);
    /* 返回ACK */
    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

/**
 * @brief       模组侧心跳，15s一个，可用于看门狗，非OTA场景下过长时 \n
 *              间无心跳时复位模组，断连后首次心跳上报所有可上报服务
 *
 */
static int32_t BaseCmdHeartbeatReqHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");

#if HIMCUL_CONF_REBOOT_CLEAN_CONFIG
    static bool firstHearbeat = true;
    if (firstHearbeat) {
        firstHearbeat = false;
        HIMCUL_FWK_ClearConfig();
    }
#endif

    if (!g_connCtx.conn) {
        /* 断连后首次连接需要上报数据 */
        int32_t ret = HIMCUL_FWK_ProfileReportAll();
        if (ret != 0) {
            HIMCUL_LOGE("report all error, ret=%d", ret);
        }
        g_connCtx.conn = true;
    }

    g_connCtx.watchDogInterval = 0;
    HIMCUL_LOGI_FRAME(frame, "recv heartbeat");
    return HIMCUL_TransSendResponse(ctx, frame->cmd, frame->id, NULL, 0);
}

/**
 * @brief       ACK监听
 *
 */
static int32_t CommAckHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    HIMCUL_LOGN_FRAME(frame, "recv ack");
    ReportAckDequeue(frame->id);
    return HIMCUL_OK;
}

#if HIMCUL_CONF_OTA_SUPPORT
static int32_t OtaCmdStartRequestHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
#define OTA_START_REQ_LEN 5
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL && frame->data != NULL && frame->len >= OTA_START_REQ_LEN,
        HIMCUL_ERR_PARAM_INVALID, "param invalid");

    uint8_t type = frame->data[0];
    uint32_t otaSize;
    int32_t ret = memcpy_s(&otaSize, sizeof(otaSize), &frame->data[1], sizeof(uint32_t));
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&otaSize, sizeof(otaSize), &frame->data[1], sizeof(uint32_t));
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    otaSize = HIMCUL_UtilsNtohl(otaSize);
    HIMCUL_LOGN_FRAME(frame, "recv ota req, type=%u size=%u", type, otaSize);

    HIMCUL_OtaEnableFlag flag = HIMCUL_PROD_OtaGetEnableFlag((HIMCUL_OtaStartType)type);
    if (flag != HIMCUL_OTA_REBOOT_FLAG_ENABLE) {
        HIMCUL_LOGN("can not ota, ret=%d flag=%u", ret, flag);
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA_NOT_READY);
    }

    ret = HIMCUL_PROD_OtaInit(otaSize);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("ota init error, ret=%d", ret);
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA_SIZE_INVALID);
    }

    (void)memset_s(&g_otaCtx, sizeof(g_otaCtx), 0, sizeof(g_otaCtx));
    HIMCUL_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING);
    g_otaCtx.otaSize = otaSize;
    g_otaCtx.crc = 0xFFFFFFFF;
    g_otaCtx.ts = ctx->curMs;
    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

static void OtaFailedProcess(void)
{
    int32_t ret = HIMCUL_PROD_OtaDeinit(HIMCUL_OTA_END_TYPE_FAILED);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("ota end error, ret=%u", ret);
    }
    (void)memset_s(&g_otaCtx, sizeof(g_otaCtx), 0, sizeof(g_otaCtx));
}

static int32_t OtaCmdFinishRequestHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    if (!HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING)) {
        HIMCUL_LOGE("not in ota state");
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }

    if (g_otaCtx.otaSize != g_otaCtx.nextOffset) {
        HIMCUL_LOGE_FRAME(frame, "ota finish data error, size=%u recv=%u", g_otaCtx.otaSize, g_otaCtx.nextOffset);
        OtaFailedProcess();
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA_DATA_INCOMPLETE);
    }
    HIMCUL_LOGN_FRAME(frame, "ota finish data trans, size=%u time=%u", g_otaCtx.otaSize,
        HIMCUL_DeltaTime(ctx->curMs, g_otaCtx.ts));

    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

static int32_t OtaCmdErrorRequestHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    if (!HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING)) {
        HIMCUL_LOGE("not in ota state");
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }
    OtaFailedProcess();
    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

static int32_t DEMO_OtaPackageRequestHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL && frame->data != NULL && frame->len > sizeof(uint32_t),
        HIMCUL_ERR_PARAM_INVALID, "param invalid");
    if (!HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING)) {
        HIMCUL_LOGE("not in ota state");
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }

    uint32_t offset;
    int32_t ret = memcpy_s(&offset, sizeof(offset), frame->data, sizeof(uint32_t));
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&offset, sizeof(offset), frame->data, sizeof(uint32_t));
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    offset = HIMCUL_UtilsNtohl(offset);

    const uint8_t *otaBin = frame->data + sizeof(uint32_t);
    uint32_t otaLen = frame->len - sizeof(uint32_t);
    HIMCUL_LOGN_FRAME(frame, "recv ota data, len=%u offset=%u/%u", otaLen, offset, g_otaCtx.otaSize);
    if (offset < g_otaCtx.nextOffset) {
        /* 重传 */
        HIMCUL_LOGI("ota packet retrans, expect=%u recv=%u", g_otaCtx.nextOffset, offset);
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
    } else if (offset != g_otaCtx.nextOffset) {
        /* 未按顺序收取 */
        HIMCUL_LOGE("ota packet seq error, expect=%u recv=%u", g_otaCtx.nextOffset, offset);
        OtaFailedProcess();
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }

    ret = HIMCUL_PROD_OtaWrite(otaBin, otaLen, offset);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("ota write error, ret=%d", ret);
        OtaFailedProcess();
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }

    (void)HIMCUL_PROD_OtaUpdateCrc(&g_otaCtx.crc, otaBin, otaLen);
    g_otaCtx.nextOffset += otaLen;
    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

static int32_t OtaCmdChecksumRequestHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL && frame->data != NULL && frame->len >= sizeof(uint32_t),
        HIMCUL_ERR_PARAM_INVALID, "param invalid");
    if (!HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING)) {
        HIMCUL_LOGE("not in ota state");
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA);
    }

    uint32_t checksum;
    int32_t ret = memcpy_s(&checksum, sizeof(checksum), frame->data, sizeof(uint32_t));
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&checksum, sizeof(checksum), frame->data, sizeof(uint32_t));
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    checksum = HIMCUL_UtilsNtohl(checksum);
    HIMCUL_LOGN_FRAME(frame, "recv ota crc, crc=%u", checksum);

    /* 校验传输过程中的crc */
    if (checksum != g_otaCtx.crc) {
        HIMCUL_LOGE("crc not match, recv=%u calc=%u", checksum, g_otaCtx.crc);
        OtaFailedProcess();
        return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_ERR_OTA_CRC_NOT_MATCH);
    }
    HIMCUL_BIT_SET(g_otaCtx.flag, OTA_FLAG_FINISH);
    return HIMCUL_TransSendResponseCode(ctx, frame->cmd, frame->id, HIMCUL_TRANS_OK);
}

static int32_t OtaCmdReportResultAckHandler(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame)
{
    HIMCUL_CHECK_RETURN_LOGW(frame != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    HIMCUL_LOGN_FRAME(frame, "recv ota ack");
    ReportAckDequeue(frame->id);
    if (HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_REBOOT)) {
        int32_t ret = HIMCUL_PROD_OtaReboot();
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGF("reboot error, ret=%d", ret);
        }
        HIMCUL_BIT_CLR(g_otaCtx.flag);
    }
    return HIMCUL_OK;
}
#endif

/**
 * @brief       支持的himcu指令
 *
 */
static const HIMCUL_TransEndpoint g_endpoint[] = {
    /* 控制指令下发 */
    {HIMCUL_CMD_BASE_SVC_PUT, BaseCmdPutAndGetReqHandler},
    /* 查询指令下发 */
    {HIMCUL_CMD_BASE_SVC_GET, BaseCmdPutAndGetReqHandler},
    /* 模组侧事件通知 */
    {HIMCUL_CMD_BASE_EVENT_NOTIFY, BaseCmdEventReqHandler},
    /* 模组侧心跳 */
    {HIMCUL_CMD_BASE_HEARTBEAT, BaseCmdHeartbeatReqHandler},
    /* 基础信息上报响应 */
    {HIMCUL_CMD_BASE_INFO_CONFIG, CommAckHandler},
    /* 服务属性上报的响应处理 */
    {HIMCUL_CMD_BASE_SVC_REPORT, CommAckHandler},
    /* 重置响应处理 */
    {HIMCUL_CMD_BASE_RESTORE_FACTORY, CommAckHandler},
    /* 重启响应处理 */
    {HIMCUL_CMD_BASE_REBOOT, CommAckHandler},
    /* 重置配置响应处理 */
    {HIMCUL_CMD_BASE_CLEAR_CONFIG, CommAckHandler},
#if HIMCUL_CONF_OTA_SUPPORT
    /* 通知升级开始 */
    {HIMCUL_CMD_UPGRADE_START, OtaCmdStartRequestHandler},
    /* 通知升级传输结束 */
    {HIMCUL_CMD_UPGRADE_TRANS_FINISH, OtaCmdFinishRequestHandler},
    /* 通知升级传输异常 */
    {HIMCUL_CMD_UPGRADE_TRANS_ERROR, OtaCmdErrorRequestHandler},
    /* 下发升级包 */
    {HIMCUL_CMD_UPGRADE_SEND_PACKET, DEMO_OtaPackageRequestHandler},
    /* 校验checksum */
    {HIMCUL_CMD_UPGRADE_CHECKSUM, OtaCmdChecksumRequestHandler},
    /* 上报升级结果 */
    {HIMCUL_CMD_UPGRADE_RESULT_REPORT, OtaCmdReportResultAckHandler},
#endif
};

static bool ReportRestoreHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_RESTORE_FACTORY, &curItem->id, NULL, 0) == HIMCUL_OK;
}

static bool ReportRebootHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_REBOOT, &curItem->id, NULL, 0) == HIMCUL_OK;
}

static bool ReportClearConfigHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_CLEAR_CONFIG, &curItem->id, NULL, 0) == HIMCUL_OK;
}

static int32_t ReportTargetGetProcess(DEMO_ReportItem *curItem, HIMCUL_Buffer *buffer)
{
    int32_t ret;
    uint32_t cnt = 0;

    if (curItem->ctx.profile.num > HIMCUL_CONF_REPORT_SVC_MAX_NUM) {
        HIMCUL_LOGE("report num invalid, num=%u", curItem->ctx.profile.num);
        return HIMCUL_ERR_REPORT_CTX_INVALID;
    }

    for (uint8_t i = 0; i < curItem->ctx.profile.num &&
        curItem->ctx.profile.num <= HIMCUL_CONF_REPORT_SVC_MAX_NUM; ++i) {
        uint8_t siid = curItem->ctx.profile.siid[i];
        uint8_t ciid = curItem->ctx.profile.ciid[i];

        ret = HIMCUL_PROD_BaseProfileReportHandler(siid, ciid, buffer);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("report get error, ret=%d siid=%u ciid=%u", ret, siid, ciid);
        } else {
            ++cnt;
        }
    }

    /* 部分成功可以上报 */
    return cnt > 0 ? 0 : -1;
}

static bool ReportProfileSvcHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    ClearSendBuffer();

    int32_t ret;
    if (curItem->ctx.profile.num == 0xFF) {
        /* 为0时全量上报 */
        ret = HIMCUL_PROD_BaseProfileReportAllHandler(&g_sendBuffer);
    } else {
        ret = ReportTargetGetProcess(curItem, &g_sendBuffer);
    }
    if (ret != HIMCUL_OK) {
        return false;
    }

    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_SVC_REPORT, &curItem->id,
        g_sendBuffer.buffer, g_sendBuffer.len) == HIMCUL_OK;
}

static bool ReportDiagHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    ClearSendBuffer();

    int32_t ret = HIMCUL_UtilsAddUint32BigEnd(&g_sendBuffer, curItem->ctx.diag.code);
    if (ret != 0) {
        HIMCUL_LOGE("add code to buf error, ret=%d", ret);
        return false;
    }

    if (curItem->ctx.diag.info[0] != '\0') {
        ret = HIMCUL_UtilsAddData(&g_sendBuffer, (const uint8_t *)curItem->ctx.diag.info,
            HIMCUL_PROD_Strnlen(curItem->ctx.diag.info, sizeof(curItem->ctx.diag.info)));
        if (ret != 0) {
            HIMCUL_LOGE("add info to buf error, ret=%d", ret);
            return false;
        }
    }
    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_DIAGNOSE_REPORT, &curItem->id,
        g_sendBuffer.buffer, g_sendBuffer.len) == HIMCUL_OK;
}

static bool ReportBaseConfigHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    ClearSendBuffer();

    int32_t ret = HIMCUL_PROD_BuildBaseConfig(&g_sendBuffer);
    if (ret != 0) {
        HIMCUL_LOGE("build base config error, ret=%d", ret);
        return false;
    }
    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_BASE_INFO_CONFIG, &curItem->id,
        g_sendBuffer.buffer, g_sendBuffer.len) == HIMCUL_OK;
}

static bool ReportBaseConfigProfileHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    ClearSendBuffer();

    int32_t ret = HIMCUL_PROD_BuildBaseConfigProfile(&g_sendBuffer);
    if (ret != 0) {
        HIMCUL_LOGE("build base config error, ret=%d", ret);
        return false;
    }
    return HIMCUL_TransSendBaseConfig(&g_himcuCtx, &curItem->id, HIMCUL_TAG_CONFIG_SVC_INFO,
        g_sendBuffer.buffer, g_sendBuffer.len) == HIMCUL_OK;
}

#if HIMCUL_CONF_OTA_SUPPORT
static bool DemoReportOtaResultHandler(uint8_t type, DEMO_ReportItem *curItem)
{
    HIMCUL_NOT_USED(type);
    ClearSendBuffer();

    return HIMCUL_TransSendRequest(&g_himcuCtx, HIMCUL_CMD_UPGRADE_RESULT_REPORT, &curItem->id,
        &curItem->ctx.otaResult, sizeof(curItem->ctx.otaResult)) == HIMCUL_OK;
}
#endif

static struct ReportHandler {
    uint8_t type;
    bool (*handler)(uint8_t type, DEMO_ReportItem *curItem);
} g_reportHandler[] = {
    {REPORT_TYPE_RESTORE, ReportRestoreHandler},
    {REPORT_TYPE_REBOOT, ReportRebootHandler},
    {REPORT_TYPE_PROFILE, ReportProfileSvcHandler},
    {REPORT_TYPE_CLEAR_CONFIG, ReportClearConfigHandler},
    {REPORT_TYPE_DIAG, ReportDiagHandler},
    {REPORT_TYPE_BASE_CONFIG, ReportBaseConfigHandler},
    {REPORT_TYPE_BASE_CONFIG_PROFILE, ReportBaseConfigProfileHandler},
#if HIMCUL_CONF_OTA_SUPPORT
    {REPORT_TYPE_OTA_RESULT, DemoReportOtaResultHandler},
#endif
};

/* 返回false则将当前处理的上报对象出队 */
static bool ReportItemProcess(DEMO_ReportItem *curItem, uint32_t curMs)
{
    if (curItem->cnt != 0 && HIMCUL_DeltaTime(curMs, curItem->ts) < HIMCUL_RETRANS_INTERVAL_MS) {
        /* 重发未超时 */
        return true;
    }
    if (curItem->cnt >= HIMCUL_RETRANS_MAX_CNT) {
        /* 重发次数超过最大值 */
        HIMCUL_LOGE("msg send over max num, cnt=%u type=%u", curItem->cnt, curItem->type);
        return false;
    }

    curItem->cnt++;
    curItem->ts = curMs;

    for (uint32_t i = 0; i < HIMCUL_ARRAY_SIZE(g_reportHandler); ++i) {
        if (g_reportHandler[i].type != curItem->type) {
            continue;
        }
        if (g_reportHandler[i].handler == NULL) {
            HIMCUL_LOGE("handler null, ctype=%u", curItem->type);
            return false;
        }

        if (!g_reportHandler[i].handler(curItem->type, curItem)) {
            HIMCUL_LOGE("handler process error, type=%u", curItem->type);
            return false;
        } else {
            HIMCUL_LOGI("handler process ok, type=%u cnt=%u ts=%u", curItem->type, curItem->cnt, curItem->ts);
            return true;
        }
    }
    return false;
}

static void ReportLoop(uint32_t curMs)
{
    struct ReportContext *ctx = &g_reportCtx;

    if (ctx->cnt == 0) {
        /* 无待上报数据 */
        return;
    }

    if (ctx->cnt > HIMCUL_CONF_REPORT_QUEUE_MAX_NUM || ctx->index >= HIMCUL_CONF_REPORT_QUEUE_MAX_NUM) {
        /* 异常恢复 */
        ctx->cnt = 0;
        ctx->index = 0;
        return;
    }

    if (!ReportItemProcess(&ctx->item[ctx->index], curMs)) {
        ReportCtxDequeue();
    }
}

static void HeartbeatProcess(uint32_t curMs)
{
    struct ConnectContext *ctx = &g_connCtx;
    if (!ctx->conn) {
        return;
    }
    if (ctx->beforeTs == 0) {
        ctx->beforeTs = curMs;
        return;
    }

    ctx->watchDogInterval += HIMCUL_DeltaTime(curMs, ctx->beforeTs);
    ctx->beforeTs = curMs;
    if (ctx->watchDogInterval >= HIMCUL_HEARTBEAT_MAX_INTERVAL_MS) {
        HIMCUL_LOGE("lose heartbeat with module");
        ctx->conn = false;
    }
}

static DEMO_ReportItem *InsertReportItem(DemoReportType type)
{
    /* 回厂/重启/清除配置 最高优先级 直接清空现有队列 */
    if (type == REPORT_TYPE_RESTORE || type == REPORT_TYPE_REBOOT || type == REPORT_TYPE_CLEAR_CONFIG) {
        g_reportCtx.cnt = 1;
        g_reportCtx.index = 0;
        (void)memset_s(&g_reportCtx.item[0], sizeof(DEMO_ReportItem), 0, sizeof(DEMO_ReportItem));
        g_reportCtx.item[0].type = type;
        return &g_reportCtx.item[0];
    }

    /* 部分上报可去重合并 */
    if (g_reportCtx.cnt > 1 && (type == REPORT_TYPE_PROFILE || type == REPORT_TYPE_BASE_CONFIG ||
        type == REPORT_TYPE_BASE_CONFIG_PROFILE || type == REPORT_TYPE_OTA_RESULT)) {
        uint8_t cnt = 0;
        uint8_t index = g_reportCtx.index;
        while (cnt < g_reportCtx.cnt - 1) {
            index = (index + 1) % HIMCUL_CONF_REPORT_QUEUE_MAX_NUM;
            if (g_reportCtx.item[index].type != type) {
                ++cnt;
                continue;
            }
            /* profile可多个siid ciid合并上报，去重需要求有空位 */
            if (type != REPORT_TYPE_PROFILE ||
                g_reportCtx.item[index].ctx.profile.num < HIMCUL_CONF_REPORT_SVC_MAX_NUM) {
                return &g_reportCtx.item[index];
            }
            ++cnt;
        }
    }

    /* 队列满 */
    if (g_reportCtx.cnt >= HIMCUL_CONF_REPORT_QUEUE_MAX_NUM) {
        HIMCUL_LOGE("queue full, cnt=%u", g_reportCtx.cnt);
        return NULL;
    }

    g_reportCtx.cnt++;
    uint8_t index = (g_reportCtx.index + g_reportCtx.cnt - 1) % HIMCUL_CONF_REPORT_QUEUE_MAX_NUM;
    (void)memset_s(&g_reportCtx.item[index], sizeof(DEMO_ReportItem), 0, sizeof(DEMO_ReportItem));
    g_reportCtx.item[index].type = type;
    return &g_reportCtx.item[index];
}

int32_t HIMCUL_FWK_ProfileReportTarget(uint8_t siid, uint8_t ciid)
{
    DEMO_ReportItem *reportItem = InsertReportItem(REPORT_TYPE_PROFILE);
    if (reportItem == NULL || reportItem->ctx.profile.num >= HIMCUL_CONF_REPORT_SVC_MAX_NUM) {
        return HIMCUL_ERR_REPORT_QUEUE_FULL;
    }
    if (reportItem->ctx.profile.num == 0xFF) {
        /* 已有全量数据上报在队列中 */
        return HIMCUL_OK;
    }

    reportItem->ctx.profile.siid[reportItem->ctx.profile.num] = siid;
    reportItem->ctx.profile.ciid[reportItem->ctx.profile.num] = ciid;
    reportItem->ctx.profile.num++;
    return HIMCUL_OK;
}

int32_t HIMCUL_FWK_ProfileReportAll(void)
{
    DEMO_ReportItem *reportItem = InsertReportItem(REPORT_TYPE_PROFILE);
    if (reportItem == NULL) {
        return HIMCUL_ERR_REPORT_QUEUE_FULL;
    }

    /* 0xFF表示全量上报 */
    reportItem->ctx.profile.num = 0xFF;
    return HIMCUL_OK;
}

int32_t HIMCUL_FWK_RestoreModule(void)
{
    return InsertReportItem(REPORT_TYPE_RESTORE) == NULL ? -1 : 0;
}

int32_t HIMCUL_FWK_RebootModule(void)
{
    return InsertReportItem(REPORT_TYPE_REBOOT) == NULL ? -1 : 0;
}

int32_t HIMCUL_FWK_ClearConfig(void)
{
    return InsertReportItem(REPORT_TYPE_CLEAR_CONFIG) == NULL ? -1 : 0;
}

int32_t HIMCUL_FWK_BaseConfigReport(void)
{
    return InsertReportItem(REPORT_TYPE_BASE_CONFIG) == NULL ? -1 : 0;
}

int32_t HIMCUL_FWK_BaseConfigProfileReport(void)
{
    return InsertReportItem(REPORT_TYPE_BASE_CONFIG_PROFILE) == NULL ? -1 : 0;
}

int32_t HIMCUL_FWK_ReportDiagnoseInfo(uint32_t code, const char *info)
{
    DEMO_ReportItem *reportItem = InsertReportItem(REPORT_TYPE_DIAG);
    if (reportItem == NULL) {
        return HIMCUL_ERR_REPORT_QUEUE_FULL;
    }

    reportItem->ctx.diag.code = code;
    int32_t ret = strcpy_s(reportItem->ctx.diag.info, sizeof(reportItem->ctx.diag.info), info);
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_STRNCPY;
    }
    return HIMCUL_OK;
}

#if HIMCUL_CONF_OTA_SUPPORT
static void OtaResultReport(uint8_t result)
{
    DEMO_ReportItem *reportItem = InsertReportItem(REPORT_TYPE_OTA_RESULT);
    if (reportItem == NULL) {
        return;
    }

    reportItem->ctx.otaResult = result;
}

static bool OtaCrcCheckProcess(uint32_t crc, uint32_t size)
{
    uint32_t ret;
#if HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT
    uint32_t curCrc = 0xFFFFFFFF;
    uint32_t offset = 0;
    while (offset < size) {
        uint32_t bufSize = HIMCUL_MIN(size - offset, sizeof(g_sendBufRaw));
        /* 复用发送buffer */
        ret = HIMCUL_PROD_OtaRead(g_sendBufRaw, bufSize, offset);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("ota read error, ret=%d offset=%u", ret, offset);
            return false;
        }
        offset += bufSize;

        ret = HIMCUL_PROD_OtaUpdateCrc(&curCrc, g_sendBufRaw, bufSize);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("crc update error, ret=%d", ret);
            return false;
        }
        HIMCUL_LOGD("crc calc, offset=%u/%u", offset, size);
    }
    if (curCrc != crc) {
        HIMCUL_LOGE("ota read check crc error, cur=%d recv=%u", curCrc, crc);
        return false;
    }
#else
    ret = HIMCUL_PROD_OtaCheckCrc(crc);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("crc check error, ret=%d crc=%u", ret, crc);
        return false;
    }
#endif
    return true;
}

static void OtaFinishProcess(uint32_t curMs)
{
    if (!HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_RUNNING)) {
        return;
    }

    int32_t ret;
    if (HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_FINISH)) {
        HIMCUL_BIT_RESET(g_otaCtx.flag, OTA_FLAG_FINISH);
        if (!OtaCrcCheckProcess(g_otaCtx.crc, g_otaCtx.otaSize)) {
            HIMCUL_LOGE("crc check error");
            OtaResultReport(HIMCUL_OTA_RESULT_ERR_CRC_NOT_MATCH);
            OtaFailedProcess();
            return;
        }
        ret = HIMCUL_PROD_OtaDeinit(HIMCUL_OTA_END_TYPE_SUCCESS);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE("ota end error, ret=%d", ret);
            OtaResultReport(HIMCUL_OTA_RESULT_ERR_INNER);
            OtaFailedProcess();
            return;
        }
        g_otaCtx.ts = curMs;
        HIMCUL_BIT_SET(g_otaCtx.flag, OTA_FLAG_REBOOT);
        OtaResultReport(HIMCUL_OTA_RESULT_OK);
    }

    /* 超时未收到ACK则强制重启 */
    if (HIMCUL_IS_BIT_SET(g_otaCtx.flag, OTA_FLAG_REBOOT) &&
        HIMCUL_DeltaTime(curMs, g_otaCtx.ts) > (HIMCUL_RETRANS_MAX_CNT * HIMCUL_RETRANS_INTERVAL_MS)) {
        HIMCUL_BIT_RESET(g_otaCtx.flag, OTA_FLAG_REBOOT);
        HIMCUL_LOGI("ota no ack reboot");
        ret = HIMCUL_PROD_OtaReboot();
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGF("reboot error, ret=%d", ret);
        }
        HIMCUL_BIT_CLR(g_otaCtx.flag);
    }
}

#endif

void HIMCUL_FWK_Loop(uint32_t curMs)
{
    int32_t ret = HIMCUL_TransIoLoop(&g_himcuCtx, curMs, HIMCUL_CONF_IO_LOOP_RECV_TIMEOUT_MS);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("io loop error, ret=%d", ret);
    }

    ReportLoop(curMs);
    HeartbeatProcess(curMs);
    OtaFinishProcess(curMs);
}

int32_t HIMCUL_FWK_Init(void)
{
    static HIMCUL_BaseCallback baseCb = {
        .strtolCb = HIMCUL_PROD_Strtol,
#if HIMCUL_CONF_LOG_SUPPORT
        .logOutput = HIMCUL_PROD_LogOutput
#endif
    };
    int32_t ret = HIMCUL_BaseRegisterCallback(&baseCb);
    if (ret != 0) {
        HIMCUL_LOGE("register callback error, ret=%d", ret);
        return ret;
    }

#if HIMCUL_CONF_UART_RING_BUFFER_SUPPORT
    ret = HIMCUL_RingBufferInit(&g_ringBuffer, g_ringBufRaw, sizeof(g_ringBufRaw));
	if (ret != 0) {
        HIMCUL_LOGE("init ring buffer error, ret=%d", ret);
        return ret;
    }
    g_himcuCtx.rxHandler = RingBufferRecvHandler;
#else
    g_himcuCtx.rxHandler = HIMCUL_PROD_TransRecvHandler;
#endif
    g_himcuCtx.txHandler = HIMCUL_PROD_TransSendHandler;

    g_himcuCtx.recvBuf = g_recvBufRaw;
    g_himcuCtx.recvBufSize = sizeof(g_recvBufRaw);
    g_himcuCtx.endpoints = g_endpoint;
    g_himcuCtx.endpointNum = HIMCUL_ARRAY_SIZE(g_endpoint);
    g_himcuCtx.putHandler = HIMCUL_PROD_BaseProfilePutHandler;
    g_himcuCtx.getHandler = HIMCUL_PROD_BaseProfileGetHandler;

    ret =  HIMCUL_PROD_TransInit();
    if (ret != 0) {
        HIMCUL_LOGE("trans init error, ret=%d", ret);
        return ret;
    }
    HIMCUL_LOGN("fwk init ok");
	return HIMCUL_OK;
}