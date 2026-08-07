/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/ztest.h>

/* Pin configuration for test device */
#define TEST_DEVICE DT_NODELABEL(test_device)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE);

#ifdef CONFIG_TEST_PINCTRL_MCHP_MEC

#define XEC_GPIO_PIN_NUM(pinmux)                                                                   \
	(((uint32_t)MCHP_XEC_PINMUX_PORT(pinmux) * 32u) + MCHP_XEC_PINMUX_PIN(pinmux))

#define XEC_GPIO_BASE_ADDR DT_REG_ADDR(DT_NODELABEL(gpio_000_036))

ZTEST(pinctrl_mchp_mec, test_slew_rate)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[1];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t cr2_addr = MEC_GPIO_CR2_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t cr2 = 0;

	zassert_equal(p->slew_rate, MCHP_XEC_SLEW_RATE_SLOW);

	cr2 = sys_read32(cr2_addr);

	zassert_equal(MEC_GPIO_CR2_SLEW_GET(cr2), MEC_GPIO_CR2_SLEW_SLOW);
}

ZTEST(pinctrl_mchp_mec, test_drive_strength)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[2];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t cr2_addr = MEC_GPIO_CR2_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t cr2 = 0;

	zassert_equal(p->drive_str, MCHP_XEC_DRV_STR_4X);

	cr2 = sys_read32(cr2_addr);
	zassert_equal(MEC_GPIO_CR2_DSTR_GET(cr2), MEC_GPIO_CR2_DSTR_4X);
}

ZTEST(pinctrl_mchp_mec, test_pullup)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[3];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t cr1_addr = MEC_GPIO_CR1_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t cr1 = 0;

	zassert_equal(p->pu, 1);

	cr1 = sys_read32(cr1_addr);
	zassert_equal(MEC_GPIO_CR1_PUD_GET(cr1), MEC_GPIO_CR1_PUD_PU);
}

ZTEST(pinctrl_mchp_mec, test_pulldown)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[4];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t cr1_addr = MEC_GPIO_CR1_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t cr1 = 0;

	zassert_equal(p->pd, 1);

	cr1 = sys_read32(cr1_addr);
	zassert_equal(MEC_GPIO_CR1_PUD_GET(cr1), MEC_GPIO_CR1_PUD_PD);
}

ZTEST(pinctrl_mchp_mec, test_output_high)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[0];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t pin_pos = MCHP_XEC_PINMUX_PIN(p->pinmux);
	uint32_t cr1_addr = MEC_GPIO_CR1_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t pout_addr = MEC_GPIO_PP_OUT_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t regval = 0;

	zassert_equal(p->out_en, 1);
	zassert_equal(p->out_hi, 1);

	regval = sys_read32(cr1_addr);
	zassert_equal(MEC_GPIO_CR1_DIR_GET(regval), MEC_GPIO_CR1_DIR_OUT);

	regval = sys_read32(pout_addr);
	zassert_equal(regval & BIT(pin_pos), BIT(pin_pos));
}

ZTEST(pinctrl_mchp_mec, test_open_drain)
{
	const struct pinctrl_state *scfg = &pcfg->states[0];
	const pinctrl_soc_pin_t *p = &scfg->pins[5];
	uint32_t pin = XEC_GPIO_PIN_NUM(p->pinmux);
	uint32_t cr1_addr = MEC_GPIO_CR1_ADDR(XEC_GPIO_BASE_ADDR, pin);
	uint32_t cr1 = 0;

	zassert_equal(p->obuf_od, 1);
	zassert_equal(p->obuf_pp, 0);

	cr1 = sys_read32(cr1_addr);
	zassert_equal(MEC_GPIO_CR1_OBUF_GET(cr1), MEC_GPIO_CR1_OBUF_OD);
}

ZTEST(pinctrl_mchp_mec, test_apply_state)
{
	int ret;

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	zassert_equal(ret, 0);
}

ZTEST_SUITE(pinctrl_mchp_mec, NULL, NULL, NULL, NULL, NULL);

#else
#define MCHP_PINCTRL_FLAG_GET(pincfg, pos) (((pincfg.pinflag) >> pos) & MCHP_PINCTRL_FLAG_MASK)

ZTEST(pinctrl_mchp, test_pullup_pulldown_none)
{
	const struct pinctrl_state *scfg;

	scfg = &pcfg->states[0];

	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[0], MCHP_PINCTRL_PULLUP_POS), 0);
	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[0], MCHP_PINCTRL_PULLDOWN_POS), 0);
	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[1], MCHP_PINCTRL_PULLUP_POS), 0);
	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[1], MCHP_PINCTRL_PULLDOWN_POS), 0);
}

ZTEST(pinctrl_mchp, test_pullup)
{
	const struct pinctrl_state *scfg;

	scfg = &pcfg->states[0];

	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[2], MCHP_PINCTRL_PULLUP_POS), 1);
}

ZTEST(pinctrl_mchp, test_pulldown)
{
	const struct pinctrl_state *scfg;

	scfg = &pcfg->states[0];

	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[3], MCHP_PINCTRL_PULLDOWN_POS), 1);
}

ZTEST(pinctrl_mchp, test_input_output_enable)
{
	const struct pinctrl_state *scfg;

	scfg = &pcfg->states[0];

	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[4], MCHP_PINCTRL_INPUTENABLE_POS), 1);
	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[4], MCHP_PINCTRL_OUTPUTENABLE_POS), 1);
}

#if defined(CONFIG_TEST_PINCTRL_MCHP_SAM)
ZTEST(pinctrl_mchp, test_drive_strength)
{
	const struct pinctrl_state *scfg;

	scfg = &pcfg->states[0];

	zassert_equal(MCHP_PINCTRL_FLAG_GET(scfg->pins[5], MCHP_PINCTRL_DRIVESTRENGTH_POS), 1);
}
#endif

ZTEST(pinctrl_mchp, test_apply_state)
{
	int ret;

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	zassert_equal(ret, 0);
}

ZTEST_SUITE(pinctrl_mchp, NULL, NULL, NULL, NULL, NULL);
#endif /* CONFIG_TEST_PINCTRL_MCHP_MEC */
