/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <generated/generated_dts_board.h>

#include "pinctrl_test.h"

static _Bool assert_pinmux(u32_t a_port, u32_t a_mux,
			   u32_t b_port, u32_t b_mux)
{
	if (a_port != b_port) {
		TC_PRINT("%s: %u, %u, %u, %u failed - port %u != %u\n",
			 __func__, a_port, a_mux, b_port, b_mux,
			 a_port, b_port);
		return 0;
	}
	if (a_mux != b_mux) {
		TC_PRINT("%s: %u, %u, %u, %u failed - mux %u != %u\n",
			 __func__, a_port, a_mux, b_port, b_mux,
			 a_mux, b_mux);
		return 0;
	}
	return 1;
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

static struct {
	u32_t uut_port;
	u32_t uut_mux;
	u32_t expected_port;
	u32_t expected_mux;
} test_pinctrl_define_pinmux[] = {
	/* assure pinmux defines are correctly generated for device 0*/
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_RX_PINMUX, 1000, 2000 },
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_TX_PINMUX, 1001, 2001 },
	/* assure PINCTRL_TEST_DEVICE0 pinmux for Device 0 is correct */
#if TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 0
	{ TEST_PINCTRL_40000000_PINCTRL_0_PINMUX, 1000, 2000 },
#elif TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 1
	{ TEST_PINCTRL_40000000_PINCTRL_1_PINMUX, 1000, 2000 },
#elif TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 2
	{ TEST_PINCTRL_40000000_PINCTRL_2_PINMUX, 1000, 2000 },
#else
#error "unexpected default state id of pinctrl for device 0"
#endif
#if TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 0
	{ TEST_PINCTRL_40000000_PINCTRL_0_PINMUX, 1001, 2001 },
#elif TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 1
	{ TEST_PINCTRL_40000000_PINCTRL_1_PINMUX, 1001, 2001 },
#elif TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 2
	{ TEST_PINCTRL_40000000_PINCTRL_2_PINMUX, 1001, 2001 },
#else
#error "unexpected shutdown state id of pinctrl for device 0"
#endif
};

static struct {
	u32_t uut_val;
	u32_t expected_val;
} test_pinctrl_define_int[] = {
	/* assure define directives are correctly generated for device 0*/
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_RX_BIAS_PULL_UP, 1000},
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_RX_INPUT_ENABLE, 1},
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_TX_BIAS_PULL_DOWN, 1001},
	{ TEST_DEVICE_4000C000_PINCTRL_DEFAULT_TX_OUTPUT_ENABLE, 1},
	/* assure PINCTRL_TEST_DEVICE0 pin control directives for Device 0
	 * are correct
	 */
#if TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 0
	{ TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_UP, 1000},
	{ TEST_PINCTRL_40000000_PINCTRL_0_INPUT_ENABLE, 1},
#elif TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 1
	{ TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_UP, 1000},
	{ TEST_PINCTRL_40000000_PINCTRL_0_INPUT_ENABLE, 1},
#elif TEST_DEVICE_4000C000_PINCTRL_DEFAULT_STATE_ID == 2
	{ TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_UP, 1000},
	{ TEST_PINCTRL_40000000_PINCTRL_0_INPUT_ENABLE, 1},
#else
#error "unexpected default state id of pinctrl for device 0"
#endif
#if TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 0
	{ TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_DOWN, 1001},
	{ TEST_PINCTRL_40000000_PINCTRL_0_OUTPUT_ENABLE, 1},
#elif TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 1
	{ TEST_PINCTRL_40000000_PINCTRL_1_BIAS_PULL_DOWN, 1001},
	{ TEST_PINCTRL_40000000_PINCTRL_1_OUTPUT_ENABLE, 1},
#elif TEST_DEVICE_4000C000_PINCTRL_SHUTDOWN_STATE_ID == 2
	{ TEST_PINCTRL_40000000_PINCTRL_2_BIAS_PULL_DOWN, 1001},
	{ TEST_PINCTRL_40000000_PINCTRL_2_OUTPUT_ENABLE, 1},
#else
#error "unexpected shutdown state id of pinctrl for device 0"
#endif
};

static struct {
	const char *uut_str;
	const char *expected_str;
} test_pinctrl_define_str[] = {
	/* Assure we are on the correct generated_dts_board.h */
	{ TEST_DEVICE_4000C000_LABEL, "DEVICE_0" },
	{ TEST_DEVICE_5000C000_LABEL, "DEVICE_1" },
	{ TEST_DEVICE_6000C000_LABEL, "DEVICE_2" },
	{ TEST_GPIO_10000000_LABEL, "GPIO_0" },
	{ TEST_GPIO_20000000_LABEL, "GPIO_1" },
	{ TEST_GPIO_30000000_LABEL, "GPIO_2" },
	{ TEST_PINCTRL_40000000_LABEL, "PINCTRL_TEST_DEVICE0" },
	{ TEST_PINCTRL_50000000_LABEL, "PINCTRL_TEST_DEVICE1" },

	/* assure non-linux pinctrl-binding directives are created */
#define xstr2(s) str2(s)
#define xstr(s) str(s)
#define str2(s1, s2) (#s1 "," #s2)
#define str(s) #s
	/* find out the index of the pin configuration */
#define rx 9999
#if rx == TEST_DEVICE_6000C000_PINMUX_DEVICE2_0_RX_TX_PIN_0
#define EXPECTED_RX_TX_PIN_0 "rx"
#define EXPECTED_FUNCTION_0 "[1234,5678]"
#define EXPECTED_RX_TX_PIN_1 "tx"
#define EXPECTED_FUNCTION_1 "[2345,6789]"
#else
#define EXPECTED_RX_TX_PIN_1 "rx"
#define EXPECTED_FUNCTION_1 "[1234,5678]"
#define EXPECTED_RX_TX_PIN_0 "tx"
#define EXPECTED_FUNCTION_0 "[2345,6789]"
#endif
#undef rx
	/* strings to compare */
#define TEST_PINMUX_LEGACY0 \
	xstr(TEST_DEVICE_6000C000_PINMUX_DEVICE2_0_RX_TX_PIN_0)
	{ TEST_PINMUX_LEGACY0, EXPECTED_RX_TX_PIN_0},
#define TEST_PINMUX_LEGACY1 \
	xstr2(TEST_DEVICE_6000C000_PINMUX_DEVICE2_0_RX_TX_FUNCTION_0)
	{ TEST_PINMUX_LEGACY1, EXPECTED_FUNCTION_0},
#define TEST_PINMUX_LEGACY2 \
	xstr(TEST_DEVICE_6000C000_PINMUX_DEVICE2_0_RX_TX_PIN_1)
	{ TEST_PINMUX_LEGACY2, EXPECTED_RX_TX_PIN_1},
#define TEST_PINMUX_LEGACY3 \
	xstr2(TEST_DEVICE_6000C000_PINMUX_DEVICE2_0_RX_TX_FUNCTION_1)
	{ TEST_PINMUX_LEGACY3, EXPECTED_FUNCTION_1},
};

/**
 * @brief Test PINCTRL define directives
 *
 * Test that PINCTRL defines in generated_dts_board.h are correctly generated.
 */
void test_pinctrl_define_directives(void)
{
	for (int i = 0; i < ARRAY_SIZE(test_pinctrl_define_str); i++) {
		if (!assert_strcmp(test_pinctrl_define_str[i].uut_str,
				   test_pinctrl_define_str[i].expected_str)) {
			TC_PRINT("%s: test string %d\n",
				 __func__, i);
			ztest_test_fail();
		}
	}

	for (int i = 0; i < ARRAY_SIZE(test_pinctrl_define_int); i++) {
		zassert_equal(test_pinctrl_define_int[i].uut_val,
			      test_pinctrl_define_int[i].expected_val,
			      "test value %d", i);
	}

	for (int i = 0; i < ARRAY_SIZE(test_pinctrl_define_pinmux); i++) {
		if (!assert_pinmux(
			test_pinctrl_define_pinmux[i].uut_port,
			test_pinctrl_define_pinmux[i].uut_mux,
			test_pinctrl_define_pinmux[i].expected_port,
			test_pinctrl_define_pinmux[i].expected_mux)) {
			TC_PRINT("%s: test pinmux %d\n",
				 __func__, i);
			ztest_test_fail();
		}
	}
}

void test_pinctrl_default_init(void)
{
#define TEST_PINCTRL_DEFAULT_INIT(label, idx)                               \
	_TEST_PINCTRL_DEFAULT_INIT1(label##_PINCTRL_##idx##_PINMUX,         \
				    label##_PINCTRL_##idx##_BIAS_PULL_UP,   \
				    label##_PINCTRL_##idx##_BIAS_PULL_DOWN, \
				    label##_PINCTRL_##idx##_INPUT_ENABLE,   \
				    label##_PINCTRL_##idx##_OUTPUT_ENABLE)
#define _TEST_PINCTRL_DEFAULT_INIT1(_mux, _up, _down, _in, _out) \
	_TEST_PINCTRL_DEFAULT_INIT2(_mux, _up, _down, _in, _out)
#define _TEST_PINCTRL_DEFAULT_INIT2(_mux0, _mux1, _up, _down, _in, _out) \
	{.pinmux = {_mux0, _mux1},                                       \
	 .bias_pull_up = _up,                                            \
	 .bias_pull_down = _down,                                        \
	 .input_enable = _in,                                            \
	 .output_enable = _out},

	struct s_pinctrl_init {
		u32_t pinmux[2];
		u32_t bias_pull_up;
		u32_t bias_pull_down;
		_Bool input_enable;
		_Bool output_enable;
	};

	const struct s_pinctrl_init pinctrl_init[] = {
#if TEST_PINCTRL_40000000_PINCTRL_COUNT > 0
		TEST_PINCTRL_DEFAULT_INIT(TEST_PINCTRL_40000000, 0)
#endif
#if TEST_PINCTRL_40000000_PINCTRL_COUNT > 1
		TEST_PINCTRL_DEFAULT_INIT(TEST_PINCTRL_40000000, 1)
#endif
#if TEST_PINCTRL_40000000_PINCTRL_COUNT > 2
		TEST_PINCTRL_DEFAULT_INIT(TEST_PINCTRL_40000000, 2)
#endif
#if TEST_PINCTRL_40000000_PINCTRL_COUNT > 3
		TEST_PINCTRL_DEFAULT_INIT(TEST_PINCTRL_40000000, 3)
#endif

/* unused default configuration index 4 */
#if TEST_PINCTRL_40000000_PINCTRL_COUNT > 4
		TEST_PINCTRL_DEFAULT_INIT(TEST_PINCTRL_40000000, 4)
#endif
	};

	/* http://www.geeksforgeeks.org/how-to-find-size-of-array-in-cc/
	 * -without-using-sizeof-operator/
	 */
	const int pinctrl_init_size = *(&pinctrl_init + 1) - pinctrl_init;

	zassert_equal(TEST_PINCTRL_40000000_PINCTRL_COUNT,
		      4,
		      "pin control init size");
	zassert_equal(pinctrl_init_size,
		      TEST_PINCTRL_40000000_PINCTRL_COUNT,
		      "pin control init size");
	zassert_equal(pinctrl_init[0].bias_pull_up,
		      TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_UP,
		      "");
	zassert_equal(pinctrl_init[0].bias_pull_down,
		      TEST_PINCTRL_40000000_PINCTRL_0_BIAS_PULL_DOWN,
		      "");
	zassert_equal(pinctrl_init[0].input_enable,
		      TEST_PINCTRL_40000000_PINCTRL_0_INPUT_ENABLE,
		      "");
	zassert_equal(pinctrl_init[0].output_enable,
		      TEST_PINCTRL_40000000_PINCTRL_0_OUTPUT_ENABLE,
		      "");
	zassert_equal(pinctrl_init[1].bias_pull_up,
		      TEST_PINCTRL_40000000_PINCTRL_1_BIAS_PULL_UP,
		      "");
	zassert_equal(pinctrl_init[1].bias_pull_down,
		      TEST_PINCTRL_40000000_PINCTRL_1_BIAS_PULL_DOWN,
		      "");
	zassert_equal(pinctrl_init[1].input_enable,
		      TEST_PINCTRL_40000000_PINCTRL_1_INPUT_ENABLE,
		      "");
	zassert_equal(pinctrl_init[1].output_enable,
		      TEST_PINCTRL_40000000_PINCTRL_1_OUTPUT_ENABLE,
		      "");
}

void test_pinctrl_pins_default_init(void)
{
#define PIN_A 23
#define PIN_B 34

#define TEST_PINCTRL_PINS_INIT(label, idx)                            \
	_TEST_PINCTRL_PINS_INIT1(label##_PINCTRL_##idx##_OUTPUT_HIGH, \
				 label##_PINCTRL_##idx##_PINS)
#define _TEST_PINCTRL_PINS_INIT1(_high, _pins) \
	_TEST_PINCTRL_PINS_INIT2(_high, _pins, 0, 0, 0, 0, 0, 0)
#define _TEST_PINCTRL_PINS_INIT2(                             \
	_high, _pin0, _pin1, _pin2, _pin3, _pin4, _pin5, ...) \
	{.pins = {_pin0, _pin1, _pin2, _pin3, _pin4, _pin5},  \
	 .output_high = _high},

	struct s_pinctrl_init {
		u32_t pins[6];
		_Bool output_high;
	};

	const struct s_pinctrl_init pinctrl_init[] = {
#if TEST_PINCTRL_50000000_PINCTRL_COUNT > 0
		TEST_PINCTRL_PINS_INIT(TEST_PINCTRL_50000000, 0)
#endif

/* unused default configuration index 001 */
#if TEST_PINCTRL_50000000_PINCTRL_COUNT > 1
		TEST_PINCTRL_PINS_INIT(TEST_PINCTRL_50000000, 1)
#endif
#if TEST_PINCTRL_50000000_PINCTRL_COUNT > 2
		TEST_PINCTRL_PINS_INIT(TEST_PINCTRL_50000000, 2)
#endif
	};

	const int pinctrl_init_size = *(&pinctrl_init + 1) - pinctrl_init;

	zassert_equal(TEST_PINCTRL_50000000_PINCTRL_COUNT,
		      1,
		      "pin control init size");
	zassert_equal(pinctrl_init_size,
		      TEST_PINCTRL_50000000_PINCTRL_COUNT,
		      "pin control init size");
	zassert_equal(pinctrl_init[0].pins[0], PIN_A, "");
	zassert_equal(pinctrl_init[0].pins[1], PIN_B, "");
	zassert_equal(pinctrl_init[0].pins[2], 0, "");
	zassert_equal(pinctrl_init[0].output_high,
		      TEST_PINCTRL_50000000_PINCTRL_0_OUTPUT_HIGH,
		      "");
}

void test_gpio_ranges(void)
{
	/* defines are generated */
	zassert_equal(TEST_GPIO_10000000_GPIO_RANGE_0_BASE,
		      GPIO_PORT_PIN0,
		      "gpio-ranges gpio port pin base");
	zassert_equal(TEST_GPIO_10000000_GPIO_RANGE_0_NPINS,
		      16,
		      "gpio-ranges gpio port pins number");
	zassert_equal(TEST_GPIO_10000000_GPIO_RANGE_0_CONTROLLER_BASE,
		      16,
		      "gpio-ranges controller port pin base");
	zassert(assert_strcmp(xstr(TEST_GPIO_10000000_GPIO_RANGE_0_CONTROLLER),
			      "TEST_PINCTRL_40000000"),
		"pass",
		"fail");
}

void test_device_controller(void)
{
	/* defines are generated */
	zassert_equal(SOC_PIN_CONTROLLER_COUNT,
		      2,
		      "soc pin controller count: %d",
		      (int)SOC_PIN_CONTROLLER_COUNT);
	zassert_equal(SOC_GPIO_CONTROLLER_COUNT,
		      3,
		      "soc gpio controller count: %d",
		      (int)SOC_GPIO_CONTROLLER_COUNT);
}

void test_main(void)
{
	ztest_test_suite(test_extract_dts_includes,
			 ztest_unit_test(test_pinctrl_define_directives),
			 ztest_unit_test(test_pinctrl_default_init),
			 ztest_unit_test(test_pinctrl_pins_default_init),
			 ztest_unit_test(test_gpio_ranges),
			 ztest_unit_test(test_device_controller));
	ztest_run_test_suite(test_extract_dts_includes);
}
