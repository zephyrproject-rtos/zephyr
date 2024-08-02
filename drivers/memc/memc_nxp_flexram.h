/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <soc.h>

#define FLEXRAM_DT_NODE    DT_INST(0, nxp_flexram)
#define IOMUXC_GPR_DT_NODE DT_NODELABEL(iomuxcgpr)

#if defined(CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API) || \
	defined(CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT)
#define FLEXRAM_INTERRUPTS_USED
#endif

#if DT_PROP_HAS_IDX(FLEXRAM_DT_NODE, flexram_bank_spec, 0)
#define FLEXRAM_RUNTIME_BANKS_USED 1
#endif

#ifdef FLEXRAM_INTERRUPTS_USED
enum memc_flexram_interrupt_cause {
#ifdef CONFIG_MEMC_NXP_FLEXRAM_ERROR_INTERRUPT
	flexram_ocram_access_error,
	flexram_itcm_access_error,
	flexram_dtcm_access_error,
#endif
#ifdef CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API
	flexram_ocram_magic_addr,
	flexram_itcm_magic_addr,
	flexram_dtcm_magic_addr,
#endif /* CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API */
};

typedef void (*flexram_callback_t)(enum memc_flexram_interrupt_cause, void *user_data);

void memc_flexram_register_callback(flexram_callback_t callback, void *user_data);
#endif /* FLEXRAM_INTERRUPTS_USED */

#ifdef FLEXRAM_RUNTIME_BANKS_USED

/*
 * call from platform_init to set up flexram if using runtime map
 * must be inlined because cannot use stack
 */
#define GPR_FLEXRAM_REG_FILL(node_id, prop, idx) \
	(((uint32_t)DT_PROP_BY_IDX(node_id, prop, idx)) << (2 * idx))
static inline void memc_flexram_dt_partition(void)
{
	/* iomuxc_gpr must be const (in ROM region) because used in reconfiguring ram */
	static IOMUXC_GPR_Type *const iomuxc_gpr =
		(IOMUXC_GPR_Type *)DT_REG_ADDR(IOMUXC_GPR_DT_NODE);
	/* do not create stack variables or use any data from ram in this function */
#if defined(CONFIG_SOC_SERIES_IMXRT11XX)
	iomuxc_gpr->GPR17 = (DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec,
						GPR_FLEXRAM_REG_FILL, (+))) & 0xFFFF;
	iomuxc_gpr->GPR18 = (((DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec,
						GPR_FLEXRAM_REG_FILL, (+)))) >> 16) & 0xFFFF;
#elif defined(CONFIG_SOC_SERIES_IMXRT10XX)
	iomuxc_gpr->GPR17 = DT_FOREACH_PROP_ELEM_SEP(FLEXRAM_DT_NODE, flexram_bank_spec,
						GPR_FLEXRAM_REG_FILL, (+));
#endif
	iomuxc_gpr->GPR16 |= IOMUXC_GPR_GPR16_FLEXRAM_BANK_CFG_SEL_MASK;
}
#endif /* FLEXRAM_RUNTIME_BANKS_USED */

#ifdef CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API
/** @brief Sets magic address for OCRAM
 *
 * Magic address allows core interrupt from FlexRAM when address
 * is accessed.
 *
 * @param ocram_addr: An address in OCRAM to set magic function on.
 * @retval 0 on success
 * @retval -EINVAL if ocram_addr is not in OCRAM
 * @retval -ENODEV if there is no OCRAM allocation in flexram
 */
int memc_flexram_set_ocram_magic_addr(uint32_t ocram_addr);

/** @brief Sets magic address for ITCM
 *
 * Magic address allows core interrupt from FlexRAM when address
 * is accessed.
 *
 * @param itcm_addr: An address in ITCM to set magic function on.
 * @retval 0 on success
 * @retval -EINVAL if itcm_addr is not in ITCM
 * @retval -ENODEV if there is no ITCM allocation in flexram
 */
int memc_flexram_set_itcm_magic_addr(uint32_t itcm_addr);

/** @brief Sets magic address for DTCM
 *
 * Magic address allows core interrupt from FlexRAM when address
 * is accessed.
 *
 * @param dtcm_addr: An address in DTCM to set magic function on.
 * @retval 0 on success
 * @retval -EINVAL if dtcm_addr is not in DTCM
 * @retval -ENODEV if there is no DTCM allocation in flexram
 */
int memc_flexram_set_dtcm_magic_addr(uint32_t dtcm_addr);

#endif /* CONFIG_MEMC_NXP_FLEXRAM_MAGIC_ADDR_API */
