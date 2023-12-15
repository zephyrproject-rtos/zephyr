/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <string.h>

#define DT_DRV_COMPAT renesas_ra_pinctrl

#define PORT_NUM 15
#define PIN_NUM  16

enum {
	PWPR_PFSWE_POS = 6,
	PWPR_B0WI_POS = 7,
};

static inline uint32_t pinctrl_ra_read_PmnFPS(size_t port, size_t pin)
{
	return sys_read32(DT_INST_REG_ADDR_BY_NAME(0, pfs) + (port * PIN_NUM + pin) * 4);
}

static inline void pinctrl_ra_write_PmnFPS(size_t port, size_t pin, uint32_t value)
{
	sys_write32(value, DT_INST_REG_ADDR_BY_NAME(0, pfs) + (port * PIN_NUM + pin) * 4);
}

static inline uint8_t pinctrl_ra_read_PMISC_PWPR(size_t port, size_t pin)
{
	return sys_read8(DT_INST_REG_ADDR_BY_NAME(0, pmisc_pwpr));
}

static inline void pinctrl_ra_write_PMISC_PWPR(uint8_t value)
{
	sys_write8(value, DT_INST_REG_ADDR_BY_NAME(0, pmisc_pwpr));
}

static void pinctrl_ra_configure_pfs(const pinctrl_soc_pin_t *pinc)
{
	pinctrl_soc_pin_t pincfg;

	memcpy(&pincfg, pinc, sizeof(pinctrl_soc_pin_t));
	pincfg.pin = 0;
	pincfg.port = 0;

	/* Clear PMR bits before configuring */
	if ((pincfg.config & PmnPFS_PMR_POS)) {
		uint32_t val = pinctrl_ra_read_PmnFPS(pinc->port, pinc->pin);

		pinctrl_ra_write_PmnFPS(pinc->port, pinc->pin, val & ~(BIT(PmnPFS_PMR_POS)));
		pinctrl_ra_write_PmnFPS(pinc->port, pinc->pin, pincfg.config & ~PmnPFS_PMR_POS);
	}

	pinctrl_ra_write_PmnFPS(pinc->port, pinc->pin, pincfg.config);
}

int pinctrl_ra_query_config(uint32_t port, uint32_t pin, struct pinctrl_ra_pin *const pincfg)
{
	if (port >= PORT_NUM || pin >= PIN_NUM) {
		return -EINVAL;
	}

	pincfg->config = pinctrl_ra_read_PmnFPS(port, pin);
	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	pinctrl_ra_write_PMISC_PWPR(0);
	pinctrl_ra_write_PMISC_PWPR(BIT(PWPR_PFSWE_POS));

	for (int i = 0; i < pin_cnt; i++) {
		pinctrl_ra_configure_pfs(&pins[i]);
	}

	pinctrl_ra_write_PMISC_PWPR(0);
	pinctrl_ra_write_PMISC_PWPR(BIT(PWPR_B0WI_POS));

	return 0;
}
