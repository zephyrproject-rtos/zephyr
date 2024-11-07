/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MACROS_H_
#define _MACROS_H_

#include <errno.h>

/* Error check. If != 0, print err code and call _SysFatalErrorHandler in main.
 * For debug mode all LEDs are turned on in case of an error.
 */

#define PRINT_AND_OOPS(code)                                                                       \
	do {                                                                                       \
		LOG_ERR("ERR_CHK Err_code: [%d] @ line: %d\t", code, __LINE__);                    \
		k_oops();                                                                          \
	} while (0)

#define ERR_CHK(err_code)                                                                          \
	do {                                                                                       \
		if (err_code) {                                                                    \
			PRINT_AND_OOPS(err_code);                                                  \
		}                                                                                  \
	} while (0)

#define ERR_CHK_MSG(err_code, msg)                                                                 \
	do {                                                                                       \
		if (err_code) {                                                                    \
			LOG_ERR("%s", msg);                                                        \
			PRINT_AND_OOPS(err_code);                                                  \
		}                                                                                  \
	} while (0)

#if (defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_ANALYZER))

#define STACK_USAGE_PRINT(thread_name, p_thread)                                                   \
	do {                                                                                       \
		static uint64_t thread_ts;                                                         \
		size_t unused_space_in_thread_bytes;                                               \
		if (k_uptime_get() - thread_ts > CONFIG_PRINT_STACK_USAGE_MS) {                    \
			k_thread_stack_space_get(p_thread, &unused_space_in_thread_bytes);         \
			thread_ts = k_uptime_get();                                                \
			LOG_DBG("Unused space in %s thread: %d bytes", thread_name,                \
				unused_space_in_thread_bytes);                                     \
		}                                                                                  \
	} while (0)
#else
#define STACK_USAGE_PRINT(thread_name, p_stack)
#endif /* (defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_ANALYZER)) */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW "\x1B[0;33m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"

#define COLOR_RESET "\x1b[0m"

#define BIT_SET(REG, BIT) ((REG) |= (BIT))
#define BIT_CLEAR(REG, BIT) ((REG) &= ~(BIT))

#endif /* _MACROS_H_ */
