/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SAM_DT_RSTC_DRIVER DT_INST(0, atmel_sam_rstc)

#include <zephyr/kernel.h>
#if defined(CONFIG_REBOOT)
#include <zephyr/sys/reboot.h>
#endif

#if defined(CONFIG_REBOOT)
#if DT_NODE_HAS_STATUS_OKAY(SAM_DT_RSTC_DRIVER)

void sys_arch_reboot(int type)
{
	Rstc *regs = (Rstc *)DT_REG_ADDR(SAM_DT_RSTC_DRIVER);

	switch (type) {
	case SYS_REBOOT_COLD:
		regs->RSTC_CR = RSTC_CR_KEY_PASSWD
			      | RSTC_CR_PROCRST
#if defined(CONFIG_SOC_SERIES_SAM3X) || defined(CONFIG_SOC_SERIES_SAM4S) || \
	defined(CONFIG_SOC_SERIES_SAM4E)
			      | RSTC_CR_PERRST
#endif /* CONFIG_SOC_SERIES_SAM3X || CONFIG_SOC_SERIES_SAM4S || CONFIG_SOC_SERIES_SAM4E */
			      ;
		break;
	default:
		break;
	}
}

#endif /* DT_NODE_HAS_STATUS_OKAY */
#endif /* CONFIG_REBOOT */
