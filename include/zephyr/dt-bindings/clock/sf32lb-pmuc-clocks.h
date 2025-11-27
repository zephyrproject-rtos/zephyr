/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SF32LB PMUC low-speed clock IDs.
 *
 * Clock subsystem IDs for the PMUC-managed low-speed clocks.
 */

#ifndef ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_PMUC_CLOCKS_H_
#define ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_PMUC_CLOCKS_H_

/**
 * @name SF32LB PMUC low-speed clock IDs
 * @{
 */

/** LRC10 internal RC oscillator clock ID. */
#define SF32LB_PMUC_CLOCK_LRC10 0
/** LRC32 internal RC oscillator clock ID. */
#define SF32LB_PMUC_CLOCK_LRC32 1
/** LXT32 external 32.768 kHz crystal oscillator clock ID. */
#define SF32LB_PMUC_CLOCK_LXT32 2

/** @} */

#endif /* ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_PMUC_CLOCKS_H_ */
