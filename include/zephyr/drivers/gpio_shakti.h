// #ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
// #define ZEPHYR_INCLUDE_DRIVERS_GPIO_H_

#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/kernel.h>

typedef uint8_t gpio_pin_t;
typedef uint32_t gpio_flags_t;

int gpio_shakti_init(const struct device *dev);

static int gpio_shakti_pin_configure(const struct device*dev,
                    gpio_pin_t pin,
                    gpio_flags_t flags);

static int gpio_shakti_pin_get_raw(const struct device *dev,
                    gpio_pin_t pin);

static int gpio_shakti_pin_set_raw(const struct device *dev,
                    gpio_pin_t pin,
                    int value);

static int gpio_shakti_pin_clear_raw(const struct device *dev,
                    gpio_pin_t pin);

static int gpio_shakti_pin_toggle(const struct device *dev,
                    gpio_pin_t pin); 