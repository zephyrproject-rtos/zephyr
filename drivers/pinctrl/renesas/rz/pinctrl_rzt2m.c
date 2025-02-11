/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rzt2m_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define PORT_NSR DT_INST_REG_ADDR_BY_NAME(0, port_nsr)
#define PTADR    DT_INST_REG_ADDR_BY_NAME(0, ptadr)

/* Port m mode control register */
#define PMC(port)        (PORT_NSR + 0x400 + port)
/* Port m function control register */
#define PFC(port)        (PORT_NSR + 0x600 + (0x4 * port))
/* IO Buffer m function switching register */
#define DRCTL(port, pin) (PORT_NSR + 0xa00 + (0x8 * port) + pin)
/* Port m region select register */
#define RSELP(port)      (PTADR + port)

#define DRCTL_DRIVE_STRENGTH(val) (val & 0x3)
#define DRCTL_PULL_UP_DOWN(val)   ((val & 0x3) << 2)
#define DRCTL_SCHMITT(val)        ((val & 0x1) << 4)
#define DRCTL_SLEW_RATE(val)      ((val & 0x1) << 5)
#define DRCTL_CONFIG(drive, pull, schmitt, slew)                                                   \
	(DRCTL_DRIVE_STRENGTH(drive) | DRCTL_PULL_UP_DOWN(pull) | DRCTL_SCHMITT(schmitt) |         \
	 DRCTL_SLEW_RATE(slew))
#define PFC_FUNC_MASK(pin) (0xf << (pin * 4))

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint8_t rselp = sys_read8(RSELP(pin->port));
	uint32_t pfc = sys_read32(PFC(pin->port)) & ~(PFC_FUNC_MASK(pin->pin));
	uint8_t pmc = sys_read8(PMC(pin->port));

	/* Set proper bit in the RSELP register to use as non-safety domain. */
	sys_write8(rselp | BIT(pin->pin), RSELP(pin->port));
	sys_write8(DRCTL_CONFIG(
		pin->drive_strength, (pin->pull_up == 1 ? 1U : (pin->pull_down == 1 ? 2U : 0)),
		pin->schmitt_enable, pin->slew_rate),
		  DRCTL(pin->port, pin->pin));

	/* Select function for the pin. */
	sys_write32(pfc | pin->func << (pin->pin * 4), PFC(pin->port));

	/* Set proper bit in the PMC register to use the pin as a peripheral IO. */
	sys_write8(pmc | BIT(pin->pin), PMC(pin->port));
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
