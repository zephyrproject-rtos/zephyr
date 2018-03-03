/*
 * The Clear BSD License
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

#include "fsl_common.h"
#include "fsl_clock.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* External XTAL (OSC) clock frequency. */
uint32_t g_xtalFreq;
/* External RTC XTAL clock frequency. */
uint32_t g_rtcXtalFreq;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t CLOCK_GetPeriphClkFreq(void)
{
    uint32_t freq;

    /* Periph_clk2_clk ---> Periph_clk */
    if (CCM->CBCDR & CCM_CBCDR_PERIPH_CLK_SEL_MASK)
    {
        switch (CCM->CBCMR & CCM_CBCMR_PERIPH_CLK2_SEL_MASK)
        {
            /* Pll3_sw_clk ---> Periph_clk2_clk ---> Periph_clk */
            case CCM_CBCMR_PERIPH_CLK2_SEL(0U):
                freq = CLOCK_GetPllFreq(kCLOCK_PllUsb1);
                break;

            /* Osc_clk ---> Periph_clk2_clk ---> Periph_clk */
            case CCM_CBCMR_PERIPH_CLK2_SEL(1U):
                freq = CLOCK_GetOscFreq();
                break;

            case CCM_CBCMR_PERIPH_CLK2_SEL(2U):
                freq = CLOCK_GetPllFreq(kCLOCK_PllSys);
                break;

            case CCM_CBCMR_PERIPH_CLK2_SEL(3U):
            default:
                freq = 0U;
                break;
        }

        freq /= (((CCM->CBCDR & CCM_CBCDR_PERIPH_CLK2_PODF_MASK) >> CCM_CBCDR_PERIPH_CLK2_PODF_SHIFT) + 1U);
    }
    /* Pre_Periph_clk ---> Periph_clk */
    else
    {
        switch (CCM->CBCMR & CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK)
        {
            /* PLL2 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(0U):
                freq = CLOCK_GetPllFreq(kCLOCK_PllSys);
                break;

            /* PLL2 PFD2 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(1U):
                freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd2);
                break;

            /* PLL2 PFD0 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(2U):
                freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd0);
                break;

            /* PLL1 divided(/2) ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(3U):
                freq = CLOCK_GetPllFreq(kCLOCK_PllArm) / (((CCM->CACRR & CCM_CACRR_ARM_PODF_MASK) >> CCM_CACRR_ARM_PODF_SHIFT) + 1U);
                break;

            default:
                freq = 0U;
                break;
        }
    }

    return freq;
}

void CLOCK_InitExternalClk(bool bypassXtalOsc)
{
    /* This device does not support bypass XTAL OSC. */
    assert(!bypassXtalOsc);

    CCM_ANALOG->MISC0_CLR = CCM_ANALOG_MISC0_XTAL_24M_PWD_MASK; /* Power up */
    while ((XTALOSC24M->LOWPWR_CTRL & XTALOSC24M_LOWPWR_CTRL_XTALOSC_PWRUP_STAT_MASK) == 0)
    {
    }
    CCM_ANALOG->MISC0_SET = CCM_ANALOG_MISC0_OSC_XTALOK_EN_MASK; /* detect freq */
    while ((CCM_ANALOG->MISC0 & CCM_ANALOG_MISC0_OSC_XTALOK_MASK) == 0)
    {
    }
    CCM_ANALOG->MISC0_CLR = CCM_ANALOG_MISC0_OSC_XTALOK_EN_MASK;
}

void CLOCK_DeinitExternalClk(void)
{
    CCM_ANALOG->MISC0_SET = CCM_ANALOG_MISC0_XTAL_24M_PWD_MASK; /* Power down */
}

void CLOCK_SwitchOsc(clock_osc_t osc)
{
    if (osc == kCLOCK_RcOsc)
        XTALOSC24M->LOWPWR_CTRL_SET = XTALOSC24M_LOWPWR_CTRL_SET_OSC_SEL_MASK;
    else
        XTALOSC24M->LOWPWR_CTRL_CLR = XTALOSC24M_LOWPWR_CTRL_CLR_OSC_SEL_MASK;
}

void CLOCK_InitRcOsc24M(void)
{
    XTALOSC24M->LOWPWR_CTRL |= XTALOSC24M_LOWPWR_CTRL_RC_OSC_EN_MASK;
}

void CLOCK_DeinitRcOsc24M(void)
{
    XTALOSC24M->LOWPWR_CTRL &= ~XTALOSC24M_LOWPWR_CTRL_RC_OSC_EN_MASK;
}

uint32_t CLOCK_GetFreq(clock_name_t name)
{
    uint32_t freq;

    switch (name)
    {
        case kCLOCK_CpuClk:
            /* Periph_clk ---> AHB Clock */
        case kCLOCK_AhbClk:
            /* Periph_clk ---> AHB Clock */
            freq = CLOCK_GetPeriphClkFreq() / (((CCM->CBCDR & CCM_CBCDR_AHB_PODF_MASK) >> CCM_CBCDR_AHB_PODF_SHIFT) + 1U);
            break;

        case kCLOCK_SemcClk:
            /* SEMC alternative clock ---> SEMC Clock */
            if (CCM->CBCDR & CCM_CBCDR_SEMC_CLK_SEL_MASK)
            {
                /* PLL3 PFD1 ---> SEMC alternative clock ---> SEMC Clock */
                if (CCM->CBCDR & CCM_CBCDR_SEMC_ALT_CLK_SEL_MASK)
                {
                    freq = CLOCK_GetUsb1PfdFreq(kCLOCK_Pfd1);
                }
                /* PLL2 PFD2 ---> SEMC alternative clock ---> SEMC Clock */
                else
                {
                    freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd2);
                }
            }
            /* Periph_clk ---> SEMC Clock */
            else
            {
                freq = CLOCK_GetPeriphClkFreq();
            }

            freq /= (((CCM->CBCDR & CCM_CBCDR_SEMC_PODF_MASK) >> CCM_CBCDR_SEMC_PODF_SHIFT) + 1U);
            break;

        case kCLOCK_IpgClk:
            /* Periph_clk ---> AHB Clock ---> IPG Clock */
            freq = CLOCK_GetPeriphClkFreq() / (((CCM->CBCDR & CCM_CBCDR_AHB_PODF_MASK) >> CCM_CBCDR_AHB_PODF_SHIFT) + 1U);
            freq /= (((CCM->CBCDR & CCM_CBCDR_IPG_PODF_MASK) >> CCM_CBCDR_IPG_PODF_SHIFT) + 1U);
            break;

        case kCLOCK_OscClk:
            freq = CLOCK_GetOscFreq();
            break;
        case kCLOCK_RtcClk:
            freq = CLOCK_GetRtcFreq();
            break;
        case kCLOCK_ArmPllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllArm);
            break;
        case kCLOCK_Usb1PllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllUsb1);
            break;
        case kCLOCK_Usb1PllPfd0Clk:
            freq = CLOCK_GetUsb1PfdFreq(kCLOCK_Pfd0);
            break;
        case kCLOCK_Usb1PllPfd1Clk:
            freq = CLOCK_GetUsb1PfdFreq(kCLOCK_Pfd1);
            break;
        case kCLOCK_Usb1PllPfd2Clk:
            freq = CLOCK_GetUsb1PfdFreq(kCLOCK_Pfd2);
            break;
        case kCLOCK_Usb1PllPfd3Clk:
            freq = CLOCK_GetUsb1PfdFreq(kCLOCK_Pfd3);
            break;
        case kCLOCK_Usb2PllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllUsb2);
            break;
        case kCLOCK_SysPllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllSys);
            break;
        case kCLOCK_SysPllPfd0Clk:
            freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd0);
            break;
        case kCLOCK_SysPllPfd1Clk:
            freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd1);
            break;
        case kCLOCK_SysPllPfd2Clk:
            freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd2);
            break;
        case kCLOCK_SysPllPfd3Clk:
            freq = CLOCK_GetSysPfdFreq(kCLOCK_Pfd3);
            break;
        case kCLOCK_EnetPll0Clk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllEnet0);
            break;
        case kCLOCK_EnetPll1Clk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllEnet1);
            break;
        case kCLOCK_EnetPll2Clk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllEnet2);
            break;
        case kCLOCK_AudioPllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllAudio);
            break;
        case kCLOCK_VideoPllClk:
            freq = CLOCK_GetPllFreq(kCLOCK_PllVideo);
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

void CLOCK_InitArmPll(const clock_arm_pll_config_t *config)
{
    CCM_ANALOG->PLL_ARM = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                          CCM_ANALOG_PLL_ARM_DIV_SELECT(config->loopDivider);

    while ((CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitArmPll(void)
{
    CCM_ANALOG->PLL_ARM = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;
}

void CLOCK_InitSysPll(const clock_sys_pll_config_t *config)
{
    CCM_ANALOG->PLL_SYS = CCM_ANALOG_PLL_SYS_ENABLE_MASK |
                          CCM_ANALOG_PLL_SYS_DIV_SELECT(config->loopDivider);

    while ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitSysPll(void)
{
    CCM_ANALOG->PLL_SYS = CCM_ANALOG_PLL_SYS_POWERDOWN_MASK;
}

void CLOCK_InitUsb1Pll(const clock_usb_pll_config_t *config)
{
    CCM_ANALOG->PLL_USB1 = CCM_ANALOG_PLL_USB1_ENABLE_MASK      |
                           CCM_ANALOG_PLL_USB1_POWER_MASK       |
                           CCM_ANALOG_PLL_USB1_EN_USB_CLKS_MASK |
                           CCM_ANALOG_PLL_USB1_DIV_SELECT(config->loopDivider);

    while ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitUsb1Pll(void)
{
    CCM_ANALOG->PLL_USB1 = 0U;
}

void CLOCK_InitUsb2Pll(const clock_usb_pll_config_t *config)
{
    CCM_ANALOG->PLL_USB2 = CCM_ANALOG_PLL_USB2_ENABLE_MASK      |
                           CCM_ANALOG_PLL_USB2_POWER_MASK       |
                           CCM_ANALOG_PLL_USB2_EN_USB_CLKS_MASK |
                           CCM_ANALOG_PLL_USB2_DIV_SELECT(config->loopDivider);

    while ((CCM_ANALOG->PLL_USB2 & CCM_ANALOG_PLL_USB2_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitUsb2Pll(void)
{
    CCM_ANALOG->PLL_USB2 = 0U;
}

void CLOCK_InitAudioPll(const clock_audio_pll_config_t *config)
{
    uint32_t pllAudio;
    uint32_t misc2 = 0;

    CCM_ANALOG->PLL_AUDIO_NUM = CCM_ANALOG_PLL_AUDIO_NUM_A(config->numerator);
    CCM_ANALOG->PLL_AUDIO_DENOM = CCM_ANALOG_PLL_AUDIO_DENOM_B(config->denominator);

    /*
     * Set post divider:
     *
     * ------------------------------------------------------------------------
     * | config->postDivider | PLL_AUDIO[POST_DIV_SELECT]  | MISC2[AUDIO_DIV] |
     * ------------------------------------------------------------------------
     * |         1           |            2                |        0         |
     * ------------------------------------------------------------------------
     * |         2           |            1                |        0         |
     * ------------------------------------------------------------------------
     * |         4           |            2                |        3         |
     * ------------------------------------------------------------------------
     * |         8           |            1                |        3         |
     * ------------------------------------------------------------------------
     * |         16          |            0                |        3         |
     * ------------------------------------------------------------------------
     */
    pllAudio = CCM_ANALOG_PLL_AUDIO_ENABLE_MASK | CCM_ANALOG_PLL_AUDIO_DIV_SELECT(config->loopDivider);

    switch (config->postDivider)
    {
        case 16:
            pllAudio |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(0);
            misc2 = CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK | CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK;
            break;

        case 8:
            pllAudio |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(1);
            misc2 = CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK | CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK;
            break;

        case 4:
            pllAudio |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2);
            misc2 = CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK | CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK;
            break;

        case 2:
            pllAudio |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(1);
            break;

        default:
            pllAudio |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2);
            break;
    }

    CCM_ANALOG->MISC2 = (CCM_ANALOG->MISC2 & ~(CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK | CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK))
                      | misc2;

    CCM_ANALOG->PLL_AUDIO = pllAudio;

    while ((CCM_ANALOG->PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitAudioPll(void)
{
    CCM_ANALOG->PLL_AUDIO = CCM_ANALOG_PLL_AUDIO_POWERDOWN_MASK;
}

void CLOCK_InitVideoPll(const clock_video_pll_config_t *config)
{
    uint32_t pllVideo;
    uint32_t misc2 = 0;

    CCM_ANALOG->PLL_VIDEO_NUM = CCM_ANALOG_PLL_VIDEO_NUM_A(config->numerator);
    CCM_ANALOG->PLL_VIDEO_DENOM = CCM_ANALOG_PLL_VIDEO_DENOM_B(config->denominator);

    /*
     * Set post divider:
     *
     * ------------------------------------------------------------------------
     * | config->postDivider | PLL_VIDEO[POST_DIV_SELECT]  | MISC2[VIDEO_DIV] |
     * ------------------------------------------------------------------------
     * |         1           |            2                |        0         |
     * ------------------------------------------------------------------------
     * |         2           |            1                |        0         |
     * ------------------------------------------------------------------------
     * |         4           |            2                |        3         |
     * ------------------------------------------------------------------------
     * |         8           |            1                |        3         |
     * ------------------------------------------------------------------------
     * |         16          |            0                |        3         |
     * ------------------------------------------------------------------------
     */
    pllVideo = CCM_ANALOG_PLL_VIDEO_ENABLE_MASK | CCM_ANALOG_PLL_VIDEO_DIV_SELECT(config->loopDivider);

    switch (config->postDivider)
    {
        case 16:
            pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(0);
            misc2 = CCM_ANALOG_MISC2_VIDEO_DIV(3);
            break;

        case 8:
            pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(1);
            misc2 = CCM_ANALOG_MISC2_VIDEO_DIV(3);
            break;

        case 4:
            pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(2);
            misc2 = CCM_ANALOG_MISC2_VIDEO_DIV(3);
            break;

        case 2:
            pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(1);
            break;

        default:
            pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(2);
            break;
    }

    CCM_ANALOG->MISC2 = (CCM_ANALOG->MISC2 & ~CCM_ANALOG_MISC2_VIDEO_DIV_MASK) | misc2;

    CCM_ANALOG->PLL_VIDEO = pllVideo;

    while ((CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitVideoPll(void)
{
    CCM_ANALOG->PLL_VIDEO = CCM_ANALOG_PLL_VIDEO_POWERDOWN_MASK;
}

void CLOCK_InitEnetPll(const clock_enet_pll_config_t *config)
{
    uint32_t enet_pll = CCM_ANALOG_PLL_ENET_ENET1_DIV_SELECT(config->loopDivider1) |
                        CCM_ANALOG_PLL_ENET_ENET0_DIV_SELECT(config->loopDivider0);

    if (config->enableClkOutput0)
    {
        enet_pll |= CCM_ANALOG_PLL_ENET_ENET1_125M_EN_MASK;
    }

    if (config->enableClkOutput1)
    {
        enet_pll |= CCM_ANALOG_PLL_ENET_ENET2_125M_EN_MASK;
    }

    if (config->enableClkOutput2)
    {
        enet_pll |= CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_MASK;
    }

    CCM_ANALOG->PLL_ENET = enet_pll;

    /* Wait for stable */
    while ((CCM_ANALOG->PLL_ENET & CCM_ANALOG_PLL_ENET_LOCK_MASK) == 0)
    {
    }
}

void CLOCK_DeinitEnetPll(void)
{
    CCM_ANALOG->PLL_ENET = CCM_ANALOG_PLL_ENET_POWERDOWN_MASK;
}

uint32_t CLOCK_GetPllFreq(clock_pll_t pll)
{
    uint32_t freq;
    uint32_t divSelect;
    uint64_t freqTmp;

    const uint32_t enetRefClkFreq[] = {
        25000000U, /* 25M */
        50000000U, /* 50M */
        100000000U, /* 100M */
        125000000U /* 125M */
    };

    /* check if PLL is enabled */
    if(!CLOCK_IsPllEnabled(CCM_ANALOG, pll))
    {
        return 0U;
    }

    /* get pll reference clock */
    freq = CLOCK_GetPllBypassRefClk(CCM_ANALOG, pll);

    /* check if pll is bypassed */
    if(CLOCK_IsPllBypassed(CCM_ANALOG, pll))
    {
        return freq;
    }

    switch (pll)
    {
        case kCLOCK_PllArm:
            freq = ((freq * ((CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK) >>
                                         CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT)) >> 1U);
            break;

        case kCLOCK_PllSys:
            /* PLL output frequency = Fref * (DIV_SELECT + NUM/DENOM). */
            freqTmp = ((uint64_t)freq * ((uint64_t)(CCM_ANALOG->PLL_SYS_NUM))) / ((uint64_t)(CCM_ANALOG->PLL_SYS_DENOM));

            if (CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_DIV_SELECT_MASK)
            {
                freq *= 22U;
            }
            else
            {
                freq *= 20U;
            }

            freq += (uint32_t)freqTmp;
            break;

        case kCLOCK_PllUsb1:
            freq = (freq * ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK) ? 22U : 20U));
            break;

        case kCLOCK_PllAudio:
            /* PLL output frequency = Fref * (DIV_SELECT + NUM/DENOM). */
            divSelect = (CCM_ANALOG->PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_DIV_SELECT_MASK) >> CCM_ANALOG_PLL_AUDIO_DIV_SELECT_SHIFT;

            freqTmp = ((uint64_t)freq * ((uint64_t)(CCM_ANALOG->PLL_AUDIO_NUM))) / ((uint64_t)(CCM_ANALOG->PLL_AUDIO_DENOM));

            freq = freq * divSelect + (uint32_t)freqTmp;

            /* AUDIO PLL output = PLL output frequency / POSTDIV. */

            /*
             * Post divider:
             *
             * PLL_AUDIO[POST_DIV_SELECT]:
             * 0x00: 4
             * 0x01: 2
             * 0x02: 1
             *
             * MISC2[AUDO_DIV]:
             * 0x00: 1
             * 0x01: 2
             * 0x02: 1
             * 0x03: 4
             */
            switch (CCM_ANALOG->PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT_MASK)
            {
                case CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(0U):
                    freq = freq >> 2U;
                    break;

                case CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(1U):
                    freq = freq >> 1U;
                    break;

                default:
                    break;
            }

            switch (CCM_ANALOG->MISC2 & (CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK | CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK))
            {
                case CCM_ANALOG_MISC2_AUDIO_DIV_MSB(1) | CCM_ANALOG_MISC2_AUDIO_DIV_LSB(1):
                    freq >>= 2U;
                    break;

                case CCM_ANALOG_MISC2_AUDIO_DIV_MSB(0) | CCM_ANALOG_MISC2_AUDIO_DIV_LSB(1):
                    freq >>= 1U;
                    break;

                default:
                    break;
            }
            break;

        case kCLOCK_PllVideo:
            /* PLL output frequency = Fref * (DIV_SELECT + NUM/DENOM). */
            divSelect = (CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_DIV_SELECT_MASK) >> CCM_ANALOG_PLL_VIDEO_DIV_SELECT_SHIFT;

            freqTmp = ((uint64_t)freq * ((uint64_t)(CCM_ANALOG->PLL_VIDEO_NUM))) / ((uint64_t)(CCM_ANALOG->PLL_VIDEO_DENOM));

            freq = freq * divSelect + (uint32_t)freqTmp;

            /* VIDEO PLL output = PLL output frequency / POSTDIV. */

            /*
             * Post divider:
             *
             * PLL_VIDEO[POST_DIV_SELECT]:
             * 0x00: 4
             * 0x01: 2
             * 0x02: 1
             *
             * MISC2[VIDEO_DIV]:
             * 0x00: 1
             * 0x01: 2
             * 0x02: 1
             * 0x03: 4
             */
            switch (CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT_MASK)
            {
                case CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(0U):
                    freq = freq >> 2U;
                    break;

                case CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(1U):
                    freq = freq >> 1U;
                    break;

                default:
                    break;
            }

            switch (CCM_ANALOG->MISC2 & CCM_ANALOG_MISC2_VIDEO_DIV_MASK)
            {
                case CCM_ANALOG_MISC2_VIDEO_DIV(3):
                    freq >>= 2U;
                    break;

                case CCM_ANALOG_MISC2_VIDEO_DIV(1):
                    freq >>= 1U;
                    break;

                default:
                    break;
            }
            break;

        case kCLOCK_PllEnet0:
            divSelect = (CCM_ANALOG->PLL_ENET & CCM_ANALOG_PLL_ENET_ENET0_DIV_SELECT_MASK)
                      >> CCM_ANALOG_PLL_ENET_ENET0_DIV_SELECT_SHIFT;
            freq = enetRefClkFreq[divSelect];
            break;

        case kCLOCK_PllEnet1:
            divSelect = (CCM_ANALOG->PLL_ENET & CCM_ANALOG_PLL_ENET_ENET1_DIV_SELECT_MASK)
                      >> CCM_ANALOG_PLL_ENET_ENET1_DIV_SELECT_SHIFT;
            freq = enetRefClkFreq[divSelect];
            break;

        case kCLOCK_PllEnet2:
            /* ref_enetpll2 if fixed at 25MHz. */
            freq = 25000000UL;
            break;

        case kCLOCK_PllUsb2:
            freq = (freq * ((CCM_ANALOG->PLL_USB2 & CCM_ANALOG_PLL_USB2_DIV_SELECT_MASK) ? 22U : 20U));
            break;

        default:
            freq = 0U;
            break;
    }

    return freq;
}

void CLOCK_InitSysPfd(clock_pfd_t pfd, uint8_t pfdFrac)
{
    uint32_t pfdIndex = (uint32_t)pfd;
    uint32_t pfd528;

    pfd528 = CCM_ANALOG->PFD_528 & ~((CCM_ANALOG_PFD_528_PFD0_CLKGATE_MASK | CCM_ANALOG_PFD_528_PFD0_FRAC_MASK) << (8 * pfdIndex));

    /* Disable the clock output first. */
    CCM_ANALOG->PFD_528 = pfd528 | (CCM_ANALOG_PFD_528_PFD0_CLKGATE_MASK << (8 * pfdIndex));

    /* Set the new value and enable output. */
    CCM_ANALOG->PFD_528 = pfd528 | (CCM_ANALOG_PFD_528_PFD0_FRAC(pfdFrac) << (8 * pfdIndex));
}

void CLOCK_DeinitSysPfd(clock_pfd_t pfd)
{
    CCM_ANALOG->PFD_528 |= CCM_ANALOG_PFD_528_PFD0_CLKGATE_MASK << (8 * pfd);
}

void CLOCK_InitUsb1Pfd(clock_pfd_t pfd, uint8_t pfdFrac)
{
    uint32_t pfdIndex = (uint32_t)pfd;
    uint32_t pfd480;

    pfd480 = CCM_ANALOG->PFD_480 & ~((CCM_ANALOG_PFD_480_PFD0_CLKGATE_MASK | CCM_ANALOG_PFD_480_PFD0_FRAC_MASK) << (8 * pfdIndex));

    /* Disable the clock output first. */
    CCM_ANALOG->PFD_480 = pfd480 | (CCM_ANALOG_PFD_480_PFD0_CLKGATE_MASK << (8 * pfdIndex));

    /* Set the new value and enable output. */
    CCM_ANALOG->PFD_480 = pfd480 | (CCM_ANALOG_PFD_480_PFD0_FRAC(pfdFrac) << (8 * pfdIndex));
}

void CLOCK_DeinitUsb1Pfd(clock_pfd_t pfd)
{
    CCM_ANALOG->PFD_480 |= CCM_ANALOG_PFD_480_PFD0_CLKGATE_MASK << (8 * pfd);
}

uint32_t CLOCK_GetSysPfdFreq(clock_pfd_t pfd)
{
    uint32_t freq = CLOCK_GetPllFreq(kCLOCK_PllSys);

    switch (pfd)
    {
        case kCLOCK_Pfd0:
            freq /= ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD0_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD0_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd1:
            freq /= ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD1_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD1_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd2:
            freq /= ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD2_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD2_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd3:
            freq /= ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD3_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD3_FRAC_SHIFT);
            break;

        default:
            freq = 0U;
            break;
    }
    freq *= 18U;

    return freq;
}

uint32_t CLOCK_GetUsb1PfdFreq(clock_pfd_t pfd)
{
    uint32_t freq = CLOCK_GetPllFreq(kCLOCK_PllUsb1);

    switch (pfd)
    {
        case kCLOCK_Pfd0:
            freq /= ((CCM_ANALOG->PFD_480 & CCM_ANALOG_PFD_480_PFD0_FRAC_MASK) >> CCM_ANALOG_PFD_480_PFD0_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd1:
            freq /= ((CCM_ANALOG->PFD_480 & CCM_ANALOG_PFD_480_PFD1_FRAC_MASK) >> CCM_ANALOG_PFD_480_PFD1_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd2:
            freq /= ((CCM_ANALOG->PFD_480 & CCM_ANALOG_PFD_480_PFD2_FRAC_MASK) >> CCM_ANALOG_PFD_480_PFD2_FRAC_SHIFT);
            break;

        case kCLOCK_Pfd3:
            freq /= ((CCM_ANALOG->PFD_480 & CCM_ANALOG_PFD_480_PFD3_FRAC_MASK) >> CCM_ANALOG_PFD_480_PFD3_FRAC_SHIFT);
            break;

        default:
            freq = 0U;
		    break;
    }
    freq *= 18U;

    return freq;
}

bool CLOCK_EnableUsbhs0Clock(clock_usb_src_t src, uint32_t freq)
{
    CCM->CCGR6 |= CCM_CCGR6_CG0_MASK ;
    USB1->USBCMD |= USBHS_USBCMD_RST_MASK;
    for (volatile uint32_t i = 0; i < 400000; i++)    /* Add a delay between RST and RS so make sure there is a DP pullup sequence*/
    {
        __ASM("nop");
    }
    PMU->REG_3P0 = (PMU->REG_3P0 & (~PMU_REG_3P0_OUTPUT_TRG_MASK)) | (PMU_REG_3P0_OUTPUT_TRG(0x17) | PMU_REG_3P0_ENABLE_LINREG_MASK);
    return true;
}


bool CLOCK_EnableUsbhs1Clock(clock_usb_src_t src, uint32_t freq)
{
    CCM->CCGR6 |= CCM_CCGR6_CG0_MASK ;
    USB2->USBCMD |= USBHS_USBCMD_RST_MASK;
    for (volatile uint32_t i = 0; i < 400000; i++)    /* Add a delay between RST and RS so make sure there is a DP pullup sequence*/
    {
        __ASM("nop");
    }
    PMU->REG_3P0 = (PMU->REG_3P0 & (~PMU_REG_3P0_OUTPUT_TRG_MASK)) | (PMU_REG_3P0_OUTPUT_TRG(0x17) | PMU_REG_3P0_ENABLE_LINREG_MASK);
    return true;
}


bool CLOCK_EnableUsbhs0PhyPllClock(clock_usb_phy_src_t src, uint32_t freq)
{
    const clock_usb_pll_config_t g_ccmConfigUsbPll  = {.loopDivider = 0U};
    CLOCK_InitUsb1Pll(&g_ccmConfigUsbPll);
    USBPHY1->CTRL &= ~USBPHY_CTRL_SFTRST_MASK;         /* release PHY from reset */
    USBPHY1->CTRL &= ~USBPHY_CTRL_CLKGATE_MASK;

    USBPHY1->PWD  = 0;
    USBPHY1->CTRL |=
    USBPHY_CTRL_ENAUTOCLR_PHY_PWD_MASK |
    USBPHY_CTRL_ENAUTOCLR_CLKGATE_MASK |
    USBPHY_CTRL_ENUTMILEVEL2_MASK |
    USBPHY_CTRL_ENUTMILEVEL3_MASK;
    return true;
}
bool CLOCK_EnableUsbhs1PhyPllClock(clock_usb_phy_src_t src, uint32_t freq)
{
    const clock_usb_pll_config_t g_ccmConfigUsbPll  = {.loopDivider = 0U};
    CLOCK_InitUsb2Pll(&g_ccmConfigUsbPll);
    USBPHY2->CTRL &= ~USBPHY_CTRL_SFTRST_MASK;         /* release PHY from reset */
    USBPHY2->CTRL &= ~USBPHY_CTRL_CLKGATE_MASK;

    USBPHY2->PWD  = 0;
    USBPHY2->CTRL |=
            USBPHY_CTRL_ENAUTOCLR_PHY_PWD_MASK |
            USBPHY_CTRL_ENAUTOCLR_CLKGATE_MASK |
            USBPHY_CTRL_ENUTMILEVEL2_MASK |
            USBPHY_CTRL_ENUTMILEVEL3_MASK;

    return true;
}
void CLOCK_DisableUsbhs0PhyPllClock(void)
{
    CLOCK_DeinitUsb1Pll();
    USBPHY1->CTRL |= USBPHY_CTRL_CLKGATE_MASK;          /* Set to 1U to gate clocks */

}
void CLOCK_DisableUsbhs1PhyPllClock(void)
{
    CLOCK_DeinitUsb2Pll();
    USBPHY2->CTRL |= USBPHY_CTRL_CLKGATE_MASK;          /* Set to 1U to gate clocks */
}
