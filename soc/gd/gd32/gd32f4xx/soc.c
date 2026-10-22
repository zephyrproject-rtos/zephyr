/*
 * Copyright (c) 2021, Teslabs Engineering S.L.
 * Copyright (c) 2025 Aleksandr Senin <al@meshium.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <soc.h>

/* GD32 HAL RCU helpers */
#include <gd32f4xx_rcu.h>

static void gd32f4xx_ck48m_init(void)
{
#define RCU_NODE DT_NODELABEL(rcu)

	/* enum indexes follow the order in dts/bindings/mfd/gd,gd32-rcu.yaml */
	enum {
		CK48M_SRC_IRC48M = 0,
		CK48M_SRC_PLL48M = 1,
	};

	enum {
		PLL48M_SRC_PLLQ = 0,
		PLL48M_SRC_PLLSAIP = 1,
	};

	int ck48m_src = COND_CODE_1(DT_NODE_HAS_PROP(RCU_NODE, gd_ck48m_source),
				    (DT_ENUM_IDX(RCU_NODE, gd_ck48m_source)),
				    (-1));

	int pll48m_src = COND_CODE_1(DT_NODE_HAS_PROP(RCU_NODE, gd_pll48m_source),
				     (DT_ENUM_IDX(RCU_NODE, gd_pll48m_source)),
				     (PLL48M_SRC_PLLQ));

	if (ck48m_src < 0) {
		return;
	}

	switch (ck48m_src) {
	case CK48M_SRC_IRC48M:
		/* Enable internal 48MHz oscillator and select it as CK48M */
		rcu_osci_on(RCU_IRC48M);
		(void)rcu_osci_stab_wait(RCU_IRC48M);
		rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M);
		break;

	case CK48M_SRC_PLL48M:
		/* Select PLL48M source (PLLQ or PLLSAIP) and route it to CK48M */
		if (pll48m_src == PLL48M_SRC_PLLSAIP) {
			rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLSAIP);
		} else {
			rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
		}

		rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);
		break;

	default:
		break;
	}
}

void soc_early_init_hook(void)
{
	SystemInit();
	gd32f4xx_ck48m_init();
}
