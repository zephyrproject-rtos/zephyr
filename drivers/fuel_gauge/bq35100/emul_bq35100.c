/*
 * Copyright (c) 2025 Orgatex GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the BQ35100 gas gauge
 */

#define DT_DRV_COMPAT ti_bq35100

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bq35100.h"

LOG_MODULE_REGISTER(EMUL_BQ35100);

/* Standard commands are byte addressed and auto-increment on read */
#define BQ35100_EMUL_REG_COUNT 0x80

/*
 * Readings reported to the driver. The test asserts on these values, keep both
 * sides in sync.
 */
#define BQ35100_EMUL_STATUS       (BQ3500_INITCOMP_BIT_MASK | (SECURITY_UNSEALED << 13))
#define BQ35100_EMUL_ACCUMULATED  (-300000)
#define BQ35100_EMUL_TEMPERATURE  2982
#define BQ35100_EMUL_VOLTAGE      3600
#define BQ35100_EMUL_CURRENT      (-25)
#define BQ35100_EMUL_SOH          95
#define BQ35100_EMUL_DESIGN_CAPACITY 1200

struct bq35100_emul_data {
	/** Standard command registers, indexed by command code */
	uint8_t regs[BQ35100_EMUL_REG_COUNT];
	/** Command code selected by the most recent write */
	uint8_t cur_reg;
};

struct bq35100_emul_cfg {
	/** I2C address of the emulator */
	uint16_t addr;
};

static void emul_bq35100_stage_mac_answer(struct bq35100_emul_data *data, uint16_t answer)
{
	sys_put_le16(answer, &data->regs[BQ35100_REG_MAC_DATA]);
}

static int emul_bq35100_control(struct bq35100_emul_data *data, uint16_t subcommand)
{
	switch (subcommand) {
	case BQ35100_MAC_CMD_DEVICETYPE:
		emul_bq35100_stage_mac_answer(data, BQ35100_DEVICE_TYPE);
		break;
	default:
		/*
		 * Security mode changes, gauge start/stop and reset need no state
		 * here: the emulator reports a permanently unsealed gauge.
		 */
		LOG_DBG("Ignoring control subcommand 0x%04X", subcommand);
		break;
	}

	return 0;
}

static int emul_bq35100_write(const struct emul *target, const uint8_t *buf, size_t len)
{
	struct bq35100_emul_data *data = target->data;

	if (len < 1) {
		LOG_ERR("Write without a command code");
		return -EIO;
	}

	data->cur_reg = buf[0];

	if (len == 1) {
		return 0;
	}

	switch (data->cur_reg) {
	case BQ35100_REG_CONTROL_STATUS:
		if (len != 3) {
			LOG_ERR("Control subcommand needs 2 data bytes, got %zu", len - 1);
			return -EIO;
		}

		return emul_bq35100_control(data, sys_get_le16(&buf[1]));
	case BQ35100_REG_MAC:
		if (len < 3) {
			LOG_ERR("MAC access needs a data flash address");
			return -EIO;
		}

		LOG_DBG("MAC access to 0x%04X with %zu payload byte(s)", sys_get_le16(&buf[1]),
			len - 3);
		return 0;
	case BQ35100_REG_MAC_DATA_SUM:
		LOG_DBG("MAC checksum 0x%02X, length %u", buf[1], buf[2]);
		return 0;
	default:
		LOG_ERR("Unsupported write to command 0x%02X", data->cur_reg);
		return -EIO;
	}
}

static int emul_bq35100_read(const struct emul *target, uint8_t *buf, size_t len)
{
	struct bq35100_emul_data *data = target->data;

	if (data->cur_reg + len > BQ35100_EMUL_REG_COUNT) {
		LOG_ERR("Read of %zu byte(s) from 0x%02X runs past the register file", len,
			data->cur_reg);
		return -EIO;
	}

	memcpy(buf, &data->regs[data->cur_reg], len);

	return 0;
}

static int bq35100_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	/*
	 * The driver selects a command code and reads it back in two separate
	 * transfers, so the selected code has to survive between them.
	 */
	if (num_msgs != 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs->flags & I2C_MSG_READ) {
		return emul_bq35100_read(target, msgs->buf, msgs->len);
	}

	return emul_bq35100_write(target, msgs->buf, msgs->len);
}

static const struct i2c_emul_api bq35100_emul_api_i2c = {
	.transfer = bq35100_emul_transfer_i2c,
};

static int emul_bq35100_init(const struct emul *target, const struct device *parent)
{
	struct bq35100_emul_data *data = target->data;

	ARG_UNUSED(parent);

	sys_put_le16(BQ35100_EMUL_STATUS, &data->regs[BQ35100_REG_CONTROL_STATUS]);
	sys_put_le32(BQ35100_EMUL_ACCUMULATED, &data->regs[BQ35100_REG_ACCUMULATED_CAPACITY]);
	sys_put_le16(BQ35100_EMUL_TEMPERATURE, &data->regs[BQ35100_REG_TEMPERATURE]);
	sys_put_le16(BQ35100_EMUL_VOLTAGE, &data->regs[BQ35100_REG_VOLTAGE]);
	sys_put_le16(BQ35100_EMUL_CURRENT, &data->regs[BQ35100_REG_CURRENT]);
	sys_put_le16(BQ35100_EMUL_SOH, &data->regs[BQ35100_REG_STATE_OF_HEALTH]);
	sys_put_le16(BQ35100_EMUL_DESIGN_CAPACITY, &data->regs[BQ35100_REG_DESIGN_CAPACITY]);

	return 0;
}

#define BQ35100_EMUL(n)                                                                            \
	static struct bq35100_emul_data bq35100_emul_data_##n;                                     \
	static const struct bq35100_emul_cfg bq35100_emul_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_bq35100_init, &bq35100_emul_data_##n,                          \
			    &bq35100_emul_cfg_##n, &bq35100_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(BQ35100_EMUL)
