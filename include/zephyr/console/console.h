/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONSOLE_CONSOLE_H_
#define ZEPHYR_INCLUDE_CONSOLE_CONSOLE_H_

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Console API
 * @defgroup console_api Console API
 * @ingroup os_services
 * @{
 */

/** @brief Initialize console device.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getchar() function and buffered
 *  output using console_putchar() function. This function supersedes,
 *  and incompatible with, callback (push-style) console handling
 *  (via console_input_fn callback, etc.).
 *
 * @return 0 on success, error code (<0) otherwise
 */
int console_init(void);

/**
 * @brief Read data from console.
 *
 * @param dummy ignored, present to follow read() prototype
 * @param buf buffer to read data to
 * @param size maximum number of bytes to read
 * @return >0, number of actually read bytes (can be less than size param)
 *         =0, in case of EOF
 *         <0, in case of error (e.g. -EAGAIN if timeout expired). errno
 *             variable is also set.
 */
k_ssize_t console_read(void *dummy, void *buf, size_t size);

/**
 * @brief Write data to console.
 *
 * @param dummy ignored, present to follow write() prototype
 * @param buf buffer to write data to
 * @param size maximum number of bytes to write
 * @return =>0, number of actually written bytes (can be less than size param)
 *         <0, in case of error (e.g. -EAGAIN if timeout expired). errno
 *             variable is also set.
 */
k_ssize_t console_write(void *dummy, const void *buf, size_t size);

/** @brief Get next char from console input buffer.
 *
 *  Return next input character from console. If no characters available,
 *  this function will block. This function is similar to ANSI C
 *  getchar() function and is intended to ease porting of existing
 *  software. Before this function can be used, console_init()
 *  should be called once. This function is incompatible with native
 *  Zephyr callback-based console input processing, shell subsystem,
 *  or console_getline().
 *
 *  @return 0-255: a character read, including control characters.
 *          <0: error occurred.
 */
int console_getchar(void);

/** @brief Output a char to console (buffered).
 *
 *  Puts a character into console output buffer. It will be sent
 *  to a console asynchronously, e.g. using an IRQ handler.
 *
 *  @return <0 on error, otherwise 0.
 */
int console_putchar(char c);

/** @brief Initialize console_getline() call.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getline() function. This function
 *  supersedes, and incompatible with, callback (push-style) console
 *  handling (via console_input_fn callback, etc.).
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

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CONSOLE_CONSOLE_H_ */
