/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_TYPES_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_TYPES_H_

/**
 * @brief BSD Sockets compatible API
 * @defgroup bsd_sockets BSD Sockets compatible API
 * @ingroup networking
 * @{
 */

#include <zephyr/types.h>

#ifdef CONFIG_POSIX_API
#ifdef __NEWLIB__
#include <sys/_timeval.h>
#else
#include <sys/types.h>
#endif /* __NEWLIB__ */
#endif /* CONFIG_POSIX_API */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_API
/* Rely on the underlying libc definition */
#ifdef __NEWLIB__
#define zsock_timeval timeval
#else
/* workaround for older Newlib 2.x, as it lacks sys/_timeval.h */
struct zsock_timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif /* __NEWLIB__ */
#else
struct zsock_timeval {
	/* Using longs, as many (?) implementations seem to use it. */
	long tv_sec;
	long tv_usec;
};
#endif /* CONFIG_POSIX_API */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_TYPES_H_ */
