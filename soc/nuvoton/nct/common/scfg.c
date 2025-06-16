/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/dt-bindings/pinctrl/nct-pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pimux_nct, LOG_LEVEL_ERR);

/* Driver config */
struct nct_scfg_config {
	/* scfg device base address */
	uintptr_t base_scfg;
	uintptr_t base_glue;
};

/*
 * Get io list which default functionality are not IOs. Then switch them to
 * GPIO in pin-mux init function.
 *
 * def-io-conf-list {
 *               pinmux = <&alt0_gpio_no_spip
 *                         &alt0_gpio_no_fpip
 *                         ...>;
 *               };
 */
#define NCT_NO_GPIO_ALT_ITEM(node_id, prop, idx) {				\
	  .group = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, group),	\
	  .bit = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, bit),		\
	  .inverted = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, inv),	\
	},

static const struct nct_alt def_alts[] = {
	DT_FOREACH_PROP_ELEM(DT_INST(0, nuvoton_nct_pinctrl_def), pinmux,
				NCT_NO_GPIO_ALT_ITEM)
};

static const struct nct_scfg_config nct_scfg_cfg = {
	.base_scfg = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg),
	.base_glue = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), glue),
};

/* Driver convenience defines */
#define HAL_SFCG_INST() (struct scfg_reg *)(nct_scfg_cfg.base_scfg)

#define HAL_GLUE_INST() (struct glue_reg *)(nct_scfg_cfg.base_glue)

/* Pin-control local functions */
static void nct_pinctrl_alt_sel(const struct nct_alt *alt, int alt_func)
{
	const uint32_t scfg_base = nct_scfg_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * alt_fun == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (!!alt_func != !!alt->inverted) {
		NCT_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NCT_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}
}

int nct_pinctrl_flash_write_protect_set(int interface)
{
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	switch (interface) {
		case NCT_FIU_FLASH_WP:
			inst_scfg->DEV_CTL3 |= BIT(NCT_DEV_CTL3_WP_GPIO55);
			if (!IS_BIT_SET(inst_scfg->DEV_CTL3, NCT_DEV_CTL3_WP_GPIO55)) {
				return -EIO;
			}
			break;
		case NCT_SPIM_FLASH_WP:
			inst_scfg->DEV_CTL3 |= BIT(NCT_DEV_CTL3_WP_INT_FL);
			if (!IS_BIT_SET(inst_scfg->DEV_CTL3, NCT_DEV_CTL3_WP_INT_FL)) {
				return -EIO;
			}
			break;
		case NCT_SPIP_FLASH_WP:
			inst_scfg->DEV_CTL3 |= BIT(NCT_DEV_CTL3_WP_GPIO76);
			if (!IS_BIT_SET(inst_scfg->DEV_CTL3, NCT_DEV_CTL3_WP_GPIO76)) {
				return -EIO;
			}
			break;
		default:
			return -EIO;
	}

        return 0;
}

void nct_dbg_freeze_enable(bool enable)
{
	const uintptr_t scfg_base = nct_scfg_cfg.base_scfg;

	if (enable) {
		NCT_DBGFRZEN3(scfg_base) &= ~BIT(NCT_DBGFRZEN3_GLBL_FRZ_DIS);
	} else {
		NCT_DBGFRZEN3(scfg_base) |= BIT(NCT_DBGFRZEN3_GLBL_FRZ_DIS);
	}
}

/* Pin-control driver registration */
static int nct_scfg_init(void)
{
	/* Change all pads whose default functionality isn't IO to GPIO */
	for (int i = 0; i < ARRAY_SIZE(def_alts); i++) {
		nct_pinctrl_alt_sel(&def_alts[i], 0);
	}

	return 0;
}

SYS_INIT(nct_scfg_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
