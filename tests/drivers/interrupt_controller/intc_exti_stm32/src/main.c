/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>


#define EXTI_DT_NODE		DT_INST(0, st_stm32_exti)
#define EXTI_NUM_LINES_TOTAL	DT_PROP(EXTI_DT_NODE, num_lines)
#define EXTI_NUM_LINES_GPIO	DT_PROP(EXTI_DT_NODE, num_gpio_lines)

#define TEST_EXTI_LINE_NR	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_nr)
#define TEST_EXTI_IRQ_NR	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_irq_nr)
#define TEST_EXTI_IRQ_PRIO	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_irq_prio)


BUILD_ASSERT(TEST_EXTI_LINE_NR < EXTI_NUM_LINES_TOTAL, "Invalid EXTI line number");

static atomic_t is_test_exti_isr_called;

static void test_exti_isr(void)
{
	zassert_true(stm32_exti_is_pending(TEST_EXTI_LINE_NR));
	stm32_exti_clear_pending(TEST_EXTI_LINE_NR);

	atomic_set(&is_test_exti_isr_called, true);
}

ZTEST(intc_exti_stm32, test_sw_interrupt_rising_trigger)
{
	int ret = 0;

	ret = stm32_exti_enable(TEST_EXTI_LINE_NR,
				STM32_EXTI_TRIG_RISING,
				STM32_EXTI_MODE_IT);
	zassert_ok(ret, "Failed to enable EXTI line %d", TEST_EXTI_LINE_NR);

	atomic_set(&is_test_exti_isr_called, false);

	ret = stm32_exti_sw_interrupt(TEST_EXTI_LINE_NR);
	zassert_ok(ret, "Failed to fire SW interrupt on EXTI line %d",
		   TEST_EXTI_LINE_NR);

	zassert_equal(is_test_exti_isr_called, true,
		      "ISR was not called for EXTI line %d", TEST_EXTI_LINE_NR);
}

ZTEST(intc_exti_stm32, test_sw_interrupt_falling_trigger)
{
	int ret = 0;

	ret = stm32_exti_enable(TEST_EXTI_LINE_NR,
				STM32_EXTI_TRIG_FALLING,
				STM32_EXTI_MODE_IT);
	zassert_ok(ret, "Failed to enable EXTI line %d", TEST_EXTI_LINE_NR);

	atomic_set(&is_test_exti_isr_called, false);

	ret = stm32_exti_sw_interrupt(TEST_EXTI_LINE_NR);
	zassert_ok(ret, "Failed to fire SW interrupt on EXTI line %d",
		   TEST_EXTI_LINE_NR);

	zassert_equal(is_test_exti_isr_called, true,
		      "ISR was not called for EXTI line %d", TEST_EXTI_LINE_NR);
}

ZTEST(intc_exti_stm32, test_sw_interrupt_both_triggers)
{
	int ret = 0;

	ret = stm32_exti_enable(TEST_EXTI_LINE_NR,
				STM32_EXTI_TRIG_BOTH,
				STM32_EXTI_MODE_IT);
	zassert_ok(ret, "Failed to enable EXTI line %d", TEST_EXTI_LINE_NR);

	atomic_set(&is_test_exti_isr_called, false);

	ret = stm32_exti_sw_interrupt(TEST_EXTI_LINE_NR);
	zassert_ok(ret, "Failed to fire SW interrupt on EXTI line %d",
		   TEST_EXTI_LINE_NR);

	zassert_equal(is_test_exti_isr_called, true,
		      "ISR was not called for EXTI line %d", TEST_EXTI_LINE_NR);
}

static void *test_exti_intc_init(void)
{
	IRQ_CONNECT(
		TEST_EXTI_IRQ_NR, TEST_EXTI_IRQ_PRIO,
		test_exti_isr, NULL, 0
	);

	irq_enable(TEST_EXTI_IRQ_NR);

	return NULL;
}

ZTEST_SUITE(intc_exti_stm32, NULL, test_exti_intc_init, NULL, NULL, NULL);
