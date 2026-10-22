/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the multidomain logging helpers.
 * @ingroup log_backend_multidomain
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
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data); /**< Serialized log message bytes. */
} __packed;

/** @brief Content of the domain count message. */
struct log_multidomain_domain_cnt {
	uint16_t count; /**< Number of domains. */
} __packed;

/** @brief Content of the source count message. */
struct log_multidomain_source_cnt {
	uint8_t domain_id; /**< Domain ID the request applies to. */
	uint16_t count;    /**< Number of sources in the domain. */
} __packed;

/** @brief Content of the domain name message. */
struct log_multidomain_domain_name {
	uint8_t domain_id; /**< Domain ID. */
	char name[];       /**< Null-terminated domain name. */
} __packed;

/** @brief Content of the source name message. */
struct log_multidomain_source_name {
	uint8_t domain_id;  /**< Domain ID. */
	uint16_t source_id; /**< Source ID within the domain. */
	char name[];        /**< Null-terminated source name. */
} __packed;

/** @brief Content of the message for getting logging levels. */
struct log_multidomain_levels {
	uint8_t domain_id;     /**< Domain ID. */
	uint16_t source_id;    /**< Source ID within the domain. */
	uint8_t level;         /**< Compile-time level. */
	uint8_t runtime_level; /**< Run-time level. */
} __packed;

/** @brief Content of the message for setting logging level. */
struct log_multidomain_set_runtime_level {
	uint8_t domain_id;     /**< Domain ID. */
	uint16_t source_id;    /**< Source ID within the domain. */
	uint8_t runtime_level; /**< Run-time level to set. */
} __packed;

/** @brief Content of the message for getting amount of dropped messages. */
struct log_multidomain_dropped {
	uint32_t dropped; /**< Number of dropped messages. */
} __packed;

/** @brief Union with all message types. */
union log_multidomain_msg_data {
	struct log_multidomain_log_msg log_msg;           /**< Log message payload. */
	struct log_multidomain_domain_cnt domain_cnt;     /**< Domain count payload. */
	struct log_multidomain_source_cnt source_cnt;     /**< Source count payload. */
	struct log_multidomain_domain_name domain_name;   /**< Domain name payload. */
	struct log_multidomain_source_name source_name;   /**< Source name payload. */
	struct log_multidomain_levels levels;             /**< Levels payload. */
	struct log_multidomain_set_runtime_level set_rt_level; /**< Set-level payload. */
	struct log_multidomain_dropped dropped;           /**< Dropped count payload. */
};

/** @brief Message. */
struct log_multidomain_msg {
	uint8_t id;     /**< Message ID, see @ref LOG_MULTIDOMAIN_HELPER_MESSAGE_IDS. */
	uint8_t status; /**< Status code, see @ref LOG_MULTIDOMAIN_STATUS. */
	union log_multidomain_msg_data data; /**< Message payload. */
} __packed;

/** @brief Forward declaration. */
struct log_multidomain_link;

/** @brief Structure with link transport API. */
struct log_multidomain_link_transport_api {
	/** @brief Initialize the transport. */
	int (*init)(struct log_multidomain_link *link);
	/** @brief Send a buffer to the remote backend. */
	int (*send)(struct log_multidomain_link *link, void *data, size_t len);
};

/** @brief Union for holding data returned by associated remote backend. */
union log_multidomain_link_dst {
	uint16_t count; /**< Domain or source count. */

	/** @brief Destination buffer for a requested name. */
	struct {
		char *dst;   /**< Output buffer for the name. */
		size_t *len; /**< In/out buffer length, updated with the name length. */
	} name;

	/** @brief Returned level settings of a source. */
	struct {
		uint8_t level;         /**< Compile-time level. */
		uint8_t runtime_level; /**< Run-time level. */
	} levels;

	/** @brief Result of a set-runtime-level request. */
	struct {
		uint8_t level; /**< Run-time level actually set. */
	} set_runtime_level;
};

/** @brief Remote link API. */
extern struct log_link_api log_multidomain_link_api;

/** @brief Remote link structure. */
struct log_multidomain_link {
	/** Transport operations. */
	const struct log_multidomain_link_transport_api *transport_api;
	/** Semaphore signaled when a response is ready. */
	struct k_sem rdy_sem;
	/** Associated log link instance. */
	const struct log_link *link;
	/** Storage for the latest response. */
	union log_multidomain_link_dst dst;
	/** Status of the last operation. */
	int status;
	/** Set once the link is ready. */
	bool ready;
};

/** @brief Forward declaration. */
struct log_multidomain_backend;

/** @brief Backend transport API. */
struct log_multidomain_backend_transport_api {
	/** Initialize the transport. */
	int (*init)(struct log_multidomain_backend *backend);
	/** Send a buffer to the remote link. */
	int (*send)(struct log_multidomain_backend *backend, void *data, size_t len);
};

/** @brief Remote backend API. */
extern const struct log_backend_api log_multidomain_backend_api;

/** @brief Remote backend structure. */
struct log_multidomain_backend {
	/** Transport operations. */
	const struct log_multidomain_backend_transport_api *transport_api;
	/** Associated log backend instance. */
	const struct log_backend *log_backend;
	/** Semaphore signaled when the remote is ready. */
	struct k_sem rdy_sem;
	/** Set when the backend is in panic mode. */
	bool panic;
	/** Status of the last operation. */
	int status;
	/** Set once the backend is ready. */
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
