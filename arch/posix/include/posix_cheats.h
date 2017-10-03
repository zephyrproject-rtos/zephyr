/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
 * Header to be able to compile the Zephyr kernel on top of a POSIX OS
 */

#ifndef _POSIX_CHEATS_H
#define _POSIX_CHEATS_H

#ifdef CONFIG_ARCH_POSIX

#ifndef main
#define main(...) zephyr_app_main(__VA_ARGS__)
#endif

/* TODO: regarding the pthreads.h provided with Zephyr, in case somebody would
 * use it, we would need to rename all symbols here adding some prefix, we will
 * then ensure this header is included in pthreads.h
 */

#endif /*CONFIG_ARCH_POSIX*/

#endif
