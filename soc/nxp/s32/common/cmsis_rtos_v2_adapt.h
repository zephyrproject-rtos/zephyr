/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_S32_COMMON_CMSIS_RTOS_V2_ADAPT_H_
#define ZEPHYR_SOC_ARM_NXP_S32_COMMON_CMSIS_RTOS_V2_ADAPT_H_

/*
 * The HAL is defining these symbols already. To avoid interference
 * between HAL and the CMSIS RTOS wrapper, redefine them under an enum.
 */
#undef TRUE
#undef FALSE

enum { FALSE, TRUE};

#endif /* ZEPHYR_SOC_ARM_NXP_S32_COMMON_CMSIS_RTOS_V2_ADAPT_H_ */
