/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A sensor gateway, all in software: a sampler thread reads a simulated
 * sensor, filters the reading with a calibration factor kept in "flash" and
 * hands it to an uplink thread, which sends it over a simulated modem.
 *
 * The main thread injects the kind of faults such a device sees in the field
 * and the task watchdog detects and handles them:
 *
 * - the modem stops answering: the uplink channel times out and the uplink
 *   is restarted, without resetting the system;
 * - the uplink retries a transmission in a tight loop, still feeding its
 *   channel: feeding faster than the minimum interval is a fault as well;
 * - the filter takes too long: the checkpoint deadline of the sampler is
 *   missed long before its channel would time out;
 * - a configuration update stores a bad calibration and the sampler hangs:
 *   this is the second acquisition fault within a short time, so its group
 *   escalates to a system reset. The bad calibration survives the reset and
 *   the restarted sampler hangs again, so the recovery fails and the system
 *   is reset once more. After this second consecutive task watchdog reset the
 *   gateway boots into a safe mode, which restores the factory calibration
 *   instead of resetting forever.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/task_wdt/task_wdt.h>

#ifdef CONFIG_TASK_WDT_SAFE_MODE
#include <zephyr/retention/bootmode.h>
#endif

#define SETTINGS_MAGIC      0x47575354U
#define FACTORY_CALIBRATION 100

#define SAMPLER_PERIOD_MS  100
#define SAMPLER_RELOAD_MS  500
/* the whole acquisition must not take longer than this */
#define ACQUISITION_MAX_MS 50

#define UPLINK_PERIOD_MS   200
#define UPLINK_RELOAD_MS   1000
/* the uplink sends at most every UPLINK_PERIOD_MS, so this is a safe margin */
#define UPLINK_MIN_FEED_MS 50

#define STACK_SIZE 1024

/* Settings kept in a RAM area surviving resets, standing in for flash */
static struct {
	uint32_t magic;
	int32_t calibration;
	bool update_applied;
} settings __noinit;

enum uplink_fault {
	UPLINK_OK,
	UPLINK_MODEM_HANG,
	UPLINK_RETRY_LOOP,
};

static atomic_t uplink_fault;
static atomic_t filter_overrun;
static atomic_t readings_sent;

K_MSGQ_DEFINE(readings, sizeof(int32_t), 16, 4);
/* given by the simulated modem when it acknowledges a transmission */
static K_SEM_DEFINE(modem_ack, 0, 1);

struct worker {
	const char *name;
	k_thread_entry_t entry;
	k_thread_stack_t *stack;
	int priority;
	struct k_thread thread;
	int channel_id;
};

static void sampler_entry(void *p1, void *p2, void *p3);
static void uplink_entry(void *p1, void *p2, void *p3);

static K_THREAD_STACK_DEFINE(sampler_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(uplink_stack, STACK_SIZE);

static struct worker sampler = {
	.name = "sampler",
	.entry = sampler_entry,
	.stack = sampler_stack,
	.priority = 5,
};

static struct worker uplink = {
	.name = "uplink",
	.entry = uplink_entry,
	.stack = uplink_stack,
	.priority = 6,
};

/* The acquisition goes through these checkpoints, in this order */
enum {
	CP_ACQUIRE,
	CP_FILTER,
	CP_PUBLISH,
};

static const uint32_t acquisition_next[] = {
	[CP_ACQUIRE] = BIT(CP_FILTER),
	[CP_FILTER] = BIT(CP_PUBLISH),
	[CP_PUBLISH] = BIT(CP_ACQUIRE),
};

static const struct task_wdt_deadline acquisition_deadlines[] = {
	{.start = CP_ACQUIRE, .end = CP_PUBLISH, .max_ms = ACQUISITION_MAX_MS},
};

TASK_WDT_SUPERVISION_DEFINE(acquisition, acquisition_next, acquisition_deadlines);

static const char *fault_name(enum task_wdt_fault_reason reason)
{
	switch (reason) {
	case TASK_WDT_FAULT_TIMEOUT:
		return "timeout";
	case TASK_WDT_FAULT_TOO_EARLY:
		return "fed too early";
	case TASK_WDT_FAULT_DEADLINE_MIN:
		return "deadline too early";
	case TASK_WDT_FAULT_DEADLINE_MAX:
		return "deadline missed";
	case TASK_WDT_FAULT_SEQUENCE:
		return "out of sequence";
	default:
		return "unknown";
	}
}

static void worker_start(struct worker *w)
{
	k_thread_create(&w->thread, w->stack, STACK_SIZE, w->entry, w, NULL, NULL, w->priority, 0,
			K_NO_WAIT);
	k_thread_name_set(&w->thread, w->name);
}

/*
 * Recovery function of both groups, called from the task watchdog recovery
 * work queue. It restarts the faulty worker, which ends the recovery by feeding
 * its channel again. If the restarted worker fails again before, the system
 * is reset.
 */
static void restart_worker(const struct task_wdt_fault *fault, void *user_data)
{
	struct worker *w = user_data;

	printk("Recovery: restarting %s (%s)\n", w->name, fault_name(fault->reason));

	k_thread_abort(&w->thread);
	worker_start(w);
}

/* A network outage is expected from time to time: always restart the uplink */
TASK_WDT_GROUP_DEFINE(network, TASK_WDT_ACTION_RECOVER, restart_worker, NULL);

/*
 * The acquisition is not expected to fail: if it fails twice within 10 s,
 * restarting the sampler does not help, so reset the whole system.
 */
static const struct task_wdt_escalation acquisition_escalation = {
	.faults = 2,
	.period_ms = 10000,
};

TASK_WDT_GROUP_DEFINE(acquisition_group, TASK_WDT_ACTION_RECOVER, restart_worker,
		      &acquisition_escalation);

static int32_t sensor_read(void)
{
	static uint32_t count;

	/* a slowly rising and falling temperature, in hundredths of a degree */
	count++;
	return 2000 + (int32_t)(count % 100 < 50 ? count % 50 : 50 - count % 50) * 10;
}

static int32_t filter(int32_t raw)
{
	if (atomic_cas(&filter_overrun, 1, 0)) {
		/* a glitch on the sensor bus makes the filter retry */
		k_msleep(4 * ACQUISITION_MAX_MS);
	}

	/* a bad calibration is never fixed by waiting, so the sampler hangs here */
	while (settings.calibration == 0) {
		k_msleep(10);
	}

	return raw * FACTORY_CALIBRATION / settings.calibration;
}

static void sampler_entry(void *p1, void *p2, void *p3)
{
	struct worker *w = p1;
	int32_t value;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		task_wdt_checkpoint(&acquisition, CP_ACQUIRE);
		value = sensor_read();

		task_wdt_checkpoint(&acquisition, CP_FILTER);
		value = filter(value);

		task_wdt_checkpoint(&acquisition, CP_PUBLISH);
		(void)k_msgq_put(&readings, &value, K_NO_WAIT);

		task_wdt_feed(w->channel_id);
		k_msleep(SAMPLER_PERIOD_MS);
	}
}

static void modem_send(struct worker *w, int32_t value)
{
	ARG_UNUSED(value);

	switch (atomic_set(&uplink_fault, UPLINK_OK)) {
	case UPLINK_MODEM_HANG:
		/* the modem never acknowledges the transmission */
		k_sem_take(&modem_ack, K_FOREVER);
		break;
	case UPLINK_RETRY_LOOP:
		/* a buggy retry loop which keeps feeding its channel */
		for (;;) {
			task_wdt_feed(w->channel_id);
			k_msleep(5);
		}
		break;
	default:
		break;
	}

	atomic_inc(&readings_sent);
}

static void uplink_entry(void *p1, void *p2, void *p3)
{
	struct worker *w = p1;
	int32_t value;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		/* send the readings in batches, feeding the channel once per batch */
		k_msleep(UPLINK_PERIOD_MS);

		while (k_msgq_get(&readings, &value, K_NO_WAIT) == 0) {
			modem_send(w, value);
		}

		task_wdt_feed(w->channel_id);
	}
}

static int worker_watch(struct worker *w, uint32_t reload_ms, struct task_wdt_group *group)
{
	w->channel_id = task_wdt_add_ext(reload_ms, NULL, w);
	if (w->channel_id < 0) {
		return w->channel_id;
	}

	(void)task_wdt_channel_thread_set(w->channel_id, &w->thread);

	return task_wdt_channel_group_set(w->channel_id, group);
}

static const char *reset_name(enum task_wdt_reset_reason reason)
{
	switch (reason) {
	case TASK_WDT_RESET_TIMEOUT:
		return "timeout";
	case TASK_WDT_RESET_TOO_EARLY:
		return "fed too early";
	case TASK_WDT_RESET_DEADLINE_MIN:
		return "deadline too early";
	case TASK_WDT_RESET_DEADLINE_MAX:
		return "deadline missed";
	case TASK_WDT_RESET_SEQUENCE:
		return "out of sequence";
	default:
		return "hardware watchdog";
	}
}

static bool safe_mode_requested(const struct task_wdt_reset_info *info, bool valid)
{
#ifdef CONFIG_TASK_WDT_SAFE_MODE
	ARG_UNUSED(info);
	ARG_UNUSED(valid);

	return bootmode_check(CONFIG_TASK_WDT_SAFE_MODE_BOOT_MODE) == 1;
#else
	return valid && info->fault_count >= 2U;
#endif
}

static void safe_mode(void)
{
	printk("Safe mode: the gateway keeps resetting, the acquisition is not started\n");

	/* a real device would report the fault and wait for a fixed update */
	settings.calibration = FACTORY_CALIBRATION;
	printk("Safe mode: restored the factory calibration\n");

#ifdef CONFIG_TASK_WDT_SAFE_MODE
	(void)bootmode_clear();
#endif
	(void)task_wdt_reset_info_clear();

	printk("Safe mode: rebooting into the normal mode\n");
	sys_reboot(SYS_REBOOT_COLD);
}

/* Inject the faults of the field, once the gateway was running for a while */
static void inject_faults(void)
{
	k_sleep(K_SECONDS(1));
	printk("Injecting: the modem stops answering\n");
	atomic_set(&uplink_fault, UPLINK_MODEM_HANG);

	k_sleep(K_SECONDS(2));
	printk("Injecting: the uplink retries a transmission forever\n");
	atomic_set(&uplink_fault, UPLINK_RETRY_LOOP);

	k_sleep(K_SECONDS(2));
	printk("Injecting: a sensor bus glitch slows down the filter\n");
	atomic_set(&filter_overrun, 1);

	k_sleep(K_SECONDS(2));
	printk("Injecting: a configuration update with a bad calibration\n");
	settings.update_applied = true;
	settings.calibration = 0;
}

int main(void)
{
	struct task_wdt_reset_info info;
	bool info_valid;
	int ret;

	printk("Sensor gateway sample\n");

	ret = task_wdt_init(NULL);
	if (ret != 0) {
		printk("task_wdt_init() failed: %d\n", ret);
		return 0;
	}

	if (settings.magic != SETTINGS_MAGIC) {
		printk("First boot, using the factory calibration\n");
		settings.magic = SETTINGS_MAGIC;
		settings.calibration = FACTORY_CALIBRATION;
		settings.update_applied = false;
	}

	info_valid = task_wdt_reset_info_get(&info) == 0;
	if (info_valid) {
		printk("Last reset: task watchdog %s, thread %s, %u consecutive\n",
		       reset_name(info.reason), info.thread_name, info.fault_count);
	} else {
		printk("Last reset: not caused by the task watchdog\n");
	}

	if (safe_mode_requested(&info, info_valid)) {
		safe_mode();
		return 0;
	}

	if (worker_watch(&sampler, SAMPLER_RELOAD_MS, &acquisition_group) != 0 ||
	    task_wdt_supervision_start(&acquisition, sampler.channel_id) != 0 ||
	    worker_watch(&uplink, UPLINK_RELOAD_MS, &network) != 0 ||
	    task_wdt_window_set(uplink.channel_id, UPLINK_MIN_FEED_MS) != 0) {
		printk("Failed to set up the task watchdog\n");
		return 0;
	}

	worker_start(&sampler);
	worker_start(&uplink);

	if (!settings.update_applied) {
		inject_faults();
	}

	for (;;) {
		k_sleep(K_SECONDS(2));
		printk("Gateway running normally, %ld readings sent\n", atomic_get(&readings_sent));
	}

	return 0;
}
