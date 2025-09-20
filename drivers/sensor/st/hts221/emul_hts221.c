#define DT_DRV_COMPAT st_hts221

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(st_hts221_emul);

#include "emul_hts221.h"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/util.h>

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "hts221_reg.h"

#include <zephyr/devicetree.h>

static void reg_write(const struct emul *target, int regn, int val)
{
	struct hts221_emul_data *data = target->data;

	__ASSERT_NO_MSG(regn < HTS221_REG_COUNT);
	data->reg[regn] = val;
}

static int reg_read(const struct emul *target, int regn)
{
	struct hts221_emul_data *data = target->data;

	__ASSERT_NO_MSG(regn < HTS221_REG_COUNT);

	return data->reg[regn];
}

static int hts221_emul_backend_set_channel(const struct emul *target, struct sensor_chan_spec ch,
					   const q31_t *value, int8_t shift)
{
	struct hts221_emul_data *data = target->data;
	int64_t scaled_value;
	int32_t millicelsius;
	int32_t reg_value;
	uint8_t reg_h, reg_l;

	switch ((int32_t)ch.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		reg_h = 1;
		reg_l = 2;
		break;
	default:
		return -ENOTSUP;
	}

	scaled_value = (int64_t)*value << shift;
	millicelsius = scaled_value * 1000 / ((int64_t)INT32_MAX + 1);
	reg_value = CLAMP(millicelsius / 125, 0, 0x7ff);

	data->reg[reg_h] = reg_value >> 3;
	data->reg[reg_l] = (reg_value & 0x7) << 5;

	return 0;
}

static int hts221_emul_backend_get_sample_range(const struct emul *target,
						struct sensor_chan_spec ch, q31_t *lower,
						q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (ch.chan_type !=
	    SENSOR_CHAN_AMBIENT_TEMP	) {
		return -ENOTSUP;
	}

	*shift = 8;
	*lower = 0;
	*upper = (int64_t)(255.875 * ((int64_t)INT32_MAX + 1)) >> *shift;
	*epsilon = (int64_t)(0.125 * ((int64_t)INT32_MAX + 1)) >> *shift;

	return 0;
}

static int hts221_emul_backend_set_attribute(const struct emul *target, struct sensor_chan_spec ch,
					     enum sensor_attribute attribute, const void *value)
{
	return 0;
}

static const struct emul_sensor_driver_api hts221_emul_api_sensor = {
	.set_channel = hts221_emul_backend_set_channel,
	.get_sample_range = hts221_emul_backend_get_sample_range,
};

int set_sensor_values(const struct emul *target, int16_t temp, int16_t tempFrac, int16_t hum,
		      int16_t humFrac)
{
	reg_write(target, HTS221_HUMIDITY_OUT_L | HTS221_AUTOINCREMENT_ADDR, humFrac * 128);
	reg_write(target, HTS221_HUMIDITY_OUT_L | HTS221_AUTOINCREMENT_ADDR + 1, hum);
	reg_write(target, HTS221_HUMIDITY_OUT_L | HTS221_AUTOINCREMENT_ADDR + 2, tempFrac * 32);
	reg_write(target, HTS221_HUMIDITY_OUT_L | HTS221_AUTOINCREMENT_ADDR + 3, temp);

	return 0;
}

int hts221_emul_init(const struct emul *target,
		     const struct device *parent)
{
	struct hts221_emul_data *data = target->data;
	struct emul_sensor_driver_api *api = target->backend_api;

	reg_write(target, HTS221_WHO_AM_I, HTS221_ID);

	reg_write(target, HTS221_H0_RH_X2 | HTS221_AUTOINCREMENT_ADDR + 1, 1);
	reg_write(target, HTS221_H0_RH_X2 | HTS221_AUTOINCREMENT_ADDR + 3, 1);

	reg_write(target, HTS221_H0_RH_X2 | HTS221_AUTOINCREMENT_ADDR + 10, 128);
	reg_write(target, HTS221_H0_RH_X2 | HTS221_AUTOINCREMENT_ADDR + 14, 32);


	struct hts221_emul_cfg *cfg;
	data->count=0;

	return 0;
}

static int hts221_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{

	unsigned int val;
	int reg;

	struct hts221_emul_data *data;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		data->cur_reg = msgs->buf[0];

		msgs++;

		if (msgs->flags & I2C_MSG_READ) {

			set_sensor_values(target, 20 + data->count, 4, 38, 5);
			data->count = (data->count + 1) % 5;

			for (int i = 0; i < msgs->len; ++i) {
				msgs->buf[i] = reg_read(target, data->cur_reg + i);
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			reg_write(target, data->cur_reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

static const struct i2c_emul_api hts221_emul_api_i2c = {
	.transfer = hts221_emul_transfer_i2c,
};

#define HTS221_EMUL_I2C(n)                                                                         \
	static const struct hts221_emul_cfg hts221_emul_cfg_##n;                                   \
	static struct hts221_emul_data hts221_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, hts221_emul_init, &hts221_emul_data_##n, &hts221_emul_cfg_##n,      \
			    &hts221_emul_api_i2c, &hts221_emul_api_sensor);

DT_INST_FOREACH_STATUS_OKAY(HTS221_EMUL_I2C)
