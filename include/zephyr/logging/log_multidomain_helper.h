/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_MULTIDOMAIN_HELPER_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_MULTIDOMAIN_HELPER_H_

/**
 * @brief Logger multidomain backend helpers
 *
 * This module aims to provide baseline for links and backends and simplify
 * the implementation. It is not core part of logging in similar way as
 * log_output module is just a helper for log message formatting. Links and
 * backends can be implemented without this helper.
 *
 * @defgroup log_backend_multidomain Logger multidomain backend helpers
 * @ingroup log_backend
 * @{
 */

/**
 * @name Multidomain message IDs
 * @anchor LOG_MULTIDOMAIN_HELPER_MESSAGE_IDS
 * @{
 */

/** @brief Logging message ID. */
#define Z_LOG_MULTIDOMAIN_ID_MSG 0

/** @brief Domain count request ID. */
#define Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_CNT 1

/** @brief Source count request ID. */
#define Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_CNT 2

/** @brief Domain name request ID. */
#define Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_NAME 3

/** @brief Source name request ID. */
#define Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_NAME 4

/** @brief Compile time and run-time levels request ID. */
#define Z_LOG_MULTIDOMAIN_ID_GET_LEVELS 5

/** @brief Setting run-time level ID. */
#define Z_LOG_MULTIDOMAIN_ID_SET_RUNTIME_LEVEL 6

/** @brief Get number of dropped message ID. */
#define Z_LOG_MULTIDOMAIN_ID_DROPPED 7

/** @brief Link-backend readiness indication ID/ */
#define Z_LOG_MULTIDOMAIN_ID_READY 8

/**@} */

/**
 * @name Multidomain status flags
 * @anchor LOG_MULTIDOMAIN_STATUS
 * @{
 */

/** @brief OK. */
#define Z_LOG_MULTIDOMAIN_STATUS_OK 0
/** @brief Error. */
#define Z_LOG_MULTIDOMAIN_STATUS_ERR 1

/**@} */

/** @brief Content of the logging message. */
struct log_multidomain_log_msg {
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data);
} __packed;

/** @brief Content of the domain count message. */
struct log_multidomain_domain_cnt {
	uint16_t count;
} __packed;

/** @brief Content of the source count message. */
struct log_multidomain_source_cnt {
	uint8_t domain_id;
	uint16_t count;
} __packed;

/** @brief Content of the domain name message. */
struct log_multidomain_domain_name {
	uint8_t domain_id;
	char name[];
} __packed;

/** @brief Content of the source name message. */
struct log_multidomain_source_name {
	uint8_t domain_id;
	uint16_t source_id;
	char name[];
} __packed;

/** @brief Content of the message for getting logging levels. */
struct log_multidomain_levels {
	uint8_t domain_id;
	uint16_t source_id;
	uint8_t level;
	uint8_t runtime_level;
} __packed;

/** @brief Content of the message for setting logging level. */
struct log_multidomain_set_runtime_level {
	uint8_t domain_id;
	uint16_t source_id;
	uint8_t runtime_level;
} __packed;

/** @brief Content of the message for getting amount of dropped messages. */
struct log_multidomain_dropped {
	uint32_t dropped;
} __packed;

/** @brief Union with all message types. */
union log_multidomain_msg_data {
	struct log_multidomain_log_msg log_msg;
	struct log_multidomain_domain_cnt domain_cnt;
	struct log_multidomain_source_cnt source_cnt;
	struct log_multidomain_domain_name domain_name;
	struct log_multidomain_source_name source_name;
	struct log_multidomain_levels levels;
	struct log_multidomain_set_runtime_level set_rt_level;
	struct log_multidomain_dropped dropped;
};

/** @brief Message. */
struct log_multidomain_msg {
	uint8_t id;
	uint8_t status;
	union log_multidomain_msg_data data;
} __packed;

/** @brief Forward declaration. */
struct log_multidomain_link;

/** @brief Structure with link transport API. */
struct log_multidomain_link_transport_api {
	int (*init)(struct log_multidomain_link *link);
	int (*send)(struct log_multidomain_link *link, void *data, size_t len);
};

/** @brief Union for holding data returned by associated remote backend. */
union log_multidomain_link_dst {
	uint16_t count;

	struct {
		char *dst;
		size_t *len;
	} name;

	struct {
		uint8_t level;
		uint8_t runtime_level;
	} levels;

	struct {
		uint8_t level;
	} set_runtime_level;
};

/** @brief Remote link API. */
extern struct log_link_api log_multidomain_link_api;

/** @brief Remote link structure. */
struct log_multidomain_link {
	const struct log_multidomain_link_transport_api *transport_api;
	struct k_sem rdy_sem;
	const struct log_link *link;
	union log_multidomain_link_dst dst;
	int status;
	bool ready;
};

/** @brief Forward declaration. */
struct log_multidomain_backend;

/** @brief Backend transport API. */
struct log_multidomain_backend_transport_api {
	int (*init)(struct log_multidomain_backend *backend);
	int (*send)(struct log_multidomain_backend *backend, void *data, size_t len);
};

/** @brief Remote backend API. */
extern const struct log_backend_api log_multidomain_backend_api;

/** @brief Remote backend structure. */
struct log_multidomain_backend {
	const struct log_multidomain_backend_transport_api *transport_api;
	const struct log_backend *log_backend;
	struct k_sem rdy_sem;
	bool panic;
	int status;
	bool ready;
};

/** @brief Function to be called when data is received from remote.
 *
 * @param link Link instance.
 * @param data Data.
 * @param len Data length.
 */
void log_multidomain_link_on_recv_cb(struct log_multidomain_link *link,
				     const void *data, size_t len);

/** @brief Function called on error reported by transport layer.
 *
 * @param link Link instance.
 * @param err Error code.
 */
void log_multidomain_link_on_error(struct log_multidomain_link *link, int err);

/** @brief Function called when connection with remote is established.
 *
 * @param link Link instance.
 * @param err Error code.
 */
void log_multidomain_link_on_started(struct log_multidomain_link *link, int err);

/** @brief Function to be called when data is received from remote.
 *
 * @param backend Backend instance.
 * @param data Data.
 * @param len Data length.
 */
void log_multidomain_backend_on_recv_cb(struct log_multidomain_backend *backend,
					const void *data, size_t len);

/** @brief Function called on error reported by transport layer.
 *
 * @param backend Backend instance.
 * @param err Error code.
 */
void log_multidomain_backend_on_error(struct log_multidomain_backend *backend, int err);

/** @brief Function called when connection with remote is established.
 *
 * @param backend Backend instance.
 * @param err Error code.
 */
void log_multidomain_backend_on_started(struct log_multidomain_backend *backend, int err);

/** @} */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_MULTIDOMAIN_HELPER_H_ */
