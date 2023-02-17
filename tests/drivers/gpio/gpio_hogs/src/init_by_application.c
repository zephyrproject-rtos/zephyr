/*
 * Copyright 2023 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define GPIO_MASK_NO_OUTPUT \
	GPIO_FLAGS_ALL & \
        ~(GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL)

const struct gpio_dt_spec output_high_gpio_specs[] = {
	DT_FOREACH_PROP_ELEM_SEP(ZEPHYR_USER_NODE, output_high_gpios,
				 GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const struct gpio_dt_spec input_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, input_gpios, {0});

/*
 * @brief Verify that the GPIO hogs driver did not automatically configure
 * any GPIO pins when CONFIG_GPIO_HOGS_INITIALIZE_BY_APPLICATION=y.
 */
ZTEST(gpio_hogs_init_by_app, test_gpio_hogs_not_configured)
{
	const struct gpio_dt_spec *spec;
	int err;

	for (int i = 0; i < ARRAY_SIZE(output_high_gpio_specs); i++) {
		spec = &output_high_gpio_specs[i];

		err = gpio_pin_is_output(spec->port, spec->pin);

		if (err == -ENOSYS) {
			ztest_test_skip();
		}

		zassert_equal(err, 0, "GPIO hog %s pin %d configured as output, but should not be",
			spec->port->name, spec->pin);
	}

	err = gpio_pin_is_input(input_gpio.port, spec->pin);
	zassert_equal(err, 0, "GPIO hog %s pin %d configured as input, but should not be",
		spec->port->name, spec->pin);
}

/*
 * @brief Verify that the GPIO hogs driver respects the mask parameter.
 */
ZTEST(gpio_hogs_init_by_app, test_masked_output_level)
{
	gpio_flags_t expected;
	const struct gpio_dt_spec *spec;

	gpio_hogs_configure(NULL, GPIO_MASK_NO_OUTPUT);

	for (int i = 0; i < ARRAY_SIZE(output_high_gpio_specs); i++) {
		spec = &output_high_gpio_specs[i];

		/* GPIO pin state should always by low when applying the GPIO_OUTPUT_INIT_MASK */
		expected = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

		assert_gpio_hog_config(spec, expected);
	}
}

/*
 * @brief Verify that the GPIO hogs driver respects the port parameter.
 */
#if 0
// FIXME - need to add another GPIO port to the overlay
ZTEST(gpio_hogs_init_by_app, test_gpio_port)
{
	gpio_flags_t expected;
	const struct gpio_dt_spec *spec;

	gpio_hogs_configure(GPIO_MASK_NO_OUTPUT);

	for (int i = 0; i < ARRAY_SIZE(output_high_gpio_specs); i++) {
		spec = &output_high_gpio_specs[i];

		/* GPIO pin state should always by low when applying the GPIO_OUTPUT_INIT_MASK */
		expected = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

		assert_gpio_hog_config(spec, expected);
	}
}
#endif

ZTEST_SUITE(gpio_hogs_init_by_app, NULL, NULL, NULL, NULL, NULL);
