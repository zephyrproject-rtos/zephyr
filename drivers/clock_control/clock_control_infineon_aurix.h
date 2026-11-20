/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_INFINEON_AURIX_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_INFINEON_AURIX_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* fBACK, the trimmed back-up clock, is 100 MHz on TC3x and TC4x */
#define AURIX_CCU_FBACK_HZ MHZ(100)

/* Valid divider values; the other encodings are reserved */
#define AURIX_CCU_DIV_VALID(d)                                                                     \
	(((d) >= 1 && (d) <= 6) || (d) == 8 || (d) == 10 || (d) == 12 || (d) == 15)

/* Divider property of instance 0; 1 if absent, for checks that test presence first */
#define AURIX_CCU_DT_DIV(prop) DT_INST_PROP_OR(0, prop, 1)

#define AURIX_CCU_CHECK_DIV(prop)                                                                  \
	BUILD_ASSERT(AURIX_CCU_DIV_VALID(AURIX_CCU_DT_DIV(prop)), #prop " uses a reserved value")

/* Busy wait on the STM; fstm_hz must not be below the current STM rate */
static inline void aurix_ccu_delay_us(uint32_t fstm_hz, uint32_t us)
{
	uint32_t cycles = (uint32_t)(((uint64_t)fstm_hz * us + USEC_PER_SEC - 1) / USEC_PER_SEC);
	uint32_t start = k_cycle_get_32();

	while ((k_cycle_get_32() - start) < cycles) {
	}
}

/**
 * @brief Wait until (reg & mask) == val, at most @p timeout_us
 *
 * @retval 0 Condition met
 * @retval -ETIMEDOUT Condition not met in time
 */
static inline int aurix_ccu_wait(mem_addr_t reg, uint32_t mask, uint32_t val, uint32_t fstm_hz,
				 uint32_t timeout_us)
{
	uint32_t cycles =
		(uint32_t)(((uint64_t)fstm_hz * timeout_us + USEC_PER_SEC - 1) / USEC_PER_SEC);
	uint32_t start = k_cycle_get_32();

	while ((sys_read32(reg) & mask) != val) {
		if ((k_cycle_get_32() - start) >= cycles) {
			return ((sys_read32(reg) & mask) == val) ? 0 : -ETIMEDOUT;
		}
	}

	return 0;
}

/* Replace a field in a register value */
#define AURIX_CCU_FIELD_SET(regval, mask, val) (((regval) & ~(mask)) | FIELD_PREP(mask, val))

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_INFINEON_AURIX_H_ */
