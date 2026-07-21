/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/cache.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>

#include <fsl_power.h>

#define WAKEUP_SECONDS 5

/*
 * Keep the boot reason visible briefly before entering standby again.
 */
#define RECOVERY_WINDOW_SECONDS 3

#define STANDBY_RAM_NODE DT_NODELABEL(stdby_ram)
#define STANDBY_RAM_SECTION LINKER_DT_NODE_REGION_NAME(STANDBY_RAM_NODE)

static uint32_t standby_cycle_count Z_GENERIC_SECTION(STANDBY_RAM_SECTION);

static void set_standby_cycle_count(uint32_t count)
{
	standby_cycle_count = count;
	(void)sys_cache_data_flush_range(&standby_cycle_count, sizeof(standby_cycle_count));
}

static void alarm_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks,
			   void *user_data)
{
	/*
	 * Required by the counter API, but never actually invoked here: on
	 * MCXE31B the RTC alarm wakes the SoC out of standby as a functional
	 * reset, so execution restarts from main() rather than this callback.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

int main(void)
{
	const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	struct counter_alarm_cfg alarm_cfg = { 0 };
	int ret;

	if (POWER_ExitFromStandbyMode()) {
		printf("Woke from standby\n");
		printf("Retained standby cycle count: %u/%u\n", standby_cycle_count,
		       CONFIG_APP_TEST_CYCLES);
	} else {
		set_standby_cycle_count(0U);
		printf("Cold boot\n");
	}

	if (standby_cycle_count >= CONFIG_APP_TEST_CYCLES) {
		printf("Completed %u standby cycles\n", CONFIG_APP_TEST_CYCLES);
		set_standby_cycle_count(0U);
		return 0;
	}

	for (int i = RECOVERY_WINDOW_SECONDS; i > 0; i--) {
		printf("Powering off in %d s...\n", i);
		k_sleep(K_SECONDS(1));
	}

	if (!device_is_ready(rtc)) {
		printf("RTC device not ready\n");
		return 0;
	}

	ret = counter_start(rtc);
	if (ret < 0) {
		printf("Could not start RTC counter (%d)\n", ret);
		return 0;
	}

	alarm_cfg.callback = alarm_callback;
	alarm_cfg.ticks = counter_us_to_ticks(rtc, (uint64_t)WAKEUP_SECONDS * USEC_PER_SEC);

	ret = counter_set_channel_alarm(rtc, 0, &alarm_cfg);
	if (ret < 0) {
		printf("Could not set RTC alarm (%d)\n", ret);
		return 0;
	}

	set_standby_cycle_count(standby_cycle_count + 1U);

	printf("Wake-up alarm set for %d seconds (cycle %u/%u)\n", WAKEUP_SECONDS,
	       standby_cycle_count, CONFIG_APP_TEST_CYCLES);
	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
