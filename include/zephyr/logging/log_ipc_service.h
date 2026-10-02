/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for IPC service logging protocol.
 * @ingroup log_ipc_service
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_IPC_SERVICE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_IPC_SERVICE_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

/**
 * @brief IPC service logging protocol
 *
 * Messages exchanged between the IPC service log link and the IPC service log
 * backend.
 *
 * @defgroup log_ipc_service IPC service logging protocol
 * @ingroup log_backend
 * @{
 */

/**
 * @name IPC service message IDs
 * @anchor LOG_IPC_SERVICE_MESSAGE_IDS
 * @{
 */

/** @brief Logging message ID. */
#define Z_LOG_IPC_SERVICE_ID_MSG 0

/** @brief Source count request ID. */
#define Z_LOG_IPC_SERVICE_ID_GET_SOURCE_CNT 1

/** @brief Source name request ID. */
#define Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME 2

/** @brief Compile time and run-time levels request ID. */
#define Z_LOG_IPC_SERVICE_ID_GET_LEVELS 3

/** @brief Setting run-time level ID. */
#define Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL 4

/** @brief Get number of dropped message ID. */
#define Z_LOG_IPC_SERVICE_ID_DROPPED 5

/** @brief Link-backend readiness indication ID. */
#define Z_LOG_IPC_SERVICE_ID_READY 6

/**@} */

/**
 * @name IPC service status flags
 * @anchor LOG_IPC_SERVICE_STATUS
 * @{
 */

/** @brief OK. */
#define Z_LOG_IPC_SERVICE_STATUS_OK 0
/** @brief Error. */
#define Z_LOG_IPC_SERVICE_STATUS_ERR 1

/**@} */

/** @brief Content of the logging message. */
struct log_ipc_service_log_msg {
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data); /**< Serialized log message bytes. */
} __packed;

/** @brief Content of the source count message. */
struct log_ipc_service_source_cnt {
	uint16_t count; /**< Number of sources in the domain. */
} __packed;

/** @brief Content of the source name message. */
struct log_ipc_service_source_name {
	uint16_t source_id; /**< Source ID within the domain. */
	char name[];        /**< Null-terminated source name. */
} __packed;

/** @brief Content of the message for getting logging levels. */
struct log_ipc_service_levels {
	uint16_t source_id;    /**< Source ID within the domain. */
	uint8_t level;         /**< Compile-time level. */
	uint8_t runtime_level; /**< Run-time level. */
} __packed;

/** @brief Content of the message for setting logging level. */
struct log_ipc_service_set_runtime_level {
	uint16_t source_id;    /**< Source ID within the domain. */
	uint8_t runtime_level; /**< Run-time level to set. */
} __packed;

/** @brief Content of the message for getting amount of dropped messages. */
struct log_ipc_service_dropped {
	uint32_t dropped; /**< Number of dropped messages. */
} __packed;

/** @brief Union with all message types. */
union log_ipc_service_msg_data {
	struct log_ipc_service_log_msg log_msg;                /**< Log message payload. */
	struct log_ipc_service_source_cnt source_cnt;          /**< Source count payload. */
	struct log_ipc_service_source_name source_name;        /**< Source name payload. */
	struct log_ipc_service_levels levels;                  /**< Levels payload. */
	struct log_ipc_service_set_runtime_level set_rt_level; /**< Set-level payload. */
	struct log_ipc_service_dropped dropped;                /**< Dropped count payload. */
};

/** @brief Message. */
struct log_ipc_service_msg {
	uint8_t id;     /**< Message ID, see @ref LOG_IPC_SERVICE_MESSAGE_IDS. */
	uint8_t status; /**< Status code, see @ref LOG_IPC_SERVICE_STATUS. */
	union log_ipc_service_msg_data data; /**< Message payload. */
} __packed;

/** @} */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_IPC_SERVICE_H_ */
