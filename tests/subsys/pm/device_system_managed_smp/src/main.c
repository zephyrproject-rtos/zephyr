/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>
#include <kernel_internal.h>

/*
 * Regression test for system-managed device power management on SMP.
 *
 * Devices may only be suspended when the last CPU enters sleep and every
 * sleeping CPU selected a power state that allows device suspension. They
 * must be resumed by the first CPU waking up, whichever CPU suspended them,
 * so that no CPU ever runs while devices are suspended.
 *
 * The test drives pm_system_suspend() directly from two threads pinned to
 * different CPUs, making the sleep and wake order fully deterministic. The
 * pm_state_set() hook provided here stands in for the architecture sleep:
 * it spins until the CPU is signaled to wake up. The first CPU to sleep is
 * always woken up first, while the other CPU is still sleeping.
 */

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS == 2, "Invalid number of cpus");

#define FIRST_CPU  0
#define SECOND_CPU 1

/* Sleeps and wake-ups are signal-driven, this is only a stuck-test guard. */
#define JOIN_TIMEOUT K_SECONDS(10)

static atomic_t cpu_asleep[CONFIG_MP_MAX_NUM_CPUS];
static atomic_t cpu_wake[CONFIG_MP_MAX_NUM_CPUS];

static atomic_t suspend_count;
static atomic_t resume_count;

static const struct device *const test_dev = DEVICE_DT_GET(DT_NODELABEL(test_dev));

static int test_dev_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		atomic_inc(&suspend_count);
		break;
	case PM_DEVICE_ACTION_RESUME:
		atomic_inc(&resume_count);
		break;
	default:
		break;
	}

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), test_dev_pm_action);

DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), NULL, PM_DEVICE_DT_GET(DT_NODELABEL(test_dev)), NULL, NULL,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	uint8_t cpu = _current_cpu->id;
	uint8_t peer = 1U - cpu;

	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	atomic_set(&cpu_asleep[cpu], 1);

	/*
	 * Last CPU to sleep: the device suspend decision has been taken by
	 * now, wake the first sleeper up. It wakes this CPU back once it has
	 * fully resumed and run its checks.
	 */
	if (atomic_get(&cpu_asleep[peer]) == 1) {
		atomic_set(&cpu_wake[peer], 1);
	}

	while (atomic_get(&cpu_wake[cpu]) == 0) {
	}

	atomic_set(&cpu_wake[cpu], 0);
	atomic_set(&cpu_asleep[cpu], 0);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

/* Idle CPUs take no PM action: system suspend is driven by the test threads. */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return NULL;
}

struct sleeper {
	uint8_t cpu;
	enum pm_state state;
	bool sleeps_first;
};

static void sleeper_fn(void *p1, void *p2, void *p3)
{
	const struct sleeper *s = p1;
	const struct pm_state_info info = {.state = s->state};
	enum pm_device_state dev_state;
	unsigned int key;
	bool suspended;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!s->sleeps_first) {
		/* Fix the sleep order: enter sleep after the other CPU. */
		while (atomic_get(&cpu_asleep[1U - s->cpu]) == 0) {
		}
	}

	zassert_true(pm_state_force(s->cpu, &info), "unable to force state %d", s->state);

	/*
	 * pm_system_suspend() runs with interrupts locked, as in the idle
	 * thread. Lock this CPU interrupts only: irq_lock() would take the
	 * SMP global lock, on which the other sleeping CPU spins.
	 */
	key = arch_irq_lock();
	suspended = pm_system_suspend(K_TICKS_FOREVER);
	arch_irq_unlock(key);

	zassert_true(suspended, "system suspend aborted");

	if (s->sleeps_first) {
		/*
		 * First CPU to wake up, while the other CPU still sleeps.
		 * Whatever power states are involved, devices must be
		 * functional before any CPU leaves pm_system_suspend().
		 */
		zassert_ok(pm_device_state_get(test_dev, &dev_state));
		zassert_equal(dev_state, PM_DEVICE_STATE_ACTIVE,
			      "CPU running while devices are suspended");

		/* Release the other CPU. */
		atomic_set(&cpu_wake[1U - s->cpu], 1);
	}
}

static K_THREAD_STACK_DEFINE(first_stack, 2048);
static K_THREAD_STACK_DEFINE(second_stack, 2048);
static struct k_thread first_thread;
static struct k_thread second_thread;

static void run_scenario(enum pm_state first_cpu_state, enum pm_state second_cpu_state)
{
	static struct sleeper first, second;
	enum pm_device_state dev_state;
	k_tid_t tid_first, tid_second;

	atomic_set(&suspend_count, 0);
	atomic_set(&resume_count, 0);

	first = (struct sleeper){
		.cpu = FIRST_CPU,
		.state = first_cpu_state,
		.sleeps_first = true,
	};
	second = (struct sleeper){
		.cpu = SECOND_CPU,
		.state = second_cpu_state,
		.sleeps_first = false,
	};

	tid_first =
		k_thread_create(&first_thread, first_stack, K_THREAD_STACK_SIZEOF(first_stack),
				sleeper_fn, &first, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	tid_second =
		k_thread_create(&second_thread, second_stack, K_THREAD_STACK_SIZEOF(second_stack),
				sleeper_fn, &second, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);

	zassert_ok(k_thread_cpu_pin(tid_first, FIRST_CPU));
	zassert_ok(k_thread_cpu_pin(tid_second, SECOND_CPU));

	k_thread_start(tid_first);
	k_thread_start(tid_second);

	zassert_ok(k_thread_join(tid_first, JOIN_TIMEOUT), "first sleeper stuck");
	zassert_ok(k_thread_join(tid_second, JOIN_TIMEOUT), "second sleeper stuck");

	zassert_ok(pm_device_state_get(test_dev, &dev_state));
	zassert_equal(dev_state, PM_DEVICE_STATE_ACTIVE,
		      "devices left suspended after all CPUs resumed");
}

/*
 * One CPU sleeps in a state that does not allow device suspension, then the
 * other CPU sleeps in a state that does. Devices must not be suspended: the
 * decision belongs to all sleeping CPUs, not only to the last one. The
 * non-permitting CPU also wakes up first: if devices had been wrongly
 * suspended, its own state would not have resumed them.
 */
ZTEST(pm_device_system_managed_smp, test_mixed_states_runtime_idle_first)
{
	run_scenario(PM_STATE_RUNTIME_IDLE, PM_STATE_SUSPEND_TO_IDLE);

	zassert_equal(atomic_get(&suspend_count), 0, "devices were suspended");
	zassert_equal(atomic_get(&resume_count), 0, "devices were resumed");
}

/* Same as above with the zephyr,pm-device-disabled state property. */
ZTEST(pm_device_system_managed_smp, test_mixed_states_pm_device_disabled_first)
{
	run_scenario(PM_STATE_STANDBY, PM_STATE_SUSPEND_TO_IDLE);

	zassert_equal(atomic_get(&suspend_count), 0, "devices were suspended");
	zassert_equal(atomic_get(&resume_count), 0, "devices were resumed");
}

/* Reversed sleep order: the non-permitting CPU is the last one to sleep. */
ZTEST(pm_device_system_managed_smp, test_mixed_states_runtime_idle_last)
{
	run_scenario(PM_STATE_SUSPEND_TO_IDLE, PM_STATE_RUNTIME_IDLE);

	zassert_equal(atomic_get(&suspend_count), 0, "devices were suspended");
	zassert_equal(atomic_get(&resume_count), 0, "devices were resumed");
}

/*
 * Every CPU sleeps in a state that allows device suspension: devices are
 * suspended by the last CPU entering sleep and resumed by the first CPU
 * waking up, which is not the CPU that suspended them.
 */
ZTEST(pm_device_system_managed_smp, test_all_states_allow_device_suspend)
{
	run_scenario(PM_STATE_SUSPEND_TO_IDLE, PM_STATE_SUSPEND_TO_IDLE);

	zassert_equal(atomic_get(&suspend_count), 1, "devices were not suspended");
	zassert_equal(atomic_get(&resume_count), 1, "devices were not resumed");
}

static void *setup(void)
{
	zassert_true(device_is_ready(test_dev), "test device not ready");
	return NULL;
}

ZTEST_SUITE(pm_device_system_managed_smp, NULL, setup, NULL, NULL, NULL);
