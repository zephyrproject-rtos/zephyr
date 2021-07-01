/*
 * ...
 *
 * ...
 *
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/pinmux.h>
#include <zephyr/types.h>
#include <sys/sys_io.h>
#include <sys/util.h>
#include <string.h>
#include <logging/log.h>
#include "gpio_utils.h"

#define DT_DRV_COMPAT ambiq_apollo3_gpio

#define PADKEY_KEY_VALUE (0x73)

/*
 * Structure gpio_ambiq_cfg maps to the registers used to configure
 * and managed the gpio pins.
 */
struct gpio_ambiq_cfg {
	struct gpio_driver_config common;
	/* gpio configuration */
	uintptr_t reg_cfg;
	/* gpio port alt configuration */
	uintptr_t reg_rd;
	/* gpio port output status */
	uintptr_t reg_wt;
	/* gpio port set output enable */
	uintptr_t reg_wts;
	/* gpio port clear output disable */
	uintptr_t reg_wtc;
	/* gpio port high impendance status */
	uintptr_t reg_en;
	/* gpio port high impendance enable */
	uintptr_t reg_ens;
	/* gpio port high impendance disable */
	uintptr_t reg_enc;
	uint8_t ngpios;
	/* gpio's irq */
	uint8_t gpio_irq[8];
};

/* Structure gpio_ambiq_data is about callback function */
struct gpio_ambiq_data {
	struct gpio_driver_data common;
	const struct device *pinmux;

	sys_slist_t callbacks;
};

/* dev macros for GPIO */
#define DEV_GPIO_DATA(dev) ((struct gpio_ambiq_data *)(dev)->data)

#define DEV_GPIO_CFG(dev) ((const struct gpio_ambiq_cfg *)(dev)->config)

static void unlock_registers(void)
{
	sys_write32(PADKEY_KEY_VALUE, (uintptr_t)0x40010060);
}

static void lock_registers(void)
{
	sys_write32(0, (uintptr_t)0x40010060);
}

/**
 * Driver functions
 */

static int gpio_ambiq_configure(const struct device *dev, gpio_pin_t pin,
				gpio_flags_t flags)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);

	//
	// TODO(michalc): here, use pinmux driver
	//
	// pinmux_ambiq_pullup(data->pinmux, pin, PINMUX_PULL_ENABLE);

	volatile uint32_t *reg_ens = (uint8_t *)gpio_config->reg_ens;
	volatile uint32_t *reg_wts = (uint8_t *)gpio_config->reg_wts;
	volatile uint32_t *reg_wtc = (uint8_t *)gpio_config->reg_wtc;
	const uint32_t mask = BIT(pin % 32);

	if (pin >= gpio_config->ngpios) {
		printk("GPIO pin number exceeds range for this GPIO port! (%s%d)\n",
		       dev->name, pin);
		return -EINVAL;
	}

	unlock_registers();

	if (flags & GPIO_INPUT) {
		pinmux_ambiq_input(data->pinmux, pin, PINMUX_INPUT_ENABLED);
	} else if (flags & GPIO_OUTPUT) {
		/* Set the output level. */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			*reg_wts |= mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			*reg_wtc |= mask;
		}
		/* Disable the input. */
		*reg_padreg &= ~((pin % 4) + 1);
	}

	/* Handle pullup / pulldown */
	if (flags & GPIO_PULL_UP) {
		pinmux_ambiq_pullup(data->pinmux, pin, PINMUX_PULLUP_ENABLE);
	} else if (flags & GPIO_PULL_DOWN) {
		if (pin == 20) {
			*reg_padreg |= (1 << (pin % 4));
		} else {
			printk("Only pad 20 has a built in pull down resistor!\n");
			return -EINVAL;
		}
	}

	lock_registers();

	return 0;
}

static int gpio_ambiq_port_get_raw(const struct device *dev,
				   gpio_port_value_t *value)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);

	/* Get raw bits of the GPIO RD (read) register. */
	*value = sys_read32(gpio_config->reg_rd);

	return 0;
}

static int gpio_ambiq_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_ens = (uint8_t *)gpio_config->reg_ens;
	uint8_t out = *reg_ens;

	*reg_wts = ((out & ~mask) | (value & mask));

	return 0;
}

static int gpio_ambiq_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);

	uint32_t reg_wts = sys_read32(gpio_config->reg_wts);
	reg_wts |= pins;
	sys_write32(reg_wts, gpio_config->reg_wts);

	return 0;
}

static int gpio_ambiq_port_clear_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);

	uint32_t reg_wtc = sys_read32(gpio_config->reg_wtc);
	reg_wtc |= pins;
	sys_write32(reg_wtc, gpio_config->reg_wtc);

	return 0;
}

static int gpio_ambiq_port_toggle_bits(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_ambiq_cfg *gpio_config = DEV_GPIO_CFG(dev);

	uint32_t reg_wt = sys_read32(gpio_config->reg_wt);
	// TODO(michalc): not sure about this.
	reg_wt ^= pins;
	sys_write32(reg_wt, gpio_config->reg_wt);

	return 0;
}

static int gpio_ambiq_manage_callback(const struct device *dev,
				      struct gpio_callback *callback, bool set)
{
	struct gpio_ambiq_data *data = DEV_GPIO_DATA(dev);

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_ambiq_isr(const void *arg)
{
	uint8_t irq = ite_intc_get_irq_num();
	const struct device *dev = arg;
	struct gpio_ite_data *data = DEV_GPIO_DATA(dev);
	uint8_t gpio_mask = gpio_irqs[irq].gpio_mask;

	if (gpio_irqs[irq].wuc_group) {
		/* Clear the WUC status register. */
		*(wuesr(gpio_irqs[irq].wuc_group)) = gpio_irqs[irq].wuc_mask;
		gpio_fire_callbacks(&data->callbacks, dev, gpio_mask);
	}
}

static int gpio_ite_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	(void)dev;
	(void)pin;
	(void)mode;
	(void)trig;

	return 0;
}

static const struct gpio_driver_api gpio_ambiq_driver_api = {
	.pin_configure = gpio_ambiq_configure,
	.port_get_raw = gpio_ambiq_port_get_raw,
	.port_set_masked_raw = gpio_ambiq_port_set_masked_raw,
	.port_set_bits_raw = gpio_ambiq_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ambiq_port_clear_bits_raw,
	.port_toggle_bits = gpio_ambiq_port_toggle_bits,
	.pin_interrupt_configure = gpio_ambiq_pin_interrupt_configure,
	.manage_callback = gpio_ambiq_manage_callback,
};

static int gpio_ambiq_init(const struct device *dev)
{
	struct gpio_ambiq_data *data = dev->data;

	data->pinmux = DEVICE_DT_GET(DT_NODELABEL(pinmux));
	if ((data->pinmux != NULL) && !device_is_read(data->pinmux)))
		{
			data->pinmux = NULL;
		}

	if (!data->pinmux) {
		return -ENOTSUP;
	}

	return 0;
}

#define GPIO_AMBIQ_DEV_CFG_DATA(inst)                                          \
	static struct gpio_ambiq_data gpio_ambiq_data_##inst = {               \
		.common = {                                                    \
			   .invert = FIXME;                                    \
		},                                                             \
		.pinmux = NULL,                                                \
	};                                                                     \
	static const struct gpio_ambiq_cfg gpio_ambiq_cfg_##inst = {           \
		.common = {                                                    \
			.port_pin_mask = FIXME,                                \
		},                                                             \
		.reg_cfg = DT_INST_REG_ADDR_BY_IDX(inst, 0),                   \
		.reg_rd = DT_INST_REG_ADDR_BY_IDX(inst, 1),                    \
		.reg_wt = DT_INST_REG_ADDR_BY_IDX(inst, 2),                    \
		.reg_wts = DT_INST_REG_ADDR_BY_IDX(inst, 3),                   \
		.reg_wtc = DT_INST_REG_ADDR_BY_IDX(inst, 4),                   \
		.reg_en = DT_INST_REG_ADDR_BY_IDX(inst, 5),                    \
		.reg_ens = DT_INST_REG_ADDR_BY_IDX(inst, 6),                   \
		.reg_enc = DT_INST_REG_ADDR_BY_IDX(inst, 7),                   \
		.ngpios = DT_PROP(DT_INST(inst, ambiq_apollo3_gpio), ngpios),  \
		.gpio_irq[0] = DT_INST_IRQ_BY_IDX(inst, 0, irq),               \
		.gpio_irq[1] = DT_INST_IRQ_BY_IDX(inst, 1, irq),               \
		.gpio_irq[2] = DT_INST_IRQ_BY_IDX(inst, 2, irq),               \
		.gpio_irq[3] = DT_INST_IRQ_BY_IDX(inst, 3, irq),               \
		.gpio_irq[4] = DT_INST_IRQ_BY_IDX(inst, 4, irq),               \
		.gpio_irq[5] = DT_INST_IRQ_BY_IDX(inst, 5, irq),               \
		.gpio_irq[6] = DT_INST_IRQ_BY_IDX(inst, 6, irq),               \
		.gpio_irq[7] = DT_INST_IRQ_BY_IDX(inst, 7, irq),               \
	}; \
	DEVICE_DT_INST_DEFINE(inst, gpio_ambiq_init, device_pm_control_nop,      \
			      &gpio_ambiq_data_##inst, &gpio_ambiq_cfg_##inst,   \
			      POST_KERNEL,                                       \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,               \
			      &gpio_ambiq_driver_api);

// Create a structure for each "okay"-ed gpio port node.
DT_INST_FOREACH_STATUS_OKAY(GPIO_AMBIQ_DEV_CFG_DATA)
