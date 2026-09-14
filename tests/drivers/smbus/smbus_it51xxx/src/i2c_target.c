/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_target.h"

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(smbus_test_target, LOG_LEVEL_INF);

struct test_target_state {
	struct i2c_target_config cfg;
	const struct device *bus;

	uint8_t write_buf[CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE];
	size_t write_len;

	uint8_t read_buf[CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE];
	size_t read_len;
	size_t read_idx;

	bool registered;
};

static struct test_target_state state;

static int target_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);

	state.write_len = 0;

	return 0;
}

static int target_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);

	if (state.write_len >= CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE) {
		LOG_ERR("write capture buffer full, dropping byte 0x%02x", val);
		return -ENOMEM;
	}

	state.write_buf[state.write_len++] = val;

	return 0;
}

static int target_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);

	state.read_idx = 0;

	if (state.read_len == 0) {
		/* Nothing preloaded: NACK the read so the test sees a real error. */
		LOG_WRN("read requested but nothing preloaded");
		return -EIO;
	}

	*val = state.read_buf[state.read_idx++];

	return 0;
}

static int target_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);

	if (state.read_idx >= state.read_len) {
		/* Host is clocking out more bytes than were preloaded. */
		LOG_WRN("read past preloaded data (idx=%u len=%u)", (unsigned int)state.read_idx,
			(unsigned int)state.read_len);
		*val = 0xFF;
		return 0;
	}

	*val = state.read_buf[state.read_idx++];

	return 0;
}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
static void target_buf_write_received(struct i2c_target_config *config, uint8_t *ptr, uint32_t len)
{
	ARG_UNUSED(config);

	if (len > CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE) {
		LOG_ERR("buf_write_received: len %u exceeds capture buffer (%d), truncating", len,
			CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE);
		len = CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE;
	}

	memcpy(state.write_buf, ptr, len);
	state.write_len = len;
}

static int target_buf_read_requested(struct i2c_target_config *cfg, uint8_t **ptr, uint32_t *len)
{
	ARG_UNUSED(cfg);

	*ptr = state.read_buf;
	*len = (uint32_t)state.read_len;

	return 0;
}
#endif

static int target_stop(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);

	return 0;
}

static const struct i2c_target_callbacks test_target_callbacks = {
	.write_requested = target_write_requested,
	.write_received = target_write_received,
	.read_requested = target_read_requested,
	.read_processed = target_read_processed,
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = target_buf_write_received,
	.buf_read_requested = target_buf_read_requested,
#endif
	.stop = target_stop,
};

int test_target_start(const struct device *bus)
{
	int ret;

	if (!device_is_ready(bus)) {
		LOG_ERR("I2C target bus %s not ready", bus->name);
		return -ENODEV;
	}

	memset(&state, 0, sizeof(state));
	state.bus = bus;
	state.cfg.address = SMBUS_TEST_TARGET_ADDR;
	state.cfg.callbacks = &test_target_callbacks;

	ret = i2c_target_register(bus, &state.cfg);
	if (ret == 0) {
		state.registered = true;
	} else {
		LOG_ERR("i2c_target_register failed: %d", ret);
	}

	return ret;
}

int test_target_stop(void)
{
	int ret;

	if (!state.registered) {
		return 0;
	}

	ret = i2c_target_unregister(state.bus, &state.cfg);
	if (ret == 0) {
		state.registered = false;
	}

	return ret;
}

void test_target_reset(void)
{
	state.write_len = 0;
	state.read_len = 0;
}

size_t test_target_write_len(void)
{
	return state.write_len;
}

const uint8_t *test_target_write_buf(void)
{
	return state.write_buf;
}

void test_target_set_read_data(const uint8_t *data, size_t len)
{
	size_t copy_len = MIN(len, CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE);

	if (data != NULL && copy_len > 0) {
		memcpy(state.read_buf, data, copy_len);
	}

	state.read_len = copy_len;
}
