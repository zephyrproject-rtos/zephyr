/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "memc_nxp_flexram.h"
#include <zephyr/dt-bindings/memory-controller/nxp,flexram.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <zephyr/irq.h>

#include "fsl_device_registers.h"


#if defined(CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API)
BUILD_ASSERT(DT_PROP(FLEXRAM_DT_NODE, flexram_has_magic_addr),
		"SOC does not support magic flexram addresses");
#endif

#define BANK_SIZE (DT_PROP(FLEXRAM_DT_NODE, flexram_bank_size) * 1024)
#define NUM_BANKS DT_PROP(FLEXRAM_DT_NODE, flexram_num_ram_banks)

#define IS_CHILD_RAM_TYPE(node_id, compat) DT_NODE_HAS_COMPAT(node_id, compat)
#define DOES_RAM_TYPE_EXIST(compat) \
	DT_FOREACH_CHILD_SEP_VARGS(FLEXRAM_DT_NODE, IS_CHILD_RAM_TYPE, (+), compat)

#if DOES_RAM_TYPE_EXIST(mmio_sram)
#define FIND_OCRAM_NODE(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, mmio_sram), (node_id), ())
#define OCRAM_DT_NODE DT_FOREACH_CHILD(FLEXRAM_DT_NODE, FIND_OCRAM_NODE)
#define OCRAM_START	(DT_REG_ADDR(OCRAM_DT_NODE))
#define OCRAM_END	(OCRAM_START + DT_REG_SIZE(OCRAM_DT_NODE))
#endif /* OCRAM */
#if DOES_RAM_TYPE_EXIST(nxp_imx_dtcm)
#define FIND_DTCM_NODE(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, nxp_imx_dtcm), (node_id), ())
#define DTCM_DT_NODE DT_FOREACH_CHILD(FLEXRAM_DT_NODE, FIND_DTCM_NODE)
#define DTCM_START	(DT_REG_ADDR(DTCM_DT_NODE))
#define DTCM_END	(DTCM_START + DT_REG_SIZE(DTCM_DT_NODE))
#endif /* DTCM */
#if DOES_RAM_TYPE_EXIST(nxp_imx_itcm)
#define FIND_ITCM_NODE(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, nxp_imx_itcm), (node_id), ())
#define ITCM_DT_NODE DT_FOREACH_CHILD(FLEXRAM_DT_NODE, FIND_ITCM_NODE)
#define ITCM_START	(DT_REG_ADDR(ITCM_DT_NODE))
#define ITCM_END	(ITCM_START + DT_REG_SIZE(ITCM_DT_NODE))
#endif /* ITCM */


#ifdef FLEXRAM_RUNTIME_BANKS_USED

#define PLUS_ONE_BANK(node_id, prop, idx) 1
#define COUNT_BANKS \
	DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec, PLUS_ONE_BANK, (+))
BUILD_ASSERT(COUNT_BANKS == NUM_BANKS, "wrong number of flexram banks defined");

#ifdef OCRAM_DT_NODE
#define ADD_BANK_IF_OCRAM(node_id, prop, idx) \
	COND_CODE_1(IS_EQ(DT_PROP_BY_IDX(node_id, prop, idx), FLEXRAM_OCRAM), \
			(BANK_SIZE), (0))
#define OCRAM_TOTAL \
	DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec, ADD_BANK_IF_OCRAM, (+))
BUILD_ASSERT((OCRAM_TOTAL) == DT_REG_SIZE(OCRAM_DT_NODE), "OCRAM node size is wrong");
#endif /* OCRAM */

#ifdef DTCM_DT_NODE
#define ADD_BANK_IF_DTCM(node_id, prop, idx) \
	COND_CODE_1(IS_EQ(DT_PROP_BY_IDX(node_id, prop, idx), FLEXRAM_DTCM), \
			(BANK_SIZE), (0))
#define DTCM_TOTAL \
	DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec, ADD_BANK_IF_DTCM, (+))
BUILD_ASSERT((DTCM_TOTAL) == DT_REG_SIZE(DTCM_DT_NODE), "DTCM node size is wrong");
#endif /* DTCM */

#ifdef ITCM_DT_NODE
#define ADD_BANK_IF_ITCM(node_id, prop, idx) \
	COND_CODE_1(IS_EQ(DT_PROP_BY_IDX(node_id, prop, idx), FLEXRAM_ITCM), \
			(BANK_SIZE), (0))
#define ITCM_TOTAL \
	DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec, ADD_BANK_IF_ITCM, (+))
BUILD_ASSERT((ITCM_TOTAL) == DT_REG_SIZE(ITCM_DT_NODE), "ITCM node size is wrong");
#endif /* ITCM */

#endif /* FLEXRAM_RUNTIME_BANKS_USED */

static FLEXRAM_Type *const base = (FLEXRAM_Type *) DT_REG_ADDR(FLEXRAM_DT_NODE);

#ifdef FLEXRAM_INTERRUPTS_USED
static flexram_callback_t flexram_callback;
static void *flexram_user_data;

void memc_flexram_register_callback(flexram_callback_t callback, void *user_data)
{
	flexram_callback = callback;
	flexram_user_data = user_data;
}

static void nxp_flexram_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (flexram_callback == NULL) {
		return;
	}

#if defined(CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT)
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_OCRAM_ERR_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_OCRAM_ERR_STATUS_MASK;
		flexram_callback(flexram_ocram_access_error, flexram_user_data);
	}
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_DTCM_ERR_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_DTCM_ERR_STATUS_MASK;
		flexram_callback(flexram_dtcm_access_error, flexram_user_data);
	}
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_ITCM_ERR_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_ITCM_ERR_STATUS_MASK;
		flexram_callback(flexram_itcm_access_error, flexram_user_data);
	}
#endif /* CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT */

#if defined(CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API)
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_OCRAM_MAM_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_OCRAM_MAM_STATUS_MASK;
		flexram_callback(flexram_ocram_magic_addr, flexram_user_data);
	}
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_DTCM_MAM_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_DTCM_MAM_STATUS_MASK;
		flexram_callback(flexram_dtcm_magic_addr, flexram_user_data);
	}
	if (base->INT_STATUS & FLEXRAM_INT_STATUS_ITCM_MAM_STATUS_MASK) {
		base->INT_STATUS |= FLEXRAM_INT_STATUS_ITCM_MAM_STATUS_MASK;
		flexram_callback(flexram_itcm_magic_addr, flexram_user_data);
	}
#endif /* CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API */
}

#if defined(CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API)
int memc_flexram_set_ocram_magic_addr(uint32_t ocram_addr)
{
#ifdef OCRAM_DT_NODE
	ocram_addr -= DT_REG_ADDR(OCRAM_DT_NODE);
	if (ocram_addr >= DT_REG_SIZE(OCRAM_DT_NODE)) {
		return -EINVAL;
	}

	base->OCRAM_MAGIC_ADDR &= ~FLEXRAM_OCRAM_MAGIC_ADDR_OCRAM_MAGIC_ADDR_MASK;
	base->OCRAM_MAGIC_ADDR |= FLEXRAM_OCRAM_MAGIC_ADDR_OCRAM_MAGIC_ADDR(ocram_addr);

	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_OCRAM_MAM_STAT_EN_MASK;
	return 0;
#else
	return -ENODEV;
#endif
}

int memc_flexram_set_itcm_magic_addr(uint32_t itcm_addr)
{
#ifdef ITCM_DT_NODE
	itcm_addr -= DT_REG_ADDR(ITCM_DT_NODE);
	if (itcm_addr >= DT_REG_SIZE(ITCM_DT_NODE)) {
		return -EINVAL;
	}

	base->ITCM_MAGIC_ADDR &= ~FLEXRAM_ITCM_MAGIC_ADDR_ITCM_MAGIC_ADDR_MASK;
	base->ITCM_MAGIC_ADDR |= FLEXRAM_ITCM_MAGIC_ADDR_ITCM_MAGIC_ADDR(itcm_addr);

	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_ITCM_MAM_STAT_EN_MASK;
	return 0;
#else
	return -ENODEV;
#endif
}

int memc_flexram_set_dtcm_magic_addr(uint32_t dtcm_addr)
{
#ifdef DTCM_DT_NODE
	dtcm_addr -= DT_REG_ADDR(DTCM_DT_NODE);
	if (dtcm_addr >= DT_REG_SIZE(DTCM_DT_NODE)) {
		return -EINVAL;
	}

	base->DTCM_MAGIC_ADDR &= ~FLEXRAM_DTCM_MAGIC_ADDR_DTCM_MAGIC_ADDR_MASK;
	base->DTCM_MAGIC_ADDR |= FLEXRAM_DTCM_MAGIC_ADDR_DTCM_MAGIC_ADDR(dtcm_addr);

	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_DTCM_MAM_STAT_EN_MASK;
	return 0;
#else
	return -ENODEV;
#endif
}
#endif /* CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API */

#endif /* FLEXRAM_INTERRUPTS_USED */

static int nxp_flexram_init(void)
{
	if (DT_PROP(FLEXRAM_DT_NODE, flexram_tcm_read_wait_mode)) {
		base->TCM_CTRL |= FLEXRAM_TCM_CTRL_TCM_WWAIT_EN_MASK;
	}
	if (DT_PROP(FLEXRAM_DT_NODE, flexram_tcm_write_wait_mode)) {
		base->TCM_CTRL |= FLEXRAM_TCM_CTRL_TCM_RWAIT_EN_MASK;
	}

#if defined(CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT)
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_OCRAM_ERR_SIG_EN_MASK;
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_DTCM_ERR_SIG_EN_MASK;
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_ITCM_ERR_SIG_EN_MASK;
	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_OCRAM_ERR_STAT_EN_MASK;
	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_DTCM_ERR_STAT_EN_MASK;
	base->INT_STAT_EN |= FLEXRAM_INT_STAT_EN_ITCM_ERR_STAT_EN_MASK;
#endif /* CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT */

#if defined(CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API)
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_OCRAM_MAM_SIG_EN_MASK;
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_DTCM_MAM_SIG_EN_MASK;
	base->INT_SIG_EN |= FLEXRAM_INT_SIG_EN_ITCM_MAM_SIG_EN_MASK;
#endif /* CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API */

#ifdef FLEXRAM_INTERRUPTS_USED
	IRQ_CONNECT(DT_IRQN(FLEXRAM_DT_NODE), DT_IRQ(FLEXRAM_DT_NODE, priority),
			nxp_flexram_isr, NULL, 0);
	irq_enable(DT_IRQN(FLEXRAM_DT_NODE));
#endif /* FLEXRAM_INTERRUPTS_USED */

	return 0;
}

SYS_INIT(nxp_flexram_init, EARLY, 0);
