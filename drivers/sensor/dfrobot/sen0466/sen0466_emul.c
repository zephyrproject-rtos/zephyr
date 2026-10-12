/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT dfrobot_sen0466

#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sen0466.h"
#include "sen0466_emul.h"

LOG_MODULE_DECLARE(SEN0466, CONFIG_SENSOR_LOG_LEVEL);

/* Converter reading the NTC gives at 25 degrees. */
#define SEN0466_EMUL_DEFAULT_ADC 512U

struct sen0466_emul_data {
	uint8_t frame[SEN0466_FRAME_LEN];
	uint8_t fixed[SEN0466_FRAME_LEN];
	uint8_t next[SEN0466_FRAME_LEN];
	bool fixed_set;
	bool next_set;
	uint16_t concentration;
	uint16_t temp_adc;
	uint8_t gas_type;
	uint8_t decimals;
	uint8_t ack;
};

/* Kept separate from the driver so a change there shows up as a test failure. */
static uint8_t sen0466_emul_checksum(const uint8_t *frame)
{
	uint8_t sum = 0U;

	for (uint8_t i = 1U; i <= 6U; i++) {
		sum += frame[i];
	}

	return (uint8_t)(~sum + 1U);
}

static void sen0466_emul_build(struct sen0466_emul_data *data, uint8_t cmd)
{
	uint8_t *frame = data->frame;

	(void)memset(frame, 0, SEN0466_FRAME_LEN);
	frame[SEN0466_RX_HEAD] = SEN0466_FRAME_HEAD;
	frame[SEN0466_RX_CMD] = cmd;

	switch (cmd) {
	case SEN0466_CMD_SET_MODE:
		frame[SEN0466_RX_ACK] = data->ack;
		break;
	case SEN0466_CMD_GET_ALL_DATA:
		sys_put_be16(data->concentration, &frame[SEN0466_RX_CONC_HI]);
		frame[SEN0466_RX_GAS_TYPE] = data->gas_type;
		frame[SEN0466_RX_DECIMALS] = data->decimals;
		sys_put_be16(data->temp_adc, &frame[SEN0466_RX_TEMP_HI]);
		break;
	default:
		break;
	}

	frame[SEN0466_FRAME_CHECKSUM] = sen0466_emul_checksum(frame);
}

static int sen0466_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	struct sen0466_emul_data *data = target->data;

	ARG_UNUSED(addr);
	__ASSERT_NO_MSG((msgs != NULL) && (num_msgs > 0));

	/* A command: the register byte followed by the request frame. */
	if ((num_msgs == 2) && ((msgs[0].flags & I2C_MSG_READ) == 0) &&
	    ((msgs[1].flags & I2C_MSG_READ) == 0)) {
		if ((msgs[0].len != 1U) || (msgs[1].len != SEN0466_FRAME_LEN)) {
			LOG_ERR("Unexpected write of %u and %u bytes", msgs[0].len, msgs[1].len);
			return -EIO;
		}

		sen0466_emul_build(data, msgs[1].buf[SEN0466_TX_CMD]);
		return 0;
	}

	if (num_msgs != 1) {
		LOG_ERR("Unexpected transfer of %d messages", num_msgs);
		return -EIO;
	}

	if (((msgs[0].flags & I2C_MSG_READ) == 0) && (msgs[0].len == 1U)) {
		return 0;
	}

	if (((msgs[0].flags & I2C_MSG_READ) == 0) || (msgs[0].len != SEN0466_FRAME_LEN)) {
		LOG_ERR("Unexpected read of %u bytes", msgs[0].len);
		return -EIO;
	}

	if (data->next_set) {
		(void)memcpy(msgs[0].buf, data->next, SEN0466_FRAME_LEN);
		data->next_set = false;
	} else if (data->fixed_set) {
		(void)memcpy(msgs[0].buf, data->fixed, SEN0466_FRAME_LEN);
	} else {
		(void)memcpy(msgs[0].buf, data->frame, SEN0466_FRAME_LEN);
	}

	return 0;
}

void sen0466_emul_set_sample(const struct emul *target, uint16_t concentration, uint8_t gas_type,
			     uint8_t decimals, uint16_t temp_adc)
{
	struct sen0466_emul_data *data = target->data;

	data->concentration = concentration;
	data->gas_type = gas_type;
	data->decimals = decimals;
	data->temp_adc = temp_adc;
}

void sen0466_emul_set_ack(const struct emul *target, uint8_t ack)
{
	struct sen0466_emul_data *data = target->data;

	data->ack = ack;
}

void sen0466_emul_set_frame(const struct emul *target, const uint8_t *frame)
{
	struct sen0466_emul_data *data = target->data;

	if (frame == NULL) {
		data->fixed_set = false;
		return;
	}

	(void)memcpy(data->fixed, frame, SEN0466_FRAME_LEN);
	data->fixed_set = true;
}

void sen0466_emul_set_next_frame(const struct emul *target, const uint8_t *frame)
{
	struct sen0466_emul_data *data = target->data;

	if (frame == NULL) {
		data->next_set = false;
		return;
	}

	(void)memcpy(data->next, frame, SEN0466_FRAME_LEN);
	data->next_set = true;
}

static int sen0466_emul_init(const struct emul *target, const struct device *parent)
{
	struct sen0466_emul_data *data = target->data;

	ARG_UNUSED(parent);

	data->gas_type = SEN0466_GAS_TYPE_CO;
	data->temp_adc = SEN0466_EMUL_DEFAULT_ADC;
	data->ack = SEN0466_ACK_OK;
	sen0466_emul_build(data, SEN0466_CMD_GET_ALL_DATA);

	return 0;
}

static const struct i2c_emul_api sen0466_emul_api_i2c = {
	.transfer = sen0466_emul_transfer_i2c,
};

#define SEN0466_EMUL(inst)                                                                         \
	static struct sen0466_emul_data sen0466_emul_data_##inst;                                  \
	EMUL_DT_INST_DEFINE(inst, sen0466_emul_init, &sen0466_emul_data_##inst, NULL,              \
			    &sen0466_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(SEN0466_EMUL)
