/* Copyright (c) 2024 Daniel Kampert
 * Author: Daniel Kampert <DanielKampert@kampis-Elektroecke.de>
 */

#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT          avago_apds9306
#define APDS9306_EMUL_NUM_REGS 128

LOG_MODULE_REGISTER(avago_apds9306, CONFIG_SENSOR_LOG_LEVEL);

struct apds9306_emul_data {
	uint8_t reg[APDS9306_EMUL_NUM_REGS];
};

struct apds9306_emul_config {
	uint8_t *reg;
	union {
		/* Unit address (chip select ordinal) of emulator */
		uint16_t chipsel;
		/* I2C address of emulator */
		uint16_t addr;
	};
};

static void apds9306_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct apds9306_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < APDS9306_EMUL_NUM_REGS);

	data->reg[reg] = val;
}

static uint8_t apds9306_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct apds9306_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < APDS9306_EMUL_NUM_REGS);

	return data->reg[reg];
}

static int apds9306_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				      int addr)
{
	unsigned int val;
	int reg;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read!");
			return -EIO;
		}

		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d!", msgs->len);
			return -EIO;
		}

		reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			switch (msgs->len) {
			case 1:
				val = apds9306_emul_get_reg(target, reg);
				msgs->buf[0] = val;
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d!", msgs->len);
				return -EIO;
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d!", msgs->len);
			}

			apds9306_emul_set_reg(target, reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d!", num_msgs);
		return -EIO;
	}

	return 0;
}

static int apds9306_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				     const q31_t *value, int8_t shift)
{
	return 0;
}

static int apds9306_emul_init(const struct emul *target, const struct device *parent)
{
	const struct apds9306_emul_config *cfg = target->cfg;
	struct apds9306_emul_data *data = target->data;
	uint8_t *reg = cfg->reg;

	ARG_UNUSED(parent);

	return 0;
}

static const struct i2c_emul_api apds9306_emul_i2c_api = {
	.transfer = apds9306_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api apds9306_emul_driver_api = {
	.set_channel = apds9306_emul_set_channel,
};

#define APDS9306_EMUL(inst)                                                                        \
	static struct apds9306_emul_data apds9306_emul_data_##inst;                                \
	static const struct apds9306_emul_config apds9306_emul_config_##inst;                      \
                                                                                                   \
	EMUL_DT_INST_DEFINE(inst, apds9306_emul_init, &apds9306_emul_data_##inst,                  \
			    &apds9306_emul_config_##inst, &apds9306_emul_i2c_api,                  \
			    &apds9306_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APDS9306_EMUL)
