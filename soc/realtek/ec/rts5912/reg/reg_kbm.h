/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_KBM_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_KBM_H

/**
 * @brief Keyboard Matrix Controller (KBM)
 */

struct kbm_regs {
	uint32_t scan_out;
	uint32_t scan_in;
	uint32_t int_en;
	uint32_t ctrl;
};

#define KBM_CTRL_KSOTYPE_Msk   BIT(0)
#define KBM_CTRL_KSI8EN_Msk    BIT(1)
#define KBM_CTRL_KSI9EN_Msk    BIT(2)
#define KBM_CTRL_KSO18EN_Msk   BIT(16)
#define KBM_CTRL_KSO19EN_Msk   BIT(17)
#define KBM_CTRL_KSIINTSTS_Msk BIT(18)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_KBM_H */
