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
#include "himcul_core.h"
#include "securec.h"

#define HIMCUL_FRAME_HEADER_LEN 8
#define HIMCU_TRANS_FRAME_CHECKSUM_MOD 256
#define TAG_BUF_LEN 64
#define HIMCU_TRANS_FRAME_MAGIC_FIRST 0xA5
#define HIMCU_TRANS_FRAME_MAGIC_SECOND 0x5A
#define HIMCU_TRANS_FRAME_VER 1
#define HIMCU_TRANS_FRAME_MAGIC_INVALID 0xFF
#define HIMCU_TRANS_FRAME_CHECKSUM_LEN 1
#define HIMCU_BIT_PER_BYTE 8
#define PDU_DUMP_BUFFER_SIZE 64
#define PDU_DUMP_LEN_PER_BYTE 3
#define TLV_1T_2L_HEADER_LEN 3
#define HIMCU_TRANS_FRAME_MAGIC_BIG_END \
    (HIMCU_TRANS_FRAME_MAGIC_FIRST << HIMCU_BIT_PER_BYTE | HIMCU_TRANS_FRAME_MAGIC_SECOND)

typedef struct {
    uint8_t siid;
    HIMCUL_Context *ctx;
    const HIMCUL_TransFrame *frame;
    HIMCUL_Buffer *buf;
} TlvWalkerParam;

typedef struct {
    HIMCUL_RingBuffer *rb;
    const uint8_t *data;
    uint32_t len;
    uint32_t head;
    uint32_t tail;
} RingBufferWriteParam;

typedef enum {
    RECV_STATE_RECV_HEADER = 0,
    RECV_STATE_CHECK_HEADER,
    RECV_STATE_RECV_BODY,
    RECV_STATE_CHECK_SUM,
    RECV_STATE_DISPATCH,
} RecvState;

typedef struct {
    const uint8_t *data;
    uint32_t len;
} SendDataItem;

#define UTILS_FSM_INIT(_fsm, _name, _init, _onChange, _tbl) \
    do { \
        (_fsm)->name = (_name); \
        (_fsm)->cur = (_init); \
        (_fsm)->onChange = (_onChange); \
        (_fsm)->tbl = (_tbl); \
        (_fsm)->num = HIMCUL_ARRAY_SIZE(_tbl); \
    } while (0)

static const HIMCUL_BaseCallback *g_baseCallback = NULL;
static uint8_t g_logLevel = HIMCUL_CONF_LOG_DEFAULT_LEVEL;

static int32_t HIMCUL_FsmRunning(HIMCUL_Fsm *fsm, void *param)
{
    if (fsm == NULL || fsm->tbl == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    if (fsm->cur < 0) {
        return fsm->cur;
    }

    int32_t before = fsm->cur;
    bool find = false;
    for (uint32_t i = 0; i < fsm->num; ++i) {
        if (fsm->tbl[i].state == fsm->cur && fsm->tbl[i].handler != NULL) {
            fsm->cur = fsm->tbl[i].handler(param, fsm->cur);
            find = true;
            break;
        }
    }
    if (!find) {
        fsm->cur = before;
    } else if (fsm->cur != before && fsm->onChange != NULL) {
        fsm->onChange(fsm, before, fsm->cur);
    }
    return fsm->cur;
}

static void HIMCUL_FsmSwitch(HIMCUL_Fsm *fsm, int32_t nextId)
{
    if (fsm == NULL || nextId < 0) {
        return;
    }
    if (nextId != fsm->cur) {
        if (fsm->onChange != NULL) {
            fsm->onChange(fsm, fsm->cur, nextId);
        }
        fsm->cur = nextId;
    }
}

static int32_t AdapterStrtol(const char *nptr, char **endptr, int base)
{
    if (g_baseCallback == NULL || g_baseCallback->strtolCb == NULL) {
        return 0;
    }
    return g_baseCallback->strtolCb(nptr, endptr, base);
}

static int32_t AdapterLogOutput(uint8_t level, const char *tag, const char *fmt, va_list arg)
{
    if (g_baseCallback == NULL || g_baseCallback->logOutput == NULL) {
        return -1;
    }
    g_baseCallback->logOutput(level, tag, fmt, arg);
    return 0;
}

void HIMCUL_SetLogLevel(uint8_t level)
{
    uint8_t beforeLevel = g_logLevel;
    if (level >= HIMCUL_LOG_LEVEL_MAX) {
        g_logLevel = HIMCUL_LOG_LEVEL_MAX;
    } else {
        g_logLevel = level;
    }
    HIMCUL_LOGN("set log level, %u=>%u", beforeLevel, g_logLevel);
}

void HIMCUL_LogOutputImpl(uint8_t level, const char *funcName, uint32_t line, const char *fmt, ...)
{
    if (fmt == NULL || funcName == NULL || g_baseCallback == NULL || g_baseCallback->logOutput == NULL ||
        level > g_logLevel) {
        return;
    }

    if (level == 0) {
        level = HIMCUL_LOG_LEVEL_FATAL;
    } else if (level >= HIMCUL_LOG_LEVEL_MAX) {
        level = HIMCUL_LOG_LEVEL_DEBUG;
    }

    char tagBuf[TAG_BUF_LEN] = {0};
    if (sprintf_s(tagBuf, TAG_BUF_LEN, "%s:%u ", funcName, line) <= 0) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    AdapterLogOutput(level, tagBuf, fmt, ap);
    va_end(ap);
}

int32_t HIMCUL_ProfileCharToBool(const uint8_t *data, uint32_t len, bool *value)
{
    int32_t intValue;
    int32_t ret = HIMCUL_ProfileCharToInt(data, len, &intValue);
    if (ret != 0) {
        return ret;
    }
    *value = (intValue != 0);
    return HIMCUL_OK;
}

int32_t HIMCUL_ProfileCharToInt(const uint8_t *data, uint32_t len, int32_t *value)
{
    if (data == NULL || len == 0 || value == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }
    /* 32位十进制数字最长不超过11字节，留结束符后12字节 */
    char valueBuf[12] = {0};
    int32_t ret = strncpy_s(valueBuf, sizeof(valueBuf), (const char *)data, len);
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_STRNCPY;
    }

    char *endPtr = NULL;
    *value = AdapterStrtol((const char *)valueBuf, &endPtr, 10);
    if (endPtr <= valueBuf) {
        return HIMCUL_ERR_ADAPTER_STRTOL;
    }
    return HIMCUL_OK;
}

int32_t HIMCUL_ProfileCharAddBool(uint8_t siid, uint8_t ciid, bool value, HIMCUL_Buffer *buf)
{
    char boolStr = value ? '1' : '0';
    return HIMCUL_ProfileCharAddStr(siid, ciid, &boolStr, sizeof(char), buf);
}

int32_t HIMCUL_ProfileCharAddInt(uint8_t siid, uint8_t ciid, int32_t value, HIMCUL_Buffer *buf)
{
    if (buf == NULL || buf->len > buf->size) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    /* 32位十进制数字最长不超过11字节，留结束符后12字节 */
    uint8_t valueBuf[12] = {0};
    int32_t ret = sprintf_s((char *)valueBuf, sizeof(valueBuf), "%d", value);
    if (ret <= 0 || ret >= sizeof(valueBuf)) {
        return HIMCUL_ERR_SECUREC_SPRINTF;
    }

    return HIMCUL_ProfileCharAddStr(siid, ciid, (const char *)valueBuf, ret, buf);
}

int32_t HIMCUL_ProfileCharAddStr(uint8_t siid, uint8_t ciid, const char *str,
    uint32_t len, HIMCUL_Buffer *buf)
{
    /* 校验len + 3避免翻转 */
    if (buf == NULL || str == NULL || len == 0 || buf->len > buf->size ||
        len > UINT32_MAX - TLV_1T_2L_HEADER_LEN) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    int32_t ret;
    do {
        ret = HIMCUL_UtilsAddUint8(buf, siid);
        if (ret != 0 ) {
            break;
        }
        /* 1 ciid + 2clen = 3 */
        ret = HIMCUL_UtilsAddUint16BigEnd(buf, TLV_1T_2L_HEADER_LEN + len);
        if (ret != 0 ) {
            break;
        }
        ret = HIMCUL_UtilsAddUint8(buf, ciid);
        if (ret != 0 ) {
            break;
        }
        ret = HIMCUL_UtilsAddUint16BigEnd(buf, len);
        if (ret != 0 ) {
            break;
        }
        ret = HIMCUL_UtilsAddData(buf, (const uint8_t *)str, len);
        if (ret != 0 ) {
            break;
        }
        return 0;
    } while (0);
    return ret;
}

/* 判断当前机器是否是小端 */
static bool IsLittleEndian(void)
{
    uint16_t x = 1;
    return *((uint8_t*)&x) == x;
}

/* 16位大小端转换 */
static uint16_t Swap16Bit(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

/* 32位大小端转换 */
static uint32_t Swap32Bit(uint32_t v)
{
    return ((v & 0x000000FFU) << 24) |
           ((v & 0x0000FF00U) << 8)  |
           ((v & 0x00FF0000U) >> 8)  |
           ((v & 0xFF000000U) >> 24);
}

uint16_t HIMCUL_UtilsHtons(uint16_t hs)
{
    return IsLittleEndian() ? Swap16Bit(hs) : hs;
}

uint32_t HIMCUL_UtilsHtonl(uint32_t hl)
{
    return IsLittleEndian() ? Swap32Bit(hl) : hl;
}

uint16_t HIMCUL_UtilsNtohs(uint16_t ns)
{
    return IsLittleEndian() ? Swap16Bit(ns) : ns;
}

uint32_t HIMCUL_UtilsNtohl(uint32_t nl)
{
    return IsLittleEndian() ? Swap32Bit(nl) : nl;
}

int32_t HIMCUL_UtilsAddTlv(HIMCUL_Buffer *buf, uint8_t type, const uint8_t *value, uint32_t valueLen)
{
    /* tag和len占用3字节 */
    if (buf == NULL || buf->len > buf->size || valueLen > UINT16_MAX ||
        buf->size - buf->len < TLV_1T_2L_HEADER_LEN + valueLen) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    buf->buffer[buf->len++] = type;
    buf->buffer[buf->len++] = (uint8_t)((valueLen >> 8) & 0xFF);
    buf->buffer[buf->len++] = (uint8_t)(valueLen & 0xFF);
    int32_t ret = memcpy_s(&buf->buffer[buf->len], buf->size - buf->len, value, valueLen);
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    buf->len += valueLen;
    return HIMCUL_OK;
}

int32_t HIMCUL_UtilsAddData(HIMCUL_Buffer *buf, const uint8_t *data, uint32_t dataLen)
{
    if (buf == NULL || data == NULL || buf->len > buf->size || buf->size - buf->len < dataLen) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    int32_t ret = memcpy_s(&buf->buffer[buf->len], buf->size - buf->len, data, dataLen);
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    buf->len += dataLen;
    return HIMCUL_OK;
}

int32_t HIMCUL_UtilsAddUint8(HIMCUL_Buffer *buf, uint32_t num)
{
    if (buf == NULL || num > UINT8_MAX || buf->len > buf->size || buf->size - buf->len < sizeof(uint8_t)) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    buf->buffer[buf->len++] = num;
    return HIMCUL_OK;
}

int32_t HIMCUL_UtilsAddUint16BigEnd(HIMCUL_Buffer *buf, uint32_t num)
{
    if (buf == NULL || num > UINT16_MAX || buf->len > buf->size || buf->size - buf->len < sizeof(uint16_t)) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint16_t nNum = HIMCUL_UtilsHtons((uint16_t)num);
    int32_t ret = memcpy_s(&buf->buffer[buf->len], buf->size - buf->len, &nNum, sizeof(uint16_t));
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    buf->len += sizeof(uint16_t);
    return HIMCUL_OK;
}

int32_t HIMCUL_UtilsAddUint32BigEnd(HIMCUL_Buffer *buf, uint32_t num)
{
    if (buf == NULL || buf->len > buf->size || buf->size - buf->len < sizeof(uint32_t)) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint32_t nNum = HIMCUL_UtilsHtonl(num);
    int32_t ret = memcpy_s(&buf->buffer[buf->len], buf->size - buf->len, &nNum, sizeof(uint32_t));
    if (ret != EOK) {
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    buf->len += sizeof(uint32_t);
    return HIMCUL_OK;
}

int32_t HIMCUL_BaseRegisterCallback(const HIMCUL_BaseCallback *cb)
{
    g_baseCallback = cb;
    return HIMCUL_OK;
}

static int32_t TlvWalker(const uint8_t *data, uint32_t len,
    int32_t (*handler)(uint8_t tag, const uint8_t *data, uint32_t len, void *param), void *param)
{
    if (data == NULL || len == 0 || handler == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint16_t index = 0;
    uint8_t tag = 0;
    uint32_t valueLen = 0;
    int32_t ret;
    while (index < len) {
        if (len - index < TLV_1T_2L_HEADER_LEN) {
            HIMCUL_LOGW("tlv invalid, len=%u index=%u", len, index);
            return HIMCUL_ERR_PARAM_INVALID;
        }
        tag = data[index++];
        uint16_t temp = 0;
        ret = memcpy_s((uint8_t *)&temp, sizeof(temp), &data[index], sizeof(uint16_t));
        if (ret != EOK) {
            HIMCUL_LOGE_MEMCPY((uint8_t *)&temp, sizeof(temp), &data[index], sizeof(uint16_t));
            return ret;
        }
        valueLen = HIMCUL_UtilsNtohs(temp);
        index += sizeof(uint16_t);
        if (valueLen > len - index) {
            HIMCUL_LOGW("tlv invalid, len=%u index=%u valueLen=%u", len, index, valueLen);
            return HIMCUL_ERR_PARAM_INVALID;
        }
        ret = handler(tag, data + index, valueLen, param);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGW("tlv handle error, ret=%d", ret);
            return ret;
        }

        index += valueLen;
    }
    return HIMCUL_OK;
}

static int32_t ProfileCharDispatch(uint8_t ciid, const uint8_t *data, uint32_t len, void *param)
{
    TlvWalkerParam *walkerParam = (TlvWalkerParam *)param;
    uint8_t siid = walkerParam->siid;
    int32_t ret;
    /* 重传消息不重复控制，仅查询上报 */
    if (HIMCUL_IS_BIT_SET(walkerParam->frame->flag, HIMCUL_FRAME_FLAG_RETRANS)) {
        if (walkerParam->ctx->getHandler == NULL) {
            HIMCUL_LOGW("put cmd get handler null, siid=%u ciid=%u len=%u", siid, ciid, len);
            /* 可能存在有put权限无get权限的属性，此处不中断流程 */
            return HIMCUL_OK;
        }

        ret = walkerParam->ctx->getHandler(siid, ciid, walkerParam->buf);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGW("get error, siid=%u ciid=%u len=%u", siid, ciid, len);
            return HIMCUL_ERR_PROFILE_GET;
        }
        return HIMCUL_OK;
    }

    if (walkerParam->ctx->putHandler == NULL) {
        HIMCUL_LOGW("put handler null, siid=%u ciid=%u len=%u", siid, ciid, len);
        return HIMCUL_ERR_PROFILE_DISPATCH;
    }

    ret = walkerParam->ctx->putHandler(siid, ciid, data, len, walkerParam->buf);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGW("put error, siid=%u ciid=%u len=%u", siid, ciid, len);
        return HIMCUL_ERR_PROFILE_PUT;
    }
    return HIMCUL_OK;
}

static int32_t ProfileServiceDispatch(uint8_t siid, const uint8_t *data, uint32_t len, void *param)
{
    if (data == NULL || len == 0) {
        return HIMCUL_OK;
    }
    TlvWalkerParam *walkerParam = (TlvWalkerParam *)param;
    walkerParam->siid = siid;
    if (walkerParam->frame->cmd == HIMCUL_CMD_BASE_SVC_PUT) {
        return TlvWalker(data, len, ProfileCharDispatch, param);
    }
    if (walkerParam->ctx->getHandler == NULL) {
        HIMCUL_LOGW("get handler null");
        return HIMCUL_ERR_PROFILE_DISPATCH;
    }

    for (uint32_t i = 0; i < len; ++i) {
        uint8_t ciid = data[i];
        int32_t ret = walkerParam->ctx->getHandler(siid, ciid, walkerParam->buf);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGW("get error, siid=%u ciid=%u", siid, ciid);
            return HIMCUL_ERR_PROFILE_GET;
        }
    }
    return HIMCUL_OK;
}

int32_t HIMCUL_ProfileDispatch(HIMCUL_Context *ctx, const HIMCUL_TransFrame *frame, HIMCUL_Buffer *buf)
{
    if (ctx == NULL || frame == NULL || buf == NULL || frame->data == NULL || frame->len == 0 ||
        (frame->cmd != HIMCUL_CMD_BASE_SVC_PUT && frame->cmd != HIMCUL_CMD_BASE_SVC_GET)) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    TlvWalkerParam param = {0, ctx, frame, buf};
    return TlvWalker(frame->data, frame->len, ProfileServiceDispatch, &param);
}

int32_t HIMCUL_RingBufferInit(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t size)
{
    /* tail指针位置不存数据，有1字节损耗 */
    if (rb == NULL || buf == NULL || size <= 1) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    rb->size = size;
    rb->buf = buf;
    rb->head = 0;
    rb->tail = 0;

    (void)memset_s(buf, size, 0, size);
    return HIMCUL_OK;
}

void HIMCUL_RingBufferClear(HIMCUL_RingBuffer *rb)
{
    if (rb == NULL) {
        return;
    }
    rb->head = rb->tail;
}

uint32_t HIMCUL_RingBufferGetBufLen(HIMCUL_RingBuffer *rb)
{
    if (rb == NULL || rb->size == 0 || rb->tail >= rb->size || rb->head >= rb->size) {
        return 0;
    }
    uint32_t head = rb->head;
    uint32_t tail = rb->tail;
    if (tail == head) {
        return rb->size - 1;
    }
    if (head > tail) {
        return head - tail - 1;
    }

    return rb->size - tail + head - 1;
}

uint32_t HIMCUL_RingBufferGetDataLen(HIMCUL_RingBuffer *rb)
{
    if (rb == NULL || rb->size == 0 || rb->tail >= rb->size || rb->head >= rb->size) {
        return 0;
    }
    uint32_t tail = rb->tail;
    uint32_t head = rb->head;
    if (tail == head) {
        return 0;
    }
    if (tail > head) {
        return tail - head;
    }
    return rb->size - head + tail;
}

static int32_t RingBufferWriteBufferRight(RingBufferWriteParam *param)
{
    HIMCUL_RingBuffer *rb = param->rb;
    const uint8_t *data = param->data;
    uint32_t len = param->len;
    uint32_t head = param->head;
    uint32_t tail = param->tail;
    uint32_t curBufLen = head - tail - 1;
    if (curBufLen == 0) {
        return 0;
    }
    uint32_t curCopyLen = HIMCUL_MIN(curBufLen, len);
    int32_t ret = memcpy_s(&rb->buf[tail], curBufLen, data, curCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&rb->buf[tail], curBufLen, data, curCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    rb->tail = tail + curCopyLen;
    return curCopyLen;
}

static int32_t RingBufferWriteBufferDiscontinuous(RingBufferWriteParam *param)
{
    HIMCUL_RingBuffer *rb = param->rb;
    const uint8_t *data = param->data;
    uint32_t len = param->len;
    uint32_t head = param->head;
    uint32_t tail = param->tail;
    uint32_t firstBufLen = rb->size - tail;
    uint32_t firstCopyLen = HIMCUL_MIN(firstBufLen, len);
    int32_t ret = memcpy_s(&rb->buf[tail], firstBufLen, data, firstCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&rb->buf[tail], firstBufLen, data, firstCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }

    tail = (tail + firstCopyLen) % rb->size;
    if (firstCopyLen == len) {
        rb->tail = tail;
        return firstCopyLen;
    }

    /* tail指针此时等于0 */
    uint32_t secondBufLen = head - tail - 1;
    if (secondBufLen == 0) {
        rb->tail = tail;
        return firstBufLen;
    }
    uint32_t secondCopyLen = HIMCUL_MIN(secondBufLen, len - firstCopyLen);
    ret = memcpy_s(&rb->buf[tail], secondBufLen, data + firstCopyLen, secondCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(&rb->buf[tail], secondBufLen, data + firstCopyLen, secondCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    rb->tail = tail + secondCopyLen;
    return firstBufLen + secondCopyLen;
}

int32_t HIMCUL_RingBufferWrite(HIMCUL_RingBuffer *rb, const uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0 || rb->size == 0 || rb->tail >= rb->size || rb->head >= rb->size) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint32_t head = rb->head;
    uint32_t tail = rb->tail;

    RingBufferWriteParam param = {
        .rb = rb,
        .data = data,
        .len = len,
        .head = head,
        .tail = tail,
    };

    /* 连续buffer居中 |...|tail|...buffer...|head|...| */
    if (param.head > param.tail) {
        return RingBufferWriteBufferRight(&param);
    }

    /* 连续buffer靠右 |head|...|tail|...buffer...| */
    if (param.head == 0) {
        param.head = rb->size;
        return RingBufferWriteBufferRight(&param);
    }

    /* 不连续buffer |...buffer...|head|...|tail|...buffer...| */
    return RingBufferWriteBufferDiscontinuous(&param);
}

static int32_t RingBufferReadDataCentered(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t len,
    uint32_t head, uint32_t tail)
{
    uint32_t curDataLen = tail - head;
    uint32_t curCopyLen = HIMCUL_MIN(curDataLen, len);
    int32_t ret = memcpy_s(buf, len, &rb->buf[head], curCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(buf, len, &rb->buf[head], curCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    rb->head = head + curCopyLen;
    return curCopyLen;
}

static int32_t RingBufferReadDataDiscontinuous(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t len,
    uint32_t head, uint32_t tail)
{
    uint32_t curDataLen = rb->size - head;
    uint32_t curCopyLen = HIMCUL_MIN(curDataLen, len);
    int32_t ret = memcpy_s(buf, len, &rb->buf[head], curCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(buf, len, &rb->buf[head], curCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    uint32_t allCopyLen = curCopyLen;
    head = (head + allCopyLen) % rb->size;
    if (allCopyLen == len) {
        rb->head = head;
        return allCopyLen;
    }

    curDataLen = tail - head;
    curCopyLen = HIMCUL_MIN(curDataLen, len - allCopyLen);
    if (curCopyLen == 0) {
        rb->head = head;
        return allCopyLen;
    }

    ret = memcpy_s(buf + allCopyLen, len - allCopyLen, &rb->buf[head], curCopyLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMCPY(buf + allCopyLen, len - allCopyLen, &rb->buf[head], curCopyLen);
        return HIMCUL_ERR_SECUREC_MEMCPY;
    }
    rb->head = head + curCopyLen;
    return allCopyLen + curCopyLen;
}

int32_t HIMCUL_RingBufferRead(HIMCUL_RingBuffer *rb, uint8_t *buf, uint32_t len)
{
    if (rb == NULL || buf == NULL || len == 0 || rb->size == 0 ||
        rb->tail >= rb->size || rb->head >= rb->size) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint32_t tail = rb->tail;
    uint32_t head = rb->head;

    /* 初始状态，无数据 |...|head=tail|...| */
    if (head == tail) {
        return 0;
    }

    /* 连续数据居中 |...|head|...data...|tail|...| */
    if (tail > head) {
        return RingBufferReadDataCentered(rb, buf, len, head, tail);
    }

    /* 不连续数据 |...data...|tail|...|head|...data...| */
    return RingBufferReadDataDiscontinuous(rb, buf, len, head, tail);
}

#if HIMCUL_CONF_FRAME_DEBUG_DUMP
static void TransFrameDataDump(const uint8_t *data, uint32_t len)
{
    HIMCUL_CHECK_V_RETURN(data != NULL && len != 0);
    uint32_t index = 0;
    char buffer[PDU_DUMP_BUFFER_SIZE] = {0};
    for (uint16_t i = 0; i < len; i++) {
        if (index >= PDU_DUMP_BUFFER_SIZE - PDU_DUMP_LEN_PER_BYTE) {
            HIMCUL_LOGD("          %s", buffer);
            index = 0;
        }
        int32_t ret = sprintf_s(buffer + index, PDU_DUMP_BUFFER_SIZE - index, "%02x ", data[i]);
        if (ret != PDU_DUMP_LEN_PER_BYTE) {
            HIMCUL_LOGE("sprintf failed, ret=%d", ret);
            return;
        }
        index += PDU_DUMP_LEN_PER_BYTE;
    }
    HIMCUL_LOGD("          %s", buffer);
}
#endif

static void TransMultiDataFrameDump(const HIMCUL_TransFrame *frame, const SendDataItem *items, uint32_t size)
{
#if HIMCUL_CONF_FRAME_DEBUG_DUMP
    if (frame == NULL) {
        return;
    }
    HIMCUL_LOGD("magic:    0x%04x", frame->magic);
    HIMCUL_LOGD("ver:      0x%02x", frame->ver);
    HIMCUL_LOGD("cmd:      0x%02x", frame->cmd);
    HIMCUL_LOGD("id:       0x%04x", frame->id);
    HIMCUL_LOGD("len:      0x%04x|%u", frame->len, frame->len);
    HIMCUL_LOGD("data:");

    if (frame->data != NULL) {
        TransFrameDataDump(frame->data, frame->len);
    } else if (items != NULL) {
        for (uint32_t i = 0; i < size; ++i) {
            TransFrameDataDump(items[i].data, items[i].len);
        }
    }

    HIMCUL_LOGD("checkSum: 0x%02x|%u", frame->checkSum, frame->checkSum);
#else
    return;
#endif
}

static void TransFrameDump(const HIMCUL_TransFrame *frame)
{
    return TransMultiDataFrameDump(frame, NULL, 0);
}

static int32_t TxSendAdapter(HIMCUL_Context *ctx, const uint8_t *data, uint32_t len)
{
    HIMCUL_CHECK_RETURN_LOGE(ctx != NULL && ctx->txHandler != NULL, HIMCUL_ERR_CTX_INVALID,"ctx invalid");
    if (data == NULL || len == 0) {
        return HIMCUL_OK;
    }

    uint32_t writeLen = 0;
    uint32_t remainLen = len;
    while (remainLen != 0) {
        int32_t ret = ctx->txHandler(data + writeLen, remainLen);
        if (ret < 0 || ret > remainLen) {
            HIMCUL_LOGW("[tx]send error, ret=%d", ret);
            return HIMCUL_ERR_TRANSPORT_SEND;
        } else if (ret != 0) {
            remainLen -= ret;
            writeLen += ret;
            HIMCUL_LOGD("[tx]send ok, len=%u/%u", writeLen, writeLen + remainLen);
        }
    }
    return HIMCUL_OK;
}

static int32_t RxRecvAdapter(HIMCUL_Context *ctx, uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{
    if (ctx == NULL || ctx->rxHandler == NULL || buf == NULL || len == 0) {
        HIMCUL_LOGF("recv ctx error");
        return HIMCUL_ERR_PARAM_INVALID;
    }

    return ctx->rxHandler(buf, len, timeoutMs);
}

static int32_t BuildFrameHeader(uint8_t buf[HIMCUL_FRAME_HEADER_LEN], uint8_t cmd, uint16_t id, uint32_t len)
{
    if (len > UINT16_MAX) {
        HIMCUL_LOGW("send too long, len=%u", len);
        return HIMCUL_ERR_TRANSPORT_TOO_LONG;
    }
    uint32_t index = 0;
    buf[index++] = HIMCU_TRANS_FRAME_MAGIC_FIRST;
    buf[index++] = HIMCU_TRANS_FRAME_MAGIC_SECOND;
    buf[index++] = HIMCU_TRANS_FRAME_VER;
    buf[index++] = cmd;
    buf[index++] = (id >> 8) & 0xFF;
    buf[index++] = id & 0xFF;
    buf[index++] = (len >> 8) & 0xFF;
    buf[index++] = len & 0xFF;
    return HIMCUL_OK;
}

static int32_t TxSendCalcSumWrapper(HIMCUL_Context *ctx, const uint8_t *data, uint32_t len, uint32_t *checksum)
{
    for (uint32_t i = 0; i < len; i++) {
        *checksum += data[i];
    }

    int32_t ret = TxSendAdapter(ctx, data, len);
    if (ret != HIMCUL_OK) {
        return ret;
    }
    return HIMCUL_OK;
}

static int32_t TransportSendFrame(HIMCUL_Context *ctx, uint8_t cmd, uint16_t id,
    const SendDataItem *items, uint32_t size)
{
    uint32_t sumValue = 0;
    uint32_t sendLen = 0;
    for (uint32_t i = 0; i < size; ++i) {
        sendLen += items[i].len;
    }

    uint8_t header[HIMCUL_FRAME_HEADER_LEN] = {0};
    int32_t ret = BuildFrameHeader(header, cmd, id, sendLen);
    if (ret != HIMCUL_OK) {
        return ret;
    }

    ret = TxSendCalcSumWrapper(ctx, header, sizeof(header), &sumValue);
    if (ret != HIMCUL_OK) {
        return ret;
    }

    for (uint32_t i = 0; i < size; ++i) {
        ret = TxSendCalcSumWrapper(ctx, items[i].data, items[i].len, &sumValue);
        if (ret != HIMCUL_OK) {
            return ret;
        }
    }

    uint8_t checksum = (uint8_t)(sumValue % HIMCU_TRANS_FRAME_CHECKSUM_MOD);
    ret = TxSendAdapter(ctx, &checksum, sizeof(checksum));
    if (ret != HIMCUL_OK) {
        return ret;
    }

    HIMCUL_TransFrame frame = {HIMCU_TRANS_FRAME_MAGIC_BIG_END, HIMCU_TRANS_FRAME_VER, cmd, id,
        sendLen, NULL, checksum, 0};
	HIMCUL_LOGN_FRAME(&frame, "send frame ok");
    TransMultiDataFrameDump(&frame, items, size);

    return HIMCUL_OK;
}

static uint16_t GetRequestId(HIMCUL_Context *ctx, uint16_t *id)
{
    uint16_t requestId = 0;
    if (id != NULL) {
        if (*id == 0) {
            requestId = ctx->private.sendSeq++;
            *id = requestId;
        } else {
            requestId = *id;
        }
    } else {
        requestId = ctx->private.sendSeq++;
    }
    if (ctx->private.sendSeq == 0) {
        ctx->private.sendSeq++;
    }
    return requestId;
}

int32_t HIMCUL_TransSendBaseConfig(HIMCUL_Context *ctx, uint16_t *id, uint8_t tag, const uint8_t *data, uint32_t len)
{
    if (ctx == NULL || data == NULL || len == 0) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint16_t requestId = GetRequestId(ctx, id);
    uint8_t tlvHeader[] = {tag, ((len >> 8) & 0xFF), (len & 0xFF)};
    SendDataItem items[] = {
        {tlvHeader, sizeof(tlvHeader)},
        {data, len}
    };
    int32_t ret = TransportSendFrame(ctx, HIMCUL_CMD_BASE_INFO_CONFIG, requestId, items, HIMCUL_ARRAY_SIZE(items));
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("send base config error, ret=%d", ret);
        return ret;
    }

    return HIMCUL_OK;
}

int32_t HIMCUL_TransSendRequest(HIMCUL_Context *ctx, uint8_t cmd, uint16_t *id, const uint8_t *data, uint32_t len)
{
    if (ctx == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    uint16_t requestId = GetRequestId(ctx, id);
    SendDataItem item = {data, len};
    int32_t ret = TransportSendFrame(ctx, cmd, requestId, &item, HIMCUL_ARRAY_SINGLE_NUM);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("send req error, ret=%d cmd=%u requestId=%u len=%u", ret, cmd, requestId, len);
        return ret;
    }

    return HIMCUL_OK;
}

int32_t HIMCUL_TransSendResponse(HIMCUL_Context *ctx, uint8_t cmd, uint16_t id, const uint8_t *data, uint32_t len)
{
    if (ctx == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    SendDataItem item = {data, len};
    int32_t ret = TransportSendFrame(ctx, cmd, id, &item, HIMCUL_ARRAY_SINGLE_NUM);
    if (ret != HIMCUL_OK) {
        HIMCUL_LOGE("send resp error, ret=%d cmd=%u id=%u len=%u", ret, cmd, id, len);
        return ret;
    }

    return HIMCUL_OK;
}

int32_t HIMCUL_TransSendResponseCode(HIMCUL_Context *ctx, uint8_t cmd, uint16_t id, uint32_t errcode)
{
    uint32_t errBigEnd = HIMCUL_UtilsHtonl(errcode);
    return HIMCUL_TransSendResponse(ctx, cmd, id, (const uint8_t *)&errBigEnd, sizeof(errBigEnd));
}

static bool ParseFrameHeader(HIMCUL_Context *ctx, const uint8_t *data, uint32_t len)
{
    if (len < HIMCUL_FRAME_HEADER_LEN) {
        return false;
    }
    (void)memset_s(&ctx->private.frame, sizeof(ctx->private.frame), 0, sizeof(ctx->private.frame));

    /* 0/1字节为魔术字 */
    if (data[0] != HIMCU_TRANS_FRAME_MAGIC_FIRST || data[1] != HIMCU_TRANS_FRAME_MAGIC_SECOND) {
        return false;
    }
    if (data[2] != HIMCU_TRANS_FRAME_VER) {
        /* 2字节为版本 */
        HIMCUL_LOGE("unsupport version, byte2=%u", data[2]);
        return false;
    }

    /* 6/7字节为长度 */
    uint32_t dataLen = (data[6] << HIMCU_BIT_PER_BYTE) | data[7];
    uint32_t frameSize = dataLen + HIMCUL_FRAME_HEADER_LEN + 1;
    if (frameSize > ctx->recvBufSize) {
        HIMCUL_LOGE("frame size too long, size=%u buf=%u", frameSize, ctx->recvBufSize);
        return false;
    }
    ctx->private.frame.magic = HIMCU_TRANS_FRAME_MAGIC_FIRST << HIMCU_BIT_PER_BYTE | HIMCU_TRANS_FRAME_MAGIC_SECOND;
    ctx->private.frame.ver = HIMCU_TRANS_FRAME_VER;
    /* 3字节为命令 */
    ctx->private.frame.cmd = data[3];
    /* 4/5字节为id */
    ctx->private.frame.id = (data[4] << HIMCU_BIT_PER_BYTE) | data[5];
    ctx->private.frame.len = dataLen;
    return true;
}

/* 收满frame头部 */
static int32_t RecvFsmRecvHeaderHandler(void *param, int32_t cur)
{
    HIMCUL_NOT_USED(cur);
    HIMCUL_Context *ctx = (HIMCUL_Context *)param;
    if (ctx == NULL || ctx->recvBuf == NULL || ctx->recvBufSize < HIMCUL_FRAME_HEADER_LEN) {
        return RECV_STATE_RECV_HEADER;
    }

    if (ctx->private.recvLen >= HIMCUL_FRAME_HEADER_LEN) {
        return RECV_STATE_CHECK_HEADER;
    }
    uint32_t bufferLen = ctx->recvBufSize - ctx->private.recvLen;
    int32_t ret = RxRecvAdapter(ctx, ctx->recvBuf + ctx->private.recvLen, bufferLen, ctx->private.timeout);
    if (ret < 0 || ret > bufferLen) {
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }

    ctx->private.recvLen += ret;
    if (ctx->private.recvLen >= HIMCUL_FRAME_HEADER_LEN) {
        /* 收齐头部，进入下一个状态检查头部是否合法 */
        return RECV_STATE_CHECK_HEADER;
    }
    return RECV_STATE_RECV_HEADER;
}

static inline uint32_t FindMagicFirst(const uint8_t *recvBuffer, uint32_t recvLen)
{
    uint32_t index = 0;
    for (; index < recvLen; ++index) {
        if (recvBuffer[index] != HIMCU_TRANS_FRAME_MAGIC_FIRST) {
            continue;
        }
        break;
    }
    return index;
}

static inline bool MoveDataToStart(uint8_t *recvBuffer, uint32_t size, uint32_t index, uint32_t validLen)
{
    /* 不足header长度，则把数据搬到起始位置并继续返回接收header */
    int ret = memmove_s(recvBuffer, size, &recvBuffer[index], validLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMMOVE(recvBuffer, size, &recvBuffer[index], validLen);
        return false;
    }
    return true;
}

/* 检查frame头部是否合法 */
static int32_t RecvFsmCheckHeaderHandler(void *param, int32_t cur)
{
    HIMCUL_NOT_USED(cur);
    HIMCUL_Context *ctx = (HIMCUL_Context *)param;
    if (ctx == NULL || ctx->recvBuf == NULL || ctx->private.recvLen < HIMCUL_FRAME_HEADER_LEN) {
        return RECV_STATE_RECV_HEADER;
    }

    uint32_t index = FindMagicFirst(ctx->recvBuf, ctx->private.recvLen);
    if (index == ctx->private.recvLen) {
        /* 已接收数据不含魔术字则清空数据重新收 */
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }

    if (index == ctx->private.recvLen - 1) {
        /* 第一个魔术字是最后一位数据则仅保留第一位魔术字接收后续数据 */
        ctx->private.recvLen = 1;
        ctx->recvBuf[0] = HIMCU_TRANS_FRAME_MAGIC_FIRST;
        return RECV_STATE_RECV_HEADER;
    }

    uint32_t validDataLen = ctx->private.recvLen - index;
    if (validDataLen < HIMCUL_FRAME_HEADER_LEN) {
        /* 不足header长度，则把数据搬到起始位置并继续返回接收header */
        ctx->private.recvLen = MoveDataToStart(ctx->recvBuf, ctx->recvBufSize, index, validDataLen) ?
            validDataLen : 0;
        return RECV_STATE_RECV_HEADER;
    }

    /* 收够header长度，进行header解析 */
    if (!ParseFrameHeader(ctx, &ctx->recvBuf[index], validDataLen)) {
        /* 已有数据frame解析失败，则抹去第一个magic，重新进行检查 */
        ctx->recvBuf[index] = HIMCU_TRANS_FRAME_MAGIC_INVALID;
        return RECV_STATE_CHECK_HEADER;
    }

    /* frame header解析成功，则把数据搬到起始位置并接收剩余payload数据 */
    if (!MoveDataToStart(ctx->recvBuf, ctx->recvBufSize, index, validDataLen)) {
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }
    ctx->private.recvLen = validDataLen;
    ctx->private.frame.data = &ctx->recvBuf[HIMCUL_FRAME_HEADER_LEN];
    return RECV_STATE_RECV_BODY;
}

/* 接收payload+checksum */
static int32_t RecvFsmRecvBodyHandler(void *param, int32_t cur)
{
    HIMCUL_NOT_USED(cur);
    HIMCUL_Context *ctx = (HIMCUL_Context *)param;
    if (ctx == NULL || ctx->recvBuf == NULL) {
        return RECV_STATE_RECV_HEADER;
    }

    int32_t ret;

    if (ctx->private.recvLen < HIMCUL_FRAME_HEADER_LEN) {
        return RECV_STATE_RECV_HEADER;
    }

    uint32_t frameSize = HIMCUL_FRAME_HEADER_LEN + ctx->private.frame.len + HIMCU_TRANS_FRAME_CHECKSUM_LEN;
    if (frameSize > ctx->recvBufSize) {
        HIMCUL_LOGE("packet over max size, %u/%u", frameSize, ctx->recvBufSize);
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }

    if (ctx->private.recvLen >= frameSize) {
        /* 数据已收齐，校验报文 */
        ctx->private.frame.checkSum = ctx->recvBuf[frameSize - 1];
        return RECV_STATE_CHECK_SUM;
    }

    uint32_t bufferLen = ctx->recvBufSize - ctx->private.recvLen;
    ret = RxRecvAdapter(ctx, ctx->recvBuf + ctx->private.recvLen, bufferLen, 0);
    if (ret < 0 || ret > bufferLen) {
        HIMCUL_LOGE("recv error, ret=%d remain=%u", ret, bufferLen);
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }

    ctx->private.recvLen += ret;
    if (ctx->private.recvLen >= frameSize) {
        /* 数据已收齐，校验报文 */
        ctx->private.frame.checkSum = ctx->recvBuf[frameSize - 1];
        return RECV_STATE_CHECK_SUM;
    }

    return RECV_STATE_RECV_BODY;
}

static int32_t RecvFsmChecksumHandler(void *param, int32_t cur)
{
    HIMCUL_NOT_USED(cur);
    HIMCUL_Context *ctx = (HIMCUL_Context *)param;
    if (ctx == NULL || ctx->recvBuf == NULL) {
        return RECV_STATE_RECV_HEADER;
    }

    uint32_t checksumSize = HIMCUL_FRAME_HEADER_LEN + ctx->private.frame.len;
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < checksumSize; ++i) {
        checksum += ctx->recvBuf[i];
    }
    checksum %= HIMCU_TRANS_FRAME_CHECKSUM_MOD;
    if ((uint8_t)checksum != ctx->private.frame.checkSum) {
        HIMCUL_LOGW_FRAME(&ctx->private.frame, "check sum error, calc=%u expect=%u",
            checksum, ctx->private.frame.checkSum);
        ctx->private.recvLen = 0;
        TransFrameDump(&ctx->private.frame);
        return RECV_STATE_RECV_HEADER;
    }
    /* checksum校验成功，进入分发状态 */
    return RECV_STATE_DISPATCH;
}

static int32_t RecvFsmDispatchHandler(void *param, int32_t cur)
{
    HIMCUL_NOT_USED(cur);
    HIMCUL_Context *ctx = (HIMCUL_Context *)param;
    if (ctx == NULL || ctx->recvBuf == NULL || ctx->endpoints == NULL || ctx->endpointNum == 0) {
        return RECV_STATE_RECV_HEADER;
    }

	HIMCUL_LOGN_FRAME(&ctx->private.frame, "recv frame");
    TransFrameDump(&ctx->private.frame);
    if (HIMCUL_IS_MCU_TO_MODULE_REQUEST(ctx->private.frame.cmd) && ctx->private.recvSeq == ctx->private.frame.id) {
        HIMCUL_LOGN_FRAME(&ctx->private.frame, "retrans request");
        HIMCUL_BIT_SET(ctx->private.frame.flag, HIMCUL_FRAME_FLAG_RETRANS);
    }
    ctx->private.recvSeq = ctx->private.frame.id;
    int32_t ret;
    bool find = false;
    for (uint32_t i = 0; i < ctx->endpointNum; ++i) {
        if (ctx->endpoints[i].cmd != ctx->private.frame.cmd || ctx->endpoints[i].handler == NULL) {
            continue;
        }
        ret = ctx->endpoints[i].handler(ctx, &ctx->private.frame);
        if (ret != HIMCUL_OK) {
            HIMCUL_LOGE_FRAME(&ctx->private.frame, "dispatch failed, ret=%d", ret);
        } else {
            HIMCUL_LOGD_FRAME(&ctx->private.frame, "dispatch success");
        }
        find = true;
        break;
    }

    if (!find) {
        HIMCUL_LOGW_FRAME(&ctx->private.frame, "no endpoint find");
    }

    uint32_t frameSize = HIMCUL_FRAME_HEADER_LEN + ctx->private.frame.len + 1;
    if (frameSize >= ctx->private.recvLen) {
        /* 已接收数据已全部处理 */
        ctx->private.recvLen = 0;
        return RECV_STATE_RECV_HEADER;
    }

    /* 已接收但未处理的数据搬到头部 */
    uint32_t remainLen = ctx->private.recvLen - frameSize;
    ret = memmove_s(ctx->recvBuf, ctx->recvBufSize, ctx->recvBuf + frameSize, remainLen);
    if (ret != EOK) {
        HIMCUL_LOGE_MEMMOVE(ctx->recvBuf, ctx->recvBufSize, ctx->recvBuf + frameSize, remainLen);
        ctx->private.recvLen = 0;
    } else {
        ctx->private.recvLen = remainLen;
    }

    return RECV_STATE_RECV_HEADER;
}

/* 收包状态机 */
static HIMCUL_FsmStateNode g_recvFsm[] = {
    /*
     * ===> RECV_STATE_RECV_HEADER 自循环收齐头部数据或者异常
     * ===> RECV_STATE_CHECK_HEADER 头部数据已收齐
     */
    { RECV_STATE_RECV_HEADER, RecvFsmRecvHeaderHandler },
    /*
     * ===> RECV_STATE_RECV_HEADER 异常或者有效数据不足头部长度
     * ===> RECV_STATE_CHECK_HEADER 自循环检查头部数据
     * ===> RECV_STATE_RECV_BODY 头部数据已收齐并有效
     */
    { RECV_STATE_CHECK_HEADER, RecvFsmCheckHeaderHandler },
    /*
     * ===> RECV_STATE_RECV_HEADER 异常
     * ===> RECV_STATE_RECV_BODY 自循环收齐数据
     * ===> RECV_STATE_CHECK_SUM 数据已收齐进行checksum校验
     */
    { RECV_STATE_RECV_BODY, RecvFsmRecvBodyHandler },
    /*
     * ===> RECV_STATE_RECV_HEADER checksum校验不通过
     * ===> RECV_STATE_DISPATCH 业务分发
     */
    { RECV_STATE_CHECK_SUM, RecvFsmChecksumHandler },
    /*
     * ===> RECV_STATE_RECV_HEADER 回退到收下一个包
     */
    { RECV_STATE_DISPATCH, RecvFsmDispatchHandler },
};

int32_t HIMCUL_TransIoLoop(HIMCUL_Context *ctx, uint32_t curMs, uint32_t timeoutMs)
{
    if (ctx == NULL) {
        return HIMCUL_ERR_PARAM_INVALID;
    }

    if (!ctx->private.init) {
        UTILS_FSM_INIT(&ctx->private.fsm, "recv_fsm", RECV_STATE_RECV_HEADER, NULL, g_recvFsm);
        ctx->private.init = true;
        ctx->private.sendSeq = 1; /* 0 id作为缺省值，发送id从1开始 */
    }

    ctx->curMs = curMs;
    ctx->private.timeout = timeoutMs;
    int32_t ret = HIMCUL_FsmRunning(&ctx->private.fsm, ctx);
    if (ret < 0) {
        HIMCUL_LOGF("fsm error, ret=%d", ret);
        HIMCUL_FsmSwitch(&ctx->private.fsm, RECV_STATE_RECV_HEADER);
        return ret;
    }
    return HIMCUL_OK;
}

int32_t HIMCUL_UtilsAddFormatData(HIMCUL_Buffer *buffer, const char *format, ...)
{
    HIMCUL_CHECK_RETURN_LOGW(buffer != NULL && format != NULL, HIMCUL_ERR_PARAM_INVALID, "param invalid");
    HIMCUL_CHECK_RETURN_LOGW(buffer->size > buffer->len, HIMCUL_ERR_BUFFER_NOT_ENOUGH,
        "buffer not enough, size=%u len=%u", buffer->size, buffer->len);

    va_list args;
    va_start(args, format);
    int32_t ret = vsprintf_s((char *)&buffer->buffer[buffer->len], buffer->size - buffer->len, format, args);
    va_end(args);
    if (ret <= 0) {
        HIMCUL_LOGE("vsprintf_s failed");
        return HIMCUL_ERR_SECUREC_VSPRINTF;
    }
    buffer->len += ret;
    return HIMCUL_OK;
}

uint32_t HIMCUL_DeltaTime(uint32_t timeNew, uint32_t timeOld)
{
    uint32_t deltaTime;

    if (timeNew >= timeOld) {
        deltaTime = timeNew - timeOld;
    } else {
        deltaTime = (UINT32_MAX - timeOld) + timeNew + 1; /* 处理时间翻转 */
    }

    return deltaTime;
}
