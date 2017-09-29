/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <zephyr/types.h>

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


#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H__ */
