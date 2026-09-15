/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <stdint.h>

/*
 * Optional peripheral self-tests for the DUT. Each peripheral is guarded by its
 * devicetree presence in peripherals.c, so absent peripherals compile out and
 * this sample can target other Infineon boards.
 */

/* Configure every present peripheral and run its baseline self-test. Call once
 * before the low-power sequence.
 */
void peripherals_setup(void);

/* Read the free-running counter used to observe clock gating; 0 if absent. */
uint32_t peripherals_counter_read(void);

/* Re-run every present peripheral's self-test after a wake. @p phase is the mode
 * name printed with each result.
 */
void peripherals_test_after_wake(const char *phase);

#endif /* PERIPHERALS_H_ */
