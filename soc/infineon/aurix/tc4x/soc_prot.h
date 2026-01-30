/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_PROT_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_PROT_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>

#include "soc_tagid.h"

#ifndef _ASMLANGUAGE

/** \brief Resource Protection Register */
typedef struct _aurix_prot_bits {
	uint32_t STATE: 3;      /**< \brief [2:0] Resource protection state (rwh) */
	uint32_t SWEN: 1;       /**< \brief [3:3] State write enable (w) */
	uint32_t reserved_4: 4; /**< \brief [7:4] \internal Reserved */
	uint32_t SEL: 8;        /**< \brief [8:15] Protection region select */
	uint32_t VM: 3;    /**< \brief [18:16] Virtual Machine definition for PROT owner (rw) */
	uint32_t VMEN: 1;  /**< \brief [19:19] VM definition Enable (rw) */
	uint32_t PRS: 3;   /**< \brief [22:20] Protection Set for PROT owner (rw) */
	uint32_t PRSEN: 1; /**< \brief [23:23] Protection Set Enable (rw) */
	uint32_t TAGID: 6; /**< \brief [29:24] TAG ID definition for PROT owner (rw) */
	uint32_t ODEF: 1;  /**< \brief [30:30] Enable for PROT owner definition (rw) */
	uint32_t OWEN: 1;  /**< \brief [31:31] Owner write enable (w) */
} aurix_prot_bits;

/** \brief Resource Protection Register   */
typedef union {
	uint32_t U;        /**< \brief Unsigned access */
	int32_t I;         /**< \brief Signed access */
	aurix_prot_bits B; /**< \brief Bitfield access */
} aurix_prot_reg;

enum aurix_prop_state {
	AURIX_PROT_STATE_INIT,
	AURIX_PROT_STATE_CONFIG,
	AURIX_PROT_STATE_CONFIGSEC,
	AURIX_PROT_STATE_CHECKSEC,
	AURIX_PROT_STATE_RUN,
	AURIX_PROT_STATE_RUNSEC,
	AURIX_PROT_STATE_RUNLOCK,
};

static inline void aurix_prot_own(volatile void *prot)
{
	aurix_prot_reg prot_val = {.U = sys_read32((mem_addr_t)prot)};

	prot_val.B.OWEN = 1;
	prot_val.B.ODEF = 1;
	prot_val.B.VM = COND_CODE_1(CONFIG_TRICORE_VIRTUALIZATION, (CONFIG_TRICORE_VM_ID), (0));
	prot_val.B.VMEN = IS_ENABLED(CONFIG_TRICORE_VIRTUALIZATION) ? 1 : 0;
	prot_val.B.TAGID = aurix_coreid_to_tagid(arch_proc_id());
	sys_write32(prot_val.U, (mem_addr_t)prot);
	barrier_dsync_fence_full();
}

static inline void aurix_prot_set_owner(volatile void *prot, uint8_t tag_id, bool vm_enable,
					uint8_t vm_id, bool prs_enable, uint8_t prs)
{
	aurix_prot_reg prot_val = {.U = sys_read32((mem_addr_t)prot)};

	prot_val.B.OWEN = 1;
	prot_val.B.ODEF = 1;
	prot_val.B.VM = vm_id;
	prot_val.B.VMEN = vm_enable ? 1 : 0;
	prot_val.B.PRS = prs;
	prot_val.B.PRSEN = prs_enable ? 1 : 0;
	prot_val.B.TAGID = tag_id;
	sys_write32(prot_val.U, (mem_addr_t)prot);
	barrier_dsync_fence_full();
}

static inline void aurix_prot_set_state(volatile void *prot, enum aurix_prop_state state)
{
	aurix_prot_reg prot_val = {.U = sys_read32((mem_addr_t)prot)};

	prot_val.B.SWEN = 1;
	prot_val.B.STATE = state;
	sys_write32(prot_val.U, (mem_addr_t)prot);
	barrier_dsync_fence_full();
}

static inline void aurix_prot_set_state_select(volatile void *prot, enum aurix_prop_state state,
					       uint8_t sel)
{
	aurix_prot_reg prot_val = {.U = sys_read32((mem_addr_t)prot)};

	prot_val.B.SWEN = 1;
	prot_val.B.SEL = sel;
	prot_val.B.STATE = state;
	sys_write32(prot_val.U, (mem_addr_t)prot);
	barrier_dsync_fence_full();
}

static inline enum aurix_prop_state aurix_prot_get_state(volatile void *prot)
{
	return ((volatile aurix_prot_reg *)prot)->B.STATE;
}

static inline void aurix_prot_set_select(volatile void *prot, uint8_t sel)
{
	aurix_prot_reg prot_val = {.U = sys_read32((mem_addr_t)prot)};

	prot_val.B.SEL = sel;
	sys_write32(prot_val.U, (mem_addr_t)prot);
}

static inline void aurix_apu_enable_write(volatile void *prot, volatile void *wr, uint8_t tag_id)
{
	atomic_ptr_t atomic_wr = ATOMIC_PTR_INIT((void *)wr);

	if (aurix_prot_get_state(prot) == AURIX_PROT_STATE_INIT) {
		atomic_set_bit(atomic_wr, tag_id);
	} else {
		aurix_prot_set_state(prot, AURIX_PROT_STATE_CONFIG);
		atomic_set_bit(atomic_wr, tag_id);
		aurix_prot_set_state(prot, AURIX_PROT_STATE_RUN);
	}
}

static inline void aurix_apu_enable_write_select(volatile void *prot, volatile void *wr,
						 uint8_t sel, uint8_t tag_id)
{
	atomic_ptr_t atomic_wr = ATOMIC_PTR_INIT((void *)wr);

	if (aurix_prot_get_state(prot) == AURIX_PROT_STATE_INIT) {
		aurix_prot_set_state(prot, AURIX_PROT_STATE_RUN);
	}
	aurix_prot_set_state_select(prot, AURIX_PROT_STATE_CONFIG, sel);
	atomic_set_bit(atomic_wr, tag_id);
	aurix_prot_set_state(prot, AURIX_PROT_STATE_RUN);
}

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_PROT_H_ */
