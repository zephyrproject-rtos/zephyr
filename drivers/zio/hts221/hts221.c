/* ST Microelectronics HTS221 humidity sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/hts221.pdf
 */

#include <zio.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/zio/hts221.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(hts221, CONFIG_ZIO_LOG_LEVEL);

static const struct zio_chan_desc hts221_chans[] = {
	{
		.name = "Humidity",
		.type = HTS221_HUMIDITY_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_NONE,
	},
	{
		.name = "Temperature",
		.type = HTS221_TEMPERATURE_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_BIG,
		.sign_bit = ZIO_SIGN_MSB,
	},
};

static const struct zio_attr_desc hts221_attr_descs[] = {
	{
		.type = ZIO_SAMPLE_RATE,
		.data_type = zio_variant_float,
	}
};

static int hts221_get_attr_descs(struct device *dev,
		const struct zio_attr_desc **attrs,
		u32_t *num_attrs)
{
	*attrs = hts221_attr_descs;
	*num_attrs = sizeof(hts221_attr_descs);
	return 0;
}

static int hts221_get_chan_descs(struct device *dev,
		const struct zio_chan_desc **chans,
		u32_t *num_chans)
{
	*chans = hts221_chans;
	*num_chans = sizeof(hts221_chans);
	return 0;
}

float linear_interpolation(lin_t *lin, s16_t x)
{
	return ((lin->y1 - lin->y0) * x +  ((lin->x1 * lin->y0) - (lin->x0 * lin->y1)))
		/ (lin->x1 - lin->x0);
}

static int hts221_read(struct device *dev)
{
	struct hts221_data *data = dev->driver_data;
	struct hts221_datum datum;
	axis1bit16_t data_raw_humidity;
	axis1bit16_t data_raw_temperature;

	/* Read output only if new value is available */
	hts221_reg_t reg;
	hts221_status_get(data->ctx, &reg.status_reg);

	if (reg.status_reg.h_da)
	{
		/* Read humidity data */
		memset(data_raw_humidity.u8bit, 0x00, sizeof(s16_t));
		hts221_humidity_raw_get(data->ctx, data_raw_humidity.u8bit);
		datum.hum_perc = linear_interpolation(&data->lin_hum,
						      data_raw_humidity.i16bit);
		if (datum.hum_perc < 0) datum.hum_perc = 0;
		if (datum.hum_perc > 100) datum.hum_perc = 100;
	}

	if (reg.status_reg.t_da)
	{
		/* Read temperature data */
		memset(data_raw_temperature.u8bit, 0x00, sizeof(s16_t));
		hts221_temperature_raw_get(data->ctx, data_raw_temperature.u8bit);
		datum.temp_degC = linear_interpolation(&data->lin_temp,
						       data_raw_temperature.i16bit);
	}

	zio_fifo_buf_push(&data->fifo, datum);

	return 0;
}

static void hts221_work_handler(struct k_work *work)
{
	struct hts221_data *data =
		CONTAINER_OF(work, struct hts221_data, work);

	hts221_read(data->dev);
}

static int hts221_trigger(struct device *dev)
{
	struct hts221_data *data = dev->driver_data;

	k_work_submit(&data->work);

	return 0;
}

static int hts221_attach_buf(struct device *dev, struct zio_buf *buf)
{
	struct hts221_data *data = dev->driver_data;

	return zio_fifo_buf_attach(&data->fifo, buf);
}

static int hts221_detach_buf(struct device *dev)
{
	struct hts221_data *data = dev->driver_data;

	return zio_fifo_buf_detach(&data->fifo);
}

static const u32_t hts221_odr_table[] = {0, 1, 7, 12};

static int hts221_get_odr_val(u32_t rate, u8_t *val)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(hts221_odr_table); odr++) {
		if (rate <= hts221_odr_table[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(hts221_odr_table)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	*val = odr;

	return 0;
}

static int hts221_set_attr(struct device *dev, const u32_t attr_idx,
		const struct zio_variant val)
{
	struct hts221_data *data = dev->driver_data;

	int res = 0;
	u32_t sample_rate = 0;
	u8_t odr = 0;

	switch (attr_idx) {
	case HTS221_SAMPLE_RATE_IDX:
		res = zio_variant_unwrap(val, sample_rate);
		if (res != 0) {
			return -EINVAL;
		}

		if (hts221_get_odr_val(sample_rate, &odr) < 0) {
			return -EINVAL;
		}
		hts221_data_rate_set(data->ctx, odr);
		data->sample_rate = sample_rate;
		return 0;
	default:
		return -EINVAL;
	}
}

static int hts221_get_attr(struct device *dev, u32_t attr_idx,
		struct zio_variant *var)
{
	struct hts221_data *data = dev->driver_data;

	switch (attr_idx) {
	case HTS221_SAMPLE_RATE_IDX:
		*var = zio_variant_wrap(data->sample_rate);
		return 0;
	default:
		return -EINVAL;
	}
}

static int hts221_init(struct device *dev)
{
	const struct hts221_config *config = dev->config->config_info;
	struct hts221_data *data = dev->driver_data;
	u8_t chip_id;

	data->bus = device_get_binding(config->master_dev_name);
	if (data->bus == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	config->bus_init(dev);

	if (hts221_device_id_get(data->ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != HTS221_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* Read humidity calibration coefficient */
	axis1bit16_t coeff;

	hts221_hum_adc_point_0_get(data->ctx, coeff.u8bit);
	data->lin_hum.x0 = coeff.i16bit;
	hts221_hum_rh_point_0_get(data->ctx, coeff.u8bit);
	data->lin_hum.y0 = coeff.u8bit[0];
	hts221_hum_adc_point_1_get(data->ctx, coeff.u8bit);
	data->lin_hum.x1 = coeff.i16bit;
	hts221_hum_rh_point_1_get(data->ctx, coeff.u8bit);
	data->lin_hum.y1 = coeff.u8bit[0];

	/* Read temperature calibration coefficient */
	hts221_temp_adc_point_0_get(data->ctx, coeff.u8bit);
	data->lin_temp.x0 = coeff.i16bit;
	hts221_temp_deg_point_0_get(data->ctx, coeff.u8bit);
	data->lin_temp.y0 = coeff.u8bit[0];
	hts221_temp_adc_point_1_get(data->ctx, coeff.u8bit);
	data->lin_temp.x1 = coeff.i16bit;
	hts221_temp_deg_point_1_get(data->ctx, coeff.u8bit);
	data->lin_temp.y1 = coeff.u8bit[0];

	/* Enable Block Data Update */
	hts221_block_data_update_set(data->ctx, PROPERTY_ENABLE);
	/* Set Output Data Rate */
	hts221_data_rate_set(data->ctx, CONFIG_HTS221_SAMPLING_RATE);
	/* Device power on */
	hts221_power_on_set(data->ctx, PROPERTY_ENABLE);

	data->dev = dev;
	k_work_init(&data->work, hts221_work_handler);

#ifdef CONFIG_HTS221_TRIGGER
	if (hts221_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

static const struct zio_dev_api hts221_driver_api = {
	.set_attr = hts221_set_attr,
	.get_attr = hts221_get_attr,
	.get_attr_descs = hts221_get_attr_descs,
	.get_chan_descs = hts221_get_chan_descs,
	.get_chan_attr_descs = NULL,
	.trigger = hts221_trigger,
	.attach_buf = hts221_attach_buf,
	.detach_buf = hts221_detach_buf,
};

static const struct hts221_config hts221_config = {
	.master_dev_name = DT_INST_0_ST_HTS221_BUS_NAME,
#if defined(DT_ST_HTS221_BUS_I2C)
	.bus_init = hts221_i2c_init,
	.i2c_slv_addr = DT_INST_0_ST_HTS221_BASE_ADDRESS,
#endif
#ifdef CONFIG_HTS221_TRIGGER
	.drdy_port = DT_INST_0_ST_HTS221_DRDY_GPIOS_CONTROLLER,
	.drdy_pin = DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN,
#endif

};

static struct hts221_data hts221_data = {
	.fifo = ZIO_FIFO_BUF_INITIALIZER(hts221_data.fifo, struct hts221_datum,
					 1),
};

DEVICE_AND_API_INIT(hts221, DT_ST_HTS221_0_LABEL, hts221_init,
		    &hts221_data, &hts221_config,
		    POST_KERNEL, CONFIG_ZIO_INIT_PRIORITY,
		    &hts221_driver_api);
