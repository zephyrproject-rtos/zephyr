/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_LOG_BACKEND_H__
#define SHELL_LOG_BACKEND_H__

#include <zephyr/zephyr.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/atomic.h>
#ifdef __cplusplus
extern "C" {
#endif

extern const struct log_backend_api log_backend_shell_api;

/** @brief Shell log backend states. */
enum shell_log_backend_state {
	SHELL_LOG_BACKEND_UNINIT,
	SHELL_LOG_BACKEND_ENABLED,
	SHELL_LOG_BACKEND_DISABLED,
	SHELL_LOG_BACKEND_PANIC,
};

/** @brief Shell log backend control block (RW data). */
struct shell_log_backend_control_block {
	atomic_t dropped_cnt;
	enum shell_log_backend_state state;
};

/** @brief Shell log backend instance structure (RO data). */
struct shell_log_backend {
	const struct log_backend *backend;
	const struct log_output *log_output;
	struct shell_log_backend_control_block *control_block;
	uint32_t timeout;
	const struct mpsc_pbuf_buffer_config *mpsc_buffer_config;
	struct mpsc_pbuf_buffer *mpsc_buffer;
};

/** @brief Shell log backend message structure. */
struct shell_log_backend_msg {
	struct log_msg *msg;
	uint32_t timestamp;
};

/** @brief Prototype of function outputting processed data. */
int z_shell_log_backend_output_func(uint8_t *data, size_t length, void *ctx);

/** @def Z_SHELL_LOG_BACKEND_DEFINE
 *  @brief Macro for creating instance of shell log backend.
 *
 *  @param _name	Shell name.
 *  @param _buf		Output buffer.
 *  @param _size	Output buffer size.
 *  @param _queue_size	Log message queue size.
 *  @param _timeout	Timeout in milliseconds for pending on queue full.
 *			Message is dropped on timeout.
 */
/** @def Z_SHELL_LOG_BACKEND_PTR
 *  @brief Macro for retrieving pointer to the instance of shell log backend.
 *
 *  @param _name Shell name.
 */
#ifdef CONFIG_SHELL_LOG_BACKEND
#define Z_SHELL_LOG_BACKEND_DEFINE(_name, _buf, _size, _queue_size, _timeout) \
	LOG_BACKEND_DEFINE(_name##_backend, log_backend_shell_api, false); \
	LOG_OUTPUT_DEFINE(_name##_log_output, z_shell_log_backend_output_func,\
			  _buf, _size); \
	static struct shell_log_backend_control_block _name##_control_block; \
	static uint32_t __aligned(Z_LOG_MSG2_ALIGNMENT) _name##_buf[128]; \
	const struct mpsc_pbuf_buffer_config _name##_mpsc_buffer_config = { \
		.buf = _name##_buf, \
		.size = ARRAY_SIZE(_name##_buf), \
		.notify_drop = NULL, \
		.get_wlen = log_msg2_generic_get_wlen, \
		.flags = MPSC_PBUF_MODE_OVERWRITE, \
	}; \
	struct mpsc_pbuf_buffer _name##_mpsc_buffer; \
	static const struct shell_log_backend _name##_log_backend = { \
		.backend = &_name##_backend, \
		.log_output = &_name##_log_output, \
		.control_block = &_name##_control_block, \
		.timeout = _timeout, \
		.mpsc_buffer_config = &_name##_mpsc_buffer_config, \
		.mpsc_buffer = &_name##_mpsc_buffer, \
	}

#define Z_SHELL_LOG_BACKEND_PTR(_name) (&_name##_log_backend)
#else /* CONFIG_LOG */
#define Z_SHELL_LOG_BACKEND_DEFINE(_name, _buf, _size, _queue_size, _timeout)
#define Z_SHELL_LOG_BACKEND_PTR(_name) NULL
#endif /* CONFIG_LOG */

/** @brief Enable shell log backend.
 *
 * @param backend		Shell log backend instance.
 * @param ctx			Pointer to shell instance.
 * @param init_log_level	Initial log level set to all logging sources.
 */
void z_shell_log_backend_enable(const struct shell_log_backend *backend,
				void *ctx, uint32_t init_log_level);

/** @brief Disable shell log backend.
 *
 * @param backend Shell log backend instance.
 */
void z_shell_log_backend_disable(const struct shell_log_backend *backend);

/** @brief Trigger processing of one log entry.
 *
 * @param backend Shell log backend instance.
 *
 * @return True if message was processed, false if FIFO was empty
 */
bool z_shell_log_backend_process(const struct shell_log_backend *backend);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_LOG_BACKEND_H__ */
