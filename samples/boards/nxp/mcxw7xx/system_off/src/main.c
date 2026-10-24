/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/drivers/hwinfo.h>
#include <fsl_device_registers.h>

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
#include <zephyr/drivers/counter.h>

#define WAKEUP_DELAY_S 5U
static const struct device *const lptmr = DEVICE_DT_GET(DT_NODELABEL(lptmr0));
#else
/* The wakeup button (a WUU external pin) is described per board and aliased as
 * "wakeup-button".
 */
static const struct wuc_dt_spec wakeup = WUC_DT_SPEC_GET(DT_ALIAS(wakeup_button));
#endif

/*
 * Each wakeup is a reset, and the reset code initialises the ECC SRAM on every
 * reset, the system off wake reset included, so the cycle counter cannot be
 * kept in SRAM. It lives in a register file register instead, which keeps its
 * value across a Power Down or Deep Power Down wakeup. REG0 is left to the
 * boot ROM wakeup CRC.
 */
#if defined(REGFILE1)
static volatile uint32_t *const cycles = &REGFILE1->REG[1];
#else
static volatile uint32_t *const cycles = &REGFILE_0->REG[1];
#endif

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
static void wakeup_alarm_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks,
			    void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

static int arm_timer_wakeup(void)
{
	struct counter_alarm_cfg cfg = {
		.callback = wakeup_alarm_cb,
		.ticks = counter_us_to_ticks(lptmr, (uint64_t)WAKEUP_DELAY_S * USEC_PER_SEC),
		.flags = 0,
	};

	(void)counter_start(lptmr);

	return counter_set_channel_alarm(lptmr, 0, &cfg);
}
#endif

int main(void)
{
	uint32_t reset_cause = 0U;
	bool woke;
	int ret;

	(void)hwinfo_get_reset_cause(&reset_cause);
	(void)hwinfo_clear_reset_cause();
	woke = (reset_cause & RESET_LOW_POWER_WAKE) != 0U;

	if (!woke) {
		/* Cold boot: seed the retained counter. */
		*cycles = 0U;
	}

	printk("%s system off demo\n", CONFIG_BOARD);
	if (woke) {
		printk("Woke from system off; retained count %u/%u\n", *cycles,
		       CONFIG_SAMPLE_APP_TEST_CYCLES);
	}

	if (*cycles >= CONFIG_SAMPLE_APP_TEST_CYCLES) {
		printk("Completed %u system-off cycles\n", CONFIG_SAMPLE_APP_TEST_CYCLES);
		return 0;
	}

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
	if (!device_is_ready(lptmr)) {
		printk("LPTMR counter device not ready\n");
		return 0;
	}

	/* LPTMR0 is a WUU wakeup source in the devicetree, so the counter driver
	 * arms the wakeup when the alarm is set - nothing else is needed here.
	 */
	ret = arm_timer_wakeup();
	if (ret < 0) {
		printk("Failed to arm LPTMR wakeup (%d)\n", ret);
		return 0;
	}
#else
	if (!device_is_ready(wakeup.dev)) {
		printk("WUC device %s not ready\n", wakeup.dev->name);
		return 0;
	}

	ret = wuc_enable_wakeup_source_dt(&wakeup);
	if (ret < 0) {
		printk("Failed to enable wakeup source (%d)\n", ret);
		return 0;
	}
#endif

	(*cycles)++;

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
	printk("Entering system off (cycle %u/%u); the timer wakes it in %u s\r\n", *cycles,
	       CONFIG_SAMPLE_APP_TEST_CYCLES, WAKEUP_DELAY_S);
#else
	printk("Entering system off (cycle %u/%u); trigger the wakeup pin to restart\r\n", *cycles,
	       CONFIG_SAMPLE_APP_TEST_CYCLES);
#endif

	sys_poweroff();

	/* Power Down and Deep Power Down wake through the reset routine, so this is
	 * never reached on a working system off.
	 */
	printk("ERROR: System off failed\n");

	return 0;
}
