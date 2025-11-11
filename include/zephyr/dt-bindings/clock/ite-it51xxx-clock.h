/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IT51XXX_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IT51XXX_CLOCK_H_

/* Clock control */
#define IT51XXX_ECPM_CGCTRL2R_OFF 0x02
#define IT51XXX_ECPM_CGCTRL3R_OFF 0x05
#define IT51XXX_ECPM_CGCTRL4R_OFF 0x09

/* Clock PLL frequency */
#define PLL_18400_KHZ 0
#define PLL_32300_KHZ 1
#define PLL_64500_KHZ 2
#define PLL_48000_KHZ 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IT51XXX_CLOCK_H_ */
