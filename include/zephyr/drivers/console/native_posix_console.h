/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_

#warning "This header is now deprecated and will be removed by v4.4. "\
	 "Use posix_arch_console.h instead."

/*
 * This header is left for compatibility with old applications
 * The console for native_posix is now provided by the posix_arch_console driver
 */

#include <posix_arch_console.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_ */
