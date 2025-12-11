/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_PCC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_PCC_H_

/* NXP Kinetis Peripheral Clock Controller IP sources */
#define KINETIS_PCC_SRC_NONE_OR_EXT 0 /* Clock off or external clock is used */
#define KINETIS_PCC_SRC_SOSC_ASYNC  1 /* System Oscillator async clock */
#define KINETIS_PCC_SRC_SIRC_ASYNC  2 /* Slow IRC async clock */
#define KINETIS_PCC_SRC_FIRC_ASYNC  3 /* Fast IRC async clock */
#define KINETIS_PCC_SRC_SPLL_ASYNC  6 /* System PLL async clock */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_PCC_H_ */
