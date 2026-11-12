/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

/*
 * Clear the whole SRAM so that the ECC check bits are initialized.
 * The loop bounds stay in registers because the stack lives in the
 * SRAM that is being cleared.
 */
void soc_reset_hook(void)
{
#if defined(__ICCARM__)
	register uint32_t r0 = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	register uint32_t r1 =
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#else
	register uint32_t r0 __asm__("r0") = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	register uint32_t r1 __asm__("r1") =
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#endif

	for (; r0 < r1; r0 += 4) {
		*(volatile uint32_t *)r0 = 0;
	}
}

#if defined(CONFIG_CLOCK_CONTROL_GD32_PLL)

/*
 * Software delay copied from the HAL SystemInit to guard Vcore
 * during the AHB prescaler walk-down.
 */
static void gd32f527_soft_delay(uint32_t time)
{
	for (volatile uint32_t i = 0U; i < (time * 10U); i++) {
	}
}

/*
 * Walk the AHB prescaler down to 1/16 before switching off PLL, so
 * the core is not running at full speed while Vcore settles.
 */
static void gd32f527_ahb_walk_down(void)
{
	uint32_t reg;

	reg = RCU_CFG0;
	reg = (reg & ~RCU_CFG0_AHBPSC) | RCU_AHB_CKSYS_DIV2;
	RCU_CFG0 = reg;
	gd32f527_soft_delay(20U);

	reg = RCU_CFG0;
	reg = (reg & ~RCU_CFG0_AHBPSC) | RCU_AHB_CKSYS_DIV4;
	RCU_CFG0 = reg;
	gd32f527_soft_delay(20U);

	reg = RCU_CFG0;
	reg = (reg & ~RCU_CFG0_AHBPSC) | RCU_AHB_CKSYS_DIV8;
	RCU_CFG0 = reg;
	gd32f527_soft_delay(20U);

	reg = RCU_CFG0;
	reg = (reg & ~RCU_CFG0_AHBPSC) | RCU_AHB_CKSYS_DIV16;
	RCU_CFG0 = reg;
	gd32f527_soft_delay(20U);
}

/*
 * Reset the RCU to the default reset state (IRC16M as system clock),
 * mirroring the non-clock part of SystemInit. The PLL driver then brings
 * up the target clock tree from this state.
 */
static void gd32f527_rcu_reset(void)
{
	uint32_t timeout = 0U;
	uint32_t stab_flag = 0U;

	/* Grant full access to CP10 and CP11 so the FPU is usable */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1U)
	SCB->CPACR |= (3UL << 10 * 2) | (3UL << 11 * 2);
#endif

	/* Enable IRC16M and wait until it is stable. */
	RCU_CTL |= RCU_CTL_IRC16MEN;
	do {
		timeout++;
		stab_flag = (RCU_CTL & RCU_CTL_IRC16MSTB);
	} while ((0U == stab_flag) && (IRC16M_STARTUP_TIMEOUT != timeout));
	if (0U == stab_flag) {
		/* Clock unstable: system cannot proceed. Hang for debugger. */
		while (1) {
		}
	}

	/* Walk AHB down if the system clock currently runs on PLL. */
	if ((RCU_CFG0 & RCU_CFG0_SCSS) == RCU_SCSS_PLLP) {
		gd32f527_ahb_walk_down();
	}

	RCU_CFG0 &= ~RCU_CFG0_SCS;
	gd32f527_soft_delay(200U);

	/* Disable PLL, clock monitor and HXTAL. */
	RCU_CTL &= ~(RCU_CTL_PLLEN | RCU_CTL_CKMEN | RCU_CTL_HXTALEN);
	RCU_CTL &= ~RCU_CTL_HXTALBPS;

	/* Reset CFG0 to its default value */
	RCU_CFG0 = 0x00000000U;

	/* Wait until IRC16M is selected as the system clock. */
	timeout = 0U;
	while (RCU_SCSS_IRC16M != (RCU_CFG0 & RCU_CFG0_SCSS)) {
		if (++timeout == IRC16M_STARTUP_TIMEOUT) {
			/* IRC16M not selected: system cannot proceed. Hang. */
			while (1) {
			}
		}
	}

	/* Restore the RCU_PLL reset value */
	RCU_PLL = 0x24003010U;

	/* Disable all clock interrupts. */
	RCU_INT = 0x00000000U;
}

#endif /* CONFIG_CLOCK_CONTROL_GD32_PLL */

#if !defined(CONFIG_CLOCK_CONTROL_GD32_PLL)
/* HAL SystemInit() always sets up a 200 MHz HCLK, so DTS must match */
BUILD_ASSERT(DT_PROP(DT_NODELABEL(rcu), clock_frequency) == MHZ(200),
	     "rcu clock-frequency must be 200 MHz when SystemInit() sets the clocks");

/* SystemInit() PLL settings assume the HAL default HXTAL_VALUE */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hxtal))
BUILD_ASSERT(DT_PROP(DT_NODELABEL(hxtal), clock_frequency) == HXTAL_VALUE,
	     "hxtal clock-frequency must match HXTAL_VALUE when SystemInit() sets the clocks");
#endif
#endif

void soc_early_init_hook(void)
{
#if defined(CONFIG_CLOCK_CONTROL_GD32_PLL)
	/*
	 * Reset the clock tree to its default state; the PLL driver performs
	 * the actual system clock configuration from the devicetree.
	 */
	gd32f527_rcu_reset();

	/* IRC16M is the system clock until the PLL driver reconfigures it */
	SystemCoreClock = IRC16M_VALUE;
#else
	/* Without the PLL driver the HAL default clock setup is used */
	SystemInit();
#endif
}
