/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/dt-bindings/wuc/wuc_nxp_s32_wkpu.h>

#define TEST_BASE 0x1000U

static uint32_t registers[0x74U / sizeof(uint32_t)];
static uint64_t callbacks;
static uint32_t inject_on_ack[2];
static uint32_t inject_after_read[2];

static uint32_t test_read32(mem_addr_t addr)
{
	zassert_true(addr >= TEST_BASE && addr < TEST_BASE + sizeof(registers));
	if (CONFIG_NXP_S32_WKPU_SOURCES_MAX == 32) {
		zassert_true(addr < TEST_BASE + 0x40U, "Access to absent second bank");
	}

	uint32_t offset = addr - TEST_BASE;
	uint32_t value = registers[offset / sizeof(uint32_t)];

	if (offset == 0x14U || offset == 0x54U) {
		registers[offset / sizeof(uint32_t)] |= inject_after_read[offset / 0x40U];
		inject_after_read[offset / 0x40U] = 0U;
	}

	return value;
}

static void test_write32(uint32_t value, mem_addr_t addr)
{
	uint32_t offset = addr - TEST_BASE;

	(void)test_read32(addr);
	if (offset == 0x14U || offset == 0x54U) {
		registers[offset / sizeof(uint32_t)] &= ~value;
		registers[offset / sizeof(uint32_t)] |= inject_on_ack[offset / 0x40U];
	} else {
		registers[offset / sizeof(uint32_t)] = value;
	}
}

/* Exercise the real driver against a W1C-aware register model. */
#define sys_read32 test_read32
#define sys_write32 test_write32
#include "../../../../../drivers/interrupt_controller/intc_wkpu_nxp_s32.c"
#undef sys_write32
#undef sys_read32

#undef DT_DRV_COMPAT
#include "../../../../../drivers/wuc/wuc_nxp_s32_wkpu.c"

BUILD_ASSERT(__builtin_types_compatible_p(
		     __typeof__(&wkpu_nxp_s32_enable_interrupt),
		     void (*)(const struct device *, uint8_t, enum wkpu_nxp_s32_trigger)),
	     "The existing WKPU interrupt-enable API must retain its void signature");

#define TEST_REG(offset) registers[(offset) / sizeof(uint32_t)]

static struct wkpu_nxp_s32_cb callback_entries[CONFIG_NXP_S32_WKPU_SOURCES_MAX];
static struct wkpu_nxp_s32_data data;
static const struct wkpu_nxp_s32_config config = {
	.base = TEST_BASE,
};
static const struct device controller = {
	.config = &config,
	.data = &data,
};
static const struct wuc_nxp_s32_wkpu_config wakeup_config = {
	.parent = &controller,
};
static const struct device wakeup = {
	.config = &wakeup_config,
};

static void callback(uint8_t source, void *arg)
{
	ARG_UNUSED(arg);
	callbacks |= BIT64(source);
	/* Callbacks may reenter the interrupt API. */
	wkpu_nxp_s32_disable_interrupt(&controller, source);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(registers, 0, sizeof(registers));
	memset(callback_entries, 0, sizeof(callback_entries));
	memset(inject_on_ack, 0, sizeof(inject_on_ack));
	memset(inject_after_read, 0, sizeof(inject_after_read));
	data = (struct wkpu_nxp_s32_data){.cb = callback_entries};
	callbacks = 0U;
	ARG_UNUSED(wkpu_nxp_s32_init);
	ARG_UNUSED(wuc_nxp_s32_wkpu_init);
}

ZTEST(wkpu_registers, test_enable_preserves_unrelated_pending)
{
	uint32_t bank = (CONFIG_NXP_S32_WKPU_SOURCES_MAX / 32U) - 1U;
	uint8_t source = bank * 32U + 1U;

	TEST_REG(WKPU_WISR(bank)) = BIT(1) | BIT(7);
	wkpu_nxp_s32_enable_interrupt(&controller, source, WKPU_NXP_S32_RISING_EDGE);

	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(7));
	zassert_equal(TEST_REG(WKPU_IRER(bank)), BIT(1));
}

ZTEST(wkpu_registers, test_disable_preserves_unrelated_pending)
{
	TEST_REG(WKPU_WISR(0)) = BIT(1) | BIT(7);
	TEST_REG(WKPU_IRER(0)) = BIT(1) | BIT(7);
	wkpu_nxp_s32_disable_interrupt(&controller, 1U);

	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(7));
	zassert_equal(TEST_REG(WKPU_IRER(0)), BIT(7));
}

ZTEST(wkpu_registers, test_disable_preserves_armed_wake_edges)
{
	TEST_REG(WKPU_WRER(0)) = BIT(1);
	TEST_REG(WKPU_WIREER(0)) = BIT(1) | BIT(7);
	TEST_REG(WKPU_WIFEER(0)) = BIT(1);
	wkpu_nxp_s32_disable_interrupt(&controller, 1U);

	zassert_equal(TEST_REG(WKPU_WIREER(0)), BIT(1) | BIT(7));
	zassert_equal(TEST_REG(WKPU_WIFEER(0)), BIT(1));
	zassert_equal(TEST_REG(WKPU_WRER(0)), BIT(1));
}

ZTEST(wkpu_registers, test_isr_preserves_other_sources)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t bank = source / 32U;

	zassert_ok(wkpu_nxp_s32_set_callback(&controller, source, source, callback, NULL));
	TEST_REG(WKPU_IRER(bank)) = BIT(31);
	TEST_REG(WKPU_WISR(bank)) = BIT(31) | BIT(7);
	wkpu_nxp_s32_interrupt_handler(&controller);

	zassert_equal(callbacks, BIT64(source));
	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(7));
}

ZTEST(wkpu_registers, test_wuc_configures_selected_source_without_irq)
{
	uint32_t expected[ARRAY_SIZE(registers)];
	const uint32_t sources[] = {
		0U, 31U,
#if CONFIG_NXP_S32_WKPU_SOURCES_MAX > 32
		32U, CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U,
#endif
	};

	for (size_t i = 0; i < ARRAY_SIZE(sources); i++) {
		uint32_t source = sources[i];
		uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);
		uint32_t bank = source / 32U;
		uint32_t mask = BIT(source % 32U);

		TEST_REG(WKPU_IRER(bank)) = BIT(7);
		TEST_REG(WKPU_WRER(bank)) = BIT(7);
		TEST_REG(WKPU_WIREER(bank)) = BIT(7);
		TEST_REG(WKPU_WIFEER(bank)) = mask;
		TEST_REG(WKPU_WISR(bank)) = BIT(7) | mask;
		memcpy(expected, registers, sizeof(expected));
		expected[WKPU_WRER(bank) / sizeof(uint32_t)] |= mask;
		expected[WKPU_WIREER(bank) / sizeof(uint32_t)] |= mask;
		expected[WKPU_WIFEER(bank) / sizeof(uint32_t)] &= ~mask;
		zassert_ok(wuc_nxp_s32_wkpu_enable(&wakeup, id));
		zassert_mem_equal(registers, expected, sizeof(expected));
		zassert_ok(wuc_nxp_s32_wkpu_enable(&wakeup, id));
		zassert_mem_equal(registers, expected, sizeof(expected));

		expected[WKPU_WRER(bank) / sizeof(uint32_t)] &= ~mask;
		expected[WKPU_WIREER(bank) / sizeof(uint32_t)] &= ~mask;
		zassert_ok(wuc_nxp_s32_wkpu_disable(&wakeup, id));
		zassert_mem_equal(registers, expected, sizeof(expected));
		zassert_ok(wuc_nxp_s32_wkpu_disable(&wakeup, id));
		zassert_mem_equal(registers, expected, sizeof(expected));
	}
}

ZTEST(wkpu_registers, test_wuc_rejects_invalid_sources)
{
	const uint32_t sources[] = {
		NXP_S32_WKPU_SOURCE(CONFIG_NXP_S32_WKPU_SOURCES_MAX,
				    NXP_S32_WKPU_EDGE_RISING),
		UINT32_MAX, 31U,
		NXP_S32_WKPU_SOURCE(31, 4),
		NXP_S32_WKPU_SOURCE(31, NXP_S32_WKPU_EDGE_RISING) | BIT(16),
		NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_FALLING),
		NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_BOTH),
	};

	for (size_t i = 0; i < ARRAY_SIZE(sources); i++) {
		zassert_equal(wuc_nxp_s32_wkpu_enable(&wakeup, sources[i]), -EINVAL);
		zassert_equal(wuc_nxp_s32_wkpu_disable(&wakeup, sources[i]), -EINVAL);
		zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, sources[i]), -EINVAL);
		zassert_equal(wuc_nxp_s32_wkpu_clear(&wakeup, sources[i]), -EINVAL);
	}
}

ZTEST(wkpu_registers, test_isr_keeps_wuc_status_until_clear)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);
	uint32_t bank = source / 32U;

	zassert_ok(wkpu_nxp_s32_set_callback(&controller, source, source, callback, NULL));
	TEST_REG(WKPU_IRER(bank)) = BIT(31);
	TEST_REG(WKPU_WRER(bank)) = BIT(31);
	TEST_REG(WKPU_WISR(bank)) = BIT(31) | BIT(7);
	wkpu_nxp_s32_interrupt_handler(&controller);

	zassert_equal(callbacks, BIT64(source));
	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(7));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(7));
}

ZTEST(wkpu_registers, test_wuc_clear_does_not_consume_pending_irq)
{
	uint32_t id = NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING);

	zassert_ok(wkpu_nxp_s32_set_callback(&controller, 1U, 1U, callback, NULL));
	TEST_REG(WKPU_IRER(0)) = BIT(1);
	TEST_REG(WKPU_WRER(0)) = BIT(1);
	TEST_REG(WKPU_WISR(0)) = BIT(1) | BIT(7);

	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(wkpu_nxp_s32_get_pending(&controller), BIT64(1));
	wkpu_nxp_s32_interrupt_handler(&controller);
	zassert_equal(callbacks, BIT64(1));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(7));

	TEST_REG(WKPU_WISR(0)) |= BIT(1);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(7));
}

ZTEST(wkpu_registers, test_irq_configuration_preserves_pending_wake)
{
	TEST_REG(WKPU_WRER(0)) = BIT(1);
	TEST_REG(WKPU_WIREER(0)) = BIT(1);
	TEST_REG(WKPU_WISR(0)) = BIT(1) | BIT(7);
	wkpu_nxp_s32_enable_interrupt(&controller, 1U, WKPU_NXP_S32_RISING_EDGE);
	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(1) | BIT(7));
	wkpu_nxp_s32_disable_interrupt(&controller, 1U);
	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(1) | BIT(7));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(
			      &wakeup, NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING)), 1);
}

ZTEST(wkpu_registers, test_irq_disable_completes_deferred_clear)
{
	uint32_t id = NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING);

	TEST_REG(WKPU_IRER(0)) = BIT(1);
	TEST_REG(WKPU_WRER(0)) = BIT(1);
	TEST_REG(WKPU_WIREER(0)) = BIT(1);
	TEST_REG(WKPU_WISR(0)) = BIT(1) | BIT(7);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	wkpu_nxp_s32_disable_interrupt(&controller, 1U);

	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(7));
	zassert_equal(TEST_REG(WKPU_WIREER(0)), BIT(1));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	TEST_REG(WKPU_WISR(0)) |= BIT(1);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
}

ZTEST(wkpu_registers, test_clear_isolates_banks_and_saved_status)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);
	uint32_t bank = source / 32U;

	wkpu_nxp_s32_capture_events(&controller, BIT64(0) | BIT64(source));
	TEST_REG(WKPU_WISR(bank)) = BIT(31) | BIT(7);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(
			      &wakeup, NXP_S32_WKPU_SOURCE(0, NXP_S32_WKPU_EDGE_RISING)), 1);
	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(7));
}

ZTEST(wkpu_registers, test_init_merges_early_snapshot_without_replaying_it)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);
	uint32_t rtc_id = NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING);

	wkpu_nxp_s32_capture_events(&controller, BIT64(source));
	TEST_REG(WKPU_WISR(0)) = BIT(1);
	zassert_ok(wkpu_nxp_s32_init(&controller));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, rtc_id), 1);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, rtc_id));
	zassert_ok(wkpu_nxp_s32_init(&controller));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 0);
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, rtc_id), 0);
}

ZTEST(wkpu_registers, test_init_preserves_armed_wake_configuration)
{
	TEST_REG(WKPU_WRER(0)) = BIT(1);
	TEST_REG(WKPU_WIREER(0)) = BIT(1) | BIT(7);
	TEST_REG(WKPU_WIFEER(0)) = BIT(1);
	TEST_REG(WKPU_WIFER(0)) = BIT(1) | BIT(7);
	zassert_ok(wkpu_nxp_s32_init(&controller));
	zassert_equal(TEST_REG(WKPU_WRER(0)), BIT(1));
	zassert_equal(TEST_REG(WKPU_WIREER(0)), BIT(1));
	zassert_equal(TEST_REG(WKPU_WIFEER(0)), BIT(1));
	zassert_equal(TEST_REG(WKPU_WIFER(0)), BIT(1));
}

ZTEST(wkpu_registers, test_init_only_acknowledges_captured_status)
{
	TEST_REG(WKPU_WISR(0)) = BIT(1);
	inject_on_ack[0] = BIT(7);
	zassert_ok(wkpu_nxp_s32_init(&controller));
	zassert_equal(TEST_REG(WKPU_WISR(0)), BIT(7));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(
			      &wakeup, NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING)), 1);
}

ZTEST(wkpu_registers, test_wuc_clear_preserves_irq_arriving_after_read)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t bank = source / 32U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);

	zassert_ok(wkpu_nxp_s32_set_callback(&controller, source, source, callback, NULL));
	TEST_REG(WKPU_IRER(bank)) = BIT(31);
	inject_after_read[bank] = BIT(31);
	zassert_ok(wuc_nxp_s32_wkpu_clear(&wakeup, id));
	zassert_equal(TEST_REG(WKPU_WISR(bank)), BIT(31));
	zassert_equal(wuc_nxp_s32_wkpu_triggered(&wakeup, id), 1);
	wkpu_nxp_s32_interrupt_handler(&controller);
	zassert_equal(callbacks, BIT64(source));
}

ZTEST(wkpu_registers, test_wuc_external_edge_modes_and_irq_conflict)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t bank = source / 32U;

	wkpu_nxp_s32_enable_interrupt(&controller, source, WKPU_NXP_S32_RISING_EDGE);
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_FALLING);

	zassert_equal(wuc_nxp_s32_wkpu_enable(&wakeup, id), -EBUSY);
	zassert_equal(TEST_REG(WKPU_WRER(bank)), 0U);
	zassert_equal(TEST_REG(WKPU_WIREER(bank)), BIT(31));
	zassert_equal(TEST_REG(WKPU_WIFEER(bank)), 0U);
	wkpu_nxp_s32_disable_interrupt(&controller, source);

	for (uint32_t edge = NXP_S32_WKPU_EDGE_RISING;
	     edge <= NXP_S32_WKPU_EDGE_BOTH; edge++) {
		id = NXP_S32_WKPU_SOURCE(source, edge);
		zassert_ok(wuc_nxp_s32_wkpu_enable(&wakeup, id));
		zassert_equal(TEST_REG(WKPU_WIREER(bank)),
			      (edge & NXP_S32_WKPU_EDGE_RISING) ? BIT(31) : 0U);
		zassert_equal(TEST_REG(WKPU_WIFEER(bank)),
			      (edge & NXP_S32_WKPU_EDGE_FALLING) ? BIT(31) : 0U);
		zassert_ok(wuc_nxp_s32_wkpu_disable(&wakeup, id));
	}
}

ZTEST(wkpu_registers, test_irq_rejects_wake_edge_conflict_without_changing_callback)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t bank = source / 32U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_FALLING);

	zassert_ok(wuc_nxp_s32_wkpu_enable(&wakeup, id));
	wkpu_nxp_s32_enable_interrupt(&controller, source, WKPU_NXP_S32_RISING_EDGE);
	zassert_equal(wkpu_nxp_s32_configure_interrupt(&controller, source, source,
						      WKPU_NXP_S32_RISING_EDGE,
						      callback, NULL), -EBUSY);
	zassert_is_null(callback_entries[source].cb);
	zassert_equal(TEST_REG(WKPU_IRER(bank)), 0U);
	zassert_equal(TEST_REG(WKPU_WIREER(bank)), 0U);
	zassert_equal(TEST_REG(WKPU_WIFEER(bank)), BIT(31));

	zassert_ok(wkpu_nxp_s32_configure_interrupt(&controller, source, source,
						   WKPU_NXP_S32_FALLING_EDGE, callback, NULL));
	zassert_equal(wkpu_nxp_s32_configure_interrupt(&controller, source, source,
						      WKPU_NXP_S32_RISING_EDGE,
						      callback, NULL), -EBUSY);
	TEST_REG(WKPU_WISR(bank)) = BIT(31);
	wkpu_nxp_s32_interrupt_handler(&controller);
	zassert_equal(callbacks, BIT64(source));
}

ZTEST(wkpu_registers, test_wuc_disable_preserves_irq_edges_and_rejects_wrong_id)
{
	uint8_t source = CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U;
	uint32_t bank = source / 32U;
	uint32_t id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_BOTH);
	uint32_t other_id = NXP_S32_WKPU_SOURCE(source, NXP_S32_WKPU_EDGE_RISING);

	wkpu_nxp_s32_enable_interrupt(&controller, source, WKPU_NXP_S32_BOTH_EDGES);
	zassert_ok(wuc_nxp_s32_wkpu_enable(&wakeup, id));
	zassert_equal(wuc_nxp_s32_wkpu_enable(&wakeup, other_id), -EBUSY);
	zassert_equal(wuc_nxp_s32_wkpu_disable(&wakeup, other_id), -EBUSY);
	zassert_equal(TEST_REG(WKPU_WRER(bank)), BIT(31));
	zassert_ok(wuc_nxp_s32_wkpu_disable(&wakeup, id));
	zassert_equal(TEST_REG(WKPU_WRER(bank)), 0U);
	zassert_equal(TEST_REG(WKPU_IRER(bank)), BIT(31));
	zassert_equal(TEST_REG(WKPU_WIREER(bank)), BIT(31));
	zassert_equal(TEST_REG(WKPU_WIFEER(bank)), BIT(31));
}

ZTEST(wkpu_registers, test_irq_rejects_invalid_sources_and_internal_edges)
{
	wkpu_nxp_s32_enable_interrupt(&controller, CONFIG_NXP_S32_WKPU_SOURCES_MAX,
				      WKPU_NXP_S32_RISING_EDGE);
	wkpu_nxp_s32_enable_interrupt(&controller, 1U, WKPU_NXP_S32_FALLING_EDGE);
	wkpu_nxp_s32_enable_interrupt(&controller, 1U, WKPU_NXP_S32_BOTH_EDGES);
	zassert_equal(wkpu_nxp_s32_configure_interrupt(
			      &controller, CONFIG_NXP_S32_WKPU_SOURCES_MAX, 1U,
			      WKPU_NXP_S32_RISING_EDGE, callback, NULL), -EINVAL);
	zassert_equal(TEST_REG(WKPU_IRER(0)), 0U);
}

ZTEST_SUITE(wkpu_registers, NULL, NULL, before, NULL, NULL);
