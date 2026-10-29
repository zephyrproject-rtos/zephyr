/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * SLI_CPC_PANIC() of the CPC core, on top of k_panic(). The core expects
 * this header from its port. Upstream, the panic loops forever in a
 * critical section; here it goes through the Zephyr fatal error handler,
 * which flushes the logs and applies the system reboot policy.
 */

#ifndef SLI_CPC_PANIC_H
#define SLI_CPC_PANIC_H

#include <zephyr/kernel.h>

#include "sli_cpc_log.h"

#define SLI_CPC_PANIC(fmt, ...)                                                                    \
	do {                                                                                       \
		SLI_CPC_LOG_ERROR(fmt, ##__VA_ARGS__);                                             \
		k_panic();                                                                         \
	} while (0)

#endif /* SLI_CPC_PANIC_H */
