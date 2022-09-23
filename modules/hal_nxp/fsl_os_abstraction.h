/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FSL_OS_ABSTRACTION__
#define __FSL_OS_ABSTRACTION__

#include <zephyr/irq.h>

/* enter critical macros */
#define OSA_SR_ALLOC() int osa_current_sr
#define OSA_ENTER_CRITICAL() osa_current_sr = irq_lock()
#define OSA_EXIT_CRITICAL() irq_unlock(osa_current_sr)

#endif /* __FSL_OS_ABSTRACTION__ */
