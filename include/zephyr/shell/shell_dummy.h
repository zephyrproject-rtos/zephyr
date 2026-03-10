/*
 * Shell backend used for testing
 *
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_DUMMY_H_
#define ZEPHYR_INCLUDE_SHELL_DUMMY_H_

#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_dummy_transport_api;

struct shell_dummy {
	bool initialized;

	/** current number of bytes in output buffer (0 if no output) */
	size_t len;

	/** output buffer to collect shell output */
	char buf[CONFIG_SHELL_BACKEND_DUMMY_BUF_SIZE];

	/** current number of bytes in input buffer */
	size_t input_len;

	/** current read position in input buffer */
	size_t input_pos;

	/** input buffer for simulating user input */
	char input_buf[CONFIG_SHELL_BACKEND_DUMMY_BUF_SIZE];
};

#define SHELL_DUMMY_DEFINE(_name)					\
	static struct shell_dummy _name##_shell_dummy;			\
	struct shell_transport _name = {				\
		.api = &shell_dummy_transport_api,			\
		.ctx = (struct shell_dummy *)&_name##_shell_dummy	\
	}

/**
 * @brief This function shall not be used directly. It provides pointer to shell
 *	  dummy backend instance.
 *
 * Function returns pointer to the shell dummy instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_dummy_get_ptr(void);

/**
 * @brief Returns the buffered output in the shell and resets the pointer
 *
 * The returned data is always followed by a nul character at position *sizep
 *
 * @param sh	Shell pointer
 * @param sizep	Returns size of data in shell buffer
 * @returns pointer to buffer containing shell output
 */
const char *shell_backend_dummy_get_output(const struct shell *sh,
					   size_t *sizep);

/**
 * @brief Clears the output buffer in the shell backend.
 *
 * @param sh	Shell pointer
 */
void shell_backend_dummy_clear_output(const struct shell *sh);

/**
 * @brief Push input data to the dummy shell backend.
 *
 * This function queues input data that will be returned by subsequent
 * read operations. Useful for testing shell input handling.
 *
 * @param sh	Shell pointer
 * @param data	Input data to push
 * @param len	Length of input data
 * @returns 0 on success, -ENOMEM if buffer is full
 */
int shell_backend_dummy_push_input(const struct shell *sh, const char *data, size_t len);

/**
 * @brief Clear the input buffer in the shell backend.
 *
 * @param sh	Shell pointer
 */
void shell_backend_dummy_clear_input(const struct shell *sh);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_DUMMY_H_ */
