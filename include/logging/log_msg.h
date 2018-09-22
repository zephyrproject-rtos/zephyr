/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_

#include <kernel.h>
#include <atomic.h>
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log message API
 * @defgroup log_msg Log message API
 * @ingroup logger
 * @{
 */

/** @brief Maximum number of arguments in the standard log entry. */
#define LOG_MAX_NARGS 6

/** @brief Number of arguments in the log entry which fits in one chunk.*/
#define LOG_MSG_NARGS_SINGLE_CHUNK 3

/** @brief Number of arguments in the head of extended standard log message..*/
#define LOG_MSG_NARGS_HEAD_CHUNK (LOG_MSG_NARGS_SINGLE_CHUNK - 1)

/** @brief Maximal amount of bytes in the hexdump entry which fits in one chunk.
 */
#define LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK \
	(LOG_MSG_NARGS_SINGLE_CHUNK * sizeof(u32_t))

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

#define ARGS_CONT_MSG (HEXDUMP_BYTES_CONT_MSG / sizeof(u32_t))

/** @brief Flag indicating standard log message. */
#define LOG_MSG_TYPE_STD 0

/** @brief Flag indicating hexdump log message. */
#define LOG_MSG_TYPE_HEXDUMP 1

/** @brief Common part of log message header. */
#define COMMON_PARAM_HDR() \
	u16_t type : 1;	   \
	u16_t ext : 1

/** @brief Number of bits used for storing length of hexdump log message. */
#define LOG_MSG_HEXDUMP_LENGTH_BITS 13

/** @brief Maximum length of log hexdump message. */
#define LOG_MSG_HEXDUMP_MAX_LENGTH ((1 << LOG_MSG_HEXDUMP_LENGTH_BITS) - 1)

/** @brief Part of log message header identifying source and level. */
struct log_msg_ids {
	u16_t level     : 3;    /*!< Severity. */
	u16_t domain_id : 3;    /*!< Originating domain. */
	u16_t source_id : 10;   /*!< Source ID. */
};

BUILD_ASSERT_MSG((sizeof(struct log_msg_ids) == sizeof(u16_t)),
		  "Structure must fit in 2 bytes");

/** Part of log message header common to standard and hexdump log message. */
struct log_msg_generic_hdr {
	COMMON_PARAM_HDR();
	u16_t reserved : 14;
};

BUILD_ASSERT_MSG((sizeof(struct log_msg_generic_hdr) == sizeof(u16_t)),
		 "Structure must fit in 2 bytes");

/** Part of log message header specific to standard log message. */
struct log_msg_std_hdr {
	COMMON_PARAM_HDR();
	u16_t reserved : 10;
	u16_t nargs    : 4;
};

BUILD_ASSERT_MSG((sizeof(struct log_msg_std_hdr) == sizeof(u16_t)),
		 "Structure must fit in 2 bytes");

/** Part of log message header specific to hexdump log message. */
struct log_msg_hexdump_hdr {
	COMMON_PARAM_HDR();
	u16_t raw_string : 1;
	u16_t length     : LOG_MSG_HEXDUMP_LENGTH_BITS;
};

BUILD_ASSERT_MSG((sizeof(struct log_msg_hexdump_hdr) == sizeof(u16_t)),
		 "Structure must fit in 2 bytes");

/** Log message header structure */
struct log_msg_hdr {
	atomic_t ref_cnt; /*!< Reference counter for tracking message users. */
	union log_msg_hdr_params {
		struct log_msg_generic_hdr generic;
		struct log_msg_std_hdr std;
		struct log_msg_hexdump_hdr hexdump;
		u16_t raw;
	} params;
	struct log_msg_ids ids; /*!< Identification part of the message.*/
	u32_t timestamp;        /*!< Timestamp. */
};

/** @brief Data part of log message. */
union log_msg_head_data {
	u32_t args[LOG_MSG_NARGS_SINGLE_CHUNK];
	u8_t bytes[LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK];
};

/** @brief Data part of extended log message. */
struct log_msg_ext_head_data {
	struct log_msg_cont *next;
	union log_msg_ext_head_data_data {
		u32_t args[LOG_MSG_NARGS_HEAD_CHUNK];
		u8_t bytes[LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK];
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

BUILD_ASSERT_MSG((sizeof(union log_msg_head_data) ==
		  sizeof(struct log_msg_ext_head_data)),
		  "Structure must be same size");

/** @brief Chunks following message head when message is extended. */
struct log_msg_cont {
	struct log_msg_cont *next; /*!< Pointer to the next chunk. */
	union log_msg_cont_data {
		u32_t args[ARGS_CONT_MSG];
		u8_t bytes[HEXDUMP_BYTES_CONT_MSG];
	} payload;
};

/** @brief Log message */
union log_msg_chunk {
	struct log_msg head;
	struct log_msg_cont cont;
};

extern struct k_mem_slab log_msg_pool;

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
static inline u32_t log_msg_domain_id_get(struct log_msg *msg)
{
	return msg->hdr.ids.domain_id;
}

/** @brief Get source ID (module or instance) of the message.
 *
 * @param msg Message
 *
 * @return Source ID.
 */
static inline u32_t log_msg_source_id_get(struct log_msg *msg)
{
	return msg->hdr.ids.source_id;
}

/** @brief Get severity level of the message.
 *
 * @param msg Message
 *
 * @return Severity message.
 */
static inline u32_t log_msg_level_get(struct log_msg *msg)
{
	return msg->hdr.ids.level;
}

/** @brief Get timestamp of the message.
 *
 * @param msg Message
 *
 * @return Timestamp value.
 */
static inline u32_t log_msg_timestamp_get(struct log_msg *msg)
{
	return msg->hdr.timestamp;
}

/** @brief Check if message is a raw string (see CONFIG_LOG_PRINTK).
 *
 * @param msg Message
 *
 * @retval true  Message contains raw string.
 * @retval false Message does not contain raw string.
 */
static inline bool log_msg_is_raw_string(struct log_msg *msg)
{
	return (msg->hdr.params.generic.type == LOG_MSG_TYPE_HEXDUMP) &&
	       (msg->hdr.params.hexdump.raw_string == 1);
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
u32_t log_msg_nargs_get(struct log_msg *msg);

/** @brief Gets argument from standard log message.
 *
 * @param msg		Standard log message.
 * @param arg_idx	Argument index.
 *
 * @return Argument value.
 */
u32_t log_msg_arg_get(struct log_msg *msg, u32_t arg_idx);


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
				       const u8_t *data,
				       u32_t length);

/** @brief Put data into hexdump log message.
 *
 * @param[in]		msg      Message.
 * @param[in]		data	 Data to be copied.
 * @param[in, out]	length   Input: requested amount. Output: actual amount.
 * @param[in]		offset   Offset.
 */
void log_msg_hexdump_data_put(struct log_msg *msg,
			      u8_t *data,
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
			      u8_t *data,
			      size_t *length,
			      size_t offset);

union log_msg_chunk *log_msg_no_space_handle(void);

static inline union log_msg_chunk *log_msg_chunk_alloc(void)
{
	union log_msg_chunk *msg = NULL;
	int err = k_mem_slab_alloc(&log_msg_pool, (void **)&msg, K_NO_WAIT);

	if (err != 0) {
		msg = log_msg_no_space_handle();
	}

	return msg;
}

/** @brief Allocate chunk for standard log message.
 *
 *  @return Allocated chunk of NULL.
 */
static inline struct log_msg *_log_msg_std_alloc(void)
{
	struct  log_msg *msg = (struct  log_msg *)log_msg_chunk_alloc();

	if (msg) {
		/* all fields reset to 0, reference counter to 1 */
		msg->hdr.ref_cnt = 1;
		msg->hdr.params.raw = 0;
		msg->hdr.params.std.type = LOG_MSG_TYPE_STD;
	}

	return msg;
}

/** @brief Allocate chunk for extended standard log message.
 *
 *  @details Extended standard log message is used when number of arguments
 *           exceeds capacity of one chunk. Extended message consists of two
 *           chunks. Such approach is taken to optimize memory usage and
 *           performance assuming that log messages with more arguments
 *           (@ref LOG_MSG_NARGS_SINGLE_CHUNK) are less common.
 *
 *  @return Allocated chunk of NULL.
 */
static inline struct log_msg *_log_msg_ext_std_alloc(void)
{
	struct log_msg_cont *cont;
	struct  log_msg *msg = _log_msg_std_alloc();

	if (msg != NULL) {
		cont = (struct log_msg_cont *)log_msg_chunk_alloc();
		if (cont == NULL) {
			k_mem_slab_free(&log_msg_pool, (void **)&msg);
			return NULL;
		}

		msg->hdr.params.generic.ext = 1;
		msg->payload.ext.next = cont;
		cont->next = NULL;
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
	struct log_msg *msg = _log_msg_std_alloc();

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
					       u32_t arg1)
{
	struct  log_msg *msg = _log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 1;
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
					       u32_t arg1,
					       u32_t arg2)
{
	struct  log_msg *msg = _log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 2;
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
					       u32_t arg1,
					       u32_t arg2,
					       u32_t arg3)
{
	struct  log_msg *msg = _log_msg_std_alloc();

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = 3;
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
				 u32_t *args,
				 u32_t nargs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_ */
