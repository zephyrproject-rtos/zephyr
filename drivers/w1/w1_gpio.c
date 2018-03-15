/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for software driven 1-Wire using GPIO lines
 *
 * This driver implements an 1-Wire interface by driving two GPIO lines under
 * software control. The GPIO pins used must be preconfigured to to a suitable
 * state, i.e. the DQ pin as open-colector/open-drain with a pull-up resistor
 * (possibly as an external component attached to the pin). When the DQ pin
 * is read it must return the state of the physical hardware line, not just the
 * last state written to it for output.
 */

#include <kernel.h>
#include <device.h>
#include <errno.h>
#include <gpio.h>
#include <misc/printk.h>
#ifdef CONFIG_SOC_FAMILY_NRF
	#include "nrfx.h"
#endif
#include <w1.h>
#include "w1_internal.h"
#include "w1_gpio_delay.h"

/* Driver GPIO config */
struct w1_gpio_config {
	char *gpio_name; /* Name of GPIO device */
	u8_t dq_pin;     /* Pin on gpio used for DQ line */
};

/* Driver instance data */
struct w1_gpio_context {
	struct device *gpio;      /* GPIO used for 1-wire line */
	u8_t dq_pin;              /* Pin on gpio used for DQ line */
	const u32_t *delays;      /* Array of standard 1-wire delays */
	struct k_mutex phy_mutex; /* To provide thread safe line sharing */
};

/**
 * Just a shorten alias to use timing table for delay
 *
 * @param context [description]
 * @param delay   delay in microseconds
 */
static inline void w1_delay_index(struct w1_gpio_context *context,
	u8_t delay_idx)
{
	w1_delay(context->delays[delay_idx]);
}

static inline void w1_gpio_set_dq(struct w1_gpio_context *context,
	int state)
{
	gpio_pin_write(context->gpio, context->dq_pin, state);
}

static inline u32_t w1_gpio_get_dq(struct w1_gpio_context *context)
{
	u32_t state;

	gpio_pin_read(context->gpio, context->dq_pin, &state);

	return state;
}

static inline bool w1_gpio_read_bit(struct device *dev)
{
	struct w1_gpio_context *context = dev->driver_data;

	bool bit = 0;

	w1_gpio_set_dq(context, 0);           /* drives DQ low */
	w1_delay_index(context, T_A);
	w1_gpio_set_dq(context, 1);           /* releases the line */
	w1_delay_index(context, T_E);
	bit = w1_gpio_get_dq(context) & 0x01; /* the bit value from the slave */
	w1_delay_index(context, T_F);

	return bit;
}

static inline u8_t w1_gpio_read_byte(struct device *dev)
{
	u8_t byte = 0;

	/* loop to read each bit in the byte, LS-bit first */
	for (int bit = 0; bit < 8; bit++) {
		byte >>= 1;
		if (w1_gpio_read_bit(dev)) {
			byte |= 0x80;
		}
	}

	return byte;
}

static inline void w1_gpio_write_bit(struct device *dev, bool bit)
{
	struct w1_gpio_context *context = dev->driver_data;

	w1_gpio_set_dq(context, 0);	/* drives DQ low */
	w1_delay_index(context, (bit ? T_A : T_C));
	w1_gpio_set_dq(context, 1);	/* releases the line */
	w1_delay_index(context, (bit ? T_B : T_D));
}

static inline void w1_gpio_write_byte(struct device *dev,
	const u8_t byte)
{
	u8_t curr_byte = byte;
	/* loop to write each bit in the byte, LS-bit first */
	for (u8_t bit = 0; bit < 8; bit++) {
		w1_gpio_write_bit(dev, curr_byte & 0x01);
		curr_byte >>= 1;
	}
}

static inline bool w1_gpio_reset_bus(struct device *dev)
{
	struct w1_gpio_context *context = dev->driver_data;

	bool res = 0;

	w1_delay_index(context, T_G);
	w1_gpio_set_dq(context, 0);	/* drives DQ low */
	w1_delay_index(context, T_H);
	w1_gpio_set_dq(context, 1);	/* releases the line */
	w1_delay_index(context, T_I);
	res = w1_gpio_get_dq(context) ^ 0x01; /* get slave presence pulse */
	w1_delay_index(context, T_J);	/* complete the reset sequence */

	return res;
}

static inline void w1_gpio_write_block(struct device *dev,
	const void *block, u32_t length)
{
	/* loop for each byte of block */
	for (u8_t i = 0; i < length; i++) {
		w1_gpio_write_byte(dev, *((u8_t *)block + i));
	}
}

static inline void w1_gpio_read_block(struct device *dev, u8_t *block,
	u32_t length)
{
	/* loop for each byte of block */
	for (int i = 0; i < length; i++) {
		*((u8_t *)block + i) = w1_gpio_read_byte(dev);
	}
}

static inline int w1_gpio_wait_for(struct device *dev,
	u8_t value, u32_t timeout)
{
	int result = -1;

	for (int i = 0; i < timeout; i++) {
		if (w1_gpio_read_bit(dev) == value) {
			result = 0;
			break;
		}

		w1_delay(1);
	}

	return result;
}

static inline void w1_gpio_lock_bus(struct device *dev)
{
	struct w1_gpio_context *context = dev->driver_data;

	k_mutex_lock(&context->phy_mutex, K_FOREVER);
}

static inline void w1_gpio_unlock_bus(struct device *dev)
{
	struct w1_gpio_context *context = dev->driver_data;

	k_mutex_unlock(&context->phy_mutex);
}

static const struct w1_driver_api api = {
	.reset_bus	= w1_gpio_reset_bus,
	.read_bit	= w1_gpio_read_bit,
	.write_bit	= w1_gpio_write_bit,
	.read_byte	= w1_gpio_read_byte,
	.write_byte	= w1_gpio_write_byte,
	.read_block	= w1_gpio_read_block,
	.write_block	= w1_gpio_write_block,
	.lock_bus	= w1_gpio_lock_bus,
	.unlock_bus	= w1_gpio_unlock_bus,
	.wait_for	= w1_gpio_wait_for,
};

static inline int w1_gpio_init(struct device *dev)
{
	struct w1_gpio_context *context = dev->driver_data;
	const struct w1_gpio_config *config = dev->config->config_info;

	context->gpio = device_get_binding(config->gpio_name);
	if (!context->gpio) {
		printk("Unable to get 1-Wire GPIO device %s\n",
			config->gpio_name);
		return -ENODEV;
	}

	context->dq_pin = config->dq_pin;
	int ret = gpio_pin_configure(context->gpio, context->dq_pin,
		(GPIO_PUD_PULL_UP | GPIO_DIR_OUT));

	if (ret) {
		printk("Error configuring GPIO %s pin %d!\n",
			config->gpio_name,
			context->dq_pin);
	}


	context->delays = delays_standard;

	k_mutex_init(&context->phy_mutex);

	/*
	 * Set driver_api at very end of init so if we return early with error
	 * then the device can't be found later by device_get_binding. This is
	 * important because driver framework ignores errors from init
	 * functions.
	 */
	dev->driver_api = &api;

	return 0;
}

#define	DEFINE_W1_GPIO(_num)						\
									\
static struct w1_gpio_context w1_gpio_dev_data_##_num;			\
									\
static const struct w1_gpio_config w1_gpio_dev_cfg_##_num = {		\
	.gpio_name = CONFIG_W1_GPIO_##_num##_GPIO,			\
	.dq_pin	= CONFIG_W1_GPIO_##_num##_DQ_PIN,			\
};									\
									\
DEVICE_INIT(w1_gpio_##_num, CONFIG_W1_GPIO_##_num##_NAME,		\
			w1_gpio_init,					\
			&w1_gpio_dev_data_##_num,			\
			&w1_gpio_dev_cfg_##_num,			\
			PRE_KERNEL_2, CONFIG_W1_INIT_PRIORITY)

#ifdef CONFIG_W1_GPIO_0
DEFINE_W1_GPIO(0);
#endif
