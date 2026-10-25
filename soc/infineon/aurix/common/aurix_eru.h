/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AURIX external request unit (ERU) helpers
 *
 * An ERU input channel selects one of its request inputs (ERS), detects
 * edges on it (ETL) and sends a trigger pulse to an output channel (OGU).
 * OGU0 to OGU3 (TC3x) or OGU0 to OGU7 (TC4x) raise the service requests
 * SRC_SCUERUy. The helpers configure an output channel to raise its service
 * request on every trigger, without pattern detection.
 *
 * TC3x: two input channels share EICRi and two output channels share IGCRj;
 * both are Safety ENDINIT protected (TC3xx UM v2.0.0, 9.5.8; TC37x appendix,
 * SCU registers). TC4x: one EICRx and one IGCRy per channel, CPU ENDINIT
 * protected (TC4Dx UM v1.1, 36.4.39 to 36.4.42).
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ERU_H_
#define ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ERU_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "aurix_endinit.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AURIX_ERU_EICR_FEN  BIT(8)
#define AURIX_ERU_EICR_REN  BIT(9)
#define AURIX_ERU_EICR_LDEN BIT(10)
#define AURIX_ERU_EICR_SEL  GENMASK(6, 4)
#define AURIX_ERU_EICR_ONP  GENMASK(14, 12)

/* IGP = 01: the output channel raises its service request on every trigger */
#define AURIX_ERU_IGCR_IGP      GENMASK(15, 14)
#define AURIX_ERU_IGCR_IGP_TRIG 1U

#if defined(CONFIG_SOC_SERIES_TC3X)

#define AURIX_ERU_BASE    0xF0036000UL
#define AURIX_ERU_EICR(x) (AURIX_ERU_BASE + 0x210UL + ((x) / 2U) * 4U)
#define AURIX_ERU_FMR     (AURIX_ERU_BASE + 0x224UL)
#define AURIX_ERU_IGCR(y) (AURIX_ERU_BASE + 0x22CUL + ((y) / 2U) * 4U)
/* Channel 2i uses bits 15:0, channel 2i + 1 bits 31:16 */
#define AURIX_ERU_SHIFT(n) (((n) % 2U) * 16U)

/* EIEN enables the trigger pulse of the channel */
#define AURIX_ERU_EICR_TRIG BIT(11)

#define AURIX_ERU_OUTPUTS 4U

#define aurix_eru_unlock() aurix_endinit_safety_unlock()
#define aurix_eru_lock(k)  aurix_endinit_safety_lock(k)

#elif defined(CONFIG_SOC_SERIES_TC4X)

#define AURIX_ERU_BASE     0xF0024000UL
#define AURIX_ERU_EICR(x)  (AURIX_ERU_BASE + 0x154UL + (x) * 4U)
#define AURIX_ERU_FMR      (AURIX_ERU_BASE + 0x178UL)
#define AURIX_ERU_IGCR(y)  (AURIX_ERU_BASE + 0x17CUL + (y) * 4U)
#define AURIX_ERU_SHIFT(n) 0U

/* TPEN enables the trigger pulse; EIEN clocks the input synchronization */
#define AURIX_ERU_EICR_TRIG   BIT(11)
#define AURIX_ERU_EICR_EIEN   BIT(0)
#define AURIX_ERU_EICR_EIEN_P BIT(1)

#define AURIX_ERU_OUTPUTS 8U

#define aurix_eru_unlock() aurix_endinit_cpu_unlock()
#define aurix_eru_lock(k)  aurix_endinit_cpu_lock(k)

#endif

#define AURIX_ERU_INPUTS 8U

static inline void aurix_eru_modify(mem_addr_t reg, unsigned int n, uint32_t clear, uint32_t set)
{
	uint32_t shift = AURIX_ERU_SHIFT(n);

	sys_write32((sys_read32(reg) & ~(clear << shift)) | (set << shift), reg);
}

/**
 * @brief Clear the event flag of an input channel
 *
 * @param channel Input channel
 */
static inline void aurix_eru_clear(uint8_t channel)
{
	sys_write32(BIT(16U + channel), AURIX_ERU_FMR);
}

/**
 * @brief Route an input channel to an output channel
 *
 * Select request input @p input of input channel @p channel, trigger on the
 * selected edges and raise the service request of output channel @p output.
 * With neither edge selected the channel is disabled.
 *
 * @param channel Input channel
 * @param input Request input of the channel (0 is input A)
 * @param output Output channel
 * @param rising Trigger on rising edges
 * @param falling Trigger on falling edges
 */
static inline void aurix_eru_configure(uint8_t channel, uint8_t input, uint8_t output, bool rising,
				       bool falling)
{
	uint32_t eicr_mask = AURIX_ERU_EICR_SEL | AURIX_ERU_EICR_ONP | AURIX_ERU_EICR_FEN |
			     AURIX_ERU_EICR_REN | AURIX_ERU_EICR_LDEN | AURIX_ERU_EICR_TRIG;
	uint32_t eicr = FIELD_PREP(AURIX_ERU_EICR_SEL, input) |
			FIELD_PREP(AURIX_ERU_EICR_ONP, output);
	unsigned int key;

	if (rising) {
		eicr |= AURIX_ERU_EICR_REN;
	}
	if (falling) {
		eicr |= AURIX_ERU_EICR_FEN;
	}

	key = aurix_eru_unlock();

#if defined(CONFIG_SOC_SERIES_TC4X)
	/* Enabling the synchronization can look like an edge */
	eicr_mask |= AURIX_ERU_EICR_EIEN | AURIX_ERU_EICR_EIEN_P;
	aurix_eru_modify(AURIX_ERU_EICR(channel), channel, eicr_mask,
			 eicr | AURIX_ERU_EICR_EIEN | AURIX_ERU_EICR_EIEN_P);
#else
	aurix_eru_modify(AURIX_ERU_EICR(channel), channel, eicr_mask, eicr);
#endif
	aurix_eru_clear(channel);

	if (rising || falling) {
		aurix_eru_modify(AURIX_ERU_IGCR(output), output, AURIX_ERU_IGCR_IGP,
				 FIELD_PREP(AURIX_ERU_IGCR_IGP, AURIX_ERU_IGCR_IGP_TRIG));
		aurix_eru_modify(AURIX_ERU_EICR(channel), channel, 0U, AURIX_ERU_EICR_TRIG);
	}

	aurix_eru_lock(key);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_AURIX_COMMON_AURIX_ERU_H_ */
