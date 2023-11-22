#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>

#include "gpio_mock.h"

DEFINE_FAKE_VALUE_FUNC(int, gpio_pin_configure, const struct device *, gpio_pin_t, gpio_flags_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_pin_configure_dt, const struct gpio_dt_spec *, gpio_flags_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_pin_interrupt_configure, const struct device, gpio_pin_t,
                       gpio_flags_t);
