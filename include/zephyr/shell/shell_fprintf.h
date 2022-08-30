/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_FPRINTF_H__
#define SHELL_FPRINTF_H__

#include <zephyr/zephyr.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*shell_fprintf_fwrite)(const void *user_ctx,
				     const char *data,
				     size_t length);

struct shell_fprintf_control_block {
	size_t buffer_cnt;
	bool autoflush;
};
/**
 * @brief fprintf context
 */
struct shell_fprintf {
	uint8_t *buffer;
	size_t buffer_size;
	shell_fprintf_fwrite fwrite;
	const void *user_ctx;
	struct shell_fprintf_control_block *ctrl_blk;
};

/**
 * @brief Macro for defining shell_fprintf instance.
 *
 * @param _name		Instance name.
 * @param _user_ctx	Pointer to user data.
 * @param _buf		Pointer to output buffer
 * @param _size		Size of output buffer.
 * @param _autoflush	Indicator if buffer shall be automatically flush.
 * @param _fwrite	Pointer to function sending data stream.
 */
#define Z_SHELL_FPRINTF_DEFINE(_name, _user_ctx, _buf, _size,	\
			    _autoflush, _fwrite)		\
	static struct shell_fprintf_control_block		\
				_name##_shell_fprintf_ctx = {	\
		.autoflush = _autoflush,			\
		.buffer_cnt = 0					\
	};							\
	static const struct shell_fprintf _name = {		\
		.buffer = _buf,					\
		.buffer_size = _size,				\
		.fwrite = _fwrite,				\
		.user_ctx = _user_ctx,				\
		.ctrl_blk = &_name##_shell_fprintf_ctx		\
	}

/**
 * @brief fprintf like function which send formatted data stream to output.
 *
 * @param sh_fprintf	fprintf instance.
 * @param fmt		Format string.
 * @param args		List of parameters to print.
 */
void z_shell_fprintf_fmt(const struct shell_fprintf *sh_fprintf,
			 char const *fmt, va_list args);

/**
 * @brief function flushing data stored in io_buffer.
 *
 * @param sh_fprintf	fprintf instance
 */
void z_shell_fprintf_buffer_flush(const struct shell_fprintf *sh_fprintf);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_FPRINTF_H__ */
