/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Driver for the Nordic Semiconductor nRF5X GPIO module.
 */

#include <errno.h>

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>
#include <soc.h>
#include <sys_io.h>

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
	/* pin callback routine enable flags, by pin number */
	uint32_t pin_callback_enables;
	/* port callback routine enable flag */
	uint8_t port_callback_enable;
};

/* convenience defines for GPIO */
#define DEV_GPIO_CFG(dev) \
	((const struct gpio_nrf5_config * const)(dev)->config->config_info)
#define DEV_GPIO_DATA(dev) \
	((struct gpio_nrf5_dev_data_t * const)(dev)->driver_data)
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

/**
 * @brief Configure pin or port
 */
static int gpio_nrf5_config(struct device *dev,
			    int access_op, uint32_t pin, int flags)
{
	volatile struct _gpio *gpio = GPIO_STRUCT(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {

		/* Check pull */
		uint8_t pull = GPIO_PULL_DISABLE;

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
			pull = GPIO_PULL_UP;
		} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
			pull = GPIO_PULL_DOWN;
		}

		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
			/* Config as output */
			gpio->PIN_CNF[pin] = GPIO_SENSE_DISABLE |
					     GPIO_DRIVE_S0S1    |
					     pull               |
					     GPIO_INPUT_DISCONNECT |
					     GPIO_DIR_OUTPUT;
		} else {
			/* Config as input */
			gpio->PIN_CNF[pin] = GPIO_SENSE_DISABLE |
					     GPIO_DRIVE_S0S1    |
					     pull               |
					     GPIO_INPUT_CONNECT |
					     GPIO_DIR_INPUT;
		}
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

/**
 * @brief Handler for port interrupts
 * @param dev Pointer to device structure for driver instance
 *
 * @return N/A
 */
static void gpio_nrf5_port_isr(void *dev)
{
}

static const struct gpio_driver_api gpio_nrf5_drv_api_funcs = {
	.config = gpio_nrf5_config,
	.read = gpio_nrf5_read,
	.write = gpio_nrf5_write,
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
	IRQ_CONNECT(GPIOTE_IRQn, CONFIG_GPIO_NRF5_PORT_P0_PRI,
		    gpio_nrf5_port_isr, DEVICE_GET(gpio_nrf5_P0), 0);

	irq_enable(GPIOTE_IRQn);

	return 0;
}

#endif /* CONFIG_GPIO_NRF5_P0 */
