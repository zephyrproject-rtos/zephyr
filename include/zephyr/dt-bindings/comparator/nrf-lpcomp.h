/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_LPCOMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_LPCOMP_H_

#include "comparator.h"

/**
 * @name Configuration flags for LPCOMP peripheral
 * @ingroup comparator_interface_nrf
 * @{
 */

/** Positive input specification. */
#define NRF_LPCOMP_POS_AIN0 0
#define NRF_LPCOMP_POS_AIN1 1
#define NRF_LPCOMP_POS_AIN2 2
#define NRF_LPCOMP_POS_AIN3 3
#define NRF_LPCOMP_POS_AIN4 4
#define NRF_LPCOMP_POS_AIN5 5
#define NRF_LPCOMP_POS_AIN6 6
#define NRF_LPCOMP_POS_AIN7 7

/** Negative input (reference) specification. */
#define NRF_LPCOMP_NEG_VDD_1_8   10
#define NRF_LPCOMP_NEG_VDD_2_8   11
#define NRF_LPCOMP_NEG_VDD_3_8   12
#define NRF_LPCOMP_NEG_VDD_4_8   13
#define NRF_LPCOMP_NEG_VDD_5_8   14
#define NRF_LPCOMP_NEG_VDD_6_8   15
#define NRF_LPCOMP_NEG_VDD_7_8   16
#define NRF_LPCOMP_NEG_VDD_1_16  18
#define NRF_LPCOMP_NEG_VDD_3_16  19
#define NRF_LPCOMP_NEG_VDD_5_16  20
#define NRF_LPCOMP_NEG_VDD_7_16  21
#define NRF_LPCOMP_NEG_VDD_9_16  22
#define NRF_LPCOMP_NEG_VDD_11_16 23
#define NRF_LPCOMP_NEG_VDD_13_16 24
#define NRF_LPCOMP_NEG_VDD_15_16 25
#define NRF_LPCOMP_NEG_AREF_AIN0 17
#define NRF_LPCOMP_NEG_AREF_AIN1 26

/** Enable hysteresis on the negative (reference) input. */
#define NRF_LPCOMP_FLAG_ENABLE_HYSTERESIS  BIT(COMPARATOR_FLAG_VENDOR_POS)

/** @cond INTERNAL_HIDDEN */
#define NRF_LPCOMP_FLAG_WAKE_ON_POS        (COMPARATOR_FLAG_VENDOR_POS + 1)
#define NRF_LPCOMP_FLAG_WAKE_ON_MASK       (0x3UL << NRF_LPCOMP_FLAG_WAKE_ON_POS)
/** @endcond */
#define NRF_LPCOMP_FLAG_WAKE_ON_BELOW_ONLY BIT(0 + NRF_LPCOMP_FLAG_WAKE_ON_POS)
#define NRF_LPCOMP_FLAG_WAKE_ON_ABOVE_ONLY BIT(1 + NRF_LPCOMP_FLAG_WAKE_ON_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_LPCOMP_H_ */
