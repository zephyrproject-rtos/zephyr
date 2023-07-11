/**
 * @brief "Bottom" of native tty uart driver
 *
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 *
 * Copyright (c) 2023 Marko Sagadin
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_SERIAL_UART_NATIVE_TTY_BOTTOM_H
#define DRIVERS_SERIAL_UART_NATIVE_TTY_BOTTOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Below enums are just differently namespaced copies of uart_config_* enums. Options that are not
 * supported on the host are not listed.
 */
enum native_tty_bottom_parity {
	NTB_PARITY_NONE,
	NTB_PARITY_ODD,
	NTB_PARITY_EVEN,
};

enum native_tty_bottom_stop_bits {
	NTB_STOP_BITS_1,
	NTB_STOP_BITS_2,
};

enum native_tty_bottom_data_bits {
	NTB_DATA_BITS_5,
	NTB_DATA_BITS_6,
	NTB_DATA_BITS_7,
	NTB_DATA_BITS_8,
};

enum native_tty_bottom_flow_control {
	NTB_FLOW_CTRL_NONE,
};

struct native_tty_bottom_cfg {
	uint32_t baudrate;
	enum native_tty_bottom_parity parity;
	enum native_tty_bottom_stop_bits stop_bits;
	enum native_tty_bottom_data_bits data_bits;
	enum native_tty_bottom_flow_control flow_ctrl;
};

/* Note: None of these functions are public interfaces. They are internal to the native tty driver.
 */

/**
 * @brief Opens tty port on the given pathname
 *
 * Returned file descriptor can be then passed to native_tty_configure_bottom to configure it.
 *
 * @param pathname
 *
 * @return file descriptor
 */
int native_tty_open_tty_bottom(const char *pathname);

/**
 * @brief Configure tty port
 *
 * @param fd	File descriptor of the tty port.
 * @param cfg	Configuration struct.
 *
 * @retval 0	if successful,
 * @retval -1	otherwise.
 */
int native_tty_configure_bottom(int fd, struct native_tty_bottom_cfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SERIAL_UART_NATIVE_TTY_BOTTOM_H */
