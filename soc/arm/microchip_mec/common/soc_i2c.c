/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <soc.h>
#include "soc_i2c.h"

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
 * NOTE: MCHP MECxxxx data sheets specificy GPIO pin numbers in octal.
 * TODO: MEC15xx and MEC172x handle ports with alternate pins.
 */
static const struct mec_i2c_port mec_i2c_ports[] = {
#if defined(CONFIG_SOC_SERIES_MEC172X) || defined(CONFIG_SOC_SERIES_MEC1501X)
	{ 0004, 1, 0003, 1 },
	{ 0131, 1, 0130, 1 }, /* VTR2. ALT on eSPI VTR3 { 0073, 2, 0072, 2 } */
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
#elif defined(CONFIG_SOC_SERIES_MEC1701X)
	{ 0004, 1, 0003, 1 },
	{ 0006, 1, 0005, 1 },
	{ 0155, 1, 0154, 1 },
	{ 0010, 1, 0007, 1 },
	{ 0144, 1, 0143, 1 },
	{ 0142, 1, 0141, 1 },
	{ 0140, 1, 0132, 1 }, /* VTR2 */
	{ 0013, 1, 0012, 1 }, /* VTR2 */
	{ 0150, 1, 0147, 1 },
	{ 0146, 1, 0145, 1 },
	{ 0131, 1, 0130, 1 }, /* VTR2 */
	{ 0xFF, 0, 0xFF, 0 }, /* No I2C Ports 11 - 15 */
	{ 0xFF, 0, 0xFF, 0 },
	{ 0xFF, 0, 0xFF, 0 },
	{ 0xFF, 0, 0xFF, 0 },
	{ 0xFF, 0, 0xFF, 0 }
#endif
};

/*
 * Read pin states of specified I2C port.
 * We GPIO control register always active RO pad input bit.
 * lines b[0]=SCL pin state at pad, b[1]=SDA pin state at pad
 */
int soc_i2c_port_lines_get(uint8_t port, uint32_t *lines)
{
	int rc;
	uint32_t scl = 0U, sda = 0U;

	if (!(BIT(port) & MEC_I2C_PORT_MASK) || !lines) {
		return -EINVAL;
	}

	*lines = 0U;

	rc = soc_gpio_input((uint32_t)mec_i2c_ports[port].scl_pin_no, &scl);
	if (rc) {
		return rc;
	}

	*lines |= scl;

	rc = soc_gpio_input((uint32_t)mec_i2c_ports[port].sda_pin_no, &sda);
	if (rc) {
		return rc;
	}

	*lines |= (sda << 1);

	return 0;
}

/*
 * For enable should we set:
 * buffer type to open drain
 * output state to High
 * output enable
 * function to I2C
 * For disable:
 * function to GPIO
 */
int soc_i2c_port_ctrl(uint8_t port, uint8_t enable)
{
	int rc;
	uint32_t output_enable = 0;
	uint32_t scl_func = MCHP_GPIO_CFG_FUNC_GPIO;
	uint32_t sda_func = MCHP_GPIO_CFG_FUNC_GPIO;

	if (!(BIT(port) & MEC_I2C_PORT_MASK)) {
		return -EINVAL;
	}

	if (enable) {
		scl_func = (uint32_t)mec_i2c_ports[port].scl_func;
		sda_func = (uint32_t)mec_i2c_ports[port].sda_func;
		output_enable = 1;
	}

	rc = soc_gpio_func_set((uint32_t)mec_i2c_ports[port].scl_pin_no,
				scl_func);
	rc = soc_gpio_func_set((uint32_t)mec_i2c_ports[port].sda_pin_no,
				sda_func);

	return 0;
}

uint32_t soc_i2c_port_pinctrl_addr(uint8_t port, uint8_t pin)
{
	if (!(BIT(port) & MEC_I2C_PORT_MASK)) {
		return 0U;
	}

	if (pin == SOC_I2C_SCL) {
		return MCHP_GPIO_CTRL_BASE +
			((uint32_t)mec_i2c_ports[port].scl_pin_no * 4U);
	} else if (pin == SOC_I2C_SDA) {
		return MCHP_GPIO_CTRL_BASE +
			((uint32_t)mec_i2c_ports[port].sda_pin_no * 4U);
	} else {
		return 0U;
	}
}
