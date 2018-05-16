/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_MSG_H
#define LOG_MSG_H

#include <kernel.h>
#include <atomic.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum number of arguments in the standard log entry. */
#define LOG_MAX_NARGS 6

/** @brief Number of arguments in the log entry which fits in one chunk.*/
#define LOG_MSG_NARGS_SINGLE_CHUNK 3

/** @brief Number of arguments in the head of extended standard log message..*/
#define LOG_MSG_NARGS_HEAD_CHUNK (LOG_MSG_NARGS_SINGLE_CHUNK - 1)

/** @brief Maxium amount of bytes in the hexdump entry which fits in one chunk. */
#define LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK \
	(LOG_MSG_NARGS_SINGLE_CHUNK * sizeof(u32_t))

/** @brief Number of bytes in the first chunk of hexdump message if message
 *         consists of more than one chunk.
 */
#define LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK \
	(LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK - sizeof(void *))

/** @brief Number of bytes that can be stored in chunks following head chunk
 *         in hexdump log message.*/
#define HEXDUMP_BYTES_CONT_MSG \
	(sizeof(struct log_msg) - sizeof(void *))

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

static_assert(sizeof(struct log_msg_ids) == sizeof(u16_t),
	      "Structure must fit in 2 bytes");

/** Part of log message header common to standard and hexdump log message. */
struct log_msg_generic_hdr {
	COMMON_PARAM_HDR();
	u16_t reserved : 14;
};

static_assert(sizeof(struct log_msg_generic_hdr) == sizeof(u16_t),
	      "Structure must fit in 2 bytes");

/** Part of log message header specific to standard log message. */
struct log_msg_std_hdr {
	COMMON_PARAM_HDR();
	u16_t reserved : 10;
	u16_t nargs    : 4;
};

static_assert(sizeof(struct log_msg_std_hdr) == sizeof(u16_t),
	      "Structure must fit in 2 bytes");

/** Part of log message header specific to hexdump log message. */
struct log_msg_hexdump_hdr {
	COMMON_PARAM_HDR();
	u16_t raw_string : 1;
	u16_t length     : LOG_MSG_HEXDUMP_LENGTH_BITS;
};

static_assert(sizeof(struct log_msg_hexdump_hdr) == sizeof(u16_t),
	      "Structure must fit in 2 bytes");

/** Log message header structure */
struct log_msg_hdr {
	atomic_t ref_cnt;       /*!< Reference counter used for tracking number of message users. */
	union {
		struct log_msg_generic_hdr generic;
		struct log_msg_std_hdr std;
		struct log_msg_hexdump_hdr hexdump;
		u16_t raw;
	} params;
	struct log_msg_ids ids; /*!< Identification part of the message.*/
	u32_t timestamp;        /*!< Timestamp. */
};

/** @brief Data part of head of standard log message. */
struct log_msg_std_head_data {
	const char *p_str;
	u32_t args[LOG_MSG_NARGS_SINGLE_CHUNK];
};

/** @brief Data part of head of extended standard log message.
 *
 * @details When message is extended, head of the message contains also a
 *          pointer to the next chunk thus data part is reduced.
 */
struct log_msg_std_ext_head_data {
	const char *p_str;
	u32_t args[LOG_MSG_NARGS_HEAD_CHUNK];
};

/** @brief Data part of log message. */
union log_msg_head_data {
	struct log_msg_std_head_data std;
	u8_t bytes[LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK];
};

/** @brief Data part of extended log message. */
struct log_msg_ext_head_data {
	struct log_msg_cont *p_next;
	union {
		struct log_msg_std_ext_head_data std;
		u8_t bytes[LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK];
	} data;
};

/** @brief Log message structure. */
struct log_msg {
	struct log_msg *p_next; /*!< Used by logger core to put message into the list.*/
	struct log_msg_hdr hdr; /*!< Message header. */

	union {
		union log_msg_head_data data;
		struct log_msg_ext_head_data ext;
	} data;                 /*!< Message data. */
};

static_assert(sizeof(union log_msg_head_data) ==
	      sizeof(struct log_msg_ext_head_data),
	      "Structure must be same size");

/** @brief Chunks following message head when message is extended. */
struct log_msg_cont {
	struct log_msg_cont *p_next;                                    /*!< Pointer to the next chunk. */
	union {
		u32_t args[LOG_MAX_NARGS - LOG_MSG_NARGS_HEAD_CHUNK];   /*!< Standard message extension. */
		u8_t bytes[HEXDUMP_BYTES_CONT_MSG];                     /*!< Hexdump message extension. */
	} data;
};



/** @brief Log message */
union log_msg_chunk {
	struct log_msg head;
	struct log_msg_cont cont;
};

extern struct k_mem_slab log_msg_pool;

/** @brief Function for indicating that message is in use.
 *
 *  @details Message can be used (read) by multiple users. Internal reference
 *           counter is atomically increased. See @ref log_msg_put.
 *
 *  @param p_msg Message.
 */
void log_msg_get(struct  log_msg *p_msg);

/** @brief Function for indicating that message is no longer in use.
 *
 *  @details Internal reference counter is atomically decreased. If reference
 *           counter equals 0 message is freed.
 *
 *  @param p_msg Message.
 */
void log_msg_put(struct  log_msg *p_msg);

/** @brief Get domain ID of the message.
 *
 * @param p_msg Message
 *
 * @return Domain ID.
 */
static inline u32_t log_msg_domain_id_get(struct log_msg *p_msg)
{
	return p_msg->hdr.ids.domain_id;
}

/** @brief Get source ID (module or instance) of the message.
 *
 * @param p_msg Message
 *
 * @return Source ID.
 */
static inline u32_t log_msg_source_id_get(struct log_msg *p_msg)
{
	return p_msg->hdr.ids.source_id;
}

/** @brief Get severity level of the message.
 *
 * @param p_msg Message
 *
 * @return Severity message.
 */
static inline u32_t log_msg_level_get(struct log_msg *p_msg)
{
	return p_msg->hdr.ids.level;
}

/** @brief Get timestamp of the message.
 *
 * @param p_msg Message
 *
 * @return Timestamp value.
 */
static inline u32_t log_msg_timestamp_get(struct log_msg *p_msg)
{
	return p_msg->hdr.timestamp;
}

/** @brief Check if message is a raw string (@ref CONFIG_LOG_PRINTK).
 *
 * @param p_msg Message
 *
 * @retval true  Message contains raw string.
 * @retval false Message does not contain raw string.
 */
static inline bool log_msg_is_raw_string(struct log_msg *p_msg)
{
	return (p_msg->hdr.params.generic.type == LOG_MSG_TYPE_HEXDUMP) &&
	       (p_msg->hdr.params.hexdump.raw_string == 1);
}


/** @brief Check if message is of standard type.
 *
 * @param p_msg Message
 *
 * @retval true  Standard message.
 * @retval false Hexdump message.
 */
static inline bool log_msg_is_std(struct log_msg *p_msg)
{
	return  (p_msg->hdr.params.generic.type == LOG_MSG_TYPE_STD);
}

/** @brief Returns number of arguments in standard log message.
 *
 * @param p_msg Standard log message.
 *
 * @return Number of arguments.
 */
u32_t log_msg_nargs_get(struct log_msg *p_msg);

/** @brief Gets argument from standard log message.
 *
 * @param p_msg		Standard log message.
 * @param arg_idx	Argument index.
 *
 * @return Argument value.
 */
u32_t log_msg_arg_get(struct log_msg *p_msg, u32_t arg_idx);


/** @brief Gets pointer to the unformatted string from standard log message.
 *
 * @param p_msg Standard log message.
 *
 * @return Pointer to the string.
 */
const char *log_msg_str_get(struct log_msg *p_msg);

/** @brief Allocates chunks for hexdump message and copies the data.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- length
 *
 *  @note Allocation and partial filling is combined for performance reasons.
 *
 *  @return Pointer to allocated head of the message or NULL
 */
struct log_msg *log_msg_hexdump_create(const u8_t *p_data, u32_t length);

/** @brief Put data into hexdump log message.
 *
 * @param[in]		p_data	 Data to be copied.
 * @param[in, out]	p_length Input: requested amount. Output: actual amount.
 * @param[in]		offset   Offset.
 */
void log_msg_hexdump_data_put(struct log_msg *p_msg,
			      u8_t *p_data,
			      u32_t *p_length,
			      u32_t offset);

/** @brief Get data from hexdump log message.
 *
 * @param[in]		p_data	 Buffer for data.
 * @param[in, out]	p_length Input: requested amount. Output: actual amount.
 * @param[in]		offset   Offset.
 */
void log_msg_hexdump_data_get(struct log_msg *p_msg,
			      u8_t *p_data,
			      u32_t *p_length,
			      u32_t offset);

/** @brief Allocate chunk for standard log message.
 *
 *  @return Allocated chunk of NULL.
 */
static inline struct log_msg *_log_msg_std_alloc(void)
{
	struct  log_msg *p_msg;
	int err = k_mem_slab_alloc(&log_msg_pool, (void **)&p_msg, K_NO_WAIT);

	if (err != 0) {
		/* For now do not overwrite.*/
		return NULL;
	}
	/* all fields reset to 0, reference counter to 1 */
	p_msg->hdr.ref_cnt = 1;
	p_msg->hdr.params.raw = 0;
	p_msg->hdr.params.std.type = LOG_MSG_TYPE_STD;
	return p_msg;
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
	struct log_msg_cont *p_cont;
	struct  log_msg *p_msg = _log_msg_std_alloc();

	if (p_msg != NULL) {
		int err = k_mem_slab_alloc(&log_msg_pool,
					   (void **)&p_cont, K_NO_WAIT);
		if (err != 0) {
			k_mem_slab_free(&log_msg_pool, (void **)&p_msg);
			return NULL;
		}

		p_msg->hdr.params.generic.ext = 1;
		p_msg->data.ext.p_next = p_cont;
		p_cont->p_next = NULL;
	}
	return p_msg;
}

/** @brief Create standard log message with no arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_0(const char *p_str)
{
	struct log_msg *p_msg = _log_msg_std_alloc();

	if (p_msg != NULL) {
		p_msg->data.data.std.p_str = p_str;
	}
	return p_msg;
}

/** @brief Create standard log message with one argument.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- argument
 *
 *  @param arg1 Argument.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_1(const char *p_str,
					       u32_t arg1)
{
	struct  log_msg *p_msg = _log_msg_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 1;
		p_msg->data.data.std.p_str = p_str;
		p_msg->data.data.std.args[0] = arg1;
	}
	return p_msg;
}

/** @brief Create standard log message with two arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_2(const char *p_str,
					       u32_t arg1,
					       u32_t arg2)
{
	struct  log_msg *p_msg = _log_msg_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 2;
		p_msg->data.data.std.p_str = p_str;
		p_msg->data.data.std.args[0] = arg1;
		p_msg->data.data.std.args[1] = arg2;
	}
	return p_msg;
}

/** @brief Create standard log message with three arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *  @param arg3 Argument 3.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_3(const char *p_str,
					       u32_t arg1,
					       u32_t arg2,
					       u32_t arg3)
{
	struct  log_msg *p_msg = _log_msg_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 3;
		p_msg->data.data.std.p_str = p_str;
		p_msg->data.data.std.args[0] = arg1;
		p_msg->data.data.std.args[1] = arg2;
		p_msg->data.data.std.args[2] = arg3;
	}
	return p_msg;
}

/** @brief Create standard log message with four arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @note Four arguments requires extended message consisting of two chunks.
 *
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *  @param arg3 Argument 3.
 *  @param arg4 Argument 4.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_4(const char *p_str,
					       u32_t arg1,
					       u32_t arg2,
					       u32_t arg3,
					       u32_t arg4)
{
	struct  log_msg *p_msg = _log_msg_ext_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 4;
		p_msg->data.ext.data.std.p_str = p_str;
		p_msg->data.ext.data.std.args[0] = arg1;
		p_msg->data.ext.data.std.args[1] = arg2;
		p_msg->data.ext.p_next->data.args[0] = arg3;
		p_msg->data.ext.p_next->data.args[1] = arg4;
	}
	return p_msg;
}

/** @brief Create standard log message with five arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @note Five arguments requires extended message consisting of two chunks.
 *
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *  @param arg3 Argument 3.
 *  @param arg4 Argument 4.
 *  @param arg4 Argument 5.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_5(const char *p_str,
					       u32_t arg1,
					       u32_t arg2,
					       u32_t arg3,
					       u32_t arg4,
					       u32_t arg5)
{
	struct  log_msg *p_msg = _log_msg_ext_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 4;
		p_msg->data.ext.data.std.p_str = p_str;
		p_msg->data.ext.data.std.args[0] = arg1;
		p_msg->data.ext.data.std.args[1] = arg2;
		p_msg->data.ext.p_next->data.args[0] = arg3;
		p_msg->data.ext.p_next->data.args[1] = arg4;
		p_msg->data.ext.p_next->data.args[2] = arg5;
	}
	return p_msg;
}

/** @brief Create standard log message with six arguments.
 *
 *  @details Function resets header and sets following fields:
 *		- message type
 *		- string pointer
 *		- number of arguments
 *		- arguments
 *
 *  @note Six arguments requires extended message consisting of two chunks.
 *
 *  @param arg1 Argument 1.
 *  @param arg2 Argument 2.
 *  @param arg3 Argument 3.
 *  @param arg4 Argument 4.
 *  @param arg5 Argument 5.
 *  @param arg6 Argument 6.
 *
 *  @return Pointer to allocated head of the message or NULL.
 */
static inline struct log_msg *log_msg_create_6(const char *p_str,
					       u32_t arg1,
					       u32_t arg2,
					       u32_t arg3,
					       u32_t arg4,
					       u32_t arg5,
					       u32_t arg6)
{
	struct  log_msg *p_msg = _log_msg_ext_std_alloc();

	if (p_msg != NULL) {
		p_msg->hdr.params.std.nargs = 4;
		p_msg->data.ext.data.std.p_str = p_str;
		p_msg->data.ext.data.std.args[0] = arg1;
		p_msg->data.ext.data.std.args[1] = arg2;
		p_msg->data.ext.p_next->data.args[0] = arg3;
		p_msg->data.ext.p_next->data.args[1] = arg4;
		p_msg->data.ext.p_next->data.args[2] = arg5;
		p_msg->data.ext.p_next->data.args[3] = arg6;
	}
	return p_msg;
}

#ifdef __cplusplus
}
#endif

#endif /* LOG_MSG_H */
