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


#ifdef CONFIG_NEWLIB_LIBC

#include <newlib.h>

#ifdef __NEWLIB__
#include <sys/_timeval.h>
#else /* __NEWLIB__ */
#include <sys/types.h>
/* workaround for older Newlib 2.x, as it lacks sys/_timeval.h */
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif /* __NEWLIB__ */

#else /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_ARCH_POSIX
#include <bits/types/struct_timeval.h>
#else
#include <sys/_timeval.h>
#endif

#endif /* CONFIG_NEWLIB_LIBC */

#ifdef __cplusplus
extern "C" {
#endif

#define zsock_timeval timeval

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_TYPES_H_ */
