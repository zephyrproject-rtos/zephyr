/*
 * Copyright (c) Qualcomm Technologies, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * APLIC direct delivery mode tests.
 *
 * Only built when CONFIG_RISCV_APLIC_DIRECT is enabled (see CMakeLists.txt);
 */

#include <stdlib.h>

#include <zephyr/arch/arch_inlines.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic_direct.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define APLIC_NODE         DT_NODELABEL(aplic)
#define APLIC_BASE         DT_REG_ADDR(APLIC_NODE)
#define APLIC_MAX_PRIORITY DT_PROP(APLIC_NODE, riscv_max_priority)
#define APLIC_NUM_SOURCES  DT_PROP(APLIC_NODE, riscv_num_sources)

#define TEST_IRQ_ID_INVALID 0 /**< Invalid ID per AIA specification */

#define TEST_IRQ_ID    9 /**< First IRQ not used in DT */
#define TEST_IRQ_ID_L2 (IRQ_TO_L2(TEST_IRQ_ID) | CONFIG_2ND_LVL_INTR_00_OFFSET)

#define TEST_IRQ_PRIORITY 1

static struct k_sem isr_sem;      /**< Semaphore used to block until an ISR fires */
static volatile uint32_t hart_id; /**< Captured hart ID set inside the ISR */

/*
 * =====================================================================
 *   Helper functions
 * =====================================================================
 */

static void get_hart_id_isr(const void *arg)
{
	hart_id = arch_proc_id();
	k_sem_give(&isr_sem);
}

static int trigger_aplic_irq(uint32_t irq_id, uint32_t hart)
{
	const uint32_t aplic_setipnum_address = APLIC_BASE + APLIC_SETIPNUM;

	sys_write32(irq_id, aplic_setipnum_address);

	int ret = k_sem_take(&isr_sem, K_SECONDS(1));

	if (ret != 0) {
		const uint32_t topi_address = APLIC_BASE + aplic_topi_off(hart);
		const uint32_t topi_value = sys_read32(topi_address);

		printk("An error occurred while triggering IRQ %u. ret = %d, topi = %u\n", irq_id,
		       ret, topi_value);
	}

	return ret;
}

static uint32_t read_claimi_address(uint32_t hart)
{
	const uint32_t claimi_address = APLIC_BASE + aplic_claimi_off(hart);

	return sys_read32(claimi_address);
}

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *pEsf)
{
	/* Used in test_aplic_spurious_interrupt to verify conditions of spurious interrupt */

	ARG_UNUSED(pEsf);

	zassert_equal(reason, K_ERR_SPURIOUS_IRQ, "Expected spurious IRQ, got reason %u", reason);

	/* Expect IRQ 0 since aplic_irq_handler is invoked with a clear claimi register */
	zassert_equal(0, riscv_aplic_get_saved_irq(), "Expected IRQ ID %d, got %u", 0,
		      riscv_aplic_get_saved_irq());
}

#ifndef CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY
int riscv_aplic_irq_set_affinity(const struct device *dev, uint32_t irq, uint32_t hart)
{
	/*
	 * Define stub if IRQ affinity is not configured.
	 * Tests that intend to verify affinity behavior are skipped if not configured;
	 * this stub allows the tests to compile without checking if
	 * CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY is defined across the entire suite
	 */
	return 0;
}

int riscv_aia_route_to_hart(uint32_t irq, uint32_t hart)
{
	/*
	 * Define stub if IRQ affinity is not configured.
	 * Tests that intend to verify affinity behavior are skipped if not configured;
	 * this stub allows the tests to compile without checking if
	 * CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY is defined across the entire suite
	 */
	return 0;
}
#endif

/*
 * =====================================================================
 *   Initialize Test Suite
 * =====================================================================
 */

static void *aplic_direct_suite_setup(void)
{
	k_sem_init(&isr_sem, 0, 1);

	irq_connect_dynamic(TEST_IRQ_ID_L2, TEST_IRQ_PRIORITY, get_hart_id_isr, NULL,
			    APLIC_SM_EDGE_RISE);

	return NULL;
}

static void aplic_direct_suite_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	irq_disconnect_dynamic(TEST_IRQ_ID_L2, TEST_IRQ_PRIORITY, get_hart_id_isr, NULL,
			       APLIC_SM_EDGE_RISE);
}

static void aplic_direct_before_test_setup(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&isr_sem);

	hart_id = 0xDEADBEEFU;

	irq_enable(TEST_IRQ_ID_L2);
}

static void aplic_direct_after_test_setup(void *fixture)
{
	ARG_UNUSED(fixture);

	ztest_set_fault_valid(false);

	/* Ensure any extraneous interrupts are cleared between tests */
	for (uint32_t hart = 0; hart < arch_num_cpus(); hart++) {
		read_claimi_address(hart);
	}

	/* Set interrupt to route to Hart 0 with priority 1 (common case) */
	riscv_aia_route_to_hart(TEST_IRQ_ID_L2, 0);
	riscv_aia_set_priority(TEST_IRQ_ID_L2, 1);

	irq_disable(TEST_IRQ_ID_L2);
}

ZTEST_SUITE(aplic_direct, NULL, aplic_direct_suite_setup, aplic_direct_before_test_setup,
	    aplic_direct_after_test_setup, aplic_direct_suite_teardown);

/*
 * =====================================================================
 *   APLIC Functionality Unit Tests
 * =====================================================================
 */

ZTEST(aplic_direct, test_aplic_direct_basic_isr_sequence)
{
	trigger_aplic_irq(TEST_IRQ_ID, 0);

	zassert_equal(hart_id, 0, "Expected hart 0 but ISR ran on hart %u", hart_id);
}

ZTEST(aplic_direct, test_aplic_direct_saved_device_and_irq_saved)
{
	trigger_aplic_irq(TEST_IRQ_ID, 0);

	zassert_equal(riscv_aplic_get_dev(), riscv_aplic_get_saved_dev(),
		      "Expected device %p, Actual device: %p", riscv_aplic_get_dev(),
		      riscv_aplic_get_saved_dev());
	zassert_equal(TEST_IRQ_ID, riscv_aplic_get_saved_irq(), "Expected IRQ %x, Actual IRQ: %x",
		      TEST_IRQ_ID, riscv_aplic_get_saved_irq());
}

ZTEST(aplic_direct, test_aplic_direct_irq_received_on_secondary_core)
{
	if (!IS_ENABLED(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)) {
		ztest_test_skip();
	}

	riscv_aia_route_to_hart(TEST_IRQ_ID_L2, 1);

	trigger_aplic_irq(TEST_IRQ_ID, 1);

	zassert_equal(hart_id, 1, "Expected hart 1 but ISR ran on hart %u", hart_id);
}

ZTEST(aplic_direct, test_aplic_direct_priority_within_config_range)
{
	const uint32_t aplic_min_priority = 0;

	const struct device *aplic = riscv_aplic_get_dev();

	for (uint32_t priority = aplic_min_priority; priority < (APLIC_MAX_PRIORITY + 1);
	     priority++) {
		int ret = riscv_aplic_set_priority(aplic, TEST_IRQ_ID, priority);

		zassert_equal(ret, 0, "Expected IRQ priority configuration to succeed");

		trigger_aplic_irq(TEST_IRQ_ID, 0);

		zassert_not_equal(hart_id, 0xDEADBEEF, "Expected ISR to update Hart ID");
	}
}

ZTEST(aplic_direct, test_aplic_direct_priority_outside_of_config_range)
{
	const struct device *aplic = riscv_aplic_get_dev();

	int ret = riscv_aplic_set_priority(aplic, TEST_IRQ_ID_L2, (APLIC_MAX_PRIORITY + 1));

	zassert_not_equal(ret, 0, "Expected IRQ priority configuration to fail");
}

ZTEST(aplic_direct, test_aplic_direct_random_hart_id)
{
	if (!IS_ENABLED(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)) {
		ztest_test_skip();
	}

	const uint32_t num_attempts = 1000;

	for (uint32_t attempts = 0; attempts < num_attempts; ++attempts) {
		const uint32_t random_hart = sys_rand32_get() % arch_num_cpus();

		hart_id = 0xDEADBEEFU;

		riscv_aia_route_to_hart(TEST_IRQ_ID_L2, random_hart);

		trigger_aplic_irq(TEST_IRQ_ID, random_hart);

		zassert_equal(hart_id, random_hart, "Expected hart %u. Actual: hart %u\n",
			      random_hart, hart_id);
	}
}

ZTEST(aplic_direct, test_aplic_direct_random_priority)
{
	const uint32_t num_attempts = 1000;
	const uint32_t aplic_min_priority = 1;
	const uint32_t expected_hart_id = 0;
	const struct device *aplic = riscv_aplic_get_dev();

	for (uint32_t attempts = 0; attempts < num_attempts; ++attempts) {
		const uint32_t random_priority =
			sys_rand32_get() % (APLIC_MAX_PRIORITY - aplic_min_priority) +
			aplic_min_priority;

		hart_id = 0xDEADBEEFU;

		int ret = riscv_aplic_set_priority(aplic, TEST_IRQ_ID, random_priority);

		zassert_equal(ret, 0, "Expected IRQ priority configuration to succeed");

		trigger_aplic_irq(TEST_IRQ_ID, expected_hart_id);

		zassert_equal(hart_id, expected_hart_id, "Expected hart %u. Actual: hart %u\n",
			      expected_hart_id, hart_id);
	}
}

ZTEST(aplic_direct, test_aplic_direct_verify_interrupt_enabled)
{
	int ret = riscv_aia_irq_is_enabled(TEST_IRQ_ID_L2);

	zassert_equal(ret, 1, "Expected test IRQ to be enabled");
}

ZTEST(aplic_direct, test_aplic_direct_verify_interrupt_disabled)
{
	int ret = -1;

	/* Out of bound test cases */
	ret = riscv_aia_irq_is_enabled(TEST_IRQ_ID_INVALID);
	zassert_equal(ret, 0, "Expected test IRQ to be disabled");

	ret = riscv_aia_irq_is_enabled(APLIC_NUM_SOURCES + 1);
	zassert_equal(ret, 0, "Expected test IRQ to be disabled");

	/* In-bounds check */
	riscv_aia_irq_disable(TEST_IRQ_ID_L2);
	ret = riscv_aia_irq_is_enabled(TEST_IRQ_ID_L2);
	zassert_equal(ret, 0, "Expected test IRQ to be disabled");
}

ZTEST(aplic_direct, test_aplic_direct_affinity_out_of_bounds)
{
	if (!IS_ENABLED(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)) {
		ztest_test_skip();
	}

	const struct device *aplic = riscv_aplic_get_dev();

	int ret = -1;

	ret = riscv_aplic_irq_set_affinity(aplic, TEST_IRQ_ID_INVALID, 0);
	zassert_not_equal(ret, 0,
			  "Expected affinity configuration to fail due to out of bound IRQ");

	ret = riscv_aplic_irq_set_affinity(aplic, TEST_IRQ_ID_L2, 50);
	zassert_not_equal(ret, 0, "Expected affinity configuration to fail due to invalid Hart ID");
}

ZTEST(aplic_direct, test_aplic_direct_source_mode_configuration)
{
	const uint32_t out_of_range_margin = 2;
	const uint32_t aplic_sm_start = (APLIC_SM_INACTIVE - out_of_range_margin);
	const uint32_t aplic_sm_end = (APLIC_SM_LEVEL_LOW + out_of_range_margin);

	const struct device *aplic_dev = riscv_aplic_get_dev();

	int ret = -1;

	for (uint32_t source_mode = aplic_sm_start; source_mode < aplic_sm_end; source_mode++) {
		ret = riscv_aplic_config_src(aplic_dev, TEST_IRQ_ID_L2, source_mode);

		/* Per AIA modes 0x2 and 0x3 are invalid */
		if ((source_mode == 0x2) || (source_mode == 0x3)) {
			zassert_not_equal(
				ret, 0,
				"Expected invalid source mode to return error. Source mode: %d",
				source_mode);
		} else {
			zassert_equal(ret, 0,
				      "Expected source mode to return success. Source mode: %d",
				      source_mode);
		}
	}
}

ZTEST(aplic_direct, test_aplic_direct_spurious_interrupt)
{
	ztest_set_fault_valid(true);

	/*
	 * Fire an interrupt on each active hart to ensure mcause is set
	 * to 11 (RISCV_IRQ_MEXT). Invoking the aplic handler explicitly
	 * omits this step, leading to non-deterministic mcause
	 */
	for (uint32_t cpu = 0; cpu < arch_num_cpus(); cpu++) {
		riscv_aia_route_to_hart(TEST_IRQ_ID_L2, cpu);

		trigger_aplic_irq(TEST_IRQ_ID, cpu);
	}

	/* Invoke the handler directly to simulate spurious interrupt */
	const struct device *dev = riscv_aplic_get_dev();

	aplic_irq_handler(dev);
}
