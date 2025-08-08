/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_native_pty_bottom.h"

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
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <nsi_tracing.h>
#include <sys/stat.h>
#include <libgen.h>

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
int np_uart_stdin_poll_in_bottom(int in_f, unsigned char *p_char, int len)
{
	if (feof(stdin)) {
		/*
		 * The stdinput is fed from a file which finished or the user
		 * pressed Ctrl+D
		 */
		return -2;
	}

	int n = -1;

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

	n = read(in_f, p_char, len);
	if ((n == -1) || (n == 0)) {
		return -1;
	}

	return n;
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
int np_uart_open_pty(const char *pty_symlink_path, const char *uart_name,
		     const char *auto_attach_cmd, bool do_auto_attach, bool wait_pts)
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

	if (pty_symlink_path != NULL) {
		int validation_result = validate_pty_symlink_path(pty_symlink_path);

		if (validation_result != 0) {
			/* Validation failed - cleanup PTY and report error */
			close(master_pty);
			report_pty_symlink_error(pty_symlink_path, validation_result);
			return -1; /* Error reported by report_pty_symlink_error, will exit */
		}

		/* Create symlink from custom path to auto-allocated PTY device */
		if (symlink(slave_pty_name, pty_symlink_path) != 0) {
			int symlink_errno = errno;

			close(master_pty);
			report_pty_symlink_error(pty_symlink_path, -symlink_errno);
			return -1; /* Error reported by report_pty_symlink_error, will exit */
		}

		nsi_print_trace("Created symlink: %s -> %s\n", pty_symlink_path, slave_pty_name);
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

/**
 * @brief Validate symlink path for PTY creation
 *
 * Performs comprehensive pre-flight validation for symlink creation including:
 * - Path format validation (no trailing slash)
 * - Parent directory existence and write permissions
 * - Symlink collision detection
 * - Platform-specific validation requirements
 *
 * @param path Symlink path to validate (NULL means no symlink creation)
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid path format (trailing slash, invalid characters)
 *         -EEXIST: Path already exists (collision detection)
 *         -ENOENT: Parent directory does not exist
 *         -EACCES: Permission denied for parent directory
 */
int validate_pty_symlink_path(const char *path)
{
	struct stat st;
	char *parent_dir_copy = NULL;
	char *parent_dir = NULL;
	int result = 0;

	if (!path || !path[0]) {
		return 0;
	}

	size_t len = strlen(path);

	if (len > 0 && path[len - 1] == '/') {
		return -EINVAL;
	}

	if (stat(path, &st) == 0) {
		return -EEXIST;
	}

	parent_dir_copy = strdup(path);
	if (!parent_dir_copy) {
		return -ENOMEM;
	}

	parent_dir = dirname(parent_dir_copy);
	if (!parent_dir) {
		free(parent_dir_copy);
		return -EINVAL;
	}

	if (stat(parent_dir, &st) != 0) {
		result = (errno == ENOENT) ? -ENOENT : -EACCES;
		goto cleanup;
	}

	if (!S_ISDIR(st.st_mode)) {
		result = -ENOTDIR;
		goto cleanup;
	}

	if (access(parent_dir, W_OK) != 0) {
		result = -EACCES;
		goto cleanup;
	}

cleanup:
	free(parent_dir_copy);
	return result;
}

/**
 * @brief Clean up a PTY symlink
 *
 * Removes a symlink created for a PTY device. This function is called
 * from the embedded context but runs in the host context where it can
 * access the host filesystem.
 *
 * @param symlink_path Path to the symlink to remove (NULL = no operation)
 * @return 0 on success, negative error code on failure
 */
int np_uart_cleanup_symlink(const char *symlink_path)
{
	if (!symlink_path) {
		return 0;
	}

	if (unlink(symlink_path) != 0) {
		int error = errno;

		WARN("Failed to remove symlink '%s': %s\n", symlink_path, strerror(error));
		return -error;
	}

	return 0;
}

/**
 * @brief Report symlink creation errors with actionable user guidance
 *
 * Provides comprehensive error reporting for symlink creation failures.
 * Each error includes specific user guidance for resolution.
 *
 * @param path Symlink path that failed
 * @param error Error code from validate_pty_symlink_path() or symlink()
 */
void report_pty_symlink_error(const char *path, int error)
{
	if (!path) {
		path = "<null>";
	}

	switch (error) {
	case -EEXIST:
		ERROR("Symlink path '%s' already exists. "
		      "Remove existing file or choose different path\n", path);
	case -ENOENT:
		ERROR("Parent directory for '%s' does not exist. "
		      "Create directory: mkdir -p $(dirname '%s')\n", path, path);
	case -ENOTDIR:
		ERROR("Parent path for '%s' exists but is not a directory. "
		      "Remove the file or choose different symlink path\n", path);
	case -EACCES:
		ERROR("Permission denied creating symlink '%s'. "
		      "Check directory permissions or choose writable location\n", path);
	case -EINVAL:
		ERROR("Invalid symlink path '%s'. "
		      "Path cannot end with '/' or contain invalid characters\n", path);
	case -ENOMEM:
		ERROR("Out of memory while validating symlink path '%s'\n", path);
	case -EAGAIN:
		ERROR("Failed to allocate PTY for symlink '%s'. "
		      "System may be out of PTY devices\n", path);
	case -EIO:
		ERROR("Failed to create symlink '%s'. "
		      "Filesystem may not support symlinks\n", path);
	default:
		ERROR("Symlink creation error for '%s': %s (errno=%d)\n",
		      path, strerror(-error), -error);
	}
}
