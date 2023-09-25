/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_

#include "comparator.h"

/**
 * @brief nRF-specific comparator configuration flags
 * @defgroup comparator_interface_nrf nRF-specific comparator configuration flags
 * @ingroup comparator_interface
 * @{
 *
 * @name Configuration flags for COMP peripheral
 * @{
 */

#define NRF_COMP_POS_AIN0      0
#define NRF_COMP_POS_AIN1      1
#define NRF_COMP_POS_AIN2      2
#define NRF_COMP_POS_AIN3      3
#define NRF_COMP_POS_AIN4      4
#define NRF_COMP_POS_AIN5      5
#define NRF_COMP_POS_AIN6      6
#define NRF_COMP_POS_AIN7      7
#define NRF_COMP_POS_VDD_DIV2  8
#define NRF_COMP_POS_VDDH_DIV5 9

/* VIN- selection for differential mode. */
#define NRF_COMP_NEG_DIFF_AIN0    10
#define NRF_COMP_NEG_DIFF_AIN1    11
#define NRF_COMP_NEG_DIFF_AIN2    12
#define NRF_COMP_NEG_DIFF_AIN3    13
#define NRF_COMP_NEG_DIFF_AIN4    14
#define NRF_COMP_NEG_DIFF_AIN5    15
#define NRF_COMP_NEG_DIFF_AIN6    16
#define NRF_COMP_NEG_DIFF_AIN7    17
/* VIN- selection for single-ended mode. */
#define NRF_COMP_NEG_SE_INT_1V2   18
#define NRF_COMP_NEG_SE_INT_1V8   19
#define NRF_COMP_NEG_SE_INT_2V4   20
#define NRF_COMP_NEG_SE_VDD       21
#define NRF_COMP_NEG_SE_AREF_AIN0 22
#define NRF_COMP_NEG_SE_AREF_AIN1 23
#define NRF_COMP_NEG_SE_AREF_AIN2 24
#define NRF_COMP_NEG_SE_AREF_AIN3 25
#define NRF_COMP_NEG_SE_AREF_AIN4 26
#define NRF_COMP_NEG_SE_AREF_AIN5 27
#define NRF_COMP_NEG_SE_AREF_AIN6 28
#define NRF_COMP_NEG_SE_AREF_AIN7 29

#define NRF_COMP_FLAG_MODE_POS        COMPARATOR_FLAG_VENDOR_POS
#define NRF_COMP_FLAG_MODE_MASK       (0x3UL << NRF_COMP_FLAG_MODE_POS)
#define NRF_COMP_FLAG_MODE_LOW_POWER  (0UL << NRF_COMP_FLAG_MODE_POS)
#define NRF_COMP_FLAG_MODE_NORMAL     (1UL << NRF_COMP_FLAG_MODE_POS)
#define NRF_COMP_FLAG_MODE_HIGH_SPEED (2UL << NRF_COMP_FLAG_MODE_POS)

#define NRF_COMP_FLAG_DIFF_HYSTERESIS BIT(COMPARATOR_FLAG_VENDOR_POS + 2)

#define NRF_COMP_FLAG_SE_THDOWN_POS   (COMPARATOR_FLAG_VENDOR_POS + 3)
#define NRF_COMP_FLAG_SE_THDOWN_MASK  (0x3FUL << NRF_COMP_FLAG_SE_THDOWN_POS)
#define NRF_COMP_FLAG_SE_THDOWN(val)  ((val) << NRF_COMP_FLAG_SE_THDOWN_POS)

#define NRF_COMP_FLAG_SE_THUP_POS     (COMPARATOR_FLAG_VENDOR_POS + 9)
#define NRF_COMP_FLAG_SE_THUP_MASK    (0x3FUL << NRF_COMP_FLAG_SE_THUP_POS)
#define NRF_COMP_FLAG_SE_THUP(val)    ((val) << NRF_COMP_FLAG_SE_THUP_POS)

/**
 * @}
 *
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_ */
