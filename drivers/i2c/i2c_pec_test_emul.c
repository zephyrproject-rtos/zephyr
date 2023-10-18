/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c_pec_test_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

#define DT_DRV_COMPAT zephyr_i2c_pec_test

#define BUFFER_SIZE CONFIG_I2C_PEC_TEST_EMUL_BUFFER_SIZE

LOG_MODULE_REGISTER(i2c_echo_pec_emul);

struct emul_i2c_pec_test_cfg {
	uint16_t addr;
};

enum emul_i2c_pec_test_state {
	IDLE,
	WAIT_LEN,
	ACCESS,
	PEC,
};

/*  Run-time data used by the emulator */
struct emul_i2c_pec_test_data {
	uint8_t buf[BUFFER_SIZE];
	enum emul_i2c_pec_test_state state;
	bool read;
	int index;
	int remaining;

	uint8_t pec;
	bool corrupt_pec;
};

static int i2c_pec_test_emul_transfer(const struct emul *target,
				      struct i2c_msg *msgs,
				      int num_msgs, int addr)
{
	const uint8_t addr8 = (uint8_t)(addr << 1);
	const uint8_t addr8_read = addr8 | 0x1;
	struct emul_i2c_pec_test_data *data = target->data;

	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *msg = &msgs[i];
		bool stop = msg->flags & I2C_MSG_STOP;
		bool read = msg->flags & I2C_MSG_READ;

		for (int j = 0; j < msg->len; j++) {
			uint8_t *b = &msg->buf[j];

			switch (data->state) {
			case IDLE:
				/* Starting a new read/write. */
				if (read) {
					LOG_ERR("Unexpected read");
					return -EIO;
				}

				if (stop) {
					LOG_ERR("Unexpected stop");
					goto err;
				}

				if (*b >= BUFFER_SIZE - 1) {
					LOG_ERR("Invalid register %02x", *b);
					return -EIO;
				}

				data->index = *b;
				data->remaining = 0;
				data->pec = crc8_ccitt(0, &addr8, 1);
				data->pec = crc8_ccitt(data->pec, b, 1);
				data->state = WAIT_LEN;
				data->read = false;
				break;

			case WAIT_LEN:
				if (stop) {
					LOG_ERR("Unexpected stop");
					goto err;
				}

				/* Waiting to read/write the access length. */
				if (read) {
					*b = BUFFER_SIZE - data->index;
					/*
					 * If we're restarting then the 8-bit
					 * address needs to be included in the
					 * PEC calculation.
					 */
					data->pec = crc8_ccitt(data->pec,
							       &addr8_read, 1);
					data->read = true;
				}

				data->remaining = *b;
				data->pec = crc8_ccitt(data->pec, b, 1);
				data->state = ACCESS;
				break;

			case ACCESS:
				if (data->read != read) {
					LOG_ERR("Unexpected restart");
					goto err;
				}

				if (stop) {
					LOG_ERR("Unexpected stop");
					goto err;
				}

				if (data->index >= BUFFER_SIZE) {
					LOG_ERR("Access overflow");
					goto err;
				}

				/* On a stop the last byte should be the PEC. */
				if (read) {
					*b = data->buf[data->index];
				} else {
					data->buf[data->index] = *b;
				}

				data->pec = crc8_ccitt(data->pec, b, 1);
				data->index++;
				data->remaining--;

				if (data->remaining == 0) {
					data->state = PEC;
				}
				break;

			case PEC:
				if (data->read != read) {
					LOG_ERR("Unexpected restart");
					goto err;
				}

				if (!stop) {
					LOG_ERR("Excepted PEC in stop message");
					goto err;
				}

				/* Flip a PEC bit to simulate corruption. */
				if (data->corrupt_pec) {
					data->pec = (data->pec & 0xfe)
						    | (~data->pec & 0xfe);
				}

				if (read) {
					*b = data->pec;
				} else if (data->pec != *b) {
					LOG_ERR("PEC got %02x, expected %02x",
						*b, data->pec);
					/*
					 * The host sees a NACK for a PEC
					 * mismatch so we don't need to return
					 * -EGAIN.
					 */
					goto err;
				}

				data->state = IDLE;
				break;
			}
		}
	}

	return 0;

err:
	data->state = IDLE;
	return -EIO;
}

void i2c_pec_test_emul_set_corrupt(const struct emul *target, bool value)
{
	struct emul_i2c_pec_test_data *data = target->data;

	data->corrupt_pec = value;
}

bool i2c_pec_test_emul_get_corrupt(const struct emul *target)
{
	struct emul_i2c_pec_test_data *data = target->data;

	return data->corrupt_pec;
}

bool i2c_pec_test_emul_is_idle(const struct emul *target)
{
	struct emul_i2c_pec_test_data *data = target->data;

	return data->state == IDLE;
}

uint8_t i2c_pec_test_emul_get_last_pec(const struct emul *target)
{
	struct emul_i2c_pec_test_data *data = target->data;

	return data->pec;
}

void i2c_pec_test_euml_reset(const struct emul *target)
{
	struct emul_i2c_pec_test_data *data = target->data;

	memset(data->buf, 0, BUFFER_SIZE);
	data->corrupt_pec = false;
	data->state = IDLE;
}

static int i2c_pec_test_emul_init(const struct emul *target,
				  const struct device *parent)
{
	ARG_UNUSED(parent);
	ARG_UNUSED(target);

	return 0;
}

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

#define EMUL_RESET_RULE_AFTER(n) \
	i2c_pec_test_euml_reset(EMUL_DT_GET(DT_DRV_INST(n)));

static void i2c_pec_test_emul_test_reset(const struct ztest_unit_test *test,
				   void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(EMUL_RESET_RULE_AFTER)
}

ZTEST_RULE(emul_test_reset, NULL, i2c_pec_test_emul_test_reset);
#endif

static const struct i2c_emul_api emul_i2c_pec_test_api = {
	.transfer = i2c_pec_test_emul_transfer,
};

#define EMUL_I2C_PEC_TEST(n)						  \
	static struct emul_i2c_pec_test_cfg emul_i2c_pec_test_cfg_##n = { \
		.addr = DT_INST_REG_ADDR(n),				  \
	};								  \
	static struct emul_i2c_pec_test_data emul_i2c_pec_test_data_##n;  \
	EMUL_DT_INST_DEFINE(n, i2c_pec_test_emul_init,			  \
			    &emul_i2c_pec_test_data_##n,		  \
			    &emul_i2c_pec_test_cfg_##n,			  \
			    &emul_i2c_pec_test_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(EMUL_I2C_PEC_TEST)
DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)
