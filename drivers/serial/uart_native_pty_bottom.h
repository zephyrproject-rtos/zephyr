/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of native PTY UART driver
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 */

#ifndef DRIVERS_SERIAL_UART_NATIVE_PTTY_BOTTOM_H
#define DRIVERS_SERIAL_UART_NATIVE_PTTY_BOTTOM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note: None of these functions are public interfaces. But internal to the native ptty driver */

int np_uart_stdin_poll_in_bottom(int in_f, unsigned char *p_char);
int np_uart_slave_connected(int fd);
int np_uart_open_pty(const char *uart_name, const char *auto_attach_cmd,
		     bool do_auto_attach, bool wait_pts);
int np_uart_pty_get_stdin_fileno(void);
int np_uart_pty_get_stdout_fileno(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SERIAL_UART_NATIVE_PTTY_BOTTOM_H */
