/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_py32.h>
#include <zephyr/dt-bindings/pinctrl/puya_py32_pinctrl.h>
#include <zephyr/ztest.h>

ZTEST(intc_gpio_py32, test_source_selection)
{
	const uint32_t masks[] = {
		EXTI_EXTICR1_EXTI0, EXTI_EXTICR1_EXTI1, EXTI_EXTICR1_EXTI2,
		EXTI_EXTICR1_EXTI3, EXTI_EXTICR2_EXTI4, EXTI_EXTICR2_EXTI5,
		EXTI_EXTICR2_EXTI6, EXTI_EXTICR2_EXTI7, EXTI_EXTICR3_EXTI8,
	};
	const uint32_t ports[] = {PY32_PORTA, PY32_PORTB, PY32_PORTF};
	uint32_t saved[ARRAY_SIZE(EXTI->EXTICR)];

	for (size_t reg = 0U; reg < ARRAY_SIZE(saved); reg++) {
		saved[reg] = EXTI->EXTICR[reg];
	}

	for (gpio_pin_t line = 0U; line < ARRAY_SIZE(masks); line++) {
		for (size_t port = 0U; port < ARRAY_SIZE(ports); port++) {
			uint32_t expected[] = {0x01010101U, 0x01010101U, 0x1U};

			if ((line >= 5U) && (ports[port] == PY32_PORTF)) {
				continue;
			}

			for (size_t reg = 0U; reg < ARRAY_SIZE(expected); reg++) {
				EXTI->EXTICR[reg] = expected[reg];
			}

			expected[line / 4U] &= ~masks[line];
			expected[line / 4U] |= port << ((line % 4U) * 8U);
			py32_exti_set_line_src_port(line, ports[port]);

			for (size_t reg = 0U; reg < ARRAY_SIZE(expected); reg++) {
				zassert_equal(EXTI->EXTICR[reg], expected[reg],
					      "Line %u port %u changed EXTICR[%zu] incorrectly",
					      line, ports[port], reg);
			}
			zassert_equal(py32_exti_get_line_src_port(line), ports[port]);
		}
	}

	for (gpio_pin_t line = 9U; line < 16U; line++) {
		py32_exti_set_line_src_port(line, PY32_PORTA);
		zassert_equal(py32_exti_get_line_src_port(line), PY32_PORTA);
	}

	for (size_t reg = 0U; reg < ARRAY_SIZE(saved); reg++) {
		EXTI->EXTICR[reg] = saved[reg];
	}
}

ZTEST_SUITE(intc_gpio_py32, NULL, NULL, NULL, NULL, NULL);

static void test_callback(gpio_port_pins_t pins, void *user)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(user);
}

ZTEST(intc_gpio_py32, test_line_ownership)
{
	zassert_ok(py32_exti_configure(PY32_PORTA, 0U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_BOTH,
				       test_callback, NULL));
	zassert_equal(py32_exti_get_line_src_port(0U), PY32_PORTA);
	zassert_equal(py32_exti_configure(PY32_PORTB, 0U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_HIGH,
					  test_callback, NULL),
		      -EBUSY);
	/* Disabling a different port must leave the owner and triggers intact. */
	zassert_ok(py32_exti_configure(PY32_PORTB, 0U, GPIO_INT_MODE_DISABLED, GPIO_INT_TRIG_LOW,
				       test_callback, NULL));
	zassert_not_equal(EXTI->IMR & BIT(0), 0U);
	zassert_not_equal(EXTI->RTSR & BIT(0), 0U);
	zassert_not_equal(EXTI->FTSR & BIT(0), 0U);
	zassert_ok(py32_exti_configure(PY32_PORTA, 0U, GPIO_INT_MODE_DISABLED, GPIO_INT_TRIG_LOW,
				       test_callback, NULL));
	zassert_equal(EXTI->IMR & BIT(0), 0U);
	zassert_equal(EXTI->RTSR & BIT(0), 0U);
	zassert_equal(EXTI->FTSR & BIT(0), 0U);
	/* The line can be acquired by another port after the owner releases it. */
	zassert_ok(py32_exti_configure(PY32_PORTB, 0U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_LOW,
				       test_callback, NULL));
	zassert_equal(py32_exti_get_line_src_port(0U), PY32_PORTB);
	zassert_ok(py32_exti_configure(PY32_PORTB, 0U, GPIO_INT_MODE_DISABLED, GPIO_INT_TRIG_LOW,
				       test_callback, NULL));
}

ZTEST(intc_gpio_py32, test_unsupported_sources)
{
	zassert_equal(py32_exti_configure(PY32_PORTB, 9U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_HIGH,
					  test_callback, NULL),
		      -ENOTSUP);
	zassert_equal(py32_exti_configure(PY32_PORTF, 5U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_HIGH,
					  test_callback, NULL),
		      -ENOTSUP);
	zassert_equal(py32_exti_configure(PY32_PORTA, 0U, GPIO_INT_MODE_LEVEL, GPIO_INT_TRIG_HIGH,
					  test_callback, NULL),
		      -ENOTSUP);
	zassert_equal(py32_exti_configure(PY32_PORTA, 16U, GPIO_INT_MODE_EDGE, GPIO_INT_TRIG_HIGH,
					  test_callback, NULL),
		      -EINVAL);
}
