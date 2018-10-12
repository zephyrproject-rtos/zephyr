/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_MAX_LINE_LEN CONFIG_CONSOLE_INPUT_MAX_LINE_LEN

/** @brief Console input representation
 *
 * This struct is used to represent an input line from a console.
 * Recorded line must be NULL terminated.
 */
struct console_input {
	/** FIFO uses first 4 bytes itself, reserve space */
	int _unused;
	/** Whether this is an mcumgr command */
	u8_t is_mcumgr : 1;
	/** Buffer where the input line is recorded */
	char line[CONSOLE_MAX_LINE_LEN];
};

/** @brief Console input processing handler signature
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  @param avail k_fifo queue keeping available input slots
 *  @param lines k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion callback for tab completion of entered commands
 *
 *  @return N/A
 */
typedef void (*console_input_fn)(struct k_fifo *avail, struct k_fifo *lines,
				 u8_t (*completion)(char *str, u8_t len));

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_CONSOLE_H_ */
