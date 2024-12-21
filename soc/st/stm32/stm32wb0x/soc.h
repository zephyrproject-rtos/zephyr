/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32WB0 family processors.
 *
 */


#ifndef _STM32WB0_SOC_H_
#define _STM32WB0_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32wb0x.h>

/** SMPS modes */
#define STM32WB0_SMPS_MODE_OFF		0
#define STM32WB0_SMPS_MODE_PRECHARGE	1
#define STM32WB0_SMPS_MODE_RUN		2

/** Active SMPS mode (provided here for usage in drivers) */
#define SMPS_MODE	_CONCAT(STM32WB0_SMPS_MODE_,			\
				DT_STRING_UNQUOTED(			\
					DT_INST(0, st_stm32wb0_pwr),	\
					smps_mode))

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WB0_SOC_H_ */
