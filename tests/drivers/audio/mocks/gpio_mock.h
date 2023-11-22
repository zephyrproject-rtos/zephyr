#ifndef GPIO_MOCK_H
#define GPIO_MOCK_H

#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>

typedef uint32_t gpio_flags_t;
typedef uint8_t gpio_pin_t;

DECLARE_FAKE_VALUE_FUNC(int, gpio_pin_configure, const struct device *, gpio_pin_t, gpio_flags_t);
DECLARE_FAKE_VALUE_FUNC(int, gpio_pin_configure_dt, const struct gpio_dt_spec *, gpio_flags_t);
DECLARE_FAKE_VALUE_FUNC(int, gpio_pin_interrupt_configure, const struct device, gpio_pin_t,
                        gpio_flags_t);

#endif /* GPIO_MOCK_H */
