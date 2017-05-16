/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the STMPE811 Touchscreen Controller
 *
 * The STMPE811 is a device for controlling resistive touchscreens which
 * has an I2C or SPI interface. It also contains a temperature sensor and a
 * very small number of GPIOs.
 *
 * This driver only implements support for the touchscreen driver over the
 * I2C interface.
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <i2c.h>
#include <kernel.h>
#include <misc/util.h>
#include <touchscreen.h>

/* Registers numbers for STMPE811 */
#define CHIP_ID		0x00
#define ID_VER		0x02
#define SYS_CTRL1	0x03
#define SYS_CTRL2	0x04
#define SPI_CFG		0x08
#define INT_CTRL	0x09
#define INT_EN		0x0A
#define INT_STA		0x0B
#define GPIO_EN		0x0C
#define GPIO_INT_STA	0x0D
#define ADC_INT_EN	0x0E
#define ADC_INT_STA	0x0F
#define GPIO_SET_PIN	0x10
#define GPIO_CLR_PIN	0x11
#define GPIO_MP_STA	0x12
#define GPIO_DIR	0x13
#define GPIO_ED		0x14
#define GPIO_RE		0x15
#define GPIO_FE		0x16
#define GPIO_ALT_FUNCT	0x17
#define ADC_CTRL1	0x20
#define ADC_CTRL2	0x21
#define ADC_CAPT	0x22
#define ADC_DATA_CH0	0x30
#define ADC_DATA_CH1	0x32
#define ADC_DATA_CH2	0x34
#define ADC_DATA_CH3	0x36
#define ADC_DATA_CH4	0x38
#define ADC_DATA_CH5	0x3A
#define ADC_DATA_CH6	0x3C
#define ADC_DATA_CH7	0x3E
#define TSC_CTRL	0x40
#define TSC_CFG		0x41
#define WDW_TR_X	0x42
#define WDW_TR_Y	0x44
#define WDW_BL_X	0x46
#define WDW_BL_Y	0x48
#define FIFO_TH		0x4A
#define FIFO_STA	0x4B
#define FIFO_SIZE	0x4C
#define TSC_DATA_X	0x4D
#define TSC_DATA_Y	0x4F
#define TSC_DATA_Z	0x51
#define TSC_DATA_XYZ	0x52
#define TSC_FRACTION_Z	0x56
#define TSC_DATA	0x57
#define TSC_I_DRIVE	0x58
#define TSC_SHIELD	0x59
#define TEMP_CTRL	0x60
#define TEMP_DATA	0x61
#define TEMP_TH		0x62

/* Values for INT_EN/INT_STA that we're interested in */
#define INT_TOUCH_DET	(1 << 0)
#define INT_FIFO_TH	(1 << 1)

/* Value for TSC status flag in TSC_CTRL */
#define TSC_STA		(1 << 7)

/* Driver config */
struct tsc_stmpe811_config {
	char *i2c_name;
	unsigned int irq;
	void (*irq_config_func)();
	u8_t i2c_addr;
	u8_t z_samples;
	u8_t fraction_z;
};

/* Driver instance data */
struct tsc_stmpe811_context {
	struct device *dev;
	struct k_mutex mutex; /* Protects i2c device and event_{x,y,z,flags} */
	struct device *i2c;
	struct k_work work;
	void (*callback)(struct device *);
	unsigned int irq;
	u16_t event_x;
	u16_t event_y;
	u16_t event_z;
	u8_t event_flags;
	u8_t i2c_addr;
	u8_t sample_size;
};

/* Flags for event_flags */
#define PEN_DOWN	(1 << 0)
#define PEN_DOWN_EVENT	(1 << 1)
#define PEN_UP_EVENT	(1 << 2)

static int write1(struct tsc_stmpe811_context *context, u8_t reg, u8_t value)
{
	return i2c_reg_write_byte(context->i2c, context->i2c_addr, reg, value);
}

static int read1(struct tsc_stmpe811_context *context, u8_t reg, u8_t *value)
{
	return i2c_reg_read_byte(context->i2c, context->i2c_addr, reg, value);
}

static int read(struct tsc_stmpe811_context *context, u8_t reg,
						u8_t *data, u8_t size)
{
	return i2c_burst_read(context->i2c, context->i2c_addr, reg, data, size);
}

static int tsc_stmpe811_update_event_flags(struct tsc_stmpe811_context *context)
{
	u8_t fifo_size;
	u8_t tsc_ctrl;
	int ret;

	/* Check for data in FIFO */
	ret = read1(context, FIFO_SIZE, &fifo_size);
	if (ret) {
		return ret; /* Error */
	}
	if (fifo_size) {
		/* We have samples, so screen must have been touched */
		context->event_flags |= PEN_DOWN_EVENT;
		return 0;
	}

	/*
	 * Clear touch detect interrupt before we process it's state,
	 * this ensures we see another interrupt when it changes.
	 */
	write1(context, INT_STA, INT_TOUCH_DET);

	/* Check if screen not being touched */
	ret = read1(context, TSC_CTRL, &tsc_ctrl);
	if (ret) {
		return ret; /* Error */
	}
	if ((tsc_ctrl & TSC_STA) == 0) {
		/* Screen isn't being touched */
		if (context->event_flags & PEN_DOWN) {
			/*
			 * And pen was down, so we're changing to up.
			 * Note, we know the FIFO is empty here so we must
			 * have emptied all it's data and the last touch
			 * position is still in event_{x,y,z}
			 */
			context->event_flags |= PEN_UP_EVENT;
		}
	}

	/*
	 * Note, if the screen is being touched we don't report that
	 * as an event, instead we use the availability of samples
	 * in the FIFO to indicate that. This prevents brief or light
	 * touches creating spurious pen down/up transitions without
	 * relevant x,y coordinates.
	 */

	return 0;
}

static void tsc_stmpe811_callback_check(struct tsc_stmpe811_context *context)
{
	k_mutex_lock(&context->mutex, K_FOREVER);

	tsc_stmpe811_update_event_flags(context);

	k_mutex_unlock(&context->mutex);

	if (context->event_flags & (PEN_DOWN_EVENT | PEN_UP_EVENT)) {
		/* Have event available so report it */
		context->callback(context->dev);
	} else {
		/* Enable interrupts so we can detect next event */
		irq_enable(context->irq);
	}
}

static void tsc_stmpe811_work(struct k_work *item)
{
	tsc_stmpe811_callback_check(
		CONTAINER_OF(item, struct tsc_stmpe811_context, work));

}

static void tsc_stmpe811_isr(void *arg)
{
	struct device *dev = arg;
	struct tsc_stmpe811_context *context = dev->driver_data;

	/* Prevent more interrupts until data has been processed */
	irq_disable(context->irq);

	k_work_submit(&context->work);
}

#define MAX_SAMPLE_SIZE 4 /* (12-bit X + 12-bit Y + 8-bit Z) */
#define MAX_SAMPLES_IN_ONE_GO 8

static int tsc_stmpe811_read_samples(struct tsc_stmpe811_context *context)
{
	u8_t data[MAX_SAMPLES_IN_ONE_GO * MAX_SAMPLE_SIZE];
	u8_t fifo_size;
	u8_t *last;
	unsigned int num_samples;
	size_t sample_size = context->sample_size;
	int ret;

	/* Get count of samples in FIFO */
	ret = read1(context, FIFO_SIZE, &fifo_size);
	if (ret) {
		return ret;
	}

	if (!fifo_size) {
		return 0;
	}

	/* Read all the samples out of the data FIFO */
	do {
		num_samples = fifo_size;
		if (num_samples > MAX_SAMPLES_IN_ONE_GO) {
			num_samples = MAX_SAMPLES_IN_ONE_GO;
		}

		ret = read(context, TSC_DATA + 0x80, data,
				num_samples * sample_size);
		if (ret < 0) {
			return ret; /* Error */
		}
	} while (fifo_size -= num_samples);

	/* Remember the coordinates of the last sample (we ignore the rest) */
	last = &data[(num_samples - 1) * sample_size];
	context->event_x = (last[0] << 4) | ((last[1] >> 4) & 0xf);
	context->event_y = ((last[1] & 0xf) << 8) | last[2];
	context->event_z = sample_size < 4 ? 0 : last[3];

	/* The fact we have a sample implies the screen was touched... */
	context->event_flags |= PEN_DOWN_EVENT;

	return 0;
}

static int tsc_stmpe811_process_event(struct tsc_stmpe811_context *context)
{
	int ret;

	do {
		/* Process pending pen up event if there is one */
		if (context->event_flags & PEN_UP_EVENT) {
			context->event_flags &= ~PEN_UP_EVENT;
			context->event_flags &= ~PEN_DOWN;
			return 0;
		}

		/*
		 * Clear FIFO interrupt so we can receive more after emptying
		 * the FIFO.
		 */
		ret = write1(context, INT_STA, INT_FIFO_TH);
		if (ret) {
			return ret;
		}

		/* Get samples from FIFO if there are any */
		ret = tsc_stmpe811_read_samples(context);
		if (ret) {
			return ret;
		}


		/* Process pending pen down event if there is one */
		if (context->event_flags & PEN_DOWN_EVENT) {
			context->event_flags &= ~PEN_DOWN_EVENT;
			context->event_flags |= PEN_DOWN;
			return 0;
		}

		/*
		 * Check again for events. We need to do this because we can't
		 * relying on the FIFO_TH interrupt to tell us when data becomes
		 * available because the datasheet says that interrupt doesn't
		 * retrigger until FIFO goes below threshold then back above it.
		 * Therefore we need to test the FIFO is below the threshold
		 * (zero) before we return -EAGAIN to the client and it waits
		 * for the next callback. We can do this check and potentially
		 * avoid a trip through the ISR and workq by using
		 * tsc_stmpe811_update_event_flags() ...
		 */
		ret = tsc_stmpe811_update_event_flags(context);
		if (ret < 0) {
			return ret;
		}
	} while (context->event_flags & (PEN_DOWN_EVENT | PEN_UP_EVENT));

	/* No event */
	return -EAGAIN;
}

static int tsc_stmpe811_get_sample(struct device *dev,
					struct touchscreen_sample *sample)
{
	struct tsc_stmpe811_context *context = dev->driver_data;
	int ret;

	k_mutex_lock(&context->mutex, K_FOREVER);

	ret = tsc_stmpe811_process_event(context);
	if (ret == 0) {
		sample->x = context->event_x;
		sample->y = context->event_y;
		sample->z = context->event_z;
		sample->flags = context->event_flags & PEN_DOWN ?
						       TOUCHSCREEN_TOUCHED : 0;
	}

	k_mutex_unlock(&context->mutex);

	/* Enable interrupts for callbacks when more events are available */
	if (ret == -EAGAIN && context->callback) {
		irq_enable(context->irq);
	}

	return ret;
}

static void tsc_stmpe811_set_callback(struct device *dev,
					void (*callback)(struct device *))
{
	struct tsc_stmpe811_context *context = dev->driver_data;

	context->callback = callback;

	/* Process callback for any currently pending event */
	tsc_stmpe811_callback_check(context);
}

static int tsc_stmpe811_reset(struct device *dev)
{
	struct tsc_stmpe811_context *context = dev->driver_data;
	const struct tsc_stmpe811_config *config = dev->config->config_info;
	int ret;
	u8_t reg;

	/* Reset */
	ret = write1(context, SYS_CTRL1, 0x02);
	if (ret) {
		return ret;
	}

	/*
	 * Setup SYS_CTRL2 to enable TSC and ADC clock (ADC needed for
	 * touchscreen too)
	 */
	reg = 0x0f;		/* All off */
	reg &= ~(1 << 1);	/* TSC_OFF = 0 */
	reg &= ~(1 << 0);	/* ADC_OFF = 0 */
	ret = write1(context, SYS_CTRL2, reg);
	if (ret) {
		return ret;
	}

	/* Setup ADC_CTRL1  */
	reg = 0;
	reg |= (4 << 4);	/* SAMPLE_TIMEn	= 4 (80 clock cycles)	*/
	reg |= (1 << 3);	/* MOD_12B	= 1 (12-bit samples)	*/
	reg |= (0 << 1);	/* REF_SEL	= 0 (internal reference)*/
	ret = write1(context, ADC_CTRL1, reg);
	if (ret) {
		return ret;
	}
	/*
	 * After reset, ADC_CTRL2 already has recommened clock of 3.25 MHz
	 * so no need to set it here.
	 */

	/* FIFO threshold = 1, i.e. interrupt on first set of touch data */
	ret = write1(context, FIFO_TH, 1);
	if (ret) {
		return ret;
	}

	/* FIFO_RESET = 1, i.e. empty the FIFO (will reset have done this?) */
	ret = write1(context, FIFO_STA, 1);
	if (ret) {
		return ret;
	}

	/* Enable interrupts we're interested in  */
	reg = INT_TOUCH_DET | INT_FIFO_TH;
	ret = write1(context, INT_EN, reg);
	if (ret) {
		return ret;
	}

	/* Set current limit to 50 mA (the maximum, to allow for all screens) */
	ret = write1(context, TSC_I_DRIVE, 1);
	if (ret) {
		return ret;
	}

	/* Setup TSC_CFG (Touchscreen controller configuration register) */
	reg = 0;
	reg |= (2 << 6);	/* AVE_CTRL	   = 1 (4 samples)	*/
	reg |= (4 << 3);	/* TOUCH_DET_DELAY = 3 (1 ms)		*/
	reg |= (3 << 0);	/* SETTLING	   = 3 (1 ms)		*/
	ret = write1(context, TSC_CFG, reg);
	if (ret) {
		return ret;
	}

	/* Setup TSC_CTRL (Touchscreen controller control register) */
	reg = 0;
	reg |= (0 << 4);	 /* TRACK  = 0 (No window tracking)	*/
	if (config->z_samples) {
		reg |= (0 << 1); /* OP_MOD = 0 (X, Y and Z)		*/
		context->sample_size = 4;
	} else {
		reg |= (1 << 1); /* OP_MOD = 1 (X, Y only)		*/
		context->sample_size = 3;
	}
	reg |= (1 << 0);	 /* EN     = 1 (Enable TSC)		*/
	ret = write1(context, TSC_CTRL, reg);
	if (ret) {
		return ret;
	}

	/* Setup TSC_FRACTION_Z */
	ret = write1(context, TSC_FRACTION_Z, config->fraction_z);
	if (ret) {
		return ret;
	}

	/* Clear all pending interrupts */
	ret = write1(context, INT_STA, 0xff);
	if (ret) {
		return ret;
	}

	/* Interrupts enabled, active low */
	ret = write1(context, INT_CTRL, 0x01);
	if (ret) {
		return ret;
	}

	return 0;
}

static struct touchscreen_api api = {
	.get_sample = tsc_stmpe811_get_sample,
	.set_callback = tsc_stmpe811_set_callback,
};

int tsc_stmpe811_init(struct device *dev)
{
	struct tsc_stmpe811_context *context = dev->driver_data;
	const struct tsc_stmpe811_config *config = dev->config->config_info;
	u8_t data[2];
	int ret;

	/* Initialise context */
	context->i2c = device_get_binding(config->i2c_name);
	if (!context->i2c) {
		return -ENODEV;
	}
	context->i2c_addr = config->i2c_addr;
	context->irq = config->irq;
	context->dev = dev;
	k_work_init(&context->work, tsc_stmpe811_work);
	k_mutex_init(&context->mutex);

	/* Configure I2C */
	union dev_config i2c_cfg = {
		.bits.speed = I2C_SPEED_FAST,
		.bits.is_master_device = 1,
	};
	ret = i2c_configure(context->i2c, i2c_cfg.raw);
	if (ret) {
		return ret;
	}

	/* Check we can read device ID and it's 0x0811 */
	ret = read(context, CHIP_ID, data, 2);
	if (ret) {
		return ret;
	}
	if (data[0] != 0x08 || data[1] != 0x11) {
		return -ENODEV;
	}

	ret = tsc_stmpe811_reset(dev);
	if (ret) {
		return ret;
	}

	config->irq_config_func();

	/*
	 * Set driver_api at very end of init so if we return early with error
	 * then the device can't be found later by device_get_binding. This is
	 * important because driver framework ignores errors from init
	 * functions.
	 */
	dev->driver_api = &api;

	return 0;
}


static void tsc_stmpe811_irq_config_func(void);

static const struct tsc_stmpe811_config tsc_stmpe811_dev_cfg = {
	.i2c_name = CONFIG_TSC_STMPE811_I2C_DEVICE,
	.i2c_addr = CONFIG_TSC_STMPE811_I2C_ADDR,
	.irq = TSC_STMPE811_IRQ,
	.irq_config_func = tsc_stmpe811_irq_config_func,
#if CONFIG_TSC_STMPE811_Z_SAMPLES
	.z_samples = CONFIG_TSC_STMPE811_Z_SAMPLES,
	.fraction_z = CONFIG_TSC_STMPE811_FRACTION_Z,
#endif
};

struct tsc_stmpe811_context tsc_stmpe811_dev_data;

DEVICE_INIT(stmpe811, CONFIG_TSC_STMPE811_NAME,
	    tsc_stmpe811_init, &tsc_stmpe811_dev_data, &tsc_stmpe811_dev_cfg,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static void tsc_stmpe811_irq_config_func(void)
{
	IRQ_CONNECT(TSC_STMPE811_IRQ,
		CONFIG_TSC_STMPE811_IRQ_PRI,
		tsc_stmpe811_isr,
		DEVICE_GET(stmpe811),
		0);
}
