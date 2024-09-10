/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/pinctrl/npcx-pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include "soc_gpio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pimux_npcx, LOG_LEVEL_ERR);

/* Driver config */
struct npcx_scfg_config {
	/* scfg device base address */
	uintptr_t base_scfg;
	uintptr_t base_dbg;
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
#define NPCX_NO_GPIO_ALT_ITEM(node_id, prop, idx) {				\
	  .group = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, group),	\
	  .bit = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, bit),		\
	  .inverted = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, inv),	\
	},

static const struct npcx_alt def_alts[] = {
	DT_FOREACH_PROP_ELEM(DT_INST(0, nuvoton_npcx_pinctrl_def), pinmux,
				NPCX_NO_GPIO_ALT_ITEM)
};

static const struct npcx_scfg_config npcx_scfg_cfg = {
	.base_scfg = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg),
	.base_dbg = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), dbg),
	.base_glue = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), glue),
};

/* Driver convenience defines */
#define HAL_SFCG_INST() (struct scfg_reg *)(npcx_scfg_cfg.base_scfg)

#define HAL_GLUE_INST() (struct glue_reg *)(npcx_scfg_cfg.base_glue)

/* Pin-control local functions */
static void npcx_pinctrl_alt_sel(const struct npcx_alt *alt, int alt_func)
{
	const uint32_t scfg_base = npcx_scfg_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * alt_fun == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (!!alt_func != !!alt->inverted) {
		NPCX_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NPCX_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}
}

/* Platform specific pin-control functions */
void npcx_lvol_set_detect_level(int lvol_ctrl, int lvol_bit, bool enable)
{
	const uintptr_t scfg_base = npcx_scfg_cfg.base_scfg;

	if (enable) {
		NPCX_LV_GPIO_CTL(scfg_base, lvol_ctrl) |= BIT(lvol_bit);
	} else {
		NPCX_LV_GPIO_CTL(scfg_base, lvol_ctrl) &= ~BIT(lvol_bit);
	}
}

bool npcx_lvol_get_detect_level(int lvol_ctrl, int lvol_bit)
{
	const uintptr_t scfg_base = npcx_scfg_cfg.base_scfg;

	return NPCX_LV_GPIO_CTL(scfg_base, lvol_ctrl) & BIT(lvol_bit);
}

void npcx_pinctrl_i2c_port_sel(int controller, int port)
{
	struct glue_reg *const inst_glue = HAL_GLUE_INST();

	if (port != 0) {
		inst_glue->SMB_SEL |= BIT(controller);
	} else {
		inst_glue->SMB_SEL &= ~BIT(controller);
	}
}

int npcx_pinctrl_flash_write_protect_set(void)
{
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	inst_scfg->DEV_CTL4 |= BIT(NPCX_DEV_CTL4_WP_IF);
	if (!IS_BIT_SET(inst_scfg->DEV_CTL4, NPCX_DEV_CTL4_WP_IF)) {
		return -EIO;
	}

	return 0;
}

bool npcx_pinctrl_flash_write_protect_is_set(void)
{
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	return IS_BIT_SET(inst_scfg->DEV_CTL4, NPCX_DEV_CTL4_WP_IF);
}

void npcx_host_interface_sel(enum npcx_hif_type hif_type)
{
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	SET_FIELD(inst_scfg->DEVCNT, NPCX_DEVCNT_HIF_TYP_SEL_FIELD, hif_type);
}

void npcx_dbg_freeze_enable(bool enable)
{
	const uintptr_t dbg_base = npcx_scfg_cfg.base_dbg;

	if (enable) {
		NPCX_DBGFRZEN3(dbg_base) &= ~BIT(NPCX_DBGFRZEN3_GLBL_FRZ_DIS);
	} else {
		NPCX_DBGFRZEN3(dbg_base) |= BIT(NPCX_DBGFRZEN3_GLBL_FRZ_DIS);
	}
}

/* Pin-control driver registration */
void scfg_init(void)
{
	/* If booter doesn't set the host interface type */
	if (!NPCX_BOOTER_IS_HIF_TYPE_SET()) {
		npcx_host_interface_sel(NPCX_HIF_TYPE_ESPI_SHI);
	}

	/* Change all pads whose default functionality isn't IO to GPIO */
	for (int i = 0; i < ARRAY_SIZE(def_alts); i++) {
		npcx_pinctrl_alt_sel(&def_alts[i], 0);
	}
}
