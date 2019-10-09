/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_LOG_BACKEND_STD_H_
#define ZEPHYR_LOG_BACKEND_STD_H_

#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <irq.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Put log message to a standard logger backend.
 *
 * @param log_output	Log output instance.
 * @param flags		Formatting flags.
 * @param msg		Log message.
 */
static inline void
log_backend_std_put(const struct log_output *const log_output, u32_t flags,
		    struct log_msg *msg)
{
	log_msg_get(msg);

	flags |= (LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(log_output, msg, flags);

	log_msg_put(msg);
}

/** @brief Put a standard logger backend into panic mode.
 *
 * @param log_output	Log output instance.
 */
static inline void
log_backend_std_panic(const struct log_output *const log_output)
{
	log_output_flush(log_output);
}

/** @brief Report dropped messages to a standard logger backend.
 *
 * @param log_output	Log output instance.
 * @param cnt		Number of dropped messages.
 */
static inline void
log_backend_std_dropped(const struct log_output *const log_output, u32_t cnt)
{
	log_output_dropped_process(log_output, cnt);
}

/** @brief Synchronously process log message by a standard logger backend.
 *
 * @param log_output	Log output instance.
 * @param flags		Formatting flags.
 * @param src_level	Log message source and level.
 * @param timestamp	Timestamp.
 * @param fmt		Log string.
 * @param ap		Log string arguments.
 */
static inline void
log_backend_std_sync_string(const struct log_output *const log_output,
			    u32_t flags, struct log_msg_ids src_level,
			    u32_t timestamp, const char *fmt, va_list ap)
{
	u32_t key;

	flags |= LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	/* Even though interrupts are locked here there are still cases when
	 * it may lead to failure. Log output is not re-entrant and irq_lock
	 * does not prevent NMI or ZLI (Zero latency interrupts). If context
	 * is interrupted by NMI it usually means fault scenario and best that
	 * can be done is to flush the output and process new data.
	 */
	log_output_flush(log_output);
	log_output_string(log_output, src_level, timestamp, fmt, ap, flags);
	irq_unlock(key);
}

/** @brief Synchronously process hexdump message by a standard logger backend.
 *
 * @param log_output	Log output instance.
 * @param flags		Formatting flags.
 * @param src_level	Log message source and level.
 * @param timestamp	Timestamp.
 * @param metadata	String associated with a hexdump.
 * @param data		Buffer to dump.
 * @param length	Length of the buffer.
 */
static inline void
log_backend_std_sync_hexdump(const struct log_output *const log_output,
			     u32_t flags, struct log_msg_ids src_level,
			     u32_t timestamp, const char *metadata,
			     const u8_t *data, u32_t length)
{
	u32_t key;

	flags |= LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	/* Even though interrupts are locked here there are still cases when
	 * it may lead to failure. Log output is not re-entrant and irq_lock
	 * does not prevent NMI or ZLI (Zero latency interrupts). If context
	 * is interrupted by NMI it usually means fault scenario and best that
	 * can be done is to flush the output and process new data.
	 */
	log_output_flush(log_output);
	log_output_hexdump(log_output, src_level, timestamp,
			metadata, data, length, flags);
	irq_unlock(key);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LOG_BACKEND_STD_H_ */
