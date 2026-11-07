/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_stm32.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h>
#include <zephyr/ztest.h>

ZTEST(intc_gpio_py32, test_source_selection)
{
	const uint32_t masks[] = {
		EXTI_EXTICR1_EXTI0, EXTI_EXTICR1_EXTI1, EXTI_EXTICR1_EXTI2,
		EXTI_EXTICR1_EXTI3, EXTI_EXTICR2_EXTI4, EXTI_EXTICR2_EXTI5,
		EXTI_EXTICR2_EXTI6, EXTI_EXTICR2_EXTI7, EXTI_EXTICR3_EXTI8,
	};
	const uint32_t ports[] = {STM32_PORTA, STM32_PORTB, STM32_PORTF};
	uint32_t saved[ARRAY_SIZE(EXTI->EXTICR)];

	for (size_t reg = 0U; reg < ARRAY_SIZE(saved); reg++) {
		saved[reg] = EXTI->EXTICR[reg];
	}

	for (gpio_pin_t line = 0U; line < ARRAY_SIZE(masks); line++) {
		for (size_t port = 0U; port < ARRAY_SIZE(ports); port++) {
			uint32_t expected[] = {0x01010101U, 0x01010101U, 0x1U};

			if ((line >= 5U) && (ports[port] == STM32_PORTF)) {
				continue;
			}

			for (size_t reg = 0U; reg < ARRAY_SIZE(expected); reg++) {
				EXTI->EXTICR[reg] = expected[reg];
			}

			expected[line / 4U] &= ~masks[line];
			expected[line / 4U] |= port << ((line % 4U) * 8U);
			stm32_exti_set_line_src_port(line, ports[port]);

			for (size_t reg = 0U; reg < ARRAY_SIZE(expected); reg++) {
				zassert_equal(EXTI->EXTICR[reg], expected[reg],
					      "Line %u port %u changed EXTICR[%zu] incorrectly",
					      line, ports[port], reg);
			}
			zassert_equal(stm32_exti_get_line_src_port(line), ports[port]);
		}
	}

	for (gpio_pin_t line = 9U; line < 16U; line++) {
		stm32_exti_set_line_src_port(line, STM32_PORTA);
		zassert_equal(stm32_exti_get_line_src_port(line), STM32_PORTA);
	}

	for (size_t reg = 0U; reg < ARRAY_SIZE(saved); reg++) {
		EXTI->EXTICR[reg] = saved[reg];
	}
}

ZTEST_SUITE(intc_gpio_py32, NULL, NULL, NULL, NULL, NULL);
