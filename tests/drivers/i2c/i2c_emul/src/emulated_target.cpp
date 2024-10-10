/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emulated_target.hpp"

DEFINE_FFF_GLOBALS;

#define DEFINE_FAKE_TARGET_FUNCTION(node_id, prop, n)                                              \
	DEFINE_FAKE_VALUE_FUNC(int, target_read_requested_##n, struct i2c_target_config *,         \
			       uint8_t *);                                                         \
	DEFINE_FAKE_VALUE_FUNC(int, target_read_processed_##n, struct i2c_target_config *,         \
			       uint8_t *);                                                         \
	DEFINE_FAKE_VALUE_FUNC(int, target_write_requested_##n, struct i2c_target_config *);       \
	DEFINE_FAKE_VALUE_FUNC(int, target_write_received_##n, struct i2c_target_config *,         \
			       uint8_t);                                                           \
	DEFINE_FAKE_VALUE_FUNC(int, target_stop_##n, struct i2c_target_config *);                  \
	DEFINE_FAKE_VALUE_FUNC(int, target_buf_read_requested_##n, struct i2c_target_config *,     \
				uint8_t **, uint32_t *)                                            \
	DEFINE_FAKE_VOID_FUNC(target_buf_write_received_##n, struct i2c_target_config *,           \
			       uint8_t *, uint32_t)

DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_FAKE_TARGET_FUNCTION);

/* clang-format off */
#define DEFINE_EMULATED_CALLBACK(node_id, prop, n)                                                 \
	{                                                                                          \
		.write_requested = target_write_requested_##n,                                     \
		.read_requested = target_read_requested_##n,                                       \
		.write_received = target_write_received_##n,                                       \
		.read_processed = target_read_processed_##n,                                       \
		COND_CODE_1(CONFIG_I2C_TARGET_BUFFER_MODE,                                         \
			    (.buf_write_received = target_buf_write_received_##n, ), ())           \
		COND_CODE_1(CONFIG_I2C_TARGET_BUFFER_MODE,                                         \
				(.buf_read_requested = target_buf_read_requested_##n, ), ())       \
		.stop = target_stop_##n,                                                           \
	},
/* clang-format on */

struct i2c_target_callbacks emulated_callbacks[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_EMULATED_CALLBACK)};

#define DEFINE_EMULATED_TARGET_CONFIG(node_id, prop, n)                                            \
	{                                                                                          \
		.flags = 0,                                                                        \
		.address = DT_PHA_BY_IDX(node_id, prop, n, addr),                                  \
		.callbacks = &emulated_callbacks[n],                                               \
	},

struct i2c_target_config emulated_target_config[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_EMULATED_TARGET_CONFIG)};
