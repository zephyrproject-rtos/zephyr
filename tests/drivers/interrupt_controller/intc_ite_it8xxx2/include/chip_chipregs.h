/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_INTC_ITE_IT8XXX2_CHIP_CHIPREGS_H_
#define ZEPHYR_TESTS_DRIVERS_INTC_ITE_IT8XXX2_CHIP_CHIPREGS_H_

#include <../soc/ite/ec/it8xxx2/chip_chipregs.h>
#include <soc_common.h>

extern volatile uint8_t fake_intc_regs[0x100];
extern struct wdt_it8xxx2_regs fake_wdt_regs;
extern struct gctrl_it8xxx2_regs fake_gctrl_regs;
extern volatile uint8_t fake_smb_slviselr;

void soc_interrupt_init(void);

#undef ECREG
#define ECREG(x) (fake_intc_regs[(uintptr_t)(x) - (EC_REG_BASE_ADDR + 0x3F00)])

#undef WDT_IT8XXX2_REGS_BASE
#define WDT_IT8XXX2_REGS_BASE (&fake_wdt_regs)

#undef GCTRL_IT8XXX2_REGS_BASE
#define GCTRL_IT8XXX2_REGS_BASE (&fake_gctrl_regs)

#undef IT8XXX2_SMB_SLVISELR
#define IT8XXX2_SMB_SLVISELR (fake_smb_slviselr)

#endif /* ZEPHYR_TESTS_DRIVERS_INTC_ITE_IT8XXX2_CHIP_CHIPREGS_H_ */
