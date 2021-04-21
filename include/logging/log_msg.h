/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_

#include <sys/atomic.h>
#include <sys/util.h>
#include <string.h>
#include <logging/log_msg2.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log message API
 * @defgroup log_msg Log message API
 * @ingroup logger
 * @{
 */

/** @brief Log argument type.
 *
 * Should preferably be equivalent to a native word size.
 */
typedef unsigned long log_arg_t;

/** @brief Maximum number of arguments in the standard log entry.
 *
 * It is limited by 4 bit nargs field in the log message.
 */
#define LOG_MAX_NARGS 15

/** @brief Number of arguments in the log entry which fits in one chunk.*/
#ifdef CONFIG_64BIT
#define LOG_MSG_NARGS_SINGLE_CHUNK 4U
#else
#define LOG_MSG_NARGS_SINGLE_CHUNK 3U
#endif

/** @brief Number of arguments in the head of extended standard log message..*/
#define LOG_MSG_NARGS_HEAD_CHUNK \
	(LOG_MSG_NARGS_SINGLE_CHUNK - (sizeof(void *)/sizeof(log_arg_t)))

/** @brief Maximal amount of bytes in the hexdump entry which fits in one chunk.
 */
#define LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK \
	(LOG_MSG_NARGS_SINGLE_CHUNK * sizeof(log_arg_t))

/** @brief Number of bytes in the first chunk of hexdump message if message
 *         consists of more than one chunk.
 */
#define LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK \
	(LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK - sizeof(void *))

/** @brief Number of bytes that can be stored in chunks following head chunk
 *         in hexdump log message.
 */
#define HEXDUMP_BYTES_CONT_MSG \
	(sizeof(struct log_msg) - sizeof(void *))

#define ARGS_CONT_MSG (HEXDUMP_BYTES_CONT_MSG / sizeof(log_arg_t))

/** @brief Flag indicating standard log message. */
#define LOG_MSG_TYPE_STD 0U

/** @brief Flag indicating hexdump log message. */
#define LOG_MSG_TYPE_HEXDUMP 1

/** @brief Common part of log message header. */
#define COMMON_PARAM_HDR() \
	uint16_t type : 1;	   \
	uint16_t ext : 1

/** @brief Number of bits used for storing length of hexdump log message. */
#define LOG_MSG_HEXDUMP_LENGTH_BITS 14

/** @brief Maximum length of log hexdump message. */
#define LOG_MSG_HEXDUMP_MAX_LENGTH (BIT(LOG_MSG_HEXDUMP_LENGTH_BITS) - 1)

/** @brief Part of log message header identifying source and level. */
struct log_msg_ids {
	uint16_t level     : 3;    /*!< Severity. */
	uint16_t domain_id : 3;    /*!< Originating domain. */
	uint16_t source_id : 10;   /*!< Source ID. */
};

/** Part of log message header common to standard and hexdump log message. */
struct log_msg_generic_hdr {
	COMMON_PARAM_HDR();
	uint16_t reserved : 14;
};

/** Part of log message header specific to standard log message. */
struct log_msg_std_hdr {
	COMMON_PARAM_HDR();
	uint16_t reserved : 10;
	uint16_t nargs    : 4;
};

/** Part of log message header specific to hexdump log message. */
struct log_msg_hexdump_hdr {
	COMMON_PARAM_HDR();
	uint16_t length     : LOG_MSG_HEXDUMP_LENGTH_BITS;
};

/** Log message header structure */
struct log_msg_hdr {
	atomic_t ref_cnt; /*!< Reference counter for tracking message users. */
	union log_msg_hdr_params {
		struct log_msg_generic_hdr generic;
		struct log_msg_std_hdr std;
		struct log_msg_hexdump_hdr hexdump;
		uint16_t raw;
	} params;
	struct log_msg_ids ids; /*!< Identification part of the message.*/
	uint32_t timestamp;        /*!< Timestamp. */
};

/** @brief Data part of log message. */
union log_msg_head_data {
	log_arg_t args[LOG_MSG_NARGS_SINGLE_CHUNK];
	uint8_t bytes[LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK];
};

/** @brief Data part of extended log message. */
struct log_msg_ext_head_data {
	struct log_msg_cont *next;
	union log_msg_ext_head_data_data {
		log_arg_t args[LOG_MSG_NARGS_HEAD_CHUNK];
		uint8_t bytes[LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK];
	} data;
};

/** @brief Log message structure. */
struct log_msg {
	struct log_msg *next;   /*!< Used by logger core list.*/
	struct log_msg_hdr hdr; /*!< Message header. */
	const char *str;
	union log_msg_data {
		union log_msg_head_data single;
		struct log_msg_ext_head_data ext;
	} payload;                 /*!< Message data. */
};

/** @brief Chunks following message head when message is extended. */
struct log_msg_cont {
	struct log_msg_cont *next; /*!< Pointer to the next chunk. */
	union log_msg_cont_data {
		log_arg_t args[ARGS_CONT_MSG];
		uint8_t bytes[HEXDUMP_BYTES_CONT_MSG];
	} payload;
};

/** @brief Log message */
union log_msg_chunk {
	struct log_msg head;
	struct log_msg_cont cont;
};

/** @brief Function for initialization of the log message pool. */
void log_msg_pool_init(void);

/** @brief Function for indicating that message is in use.
 *
 *  @details Message can be used (read) by multiple users. Internal reference
 *           counter is atomically increased. See @ref log_msg_put.
 *
 *  @param msg Message.
 */
void log_msg_get(struct log_msg *msg);

/** @brief Function for indicating that message is no longer in use.
 *
 *  @details Internal reference counter is atomically decreased. If reference
 *           counter equals 0 message is freed.
 *
 *  @param msg Message.
 */
void log_msg_put(struct log_msg *msg);

/** @brief Get domain ID of the message.
 *
 * @param msg Message
 *
 * @return Domain ID.
 */
static inline uint32_t log_msg_domain_id_get(struct log_msg *msg)
{
	return msg->hdr.ids.domain_id;
}

/** @brief Get source ID (module or instance) of the message.
 *
 * @param msg Message
 *
 * @return Source ID.
 */
static inline uint32_t log_msg_source_id_get(struct log_msg *msg)
{
	return msg->hdr.ids.source_id;
}

/** @brief Get severity level of the message.
 *
 * @param msg Message
 *
 * @return Severity message.
 */
static inline uint32_t log_msg_level_get(struct log_msg *msg)
{
	return msg->hdr.ids.level;
}

/** @brief Get timestamp of the message.
 *
 * @param msg Message
 *
 * @return Timestamp value.
 */
static inline uint32_t log_msg_timestamp_get(struct log_msg *msg)
{
	return msg->hdr.timestamp;
}

/** @brief Check if message is of standard type.
 *
 * @param msg Message
 *
 * @retval true  Standard message.
 * @retval false Hexdump message.
 */
static inline bool log_msg_is_std(struct log_msg *msg)
{
	return  (msg->hdr.params.generic.type == LOG_MSG_TYPE_STD);
}

/** @brief Returns number of arguments in standard log message.
 *
 * @param msg Standard log message.
 *
 * @return Number of arguments.
 */
uint32_t log_msg_nargs_get(struct log_msg *msg);

/** @brief Gets argument from standard log message.
 *
 * @param msg		Standard log message.
 * @param arg_idx	Argument index.
 *
 * @return Argument value or 0 if arg_idx exceeds number of arguments in the
 *	   message.
 */
log_arg_t log_msg_arg_get(struct log_msg *msg, uint32_t arg_idx);


/** @brief Gets pointer to the unformatted string from standard log message.
 *
 * @param msg Standard log message.
 *
 * @return Pointer to the string.
 */
const char *log_msg_str_get(struct log_msg *msg);

/** @brief Allocates chunks for hexdump message and copies the data.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- length
 *
 *  @note Allocation and partial filling is combined for performance reasons.
 *
 * @param str		String.
 * @param data		Data.
 * @param length	Data length.
 *
 * @return Pointer to allocated head of the message or NULL
 */
struct log_msg *log_msg_hexdump_create(const char *str,
				       const uint8_t *data,
				       uint32_t length);

/** @brief Put data into hexdump log message.
 *
 * @param[in]		msg      Message.
 * @param[in]		data	 Data to be copied.
 * @param[in, out]	length   Input: requested amount. Output: actual amount.
 * @param[in]		offset   Offset.
 */
void log_msg_hexdump_data_put(struct log_msg *msg,
			      uint8_t *data,
			      size_t *length,
			      size_t offset);

/** @brief Get data from hexdump log message.
 *
 * @param[in]		msg      Message.
 * @param[in]		data	 Buffer for data.
 * @param[in, out]	length   Input: requested amount. Output: actual amount.
 * @param[in]		offset   Offset.
 */
void log_msg_hexdump_data_get(struct log_msg *msg,
			      uint8_t *data,
			      size_t *length,
			      size_t offset);

union log_msg_chunk *log_msg_no_space_handle(void);

/** @brief Allocate single chunk from the pool.
 *
 * @return Pointer to the allocated chunk or NULL if failed to allocate.
 */
union log_msg_chunk *log_msg_chunk_alloc(void);

/** @brief Allocate chunk for standard log message.
 *
 *  @return Allocated chunk of NULL.
 */
static inline struct log_msg *z_log_msg_std_alloc(void)
{
	struct  log_msg *msg = (struct  log_msg *)log_msg_chunk_alloc();

	if (msg != NULL) {
		/* all fields reset to 0, reference counter to 1 */
		msg->hdr.ref_cnt = 1;
		msg->hdr.params.raw = 0U;
		msg->hdr.params.std.type = LOG_MSG_TYPE_STD;

		if (IS_ENABLED(CONFIG_USERSPACE)) {
			/* it may be used in msg_free() function. */
			msg->hdr.ids.level = 0;
			msg->hdr.ids.domain_id = 0;
			msg->hdr.ids.source_id = 0;
		}
	}

	return msg;
}

/** @brief Create standard log message with no arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_0(const char *str)
{
	struct log_msg *msg = z_log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
	}

	return msg;
}

/** @brief Create standard log message with one argument.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- argument
 *
 *  @param str  String.
 *  @param arg1 Argument.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_1(const char *str,
					       log_arg_t arg1)
{
	struct  log_msg *msg = z_log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 1U;
		msg->payload.single.args[0] = arg1;
	}

	return msg;
}

/** @brief Create standard log message with two arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @param str  String.
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_2(const char *str,
					       log_arg_t arg1,
					       log_arg_t arg2)
{
	struct  log_msg *msg = z_log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 2U;
		msg->payload.single.args[0] = arg1;
		msg->payload.single.args[1] = arg2;
	}

	return msg;
}

/** @brief Create standard log message with three arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @param str  String.
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *  @param arg3 Argument 3.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_3(const char *str,
					       log_arg_t arg1,
					       log_arg_t arg2,
					       log_arg_t arg3)
{
	struct  log_msg *msg = z_log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 3U;
		msg->payload.single.args[0] = arg1;
		msg->payload.single.args[1] = arg2;
		msg->payload.single.args[2] = arg3;
	}

	return msg;
}

/** @brief Create standard log message with variable number of arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @param str   String.
 *  @param args  Array with arguments.
 *  @param nargs Number of arguments.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
struct log_msg *log_msg_create_n(const char *str,
				 log_arg_t *args,
				 uint32_t nargs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_ */
