/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_NXP_COMMON_H_
#define _SOC_NXP_COMMON_H_

/* Default implementation of Wakeup Signal configuration.
 * This is used in case the platform specific soc.h file
 * does not have an implementation for these macros.
 */
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) do { } while (0)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) do { } while (0)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) do { } while (0)

#endif /* _SOC_COMMON_H_ */
