/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>

#define DELAY 2000000
#define ALARM_CHANNEL_ID 0

struct counter_alarm_cfg alarm_cfg;

#if defined(CONFIG_BOARD_ATSAMD20_XPRO)
#define TIMER DT_NODELABEL(tc4)
#elif defined(CONFIG_SOC_FAMILY_SAM)
#define TIMER DT_NODELABEL(tc0)
#elif defined(CONFIG_COUNTER_MICROCHIP_MCP7940N)
#define TIMER DT_NODELABEL(extrtc0)
#elif defined(CONFIG_COUNTER_NRF_RTC)
#define TIMER DT_NODELABEL(rtc0)
#elif defined(CONFIG_COUNTER_TIMER_STM32)
#define TIMER DT_INST(0, st_stm32_counter)
#elif defined(CONFIG_COUNTER_RTC_STM32)
#define TIMER DT_INST(0, st_stm32_rtc)
#elif defined(CONFIG_COUNTER_SMARTBOND_TIMER)
#define TIMER DT_NODELABEL(timer3)
#elif defined(CONFIG_COUNTER_NATIVE_POSIX)
#define TIMER DT_NODELABEL(counter0)
#elif defined(CONFIG_COUNTER_XLNX_AXI_TIMER)
#define TIMER DT_INST(0, xlnx_xps_timer_1_00_a)
#elif defined(CONFIG_COUNTER_TMR_ESP32)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_MCUX_CTIMER)
#define TIMER DT_NODELABEL(ctimer0)
#elif defined(CONFIG_COUNTER_NXP_S32_SYS_TIMER)
#define TIMER DT_NODELABEL(stm0)
#elif defined(CONFIG_COUNTER_TIMER_GD32)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_GECKO_RTCC)
#define TIMER DT_NODELABEL(rtcc0)
#elif defined(CONFIG_COUNTER_GECKO_STIMER)
#define TIMER DT_NODELABEL(stimer0)
#elif defined(CONFIG_COUNTER_INFINEON_CAT1)
#define TIMER DT_NODELABEL(counter0_0)
#elif defined(CONFIG_COUNTER_AMBIQ)
#define TIMER DT_NODELABEL(counter0)
#elif defined(CONFIG_COUNTER_SNPS_DW)
#define TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_TIMER_RPI_PICO)
#define TIMER DT_NODELABEL(timer)
#endif

static void test_counter_interrupt_fn(const struct device *counter_dev,
				      uint8_t chan_id, uint32_t ticks,
				      void *user_data)
{
	struct counter_alarm_cfg *config = user_data;
	uint32_t now_ticks;
	uint64_t now_usec;
	int now_sec;
	int err;

	err = counter_get_value(counter_dev, &now_ticks);
	if (err) {
		printk("Failed to read counter value (err %d)", err);
		return;
	}

	now_usec = counter_ticks_to_us(counter_dev, now_ticks);
	now_sec = (int)(now_usec / USEC_PER_SEC);

	printk("!!! Alarm !!!\n");
	printk("Now: %u\n", now_sec);

	/* Set a new alarm with a double length duration */
	config->ticks = config->ticks * 2U;

	printk("Set alarm in %u sec (%u ticks)\n",
	       (uint32_t)(counter_ticks_to_us(counter_dev,
					   config->ticks) / USEC_PER_SEC),
	       config->ticks);

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					user_data);
	if (err != 0) {
		printk("Alarm could not be set\n");
	}
}

int main(void)
{
	const struct device *const counter_dev = DEVICE_DT_GET(TIMER);
	int err;

	printk("Counter alarm sample\n\n");

	if (!device_is_ready(counter_dev)) {
		printk("device not ready.\n");
		return 0;
	}

	counter_start(counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
	alarm_cfg.callback = test_counter_interrupt_fn;
	alarm_cfg.user_data = &alarm_cfg;

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					&alarm_cfg);
	printk("Set alarm in %u sec (%u ticks)\n",
	       (uint32_t)(counter_ticks_to_us(counter_dev,
					   alarm_cfg.ticks) / USEC_PER_SEC),
	       alarm_cfg.ticks);

	if (-EINVAL == err) {
		printk("Alarm settings invalid\n");
	} else if (-ENOTSUP == err) {
		printk("Alarm setting request not supported\n");
	} else if (err != 0) {
		printk("Error\n");
	}

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
