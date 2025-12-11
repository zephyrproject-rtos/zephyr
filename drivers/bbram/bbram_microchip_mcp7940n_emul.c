/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp7940n

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_bbram.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(bbram_microchip_mcp7940n, CONFIG_BBRAM_LOG_LEVEL);

#define MICROCHIP_MCP7940N_SRAM_OFFSET               0x20
#define MICROCHIP_MCP7940N_SRAM_SIZE                 64
#define MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS 0x03
#define MICROCHIP_MCP7940N_RTCWKDAY_VBATEN_BIT       BIT(3)
#define MICROCHIP_MCP7940N_RTCWKDAY_PWRFAIL_BIT      BIT(4)

struct mcp7940n_emul_cfg {
};

struct mcp7940n_emul_data {
	uint8_t rtcwkday;
	uint8_t data[MICROCHIP_MCP7940N_SRAM_SIZE];
};

static int mcp7940n_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);
	return 0;
}

static int mcp7940n_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				      int addr)
{
	struct mcp7940n_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
	if (FIELD_GET(I2C_MSG_READ, msgs->flags)) {
		LOG_ERR("Unexpected read");
		return -EIO;
	}
	if (msgs->len < 1) {
		LOG_ERR("Unexpected msg0 length %d", msgs->len);
		return -EIO;
	}

	uint8_t regn = msgs->buf[0];
	bool is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
	bool is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;

	if (!is_stop && !is_read) {
		/* First message was a write with the register number, check next message */
		msgs++;
		is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
		is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;
	}

	if (is_read) {
		/* Read data */
		if (regn == MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS) {
			msgs->buf[0] = data->rtcwkday;
			return 0;
		}
		if (regn >= MICROCHIP_MCP7940N_SRAM_OFFSET &&
		    regn + msgs->len <=
			    MICROCHIP_MCP7940N_SRAM_OFFSET + MICROCHIP_MCP7940N_SRAM_SIZE) {
			for (int i = 0; i < msgs->len; ++i) {
				msgs->buf[i] =
					data->data[regn + i - MICROCHIP_MCP7940N_SRAM_OFFSET];
			}
			return 0;
		}
	} else {
		/* Write data */
		if (regn == MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS) {
			data->rtcwkday = msgs->buf[1];
			return 0;
		}
		if (regn >= MICROCHIP_MCP7940N_SRAM_OFFSET &&
		    regn + msgs->len - 1 <=
			    MICROCHIP_MCP7940N_SRAM_OFFSET + MICROCHIP_MCP7940N_SRAM_SIZE) {
			for (int i = 0; i < msgs->len; ++i) {
				data->data[regn + i - MICROCHIP_MCP7940N_SRAM_OFFSET] =
					msgs->buf[1 + i];
			}
			return 0;
		}
	}

	return -EIO;
}

static const struct i2c_emul_api mcp7940n_emul_api_i2c = {
	.transfer = mcp7940n_emul_transfer_i2c,
};

static int mcp7940n_emul_backend_set_data(const struct emul *target, size_t offset, size_t count,
					  const uint8_t *buffer)
{
	struct mcp7940n_emul_data *data = target->data;

	if (offset + count > MICROCHIP_MCP7940N_SRAM_SIZE) {
		return -ERANGE;
	}

	for (size_t i = 0; i < count; ++i) {
		data->data[offset + i] = buffer[i];
	}
	return 0;
}

static int mcp7940n_emul_backend_get_data(const struct emul *target, size_t offset, size_t count,
					  uint8_t *buffer)
{
	struct mcp7940n_emul_data *data = target->data;

	if (offset + count > MICROCHIP_MCP7940N_SRAM_SIZE) {
		return -ERANGE;
	}

	for (size_t i = 0; i < count; ++i) {
		buffer[i] = data->data[offset + i];
	}
	return 0;
}

static const struct emul_bbram_driver_api mcp7940n_emul_backend_api = {
	.set_data = mcp7940n_emul_backend_set_data,
	.get_data = mcp7940n_emul_backend_get_data,
};

#define MCP7940N_EMUL(inst)                                                                        \
	static const struct mcp7940n_emul_cfg mcp7940n_emul_cfg_##inst;                            \
	static struct mcp7940n_emul_data mcp7940n_emul_data_##inst;                                \
	EMUL_DT_INST_DEFINE(inst, mcp7940n_emul_init, &mcp7940n_emul_data_##inst,                  \
			    &mcp7940n_emul_cfg_##inst, &mcp7940n_emul_api_i2c,                     \
			    &mcp7940n_emul_backend_api)

DT_INST_FOREACH_STATUS_OKAY(MCP7940N_EMUL)
