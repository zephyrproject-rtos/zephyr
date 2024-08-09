/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_INTERNAL_H_
#define ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_INTERNAL_H_

/* Return code used by syscon MUX to indicate to parents that it is using
 * the clock input, and therefore the clock cannot be gated.
 */
#define NXP_SYSCON_MUX_ERR_SAFEGATE -EIO

#endif /* ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_INTERNAL_H_ */
