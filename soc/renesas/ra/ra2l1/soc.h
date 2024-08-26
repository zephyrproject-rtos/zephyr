/*
 * Copyright (c) 2021-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RA2L1_SOC_H__
#define __RA2L1_SOC_H__

#include <zephyr/arch/common/sys_io.h>
#include <cmsis_core_m_defaults.h>
#include <ofs_regs_ra.h>

#define SYSC_BASE  ((mm_reg_t)DT_REG_ADDR_BY_NAME(DT_NODELABEL(sysc), sysc))

/* REGISTER PROTECTION
 * 1 -> Write enabled
 * 0 -> Write disabled
 */
#define SYSC_PRCR	(SYSC_BASE + 0x3fe)
#define SYSC_PRCR_CLK_PROT	BIT(0)
#define SYSC_PRCR_LP_PROT	BIT(1)
#define SYSC_PRCR_LVD_PROT	BIT(3)
#define SYSC_PRCR_PRKEY_Off	8
#define SYSC_PRCR_PRKEY_Msk	GENMASK(15, 8)
#define SYSC_PRCR_PRKEY(x)	\
		(((x)<<SYSC_PRCR_PRKEY_Off) & SYSC_PRCR_PRKEY_Msk)

static ALWAYS_INLINE uint16_t get_register_protection(void)
{
	return sys_read16(SYSC_PRCR);
}

static ALWAYS_INLINE void set_register_protection(uint16_t prcr_val)
{
	prcr_val &= SYSC_PRCR_CLK_PROT | SYSC_PRCR_LP_PROT | SYSC_PRCR_LVD_PROT;
	sys_write16(prcr_val | SYSC_PRCR_PRKEY(0xA5), SYSC_PRCR);
}

void set_memwait(bool enable);
bool get_memwait_state(void);

uint16_t init_reset_status(void);

#endif /* __RA2L1_SOC_H__ */
