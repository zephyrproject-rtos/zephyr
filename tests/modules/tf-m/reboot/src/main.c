/*
 * Copyright (c) 2026 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Reboot from one thread while another thread keeps issuing Secure calls.
 *
 * With the TF-M Platform partition, sys_arch_reboot() is a Secure call too,
 * and sys_reboot() makes it with interrupts locked. If the TF-M NS interface
 * lock is held by the worker at that point, sys_reboot() must not end up
 * blocking on it with interrupts locked, which CONFIG_SPIN_VALIDATE turns
 * into an assertion failure.
 *
 * The worker holds the lock only while it is in the Secure world, so the
 * reboot is requested only once the worker has been observed preempted there.
 * No public API exposes the security state a thread was preempted in, so the
 * test reads the EXC_RETURN the arch stores in the thread struct.
 * A __noinit magic tells the boot following the requested reset apart from
 * the first boot.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <cmsis_core.h>

#include <psa/crypto.h>

#if !defined(CONFIG_ARM_STORE_EXC_RETURN)
#error "This test needs the EXC_RETURN of preempted threads to be stored"
#endif

#define REBOOTED_MAGIC 0x52454254

/* Survives the reset on the allowed platforms. If a run dies between setting
 * the magic and the reset, the following first boot reports a pass without
 * the preceding output and the ordered console harness fails that run.
 */
static __noinit uint32_t rebooted_magic;

static K_THREAD_STACK_DEFINE(worker_stack, 2048);
static struct k_thread worker_thread;

static void worker(void *p1, void *p2, void *p3)
{
	uint8_t buf[64];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Keep the NS interface lock taken nearly all of the time. */
	while (true) {
		(void)psa_generate_random(buf, sizeof(buf));
	}
}

static bool worker_in_secure_world(void)
{
	/* CONFIG_ARM_STORE_EXC_RETURN, selected by
	 * CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS, records the EXC_RETURN
	 * and thereby the security state a thread was preempted in.
	 */
	return (worker_thread.arch.mode_exc_return & EXC_RETURN_S) != 0;
}

int main(void)
{
	if (rebooted_magic == REBOOTED_MAGIC) {
		rebooted_magic = 0;
		printk("Back after the reboot: PASS\n");
		return 0;
	}

	printk("Starting the Secure call worker\n");

	k_thread_create(&worker_thread, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack), worker,
			NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	/* Each wake-up from the timer interrupt preempts the worker wherever it
	 * happens to be. Retry until that is inside a Secure call, so that the
	 * NS interface lock is known to be held when sys_reboot() runs.
	 */
	for (int i = 0; i < 1000 && !worker_in_secure_world(); i++) {
		k_msleep(1);
	}

	if (!worker_in_secure_world()) {
		printk("Worker never seen in the Secure world: FAIL\n");
		return 0;
	}

	printk("Worker preempted in the Secure world\n");

	rebooted_magic = REBOOTED_MAGIC;
	printk("Rebooting\n");
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}
