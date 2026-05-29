/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_gpio_1_00_a

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

/* AXI GPIO v2 register offsets (See Xilinx PG144 for details) */
#define GPIO_DATA_OFFSET  0x0000
#define GPIO_TRI_OFFSET   0x0004
#define GPIO2_OFFSET      0x0008
#define GPIO2_DATA_OFFSET 0x0008
#define GPIO2_TRI_OFFSET  0x000c
#define GIER_OFFSET       0x011c
#define IPISR_OFFSET      0x0120
#define IPIER_OFFSET      0x0128

/* GIER bit definitions */
#define GIER_GIE BIT(31)

/* IPISR and IPIER bit definitions */
#define IPIXX_CH1_IE BIT(0)
#define IPIXX_CH2_IE BIT(1)

/* Maximum number of GPIOs supported per channel */
#define MAX_GPIOS 32

struct gpio_xlnx_axi_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mm_reg_t base;
	uint8_t channel;
	bool all_inputs: 1;
	bool all_outputs: 1;
	bool interrupts_available: 1;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct gpio_xlnx_axi_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Shadow registers for data out and tristate */
	uint32_t dout;
	uint32_t tri;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	uint32_t previous_data_reading;
	sys_slist_t callbacks;
	uint32_t rising_edge_interrupts;
	uint32_t falling_edge_interrupts;
	/* Workaround to handle channel 2 interrupts from channel 1*/
	const struct device *other_channel_device;
#endif
};

static inline uint32_t gpio_xlnx_axi_read_data(const struct device *dev)
{
	const struct gpio_xlnx_axi_config *config = dev->config;

	return sys_read32(config->base + (config->channel * GPIO2_OFFSET) + GPIO_DATA_OFFSET);
}

static inline void gpio_xlnx_axi_write_data(const struct device *dev, uint32_t val)
{
	const struct gpio_xlnx_axi_config *config = dev->config;

	sys_write32(val, config->base + (config->channel * GPIO2_OFFSET) + GPIO_DATA_OFFSET);
}

static inline void gpio_xlnx_axi_write_tri(const struct device *dev, uint32_t val)
{
	const struct gpio_xlnx_axi_config *config = dev->config;

	sys_write32(val, config->base + (config->channel * GPIO2_OFFSET) + GPIO_TRI_OFFSET);
}

static int gpio_xlnx_axi_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_xlnx_axi_config *config = dev->config;
	struct gpio_xlnx_axi_data *data = dev->data;
	unsigned int key;

	if (!(BIT(pin) & config->common.port_pin_mask)) {
		return -EINVAL;
	}

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_INPUT) != 0) && config->all_outputs) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_OUTPUT) != 0) && config->all_inputs) {
		return -ENOTSUP;
	}

	key = irq_lock();

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		data->tri |= BIT(pin);
		break;
	case GPIO_OUTPUT:
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			data->dout |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			data->dout &= ~BIT(pin);
		}
		data->tri &= ~BIT(pin);
		break;
	default:
		return -ENOTSUP;
	}

	gpio_xlnx_axi_write_data(dev, data->dout);
	gpio_xlnx_axi_write_tri(dev, data->tri);

	irq_unlock(key);

	return 0;
}

static int gpio_xlnx_axi_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	*value = gpio_xlnx_axi_read_data(dev);
	return 0;
}

static int gpio_xlnx_axi_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	struct gpio_xlnx_axi_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->dout = (data->dout & ~mask) | (mask & value);
	gpio_xlnx_axi_write_data(dev, data->dout);
	irq_unlock(key);

	return 0;
}

static int gpio_xlnx_axi_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_xlnx_axi_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->dout |= pins;
	gpio_xlnx_axi_write_data(dev, data->dout);
	irq_unlock(key);

	return 0;
}

static int gpio_xlnx_axi_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_xlnx_axi_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->dout &= ~pins;
	gpio_xlnx_axi_write_data(dev, data->dout);
	irq_unlock(key);

	return 0;
}

static int gpio_xlnx_axi_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_xlnx_axi_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->dout ^= pins;
	gpio_xlnx_axi_write_data(dev, data->dout);
	irq_unlock(key);

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
/**
 * Enables interrupts for the given pins on the channel
 * The axi gpio can only enable interrupts for an entire port, so we need to track
 * the pins and modes ourselves.
 */
static int gpio_xlnx_axi_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_xlnx_axi_config *config = dev->config;
	struct gpio_xlnx_axi_data *data = dev->data;
	const uint32_t pin_mask = BIT(pin);
	const uint32_t chan_mask = BIT(config->channel);
	unsigned int key;
	uint32_t enabled_interrupts;

	if (!config->interrupts_available) {
		return -ENOTSUP;
	}

	if ((mode & GPIO_INT_ENABLE) && !(mode & GPIO_INT_EDGE)) {
		/* only edge detection is supported */
		return -ENOTSUP;
	}

	key = irq_lock();

	data->rising_edge_interrupts &= ~pin_mask;
	data->falling_edge_interrupts &= ~pin_mask;

	if (mode & GPIO_INT_ENABLE) {
		if (trig & GPIO_INT_HIGH_1) {
			data->rising_edge_interrupts |= pin_mask;
		}
		if (trig & GPIO_INT_LOW_0) {
			data->falling_edge_interrupts |= pin_mask;
		}
	}

	/* if there's at least one pin interrupt enabled on the channel, enable the interrupts
	 * for that entire channel without changing the other channel
	 */
	enabled_interrupts = sys_read32(config->base + IPIER_OFFSET);
	if (data->rising_edge_interrupts || data->falling_edge_interrupts) {
		if (!(enabled_interrupts & chan_mask)) {
			/* Clear any pending interrupts and update last state before enabling
			 * interrupt
			 */
			if (sys_read32(config->base + IPISR_OFFSET) & chan_mask) {
				sys_write32(chan_mask, config->base + IPISR_OFFSET);
			}
			data->previous_data_reading = gpio_xlnx_axi_read_data(dev);

			enabled_interrupts |= chan_mask;
		}
	} else {
		enabled_interrupts &= ~chan_mask;
	}
	sys_write32(enabled_interrupts, config->base + IPIER_OFFSET);

	irq_unlock(key);
	return 0;
}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static int gpio_xlnx_axi_manage_callback(const struct device *dev, struct gpio_callback *callback,
					 bool set)
{
	struct gpio_xlnx_axi_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
/**
 * Returns the pins on this devices channel which changed and also have an interrupt enabled on that
 * pin. Also clears the pending interrupt for that channel.
 */
static uint32_t gpio_xlnx_axi_get_pending_int(const struct device *dev)
{
	const struct gpio_xlnx_axi_config *config = dev->config;
	struct gpio_xlnx_axi_data *data = dev->data;
	const uint32_t chan_mask = BIT(config->channel);
	unsigned int key;
	uint32_t interrupt_flags;
	uint32_t current_data;
	uint32_t changed_pins;
	uint32_t changed_and_rising_edge;
	uint32_t changed_and_falling_edge;
	uint32_t interrupts;

	key = irq_lock();

	/* make sure interrupt was for this channel */
	interrupt_flags = sys_read32(config->base + IPISR_OFFSET);

	if (!(interrupt_flags & chan_mask)) {
		irq_unlock(key);
		return 0;
	}

	/* clear pending interrupt for the whole channel */
	sys_write32(chan_mask, config->base + IPISR_OFFSET);

	/* find which pins changed and also have an interrupt enabled */
	current_data = gpio_xlnx_axi_read_data(dev);
	changed_pins = current_data ^ data->previous_data_reading;
	data->previous_data_reading = current_data;
	changed_and_rising_edge = (changed_pins & current_data);
	changed_and_falling_edge = (changed_pins & ~current_data);
	interrupts = (changed_and_rising_edge & data->rising_edge_interrupts) |
		     (changed_and_falling_edge & data->falling_edge_interrupts);

	irq_unlock(key);
	return interrupts;
}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static void gpio_xlnx_axi_isr(const struct device *dev)
{
	struct gpio_xlnx_axi_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, gpio_xlnx_axi_get_pending_int(dev));

	/* Since both channels use the same interrupt, only the first channel registers the ISR.
	 * If the second channel is also enabled, then check for any events on it as well.
	 */
	if (data->other_channel_device) {
		struct gpio_xlnx_axi_data *other_data = data->other_channel_device->data;

		gpio_fire_callbacks(&other_data->callbacks, data->other_channel_device,
				    gpio_xlnx_axi_get_pending_int(data->other_channel_device));
	}
}
#endif

static int gpio_xlnx_axi_init(const struct device *dev)
{
	struct gpio_xlnx_axi_data *data = dev->data;

	gpio_xlnx_axi_write_data(dev, data->dout);
	gpio_xlnx_axi_write_tri(dev, data->tri);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	const struct gpio_xlnx_axi_config *config = dev->config;

	if (config->irq_config_func != NULL) {
		/* Note: This is only called for the first channel, even if the second is enabled.
		 * Need to perform the setup for both channels.
		 * Disable all interrupts.
		 */
		sys_write32(0x0, config->base + IPIER_OFFSET);

		/* Clear all pending interrupts */
		sys_write32(sys_read32(config->base + IPISR_OFFSET), config->base + IPISR_OFFSET);

		/* Enable global interrupts for this gpio device */
		sys_write32(GIER_GIE, config->base + GIER_OFFSET);

		config->irq_config_func(dev);
	}
#endif
	return 0;
}

static DEVICE_API(gpio, gpio_xlnx_axi_driver_api) = {
	.pin_configure = gpio_xlnx_axi_pin_configure,
	.port_get_raw = gpio_xlnx_axi_port_get_raw,
	.port_set_masked_raw = gpio_xlnx_axi_port_set_masked_raw,
	.port_set_bits_raw = gpio_xlnx_axi_port_set_bits_raw,
	.port_clear_bits_raw = gpio_xlnx_axi_port_clear_bits_raw,
	.port_toggle_bits = gpio_xlnx_axi_port_toggle_bits,
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	.pin_interrupt_configure = gpio_xlnx_axi_pin_interrupt_configure,
	.manage_callback = gpio_xlnx_axi_manage_callback,
	.get_pending_int = gpio_xlnx_axi_get_pending_int,
#endif
};

#define GPIO_XLNX_AXI_GPIO2_HAS_COMPAT_STATUS_OKAY(n)                                              \
	UTIL_AND(DT_NODE_HAS_COMPAT(DT_INST_CHILD(n, gpio2), xlnx_xps_gpio_1_00_a_gpio2),          \
		 DT_NODE_HAS_STATUS_OKAY(DT_INST_CHILD(n, gpio2)))

#define GPIO_XLNX_AXI_GPIO2_COND_INIT(n)                                                           \
	IF_ENABLED(UTIL_AND(DT_INST_PROP_OR(n, xlnx_is_dual, 1),                                   \
			    GPIO_XLNX_AXI_GPIO2_HAS_COMPAT_STATUS_OKAY(n)),                        \
		   (GPIO_XLNX_AXI_GPIO2_INIT(n)));

#define GPIO_XLNX_AXI_GPIO2_INIT(n)                                                                \
	static struct gpio_xlnx_axi_data gpio_xlnx_axi_##n##_2_data = {                            \
		.dout = DT_INST_PROP_OR(n, xlnx_dout_default_2, 0),                                \
		.tri = DT_INST_PROP_OR(n, xlnx_tri_default_2, GENMASK(MAX_GPIOS - 1, 0)),          \
	};                                                                                         \
                                                                                                   \
	static const struct gpio_xlnx_axi_config gpio_xlnx_axi_##n##_2_config = {                  \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(                   \
					DT_INST_PROP_OR(n, xlnx_gpio2_width, MAX_GPIOS)),          \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.channel = 1,                                                                      \
		.all_inputs = DT_INST_PROP_OR(n, xlnx_all_inputs_2, 0),                            \
		.all_outputs = DT_INST_PROP_OR(n, xlnx_all_outputs_2, 0),                          \
		.interrupts_available = DT_INST_NODE_HAS_PROP(n, interrupts)};                     \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_INST_CHILD(n, gpio2), &gpio_xlnx_axi_init, NULL,                       \
			 &gpio_xlnx_axi_##n##_2_data, &gpio_xlnx_axi_##n##_2_config, PRE_KERNEL_1, \
			 CONFIG_GPIO_INIT_PRIORITY, &gpio_xlnx_axi_driver_api);

#define GPIO_XLNX_AXI_INIT(n)                                                                      \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, interrupts),                                           \
		   (static void gpio_xlnx_axi_##n##_irq_config(const struct device *dev);))        \
                                                                                                   \
	GPIO_XLNX_AXI_GPIO2_COND_INIT(n);                                                          \
                                                                                                   \
	static struct gpio_xlnx_axi_data gpio_xlnx_axi_##n##_data = {                              \
		.dout = DT_INST_PROP_OR(n, xlnx_dout_default, 0),                                  \
		.tri = DT_INST_PROP_OR(n, xlnx_tri_default, GENMASK(MAX_GPIOS - 1, 0)),            \
		IF_ENABLED(UTIL_AND(UTIL_AND(DT_INST_NODE_HAS_PROP(n, interrupts),                 \
					     DT_INST_PROP_OR(n, xlnx_is_dual, 1)),                 \
				    GPIO_XLNX_AXI_GPIO2_HAS_COMPAT_STATUS_OKAY(n)),                \
			   (.other_channel_device = DEVICE_DT_GET(DT_INST_CHILD(n, gpio2))))};     \
                                                                                                   \
	static const struct gpio_xlnx_axi_config gpio_xlnx_axi_##n##_config = {                    \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(                   \
					DT_INST_PROP_OR(n, xlnx_gpio_width, MAX_GPIOS)),           \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.channel = 0,                                                                      \
		.all_inputs = DT_INST_PROP_OR(n, xlnx_all_inputs, 0),                              \
		.all_outputs = DT_INST_PROP_OR(n, xlnx_all_outputs, 0),                            \
		.interrupts_available = DT_INST_NODE_HAS_PROP(n, interrupts),                      \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, interrupts),                                   \
			   (.irq_config_func = gpio_xlnx_axi_##n##_irq_config))};                  \
                                                                                                   \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, interrupts),                                           \
		   (static void gpio_xlnx_axi_##n##_irq_config(const struct device *dev)           \
		    {                                                                              \
			   ARG_UNUSED(dev);                                                        \
                                                                                                   \
			   IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                  \
				       gpio_xlnx_axi_isr, DEVICE_DT_INST_GET(n), 0);               \
                                                                                                   \
			   irq_enable(DT_INST_IRQN(n));                                            \
		   }))                                                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_xlnx_axi_init, NULL, &gpio_xlnx_axi_##n##_data,              \
			      &gpio_xlnx_axi_##n##_config, PRE_KERNEL_1,                           \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_xlnx_axi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_XLNX_AXI_INIT)
