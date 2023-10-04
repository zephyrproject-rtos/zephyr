/*
 * Copyright 2023 Jonas Otto
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/usb_c/emul_fusb302b.h>

#define DT_DRV_COMPAT fcs_fusb302b

LOG_MODULE_REGISTER(fusb302b_emul, LOG_LEVEL_DBG);

struct chip_state {
	uint8_t device_id;
	uint8_t measure;
	uint8_t switches0;
	uint8_t power;
	uint8_t control0;
	uint8_t control3;
};

struct environment_state {
	int vbus_mv;
	/* TODO: CC state */
};

struct fusb_emul_data {
	struct chip_state state;
	struct environment_state env;
};

struct fusb_emul_cfg {
	uint16_t addr;
};

void set_vbus(const struct emul *emul, int mv)
{
	struct fusb_emul_data *data = emul->data;
	data->env.vbus_mv = mv;
}

void reset_chip(struct chip_state *state)
{
	LOG_INF("Resetting chip");
	state->device_id = 0b10010001;
	state->measure = 0b00110001;
	state->switches0 = 0b00000011;
	state->power = 1;
	state->control0 = 0b00100100;
	state->control3 = 0b00000110;
}

int fusb_emul_init(const struct emul *emul, const struct device *parent)
{
	struct fusb_emul_data *data = emul->data;
	reset_chip(&data->state);
	data->env.vbus_mv = 0;
	return 0;
}

bool bit_is_set(uint8_t val, uint8_t index)
{
	return (val & 1 << index) ? true : false;
}

int mdac_mv(const struct fusb_emul_data *data)
{
	const struct chip_state *state = &data->state;

	if (!(bit_is_set(state->power, 2) && bit_is_set(state->power, 1))) {
		LOG_WRN("Reading status0 while measure block is not powered");
		return 0;
	}

	bool meas_cc1 = bit_is_set(state->switches0, 2);
	bool meas_cc2 = bit_is_set(state->switches0, 3);
	if (meas_cc1 && meas_cc2) {
		LOG_ERR("Invalid configuration: CC1 and CC2 connected to measure block");
	}

	bool meas_vbus = bit_is_set(state->measure, 6);
	if (meas_vbus && (meas_cc1 || meas_cc2)) {
		LOG_ERR("Invalid configuration: CC1 and CC2 must not be connected to measure block "
			"while measuring VBUS");
	}

	uint8_t mdac = state->measure & 0b00111111;

	if (meas_vbus) {
		return (mdac + 1) * 420;
	} else {
		return (mdac + 1) * 42;
	}
}

bool status0_cmp(const struct fusb_emul_data *data)
{
	const struct chip_state *state = &data->state;
	const struct environment_state *env = &data->env;

	/* Set COMP */
	int mdac_reference_voltage = mdac_mv(data);
	bool meas_vbus = bit_is_set(state->measure, 6);
	if (meas_vbus) {
		uint8_t higher = env->vbus_mv > mdac_reference_voltage ? 1 : 0;

	} else {
		LOG_WRN("CC measurement not impl");
	}

	return env->vbus_mv > mdac_reference_voltage;
}

uint8_t status0(const struct fusb_emul_data *data)
{
	/* Not implemented: WAKE, ALERT, CRC_CHK, ACTIVITY, VBUSOK, BC_LVL */
	uint8_t val = 0;

	val |= status0_cmp(data) << 5;
	return val;
}

void reg_handle_rw(uint8_t *data, struct i2c_msg *msgs, int num_msgs)
{
	if (msgs[0].len == 1) {
		/* Read */
		__ASSERT_NO_MSG(num_msgs = 2);
		__ASSERT_NO_MSG(msgs[1].len == 1);
		__ASSERT_NO_MSG(msgs[1].flags & I2C_MSG_READ);
		msgs[1].buf[0] = *data;

	} else {
		/* Write */
		__ASSERT_NO_MSG(num_msgs = 1);
		__ASSERT_NO_MSG(msgs[0].len == 2);
		*data = msgs[0].buf[1];
	}
}

int fusb_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{

	struct fusb_emul_data *data = target->data;
	const struct fusb_emul_cfg *cfg = target->cfg;

	__ASSERT_NO_MSG(msgs != NULL);
	__ASSERT_NO_MSG(num_msgs > 0);
	__ASSERT_NO_MSG(addr == cfg->addr);

	/* LOG_INF("num_msgs=%d", num_msgs); */

	__ASSERT(!(msgs[0].flags & I2C_MSG_READ), "First message must be register address write");

	__ASSERT_NO_MSG(msgs[0].len > 0);
	uint8_t reg = msgs[0].buf[0];
	/* LOG_DBG("Accessing register 0x%02x", reg); */

	switch (reg) {
	case 0x01: {
		/* Device ID */
		__ASSERT_NO_MSG(msgs[0].len == 1);
		__ASSERT_NO_MSG(num_msgs == 2);
		__ASSERT_NO_MSG(msgs[1].len == 1);
		__ASSERT_NO_MSG(msgs[1].flags & I2C_MSG_READ);
		msgs[1].buf[0] = data->state.device_id;
		return 0;
	}
	case 0x02: {
		/* Switches0 */
		reg_handle_rw(&data->state.switches0, msgs, num_msgs);
		bool meas_cc1 = bit_is_set(data->state.switches0, 2);
		bool meas_cc2 = bit_is_set(data->state.switches0, 3);
		if (meas_cc1 && meas_cc2) {
			LOG_ERR("Both CC1 and CC2 connected to measure block!");
			__ASSERT(false, "Both CC1 and CC2 connected to measure block!");
			return -EINVAL;
		}
		return 0;
	}
	case 0x04: {
		/* Measure */
		reg_handle_rw(&data->state.measure, msgs, num_msgs);
		return 0;
	}
	case 0x06: {
		/* Control0 */
		LOG_WRN("Control0 behavior not modeled");
		reg_handle_rw(&data->state.control0, msgs, num_msgs);
		return 0;
	}
	case 0x09: {
		/* Control3 */
		LOG_WRN("Control3 behavior not modeled");
		reg_handle_rw(&data->state.control3, msgs, num_msgs);
		return 0;
	}
	case 0x0b: {
		/* Power */
		reg_handle_rw(&data->state.power, msgs, num_msgs);
		return 0;
	}
	case 0x0c: {
		/* Reset */
		__ASSERT_NO_MSG(msgs[0].len == 2);
		if (msgs[0].buf[1] == 0x01) {
			reset_chip(&data->state);
			return 0;
		} else {
			LOG_ERR("PD_RESET not implemented");
			return -ENOSYS;
		}
		break;
	}
	case 0x40: {
		/* Status0 */
		__ASSERT(msgs[0].len == 1, "Status0 register is read-only");
		__ASSERT(num_msgs == 2, "Status0 register is read-only");
		__ASSERT(msgs[1].len == 1, "Status0 register is read-only");
		__ASSERT(msgs[1].flags & I2C_MSG_READ, "Status0 register is read-only");
		msgs[1].buf[0] = status0(data);
		return 0;
	}
	default: {
		LOG_ERR("Register access for register 0x%02x not implemented", reg);
		return -ENOSYS;
	}
	}

	__ASSERT_NO_MSG(false); /* Unreachable */
}

static struct i2c_emul_api fusb_emul_api_i2c = {
	.transfer = fusb_emul_transfer_i2c,
};

#define FUSB_EMUL_DATA(n) static struct fusb_emul_data fusb_emul_data_##n;

#define FUSB_EMUL_DEFINE(n, bus_api)                                                               \
	EMUL_DT_INST_DEFINE(n, fusb_emul_init, &fusb_emul_data_##n, &fusb_emul_cfg_##n, &bus_api,  \
			    NULL)

#define FUSB_EMUL_I2C(n)                                                                           \
	FUSB_EMUL_DATA(n)                                                                          \
	static const struct fusb_emul_cfg fusb_emul_cfg_##n = {.addr = DT_INST_REG_ADDR(n)};       \
	FUSB_EMUL_DEFINE(n, fusb_emul_api_i2c)

DT_INST_FOREACH_STATUS_OKAY(FUSB_EMUL_I2C)
