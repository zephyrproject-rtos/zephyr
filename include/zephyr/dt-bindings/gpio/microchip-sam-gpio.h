/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_

/*
 * By default the gpio driver will enable slew rate control for
 * each GPIO being used.
 * Use this flag to disable the enable operation
 */
#define SAM_GPIO_DIS_SLEWRATE		(1U << 8)
/*
 * With this flag, IFEN and IFSCEN will be enabled simultaneously
 */
#define SAM_GPIO_DEBOUNCE		(1U << 9)
/*
 * The Schmitt Trigger is enabled after chip reset
 * Use this flag to disable it
 */
#define SAM_GPIO_DIS_SCHMIT		(1U << 10)

/*
 * Use these flags to select the drive strength
 * Otherwise the SAM_GPIO_DRVSTR_DEFAULT will be used
 */
#define SAM_GPIO_DRVSTR_POS		(11U)
#define SAM_GPIO_DRVSTR_MASK		(0x3U << SAM_GPIO_DRVSTR_POS)
#define SAM_GPIO_DRVSTR_DEFAULT		(0U << SAM_GPIO_DRVSTR_POS)
#define SAM_GPIO_DRVSTR_LOW		(1U << SAM_GPIO_DRVSTR_POS)
#define SAM_GPIO_DRVSTR_MED		(2U << SAM_GPIO_DRVSTR_POS)
#define SAM_GPIO_DRVSTR_HI		(3U << SAM_GPIO_DRVSTR_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_ */
