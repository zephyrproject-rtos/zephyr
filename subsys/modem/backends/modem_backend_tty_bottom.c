/*
 * Copyright (c) 2022 Trackunit Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <poll.h>

int modem_backend_tty_poll_bottom(int fd)
{
	struct pollfd pd = { .fd = fd, .events = POLLIN };
	int ret;

	ret = poll(&pd, 1, 0);

	if ((ret > 0) && !(pd.revents & POLLIN)) {
		/* we are only interested in POLLIN events */
		ret = 0;
	}

	return ret;
}

int modem_backend_tty_open_bottom(const char *tty_path)
{
	return open(tty_path, (O_RDWR | O_NONBLOCK), 0644);
}
