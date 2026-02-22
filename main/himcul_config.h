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
#ifndef HIMCU_LITE_CONFIG_H
#define HIMCU_LITE_CONFIG_H

/* 是否打开日志，关闭可以减少ROM */
#ifndef HIMCUL_CONF_LOG_SUPPORT
#define HIMCUL_CONF_LOG_SUPPORT                     1
#endif

/* 日志编译等级，参考HIMCUL_LOG_LEVEL_XXX宏，降低等级可以减少ROM */
#ifndef HIMCUL_CONF_LOG_BUILD_LEVEL
#define HIMCUL_CONF_LOG_BUILD_LEVEL                 7
#endif

/* 默认日志等级 */
#ifndef HIMCUL_CONF_LOG_DEFAULT_LEVEL
#define HIMCUL_CONF_LOG_DEFAULT_LEVEL               4
#endif

/* 是否打开帧调测信息，关闭可以减少ROM */
#ifndef HIMCUL_CONF_FRAME_DEBUG_DUMP
#define HIMCUL_CONF_FRAME_DEBUG_DUMP                0
#endif

/* 是否支持MCU的OTA升级 */
#ifndef HIMCUL_CONF_OTA_SUPPORT
#define HIMCUL_CONF_OTA_SUPPORT                     1
#endif

/* 是否支持OTA升级后从FLASH读取固件并进行CRC校验，如果MCU自身固件支持完整性校验可以不打开 */
#ifndef HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT
#define HIMCUL_CONF_OTA_READ_CRC_CHECK_SUPPORT      1
#endif

/* ota升级包最大大小 */
#ifndef HIMCUL_CONF_OTA_MAX_SIZE
#define HIMCUL_CONF_OTA_MAX_SIZE                    0xFFFFFFFF
#endif

/* 是否打开串口环形缓冲区，打开后会申请 HIMCUL_CONF_TRANS_FRAME_MAX_SIZE 大小RAM用于缓存数据 */
#ifndef HIMCUL_CONF_UART_RING_BUFFER_SUPPORT
#define HIMCUL_CONF_UART_RING_BUFFER_SUPPORT        0
#endif

/* 是否打开MCU启动后首次连接上模组后清除模组的配置，仅用于调测 */
#ifndef HIMCUL_CONF_REBOOT_CLEAN_CONFIG
#define HIMCUL_CONF_REBOOT_CLEAN_CONFIG             1
#endif

/* 是否由MCU配置SN */
#ifndef HIMCUL_CONF_MCU_SN_SUPPORT
#define HIMCUL_CONF_MCU_SN_SUPPORT                  0
#endif

/* 是否打开重传，建议打开 */
#ifndef HIMCUL_CONF_RETRANS_SUPPORT
#define HIMCUL_CONF_RETRANS_SUPPORT                 1
#endif

/* 是否打开心跳，建议打开 */
#ifndef HIMCUL_CONF_HEARTBEAT_SUPPORT
#define HIMCUL_CONF_HEARTBEAT_SUPPORT               1
#endif

/* 是否使用自定义协议，不指定则模组默认会根据配网类型填充 */
#ifndef HIMCUL_CONF_CUSTOM_PROTOCOL_ENABLE
#define HIMCUL_CONF_CUSTOM_PROTOCOL_ENABLE           0
#endif

/* 自定义协议类型，不指定则模组默认会根据配网类型填充 */
#ifndef HIMCUL_CONF_CUSTOM_PROTOCOL_TYPE
#define HIMCUL_CONF_CUSTOM_PROTOCOL_TYPE             17
#endif

/* 是否使能靠近发现 */
#ifndef HIMCU_CONF_NEAR_DISCOVERY_ENABLE
#define HIMCU_CONF_NEAR_DISCOVERY_ENABLE            1
#endif

/* 靠近发现功率 */
#ifndef HIMCU_CONF_NEAR_DISCOVERY_POWER
#define HIMCU_CONF_NEAR_DISCOVERY_POWER             0xF8
#endif

/* 最大同时上报服务数量 */
#ifndef HIMCUL_CONF_REPORT_SVC_MAX_NUM
#define HIMCUL_CONF_REPORT_SVC_MAX_NUM              5
#endif

/* 调用HIMCUL_TransIoLoop时等待的时间，传递给串口接收函数中的timeout */
#ifndef HIMCUL_CONF_IO_LOOP_RECV_TIMEOUT_MS
#define HIMCUL_CONF_IO_LOOP_RECV_TIMEOUT_MS         50
#endif

/* 数据上报轮询间隔 */
#ifndef HIMCUL_CONF_REPORT_LOOP_MS
#define HIMCUL_CONF_REPORT_LOOP_MS                  100
#endif

/* 上报消息队列最大数量 */
#ifndef HIMCUL_CONF_REPORT_QUEUE_MAX_NUM
#define HIMCUL_CONF_REPORT_QUEUE_MAX_NUM            5
#endif

/* 诊断信息最大长度 */
#ifndef HIMCUL_CONF_DIAG_INFO_MAX_LEN
#define HIMCUL_CONF_DIAG_INFO_MAX_LEN               8
#endif

/* 传输数据帧最大大小，会申请2倍的RAM空间用来做收发缓冲区 */
#ifndef HIMCUL_CONF_TRANS_FRAME_MAX_SIZE
#define HIMCUL_CONF_TRANS_FRAME_MAX_SIZE            512
#endif

#endif /* HIMCU_LITE_CONFIG_H */