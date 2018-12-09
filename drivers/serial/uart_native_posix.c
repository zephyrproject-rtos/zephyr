/*
 * Copyright (c) 2018, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

#include "uart.h"
#include "cmdline.h" /* native_posix command line options header */
#include "soc.h"

/*
 * UART driver for POSIX ARCH based boards.
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

static int np_uart_stdin_poll_in(struct device *dev, unsigned char *p_char);
static int np_uart_tty_poll_in(struct device *dev, unsigned char *p_char);
static void np_uart_poll_out(struct device *dev,
				      unsigned char out_char);

static bool auto_attach;
static const char default_cmd[] = CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD;
static char *auto_attach_cmd;

static struct uart_driver_api np_uart_driver_api = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_tty_poll_in,
};

struct native_uart_status {
	int out_fd; /* File descriptor used for output */
	int in_fd; /* File descriptor used for input */
};

#define ERROR posix_print_error_and_exit
#define WARN posix_print_warning

/**
 * Attempt to connect a terminal emulator to the slave side of the pty
 * If -attach_uart_cmd=<cmd> is provided as a command line option, <cmd> will be
 * used. Otherwise, the default command,
 * CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD, will be used instead
 */
static void attach_to_tty(const char *slave_tty)
{
	if (auto_attach_cmd == NULL) {
		auto_attach_cmd = (char *)default_cmd;
	}
	char command[strlen(auto_attach_cmd) + strlen(slave_tty)];

	sprintf(command, auto_attach_cmd, slave_tty);

	int ret = system(command);

	if (ret != 0) {
		WARN("Could not attach to the UART with \"%s\"\n", command);
		WARN("The command returned %i\n", WEXITSTATUS(ret));
	}
}

/**
 * Attempt to allocate and open a new pseudoterminal
 *
 * Returns the file descriptor of the master side
 * If auto_attach was set, it will also attempt to connect a new terminal
 * emulator to its slave side.
 */
static int open_tty(void)
{
	int master_pty;
	char *slave_pty_name;
	struct termios ter;
	struct winsize win;
	int err_nbr;
	int ret;

	win.ws_col = 80;
	win.ws_row = 24;

	master_pty = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_pty == -1) {
		ERROR("Could not open a new TTY for the UART\n");
	}
	ret = grantpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not grant access to the slave PTY side (%i)\n",
			errno);
	}
	ret = unlockpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not unlock the slave PTY side (%i)\n", errno);
	}
	slave_pty_name = ptsname(master_pty);
	if (slave_pty_name == NULL) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Error getting slave PTY device name (%i)\n", errno);
	}
	/* Set the master PTY as non blocking */
	fcntl(master_pty, F_SETFL,  fcntl(master_pty, F_GETFL) | O_NONBLOCK);

	/*
	 * Set terminal in "raw" mode:
	 *  Not canonical (no line input)
	 *  No signal generation from Ctr+{C|Z..}
	 *  No echoing, no input or output processing
	 *  No replacing of NL or CR
	 *  No flow control
	 */
	ret = tcgetattr(master_pty, &ter);
	if (ret == -1) {
		ERROR("Could not read terminal driver settings\n");
	}
	ter.c_cc[VMIN] = 0;
	ter.c_cc[VTIME] = 0;
	ter.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
	ter.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK
			 | ISTRIP | IXON | PARMRK);
	ter.c_oflag &= ~OPOST;
	ret = tcsetattr(master_pty, TCSANOW, &ter);
	if (ret == -1) {
		ERROR("Could not change terminal driver settings\n");
	}

	posix_print_trace("UART connected to pseudotty: %s\n", slave_pty_name);

	if (auto_attach) {
		attach_to_tty(slave_pty_name);
	}

	return master_pty;
}

/**
 * @brief Initialize the native_posix serial port
 *
 * @param dev UART device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_init(struct device *dev)
{
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->driver_data;

	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		int tty_fn = open_tty();

		d->in_fd = tty_fn;
		d->out_fd = tty_fn;
		np_uart_driver_api.poll_in = np_uart_tty_poll_in;
	} else { /* NATIVE_UART_0_ON_STDINOUT */
		d->in_fd  = STDIN_FILENO;
		d->out_fd = STDOUT_FILENO;
		np_uart_driver_api.poll_in = np_uart_stdin_poll_in;

		if (isatty(STDIN_FILENO)) {
			WARN("The UART driver has been configured to map to the"
			     " process stdin&out (NATIVE_UART_0_ON_STDINOUT), "
			     "but stdin seems to be left attached to the shell."
			     " This will most likely NOT behave as you want it "
			     "to. This option is NOT meant for interactive use "
			     "but for piping/feeding from/to files to the UART"
			     );
		}
	}

	return 0;
}

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
static void np_uart_poll_out(struct device *dev,
				      unsigned char out_char)
{
	int ret;
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->driver_data;
	ret = write(d->out_fd, &out_char, 1);

	if (ret != 1) {
		WARN("%s: a character could not be output\n", __func__);
	}
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
static int np_uart_stdin_poll_in(struct device *dev, unsigned char *p_char)
{
	static bool disconnected;

	if (disconnected || feof(stdin)) {
		/*
		 * The stdinput is fed from a file which finished or the user
		 * pressed Crtl+D
		 */
		disconnected = true;
		return -1;
	}

	int n = -1;
	int in_f = ((struct native_uart_status *)dev->driver_data)->in_fd;

	int ready;
	fd_set readfds;
	static struct timeval timeout; /* just zero */

	FD_ZERO(&readfds);
	FD_SET(in_f, &readfds);

	ready = select(in_f+1, &readfds, NULL, NULL, &timeout);

	if (ready == 0) {
		return -1;
	} else if (ready == -1) {
		ERROR("%s: Error on select ()\n", __func__);
	}

	n = read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}

	return 0;
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
static int np_uart_tty_poll_in(struct device *dev, unsigned char *p_char)
{
	int n = -1;
	int in_f = ((struct native_uart_status *)dev->driver_data)->in_fd;

	n = read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}
	return 0;
}

static struct native_uart_status native_uart_status_0;

DEVICE_AND_API_INIT(uart_native_posix0,
	    CONFIG_UART_NATIVE_POSIX_PORT_0_NAME, &np_uart_init,
	    (void *)&native_uart_status_0, NULL,
	    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	    &np_uart_driver_api);

static void np_add_uart_options(void)
{
	if (!IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		return;
	}

	static struct args_struct_t uart_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{false, false, true,
		"attach_uart", "", 'b',
		(void *)&auto_attach, NULL,
		"Automatically attach to the UART terminal"},
		{false, false, false,
		"attach_uart_cmd", "\"cmd\"", 's',
		(void *)&auto_attach_cmd, NULL,
		"Command used to automatically attach to the terminal, by "
		"default: '" CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD "'"},

		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(uart_options);
}

static void np_cleanup_uart(void)
{
	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		if (native_uart_status_0.in_fd != 0) {
			close(native_uart_status_0.in_fd);
		}
	}
}

NATIVE_TASK(np_add_uart_options, PRE_BOOT_1, 11);
NATIVE_TASK(np_cleanup_uart, ON_EXIT, 99);
