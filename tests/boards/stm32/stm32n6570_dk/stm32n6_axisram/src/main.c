/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/*
 * AXISRAM3 to AXISRAM6 stay unusable until the RAMCFG instance owning each of
 * them clocks and enables it, which the SoC driver does at PRE_KERNEL_2.
 * Building the devicetree says nothing about whether that happened, so place a
 * buffer over each bank and use it.
 */

#define BANK_BUF_DEFINE(node_id)                                                                   \
	static uint8_t bank_buf_##node_id[DT_REG_SIZE(node_id)] Z_GENERIC_SECTION(                  \
		LINKER_DT_NODE_REGION_NAME_TOKEN(node_id));

#define BANK_ENTRY(node_id)                                                                        \
	{                                                                                          \
		.name = LINKER_DT_NODE_REGION_NAME(node_id),                                       \
		.buf = bank_buf_##node_id,                                                         \
		.size = sizeof(bank_buf_##node_id),                                                \
		.addr = DT_REG_ADDR(node_id),                                                      \
	},

#define BANK_IF_OKAY(fn, label)                                                                    \
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(label)), (fn(DT_NODELABEL(label))))

#define FOR_EACH_BANK(fn)                                                                          \
	BANK_IF_OKAY(fn, axisram3)                                                                 \
	BANK_IF_OKAY(fn, axisram4)                                                                 \
	BANK_IF_OKAY(fn, axisram5)                                                                 \
	BANK_IF_OKAY(fn, axisram6)

FOR_EACH_BANK(BANK_BUF_DEFINE)

struct bank {
	const char *name;
	uint8_t *buf;
	size_t size;
	uintptr_t addr;
};

static const struct bank banks[] = {FOR_EACH_BANK(BANK_ENTRY)};

BUILD_ASSERT(ARRAY_SIZE(banks) > 0, "no AXISRAM bank is enabled");

/* Vary the value with the offset so that a stuck address line shows up */
static uint8_t pattern_of(size_t bank, size_t off)
{
	return (uint8_t)(0x5aU + bank * 16U + (off & 0xffU) + ((off >> 8) & 0xffU));
}

/* The linker placed each buffer where the devicetree says its bank is */
ZTEST(stm32n6_axisram, test_placement)
{
	for (size_t i = 0; i < ARRAY_SIZE(banks); i++) {
		zassert_equal((uintptr_t)banks[i].buf, banks[i].addr,
			      "%s buffer is at %p, expected %#lx", banks[i].name,
			      (void *)banks[i].buf, (unsigned long)banks[i].addr);
	}
}

/*
 * Both ends and the middle of every bank hold what was written to them. A bank
 * that was never enabled faults or reads back as zero here.
 */
ZTEST(stm32n6_axisram, test_read_write)
{
	for (size_t i = 0; i < ARRAY_SIZE(banks); i++) {
		const size_t offsets[] = {0U, banks[i].size / 2U, banks[i].size - 1U};

		for (size_t j = 0; j < ARRAY_SIZE(offsets); j++) {
			banks[i].buf[offsets[j]] = pattern_of(i, offsets[j]);
		}

		for (size_t j = 0; j < ARRAY_SIZE(offsets); j++) {
			zassert_equal(banks[i].buf[offsets[j]], pattern_of(i, offsets[j]),
				      "%s does not hold what was written at offset %zu",
				      banks[i].name, offsets[j]);
		}
	}
}

/*
 * The banks are separate memories rather than aliases of one another: write
 * all of them and only then read them back, which catches a bank whose address
 * decoding lands on a different one.
 */
ZTEST(stm32n6_axisram, test_banks_are_distinct)
{
	for (size_t i = 0; i < ARRAY_SIZE(banks); i++) {
		banks[i].buf[0] = (uint8_t)(0xa0U + i);
		banks[i].buf[banks[i].size - 1U] = (uint8_t)(0xb0U + i);
	}

	for (size_t i = 0; i < ARRAY_SIZE(banks); i++) {
		zassert_equal(banks[i].buf[0], (uint8_t)(0xa0U + i), "%s aliases another bank",
			      banks[i].name);
		zassert_equal(banks[i].buf[banks[i].size - 1U], (uint8_t)(0xb0U + i),
			      "%s aliases another bank at its top", banks[i].name);
	}
}

ZTEST_SUITE(stm32n6_axisram, NULL, NULL, NULL, NULL, NULL);
