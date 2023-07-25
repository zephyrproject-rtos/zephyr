/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define FXOS8700_I2C_ADDR	0x1d

/* Reduced regmap for FXOS8700 */
#define FXOS8700_REG_STATUS			0x00
#define FXOS8700_REG_F_SETUP			0x09
#define FXOS8700_REG_WHOAMI			0x0d
#define FXOS8700_REG_CTRLREG1			0x2a
#define FXOS8700_REG_CTRLREG2			0x2b
#define FXOS8700_REG_CTRLREG3			0x2c
#define FXOS8700_REG_CTRLREG4			0x2d
#define FXOS8700_REG_CTRLREG5			0x2e

#define WHOAMI_ID_FXOS8700			0xC7

#define FXOS8700_CTRLREG2_RST_MASK		0x40


static const struct device *const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(i2c0));

/**
 * Setup and enable the fxos8700 with its max sample rate and
 * FIFO.
 */
static int fxos8700_fifo_cfg(void)
{
	int res;
	uint8_t data;

	TC_PRINT("Configuring FXOS8700\n");

	/* Signal a reset */
	i2c_reg_write_byte(i2c_bus, FXOS8700_I2C_ADDR,
			   FXOS8700_REG_CTRLREG2, FXOS8700_CTRLREG2_RST_MASK);

	k_busy_wait(USEC_PER_MSEC);

	TC_PRINT("Getting whoami\n");
	res = i2c_reg_read_byte(i2c_bus, FXOS8700_I2C_ADDR,
			      FXOS8700_REG_WHOAMI, &data);
	if (res != 0) {
		TC_PRINT("Could not get WHOAMI value after reset\n");
		return TC_FAIL;
	}

	if (data != WHOAMI_ID_FXOS8700) {
		TC_PRINT("Not an FXOS8700 sensor\n");
		return TC_FAIL;
	}

	/* Enable FIFO mode with a watermark of 16 */
	res = i2c_reg_write_byte(i2c_bus,
			    FXOS8700_I2C_ADDR,
			    FXOS8700_REG_F_SETUP,
			    0x50);

	if (res != 0) {
		TC_PRINT("Failed to setup FIFO\n");
		return TC_FAIL;
	}

	/* Activate the sensor */
	res = i2c_reg_write_byte(i2c_bus,
			    FXOS8700_I2C_ADDR,
			    FXOS8700_REG_CTRLREG1,
			    0x01);

	if (res != 0) {
		TC_PRINT("Failed to activate the sensor\n");
		return TC_FAIL;
	}

	TC_PRINT("Configured FXOS8700\n");

	return TC_PASS;
}

#define FXOS8700_XFERS 10

static uint8_t sample_buf[64];
static uint8_t reg = 0x01;
static struct i2c_msg msgs[2] = { { .buf = &reg, .len = 1, .flags = I2C_MSG_WRITE },
				  { .buf = sample_buf,
				    .len = sizeof(sample_buf),
				    .flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP } };

/* Read 3 axis 14 bit (2 byte) data */

static int test_i2c_fxos8700_sync(void)
{
	int res;

	TC_PRINT("fxos8700 sync test ...\n");
	fxos8700_fifo_cfg();

	for (int i = 0; i < FXOS8700_XFERS; i++) {
		res = i2c_transfer(i2c_bus,
			     msgs,
			     2,
			     FXOS8700_I2C_ADDR);

		zassert_ok(res, "expected xfer success");
	}

	TC_PRINT("fxos8700 async test pass\n");
	return TC_PASS;
}

ZTEST(frdm_k64f_i2c, test_i2c_sync)
{
	zassert_equal(test_i2c_fxos8700_sync(), TC_PASS, "i2c sync test");
}

static uint32_t xfer_count;
static int xfer_res;
static struct k_sem xfer_sem;

static void test_i2c_fxos8700_async_cb(const struct device *dev, int result, void *userdata)
{
	int res;

	if (result != 0) {
		xfer_res = result;
		k_sem_give(&xfer_sem);
		return;
	}

	if (xfer_count >= FXOS8700_XFERS) {
		xfer_res = 0;
		k_sem_give(&xfer_sem);
		return;
	}

	xfer_count++;
	res = i2c_transfer_cb(dev, msgs, 2, FXOS8700_I2C_ADDR,
				 test_i2c_fxos8700_async_cb, NULL);
	zassert_ok(res, "expected ok for async transfer start");
}


static int test_i2c_fxos8700_transfer_cb(void)
{
	int res;

	TC_PRINT("fxos8700 async test ...\n");

	fxos8700_fifo_cfg();

	xfer_count = 0;
	k_sem_init(&xfer_sem, 0, 1);

	res = i2c_transfer_cb(i2c_bus, msgs, 2, FXOS8700_I2C_ADDR,
				 test_i2c_fxos8700_async_cb, NULL);
	zassert_ok(res, "expected ok for async transfer start");

	k_sem_take(&xfer_sem, K_FOREVER);

	zassert_ok(xfer_res, "expected success of xfer");

	TC_PRINT("fxos8700 async test pass\n");
	return TC_PASS;
}

ZTEST(frdm_k64f_i2c, test_i2c_transfer_cb)
{
	zassert_equal(test_i2c_fxos8700_transfer_cb(), TC_PASS, "i2c_transfer_cb");
}



static struct k_poll_signal xfer_signal;

/* Mimic synchronous call with async_sem data and callback */
static int test_i2c_fxos8700_transfer_signal(void)
{
	int res;

	TC_PRINT("fxos8700 i2c_transfer_signal test ...\n");

	uint8_t usample_buf[64];
	uint8_t ureg = 0x01;
	struct i2c_msg umsgs[2] = { { .buf = &ureg, .len = 1, .flags = I2C_MSG_WRITE },
				  { .buf = usample_buf,
				    .len = sizeof(usample_buf),
				    .flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP } };

	for (int i = 0; i < 2; i++) {
		TC_PRINT("umsgs[%d].flags %x\n", i, umsgs[i].flags);
	}

	k_poll_signal_init(&xfer_signal);

	struct k_poll_event events[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &xfer_signal),
	};

	fxos8700_fifo_cfg();

	for (int i = 0; i < FXOS8700_XFERS; i++) {
		res = i2c_transfer_signal(i2c_bus, umsgs, 2, FXOS8700_I2C_ADDR,
					 &xfer_signal);
		TC_PRINT("result of transfer_signal, %d\n", res);

		zassert_ok(res, "expected ok for async transfer start");

		TC_PRINT("polling for completion\n");

		/* Poll signal */
		k_poll(events, 1, K_FOREVER);

		unsigned int signaled;
		int signal_result;

		k_poll_signal_check(&xfer_signal, &signaled, &signal_result);

		TC_PRINT("signaled %d, signal result %d\n", signaled, signal_result);

		zassert_true(signaled > 0, "expected signaled to be non-zero");
		zassert_ok(signal_result, "expected result to be ok\n");

		TC_PRINT("resetting signal\n");
		k_poll_signal_reset(&xfer_signal);
	}

	TC_PRINT("fxos8700 i2c_transfer_signal test pass\n");
	return TC_PASS;
}

ZTEST(frdm_k64f_i2c, test_i2c_transfer_signal)
{
	zassert_equal(test_i2c_fxos8700_transfer_signal(), TC_PASS,
		"i2c_transfer_signal supervisor mode");
}

ZTEST_SUITE(frdm_k64f_i2c, NULL, NULL, NULL, NULL, NULL);
