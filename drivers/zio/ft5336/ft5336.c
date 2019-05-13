/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zio.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/zio/ft5336.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ft5336, CONFIG_ZIO_LOG_LEVEL);

#define MAX_TOUCHES		(5)

struct ft5336_config {
	char *i2c_name;
	u8_t i2c_address;
};

struct ft5336_data {
	struct device *i2c;
	struct k_work work;
	struct device *dev;
	ZIO_FIFO_BUF_DECLARE(fifo, struct ft5336_datum, CONFIG_FT5336_FIFO_SIZE);
};

struct ft5336_raw {
	u8_t gest_id;
	u8_t td_status;
	struct {
		u8_t xh;
		u8_t xl;
		u8_t yh;
		u8_t yl;
		u8_t reserved[2];
	} touch[MAX_TOUCHES];
} __packed;

static const struct zio_chan_desc ft5336_chans[] = {
	{
		.name = "x",
		.type = FT5336_COORD_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_NONE,
	},
	{
		.name = "y",
		.type = FT5336_COORD_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_NONE,
	},
	{
		.name = "event",
		.type = FT5336_EVENT_TYPE,
		.bit_width = 8,
		.byte_size = 1,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_NONE,
	},
	{
		.name = "id",
		.type = FT5336_ID_TYPE,
		.bit_width = 8,
		.byte_size = 1,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_NONE,
	},
};

static int ft5336_get_chan_descs(struct device *dev,
		const struct zio_chan_desc **chans,
		u32_t *num_chans)
{
	*chans = ft5336_chans;
	*num_chans = sizeof(ft5336_chans);
	return 0;
}

static int ft5336_read(struct device *dev)
{
	const struct ft5336_config *config = dev->config->config_info;
	struct ft5336_data *data = dev->driver_data;
	struct ft5336_raw raw;
	struct ft5336_datum datum;
	int i;

	if (i2c_burst_read(data->i2c, config->i2c_address, 1, (u8_t *) &raw,
			   sizeof(raw))) {
		LOG_ERR("Could not read raw data");
		return -EIO;
	}

	for (i = 0; i < MAX_TOUCHES; i++) {
		datum.id = i;
		datum.event = raw.touch[i].xh >> 6;
		datum.x = ((raw.touch[i].xh & 0xf) << 8) | raw.touch[i].xl;
		datum.y = ((raw.touch[i].yh & 0xf) << 8) | raw.touch[i].yl;
		LOG_DBG("touch %d event %d at (%d,%d)",
			datum.id, datum.event, datum.x, datum.y);

		zio_fifo_buf_push(&data->fifo, datum);
	}

	return 0;
}

static void ft5336_work_handler(struct k_work *work)
{
	struct ft5336_data *data =
		CONTAINER_OF(work, struct ft5336_data, work);

	ft5336_read(data->dev);
}

static int ft5336_trigger(struct device *dev)
{
	struct ft5336_data *data = dev->driver_data;

	k_work_submit(&data->work);

	return 0;
}

static int ft5336_attach_buf(struct device *dev, struct zio_buf *buf)
{
	struct ft5336_data *data = dev->driver_data;

	return zio_fifo_buf_attach(&data->fifo, buf);
}

static int ft5336_detach_buf(struct device *dev)
{
	struct ft5336_data *data = dev->driver_data;

	return zio_fifo_buf_detach(&data->fifo);
}

static int ft5336_init(struct device *dev)
{
	const struct ft5336_config *config = dev->config->config_info;
	struct ft5336_data *data = dev->driver_data;

	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address, 0, 0)) {
		LOG_ERR("Could not enable");
		return -EINVAL;
	}

	data->dev = dev;
	k_work_init(&data->work, ft5336_work_handler);

	return 0;
}

static const struct zio_dev_api ft5336_driver_api = {
	.set_attr = NULL,
	.get_attr = NULL,
	.get_attr_descs = NULL,
	.get_chan_descs = ft5336_get_chan_descs,
	.get_chan_attr_descs = NULL,
	.trigger = ft5336_trigger,
	.attach_buf = ft5336_attach_buf,
	.detach_buf = ft5336_detach_buf,
};

static const struct ft5336_config ft5336_config = {
	.i2c_name = DT_INST_0_FOCALTECH_FT5336_BUS_NAME,
	.i2c_address = DT_INST_0_FOCALTECH_FT5336_BASE_ADDRESS,
};

static struct ft5336_data ft5336_data = {
	.fifo = ZIO_FIFO_BUF_INITIALIZER(ft5336_data.fifo, struct ft5336_datum,
					 CONFIG_FT5336_FIFO_SIZE),
};

DEVICE_AND_API_INIT(ft5336, DT_INST_0_FOCALTECH_FT5336_LABEL, ft5336_init,
		    &ft5336_data, &ft5336_config,
		    POST_KERNEL, CONFIG_ZIO_INIT_PRIORITY,
		    &ft5336_driver_api);
