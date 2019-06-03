/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <counter.h>
#include <drivers/system_timer.h>

extern int z_clock_hw_cycles_per_sec;

static struct device *counter;	/* Counter device used as clock source. */
static u32_t curr_cycle;	/* Current (last announced) clock cycle. */
static u32_t alarm_cycle;	/* Clock cycle on which alarm is scheduled. */

static void alarm(struct device *dev, u8_t chan_id,
		  u32_t cycle, void *user_data)
{
	s32_t ticks;

	ticks = (cycle - curr_cycle) / sys_clock_hw_cycles_per_tick();
	curr_cycle = cycle;

	z_clock_announce(ticks);
}

int z_clock_driver_init(struct device *device)
{
	struct counter_top_cfg counter_top_cfg = { 0 };
	int status;
	u32_t max;

	counter = device_get_binding(DT_CLOCK_SOURCE_ON_DEV_NAME);

	__ASSERT(counter != NULL, "System clock device not found!");
	__ASSERT(counter_get_num_of_channels(counter) > 0,
		"System clock device must have at least one compare channel!");
	__ASSERT(counter_is_counting_up(counter),
		"System clock device must count up!");

	z_clock_hw_cycles_per_sec = counter_get_frequency(counter);

	max = counter_get_max_top_value(counter);
	__ASSERT(max == UINT32_MAX,
		"Maximum counter top value must be equal to UINT32_MAX!");

	/*
	 * Set counter top to the largest possible value. Note, that some
	 * counters might not support ceratin reset modes, so we might need
	 * more that one attempt.
	 */
	counter_top_cfg.ticks = max;
	status = counter_set_top_value(counter, &counter_top_cfg);
	if (status != 0) {
		counter_top_cfg.flags |= COUNTER_TOP_CFG_DONT_RESET;
		status = counter_set_top_value(counter, &counter_top_cfg);
	}
	__ASSERT(status == 0, "Could not configure system clock device!");

	status = counter_start(counter);
	__ASSERT(status == 0, "Could not start system clock!");

	/* Schedule first alarm  */
	z_clock_set_timeout(K_FOREVER, false);

#if 1
	/* DEMO: Print information about current clock source. */
	printk("Clock Source: %s (frequency: %u Hz)\n",
		DT_CLOCK_SOURCE_ON_DEV_NAME,
		z_clock_hw_cycles_per_sec);
#endif

	return status;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	u32_t now = counter_read(counter);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm,
		.absolute = true,
	};
	int status;

	if ((s32_t)(alarm_cycle - now) > 0 &&
	    (s32_t)(alarm_cycle - now) < sys_clock_hw_cycles_per_tick()) {
		/* Just wait for alarm if it is scheduled within one tick. */
		return;
	}

	if (ticks > (counter_get_max_relative_alarm(counter) /
					sys_clock_hw_cycles_per_tick())) {
		ticks = K_FOREVER;
	}

	if (ticks != K_FOREVER) {
		u32_t delta = ticks * sys_clock_hw_cycles_per_tick();
		u32_t alarm = curr_cycle + delta;

		do {
			/*
			 * If alarm is in the past or closer than one tick from
			 * now, delay it to next tick boundary.
			 */
			while ((s32_t)(alarm - now) < sys_clock_hw_cycles_per_tick()) {
				alarm += sys_clock_hw_cycles_per_tick();
			}

			alarm_cfg.ticks = alarm;
			counter_cancel_channel_alarm(counter, 0);
			status = counter_set_channel_alarm(counter, 0,
								&alarm_cfg);

			/*
			 * If we are still before alarm, the work is done.
			 * Otherwise there is a risk that we missed alarm,
			 * so in order to avoid uncertainty repeat counter
			 * configuration.
			 */
			now = counter_read(counter);
		} while ((s32_t)(now - alarm) >= 0);

	} else {
		alarm_cfg.ticks = now + counter_get_max_relative_alarm(counter);
		counter_cancel_channel_alarm(counter, 0);
		status = counter_set_channel_alarm(counter, 0, &alarm_cfg);
	}

	__ASSERT(status == 0,
		 "Cannot set alarm. Error %d!", status);

	alarm_cycle = alarm_cfg.ticks;
}

u32_t z_clock_elapsed(void)
{
	return (counter_read(counter) - curr_cycle) /
					sys_clock_hw_cycles_per_tick();
}

u32_t z_timer_cycle_get_32(void)
{
	return counter_read(counter);
}
