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
#ifndef HIMCU_LITE_PRODUCT_H
#define HIMCU_LITE_PRODUCT_H

#include "himcul_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       TODO: 日志输出，按平台实际能力修改，当前输出到printf与vprinf中， \n
 *                    无日志能力则修改HIMCUL_CONF_LOG_SUPPORT，关闭所有日志输出
 *
 * @param[in]   level: 日志等级
 * @param[in]   tag: 日志tag
 * @param[in]   fmt: 格式化日志
 * @param[in]   arg: 格式化日志参数
 */
void HIMCUL_PROD_LogOutput(uint8_t level, const char *tag, const char *fmt, va_list arg);

/**
 * @brief       TODO: 模组事件处理
 *
 * @param[in]   event: 模组事件，参考 HIMCUL_Event
 */
void HIMCUL_PROD_EventProcess(uint32_t event);

/**
 * @brief       TODO: 字符转数字
 *
 * @param[in]   nptr: 输入字符串
 * @param[in]   endptr: 输出指针，指向“未被解析”的剩余字符串；可为 NULL
 * @param[in]   base: 进制，为0自动识别
 * @return
 *              - 解析出的数字
 */
int32_t HIMCUL_PROD_Strtol(const char *nptr, char **endptr, int base);

/**
 * @brief       TODO: 计算字符串长度
 *
 * @param[in]   str: 字符指针，需携带结束符
 * @return
 *              - 长度
 */
uint32_t HIMCUL_PROD_Strlen(const char *str);

/**
 * @brief       TODO: 计算字符串长度，并制定最大值
 *
 * @param[in]   str: 字符指针，需携带结束符
 * @param[in]   size: 最大长度
 * @return
 *              - 长度
 */
uint32_t HIMCUL_PROD_Strnlen(const char *str, uint32_t size);

/**
 * @brief       TODO: 传输接口初始化
 *
 * @param[in]   event: 模组事件，参考 HIMCUL_Event
 * @param[in]   tag: 日志tag
 * @param[in]   fmt: 格式化日志
 * @param[in]   arg: 格式化日志参数
 * @return
 *              - HIMCUL_OK: 成功
 *              - 其他: 失败
 */
int32_t HIMCUL_PROD_TransInit(void);

/**
 * @brief       TODO: 串口发
 *
 * @param[in]   data: 发送数据
 * @param[in]   len: 发送数据长度
 * @return
 *              - >= 0: 已发送数据的长度
 *              - < 0: 失败
 */
int32_t HIMCUL_PROD_TransSendHandler(const uint8_t *data, uint32_t len);

/**
 * @brief           TODO: 串口收，如果未打开环形缓冲区，需实现串口数据接收逻辑, 如果打开则串口数据中断中调用 \n
 *                        HIMCUL_FWK_WriteDataRingBuffer
 * @param[in,out]   buf: 接收数据缓冲区
 * @param[in]       len: 缓冲区长度
 * @param[in]       timeoutMs: 最大超时时间，单位ms
 * @return
 *              - >= 0: 已接收数据长度
 *              - < 0: 失败
 */
int32_t HIMCUL_PROD_TransRecvHandler(uint8_t *buf, uint32_t len, uint32_t timeoutMs);


int32_t HIMCUL_PROD_BuildBaseConfig(HIMCUL_Buffer *buffer);
int32_t HIMCUL_PROD_BuildBaseConfigProfile(HIMCUL_Buffer *buffer);
int32_t HIMCUL_PROD_BaseProfilePutHandler(uint8_t siid, uint8_t ciid,
    const uint8_t *data, uint32_t len, HIMCUL_Buffer *buffer);
int32_t HIMCUL_PROD_BaseProfileGetHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buffer);
int32_t HIMCUL_PROD_BaseProfileReportHandler(uint8_t siid, uint8_t ciid, HIMCUL_Buffer *buffer);
int32_t HIMCUL_PROD_BaseProfileReportAllHandler(HIMCUL_Buffer *buffer);

int32_t HIMCUL_PROD_TransInit(void);
int32_t HIMCUL_PROD_TransSendHandler(const uint8_t *data, uint32_t len);
int32_t HIMCUL_PROD_TransRecvHandler(uint8_t *buf, uint32_t len, uint32_t timeoutMs);

void HIMCUL_PROD_EventProcess(uint32_t event);

#ifdef __cplusplus
}
#endif

#endif /* HIMCU_LITE_PRODUCT_H */