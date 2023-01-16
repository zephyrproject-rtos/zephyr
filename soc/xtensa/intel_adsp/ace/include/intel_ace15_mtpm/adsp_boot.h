/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_BOOT_H_
#define ZEPHYR_SOC_INTEL_ADSP_BOOT_H_

#define DSPCS_REG					0x178d00

struct dspcs {
	/*
	 * DSPCSx
	 * DSP Core Shim
	 *
	 * These registers are added by Intel outside of the Tensilica Core for general operation
	 * control, such as reset, stall, power gating, clock gating etc.
	 * Note: These registers are accessible through the host space or DSP space depending on
	 * ownership, as governed by SAI and RS.
	 */
	struct {
		uint32_t cap;
		uint32_t ctl;
	} capctl[3];
	uint32_t unused0[10];

	/*
	 * DSPBRx
	 * DSP Boot / Recovery
	 *
	 * These registers are added by Intel outside of the Tensilica Core for boot / recovery
	 * control, such as boot path, watch dog timer etc.
	 */
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


#define DSPCS_CTL_SPA					BIT(0)
#define DSPCS_CTL_CPA					BIT(8)

#define DSPBR_BCTL_BYPROM				BIT(0)
#define DSPBR_BCTL_WAITIPCG				BIT(16)
#define DSPBR_BCTL_WAITIPPG				BIT(17)

#define DSPBR_BATTR_LPSCTL_RESTORE_BOOT			BIT(12)
#define DSPBR_BATTR_LPSCTL_HP_CLOCK_BOOT		BIT(13)
#define DSPBR_BATTR_LPSCTL_LP_CLOCK_BOOT		BIT(14)
#define DSPBR_BATTR_LPSCTL_L1_MIN_WAY			BIT(15)
#define DSPBR_BATTR_LPSCTL_BATTR_SLAVE_CORE		BIT(16)

#define DSPBR_WDT_RESUME				BIT(8)
#define DSPBR_WDT_RESTART_COMMAND			0x76

#define DSPCS (*(volatile struct dspcs *)DSPCS_REG)

#endif /* ZEPHYR_SOC_INTEL_ADSP_BOOT_H_ */
