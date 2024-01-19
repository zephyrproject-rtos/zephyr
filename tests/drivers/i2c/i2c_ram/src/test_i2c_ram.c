/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_i2c_basic
 * @{
 * @defgroup t_i2c_read_write test_i2c_read_write
 * @brief TestPurpose: verify I2C master can read and write
 * @}
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define FRAM_ADDR (0b10100000 >> 1)

#if DT_NODE_HAS_STATUS(DT_ALIAS(mb85), okay)
#define I2C_DEV_NODE	DT_ALIAS(mb85)
#define TX_DATA_OFFSET 2
const uint8_t tx_data[9] = {0x00, 0x00, 'Z', 'e', 'p', 'h', 'y', 'r', '\n'};
const uint8_t rx_cmd[2] = {0x00, 0x00};
#elif DT_NODE_HAS_STATUS(DT_ALIAS(mb94), okay)
#define I2C_DEV_NODE	DT_ALIAS(mb94)
#define TX_DATA_OFFSET 3
#define FRAM_READ_CMD 0x4D
#define FRAM_WRITE_CMD 0x47
const uint8_t tx_data[10] = {FRAM_WRITE_CMD, 0x00, 7, 'Z', 'e', 'p', 'h', 'y', 'r', '\n'};
const uint8_t rx_cmd[3] = {FRAM_READ_CMD, 0x00, 7};
#else
#error "Please set the correct I2C device and alias for mb85 or mb94 to be status okay"
#endif

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
struct i2c_msg msgs[2];
uint8_t rx_data[7];

const struct device *i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);

/* Address from datasheet is 0b1010xxxr where x bits are additional
 * memory address bits and r is the r/w i2c bit.
 *
 * However... the address needs to be shifted into the lower 7 bits as
 * Zephyr expects a 7bit device address and shifts this left to set the
 * i2c r/w bit.
 */

static void *i2c_fram_setup(void)
{
	int ret;
	uint32_t i2c_cfg_tmp;

	zassert_true(device_is_ready(i2c_dev), "I2C device is not read");

	/* 1. Verify i2c_configure() */
	zassert_ok(i2c_configure(i2c_dev, i2c_cfg), "I2C config failed");

	/* 2. Verify i2c_get_config(), optional API */
	ret = i2c_get_config(i2c_dev, &i2c_cfg_tmp);
	if (ret != -ENOSYS) {
		zassert_equal(i2c_cfg, i2c_cfg_tmp,
			      "I2C get_config returned invalid config");
	}

	return NULL;
}

static void i2c_fram_before(void *f)
{
	memset(rx_data, 0, ARRAY_SIZE(rx_data));
}

ZTEST(i2c_fram, test_fram_transfer)
{
	msgs[0].buf = tx_data;
	msgs[0].len = ARRAY_SIZE(tx_data);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	zassert_ok(i2c_transfer(i2c_dev, msgs, 1, FRAM_ADDR),
		   "I2C write to fram failed");

	/* Write the address and read the data back */
	msgs[0].buf = rx_cmd;
	msgs[0].len = ARRAY_SIZE(rx_cmd);
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = 7;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	zassert_ok(i2c_transfer(i2c_dev, msgs, 2, FRAM_ADDR),
		   "I2C read from fram failed");

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}

ZTEST(i2c_fram, test_fram_write_read)
{
	zassert_ok(i2c_write(i2c_dev, tx_data, ARRAY_SIZE(tx_data), FRAM_ADDR),
		   "I2C write to fram failed");

	zassert_ok(i2c_write_read(i2c_dev, FRAM_ADDR, rx_cmd, ARRAY_SIZE(rx_cmd),
				  rx_data, ARRAY_SIZE(rx_data)),
		   "I2C read from fram failed");

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}


#ifdef CONFIG_I2C_CALLBACK
K_SEM_DEFINE(transfer_sem, 0, 1);

static void i2c_fram_transfer_cb(const struct device *dev, int result, void *data)
{
	struct k_sem *s = data;

	k_sem_give(s);
}

ZTEST(i2c_fram, test_fram_transfer_cb)
{
	msgs[0].buf = tx_data;
	msgs[0].len = ARRAY_SIZE(tx_data);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	zassert_ok(i2c_transfer_cb(i2c_dev, msgs, 1, FRAM_ADDR,
				   i2c_fram_transfer_cb,
				   &transfer_sem), "I2C write to fram failed");

	k_sem_take(&transfer_sem, K_FOREVER);

	/* Write the address and read the data back */
	msgs[0].buf = rx_cmd;
	msgs[0].len = ARRAY_SIZE(rx_cmd);
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = ARRAY_SIZE(rx_data);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	zassert_ok(i2c_transfer_cb(i2c_dev, msgs, 2, FRAM_ADDR,
			   i2c_fram_transfer_cb, &transfer_sem),
		   "I2C read from fram failed");

	k_sem_take(&transfer_sem, K_FOREVER);

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");

}
#endif /* CONFIG_I2C_CALLBACK */

#ifdef CONFIG_I2C_RTIO
#include <zephyr/rtio/rtio.h>

I2C_IODEV_DEFINE(i2c_iodev, I2C_DEV_NODE, FRAM_ADDR);
RTIO_DEFINE(i2c_rtio, 2, 2);

ZTEST(i2c_fram, test_fram_rtio)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *wr_cqe, *rd_cqe;

	wr_sqe = rtio_sqe_acquire(&i2c_rtio);
	wr_sqe.iodev_flags |= RTIO_IODEV_SQE_STOP;
	rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, tx_data, ARRAY_SIZE(tx_data), tx_data);
	zassert_ok(rtio_submit(&i2c_rtio, 1), "submit should succeed");

	wr_cqe = rtio_cqe_consume(&i2c_rtio);
	zassert_ok(wr_cqe->result, "i2c write should succeed");
	rtio_cqe_release(&i2c_rtio, wr_cqe);

	/* Write the address and read the data back */
	msgs[0].len = ADDR_LEN;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = ARRAY_SIZE(rx_data);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	wr_sqe = rtio_sqe_acquire(&i2c_rtio);
	wr_sqe.flags |= RTIO_SQE_TRANSACTION;
	rd_sqe = rtio_sqe_acquire(&i2c_rtio);
	rd_cqe.iodev_flags |= RTIO_IODEV_SQE_STOP | RTIO_IODEV_I2C_RESTART;
	rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, tx_data, ADDR_LEN, tx_data);
	rtio_sqe_prep_read(rd_sqe, &i2c_iodev, 0, rx_data, ARRAY_SIZE(rx_data), rx_data);
	zassert_ok(rtio_submit(&i2c_rtio, 2), "submit should succeed");

	wr_cqe = rtio_cqe_acquire(&i2c_rtio);
	rd_cqe = rtio_cqe_acquire(&i2c_rtio);
	zassert_ok(wr_cqe->result, "i2c write should succeed");
	zassert_ok(rd_cqe->result, "i2c read should succeed");
	rtio_cqe_release(&i2c_rtio, wr_cqe);
	rtio_cqe_release(&i2c_rtio, rd_cqe);

	zassert_equal(memcmp(&tx_data[ADDR_LEN], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}
#endif /* CONFIG_I2C_RTIO */

ZTEST_SUITE(i2c_fram, NULL, i2c_fram_setup, i2c_fram_before, NULL, NULL);
