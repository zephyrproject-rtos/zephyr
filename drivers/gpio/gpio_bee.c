/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#include <zephyr/dt-bindings/gpio/realtek-bee-gpio.h>

#ifdef GPIO_INT_MASK
#undef GPIO_INT_MASK
#endif

#include <rtl_rcc.h>
#include <rtl_pinmux.h>
#include <rtl_gpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_bee, CONFIG_GPIO_LOG_LEVEL);

#define GPIO_GET_PORT_INT_STATUS(port) (((GPIO_TypeDef *)(port))->GPIO_INT_STS)
#define GPIO_GET_PORT_DIRECTION(port)  (((GPIO_TypeDef *)(port))->GPIO_DDR)

struct gpio_pad_node {
	uint8_t pad_num;
	uint8_t pin_debounce_ms;
	bool both_edge;
};

struct gpio_bee_irq_info {
	const struct device *irq_dev;
	uint8_t num_irq;
	struct gpio_irq_info {
		uint32_t irq;
		uint32_t priority;
	} gpio_irqs[];
};

struct gpio_bee_config {
	struct gpio_driver_config common;
	uint16_t clkid;
	GPIO_TypeDef *port_base;
	const struct pinctrl_dev_config *pcfg;
	struct gpio_bee_irq_info *irq_info;
};

struct gpio_bee_data {
	const struct device *dev;
	sys_slist_t cb;
	struct gpio_pad_node *array;
};

static uint32_t gpio_bee_get_pull_config(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return PAD_PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		return PAD_PULL_DOWN;
	} else {
		return PAD_PULL_NONE;
	}
}

static void gpio_bee_fill_init_struct(GPIO_InitTypeDef *init_struct, uint32_t gpio_bit,
				      gpio_flags_t flags, uint8_t debounce_ms)
{
	GPIO_StructInit(init_struct);

	if (debounce_ms) {
		init_struct->GPIO_DebounceClkSource = GPIO_DEBOUNCE_32K;
		init_struct->GPIO_DebounceClkDiv = GPIO_DEBOUNCE_DIVIDER_32;
		init_struct->GPIO_DebounceCntLimit = debounce_ms;
		init_struct->GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
	} else {
		init_struct->GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;
	}

	init_struct->GPIO_Pin = gpio_bit;
	init_struct->GPIO_Mode = flags & GPIO_OUTPUT ? GPIO_Mode_OUT : GPIO_Mode_IN;
	init_struct->GPIO_OutPutMode =
		flags & GPIO_OPEN_DRAIN ? GPIO_OUTPUT_OPENDRAIN : GPIO_OUTPUT_PUSHPULL;
	init_struct->GPIO_ITCmd = flags & GPIO_INT_ENABLE ? ENABLE : DISABLE;
	init_struct->GPIO_ITTrigger =
		flags & GPIO_INT_EDGE ? GPIO_INT_Trigger_EDGE : GPIO_INT_Trigger_LEVEL;
	init_struct->GPIO_ITPolarity = flags & GPIO_INT_LOW_0 ? GPIO_INT_POLARITY_ACTIVE_LOW
							      : GPIO_INT_POLARITY_ACTIVE_HIGH;
}

static int gpio_bee_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_bee_config *config = port->config;
	struct gpio_bee_data *data = port->data;
	GPIO_TypeDef *port_base = config->port_base;
	uint32_t gpio_bit = BIT(pin);
	int pad_pin = data->array[pin].pad_num;
	uint32_t pull_config;
	GPIO_InitTypeDef gpio_init_struct;
	uint8_t debounce_ms =
		(flags & BEE_GPIO_INPUT_DEBOUNCE_MS_MASK) >> BEE_GPIO_INPUT_DEBOUNCE_MS_POS;

	LOG_DBG("port=%s, pin=%d, flags=0x%x, line%d", port->name, pin, flags, __LINE__);

	__ASSERT(pad_pin < TOTAL_PIN_NUM, "gpio port or pin error");

	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if (flags == GPIO_DISCONNECTED) {
		Pinmux_Deinit(pad_pin);
		Pad_Config(pad_pin, PAD_SW_MODE, PAD_NOT_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
			   PAD_OUT_HIGH);
		return 0;
	}

	pull_config = gpio_bee_get_pull_config(flags);

	if (debounce_ms) {
		data->array[pin].pin_debounce_ms = debounce_ms;
	} else {
		data->array[pin].pin_debounce_ms = 0;
	}

	gpio_bee_fill_init_struct(&gpio_init_struct, gpio_bit, flags, debounce_ms);

	Pad_Dedicated_Config(pad_pin, DISABLE);
	Pad_Config(pad_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, pull_config,
		   (flags & GPIO_OUTPUT) ? PAD_OUT_ENABLE : PAD_OUT_DISABLE,
		   (flags & GPIO_OUTPUT_INIT_HIGH) ? PAD_OUT_HIGH : PAD_OUT_LOW);
	Pinmux_Config(pad_pin, DWGPIO);

	switch (flags & (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) {
	case (GPIO_OUTPUT_HIGH):
		GPIO_WriteBit(port_base, gpio_bit, 1);
		break;
	case (GPIO_OUTPUT_LOW):
		GPIO_WriteBit(port_base, gpio_bit, 0);
		break;
	default:
		break;
	}

	/* to avoid trigger gpio interrupt */
	if (debounce_ms && (flags & GPIO_INT_ENABLE)) {
		GPIO_INTConfig(port_base, gpio_bit, DISABLE);
		GPIO_Init(port_base, &gpio_init_struct);
		GPIO_MaskINTConfig(port_base, gpio_bit, ENABLE);
		GPIO_INTConfig(port_base, gpio_bit, ENABLE);
		k_busy_wait(data->array[pin].pin_debounce_ms * 2 * USEC_PER_MSEC);
		GPIO_ClearINTPendingBit(port_base, gpio_bit);
		GPIO_MaskINTConfig(port_base, gpio_bit, DISABLE);
	} else {
		GPIO_Init(port_base, &gpio_init_struct);
	}

	return 0;
}

static int gpio_bee_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	*value = GPIO_ReadInputData(port_base);

	return 0;
}

static int gpio_bee_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	gpio_port_pins_t pins_value = GPIO_ReadInputData(port_base);

	pins_value = (pins_value & ~mask) | (mask & value);
	GPIO_Write(port_base, pins_value);

	return 0;
}

static int gpio_bee_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	GPIO_SetBits(port_base, pins);

	return 0;
}

static int gpio_bee_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	GPIO_ResetBits(port_base, pins);

	return 0;
}

static int gpio_bee_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	uint32_t pins_value = GPIO_ReadInputData(port_base);

	pins_value = pins_value ^ pins;
	GPIO_Write(port_base, pins_value);
	LOG_DBG("port=%s, pin=0x%x, pins_value=0x%x, line%d", port->name, pins, pins_value,
		__LINE__);

	return 0;
}

static int gpio_bee_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_bee_config *config = port->config;
	struct gpio_bee_data *data = port->data;
	GPIO_TypeDef *port_base = config->port_base;
	uint32_t gpio_bit = BIT(pin);
	GPIO_InitTypeDef gpio_init_struct;

	LOG_DBG("port=%s, pin=%d, mode=0x%x, trig=0x%x, line%d", port->name, pin, mode, trig,
		__LINE__);

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		GPIO_MaskINTConfig(port_base, gpio_bit, ENABLE);
		GPIO_INTConfig(port_base, gpio_bit, DISABLE);
		return 0;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		GPIO_INTConfig(port_base, gpio_bit, ENABLE);
		GPIO_MaskINTConfig(port_base, gpio_bit, DISABLE);
		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	GPIO_INTConfig(port_base, gpio_bit, DISABLE);

	GPIO_StructInit(&gpio_init_struct);

	gpio_init_struct.GPIO_Pin = gpio_bit;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_IN;
	if (data->array[pin].pin_debounce_ms) {
		gpio_init_struct.GPIO_DebounceClkSource = GPIO_DEBOUNCE_32K;
		gpio_init_struct.GPIO_DebounceClkDiv = GPIO_DEBOUNCE_DIVIDER_32;
		gpio_init_struct.GPIO_DebounceCntLimit = data->array[pin].pin_debounce_ms;
		gpio_init_struct.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
	} else {
		gpio_init_struct.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		gpio_init_struct.GPIO_ITCmd = ENABLE;
		gpio_init_struct.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		gpio_init_struct.GPIO_ITCmd = ENABLE;
		gpio_init_struct.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
	} else {
		return -ENOTSUP;
	}

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		data->array[pin].both_edge = false;
		gpio_init_struct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
		break;
	case GPIO_INT_TRIG_HIGH:
		data->array[pin].both_edge = false;
		gpio_init_struct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
		break;
	case GPIO_INT_TRIG_BOTH:
		data->array[pin].both_edge = true;
		gpio_init_struct.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
		gpio_init_struct.GPIO_ITPolarity = GPIO_ReadInputDataBit(port_base, gpio_bit)
							   ? GPIO_INT_POLARITY_ACTIVE_LOW
							   : GPIO_INT_POLARITY_ACTIVE_HIGH;
		break;
	default:
		return -ENOTSUP;
	}

	GPIO_Init(port_base, &gpio_init_struct);
	GPIO_MaskINTConfig(port_base, gpio_bit, ENABLE);
	GPIO_INTConfig(port_base, gpio_bit, ENABLE);

	/* to avoid trigger gpio interrupt */
	if (data->array[pin].pin_debounce_ms) {
		k_busy_wait(data->array[pin].pin_debounce_ms * 2 * USEC_PER_MSEC);
	}

	GPIO_ClearINTPendingBit(port_base, gpio_bit);
	GPIO_MaskINTConfig(port_base, gpio_bit, DISABLE);

	return 0;
}

static int gpio_bee_manage_callback(const struct device *port, struct gpio_callback *cb, bool set)
{
	struct gpio_bee_data *data = port->data;

	return gpio_manage_callback(&data->cb, cb, set);
}

static uint32_t gpio_bee_get_pending_int(const struct device *port)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;

	return GPIO_GET_PORT_INT_STATUS(port_base);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
int gpio_bee_port_get_direction(const struct device *port, gpio_port_pins_t map,
				gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_bee_config *config = port->config;
	GPIO_TypeDef *port_base = config->port_base;
	gpio_port_pins_t gpio_dir_status = GPIO_GET_PORT_DIRECTION(port_base);

	if (inputs != NULL) {
		*inputs = gpio_dir_status;
	}

	if (outputs != NULL) {
		*outputs = ~gpio_dir_status;
	}

	return 0;
}
#endif

static void gpio_bee_isr(void *arg)
{
	const struct device *dev = (struct device *)arg;
	const struct gpio_bee_config *config = dev->config;
	struct gpio_bee_data *data = dev->data;
	GPIO_TypeDef *port_base = config->port_base;
	uint32_t pins = GPIO_GET_PORT_INT_STATUS(port_base);

	for (uint32_t i = 0; i < 32; i++) {
		if ((BIT(i) & pins) && data->array[i].both_edge) {
			GPIO_SetPolarity(port_base, BIT(i),
					 GPIO_ReadInputDataBit(port_base, BIT(i))
						 ? GPIO_INT_POLARITY_ACTIVE_LOW
						 : GPIO_INT_POLARITY_ACTIVE_HIGH);
		}
	}

	gpio_fire_callbacks(&data->cb, dev, pins);

	GPIO_ClearINTPendingBit(port_base, UINT32_MAX);
}

static int gpio_bee_init(const struct device *dev)
{
	struct gpio_bee_data *data = dev->data;
	const struct gpio_bee_config *config = dev->config;
	const struct pinctrl_state *state;
	uint8_t pin_num, pad_num;
	int ret;

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	for (uint8_t i = 0; i < config->irq_info->num_irq; ++i) {
		irq_connect_dynamic(config->irq_info->gpio_irqs[i].irq,
				    config->irq_info->gpio_irqs[i].priority,
				    (const void *)gpio_bee_isr, dev, 0);
		irq_enable(config->irq_info->gpio_irqs[i].irq);
	}

	data->dev = dev;

	for (uint8_t i = 0; i < 32; i++) {
		data->array[i].pad_num = TOTAL_PIN_NUM;
	}

	ret = pinctrl_lookup_state(config->pcfg, PINCTRL_STATE_DEFAULT, &state);
	if ((ret < 0) && (ret != -ENOENT)) {
		LOG_ERR("GPIO relate pins should be configured on dts pinctrl node");
		return -EIO;
	}

	for (uint8_t state_cnt = 0; state_cnt < state->pin_cnt; state_cnt++) {
		pad_num = state->pins[state_cnt].pin;
		pin_num = GPIO_GetNum(pad_num) & 31;
		if (pin_num == 0xff) {
			LOG_ERR("Wrong pad is configured as GPIO.");
			continue;
		}

		if (data->array[pin_num].pad_num != TOTAL_PIN_NUM) {
			LOG_ERR("Redundant configuration for different pads(%d) "
				"using the same GPIO pin(%s: %d).",
				data->array[pin_num].pad_num, dev->name, pin_num);
			continue;
		}

		data->array[pin_num].pad_num = pad_num;
	}

	return ret;
}

static DEVICE_API(gpio, gpio_bee_driver_api) = {
	.pin_configure = gpio_bee_pin_configure,
	.port_get_raw = gpio_bee_port_get_raw,
	.port_set_masked_raw = gpio_bee_port_set_masked_raw,
	.port_set_bits_raw = gpio_bee_port_set_bits_raw,
	.port_clear_bits_raw = gpio_bee_port_clear_bits_raw,
	.port_toggle_bits = gpio_bee_port_toggle_bits,
	.pin_interrupt_configure = gpio_bee_pin_interrupt_configure,
	.manage_callback = gpio_bee_manage_callback,
	.get_pending_int = gpio_bee_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_bee_port_get_direction,
#endif
};

#define GPIO_BEE_SET_GPIO_IRQ_INFO(irq_idx, index)                                                 \
	{                                                                                          \
		.irq = DT_INST_IRQ_BY_IDX(index, irq_idx, irq),                                    \
		.priority = DT_INST_IRQ_BY_IDX(index, irq_idx, priority),                          \
	}

/* clang-format off */

#define GPIO_BEE_SET_IRQ_INFO(index)                                                           \
	static struct gpio_bee_irq_info gpio_bee_irq_info##index = {                           \
		.gpio_irqs = {                                                                 \
			LISTIFY(DT_NUM_IRQS(DT_DRV_INST(index)), GPIO_BEE_SET_GPIO_IRQ_INFO,   \
				(,), index)                                                    \
		},                                                                             \
		.num_irq = DT_NUM_IRQS(DT_DRV_INST(index))                                     \
	};

/* clang-format on */

#define GPIO_BEE_GET_IRQ_INFO(index) .irq_info = &gpio_bee_irq_info##index,

#define GPIO_BEE_ARRAY_DEFINE(index) struct gpio_pad_node gpio_pad_node_array##index[32];

#define GPIO_BEE_DATA_INIT(index) .array = gpio_pad_node_array##index,

#define GPIO_BEE_DEVICE_INIT(index)                                                                \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	GPIO_BEE_ARRAY_DEFINE(index)                                                               \
	GPIO_BEE_SET_IRQ_INFO(index)                                                               \
	static const struct gpio_bee_config gpio_bee_port##index##_cfg = {                         \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(index),           \
			},                                                                         \
		.port_base = (GPIO_TypeDef *)DT_INST_REG_ADDR(index),                              \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		GPIO_BEE_GET_IRQ_INFO(index)};                                                     \
                                                                                                   \
	static struct gpio_bee_data gpio_bee_port##index##_data = {GPIO_BEE_DATA_INIT(index)};     \
	DEVICE_DT_INST_DEFINE(index, gpio_bee_init, NULL, &gpio_bee_port##index##_data,            \
			      &gpio_bee_port##index##_cfg, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, \
			      &gpio_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BEE_DEVICE_INIT)
