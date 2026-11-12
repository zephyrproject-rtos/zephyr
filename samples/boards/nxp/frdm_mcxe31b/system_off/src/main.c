/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/cache.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/clock.h>

#include <fsl_power.h>
#include <fsl_clock.h>

#include "standby.h"

#define WAKEUP_SECONDS 5

/*
 * Keep the boot reason visible briefly before entering standby again.
 */
#define RECOVERY_WINDOW_SECONDS 3

#define STANDBY_RAM_NODE DT_NODELABEL(stdby_ram)
#define STANDBY_RAM_SECTION LINKER_DT_NODE_REGION_NAME(STANDBY_RAM_NODE)

static struct standby_data retained Z_GENERIC_SECTION(STANDBY_RAM_SECTION);

static int set_standby_cycle_count(uint32_t count)
{
	retained.magic = STANDBY_MAGIC;
	retained.cycles = count;
	retained.check = ~count;
	return sys_cache_data_flush_range(&retained, sizeof(retained));
}

#if defined(CONFIG_APP_USE_CPU1)
static void update_cpu1_clock(bool enable)
{
	if (enable) {
		MC_ME->PRTN0_CORE1_PCONF |= MC_ME_PRTN0_CORE1_PCONF_CCE_MASK;
	} else {
		MC_ME->PRTN0_CORE1_PCONF &= ~MC_ME_PRTN0_CORE1_PCONF_CCE_MASK;
	}
	MC_ME->PRTN0_CORE1_PUPD = MC_ME_PRTN0_CORE1_PUPD_CCUPD_MASK;
	MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY(0x5AF0U);
	MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY(0xA50FU);
}

static int start_cpu1(void)
{
	if ((uintptr_t)&retained != DT_REG_ADDR(STANDBY_RAM_NODE)) {
		printf("FAIL: CPU1 mailbox is not at the start of standby SRAM\n");
		return -EINVAL;
	}
	if (MC_ME->PRTN0_CORE1_STAT & MC_ME_PRTN0_CORE1_STAT_CCS_MASK) {
		printf("FAIL: CPU1 was already enabled\n");
		return -EBUSY;
	}

	retained.request = 0U;
	retained.state = 0U;
	__DSB();
	MC_ME->PRTN0_CORE1_ADDR = DT_REG_ADDR(DT_NODELABEL(flash0)) +
				DT_REG_ADDR(DT_NODELABEL(cpu1_code_partition));
	update_cpu1_clock(true);

	if (!WAIT_FOR((MC_ME->PRTN0_CORE1_PUPD & MC_ME_PRTN0_CORE1_PUPD_CCUPD_MASK) == 0U &&
		      (MC_ME->PRTN0_CORE1_STAT & MC_ME_PRTN0_CORE1_STAT_CCS_MASK) != 0U &&
		      retained.state == CPU1_READY, 1000000, k_busy_wait(10))) {
		printf("FAIL: CPU1 did not start\n");
		return -ETIMEDOUT;
	}
	printf("CPU1 started\n");
	return 0;
}

static int stop_cpu1(void)
{
	retained.request = CPU1_STOP;
	__DSB();
	if (!WAIT_FOR(retained.state == CPU1_PARKED &&
		      (MC_ME->PRTN0_CORE1_STAT & MC_ME_PRTN0_CORE1_STAT_WFI_MASK) != 0U,
		      1000000, k_busy_wait(10))) {
		printf("FAIL: CPU1 did not acknowledge shutdown and enter WFI (state %u)\n",
		       retained.state);
		return -ETIMEDOUT;
	}

	update_cpu1_clock(false);
	if (!WAIT_FOR((MC_ME->PRTN0_CORE1_PUPD & MC_ME_PRTN0_CORE1_PUPD_CCUPD_MASK) == 0U &&
		      (MC_ME->PRTN0_CORE1_STAT & MC_ME_PRTN0_CORE1_STAT_CCS_MASK) == 0U,
		      100000, k_busy_wait(10))) {
		printf("FAIL: CPU1 clock did not stop\n");
		return -ETIMEDOUT;
	}
	printf("CPU1 parked and clock gated\n");
	return 0;
}
#endif

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
		if (retained.magic != STANDBY_MAGIC || retained.check != ~retained.cycles ||
		    retained.cycles == 0U || retained.cycles > CONFIG_APP_TEST_CYCLES) {
			printf("FAIL: standby SRAM contents were not retained\n");
			return -EIO;
		}
		printf("Woke from standby\n");
		printf("Retained standby cycle count: %u/%u\n", retained.cycles,
		       CONFIG_APP_TEST_CYCLES);
#if defined(CONFIG_SOC_MCXE32B_CPU0)
		uint64_t sources = soc_mcxe32x_wakeup_sources();
		uint64_t expected = 0U;

		if (!IS_ENABLED(CONFIG_APP_WAKEUP_GPIO)) {
			expected |= BIT64(1);
		}
		if (!IS_ENABLED(CONFIG_APP_WAKEUP_RTC)) {
			expected |= BIT64(30);
		}
		printf("Wake-up sources: 0x%08x%08x\n", (uint32_t)(sources >> 32U),
		       (uint32_t)sources);
		if ((sources & expected) == 0U) {
			printf("FAIL: expected wake-up source was not recorded\n");
			return -EIO;
		}
#endif
	} else {
		ret = set_standby_cycle_count(0U);
		if (ret < 0) {
			printf("FAIL: cannot flush standby SRAM (%d)\n", ret);
			return ret;
		}
		printf("Cold boot\n");
	}

#if defined(CONFIG_SOC_MCXE32B_CPU0)
	uint32_t rate = CLOCK_GetCoreClkFreq();

	printf("Run clock: %u Hz\n", rate);
	if (rate != DT_PROP(DT_NODELABEL(core0), clock_frequency)) {
		printf("FAIL: run clock was not restored\n");
		return -EIO;
	}
#endif

	if (retained.cycles >= CONFIG_APP_TEST_CYCLES) {
		ret = set_standby_cycle_count(0U);
		if (ret < 0) {
			printf("FAIL: cannot flush standby SRAM (%d)\n", ret);
			return ret;
		}
		printf("Completed %u standby cycles\n", CONFIG_APP_TEST_CYCLES);
		return 0;
	}

#if defined(CONFIG_APP_USE_CPU1)
	ret = start_cpu1();
	if (ret < 0) {
		return ret;
	}
#endif

	for (int i = RECOVERY_WINDOW_SECONDS; i > 0; i--) {
		printf("Powering off in %d s...\n", i);
		k_sleep(K_SECONDS(1));
	}

	if (!IS_ENABLED(CONFIG_APP_WAKEUP_GPIO)) {
		if (!device_is_ready(rtc)) {
			printf("FAIL: RTC device not ready\n");
			return -ENODEV;
		}

		ret = counter_start(rtc);
		if (ret < 0) {
			printf("FAIL: could not start RTC counter (%d)\n", ret);
			return ret;
		}
	}

#if defined(CONFIG_APP_USE_CPU1)
	ret = stop_cpu1();
	if (ret < 0) {
		return ret;
	}
#endif

#if defined(CONFIG_APP_WAKEUP_GPIO) || defined(CONFIG_APP_WAKEUP_BOTH)
	const struct gpio_dt_spec wake = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wake_gpios);

	if (!gpio_is_ready_dt(&wake)) {
		printf("FAIL: wake-up GPIO is not ready\n");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&wake, GPIO_INPUT);
	if (ret < 0) {
		printf("FAIL: could not configure wake-up GPIO (%d)\n", ret);
		return ret;
	}
	/* Allow the input buffer and pull-up to settle before checking the level. */
	k_sleep(K_MSEC(1));
	ret = gpio_pin_get_dt(&wake);
	if (ret != 0) {
		printf("FAIL: wake-up GPIO is already active or cannot be read (%d)\n", ret);
		return ret < 0 ? ret : -EBUSY;
	}
#endif

	/* No interrupt handler may consume a wake event between arming and WFI. */
	(void)irq_lock();

#if defined(CONFIG_APP_WAKEUP_GPIO) || defined(CONFIG_APP_WAKEUP_BOTH)
	ret = gpio_pin_interrupt_configure_dt(&wake, GPIO_INT_EDGE_TO_ACTIVE | GPIO_INT_WAKEUP);
	if (ret < 0) {
		printf("FAIL: could not arm GPIO wake-up (%d)\n", ret);
		k_panic();
	}
	printf("Wake-up GPIO PTE0 armed\n");
#endif

	if (!IS_ENABLED(CONFIG_APP_WAKEUP_GPIO)) {
		alarm_cfg.callback = alarm_callback;
		alarm_cfg.ticks = counter_us_to_ticks(rtc,
						     (uint64_t)WAKEUP_SECONDS * USEC_PER_SEC);
		ret = counter_set_channel_alarm(rtc, 0, &alarm_cfg);
		if (ret < 0) {
			printf("FAIL: could not set RTC alarm (%d)\n", ret);
			k_panic();
		}
		printf("Wake-up alarm set for %d seconds (cycle %u/%u)\n", WAKEUP_SECONDS,
		       retained.cycles + 1U, CONFIG_APP_TEST_CYCLES);
	}

	ret = set_standby_cycle_count(retained.cycles + 1U);
	if (ret < 0) {
		printf("FAIL: cannot flush standby SRAM (%d)\n", ret);
		k_panic();
	}
	printf("Powering off\n");

	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!WAIT_FOR(uart_irq_tx_complete(console) > 0, 10000, k_busy_wait(10))) {
		printf("FAIL: console transmission did not finish\n");
		k_panic();
	}

	sys_poweroff();

	return 0;
}
