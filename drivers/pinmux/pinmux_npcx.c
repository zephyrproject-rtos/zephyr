/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinmux.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pimux_npcx, LOG_LEVEL_ERR);

/* Driver config */
struct npcx_pinctrl_config {
	/* scfg device base address */
	uintptr_t base;
};

/* Default io list which default functionality are not IOs */
#define DT_DRV_COMPAT nuvoton_npcx_pinctrl_def
static const struct npcx_alt def_alts[] = DT_NPCX_ALT_ITEMS_LIST(0);

static const struct npcx_pinctrl_config npcx_pinctrl_cfg = {
	.base = DT_REG_ADDR(DT_NODELABEL(scfg)),
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) \
	((const struct npcx_pinctrl_config *)(dev)->config)

#define HAL_INSTANCE(dev) \
	(struct scfg_reg *)(DRV_CONFIG(dev)->base)

/* Pin-control local functions */
static void npcx_pinctrl_alt_sel(const struct npcx_alt *alt, int alt_func)
{
	uint32_t scfg_base = npcx_pinctrl_cfg.base;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * alt_fun == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (!!alt_func != !!alt->inverted)
		NPCX_DEVALT(scfg_base, alt->group) |=  alt_mask;
	else
		NPCX_DEVALT(scfg_base, alt->group) &= ~alt_mask;
}

/* Soc specific pin-control functions */
void soc_pinctrl_mux_configure(const struct npcx_alt *alts_list,
		      uint8_t alts_size, int altfunc)
{
	int i;

	for (i = 0; i < alts_size; i++, alts_list++) {
		npcx_pinctrl_alt_sel(alts_list, altfunc);
	}
}

/* Pin-control driver registration */
static int npcx_pinctrl_init(const struct device *dev)
{
	struct scfg_reg *inst = HAL_INSTANCE(dev);

#if defined(CONFIG_SOC_SERIES_NPCX7)
	/*
	 * Set bit 7 of DEVCNT again for npcx7 series. Please see Errata
	 * for more information. It will be fixed in next chip.
	 */
	inst->DEVCNT |= BIT(7);
#endif

	/* Change all pads whose default functionality isn't IO to GPIO */
	soc_pinctrl_mux_configure(def_alts, ARRAY_SIZE(def_alts), 0);

	return 0;
}

DEVICE_AND_API_INIT(npcx_pinctrl,
		    DT_LABEL(DT_NODELABEL(scfg)),
		    &npcx_pinctrl_init,
		    NULL, &npcx_pinctrl_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    NULL);
