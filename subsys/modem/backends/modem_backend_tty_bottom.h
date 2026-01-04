/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODEM_BACKEND_UART_TTY_BOTTOM_
#define ZEPHYR_MODEM_BACKEND_UART_TTY_BOTTOM_

int modem_backend_tty_poll_bottom(int fd);
int modem_backend_tty_open_bottom(const char *tty_path);

#endif /* ZEPHYR_MODEM_BACKEND_UART_TTY_BOTTOM_ */
