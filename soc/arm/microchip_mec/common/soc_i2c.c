/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include "soc_i2c.h"

/* pinctrl Node contains the base address of the GPIO Control Registers */
#define MCHP_XEC_GPIO_REG_BASE	((struct gpio_regs *)DT_REG_ADDR(DT_NODELABEL(pinctrl)))

/* Too many MEC1501 HAL bugs */
#ifndef MEC_I2C_PORT_MASK
#define MEC_I2C_PORT_MASK	0xFFFFU
#endif

/* encode GPIO pin number and alternate function for an I2C port */
struct mec_i2c_port {
	uint8_t scl_pin_no;
	uint8_t scl_func;
	uint8_t sda_pin_no;
	uint8_t sda_func;
};

/*
 * indexed by port number: all on VTR1 except as commented
 * NOTE: MCHP MECxxxx data sheets specify GPIO pin numbers in octal.
 * TODO: MEC15xx and MEC172x handle ports with alternate pins.
 */
static const struct mec_i2c_port mec_i2c_ports[] = {
#if defined(CONFIG_SOC_SERIES_MEC172X) || defined(CONFIG_SOC_SERIES_MEC1501X)
	{ 0004, 1, 0003, 1 },
	{ 0131, 1, 0130, 1 }, /* VTR2. ALT on eSPI VTR3 {0073, 2, 0072, 2} */
	{ 0155, 1, 0154, 1 },
	{ 0010, 1, 0007, 1 },
	{ 0144, 1, 0143, 1 },
	{ 0142, 1, 0141, 1 },
	{ 0140, 1, 0132, 1 },
	{ 0013, 1, 0012, 1 }, /* VTR2. ALT { 0024, 3, 0152, 3 } VTR1 */
#if defined(CONFIG_SOC_SERIES_MEC172X)
	{ 0230, 1, 0231, 1 }, /* VTR2 176 pin only */
#else
	{ 0212, 1, 0211, 1 }, /* VTR1 MEC1523 SZ and 3Y only */
#endif
	{ 0146, 1, 0145, 1 },
	{ 0107, 3, 0030, 2 },
	{ 0062, 2, 0000, 3 }, /* SCL on VTR1, SDA on VBAT. ALT 176 pin only */
	{ 0027, 3, 0026, 3 },
	{ 0065, 2, 0066, 2 }, /* VTR3 */
	{ 0071, 2, 0070, 2 }, /* VTR3 */
	{ 0150, 1, 0147, 1 }
#endif
};

/*
 * Read pin states of specified I2C port.
 * We GPIO control register always active RO pad input bit.
 * lines b[0]=SCL pin state at pad, b[1]=SDA pin state at pad
 */
int soc_i2c_port_lines_get(uint8_t port, uint32_t *lines)
{
	struct gpio_regs *regs = MCHP_XEC_GPIO_REG_BASE;
	uint32_t idx_scl = 0;
	uint32_t idx_sda = 0;
	uint32_t pinval = 0;

	if (!(BIT(port) & MEC_I2C_PORT_MASK) || !lines) {
		return -EINVAL;
	}

	idx_scl = (uint32_t)mec_i2c_ports[port].scl_pin_no;
	idx_sda = (uint32_t)mec_i2c_ports[port].sda_pin_no;

	if ((idx_scl == 0xFF) || (idx_sda == 0xFF)) {
		return -EINVAL;
	}

	if (regs->CTRL[idx_scl] & BIT(MCHP_GPIO_CTRL_INPAD_VAL_POS)) {
		pinval |= BIT(SOC_I2C_SCL_POS);
	}
	if (regs->CTRL[idx_sda] & BIT(MCHP_GPIO_CTRL_INPAD_VAL_POS)) {
		pinval |= BIT(SOC_I2C_SDA_POS);
	}

	*lines = pinval;

	return 0;
}
