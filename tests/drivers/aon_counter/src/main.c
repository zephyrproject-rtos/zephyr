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
#include <ztest.h>

#include <misc/printk.h>

#include <device.h>
#include <counter.h>


/*
 * 0 if not called, != 0 is the value of the counter
 *
 * Note to avoid 0 being posted there because the counter reached 0,
 * we'll add 1 if this is the case.
 */
static volatile uint32_t aonpt_example_callback_was_called;

static void aonpt_example_callback(struct device *dev, void *user_data)
{
	uint32_t counter;

	printk("Periodic timer callback data %p\n", user_data);
	counter = counter_read(dev);
	printk("Periodic timer callback value %u\n", counter);
	aonpt_example_callback_was_called = counter == 0 ? 1 : counter;
}

static void free_running_counter_example(void)
{
	int r;
	volatile uint32_t delay = 0;
	unsigned pt_loops = 20, i;
	uint32_t c_vals[pt_loops + 1];

	struct device *aon_counter_dev;

	printk("Always-on free running counter example app\n");

	aon_counter_dev = device_get_binding("AON_COUNTER");
	assert_not_null(aon_counter_dev, "Counter device not found\n");

	r = counter_start(aon_counter_dev);
	assert_equal(r, 0, "Counter device enable didn't return 0\n");

	/*
	 * The AON counter runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 */
	c_vals[0] = counter_read(aon_counter_dev);
	for (delay = 5000; delay--;) {
	}
	c_vals[1] = counter_read(aon_counter_dev);
	printk("Always-on counter before 5k empty loop %u / after %u\n",
	       c_vals[0], c_vals[1]);
	assert_true(c_vals[1] > c_vals[0],
		    "Always-on counter failed to increase during 5k loop");

	c_vals[0] = counter_read(aon_counter_dev);
	for (i = 1; i <= pt_loops; i++) {	/* note the i + 1 */
		for (delay = 500; delay--;) {
		}
		c_vals[i] = counter_read(aon_counter_dev);
		printk("Always-on counter before 500 empty loop %u / after %u\n",
		       c_vals[i - 1], c_vals[i]);
		assert_true(c_vals[i] > c_vals[i - 1],
			    "Always-on counter failed to increase "
			    "during 500 loop");
	}

	/*
	 * arduino 101 loader assumes the counter is running.
	 * If the counter is stopped, the next app you flash in
	 * can not start without a hard reset or power cycle.
	 * Let's leave the counter in running state.
	 */
}

static void periodic_timer_example(void)
{
	int r;
	volatile uint32_t delay = 0;
	uint32_t i = 0;
	uint32_t dummy_data = 30;
	uint32_t timer_initial_value = 10000;
	const unsigned pt_loops = 20;
	uint32_t pt_val0, pt_vals[pt_loops + 1];
	struct device *aon_periodic_timer_dev = NULL;

	printk("Periodic timer example app\n");
	aon_periodic_timer_dev = device_get_binding("AON_TIMER");
	assert_not_null(aon_periodic_timer_dev, "Timer device not found\n");

	counter_start(aon_periodic_timer_dev);
	printk("Periodic timer started\n");

	/*
	 * The AON timer runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 *
	 * Note it counts down!
	 */
	pt_val0 = counter_read(aon_periodic_timer_dev);
	for (delay = 5000; delay--;) {
	}
	pt_vals[0] = counter_read(aon_periodic_timer_dev);
	printk("Periodic timer value before 5k %u, after %u\n",
	       pt_val0, pt_vals[0]);
	assert_true(pt_vals[0] < pt_val0,
		    "timer failed to decrease in 5k empty loop");
	for (i = 1; i < pt_loops + 1; i++) {	/* note the +1 */
		for (delay = 500; delay--;) {
		}
		pt_vals[i] = counter_read(aon_periodic_timer_dev);
		printk("Periodic timer value before 500 %u, after %u\n",
		       pt_vals[i - 1], pt_vals[i]);
		assert_true(pt_vals[i] < pt_vals[i - 1],
			    "timer failed to decrease in 500 empty loop");
	}

	r = counter_set_alarm(aon_periodic_timer_dev, aonpt_example_callback,
			      timer_initial_value, (void *)&dummy_data);
	assert_equal(r, 0, "Periodic Timer was not started yet\n");

	printk("Periodic Timer alarm on\n");

	/* long delay for the alarm and callback to happen */
	for (delay = 5000000; delay--;) {
	}
	assert_not_equal(aonpt_example_callback_was_called, 0,
			 "alarm callback was not called");
	printk("Alarm callback was called with counter %u\n",
		aonpt_example_callback_was_called);

	/* callback is turned off */
	r = counter_set_alarm(aon_periodic_timer_dev, NULL,
			      timer_initial_value, (void *)&dummy_data);
	assert_equal(r, 0, "Periodic timer was not started yet\n");

	printk("Periodic timer alarm off\n");

	pt_vals[0] = counter_read(aon_periodic_timer_dev);
	for (i = 1; i < pt_loops + 1; i++) {	/* note the +1 */
		for (delay = 500; delay--;) {
		}
		pt_vals[i] = counter_read(aon_periodic_timer_dev);
		printk("Periodic timer value before 500 %u, after %u\n",
		       pt_vals[i - 1], pt_vals[i]);
		assert_true(pt_vals[i] < pt_vals[i - 1],
			    "timer failed to decrease in 500 empty loop");
	}

	counter_stop(aon_periodic_timer_dev);
}

void test_main(void)
{
	ztest_test_suite(
		aon_counter_test,
		ztest_unit_test(free_running_counter_example),
		ztest_unit_test(periodic_timer_example));
	ztest_run_test_suite(aon_counter_test);
}
