/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

/*
 * A controller has to take its slave select input from SSI_CTRL rather than
 * the pin, or it stays selected as a target. Bit n selects the register for
 * instance n, bit n + 8 holds the value fed to it.
 */
#define SSI_CTRL_REG (DT_REG_ADDR_BY_NAME(DT_NODELABEL(clockctrl), clkctl_per_slv) + 0x28U)

#define SSI_IS_TARGET(n) DT_PROP(DT_NODELABEL(spi##n), serial_target)

#define SSI_CONTROLLER_BITS(n)                                                                     \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(spi##n), okay),                                \
		    (COND_CODE_1(SSI_IS_TARGET(n), (0U), (BIT(n) | BIT((n) + 8)))), (0U))

static int alif_ssi_init(void)
{
	const uint32_t controllers = SSI_CONTROLLER_BITS(0) | SSI_CONTROLLER_BITS(1) |
				     SSI_CONTROLLER_BITS(2) | SSI_CONTROLLER_BITS(3);

	if (controllers != 0U) {
		sys_set_bits(SSI_CTRL_REG, controllers);
	}

	return 0;
}

SYS_INIT(alif_ssi_init, PRE_KERNEL_1, 0);
