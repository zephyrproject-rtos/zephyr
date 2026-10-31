/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Unix domain sockets run in the native simulator context. */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <nsi_hw_scheduler.h>
#include <nsi_tasks.h>
#include <nsi_tracing.h>

#include "bs_cmd_line.h"
#include "bs_dynargs.h"

#include "hci_virt_bottom.h"

#define DEFAULT_SOCKET_PATH "/tmp/hci0.sock"
#define BTSNOOP_DATALINK_HCI_H4 1002U
#define BTSNOOP_EPOCH_DELTA_US 0x00DCDDB30F2F8000ULL
#define BTSNOOP_FILE_HEADER_LEN 16U
#define BTSNOOP_RECORD_HEADER_LEN 24U
#define BTSNOOP_MAX_FRAME 2048U
#define BTSNOOP_FLAG_RECEIVED 1U
#define BTSNOOP_FLAG_COMMAND_EVENT 2U
#define H4_CMD 0x01U
#define H4_EVT 0x04U

static int listen_fd = -1;
static int client_fd = -1;
static int btsnoop_fd = -1;
static char *socket_path_arg;
static char *btsnoop_file_path_arg;
static char *bd_addr_arg;
static uint32_t btsnoop_drops;
/* Notify the receiver when another context disconnects the host. */
static bool disconnect_pending;

static void put_be32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)(value >> 24U);
	dst[1] = (uint8_t)(value >> 16U);
	dst[2] = (uint8_t)(value >> 8U);
	dst[3] = (uint8_t)value;
}

static void put_be64(uint8_t *dst, uint64_t value)
{
	put_be32(dst, (uint32_t)(value >> 32U));
	put_be32(dst + 4, (uint32_t)value);
}

static void close_fd(int *fd)
{
	if (*fd < 0) {
		return;
	}

	if (close(*fd) < 0) {
		nsi_print_warning("hci_virt: close: %s\n", strerror(errno));
	}
	*fd = -1;
}

static void disconnect_host(void)
{
	if (client_fd >= 0) {
		nsi_print_trace("hci_virt: Host disconnected\n");
		close_fd(&client_fd);
	}
}

static void set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		nsi_print_error_and_exit("hci_virt: fcntl: %s\n", strerror(errno));
	}
}

static void register_socket_args(void)
{
	static bs_args_struct_t opts[] = {
		{
			.option = "hci-socket",
			.name = "path",
			.type = 's',
			.dest = &socket_path_arg,
			.descript = "Unix socket path for HCI transport "
				    "(default: " DEFAULT_SOCKET_PATH ")",
		},
		{
			.option = "btsnoop-file",
			.name = "path",
			.type = 's',
			.dest = &btsnoop_file_path_arg,
			.descript = "File path for BTSnoop HCI capture "
				    "(disabled by default)",
		},
		{
			.option = "bd-addr",
			.name = "addr",
			.type = 's',
			.dest = &bd_addr_arg,
			.descript = "Controller public address in AA:BB:CC:DD:EE:FF format",
		},
		ARG_TABLE_ENDMARKER,
	};

	bs_add_extra_dynargs(opts);
}

NSI_TASK(register_socket_args, PRE_BOOT_1, 1);

static const char *get_path(void)
{
	return socket_path_arg != NULL && socket_path_arg[0] != '\0' ? socket_path_arg
								     : DEFAULT_SOCKET_PATH;
}

const char *hci_virt_bd_addr(void)
{
	return bd_addr_arg;
}

static bool btsnoop_enabled(void)
{
	return btsnoop_file_path_arg != NULL && btsnoop_file_path_arg[0] != '\0';
}

static int write_nonblocking_all(int fd, const uint8_t *buf, size_t len)
{
	size_t total = 0U;

	while (total < len) {
		ssize_t count = send(fd, buf + total, len - total, MSG_NOSIGNAL);

		if (count > 0) {
			total += (size_t)count;
			continue;
		}

		if (count < 0 && errno == EINTR) {
			continue;
		}

		return count < 0 ? -errno : -EIO;
	}

	return 0;
}

static void write_btsnoop_all(const uint8_t *buf, size_t len)
{
	size_t total = 0U;

	while (total < len) {
		ssize_t count = write(btsnoop_fd, buf + total, len - total);

		if (count > 0) {
			total += (size_t)count;
			continue;
		}

		if (count < 0 && errno == EINTR) {
			continue;
		}

		nsi_print_error_and_exit("hci_virt: BTSnoop write: %s\n",
					 strerror(count < 0 ? errno : EIO));
	}
}

static void open_btsnoop_file(void)
{
	uint8_t header[BTSNOOP_FILE_HEADER_LEN] = "btsnoop";

	if (!btsnoop_enabled()) {
		return;
	}

	/* Unbuffered writes preserve completed records on SIGKILL teardown. */
	btsnoop_fd = open(btsnoop_file_path_arg, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
			  0644);
	if (btsnoop_fd < 0) {
		nsi_print_error_and_exit("hci_virt: open BTSnoop file %s: %s\n",
					 btsnoop_file_path_arg, strerror(errno));
	}

	put_be32(&header[8], 1U);
	put_be32(&header[12], BTSNOOP_DATALINK_HCI_H4);

	write_btsnoop_all(header, sizeof(header));

	nsi_print_trace("hci_virt: BTSnoop file = %s\n", btsnoop_file_path_arg);
}

static int accept_client(int fd)
{
	int accepted = accept(fd, NULL, NULL);

	if (accepted >= 0) {
		set_nonblocking(accepted);
	} else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
		nsi_print_warning("hci_virt: accept: %s\n", strerror(errno));
	}

	return accepted;
}

static void open_socket(const char *path)
{
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	struct stat path_stat;

	if (strlen(path) >= sizeof(addr.sun_path)) {
		nsi_print_error_and_exit("hci_virt: socket path is too long: %s\n", path);
	}

	if (lstat(path, &path_stat) == 0) {
		if (!S_ISSOCK(path_stat.st_mode)) {
			nsi_print_error_and_exit(
				"hci_virt: refusing to remove non-socket path %s\n", path);
		}

		if (unlink(path) < 0) {
			nsi_print_error_and_exit("hci_virt: unlink %s: %s\n", path,
						 strerror(errno));
		}
	} else if (errno != ENOENT) {
		nsi_print_error_and_exit("hci_virt: lstat %s: %s\n", path, strerror(errno));
	}

	listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (listen_fd < 0) {
		nsi_print_error_and_exit("hci_virt: socket: %s\n", strerror(errno));
	}

	memcpy(addr.sun_path, path, strlen(path) + 1U);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		nsi_print_error_and_exit("hci_virt: bind %s: %s\n", path, strerror(errno));
	}

	if (listen(listen_fd, 1) < 0) {
		nsi_print_error_and_exit("hci_virt: listen: %s\n", strerror(errno));
	}

	set_nonblocking(listen_fd);
	nsi_print_trace("hci_virt: Unix socket = %s\n", path);
}

void hci_virt_open(void)
{
	if (btsnoop_enabled() && strcmp(get_path(), btsnoop_file_path_arg) == 0) {
		nsi_print_error_and_exit(
			"hci_virt: HCI socket and BTSnoop file need different paths\n");
	}

	open_socket(get_path());

	open_btsnoop_file();
}

bool hci_virt_poll(void)
{
	struct pollfd pfd = {.fd = client_fd, .events = POLLIN};
	int rc;

	if (disconnect_pending) {
		return true;
	}

	if (client_fd < 0) {
		client_fd = accept_client(listen_fd);
		return false;
	}

	rc = poll(&pfd, 1, 0);
	if (rc < 0) {
		if (errno != EINTR) {
			nsi_print_warning("hci_virt: poll: %s\n", strerror(errno));
			disconnect_host();
			disconnect_pending = true;
			return true;
		}
		return false;
	}

	return (pfd.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0;
}

int32_t hci_virt_read(void *buf, size_t len)
{
	ssize_t count;

	if (disconnect_pending) {
		disconnect_pending = false;
		return -ECONNRESET;
	}

	if (client_fd < 0) {
		return -ENOTCONN;
	}

	count = read(client_fd, buf, len);
	if (count > 0) {
		return (int32_t)count;
	}

	if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
		return 0;
	}

	disconnect_host();

	return -ECONNRESET;
}

int hci_virt_write(const void *buf, size_t len)
{
	int err;

	if (client_fd < 0) {
		return -ENOTCONN;
	}

	err = write_nonblocking_all(client_fd, buf, len);
	if (err != 0) {
		disconnect_host();
		disconnect_pending = true;
	}

	return err;
}

void hci_virt_btsnoop_capture(const void *buf, size_t len, bool tx)
{
	uint8_t record[BTSNOOP_RECORD_HEADER_LEN + BTSNOOP_MAX_FRAME];
	const uint8_t *payload = buf;
	uint32_t flags = tx ? 0U : BTSNOOP_FLAG_RECEIVED;

	if (btsnoop_fd < 0) {
		return;
	}

	if (len == 0U || len > BTSNOOP_MAX_FRAME) {
		btsnoop_drops++;
		return;
	}

	if (payload[0] == H4_CMD || payload[0] == H4_EVT) {
		flags |= BTSNOOP_FLAG_COMMAND_EVENT;
	}

	put_be32(&record[0], (uint32_t)len);
	put_be32(&record[4], (uint32_t)len);
	put_be32(&record[8], flags);
	put_be32(&record[12], btsnoop_drops);
	put_be64(&record[16], nsi_hws_get_time() + BTSNOOP_EPOCH_DELTA_US);
	memcpy(&record[BTSNOOP_RECORD_HEADER_LEN], payload, len);

	write_btsnoop_all(record, BTSNOOP_RECORD_HEADER_LEN + len);
}

static void cleanup_sockets(void)
{
	close_fd(&client_fd);
	close_fd(&btsnoop_fd);
	if (listen_fd >= 0) {
		close_fd(&listen_fd);
		if (unlink(get_path()) < 0 && errno != ENOENT) {
			nsi_print_warning("hci_virt: unlink: %s\n", strerror(errno));
		}
	}
}

NSI_TASK(cleanup_sockets, ON_EXIT_PRE, 100);
