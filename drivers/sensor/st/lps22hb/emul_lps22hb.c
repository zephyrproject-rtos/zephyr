#define DT_DRV_COMPAT st_lps22hb_press

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/devicetree.h>

#include "emul_lps22hb.h"

LOG_MODULE_REGISTER(LPS22HB_EMUL);

static void reg_write(const struct emul *target, int regn, int val)
{
	struct lps22hb_emul_data *data = target->data;
	__ASSERT_NO_MSG(regn < LPS22HB_REG_COUNT);
	data->reg[regn] = val;
}

static int reg_read(const struct emul *target, int regn)
{
	struct lps22hb_emul_data *data = target->data;

	__ASSERT_NO_MSG(regn < LPS22HB_REG_COUNT);

	return data->reg[regn];
}

int set_sensor_values_a(const struct emul *target, int16_t temp, int16_t press)
{
	reg_write(target, LPS22HB_REG_PRESS_OUT_XL, 0);
	reg_write(target, LPS22HB_REG_PRESS_OUT_L, temp % 2);
	reg_write(target, LPS22HB_REG_PRESS_OUT_H, temp / 1.6);
	reg_write(target, LPS22HB_REG_TEMP_OUT_L, temp % 3);
	reg_write(target, LPS22HB_REG_TEMP_OUT_H, temp / 2.56);

	return 0;
}

static int lps22hb_emul_init(const struct emul *target, const struct device *parent)
{
	const struct lps22hb_emul_cfg *const config = target->cfg;
	struct lps22hb_emul_data *data = target->data;

	reg_write(target, LPS22HB_REG_WHO_AM_I, LPS22HB_VAL_WHO_AM_I);
	set_sensor_values_a(target, 101, 101);

	return 0;
}

static int lps22hb_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	unsigned int val;
	int reg;

	struct lps22hb_emul_data *data;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 1:
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}

		data->cur_reg = msgs->buf[0];

		msgs++;

		if (msgs->flags & I2C_MSG_READ) {
			set_sensor_values_a(target, 21 + data->count, 101);
			data->count = (data->count + 1) % 4;

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

static const struct i2c_emul_api lps22hb_emul_api_i2c = {
	.transfer = lps22hb_emul_transfer_i2c,
};

#define LPS22HB_EMUL_DEFINE(inst)                                                                  \
	static struct lps22hb_emul_data lps22hb_emul_data_##inst;                                  \
	static const struct lps22hb_emul_cfg lps22hb_emul_cfg_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, lps22hb_emul_init, &lps22hb_emul_data_##inst,                    \
			    &lps22hb_emul_cfg_##inst, &lps22hb_emul_api_i2c, 0);

DT_INST_FOREACH_STATUS_OKAY(LPS22HB_EMUL_DEFINE)
