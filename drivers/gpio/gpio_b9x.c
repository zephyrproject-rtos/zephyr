/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/pm/device.h>

#include <string.h>

#if CONFIG_SOC_RISCV_TELINK_B91
#define GPIO_IRQ_REG reg_gpio_irq_risc_mask
#elif CONFIG_SOC_RISCV_TELINK_B92
#define GPIO_IRQ_REG reg_gpio_irq_ctrl
#include "gpio.h"
#elif CONFIG_SOC_RISCV_TELINK_B95
#define GPIO_IRQ_REG reg_gpio_irq_ctrl
#include "gpio.h"
#else
#error "GPIO driver is unsupported for chosen SoC!"
#endif

/* Driver dts compatibility: telink,b9x_gpio */
#define DT_DRV_COMPAT telink_b9x_gpio

/* Get GPIO instance */
#define GET_GPIO(dev)           ((volatile struct gpio_b9x_t *)	\
				 ((const struct gpio_b9x_config *)dev->config)->gpio_base)

/* Get GPIO IRQ number defined in dts */
#define GET_IRQ_NUM(dev)        (((const struct gpio_b9x_config *)dev->config)->irq_num)

/* Get GPIO IRQ priority defined in dts */
#define GET_IRQ_PRIORITY(dev)   (((const struct gpio_b9x_config *)dev->config)->irq_priority)

/* Get GPIO port number: port A - 0, port B - 1, ..., port F - 5  */
#define GET_PORT_NUM(gpio)      ((uint8_t)(((uint32_t)gpio - DT_REG_ADDR(DT_NODELABEL(gpioa))) / \
					   DT_REG_SIZE(DT_NODELABEL(gpioa))))

/* Check that gpio is port C */
#define IS_PORT_C(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpioc)))

/* Check that gpio is port D */
#define IS_PORT_D(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpiod)))

/* Check that gpio is port F */
#if CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95
#define IS_PORT_F(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpiof)))
#else
#define IS_PORT_F(gpio)         0
#endif

/* Check that gpio is port G */
#if  CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
#define IS_PORT_G(gpio)         0
#elif CONFIG_SOC_RISCV_TELINK_B95
#define IS_PORT_G(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpiog)))
#endif

/* Check that 'inst' has only 1 interrupt selected in dts */
#define IS_INST_IRQ_EN(inst)    (DT_NUM_IRQS(DT_DRV_INST(inst)) == 1)

/* Max pin number per port (pin 0..7) */
#define PIN_NUM_MAX ((uint8_t)7u)

/* IRQ Enable registers */
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
#define reg_irq_risc0_en(i)      REG_ADDR8(0x140338 + i)
#define reg_irq_risc1_en(i)      REG_ADDR8(0x140340 + i)
#elif CONFIG_SOC_RISCV_TELINK_B95
#define reg_irq_risc0_en(i)      REG_ADDR8(0x140c08 + (i << 4))
#define reg_irq_risc1_en(i)      REG_ADDR8(0x140c09 + (i << 4))
#endif

#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
#define GPIO_SET_LOW_LEVEL(gpio, pin)   WRITE_BIT(gpio->output, pin, 0)
#define GPIO_SET_HIGH_LEVEL(gpio, pin)  WRITE_BIT(gpio->output, pin, 1)
#elif CONFIG_SOC_RISCV_TELINK_B95
#define GPIO_SET_LOW_LEVEL(gpio, pin)   WRITE_BIT(gpio->output_clr, pin, 1)
#define GPIO_SET_HIGH_LEVEL(gpio, pin)  WRITE_BIT(gpio->output, pin, 1)
#endif

/* GPIO Wakeup Enable registers */
#if CONFIG_SOC_RISCV_TELINK_B91
#define reg_wakeup_trig_pol_base 0x41
#define reg_wakeup_trig_en_base  0x46
#elif CONFIG_SOC_RISCV_TELINK_B92
#define reg_wakeup_trig_pol_base 0x3f
#define reg_wakeup_trig_en_base  0x45
#elif CONFIG_SOC_RISCV_TELINK_B95
#define reg_wakeup_trig_pol_base 0x3f
#define reg_wakeup_trig_en_base  0x45
#else
#error "SOC version is not supported"
#endif

/* Pull-up/down resistors */
#define GPIO_PIN_UP_DOWN_FLOAT   ((uint8_t)0u)
#define GPIO_PIN_PULLDOWN_100K   ((uint8_t)2u)
#define GPIO_PIN_PULLUP_10K      ((uint8_t)3u)

/* GPIO interrupt types */
#define INTR_RISING_EDGE         ((uint8_t)0u)
#define INTR_FALLING_EDGE        ((uint8_t)1u)

/* Supported IRQ numbers */
#if CONFIG_SOC_RISCV_TELINK_B91
#define IRQ_GPIO                 ((uint8_t)25u)
#endif
#define IRQ_GPIO2_RISC0          ((uint8_t)26u)
#define IRQ_GPIO2_RISC1          ((uint8_t)27u)

#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
/* b9x GPIO registers structure */
struct gpio_b9x_t {
	uint8_t input;                  /* Input: read GPI input */
	uint8_t ie;                     /* IE: input enable, high active. 1: enable, 0: disable */
	uint8_t oen;                    /* OEN: output enable, low active. 0: enable, 1: disable */
	uint8_t output;                 /* Output: configure GPIO output */
	uint8_t polarity;               /* Polarity: interrupt polarity: rising, falling */
	uint8_t ds;                     /* DS: drive strength. 1: maximum (default), 0: minimal */
	uint8_t actas_gpio;             /* Act as GPIO: enable (1) or disable (0) GPIO function */
	uint8_t irq_en;                 /* Act as GPIO: enable (1) or disable (0) GPIO function */
};
#elif CONFIG_SOC_RISCV_TELINK_B95
struct gpio_b9x_t {
	uint8_t input;                  /* Input: read GPI input */
	uint8_t ie;                     /* IE: input enable, high active. 1: enable, 0: disable */
	uint8_t oen;                    /* OEN: output enable, low active. 0: enable, 1: disable */
	uint8_t rsvd0;                  /* reserve */
	uint8_t polarity;               /* Polarity: interrupt polarity: rising, falling */
	uint8_t ds;                     /* DS: drive strength. 1: maximum (default), 0: minimal */
	uint8_t actas_gpio;             /* Act as GPIO: enable (1) or disable (0) GPIO function */
	uint8_t irq_en;                 /* IRQ_EN:GPIO interrupt */
	uint8_t irq_risc0_en;           /* IRQ_RISC0_EN:GPIO2RISC[0] interrupt */
	uint8_t irq_risc1_en;           /* IRQ_RISC1_EN:GPIO2RISC[1] interrupt */
	uint8_t pulldown;               /* PULLDOWN: */
	uint8_t pullup;                 /* PULLDOWN: */
	uint8_t output;                 /* Output: GPIO output set */
	uint8_t output_clr;             /* Output: GPIO output clear */
	uint8_t output_toggle;          /* Output: GPIO output toggle */
	uint8_t rsvd2;                  /* reserve */
};
#endif

/* GPIO IRQ configuration structure */
struct gpio_b9x_pin_irq_config {
	gpio_port_value_t pin_last_value;
	gpio_port_value_t irq_en_rising;
	gpio_port_value_t irq_en_falling;
	gpio_port_value_t irq_en_both;
};

/* GPIO driver configuration structure */
struct gpio_b9x_config {
	struct gpio_driver_config common;
	uint32_t gpio_base;
	uint8_t irq_num;
	uint8_t irq_priority;
	struct gpio_b9x_pin_irq_config *pin_irq_state;
	void (*pirq_connect)(void);
};

struct gpio_b9x_retention_data {
	struct gpio_b9x_t gpio_b9x_periph_config;
	uint8_t gpio_b9x_irq_conf;
	uint8_t analog_in_conf;
	uint8_t analog_pupd_conf[2];
	uint8_t risc0_irq_conf;
	uint8_t risc1_irq_conf;
};

/* GPIO driver data structure */
struct gpio_b9x_data {
	struct gpio_driver_data common; /* driver data */
	sys_slist_t callbacks;          /* list of callbacks */
#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION
	struct gpio_b9x_retention_data gpio_b9x_retention; /* list of necessary retained data */
#endif /* CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */
};

#ifdef CONFIG_PM_DEVICE
/* Set Wake-up Enable bit based on GPIO pin number */
static inline void gpio_b9x_irq_pin_wakeup_set(const struct device *dev, gpio_pin_t pin,
	uint8_t trigger_type)
{
	const uint8_t wakeup_trigger_pol_reg = reg_wakeup_trig_pol_base +
		GET_PORT_NUM(GET_GPIO(dev));
	const uint8_t wakeup_trigger_en_reg = reg_wakeup_trig_en_base +
		GET_PORT_NUM(GET_GPIO(dev));

	switch (trigger_type) {
	case INTR_RISING_EDGE:
		analog_write_reg8(wakeup_trigger_pol_reg,
			analog_read_reg8(wakeup_trigger_pol_reg) & ~BIT(pin));
		break;
	case INTR_FALLING_EDGE:
		analog_write_reg8(wakeup_trigger_pol_reg,
			analog_read_reg8(wakeup_trigger_pol_reg) | BIT(pin));
		break;
	}
	analog_write_reg8(wakeup_trigger_en_reg,
		analog_read_reg8(wakeup_trigger_en_reg) | BIT(pin));
}

/* Clear Wake-up Enable bit based on GPIO pin number */
static inline void gpio_b9x_irq_pin_wakeup_clr(const struct device *dev, gpio_pin_t pin)
{
	const uint8_t wakeup_trigger_en_reg = reg_wakeup_trig_en_base +
		GET_PORT_NUM(GET_GPIO(dev));

	analog_write_reg8(wakeup_trigger_en_reg,
		analog_read_reg8(wakeup_trigger_en_reg) & ~BIT(pin));
}

#endif /* CONFIG_PM_DEVICE */


/* Set IRQ Enable bit based on IRQ number */
static inline void gpio_b9x_irq_en_set(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);

	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

	irq -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

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
static inline void gpio_b9x_irq_en_clr(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

	irq -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

	if (irq == IRQ_GPIO) {
		BM_CLR(gpio->irq_en, BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC0) {
		BM_CLR(reg_irq_risc0_en(GET_PORT_NUM(gpio)), BIT(pin));
	} else if (irq == IRQ_GPIO2_RISC1) {
		BM_CLR(reg_irq_risc1_en(GET_PORT_NUM(gpio)), BIT(pin));
	}

#if CONFIG_PM_DEVICE
	gpio_b9x_irq_pin_wakeup_clr(dev, pin);
#endif /* CONFIG_PM_DEVICE */
}

/* Get IRQ Enable register value */
static inline uint8_t gpio_b9x_irq_en_get(const struct device *dev)
{
	uint8_t status = 0;
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

	irq -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

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
static inline void gpio_b9x_irq_status_clr(uint8_t irq)
{
	gpio_irq_status_e status = 0;

	irq -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

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
void gpio_b9x_irq_set(const struct device *dev, gpio_pin_t pin,
		      uint8_t trigger_type)
{
	uint8_t irq_lvl = 0;
	uint8_t irq_mask = 0;
	uint8_t irq_num = GET_IRQ_NUM(dev);
	uint8_t irq_prioriy = GET_IRQ_PRIORITY(dev);
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

	irq_num -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

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
		BM_CLR(GPIO_IRQ_REG, irq_lvl);
		break;

	case INTR_FALLING_EDGE:
		BM_SET(gpio->polarity, BIT(pin));
		BM_CLR(GPIO_IRQ_REG, irq_lvl);
		break;
	}

	if (irq_num == IRQ_GPIO) {
		reg_gpio_irq_ctrl |= FLD_GPIO_CORE_INTERRUPT_EN;
	}
	gpio_b9x_irq_status_clr(irq_num);
	BM_SET(GPIO_IRQ_REG, irq_mask);

	/* Enable peripheral interrupt */
	gpio_b9x_irq_en_set(dev, pin);

#if CONFIG_PM_DEVICE
	gpio_b9x_irq_pin_wakeup_set(dev, pin, trigger_type);
#endif /* CONFIG_PM_DEVICE */

	/* Enable PLIC interrupt */
	riscv_plic_irq_enable(irq_num);
	riscv_plic_set_priority(irq_num, irq_prioriy);
}

/* Set pin's pull-up/down resistor */
static void gpio_b9x_up_down_res_set(volatile struct gpio_b9x_t *gpio,
				     gpio_pin_t pin,
				     uint8_t up_down_res)
{
	uint8_t val;
	uint8_t mask;
	uint8_t analog_reg;

	pin = BIT(pin);
	val = up_down_res & 0x03;
	#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	if (IS_PORT_F(gpio)) {
		analog_reg = 0x23 + ((pin & 0xf0) ? 1 : 0);
	} else {
		analog_reg = 0x0e + (GET_PORT_NUM(gpio) << 1) + ((pin & 0xf0) ? 1 : 0);
	}
	#elif CONFIG_SOC_RISCV_TELINK_B95
	if ((IS_PORT_F(gpio)) || (IS_PORT_G(gpio))) {
		return;
	}
	analog_reg = 0x17 + (GET_PORT_NUM(gpio) << 1) + ((pin & 0xf0) ? 1 : 0);
	#endif

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
static void gpio_b9x_config_up_down_res(volatile struct gpio_b9x_t *gpio,
					gpio_pin_t pin,
					gpio_flags_t flags)
{
	if ((flags & GPIO_PULL_UP) != 0) {
		gpio_b9x_up_down_res_set(gpio, pin, GPIO_PIN_PULLUP_10K);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		gpio_b9x_up_down_res_set(gpio, pin, GPIO_PIN_PULLDOWN_100K);
	} else {
		gpio_b9x_up_down_res_set(gpio, pin, GPIO_PIN_UP_DOWN_FLOAT);
	}
}

/* Config Pin In/Out direction */
static void gpio_b9x_config_in_out(volatile struct gpio_b9x_t *gpio,
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
	/* Clear GPIO output value for input configuration */
	if (IS_PORT_F(gpio) && (flags & GPIO_INPUT)) {
		GPIO_SET_LOW_LEVEL(gpio, pin);
	}

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
static int gpio_b9x_init(const struct device *dev)
{
	const struct gpio_b9x_config *cfg = dev->config;

	cfg->pirq_connect();

	return 0;
}

/* API implementation: pin_configure */
static int gpio_b9x_pin_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

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

#if CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95
	/* Avoid pulls in B92 SoC in PF[0:5] due to silicone limitation */
	if (IS_PORT_F(gpio) && (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN))
	&& (pin != 6) && (pin != 7)) {
		return -ENOTSUP;
	}
#endif

	/* Set GPIO init state if defined to avoid glitches */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		GPIO_SET_HIGH_LEVEL(gpio, pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		GPIO_SET_LOW_LEVEL(gpio, pin);
	}

	/* GPIO function enable */
	WRITE_BIT(gpio->actas_gpio, pin, 1);

	/* Set GPIO pull-up / pull-down resistors */
	gpio_b9x_config_up_down_res(gpio, pin, flags);

	/* Enable/disable input/output */
	gpio_b9x_config_in_out(gpio, pin, flags);

	return 0;
}

/* API implementation: port_get_raw */
static int gpio_b9x_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);

	*value = gpio->input;

	return 0;
}

/* API implementation: port_set_masked_raw */
static int gpio_b9x_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	gpio->output = (gpio->output & ~mask) | (value & mask);
#elif CONFIG_SOC_RISCV_TELINK_B95
	gpio->output_clr = (mask);
	gpio->output = (value & mask);
#endif
	return 0;
}

/* API implementation: port_set_bits_raw */
static int gpio_b9x_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t mask)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	gpio->output |= mask;
#elif CONFIG_SOC_RISCV_TELINK_B95
	gpio->output = (mask);
#endif
	return 0;
}

/* API implementation: port_clear_bits_raw */
static int gpio_b9x_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t mask)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	gpio->output &= ~mask;
#elif CONFIG_SOC_RISCV_TELINK_B95
	gpio->output_clr = (mask);
#endif
	return 0;
}

/* API implementation: port_toggle_bits */
static int gpio_b9x_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t mask)
{
	volatile struct gpio_b9x_t *gpio = GET_GPIO(dev);
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	gpio->output ^= mask;
#elif CONFIG_SOC_RISCV_TELINK_B95
	uint8_t bits = (mask & 0xff);

	gpio->output_toggle = bits;
#endif

	return 0;
}

/* API implementation: interrupts handler */
#if IS_INST_IRQ_EN(0) || IS_INST_IRQ_EN(1) || IS_INST_IRQ_EN(2) || \
	IS_INST_IRQ_EN(3) || IS_INST_IRQ_EN(4)
static void gpio_b9x_irq_handler(const struct device *dev)
{
	struct gpio_b9x_data *data				= dev->data;
	const struct gpio_b9x_config *cfg		= dev->config;
#ifdef CONFIG_PM
	const uint8_t wakeup_trigger_pol_reg	= reg_wakeup_trig_pol_base +
		GET_PORT_NUM(GET_GPIO(dev));
#endif
	uint8_t irq = GET_IRQ_NUM(dev);

	gpio_port_value_t current_pins       = GET_GPIO(dev)->input;
	gpio_port_value_t changed_pins0      = (cfg->pin_irq_state->pin_last_value ^ current_pins)
		& (~current_pins);
	gpio_port_value_t changed_pins1      = (cfg->pin_irq_state->pin_last_value ^ current_pins)
		& current_pins;
	gpio_port_value_t fired_irqs_rising  = changed_pins1 & cfg->pin_irq_state->irq_en_rising;
	gpio_port_value_t fired_irqs_falling = changed_pins0 & cfg->pin_irq_state->irq_en_falling;
	gpio_port_value_t fired_irqs_both    = (changed_pins0 | changed_pins1)
		& cfg->pin_irq_state->irq_en_both;
	gpio_port_value_t fired_irqs         = fired_irqs_rising | fired_irqs_falling
		| fired_irqs_both;

	cfg->pin_irq_state->pin_last_value = current_pins;
	GET_GPIO(dev)->polarity ^= (changed_pins0 | changed_pins1);

#if CONFIG_PM
	analog_write_reg8(wakeup_trigger_pol_reg, GET_GPIO(dev)->polarity);
#endif

	gpio_b9x_irq_status_clr(irq);
	gpio_fire_callbacks(&data->callbacks, dev, fired_irqs);
}
#endif

/* API implementation: pin_interrupt_configure */
static int gpio_b9x_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_b9x_config *cfg = dev->config;
	int ret_status = 0;
	bool current_pin_value = ((GET_GPIO(dev)->input) >> pin) & 0x0001;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:                /* GPIO interrupt disable */
		gpio_b9x_irq_en_clr(dev, pin);
		break;

	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_HIGH) {       /* GPIO interrupt Rising edge */
			BM_SET(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_falling, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_both, BIT(pin));
		} else if (trig == GPIO_INT_TRIG_LOW) { /* GPIO interrupt Falling edge */
			BM_SET(cfg->pin_irq_state->irq_en_falling, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_both, BIT(pin));
		} else if (trig == GPIO_INT_TRIG_BOTH) { /* GPIO interrupt Both edge */
			BM_SET(cfg->pin_irq_state->irq_en_both, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_falling, BIT(pin));
		} else {
			ret_status = -ENOTSUP;
		}

		/*
		 * Select the falling edge/low level IRQ as
		 * a wakeup source if the initial pin state is high.
		 * The opposite solution is used when initial state is low.
		 */
		if (current_pin_value) {
			gpio_b9x_irq_set(dev, pin, INTR_FALLING_EDGE);
		} else {
			gpio_b9x_irq_set(dev, pin, INTR_RISING_EDGE);
		}

		if (ret_status == 0) {
			current_pin_value ? BM_SET(cfg->pin_irq_state->pin_last_value, BIT(pin)) :
				BM_CLR(cfg->pin_irq_state->pin_last_value, BIT(pin));
		}
		break;

	case GPIO_INT_MODE_LEVEL:
	default:
		ret_status = -ENOTSUP;
		break;
	}

	return ret_status;
}

/* API implementation: manage_callback */
static int gpio_b9x_manage_callback(const struct device *dev,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_b9x_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION

static int gpio_b9x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct gpio_b9x_config *cfg	= dev->config;
	struct gpio_b9x_data *data			= dev->data;
	uint8_t irq_num						= GET_IRQ_NUM(dev);
	uint8_t irq_priority				= GET_IRQ_PRIORITY(dev);
	struct gpio_b9x_t *gpio				= (struct gpio_b9x_t *)cfg->gpio_base;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		{
			extern volatile bool b9x_deep_sleep_retention;

			if (b9x_deep_sleep_retention) {
				memcpy(gpio, &data->gpio_b9x_retention.gpio_b9x_periph_config,
				sizeof(data->gpio_b9x_retention.gpio_b9x_periph_config));
				if (IS_PORT_C(gpio)) {
					analog_write_reg8(areg_gpio_pc_ie,
					data->gpio_b9x_retention.analog_in_conf);
				} else if (IS_PORT_D(gpio)) {
					analog_write_reg8(areg_gpio_pd_ie,
					data->gpio_b9x_retention.analog_in_conf);
				}
				if (IS_PORT_F(gpio)) {
					analog_write_reg8(0x23,
					data->gpio_b9x_retention.analog_pupd_conf[0]);
					analog_write_reg8(0x24,
					data->gpio_b9x_retention.analog_pupd_conf[1]);
				} else {
					analog_write_reg8(0x0e + (GET_PORT_NUM(gpio) << 1),
					data->gpio_b9x_retention.analog_pupd_conf[0]);
					analog_write_reg8(0x0e + (GET_PORT_NUM(gpio) << 1) + 1,
					data->gpio_b9x_retention.analog_pupd_conf[1]);
				}

				reg_gpio_irq_ctrl = data->gpio_b9x_retention.gpio_b9x_irq_conf;
				reg_irq_risc0_en(GET_PORT_NUM(gpio))
				= data->gpio_b9x_retention.risc0_irq_conf;
				reg_irq_risc1_en(GET_PORT_NUM(gpio))
				= data->gpio_b9x_retention.risc1_irq_conf;

				/* The idea behind this code is to set the pending IRQ based on
				 * pin level. The wakeup by GPIO doesn't set the interrupt pending
				 * bit so we need to trigger the IRQ manually. To achieve this we
				 * temporary switch the IRQ trigger to level mode, getting the
				 * pending bit and restoring the edge mode
				 */
				irq_num -= CONFIG_2ND_LVL_ISR_TBL_OFFSET;

				if (irq_num == IRQ_GPIO) {
					BM_SET(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO);
				} else if (irq_num == IRQ_GPIO2_RISC0) {
					BM_SET(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
				} else if (irq_num == IRQ_GPIO2_RISC1) {
					BM_SET(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
				}

				riscv_plic_irq_enable(irq_num);
				riscv_plic_set_priority(irq_num, irq_priority);

				if (irq_num == IRQ_GPIO) {
					BM_CLR(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO);
				} else if (irq_num == IRQ_GPIO2_RISC0) {
					BM_CLR(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
				} else if (irq_num == IRQ_GPIO2_RISC1) {
					BM_CLR(GPIO_IRQ_REG, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
				}
			}
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		memcpy(&data->gpio_b9x_retention.gpio_b9x_periph_config, gpio,
		sizeof(data->gpio_b9x_retention.gpio_b9x_periph_config));
		data->gpio_b9x_retention.gpio_b9x_irq_conf
		= reg_gpio_irq_ctrl;
		data->gpio_b9x_retention.risc0_irq_conf
		= reg_irq_risc0_en(GET_PORT_NUM(gpio));
		data->gpio_b9x_retention.risc1_irq_conf
		= reg_irq_risc1_en(GET_PORT_NUM(gpio));

		if (IS_PORT_C(gpio)) {
			data->gpio_b9x_retention.analog_in_conf
			= analog_read_reg8(areg_gpio_pc_ie);
		} else if (IS_PORT_D(gpio))	{
			data->gpio_b9x_retention.analog_in_conf
			= analog_read_reg8(areg_gpio_pd_ie);
		}
		if (IS_PORT_F(gpio)) {
			data->gpio_b9x_retention.analog_pupd_conf[0]
			= analog_read_reg8(0x23);
			data->gpio_b9x_retention.analog_pupd_conf[1]
			= analog_read_reg8(0x24);
		} else {
			data->gpio_b9x_retention.analog_pupd_conf[0]
			= analog_read_reg8(0x0e + (GET_PORT_NUM(gpio) << 1));
			data->gpio_b9x_retention.analog_pupd_conf[1]
			= analog_read_reg8(0x0e + (GET_PORT_NUM(gpio) << 1) + 1);
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */

/* GPIO driver APIs structure */
static const struct gpio_driver_api gpio_b9x_driver_api = {
	.pin_configure = gpio_b9x_pin_configure,
	.port_get_raw = gpio_b9x_port_get_raw,
	.port_set_masked_raw = gpio_b9x_port_set_masked_raw,
	.port_set_bits_raw = gpio_b9x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_b9x_port_clear_bits_raw,
	.port_toggle_bits = gpio_b9x_port_toggle_bits,
	.pin_interrupt_configure = gpio_b9x_pin_interrupt_configure,
	.manage_callback = gpio_b9x_manage_callback
};

/* If instance 0 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0
static void gpio_b9x_irq_connect_0(void)
{
	#if IS_INST_IRQ_EN(0)
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(0), 0);
	#endif
}
#endif

/* If instance 1 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
static void gpio_b9x_irq_connect_1(void)
{
	#if IS_INST_IRQ_EN(1)
	IRQ_CONNECT(DT_INST_IRQN(1), DT_INST_IRQ(1, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(1), 0);
	#endif
}
#endif

/* If instance 2 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 2
static void gpio_b9x_irq_connect_2(void)
{
	#if IS_INST_IRQ_EN(2)
	IRQ_CONNECT(DT_INST_IRQN(2), DT_INST_IRQ(2, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(2), 0);
	#endif
}
#endif

/* If instance 3 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 3
static void gpio_b9x_irq_connect_3(void)
{
	#if IS_INST_IRQ_EN(3)
	IRQ_CONNECT(DT_INST_IRQN(3), DT_INST_IRQ(3, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(3), 0);
	#endif
}
#endif

/* If instance 4 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 4
static void gpio_b9x_irq_connect_4(void)
{
	#if IS_INST_IRQ_EN(4)
	IRQ_CONNECT(DT_INST_IRQN(4), DT_INST_IRQ(4, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(4), 0);
	#endif
}
#endif

#if CONFIG_SOC_RISCV_TELINK_B95
/* If instance 5 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 5
static void gpio_b9x_irq_connect_5(void)
{
	#if IS_INST_IRQ_EN(5)
	IRQ_CONNECT(DT_INST_IRQN(5), DT_INST_IRQ(5, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(5), 0);
	#endif
}
#endif

/* If instance 6 is present and has interrupt enabled, connect IRQ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 6
static void gpio_b9x_irq_connect_6(void)
{
	#if IS_INST_IRQ_EN(6)
	IRQ_CONNECT(DT_INST_IRQN(6), DT_INST_IRQ(6, priority),
		    gpio_b9x_irq_handler,
		    DEVICE_DT_INST_GET(6), 0);
	#endif
}
#endif

#endif

#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION
#define PM_DEVICE_INST_DEFINE(n, gpio_b9x_pm_action)  \
PM_DEVICE_DT_INST_DEFINE(n, gpio_b9x_pm_action);
#define PM_DEVICE_INST_GET(n) PM_DEVICE_DT_INST_GET(n)
#else
#define PM_DEVICE_INST_DEFINE(n, gpio_b9x_pm_action)
#define PM_DEVICE_INST_GET(n)  NULL
#endif /* CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */


/* GPIO driver registration */
#define GPIO_B9X_INIT(n)						    \
	PM_DEVICE_INST_DEFINE(n, gpio_b9x_pm_action)	\
	static struct gpio_b9x_pin_irq_config gpio_b9x_pin_irq_state_##n; \
	static const struct gpio_b9x_config gpio_b9x_config_##n = {	    \
		.common = {						    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n) \
		},							    \
		.gpio_base = DT_INST_REG_ADDR(n),			    \
		.irq_num = DT_INST_IRQN(n),				    \
		.irq_priority = DT_INST_IRQ(n, priority),		    \
		.pin_irq_state = &gpio_b9x_pin_irq_state_##n,	\
		.pirq_connect = gpio_b9x_irq_connect_##n		    \
	};								    \
	static struct gpio_b9x_data gpio_b9x_data_##n;			    \
									    \
	DEVICE_DT_INST_DEFINE(n, gpio_b9x_init,				    \
			      PM_DEVICE_INST_GET(n),					    \
			      &gpio_b9x_data_##n,			    \
			      &gpio_b9x_config_##n,			    \
			      PRE_KERNEL_1,				    \
			      CONFIG_GPIO_INIT_PRIORITY,		    \
			      &gpio_b9x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_B9X_INIT)
