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
 * @brief Check for available input on tty file descriptor
 *
 * @param fd
 *
 * @retval 1 if data is available
 * @retval 0 if data is not available, or poll() was interrupted by a signal
 * @retval -NSI_ERRNO_MID_EIO if the port hung up or failed
 * @retval -NSI_ERRNO_MID_EBADF if @p fd is not an open file
 * @retval <0 the host's errno of a failed poll(), negated, as an NSI_ERRNO_MID_* value
 */
int native_tty_poll_bottom(int fd);

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

/**
 * @brief Turn the tty driver's low latency mode on or off.
 *
 * In low latency mode, the driver delivers received data at once. USB serial
 * adapters otherwise hold a partial packet until a latency timer expires.
 *
 * @param fd		File descriptor of the tty port.
 * @param enable	Turn low latency on when non-zero, off otherwise.
 *
 * @retval 0	if low latency was off before the call,
 * @retval 1	if low latency was on before the call,
 * @retval <0	the host's errno of the failed ioctl, negated, as an NSI_ERRNO_MID_* value.
 */
int native_tty_set_low_latency_bottom(int fd, int enable);

/**
 * @brief Find the tty of a USB serial adapter by vendor and product ID.
 *
 * Tries the pairs in order and returns the first tty under @p tty_class whose
 * USB device matches one; among ttys of the same pair, the lowest name wins,
 * numbers compared by value, so ttyUSB2 comes before ttyUSB10.
 *
 * @param tty_class	Directory listing the ttys, normally "/sys/class/tty".
 * @param ids		Vendor and product ID pairs.
 * @param pairs		Number of pairs in @p ids.
 * @param path		Receives "/dev/" and the tty name.
 * @param size		Size of @p path.
 *
 * @retval 0				if a tty was found,
 * @retval -NSI_ERRNO_MID_ENODEV	if no tty matches any pair,
 * @retval -NSI_ERRNO_MID_ENAMETOOLONG	if @p path is too small for the tty path,
 * @retval <0				the host's errno of failing to read @p tty_class, negated,
 *					as an NSI_ERRNO_MID_* value.
 */
int native_tty_find_usb_bottom(const char *tty_class, const uint32_t *ids, int pairs, char *path,
			       long size);

/**
 * @brief Read bottom tty configuration
 *
 * @param fd
 * @param cfg
 *
 * @return 0 on success, negative value on error
 */
int native_tty_read_bottom_cfg(int fd, struct native_tty_bottom_cfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SERIAL_UART_NATIVE_TTY_BOTTOM_H */
