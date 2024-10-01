/*
 * Copyright (c) 2018, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_posix_uart

#include <stdbool.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "cmdline.h" /* native_posix command line options header */
#include "posix_native_task.h"
#include "uart_native_ptty_bottom.h"
#include "nsi_host_trampolines.h"

/*
 * UART driver for POSIX ARCH based boards.
 * It can support up to two UARTs.
 *
 * For the first UART:
 *
 * It can either be connected to the process STDIN+STDOUT
 * OR
 * to a dedicated pseudo terminal
 *
 * The 2nd option is the recommended one for interactive use, as the pseudo
 * terminal driver will be configured in "raw" mode, and will therefore behave
 * more like a real UART.
 *
 * When connected to its own pseudo terminal, it may also auto attach a terminal
 * emulator to it, if set so from command line.
 */

static int np_uart_stdin_poll_in(const struct device *dev,
				 unsigned char *p_char);
static int np_uart_tty_poll_in(const struct device *dev,
			       unsigned char *p_char);
static void np_uart_poll_out(const struct device *dev,
				      unsigned char out_char);

static bool auto_attach;
static bool wait_pts;
static char *auto_attach_cmd = CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD;

struct native_uart_status {
	int out_fd; /* File descriptor used for output */
	int in_fd; /* File descriptor used for input */
};

static struct native_uart_status native_uart_status_0;

static struct uart_driver_api np_uart_driver_api_0 = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_tty_poll_in,
};

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
static struct native_uart_status native_uart_status_1;

static struct uart_driver_api np_uart_driver_api_1 = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_tty_poll_in,
};
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

#define ERROR posix_print_error_and_exit
#define WARN posix_print_warning



/**
 * @brief Initialize the first native_posix serial port
 *
 * @param dev UART_0 device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_0_init(const struct device *dev)
{
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->data;

	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		int tty_fn = np_uart_open_ptty(dev->name, auto_attach_cmd, auto_attach, wait_pts);

		d->in_fd = tty_fn;
		d->out_fd = tty_fn;
		np_uart_driver_api_0.poll_in = np_uart_tty_poll_in;
	} else { /* NATIVE_UART_0_ON_STDINOUT */
		d->in_fd  = np_uart_ptty_get_stdin_fileno();
		d->out_fd = np_uart_ptty_get_stdout_fileno();
		np_uart_driver_api_0.poll_in = np_uart_stdin_poll_in;
	}

	return 0;
}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
/*
 * Initialize the 2nd UART port.
 * This port will be always attached to its own new pseudoterminal.
 */
static int np_uart_1_init(const struct device *dev)
{
	struct native_uart_status *d;
	int tty_fn;

	d = (struct native_uart_status *)dev->data;

	tty_fn = np_uart_open_ptty(dev->name, NULL, false, wait_pts);

	d->in_fd = tty_fn;
	d->out_fd = tty_fn;

	return 0;
}
#endif

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
static void np_uart_poll_out(const struct device *dev,
				      unsigned char out_char)
{
	int ret;
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (wait_pts) {

		while (1) {
			int rc = np_uart_slave_connected(d->out_fd);

			if (rc == 1) {
				break;
			}
			k_sleep(K_MSEC(100));
		}
	}

	/* The return value of write() cannot be ignored (there is a warning)
	 * but we do not need the return value for anything.
	 */
	ret = nsi_host_write(d->out_fd, &out_char, 1);
	(void) ret;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_stdin_poll_in(const struct device *dev,
				 unsigned char *p_char)
{
	int in_f = ((struct native_uart_status *)dev->data)->in_fd;
	static bool disconnected;
	int rc;

	if (disconnected == true) {
		return -1;
	}

	rc = np_uart_stdin_poll_in_bottom(in_f, p_char);
	if (rc == -2) {
		disconnected = true;
		return -1;
	}

	return rc;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_tty_poll_in(const struct device *dev,
			       unsigned char *p_char)
{
	int n = -1;
	int in_f = ((struct native_uart_status *)dev->data)->in_fd;

	n = nsi_host_read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}
	return 0;
}

DEVICE_DT_INST_DEFINE(0,
	    np_uart_0_init, NULL,
	    (void *)&native_uart_status_0, NULL,
	    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
	    &np_uart_driver_api_0);

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
DEVICE_DT_INST_DEFINE(1,
	    np_uart_1_init, NULL,
	    (void *)&native_uart_status_1, NULL,
	    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
	    &np_uart_driver_api_1);
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

static void auto_attach_cmd_cb(char *argv, int offset)
{
	auto_attach_cmd = &argv[offset];
	auto_attach = true;
}

static void np_add_uart_options(void)
{
	if (!IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		return;
	}

	static struct args_struct_t uart_options[] = {
	{
		.is_switch = true,
		.option = "attach_uart",
		.type = 'b',
		.dest = (void *)&auto_attach,
		.descript = "Automatically attach to the UART terminal"
	},
	{
		.option = "attach_uart_cmd",
		.name = "\"cmd\"",
		.type = 's',
		.call_when_found = auto_attach_cmd_cb,
		.descript = "Command used to automatically attach to the terminal (implies "
			    "auto_attach), by default: "
			    "'" CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD "'"
	},
	IF_ENABLED(CONFIG_UART_NATIVE_WAIT_PTS_READY_ENABLE, (
		{
		.is_switch = true,
		.option = "wait_uart",
		.type = 'b',
		.dest = (void *)&wait_pts,
		.descript = "Hold writes to the uart/pts until a client is connected/ready"
		},
	))
	ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(uart_options);
}

static void np_cleanup_uart(void)
{
	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		if (native_uart_status_0.in_fd != 0) {
			nsi_host_close(native_uart_status_0.in_fd);
		}
	}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
	if (native_uart_status_1.in_fd != 0) {
		nsi_host_close(native_uart_status_1.in_fd);
	}
#endif
}

NATIVE_TASK(np_add_uart_options, PRE_BOOT_1, 11);
NATIVE_TASK(np_cleanup_uart, ON_EXIT, 99);
