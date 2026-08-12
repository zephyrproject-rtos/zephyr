/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Emulator for the MAXM86161 Optical Biosensor
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "maxm86161.h"
#include "maxm86161_emul.h"

LOG_MODULE_REGISTER(maxm86161_emul, CONFIG_SENSOR_LOG_LEVEL);

#define MAXM86161_MAXM86161_REG_COUNT 256

struct maxm86161_emul_data {
	uint8_t regs[MAXM86161_MAXM86161_REG_COUNT];
	/* Fault injection: when fault_armed, a transaction whose starting
	 * register address equals fault_reg returns fault_err.
	 */
	bool fault_armed;
	uint8_t fault_reg;
	int fault_err;
	/* When true (default), self-clearing bits (RESET, TEMP_EN) auto-clear
	 * after a write, matching the real device. Tests may disable this to
	 * exercise timeout / not-complete branches.
	 */
	bool selfclear_enabled;
};

struct maxm86161_emul_cfg {
	uint16_t addr;
};

int maxm86161_mock_set_register(void *data_ptr, uint8_t reg, uint8_t value)
{
	struct maxm86161_emul_data *data = data_ptr;

	data->regs[reg] = value;
	return 0;
}

int maxm86161_mock_get_register(void *data_ptr, uint8_t reg, uint8_t *value)
{
	struct maxm86161_emul_data *data = data_ptr;

	if (!value) {
		return -EINVAL;
	}
	*value = data->regs[reg];
	return 0;
}

int maxm86161_mock_set_fault(void *data_ptr, uint8_t reg, int err)
{
	struct maxm86161_emul_data *data = data_ptr;

	data->fault_armed = (err != 0);
	data->fault_reg = reg;
	data->fault_err = err;
	return 0;
}

int maxm86161_mock_clear_fault(void *data_ptr)
{
	struct maxm86161_emul_data *data = data_ptr;

	data->fault_armed = false;
	data->fault_err = 0;
	return 0;
}

int maxm86161_mock_set_selfclear(void *data_ptr, bool enable)
{
	struct maxm86161_emul_data *data = data_ptr;

	data->selfclear_enabled = enable;
	return 0;
}

static int maxm86161_emul_transfer_i2c(const struct emul *target,
				       struct i2c_msg msgs[],
				       int num_msgs, int addr)
{
	struct maxm86161_emul_data *data = target->data;

	if (!msgs || num_msgs < 1 || num_msgs > 2) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	/* First message must always be a write containing the register address */
	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("First message must be a write (register address)");
		return -EIO;
	}

	if (msgs[0].len < 1) {
		LOG_ERR("Write message too short: %d", msgs[0].len);
		return -EIO;
	}

	uint8_t reg = msgs[0].buf[0];

	/* Injected bus fault for the targeted register. */
	if (data->fault_armed && reg == data->fault_reg) {
		LOG_DBG("Injected fault %d on reg 0x%02x", data->fault_err, reg);
		return data->fault_err;
	}

	if (num_msgs == 1) {
		if (msgs[0].len < 2) {
			/* Address-only write (no data) - valid for some protocols */
			return 0;
		}

		/* Write transaction: buf[0]=reg, buf[1..n]=data */
		for (int i = 1; i < msgs[0].len; i++) {
			data->regs[(uint8_t)(reg + (i - 1))] = msgs[0].buf[i];
		}

		/* Handle self-clearing bits */
		if (data->selfclear_enabled) {
			if (reg == MAXM86161_REG_SYSTEM_CONTROL) {
				data->regs[MAXM86161_REG_SYSTEM_CONTROL] &=
				    ~MAXM86161_MSK_SYSTEM_CONTROL_RESET;
			}
			if (reg == MAXM86161_REG_DIE_TEMP_CONFIG) {
				data->regs[MAXM86161_REG_DIE_TEMP_CONFIG] &=
				    ~MAXM86161_MSK_DIE_TEMP_CONFIG_TEMP_EN;
			}
		}
	} else {
		/* Read transaction: msgs[0]=reg addr, msgs[1]=read buffer */
		if (!(msgs[1].flags & I2C_MSG_READ)) {
			LOG_ERR("Second message must be a read");
			return -EIO;
		}

		/*
		 * FIFO_DATA (0x08) has auto-increment disabled on the real
		 * device: repeated reads pop successive bytes from the same
		 * register. Emulate that by holding the address so a 3-byte
		 * burst always returns the three FIFO_DATA bytes pre-loaded at
		 * 0x08/0x09/0x0A, and multi-sample reads stay sample-aligned.
		 */
		if (reg == MAXM86161_REG_FIFO_DATA_REGISTER) {
			for (int i = 0; i < msgs[1].len; i++) {
				msgs[1].buf[i] = data->regs[(uint8_t)(reg + (i %
					MAXM86161_FIFO_SAMPLE_SIZE))];
			}
		} else {
			for (int i = 0; i < msgs[1].len; i++) {
				msgs[1].buf[i] = data->regs[(uint8_t)(reg + i)];
			}
		}
	}

	return 0;
}

static int maxm86161_emul_init(const struct emul *target,
			       const struct device *parent)
{
	struct maxm86161_emul_data *data = target->data;

	memset(data->regs, 0, sizeof(data->regs));
	data->fault_armed = false;
	data->fault_err = 0;
	data->selfclear_enabled = true;

	/* Part ID must match expected value for init to succeed */
	data->regs[MAXM86161_REG_PART_ID] = MAXM86161_PART_ID_VAL;

	return 0;
}

static const struct i2c_emul_api maxm86161_emul_api_i2c = {
	.transfer = maxm86161_emul_transfer_i2c,
};

#define MAXM86161_EMUL(n)                                                     \
	static const struct maxm86161_emul_cfg maxm86161_emul_cfg_##n = {     \
	    .addr = DT_INST_REG_ADDR(n),                                      \
	};                                                                    \
	static struct maxm86161_emul_data maxm86161_emul_data_##n;            \
	EMUL_DT_INST_DEFINE(n, maxm86161_emul_init, &maxm86161_emul_data_##n, \
			    &maxm86161_emul_cfg_##n, &maxm86161_emul_api_i2c, \
			    NULL)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_maxm86161
DT_INST_FOREACH_STATUS_OKAY(MAXM86161_EMUL)
