/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/regset_target_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_BUF_SIZE 8U
#define TEST_I2C_ADDR 0x42U

struct fake_i2c_data {
	struct i2c_target_config *target_cfg;
	int register_ret;
	int unregister_ret;
};

static struct fake_i2c_data fake_i2c_bus_data;

static int fake_i2c_target_register(const struct device *dev,
				    struct i2c_target_config *cfg)
{
	struct fake_i2c_data *data = dev->data;

	if (data->register_ret != 0) {
		return data->register_ret;
	}

	if (data->target_cfg != NULL) {
		return -EBUSY;
	}

	data->target_cfg = cfg;
	return 0;
}

static int fake_i2c_target_unregister(const struct device *dev,
				      struct i2c_target_config *cfg)
{
	struct fake_i2c_data *data = dev->data;

	if (data->unregister_ret != 0) {
		return data->unregister_ret;
	}

	if (data->target_cfg != cfg) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	return 0;
}

static DEVICE_API(i2c, fake_i2c_api) = {
	.target_register = fake_i2c_target_register,
	.target_unregister = fake_i2c_target_unregister,
};

DEVICE_DEFINE(fake_i2c_bus, "fake_i2c", NULL, NULL,
	      &fake_i2c_bus_data, NULL,
	      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &fake_i2c_api);

struct test_regset_data {
	struct regset_target_lib_data regset_data;
	uint32_t read_cnt;
	uint32_t write_cnt;
	uint32_t changed_cnt;
	off_t last_read_offset;
	off_t last_write_offset;
	bool clear_on_read;
	bool override_read_val;
	uint8_t read_val_override;
	bool write_1_to_clear;
};

struct test_regset_config {
	struct regset_target_lib_config regset_cfg;
};

REGSET_TARGET_LIB_STRUCT_CHECK(struct test_regset_config,
			       struct test_regset_data);

static void test_regset_read_cb(const struct device *dev, off_t offset,
				uint8_t *reg, uint8_t *val)
{
	struct test_regset_data *data = dev->data;

	data->read_cnt++;
	data->last_read_offset = offset;

	if (data->override_read_val) {
		*val = data->read_val_override;
	}

	if (data->clear_on_read) {
		*reg = 0U;
	}
}

static void test_regset_write_cb(const struct device *dev, off_t offset,
				 uint8_t *reg, uint8_t val)
{
	struct test_regset_data *data = dev->data;

	data->write_cnt++;
	data->last_write_offset = offset;

	if (data->write_1_to_clear) {
		*reg &= ~val;
	} else {
		*reg = val;
	}
}

static void test_regset_changed_cb(const struct device *dev)
{
	struct test_regset_data *data = dev->data;

	data->changed_cnt++;
}

static const struct regset_target_lib_api test_regset_api = {
	.read = test_regset_read_cb,
	.write = test_regset_write_cb,
	.changed = test_regset_changed_cb,
};

static uint8_t test_buf_8bit[TEST_BUF_SIZE];
static struct test_regset_data test_data_8bit;
static const struct test_regset_config test_cfg_8bit = {
	.regset_cfg = {
		.api = &test_regset_api,
		.bus = {
			.bus = &DEVICE_NAME_GET(fake_i2c_bus),
			.addr = TEST_I2C_ADDR,
		},
		.buffer_size = TEST_BUF_SIZE,
		.buffer = test_buf_8bit,
		.address_width = 8U,
		.auto_register = true,
	},
};

DEVICE_DEFINE(test_dev_8bit, "test_regset_8bit", regset_target_lib_init, NULL,
	      &test_data_8bit, &test_cfg_8bit,
	      POST_KERNEL, CONFIG_I2C_TARGET_INIT_PRIORITY, &regset_target_api);

static uint8_t test_buf_16bit[TEST_BUF_SIZE];
static struct test_regset_data test_data_16bit;
static const struct test_regset_config test_cfg_16bit = {
	.regset_cfg = {
		.api = &test_regset_api,
		.bus = {
			.bus = &DEVICE_NAME_GET(fake_i2c_bus),
			.addr = TEST_I2C_ADDR,
		},
		.buffer_size = TEST_BUF_SIZE,
		.buffer = test_buf_16bit,
		.address_width = 16U,
		.auto_register = false,
	},
};

DEVICE_DEFINE(test_dev_16bit, "test_regset_16bit", regset_target_lib_init, NULL,
	      &test_data_16bit, &test_cfg_16bit,
	      POST_KERNEL, CONFIG_I2C_TARGET_INIT_PRIORITY, &regset_target_api);

static uint8_t test_buf_no_api[TEST_BUF_SIZE];
static struct test_regset_data test_data_no_api;
static const struct test_regset_config test_cfg_no_api = {
	.regset_cfg = {
		.api = NULL,
		.bus = {
			.bus = &DEVICE_NAME_GET(fake_i2c_bus),
			.addr = TEST_I2C_ADDR,
		},
		.buffer_size = TEST_BUF_SIZE,
		.buffer = test_buf_no_api,
		.address_width = 8U,
		.auto_register = false,
	},
};

DEVICE_DEFINE(test_dev_no_api, "test_regset_no_api", regset_target_lib_init, NULL,
	      &test_data_no_api, &test_cfg_no_api,
	      POST_KERNEL, CONFIG_I2C_TARGET_INIT_PRIORITY, &regset_target_api);

static const struct device *const dev_8bit = &DEVICE_NAME_GET(test_dev_8bit);
static const struct device *const dev_16bit = &DEVICE_NAME_GET(test_dev_16bit);
static const struct device *const dev_no_api = &DEVICE_NAME_GET(test_dev_no_api);

static void reset_test_device(const struct device *dev, uint8_t *buf)
{
	struct test_regset_data *data = dev->data;

	memset(buf, 0, TEST_BUF_SIZE);
	memset(data, 0, sizeof(*data));
	zassert_ok(regset_target_lib_init(dev));
}

static void regset_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&fake_i2c_bus_data, 0, sizeof(fake_i2c_bus_data));
	reset_test_device(dev_16bit, test_buf_16bit);
	reset_test_device(dev_no_api, test_buf_no_api);
	reset_test_device(dev_8bit, test_buf_8bit);
}

ZTEST(regset_target_lib, test_init_and_register)
{
	/* dev_8bit has auto_register = true, so it is registered by regset_before */
	zassert_not_null(fake_i2c_bus_data.target_cfg);
	zassert_equal(fake_i2c_bus_data.target_cfg->address, TEST_I2C_ADDR);

	/* Unregister dev_8bit and register dev_16bit (auto_register = false) */
	zassert_ok(i2c_target_driver_unregister(dev_8bit));
	zassert_is_null(fake_i2c_bus_data.target_cfg);

	zassert_ok(i2c_target_driver_register(dev_16bit));
	zassert_equal(fake_i2c_bus_data.target_cfg, &test_data_16bit.regset_data.config);

	/* Change runtime address */
	zassert_ok(regset_target_lib_set_addr(dev_16bit, 0x55U));
	zassert_equal(fake_i2c_bus_data.target_cfg->address, 0x55U);

	/* Verify set_addr propagates unregister failure */
	fake_i2c_bus_data.unregister_ret = -EIO;
	zassert_equal(regset_target_lib_set_addr(dev_16bit, 0x56U), -EIO);
	fake_i2c_bus_data.unregister_ret = 0;

	/* Verify set_addr propagates register failure */
	fake_i2c_bus_data.register_ret = -ENOSYS;
	zassert_equal(regset_target_lib_set_addr(dev_16bit, 0x56U), -ENOSYS);
	fake_i2c_bus_data.register_ret = 0;
}

ZTEST(regset_target_lib, test_host_read_write_data)
{
	const uint8_t write_pattern[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
	uint8_t read_buf[TEST_BUF_SIZE] = { 0 };

	zassert_equal(regset_target_lib_get_size(dev_8bit), TEST_BUF_SIZE);

	zassert_ok(regset_target_lib_write_data(dev_8bit, 2, write_pattern,
						sizeof(write_pattern)));
	zassert_ok(regset_target_lib_read_data(dev_8bit, 2, read_buf,
					       sizeof(write_pattern)));
	zassert_mem_equal(read_buf, write_pattern, sizeof(write_pattern));

	/* Out-of-bounds read and write must fail with -EINVAL */
	zassert_equal(regset_target_lib_write_data(dev_8bit, TEST_BUF_SIZE - 2,
						   write_pattern, sizeof(write_pattern)),
		      -EINVAL);
	zassert_equal(regset_target_lib_read_data(dev_8bit, TEST_BUF_SIZE - 2,
						  read_buf, sizeof(write_pattern)),
		      -EINVAL);
}

ZTEST(regset_target_lib, test_i2c_pio_8bit_no_api)
{
	const struct i2c_target_callbacks *cb;
	struct i2c_target_config *cfg;
	uint8_t val = 0U;

	zassert_ok(i2c_target_driver_unregister(dev_8bit));
	zassert_ok(i2c_target_driver_register(dev_no_api));

	cfg = fake_i2c_bus_data.target_cfg;
	zassert_not_null(cfg);
	cb = cfg->callbacks;

	/* Write starting at offset 6, wrapping around buffer_size (8) to offset 0 */
	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 6U));
	zassert_ok(cb->write_received(cfg, 0xa1U));
	zassert_ok(cb->write_received(cfg, 0xa2U));
	zassert_ok(cb->write_received(cfg, 0xa3U));
	zassert_ok(cb->stop(cfg));

	zassert_equal(test_buf_no_api[6], 0xa1U);
	zassert_equal(test_buf_no_api[7], 0xa2U);
	zassert_equal(test_buf_no_api[0], 0xa3U);

	/* Set read offset to 6 and read 3 bytes across wrap-around boundary */
	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 6U));
	zassert_ok(cb->read_requested(cfg, &val));
	zassert_equal(val, 0xa1U);
	zassert_ok(cb->read_processed(cfg, &val));
	zassert_equal(val, 0xa2U);
	zassert_ok(cb->read_processed(cfg, &val));
	zassert_equal(val, 0xa3U);
	zassert_ok(cb->stop(cfg));
}

ZTEST(regset_target_lib, test_i2c_pio_16bit)
{
	const struct i2c_target_callbacks *cb;
	struct i2c_target_config *cfg;
	uint8_t val = 0U;

	zassert_ok(i2c_target_driver_unregister(dev_8bit));
	zassert_ok(i2c_target_driver_register(dev_16bit));

	cfg = fake_i2c_bus_data.target_cfg;
	zassert_not_null(cfg);
	cb = cfg->callbacks;

	/* Address-only write (0x0003) should not trigger changed callback */
	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 0x00U));
	zassert_ok(cb->write_received(cfg, 0x03U));
	zassert_ok(cb->stop(cfg));
	zassert_equal(test_data_16bit.changed_cnt, 0U);

	/* Write 2 bytes at 16-bit offset 0x0003 */
	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 0x00U));
	zassert_ok(cb->write_received(cfg, 0x03U));
	zassert_ok(cb->write_received(cfg, 0x5aU));
	zassert_ok(cb->write_received(cfg, 0x6bU));
	zassert_ok(cb->stop(cfg));

	zassert_equal(test_data_16bit.changed_cnt, 1U);
	zassert_equal(test_buf_16bit[3], 0x5aU);
	zassert_equal(test_buf_16bit[4], 0x6bU);

	/* Read back from 16-bit offset 0x0003 */
	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 0x00U));
	zassert_ok(cb->write_received(cfg, 0x03U));
	zassert_ok(cb->read_requested(cfg, &val));
	zassert_equal(val, 0x5aU);
	zassert_ok(cb->read_processed(cfg, &val));
	zassert_equal(val, 0x6bU);
	zassert_ok(cb->stop(cfg));
}

ZTEST(regset_target_lib, test_api_callbacks)
{
	struct i2c_target_config *cfg = fake_i2c_bus_data.target_cfg;
	const struct i2c_target_callbacks *cb = cfg->callbacks;
	uint8_t val = 0U;

	test_buf_8bit[1] = 0x45U;
	test_buf_8bit[2] = 0x99U;
	test_buf_8bit[3] = 0xffU;

	/*
	 * 1. On-the-fly read value modification:
	 * callback overrides *val while leaving backing buffer entry unchanged.
	 */
	test_data_8bit.override_read_val = true;
	test_data_8bit.read_val_override = 0xdeU;

	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 1U));
	zassert_ok(cb->read_requested(cfg, &val));
	zassert_equal(val, 0xdeU);
	zassert_equal(test_buf_8bit[1], 0x45U);
	zassert_equal(test_data_8bit.read_cnt, 1U);
	zassert_equal(test_data_8bit.last_read_offset, 1);

	/*
	 * 2. Clear-on-read via read_processed:
	 * callback leaves *val as the pre-fetched buffer byte (0x99) and clears *reg.
	 */
	test_data_8bit.override_read_val = false;
	test_data_8bit.clear_on_read = true;

	zassert_ok(cb->read_processed(cfg, &val));
	zassert_equal(val, 0x99U);
	zassert_equal(test_buf_8bit[2], 0x00U);
	zassert_equal(test_data_8bit.read_cnt, 2U);
	zassert_equal(test_data_8bit.last_read_offset, 2);
	zassert_ok(cb->stop(cfg));

	/*
	 * 3. Custom write behavior (write-1-to-clear):
	 * writing 0x0f to 0xff clears the lower nibble to 0xf0.
	 */
	test_data_8bit.write_1_to_clear = true;

	zassert_ok(cb->write_requested(cfg));
	zassert_ok(cb->write_received(cfg, 3U));
	zassert_ok(cb->write_received(cfg, 0x0fU));
	zassert_ok(cb->stop(cfg));

	zassert_equal(test_buf_8bit[3], 0xf0U);
	zassert_equal(test_data_8bit.write_cnt, 1U);
	zassert_equal(test_data_8bit.last_write_offset, 3);
	zassert_equal(test_data_8bit.changed_cnt, 1U);
}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
ZTEST(regset_target_lib, test_i2c_buf_mode)
{
	struct i2c_target_config *cfg = fake_i2c_bus_data.target_cfg;
	const struct i2c_target_callbacks *cb = cfg->callbacks;
	uint8_t write_frame[] = { 6U, 0x10U, 0x20U, 0x30U };
	uint8_t addr_only_frame[] = { 6U };
	uint8_t *read_ptr = NULL;
	uint32_t read_len = 0U;

	/* Buffered write with custom write callback and wrap-around from 6 -> 7 -> 0 */
	cb->buf_write_received(cfg, write_frame, sizeof(write_frame));
	zassert_ok(cb->stop(cfg));

	zassert_equal(test_data_8bit.write_cnt, 3U);
	zassert_equal(test_data_8bit.changed_cnt, 1U);
	zassert_equal(test_buf_8bit[6], 0x10U);
	zassert_equal(test_buf_8bit[7], 0x20U);
	zassert_equal(test_buf_8bit[0], 0x30U);

	/* Set offset via address-only buffered write, then request read buffer */
	cb->buf_write_received(cfg, addr_only_frame, sizeof(addr_only_frame));
	zassert_ok(cb->buf_read_requested(cfg, &read_ptr, &read_len));
	zassert_equal_ptr(read_ptr, &test_buf_8bit[6]);
	zassert_equal(read_len, TEST_BUF_SIZE);
	zassert_ok(cb->stop(cfg));
	zassert_equal(test_data_8bit.changed_cnt, 1U);

	/* Buffered write with no api callbacks (default memcpy path) */
	zassert_ok(i2c_target_driver_unregister(dev_8bit));
	zassert_ok(i2c_target_driver_register(dev_no_api));
	cfg = fake_i2c_bus_data.target_cfg;
	cb = cfg->callbacks;

	uint8_t memcpy_frame[] = { 1U, 0xb1U, 0xb2U };

	cb->buf_write_received(cfg, memcpy_frame, sizeof(memcpy_frame));
	zassert_ok(cb->stop(cfg));
	zassert_equal(test_buf_no_api[1], 0xb1U);
	zassert_equal(test_buf_no_api[2], 0xb2U);
}
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

ZTEST_SUITE(regset_target_lib, NULL, NULL, regset_before, NULL, NULL);
