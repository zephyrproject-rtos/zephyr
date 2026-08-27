/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TESTS_DRIVERS_I2C_I2C_EMUL_INCLUDE_EMULATED_TARGET_H
#define _TESTS_DRIVERS_I2C_I2C_EMUL_INCLUDE_EMULATED_TARGET_H

#include <functional>

#define CUSTOM_FFF_FUNCTION_TEMPLATE(RETURN, FUNCNAME, ...)                                        \
	std::function<RETURN(__VA_ARGS__)> FUNCNAME

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>

#define CONTROLLER_LABEL DT_NODELABEL(i2c0)
#define TARGET_LABEL(n)  DT_NODELABEL(DT_CAT(i2c, n))
#define FORWARD_COUNT    DT_PROP_LEN(CONTROLLER_LABEL, forwards)

extern struct i2c_target_callbacks emulated_callbacks[FORWARD_COUNT];
extern struct i2c_target_config emulated_target_config[FORWARD_COUNT];

/* Declare all the fake functions needed */
#define DECLARE_FAKE_TARGET_FUNCTIONS(node_id, prop, n)                                            \
	DECLARE_FAKE_VALUE_FUNC(int, target_read_requested_##n, struct i2c_target_config *,        \
				uint8_t *);                                                        \
	DECLARE_FAKE_VALUE_FUNC(int, target_read_processed_##n, struct i2c_target_config *,        \
				uint8_t *);                                                        \
	DECLARE_FAKE_VALUE_FUNC(int, target_write_requested_##n, struct i2c_target_config *);      \
	DECLARE_FAKE_VALUE_FUNC(int, target_write_received_##n, struct i2c_target_config *,        \
				uint8_t);                                                          \
	DECLARE_FAKE_VALUE_FUNC(int, target_stop_##n, struct i2c_target_config *);                 \
	DECLARE_FAKE_VALUE_FUNC(int, target_buf_read_requested_##n, struct i2c_target_config *,    \
				uint8_t **, uint32_t *)                                            \
	DECLARE_FAKE_VOID_FUNC(target_buf_write_received_##n, struct i2c_target_config *,          \
			       uint8_t *, uint32_t)

DT_FOREACH_PROP_ELEM(CONTROLLER_LABEL, forwards, DECLARE_FAKE_TARGET_FUNCTIONS)

#define FFF_FAKE_ACTION(node_id, prop, n, fn)                                                      \
	do {                                                                                       \
		fn(target_read_requested_##n);                                                     \
		fn(target_read_processed_##n);                                                     \
		fn(target_write_requested_##n);                                                    \
		fn(target_write_received_##n);                                                     \
		fn(target_stop_##n);                                                               \
		fn(target_buf_read_requested_##n);                                                 \
		fn(target_buf_write_received_##n);                                                 \
	} while (0);

#define FFF_FAKES_LIST_FOREACH(fn)                                                                 \
	DT_FOREACH_PROP_ELEM_VARGS(CONTROLLER_LABEL, forwards, FFF_FAKE_ACTION, fn)

#endif /* _TESTS_DRIVERS_I2C_I2C_EMUL_INCLUDE_EMULATED_TARGET_H */
