/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

int rk3588_pinctrl_configure_pin(const pinctrl_soc_pin_t *pin, uintptr_t base);

#define RK3588_IOC_SIZE 0x10000U
#define TEST_CONSUMER   DT_NODELABEL(test_consumer)

PINCTRL_DT_DEFINE(TEST_CONSUMER);

static uint32_t fake_ioc[RK3588_IOC_SIZE / sizeof(uint32_t)];

static uint32_t ioc_read(uint16_t offset)
{
	return fake_ioc[offset / sizeof(uint32_t)];
}

static void ioc_clear(void)
{
	(void)memset(fake_ioc, 0, sizeof(fake_ioc));
}

static void configure_and_assert(const struct rk3588_pinctrl_soc_pin *pin, int expected_ret)
{
	zassert_equal(rk3588_pinctrl_configure_pin(pin, (uintptr_t)fake_ioc), expected_ret);
}

ZTEST(rk3588_pinctrl, test_pin_encoding)
{
	zassert_equal(RK3588_PIN_UART2_TX_M0, 0x0daU);
	zassert_equal(RK3588_PIN_UART2_RX_M0, 0x0eaU);
	zassert_equal(RK3588_PIN_I2C0_SDA_M1, 0x969U);
	zassert_equal(RK3588_PIN_I2C6_SCL_M0, 0x189U);
	zassert_equal(RK3588_PIN_I2C6_SDA_M0, 0x179U);
	zassert_equal(RK3588_DT_PIN(RK3588_PIN_UART2_TX_M0), 13U);
	zassert_equal(RK3588_DT_PIN(RK3588_PIN_I2C0_SDA_M1), 150U);
	zassert_equal(RK3588_PINMUX_BANK(RK3588_PIN_I2C0_SDA_M1), 4U);
	zassert_equal(RK3588_PINMUX_FUNCTION(RK3588_PIN_I2C0_SDA_M1), 9U);
}

ZTEST(rk3588_pinctrl, test_multiple_state_phandles)
{
	const struct pinctrl_dev_config *config = PINCTRL_DT_DEV_CONFIG_GET(TEST_CONSUMER);
	const struct pinctrl_state *state;

	zassert_equal(config->state_cnt, 1U);
	state = &config->states[0];
	zassert_equal(state->id, PINCTRL_STATE_DEFAULT);
	zassert_equal(state->pin_cnt, 3U);
	zassert_equal(state->pins[0].pin, 0U);
	zassert_equal(state->pins[1].pin, 1U);
	zassert_equal(state->pins[2].pin, 2U);
}

ZTEST(rk3588_pinctrl, test_uart2_pmu2_bus_route)
{
	const struct rk3588_pinctrl_soc_pin tx = {
		.pin = 13,
		.function = 10,
		.bias = RK3588_PINCTRL_BIAS_PULL_UP,
		.flags = RK3588_PINCTRL_INPUT_ENABLE,
	};

	ioc_clear();
	configure_and_assert(&tx, 0);

	/* PMU2 B_H selects route value 8 for B5. */
	zassert_equal(ioc_read(0x4000), 0x00f00080U);

	/* BUS_IOC B_H selects UART2_TX_M0 (function 10). */
	zassert_equal(ioc_read(0x800c), 0x00f000a0U);

	/* PMU2 B pull-up and IE. */
	zassert_equal(ioc_read(0x4028), 0x0c000c00U);
	zassert_equal(ioc_read(0x4034), 0x00200020U);
}

ZTEST(rk3588_pinctrl, test_gpio4_c6_mixed_blocks)
{
	const struct rk3588_pinctrl_soc_pin sda = {
		.pin = 150,
		.function = 9,
		.bias = RK3588_PINCTRL_BIAS_PULL_UP,
		.drive_strength_ohms = 50,
		.flags = RK3588_PINCTRL_INPUT_ENABLE | RK3588_PINCTRL_SCHMITT_ENABLE,
	};

	ioc_clear();
	configure_and_assert(&sda, 0);

	zassert_equal(ioc_read(0x8094), 0x0f000900U);
	zassert_equal(ioc_read(0xa148), 0x30003000U);
	zassert_equal(ioc_read(0xa094), 0x03000200U);
	zassert_equal(ioc_read(0xa1b8), 0x00400040U);
	zassert_equal(ioc_read(0xa248), 0x00400040U);
}

ZTEST(rk3588_pinctrl, test_three_bit_drive_encoding)
{
	const struct rk3588_pinctrl_soc_pin pin = {
		.pin = 109, /* GPIO3_B5 */
		.function = 10,
		.drive_strength_ohms = 25,
	};

	ioc_clear();
	configure_and_assert(&pin, 0);

	/* GPIO3_B5 is the second pin in VCCIO3_5 GPIO3B_DS_L. */
	zassert_equal(ioc_read(0xa06c), 0x00700050U);
	zassert_equal(ioc_read(0x806c), 0x00f000a0U);
}

ZTEST(rk3588_pinctrl, test_two_bit_drive_encoding)
{
	const struct rk3588_pinctrl_soc_pin pin = {
		.pin = 48, /* GPIO1_C0 */
		.function = 0,
		.drive_strength_ohms = 33,
	};

	ioc_clear();
	configure_and_assert(&pin, 0);

	/* GPIO1_C0 is the first 2-bit field in VCCIO1_4 GPIO1C_DS_L. */
	zassert_equal(ioc_read(0x9030), 0x00030001U);
}

ZTEST(rk3588_pinctrl, test_mixed_drive_widths)
{
	const struct rk3588_pinctrl_soc_pin gpio1_d4 = {
		.pin = 60,
		.function = 0,
		.drive_strength_ohms = 33,
	};
	const struct rk3588_pinctrl_soc_pin gpio2_a0 = {
		.pin = 64,
		.function = 0,
		.drive_strength_ohms = 33,
	};
	const struct rk3588_pinctrl_soc_pin gpio2_d0 = {
		.pin = 88,
		.function = 0,
		.drive_strength_ohms = 33,
	};
	const struct rk3588_pinctrl_soc_pin gpio4_c0 = {
		.pin = 144,
		.function = 0,
		.drive_strength_ohms = 33,
	};

	ioc_clear();
	configure_and_assert(&gpio1_d4, 0);
	zassert_equal(ioc_read(0x903c), 0x00030001U);

	ioc_clear();
	configure_and_assert(&gpio2_a0, 0);
	zassert_equal(ioc_read(0xd040), 0x00070001U);

	ioc_clear();
	configure_and_assert(&gpio2_d0, 0);
	zassert_equal(ioc_read(0xd058), 0x00070001U);

	ioc_clear();
	configure_and_assert(&gpio4_c0, 0);
	zassert_equal(ioc_read(0xc090), 0x00070001U);
}

ZTEST(rk3588_pinctrl, test_gpio0_drive_widths)
{
	const struct rk3588_pinctrl_soc_pin gpio0_a0 = {
		.pin = 0, /* GPIO0_A0, 2-bit drive field */
		.function = 0,
		.drive_strength_ohms = 25,
	};
	const struct rk3588_pinctrl_soc_pin gpio0_b5 = {
		.pin = 13, /* GPIO0_B5, 3-bit drive field */
		.function = 0,
		.drive_strength_ohms = 25,
	};

	ioc_clear();
	configure_and_assert(&gpio0_a0, 0);
	zassert_equal(ioc_read(0x0010), 0x00030003U);

	ioc_clear();
	configure_and_assert(&gpio0_b5, 0);
	zassert_equal(ioc_read(0x4014), 0x00700050U);
}

ZTEST(rk3588_pinctrl, test_trm_only_mux_values)
{
	const struct rk3588_pinctrl_soc_pin gpio0_a7 = {
		.pin = 7, /* GPIO0_A7, PMIC_INT_L */
		.function = 1,
	};
	const struct rk3588_pinctrl_soc_pin gpio2_c4 = {
		.pin = 84, /* GPIO2_C4, TEST_CLKOUT_M1 */
		.function = 3,
	};
	const struct rk3588_pinctrl_soc_pin gpio4_d5 = {
		.pin = 157, /* GPIO4_D5, TEST_CLKOUT_M0 */
		.function = 4,
	};

	ioc_clear();
	configure_and_assert(&gpio0_a7, 0);
	zassert_equal(ioc_read(0x0004), 0xf0001000U);

	ioc_clear();
	configure_and_assert(&gpio2_c4, 0);
	zassert_equal(ioc_read(0x8054), 0x000f0003U);

	ioc_clear();
	configure_and_assert(&gpio4_d5, 0);
	zassert_equal(ioc_read(0x809c), 0x00f00040U);
}

ZTEST(rk3588_pinctrl, test_uart4_ie_uses_vccio1_4)
{
	const struct rk3588_pinctrl_soc_pin rx = {
		.pin = 42, /* GPIO1_B2, UART4_RX_M2 */
		.function = 10,
		.flags = RK3588_PINCTRL_INPUT_ENABLE,
	};
	const struct rk3588_pinctrl_soc_pin tx = {
		.pin = 43, /* GPIO1_B3, UART4_TX_M2 */
		.function = 10,
		.flags = RK3588_PINCTRL_INPUT_ENABLE,
	};

	ioc_clear();
	configure_and_assert(&rx, 0);
	zassert_equal(ioc_read(0x8028), 0x0f000a00U);
	zassert_equal(ioc_read(0x9184), 0x00040004U);

	ioc_clear();
	configure_and_assert(&tx, 0);
	zassert_equal(ioc_read(0x8028), 0xf000a000U);
	zassert_equal(ioc_read(0x9184), 0x00080008U);
}

ZTEST(rk3588_pinctrl, test_flag_disables_and_conflicts)
{
	const struct rk3588_pinctrl_soc_pin input_disable = {
		.pin = 0,
		.function = 0,
		.flags = RK3588_PINCTRL_INPUT_DISABLE,
	};
	const struct rk3588_pinctrl_soc_pin schmitt_disable = {
		.pin = 0,
		.function = 0,
		.flags = RK3588_PINCTRL_SCHMITT_DISABLE,
	};
	const struct rk3588_pinctrl_soc_pin input_conflict = {
		.pin = 0,
		.function = 0,
		.flags = (uint8_t)(RK3588_PINCTRL_INPUT_ENABLE | RK3588_PINCTRL_INPUT_DISABLE),
	};
	const struct rk3588_pinctrl_soc_pin schmitt_conflict = {
		.pin = 0,
		.function = 0,
		.flags = (uint8_t)(RK3588_PINCTRL_SCHMITT_ENABLE | RK3588_PINCTRL_SCHMITT_DISABLE),
	};

	ioc_clear();
	configure_and_assert(&input_disable, 0);
	zassert_equal(ioc_read(0x0028), 0x00010000U);

	ioc_clear();
	configure_and_assert(&schmitt_disable, 0);
	zassert_equal(ioc_read(0x0030), 0x00010000U);

	ioc_clear();
	configure_and_assert(&input_conflict, -EINVAL);
	zassert_equal(ioc_read(0x0028), 0U);

	ioc_clear();
	configure_and_assert(&schmitt_conflict, -EINVAL);
	zassert_equal(ioc_read(0x0030), 0U);
}

ZTEST(rk3588_pinctrl, test_gpio0_d6_pmu2_only)
{
	const struct rk3588_pinctrl_soc_pin pin = {
		.pin = 30, /* GPIO0_D6 */
		.function = 2,
	};

	ioc_clear();
	configure_and_assert(&pin, 0);

	zassert_equal(ioc_read(0x4010), 0x0f000200U);
	zassert_equal(ioc_read(0x801c), 0U);
}

ZTEST(rk3588_pinctrl, test_all_gpio_muxes)
{
	static const uint8_t unsupported_pins[] = {
		12, 31, 68, 69, 86, 87, 126, 127, 151, 158, 159,
	};

	for (uint8_t pin = 0; pin < RK3588_PIN_COUNT; pin++) {
		const struct rk3588_pinctrl_soc_pin gpio = {
			.pin = pin,
			.function = 0,
		};
		int expected_ret = 0;

		for (size_t i = 0; i < ARRAY_SIZE(unsupported_pins); i++) {
			if (pin == unsupported_pins[i]) {
				expected_ret = -ENOTSUP;
				break;
			}
		}

		ioc_clear();
		configure_and_assert(&gpio, expected_ret);
	}
}

ZTEST(rk3588_pinctrl, test_unsupported_reserved_pads)
{
	static const uint8_t pins[] = {
		12, 31, 68, 69, 86, 87, 126, 127, 151, 158, 159,
	};

	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		const struct rk3588_pinctrl_soc_pin pin = {
			.pin = pins[i],
			.function = 0,
			.drive_strength_ohms = 33,
		};
		int ret;

		ioc_clear();
		ret = rk3588_pinctrl_configure_pin(&pin, (uintptr_t)fake_ioc);
		zassert_equal(ret, -ENOTSUP, "pin %u should have no electrical fields: %d",
			      (unsigned int)pins[i], ret);
	}
}

ZTEST(rk3588_pinctrl, test_board_macros_are_supported)
{
	static const uint16_t board_pins[] = {
		RK3588_PIN_UART2_TX_M0,  RK3588_PIN_UART2_RX_M0,  RK3588_PIN_UART2_TX_M2,
		RK3588_PIN_UART2_RX_M2,  RK3588_PIN_UART3_TX_M1,  RK3588_PIN_UART3_RX_M1,
		RK3588_PIN_UART4_TX_M2,  RK3588_PIN_UART4_RX_M2,  RK3588_PIN_UART7_TX_M2,
		RK3588_PIN_UART7_RX_M2,  RK3588_PIN_I2C0_SCL_M1,  RK3588_PIN_I2C0_SDA_M1,
		RK3588_PIN_I2C6_SCL_M0,  RK3588_PIN_I2C6_SDA_M0,  RK3588_PIN_SPI0_MISO_M2,
		RK3588_PIN_SPI0_MOSI_M2, RK3588_PIN_SPI0_CLK_M2,  RK3588_PIN_SPI0_CS0_M2,
		RK3588_PIN_SPI0_CS1_M2,  RK3588_PIN_SPI1_MISO_M1, RK3588_PIN_SPI1_MOSI_M1,
		RK3588_PIN_SPI1_CLK_M1,  RK3588_PIN_SPI1_CS0_M1,  RK3588_PIN_SPI1_CS1_M1,
		RK3588_PIN_SPI3_MISO_M0, RK3588_PIN_SPI3_MOSI_M0, RK3588_PIN_SPI3_CLK_M0,
		RK3588_PIN_PWM2_M1,      RK3588_PIN_PWM5_M2,      RK3588_PIN_PWM6_M2,
		RK3588_PIN_PWM7_IR_M3,   RK3588_PIN_PWM8_M0,      RK3588_PIN_PWM12_M0,
		RK3588_PIN_PWM13_M0,
	};

	for (size_t i = 0; i < ARRAY_SIZE(board_pins); i++) {
		const struct rk3588_pinctrl_soc_pin pin = {
			.pin = RK3588_DT_PIN(board_pins[i]),
			.function = RK3588_PINMUX_FUNCTION(board_pins[i]),
		};
		int ret;

		ioc_clear();
		ret = rk3588_pinctrl_configure_pin(&pin, (uintptr_t)fake_ioc);
		zassert_equal(ret, 0, "board pin %zu (0x%04x), pin %u, function %u: %d", i,
			      (unsigned int)board_pins[i], (unsigned int)pin.pin,
			      (unsigned int)pin.function, ret);
	}
}

ZTEST(rk3588_pinctrl, test_invalid_requests)
{
	const struct rk3588_pinctrl_soc_pin reserved_mux = {
		.pin = 12, /* GPIO0_B4 */
		.function = 1,
	};
	const struct rk3588_pinctrl_soc_pin invalid_drive = {
		.pin = 72, /* GPIO2_B0, 2-bit drive field */
		.function = 0,
		.bias = RK3588_PINCTRL_BIAS_PULL_UP,
		.drive_strength_ohms = 66,
	};
	const struct rk3588_pinctrl_soc_pin invalid_mux = {
		.pin = 13, /* GPIO0_B5 */
		.function = 15,
		.bias = RK3588_PINCTRL_BIAS_PULL_UP,
	};

	ioc_clear();
	configure_and_assert(&reserved_mux, -ENOTSUP);
	ioc_clear();
	configure_and_assert(&invalid_drive, -ENOTSUP);
	zassert_equal(ioc_read(0xa124), 0U);
	ioc_clear();
	configure_and_assert(&invalid_mux, -EINVAL);
	zassert_equal(ioc_read(0x4028), 0U);
}

ZTEST_SUITE(rk3588_pinctrl, NULL, NULL, NULL, NULL, NULL);
