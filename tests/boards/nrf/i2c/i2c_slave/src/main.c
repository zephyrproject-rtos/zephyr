/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/i2c.h>
#include <nrfx_twis.h>

#include <zephyr/ztest.h>

#if CONFIG_NRFX_TWIS1
#define I2C_S_INSTANCE 1
#elif CONFIG_NRFX_TWIS2
#define I2C_S_INSTANCE 2
#elif CONFIG_NRFX_TWIS22
#define I2C_S_INSTANCE 22
#elif CONFIG_NRFX_TWIS131
#define I2C_S_INSTANCE 131
#else
#error "TWIS instance not enabled or not supported"
#endif

#define NODE_SENSOR DT_NODELABEL(sensor)
#define NODE_TWIS   DT_ALIAS(i2c_slave)

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define TEST_DATA_SIZE 6
static const uint8_t msg[TEST_DATA_SIZE] = "Nordic";
static const nrfx_twis_t twis = NRFX_TWIS_INSTANCE(I2C_S_INSTANCE);

static uint8_t i2c_slave_buffer[TEST_DATA_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[TEST_DATA_SIZE];
struct i2c_api_twis_fixture {
	const struct device *dev;
	uint8_t addr;
	uint8_t *const master_buffer;
	uint8_t *const slave_buffer;
};

void i2s_slave_handler(nrfx_twis_evt_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		TC_PRINT("TWIS event: read request\n");
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		TC_PRINT("TWIS event: read done\n");
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		TC_PRINT("TWIS event: write request\n");
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		zassert_mem_equal(i2c_slave_buffer, msg, TEST_DATA_SIZE);
		TC_PRINT("TWIS event: write done\n");
		break;
	default:
		TC_PRINT("TWIS event: %d\n", p_event->type);
		break;
	}
}

static void *test_setup(void)
{
	static struct i2c_api_twis_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_BUS(NODE_SENSOR)),
		.addr = DT_REG_ADDR(NODE_SENSOR),
		.master_buffer = i2c_master_buffer,
		.slave_buffer = i2c_slave_buffer,
	};
	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	int ret;

	zassert_equal(NRFX_SUCCESS, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority),
		    NRFX_TWIS_INST_HANDLER_GET(I2C_S_INSTANCE), NULL, 0);

	nrfx_twis_enable(&twis);

	return &fixture;
}

static void cleanup_buffers(void *argc)
{
	struct i2c_api_twis_fixture *fixture = (struct i2c_api_twis_fixture *)argc;

	memset(fixture->slave_buffer, 0, TEST_DATA_SIZE);
	memset(fixture->master_buffer, 0, TEST_DATA_SIZE);
}

ZTEST_USER_F(i2c_api_twis, test_i2c_read_write)
{
	int ret = i2c_write_read(fixture->dev, fixture->addr, msg, TEST_DATA_SIZE,
				 fixture->master_buffer, TEST_DATA_SIZE);

	zassert_ok(ret);
	zassert_mem_equal(fixture->master_buffer, msg, TEST_DATA_SIZE);
}

ZTEST_USER_F(i2c_api_twis, test_i2c_read)
{
	/* Prepare slave data */
	strncpy(fixture->slave_buffer, msg, TEST_DATA_SIZE);
	zassert_mem_equal(fixture->slave_buffer, msg, TEST_DATA_SIZE);

	int ret = i2c_read(fixture->dev, fixture->master_buffer, TEST_DATA_SIZE, fixture->addr);

	zassert_ok(ret);
	zassert_mem_equal(fixture->master_buffer, msg, TEST_DATA_SIZE);
}

ZTEST_USER_F(i2c_api_twis, test_i2c_write)
{
	int ret = i2c_write(fixture->dev, msg, TEST_DATA_SIZE, fixture->addr);

	zassert_ok(ret);
	zassert_mem_equal(fixture->slave_buffer, msg, TEST_DATA_SIZE);
}

ZTEST_SUITE(i2c_api_twis, NULL, test_setup, NULL, cleanup_buffers, NULL);
