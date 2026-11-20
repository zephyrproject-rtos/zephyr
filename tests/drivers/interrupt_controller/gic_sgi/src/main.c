/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/ztest.h>

#define TEST_SGI 15U

struct gic_sgi_fixture {
	struct k_sem received;
	atomic_t receiver_cpu;
};

static void sgi_handler(const void *arg)
{
	struct gic_sgi_fixture *fixture = (struct gic_sgi_fixture *)arg;

	atomic_set(&fixture->receiver_cpu, arch_curr_cpu()->id);
	k_sem_give(&fixture->received);
}

static void *gic_sgi_setup(void)
{
	static struct gic_sgi_fixture fixture;

	zassert_ok(k_sem_init(&fixture.received, 0U, 1U));
	IRQ_CONNECT(TEST_SGI, IRQ_DEFAULT_PRIORITY, sgi_handler, &fixture, 0);
	irq_enable(TEST_SGI);

	return &fixture;
}

static void assert_sgi_rejected(unsigned int sgi_id, uint64_t affinity, int expected_status)
{
	unsigned int key = irq_lock();
	int status = gic_raise_sgi_by_affinity(sgi_id, affinity);
	bool pending;

	/* Observe pending state before an incorrectly routed local SGI can be handled. */
	barrier_dsync_fence_full();
	pending = arm_gic_irq_is_pending(TEST_SGI);
	irq_unlock(key);

	zassert_equal(status, expected_status, "Unexpected SGI status: %d", status);
	zassert_false(pending, "Rejected request left a local SGI pending");
}

ZTEST_F(gic_sgi, test_local_affinity)
{
	uint64_t affinity = MPIDR_TO_CORE(GET_MPIDR());

	zassert_ok(gic_raise_sgi_by_affinity(TEST_SGI, affinity));
	zassert_ok(k_sem_take(&fixture->received, K_MSEC(100)), "Local SGI was not delivered");
}

ZTEST_F(gic_sgi, test_mpidr_flags)
{
	uint64_t mpidr = MPIDR_TO_CORE(GET_MPIDR()) | BIT64(24) | BIT64(30) | BIT64(31);

	zassert_ok(gic_raise_sgi_by_affinity(TEST_SGI, mpidr));
	zassert_ok(k_sem_take(&fixture->received, K_MSEC(100)), "Local SGI was not delivered");
}

ZTEST(gic_sgi, test_remote_affinity)
{
	uint64_t affinity = MPIDR_TO_CORE(GET_MPIDR());

	for (uint32_t shift = 0U; shift <= 16U; shift += 8U) {
		assert_sgi_rejected(TEST_SGI, affinity ^ BIT64(shift + 7U), -ENOENT);
	}

	if (IS_ENABLED(CONFIG_ARM64)) {
		assert_sgi_rejected(TEST_SGI, affinity ^ BIT64(39), -ENOENT);
	}
}

ZTEST(gic_sgi, test_invalid_sgi_id)
{
	uint64_t affinity = MPIDR_TO_CORE(GET_MPIDR());

	assert_sgi_rejected(TEST_SGI | BIT(4), affinity, -EINVAL);
	assert_sgi_rejected(UINT32_MAX, affinity, -EINVAL);
}

#ifdef CONFIG_SMP
struct cpu_target {
	uint64_t affinity;
	uint8_t mask;
};

struct sgi_request {
	uint64_t affinity;
	int status;
};

static void run_on_cpu(uint32_t cpu, k_thread_entry_t entry, void *arg)
{
	static K_THREAD_STACK_DEFINE(stack, 1024);
	static struct k_thread thread;

	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), entry, arg, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&thread, cpu));
	k_thread_start(&thread);
	zassert_ok(k_thread_join(&thread, K_SECONDS(1)));
}

static void read_cpu_target(void *arg, void *unused1, void *unused2)
{
	struct cpu_target *target = arg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	target->affinity = MPIDR_TO_CORE(GET_MPIDR());
	target->mask = sys_read8(GICD_ITARGETSRn);
}

static void send_sgi(void *arg, void *unused1, void *unused2)
{
	struct sgi_request *request = arg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	request->status = gic_raise_sgi_by_affinity(TEST_SGI, request->affinity);
}

ZTEST_F(gic_sgi, test_remote_sgi_delivery)
{
	for (uint32_t cpu = 0U; cpu < arch_num_cpus(); cpu++) {
		struct cpu_target target;
		struct sgi_request request;

		run_on_cpu(cpu, read_cpu_target, &target);
		request.affinity = target.affinity;
		atomic_set(&fixture->receiver_cpu, -1);
		run_on_cpu((cpu + 1U) % arch_num_cpus(), send_sgi, &request);
		zassert_ok(request.status);
		zassert_ok(k_sem_take(&fixture->received, K_MSEC(100)),
			   "Remote SGI was not delivered");
		zassert_equal(atomic_get(&fixture->receiver_cpu), cpu, "SGI reached the wrong CPU");
	}
}

ZTEST(gic_sgi, test_spi_target_union)
{
	uint8_t expected = 0U;
	uint32_t irq_count = MIN(((sys_read32(GICD_TYPER) & 0x1fU) + 1U) * 32U, 1020U);

	for (uint32_t cpu = 0U; cpu < arch_num_cpus(); cpu++) {
		struct cpu_target target;

		run_on_cpu(cpu, read_cpu_target, &target);
		zassert_not_equal(target.mask, 0U);
		expected |= target.mask;
	}

	for (uint32_t irq = GIC_SPI_INT_BASE; irq < irq_count; irq++) {
		zassert_equal(sys_read8(GICD_ITARGETSRn + irq), expected,
			      "SPI %u does not target all initialized CPUs", irq);
	}
}
#endif

ZTEST_SUITE(gic_sgi, NULL, gic_sgi_setup, NULL, NULL, NULL);
