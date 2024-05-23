/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/dt-bindings/pinctrl/npcm-pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pimux_npcm, LOG_LEVEL_ERR);

/* Driver config */
struct npcm_scfg_config {
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
#define NPCM_NO_GPIO_ALT_ITEM(node_id, prop, idx) {				\
	  .group = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, group),	\
	  .bit = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, bit),		\
	  .inverted = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), alts, inv),	\
	},

static const struct npcm_alt def_alts[] = {
	DT_FOREACH_PROP_ELEM(DT_INST(0, nuvoton_npcm_pinctrl_def), pinmux,
				NPCM_NO_GPIO_ALT_ITEM)
};

static const struct npcm_scfg_config npcm_scfg_cfg = {
	.base_scfg = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg),
	.base_glue = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), glue),
};

/* Driver convenience defines */
#define HAL_SFCG_INST() (struct scfg_reg *)(npcm_scfg_cfg.base_scfg)

#define HAL_GLUE_INST() (struct glue_reg *)(npcm_scfg_cfg.base_glue)

/* Pin-control local functions */
static void npcm_pinctrl_alt_sel(const struct npcm_alt *alt, int alt_func)
{
	const uint32_t scfg_base = npcm_scfg_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * alt_fun == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (!!alt_func != !!alt->inverted) {
		NPCM_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NPCM_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}
}

/* Pin-control driver registration */
static int npcm_scfg_init(void)
{
	/* Change all pads whose default functionality isn't IO to GPIO */
	for (int i = 0; i < ARRAY_SIZE(def_alts); i++) {
		npcm_pinctrl_alt_sel(&def_alts[i], 0);
	}

	return 0;
}

SYS_INIT(npcm_scfg_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
