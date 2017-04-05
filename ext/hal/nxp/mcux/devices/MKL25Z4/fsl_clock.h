/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
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
 * o Neither the name of copyright holder nor the names of its
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

#ifndef _FSL_CLOCK_H_
#define _FSL_CLOCK_H_

#include "fsl_common.h"

/*! @addtogroup clock */
/*! @{ */

/*! @file */

/*******************************************************************************
 * Configurations
 ******************************************************************************/

/*! @brief Configures whether to check a parameter in a function.
 *
 * Some MCG settings must be changed with conditions, for example:
 *  1. MCGIRCLK settings, such as the source, divider, and the trim value should not change when
 *     MCGIRCLK is used as a system clock source.
 *  2. MCG_C7[OSCSEL] should not be changed  when the external reference clock is used
 *     as a system clock source. For example, in FBE/BLPE/PBE modes.
 *  3. The users should only switch between the supported clock modes.
 *
 * MCG functions check the parameter and MCG status before setting, if not allowed
 * to change, the functions return error. The parameter checking increases code size,
 * if code size is a critical requirement, change #MCG_CONFIG_CHECK_PARAM to 0 to
 * disable parameter checking.
 */
#ifndef MCG_CONFIG_CHECK_PARAM
#define MCG_CONFIG_CHECK_PARAM 0U
#endif

/*! @brief Configure whether driver controls clock
 *
 * When set to 0, peripheral drivers will enable clock in initialize function
 * and disable clock in de-initialize function. When set to 1, peripheral
 * driver will not control the clock, application could contol the clock out of
 * the driver.
 *
 * @note All drivers share this feature switcher. If it is set to 1, application
 * should handle clock enable and disable for all drivers.
 */
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL))
#define FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL 0
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief CLOCK driver version 2.2.1. */
#define FSL_CLOCK_DRIVER_VERSION (MAKE_VERSION(2, 2, 1))
/*@}*/

/*! @brief External XTAL0 (OSC0) clock frequency.
 *
 * The XTAL0/EXTAL0 (OSC0) clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal0Freq to set the value in the clock driver. For example,
 * if XTAL0 is 8 MHz:
 * @code
 * CLOCK_InitOsc0(...); // Set up the OSC0
 * CLOCK_SetXtal0Freq(80000000); // Set the XTAL0 value to the clock driver.
 * @endcode
 *
 * This is important for the multicore platforms where only one core needs to set up the
 * OSC0 using the CLOCK_InitOsc0. All other cores need to call the CLOCK_SetXtal0Freq
 * to get a valid clock frequency.
 */
extern uint32_t g_xtal0Freq;

/*! @brief External XTAL32/EXTAL32/RTC_CLKIN clock frequency.
 *
 * The XTAL32/EXTAL32/RTC_CLKIN clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal32Freq to set the value in the clock driver.
 *
 * This is important for the multicore platforms where only one core needs to set up
 * the clock. All other cores need to call the CLOCK_SetXtal32Freq
 * to get a valid clock frequency.
 */
extern uint32_t g_xtal32Freq;

#if (defined(OSC) && !(defined(OSC0)))
#define OSC0 OSC
#endif

/*! @brief Clock ip name array for DMAMUX. */
#define DMAMUX_CLOCKS  \
    {                  \
        kCLOCK_Dmamux0 \
    }

/*! @brief Clock ip name array for RTC. */
#define RTC_CLOCKS  \
    {               \
        kCLOCK_Rtc0 \
    }

/*! @brief Clock ip name array for SPI. */
#define SPI_CLOCKS               \
    {                            \
        kCLOCK_Spi0, kCLOCK_Spi1 \
    }

/*! @brief Clock ip name array for PIT. */
#define PIT_CLOCKS  \
    {               \
        kCLOCK_Pit0 \
    }

/*! @brief Clock ip name array for PORT. */
#define PORT_CLOCKS                                                          \
    {                                                                        \
        kCLOCK_PortA, kCLOCK_PortB, kCLOCK_PortC, kCLOCK_PortD, kCLOCK_PortE \
    }

/*! @brief Clock ip name array for TSI. */
#define TSI_CLOCKS  \
    {               \
        kCLOCK_Tsi0 \
    }

/*! @brief Clock ip name array for DAC. */
#define DAC_CLOCKS  \
    {               \
        kCLOCK_Dac0 \
    }

/*! @brief Clock ip name array for LPTMR. */
#define LPTMR_CLOCKS  \
    {                 \
        kCLOCK_Lptmr0 \
    }

/*! @brief Clock ip name array for ADC16. */
#define ADC16_CLOCKS \
    {                \
        kCLOCK_Adc0  \
    }

/*! @brief Clock ip name array for DMA. */
#define DMA_CLOCKS  \
    {               \
        kCLOCK_Dma0 \
    }

/*! @brief Clock ip name array for LPSCI/UART0. */
#define UART0_CLOCKS \
    {                \
        kCLOCK_Uart0 \
    }

/*! @brief Clock ip name array for UART. */
#define UART_CLOCKS                                  \
    {                                                \
        kCLOCK_IpInvalid, kCLOCK_Uart1, kCLOCK_Uart2 \
    }

/*! @brief Clock ip name array for TPM. */
#define TPM_CLOCKS                            \
    {                                         \
        kCLOCK_Tpm0, kCLOCK_Tpm1, kCLOCK_Tpm2 \
    }

/*! @brief Clock ip name array for I2C. */
#define I2C_CLOCKS               \
    {                            \
        kCLOCK_I2c0, kCLOCK_I2c1 \
    }

/*! @brief Clock ip name array for FTF. */
#define FTF_CLOCKS  \
    {               \
        kCLOCK_Ftf0 \
    }

/*! @brief Clock ip name array for CMP. */
#define CMP_CLOCKS  \
    {               \
        kCLOCK_Cmp0 \
    }

/*!
 * @brief LPO clock frequency.
 */
#define LPO_CLK_FREQ 1000U

/*! @brief Peripherals clock source definition. */
#define SYS_CLK kCLOCK_CoreSysClk
#define BUS_CLK kCLOCK_BusClk

#define I2C0_CLK_SRC BUS_CLK
#define I2C1_CLK_SRC BUS_CLK
#define SPI0_CLK_SRC BUS_CLK
#define SPI1_CLK_SRC SYS_CLK
#define UART1_CLK_SRC BUS_CLK
#define UART2_CLK_SRC BUS_CLK

/*! @brief Clock name used to get clock frequency. */
typedef enum _clock_name
{

    /* ----------------------------- System layer clock -------------------------------*/
    kCLOCK_CoreSysClk,   /*!< Core/system clock                                         */
    kCLOCK_PlatClk,      /*!< Platform clock                                            */
    kCLOCK_BusClk,       /*!< Bus clock                                                 */
    kCLOCK_FlexBusClk,   /*!< FlexBus clock                                             */
    kCLOCK_FlashClk,     /*!< Flash clock                                               */
    kCLOCK_PllFllSelClk, /*!< The clock after SIM[PLLFLLSEL].                           */

    /* ---------------------------------- OSC clock -----------------------------------*/
    kCLOCK_Er32kClk,  /*!< External reference 32K clock (ERCLK32K)                   */
    kCLOCK_Osc0ErClk, /*!< OSC0 external reference clock (OSC0ERCLK)                 */

    /* ----------------------------- MCG and MCG-Lite clock ---------------------------*/
    kCLOCK_McgFixedFreqClk,   /*!< MCG fixed frequency clock (MCGFFCLK)                      */
    kCLOCK_McgInternalRefClk, /*!< MCG internal reference clock (MCGIRCLK)                   */
    kCLOCK_McgFllClk,         /*!< MCGFLLCLK                                                 */
    kCLOCK_McgPll0Clk,        /*!< MCGPLL0CLK                                                */
    kCLOCK_McgExtPllClk,      /*!< EXT_PLLCLK                                                */

    /* --------------------------------- Other clock ----------------------------------*/
    kCLOCK_LpoClk, /*!< LPO clock                                                 */

} clock_name_t;

/*! @brief USB clock source definition. */
typedef enum _clock_usb_src
{
    kCLOCK_UsbSrcPll0 = SIM_SOPT2_USBSRC(1U) | SIM_SOPT2_PLLFLLSEL(1U), /*!< Use PLL0.      */
    kCLOCK_UsbSrcExt = SIM_SOPT2_USBSRC(0U)                             /*!< Use USB_CLKIN. */
} clock_usb_src_t;

/*------------------------------------------------------------------------------

 clock_gate_t definition:

 31                              16                              0
 -----------------------------------------------------------------
 | SIM_SCGC register offset       |   control bit offset in SCGC |
 -----------------------------------------------------------------

 For example, the SDHC clock gate is controlled by SIM_SCGC3[17], the
 SIM_SCGC3 offset in SIM is 0x1030, then kCLOCK_GateSdhc0 is defined as

              kCLOCK_GateSdhc0 = (0x1030 << 16) | 17;

------------------------------------------------------------------------------*/

#define CLK_GATE_REG_OFFSET_SHIFT 16U
#define CLK_GATE_REG_OFFSET_MASK 0xFFFF0000U
#define CLK_GATE_BIT_SHIFT_SHIFT 0U
#define CLK_GATE_BIT_SHIFT_MASK 0x0000FFFFU

#define CLK_GATE_DEFINE(reg_offset, bit_shift)                                  \
    ((((reg_offset) << CLK_GATE_REG_OFFSET_SHIFT) & CLK_GATE_REG_OFFSET_MASK) | \
     (((bit_shift) << CLK_GATE_BIT_SHIFT_SHIFT) & CLK_GATE_BIT_SHIFT_MASK))

#define CLK_GATE_ABSTRACT_REG_OFFSET(x) (((x)&CLK_GATE_REG_OFFSET_MASK) >> CLK_GATE_REG_OFFSET_SHIFT)
#define CLK_GATE_ABSTRACT_BITS_SHIFT(x) (((x)&CLK_GATE_BIT_SHIFT_MASK) >> CLK_GATE_BIT_SHIFT_SHIFT)

/*! @brief Clock gate name used for CLOCK_EnableClock/CLOCK_DisableClock. */
typedef enum _clock_ip_name
{
    kCLOCK_IpInvalid = 0U,
    kCLOCK_I2c0 = CLK_GATE_DEFINE(0x1034U, 6U),
    kCLOCK_I2c1 = CLK_GATE_DEFINE(0x1034U, 7U),
    kCLOCK_Uart0 = CLK_GATE_DEFINE(0x1034U, 10U),
    kCLOCK_Uart1 = CLK_GATE_DEFINE(0x1034U, 11U),
    kCLOCK_Uart2 = CLK_GATE_DEFINE(0x1034U, 12U),
    kCLOCK_Usbfs0 = CLK_GATE_DEFINE(0x1034U, 18U),
    kCLOCK_Cmp0 = CLK_GATE_DEFINE(0x1034U, 19U),
    kCLOCK_Spi0 = CLK_GATE_DEFINE(0x1034U, 22U),
    kCLOCK_Spi1 = CLK_GATE_DEFINE(0x1034U, 23U),

    kCLOCK_Lptmr0 = CLK_GATE_DEFINE(0x1038U, 0U),
    kCLOCK_Tsi0 = CLK_GATE_DEFINE(0x1038U, 5U),
    kCLOCK_PortA = CLK_GATE_DEFINE(0x1038U, 9U),
    kCLOCK_PortB = CLK_GATE_DEFINE(0x1038U, 10U),
    kCLOCK_PortC = CLK_GATE_DEFINE(0x1038U, 11U),
    kCLOCK_PortD = CLK_GATE_DEFINE(0x1038U, 12U),
    kCLOCK_PortE = CLK_GATE_DEFINE(0x1038U, 13U),

    kCLOCK_Ftf0 = CLK_GATE_DEFINE(0x103CU, 0U),
    kCLOCK_Dmamux0 = CLK_GATE_DEFINE(0x103CU, 1U),
    kCLOCK_Pit0 = CLK_GATE_DEFINE(0x103CU, 23U),
    kCLOCK_Tpm0 = CLK_GATE_DEFINE(0x103CU, 24U),
    kCLOCK_Tpm1 = CLK_GATE_DEFINE(0x103CU, 25U),
    kCLOCK_Tpm2 = CLK_GATE_DEFINE(0x103CU, 26U),
    kCLOCK_Adc0 = CLK_GATE_DEFINE(0x103CU, 27U),
    kCLOCK_Rtc0 = CLK_GATE_DEFINE(0x103CU, 29U),
    kCLOCK_Dac0 = CLK_GATE_DEFINE(0x103CU, 31U),

    kCLOCK_Dma0 = CLK_GATE_DEFINE(0x1040U, 8U),
} clock_ip_name_t;

/*!@brief SIM configuration structure for clock setting. */
typedef struct _sim_clock_config
{
    uint8_t pllFllSel;
    uint8_t er32kSrc; /*!< ERCLK32K source selection.   */
    uint32_t clkdiv1; /*!< SIM_CLKDIV1.                 */
} sim_clock_config_t;

/*! @brief OSC work mode. */
typedef enum _osc_mode
{
    kOSC_ModeExt = 0U, /*!< Use an external clock.   */
#if (defined(MCG_C2_EREFS_MASK) && !(defined(MCG_C2_EREFS0_MASK)))
    kOSC_ModeOscLowPower = MCG_C2_EREFS_MASK, /*!< Oscillator low power. */
#else
    kOSC_ModeOscLowPower = MCG_C2_EREFS0_MASK, /*!< Oscillator low power. */
#endif
    kOSC_ModeOscHighGain = 0U
#if (defined(MCG_C2_EREFS_MASK) && !(defined(MCG_C2_EREFS0_MASK)))
                           |
                           MCG_C2_EREFS_MASK
#else
                           |
                           MCG_C2_EREFS0_MASK
#endif
#if (defined(MCG_C2_HGO_MASK) && !(defined(MCG_C2_HGO0_MASK)))
                           |
                           MCG_C2_HGO_MASK, /*!< Oscillator high gain. */
#else
                           |
                           MCG_C2_HGO0_MASK, /*!< Oscillator high gain. */
#endif
} osc_mode_t;

/*! @brief Oscillator capacitor load setting.*/
enum _osc_cap_load
{
    kOSC_Cap2P = OSC_CR_SC2P_MASK,  /*!< 2  pF capacitor load */
    kOSC_Cap4P = OSC_CR_SC4P_MASK,  /*!< 4  pF capacitor load */
    kOSC_Cap8P = OSC_CR_SC8P_MASK,  /*!< 8  pF capacitor load */
    kOSC_Cap16P = OSC_CR_SC16P_MASK /*!< 16 pF capacitor load */
};

/*! @brief OSCERCLK enable mode. */
enum _oscer_enable_mode
{
    kOSC_ErClkEnable = OSC_CR_ERCLKEN_MASK,       /*!< Enable.              */
    kOSC_ErClkEnableInStop = OSC_CR_EREFSTEN_MASK /*!< Enable in stop mode. */
};

/*! @brief OSC configuration for OSCERCLK. */
typedef struct _oscer_config
{
    uint8_t enableMode; /*!< OSCERCLK enable mode. OR'ed value of @ref _oscer_enable_mode. */

} oscer_config_t;

/*!
 * @brief OSC Initialization Configuration Structure
 *
 * Defines the configuration data structure to initialize the OSC.
 * When porting to a new board, set the following members
 * according to the board setting:
 * 1. freq: The external frequency.
 * 2. workMode: The OSC module mode.
 */
typedef struct _osc_config
{
    uint32_t freq;              /*!< External clock frequency.    */
    uint8_t capLoad;            /*!< Capacitor load setting.      */
    osc_mode_t workMode;        /*!< OSC work mode setting.       */
    oscer_config_t oscerConfig; /*!< Configuration for OSCERCLK.  */
} osc_config_t;

/*! @brief MCG FLL reference clock source select. */
typedef enum _mcg_fll_src
{
    kMCG_FllSrcExternal, /*!< External reference clock is selected          */
    kMCG_FllSrcInternal  /*!< The slow internal reference clock is selected */
} mcg_fll_src_t;

/*! @brief MCG internal reference clock select */
typedef enum _mcg_irc_mode
{
    kMCG_IrcSlow, /*!< Slow internal reference clock selected */
    kMCG_IrcFast  /*!< Fast internal reference clock selected */
} mcg_irc_mode_t;

/*! @brief MCG DCO Maximum Frequency with 32.768 kHz Reference */
typedef enum _mcg_dmx32
{
    kMCG_Dmx32Default, /*!< DCO has a default range of 25% */
    kMCG_Dmx32Fine     /*!< DCO is fine-tuned for maximum frequency with 32.768 kHz reference */
} mcg_dmx32_t;

/*! @brief MCG DCO range select */
typedef enum _mcg_drs
{
    kMCG_DrsLow,     /*!< Low frequency range       */
    kMCG_DrsMid,     /*!< Mid frequency range       */
    kMCG_DrsMidHigh, /*!< Mid-High frequency range  */
    kMCG_DrsHigh     /*!< High frequency range      */
} mcg_drs_t;

/*! @brief MCG PLL reference clock select */
typedef enum _mcg_pll_ref_src
{
    kMCG_PllRefOsc0, /*!< Selects OSC0 as PLL reference clock                 */
    kMCG_PllRefOsc1  /*!< Selects OSC1 as PLL reference clock                 */
} mcg_pll_ref_src_t;

/*! @brief MCGOUT clock source. */
typedef enum _mcg_clkout_src
{
    kMCG_ClkOutSrcOut,      /*!< Output of the FLL is selected (reset default)  */
    kMCG_ClkOutSrcInternal, /*!< Internal reference clock is selected           */
    kMCG_ClkOutSrcExternal, /*!< External reference clock is selected           */
} mcg_clkout_src_t;

/*! @brief MCG Automatic Trim Machine Select */
typedef enum _mcg_atm_select
{
    kMCG_AtmSel32k, /*!< 32 kHz Internal Reference Clock selected  */
    kMCG_AtmSel4m   /*!< 4 MHz Internal Reference Clock selected   */
} mcg_atm_select_t;

/*! @brief MCG OSC Clock Select */
typedef enum _mcg_oscsel
{
    kMCG_OscselOsc, /*!< Selects System Oscillator (OSCCLK) */
    kMCG_OscselRtc, /*!< Selects 32 kHz RTC Oscillator      */
} mcg_oscsel_t;

/*! @brief MCG PLLCS select */
typedef enum _mcg_pll_clk_select
{
    kMCG_PllClkSelPll0, /*!< PLL0 output clock is selected  */
    kMCG_PllClkSelPll1  /* PLL1 output clock is selected    */
} mcg_pll_clk_select_t;

/*! @brief MCG clock monitor mode. */
typedef enum _mcg_monitor_mode
{
    kMCG_MonitorNone, /*!< Clock monitor is disabled.         */
    kMCG_MonitorInt,  /*!< Trigger interrupt when clock lost. */
    kMCG_MonitorReset /*!< System reset when clock lost.      */
} mcg_monitor_mode_t;

/*! @brief MCG status. */
enum _mcg_status
{
    kStatus_MCG_ModeUnreachable = MAKE_STATUS(kStatusGroup_MCG, 0),       /*!< Can't switch to target mode. */
    kStatus_MCG_ModeInvalid = MAKE_STATUS(kStatusGroup_MCG, 1),           /*!< Current mode invalid for the specific
                                                                               function. */
    kStatus_MCG_AtmBusClockInvalid = MAKE_STATUS(kStatusGroup_MCG, 2),    /*!< Invalid bus clock for ATM. */
    kStatus_MCG_AtmDesiredFreqInvalid = MAKE_STATUS(kStatusGroup_MCG, 3), /*!< Invalid desired frequency for ATM. */
    kStatus_MCG_AtmIrcUsed = MAKE_STATUS(kStatusGroup_MCG, 4),            /*!< IRC is used when using ATM. */
    kStatus_MCG_AtmHardwareFail = MAKE_STATUS(kStatusGroup_MCG, 5),       /*!< Hardware fail occurs during ATM. */
    kStatus_MCG_SourceUsed = MAKE_STATUS(kStatusGroup_MCG, 6)             /*!< Can't change the clock source because
                                                                               it is in use. */
};

/*! @brief MCG status flags. */
enum _mcg_status_flags_t
{
    kMCG_Osc0LostFlag = (1U << 0U), /*!< OSC0 lost.         */
    kMCG_Osc0InitFlag = (1U << 1U), /*!< OSC0 crystal initialized. */
    kMCG_Pll0LostFlag = (1U << 5U), /*!< PLL0 lost.         */
    kMCG_Pll0LockFlag = (1U << 6U), /*!< PLL0 locked.       */
};

/*! @brief MCG internal reference clock (MCGIRCLK) enable mode definition. */
enum _mcg_irclk_enable_mode
{
    kMCG_IrclkEnable = MCG_C1_IRCLKEN_MASK,       /*!< MCGIRCLK enable.              */
    kMCG_IrclkEnableInStop = MCG_C1_IREFSTEN_MASK /*!< MCGIRCLK enable in stop mode. */
};

/*! @brief MCG PLL clock enable mode definition. */
enum _mcg_pll_enable_mode
{
    kMCG_PllEnableIndependent = MCG_C5_PLLCLKEN0_MASK, /*!< MCGPLLCLK enable independent of the
                                                           MCG clock mode. Generally, the PLL
                                                           is disabled in FLL modes
                                                           (FEI/FBI/FEE/FBE). Setting the PLL clock
                                                           enable independent, enables the
                                                           PLL in the FLL modes.          */
    kMCG_PllEnableInStop = MCG_C5_PLLSTEN0_MASK        /*!< MCGPLLCLK enable in STOP mode. */
};

/*! @brief MCG mode definitions */
typedef enum _mcg_mode
{
    kMCG_ModeFEI = 0U, /*!< FEI   - FLL Engaged Internal         */
    kMCG_ModeFBI,      /*!< FBI   - FLL Bypassed Internal        */
    kMCG_ModeBLPI,     /*!< BLPI  - Bypassed Low Power Internal  */
    kMCG_ModeFEE,      /*!< FEE   - FLL Engaged External         */
    kMCG_ModeFBE,      /*!< FBE   - FLL Bypassed External        */
    kMCG_ModeBLPE,     /*!< BLPE  - Bypassed Low Power External  */
    kMCG_ModePBE,      /*!< PBE   - PLL Bypassed External        */
    kMCG_ModePEE,      /*!< PEE   - PLL Engaged External         */
    kMCG_ModeError     /*!< Unknown mode                         */
} mcg_mode_t;

/*! @brief MCG PLL configuration. */
typedef struct _mcg_pll_config
{
    uint8_t enableMode; /*!< Enable mode. OR'ed value of @ref _mcg_pll_enable_mode. */
    uint8_t prdiv;      /*!< Reference divider PRDIV.    */
    uint8_t vdiv;       /*!< VCO divider VDIV.           */
} mcg_pll_config_t;

/*! @brief MCG mode change configuration structure
 *
 * When porting to a new board, set the following members
 * according to the board setting:
 * 1. frdiv: If the FLL uses the external reference clock, set this
 *    value to ensure that the external reference clock divided by frdiv is
 *    in the 31.25 kHz to 39.0625 kHz range.
 * 2. The PLL reference clock divider PRDIV: PLL reference clock frequency after
 *    PRDIV should be in the FSL_FEATURE_MCG_PLL_REF_MIN to
 *    FSL_FEATURE_MCG_PLL_REF_MAX range.
 */
typedef struct _mcg_config
{
    mcg_mode_t mcgMode; /*!< MCG mode.                   */

    /* ----------------------- MCGIRCCLK settings ------------------------ */
    uint8_t irclkEnableMode; /*!< MCGIRCLK enable mode.       */
    mcg_irc_mode_t ircs;     /*!< Source, MCG_C2[IRCS].       */
    uint8_t fcrdiv;          /*!< Divider, MCG_SC[FCRDIV].    */

    /* ------------------------ MCG FLL settings ------------------------- */
    uint8_t frdiv;     /*!< Divider MCG_C1[FRDIV].      */
    mcg_drs_t drs;     /*!< DCO range MCG_C4[DRST_DRS]. */
    mcg_dmx32_t dmx32; /*!< MCG_C4[DMX32].              */

    /* ------------------------ MCG PLL settings ------------------------- */
    mcg_pll_config_t pll0Config; /*!< MCGPLL0CLK configuration.   */

} mcg_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @brief Enable the clock for specific IP.
 *
 * @param name  Which clock to enable, see \ref clock_ip_name_t.
 */
static inline void CLOCK_EnableClock(clock_ip_name_t name)
{
    uint32_t regAddr = SIM_BASE + CLK_GATE_ABSTRACT_REG_OFFSET((uint32_t)name);
    (*(volatile uint32_t *)regAddr) |= (1U << CLK_GATE_ABSTRACT_BITS_SHIFT((uint32_t)name));
}

/*!
 * @brief Disable the clock for specific IP.
 *
 * @param name  Which clock to disable, see \ref clock_ip_name_t.
 */
static inline void CLOCK_DisableClock(clock_ip_name_t name)
{
    uint32_t regAddr = SIM_BASE + CLK_GATE_ABSTRACT_REG_OFFSET((uint32_t)name);
    (*(volatile uint32_t *)regAddr) &= ~(1U << CLK_GATE_ABSTRACT_BITS_SHIFT((uint32_t)name));
}

/*! @brief Set ERCLK32K source. */
static inline void CLOCK_SetEr32kClock(uint32_t src)
{
    SIM->SOPT1 = ((SIM->SOPT1 & ~SIM_SOPT1_OSC32KSEL_MASK) | SIM_SOPT1_OSC32KSEL(src));
}

/*! @brief Set PLLFLLSEL clock source. */
static inline void CLOCK_SetPllFllSelClock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_PLLFLLSEL_MASK) | SIM_SOPT2_PLLFLLSEL(src));
}

/*! @brief Set TPM clock source. */
static inline void CLOCK_SetTpmClock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(src));
}

/*! @brief Set LPSCI0 (UART0) clock source. */
static inline void CLOCK_SetLpsci0Clock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_UART0SRC_MASK) | SIM_SOPT2_UART0SRC(src));
}

/*! @brief Enable USB FS clock.
 *
 * @param src  USB FS clock source.
 * @param freq The frequency specified by src.
 * @retval true The clock is set successfully.
 * @retval false The clock source is invalid to get proper USB FS clock.
 */
bool CLOCK_EnableUsbfs0Clock(clock_usb_src_t src, uint32_t freq);

/*! @brief Disable USB FS clock.
 *
 * Disable USB FS clock.
 */
static inline void CLOCK_DisableUsbfs0Clock(void)
{
    CLOCK_DisableClock(kCLOCK_Usbfs0);
}

/*! @brief Set CLKOUT source. */
static inline void CLOCK_SetClkOutClock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_CLKOUTSEL_MASK) | SIM_SOPT2_CLKOUTSEL(src));
}

/*! @brief Set RTC_CLKOUT source. */
static inline void CLOCK_SetRtcClkOutClock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_RTCCLKOUTSEL_MASK) | SIM_SOPT2_RTCCLKOUTSEL(src));
}

/*!
 * @brief
 * Set the SIM_CLKDIV1[OUTDIV1], SIM_CLKDIV1[OUTDIV4].
 */
static inline void CLOCK_SetOutDiv(uint32_t outdiv1, uint32_t outdiv4)
{
    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(outdiv1) | SIM_CLKDIV1_OUTDIV4(outdiv4);
}

/*!
 * @brief Gets the clock frequency for a specific clock name.
 *
 * This function checks the current clock configurations and then calculates
 * the clock frequency for a specific clock name defined in clock_name_t.
 * The MCG must be properly configured before using this function.
 *
 * @param clockName Clock names defined in clock_name_t
 * @return Clock frequency value in Hertz
 */
uint32_t CLOCK_GetFreq(clock_name_t clockName);

/*!
 * @brief Get the core clock or system clock frequency.
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetCoreSysClkFreq(void);

/*!
 * @brief Get the platform clock frequency.
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetPlatClkFreq(void);

/*!
 * @brief Get the bus clock frequency.
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetBusClkFreq(void);

/*!
 * @brief Get the flash clock frequency.
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetFlashClkFreq(void);

/*!
 * @brief Get the output clock frequency selected by SIM[PLLFLLSEL].
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetPllFllSelClkFreq(void);

/*!
 * @brief Get the external reference 32K clock frequency (ERCLK32K).
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetEr32kClkFreq(void);

/*!
 * @brief Get the OSC0 external reference clock frequency (OSC0ERCLK).
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetOsc0ErClkFreq(void);

/*!
 * @brief Set the clock configure in SIM module.
 *
 * This function sets system layer clock settings in SIM module.
 *
 * @param config Pointer to the configure structure.
 */
void CLOCK_SetSimConfig(sim_clock_config_t const *config);

/*!
 * @brief Set the system clock dividers in SIM to safe value.
 *
 * The system level clocks (core clock, bus clock, flexbus clock and flash clock)
 * must be in allowed ranges. During MCG clock mode switch, the MCG output clock
 * changes then the system level clocks may be out of range. This function could
 * be used before MCG mode change, to make sure system level clocks are in allowed
 * range.
 *
 * @param config Pointer to the configure structure.
 */
static inline void CLOCK_SetSimSafeDivs(void)
{
    SIM->CLKDIV1 = 0x10030000U;
}

/*! @name MCG frequency functions. */
/*@{*/

/*!
 * @brief Gets the MCG output clock (MCGOUTCLK) frequency.
 *
 * This function gets the MCG output clock frequency in Hz based on the current MCG
 * register value.
 *
 * @return The frequency of MCGOUTCLK.
 */
uint32_t CLOCK_GetOutClkFreq(void);

/*!
 * @brief Gets the MCG FLL clock (MCGFLLCLK) frequency.
 *
 * This function gets the MCG FLL clock frequency in Hz based on the current MCG
 * register value. The FLL is enabled in FEI/FBI/FEE/FBE mode and
 * disabled in low power state in other modes.
 *
 * @return The frequency of MCGFLLCLK.
 */
uint32_t CLOCK_GetFllFreq(void);

/*!
 * @brief Gets the MCG internal reference clock (MCGIRCLK) frequency.
 *
 * This function gets the MCG internal reference clock frequency in Hz based
 * on the current MCG register value.
 *
 * @return The frequency of MCGIRCLK.
 */
uint32_t CLOCK_GetInternalRefClkFreq(void);

/*!
 * @brief Gets the MCG fixed frequency clock (MCGFFCLK) frequency.
 *
 * This function gets the MCG fixed frequency clock frequency in Hz based
 * on the current MCG register value.
 *
 * @return The frequency of MCGFFCLK.
 */
uint32_t CLOCK_GetFixedFreqClkFreq(void);

/*!
 * @brief Gets the MCG PLL0 clock (MCGPLL0CLK) frequency.
 *
 * This function gets the MCG PLL0 clock frequency in Hz based on the current MCG
 * register value.
 *
 * @return The frequency of MCGPLL0CLK.
 */
uint32_t CLOCK_GetPll0Freq(void);

/*@}*/

/*! @name MCG clock configuration. */
/*@{*/

/*!
 * @brief Enables or disables the MCG low power.
 *
 * Enabling the MCG low power disables the PLL and FLL in bypass modes. In other words,
 * in FBE and PBE modes, enabling low power sets the MCG to BLPE mode. In FBI and
 * PBI modes, enabling low power sets the MCG to BLPI mode.
 * When disabling the MCG low power, the PLL or FLL are enabled based on MCG settings.
 *
 * @param enable True to enable MCG low power, false to disable MCG low power.
 */
static inline void CLOCK_SetLowPowerEnable(bool enable)
{
    if (enable)
    {
        MCG->C2 |= MCG_C2_LP_MASK;
    }
    else
    {
        MCG->C2 &= ~MCG_C2_LP_MASK;
    }
}

/*!
 * @brief Configures the Internal Reference clock (MCGIRCLK).
 *
 * This function sets the \c MCGIRCLK base on parameters. It also selects the IRC
 * source. If the fast IRC is used, this function sets the fast IRC divider.
 * This function also sets whether the \c MCGIRCLK is enabled in stop mode.
 * Calling this function in FBI/PBI/BLPI modes may change the system clock. As a result,
 * using the function in these modes it is not allowed.
 *
 * @param enableMode MCGIRCLK enable mode, OR'ed value of @ref _mcg_irclk_enable_mode.
 * @param ircs       MCGIRCLK clock source, choose fast or slow.
 * @param fcrdiv     Fast IRC divider setting (\c FCRDIV).
 * @retval kStatus_MCG_SourceUsed Because the internall reference clock is used as a clock source,
 * the confuration should not be changed. Otherwise, a glitch occurs.
 * @retval kStatus_Success MCGIRCLK configuration finished successfully.
 */
status_t CLOCK_SetInternalRefClkConfig(uint8_t enableMode, mcg_irc_mode_t ircs, uint8_t fcrdiv);

/*!
 * @brief Selects the MCG external reference clock.
 *
 * Selects the MCG external reference clock source, changes the MCG_C7[OSCSEL],
 * and waits for the clock source to be stable. Because the external reference
 * clock should not be changed in FEE/FBE/BLPE/PBE/PEE modes, do not call this function in these modes.
 *
 * @param oscsel MCG external reference clock source, MCG_C7[OSCSEL].
 * @retval kStatus_MCG_SourceUsed Because the external reference clock is used as a clock source,
 * the confuration should not be changed. Otherwise, a glitch occurs.
 * @retval kStatus_Success External reference clock set successfully.
 */
status_t CLOCK_SetExternalRefClkConfig(mcg_oscsel_t oscsel);

/*!
 * @brief Set the FLL external reference clock divider value.
 *
 * Sets the FLL external reference clock divider value, the register MCG_C1[FRDIV].
 *
 * @param frdiv The FLL external reference clock divider value, MCG_C1[FRDIV].
 */
static inline void CLOCK_SetFllExtRefDiv(uint8_t frdiv)
{
    MCG->C1 = (MCG->C1 & ~MCG_C1_FRDIV_MASK) | MCG_C1_FRDIV(frdiv);
}

/*!
 * @brief Enables the PLL0 in FLL mode.
 *
 * This function sets us the PLL0 in FLL mode and reconfigures
 * the PLL0. Ensure that the PLL reference
 * clock is enabled before calling this function and that the PLL0 is not used as a clock source.
 * The function CLOCK_CalcPllDiv gets the correct PLL
 * divider values.
 *
 * @param config Pointer to the configuration structure.
 */
void CLOCK_EnablePll0(mcg_pll_config_t const *config);

/*!
 * @brief Disables the PLL0 in FLL mode.
 *
 * This function disables the PLL0 in FLL mode. It should be used together with the
 * @ref CLOCK_EnablePll0.
 */
static inline void CLOCK_DisablePll0(void)
{
    MCG->C5 &= ~(MCG_C5_PLLCLKEN0_MASK | MCG_C5_PLLSTEN0_MASK);
}

/*!
 * @brief Calculates the PLL divider setting for a desired output frequency.
 *
 * This function calculates the correct reference clock divider (\c PRDIV) and
 * VCO divider (\c VDIV) to generate a desired PLL output frequency. It returns the
 * closest frequency match with the corresponding \c PRDIV/VDIV
 * returned from parameters. If a desired frequency is not valid, this function
 * returns 0.
 *
 * @param refFreq    PLL reference clock frequency.
 * @param desireFreq Desired PLL output frequency.
 * @param prdiv      PRDIV value to generate desired PLL frequency.
 * @param vdiv       VDIV value to generate desired PLL frequency.
 * @return Closest frequency match that the PLL was able generate.
 */
uint32_t CLOCK_CalcPllDiv(uint32_t refFreq, uint32_t desireFreq, uint8_t *prdiv, uint8_t *vdiv);

/*@}*/

/*! @name MCG clock lock monitor functions. */
/*@{*/

/*!
 * @brief Sets the OSC0 clock monitor mode.
 *
 * This function sets the OSC0 clock monitor mode. See @ref mcg_monitor_mode_t for details.
 *
 * @param mode Monitor mode to set.
 */
void CLOCK_SetOsc0MonitorMode(mcg_monitor_mode_t mode);

/*!
 * @brief Sets the PLL0 clock monitor mode.
 *
 * This function sets the PLL0 clock monitor mode. See @ref mcg_monitor_mode_t for details.
 *
 * @param mode Monitor mode to set.
 */
void CLOCK_SetPll0MonitorMode(mcg_monitor_mode_t mode);

/*!
 * @brief Gets the MCG status flags.
 *
 * This function gets the MCG clock status flags. All status flags are
 * returned as a logical OR of the enumeration @ref _mcg_status_flags_t. To
 * check a specific flag, compare the return value with the flag.
 *
 * Example:
 * @code
   // To check the clock lost lock status of OSC0 and PLL0.
   uint32_t mcgFlags;

   mcgFlags = CLOCK_GetStatusFlags();

   if (mcgFlags & kMCG_Osc0LostFlag)
   {
       // OSC0 clock lock lost. Do something.
   }
   if (mcgFlags & kMCG_Pll0LostFlag)
   {
       // PLL0 clock lock lost. Do something.
   }
   @endcode
 *
 * @return  Logical OR value of the @ref _mcg_status_flags_t.
 */
uint32_t CLOCK_GetStatusFlags(void);

/*!
 * @brief Clears the MCG status flags.
 *
 * This function clears the MCG clock lock lost status. The parameter is a logical
 * OR value of the flags to clear. See @ref _mcg_status_flags_t.
 *
 * Example:
 * @code
   // To clear the clock lost lock status flags of OSC0 and PLL0.

   CLOCK_ClearStatusFlags(kMCG_Osc0LostFlag | kMCG_Pll0LostFlag);
   @endcode
 *
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration @ref _mcg_status_flags_t.
 */
void CLOCK_ClearStatusFlags(uint32_t mask);

/*@}*/

/*!
 * @name OSC configuration
 * @{
 */

/*!
 * @brief Configures the OSC external reference clock (OSCERCLK).
 *
 * This function configures the OSC external reference clock (OSCERCLK).
 * This is an example to enable the OSCERCLK in normal and stop modes and also set
 * the output divider to 1:
 *
   @code
   oscer_config_t config =
   {
       .enableMode = kOSC_ErClkEnable | kOSC_ErClkEnableInStop,
       .erclkDiv   = 1U,
   };

   OSC_SetExtRefClkConfig(OSC, &config);
   @endcode
 *
 * @param base   OSC peripheral address.
 * @param config Pointer to the configuration structure.
 */
static inline void OSC_SetExtRefClkConfig(OSC_Type *base, oscer_config_t const *config)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_ERCLKEN_MASK | OSC_CR_EREFSTEN_MASK);
    reg |= config->enableMode;

    base->CR = reg;
}

/*!
 * @brief Sets the capacitor load configuration for the oscillator.
 *
 * This function sets the specified capacitors configuration for the oscillator.
 * This should be done in the early system level initialization function call
 * based on the system configuration.
 *
 * @param base   OSC peripheral address.
 * @param capLoad OR'ed value for the capacitor load option, see \ref _osc_cap_load.
 *
 * Example:
   @code
   // To enable only 2 pF and 8 pF capacitor load, please use like this.
   OSC_SetCapLoad(OSC, kOSC_Cap2P | kOSC_Cap8P);
   @endcode
 */
static inline void OSC_SetCapLoad(OSC_Type *base, uint8_t capLoad)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_SC2P_MASK | OSC_CR_SC4P_MASK | OSC_CR_SC8P_MASK | OSC_CR_SC16P_MASK);
    reg |= capLoad;

    base->CR = reg;
}

/*!
 * @brief Initializes the OSC0.
 *
 * This function initializes the OSC0 according to the board configuration.
 *
 * @param  config Pointer to the OSC0 configuration structure.
 */
void CLOCK_InitOsc0(osc_config_t const *config);

/*!
 * @brief Deinitializes the OSC0.
 *
 * This function deinitializes the OSC0.
 */
void CLOCK_DeinitOsc0(void);

/* @} */

/*!
 * @name External clock frequency
 * @{
 */

/*!
 * @brief Sets the XTAL0 frequency based on board settings.
 *
 * @param freq The XTAL0/EXTAL0 input clock frequency in Hz.
 */
static inline void CLOCK_SetXtal0Freq(uint32_t freq)
{
    g_xtal0Freq = freq;
}

/*!
 * @brief Sets the XTAL32/RTC_CLKIN frequency based on board settings.
 *
 * @param freq The XTAL32/EXTAL32/RTC_CLKIN input clock frequency in Hz.
 */
static inline void CLOCK_SetXtal32Freq(uint32_t freq)
{
    g_xtal32Freq = freq;
}
/* @} */

/*!
 * @name MCG auto-trim machine.
 * @{
 */

/*!
 * @brief Auto trims the internal reference clock.
 *
 * This function trims the internal reference clock by using the external clock. If
 * successful, it returns the kStatus_Success and the frequency after
 * trimming is received in the parameter @p actualFreq. If an error occurs,
 * the error code is returned.
 *
 * @param extFreq      External clock frequency, which should be a bus clock.
 * @param desireFreq   Frequency to trim to.
 * @param actualFreq   Actual frequency after trimming.
 * @param atms         Trim fast or slow internal reference clock.
 * @retval kStatus_Success ATM success.
 * @retval kStatus_MCG_AtmBusClockInvalid The bus clock is not in allowed range for the ATM.
 * @retval kStatus_MCG_AtmDesiredFreqInvalid MCGIRCLK could not be trimmed to the desired frequency.
 * @retval kStatus_MCG_AtmIrcUsed Could not trim because MCGIRCLK is used as a bus clock source.
 * @retval kStatus_MCG_AtmHardwareFail Hardware fails while trimming.
 */
status_t CLOCK_TrimInternalRefClk(uint32_t extFreq, uint32_t desireFreq, uint32_t *actualFreq, mcg_atm_select_t atms);
/* @} */

/*! @name MCG mode functions. */
/*@{*/

/*!
 * @brief Gets the current MCG mode.
 *
 * This function checks the MCG registers and determines the current MCG mode.
 *
 * @return Current MCG mode or error code; See @ref mcg_mode_t.
 */
mcg_mode_t CLOCK_GetMode(void);

/*!
 * @brief Sets the MCG to FEI mode.
 *
 * This function sets the MCG to FEI mode. If setting to FEI mode fails
 * from the current mode, this function returns an error.
 *
 * @param       dmx32  DMX32 in FEI mode.
 * @param       drs The DCO range selection.
 * @param       fllStableDelay Delay function to  ensure that the FLL is stable. Passing
 *              NULL does not cause a delay.
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 * @note If @p dmx32 is set to kMCG_Dmx32Fine, the slow IRC must not be trimmed
 * to a frequency above 32768 Hz.
 */
status_t CLOCK_SetFeiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to FEE mode.
 *
 * This function sets the MCG to FEE mode. If setting to FEE mode fails
 * from the current mode, this function returns an error.
 *
 * @param   frdiv  FLL reference clock divider setting, FRDIV.
 * @param   dmx32  DMX32 in FEE mode.
 * @param   drs    The DCO range selection.
 * @param   fllStableDelay Delay function to make sure FLL is stable. Passing
 *          NULL does not cause a delay.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_SetFeeMode(uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to FBI mode.
 *
 * This function sets the MCG to FBI mode. If setting to FBI mode fails
 * from the current mode, this function returns an error.
 *
 * @param  dmx32  DMX32 in FBI mode.
 * @param  drs  The DCO range selection.
 * @param  fllStableDelay Delay function to make sure FLL is stable. If the FLL
 *         is not used in FBI mode, this parameter can be NULL. Passing
 *         NULL does not cause a delay.
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 * @note If @p dmx32 is set to kMCG_Dmx32Fine, the slow IRC must not be trimmed
 * to frequency above 32768 Hz.
 */
status_t CLOCK_SetFbiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to FBE mode.
 *
 * This function sets the MCG to FBE mode. If setting to FBE mode fails
 * from the current mode, this function returns an error.
 *
 * @param   frdiv  FLL reference clock divider setting, FRDIV.
 * @param   dmx32  DMX32 in FBE mode.
 * @param   drs    The DCO range selection.
 * @param   fllStableDelay Delay function to make sure FLL is stable. If the FLL
 *          is not used in FBE mode, this parameter can be NULL. Passing NULL
 *          does not cause a delay.
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_SetFbeMode(uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to BLPI mode.
 *
 * This function sets the MCG to BLPI mode. If setting to BLPI mode fails
 * from the current mode, this function returns an error.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_SetBlpiMode(void);

/*!
 * @brief Sets the MCG to BLPE mode.
 *
 * This function sets the MCG to BLPE mode. If setting to BLPE mode fails
 * from the current mode, this function returns an error.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_SetBlpeMode(void);

/*!
 * @brief Sets the MCG to PBE mode.
 *
 * This function sets the MCG to PBE mode. If setting to PBE mode fails
 * from the current mode, this function returns an error.
 *
 * @param   pllcs  The PLL selection, PLLCS.
 * @param   config Pointer to the PLL configuration.
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 *
 * @note
 * 1. The parameter \c pllcs selects the PLL. For platforms with
 * only one PLL, the parameter pllcs is kept for interface compatibility.
 * 2. The parameter \c config is the PLL configuration structure. On some
 * platforms,  it is possible to choose the external PLL directly, which renders the
 * configuration structure not necessary. In this case, pass in NULL.
 * For example: CLOCK_SetPbeMode(kMCG_OscselOsc, kMCG_PllClkSelExtPll, NULL);
 */
status_t CLOCK_SetPbeMode(mcg_pll_clk_select_t pllcs, mcg_pll_config_t const *config);

/*!
 * @brief Sets the MCG to PEE mode.
 *
 * This function sets the MCG to PEE mode.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 *
 * @note This function only changes the CLKS to use the PLL/FLL output. If the
 *       PRDIV/VDIV are different than in the PBE mode, set them up
 *       in PBE mode and wait. When the clock is stable, switch to PEE mode.
 */
status_t CLOCK_SetPeeMode(void);

/*!
 * @brief Switches the MCG to FBE mode from the external mode.
 *
 * This function switches the MCG from external modes (PEE/PBE/BLPE/FEE) to the FBE mode quickly.
 * The external clock is used as the system clock souce and PLL is disabled. However,
 * the FLL settings are not configured. This is a lite function with a small code size, which is useful
 * during the mode switch. For example, to switch from PEE mode to FEI mode:
 *
 * @code
 * CLOCK_ExternalModeToFbeModeQuick();
 * CLOCK_SetFeiMode(...);
 * @endcode
 *
 * @retval kStatus_Success Switched successfully.
 * @retval kStatus_MCG_ModeInvalid If the current mode is not an external mode, do not call this function.
 */
status_t CLOCK_ExternalModeToFbeModeQuick(void);

/*!
 * @brief Switches the MCG to FBI mode from internal modes.
 *
 * This function switches the MCG from internal modes (PEI/PBI/BLPI/FEI) to the FBI mode quickly.
 * The MCGIRCLK is used as the system clock souce and PLL is disabled. However,
 * FLL settings are not configured. This is a lite function with a small code size, which is useful
 * during the mode switch. For example, to switch from PEI mode to FEE mode:
 *
 * @code
 * CLOCK_InternalModeToFbiModeQuick();
 * CLOCK_SetFeeMode(...);
 * @endcode
 *
 * @retval kStatus_Success Switched successfully.
 * @retval kStatus_MCG_ModeInvalid If the current mode is not an internal mode, do not call this function.
 */
status_t CLOCK_InternalModeToFbiModeQuick(void);

/*!
 * @brief Sets the MCG to FEI mode during system boot up.
 *
 * This function sets the MCG to FEI mode from the reset mode. It can also be used to
 * set up MCG during system boot up.
 *
 * @param  dmx32  DMX32 in FEI mode.
 * @param  drs The DCO range selection.
 * @param  fllStableDelay Delay function to ensure that the FLL is stable.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 * @note If @p dmx32 is set to kMCG_Dmx32Fine, the slow IRC must not be trimmed
 * to frequency above 32768 Hz.
 */
status_t CLOCK_BootToFeiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to FEE mode during system bootup.
 *
 * This function sets MCG to FEE mode from the reset mode. It can also be used to
 * set up the MCG during system boot up.
 *
 * @param   oscsel OSC clock select, OSCSEL.
 * @param   frdiv  FLL reference clock divider setting, FRDIV.
 * @param   dmx32  DMX32 in FEE mode.
 * @param   drs    The DCO range selection.
 * @param   fllStableDelay Delay function to ensure that the FLL is stable.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_BootToFeeMode(
    mcg_oscsel_t oscsel, uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void));

/*!
 * @brief Sets the MCG to BLPI mode during system boot up.
 *
 * This function sets the MCG to BLPI mode from the reset mode. It can also be used to
 * set up the MCG during sytem boot up.
 *
 * @param  fcrdiv Fast IRC divider, FCRDIV.
 * @param  ircs   The internal reference clock to select, IRCS.
 * @param  ircEnableMode  The MCGIRCLK enable mode, OR'ed value of @ref _mcg_irclk_enable_mode.
 *
 * @retval kStatus_MCG_SourceUsed Could not change MCGIRCLK setting.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_BootToBlpiMode(uint8_t fcrdiv, mcg_irc_mode_t ircs, uint8_t ircEnableMode);

/*!
 * @brief Sets the MCG to BLPE mode during sytem boot up.
 *
 * This function sets the MCG to BLPE mode from the reset mode. It can also be used to
 * set up the MCG during sytem boot up.
 *
 * @param  oscsel OSC clock select, MCG_C7[OSCSEL].
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_BootToBlpeMode(mcg_oscsel_t oscsel);

/*!
 * @brief Sets the MCG to PEE mode during system boot up.
 *
 * This function sets the MCG to PEE mode from reset mode. It can also be used to
 * set up the MCG during system boot up.
 *
 * @param   oscsel OSC clock select, MCG_C7[OSCSEL].
 * @param   pllcs  The PLL selection, PLLCS.
 * @param   config Pointer to the PLL configuration.
 *
 * @retval kStatus_MCG_ModeUnreachable Could not switch to the target mode.
 * @retval kStatus_Success Switched to the target mode successfully.
 */
status_t CLOCK_BootToPeeMode(mcg_oscsel_t oscsel, mcg_pll_clk_select_t pllcs, mcg_pll_config_t const *config);

/*!
 * @brief Sets the MCG to a target mode.
 *
 * This function sets MCG to a target mode defined by the configuration
 * structure. If switching to the target mode fails, this function
 * chooses the correct path.
 *
 * @param  config Pointer to the target MCG mode configuration structure.
 * @return Return kStatus_Success if switched successfully; Otherwise, it returns an error code #_mcg_status.
 *
 * @note If the external clock is used in the target mode, ensure that it is
 * enabled. For example, if the OSC0 is used, set up OSC0 correctly before calling this
 * function.
 */
status_t CLOCK_SetMcgConfig(mcg_config_t const *config);

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @} */

#endif /* _FSL_CLOCK_H_ */
