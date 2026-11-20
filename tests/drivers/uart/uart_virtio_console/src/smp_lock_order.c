/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

static atomic_t scheduler_locked;
static atomic_t clear_entered;
static atomic_t intercept_clear;

uint32_t __real_z_impl_k_event_clear(struct k_event *event, uint32_t events);

uint32_t __wrap_z_impl_k_event_clear(struct k_event *event, uint32_t events)
{
	if (atomic_cas(&intercept_clear, 1, 0)) {
		unsigned int key = irq_lock();
		uint32_t result;

		/* Prevent a timer ISR from entering the scheduler during the handshake. */
		atomic_set(&clear_entered, 1);
		while (atomic_get(&scheduler_locked) == 0) {
			arch_nop();
		}
		result = __real_z_impl_k_event_clear(event, events);
		irq_unlock(key);
		return result;
	}
	return __real_z_impl_k_event_clear(event, events);
}

static void yielding_sender(void *p1, void *p2, void *p3)
{
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	atomic_set(&intercept_clear, 1);
	uart_poll_out(console, 'A');
}

static void scheduler_sender(void *p1, void *p2, void *p3)
{
	extern struct k_spinlock _sched_spinlock;
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	k_spinlock_key_t key;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	while (atomic_get(&clear_entered) == 0) {
		arch_nop();
	}
	key = k_spin_lock(&_sched_spinlock);
	atomic_set(&scheduler_locked, 1);
	uart_poll_out(console, 'B');
	k_spin_unlock(&_sched_spinlock, key);
}

ZTEST(virtio_console_smp, test_scheduler_and_tx_lock_order)
{
	static K_THREAD_STACK_DEFINE(sender_stack, 2048);
	static K_THREAD_STACK_DEFINE(scheduler_stack, 2048);
	static struct k_thread sender;
	static struct k_thread scheduler;
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	zassert_true(device_is_ready(console));
	k_thread_create(&sender, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack), yielding_sender,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	k_thread_create(&scheduler, scheduler_stack, K_THREAD_STACK_SIZEOF(scheduler_stack),
			scheduler_sender, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&sender, 0));
	zassert_ok(k_thread_cpu_pin(&scheduler, 1));
	/* CPU 1 waits until CPU 0 reaches the wrapped event clear. */
	k_thread_start(&scheduler);
	k_thread_start(&sender);
	zassert_ok(k_thread_join(&sender, K_SECONDS(2)));
	zassert_ok(k_thread_join(&scheduler, K_SECONDS(2)));
	zassert_equal(atomic_get(&clear_entered), 1);
}

ZTEST_SUITE(virtio_console_smp, NULL, NULL, NULL, NULL, NULL);
