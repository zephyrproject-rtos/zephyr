/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/console/console.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

#include <fsl_irtc.h>

#define WAKEUP_SECONDS CONFIG_SAMPLE_RT700_SYSTEM_OFF_WAKEUP_SECONDS
#define WAKEUP_CYCLES  3U

/*
 * Deep power down turns off every supply except VDD1V8_AO, so SRAM and all
 * module registers lose their contents on wake up - the iRTC is the sole
 * exception. The alarm registers are unused here (the wake timer, not the
 * calendar alarm, provides the wake up), so borrow two of them to carry the
 * cycle counter across the power down. Both hold reserved bit ranges, hence
 * the packing below. RTC0->GPR looks like a better fit but is off limits: the
 * boot ROM keeps its flash state context there.
 */
#define WAKE_RTC ((RTC_Type *)DT_REG_ADDR(DT_NODELABEL(rtc0)))

/* ALM_YEARMON holds a magic word marking the counter as valid. */
#define WAKE_MAGIC_REG  ALM_YEARMON
#define WAKE_MAGIC_MASK (RTC_ALM_YEARMON_ALM_MON_MASK | RTC_ALM_YEARMON_ALM_YEAR_MASK)
#define WAKE_MAGIC      0xA50CU

/* ALM_HOURMIN holds the counter itself, 11 bits split across two fields. */
#define WAKE_COUNT_REG ALM_HOURMIN
#define WAKE_COUNT_MAX 0x7FFU

BUILD_ASSERT(WAKEUP_CYCLES <= WAKE_COUNT_MAX, "Wake up count does not fit in ALM_HOURMIN");

static uint16_t wake_count_pack(uint16_t count)
{
	return RTC_ALM_HOURMIN_ALM_MIN(count) | RTC_ALM_HOURMIN_ALM_HOUR(count >> 6);
}

static uint16_t wake_count_unpack(uint16_t reg)
{
	return ((reg & RTC_ALM_HOURMIN_ALM_MIN_MASK) >> RTC_ALM_HOURMIN_ALM_MIN_SHIFT) |
	       (((reg & RTC_ALM_HOURMIN_ALM_HOUR_MASK) >> RTC_ALM_HOURMIN_ALM_HOUR_SHIFT) << 6);
}

static uint16_t wake_count_get(void)
{
	if ((WAKE_RTC->WAKE_MAGIC_REG & WAKE_MAGIC_MASK) != WAKE_MAGIC) {
		return 0U;
	}

	return wake_count_unpack(WAKE_RTC->WAKE_COUNT_REG);
}

static int wake_count_set(uint16_t count)
{
	status_t status;

	/*
	 * Write protection re-arms itself a couple of seconds after being
	 * lifted, so lock first to guarantee a full window.
	 */
	(void)IRTC_SetWriteProtection(WAKE_RTC, true);
	status = IRTC_SetWriteProtection(WAKE_RTC, false);
	if (status != kStatus_Success) {
		return -EIO;
	}

	WAKE_RTC->WAKE_MAGIC_REG = WAKE_MAGIC;
	WAKE_RTC->WAKE_COUNT_REG = wake_count_pack(count);

	(void)IRTC_SetWriteProtection(WAKE_RTC, true);

	return 0;
}

/*
 * DEEPPDF distinguishes a deep power down wake up from any other boot. It is
 * only cleared by a VDD1V8_AO power-on-reset, so clear it here to keep it
 * meaningful across the resets a debug session issues.
 */
static bool woke_from_poweroff(void)
{
	bool woke = (PMC0->FLAGS & PMC_FLAGS_DEEPPDF_MASK) != 0U;

	PMC0->FLAGS = PMC_FLAGS_DEEPPDF_MASK;

	return woke;
}

/*
 * Hold the run at a prompt until the operator presses ENTER, so the first power
 * down cannot happen before the current meter is armed. Only the cold boot asks:
 * a wake up is a fresh boot of the same image, and stopping for input on every
 * cycle would defeat the unattended cycling the measurement relies on.
 */
static void wait_for_enter(void)
{
	console_init();

	printf("Press ENTER to start the power-off cycles\n");

	while (true) {
		int c = console_getchar();

		if ((c == '\r') || (c == '\n')) {
			return;
		}
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(irtc_wake));
	struct counter_alarm_cfg alarm_cfg = {0};
	uint16_t count = woke_from_poweroff() ? wake_count_get() : 0U;
	int ret;

	if (count == 0U) {
		printf("Cold boot, powering off %u times\n", WAKEUP_CYCLES);

		if (IS_ENABLED(CONFIG_SAMPLE_RT700_SYSTEM_OFF_WAIT_FOR_ENTER)) {
			wait_for_enter();
		}
	} else {
		printf("Woke up from deep power down, cycle %u of %u\n", count, WAKEUP_CYCLES);
	}

	if (count >= WAKEUP_CYCLES) {
		printf("Done, staying awake\n");
		return 0;
	}

	if (!device_is_ready(dev)) {
		printf("Wake timer device not ready\n");
		return 0;
	}

	if (!pm_device_wakeup_enable(dev, true)) {
		printf("Could not enable wake timer as wakeup source\n");
		return 0;
	}

	ret = counter_start(dev);
	if (ret < 0) {
		printf("Could not start wake timer (%d)\n", ret);
		return 0;
	}

	alarm_cfg.ticks = counter_us_to_ticks(dev, WAKEUP_SECONDS * USEC_PER_SEC);

	ret = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	if (ret < 0) {
		printf("Could not set wake timer alarm (%d)\n", ret);
		return 0;
	}

	ret = wake_count_set(count + 1U);
	if (ret < 0) {
		printf("Could not store wake up count (%d)\n", ret);
		return 0;
	}

	printf("Wake-up alarm set for %u seconds\n", WAKEUP_SECONDS);
	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
