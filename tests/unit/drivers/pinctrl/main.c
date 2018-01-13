/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "autoconf.h"
#include "include/generated/generated_dts_board.h"

#include <string.h>
#include <zephyr/types.h>
#include <device.h>

/* data as produced by inline code generation */
#define TEST_DEVICE_NAME st_stm32_pinctrl_48000000
#define TEST_DATA st_stm32_pinctrl_48000000_data
#define TEST_CONFIG st_stm32_pinctrl_48000000_config
#define TEST_FUNCTION_DATA st_stm32_pinctrl_48000000_function
#define TEST_STATE_NAME_DATA st_stm32_pinctrl_48000000_state_name
#define TEST_PINCTRL_STATE_DATA st_stm32_pinctrl_48000000_pinctrl_state
#define TEST_PINCTRL_DATA st_stm32_pinctrl_48000000_pinctrl

/* device functions have an offset of PINCTRL_FUNCTION_DEVICE_BASE */
#define TEST_CLIENT_FUNCTION_DEVICE \
	(TEST_CLIENT_FUNCTION + PINCTRL_FUNCTION_DEVICE_BASE)

/* Test mock */
struct mock_info {
	int config_get_invocation;
	int config_set_invocation;
	int mux_get_invocation;
	int mux_set_invocation;
	u32_t mux_set_pin;
	u16_t mux_set_func;
	int device_init_invocation;
	struct device_config client_config;
	struct device client;
};

/* forward declarations */
static int mock_config_get(struct device *dev, u16_t pin, u32_t *config);
static int mock_config_set(struct device *dev, u16_t pin, u32_t config);
static int mock_mux_get(struct device *dev, u16_t pin, u16_t *func);
static int mock_mux_set(struct device *dev, u16_t pin, u16_t func);
static int mock_device_init(struct device *dev);

/**
 * @code{.codegen}
 * compatible = 'st,stm32-pinctrl'
 * config_get = 'mock_config_get'
 * config_set = 'mock_config_set'
 * mux_get = 'mock_mux_get'
 * mux_set = 'mock_mux_set'
 * data_info = 'struct mock_info'
 * device_init = 'mock_device_init'
 * codegen.out_include('templates/drivers/pinctrl_tmpl.c')
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

/** Defines for values from Device Tree
 * @code{.codegen}
 * test_device_id = codegen.edts().device_ids_by_compatible(compatible)[0]
 * codegen.outl('#define TEST_DRIVER_NAME "{}"'.format( \
 *     codegen.edts().device_property(test_device_id, 'label')))
 * codegen.outl('#define TEST_PIN_COUNT {}'.format( \
 *     _pin_controllers[0].pin_count()))
 * codegen.outl('#define TEST_FUNCTION_COUNT {}'.format( \
 *     _pin_controllers[0].function_count()))
 * codegen.outl('#define TEST_STATE_NAME_COUNT {}'.format( \
 *     _pin_controllers[0].state_name_count()))
 * codegen.outl('#define TEST_STATE_COUNT {}'.format( \
 *     _pin_controllers[0].state_count()))
 * codegen.outl('#define TEST_PINCTRL_COUNT {}'.format( \
 *     _pin_controllers[0].pinctrl_count()))
 *
 * test_client_name = "UART_1"
 * test_client_device_id = codegen.edts().device_id_by_label(test_client_name)
 * codegen.outl('#define TEST_CLIENT_NAME "{}"'.format(test_client_name))
 * codegen.outl('#define TEST_CLIENT_FUNCTION {}'.format( \
 *     _pin_controllers[0].function_id(test_client_name)))
 * codegen.outl('#define TEST_CLIENT_DEFAULT_STATE {}'.format( \
 *     _pin_controllers[0].state_id(test_client_name, 'default')))
 *
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

/* UART_1 default tx */
#define TEST_CLIENT_PINCTRL_PINCONF_PIN 22
#define TEST_CLIENT_PINCTRL_PINCONF_MUX 0
#define TEST_CLIENT_GROUP TEST_CLIENT_DEFAULT_STATE
#define TEST_CLIENT_GROUP_PINS 2 /* rx, tx */

static int mock_config_get(struct device *dev, u16_t pin, u32_t *config)
{
	TEST_DATA.config_get_invocation++;
	return 0;
}

static int mock_config_set(struct device *dev, u16_t pin, u32_t config)
{
	TEST_DATA.config_set_invocation++;
	return 0;
}

static int mock_mux_get(struct device *dev, u16_t pin, u16_t *func)
{
	TEST_DATA.mux_get_invocation++;
	return 0;
}

static int mock_mux_set(struct device *dev, u16_t pin, u16_t func)
{
	TEST_DATA.mux_set_invocation++;
	TEST_DATA.mux_set_pin = pin;
	TEST_DATA.mux_set_func = func;
	return 0;
}

static int mock_device_init(struct device *dev)
{
	TEST_DATA.device_init_invocation++;
	return 0;
}

/* Defines for what DEVICE_API_INIT generates */
#define TEST_DEVICE DEVICE_NAME_GET(TEST_DEVICE_NAME)

/* Access pinctrl template data structures */
#define TEST_API pinctrl_tmpl_driver_api
#define TEST_DEVICE_CONFIG_INFO(_device_name) \
	_TEST_DEVICE_CONFIG_INFO1(_device_name)
#define _TEST_DEVICE_CONFIG_INFO1(_device_name) \
	((const struct pinctrl_tmpl_config *)_device_name.config->config_info)

/* Replace Pinctrl API syscall interface */
__syscall u16_t pinctrl_get_pins_count(struct device *dev)
{
	return _impl_pinctrl_get_pins_count(dev);
}

__syscall u16_t pinctrl_get_groups_count(struct device *dev)
{
	return _impl_pinctrl_get_groups_count(dev);
}

__syscall int pinctrl_get_group_pins(struct device *dev, u16_t group,
				     u16_t *pins, u16_t *num_pins)
{
	return _impl_pinctrl_get_group_pins(dev, group, pins, num_pins);
}

__syscall u16_t pinctrl_get_states_count(struct device *dev)
{
	return _impl_pinctrl_get_states_count(dev);
}

__syscall int pinctrl_get_state_group(struct device *dev, u16_t state,
				      u16_t *group)
{
	return _impl_pinctrl_get_state_group(dev, state, group);
}

__syscall u16_t pinctrl_get_functions_count(struct device *dev)
{
	return _impl_pinctrl_get_functions_count(dev);
}

__syscall int pinctrl_get_function_group(struct device *dev, u16_t func,
					 const char *name, u16_t *group)
{
	return _impl_pinctrl_get_function_group(dev, func, name, group);
}

__syscall int pinctrl_get_function_groups(struct device *dev, u16_t func,
					  u16_t *groups, u16_t *num_groups)
{
	return _impl_pinctrl_get_function_groups(dev, func, groups, num_groups);
}

__syscall int pinctrl_get_function_state(struct device *dev, u16_t func,
					 const char *name, u16_t *state)
{
	return _impl_pinctrl_get_function_state(dev, func, name, state);
}

__syscall int pinctrl_get_function_states(struct device *dev, u16_t func,
					  u16_t *states, u16_t *num_states)
{
	return _impl_pinctrl_get_function_states(dev, func, states, num_states);
}

__syscall int pinctrl_get_device_function(struct device *dev,
					  struct device *other, u16_t *func)
{
	return _impl_pinctrl_get_device_function(dev, other, func);
}

__syscall int pinctrl_get_gpio_range(struct device *dev, struct device *gpio,
				     u32_t gpio_pin, u16_t *pin,
				     u16_t *base_pin, u8_t *num_pins)
{
	return _impl_pinctrl_get_gpio_range(
		dev, gpio, gpio_pin, pin, base_pin, num_pins);
}

__syscall int pinctrl_config_get(struct device *dev, u16_t pin, u32_t *config)
{
	return _impl_pinctrl_config_get(dev, pin, config);
}

__syscall int pinctrl_config_set(struct device *dev, u16_t pin, u32_t config)
{
	return _impl_pinctrl_config_set(dev, pin, config);
}

__syscall int pinctrl_config_group_get(struct device *dev, u16_t group,
				       u32_t *configs, u16_t *num_configs)
{
	return _impl_pinctrl_config_group_get(dev, group, configs, num_configs);
}

__syscall int pinctrl_config_group_set(struct device *dev, u16_t group,
				       const u32_t *configs, u16_t num_configs)
{
	return _impl_pinctrl_config_group_set(dev, group, configs, num_configs);
}

__syscall int pinctrl_mux_request(struct device *dev, u16_t pin,
				  const char *owner)
{
	return _impl_pinctrl_mux_request(dev, pin, owner);
}

__syscall int pinctrl_mux_free(struct device *dev, u16_t pin, const char *owner)
{
	return _impl_pinctrl_mux_free(dev, pin, owner);
}

__syscall int pinctrl_mux_get(struct device *dev, u16_t pin, u16_t *func)
{
	return _impl_pinctrl_mux_get(dev, pin, func);
}

__syscall int pinctrl_mux_set(struct device *dev, u16_t pin, u16_t func)
{
	return _impl_pinctrl_mux_set(dev, pin, func);
}

__syscall int pinctrl_mux_group_set(struct device *dev, u16_t group, u16_t func)
{
	return _impl_pinctrl_mux_group_set(dev, group, func);
}

__syscall int pinctrl_state_set(struct device *dev, u16_t state)
{
	return _impl_pinctrl_state_set(dev, state);
}


static _Bool assert_strcmp(const char *s1, const char *s2)
{
	const char *_s1 = s1;
	const char *_s2 = s2;

	for (int i = 0; i <= 1000; i++) {
		if ((*s1 == 0) && (*s2 == 0)) {
			return 1;
		}
		if (*s1 != *s2) {
			TC_PRINT("%s: %s, %s failed - char %c != %c (%d)\n",
				 __func__, _s1, _s2, *s1, *s2, i);
			return 0;
		}
		if (((*s1 == 0) && (*s2 != 0)) || ((*s1 != 0) && (*s2 == 0))) {
			TC_PRINT("%s: %s, %s failed - end of string (%d)\n",
				 __func__, _s1, _s2, i);
			return 0;
		}
		if (i >= 1000) {
			TC_PRINT("%s: %s, %s failed - no end of string (%d)\n",
				 __func__, _s1, _s2, i);
			return 0;
		}
		s1++;
		s2++;
	};
	return 1;
}

const char *error(int err)
{
	if (err == 0) {
		return "OK";
	}
	if (err == EINVAL) {
		return "EINVAL";
	}
	if (err == ENOTSUP) {
		return "ENOTSUP";
	}
	return strerror(err);

}

static void mock_reset(void)
{
	TEST_DATA.config_get_invocation = 0;
	TEST_DATA.config_set_invocation = 0;
	TEST_DATA.mux_get_invocation = 0;
	TEST_DATA.mux_set_invocation = 0;
	TEST_DATA.mux_set_pin = 0xFFFF;
	TEST_DATA.mux_set_func = 0xFFFF;
	TEST_DATA.device_init_invocation = 0;
	TEST_DATA.client_config.name = TEST_CLIENT_NAME;
	TEST_DATA.client_config.init = 0;
	TEST_DATA.client_config.config_info = 0;
	TEST_DATA.client.config = &TEST_DATA.client_config,
	TEST_DATA.client.driver_api = 0;
	TEST_DATA.client.driver_data = 0;

	/* fake init of mux_request data */
	pinctrl_tmpl_mux_owner_initialized = 0;
	zassert_equal(0,
		      pinctrl_tmpl_mux_request_init(&TEST_DEVICE),
		      "init: pinctrl_tmpl_mux_request_init failed");
}

void test_pinctrl_tmpl_test_data(void)
{
	assert_strcmp(TEST_DRIVER_NAME, "PINCTRL");
	assert_strcmp(TEST_CLIENT_NAME, "UART_1");
	zassert_true((TEST_PIN_COUNT > 0),
		     "test data: TEST_PIN_COUNT == 0");
	zassert_true((TEST_FUNCTION_COUNT > 0),
		     "test data: TEST_FUNCTION_COUNT == 0");
	zassert_true((TEST_STATE_NAME_COUNT > 0),
		     "test data: TEST_STATE_NAME_COUNT == 0");
	zassert_true((TEST_STATE_COUNT > 0),
		     "test data: TEST_STATE_COUNT == 0");
	zassert_true((TEST_PINCTRL_COUNT > 0),
		     "test data: TEST_PINCTRL_COUNT == 0");
}

void test_pinctrl_tmpl_init(void)
{
	zassert_equal(TEST_API.config.get,
		      mock_config_get,
		      "init: pinctrl_config_get API init failed");
	zassert_equal(TEST_API.mux.get,
		      mock_mux_get,
		      "init: pinctrl_mux_get API init failed");
	zassert_equal(TEST_CONFIG.mux_set,
		      mock_mux_set,
		      "init: pinctrl_mux_set API init failed");
	zassert_equal(TEST_API.state.set,
		      pinctrl_tmpl_state_set,
		      "init: pinctrl_state_set API init failed");
	zassert_equal((const void *)&TEST_CONFIG,
		      TEST_DEVICE.config->config_info,
		      "init: driver config_info init failed");
	zassert_equal(1,
		      assert_strcmp(TEST_DRIVER_NAME, TEST_DEVICE.config->name),
		      "init: driver name init failed");
	zassert_equal(TEST_PIN_COUNT,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->pin_count,
		      "init: driver config_info->pin_count init failed");
	zassert_equal(TEST_FUNCTION_COUNT,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)
						->device_function_count,
		      "init: driver config_info->device_function_count init failed");
	zassert_equal(TEST_STATE_NAME_COUNT,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->state_name_count,
		      "init: driver config_info->state_name_count init failed");
	zassert_equal(TEST_STATE_COUNT,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->pinctrl_state_count,
		      "init: driver config_info->pinctrl_state_count init failed");
	zassert_equal(TEST_PINCTRL_COUNT,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->pinctrl_count,
		      "init: driver config_info->pinctrl_count init failed");
	zassert_equal(TEST_FUNCTION_DATA,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)
						->device_function_data,
		      "init: driver config_info->device_function_data init failed");
	zassert_equal(TEST_STATE_NAME_DATA,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->state_name_data,
		      "init: driver config_info->state_name_data init failed");
	zassert_equal(TEST_PINCTRL_STATE_DATA,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->pinctrl_state_data,
		      "init: driver config_info->pinctrl_state_data "
		      "init failed");
	zassert_equal(TEST_PINCTRL_DATA,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->pinctrl_data,
		      "init: driver config_info->pinctrl_data "
		      "init failed");
	zassert_equal(mock_device_init,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->device_init,
		      "init: driver config_info->device_init init failed");
	zassert_equal(mock_mux_set,
		      TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)->mux_set,
		      "init: driver config_info->mux_set init failed");

	/* Assure pinctrl pins are in valid range */
	for (int pinctrl = 0; pinctrl < TEST_PINCTRL_COUNT; pinctrl++) {
		int pin = TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)
						->pinctrl_data[pinctrl].pin;

		zassert_equal(1, (pin < TEST_PIN_COUNT),
			      "init: driver config_info->pinctrl_data failed: "
			      "pinctrl %d pin %d", pinctrl, pin);
	}

	/* call device init function provided by driver writer (our mock) */
	mock_reset();
	int ret = TEST_DEVICE_CONFIG_INFO(TEST_DEVICE)
						->device_init(&TEST_DEVICE);

	zassert_equal(0, ret,
		      "init: device_init() failed return: %d %s",
		      ret, error(-ret));
	zassert_equal(1,
		      TEST_DATA.device_init_invocation,
		      "init: device_init() not called");

	/* call mux request init function (part of initialization) */
	mock_reset();
	ret = pinctrl_tmpl_mux_request_init(&TEST_DEVICE);
	zassert_equal(0, ret,
		      "init: pinctrl_tmpl_mux_request_init() "
		      "failed return: %d %s", ret, error(-ret));

	/* call device init function provided by template*/
	mock_reset();
	ret = TEST_DEVICE.config->init(&TEST_DEVICE);
	zassert_equal(0, ret,
		      "init: init() failed return: %d %s", ret, error(-ret));
	zassert_equal(1,
		      TEST_DATA.device_init_invocation,
		      "init: device_init() not called");
	zassert_equal(TEST_PINCTRL_COUNT,
		      TEST_DATA.config_set_invocation,
		      "init: default initialisation (config) not called");
	zassert_equal(TEST_PINCTRL_COUNT,
		      TEST_DATA.mux_set_invocation,
		      "init: default initialisation (mux) not called");
}

void test_pinctrl_tmpl_control_get_pins_count(void)
{
	mock_reset();

	zassert_equal(TEST_PIN_COUNT,
		      pinctrl_get_pins_count(&TEST_DEVICE),
		      "api: pinctrl_get_pins_count failed");
}

void test_pinctrl_tmpl_control_get_function_state(void)
{
	u16_t state = 0;
	int ret;

	mock_reset();
	ret = pinctrl_get_function_state(
		&TEST_DEVICE, TEST_CLIENT_FUNCTION_DEVICE, "default", &state),
	zassert_equal(0, ret,
		      "api: pinctrl_get_function_state failed (%d %s)",
		      (int)ret, error(ret));
	zassert_equal(TEST_CLIENT_DEFAULT_STATE,
		      state,
		      "api: pinctrl_get_function_state wrong state %d (expected %d).",
		      (int)state, (int)TEST_CLIENT_DEFAULT_STATE);
}

void test_pinctrl_tmpl_control_get_function_states(void)
{
	u16_t states[TEST_STATE_COUNT];
	u16_t num_states;

	mock_reset();
	/* Only default state configured */
	num_states = TEST_STATE_COUNT;
	states[0] = 0;
	states[1] = 0;
	zassert_equal(0,
		      pinctrl_get_function_states(&TEST_DEVICE,
						  TEST_CLIENT_FUNCTION_DEVICE,
						  &states[0],
						  &num_states),
		      "api: pinctrl_get_function_states failed");
	zassert_equal(1,
		      num_states,
		      "api: pinctrl_get_function_states wrong state count");
	zassert_equal(TEST_CLIENT_DEFAULT_STATE,
		      states[0],
		      "api: pinctrl_get_function_state wrong state");
	/* error - not sufficent array space */
	num_states = 0;
	states[0] = 0;
	states[1] = 0;
	zassert_equal(-EINVAL,
		      pinctrl_get_function_states(&TEST_DEVICE,
						  TEST_CLIENT_FUNCTION_DEVICE,
						  &states[0],
						  &num_states),
		      "api: pinctrl_get_function_states failed");
	zassert_equal(1,
		      num_states,
		      "api: pinctrl_get_function_states wrong state count");
	zassert_equal(
		0, states[0], "api: pinctrl_get_function_state wrong state");
	/* error - unknown function */
	num_states = 2;
	states[0] = 0;
	states[1] = 0;
	zassert_equal(-ENODEV,
		      pinctrl_get_function_states(&TEST_DEVICE,
						  TEST_FUNCTION_COUNT,
						  &states[0],
						  &num_states),
		      "api: pinctrl_get_function_states failed");
	zassert_equal(0,
		      num_states,
		      "api: pinctrl_get_function_states wrong state count");
	zassert_equal(
		0, states[0], "api: pinctrl_get_function_state wrong state");
}

void test_pinctrl_tmpl_control_get_device_function(void)
{
	u16_t func = 0;

	mock_reset();
	zassert_equal(0,
		      pinctrl_get_device_function(
			      &TEST_DEVICE, &TEST_DATA.client, &func),
		      "api: pinctrl_get_device_function failed");
	zassert_equal(TEST_CLIENT_FUNCTION_DEVICE,
		      func,
		      "api: pinctrl_get_device_function wrong function");
}

void test_pinctrl_tmpl_config_group_get(void)
{
	u16_t group;
	u16_t num_configs;
	u32_t configs[TEST_PIN_COUNT];

	/* normal case */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	num_configs = TEST_PIN_COUNT;
	zassert_equal(0,
		      pinctrl_tmpl_config_group_get(
			      &TEST_DEVICE, group, &configs[0], &num_configs),
		      "api: pinctrl_tmpl_config_group_get failed");
	zassert_equal(
		TEST_CLIENT_GROUP_PINS,
		num_configs,
		"api: pinctrl_tmpl_config_group_get wrong config count: %d",
		(int)num_configs);
	zassert_equal(
		TEST_CLIENT_GROUP_PINS,
		TEST_DATA.config_get_invocation,
		"api: pinctrl_tmpl_config_group_get "
		"config_get called: %d times",
		(int)TEST_DATA.config_get_invocation);
	/* error - configs array (aka. num_configs) to small */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	num_configs = TEST_CLIENT_GROUP_PINS - 1;
	zassert_equal(-EINVAL,
		      pinctrl_tmpl_config_group_get(
			      &TEST_DEVICE, group, &configs[0], &num_configs),
		      "api: pinctrl_tmpl_config_group_get failed");
	zassert_equal(
		TEST_CLIENT_GROUP_PINS,
		num_configs,
		"api: pinctrl_tmpl_config_group_get wrong config count: %d",
		(int)num_configs);
	zassert_equal(
		TEST_CLIENT_GROUP_PINS - 1,
		TEST_DATA.config_get_invocation,
		"api: pinctrl_tmpl_config_group_get "
		"config_get called: %d times",
		(int)TEST_DATA.config_get_invocation);
	/* error - unknown group */
	mock_reset();
	group = TEST_STATE_COUNT;
	num_configs = TEST_PIN_COUNT;
	zassert_equal(-ENOTSUP,
		      pinctrl_tmpl_config_group_get(
			      &TEST_DEVICE, group, &configs[0], &num_configs),
		      "api: pinctrl_tmpl_config_group_get failed");
	zassert_equal(
		0,
		num_configs,
		"api: pinctrl_tmpl_config_group_get wrong config count: %d",
		(int)num_configs);
	zassert_equal(
		0,
		TEST_DATA.config_get_invocation,
		"api: pinctrl_tmpl_config_group_get "
		"config_get called: %d times",
		(int)TEST_DATA.config_get_invocation);
}

void test_pinctrl_tmpl_config_group_set(void)
{
	u16_t group;
	u16_t num_configs;
	u32_t configs[TEST_PIN_COUNT];

	/* normal case */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	num_configs = TEST_CLIENT_GROUP_PINS;
	zassert_equal(0,
		      pinctrl_tmpl_config_group_set(
			      &TEST_DEVICE, group, &configs[0], num_configs),
		      "api: pinctrl_tmpl_config_group_set failed");
	zassert_equal(
		TEST_CLIENT_GROUP_PINS,
		TEST_DATA.config_set_invocation,
		"api: pinctrl_tmpl_config_group_set "
		"config_set called: %d times",
		(int)TEST_DATA.config_set_invocation);
	/* error - configs array (aka. num_configs) to small */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	num_configs = TEST_CLIENT_GROUP_PINS - 1;
	zassert_equal(-EINVAL,
		      pinctrl_tmpl_config_group_set(
			      &TEST_DEVICE, group, &configs[0], num_configs),
		      "api: pinctrl_tmpl_config_group_set failed");
	zassert_equal(
		TEST_CLIENT_GROUP_PINS - 1,
		TEST_DATA.config_set_invocation,
		"api: pinctrl_tmpl_config_group_set "
		"config_set called: %d times",
		(int)TEST_DATA.config_set_invocation);
	/* error - unknown group */
	mock_reset();
	group = TEST_STATE_COUNT;
	num_configs = TEST_PIN_COUNT;
	zassert_equal(-ENOTSUP,
		      pinctrl_tmpl_config_group_set(
			      &TEST_DEVICE, group, &configs[0], num_configs),
		      "api: pinctrl_tmpl_config_group_set failed");
	zassert_equal(
		0,
		TEST_DATA.config_set_invocation,
		"api: pinctrl_tmpl_config_group_set "
		"config_set called: %d times",
		(int)TEST_DATA.config_set_invocation);
}

void test_pinctrl_tmpl_mux_request_free(void)
{
	const char *owner1 = "xxxx";
	const char *owner2 = "yyyy";

	mock_reset();

	/* owner1 requests all pins - all pins should be available */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(0,
			      pinctrl_mux_request(&TEST_DEVICE, pin, owner1),
			      "api: pinctrl_mux_request failed (pin: %d)",
			      (int)pin);
	}
	/* owner2 requests all pins - none should be available */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(
			-EBUSY,
			pinctrl_mux_request(&TEST_DEVICE, pin, owner2),
			"api: pinctrl_mux_request wrongly passed (pin: %d)",
			(int)pin);
	}
	/* owner1 frees all pins - all pins should be available afterwards */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(0,
			      pinctrl_mux_free(&TEST_DEVICE, pin, owner1),
			      "api: pinctrl_mux_free failed (pin: %d)",
			      (int)pin);
	}
	/* owner2 requests all pins - all pins should be available */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(0,
			      pinctrl_mux_request(&TEST_DEVICE, pin, owner2),
			      "api: pinctrl_mux_request failed (pin: %d)",
			      (int)pin);
	}
	/* owner1 tries to free all pins - should not be possible - not owner */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(-EACCES,
			      pinctrl_mux_free(&TEST_DEVICE, pin, owner1),
			      "api: pinctrl_mux_free wrongly passed (pin: %d)",
			      (int)pin);
	}
	/* owner2 frees all pins - all pins should be available afterwards */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(0,
			      pinctrl_mux_free(&TEST_DEVICE, pin, owner2),
			      "api: pinctrl_mux_free failed (pin: %d)",
			      (int)pin);
	}
	/* owner1 requests all pins - all pins should be available */
	for (u16_t pin = 0; pin < TEST_PIN_COUNT;
	     pin++) {
		zassert_equal(0,
			      pinctrl_mux_request(&TEST_DEVICE, pin, owner1),
			      "api: pinctrl_mux_request failed (pin: %d)",
			      (int)pin);
	}
	zassert_equal(
		-ENOTSUP,
		pinctrl_mux_request(&TEST_DEVICE,
				    TEST_PIN_COUNT,
				    owner2),
		"api: pinctrl_mux_request wrongly passed (pin: %d)",
		(int)TEST_PIN_COUNT);
}

void test_pinctrl_tmpl_mux_set(void)
{
	u16_t pin;
	u16_t func;
	int ret;

	/* hardware pinmux*/
	mock_reset();
	pin = TEST_CLIENT_PINCTRL_PINCONF_PIN;
	func = TEST_CLIENT_PINCTRL_PINCONF_MUX;

	ret = pinctrl_mux_set(&TEST_DEVICE, pin, func);
	zassert_equal(0, ret,
		      "api: pinctrl_tmpl_mux_set failed %d %s\n",
		      ret, error(-ret));
	zassert_equal(
		1,
		TEST_DATA.mux_set_invocation,
		"api: pinctrl_tmpl_mux_set called: %d times",
		(int)TEST_DATA.mux_set_invocation);
	zassert_equal(
		pin,
		TEST_DATA.mux_set_pin,
		"api: pinctrl_tmpl_mux_set unexpected pin: %d",
		(int)TEST_DATA.mux_set_pin);
	zassert_equal(
		func,
		TEST_DATA.mux_set_func,
		"api: pinctrl_tmpl_mux_set unexpected func: %d",
		(int)TEST_DATA.mux_set_func);

	/* device pinmux*/
	mock_reset();
	pin = TEST_CLIENT_PINCTRL_PINCONF_PIN;
	func = TEST_CLIENT_FUNCTION_DEVICE;
	zassert_false((func == TEST_CLIENT_PINCTRL_PINCONF_MUX),
		      "api: pinctrl_tmpl_mux_set wrong test setup\n");

	ret = pinctrl_mux_set(&TEST_DEVICE, pin, func);
	zassert_equal(0, ret,
		      "api: pinctrl_tmpl_mux_set failed %d %s\n",
		      ret, error(-ret));
	func = TEST_CLIENT_PINCTRL_PINCONF_MUX;
	zassert_equal(
		1,
		TEST_DATA.mux_set_invocation,
		"api: pinctrl_tmpl_mux_set called: %d times",
		(int)TEST_DATA.mux_set_invocation);
	zassert_equal(
		pin,
		TEST_DATA.mux_set_pin,
		"api: pinctrl_tmpl_mux_set unexpected pin: %d",
		(int)TEST_DATA.mux_set_pin);
	zassert_equal(
		func,
		TEST_DATA.mux_set_func,
		"api: pinctrl_tmpl_mux_set unexpected func: %d",
		(int)TEST_DATA.mux_set_func);
}

void test_pinctrl_tmpl_mux_group_set(void)
{
	u16_t group;
	u16_t func;

	/* normal case */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	func = TEST_CLIENT_FUNCTION_DEVICE;
	zassert_equal(0,
		      pinctrl_tmpl_mux_group_set(&TEST_DEVICE, group, func),
		      "api: pinctrl_tmpl_mux_group_set failed");
	zassert_equal(
		TEST_CLIENT_GROUP_PINS,
		TEST_DATA.mux_set_invocation,
		"api: pinctrl_tmpl_mux_group_set config_set called: %d times",
		(int)TEST_DATA.mux_set_invocation);
	/* error - unknown function */
	mock_reset();
	group = TEST_CLIENT_GROUP;
	func = TEST_FUNCTION_COUNT;
	zassert_equal(-ENOTSUP,
		      pinctrl_tmpl_mux_group_set(&TEST_DEVICE, group, func),
		      "api: pinctrl_tmpl_mux_group_set failed");
	zassert_equal(
		0,
		TEST_DATA.mux_set_invocation,
		"api: pinctrl_tmpl_mux_group_set config_set called: %d times",
		(int)TEST_DATA.mux_set_invocation);
	/* error - unknown group */
	mock_reset();
	group = TEST_STATE_COUNT;
	func = TEST_CLIENT_FUNCTION_DEVICE;
	zassert_equal(-ENOTSUP,
		      pinctrl_tmpl_mux_group_set(&TEST_DEVICE, group, func),
		      "api: pinctrl_tmpl_mux_group_set failed");
	zassert_equal(
		0,
		TEST_DATA.mux_set_invocation,
		"api: pinctrl_tmpl_mux_group_set config_set called: %d times",
		(int)TEST_DATA.mux_set_invocation);
}

void test_pinctrl_tmpl_state_set(void)
{
	mock_reset();
	int ret = pinctrl_state_set(&TEST_DEVICE, TEST_CLIENT_DEFAULT_STATE);

	zassert_equal(
		0, ret,
		"api: pinctrl_state_set failed: %d %s\n",
		ret, error(-ret));
}

void test_pinctrl_pinmux(void)
{
	mock_reset();
	int ret = pinmux_pin_set(&TEST_DEVICE, 1, 2);

	zassert_equal(0, ret,
		      "pinctrl_pinmux: pinmux_pin_set() failed: %d %s",
		      ret, error(-ret));
	zassert_equal(1,
		      TEST_DATA.mux_set_invocation,
		      "pinctrl_pinmux: mux_set not called");
}

void test_main(void)
{
	ztest_test_suite(
		test_pinctrl_tmpl,
		ztest_unit_test(test_pinctrl_tmpl_test_data),
		ztest_unit_test(test_pinctrl_tmpl_init),
		ztest_unit_test(test_pinctrl_tmpl_control_get_pins_count),
		ztest_unit_test(test_pinctrl_tmpl_control_get_function_state),
		ztest_unit_test(test_pinctrl_tmpl_control_get_function_states),
		ztest_unit_test(test_pinctrl_tmpl_control_get_device_function),
		ztest_unit_test(test_pinctrl_tmpl_config_group_get),
		ztest_unit_test(test_pinctrl_tmpl_config_group_set),
		ztest_unit_test(test_pinctrl_tmpl_mux_request_free),
		ztest_unit_test(test_pinctrl_tmpl_mux_set),
		ztest_unit_test(test_pinctrl_tmpl_mux_group_set),
		ztest_unit_test(test_pinctrl_tmpl_state_set),
		ztest_unit_test(test_pinctrl_pinmux));
	ztest_run_test_suite(test_pinctrl_tmpl);
}
