/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_H_

#include <logging/log_msg.h>
#include <sys/util.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log output API
 * @defgroup log_output Log output API
 * @ingroup logger
 * @{
 */

/** @brief Flag forcing ANSI escape code colors, red (errors), yellow
 *         (warnings).
 */
#define LOG_OUTPUT_FLAG_COLORS			BIT(0)

/** @brief Flag forcing timestamp */
#define LOG_OUTPUT_FLAG_TIMESTAMP		BIT(1)

/** @brief Flag forcing timestamp formatting. */
#define LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP	BIT(2)

/** @brief Flag forcing severity level prefix. */
#define LOG_OUTPUT_FLAG_LEVEL			BIT(3)

/** @brief Flag preventing the logger from adding CR and LF characters. */
#define LOG_OUTPUT_FLAG_CRLF_NONE		BIT(4)

/** @brief Flag forcing a single LF character for line breaks. */
#define LOG_OUTPUT_FLAG_CRLF_LFONLY		BIT(5)

/** @brief Flag forcing syslog format specified in RFC 5424
 */
#define LOG_OUTPUT_FLAG_FORMAT_SYSLOG		BIT(6)

/** @brief Flag forcing syslog format specified in mipi sys-t
 */
#define LOG_OUTPUT_FLAG_FORMAT_SYST		BIT(7)

/**
 * @brief Prototype of the function processing output data.
 *
 * @param data Data.
 * @param length Data length.
 * @param ctx  User context.
 *
 * @return Number of bytes processed, dropped or discarded.
 *
 * @note If the log output function cannot process all of the data, it is
 *       its responsibility to mark them as dropped or discarded by returning
 *       the corresponding number of bytes dropped or discarded to the caller.
 */
typedef int (*log_output_func_t)(u8_t *buf, size_t size, void *ctx);

/* @brief Control block structure for log_output instance.  */
struct log_output_control_block {
	size_t offset;
	void *ctx;
	const char *hostname;
};

/** @brief Log_output instance structure. */
struct log_output {
	log_output_func_t func;
	struct log_output_control_block *control_block;
	u8_t *buf;
	size_t size;
};

/** @brief Create log_output instance.
 *
 * @param _name Instance name.
 * @param _func Function for processing output data.
 * @param _buf  Pointer to the output buffer.
 * @param _size Size of the output buffer.
 */
#define LOG_OUTPUT_DEFINE(_name, _func, _buf, _size)			\
	static struct log_output_control_block _name##_control_block;	\
	static const struct log_output _name = {			\
		.func = _func,						\
		.control_block = &_name##_control_block,		\
		.buf = _buf,						\
		.size = _size,						\
	}

/** @brief Process log messages to readable strings.
 *
 * Function is using provided context with the buffer and output function to
 * process formatted string and output the data.
 *
 * @param log_output Pointer to the log output instance.
 * @param msg Log message.
 * @param flags Optional flags.
 */
void log_output_msg_process(const struct log_output *log_output,
			    struct log_msg *msg,
			    u32_t flags);

/** @brief Process log string
 *
 * Function is formatting provided string adding optional prefixes and
 * postfixes.
 *
 * @param log_output Pointer to log_output instance.
 * @param src_level  Log source and level structure.
 * @param timestamp  Timestamp.
 * @param fmt        String.
 * @param ap         String arguments.
 * @param flags      Optional flags.
 *
 */
void log_output_string(const struct log_output *log_output,
		       struct log_msg_ids src_level, u32_t timestamp,
		       const char *fmt, va_list ap, u32_t flags);

/** @brief Process log hexdump
 *
 * Function is formatting provided hexdump adding optional prefixes and
 * postfixes.
 *
 * @param log_output Pointer to log_output instance.
 * @param src_level  Log source and level structure.
 * @param timestamp  Timestamp.
 * @param metadata   String.
 * @param data       Data.
 * @param length     Data length.
 * @param flags      Optional flags.
 *
 */
void log_output_hexdump(const struct log_output *log_output,
			     struct log_msg_ids src_level, u32_t timestamp,
			     const char *metadata, const u8_t *data,
			     u32_t length, u32_t flags);

/** @brief Process dropped messages indication.
 *
 * Function prints error message indicating lost log messages.
 *
 * @param log_output Pointer to the log output instance.
 * @param cnt        Number of dropped messages.
 */
void log_output_dropped_process(const struct log_output *log_output, u32_t cnt);

/** @brief Flush output buffer.
 *
 * @param log_output Pointer to the log output instance.
 */
void log_output_flush(const struct log_output *log_output);

/** @brief Function for setting user context passed to the output function.
 *
 * @param log_output	Pointer to the log output instance.
 * @param ctx		User context.
 */
static inline void log_output_ctx_set(const struct log_output *log_output,
				      void *ctx)
{
	log_output->control_block->ctx = ctx;
}

/** @brief Function for setting hostname of this device
 *
 * @param log_output	Pointer to the log output instance.
 * @param hostname	Hostname of this device
 */
static inline void log_output_hostname_set(const struct log_output *log_output,
					   const char *hostname)
{
	log_output->control_block->hostname = hostname;
}

/** @brief Set timestamp frequency.
 *
 * @param freq Frequency in Hz.
 */
void log_output_timestamp_freq_set(u32_t freq);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_H_ */
