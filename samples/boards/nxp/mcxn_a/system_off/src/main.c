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
#include <zephyr/toolchain.h>

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
#include <zephyr/drivers/counter.h>

#define WAKEUP_DELAY_S 5U
static const struct device *const lptmr = DEVICE_DT_GET(DT_NODELABEL(lptmr0));
/* LPTMR0 reaches the core as a WUU internal-module wakeup source. */
static const struct wuc_dt_spec wakeup = WUC_DT_SPEC_GET(DT_NODELABEL(lptmr0));
#else
/* The wakeup button (a WUU external pin) is described per board and aliased as
 * "wakeup-button".
 */
static const struct wuc_dt_spec wakeup = WUC_DT_SPEC_GET(DT_ALIAS(wakeup_button));
#endif

/*
 * Deep Power Down gates every SRAM array, so the cycle counter that has to
 * survive the power-off/wake reset must live in an array whose low-power
 * retention is armed before entering Deep Power Down. This mirrors the SoC
 * suspend-to-RAM retention path; a production application would share that code.
 *
 *   - MCXN / MCXAxx7: the VBAT backup SRAM regulator retains RAMA. The linker
 *     places .noinit (and so the counter) in RAMA because the board overlay
 *     points zephyr,sram there.
 *   - other MCXA: the SPC SRAM retention LDO retains every array, so the counter
 *     is retained wherever it lands.
 */
#if defined(CONFIG_SOC_FAMILY_MCXN) || defined(CONFIG_SOC_SERIES_MCXAXX7)
#include <fsl_vbat.h>

#define VBAT0_ADDR ((VBAT_Type *)DT_REG_ADDR(DT_INST(0, nxp_vbat)))
#define MCXN_RAMA_ALL						\
	(kVBAT_SramArray0 | kVBAT_SramArray1 | kVBAT_SramArray2 | kVBAT_SramArray3)

static void retain_counter_ram(void)
{
	if (!VBAT_CheckFRO16kEnabled(VBAT0_ADDR)) {
		VBAT_EnableFRO16k(VBAT0_ADDR, true);
	}
	VBAT_UngateFRO16k(VBAT0_ADDR, kVBAT_EnableClockToVddSys);
	if (!VBAT_CheckBandgapEnabled(VBAT0_ADDR)) {
		(void)VBAT_EnableBandgap(VBAT0_ADDR, true);
	}
	VBAT_EnableBandgapRefreshMode(VBAT0_ADDR, true);
	(void)VBAT_EnableBackupSRAMRegulator(VBAT0_ADDR, true);
	VBAT_RetainSRAMsInLowPowerModes(VBAT0_ADDR, MCXN_RAMA_ALL);
}
#elif defined(CONFIG_SOC_FAMILY_MCXA)
#include <fsl_spc.h>

#define SPC0_ADDR ((SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc)))

static void retain_counter_ram(void)
{
	SPC_EnableSRAMLdo(SPC0_ADDR, true);
	SPC_RetainSRAMArray(SPC0_ADDR, 0xFU);
}
#else
static void retain_counter_ram(void)
{
}
#endif

/* Retained across the Deep Power Down reset (see retain_counter_ram()).
 * Left uninitialised so the C runtime never clears it; a cold boot is
 * detected from the reset cause and seeds it explicitly.
 */
static __noinit uint32_t cycles;

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
		cycles = 0U;
	}

	printk("%s system off demo\n", CONFIG_BOARD);
	if (woke) {
		printk("Woke from system off; retained count %u/%u\n", cycles,
		       CONFIG_SAMPLE_APP_TEST_CYCLES);
	}

	if (cycles >= CONFIG_SAMPLE_APP_TEST_CYCLES) {
		printk("Completed %u system-off cycles\n", CONFIG_SAMPLE_APP_TEST_CYCLES);
		return 0;
	}

	if (!device_is_ready(wakeup.dev)) {
		printk("WUC device %s not ready\n", wakeup.dev->name);
		return 0;
	}

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
	if (!device_is_ready(lptmr)) {
		printk("LPTMR counter device not ready\n");
		return 0;
	}

	ret = arm_timer_wakeup();
	if (ret < 0) {
		printk("Failed to arm LPTMR wakeup (%d)\n", ret);
		return 0;
	}
#endif

	ret = wuc_enable_wakeup_source_dt(&wakeup);
	if (ret < 0) {
		printk("Failed to enable wakeup source (%d)\n", ret);
		return 0;
	}

	cycles++;
	retain_counter_ram();

#if defined(CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER)
	printk("Entering system off (cycle %u/%u); the timer wakes it in %u s\r\n", cycles,
	       CONFIG_SAMPLE_APP_TEST_CYCLES, WAKEUP_DELAY_S);
#else
	printk("Entering system off (cycle %u/%u); trigger the wakeup pin to restart\r\n", cycles,
	       CONFIG_SAMPLE_APP_TEST_CYCLES);
#endif

	sys_poweroff();

	/* Deep Power Down wakes through the reset routine, so this is never
	 * reached on a working system off.
	 */
	printk("ERROR: System off failed\n");

	return 0;
}
