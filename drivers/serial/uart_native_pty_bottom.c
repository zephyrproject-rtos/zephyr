/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _XOPEN_SOURCE
/* Note: This is used only for interaction with the host C library, and is therefore exempt of
 * coding guidelines rule A.4&5 which applies to the embedded code using embedded libraries
 */
#define _XOPEN_SOURCE 600

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <poll.h>
#include <nsi_tracing.h>

#define ERROR nsi_print_error_and_exit
#define WARN nsi_print_warning

/**
 * @brief Poll the device for input.
 *
 * @param in_f   Input file descriptor
 * @param p_char Pointer to character.
 * @param len    Maximum number of characters to read.
 *
 * @retval >0 Number of characters actually read
 * @retval -1 If no character was available to read
 * @retval -2 if the stdin is disconnected
 */
int np_uart_stdin_read_bottom(int in_f, unsigned char *p_char, int len)
{
	int n = -1;
	int ready;

	struct pollfd fds = {.fd = in_f, .events = POLLIN};

	ready = poll(&fds, 1, 0);

	if (ready == 0) {
		return -1;
	} else if (ready == -1) {
		ERROR("%s: Error on poll ()\n", __func__);
	}

	if (len == 0) {
		return 0;
	}

	n = read(in_f, p_char, len);

	if (n == 0) {
		/* Attempting to read > 0 but getting 0 characters back
		 * indicates we reached EOF
		 */
		return -2;
	} else {
		return n;
	}
}

/**
 * @brief Check if the output descriptor has something connected to the slave side
 *
 * @param fd file number
 *
 * @retval 0 Nothing connected yet
 * @retval 1 Something connected to the slave side
 */
int np_uart_slave_connected(int fd)
{
	struct pollfd pfd = { .fd = fd, .events = POLLHUP };
	int ret;

	ret = poll(&pfd, 1, 0);
	if (ret == -1) {
		int err = errno;
		/*
		 * Possible errors are:
		 *  * EINTR :A signal was received => ok
		 *  * EFAULT and EINVAL: parameters/programming error
		 *  * ENOMEM no RAM left
		 */
		if (err != EINTR) {
			ERROR("%s: unexpected error during poll, errno=%i,%s\n",
			      __func__, err, strerror(err));
		}
	}
	if (!(pfd.revents & POLLHUP)) {
		/* There is now a reader on the slave side */
		return 1;
	}
	return 0;
}

/**
 * Attempt to connect a terminal emulator to the slave side of the pty
 * If -attach_uart_cmd=<cmd> is provided as a command line option, <cmd> will be
 * used. Otherwise, the default command,
 * CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD, will be used instead
 */
static void attach_to_pty(const char *slave_pty, const char *auto_attach_cmd)
{
	char command[strlen(auto_attach_cmd) + strlen(slave_pty) + 1];

	sprintf(command, auto_attach_cmd, slave_pty);

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
int np_uart_open_pty(const char *uart_name, const char *auto_attach_cmd,
		     bool do_auto_attach, bool wait_pts)
{
	int master_pty;
	char *slave_pty_name;
	struct termios ter;
	int err_nbr;
	int ret;
	int flags;

	master_pty = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_pty == -1) {
		ERROR("Could not open a new PTY for the UART\n");
	}
	ret = grantpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not grant access to the slave PTY side (%i)\n",
			err_nbr);
	}
	ret = unlockpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not unlock the slave PTY side (%i)\n", err_nbr);
	}
	slave_pty_name = ptsname(master_pty);
	if (slave_pty_name == NULL) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Error getting slave PTY device name (%i)\n", err_nbr);
	}
	/* Set the master PTY as non blocking */
	flags = fcntl(master_pty, F_GETFL);
	if (flags == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not read the master PTY file status flags (%i)\n",
			err_nbr);
	}

	ret = fcntl(master_pty, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not set the master PTY as non-blocking (%i)\n",
			err_nbr);
	}

	(void) err_nbr;

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

	nsi_print_trace("%s connected to pseudotty: %s\n",
			  uart_name, slave_pty_name);

	if (wait_pts) {
		/*
		 * This trick sets the HUP flag on the pty master, making it
		 * possible to detect a client connection using poll.
		 * The connection of the client would cause the HUP flag to be
		 * cleared, and in turn set again at disconnect.
		 */
		ret = open(slave_pty_name, O_RDWR | O_NOCTTY);
		if (ret == -1) {
			err_nbr = errno;
			ERROR("%s: Could not open terminal from the slave side (%i,%s)\n",
				__func__, err_nbr, strerror(err_nbr));
		}
		ret = close(ret);
		if (ret == -1) {
			err_nbr = errno;
			ERROR("%s: Could not close terminal from the slave side (%i,%s)\n",
				__func__, err_nbr, strerror(err_nbr));
		}
	}
	if (do_auto_attach) {
		attach_to_pty(slave_pty_name, auto_attach_cmd);
	}

	return master_pty;
}

int np_uart_pty_get_stdin_fileno(void)
{
	return STDIN_FILENO;
}

int np_uart_pty_get_stdout_fileno(void)
{
	return STDOUT_FILENO;
}
