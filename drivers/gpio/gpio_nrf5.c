/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
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
#include "nrf5_common.h"
#include "gpio_utils.h"

#if defined(CONFIG_SOC_SERIES_NRF51X)
#define GPIOTE_CHAN_COUNT (4)
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#define GPIOTE_CHAN_COUNT (8)
#else
#error "Platform not defined."
#endif

/* GPIO structure for nRF5X. More detailed description of each register can be found in nrf5X.h */
struct _gpio {
	__I  uint32_t RESERVED0[321];
	__IO uint32_t OUT;
	__IO uint32_t OUTSET;
	__IO uint32_t OUTCLR;

	__I uint32_t  IN;
	__IO uint32_t DIR;
	__IO uint32_t DIRSET;
	__IO uint32_t DIRCLR;
	__IO uint32_t LATCH;
	__IO uint32_t DETECTMODE;
	__I uint32_t  RESERVED1[118];
	__IO uint32_t PIN_CNF[32];
};

/* GPIOTE structure for nRF5X. More detailed description of each register can be found in nrf5X.h */
struct _gpiote {
	__O uint32_t  TASKS_OUT[8];
	__I uint32_t  RESERVED0[4];

	__O uint32_t  TASKS_SET[8];
	__I uint32_t  RESERVED1[4];

	__O uint32_t  TASKS_CLR[8];
	__I uint32_t  RESERVED2[32];
	__IO uint32_t EVENTS_IN[8];
	__I uint32_t  RESERVED3[23];
	__IO uint32_t EVENTS_PORT;
	__I uint32_t  RESERVED4[97];
	__IO uint32_t INTENSET;
	__IO uint32_t INTENCLR;
	__I uint32_t  RESERVED5[129];
	__IO uint32_t CONFIG[8];
};

/** Configuration data */
struct gpio_nrf5_config {
	/* GPIO module base address */
	uint32_t gpio_base_addr;
	/* Port Control module base address */
	uint32_t port_base_addr;
	/* GPIO Task Event base address */
	uint32_t gpiote_base_addr;
};

struct gpio_nrf5_data {
	/* list of registered callbacks */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	uint32_t pin_callback_enables;

	/*@todo: move GPIOTE channel management to a separate module */
	uint32_t gpiote_chan_mask;
};

/* convenience defines for GPIO */
#define DEV_GPIO_CFG(dev) \
	((const struct gpio_nrf5_config * const)(dev)->config->config_info)
#define DEV_GPIO_DATA(dev) \
	((struct gpio_nrf5_data * const)(dev)->driver_data)
#define GPIO_STRUCT(dev) \
	((volatile struct _gpio *)(DEV_GPIO_CFG(dev))->gpio_base_addr)

/* convenience defines for GPIOTE */
#define GPIOTE_STRUCT(dev) \
	((volatile struct _gpiote *)(DEV_GPIO_CFG(dev))->gpiote_base_addr)


#define GPIO_SENSE_DISABLE    (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_SENSE_ENABLE     (GPIO_PIN_CNF_SENSE_Enabled << GPIO_PIN_CNF_SENSE_Pos)
#define GPIO_PULL_DISABLE     (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
#define GPIO_PULL_DOWN        (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos)
#define GPIO_PULL_UP          (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
#define GPIO_INPUT_CONNECT    (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
#define GPIO_INPUT_DISCONNECT (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
#define GPIO_DIR_INPUT        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos)
#define GPIO_DIR_OUTPUT       (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)

#define GPIO_DRIVE_S0S1 (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0S1 (GPIO_PIN_CNF_DRIVE_H0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_S0H1 (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0H1 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_D0S1 (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_D0H1 (GPIO_PIN_CNF_DRIVE_D0H1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_S0D1 (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_DRIVE_H0D1 (GPIO_PIN_CNF_DRIVE_H0D1 << GPIO_PIN_CNF_DRIVE_Pos)

#define GPIOTE_CFG_EVT (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos)
#define GPIOTE_CFG_TASK (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)
#define GPIOTE_CFG_POL_L2H (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos)
#define GPIOTE_CFG_POL_H2L (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos)
#define GPIOTE_CFG_POL_TOGG (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos)
#define GPIOTE_CFG_PIN(pin) ((pin << GPIOTE_CONFIG_PSEL_Pos) & GPIOTE_CONFIG_PSEL_Msk)
#define GPIOTE_CFG_PIN_GET(config) ((config & GPIOTE_CONFIG_PSEL_Msk) >> \
				GPIOTE_CONFIG_PSEL_Pos)

static int gpiote_find_channel(struct device *dev, uint32_t pin)
{
	volatile struct _gpiote *gpiote = GPIOTE_STRUCT(dev);
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
	int i;

	for (i = 0; i < GPIOTE_CHAN_COUNT; i++) {
		if ((data->gpiote_chan_mask & BIT(i)) &&
		    (GPIOTE_CFG_PIN_GET(gpiote->CONFIG[i]) == pin)) {
			return i;
		}
	}

	return -ENODEV;
}

/**
 * @brief Configure pin or port
 */
static int gpio_nrf5_config(struct device *dev,
			    int access_op, uint32_t pin, int flags)
{
	/* Note D0D1 is not supported so we switch to S0S1.  */
	static const uint32_t drive_strength[4][4] = {
		{GPIO_DRIVE_S0S1, GPIO_DRIVE_S0H1, 0, GPIO_DRIVE_S0D1},
		{GPIO_DRIVE_H0S1, GPIO_DRIVE_H0H1, 0, GPIO_DRIVE_H0D1},
		{0, 0, 0, 0},
		{GPIO_DRIVE_D0S1, GPIO_DRIVE_D0H1, 0, GPIO_DRIVE_S0S1}
	};
	volatile struct _gpiote *gpiote = GPIOTE_STRUCT(dev);
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
	volatile struct _gpio *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {

		/* Check pull */
		uint8_t pull = GPIO_PULL_DISABLE;
		int ds_low = (flags & GPIO_DS_LOW_MASK) >> GPIO_DS_LOW_POS;
		int ds_high = (flags & GPIO_DS_HIGH_MASK) >> GPIO_DS_HIGH_POS;

		__ASSERT_NO_MSG(ds_low != 2);
		__ASSERT_NO_MSG(ds_high != 2);

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
			pull = GPIO_PULL_UP;
		} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
			pull = GPIO_PULL_DOWN;
		}

		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
			/* Config as output */
			gpio->PIN_CNF[pin] = (GPIO_SENSE_DISABLE |
					      drive_strength[ds_low][ds_high] |
					      pull |
					      GPIO_INPUT_DISCONNECT |
					      GPIO_DIR_OUTPUT);
		} else {
			/* Config as input */
			gpio->PIN_CNF[pin] = (GPIO_SENSE_DISABLE |
					      drive_strength[ds_low][ds_high] |
					      pull |
					      GPIO_INPUT_CONNECT |
					      GPIO_DIR_INPUT);
		}
	} else {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT) {
		uint32_t config = 0;

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
		if (__builtin_popcount(data->gpiote_chan_mask) ==
				GPIOTE_CHAN_COUNT) {
			return -EIO;
		}

		/* check if already allocated to replace */
		int i = gpiote_find_channel(dev, pin);

		if (i < 0) {
			/* allocate a GPIOTE channel */
			i = __builtin_ffs(~(data->gpiote_chan_mask)) - 1;
		}

		data->gpiote_chan_mask |= BIT(i);

		/* configure GPIOTE channel */
		config |= GPIOTE_CFG_EVT;
		config |= GPIOTE_CFG_PIN(pin);

		gpiote->CONFIG[i] = config;
	}


	return 0;
}

static int gpio_nrf5_read(struct device *dev,
			  int access_op, uint32_t pin, uint32_t *value)
{
	volatile struct _gpio *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = gpio->IN & BIT(pin);
	} else { /* GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
	}
	return 0;
}

static int gpio_nrf5_write(struct device *dev,
			   int access_op, uint32_t pin, uint32_t value)
{
	volatile struct _gpio *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) { /* 1 */
			gpio->OUTSET = BIT(pin);
		} else { /* 0 */
			gpio->OUTCLR = BIT(pin);
		}
	} else { /* GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
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
				    int access_op, uint32_t pin)
{
	volatile struct _gpiote *gpiote = GPIOTE_STRUCT(dev);
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
	int i;

	if (access_op == GPIO_ACCESS_BY_PIN) {

		i = gpiote_find_channel(dev, pin);
		if (i < 0) {
			return i;
		}

		data->pin_callback_enables |= BIT(pin);
		/* clear event before any interrupt triggers */
		gpiote->EVENTS_IN[i] = 0;
		/* enable interrupt for the GPIOTE channel */
		gpiote->INTENSET |= BIT(i);
	} else {
		return -ENOTSUP;
	}

	return 0;
}


static int gpio_nrf5_disable_callback(struct device *dev,
				     int access_op, uint32_t pin)
{
	volatile struct _gpiote *gpiote = GPIOTE_STRUCT(dev);
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
	int i;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		i = gpiote_find_channel(dev, pin);
		if (i < 0) {
			return i;
		}

		data->pin_callback_enables &= ~(BIT(pin));
		/* disable interrupt for the GPIOTE channel */
		gpiote->INTENCLR &= ~(BIT(i));
	} else {
		return -ENOTSUP;
	}

	return 0;
}



/**
 * @brief Handler for port interrupts
 * @param dev Pointer to device structure for driver instance
 *
 * @return N/A
 */
static void gpio_nrf5_port_isr(void *arg)
{
	struct device *dev = arg;
	volatile struct _gpiote *gpiote = GPIOTE_STRUCT(dev);
	struct gpio_nrf5_data *data = DEV_GPIO_DATA(dev);
	uint32_t enabled_int, int_status = 0;
	int i;

	for (i = 0; i < GPIOTE_CHAN_COUNT; i++) {
		if (gpiote->EVENTS_IN[i]) {
			gpiote->EVENTS_IN[i] = 0;
			int_status |= BIT(GPIOTE_CFG_PIN_GET(gpiote->CONFIG[i]));
		}
	}

	enabled_int = int_status & data->pin_callback_enables;

	irq_disable(NRF5_IRQ_GPIOTE_IRQn);

	/* Call the registered callbacks */
	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);

	irq_enable(NRF5_IRQ_GPIOTE_IRQn);
}

static const struct gpio_driver_api gpio_nrf5_drv_api_funcs = {
	.config = gpio_nrf5_config,
	.read = gpio_nrf5_read,
	.write = gpio_nrf5_write,
	.manage_callback = gpio_nrf5_manage_callback,
	.enable_callback = gpio_nrf5_enable_callback,
	.disable_callback = gpio_nrf5_disable_callback,
};

/* Initialization for GPIO Port 0 */
#ifdef CONFIG_GPIO_NRF5_P0

static int gpio_nrf5_P0_init(struct device *dev);

static const struct gpio_nrf5_config gpio_nrf5_P0_cfg = {
	.gpio_base_addr   = NRF_GPIO_BASE,
	.port_base_addr   = NRF_GPIO_BASE,
	.gpiote_base_addr = NRF_GPIOTE_BASE,
};

static struct gpio_nrf5_data gpio_data_P0;

DEVICE_AND_API_INIT(gpio_nrf5_P0, CONFIG_GPIO_NRF5_P0_DEV_NAME, gpio_nrf5_P0_init,
		    &gpio_data_P0, &gpio_nrf5_P0_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_nrf5_drv_api_funcs);

static int gpio_nrf5_P0_init(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_GPIOTE_IRQn, CONFIG_GPIO_NRF5_PORT_P0_PRI,
		    gpio_nrf5_port_isr, DEVICE_GET(gpio_nrf5_P0), 0);

	irq_enable(NRF5_IRQ_GPIOTE_IRQn);

	return 0;
}

#endif /* CONFIG_GPIO_NRF5_P0 */
