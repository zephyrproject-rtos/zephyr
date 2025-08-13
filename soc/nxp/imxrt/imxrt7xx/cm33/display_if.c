/*
 * Copyright 2024,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/autoconf.h>
#include "fsl_power.h"
#include "fsl_clock.h"

/* Weak so board can override this function */
void __weak imxrt_pre_init_display_interface(void)
{
	/* Assert MIPI control reset. */
	RESET_SetPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);

	/* Disable MIPI DSI power down. */
	POWER_DisablePD(kPDRUNCFG_APD_MIPIDSI);
	POWER_DisablePD(kPDRUNCFG_PPD_MIPIDSI);
	POWER_DisablePD(kPDRUNCFG_PD_VDD2_MIPI);

	/* Apply power down configuration. */
	POWER_ApplyPD();

	/* Configure MIPY ESC clock. */
	/* Use PLL PFD1 as clock source, 396m. */
	CLOCK_AttachClk(kMAIN_PLL_PFD1_to_MIPI_DPHYESC_CLK);
	/* RxClkEsc min 60MHz, TxClkEsc 12 to 20MHz. */
	/* RxClkEsc = 396MHz / 6 = 66MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivDphyEscRxClk, 6);
	/* TxClkEsc = 396MHz / 6 / 4 = 16.5MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivDphyEscTxClk, 4);

#ifdef CONFIG_MIPI_DBI
	/* When using LCDIF, with 279.53MHz DBI source clock and 16bpp format, a 14 wr
	 * period requires a 279.53MHz / 14 * 16 = 319.46Mhz DPHY clk source. Considering
	 * the DCS packaging cost, the MIPI DPHY speed shall be ***SLIGHTLY*** larger
	 * than the DBI interface speed. DPHY uses AUDIO_PLL_PFD2 which is 532.48MHz as
	 * source, the frequency is 532.48 * 18 / 30 = 319.49MHz, which meets the
	 * requirement.
	 */
	uint32_t mipiDsiDphyBitClkFreq_Hz =
			DT_PROP(DT_NODELABEL(lcdif), clock_frequency) /
			DT_PROP_OR(DT_NODELABEL(lcdif), divider, 1) /
			DT_PROP(DT_NODELABEL(lcdif), wr_period) * 16U;
#else
	/* The DPHY bit clock must be fast enough to send out the pixels, it should be
	 * larger than:
	 * (Pixel clock * bit per output pixel) / number of MIPI data lane
	 * DPHY uses AUDIO pll pfd2 as aource, and its max allowed clock frequency is
	 * 532.48 x 18 / 16 = 599.04MHz. The MIPI panel supports up to 850MHz bit clock.
	 */
	uint32_t mipiDsiDphyBitClkFreq_Hz = DT_PROP(DT_NODELABEL(mipi_dsi), phy_clock);
#endif
	uint8_t clockDiv = (uint8_t)((uint64_t)CLOCK_GetAudioPllFreq() * 18U /
					(uint64_t)mipiDsiDphyBitClkFreq_Hz);

	CLOCK_InitAudioPfd(kCLOCK_Pfd2, clockDiv);

	CLOCK_AttachClk(kAUDIO_PLL_PFD2_to_MIPI_DSI_HOST_PHY);
	CLOCK_SetClkDiv(kCLOCK_DivDphyClk, 1);
}

void __weak imxrt_post_init_display_interface(void)
{
	/* Clear MIPI control reset. */
	RESET_ClearPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);
}

void __weak imxrt_deinit_display_interface(void)
{
	/* Assert MIPI DSI reset */
	RESET_SetPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);
	/* Remove clock from DPHY */
	CLOCK_AttachClk(kNONE_to_MIPI_DSI_HOST_PHY);
}
