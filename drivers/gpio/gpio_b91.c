/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>


/* Driver dts compatibility: telink,b91_gpio */
#define DT_DRV_COMPAT telink_b91_gpio

/* Get GPIO instance */
#define GET_GPIO(dev)           ((volatile struct gpio_b91_t *)	\
				 ((const struct gpio_b91_config *)dev->config)->gpio_base)

/* Get GPIO IRQ number defined in dts */
#define GET_IRQ_NUM(dev)        (((const struct gpio_b91_config *)dev->config)->irq_num)

/* Get GPIO IRQ priority defined in dts */
#define GET_IRQ_PRIORITY(dev)   (((const struct gpio_b91_config *)dev->config)->irq_priority)

/* Get GPIO port number: port A - 0, port B - 1, ..., port F - 5  */
#define GET_PORT_NUM(gpio)      ((uint8_t)(((uint32_t)gpio - DT_REG_ADDR(DT_NODELABEL(gpioa))) / \
					   DT_REG_SIZE(DT_NODELABEL(gpioa))))

/* Check that gpio is port C */
#define IS_PORT_C(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpioc)))

/* Check that gpio is port D */
#define IS_PORT_D(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpiod)))

/* Check that 'inst' has only 1 interrupt selected in dts */
#define IS_INST_IRQ_EN(inst)    (DT_NUM_IRQS(DT_DRV_INST(inst)) == 1)

/* Max pin number per port (pin 0..7) */
#define PIN_NUM_MAX ((uint8_t)7u)

/* IRQ Enable registers */
#define reg_irq_risc0_en(i)      REG_ADDR8(0x140338 + i)
#define reg_irq_risc1_en(i)      REG_ADDR8(0x140340 + i)

/* Pull-up/down resistors */
#define GPIO_PIN_UP_DOWN_FLOAT   ((uint8_t)0u)
#define GPIO_PIN_PULLDOWN_100K   ((uint8_t)2u)
#define GPIO_PIN_PULLUP_10K      ((uint8_t)3u)

/* GPIO interrupt types */
#define INTR_RISING_EDGE         ((uint8_t)0u)
#define INTR_FALLING_EDGE        ((uint8_t)1u)
#define INTR_HIGH_LEVEL          ((uint8_t)2u)
#define INTR_LOW_LEVEL           ((uint8_t)3u)

/* Supported IRQ numbers */
#define IRQ_GPIO                 ((uint8_t)25u)
#define IRQ_GPIO2_RISC0          ((uint8_t)26u)
#define IRQ_GPIO2_RISC1          ((uint8_t)27u)


/* B91 GPIO registers structure */
struct gpio_b91_t {
	uint8_t input;                  /* Input: read GPI input */
	uint8_t ie;                     /* IE: input enable, high active. 1: enable, 0: disable */
	uint8_t oen;                    /* OEN: output enable, low active. 0: enable, 1: disable */
	uint8_t output;                 /* Output: configure GPIO output */
	uint8_t polarity;               /* Polarity: interrupt polarity: rising, falling */
	uint8_t ds;                     /* DS: drive strength. 1: maximum (default), 0: minimal */
	uint8_t actas_gpio;             /* Act as GPIO: enable (1) or disable (0) GPIO function */
	uint8_t irq_en;                 /* Act as GPIO: enable (1) or disable (0) GPIO function */
};

/* GPIO driver configuration structure */
struct gpio_b91_config {
	struct gpio_driver_config common;
	uint32_t gpio_base;
	uint32_t irq_num;
	uint8_t irq_priority;
	void (*pirq_connect)(void);
};

/* GPIO driver data structure */
struct gpio_b91_data {
	struct gpio_driver_data common; /* driver data */
	sys_slist_t callbacks;          /* list of callbacks */
};


/* Set IRQ Enable bit based on IRQ number */
static inline void gpiob_b91_irq_en_set(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);

	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	if (irq == IRQ_GPIO) {
		BM_SET(gpio->irq_en, BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC0) {
		BM_SET(reg_irq_risc0_en(GET_PORT_NUM(gpio)), BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC1) {
		BM_SET(reg_irq_risc1_en(GET_PORT_NUM(gpio)), BIT(pin));
	} else {
		__ASSERT(false, "Not supported GPIO IRQ number.");
	}
}

/* Clear IRQ Enable bit based on IRQ number */
static inline void gpiob_b91_irq_en_clr(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	if (irq == IRQ_GPIO) {
		BM_CLR(gpio->irq_en, BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC0) {
		BM_CLR(reg_irq_risc0_en(GET_PORT_NUM(gpio)), BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC1) {
		BM_CLR(reg_irq_risc1_en(GET_PORT_NUM(gpio)), BIT(pin));
	}
}

/* Get IRQ Enable register value */
static inline uint8_t gpio_b91_irq_en_get(const struct device *dev)
{
	uint8_t status = 0;
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	if (irq == IRQ_GPIO) {
		status = gpio->irq_en;
	} else if (irq == IRQ_GPIO2_RISC0) {
		status = reg_irq_risc0_en(GET_PORT_NUM(gpio));
	} else if (irq == IRQ_GPIO2_RISC1) {
		status = reg_irq_risc1_en(GET_PORT_NUM(gpio));
	}

	return status;
}

/* Clear IRQ Status bit */
static inline void gpio_b91_irq_status_clr(uint8_t irq)
{
	gpio_irq_status_e status = 0;

	if (irq == IRQ_GPIO) {
		status = FLD_GPIO_IRQ_CLR;
	} else if (irq == IRQ_GPIO2_RISC0) {
		status = FLD_GPIO_IRQ_GPIO2RISC0_CLR;
	} else if (irq == IRQ_GPIO2_RISC1) {
		status = FLD_GPIO_IRQ_GPIO2RISC1_CLR;
	}

	reg_gpio_irq_clr = status;
}

/* Set pin's irq type */
void gpio_b91_irq_set(const struct device *dev, gpio_pin_t pin,
		      uint8_t trigger_type)
{
	uint8_t irq_lvl = 0;
	uint8_t irq_mask = 0;
	uint8_t irq_num = GET_IRQ_NUM(dev);
	uint8_t irq_prioriy = GET_IRQ_PRIORITY(dev);
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	/* Get level and mask based on IRQ number */
	if (irq_num == IRQ_GPIO) {
		irq_lvl = FLD_GPIO_IRQ_LVL_GPIO;
		irq_mask = FLD_GPIO_IRQ_MASK_GPIO;
	} else if (irq_num == IRQ_GPIO2_RISC0) {
		irq_lvl = FLD_GPIO_IRQ_LVL_GPIO2RISC0;
		irq_mask = FLD_GPIO_IRQ_MASK_GPIO2RISC0;
	} else if (irq_num == IRQ_GPIO2_RISC1) {
		irq_lvl = FLD_GPIO_IRQ_LVL_GPIO2RISC1;
		irq_mask = FLD_GPIO_IRQ_MASK_GPIO2RISC1;
	}

	/* Set polarity and level */
	switch (trigger_type) {
	case INTR_RISING_EDGE:
		BM_CLR(gpio->polarity, BIT(pin));
		BM_CLR(reg_gpio_irq_risc_mask, irq_lvl);
		break;

	case INTR_FALLING_EDGE:
		BM_SET(gpio->polarity, BIT(pin));
		BM_CLR(reg_gpio_irq_risc_mask, irq_lvl);
		break;

	case INTR_HIGH_LEVEL:
		BM_CLR(gpio->polarity, BIT(pin));
		BM_SET(reg_gpio_irq_risc_mask, irq_lvl);
		break;

	case INTR_LOW_LEVEL:
		BM_SET(gpio->polarity, BIT(pin));
		BM_SET(reg_gpio_irq_risc_mask, irq_lvl);
		break;
	}

	if (irq_num == IRQ_GPIO) {
		reg_gpio_irq_ctrl |= FLD_GPIO_CORE_INTERRUPT_EN;
	}
	gpio_b91_irq_status_clr(irq_num);
	BM_SET(reg_gpio_irq_risc_mask, irq_mask);

	/* Enable peripheral interrupt */
	gpiob_b91_irq_en_set(dev, pin);

	/* Enable PLIC interrupt */
	riscv_plic_irq_enable(irq_num);
	riscv_plic_set_priority(irq_num, irq_prioriy);
}

/* Set pin's pull-up/down resistor */
static void gpio_b91_up_down_res_set(volatile struct gpio_b91_t *gpio,
				     gpio_pin_t pin,
				     uint8_t up_down_res)
{
	uint8_t val;
	uint8_t mask;
	uint8_t analog_reg;

	pin = BIT(pin);
	val = up_down_res & 0x03;
	analog_reg = 0x0e + (GET_PORT_NUM(gpio) << 1) + ((pin & 0xf0) ? 1 : 0);

	if (pin & 0x11) {
		val = val << 0;
		mask = 0xfc;
	} else if (pin & 0x22) {
		val = val << 2;
		mask = 0xf3;
	} else if (pin & 0x44) {
		val = val << 4;
		mask = 0xcf;
	} else if (pin & 0x88) {
		val = val << 6;
		mask = 0x3f;
	} else {
		return;
	}

	analog_write_reg8(analog_reg, (analog_read_reg8(analog_reg) & mask) | val);
}

/* Config Pin pull-up / pull-down resistors */
static void gpio_b91_config_up_down_res(volatile struct gpio_b91_t *gpio,
					gpio_pin_t pin,
					gpio_flags_t flags)
{
	if ((flags & GPIO_PULL_UP) != 0) {
		gpio_b91_up_down_res_set(gpio, pin, GPIO_PIN_PULLUP_10K);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		gpio_b91_up_down_res_set(gpio, pin, GPIO_PIN_PULLDOWN_100K);
	} else {
		gpio_b91_up_down_res_set(gpio, pin, GPIO_PIN_UP_DOWN_FLOAT);
	}
}

/* Config Pin In/Out direction */
static void gpio_b91_config_in_out(volatile struct gpio_b91_t *gpio,
				   gpio_pin_t pin,
				   gpio_flags_t flags)
{
	uint8_t ie_addr = 0;

	/* Port C and D Input Enable registers are located in another place: analog */
	if (IS_PORT_C(gpio)) {
		ie_addr = areg_gpio_pc_ie;
	} else if (IS_PORT_D(gpio)) {
		ie_addr = areg_gpio_pd_ie;
	}

	/* Enable/disable output */
	WRITE_BIT(gpio->oen, pin, ~flags & GPIO_OUTPUT);

	/* Enable/disable input */
	if (ie_addr != 0) {
		/* Port C and D are located in analog space */
		if (flags & GPIO_INPUT) {
			analog_write_reg8(ie_addr, analog_read_reg8(ie_addr) | BIT(pin));
		} else {
			analog_write_reg8(ie_addr, analog_read_reg8(ie_addr) & (~BIT(pin)));
		}
	} else {
		/* Input Enable registers of all other ports are located in common GPIO space */
		WRITE_BIT(gpio->ie, pin, flags & GPIO_INPUT);
	}
}

/* GPIO driver initialization */
static int gpio_b91_init(const struct device *dev)
{
	const struct gpio_b91_config *cfg = dev->config;

	cfg->pirq_connect();

	return 0;
}

/* API implementation: pin_configure */
static int gpio_b91_pin_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	/* Check input parameters: pin number */
	if (pin > PIN_NUM_MAX) {
		return -ENOTSUP;
	}

	/* Check input parameters: open-source and open-drain */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Check input parameters: simultaneous in/out mode */
	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		return -ENOTSUP;
	}

	/* Set GPIO init state if defined to avoid glitches */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		gpio->output |= BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		gpio->output &= ~BIT(pin);
	}

	/* GPIO function enable */
	WRITE_BIT(gpio->actas_gpio, BIT(pin), 1);

	/* Set GPIO pull-up / pull-down resistors */
	gpio_b91_config_up_down_res(gpio, pin, flags);

	/* Enable/disable input/output */
	gpio_b91_config_in_out(gpio, pin, flags);

	return 0;
}

/* API implementation: port_get_raw */
static int gpio_b91_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	*value = gpio->input;

	return 0;
}

/* API implementation: port_set_masked_raw */
static int gpio_b91_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	gpio->output = (gpio->output & ~mask) | (value & mask);

	return 0;
}

/* API implementation: port_set_bits_raw */
static int gpio_b91_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t mask)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	gpio->output |= mask;

	return 0;
}

/* API implementation: port_clear_bits_raw */
static int gpio_b91_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t mask)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	gpio->output &= ~mask;

	return 0;
}

/* API implementation: port_toggle_bits */
static int gpio_b91_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t mask)
{
	volatile struct gpio_b91_t *gpio = GET_GPIO(dev);

	gpio->output ^= mask;

	return 0;
}

/* API implementation: interrupts handler */
#if IS_INST_IRQ_EN(0) || IS_INST_IRQ_EN(1) || IS_INST_IRQ_EN(2) || \
	IS_INST_IRQ_EN(3) || IS_INST_IRQ_EN(4)
static void gpio_b91_irq_handler(const struct device *dev)
{
	struct gpio_b91_data *data = dev->data;
	uint8_t irq = GET_IRQ_NUM(dev);
	uint8_t status = gpio_b91_irq_en_get(dev);

	gpio_b91_irq_status_clr(irq);
	gpio_fire_callbacks(&data->callbacks, dev, status);
}
#endif

/* API implementation: pin_interrupt_configure */
static int gpio_b91_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	int ret_status = 0;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:                /* GPIO interrupt disable */
		gpiob_b91_irq_en_clr(dev, pin);
		break;

	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_HIGH) {       /* GPIO interrupt High level */
			gpio_b91_irq_set(dev, pin, INTR_HIGH_LEVEL);
		} else if (trig == GPIO_INT_TRIG_LOW) { /* GPIO interrupt Low level */
			gpio_b91_irq_set(dev, pin, INTR_LOW_LEVEL);
		} else {
			ret_status = -ENOTSUP;
		}
		break;

	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_HIGH) {       /* GPIO interrupt Rising edge */
			gpio_b91_irq_set(dev, pin, INTR_RISING_EDGE);
		} else if (trig == GPIO_INT_TRIG_LOW) { /* GPIO interrupt Falling edge */
			gpio_b91_irq_set(dev, pin, INTR_FALLING_EDGE);
		} else {
			ret_status = -ENOTSUP;
		}
		break;

	default:
		ret_status = -ENOTSUP;
		break;
	}

	return ret_status;
}

/* API implementation: manage_callback */
static int gpio_b91_manage_callback(const struct device *dev,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_b91_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

/* GPIO driver APIs structure */
static DEVICE_API(gpio, gpio_b91_driver_api) = {
	.pin_configure = gpio_b91_pin_configure,
	.port_get_raw = gpio_b91_port_get_raw,
	.port_set_masked_raw = gpio_b91_port_set_masked_raw,
	.port_set_bits_raw = gpio_b91_port_set_bits_raw,
	.port_clear_bits_raw = gpio_b91_port_clear_bits_raw,
	.port_toggle_bits = gpio_b91_port_toggle_bits,
	.pin_interrupt_configure = gpio_b91_pin_interrupt_configure,
	.manage_callback = gpio_b91_manage_callback
};

/* If instance 0 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0
static void gpio_b91_irq_connect_0(void)
{
	#if IS_INST_IRQ_EN(0)
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    gpio_b91_irq_handler,
		    DEVICE_DT_INST_GET(0), 0);
	#endif
}
#endif

/* If instance 1 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
static void gpio_b91_irq_connect_1(void)
{
	#if IS_INST_IRQ_EN(1)
	IRQ_CONNECT(DT_INST_IRQN(1), DT_INST_IRQ(1, priority),
		    gpio_b91_irq_handler,
		    DEVICE_DT_INST_GET(1), 0);
	#endif
}
#endif

/* If instance 2 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 2
static void gpio_b91_irq_connect_2(void)
{
	#if IS_INST_IRQ_EN(2)
	IRQ_CONNECT(DT_INST_IRQN(2), DT_INST_IRQ(2, priority),
		    gpio_b91_irq_handler,
		    DEVICE_DT_INST_GET(2), 0);
	#endif
}
#endif

/* If instance 3 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 3
static void gpio_b91_irq_connect_3(void)
{
	#if IS_INST_IRQ_EN(3)
	IRQ_CONNECT(DT_INST_IRQN(3), DT_INST_IRQ(3, priority),
		    gpio_b91_irq_handler,
		    DEVICE_DT_INST_GET(3), 0);
	#endif
}
#endif

/* If instance 4 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 4
static void gpio_b91_irq_connect_4(void)
{
	#if IS_INST_IRQ_EN(4)
	IRQ_CONNECT(DT_INST_IRQN(4), DT_INST_IRQ(4, priority),
		    gpio_b91_irq_handler,
		    DEVICE_DT_INST_GET(4), 0);
	#endif
}
#endif

/* GPIO driver registration */
#define GPIO_B91_INIT(n)						    \
	static const struct gpio_b91_config gpio_b91_config_##n = {	    \
		.common = {						    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n) \
		},							    \
		.gpio_base = DT_INST_REG_ADDR(n),			    \
		.irq_num = DT_INST_IRQN(n),				    \
		.irq_priority = DT_INST_IRQ(n, priority),		    \
		.pirq_connect = gpio_b91_irq_connect_##n		    \
	};								    \
	static struct gpio_b91_data gpio_b91_data_##n;			    \
									    \
	DEVICE_DT_INST_DEFINE(n, gpio_b91_init,				    \
			      NULL,					    \
			      &gpio_b91_data_##n,			    \
			      &gpio_b91_config_##n,			    \
			      PRE_KERNEL_1,				    \
			      CONFIG_GPIO_INIT_PRIORITY,		    \
			      &gpio_b91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_B91_INIT)
