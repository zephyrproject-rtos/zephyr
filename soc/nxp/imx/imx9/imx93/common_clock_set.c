/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_clock.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(imx93_common_clock_set, CONFIG_SOC_LOG_LEVEL);

uint32_t common_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	clock_name_t root;
	uint32_t root_rate;
	clock_root_t clk_root;
	clock_lpcg_t clk_gate;
	uint32_t divider;

	switch (clock_name) {
	case IMX_CCM_MEDIA_AXI_CLK:
		clk_root = kCLOCK_Root_MediaAxi;
		clk_gate = kCLOCK_IpInvalid;
		CLOCK_SetRootClockMux(kCLOCK_Root_MediaAxi,
				      kCLOCK_MEDIAAXI_ClockRoot_MuxSysPll1Pfd1);
		break;
	case IMX_CCM_MEDIA_APB_CLK:
		clk_root = kCLOCK_Root_MediaApb;
		clk_gate = kCLOCK_IpInvalid;
		CLOCK_SetRootClockMux(kCLOCK_Root_MediaApb,
				      kCLOCK_MEDIAAPB_ClockRoot_MuxSysPll1Pfd1Div2);
		break;
	case IMX_CCM_MEDIA_DISP_PIX_CLK:
		clk_root = kCLOCK_Root_MediaDispPix;
		clk_gate = kCLOCK_Lcdif;
		CLOCK_SetRootClockMux(kCLOCK_Root_MediaDispPix,
				      kCLOCK_MEDIADISPPIX_ClockRoot_MuxVideoPll1Out);
		break;
	case IMX_CCM_MEDIA_LDB_CLK:
		clk_root = kCLOCK_Root_MediaLdb;
		clk_gate = kCLOCK_Lvds;
		CLOCK_SetRootClockMux(kCLOCK_Root_MediaLdb,
				      kCLOCK_MEDIALDB_ClockRoot_MuxVideoPll1Out);
		break;
	case IMX_CCM_MIPI_PHY_CFG_CLK:
		clk_root = kCLOCK_Root_MipiPhyCfg;
		clk_gate = kCLOCK_Mipi_Dsi;
		break;
	case IMX_CCM_CAM_PIX_CLK:
		clk_root = kCLOCK_Root_CamPix;
		clk_gate = kCLOCK_Mipi_Csi;
		CLOCK_SetRootClockMux(kCLOCK_Root_CamPix,
				      kCLOCK_MEDIALDB_ClockRoot_MuxVideoPll1Out);
		break;
	default:
		return -ENOTSUP;
	}

	root = CLOCK_GetRootClockSource(clk_root, CLOCK_GetRootClockMux(clk_root));
	root_rate = g_clockSourceFreq[root];
	divider = ((root_rate + (rate - 1)) / rate);

	LOG_DBG("clock_name: 0x%x, root_rate: %d, divider: %d", clock_name, root_rate, divider);

	if (clk_gate < kCLOCK_IpInvalid) {
		CLOCK_DisableClock(clk_gate);
	}

	CLOCK_SetRootClockDiv(clk_root, divider);
	CLOCK_PowerOnRootClock(clk_root);

	if (clk_gate < kCLOCK_IpInvalid) {
		CLOCK_EnableClock(clk_gate);
	}

	return 0;
}
