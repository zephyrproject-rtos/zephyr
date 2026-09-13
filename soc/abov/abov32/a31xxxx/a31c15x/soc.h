/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the A31C15X family processors.
 *
 * Based on reference manual:
 *   advanced ARM(r)-based 32-bit MCUs
 *
 */


#ifndef ZEPHYR_SOC_ABOV_ABOV32_A31XXXX_A31C15X_SOC_H_
#define ZEPHYR_SOC_ABOV_ABOV32_A31XXXX_A31C15X_SOC_H_

#ifndef _ASMLANGUAGE

/* CMSIS device header from the ABOV HAL (modules/hal/abov32). It supplies
 * __NVIC_PRIO_BITS, the IRQn_Type enum and the peripheral register structs.
 * Its own core_cm0plus.h/system_a31xxxx.h includes are compiled out under
 * Zephyr (see the __ZEPHYR__ guard in a31c15x.h) so they don't collide with
 * the CMSIS core Zephyr already provides via modules/cmsis_6.
 */
#include "a31c15x.h"

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_SOC_ABOV_ABOV32_A31XXXX_A31C15X_SOC_H_ */
