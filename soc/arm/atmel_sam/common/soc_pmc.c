/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Power Management Controller (PMC) module
 * HAL driver.
 */

#include <soc.h>
#include <sys/__assert.h>
#include <sys/util.h>

#if ID_PERIPH_COUNT > 74
#error "Unsupported SoC, update soc_pmc.c functions"
#endif

void soc_pmc_peripheral_enable(u32_t id)
{
	__ASSERT(id < ID_PERIPH_COUNT, "Invalid peripheral id");

	if (id < 32) {
		PMC->PMC_PCER0 = BIT(id);
#if ID_PERIPH_COUNT > 32
	} else if (id < 64) {
		PMC->PMC_PCER1 = BIT(id & 0x1F);
#endif
#if ID_PERIPH_COUNT > 64
	} else {
		/* Nothing to do, thes peripherals can't be enabled */
#endif
	}
}

void soc_pmc_peripheral_disable(u32_t id)
{
	__ASSERT(id < ID_PERIPH_COUNT, "Invalid peripheral id");

	if (id < 32) {
		PMC->PMC_PCDR0 = BIT(id);
#if ID_PERIPH_COUNT > 32
	} else if (id < 64) {
		PMC->PMC_PCDR1 = BIT(id & 0x1F);
#endif
#if ID_PERIPH_COUNT > 64
	} else {
		/* Nothing to do, these peripherals can't be disabled */
#endif
	}
}

u32_t soc_pmc_peripheral_is_enabled(u32_t id)
{
	__ASSERT(id < ID_PERIPH_COUNT, "Invalid peripheral id");

	if (id < 32) {
		return (PMC->PMC_PCSR0 & BIT(id)) != 0;
#if ID_PERIPH_COUNT > 32
	} else if (id < 64) {
		return (PMC->PMC_PCSR1 & BIT(id & 0x1F)) != 0;
#endif
#if ID_PERIPH_COUNT > 64
	} else {
		/* These peripherals are always enabled */
		return 1;
#endif
	}
	return 0;
}
