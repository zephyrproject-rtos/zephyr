/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CCM_IMX6SX_H__
#define __CCM_IMX6SX_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup ccm_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CCM_TUPLE(reg, shift, mask) ((offsetof(CCM_Type, reg) & 0xFF) | ((shift) << 8) | (((mask >> shift) & 0xFFFF) << 16))
#define CCM_TUPLE_REG(base, tuple)  (*((volatile uint32_t *)((uint32_t)base + ((tuple) & 0xFF))))
#define CCM_TUPLE_SHIFT(tuple)      (((tuple) >> 8) & 0x1F)
#define CCM_TUPLE_MASK(tuple)       ((uint32_t)((((tuple) >> 16) & 0xFFFF) << ((((tuple) >> 8) & 0x1F))))

/*!
 * @brief Root control names for root clock setting.
 *
 * These constants define the root control names for root clock setting.\n
 * - 0:7: REG offset to CCM_BASE in bytes.
 * - 8:15: Root clock setting bit field shift.
 * - 16:31: Root clock setting bit field width.
 */
enum _ccm_root_clock_control
{
    ccmRootPll1SwClkSel     = CCM_TUPLE(CCSR, CCM_CCSR_pll1_sw_clk_sel_SHIFT, CCM_CCSR_pll1_sw_clk_sel_MASK),             /*!< PLL1 SW Clock control name.*/
    ccmRootStepSel          = CCM_TUPLE(CCSR, CCM_CCSR_step_sel_SHIFT, CCM_CCSR_step_sel_MASK),                           /*!< Step SW Clock control name.*/
    ccmRootPeriph2ClkSel    = CCM_TUPLE(CBCDR, CCM_CBCDR_periph2_clk_sel_SHIFT, CCM_CBCDR_periph2_clk_sel_MASK),          /*!< Peripheral2 Clock control name.*/
    ccmRootPrePeriph2ClkSel = CCM_TUPLE(CBCMR, CCM_CBCMR_pre_periph2_clk_sel_SHIFT, CCM_CBCMR_pre_periph2_clk_sel_MASK),  /*!< Pre Peripheral2 Clock control name.*/
    ccmRootPeriph2Clk2Sel   = CCM_TUPLE(CBCMR, CCM_CBCMR_periph2_clk2_sel_SHIFT, CCM_CBCMR_periph2_clk2_sel_MASK),        /*!< Peripheral2 Clock2 Clock control name.*/
    ccmRootPll3SwClkSel     = CCM_TUPLE(CCSR, CCM_CCSR_pll3_sw_clk_sel_SHIFT, CCM_CCSR_pll3_sw_clk_sel_MASK),             /*!< PLL3 SW Clock control name.*/
    ccmRootOcramClkSel      = CCM_TUPLE(CBCDR, CCM_CBCDR_ocram_clk_sel_SHIFT, CCM_CBCDR_ocram_clk_sel_MASK),              /*!< OCRAM Clock control name.*/
    ccmRootOcramAltClkSel   = CCM_TUPLE(CBCDR, CCM_CBCDR_ocram_alt_clk_sel_SHIFT, CCM_CBCDR_ocram_alt_clk_sel_MASK),      /*!< OCRAM ALT Clock control name.*/
    ccmRootPeriphClkSel     = CCM_TUPLE(CBCDR, CCM_CBCDR_periph_clk_sel_SHIFT, CCM_CBCDR_periph_clk_sel_MASK),            /*!< Peripheral Clock control name.*/
    ccmRootPeriphClk2Sel    = CCM_TUPLE(CBCMR, CCM_CBCMR_periph_clk2_sel_SHIFT, CCM_CBCMR_periph_clk2_sel_MASK),          /*!< Peripheral Clock2 control name.*/
    ccmRootPrePeriphClkSel  = CCM_TUPLE(CBCMR, CCM_CBCMR_pre_periph_clk_sel_SHIFT, CCM_CBCMR_pre_periph_clk_sel_MASK),    /*!< Pre Peripheral Clock control name.*/
    ccmRootPcieAxiClkSel    = CCM_TUPLE(CBCMR, CCM_CBCMR_pcie_axi_clk_sel_SHIFT, CCM_CBCMR_pcie_axi_clk_sel_MASK),        /*!< PCIE AXI Clock control name.*/
    ccmRootPerclkClkSel     = CCM_TUPLE(CSCMR1, CCM_CSCMR1_perclk_clk_sel_SHIFT, CCM_CSCMR1_perclk_clk_sel_MASK),         /*!< Pre Clock control name.*/
    ccmRootUsdhc1ClkSel     = CCM_TUPLE(CSCMR1, CCM_CSCMR1_usdhc1_clk_sel_SHIFT, CCM_CSCMR1_usdhc1_clk_sel_MASK),         /*!< USDHC1 Clock control name.*/
    ccmRootUsdhc2ClkSel     = CCM_TUPLE(CSCMR1, CCM_CSCMR1_usdhc2_clk_sel_SHIFT, CCM_CSCMR1_usdhc2_clk_sel_MASK),         /*!< USDHC2 Clock control name.*/
    ccmRootUsdhc3ClkSel     = CCM_TUPLE(CSCMR1, CCM_CSCMR1_usdhc3_clk_sel_SHIFT, CCM_CSCMR1_usdhc3_clk_sel_MASK),         /*!< USDHC3 Clock control name.*/
    ccmRootUsdhc4ClkSel     = CCM_TUPLE(CSCMR1, CCM_CSCMR1_usdhc4_clk_sel_SHIFT, CCM_CSCMR1_usdhc4_clk_sel_MASK),         /*!< USDHC4 Clock control name.*/
    ccmRootAclkEimSlowSel   = CCM_TUPLE(CSCMR1, CCM_CSCMR1_aclk_eim_slow_sel_SHIFT, CCM_CSCMR1_aclk_eim_slow_sel_MASK),   /*!< ACLK EIM SLOW Clock control name.*/
    ccmRootGpuAxiSel        = CCM_TUPLE(CBCMR, CCM_CBCMR_gpu_axi_sel_SHIFT, CCM_CBCMR_gpu_axi_sel_MASK),                  /*!< GPU AXI Clock control name.*/
    ccmRootGpuCoreSel       = CCM_TUPLE(CBCMR, CCM_CBCMR_gpu_core_sel_SHIFT, CCM_CBCMR_gpu_core_sel_MASK),                /*!< GPU Core Clock control name.*/
    ccmRootVidClkSel        = CCM_TUPLE(CSCMR2, CCM_CSCMR2_vid_clk_sel_SHIFT, CCM_CSCMR2_vid_clk_sel_MASK),               /*!< VID Clock control name.*/
    ccmRootEsaiClkSel       = CCM_TUPLE(CSCMR2, CCM_CSCMR2_esai_clk_sel_SHIFT, CCM_CSCMR2_esai_clk_sel_MASK),             /*!< ESAI Clock control name.*/
    ccmRootAudioClkSel      = CCM_TUPLE(CDCDR, CCM_CDCDR_audio_clk_sel_SHIFT, CCM_CDCDR_audio_clk_sel_MASK),              /*!< AUDIO Clock control name.*/
    ccmRootSpdif0ClkSel     = CCM_TUPLE(CDCDR, CCM_CDCDR_spdif0_clk_sel_SHIFT, CCM_CDCDR_spdif0_clk_sel_MASK),            /*!< SPDIF0 Clock control name.*/
    ccmRootSsi1ClkSel       = CCM_TUPLE(CSCMR1, CCM_CSCMR1_ssi1_clk_sel_SHIFT, CCM_CSCMR1_ssi1_clk_sel_MASK),             /*!< SSI1 Clock control name.*/
    ccmRootSsi2ClkSel       = CCM_TUPLE(CSCMR1, CCM_CSCMR1_ssi2_clk_sel_SHIFT, CCM_CSCMR1_ssi2_clk_sel_MASK),             /*!< SSI2 Clock control name.*/
    ccmRootSsi3ClkSel       = CCM_TUPLE(CSCMR1, CCM_CSCMR1_ssi3_clk_sel_SHIFT, CCM_CSCMR1_ssi3_clk_sel_MASK),             /*!< SSI3 Clock control name.*/
    ccmRootLcdif2ClkSel     = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif2_clk_sel_SHIFT, CCM_CSCDR2_lcdif2_clk_sel_MASK),         /*!< LCDIF2 Clock control name.*/
    ccmRootLcdif2PreClkSel  = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif2_pre_clk_sel_SHIFT, CCM_CSCDR2_lcdif2_pre_clk_sel_MASK), /*!< LCDIF2 Pre Clock control name.*/
    ccmRootLdbDi1ClkSel     = CCM_TUPLE(CS2CDR, CCM_CS2CDR_ldb_di1_clk_sel_SHIFT, CCM_CS2CDR_ldb_di1_clk_sel_MASK),       /*!< LDB DI1 Clock control name.*/
    ccmRootLdbDi0ClkSel     = CCM_TUPLE(CS2CDR, CCM_CS2CDR_ldb_di0_clk_sel_SHIFT, CCM_CS2CDR_ldb_di0_clk_sel_MASK),       /*!< LDB DI0 Clock control name.*/
    ccmRootLcdif1ClkSel     = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif1_clk_sel_SHIFT, CCM_CSCDR2_lcdif1_clk_sel_MASK),         /*!< LCDIF1 Clock control name.*/
    ccmRootLcdif1PreClkSel  = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif1_pre_clk_sel_SHIFT, CCM_CSCDR2_lcdif1_pre_clk_sel_MASK), /*!< LCDIF1 Pre Clock control name.*/
    ccmRootM4ClkSel         = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_m4_clk_sel_SHIFT, CCM_CHSCCDR_m4_clk_sel_MASK),              /*!< M4 Clock control name.*/
    ccmRootM4PreClkSel      = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_m4_pre_clk_sel_SHIFT, CCM_CHSCCDR_m4_pre_clk_sel_MASK),      /*!< M4 Pre Clock control name.*/
    ccmRootEnetClkSel       = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_enet_clk_sel_SHIFT, CCM_CHSCCDR_enet_clk_sel_MASK),          /*!< Ethernet Clock control name.*/
    ccmRootEnetPreClkSel    = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_enet_pre_clk_sel_SHIFT, CCM_CHSCCDR_enet_pre_clk_sel_MASK),  /*!< Ethernet Pre Clock control name.*/
    ccmRootQspi2ClkSel      = CCM_TUPLE(CS2CDR, CCM_CS2CDR_qspi2_clk_sel_SHIFT, CCM_CS2CDR_qspi2_clk_sel_MASK),           /*!< QSPI2 Clock control name.*/
    ccmRootDisplayClkSel    = CCM_TUPLE(CSCDR3, CCM_CSCDR3_display_clk_sel_SHIFT, CCM_CSCDR3_display_clk_sel_MASK),       /*!< Display Clock control name.*/
    ccmRootCsiClkSel        = CCM_TUPLE(CSCDR3, CCM_CSCDR3_csi_clk_sel_SHIFT, CCM_CSCDR3_csi_clk_sel_MASK),               /*!< CSI Clock control name.*/
    ccmRootCanClkSel        = CCM_TUPLE(CSCMR2, CCM_CSCMR2_can_clk_sel_SHIFT, CCM_CSCMR2_can_clk_sel_MASK),               /*!< CAN Clock control name.*/
    ccmRootEcspiClkSel      = CCM_TUPLE(CSCDR2, CCM_CSCDR2_ecspi_clk_sel_SHIFT, CCM_CSCDR2_ecspi_clk_sel_MASK),           /*!< ECSPI Clock control name.*/
    ccmRootUartClkSel       = CCM_TUPLE(CSCDR1, CCM_CSCDR1_uart_clk_sel_SHIFT, CCM_CSCDR1_uart_clk_sel_MASK)              /*!< UART Clock control name.*/
};

/*! @brief Root clock select enumeration for pll1_sw_clk_sel. */
enum _ccm_rootmux_pll1_sw_clk_sel
{
    ccmRootmuxPll1SwClkPll1MainClk = 0U, /*!< PLL1 SW Clock from PLL1 Main Clock.*/
    ccmRootmuxPll1SwClkStepClk     = 1U, /*!< PLL1 SW Clock from Step Clock.*/
};

/*! @brief Root clock select enumeration for step_sel. */
enum _ccm_rootmux_step_sel
{
    ccmRootmuxStepOsc24m   = 0U, /*!< Step Clock from OSC 24M.*/
    ccmRootmuxStepPll2Pfd2 = 1U, /*!< Step Clock from PLL2 PFD2.*/
};

/*! @brief Root clock select enumeration for periph2_clk_sel. */
enum _ccm_rootmux_periph2_clk_sel
{
    ccmRootmuxPeriph2ClkPrePeriph2Clk = 0U, /*!< Peripheral2 Clock from Pre Peripheral2 Clock.*/
    ccmRootmuxPeriph2ClkPeriph2Clk2   = 1U, /*!< Peripheral2 Clock from Peripheral2.*/
};

/*! @brief Root clock select enumeration for pre_periph2_clk_sel. */
enum _ccm_rootmux_pre_periph2_clk_sel
{
    ccmRootmuxPrePeriph2ClkPll2     = 0U, /*!< Pre Peripheral2 Clock from PLL2.*/
    ccmRootmuxPrePeriph2ClkPll2Pfd2 = 1U, /*!< Pre Peripheral2 Clock from PLL2 PFD2.*/
    ccmRootmuxPrePeriph2ClkPll2Pfd0 = 2U, /*!< Pre Peripheral2 Clock from PLL2 PFD0.*/
    ccmRootmuxPrePeriph2ClkPll4     = 3U, /*!< Pre Peripheral2 Clock from PLL4.*/
};

/*! @brief Root clock select enumeration for periph2_clk2_sel. */
enum _ccm_rootmux_periph2_clk2_sel
{
    ccmRootmuxPeriph2Clk2Pll3SwClk = 0U, /*!< Peripheral2 Clock from PLL3 SW Clock.*/
    ccmRootmuxPeriph2Clk2Osc24m    = 1U, /*!< Peripheral2 Clock from OSC 24M.*/
};

/*! @brief Root clock select enumeration for pll3_sw_clk_sel. */
enum _ccm_rootmux_pll3_sw_clk_sel
{
    ccmRootmuxPll3SwClkPll3          = 0U, /*!< PLL3 SW Clock from PLL3.*/
    ccmRootmuxPll3SwClkPll3BypassClk = 1U, /*!< PLL3 SW Clock from PLL3 Bypass Clock.*/
};

/*! @brief Root clock select enumeration for ocram_clk_sel. */
enum _ccm_rootmux_ocram_clk_sel
{
    ccmRootmuxOcramClkPeriphClk   = 0U, /*!< OCRAM Clock from Peripheral Clock.*/
    ccmRootmuxOcramClkOcramAltClk = 1U, /*!< OCRAM Clock from OCRAM ALT Clock.*/
};

/*! @brief Root clock select enumeration for ocram_alt_clk_sel. */
enum _ccm_rootmux_ocram_alt_clk_sel
{
    ccmRootmuxOcramAltClkPll2Pfd2 = 0U, /*!< OCRAM ALT Clock from PLL2 PFD2.*/
    ccmRootmuxOcramAltClkPll3Pfd1 = 1U, /*!< OCRAM ALT Clock from PLL3 PFD1.*/
};

/*! @brief Root clock select enumeration for periph_clk_sel. */
enum _ccm_rootmux_periph_clk_sel
{
    ccmRootmuxPeriphClkPrePeriphClkSel = 0U, /*!< Peripheral Clock from Pre Peripheral .*/
    ccmRootmuxPeriphClkPeriphClk2Sel   = 1U, /*!< Peripheral Clock from Peripheral2.*/
};

/*! @brief Root clock select enumeration for periph_clk2_sel. */
enum _ccm_rootmux_periph_clk2_sel
{
    ccmRootmuxPeriphClk2Pll3SwClk = 0U, /*!< Peripheral Clock2 from from PLL3 SW Clock.*/
    ccmRootmuxPeriphClk2OSC24m    = 1U, /*!< Peripheral Clock2 from OSC 24M.*/
    ccmRootmuxPeriphClk2Pll2      = 2U, /*!< Peripheral Clock2 from PLL2.*/
};

/*! @brief Root clock select enumeration for pre_periph_clk_sel. */
enum _ccm_rootmux_pre_periph_clk_sel
{
    ccmRootmuxPrePeriphClkPll2         = 0U, /*!< Pre Peripheral Clock from PLL2.*/
    ccmRootmuxPrePeriphClkPll2Pfd2     = 1U, /*!< Pre Peripheral Clock from PLL2 PFD2.*/
    ccmRootmuxPrePeriphClkPll2Pfd0     = 2U, /*!< Pre Peripheral Clock from PLL2 PFD0.*/
    ccmRootmuxPrePeriphClkPll2Pfd2div2 = 3U, /*!< Pre Peripheral Clock from PLL2 PFD2 divided by 2.*/
};

/*! @brief Root clock select enumeration for pcie_axi_clk_sel. */
enum _ccm_rootmux_pcie_axi_clk_sel
{
    ccmRootmuxPcieAxiClkAxiClk = 0U, /*!< PCIE AXI Clock from AXI Clock.*/
    ccmRootmuxPcieAxiClkAhbClk = 1U, /*!< PCIE AXI Clock from AHB Clock.*/
};

/*! @brief Root clock select enumeration for perclk_clk_sel. */
enum _ccm_rootmux_perclk_clk_sel
{
    ccmRootmuxPerclkClkIpgClkRoot = 0U, /*!< Perclk from IPG Clock Root.*/
    ccmRootmuxPerclkClkOsc24m     = 1U, /*!< Perclk from OSC 24M.*/
};

/*! @brief Root clock select enumeration for usdhc1_clk_sel. */
enum _ccm_rootmux_usdhc1_clk_sel
{
    ccmRootmuxUsdhc1ClkPll2Pfd2 = 0U, /*!< USDHC1 Clock from PLL2 PFD2.*/
    ccmRootmuxUsdhc1ClkPll2Pfd0 = 1U, /*!< USDHC1 Clock from PLL2 PFD0.*/
};

/*! @brief Root clock select enumeration for usdhc2_clk_sel. */
enum _ccm_rootmux_usdhc2_clk_sel
{
    ccmRootmuxUsdhc2ClkPll2Pfd2 = 0U, /*!< USDHC2 Clock from PLL2 PFD2.*/
    ccmRootmuxUsdhc2ClkPll2Pfd0 = 1U, /*!< USDHC2 Clock from PLL2 PFD0.*/
};

/*! @brief Root clock select enumeration for usdhc3_clk_sel. */
enum _ccm_rootmux_usdhc3_clk_sel
{
    ccmRootmuxUsdhc3ClkPll2Pfd2 = 0U, /*!< USDHC3 Clock from PLL2 PFD2.*/
    ccmRootmuxUsdhc3ClkPll2Pfd0 = 1U, /*!< USDHC3 Clock from PLL2 PFD0.*/
};

/*! @brief Root clock select enumeration for usdhc4_clk_sel. */
enum _ccm_rootmux_usdhc4_clk_sel
{
    ccmRootmuxUsdhc4ClkPll2Pfd2 = 0U, /*!< USDHC4 Clock from PLL2 PFD2.*/
    ccmRootmuxUsdhc4ClkPll2Pfd0 = 1U, /*!< USDHC4 Clock from PLL2 PFD0.*/
};

/*! @brief Root clock select enumeration for aclk_eim_slow_sel. */
enum _ccm_rootmux_aclk_eim_slow_sel
{
    ccmRootmuxAclkEimSlowAxiClk    = 0U, /*!< Aclk EimSlow Clock from AXI Clock.*/
    ccmRootmuxAclkEimSlowPll3SwClk = 1U, /*!< Aclk EimSlow Clock from PLL3 SW Clock.*/
    ccmRootmuxAclkEimSlowPll2Pfd2  = 2U, /*!< Aclk EimSlow Clock from PLL2 PFD2.*/
    ccmRootmuxAclkEimSlowPll3Pfd0  = 3U, /*!< Aclk EimSlow Clock from PLL3 PFD0.*/
};

/*! @brief Root clock select enumeration for gpu_axi_sel. */
enum _ccm_rootmux_gpu_axi_sel
{
    ccmRootmuxGpuAxiPll2Pfd2 = 0U, /*!< GPU AXI Clock from PLL2 PFD2.*/
    ccmRootmuxGpuAxiPll3Pfd0 = 1U, /*!< GPU AXI Clock from PLL3 PFD0.*/
    ccmRootmuxGpuAxiPll2Pfd1 = 2U, /*!< GPU AXI Clock from PLL2 PFD1.*/
    ccmRootmuxGpuAxiPll2     = 3U, /*!< GPU AXI Clock from PLL2.*/
};

/*! @brief Root clock select enumeration for gpu_core_sel. */
enum _ccm_rootmux_gpu_core_sel
{
    ccmRootmuxGpuCorePll3Pfd1 = 0U, /*!< GPU Core Clock from PLL3 PFD1.*/
    ccmRootmuxGpuCorePll3Pfd0 = 1U, /*!< GPU Core Clock from PLL3 PFD0.*/
    ccmRootmuxGpuCorePll2     = 2U, /*!< GPU Core Clock from PLL2.*/
    ccmRootmuxGpuCorePll2Pfd2 = 3U, /*!< GPU Core Clock from PLL2 PFD2.*/
};

/*! @brief Root clock select enumeration for vid_clk_sel. */
enum _ccm_rootmux_vid_clk_sel
{
    ccmRootmuxVidClkPll3Pfd1 = 0U, /*!< VID Clock from PLL3 PFD1.*/
    ccmRootmuxVidClkPll3     = 1U, /*!< VID Clock from PLL3.*/
    ccmRootmuxVidClkPll3Pfd3 = 2U, /*!< VID Clock from PLL3 PFD3.*/
    ccmRootmuxVidClkPll4     = 3U, /*!< VID Clock from PLL4.*/
    ccmRootmuxVidClkPll5     = 4U, /*!< VID Clock from PLL5.*/
};

/*! @brief Root clock select enumeration for esai_clk_sel. */
enum _ccm_rootmux_esai_clk_sel
{
    ccmRootmuxEsaiClkPll4      = 0U, /*!< ESAI Clock from PLL4.*/
    ccmRootmuxEsaiClkPll3Pfd2  = 1U, /*!< ESAI Clock from PLL3 PFD2.*/
    ccmRootmuxEsaiClkPll5      = 2U, /*!< ESAI Clock from PLL5.*/
    ccmRootmuxEsaiClkPll3SwClk = 3U, /*!< ESAI Clock from PLL3 SW Clock.*/
};

/*! @brief Root clock select enumeration for audio_clk_sel. */
enum _ccm_rootmux_audio_clk_sel
{
    ccmRootmuxAudioClkPll4      = 0U, /*!< Audio Clock from PLL4.*/
    ccmRootmuxAudioClkPll3Pfd2  = 1U, /*!< Audio Clock from PLL3 PFD2.*/
    ccmRootmuxAudioClkPll5      = 2U, /*!< Audio Clock from PLL5.*/
    ccmRootmuxAudioClkPll3SwClk = 3U, /*!< Audio Clock from PLL3 SW Clock.*/
};

/*! @brief Root clock select enumeration for spdif0_clk_sel. */
enum _ccm_rootmux_spdif0_clk_sel
{
    ccmRootmuxSpdif0ClkPll4      = 0U, /*!< SPDIF0 Clock from PLL4.*/
    ccmRootmuxSpdif0ClkPll3Pfd2  = 1U, /*!< SPDIF0 Clock from PLL3 PFD2.*/
    ccmRootmuxSpdif0ClkPll5      = 2U, /*!< SPDIF0 Clock from PLL5.*/
    ccmRootmuxSpdif0ClkPll3SwClk = 3U, /*!< SPDIF0 Clock from PLL3 SW Clock.*/
};

/*! @brief Root clock select enumeration for ssi1_clk_sel. */
enum _ccm_rootmux_ssi1_clk_sel
{
    ccmRootmuxSsi1ClkPll3Pfd2 = 0U, /*!< SSI1 Clock from PLL3 PFD2.*/
    ccmRootmuxSsi1ClkPll5     = 1U, /*!< SSI1 Clock from PLL5.*/
    ccmRootmuxSsi1ClkPll4     = 2U, /*!< SSI1 Clock from PLL4.*/
};

/*! @brief Root clock select enumeration for ssi2_clk_sel. */
enum _ccm_rootmux_ssi2_clk_sel
{
    ccmRootmuxSsi2ClkPll3Pfd2 = 0U, /*!< SSI2 Clock from PLL3 PFD2.*/
    ccmRootmuxSsi2ClkPll5     = 1U, /*!< SSI2 Clock from PLL5.*/
    ccmRootmuxSsi2ClkPll4     = 2U, /*!< SSI2 Clock from PLL4.*/
};

/*! @brief Root clock select enumeration for ssi3_clk_sel. */
enum _ccm_rootmux_ssi3_clk_sel
{
    ccmRootmuxSsi3ClkPll3Pfd2 = 0U, /*!< SSI3 Clock from PLL3 PFD2.*/
    ccmRootmuxSsi3ClkPll5     = 1U, /*!< SSI3 Clock from PLL5.*/
    ccmRootmuxSsi3ClkPll4     = 2U, /*!< SSI3 Clock from PLL4.*/
};

/*! @brief Root clock select enumeration for lcdif2_clk_sel. */
enum _ccm_rootmux_lcdif2_clk_sel
{
    ccmRootmuxLcdif2ClkLcdif2PreClk = 0U, /*!< LCDIF2 Clock from LCDIF2 Pre Clock.*/
    ccmRootmuxLcdif2ClkIppDi0Clk    = 1U, /*!< LCDIF2 Clock from IPP DI0 Clock.*/
    ccmRootmuxLcdif2ClkIppDi1Clk    = 2U, /*!< LCDIF2 Clock from IPP DI0 Clock.*/
    ccmRootmuxLcdif2ClkLdbDi0Clk    = 3U, /*!< LCDIF2 Clock from LDB DI0 Clock.*/
    ccmRootmuxLcdif2ClkLdbDi1Clk    = 4U, /*!< LCDIF2 Clock from LDB DI0 Clock.*/
};

/*! @brief Root clock select enumeration for lcdif2_pre_clk_sel. */
enum _ccm_rootmux_lcdif2_pre_clk_sel
{
    ccmRootmuxLcdif2ClkPrePll2     = 0U, /*!< LCDIF2 Pre Clock from PLL2.*/
    ccmRootmuxLcdif2ClkPrePll3Pfd3 = 1U, /*!< LCDIF2 Pre Clock from PLL3 PFD3.*/
    ccmRootmuxLcdif2ClkPrePll5     = 2U, /*!< LCDIF2 Pre Clock from PLL3 PFD5.*/
    ccmRootmuxLcdif2ClkPrePll2Pfd0 = 3U, /*!< LCDIF2 Pre Clock from PLL2 PFD0.*/
    ccmRootmuxLcdif2ClkPrePll2Pfd3 = 4U, /*!< LCDIF2 Pre Clock from PLL2 PFD3.*/
    ccmRootmuxLcdif2ClkPrePll3Pfd1 = 5U, /*!< LCDIF2 Pre Clock from PLL3 PFD1.*/
};

/*! @brief Root clock select enumeration for ldb_di1_clk_sel. */
enum _ccm_rootmux_ldb_di1_clk_sel
{
    ccmRootmuxLdbDi1ClkPll3SwClk = 0U, /*!< lDB DI1 Clock from PLL3 SW Clock.*/
    ccmRootmuxLdbDi1ClkPll2Pfd0  = 1U, /*!< lDB DI1 Clock from PLL2 PFD0.*/
    ccmRootmuxLdbDi1ClkPll2Pfd2  = 2U, /*!< lDB DI1 Clock from PLL2 PFD2.*/
    ccmRootmuxLdbDi1ClkPll2      = 3U, /*!< lDB DI1 Clock from PLL2.*/
    ccmRootmuxLdbDi1ClkPll3Pfd3  = 4U, /*!< lDB DI1 Clock from PLL3 PFD3.*/
    ccmRootmuxLdbDi1ClkPll3Pfd2  = 5U, /*!< lDB DI1 Clock from PLL3 PFD2.*/
};

/*! @brief Root clock select enumeration for ldb_di0_clk_sel. */
enum _ccm_rootmux_ldb_di0_clk_sel
{
    ccmRootmuxLdbDi0ClkPll5     = 0U, /*!< lDB DI0 Clock from PLL5.*/
    ccmRootmuxLdbDi0ClkPll2Pfd0 = 1U, /*!< lDB DI0 Clock from PLL2 PFD0.*/
    ccmRootmuxLdbDi0ClkPll2Pfd2 = 2U, /*!< lDB DI0 Clock from PLL2 PFD2.*/
    ccmRootmuxLdbDi0ClkPll2Pfd3 = 3U, /*!< lDB DI0 Clock from PLL2 PFD3.*/
    ccmRootmuxLdbDi0ClkPll3Pfd1 = 4U, /*!< lDB DI0 Clock from PLL3 PFD1.*/
    ccmRootmuxLdbDi0ClkPll3Pfd3 = 5U, /*!< lDB DI0 Clock from PLL3 PFD3.*/
};

/*! @brief Root clock select enumeration for lcdif1_clk_sel. */
enum _ccm_rootmux_lcdif1_clk_sel
{
    ccmRootmuxLcdif1ClkLcdif1PreClk = 0U, /*!< LCDIF1 clock from LCDIF1 Pre Clock.*/
    ccmRootmuxLcdif1ClkIppDi0Clk    = 1U, /*!< LCDIF1 clock from IPP DI0 Clock.*/
    ccmRootmuxLcdif1ClkIppDi1Clk    = 2U, /*!< LCDIF1 clock from IPP DI1 Clock.*/
    ccmRootmuxLcdif1ClkLdbDi0Clk    = 3U, /*!< LCDIF1 clock from LDB DI0 Clock.*/
    ccmRootmuxLcdif1ClkLdbDi1Clk    = 4U, /*!< LCDIF1 clock from LDB DI1 Clock.*/
};

/*! @brief Root clock select enumeration for lcdif1_pre_clk_sel. */
enum _ccm_rootmux_lcdif1_pre_clk_sel
{
    ccmRootmuxLcdif1PreClkPll2     = 0U, /*!< LCDIF1 pre clock from PLL2.*/
    ccmRootmuxLcdif1PreClkPll3Pfd3 = 1U, /*!< LCDIF1 pre clock from PLL3 PFD3.*/
    ccmRootmuxLcdif1PreClkPll5     = 2U, /*!< LCDIF1 pre clock from PLL5.*/
    ccmRootmuxLcdif1PreClkPll2Pfd0 = 3U, /*!< LCDIF1 pre clock from PLL2 PFD0.*/
    ccmRootmuxLcdif1PreClkPll2Pfd1 = 4U, /*!< LCDIF1 pre clock from PLL2 PFD1.*/
    ccmRootmuxLcdif1PreClkPll3Pfd1 = 5U, /*!< LCDIF1 pre clock from PLL3 PFD1.*/
};

/*! @brief Root clock select enumeration for m4_clk_sel. */
enum _ccm_rootmux_m4_clk_sel
{
    ccmRootmuxM4ClkM4PreClk  = 0U, /*!< M4 clock from M4 Pre Clock.*/
    ccmRootmuxM4ClkPll3Pfd3  = 1U, /*!< M4 clock from PLL3 PFD3.*/
    ccmRootmuxM4ClkIppDi0Clk = 2U, /*!< M4 clock from IPP DI0 Clock.*/
    ccmRootmuxM4ClkIppDi1Clk = 3U, /*!< M4 clock from IPP DI1 Clock.*/
    ccmRootmuxM4ClkLdbDi0Clk = 4U, /*!< M4 clock from LDB DI0 Clock.*/
    ccmRootmuxM4ClkLdbDi1Clk = 5U, /*!< M4 clock from LDB DI1 Clock.*/
};

/*! @brief Root clock select enumeration for m4_pre_clk_sel. */
enum _ccm_rootmux_m4_pre_clk_sel
{
    ccmRootmuxM4PreClkPll2      = 0U, /*!< M4 pre clock from PLL2.*/
    ccmRootmuxM4PreClkPll3SwClk = 1U, /*!< M4 pre clock from PLL3 SW Clock.*/
    ccmRootmuxM4PreClkOsc24m    = 2U, /*!< M4 pre clock from OSC 24M.*/
    ccmRootmuxM4PreClkPll2Pfd0  = 3U, /*!< M4 pre clock from PLL2 PFD0.*/
    ccmRootmuxM4PreClkPll2Pfd2  = 4U, /*!< M4 pre clock from PLL2 PFD2.*/
    ccmRootmuxM4PreClkPll3Pfd3  = 5U, /*!< M4 pre clock from PLL3 PFD3.*/
};

/*! @brief Root clock select enumeration for nent_clk_sel. */
enum _ccm_rootmux_enet_clk_sel
{
    ccmRootmuxEnetClkEnetPreClk = 0U, /*!< Ethernet clock from Ethernet Pre Clock.*/
    ccmRootmuxEnetClkIppDi0Clk  = 1U, /*!< Ethernet clock from IPP DI0 Clock.*/
    ccmRootmuxEnetClkIppDi1Clk  = 2U, /*!< Ethernet clock from IPP DI1 Clock.*/
    ccmRootmuxEnetClkLdbDi0Clk  = 3U, /*!< Ethernet clock from LDB DI0 Clock.*/
    ccmRootmuxEnetClkLdbDi1Clk  = 4U, /*!< Ethernet clock from LDB DI1 Clock.*/
};

/*! @brief Root clock select enumeration for enet_pre_clk_sel. */
enum _ccm_rootmux_enet_pre_clk_sel
{
    ccmRootmuxEnetPreClkPll2      = 0U, /*!< Ethernet Pre clock from PLL2.*/
    ccmRootmuxEnetPreClkPll3SwClk = 1U, /*!< Ethernet Pre clock from PLL3 SW Clock.*/
    ccmRootmuxEnetPreClkPll5      = 2U, /*!< Ethernet Pre clock from PLL5.*/
    ccmRootmuxEnetPreClkPll2Pfd0  = 3U, /*!< Ethernet Pre clock from PLL2 PFD0.*/
    ccmRootmuxEnetPreClkPll2Pfd2  = 4U, /*!< Ethernet Pre clock from PLL2 PFD2.*/
    ccmRootmuxEnetPreClkPll3Pfd2  = 5U, /*!< Ethernet Pre clock from PLL3 PFD2.*/
};

/*! @brief Root clock select enumeration for qspi2_clk_sel. */
enum _ccm_rootmux_qspi2_clk_sel
{
    ccmRootmuxQspi2ClkPll2Pfd0  = 0U, /*!< QSPI2 Clock from PLL2 PFD0.*/
    ccmRootmuxQspi2ClkPll2      = 1U, /*!< QSPI2 Clock from PLL2.*/
    ccmRootmuxQspi2ClkPll3SwClk = 2U, /*!< QSPI2 Clock from PLL3 SW Clock.*/
    ccmRootmuxQspi2ClkPll2Pfd2  = 3U, /*!< QSPI2 Clock from PLL2 PFD2.*/
    ccmRootmuxQspi2ClkPll3Pfd3  = 4U, /*!< QSPI2 Clock from PLL3 PFD3.*/
};

/*! @brief Root clock select enumeration for display_clk_sel. */
enum _ccm_rootmux_display_clk_sel
{
    ccmRootmuxDisplayClkPll2      = 0U, /*!< Display Clock from PLL2.*/
    ccmRootmuxDisplayClkPll2Pfd2  = 1U, /*!< Display Clock from PLL2 PFD2.*/
    ccmRootmuxDisplayClkPll3SwClk = 2U, /*!< Display Clock from PLL3 SW Clock.*/
    ccmRootmuxDisplayClkPll3Pfd1  = 3U, /*!< Display Clock from PLL3 PFD1.*/
};

/*! @brief Root clock select enumeration for csi_clk_sel. */
enum _ccm_rootmux_csi_clk_sel
{
    ccmRootmuxCsiClkOSC24m        = 0U, /*!< CSI Clock from OSC 24M.*/
    ccmRootmuxCsiClkPll2Pfd2      = 1U, /*!< CSI Clock from PLL2 PFD2.*/
    ccmRootmuxCsiClkPll3SwClkDiv2 = 2U, /*!< CSI Clock from PLL3 SW clock divided by 2.*/
    ccmRootmuxCsiClkPll3Pfd1      = 3U, /*!< CSI Clock from PLL3 PFD1.*/
};

/*! @brief Root clock select enumeration for can_clk_sel. */
enum _ccm_rootmux_can_clk_sel
{
    ccmRootmuxCanClkPll3SwClkDiv8     = 0U, /*!< CAN Clock from PLL3 SW clock divided by 8.*/
    ccmRootmuxCanClkOsc24m            = 1U, /*!< CAN Clock from OSC 24M.*/
    ccmRootmuxCanClkPll3SwClkDiv6     = 2U, /*!< CAN Clock from PLL3 SW clock divided by 6.*/
    ccmRootmuxCanClkDisableFlexcanClk = 3U, /*!< Disable FlexCAN clock.*/
};

/*! @brief Root clock select enumeration for ecspi_clk_sel. */
enum _ccm_rootmux_ecspi_clk_sel
{
    ccmRootmuxEcspiClkPll3SwClkDiv8 = 0U, /*!< ecSPI Clock from PLL3 SW clock divided by 8.*/
    ccmRootmuxEcspiClkOsc24m        = 1U, /*!< ecSPI Clock from OSC 24M.*/
};

/*! @brief Root clock select enumeration for uart_clk_sel. */
enum _ccm_rootmux_uart_clk_sel
{
    ccmRootmuxUartClkPll3SwClkDiv6 = 0U, /*!< UART Clock from PLL3 SW clock divided by 6.*/
    ccmRootmuxUartClkOsc24m        = 1U, /*!< UART Clock from OSC 24M.*/
};

/*!
 * @brief Root control names for root divider setting.
 *
 * These constants define the root control names for root divider setting.\n
 * - 0:7: REG offset to CCM_BASE in bytes.
 * - 8:15: Root divider setting bit field shift.
 * - 16:31: Root divider setting bit field width.
 */
enum _ccm_root_div_control
{
    ccmRootArmPodf         = CCM_TUPLE(CACRR, CCM_CACRR_arm_podf_SHIFT, CCM_CACRR_arm_podf_MASK),                        /*!< ARM Clock post divider control names.*/
    ccmRootFabricMmdcPodf  = CCM_TUPLE(CBCDR, CCM_CBCDR_fabric_mmdc_podf_SHIFT, CCM_CBCDR_fabric_mmdc_podf_MASK),        /*!< Fabric MMDC Clock post divider control names.*/
    ccmRootPeriph2Clk2Podf = CCM_TUPLE(CBCDR, CCM_CBCDR_periph2_clk2_podf_SHIFT, CCM_CBCDR_periph2_clk2_podf_MASK),      /*!< Peripheral2 Clock2 post divider control names.*/
    ccmRootOcramPodf       = CCM_TUPLE(CBCDR, CCM_CBCDR_ocram_podf_SHIFT, CCM_CBCDR_ocram_podf_MASK),                    /*!< OCRAM Clock post divider control names.*/
    ccmRootAhbPodf         = CCM_TUPLE(CBCDR, CCM_CBCDR_ahb_podf_SHIFT, CCM_CBCDR_ahb_podf_MASK),                        /*!< AHB Clock post divider control names.*/
    ccmRootPeriphClk2Podf  = CCM_TUPLE(CBCDR, CCM_CBCDR_periph_clk2_podf_SHIFT, CCM_CBCDR_periph_clk2_podf_MASK),        /*!< Peripheral Clock2 post divider control names.*/
    ccmRootPerclkPodf      = CCM_TUPLE(CSCMR1, CCM_CSCMR1_perclk_podf_SHIFT, CCM_CSCMR1_perclk_podf_MASK),               /*!< Pre Clock post divider control names.*/
    ccmRootIpgPodf         = CCM_TUPLE(CBCDR, CCM_CBCDR_ipg_podf_SHIFT, CCM_CBCDR_ipg_podf_MASK),                        /*!< IPG Clock post divider control names.*/
    ccmRootUsdhc1Podf      = CCM_TUPLE(CSCDR1, CCM_CSCDR1_usdhc1_podf_SHIFT, CCM_CSCDR1_usdhc1_podf_MASK),               /*!< USDHC1 Clock post divider control names.*/
    ccmRootUsdhc2Podf      = CCM_TUPLE(CSCDR1, CCM_CSCDR1_usdhc2_podf_SHIFT, CCM_CSCDR1_usdhc2_podf_MASK),               /*!< USDHC2 Clock post divider control names.*/
    ccmRootUsdhc3Podf      = CCM_TUPLE(CSCDR1, CCM_CSCDR1_usdhc3_podf_SHIFT, CCM_CSCDR1_usdhc3_podf_MASK),               /*!< USDHC3 Clock post divider control names.*/
    ccmRootUsdhc4Podf      = CCM_TUPLE(CSCDR1, CCM_CSCDR1_usdhc4_podf_SHIFT, CCM_CSCDR1_usdhc4_podf_MASK),               /*!< USDHC4 Clock post divider control names.*/
    ccmRootAclkEimSlowPodf = CCM_TUPLE(CSCMR1, CCM_CSCMR1_aclk_eim_slow_podf_SHIFT, CCM_CSCMR1_aclk_eim_slow_podf_MASK), /*!< ACLK EIM SLOW Clock post divider control names.*/
    ccmRootGpuAxiPodf      = CCM_TUPLE(CBCMR, CCM_CBCMR_gpu_axi_podf_SHIFT, CCM_CBCMR_gpu_axi_podf_MASK),                /*!< GPU AXI Clock post divider control names.*/
    ccmRootGpuCorePodf     = CCM_TUPLE(CBCMR, CCM_CBCMR_gpu_core_podf_SHIFT, CCM_CBCMR_gpu_core_podf_MASK),              /*!< GPU Core Clock post divider control names.*/
    ccmRootVidClkPodf      = CCM_TUPLE(CSCMR2, CCM_CSCMR2_vid_clk_podf_SHIFT, CCM_CSCMR2_vid_clk_podf_MASK),             /*!< VID Clock post divider control names.*/
    ccmRootEsaiClkPodf     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_esai_clk_podf_SHIFT, CCM_CS1CDR_esai_clk_podf_MASK),           /*!< ESAI Clock pre divider control names.*/
    ccmRootEsaiClkPred     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_esai_clk_pred_SHIFT, CCM_CS1CDR_esai_clk_pred_MASK),           /*!< ESAI Clock post divider control names.*/
    ccmRootAudioClkPodf    = CCM_TUPLE(CDCDR, CCM_CDCDR_audio_clk_podf_SHIFT, CCM_CDCDR_audio_clk_podf_MASK),            /*!< AUDIO Clock post divider control names.*/
    ccmRootAudioClkPred    = CCM_TUPLE(CDCDR, CCM_CDCDR_audio_clk_pred_SHIFT, CCM_CDCDR_audio_clk_pred_MASK),            /*!< AUDIO Clock pre divider control names.*/
    ccmRootSpdif0ClkPodf   = CCM_TUPLE(CDCDR, CCM_CDCDR_spdif0_clk_podf_SHIFT, CCM_CDCDR_spdif0_clk_podf_MASK),          /*!< SPDIF0 Clock post divider control names.*/
    ccmRootSpdif0ClkPred   = CCM_TUPLE(CDCDR, CCM_CDCDR_spdif0_clk_pred_SHIFT, CCM_CDCDR_spdif0_clk_pred_MASK),          /*!< SPDIF0 Clock pre divider control names.*/
    ccmRootSsi1ClkPodf     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_ssi1_clk_podf_SHIFT, CCM_CS1CDR_ssi1_clk_podf_MASK),           /*!< SSI1 Clock post divider control names.*/
    ccmRootSsi1ClkPred     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_ssi1_clk_pred_SHIFT, CCM_CS1CDR_ssi1_clk_pred_MASK),           /*!< SSI1 Clock pre divider control names.*/
    ccmRootSsi2ClkPodf     = CCM_TUPLE(CS2CDR, CCM_CS2CDR_ssi2_clk_podf_SHIFT, CCM_CS2CDR_ssi2_clk_podf_MASK),           /*!< SSI2 Clock post divider control names.*/
    ccmRootSsi2ClkPred     = CCM_TUPLE(CS2CDR, CCM_CS2CDR_ssi2_clk_pred_SHIFT, CCM_CS2CDR_ssi2_clk_pred_MASK),           /*!< SSI2 Clock pre divider control names.*/
    ccmRootSsi3ClkPodf     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_ssi3_clk_podf_SHIFT, CCM_CS1CDR_ssi3_clk_podf_MASK),           /*!< SSI3 Clock post divider control names.*/
    ccmRootSsi3ClkPred     = CCM_TUPLE(CS1CDR, CCM_CS1CDR_ssi3_clk_pred_SHIFT, CCM_CS1CDR_ssi3_clk_pred_MASK),           /*!< SSI3 Clock pre divider control names.*/
    ccmRootLcdif2Podf      = CCM_TUPLE(CSCMR1, CCM_CSCMR1_lcdif2_podf_SHIFT, CCM_CSCMR1_lcdif2_podf_MASK),               /*!< LCDIF2 Clock post divider control names.*/
    ccmRootLcdif2Pred      = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif2_pred_SHIFT, CCM_CSCDR2_lcdif2_pred_MASK),               /*!< LCDIF2 Clock pre divider control names.*/
    ccmRootLdbDi1Div       = CCM_TUPLE(CSCMR2, CCM_CSCMR2_ldb_di1_div_SHIFT, CCM_CSCMR2_ldb_di1_div_MASK),               /*!< LDB DI1 Clock divider control names.*/
    ccmRootLdbDi0Div       = CCM_TUPLE(CSCMR2, CCM_CSCMR2_ldb_di0_div_SHIFT, CCM_CSCMR2_ldb_di0_div_MASK),               /*!< LCDIDI0 Clock divider control names.*/
    ccmRootLcdif1Podf      = CCM_TUPLE(CBCMR, CCM_CBCMR_lcdif1_podf_SHIFT, CCM_CBCMR_lcdif1_podf_MASK),                  /*!< LCDIF1 Clock post divider control names.*/
    ccmRootLcdif1Pred      = CCM_TUPLE(CSCDR2, CCM_CSCDR2_lcdif1_pred_SHIFT, CCM_CSCDR2_lcdif1_pred_MASK),               /*!< LCDIF1 Clock pre divider control names.*/
    ccmRootM4Podf          = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_m4_podf_SHIFT, CCM_CHSCCDR_m4_podf_MASK),                    /*!< M4 Clock post divider control names.*/
    ccmRootEnetPodf        = CCM_TUPLE(CHSCCDR, CCM_CHSCCDR_enet_podf_SHIFT, CCM_CHSCCDR_enet_podf_MASK),                /*!< Ethernet Clock post divider control names.*/
    ccmRootQspi1Podf       = CCM_TUPLE(CSCMR1, CCM_CSCMR1_qspi1_podf_SHIFT, CCM_CSCMR1_qspi1_podf_MASK),                 /*!< QSPI1 Clock post divider control names.*/
    ccmRootQspi2ClkPodf    = CCM_TUPLE(CS2CDR, CCM_CS2CDR_qspi2_clk_podf_SHIFT, CCM_CS2CDR_qspi2_clk_podf_MASK),         /*!< QSPI2 Clock post divider control names.*/
    ccmRootQspi2ClkPred    = CCM_TUPLE(CS2CDR, CCM_CS2CDR_qspi2_clk_pred_SHIFT, CCM_CS2CDR_qspi2_clk_pred_MASK),         /*!< QSPI2 Clock pre divider control names.*/
    ccmRootDisplayPodf     = CCM_TUPLE(CSCDR3, CCM_CSCDR3_display_podf_SHIFT, CCM_CSCDR3_display_podf_MASK),             /*!< Display Clock post divider control names.*/
    ccmRootCsiPodf         = CCM_TUPLE(CSCDR3, CCM_CSCDR3_csi_podf_SHIFT, CCM_CSCDR3_csi_podf_MASK),                     /*!< CSI Clock post divider control names.*/
    ccmRootCanClkPodf      = CCM_TUPLE(CSCMR2, CCM_CSCMR2_can_clk_podf_SHIFT, CCM_CSCMR2_can_clk_podf_MASK),             /*!< CAN Clock post divider control names.*/
    ccmRootEcspiClkPodf    = CCM_TUPLE(CSCDR2, CCM_CSCDR2_ecspi_clk_podf_SHIFT, CCM_CSCDR2_ecspi_clk_podf_MASK),         /*!< ECSPI Clock post divider control names.*/
    ccmRootUartClkPodf     = CCM_TUPLE(CSCDR1, CCM_CSCDR1_uart_clk_podf_SHIFT, CCM_CSCDR1_uart_clk_podf_MASK)            /*!< UART Clock post divider control names.*/
};

/*!
 * @brief CCM CCGR gate control for each module independently.
 *
 * These constants define the ccm ccgr clock gate for each module.\n
 * - 0:7: REG offset to CCM_BASE in bytes.
 * - 8:15: Root divider setting bit field shift.
 * - 16:31: Root divider setting bit field width.
 */
enum _ccm_ccgr_gate
{
    ccmCcgrGateAipsTz1Clk         = CCM_TUPLE(CCGR0, CCM_CCGR0_CG0_SHIFT, CCM_CCGR0_CG0_MASK),   /*!< AipsTz1 Clock Gate.*/
    ccmCcgrGateAipsTz2Clk         = CCM_TUPLE(CCGR0, CCM_CCGR0_CG1_SHIFT, CCM_CCGR0_CG1_MASK),   /*!< AipsTz2 Clock Gate.*/
    ccmCcgrGateApbhdmaHclk        = CCM_TUPLE(CCGR0, CCM_CCGR0_CG2_SHIFT, CCM_CCGR0_CG2_MASK),   /*!< ApbhdmaH Clock Gate.*/
    ccmCcgrGateAsrcClk            = CCM_TUPLE(CCGR0, CCM_CCGR0_CG3_SHIFT, CCM_CCGR0_CG3_MASK),   /*!< Asrc Clock Gate.*/
    ccmCcgrGateCaamSecureMemClk   = CCM_TUPLE(CCGR0, CCM_CCGR0_CG4_SHIFT, CCM_CCGR0_CG4_MASK),   /*!< CaamSecureMem Clock Gate.*/
    ccmCcgrGateCaamWrapperAclk    = CCM_TUPLE(CCGR0, CCM_CCGR0_CG5_SHIFT, CCM_CCGR0_CG5_MASK),   /*!< CaamWrapperA Clock Gate.*/
    ccmCcgrGateCaamWrapperIpg     = CCM_TUPLE(CCGR0, CCM_CCGR0_CG6_SHIFT, CCM_CCGR0_CG6_MASK),   /*!< CaamWrapperIpg Clock Gate.*/
    ccmCcgrGateCan1Clk            = CCM_TUPLE(CCGR0, CCM_CCGR0_CG7_SHIFT, CCM_CCGR0_CG7_MASK),   /*!< Can1 Clock Gate.*/
    ccmCcgrGateCan1SerialClk      = CCM_TUPLE(CCGR0, CCM_CCGR0_CG8_SHIFT, CCM_CCGR0_CG8_MASK),   /*!< Can1 Serial Clock Gate.*/
    ccmCcgrGateCan2Clk            = CCM_TUPLE(CCGR0, CCM_CCGR0_CG9_SHIFT, CCM_CCGR0_CG9_MASK),   /*!< Can2 Clock Gate.*/
    ccmCcgrGateCan2SerialClk      = CCM_TUPLE(CCGR0, CCM_CCGR0_CG10_SHIFT, CCM_CCGR0_CG10_MASK), /*!< Can2 Serial Clock Gate.*/
    ccmCcgrGateArmDbgClk          = CCM_TUPLE(CCGR0, CCM_CCGR0_CG11_SHIFT, CCM_CCGR0_CG11_MASK), /*!< Arm Debug Clock Gate.*/
    ccmCcgrGateDcic1Clk           = CCM_TUPLE(CCGR0, CCM_CCGR0_CG12_SHIFT, CCM_CCGR0_CG12_MASK), /*!< Dcic1 Clock Gate.*/
    ccmCcgrGateDcic2Clk           = CCM_TUPLE(CCGR0, CCM_CCGR0_CG13_SHIFT, CCM_CCGR0_CG13_MASK), /*!< Dcic2 Clock Gate.*/
    ccmCcgrGateAipsTz3Clk         = CCM_TUPLE(CCGR0, CCM_CCGR0_CG15_SHIFT, CCM_CCGR0_CG15_MASK), /*!< AipsTz3 Clock Gate.*/
    ccmCcgrGateEcspi1Clk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG0_SHIFT, CCM_CCGR1_CG0_MASK),   /*!< Ecspi1 Clock Gate.*/
    ccmCcgrGateEcspi2Clk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG1_SHIFT, CCM_CCGR1_CG1_MASK),   /*!< Ecspi2 Clock Gate.*/
    ccmCcgrGateEcspi3Clk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG2_SHIFT, CCM_CCGR1_CG2_MASK),   /*!< Ecspi3 Clock Gate.*/
    ccmCcgrGateEcspi4Clk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG3_SHIFT, CCM_CCGR1_CG3_MASK),   /*!< Ecspi4 Clock Gate.*/
    ccmCcgrGateEcspi5Clk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG4_SHIFT, CCM_CCGR1_CG4_MASK),   /*!< Ecspi5 Clock Gate.*/
    ccmCcgrGateEpit1Clk           = CCM_TUPLE(CCGR1, CCM_CCGR1_CG6_SHIFT, CCM_CCGR1_CG6_MASK),   /*!< Epit1 Clock Gate.*/
    ccmCcgrGateEpit2Clk           = CCM_TUPLE(CCGR1, CCM_CCGR1_CG7_SHIFT, CCM_CCGR1_CG7_MASK),   /*!< Epit2 Clock Gate.*/
    ccmCcgrGateEsaiClk            = CCM_TUPLE(CCGR1, CCM_CCGR1_CG8_SHIFT, CCM_CCGR1_CG8_MASK),   /*!< Esai Clock Gate.*/
    ccmCcgrGateWakeupClk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG9_SHIFT, CCM_CCGR1_CG9_MASK),   /*!< Wakeup Clock Gate.*/
    ccmCcgrGateGptClk             = CCM_TUPLE(CCGR1, CCM_CCGR1_CG10_SHIFT, CCM_CCGR1_CG10_MASK), /*!< Gpt Clock Gate.*/
    ccmCcgrGateGptSerialClk       = CCM_TUPLE(CCGR1, CCM_CCGR1_CG11_SHIFT, CCM_CCGR1_CG11_MASK), /*!< Gpt Serial Clock Gate.*/
    ccmCcgrGateGpuClk             = CCM_TUPLE(CCGR1, CCM_CCGR1_CG13_SHIFT, CCM_CCGR1_CG13_MASK), /*!< Gpu Clock Gate.*/
    ccmCcgrGateOcramSClk          = CCM_TUPLE(CCGR1, CCM_CCGR1_CG14_SHIFT, CCM_CCGR1_CG14_MASK), /*!< OcramS Clock Gate.*/
    ccmCcgrGateCanfdClk           = CCM_TUPLE(CCGR1, CCM_CCGR1_CG15_SHIFT, CCM_CCGR1_CG15_MASK), /*!< Canfd Clock Gate.*/
    ccmCcgrGateCsiClk             = CCM_TUPLE(CCGR2, CCM_CCGR2_CG1_SHIFT, CCM_CCGR2_CG1_MASK),   /*!< Csi Clock Gate.*/
    ccmCcgrGateI2c1Serialclk      = CCM_TUPLE(CCGR2, CCM_CCGR2_CG3_SHIFT, CCM_CCGR2_CG3_MASK),   /*!< I2c1 Serial Clock Gate.*/
    ccmCcgrGateI2c2Serialclk      = CCM_TUPLE(CCGR2, CCM_CCGR2_CG4_SHIFT, CCM_CCGR2_CG4_MASK),   /*!< I2c2 Serial Clock Gate.*/
    ccmCcgrGateI2c3Serialclk      = CCM_TUPLE(CCGR2, CCM_CCGR2_CG5_SHIFT, CCM_CCGR2_CG5_MASK),   /*!< I2c3 Serial Clock Gate.*/
    ccmCcgrGateIimClk             = CCM_TUPLE(CCGR2, CCM_CCGR2_CG6_SHIFT, CCM_CCGR2_CG6_MASK),   /*!< Iim Clock Gate.*/
    ccmCcgrGateIomuxIptClkIo      = CCM_TUPLE(CCGR2, CCM_CCGR2_CG7_SHIFT, CCM_CCGR2_CG7_MASK),   /*!< Iomux Ipt Clock Gate.*/
    ccmCcgrGateIpmux1Clk          = CCM_TUPLE(CCGR2, CCM_CCGR2_CG8_SHIFT, CCM_CCGR2_CG8_MASK),   /*!< Ipmux1 Clock Gate.*/
    ccmCcgrGateIpmux2Clk          = CCM_TUPLE(CCGR2, CCM_CCGR2_CG9_SHIFT, CCM_CCGR2_CG9_MASK),   /*!< Ipmux2 Clock Gate.*/
    ccmCcgrGateIpmux3Clk          = CCM_TUPLE(CCGR2, CCM_CCGR2_CG10_SHIFT, CCM_CCGR2_CG10_MASK), /*!< Ipmux3 Clock Gate.*/
    ccmCcgrGateIpsyncIp2apbtTasc1 = CCM_TUPLE(CCGR2, CCM_CCGR2_CG11_SHIFT, CCM_CCGR2_CG11_MASK), /*!< IpsyncIp2apbtTasc1 Clock Gate.*/
    ccmCcgrGateLcdClk             = CCM_TUPLE(CCGR2, CCM_CCGR2_CG14_SHIFT, CCM_CCGR2_CG14_MASK), /*!< Lcd Clock Gate.*/
    ccmCcgrGatePxpClk             = CCM_TUPLE(CCGR2, CCM_CCGR2_CG15_SHIFT, CCM_CCGR2_CG15_MASK), /*!< Pxp Clock Gate.*/
    ccmCcgrGateM4Clk              = CCM_TUPLE(CCGR3, CCM_CCGR3_CG1_SHIFT, CCM_CCGR3_CG1_MASK),   /*!< M4 Clock Gate.*/
    ccmCcgrGateEnetClk            = CCM_TUPLE(CCGR3, CCM_CCGR3_CG2_SHIFT, CCM_CCGR3_CG2_MASK),   /*!< Enet Clock Gate.*/
    ccmCcgrGateDispAxiClk         = CCM_TUPLE(CCGR3, CCM_CCGR3_CG3_SHIFT, CCM_CCGR3_CG3_MASK),   /*!< DispAxi Clock Gate.*/
    ccmCcgrGateLcdif2PixClk       = CCM_TUPLE(CCGR3, CCM_CCGR3_CG4_SHIFT, CCM_CCGR3_CG4_MASK),   /*!< Lcdif2Pix Clock Gate.*/
    ccmCcgrGateLcdif1PixClk       = CCM_TUPLE(CCGR3, CCM_CCGR3_CG5_SHIFT, CCM_CCGR3_CG5_MASK),   /*!< Lcdif1Pix Clock Gate.*/
    ccmCcgrGateLdbDi0Clk          = CCM_TUPLE(CCGR3, CCM_CCGR3_CG6_SHIFT, CCM_CCGR3_CG6_MASK),   /*!< LdbDi0 Clock Gate.*/
    ccmCcgrGateQspi1Clk           = CCM_TUPLE(CCGR3, CCM_CCGR3_CG7_SHIFT, CCM_CCGR3_CG7_MASK),   /*!< Qspi1 Clock Gate.*/
    ccmCcgrGateMlbClk             = CCM_TUPLE(CCGR3, CCM_CCGR3_CG9_SHIFT, CCM_CCGR3_CG9_MASK),   /*!< Mlb Clock Gate.*/
    ccmCcgrGateMmdcCoreAclkFastP0 = CCM_TUPLE(CCGR3, CCM_CCGR3_CG10_SHIFT, CCM_CCGR3_CG10_MASK), /*!< Mmdc Core Aclk FastP0 Clock Gate.*/
    ccmCcgrGateMmdcCoreIpgClkP0   = CCM_TUPLE(CCGR3, CCM_CCGR3_CG12_SHIFT, CCM_CCGR3_CG12_MASK), /*!< Mmdc Core Ipg Clk P0 Clock Gate.*/
    ccmCcgrGateMmdcCoreIpgClkP1   = CCM_TUPLE(CCGR3, CCM_CCGR3_CG13_SHIFT, CCM_CCGR3_CG13_MASK), /*!< Mmdc Core Ipg Clk P1 Clock Gate.*/
    ccmCcgrGateOcramClk           = CCM_TUPLE(CCGR3, CCM_CCGR3_CG14_SHIFT, CCM_CCGR3_CG14_MASK), /*!< Ocram Clock Gate.*/
    ccmCcgrGatePcieRoot           = CCM_TUPLE(CCGR4, CCM_CCGR4_CG0_SHIFT, CCM_CCGR4_CG0_MASK),   /*!< Pcie Clock Gate.*/
    ccmCcgrGateQspi2Clk           = CCM_TUPLE(CCGR4, CCM_CCGR4_CG5_SHIFT, CCM_CCGR4_CG5_MASK),   /*!< Qspi2 Clock Gate.*/
    ccmCcgrGatePl301Mx6qper1Bch   = CCM_TUPLE(CCGR4, CCM_CCGR4_CG6_SHIFT, CCM_CCGR4_CG6_MASK),   /*!< Pl301Mx6qper1Bch Clock Gate.*/
    ccmCcgrGatePl301Mx6qper2Main  = CCM_TUPLE(CCGR4, CCM_CCGR4_CG7_SHIFT, CCM_CCGR4_CG7_MASK),   /*!< Pl301Mx6qper2Main Clock Gate.*/
    ccmCcgrGatePwm1Clk            = CCM_TUPLE(CCGR4, CCM_CCGR4_CG8_SHIFT, CCM_CCGR4_CG8_MASK),   /*!< Pwm1 Clock Gate.*/
    ccmCcgrGatePwm2Clk            = CCM_TUPLE(CCGR4, CCM_CCGR4_CG9_SHIFT, CCM_CCGR4_CG9_MASK),   /*!< Pwm2 Clock Gate.*/
    ccmCcgrGatePwm3Clk            = CCM_TUPLE(CCGR4, CCM_CCGR4_CG10_SHIFT, CCM_CCGR4_CG10_MASK), /*!< Pwm3 Clock Gate.*/
    ccmCcgrGatePwm4Clk            = CCM_TUPLE(CCGR4, CCM_CCGR4_CG11_SHIFT, CCM_CCGR4_CG11_MASK), /*!< Pwm4 Clock Gate.*/
    ccmCcgrGateRawnandUBchInptApb = CCM_TUPLE(CCGR4, CCM_CCGR4_CG12_SHIFT, CCM_CCGR4_CG12_MASK), /*!< RawnandUBchInptApb Clock Gate.*/
    ccmCcgrGateRawnandUGpmiBch    = CCM_TUPLE(CCGR4, CCM_CCGR4_CG13_SHIFT, CCM_CCGR4_CG13_MASK), /*!< RawnandUGpmiBch Clock Gate.*/
    ccmCcgrGateRawnandUGpmiGpmiIo = CCM_TUPLE(CCGR4, CCM_CCGR4_CG14_SHIFT, CCM_CCGR4_CG14_MASK), /*!< RawnandUGpmiGpmiIo Clock Gate.*/
    ccmCcgrGateRawnandUGpmiInpApb = CCM_TUPLE(CCGR4, CCM_CCGR4_CG15_SHIFT, CCM_CCGR4_CG15_MASK), /*!< RawnandUGpmiInpApb Clock Gate.*/
    ccmCcgrGateRomClk             = CCM_TUPLE(CCGR5, CCM_CCGR5_CG0_SHIFT, CCM_CCGR5_CG0_MASK),   /*!< Rom Clock Gate.*/
    ccmCcgrGateSdmaClk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG3_SHIFT, CCM_CCGR5_CG3_MASK),   /*!< Sdma Clock Gate.*/
    ccmCcgrGateSpbaClk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG6_SHIFT, CCM_CCGR5_CG6_MASK),   /*!< Spba Clock Gate.*/
    ccmCcgrGateSpdifAudioClk      = CCM_TUPLE(CCGR5, CCM_CCGR5_CG7_SHIFT, CCM_CCGR5_CG7_MASK),   /*!< SpdifAudio Clock Gate.*/
    ccmCcgrGateSsi1Clk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG9_SHIFT, CCM_CCGR5_CG9_MASK),   /*!< Ssi1 Clock Gate.*/
    ccmCcgrGateSsi2Clk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG10_SHIFT, CCM_CCGR5_CG10_MASK), /*!< Ssi2 Clock Gate.*/
    ccmCcgrGateSsi3Clk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG11_SHIFT, CCM_CCGR5_CG11_MASK), /*!< Ssi3 Clock Gate.*/
    ccmCcgrGateUartClk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG12_SHIFT, CCM_CCGR5_CG12_MASK), /*!< Uart Clock Gate.*/
    ccmCcgrGateUartSerialClk      = CCM_TUPLE(CCGR5, CCM_CCGR5_CG13_SHIFT, CCM_CCGR5_CG13_MASK), /*!< Uart Serial Clock Gate.*/
    ccmCcgrGateSai1Clk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG14_SHIFT, CCM_CCGR5_CG14_MASK), /*!< Sai1 Clock Gate.*/
    ccmCcgrGateSai2Clk            = CCM_TUPLE(CCGR5, CCM_CCGR5_CG15_SHIFT, CCM_CCGR5_CG15_MASK), /*!< Sai2 Clock Gate.*/
    ccmCcgrGateUsboh3Clk          = CCM_TUPLE(CCGR6, CCM_CCGR6_CG0_SHIFT, CCM_CCGR6_CG0_MASK),   /*!< Usboh3 Clock Gate.*/
    ccmCcgrGateUsdhc1Clk          = CCM_TUPLE(CCGR6, CCM_CCGR6_CG1_SHIFT, CCM_CCGR6_CG1_MASK),   /*!< Usdhc1 Clock Gate.*/
    ccmCcgrGateUsdhc2Clk          = CCM_TUPLE(CCGR6, CCM_CCGR6_CG2_SHIFT, CCM_CCGR6_CG2_MASK),   /*!< Usdhc2 Clock Gate.*/
    ccmCcgrGateUsdhc3Clk          = CCM_TUPLE(CCGR6, CCM_CCGR6_CG3_SHIFT, CCM_CCGR6_CG3_MASK),   /*!< Usdhc3 Clock Gate.*/
    ccmCcgrGateUsdhc4Clk          = CCM_TUPLE(CCGR6, CCM_CCGR6_CG4_SHIFT, CCM_CCGR6_CG4_MASK),   /*!< Usdhc4 Clock Gate.*/
    ccmCcgrGateEimSlowClk         = CCM_TUPLE(CCGR6, CCM_CCGR6_CG5_SHIFT, CCM_CCGR6_CG5_MASK),   /*!< EimSlow Clock Gate.*/
    ccmCcgrGatePwm8Clk            = CCM_TUPLE(CCGR6, CCM_CCGR6_CG8_SHIFT, CCM_CCGR6_CG8_MASK),   /*!< Pwm8 Clock Gate.*/
    ccmCcgrGateVadcClk            = CCM_TUPLE(CCGR6, CCM_CCGR6_CG10_SHIFT, CCM_CCGR6_CG10_MASK), /*!< Vadc Clock Gate.*/
    ccmCcgrGateGisClk             = CCM_TUPLE(CCGR6, CCM_CCGR6_CG11_SHIFT, CCM_CCGR6_CG11_MASK), /*!< Gis Clock Gate.*/
    ccmCcgrGateI2c4SerialClk      = CCM_TUPLE(CCGR6, CCM_CCGR6_CG12_SHIFT, CCM_CCGR6_CG12_MASK), /*!< I2c4 Serial Clock Gate.*/
    ccmCcgrGatePwm5Clk            = CCM_TUPLE(CCGR6, CCM_CCGR6_CG13_SHIFT, CCM_CCGR6_CG13_MASK), /*!< Pwm5 Clock Gate.*/
    ccmCcgrGatePwm6Clk            = CCM_TUPLE(CCGR6, CCM_CCGR6_CG14_SHIFT, CCM_CCGR6_CG14_MASK), /*!< Pwm6 Clock Gate.*/
    ccmCcgrGatePwm7Clk            = CCM_TUPLE(CCGR6, CCM_CCGR6_CG15_SHIFT, CCM_CCGR6_CG15_MASK), /*!< Pwm7 Clock Gate.*/
};

/*! @brief CCM gate control value. */
enum _ccm_gate_value
{
    ccmClockNotNeeded = 0U, /*!< Clock always disabled.*/
    ccmClockNeededRun = 1U, /*!< Clock enabled when CPU is running.*/
    ccmClockNeededAll = 3U  /*!< Clock always enabled.*/
};

/*! @brief CCM override clock enable signal from module. */
enum _ccm_overrided_enable_signal
{
    ccmOverridedSignalFromGpt     = 1U << 5,  /*!< Override clock enable signal from GPT.*/
    ccmOverridedSignalFromEpit    = 1U << 6,  /*!< Override clock enable signal from EPIT.*/
    ccmOverridedSignalFromUsdhc   = 1U << 7,  /*!< Override clock enable signal from USDHC.*/
    ccmOverridedSignalFromGpu     = 1U << 10, /*!< Override clock enable signal from GPU.*/
    ccmOverridedSignalFromCan2Cpi = 1U << 28, /*!< Override clock enable signal from CAN2.*/
    ccmOverridedSignalFromCan1Cpi = 1U << 30  /*!< Override clock enable signal from CAN1.*/
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name CCM Root Clock Setting
 * @{
 */

/*!
 * @brief Set clock root mux.
 * User maybe need to set more than one mux node according to the clock tree
 * description on the reference manual.
 *
 * @param base CCM base pointer.
 * @param ccmRootClk Root clock control (see @ref _ccm_root_clock_control enumeration).
 * @param mux Root mux value (see @ref _ccm_rootmux_xxx enumeration).
 */
static inline void CCM_SetRootMux(CCM_Type * base, uint32_t ccmRootClk, uint32_t mux)
{
    CCM_TUPLE_REG(base, ccmRootClk) = (CCM_TUPLE_REG(base, ccmRootClk) & (~CCM_TUPLE_MASK(ccmRootClk))) |
                                      (((uint32_t)((mux) << CCM_TUPLE_SHIFT(ccmRootClk))) & CCM_TUPLE_MASK(ccmRootClk));
}

/*!
 * @brief Get clock root mux.
 * In order to get the clock source of root, user maybe need to get more than one
 * node's mux value to obtain the final clock source of root.
 *
 * @param base CCM base pointer.
 * @param ccmRootClk Root clock control (see @ref _ccm_root_clock_control enumeration).
 * @return Root mux value (see @ref _ccm_rootmux_xxx enumeration).
 */
static inline uint32_t CCM_GetRootMux(CCM_Type * base, uint32_t ccmRootClk)
{
    return (CCM_TUPLE_REG(base, ccmRootClk) & CCM_TUPLE_MASK(ccmRootClk)) >> CCM_TUPLE_SHIFT(ccmRootClk);
}

/*!
 * @brief Set root clock divider.
 * User should set the dividers carefully according to the clock tree on
 * the reference manual. Take care of that the setting of one divider value
 * may affect several clock root.
 *
 * @param base CCM base pointer.
 * @param ccmRootDiv Root divider control (see @ref _ccm_root_div_control enumeration)
 * @param div Divider value (divider = div + 1).
 */
static inline void CCM_SetRootDivider(CCM_Type * base, uint32_t ccmRootDiv, uint32_t div)
{
    CCM_TUPLE_REG(base, ccmRootDiv) = (CCM_TUPLE_REG(base, ccmRootDiv) & (~CCM_TUPLE_MASK(ccmRootDiv))) |
                                      (((uint32_t)((div) << CCM_TUPLE_SHIFT(ccmRootDiv))) & CCM_TUPLE_MASK(ccmRootDiv));
}

/*!
 * @brief Get root clock divider.
 * In order to get divider value of clock root, user should get specific
 * divider value according to the clock tree description on reference manual.
 * Then calculate the root clock with those divider value.
 *
 * @param base CCM base pointer.
 * @param ccmRootDiv Root control (see @ref _ccm_root_div_control enumeration).
 * @param div Pointer to divider value store address.
 * @return Root divider value.
 */
static inline uint32_t CCM_GetRootDivider(CCM_Type * base, uint32_t ccmRootDiv)
{
    return (CCM_TUPLE_REG(base, ccmRootDiv) & CCM_TUPLE_MASK(ccmRootDiv)) >> CCM_TUPLE_SHIFT(ccmRootDiv);
}

/*!
 * @brief Set handshake mask of MMDC module.
 * During divider ratio mmdc_axi_podf change or sync mux periph2_clk_sel
 * change (but not jtag) or SRC request during warm reset, mask handshake with mmdc module.
 *
 * @param base CCM base pointer.
 * @param mask True: mask handshake with MMDC; False: allow handshake with MMDC.
 */
void CCM_SetMmdcHandshakeMask(CCM_Type * base, bool mask);

/*@}*/

/*!
 * @name CCM Gate Control
 * @{
 */

/*!
 * @brief Set CCGR gate control for each module
 * User should set specific gate for each module according to the description
 * of the table of system clocks, gating and override in CCM chapter of
 * reference manual. Take care of that one module may need to set more than
 * one clock gate.
 *
 * @param base CCM base pointer.
 * @param ccmGate Gate control for each module (see @ref _ccm_ccgr_gate enumeration).
 * @param control Gate control value (see @ref _ccm_gate_value).
 */
static inline void CCM_ControlGate(CCM_Type * base, uint32_t ccmGate, uint32_t control)
{
    CCM_TUPLE_REG(base, ccmGate) = (CCM_TUPLE_REG(base, ccmGate) & (~CCM_TUPLE_MASK(ccmGate))) |
                                      (((uint32_t)((control) << CCM_TUPLE_SHIFT(ccmGate))) & CCM_TUPLE_MASK(ccmGate));
}

/*!
 * @brief Set override or do not override clock enable signal from module.
 * This is applicable only for modules whose clock enable signals are used.
 *
 * @param base CCM base pointer.
 * @param signal Overrided enable signal from module (see @ref _ccm_overrided_enable_signal enumeration).
 * @param control Override / Do not override clock enable signal from module.
 *                - true: override clock enable signal.
 *                - false: Do not override clock enable signal.
 */
void CCM_SetClockEnableSignalOverrided(CCM_Type * base, uint32_t signal, bool control);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __CCM_IMX6SX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
