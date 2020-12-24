/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_pinctrl

#include <drivers/pinmux.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pimux_npcx, LOG_LEVEL_ERR);

/* Driver config */
struct npcx_pinctrl_config {
	/* scfg device base address */
	uintptr_t base_scfg;
	uintptr_t base_glue;
};

/*
 * Get io list which default functionality are not IOs. Then switch them to
 * GPIO in pin-mux init function.
 *
 * def_io_conf: def_io_conf_list {
 *               compatible = "nuvoton,npcx-pinctrl-def";
 *               pinctrl-0 = <&alt0_gpio_no_spip
 *                            &alt0_gpio_no_fpip
 *                            ...>;
 *               };
 */
static const struct npcx_alt def_alts[] =
			NPCX_DT_IO_ALT_ITEMS_LIST(nuvoton_npcx_pinctrl_def, 0);

static const struct npcx_lvol def_lvols[] = NPCX_DT_IO_LVOL_ITEMS_DEF_LIST;

static const struct npcx_pinctrl_config npcx_pinctrl_cfg = {
	.base_scfg = DT_INST_REG_ADDR_BY_NAME(0, scfg),
	.base_glue = DT_INST_REG_ADDR_BY_NAME(0, glue),
};

/* Driver convenience defines */
#define HAL_SFCG_INST() (struct scfg_reg *)(npcx_pinctrl_cfg.base_scfg)

#define HAL_GLUE_INST() (struct glue_reg *)(npcx_pinctrl_cfg.base_glue)

/* Pin-control local functions */
static void npcx_pinctrl_alt_sel(const struct npcx_alt *alt, int alt_func)
{
	const uint32_t scfg_base = npcx_pinctrl_cfg.base_scfg;
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
void npcx_pinctrl_mux_configure(const struct npcx_alt *alts_list,
		      uint8_t alts_size, int altfunc)
{
	int i;

	for (i = 0; i < alts_size; i++, alts_list++) {
		npcx_pinctrl_alt_sel(alts_list, altfunc);
	}
}

void npcx_lvol_pads_configure(void)
{
	const uint32_t scfg_base = npcx_pinctrl_cfg.base_scfg;

	for (int i = 0; i < ARRAY_SIZE(def_lvols); i++) {
		NPCX_LV_GPIO_CTL(scfg_base, def_lvols[i].ctrl)
					|= BIT(def_lvols[i].bit);
		LOG_DBG("IO%x%x turn on low-voltage", def_lvols[i].io_port,
							def_lvols[i].io_bit);
	}
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

/* Pin-control driver registration */
static int npcx_pinctrl_init(const struct device *dev)
{
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

#if defined(CONFIG_SOC_SERIES_NPCX7)
	/*
	 * Set bit 7 of DEVCNT again for npcx7 series. Please see Errata
	 * for more information. It will be fixed in next chip.
	 */
	inst_scfg->DEVCNT |= BIT(7);
#endif

	/* Change all pads whose default functionality isn't IO to GPIO */
	npcx_pinctrl_mux_configure(def_alts, ARRAY_SIZE(def_alts), 0);

	/* Configure default low-voltage pads */
	npcx_lvol_pads_configure();

	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(scfg),
		    &npcx_pinctrl_init,
		    device_pm_control_nop,
		    NULL, &npcx_pinctrl_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    NULL);
