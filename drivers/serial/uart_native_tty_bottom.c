/**
 * @brief "Bottom" of native tty uart driver
 *
 * Copyright (c) 2023 Marko Sagadin
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_native_tty_bottom.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <nsi_tracing.h>

#define WARN(...)  nsi_print_warning(__VA_ARGS__)
#define ERROR(...) nsi_print_error_and_exit(__VA_ARGS__)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

struct baudrate_termios_pair {
	int baudrate;
	speed_t termios_baudrate;
};

/**
 * @brief Lookup table for mapping the baud rate to the macro understood by termios.
 */
static const struct baudrate_termios_pair baudrate_lut[] = {
	{1200, B1200},       {1800, B1800},       {2400, B2400},       {4800, B4800},
	{9600, B9600},       {19200, B19200},     {38400, B38400},     {57600, B57600},
	{115200, B115200},   {230400, B230400},   {460800, B460800},   {500000, B500000},
	{576000, B576000},   {921600, B921600},   {1000000, B1000000}, {1152000, B1152000},
	{1500000, B1500000}, {2000000, B2000000}, {2500000, B2500000}, {3000000, B3000000},
	{3500000, B3500000}, {4000000, B4000000},
};

/**
 * @brief Set given termios to defaults appropriate for communicating with serial port devices.
 *
 * @param ter
 */
static inline void native_tty_termios_defaults_set(struct termios *ter)
{
	/* Set terminal in "serial" mode:
	 *  - Not canonical (no line input)
	 *  - No signal generation from Ctr+{C|Z..}
	 *  - No echoing
	 */
	ter->c_lflag &= ~(ICANON | ISIG | ECHO);

	/* No special interpretation of output bytes.
	 * No conversion of newline to carriage return/line feed.
	 */
	ter->c_oflag &= ~(OPOST | ONLCR);

	/* No software flow control. */
	ter->c_iflag &= ~(IXON | IXOFF | IXANY);

	/* No blocking, return immediately with what is available. */
	ter->c_cc[VMIN] = 0;
	ter->c_cc[VTIME] = 0;

	/* No special handling of bytes on receive. */
	ter->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	/* - Enable reading data and ignore control lines */
	ter->c_cflag |= CREAD | CLOCAL;
}

/**
 * @brief Set the baud rate speed in the termios structure
 *
 * @param ter
 * @param baudrate
 */
static inline void native_tty_baud_speed_set(struct termios *ter, int baudrate)
{
	for (int i = 0; i < ARRAY_SIZE(baudrate_lut); i++) {
		if (baudrate_lut[i].baudrate == baudrate) {
			cfsetospeed(ter, baudrate_lut[i].termios_baudrate);
			cfsetispeed(ter, baudrate_lut[i].termios_baudrate);
			return;
		}
	}
	ERROR("Could not set baudrate, as %d is not supported.\n", baudrate);
}

/**
 * @brief Set parity setting in the termios structure
 *
 * @param ter
 * @param parity
 */
static inline void native_tty_baud_parity_set(struct termios *ter,
					      enum native_tty_bottom_parity parity)
{
	switch (parity) {
	case NTB_PARITY_NONE:
		ter->c_cflag &= ~PARENB;
		break;
	case NTB_PARITY_ODD:
		ter->c_cflag |= PARENB;
		ter->c_cflag |= PARODD;
		break;
	case NTB_PARITY_EVEN:
		ter->c_cflag |= PARENB;
		ter->c_cflag &= ~PARODD;
		break;
	default:
		/* Parity options mark and space are not supported on this driver. */
		ERROR("Could not set parity.\n");
	}
}

/**
 * @brief Set the number of stop bits in the termios structure
 *
 * @param ter
 * @param stop_bits
 *
 */
static inline void native_tty_stop_bits_set(struct termios *ter,
					    enum native_tty_bottom_stop_bits stop_bits)
{
	switch (stop_bits) {
	case NTB_STOP_BITS_1:
		ter->c_cflag &= ~CSTOPB;
		break;
	case NTB_STOP_BITS_2:
		ter->c_cflag |= CSTOPB;
		break;
	default:
		/* Anything else is not supported in termios. */
		ERROR("Could not set number of data bits.\n");
	}
}

/**
 * @brief Set the number of data bits in the termios structure
 *
 * @param ter
 * @param stop_bits
 *
 */
static inline void native_tty_data_bits_set(struct termios *ter,
					    enum native_tty_bottom_data_bits data_bits)
{
	unsigned int data_bits_to_set = CS5;

	switch (data_bits) {
	case NTB_DATA_BITS_5:
		data_bits_to_set = CS5;
		break;
	case NTB_DATA_BITS_6:
		data_bits_to_set = CS6;
		break;
	case NTB_DATA_BITS_7:
		data_bits_to_set = CS7;
		break;
	case NTB_DATA_BITS_8:
		data_bits_to_set = CS8;
		break;
	default:
		/* Anything else is not supported in termios */
		ERROR("Could not set number of data bits.\n");
	}

	/* Clear all bits that set the data size */
	ter->c_cflag &= ~CSIZE;
	ter->c_cflag |= data_bits_to_set;
}

int native_tty_poll_bottom(int fd)
{
	struct pollfd pfd = { .fd = fd, .events = POLLIN };

	return poll(&pfd, 1, 0);
}

int native_tty_open_tty_bottom(const char *pathname)
{
	int fd = open(pathname, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		ERROR("Failed to open serial port %s, errno: %i\n", pathname, errno);
	}

	return fd;
}

int native_tty_configure_bottom(int fd, struct native_tty_bottom_cfg *cfg)
{
	int rc, err;
	/* Structure used to control properties of a serial port */
	struct termios ter;

	/* Read current terminal driver settings */
	rc = tcgetattr(fd, &ter);
	if (rc) {
		WARN("Could not read terminal driver settings\n");
		return rc;
	}

	native_tty_termios_defaults_set(&ter);

	native_tty_baud_speed_set(&ter, cfg->baudrate);
	native_tty_baud_parity_set(&ter, cfg->parity);
	native_tty_stop_bits_set(&ter, cfg->stop_bits);
	native_tty_data_bits_set(&ter, cfg->data_bits);

	cfg->flow_ctrl = NTB_FLOW_CTRL_NONE;

	rc = tcsetattr(fd, TCSANOW, &ter);
	if (rc) {
		err = errno;
		WARN("Could not set serial port settings, reason: %s\n", strerror(err));
		return err;
	}

	/* tcsetattr returns success if ANY of the requested changes were successfully carried out,
	 * not if ALL were. So we need to read back the settings and check if they are equal to the
	 * requested ones.
	 */
	struct termios read_ter;

	rc = tcgetattr(fd, &read_ter);
	if (rc) {
		err = errno;
		WARN("Could not read serial port settings, reason: %s\n", strerror(err));
		return err;
	}

	if (ter.c_cflag != read_ter.c_cflag || ter.c_iflag != read_ter.c_iflag ||
	    ter.c_oflag != read_ter.c_oflag || ter.c_lflag != read_ter.c_lflag ||
	    ter.c_line != read_ter.c_line || ter.c_ispeed != read_ter.c_ispeed ||
	    ter.c_ospeed != read_ter.c_ospeed || 0 != memcmp(ter.c_cc, read_ter.c_cc, NCCS)) {
		WARN("Read serial port settings do not match set ones.\n");
		return -1;
	}

	/* Flush both input and output */
	rc = tcflush(fd, TCIOFLUSH);
	if (rc) {
		WARN("Could not flush serial port\n");
		return rc;
	}

	return 0;
}
