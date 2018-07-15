/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 ARM Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the Nordic Semiconductor nRF5X GPIO module.
 */
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>
#include <soc.h>
#include <sys_io.h>
#include <nrf_gpiote.h>
#include "nrf_common.h"
#include "gpio_utils.h"

#define GPIO_PIN_CNF_SENSE_Invalid	0x01

/*@todo: move GPIOTE channel management to a separate module */
/* Reserve channels below the base index */
#define GPIOTE_CH_BASE CONFIG_GPIO_NRF5_GPIOTE_CHAN_BASE
static u32_t gpiote_chan_mask = BIT_MASK(GPIOTE_CH_BASE);

/** Configuration data */
struct gpio_nrf5_config {
	/* GPIO module base address */
	u32_t gpio_base_addr;
	/* GPIO port */
	u8_t gpio_port;
};

struct gpio_nrf5_data {
	/* list of registered callbacks */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

/* convenience defines for GPIO */
#define DEV_GPIO_CFG(dev) \
	((const struct gpio_nrf5_config * const)(dev)->config->config_info)
#define DEV_GPIO_DATA(dev) \
	((struct gpio_nrf5_data * const)(dev)->driver_data)
#define GPIO_STRUCT(dev) \
	((volatile NRF_GPIO_Type *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define GPIO_PORT(dev) \
	(DEV_GPIO_CFG(dev)->gpio_port)

#define GPIO_SENSE_DISABLE    (GPIO_PIN_CNF_SENSE_Disabled << \
			       GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_SENSE_LOW        (GPIO_PIN_CNF_SENSE_Low << \
			       GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_SENSE_HIGH       (GPIO_PIN_CNF_SENSE_High << \
			       GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_SENSE_INVALID    (GPIO_PIN_CNF_SENSE_Invalid << \
			       GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_PULL_DISABLE     (GPIO_PIN_CNF_PULL_Disabled << \
			       GPIO_PIN_CNF_PULL_Pos)
#define GPIO_PULL_DOWN        (GPIO_PIN_CNF_PULL_Pulldown << \
			       GPIO_PIN_CNF_PULL_Pos)
#define GPIO_PULL_UP          (GPIO_PIN_CNF_PULL_Pullup << \
			       GPIO_PIN_CNF_PULL_Pos)
#define GPIO_INPUT_CONNECT    (GPIO_PIN_CNF_INPUT_Connect << \
			       GPIO_PIN_CNF_INPUT_Pos)
#define GPIO_INPUT_DISCONNECT (GPIO_PIN_CNF_INPUT_Disconnect << \
			       GPIO_PIN_CNF_INPUT_Pos)
#define GPIO_DIR_INPUT        (GPIO_PIN_CNF_DIR_Input << \
			       GPIO_PIN_CNF_DIR_Pos)
#define GPIO_DIR_OUTPUT       (GPIO_PIN_CNF_DIR_Output << \
			       GPIO_PIN_CNF_DIR_Pos)

#define GPIO_DRIVE_S0S1 (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0S1 (GPIO_PIN_CNF_DRIVE_H0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_S0H1 (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0H1 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_D0S1 (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_D0H1 (GPIO_PIN_CNF_DRIVE_D0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_S0D1 (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0D1 (GPIO_PIN_CNF_DRIVE_H0D1 << GPIO_PIN_CNF_DRIVE_Pos)

#define GPIOTE_CFG_EVT  (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos)
#define GPIOTE_CFG_TASK (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)

#define GPIOTE_CFG_POL_L2H  (GPIOTE_CONFIG_POLARITY_LoToHi << \
			     GPIOTE_CONFIG_POLARITY_Pos)
#define GPIOTE_CFG_POL_H2L  (GPIOTE_CONFIG_POLARITY_HiToLo << \
			     GPIOTE_CONFIG_POLARITY_Pos)
#define GPIOTE_CFG_POL_TOGG (GPIOTE_CONFIG_POLARITY_Toggle << \
			     GPIOTE_CONFIG_POLARITY_Pos)

#if defined(CONFIG_GPIO_NRF5_P1)
#define GPIOTE_CFG_PORT(port)       ((port << GPIOTE_CONFIG_PORT_Pos) & \
				     GPIOTE_CONFIG_PORT_Msk)
#define GPIOTE_CFG_PORT_GET(config) ((config & GPIOTE_CONFIG_PORT_Msk) >> \
				     GPIOTE_CONFIG_PORT_Pos)
#else /* !CONFIG_GPIO_NRF5_P1 */
#define GPIOTE_CFG_PORT(port)       0
#define GPIOTE_CFG_PORT_GET(config) 0
#endif /* !CONFIG_GPIO_NRF5_P1 */

#define GPIOTE_CFG_PIN(pin)        ((pin << GPIOTE_CONFIG_PSEL_Pos) & \
				    GPIOTE_CONFIG_PSEL_Msk)
#define GPIOTE_CFG_PIN_GET(config) ((config & GPIOTE_CONFIG_PSEL_Msk) >> \
				    GPIOTE_CONFIG_PSEL_Pos)

static int gpiote_find_channel(struct device *dev, u32_t pin, u32_t port)
{
	volatile NRF_GPIOTE_Type *gpiote = (void *)NRF_GPIOTE_BASE;
	int i;

	for (i = GPIOTE_CH_BASE; i < GPIOTE_CH_NUM; i++) {
		if ((gpiote_chan_mask & BIT(i)) &&
		    (GPIOTE_CFG_PIN_GET(gpiote->CONFIG[i]) == pin) &&
		    (GPIOTE_CFG_PORT_GET(gpiote->CONFIG[i]) == port)) {
			return i;
		}
	}

	return -ENODEV;
}

/**
 * @brief Configure pin or port
 */
static int gpio_nrf5_config(struct device *dev,
			    int access_op, u32_t pin, int flags)
{
	/* Note D0D1 is not supported so we switch to S0S1.  */
	static const u32_t drive_strength[4][4] = {
		{GPIO_DRIVE_S0S1, GPIO_DRIVE_S0H1, 0, GPIO_DRIVE_S0D1},
		{GPIO_DRIVE_H0S1, GPIO_DRIVE_H0H1, 0, GPIO_DRIVE_H0D1},
		{0, 0, 0, 0},
		{GPIO_DRIVE_D0S1, GPIO_DRIVE_D0H1, 0, GPIO_DRIVE_S0S1}
	};
	volatile NRF_GPIOTE_Type *gpiote = (void *)NRF_GPIOTE_BASE;
	volatile NRF_GPIO_Type *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {

		/* Check pull */
		u8_t pull = GPIO_PULL_DISABLE;
		int ds_low = (flags & GPIO_DS_LOW_MASK) >> GPIO_DS_LOW_POS;
		int ds_high = (flags & GPIO_DS_HIGH_MASK) >> GPIO_DS_HIGH_POS;
		unsigned int sense = (flags & GPIO_PIN_CNF_SENSE_Msk);

		__ASSERT_NO_MSG(ds_low != 2);
		__ASSERT_NO_MSG(ds_high != 2);

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
			pull = GPIO_PULL_UP;
		} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
			pull = GPIO_PULL_DOWN;
		}

		if (sense == GPIO_SENSE_INVALID) {
			__ASSERT_NO_MSG(sense == GPIO_SENSE_INVALID);
			sense = GPIO_SENSE_DISABLE;
		}

		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
			/* Set initial output value */
			if (pull == GPIO_PULL_UP) {
				gpio->OUTSET = BIT(pin);
			} else if (pull == GPIO_PULL_DOWN) {
				gpio->OUTCLR = BIT(pin);
			}
			/* Config as output */
			gpio->PIN_CNF[pin] = (GPIO_SENSE_DISABLE |
					      drive_strength[ds_low][ds_high] |
					      pull |
					      GPIO_INPUT_DISCONNECT |
					      GPIO_DIR_OUTPUT);
		} else {
			/* Config as input */
			gpio->PIN_CNF[pin] = (sense |
					      drive_strength[ds_low][ds_high] |
					      pull |
					      GPIO_INPUT_CONNECT |
					      GPIO_DIR_INPUT);
		}
	} else {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT) {
		u32_t config = 0;
		u32_t port = GPIO_PORT(dev);

		/* check if already allocated to replace */
		int i = gpiote_find_channel(dev, pin, port);

		if (i < 0) {
			if (popcount(gpiote_chan_mask) == GPIOTE_CH_NUM) {
				return -EIO;
			}

			/* allocate a GPIOTE channel */
			i = find_lsb_set(~gpiote_chan_mask) - 1;
			gpiote_chan_mask |= BIT(i);
		}

		/* configure GPIOTE channel */
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_DOUBLE_EDGE) {
				config |= GPIOTE_CFG_POL_TOGG;
			} else if (flags & GPIO_INT_ACTIVE_HIGH) {
				config |= GPIOTE_CFG_POL_L2H;
			} else {
				config |= GPIOTE_CFG_POL_H2L;
			}
		} else { /* GPIO_INT_LEVEL */
			/*@todo: use SENSE for this? */
			return -ENOTSUP;
		}
		config |= GPIOTE_CFG_EVT;
		config |= GPIOTE_CFG_PIN(pin);
		config |= GPIOTE_CFG_PORT(port);

		gpiote->CONFIG[i] = config;
	}


	return 0;
}

static int gpio_nrf5_read(struct device *dev,
			  int access_op, u32_t pin, u32_t *value)
{
	volatile NRF_GPIO_Type *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (gpio->IN >> pin) & 0x1;
	} else {
		*value = gpio->IN;
	}
	return 0;
}

static int gpio_nrf5_write(struct device *dev,
			   int access_op, u32_t pin, u32_t value)
{
	volatile NRF_GPIO_Type *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) { /* 1 */
			gpio->OUTSET = BIT(pin);
		} else { /* 0 */
			gpio->OUTCLR = BIT(pin);
		}
	} else {
		gpio->OUT = value;
	}
	return 0;
}

static int gpio_nrf5_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_nrf5_enable_callback(struct device *dev,
				    int access_op, u32_t pin)
{
	if (access_op == GPIO_ACCESS_BY_PIN) {
		volatile NRF_GPIOTE_Type *gpiote = (void *)NRF_GPIOTE_BASE;
		struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
		int port = GPIO_PORT(dev);
		int i;

		i = gpiote_find_channel(dev, pin, port);
		if (i < 0) {
			return i;
		}

		data->pin_callback_enables |= BIT(pin);
		/* clear event before any interrupt triggers */
		gpiote->EVENTS_IN[i] = 0;
		/* enable interrupt for the GPIOTE channel */
		gpiote->INTENSET = BIT(i);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_nrf5_disable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	if (access_op == GPIO_ACCESS_BY_PIN) {
		volatile NRF_GPIOTE_Type *gpiote = (void *)NRF_GPIOTE_BASE;
		struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
		int port = GPIO_PORT(dev);
		int i;

		i = gpiote_find_channel(dev, pin, port);
		if (i < 0) {
			return i;
		}

		data->pin_callback_enables &= ~(BIT(pin));
		/* disable interrupt for the GPIOTE channel */
		gpiote->INTENCLR = BIT(i);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* NOTE: forward declarations to use in ISR below */
#if defined(CONFIG_GPIO_NRF5_P0)
DEVICE_DECLARE(gpio_nrf5_P0);
#endif /* CONFIG_GPIO_NRF5_P0 */
#if defined(CONFIG_GPIO_NRF5_P1)
DEVICE_DECLARE(gpio_nrf5_P1);
#endif /* CONFIG_GPIO_NRF5_P1 */

/**
 * @brief Handler for port interrupts
 * @param dev Pointer to device structure for driver instance
 *
 * @return N/A
 */
static void gpio_nrf5_port_isr(void *arg)
{
	volatile NRF_GPIOTE_Type *gpiote = (void *)NRF_GPIOTE_BASE;
	struct gpio_nrf5_data *data;
#if defined(CONFIG_GPIO_NRF5_P0)
	u32_t int_status_p0 = 0;
#endif /* CONFIG_GPIO_NRF5_P0 */
#if defined(CONFIG_GPIO_NRF5_P1)
	u32_t int_status_p1 = 0;
#endif /* CONFIG_GPIO_NRF5_P1 */
	struct device *dev;
	u32_t enabled_int;
	int i;

	for (i = GPIOTE_CH_BASE; i < GPIOTE_CH_NUM; i++) {
		if (gpiote->EVENTS_IN[i]) {
			int port = GPIOTE_CFG_PORT_GET(gpiote->CONFIG[i]);
			int pin = GPIOTE_CFG_PIN_GET(gpiote->CONFIG[i]);

			gpiote->EVENTS_IN[i] = 0;

			switch (port) {
#if defined(CONFIG_GPIO_NRF5_P0)
			case 0:
				int_status_p0 |= BIT(pin);
				break;
#endif /* CONFIG_GPIO_NRF5_P0 */

#if defined(CONFIG_GPIO_NRF5_P1)
			case 1:
				int_status_p1 |= BIT(pin);
				break;
#endif /* CONFIG_GPIO_NRF5_P1 */

			default:
				/* NOTE: unknown port, ignore it */
				break;
			}
		}
	}

#if defined(CONFIG_GPIO_NRF5_P0)
	dev = DEVICE_GET(gpio_nrf5_P0);
	data = DEV_GPIO_DATA(dev);

	enabled_int = int_status_p0 & data->pin_callback_enables;

	/* Call the registered callbacks */
	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);
#endif /* CONFIG_GPIO_NRF5_P0 */

#if defined(CONFIG_GPIO_NRF5_P1)
	dev = DEVICE_GET(gpio_nrf5_P1);
	data = DEV_GPIO_DATA(dev);

	enabled_int = int_status_p1 & data->pin_callback_enables;

	/* Call the registered callbacks */
	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);
#endif /* CONFIG_GPIO_NRF5_P1 */
}

static const struct gpio_driver_api gpio_nrf5_drv_api_funcs = {
	.config = gpio_nrf5_config,
	.read = gpio_nrf5_read,
	.write = gpio_nrf5_write,
	.manage_callback = gpio_nrf5_manage_callback,
	.enable_callback = gpio_nrf5_enable_callback,
	.disable_callback = gpio_nrf5_disable_callback,
};

static int gpio_nrf5_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIOTE_NRF5_IRQ, CONFIG_GPIOTE_NRF5_IRQ_PRI,
		    gpio_nrf5_port_isr, NULL, 0);

	irq_enable(CONFIG_GPIOTE_NRF5_IRQ);

	return 0;
}

/* Enable GPIOTE Interrupt */
void nrf_gpiote_interrupt_enable(uint32_t mask)
{
	nrf_gpiote_int_enable(mask);
}

/* Disable GPIOTE Interrupt */
void  nrf_gpiote_interrupt_disable(uint32_t mask)
{
	nrf_gpiote_int_disable(mask);
}

/* Clear GPIOTE Port Event */
void nrf_gpiote_clear_port_event(void)
{
	NRF_GPIOTE->EVENTS_PORT = 0;
}

/* Initialization for GPIO Port 0 */
#ifdef CONFIG_GPIO_NRF5_P0
static int gpio_nrf5_P0_init(struct device *dev)
{
	gpio_nrf5_init(dev);

	return 0;
}

static const struct gpio_nrf5_config gpio_nrf5_P0_cfg = {
	.gpio_base_addr = NRF_GPIO_BASE,
	.gpio_port      = 0,
};

static struct gpio_nrf5_data gpio_data_P0;

DEVICE_AND_API_INIT(gpio_nrf5_P0, CONFIG_GPIO_NRF5_P0_DEV_NAME, gpio_nrf5_P0_init,
		    &gpio_data_P0, &gpio_nrf5_P0_cfg,
		    POST_KERNEL, CONFIG_GPIO_NRF5_INIT_PRIORITY,
		    &gpio_nrf5_drv_api_funcs);

#endif /* CONFIG_GPIO_NRF5_P0 */

/* Initialization for GPIO Port 1 */
#if defined(CONFIG_GPIO_NRF5_P1)
static int gpio_nrf5_P1_init(struct device *dev)
{
#if !defined(CONFIG_GPIO_NRF5_P0)
	gpio_nrf5_init(dev);
#endif /* CONFIG_GPIO_NRF5_P0 */

	return 0;
}

static const struct gpio_nrf5_config gpio_nrf5_P1_cfg = {
	.gpio_base_addr = NRF_P1_BASE,
	.gpio_port      = 1,
};

static struct gpio_nrf5_data gpio_data_P1;


DEVICE_AND_API_INIT(gpio_nrf5_P1, CONFIG_GPIO_NRF5_P1_DEV_NAME,
		    gpio_nrf5_P1_init, &gpio_data_P1, &gpio_nrf5_P1_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_nrf5_drv_api_funcs);

#endif /* CONFIG_GPIO_NRF5_P1 */
