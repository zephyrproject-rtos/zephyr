/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONSOLE_H_
#define ZEPHYR_INCLUDE_CONSOLE_H_

#include <zephyr/types.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize console_getchar()/putchar() calls.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getchar() function and buffered
 *  output using console_putchar() function. This function supersedes,
 *  and incompatible with, callback (push-style) console handling
 *  (via console_input_fn callback, etc.).
 *
 *  @return N/A
 */
void console_init(void);

/** @brief Get next char from console input buffer.
 *
 *  Return next input character from console. If no characters available,
 *  this function will block. This function is similar to ANSI C
 *  getchar() function and is intended to ease porting of existing
 *  software. Before this function can be used, console_getchar_init()
 *  should be called once. This function is incompatible with native
 *  Zephyr callback-based console input processing, shell subsystem,
 *  or console_getline().
 *
 *  @return A character read, including control characters.
 */
u8_t console_getchar(void);

/** @brief Output a char to console (buffered).
 *
 *  Puts a character into console output buffer. It will be sent
 *  to a console asynchronously, e.g. using an IRQ handler.
 *
 *  @return -1 on output buffer overflow, otherwise 0.
 */
int console_putchar(char c);

/** @brief Initialize console_getline() call.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getline() function. This function
 *  supersedes, and incompatible with, callback (push-style) console
 *  handling (via console_input_fn callback, etc.).
 *
 *  @return N/A
 */
void console_getline_init(void);

/** @brief Get next line from console input buffer.
 *
 *  Return next input line from console. Until full line is available,
 *  this function will block. This function is similar to ANSI C
 *  gets() function (except a line is returned in system-owned buffer,
 *  and system takes care of the buffer overflow checks) and is
 *  intended to ease porting of existing software. Before this function
 *  can be used, console_getline_init() should be called once. This
 *  function is incompatible with native Zephyr callback-based console
 *  input processing, shell subsystem, or console_getchar().
 *
 *  @return A pointer to a line read, not including EOL character(s).
 *          A line resides in a system-owned buffer, so an application
 *          should finish any processing of this line immediately
 *          after console_getline() call, or the buffer can be reused.
 */
char *console_getline(void);

/** @brief Initialize legacy fifo-based line input
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  This is a special-purpose function, it's recommended to use
 *  console_getchar() or console_getline() functions instead.
 *
 *  @param avail_queue k_fifo queue keeping available line buffers
 *  @param out_queue k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion callback for tab completion of entered commands
 */
void console_register_line_input(struct k_fifo *avail_queue,
				 struct k_fifo *out_queue,
				 u8_t (*completion)(char *str, u8_t len));


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONSOLE_H_ */
