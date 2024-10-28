/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/net/socket.h>

/* Get size, in elements, of an array within a struct. */
#define STRUCT_MEMBER_ARRAY_SIZE(type, field) ARRAY_SIZE(((type *)0)->field)

/* Returns results in word_idx and bit_mask "output" params */
#define FD_SET_CALC_OFFSETS(set, word_idx, bit_mask) { \
	unsigned int b_idx = fd % (sizeof(set->bitset[0]) * 8); \
	word_idx = fd / (sizeof(set->bitset[0]) * 8); \
	bit_mask = 1 << b_idx; \
	}

int zvfs_poll_internal(struct zvfs_pollfd *fds, int nfds, k_timeout_t timeout);

void ZVFS_FD_ZERO(struct zvfs_fd_set *set)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(set->bitset); i++) {
		set->bitset[i] = 0U;
	}
}

int ZVFS_FD_ISSET(int fd, struct zvfs_fd_set *set)
{
	uint32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZVFS_FD_SETSIZE) {
		return 0;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	return (set->bitset[word_idx] & bit_mask) != 0U;
}

void ZVFS_FD_CLR(int fd, struct zvfs_fd_set *set)
{
	uint32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZVFS_FD_SETSIZE) {
		return;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	set->bitset[word_idx] &= ~bit_mask;
}

void ZVFS_FD_SET(int fd, struct zvfs_fd_set *set)
{
	uint32_t word_idx, bit_mask;

	if (fd < 0 || fd >= ZVFS_FD_SETSIZE) {
		return;
	}

	FD_SET_CALC_OFFSETS(set, word_idx, bit_mask);

	set->bitset[word_idx] |= bit_mask;
}

int z_impl_zvfs_select(int nfds, struct zvfs_fd_set *ZRESTRICT readfds,
		       struct zvfs_fd_set *ZRESTRICT writefds,
		       struct zvfs_fd_set *ZRESTRICT exceptfds,
		       const struct timespec *ZRESTRICT timeout, const void *ZRESTRICT sigmask)
{
	struct zvfs_pollfd pfds[CONFIG_ZVFS_POLL_MAX];
	k_timeout_t poll_timeout;
	int i, res;
	int num_pfds = 0;
	int num_selects = 0;
	int fd_no = 0;

	for (i = 0; i < STRUCT_MEMBER_ARRAY_SIZE(struct zvfs_fd_set, bitset); i++) {
		uint32_t bit_mask = 1U;
		uint32_t read_mask = 0U, write_mask = 0U, except_mask = 0U;
		uint32_t ored_mask;

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
			fd_no += sizeof(ored_mask) * 8;
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
					events |= ZVFS_POLLIN;
				}

				if (write_mask & bit_mask) {
					events |= ZVFS_POLLOUT;
				}

				if (except_mask & bit_mask) {
					events |= ZVFS_POLLPRI;
				}

				pfds[num_pfds].fd = fd_no;
				pfds[num_pfds++].events = events;
			}

			bit_mask <<= 1;
			fd_no++;
		} while (bit_mask != 0U);
	}

	if (timeout == NULL) {
		poll_timeout = K_FOREVER;
	} else {
		poll_timeout =
			K_USEC(timeout->tv_sec * USEC_PER_SEC + timeout->tv_nsec / NSEC_PER_USEC);
	}

	res = zvfs_poll_internal(pfds, num_pfds, poll_timeout);
	if (res == -1) {
		return -1;
	}

	if (readfds != NULL) {
		ZVFS_FD_ZERO(readfds);
	}

	if (writefds != NULL) {
		ZVFS_FD_ZERO(writefds);
	}

	if (exceptfds != NULL) {
		ZVFS_FD_ZERO(exceptfds);
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
		if (revents & ZVFS_POLLNVAL) {
			errno = EBADF;
			return -1;
		}

		if (revents & ZVFS_POLLIN) {
			if (readfds != NULL) {
				ZVFS_FD_SET(fd, readfds);
				num_selects++;
			}
		}

		if (revents & ZVFS_POLLOUT) {
			if (writefds != NULL) {
				ZVFS_FD_SET(fd, writefds);
				num_selects++;
			}
		}

		/* It's unclear if HUP/ERR belong here. At least not ignore
		 * them. Zephyr doesn't use HUP and barely use ERR so far.
		 */
		if (revents & (ZVFS_POLLPRI | ZVFS_POLLHUP | ZVFS_POLLERR)) {
			if (exceptfds != NULL) {
				ZVFS_FD_SET(fd, exceptfds);
				num_selects++;
			}

			if (writefds != NULL) {
				ZVFS_FD_SET(fd, writefds);
				num_selects++;
			}
		}

		res--;
	}

	return num_selects;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_select(int nfds, struct zvfs_fd_set *ZRESTRICT readfds,
			      struct zvfs_fd_set *ZRESTRICT writefds,
			      struct zvfs_fd_set *ZRESTRICT exceptfds,
			      const struct timespec *ZRESTRICT timeout,
			      const void *ZRESTRICT sigmask)
{
	struct zvfs_fd_set *readfds_copy = NULL, *writefds_copy = NULL, *exceptfds_copy = NULL;
	struct timespec *to = NULL;
	int ret = -1;

	if (readfds) {
		readfds_copy =
			k_usermode_alloc_from_copy((void *)readfds, sizeof(struct zvfs_fd_set));
		if (!readfds_copy) {
			errno = ENOMEM;
			goto out;
		}
	}

	if (writefds) {
		writefds_copy =
			k_usermode_alloc_from_copy((void *)writefds, sizeof(struct zvfs_fd_set));
		if (!writefds_copy) {
			errno = ENOMEM;
			goto out;
		}
	}

	if (exceptfds) {
		exceptfds_copy =
			k_usermode_alloc_from_copy((void *)exceptfds, sizeof(struct zvfs_fd_set));
		if (!exceptfds_copy) {
			errno = ENOMEM;
			goto out;
		}
	}

	if (timeout) {
		to = k_usermode_alloc_from_copy((void *)timeout, sizeof(*to));
		if (!to) {
			errno = ENOMEM;
			goto out;
		}
	}

	ret = z_impl_zvfs_select(nfds, readfds_copy, writefds_copy, exceptfds_copy, to, sigmask);

	if (ret >= 0) {
		if (readfds_copy) {
			k_usermode_to_copy((void *)readfds, readfds_copy,
					   sizeof(struct zvfs_fd_set));
		}

		if (writefds_copy) {
			k_usermode_to_copy((void *)writefds, writefds_copy,
					   sizeof(struct zvfs_fd_set));
		}

		if (exceptfds_copy) {
			k_usermode_to_copy((void *)exceptfds, exceptfds_copy,
					   sizeof(struct zvfs_fd_set));
		}
	}

out:
	k_free(to);
	k_free(readfds_copy);
	k_free(writefds_copy);
	k_free(exceptfds_copy);

	return ret;
}
#include <zephyr/syscalls/zvfs_select_mrsh.c>
#endif
