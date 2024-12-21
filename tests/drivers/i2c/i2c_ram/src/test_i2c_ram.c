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
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#define RAM_ADDR (0b10100010 >> 1)

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(i2c_ram))
#define I2C_DEV_NODE	DT_ALIAS(i2c_ram)
#define TX_DATA_OFFSET 2
static uint8_t tx_data[9] = {0x00, 0x00, 'Z', 'e', 'p', 'h', 'y', 'r', '\n'};
static uint8_t rx_cmd[2] = {0x00, 0x00};
#else
#error "Please set the correct I2C device and alias for i2c_ram to be status okay"
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

static void *i2c_ram_setup(void)
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


static uint16_t addr;

static void i2c_ram_before(void *f)
{
	tx_data[0] = (addr >> 8) & 0xFF;
	tx_data[1] = (addr) & 0xFF;
	rx_cmd[0] = (addr >> 8) & 0xFF;
	rx_cmd[1] = (addr) & 0xFF;
	addr += ARRAY_SIZE(tx_data) - TX_DATA_OFFSET;
	memset(rx_data, 0, ARRAY_SIZE(rx_data));

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_get(i2c_dev);
#endif
}

static void i2c_ram_after(void *f)
{
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_put(i2c_dev);
#endif
}

ZTEST(i2c_ram, test_ram_transfer)
{
	TC_PRINT("ram using i2c_transfer from thread %p addr %x\n", k_current_get(), addr);

	msgs[0].buf = tx_data;
	msgs[0].len = ARRAY_SIZE(tx_data);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	zassert_ok(i2c_transfer(i2c_dev, msgs, 1, RAM_ADDR),
		   "I2C write to fram failed");

	/* Write the address and read the data back */
	msgs[0].buf = rx_cmd;
	msgs[0].len = ARRAY_SIZE(rx_cmd);
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = 7;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	zassert_ok(i2c_transfer(i2c_dev, msgs, 2, RAM_ADDR),
		   "I2C read from fram failed");

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}

ZTEST(i2c_ram, test_ram_write_read)
{
	TC_PRINT("ram using i2c_write and i2c_write_read from thread %p addr %x\n",
		 k_current_get(), addr);

	zassert_ok(i2c_write(i2c_dev, tx_data, ARRAY_SIZE(tx_data), RAM_ADDR),
		   "I2C write to fram failed");

	zassert_ok(i2c_write_read(i2c_dev, RAM_ADDR, rx_cmd, ARRAY_SIZE(rx_cmd),
				  rx_data, ARRAY_SIZE(rx_data)),
		   "I2C read from fram failed");

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}


#ifdef CONFIG_I2C_CALLBACK
K_SEM_DEFINE(transfer_sem, 0, 1);

static void i2c_ram_transfer_cb(const struct device *dev, int result, void *data)
{
	struct k_sem *s = data;

	k_sem_give(s);
}

ZTEST(i2c_ram, test_ram_transfer_cb)
{
	msgs[0].buf = tx_data;
	msgs[0].len = ARRAY_SIZE(tx_data);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	zassert_ok(i2c_transfer_cb(i2c_dev, msgs, 1, RAM_ADDR,
				   i2c_ram_transfer_cb,
				   &transfer_sem), "I2C write to fram failed");

	k_sem_take(&transfer_sem, K_FOREVER);

	/* Write the address and read the data back */
	msgs[0].buf = rx_cmd;
	msgs[0].len = ARRAY_SIZE(rx_cmd);
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = ARRAY_SIZE(rx_data);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	zassert_ok(i2c_transfer_cb(i2c_dev, msgs, 2, RAM_ADDR,
			   i2c_ram_transfer_cb, &transfer_sem),
		   "I2C read from fram failed");

	k_sem_take(&transfer_sem, K_FOREVER);

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");

}
#endif /* CONFIG_I2C_CALLBACK */

#ifdef CONFIG_I2C_RTIO
#include <zephyr/rtio/rtio.h>

I2C_IODEV_DEFINE(i2c_iodev, I2C_DEV_NODE, RAM_ADDR);
RTIO_DEFINE(i2c_rtio, 2, 2);

ZTEST(i2c_ram, test_ram_rtio)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *wr_cqe, *rd_cqe;

	TC_PRINT("submitting write from thread %p addr %x\n", k_current_get(), addr);
	wr_sqe = rtio_sqe_acquire(&i2c_rtio);
	rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, tx_data, ARRAY_SIZE(tx_data), tx_data);
	wr_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	zassert_ok(rtio_submit(&i2c_rtio, 1), "submit should succeed");

	wr_cqe = rtio_cqe_consume(&i2c_rtio);
	zassert_ok(wr_cqe->result, "i2c write should succeed");
	rtio_cqe_release(&i2c_rtio, wr_cqe);

	/* Write the address and read the data back */
	msgs[0].len = ARRAY_SIZE(rx_cmd);
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_data;
	msgs[1].len = ARRAY_SIZE(rx_data);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	wr_sqe = rtio_sqe_acquire(&i2c_rtio);
	rd_sqe = rtio_sqe_acquire(&i2c_rtio);
	rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, rx_cmd, ARRAY_SIZE(rx_cmd), rx_cmd);
	rtio_sqe_prep_read(rd_sqe, &i2c_iodev, 0, rx_data, ARRAY_SIZE(rx_data), rx_data);
	wr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rd_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	zassert_ok(rtio_submit(&i2c_rtio, 2), "submit should succeed");

	wr_cqe = rtio_cqe_consume(&i2c_rtio);
	rd_cqe = rtio_cqe_consume(&i2c_rtio);
	zassert_ok(wr_cqe->result, "i2c write should succeed");
	zassert_ok(rd_cqe->result, "i2c read should succeed");
	rtio_cqe_release(&i2c_rtio, wr_cqe);
	rtio_cqe_release(&i2c_rtio, rd_cqe);

	zassert_equal(memcmp(&tx_data[TX_DATA_OFFSET], &rx_data[0], ARRAY_SIZE(rx_data)), 0,
		      "Written and Read data should match");
}

static enum isr_rtio_state {
	INIT,
	WRITE_WAIT,
	READ_CMD_WAIT,
	READ_DATA_WAIT,
	DONE
} isr_state = INIT;

K_SEM_DEFINE(ram_rtio_isr_sem, 0, 1);

void ram_rtio_isr(struct k_timer *tid)
{
	struct rtio_sqe *wr_sqe, *rd_sqe;
	struct rtio_cqe *wr_cqe, *rd_cqe;

	switch (isr_state) {
	case INIT:
		TC_PRINT("timer submitting write, addr %x\n", addr);
		wr_sqe = rtio_sqe_acquire(&i2c_rtio);
		rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, tx_data, ARRAY_SIZE(tx_data), tx_data);
		wr_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
		zassert_ok(rtio_submit(&i2c_rtio, 0), "submit should succeed");
		isr_state += 1;
		break;
	case WRITE_WAIT:
		wr_cqe = rtio_cqe_consume(&i2c_rtio);
		if (wr_cqe) {
			TC_PRINT("timer checking write result, submitting read\n");
			zassert_ok(wr_cqe->result, "i2c write should succeed");
			rtio_cqe_release(&i2c_rtio, wr_cqe);

			/* Write the address and read the data back */
			msgs[0].len = ARRAY_SIZE(rx_cmd);
			msgs[0].flags = I2C_MSG_WRITE;
			msgs[1].buf = rx_data;
			msgs[1].len = ARRAY_SIZE(rx_data);
			msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

			wr_sqe = rtio_sqe_acquire(&i2c_rtio);
			rd_sqe = rtio_sqe_acquire(&i2c_rtio);
			rtio_sqe_prep_write(wr_sqe, &i2c_iodev, 0, rx_cmd,
					    ARRAY_SIZE(rx_cmd), rx_cmd);
			rtio_sqe_prep_read(rd_sqe, &i2c_iodev, 0, rx_data,
					   ARRAY_SIZE(rx_data), rx_data);
			wr_sqe->flags |= RTIO_SQE_TRANSACTION;
			rd_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
			zassert_ok(rtio_submit(&i2c_rtio, 0), "submit should succeed");
			isr_state += 1;
		}
		break;
	case READ_CMD_WAIT:
		wr_cqe = rtio_cqe_consume(&i2c_rtio);
		if (wr_cqe) {
			TC_PRINT("read command complete\n");
			zassert_ok(wr_cqe->result, "i2c read command should succeed");
			rtio_cqe_release(&i2c_rtio, wr_cqe);
			isr_state += 1;
		}
		break;
	case READ_DATA_WAIT:
		rd_cqe = rtio_cqe_consume(&i2c_rtio);
		if (rd_cqe) {
			TC_PRINT("read data complete\n");
			zassert_ok(rd_cqe->result, "i2c read data should succeed");
			rtio_cqe_release(&i2c_rtio, rd_cqe);
			isr_state += 1;
			k_sem_give(&ram_rtio_isr_sem);
			k_timer_stop(tid);
		}
		break;
	default:

		zassert_ok(-1, "Should not get here");
	}
}

K_TIMER_DEFINE(ram_rtio_isr_timer, ram_rtio_isr, NULL);


ZTEST(i2c_ram, test_ram_rtio_isr)
{
	k_timer_start(&ram_rtio_isr_timer, K_MSEC(1), K_MSEC(1));
	k_sem_take(&ram_rtio_isr_sem, K_FOREVER);
}

#endif /* CONFIG_I2C_RTIO */

ZTEST_SUITE(i2c_ram, NULL, i2c_ram_setup, i2c_ram_before, i2c_ram_after, NULL);
