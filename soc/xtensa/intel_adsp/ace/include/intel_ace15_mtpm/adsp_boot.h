/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_BOOT_H_
#define ZEPHYR_SOC_INTEL_ADSP_BOOT_H_

/* Boot / recovery capability function registers */

struct dfdspbrcp {
	struct {
		uint32_t cap;
		uint32_t ctl;
	} capctl[3];
	uint32_t unused0[10];
	struct {
		uint32_t brcap;
		uint32_t wdtcs;
		uint32_t wdtipptr;
		uint32_t unused1;
		uint32_t bctl;
		uint32_t baddr;
		uint32_t battr;
		uint32_t unused2;
	} bootctl[3];
};

/* These registers are added by Intel outside of the Tensilica Core
 * for boot / recovery control, such as boot path, watch dog timer etc.
 */
#define DFDSPBRCP_REG				0x178d00

#define DFDSPBRCP_CTL_SPA			BIT(0)
#define DFDSPBRCP_CTL_CPA			BIT(8)
#define DFDSPBRCP_BCTL_BYPROM		BIT(0)
#define DFDSPBRCP_BCTL_WAITIPCG		BIT(16)
#define DFDSPBRCP_BCTL_WAITIPPG		BIT(17)

#define DFDSPBRCP_BATTR_LPSCTL_RESTORE_BOOT		BIT(12)
#define DFDSPBRCP_BATTR_LPSCTL_HP_CLOCK_BOOT	BIT(13)
#define DFDSPBRCP_BATTR_LPSCTL_LP_CLOCK_BOOT	BIT(14)
#define DFDSPBRCP_BATTR_LPSCTL_L1_MIN_WAY		BIT(15)
#define DFDSPBRCP_BATTR_LPSCTL_BATTR_SLAVE_CORE	BIT(16)

#define DFDSPBRCP_WDT_RESUME		BIT(8)
#define DFDSPBRCP_WDT_RESTART_COMMAND	0x76

#define DFDSPBRCP (*(volatile struct dfdspbrcp *)DFDSPBRCP_REG)

#endif /* ZEPHYR_SOC_INTEL_ADSP_BOOT_H_ */
