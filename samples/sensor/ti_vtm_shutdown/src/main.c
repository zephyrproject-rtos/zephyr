/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_vtm.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/reboot.h>
#include <stdio.h>

#define CRIT_TRIG_TYPE ((enum sensor_trigger_type)TI_VTM_TRIG_TH2)
#define CRIT_ATTR      ((enum sensor_attribute)TI_VTM_ATTR_TH2_THRESH)

static const struct device *const vtm_temp = DEVICE_DT_GET(DT_NODELABEL(sensor0));

static const struct sensor_trigger crit_trig = {
	.type = CRIT_TRIG_TYPE,
	.chan = SENSOR_CHAN_DIE_TEMP,
};

BUILD_ASSERT(CONFIG_APP_TEST_BELOW_MC != 0 ||
	     CONFIG_APP_CRITICAL_MC < DT_PROP(DT_PARENT(DT_NODELABEL(sensor0)), ti_max_reset_mc),
	     "APP_CRITICAL_MC must stay below ti,max-reset-mc");

#define SHUTDOWN_STACK_SIZE 2048

static atomic_t crit_seen;
static K_SEM_DEFINE(shutdown_sem, 0, 1);
static K_THREAD_STACK_DEFINE(shutdown_stack, SHUTDOWN_STACK_SIZE);
static struct k_thread shutdown_thread;

static int32_t val_to_mc(const struct sensor_value *val)
{
	return val->val1 * 1000 + val->val2 / 1000;
}

static void mc_to_val(int32_t temp_mc, struct sensor_value *val)
{
	val->val1 = temp_mc / 1000;
	val->val2 = (temp_mc % 1000) * 1000;
}

static int read_temp_mc(int32_t *temp_mc)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch_chan(vtm_temp, SENSOR_CHAN_DIE_TEMP);
	if (ret == 0) {
		ret = sensor_channel_get(vtm_temp, SENSOR_CHAN_DIE_TEMP, &val);
	}
	if (ret == 0) {
		*temp_mc = val_to_mc(&val);
	}

	return ret;
}

static void print_mc(const char *prefix, int32_t mc, const char *suffix)
{
	uint32_t abs_mc = (mc < 0) ? -(uint32_t)mc : (uint32_t)mc;

	printf("%s%s%u.%03u C%s", prefix, (mc < 0) ? "-" : "", abs_mc / 1000U, abs_mc % 1000U,
	       suffix);
}

static void shutdown_thread_fn(void *p1, void *p2, void *p3)
{
	int32_t temp_mc;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&shutdown_sem, K_FOREVER);

	if (read_temp_mc(&temp_mc) == 0) {
		print_mc("VTM-SHUTDOWN: critical temperature, T=", temp_mc, "\n");
	} else {
		printf("VTM-SHUTDOWN: critical temperature, T unreadable\n");
	}

	if (IS_ENABLED(CONFIG_APP_ACTION_REBOOT)) {
		printf("VTM-SHUTDOWN: rebooting\n");
		sys_reboot(SYS_REBOOT_COLD);
	}

	printf("VTM-SHUTDOWN: action is log only\n");
}

static void crit_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(trig);

	/* The alert is a level: disarm before it asserts the line again */
	(void)sensor_trigger_set(dev, &crit_trig, NULL);

	if (atomic_cas(&crit_seen, 0, 1)) {
		k_sem_give(&shutdown_sem);
	}
}

static int start_shutdown_thread(void)
{
	k_tid_t tid = k_thread_create(&shutdown_thread, shutdown_stack,
				      K_THREAD_STACK_SIZEOF(shutdown_stack), shutdown_thread_fn,
				      NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0,
				      K_FOREVER);

#ifdef CONFIG_SCHED_CPU_MASK
	int ret = k_thread_cpu_pin(tid, 0);

	if (ret < 0) {
		return ret;
	}
#endif
	k_thread_start(tid);

	return 0;
}

static int arm_critical(int32_t temp_mc)
{
	struct sensor_value val;
	int32_t crit_mc = CONFIG_APP_CRITICAL_MC;
	int ret;

	if (CONFIG_APP_TEST_BELOW_MC != 0) {
		crit_mc = temp_mc - CONFIG_APP_TEST_BELOW_MC;
	}

	mc_to_val(crit_mc, &val);
	/* Threshold first: the trigger enable may assert at once */
	ret = sensor_attr_set(vtm_temp, SENSOR_CHAN_DIE_TEMP, CRIT_ATTR, &val);
	if (ret < 0) {
		printf("VTM-TRIP: threshold set failed (%d)\n", ret);
		return ret;
	}

	ret = sensor_attr_get(vtm_temp, SENSOR_CHAN_DIE_TEMP, CRIT_ATTR, &val);
	if (ret < 0) {
		printf("VTM-TRIP: threshold readback failed (%d)\n", ret);
		return ret;
	}
	print_mc("VTM-TRIP: critical threshold programmed ", crit_mc, "");
	print_mc(", reads back ", val_to_mc(&val), "\n");

	ret = sensor_trigger_set(vtm_temp, &crit_trig, crit_handler);
	if (ret < 0) {
		printf("VTM-TRIP: trigger set failed (%d)\n", ret);
	}

	return ret;
}

int main(void)
{
	int32_t temp_mc;
	int ret;

	if (!device_is_ready(vtm_temp)) {
		printf("VTM-TRIP: FAIL sensor0 not ready\n");
		return 0;
	}

	ret = read_temp_mc(&temp_mc);
	if (ret < 0) {
		printf("VTM-TRIP: FAIL temperature read (%d)\n", ret);
		return 0;
	}
	print_mc("VTM-TRIP: die temperature ", temp_mc, "\n");

	ret = start_shutdown_thread();
	if (ret < 0) {
		printf("VTM-TRIP: FAIL shutdown thread (%d)\n", ret);
		return 0;
	}

	k_sleep(K_SECONDS(CONFIG_APP_ARM_DELAY_S));

	if (arm_critical(temp_mc) < 0) {
		printf("VTM-TRIP: FAIL arming\n");
		return 0;
	}

	if (CONFIG_APP_TEST_BELOW_MC != 0) {
		int64_t t0 = k_uptime_get();

		while (!atomic_get(&crit_seen) &&
		       k_uptime_get() - t0 < CONFIG_APP_TEST_TIMEOUT_MS) {
			k_msleep(10);
		}

		if (atomic_get(&crit_seen)) {
			printf("VTM-TRIP: PASS trigger fired after %d ms\n",
			       (int)(k_uptime_get() - t0));
		} else {
			printf("VTM-TRIP: FAIL no trigger within %d ms\n",
			       CONFIG_APP_TEST_TIMEOUT_MS);
		}
	}

	while (1) {
		if (read_temp_mc(&temp_mc) == 0) {
			print_mc("VTM-TRIP: T=", temp_mc, "\n");
		}
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
