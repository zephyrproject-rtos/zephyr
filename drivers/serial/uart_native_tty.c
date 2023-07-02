/**
 * @brief UART Driver for interacting with host serial ports
 *
 * @note  Driver can open and send characters to the host serial ports (such as /dev/ttyUSB0 or
 * /dev/ttyACM0). Only polling Uart API is implemented. Driver can be configured via devicetree,
 * command line options or at runtime.
 *
 * To learn more see Native TYY section at:
 * https://docs.zephyrproject.org/latest/boards/posix/native_posix/doc/index.html
 * or
 * ${ZEPHYR_BASE}/boards/posix/native_posix/doc/index.rst
 *
 * Copyright (c) 2023 Marko Sagadin
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "cmdline.h"
#include "posix_native_task.h"

#define WARN(msg, ...)  posix_print_warning(msg, ##__VA_ARGS__)
#define ERROR(msg, ...) posix_print_error_and_exit(msg, ##__VA_ARGS__)

#define DT_DRV_COMPAT zephyr_native_tty_uart

struct native_tty_data {
	/* File descriptor used for the tty device. */
	int fd;
	/* Absolute path to the tty device. */
	char *serial_port;
	/* Baudrate set from the command line. If UINT32_MAX, it was not set. */
	int cmd_baudrate;
	/* Serial port set from the command line. If NULL, it was not set. */
	char *cmd_serial_port;
};

struct native_tty_config {
	struct uart_config uart_config;
};

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
 *
 * @retval 0		If successful,
 * @retval -ENOTSUP	If requested baud rate is not supported.
 */
static inline int native_tty_baud_speed_set(struct termios *ter, int baudrate)
{
	for (int i = 0; i < ARRAY_SIZE(baudrate_lut); i++) {
		if (baudrate_lut[i].baudrate == baudrate) {
			cfsetospeed(ter, baudrate_lut[i].termios_baudrate);
			cfsetispeed(ter, baudrate_lut[i].termios_baudrate);
			return 0;
		}
	}
	return -ENOTSUP;
}

/**
 * @brief Set parity setting in the termios structure
 *
 * @param ter
 * @param parity
 *
 * @retval 0		If successful.
 * @retval -ENOTSUP	If requested parity is not supported.
 */
static inline int native_tty_baud_parity_set(struct termios *ter, enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		ter->c_cflag &= ~PARENB;
		break;
	case UART_CFG_PARITY_ODD:
		ter->c_cflag |= PARENB;
		ter->c_cflag |= PARODD;
		break;
	case UART_CFG_PARITY_EVEN:
		ter->c_cflag |= PARENB;
		ter->c_cflag &= ~PARODD;
		break;
	default:
		/* Parity options mark and space (UART_CFG_PARITY_MARK and UART_CFG_PARITY_SPACE)
		 * are not supported on this driver.
		 */
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Set the number of stop bits in the termios structure
 *
 * @param ter
 * @param stop_bits
 *
 * @retval 0		If successful.
 * @retval -ENOTSUP	If requested number of stop bits is not supported.
 */
static inline int native_tty_stop_bits_set(struct termios *ter,
					   enum uart_config_stop_bits stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		ter->c_cflag &= ~CSTOPB;
		break;
	case UART_CFG_STOP_BITS_2:
		ter->c_cflag |= CSTOPB;
		break;
	default:
		/* Anything else is not supported in termios. */
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Set the number of data bits in the termios structure
 *
 * @param ter
 * @param stop_bits
 *
 * @retval 0		If successful.
 * @retval -ENOTSUP	If requested number of data bits is not supported.
 */
static inline int native_tty_data_bits_set(struct termios *ter,
					   enum uart_config_data_bits data_bits)
{
	unsigned int data_bits_to_set;

	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		data_bits_to_set = CS5;
		break;
	case UART_CFG_DATA_BITS_6:
		data_bits_to_set = CS6;
		break;
	case UART_CFG_DATA_BITS_7:
		data_bits_to_set = CS7;
		break;
	case UART_CFG_DATA_BITS_8:
		data_bits_to_set = CS8;
		break;
	default:
		/* Anything else is not supported in termios */
		return -ENOTSUP;
	}

	/* Clear all bits that set the data size */
	ter->c_cflag &= ~CSIZE;
	ter->c_cflag |= data_bits_to_set;
	return 0;
}

/*
 * @brief Output a character towards the serial port
 *
 * @param dev		UART device structure.
 * @param out_char	Character to send.
 */
static void native_tty_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	struct native_tty_data *data = dev->data;

	int ret = write(data->fd, &out_char, 1);

	if (ret == -1) {
		ERROR("Could not write to %s, reason: %s\n", data->serial_port, strerror(errno));
	}
}

/**
 * @brief Poll the device for input.
 *
 * @param dev		UART device structure.
 * @param p_char	Pointer to a character.
 *
 * @retval 0	If a character arrived.
 * @retval -1	If no character was available to read.
 */
static int native_tty_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct native_tty_data *data = dev->data;

	return read(data->fd, p_char, 1) > 0 ? 0 : -1;
}

static int native_tty_configure(const struct device *dev, const struct uart_config *cfg)
{
	int rc, err;
	int fd = ((struct native_tty_data *)dev->data)->fd;

	/* Structure used to control properties of a serial port */
	struct termios ter;

	/* Read current terminal driver settings */
	rc = tcgetattr(fd, &ter);
	if (rc) {
		WARN("Could not read terminal driver settings\n");
		return rc;
	}

	native_tty_termios_defaults_set(&ter);

	rc = native_tty_baud_speed_set(&ter, cfg->baudrate);
	if (rc) {
		WARN("Could not set baudrate, as %d is not supported.\n", cfg->baudrate);
		return rc;
	}

	rc = native_tty_baud_parity_set(&ter, cfg->parity);
	if (rc) {
		WARN("Could not set parity.\n");
		return rc;
	}

	rc = native_tty_stop_bits_set(&ter, cfg->stop_bits);
	if (rc) {
		WARN("Could not set number of data bits.\n");
		return rc;
	}

	rc = native_tty_data_bits_set(&ter, cfg->data_bits);
	if (rc) {
		WARN("Could not set number of data bits.\n");
		return rc;
	}

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		WARN("Could not set flow control, any kind of hw flow control is not supported.\n");
		return -ENOTSUP;
	}

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
		return -EBADE;
	}

	/* Flush both input and output */
	rc = tcflush(fd, TCIOFLUSH);
	if (rc) {
		WARN("Could not flush serial port\n");
		return rc;
	}

	return 0;
}

static int native_tty_serial_init(const struct device *dev)
{
	struct native_tty_data *data = dev->data;
	struct uart_config uart_config = ((struct native_tty_config *)dev->config)->uart_config;

	/* Default value for cmd_serial_port is NULL, this is due to the set 's' type in command
	 * line opts. If it is anything else then it was configured via command line.
	 */
	if (data->cmd_serial_port) {
		data->serial_port = data->cmd_serial_port;
	}

	/* Default value for cmd_baudrate is UINT32_MAX, this is due to the set 'u' type in command
	 * line opts. If it is anything else then it was configured via command line.
	 */
	if (data->cmd_baudrate != UINT32_MAX) {
		uart_config.baudrate = data->cmd_baudrate;
	}

	/* Serial port needs to be set either in the devicetree or provided via command line opts,
	 * if that is not the case, then abort.
	 */
	if (!data->serial_port) {
		ERROR("%s: path to the serial port was not set.\n", dev->name);
	}

	/* Try to open a serial port as with read/write access, also prevent serial port
	 * from becoming the controlling terminal.
	 */
	data->fd = open(data->serial_port, O_RDWR | O_NOCTTY);
	if (data->fd < 0) {
		ERROR("%s: failed to open serial port %s, reason: %s\n", dev->name,
		      data->serial_port, strerror(errno));
	}

	if (native_tty_configure(dev, &uart_config)) {
		ERROR("%s: could not configure serial port %s\n", dev->name, data->serial_port);
	}

	posix_print_trace("%s connected to the serial port: %s\n", dev->name, data->serial_port);

	return 0;
}

static struct uart_driver_api native_tty_uart_driver_api = {
	.poll_out = native_tty_uart_poll_out,
	.poll_in = native_tty_uart_poll_in,
	.configure = native_tty_configure,
};

#define NATIVE_TTY_INSTANCE(inst)                                                                  \
	static const struct native_tty_config native_tty_##inst##_cfg = {                          \
		.uart_config =                                                                     \
			{                                                                          \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.baudrate = DT_INST_PROP(inst, current_speed),                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct native_tty_data native_tty_##inst##_data = {                                 \
		.serial_port = DT_INST_PROP_OR(inst, serial_port, NULL),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, native_tty_serial_init, NULL, &native_tty_##inst##_data,       \
			      &native_tty_##inst##_cfg, PRE_KERNEL_1, 55,                          \
			      &native_tty_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_INSTANCE);

#define INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))


#define NATIVE_TTY_COMMAND_LINE_OPTS(inst)                                                         \
	{                                                                                          \
		.option = INST_NAME(inst) "_port",						   \
		.name = "\"serial_port\"",                                                         \
		.type = 's',                                                                       \
		.dest = &native_tty_##inst##_data.cmd_serial_port,                                 \
		.descript = "Set a serial port for " INST_NAME(inst) " uart device, "		   \
		"overriding the one in devicetree.",						   \
	},                                                                                         \
	{											   \
		.option = INST_NAME(inst) "_baud",						   \
		.name = "baudrate",								   \
		.type = 'u',									   \
		.dest = &native_tty_##inst##_data.cmd_baudrate,					   \
		.descript = "Set a baudrate for " INST_NAME(inst) " device, overriding the "	   \
		"baudrate of " STRINGIFY(DT_INST_PROP(inst, current_speed))			   \
		"set in the devicetree.",							   \
	},

/**
 * @brief Adds command line options for setting serial port and baud rate for each uart device.
 */
static void native_tty_add_serial_options(void)
{
	static struct args_struct_t opts[] = {
		DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_COMMAND_LINE_OPTS) ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(opts);
}

#define NATIVE_TTY_CLEANUP(inst)                                                                   \
	if (native_tty_##inst##_data.fd != 0) {                                                    \
		close(native_tty_##inst##_data.fd);                                                \
	}

/**
 * @brief Cleans up any open serial ports on the exit.
 */
static void native_tty_cleanup_uart(void)
{
	DT_INST_FOREACH_STATUS_OKAY(NATIVE_TTY_CLEANUP);
}

NATIVE_TASK(native_tty_add_serial_options, PRE_BOOT_1, 11);
NATIVE_TASK(native_tty_cleanup_uart, ON_EXIT, 99);
