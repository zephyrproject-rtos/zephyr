/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/drivers/mfd/bflb-system-control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bflb_reset, CONFIG_KERNEL_LOG_LEVEL);

#include <bflb_soc.h>
#include <aon_reg.h>
#include <glb_reg.h>
#include <pds_reg.h>
#include <hbn_reg.h>


void sys_arch_reboot(int type)
{
	uint32_t tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG2_OFFSET);
	const struct device *aon_scratch0_dev = DEVICE_DT_GET(DT_NODELABEL(aon_scratch0));
	uint32_t aon_scratch0;

	(void)retained_mem_read(aon_scratch0_dev, 0, (uint8_t *)&aon_scratch0, sizeof(uint32_t));

	tmp &= ~(GLB_REG_CTRL_SYS_RESET_MSK | GLB_REG_CTRL_CPU_RESET_MSK
		 | GLB_REG_CTRL_PWRON_RST_MSK);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);

	switch (type) {
	case SYS_REBOOT_COLD:
		/* TODO: Should trigger reset for various peripherals not covered automatically
		 * via a future reset controller
		 */
		tmp |= GLB_REG_CTRL_PWRON_RST_MSK;
		/* No differences on E24 platforms */
#if !(defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X) \
	|| defined(CONFIG_SOC_SERIES_BL70XL))
		break;
#endif
	case SYS_REBOOT_WARM:
	default:
		tmp |= GLB_REG_CTRL_CPU_RESET_MSK | GLB_REG_CTRL_SYS_RESET_MSK;
		break;
	}

	aon_scratch0 |= AON_SCRATCH0_FLAG_SW_REBOOT;
	(void)retained_mem_write(aon_scratch0_dev, 0, (uint8_t *)&aon_scratch0, sizeof(uint32_t));
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);

	while (true) {
	}

	CODE_UNREACHABLE;
}
