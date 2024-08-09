/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_PBCFG_A_Type.h>
#include <SI32_PBSTD_A_Type.h>
#include <SI32_RSTSRC_A_Type.h>
#include <SI32_VMON_A_Type.h>
#include <SI32_WDTIMER_A_Type.h>
#include <si32_device.h>
#include <silabs_crossbar_config.h>

static void gpio_init(void)
{
	/* After a device reset, all crossbars enter a disabled default reset state, causing all
	 * port bank pins into a high impedance digital input mode.
	 */

	/* Enable clocks that we need in any case */
	SI32_CLKCTRL_A_enable_apb_to_modules_0(
		SI32_CLKCTRL_0, SI32_CLKCTRL_A_APBCLKG0_PB0 | SI32_CLKCTRL_A_APBCLKG0_FLASHCTRL0);

	/* Enable misc clocks.
	 * We may or may nor need all of that but it includes some basics like
	 * LOCK0, WDTIMER0 and DMA. Doing it here prevents the actual drivers
	 * needing to know whoelse needs one of these clocks.
	 */
	SI32_CLKCTRL_A_enable_apb_to_modules_1(
		SI32_CLKCTRL_0, SI32_CLKCTRL_A_APBCLKG1_MISC0 | SI32_CLKCTRL_A_APBCLKG1_MISC1);
	SI32_CLKCTRL_A_enable_ahb_to_dma_controller(SI32_CLKCTRL_0);

	/* Apply pinctrl gpio settings */
	SI32_PBSTD_A_write_pins_high(SI32_PBSTD_0, PORTBANK_0_PINS_HIGH);
	SI32_PBSTD_A_write_pins_low(SI32_PBSTD_0, PORTBANK_0_PINS_LOW);
	SI32_PBSTD_A_set_pins_digital_input(SI32_PBSTD_0, PORTBANK_0_PINS_DIGITAL_INPUT);
	SI32_PBSTD_A_set_pins_push_pull_output(SI32_PBSTD_0, PORTBANK_0_PINS_PUSH_PULL_OUTPUT);
	SI32_PBSTD_A_set_pins_analog(SI32_PBSTD_0, PORTBANK_0_PINS_ANALOG);

	SI32_PBSTD_A_write_pins_high(SI32_PBSTD_1, PORTBANK_1_PINS_HIGH);
	SI32_PBSTD_A_write_pins_low(SI32_PBSTD_1, PORTBANK_1_PINS_LOW);
	SI32_PBSTD_A_set_pins_digital_input(SI32_PBSTD_1, PORTBANK_1_PINS_DIGITAL_INPUT);
	SI32_PBSTD_A_set_pins_push_pull_output(SI32_PBSTD_1, PORTBANK_1_PINS_PUSH_PULL_OUTPUT);
	SI32_PBSTD_A_set_pins_analog(SI32_PBSTD_1, PORTBANK_1_PINS_ANALOG);

	SI32_PBSTD_A_write_pins_high(SI32_PBSTD_2, PORTBANK_2_PINS_HIGH);
	SI32_PBSTD_A_write_pins_low(SI32_PBSTD_2, PORTBANK_2_PINS_LOW);
	SI32_PBSTD_A_set_pins_digital_input(SI32_PBSTD_2, PORTBANK_2_PINS_DIGITAL_INPUT);
	SI32_PBSTD_A_set_pins_push_pull_output(SI32_PBSTD_2, PORTBANK_2_PINS_PUSH_PULL_OUTPUT);
	SI32_PBSTD_A_set_pins_analog(SI32_PBSTD_2, PORTBANK_2_PINS_ANALOG);

	SI32_PBSTD_A_write_pins_high(SI32_PBSTD_3, PORTBANK_3_PINS_HIGH);
	SI32_PBSTD_A_write_pins_low(SI32_PBSTD_3, PORTBANK_3_PINS_LOW);
	SI32_PBSTD_A_set_pins_digital_input(SI32_PBSTD_3, PORTBANK_3_PINS_DIGITAL_INPUT);
	SI32_PBSTD_A_set_pins_push_pull_output(SI32_PBSTD_3, PORTBANK_3_PINS_PUSH_PULL_OUTPUT);
	SI32_PBSTD_A_set_pins_analog(SI32_PBSTD_3, PORTBANK_3_PINS_ANALOG);

	/* Configure skips */
	SI32_PBSTD_A_write_pbskipen(SI32_PBSTD_0, PORTBANK_0_SKIPEN_VALUE);
	SI32_PBSTD_A_write_pbskipen(SI32_PBSTD_1, PORTBANK_1_SKIPEN_VALUE);
	SI32_PBSTD_A_write_pbskipen(SI32_PBSTD_2, PORTBANK_2_SKIPEN_VALUE);
	SI32_PBSTD_A_write_pbskipen(SI32_PBSTD_3, PORTBANK_3_SKIPEN_VALUE);

	/* Configure crossbars */
	SI32_PBCFG_A_enable_xbar0l_peripherals(SI32_PBCFG_0, CROSSBAR_0_CONFIG & 0xFFFFFFFF);
	SI32_PBCFG_A_enable_xbar0h_peripherals(SI32_PBCFG_0,
					       (CROSSBAR_0_CONFIG >> 32) & 0xFFFFFFFF);
	SI32_PBCFG_A_enable_xbar1_peripherals(SI32_PBCFG_0, CROSSBAR_1_CONFIG & 0xFFFFFFFF);

	/* Enable crossbars */
	SI32_PBCFG_A_enable_crossbar_0(SI32_PBCFG_0);
	SI32_PBCFG_A_enable_crossbar_1(SI32_PBCFG_0);
}

static void vddlow_irq_handler(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to do here, we just don't want any spurious interrupts */
}

static void vmon_init(void)
{
	/* VMON must be enabled for flash write/erase support */

	NVIC_ClearPendingIRQ(VDDLOW_IRQn);

	IRQ_CONNECT(VDDLOW_IRQn, 0, vddlow_irq_handler, NULL, 0);
	irq_enable(VDDLOW_IRQn);

	SI32_VMON_A_enable_vdd_supply_monitor(SI32_VMON_0);
	SI32_VMON_A_enable_vdd_low_interrupt(SI32_VMON_0);
	SI32_VMON_A_select_vdd_standard_threshold(SI32_VMON_0);
}

__no_optimization static void busy_delay(uint32_t cycles)
{
	while (cycles) {
		cycles--;
	}
}

static int silabs_sim3u_init(void)
{
	uint32_t key;

	key = irq_lock();

	/* The watchdog may be enabled already so we have to disable it */
	SI32_WDTIMER_A_reset_counter(SI32_WDTIMER_0);
	SI32_WDTIMER_A_stop_counter(SI32_WDTIMER_0);
	SI32_RSTSRC_A_disable_watchdog_timer_reset_source(SI32_RSTSRC_0);

	/* Since a hardware reset affects the debug hardware as well, this makes it easier to
	 * recover from a broken firmware.
	 *
	 * For details, see erratum #H2 in the document "SiM3U1xx and SiM3C1xx Revision B Errata".
	 */
	busy_delay(3000000);

	gpio_init();
	vmon_init();

	irq_unlock(key);

	return 0;
}

SYS_INIT(silabs_sim3u_init, PRE_KERNEL_1, 0);
