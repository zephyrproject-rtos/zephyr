/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <counter.h>

static void aonpt_example_callback(struct device *dev, void *user_data);

static void free_running_counter_example(void);
static void periodic_timer_example(void);

void main(void)
{
	/* test quark Always-on free running counter */
	free_running_counter_example();

	/* test quark Always-on periodic timer */
	periodic_timer_example();
}

static void free_running_counter_example(void)
{
	volatile uint32_t delay = 0;
	uint32_t c_val = 0, i = 0;
	uint32_t dummy_data = 30;
	uint32_t counter_initial_value = 10000;

	struct device *aon_counter_dev;

	aon_counter_dev = device_get_binding("AON_COUNTER");

	if (!aon_counter_dev) {
		printk("Counter device not found\n");
		return;
	}

	printk("Always-on free running counter example app\n");

	if (counter_start(aon_counter_dev) != DEV_OK) {
		printk("Counter device enabling fail!\n");
		return;
	}

	printk("Always-on counter started\n");

	/* The AON counter runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 */
	for (delay = 5000; delay--;) {
	}

	for (i = 0; i < 20; i++) {
		for (delay = 500; delay--;) {
		}

		c_val = counter_read(aon_counter_dev);
		printk("Always-on counter value: %d\n", c_val);
	}

	if (counter_set_alarm(aon_counter_dev, NULL, counter_initial_value,
			      (void *)&dummy_data) != DEV_OK) {
		printk("Always-on counter does not support alarm!\n");
	}

	counter_stop(aon_counter_dev);

	printk("Always-on counter stopped\n");
}

static void periodic_timer_example(void)
{
	volatile uint32_t delay = 0;
	uint32_t pt_val = 0, i = 0;
	uint32_t dummy_data = 30;
	uint32_t timer_initial_value = 10000;

	struct device *aon_periodic_timer_dev = NULL;

	aon_periodic_timer_dev = device_get_binding("AON_TIMER");

	if (!aon_periodic_timer_dev) {
		printk("Timer device not found\n");
		return;
	}

	printk("Periodic timer example app\n");

	counter_start(aon_periodic_timer_dev);

	printk("Periodic timer started\n");

	/* The AON timer runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 */
	for (delay = 5000; delay--;) {
	}

	for (i = 0; i < 20; i++) {
		for (delay = 500; delay--;) {
		}

		pt_val = counter_read(aon_periodic_timer_dev);
		printk("Periodic timer value: %x\n", pt_val);
	}

	if (counter_set_alarm(aon_periodic_timer_dev, aonpt_example_callback,
			      timer_initial_value, (void *)&dummy_data)
			      != DEV_OK) {
		printk("Periodic Timer was not started yet\n");
		return;
	}

	printk("Periodic Timer alarm on\n");

	/* long delay for the alarm and callback to happen */
	for (delay = 5000000; delay--;) {
	}

	/* callback is turned off */
	if (counter_set_alarm(aon_periodic_timer_dev, NULL,
			      timer_initial_value, (void *)&dummy_data)
			      != DEV_OK) {
		printk("Periodic timer was not started yet\n");
		return;
	}

	printk("Periodic timer alarm off\n");

	for (i = 0; i < 20; i++) {
		for (delay = 500; delay--;) {
		}

		pt_val = counter_read(aon_periodic_timer_dev);
		printk("Periodic timer value: %x\n", pt_val);
	}

	counter_stop(aon_periodic_timer_dev);

	printk("Periodic timer stopped\n");
}

static void aonpt_example_callback(struct device *dev, void *user_data)
{
	printk("Periodic timer callback data %d\n", *((uint32_t *)user_data));

	printk("Periodic timer callback value %d\n", counter_read(dev));
}
