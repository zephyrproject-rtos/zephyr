/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_S32_S32K_SOC_H_
#define _NXP_S32_S32K_SOC_H_

#include <S32K344.h>

#if defined(CONFIG_CMSIS_RTOS_V2)
/*
 * The HAL is defining these symbols already. To avoid redefinitions,
 * let CMSIS RTOS wrapper define them.
 */
#undef TRUE
#undef FALSE
#endif

#endif /* _NXP_S32_S32K_SOC_H_ */
