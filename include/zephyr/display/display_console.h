/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Text console on the chosen display.
 *
 * A grid of character cells shown on the display selected by the
 * zephyr,display chosen node. The console keeps the text and interprets
 * the subset of VT100 control sequences used by the logging and shell
 * subsystems: cursor movement, line and screen erase, and SGR colours.
 * The renderer selected with Kconfig draws the cells.
 *
 * The console shows the data transmitted by the emulated UART selected by
 * the zephyr,console chosen node, and feeds that UART with the keys of
 * every input device. This API is the byte interface the attachment uses;
 * another transport can write to it the same way.
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_DISPLAY_CONSOLE_H_
#define ZEPHYR_INCLUDE_DISPLAY_DISPLAY_CONSOLE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Text console on the chosen display
 * @defgroup display_console Display console
 * @ingroup display_interface
 * @{
 */

/**
 * @brief Initialize the console on the chosen display.
 *
 * Safe to call more than once; later calls return the result of the
 * first. The display driver must already be initialized.
 *
 * @retval 0 Success, or already initialized.
 * @retval -ENODEV The display is not ready.
 * @retval -ENOTSUP The pixel format of the display is not supported.
 * @retval -EINVAL The configured font is unavailable or unsuitable.
 * @retval -ENOSPC The display or the pixel buffer is too small for the font.
 * @retval -errno Other error returned by the display driver.
 */
int display_console_init(void);

/**
 * @brief Check whether the console is initialized and usable.
 *
 * @retval true The console accepts output.
 * @retval false display_console_init() has not succeeded.
 */
bool display_console_ready(void);

/**
 * @brief Get the size of the text grid.
 *
 * @param rows Number of text rows.
 * @param cols Number of text columns.
 *
 * @retval 0 Success.
 * @retval -ENODEV The console is not initialized.
 */
int display_console_size(uint16_t *rows, uint16_t *cols);

/**
 * @brief Write bytes to the console.
 *
 * Updates the text grid and schedules a redraw. After a fatal error, with
 * the CFB renderer and logging enabled, a completed line is drawn before
 * the call returns. Can be called from any context. Bytes are dropped when
 * the console is not initialized.
 *
 * @param data Bytes to write.
 * @param len Number of bytes.
 */
void display_console_write(const uint8_t *data, size_t len);

/**
 * @brief Redraw the changed rows now, in the calling context.
 *
 * From interrupt context the redraw is scheduled instead. After a fatal
 * error, with the CFB renderer and logging enabled, the rows are drawn from
 * any context.
 */
void display_console_flush(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DISPLAY_DISPLAY_CONSOLE_H_ */
