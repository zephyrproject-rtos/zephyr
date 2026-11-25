/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

#define TEST_BASE 0x1000U

static uint32_t registers[0x74U / sizeof(uint32_t)];
static uint64_t callbacks;
static uint32_t inject_on_ack[2];

static uint32_t test_read32(mem_addr_t addr)
{
	zassert_true(addr >= TEST_BASE && addr < TEST_BASE + sizeof(registers));
	if (CONFIG_NXP_S32_WKPU_SOURCES_MAX == 32) {
		zassert_true(addr < TEST_BASE + 0x40U, "Access to absent second bank");
	}

	return registers[(addr - TEST_BASE) / sizeof(uint32_t)];
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
	data = (struct wkpu_nxp_s32_data){.cb = callback_entries};
	callbacks = 0U;
	ARG_UNUSED(wkpu_nxp_s32_init);
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

ZTEST_SUITE(wkpu_registers, NULL, NULL, before, NULL, NULL);
