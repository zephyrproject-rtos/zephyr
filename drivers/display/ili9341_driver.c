/**
 * Copyright (c) 2023 Mr Beam Lasers GmbH.
 * Copyright (c) 2023 Amrith Venkat Kesavamoorthi <amrith@mr-beam.org>
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ilitek_ili9341_driver

#include "ili9341_driver.h"
// #include "display_ili9xxx.h"
// #include <zephyr/dt-bindings/display/ili9xxx.h>

#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9341, CONFIG_DISPLAY_LOG_LEVEL);

/* Command/data GPIO level for commands. */
#define ILI9341_GPIO_LEVEL_CMD 0U

/* Command/data GPIO level for data. */
#define ILI9341_GPIO_LEVEL_DATA 1U

/* Maximum number of default init registers  */
#define ILI9341_NUM_DEFAULT_INIT_REGS 19U

/* Display data struct */
struct ili9341_data {
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

/* Configuration data struct.*/
struct ili9341_config {
	const struct ili9xxx_quirks *quirks;
	struct spi_dt_spec spi;
	struct gpio_dt_spec cmd_data;
	struct gpio_dt_spec reset;
	uint8_t pixel_format;
    uint16_t rotation;
	uint16_t x_resolution;
	uint16_t y_resolution;
	bool inversion;
	const void *regs;
    int (*regs_init_fn)(const struct device *dev);
};

/* Initialization command data struct  */
struct ili9341_default_init_regs {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[ILI9341_NUM_DEFAULT_INIT_REGS];
};

struct spi_shakti_cfg {
    struct gpio_dt_spec ncs;
	uint32_t base;
	uint32_t f_sys;
    const struct pinctrl_dev_config *pcfg;
    struct k_mutex mutex;
};

static int ili9341_transmit(const struct device *dev, uint8_t cmd, const void *tx_data,
			    size_t tx_len)
{
	const struct ili9341_config *config = dev->config;
	int ret;
	struct spi_buf tx_buf = {.buf = &cmd, .len = 1U};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1U};
	gpio_pin_set_dt(&((struct spi_shakti_cfg*)(((config->spi).bus)->config))->ncs, 1);
	ret = gpio_pin_set_dt(&config->cmd_data, ILI9341_GPIO_LEVEL_CMD);
	if (ret < 0) {
		return ret;
	}
	ret = spi_write_dt(&config->spi, &tx_bufs);
	if (ret < 0) {
		return ret;
	}

	/* send data (if any) */
	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_len;

		ret = gpio_pin_set_dt(&config->cmd_data, ILI9341_GPIO_LEVEL_DATA);
		if (ret < 0) {
			return ret;
		}
		ret = spi_write_dt(&config->spi, &tx_bufs);
		if (ret < 0) {
			return ret;
		}
	}
	gpio_pin_set_dt(&((struct spi_shakti_cfg*)(((config->spi).bus)->config))->ncs, 0);
	return 0;
}

int ili9341_regs_init(const struct device *dev)
{
	const struct ili9341_config *config = dev->config;
	const struct ili9341_regs *regs = config->regs;

	int r;

	LOG_HEXDUMP_DBG(regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN, "PWSEQCTRL");
	r = ili9341_transmit(dev, ILI9341_PWSEQCTRL, regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrla, ILI9341_TIMCTRLA_LEN, "TIMCTRLA");
	r = ili9341_transmit(dev, ILI9341_TIMCTRLA, regs->timctrla, ILI9341_TIMCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrlb, ILI9341_TIMCTRLB_LEN, "TIMCTRLB");
	r = ili9341_transmit(dev, ILI9341_TIMCTRLB, regs->timctrlb, ILI9341_TIMCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pumpratioctrl, ILI9341_PUMPRATIOCTRL_LEN, "PUMPRATIOCTRL");
	r = ili9341_transmit(dev, ILI9341_PUMPRATIOCTRL, regs->pumpratioctrl,
			     ILI9341_PUMPRATIOCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrla, ILI9341_PWCTRLA_LEN, "PWCTRLA");
	r = ili9341_transmit(dev, ILI9341_PWCTRLA, regs->pwctrla, ILI9341_PWCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrlb, ILI9341_PWCTRLB_LEN, "PWCTRLB");
	r = ili9341_transmit(dev, ILI9341_PWCTRLB, regs->pwctrlb, ILI9341_PWCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamset, ILI9341_GAMSET_LEN, "GAMSET");
	r = ili9341_transmit(dev, ILI9341_GAMSET, regs->gamset, ILI9341_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9341_FRMCTR1_LEN, "FRMCTR1");
	r = ili9341_transmit(dev, ILI9341_FRMCTR1, regs->frmctr1, ILI9341_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9341_DISCTRL_LEN, "DISCTRL");
	r = ili9341_transmit(dev, ILI9341_DISCTRL, regs->disctrl, ILI9341_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9341_PWCTRL1_LEN, "PWCTRL1");
	r = ili9341_transmit(dev, ILI9341_PWCTRL1, regs->pwctrl1, ILI9341_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9341_PWCTRL2_LEN, "PWCTRL2");
	r = ili9341_transmit(dev, ILI9341_PWCTRL2, regs->pwctrl2, ILI9341_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9341_VMCTRL1_LEN, "VMCTRL1");
	r = ili9341_transmit(dev, ILI9341_VMCTRL1, regs->vmctrl1, ILI9341_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9341_VMCTRL2_LEN, "VMCTRL2");
	r = ili9341_transmit(dev, ILI9341_VMCTRL2, regs->vmctrl2, ILI9341_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9341_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9341_transmit(dev, ILI9341_PGAMCTRL, regs->pgamctrl, ILI9341_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9341_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9341_transmit(dev, ILI9341_NGAMCTRL, regs->ngamctrl, ILI9341_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->enable3g, ILI9341_ENABLE3G_LEN, "ENABLE3G");
	r = ili9341_transmit(dev, ILI9341_ENABLE3G, regs->enable3g, ILI9341_ENABLE3G_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifmode, ILI9341_IFMODE_LEN, "IFMODE");
	r = ili9341_transmit(dev, ILI9341_IFMODE, regs->ifmode, ILI9341_IFMODE_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifctl, ILI9341_IFCTL_LEN, "IFCTL");
	r = ili9341_transmit(dev, ILI9341_IFCTL, regs->ifctl, ILI9341_IFCTL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->etmod, ILI9341_ETMOD_LEN, "ETMOD");
	r = ili9341_transmit(dev, ILI9341_ETMOD, regs->etmod, ILI9341_ETMOD_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9341_exit_sleep(const struct device *dev)
{
	int ret;

	ret = ili9341_transmit(dev, ILI9XXX_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}
    k_sleep(K_MSEC(ILI9XXX_SLEEP_OUT_TIME));


	/*
	 * Exit sleepmode and enable display. 30ms on top of the sleepout time to account for
	 * any manufacturing defects.
	 * This is to allow time for the supply voltages and clock circuits stabilize
	 */
	// k_msleep(GC9X01X_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ili9341_enter_sleep(const struct device *dev)
{
	int ret;

	ret = ili9341_transmit(dev, GC9X01X_CMD_SLPIN, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Exit sleepmode and enable display. 30ms on top of the sleepout time to account for
	 * any manufacturing defects.
	 */
	k_msleep(GC9X01X_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}
#endif

static int ili9341_hw_reset(const struct device *dev)
{
	const struct ili9341_config *config = dev->config;

	if (config->reset.port == NULL) {
		return -ENODEV;
	}

	gpio_pin_set_dt(&config->reset, 1U);
	// k_msleep(100);
	gpio_pin_set_dt(&config->reset, 0U);
	// k_msleep(10);

	return 0;
}

static int ili9341_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return ili9341_transmit(dev, ILI9XXX_DISPON, NULL, 0);
}

static int ili9341_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return ili9341_transmit(dev, ILI9XXX_DISPOFF, NULL, 0);
}

static int ili9341_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct ili9341_data *data = dev->data;
	int ret;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = ILI9XXX_PIXSET_MCU_16_BIT | ILI9XXX_PIXSET_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = ILI9XXX_PIXSET_MCU_18_BIT | ILI9XXX_PIXSET_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	ret = ili9341_transmit(dev, ILI9XXX_PIXSET, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int ili9341_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	const struct ili9341_config *config = dev->config;
    struct ili9341_data *data = dev->data;
	int ret;
	uint8_t tx_data = ILI9XXX_MADCTL_BGR;

    if (orientation == DISPLAY_ORIENTATION_NORMAL) {
        tx_data |= ILI9XXX_MADCTL_MX;
    } else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
        tx_data |= ILI9XXX_MADCTL_MV;
    } else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
        tx_data |= ILI9XXX_MADCTL_MY;
    } else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
        tx_data |= ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MX |
                ILI9XXX_MADCTL_MY;
    }

	ret = ili9341_transmit(dev, ILI9XXX_MADCTL, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

static int ili9341_configure(const struct device *dev)
{
	const struct ili9341_config *config = dev->config;

	int r;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;

	/* pixel format */
	if (config->pixel_format == ILI9XXX_PIXEL_FORMAT_RGB565) {
		pixel_format = PIXEL_FORMAT_RGB_565;
	} else {
		pixel_format = PIXEL_FORMAT_RGB_888;
	}

	r = ili9341_set_pixel_format(dev, pixel_format);
	if (r < 0) {
		return r;
	}

	/* orientation */
	if (config->rotation == 0U) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (config->rotation == 90U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (config->rotation == 180U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	r = ili9341_set_orientation(dev, orientation);
	if (r < 0) {
		return r;
	}

	if (config->inversion) {
		r = ili9341_transmit(dev, ILI9XXX_DINVON, NULL, 0U);
		if (r < 0) {
			return r;
		}
	}

	r = config->regs_init_fn(dev);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9341_init(const struct device *dev)
{
	const struct ili9341_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI device is not ready");
		printf("SPI not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->cmd_data)) {
		LOG_ERR("Command/Data GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure command/data GPIO (%d)", ret);
		return ret;
	}

	if (config->reset.port != NULL) {
		if (!device_is_ready(config->reset.port)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	ili9341_hw_reset(dev);

    ret = ili9341_transmit(dev, ILI9XXX_SWRESET, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Error transmit command Software Reset (%d)", ret);
		return ret;
	}

	ili9341_display_blanking_on(dev);

	ret = ili9341_configure(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure display (%d)", ret);
		return ret;
	}

	ret = ili9341_exit_sleep(dev);
	if (ret < 0) {
		LOG_ERR("Could not exit sleep mode (%d)", ret);
		return ret;
	}

	return 0;
}

static int ili9341_set_mem_area(const struct device *dev, const uint16_t x, 
                                const uint16_t y,
				                const uint16_t w, 
                                const uint16_t h)
{
	int ret;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	ret = ili9341_transmit(dev, ILI9XXX_CASET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	ret = ili9341_transmit(dev, ILI9XXX_PASET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ili9341_write(const struct device *dev, const uint16_t x, 
                        const uint16_t y,
			            const struct display_buffer_descriptor *desc, 
                        const void *buf)
{
	const struct ili9341_config *config = dev->config;
	struct ili9341_data *data = dev->data;
	int ret;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <= desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height, x, y);
	ret = ili9341_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ret = ili9341_transmit(dev, ILI9XXX_RAMWR, write_data_start,
				  desc->width * data->bytes_per_pixel * write_h);
	if (ret < 0) {
		return ret;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1U;

	write_data_start += desc->pitch * data->bytes_per_pixel;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * data->bytes_per_pixel * write_h;

		ret = spi_write_dt(&config->spi, &tx_bufs);
		if (ret < 0) {
			return ret;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}

static void ili9341_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9341_data *data = dev->data;
	const struct ili9341_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = config->x_resolution;
		capabilities->y_resolution = config->y_resolution;
	} else {
		capabilities->x_resolution = config->y_resolution;
		capabilities->y_resolution = config->x_resolution;
	}

	capabilities->current_orientation = data->orientation;
}

#ifdef CONFIG_PM_DEVICE
static int ili9341_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = ili9341_exit_sleep(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ili9341_enter_sleep(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* Device driver API*/
static const struct display_driver_api ili9341_api = {
	.blanking_on = ili9341_display_blanking_on,
	.blanking_off = ili9341_display_blanking_off,
	.write = ili9341_write,
	.get_capabilities = ili9341_get_capabilities,
	.set_pixel_format = ili9341_set_pixel_format,
	.set_orientation = ili9341_set_orientation,
};

static const struct ili9xxx_quirks ili9341_quirks = {
	.cmd_set = CMD_SET_1,
};

// #define ILI9XXX_INIT(inst)                                                     \
// 	ILI9341_REGS_INIT(inst);                                                 \
// 	static const struct ili9341_config ili9341_config_##inst = {              \
// 		.quirks = &ili##t##_quirks,                                    \
// 		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0) ,        \
// 		.cmd_data = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpios),                           \
// 		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
// 		.pixel_format = DT_INST_PROP(inst, pixel_format),  \
// 		.rotation = DT_INST_PROP(inst, rotation),          \
// 		.x_resolution = DT_INST_PROP(inst, width),                                         \
// 		.y_resolution = DT_INST_PROP(inst, height),                                        \
// 		.inversion = DT_INST_PROP(inst, display_inversion),\
// 		.regs = &ili9341_regs_##inst,                                     \
// 		.regs_init_fn = ili9341_regs_init,                            \
// 	};                                                                     \
// 									       \
// 	static struct ili9341_data ili9341_data_##inst;                           \

// 	DEVICE_DT_INST_DEFINE(inst, ili9341_init,                  \
// 			    NULL, &ili9341_data_##inst,                           \
// 			    &ili9341_config_##inst, POST_KERNEL,                  \
// 			    CONFIG_DISPLAY_INIT_PRIORITY, &ili9341_api);

// DT_INST_FOREACH_STATUS_OKAY(ILI9341_INIT)

#define ILI9341_INIT(inst)                                                                         \
	ILI9341_REGS_INIT(inst);                                                                   \
	static const struct ili9341_config ili9341_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0) ,        \
		.quirks = &ili9341_quirks,                                    \
		.cmd_data = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpios),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.rotation = DT_INST_ENUM_IDX(inst, rotation),                                \
		.x_resolution = DT_INST_PROP(inst, width),                                         \
		.y_resolution = DT_INST_PROP(inst, height),                                        \
		.inversion = DT_INST_PROP(inst, display_inversion),\
		.regs = &ili9341_regs_##inst,      \
		.regs_init_fn = ili9341_regs_init,                            \
	};                                                                                         \
	static struct ili9341_data ili9341_data_##inst;                                            \
	PM_DEVICE_DT_INST_DEFINE(inst, ili9341_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(inst, &ili9341_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &ili9341_data_##inst, &ili9341_config_##inst, POST_KERNEL,           \
			      CONFIG_DISPLAY_INIT_PRIORITY, &ili9341_api);

DT_INST_FOREACH_STATUS_OKAY(ILI9341_INIT)
