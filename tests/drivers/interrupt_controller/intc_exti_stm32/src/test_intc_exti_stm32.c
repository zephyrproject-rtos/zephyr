/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>


#define EXTI_DT_NODE			DT_INST(0, st_stm32_exti)
#define EXTI_NUM_LINES_TOTAL	DT_PROP(EXTI_DT_NODE, num_lines)
#define EXTI_NUM_LINES_GPIO		16U

// #if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, resources))

#define TEST_EXTI_LINE_NR	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_nr)
#define TEST_EXTI_IRQ_NR	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_irq_nr)
#define TEST_EXTI_IRQ_PRIO	DT_PROP(DT_INST(0, test_st_stm32_exti), exti_line_irq_prio)

// #endif /* DT_NODE_HAS_STATUS_OKAY(DT_INST(0, test_st_stm32_exti)) */

static void test_exti_isr(void)
{
	for (size_t line_num = 0; line_num < EXTI_NUM_LINES_TOTAL; line_num++) {
		if (stm32_exti_is_pending(line_num) != 0) {
			stm32_exti_clear_pending(line_num);
			zassert_equal(line_num, TEST_EXTI_LINE_NR);
		}
	}
}

ZTEST(intc_exti_stm32, test_available_num_lines_for_test)
{
	/*
	 * The testable EXTI lines on stm32 platform are usually above lenght of 16.
	 * The lines 0-15 are reserved for GPIO and will be always present and connected to
	 * GPIO pinterrups
	*/
	zassert_equal(EXTI_NUM_LINES_TOTAL > EXTI_NUM_LINES_GPIO, true);
}

ZTEST(intc_exti_stm32, test_sw_interrupt)
{
	int ret = 0;

	ret = stm32_exti_enable(TEST_EXTI_LINE_NR,
							STM32_EXTI_TRIG_RISING,
							STM32_EXTI_MODE_IT);
	zassert_equal(ret, 0);

	ret = stm32_exti_sw_interrupt(TEST_EXTI_LINE_NR);
	zassert_equal(ret, 0);
}

static int test_exti_intc_init(void)
{
	IRQ_CONNECT(
		TEST_EXTI_IRQ_NR, TEST_EXTI_IRQ_PRIO,
		test_exti_isr, NULL, 0
	);
	irq_enable(TEST_EXTI_IRQ_NR);
	return 0;
}

SYS_INIT(test_exti_intc_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
