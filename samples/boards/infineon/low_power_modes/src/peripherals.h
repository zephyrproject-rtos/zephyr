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
 * Optional peripheral self-tests for the DUT application.
 *
 * Every peripheral is guarded by its devicetree presence inside peripherals.c,
 * so this sample can target other Infineon boards that expose a different subset
 * of peripherals.  A peripheral that is absent from the board's devicetree is
 * compiled out entirely and its self-test simply does nothing.
 */

/*
 * Configure every peripheral present on this board and run its baseline
 * self-test.  Call once before the low-power sequence starts.
 */
void peripherals_setup(void);

/*
 * Read the free-running counter used to observe clock gating across DeepSleep.
 * Returns 0 when no counter is present.
 */
uint32_t peripherals_counter_read(void);

/*
 * Re-run every present peripheral's self-test after a low-power wake and report
 * whether the free-running counter resumed.  @p phase is the mode name printed
 * with each result (for example "DeepSleep").
 */
void peripherals_test_after_wake(const char *phase);

#endif /* PERIPHERALS_H_ */
