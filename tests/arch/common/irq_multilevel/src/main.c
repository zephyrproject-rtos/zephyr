/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_multilevel.h>
#include <zephyr/ztest.h>

#define TEST_L1_IRQ 3U
#define TEST_L2_IRQ 5U

#define TEST_ENCODED_L2_IRQ (TEST_L1_IRQ | IRQ_TO_L2(TEST_L2_IRQ))

ZTEST(irq_multilevel, test_level_1_increment)
{
	const unsigned int irq = TEST_L1_IRQ;
	const unsigned int expected = TEST_L1_IRQ + 1U;
	const unsigned int actual = irq_increment(irq, 1U);

	zassert_equal(actual, expected, "irq_increment(%u, 1) returned 0x%x, expected 0x%x", irq,
		      actual, expected);
}

ZTEST(irq_multilevel, test_level_2_encoding_matches_macro)
{
	const unsigned int expected = IRQ_TO_L2(TEST_L2_IRQ);
	const unsigned int actual = irq_to_level_2(TEST_L2_IRQ);

	zassert_equal(actual, expected,
		      "irq_to_level_2(%u) returned 0x%x, "
		      "but IRQ_TO_L2(%u) returned 0x%x",
		      TEST_L2_IRQ, actual, TEST_L2_IRQ, expected);
}

ZTEST(irq_multilevel, test_level_2_get_level)
{
	const unsigned int irq = TEST_ENCODED_L2_IRQ;
	const unsigned int actual = irq_get_level(irq);

	zassert_equal(actual, 2U, "irq_get_level(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_2_decode)
{
	const unsigned int irq = TEST_ENCODED_L2_IRQ;
	const unsigned int actual = irq_from_level_2(irq);

	zassert_equal(actual, TEST_L2_IRQ, "irq_from_level_2(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_2_parent)
{
	const unsigned int irq = TEST_ENCODED_L2_IRQ;
	const unsigned int actual = irq_parent_level_2(irq);

	zassert_equal(actual, TEST_L1_IRQ, "irq_parent_level_2(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_2_intc_irq)
{
	const unsigned int irq = TEST_ENCODED_L2_IRQ;
	const unsigned int actual = irq_get_intc_irq(irq);

	zassert_equal(actual, TEST_L1_IRQ, "irq_get_intc_irq(0x%x) returned 0x%x", irq, actual);
}

ZTEST(irq_multilevel, test_level_2_increment)
{
	const unsigned int irq = TEST_ENCODED_L2_IRQ;
	const unsigned int expected = TEST_L1_IRQ | IRQ_TO_L2(TEST_L2_IRQ + 1U);
	const unsigned int actual = irq_increment(irq, 1U);

	zassert_equal(actual, expected, "irq_increment(0x%x, 1) returned 0x%x, expected 0x%x", irq,
		      actual, expected);
}

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS)
#define TEST_L3_IRQ 7U

#define TEST_ENCODED_L3_IRQ (TEST_L1_IRQ | IRQ_TO_L2(TEST_L2_IRQ) | IRQ_TO_L3(TEST_L3_IRQ))

#define TEST_ENCODED_L3_PARENT (TEST_L1_IRQ | IRQ_TO_L2(TEST_L2_IRQ))

ZTEST(irq_multilevel, test_level_3_encoding_matches_macro)
{
	const unsigned int expected = IRQ_TO_L3(TEST_L3_IRQ);
	const unsigned int actual = irq_to_level_3(TEST_L3_IRQ);

	zassert_equal(actual, expected,
		      "irq_to_level_3(%u) returned 0x%x, "
		      "but IRQ_TO_L3(%u) returned 0x%x",
		      TEST_L3_IRQ, actual, TEST_L3_IRQ, expected);
}

ZTEST(irq_multilevel, test_level_3_get_level)
{
	const unsigned int irq = TEST_ENCODED_L3_IRQ;
	const unsigned int actual = irq_get_level(irq);

	zassert_equal(actual, 3U, "irq_get_level(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_3_decode)
{
	const unsigned int irq = TEST_ENCODED_L3_IRQ;
	const unsigned int actual = irq_from_level_3(irq);

	zassert_equal(actual, TEST_L3_IRQ, "irq_from_level_3(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_3_parent)
{
	const unsigned int irq = TEST_ENCODED_L3_IRQ;
	const unsigned int actual = irq_parent_level_3(irq);

	zassert_equal(actual, TEST_L2_IRQ, "irq_parent_level_3(0x%x) returned %u", irq, actual);
}

ZTEST(irq_multilevel, test_level_3_intc_irq)
{
	const unsigned int irq = TEST_ENCODED_L3_IRQ;
	const unsigned int actual = irq_get_intc_irq(irq);

	zassert_equal(actual, TEST_ENCODED_L3_PARENT,
		      "irq_get_intc_irq(0x%x) returned 0x%x, expected 0x%x", irq, actual,
		      TEST_ENCODED_L3_PARENT);
}

ZTEST(irq_multilevel, test_level_3_increment)
{
	const unsigned int irq = TEST_ENCODED_L3_IRQ;
	const unsigned int expected =
		TEST_L1_IRQ | IRQ_TO_L2(TEST_L2_IRQ) | IRQ_TO_L3(TEST_L3_IRQ + 1U);
	const unsigned int actual = irq_increment(irq, 1U);

	zassert_equal(actual, expected, "irq_increment(0x%x, 1) returned 0x%x, expected 0x%x", irq,
		      actual, expected);
}

#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

ZTEST_SUITE(irq_multilevel, NULL, NULL, NULL, NULL, NULL);
