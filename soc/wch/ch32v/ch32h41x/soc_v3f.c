/*
 * Copyright (c) 2026 Liu Changjie
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util.h>

/*
 * The V5F application image is linked at the real flash address 0x08010000
 * (see ch32h417_v5f.dtsi). The waker hands that same address to the PFIC, which
 * releases the V5F core with its PC set there.
 */
#define CH32H417_V5F_START_ADDR 0x08010000UL
#define CH32H417_PFIC_WAKEIP1   0xe000e724UL
#define CH32H417_PFIC_SCTLR     0xe000ed10UL

void soc_early_init_hook(void)
{
#ifdef CONFIG_SOC_CH32H417_BOOT_V5F
	volatile uint32_t *wakeip1 = (volatile uint32_t *)CH32H417_PFIC_WAKEIP1;
	volatile uint32_t *sctlr = (volatile uint32_t *)CH32H417_PFIC_SCTLR;

	/* Equivalent to the validated V3F waker: PFIC->WAKEIP[1] = V5F image start. */
	*wakeip1 = CH32H417_V5F_START_ADDR;

	/* PFIC->SCTLR SLEEPONEXIT so the boot core idles after waking the V5F. */
	*sctlr |= BIT(5);
#endif /* CONFIG_SOC_CH32H417_BOOT_V5F */
}
