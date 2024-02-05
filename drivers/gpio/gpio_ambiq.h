#ifndef ZEPHYR_DRIVER_GPIO_GPIO_AMBIQ_H_
#define ZEPHYR_DRIVER_GPIO_GPIO_AMBIQ_H_

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <am_mcu_apollo.h>

typedef void (*ambiq_gpio_cfg_func_t)(void);

struct ambiq_gpio_config {
	struct gpio_driver_config common;
	uint32_t base;
	uint32_t offset;
	uint32_t irq_num;
	ambiq_gpio_cfg_func_t cfg_func;
	uint8_t ngpios;
};

struct ambiq_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
	struct k_spinlock lock;
};

int ambiq_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value);
int ambiq_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins);
int ambiq_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins);
int ambiq_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins);
int ambiq_gpio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set);
#endif
