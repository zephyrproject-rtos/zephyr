/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

/*
 * A controller has to take its slave select input from SSI_CTRL rather than
 * the pin, or it stays selected as a target. Bit n selects the register for
 * instance n, bit n + 8 holds the value fed to it.
 */
#define CLKCTL_PER_SLV_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(clockctrl), clkctl_per_slv)
#define CLKCTL_PER_SLV_SIZE DT_REG_SIZE_BY_NAME(DT_NODELABEL(clockctrl), clkctl_per_slv)
#define SSI_CTRL_OFFSET     0x28U

#define SSI_IS_TARGET(n) DT_PROP(DT_NODELABEL(spi##n), serial_target)

#define SSI_CONTROLLER_BITS(n)                                                                     \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(spi##n), okay),                                \
		    (COND_CODE_1(SSI_IS_TARGET(n), (0U), (BIT(n) | BIT((n) + 8)))), (0U))

static int alif_ssi_init(void)
{
	const uint32_t controllers = SSI_CONTROLLER_BITS(0) | SSI_CONTROLLER_BITS(1) |
				     SSI_CONTROLLER_BITS(2) | SSI_CONTROLLER_BITS(3);

	if (controllers != 0U) {
		mm_reg_t base;

		device_map(&base, CLKCTL_PER_SLV_ADDR, CLKCTL_PER_SLV_SIZE, K_MEM_CACHE_NONE);
		sys_set_bits(base + SSI_CTRL_OFFSET, controllers);
		device_unmap(base, CLKCTL_PER_SLV_SIZE);
	}

	return 0;
}

SYS_INIT(alif_ssi_init, PRE_KERNEL_1, 0);
