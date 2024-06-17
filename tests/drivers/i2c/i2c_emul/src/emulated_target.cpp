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
	DEFINE_FAKE_VALUE_FUNC(int, target_stop_##n, struct i2c_target_config *);

DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_FAKE_TARGET_FUNCTION);

#define DEFINE_EMULATED_CALLBACK(node_id, prop, n)                                                 \
	[n] = {                                                                                    \
		.write_requested = target_write_requested_##n,                                     \
		.read_requested = target_read_requested_##n,                                       \
		.write_received = target_write_received_##n,                                       \
		.read_processed = target_read_processed_##n,                                       \
		.stop = target_stop_##n,                                                           \
	},

struct i2c_target_callbacks emulated_callbacks[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_EMULATED_CALLBACK)};

#define DEFINE_EMULATED_TARGET_CONFIG(node_id, prop, n)                                            \
	[n] = {                                                                                    \
		.flags = 0,                                                                        \
		.address = DT_PHA_BY_IDX(node_id, prop, n, addr),                                  \
		.callbacks = &emulated_callbacks[n],                                               \
	},

struct i2c_target_config emulated_target_config[FORWARD_COUNT] = {
	DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DEFINE_EMULATED_TARGET_CONFIG)};
