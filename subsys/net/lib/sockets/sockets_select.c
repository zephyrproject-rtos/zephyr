/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/socket.h>

/* Get size, in elements, of an array within a struct. */
#define STRUCT_MEMBER_ARRAY_SIZE(type, field) ARRAY_SIZE(((type *)0)->field)

/* Returns results in word_idx and bit_mask "output" params */
#define FD_SET_CALC_OFFSETS(set, word_idx, bit_mask) { \
	unsigned int b_idx = fd % (sizeof(set->bitset[0]) * 8); \
	word_idx = fd / (sizeof(set->bitset[0]) * 8); \
	bit_mask = 1 << b_idx; \
	}

void ZSOCK_FD_ZERO(zsock_fd_set *set)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(set->bitset); i++) {
		set->bitset[i] = 0U;
	}
}

int ZSOCK_FD_ISSET(int fd, zsock_fd_set *set)
{
	u32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZSOCK_FD_SETSIZE) {
		return 0;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	return (set->bitset[word_idx] & bit_mask) != 0U;
}

void ZSOCK_FD_CLR(int fd, zsock_fd_set *set)
{
	u32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZSOCK_FD_SETSIZE) {
		return;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	set->bitset[word_idx] &= ~bit_mask;
}

void ZSOCK_FD_SET(int fd, zsock_fd_set *set)
{
	u32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZSOCK_FD_SETSIZE) {
		return;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	set->bitset[word_idx] |= bit_mask;
}

int zsock_select(int nfds, zsock_fd_set *readfds, zsock_fd_set *writefds,
		 zsock_fd_set *exceptfds, struct zsock_timeval *timeout)
{
	struct zsock_pollfd pfds[CONFIG_NET_SOCKETS_POLL_MAX];
	int i, res, poll_timeout;
	int num_pfds = 0;
	int num_selects = 0;
	int fd_no = 0;

	for (i = 0; i < STRUCT_MEMBER_ARRAY_SIZE(zsock_fd_set, bitset); i++) {
		u32_t bit_mask = 1U;
		u32_t read_mask = 0U, write_mask = 0U, except_mask = 0U;
		u32_t ored_mask;

		if (readfds != NULL) {
			read_mask = readfds->bitset[i];
		}

		if (writefds != NULL) {
			write_mask = writefds->bitset[i];
		}

		if (exceptfds != NULL) {
			except_mask = exceptfds->bitset[i];
		}

		ored_mask = read_mask | write_mask | except_mask;
		if (ored_mask == 0U) {
			continue;
		}

		do {
			if (ored_mask & bit_mask) {
				int events = 0;

				if (num_pfds >= ARRAY_SIZE(pfds)) {
					errno = ENOMEM;
					return -1;
				}

				if (read_mask & bit_mask) {
					events |= ZSOCK_POLLIN;
				}

				if (write_mask & bit_mask) {
					events |= ZSOCK_POLLOUT;
				}

				if (except_mask & bit_mask) {
					events |= ZSOCK_POLLPRI;
				}

				pfds[num_pfds].fd = fd_no;
				pfds[num_pfds++].events = events;
			}

			bit_mask <<= 1;
			fd_no++;
		} while (bit_mask != 0U);
	}

	poll_timeout = -1;
	if (timeout != NULL) {
		poll_timeout = timeout->tv_sec * 1000
			       + timeout->tv_usec / 1000;
	}

	res = zsock_poll(pfds, num_pfds, poll_timeout);
	if (res == -1) {
		return -1;
	}

	if (readfds != NULL) {
		ZSOCK_FD_ZERO(readfds);
	}

	if (writefds != NULL) {
		ZSOCK_FD_ZERO(writefds);
	}

	if (exceptfds != NULL) {
		ZSOCK_FD_ZERO(exceptfds);
	}

	for (i = 0; i < num_pfds && res > 0; i++) {
		short revents = pfds[i].revents;
		int fd = pfds[i].fd;

		if (revents == 0) {
			continue;
		}

		/* POSIX: "EBADF: One or more of the file descriptor sets
		 * specified a file descriptor that is not a valid open
		 * file descriptor."
		 * So, unlike poll(), a single invalid fd aborts the entire
		 * select().
		 */
		if (revents & ZSOCK_POLLNVAL) {
			errno = EBADF;
			return -1;
		}

		if (revents & ZSOCK_POLLIN) {
			if (readfds != NULL) {
				ZSOCK_FD_SET(fd, readfds);
				num_selects++;
			}
		}

		if (revents & ZSOCK_POLLOUT) {
			if (writefds != NULL) {
				ZSOCK_FD_SET(fd, writefds);
				num_selects++;
			}
		}

		/* It's unclear if HUP/ERR belong here. At least not ignore
		 * them. Zephyr doesn't use HUP and barely use ERR so far.
		 */
		if (revents & (ZSOCK_POLLPRI | ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
			if (exceptfds != NULL) {
				ZSOCK_FD_SET(fd, exceptfds);
				num_selects++;
			}
		}

		res--;
	}

	return num_selects;
}
