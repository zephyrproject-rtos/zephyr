/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <nrfx_twis.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>

#define NODE_TWIM DT_NODELABEL(dut_twim)
#define NODE_TWIS DT_NODELABEL(dut_twis)

#define TWIM_DEV_CONFIG (I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER)

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define I2C_DEVICE_ADDR DT_REG_ADDR(DT_NODELABEL(sensor))
#define TEST_DATA_SIZE  8

/*
 * TWIM - RTIO driver
 * TWIS - standard TWIS driver
 */

RTIO_DEFINE(twim_rtio_context, 4, 2);

I2C_IODEV_DEFINE(twim_rtio_io_dev, NODE_TWIM, I2C_DEVICE_ADDR);

static nrfx_twis_t twis = {.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)};

static uint8_t i2c_slave_buffer[TEST_DATA_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[TEST_DATA_SIZE];

struct i2c_api_twis_fixture {
	uint8_t addr;
	uint8_t *const master_buffer;
	uint8_t *const slave_buffer;
};

static struct i2c_api_twis_fixture fixture = {
	.addr = I2C_DEVICE_ADDR,
	.master_buffer = i2c_master_buffer,
	.slave_buffer = i2c_slave_buffer,
};

static void i2s_slave_handler(nrfx_twis_event_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		break;
	default:
		break;
	}
}

static void cleanup_buffers(void *nullp)
{
	memset(fixture.slave_buffer, 0x00, TEST_DATA_SIZE);
	memset(fixture.master_buffer, 0xFF, TEST_DATA_SIZE);
}

static void prepare_test_data(uint8_t *buffer, size_t buffer_size)
{
	for (size_t counter = 0; counter < buffer_size; counter++) {
		buffer[counter] = sys_rand8_get();
	}
}

static void validate_buffers(void)
{
	for (unsigned int i = 0; i < TEST_DATA_SIZE; i++) {
		if (fixture.master_buffer[i] != fixture.slave_buffer[i]) {
			zexpect_equal(fixture.master_buffer[i], fixture.slave_buffer[i],
				      "master_buffer[%u] (0x%x) != slave_buffer[%u] (0x%x)\n", i,
				      fixture.master_buffer[i], i, fixture.slave_buffer[i]);
		}
	}
}

static void twim_rtio_configure(void)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_i2c_configure(sqe, &twim_rtio_io_dev, 0, TWIM_DEV_CONFIG, NULL);

	rtio_submit(&twim_rtio_context, 1);

	cqe = rtio_cqe_consume_block(&twim_rtio_context);
	zassert_ok(cqe->result, "[RTIO CONFIG] rtio_cqe_consume failed: %d\n", cqe->result);
	rtio_cqe_release(&twim_rtio_context, cqe);
}

static void twim_rtio_write(void)
{
	int ret;
	struct rtio_sqe *wr_sqe;
	struct rtio_cqe *wr_cqe;

	TC_PRINT("TWIM RTIO WRITE\n");

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, 0, fixture.master_buffer, TEST_DATA_SIZE,
			    NULL);

	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	ret = rtio_submit(&twim_rtio_context, 1);

	zassert_ok(ret, "[RTIO WRITE] rtio_submit failed: %d\n", ret);

	wr_cqe = rtio_cqe_consume_block(&twim_rtio_context);

	zassert_ok(ret, "[RTIO WRITE] rtio_cqe_consume_block failed: %d\n", wr_cqe->result);

	rtio_cqe_release(&twim_rtio_context, wr_cqe);
}

static void twim_rtio_read(void)
{
	int ret;
	struct rtio_sqe *rd_sqe;
	struct rtio_cqe *rd_cqe;

	TC_PRINT("TWIM RTIO READ\n");

	rd_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_read(rd_sqe, &twim_rtio_io_dev, 0, fixture.master_buffer, TEST_DATA_SIZE,
			   NULL);

	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	ret = rtio_submit(&twim_rtio_context, 1);

	zassert_ok(ret, "[RTIO READ] rtio_submit failed: %d\n", ret);

	rd_cqe = rtio_cqe_consume_block(&twim_rtio_context);

	zassert_ok(ret, "[RTIO READ] rtio_cqe_consume_block failed: %d\n", rd_cqe->result);

	rtio_cqe_release(&twim_rtio_context, rd_cqe);
}

static void *test_setup(void)
{
	int ret;

	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	zassert_equal(0, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);

	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority), nrfx_twis_irq_handler, &twis,
		    0);

	nrfx_twis_enable(&twis);
	TC_PRINT("TWIS CFG done\n");

	twim_rtio_configure();
	TC_PRINT("TWIM RTIO CFG done\n");

	return NULL;
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_write)
{
	TC_PRINT("TWIM RTIO write test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);
	twim_rtio_write();
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_read)
{
	TC_PRINT("TWIM RTIO write test\n");

	prepare_test_data(fixture.slave_buffer, TEST_DATA_SIZE);
	twim_rtio_read();
	validate_buffers();
}

/*
 * Test case is only usable for devices that use a bounce buffer
 * for example NRF54H20, but can be run on any device
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_rx_buffers_handling)
{
	int ret;
	struct rtio_sqe *wr_sqe;
	struct rtio_cqe *wr_cqe;

	static uint8_t i2c_aux_buffer[TEST_DATA_SIZE];

	TC_PRINT("TWIM RTIO RX buffers handling test\n");

	memset(fixture.slave_buffer, 0xDB, TEST_DATA_SIZE);
	twim_rtio_read();

	memset(fixture.master_buffer, 0x00, TEST_DATA_SIZE);
	memset(i2c_aux_buffer, 0xCD, TEST_DATA_SIZE);

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, 0, i2c_aux_buffer, TEST_DATA_SIZE, NULL);

	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	ret = rtio_submit(&twim_rtio_context, 1);

	zassert_ok(ret, "[RTIO WRITE] rtio_submit failed: %d\n", ret);

	wr_cqe = rtio_cqe_consume_block(&twim_rtio_context);

	zassert_ok(ret, "[RTIO WRITE] rtio_cqe_consume_block failed: %d\n", wr_cqe->result);

	rtio_cqe_release(&twim_rtio_context, wr_cqe);

	TC_PRINT("TWIM RTIO SEPARATE BUFFERS - done\n");

	zexpect_mem_equal(i2c_aux_buffer, fixture.slave_buffer, TEST_DATA_SIZE,
			  "Read data doe not match written data\n");

	zexpect_true(
		memcmp(i2c_aux_buffer, fixture.master_buffer, TEST_DATA_SIZE) != 0,
		"Buffer from the previous RTIO operation not used but modified by the driver anyway\n");
}

ZTEST_SUITE(twim_rtio_twis_loopback, NULL, test_setup, cleanup_buffers, cleanup_buffers, NULL);
