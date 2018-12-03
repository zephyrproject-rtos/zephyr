/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
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
/*! @brief CLOCK driver version 2.1.0. */
#define FSL_CLOCK_DRIVER_VERSION (MAKE_VERSION(2, 1, 0))
/*@}*/

/*! @brief External XTAL0 (OSC0/SYSOSC) clock frequency.
 *
 * The XTAL0/EXTAL0 (OSC0/SYSOSC) clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal0Freq to set the value in the clock driver. For example,
 * if XTAL0 is 8 MHz:
 * @code
 * CLOCK_InitSysOsc(...); // Set up the OSC0/SYSOSC
 * CLOCK_SetXtal0Freq(80000000); // Set the XTAL0 value in the clock driver.
 * @endcode
 *
 * This is important for the multicore platforms where only one core needs to set up the
 * OSC0/SYSOSC using CLOCK_InitSysOsc. All other cores need to call the CLOCK_SetXtal0Freq
 * to get a valid clock frequency.
 */
extern uint32_t g_xtal0Freq;

/*! @brief External XTAL32/EXTAL32 clock frequency.
 *
 * The XTAL32/EXTAL32 clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal32Freq to set the value in the clock driver.
 *
 * This is important for the multicore platforms where only one core needs to set up
 * the clock. All other cores need to call the CLOCK_SetXtal32Freq
 * to get a valid clock frequency.
 */
extern uint32_t g_xtal32Freq;

/*! @brief Clock ip name array for MAX. */
#define MAX_CLOCKS  \
    {               \
        kCLOCK_Max0 \
    }

/*! @brief Clock ip name array for EDMA. */
#define EDMA_CLOCKS                \
    {                              \
        kCLOCK_Edma0, kCLOCK_Edma1 \
    }

/*! @brief Clock ip name array for FLEXBUS. */
#define FLEXBUS_CLOCKS \
    {                  \
        kCLOCK_Flexbus \
    }

/*! @brief XRDC clock gate number. */
#define FSL_CLOCK_XRDC_GATE_COUNT (5U)

/*! @brief Clock ip name array for XRDC. */
#define XRDC_CLOCKS                                                                           \
    {                                                                                         \
        kCLOCK_Xrdc0Mgr, kCLOCK_Xrdc0Pac, kCLOCK_Xrdc0Mrc, kCLOCK_Xrdc0PacB, kCLOCK_Xrdc0MrcB \
    }

/*! @brief Clock ip name array for SEMA42. */
#define SEMA42_CLOCKS                  \
    {                                  \
        kCLOCK_Sema420, kCLOCK_Sema421 \
    }

/*! @brief Clock ip name array for DMAMUX. */
#define DMAMUX_CLOCKS                  \
    {                                  \
        kCLOCK_Dmamux0, kCLOCK_Dmamux1 \
    }

/*! @brief Clock ip name array for MU. */
#if (defined(RV32M1_cm0plus_SERIES) || defined(RV32M1_zero_riscy_SERIES))
#define MU_CLOCKS  \
    {              \
        kCLOCK_MuB \
    }
#else
#define MU_CLOCKS  \
    {              \
        kCLOCK_MuA \
    }
#endif

/*! @brief Clock ip name array for CRC. */
#define CRC_CLOCKS  \
    {               \
        kCLOCK_Crc0 \
    }

/*! @brief Clock ip name array for LPIT. */
#define LPIT_CLOCKS                \
    {                              \
        kCLOCK_Lpit0, kCLOCK_Lpit1 \
    }

/*! @brief Clock ip name array for TPM. */
#define TPM_CLOCKS                                         \
    {                                                      \
        kCLOCK_Tpm0, kCLOCK_Tpm1, kCLOCK_Tpm2, kCLOCK_Tpm3 \
    }

/*! @brief Clock ip name array for TRNG. */
#define TRNG_CLOCKS \
    {               \
        kCLOCK_Trng \
    }

/*! @brief Clock ip name array for SMVSIM. */
#define EMVSIM_CLOCKS  \
    {                  \
        kCLOCK_Emvsim0 \
    }

/*! @brief Clock ip name array for EWM. */
#define EWM_CLOCKS  \
    {               \
        kCLOCK_Ewm0 \
    }

/*! @brief Clock ip name array for FLEXIO. */
#define FLEXIO_CLOCKS  \
    {                  \
        kCLOCK_Flexio0 \
    }

/*! @brief Clock ip name array for LPI2C0. */
#define LPI2C_CLOCKS                                               \
    {                                                              \
        kCLOCK_Lpi2c0, kCLOCK_Lpi2c1, kCLOCK_Lpi2c2, kCLOCK_Lpi2c3 \
    }

/*! @brief Clock ip name array for SAI. */
#define SAI_CLOCKS  \
    {               \
        kCLOCK_Sai0 \
    }

/*! @brief Clock ip name array for SDHC. */
#define USDHC_CLOCKS \
    {                \
        kCLOCK_Sdhc0 \
    }

/*! @brief Clock ip name array for LPSPI. */
#define LPSPI_CLOCKS                                               \
    {                                                              \
        kCLOCK_Lpspi0, kCLOCK_Lpspi1, kCLOCK_Lpspi2, kCLOCK_Lpspi3 \
    }

/*! @brief Clock ip name array for LPUART. */
#define LPUART_CLOCKS                                                  \
    {                                                                  \
        kCLOCK_Lpuart0, kCLOCK_Lpuart1, kCLOCK_Lpuart2, kCLOCK_Lpuart3 \
    }

/*! @brief Clock ip name array for USB. */
#define USB_CLOCKS  \
    {               \
        kCLOCK_Usb0 \
    }

/*! @brief Clock ip name array for PORT. */
#define PORT_CLOCKS                                                          \
    {                                                                        \
        kCLOCK_PortA, kCLOCK_PortB, kCLOCK_PortC, kCLOCK_PortD, kCLOCK_PortE \
    }

/*! @brief Clock ip name array for LPADC. */
#define LPADC_CLOCKS  \
    {                 \
        kCLOCK_Lpadc0 \
    }

/*! @brief Clock ip name array for DAC. */
#define LPDAC_CLOCKS \
    {                \
        kCLOCK_Dac0  \
    }

/*! @brief Clock ip name array for INTMUX. */
#if (defined(RV32M1_ri5cy_SERIES) || defined(RV32M1_zero_riscy_SERIES))
#define INTMUX_CLOCKS                  \
    {                                  \
        kCLOCK_Intmux0, kCLOCK_Intmux1 \
    }
#else
#define INTMUX_CLOCKS                    \
    {                                    \
        kCLOCK_IpInvalid, kCLOCK_Intmux0 \
    }
#endif

/*! @brief Clock ip name array for EXT. */
#define EXT_CLOCKS               \
    {                            \
        kCLOCK_Ext0, kCLOCK_Ext1 \
    }

/*! @brief Clock ip name array for VREF. */
#define VREF_CLOCKS \
    {               \
        kCLOCK_Vref \
    }

/*! @brief Clock ip name array for FGPIO. */
#define FGPIO_CLOCKS                                                                          \
    {                                                                                         \
        kCLOCK_IpInvalid, kCLOCK_IpInvalid, kCLOCK_IpInvalid, kCLOCK_IpInvalid, kCLOCK_Rgpio1 \
    }

/*! @brief Clock name used to get clock frequency.
 *
 * These clocks source would be generated from SCG module.
 */
typedef enum _clock_name
{
    /* ----------------------------- System layer clock -------------------------------*/
    kCLOCK_CoreSysClk, /*!< Core 0/1 clock. */
    kCLOCK_SlowClk,    /*!< SLOW_CLK with DIVSLOW. */
    kCLOCK_PlatClk,    /*!< PLAT_CLK. */
    kCLOCK_SysClk,     /*!< SYS_CLK. */
    kCLOCK_BusClk,     /*!< BUS_CLK with DIVBUS. */
    kCLOCK_ExtClk,     /*!< One clock selection of CLKOUT from main clock after DIVCORE and DIVEXT divider.*/

    /* ------------------------------------ SCG clock ---------------------------------*/
    kCLOCK_ScgSysLpFllAsyncDiv1Clk, /*!< LPFLL_DIV1_CLK. */
    kCLOCK_ScgSysLpFllAsyncDiv2Clk, /*!< LPFLL_DIV1_CLK. */
    kCLOCK_ScgSysLpFllAsyncDiv3Clk, /*!< LPFLL_DIV1_CLK. */

    kCLOCK_ScgSircAsyncDiv1Clk, /*!< SIRCDIV1_CLK.                                   */
    kCLOCK_ScgSircAsyncDiv2Clk, /*!< SIRCDIV2_CLK.                                   */
    kCLOCK_ScgSircAsyncDiv3Clk, /*!< SIRCDIV3_CLK.                                   */

    kCLOCK_ScgFircAsyncDiv1Clk, /*!< FIRCDIV1_CLK.                                   */
    kCLOCK_ScgFircAsyncDiv2Clk, /*!< FIRCDIV2_CLK.                                   */
    kCLOCK_ScgFircAsyncDiv3Clk, /*!< FIRCDIV3_CLK.                                   */

    kCLOCK_ScgSysOscAsyncDiv1Clk, /*!< SOSCDIV1_CLK.                                   */
    kCLOCK_ScgSysOscAsyncDiv2Clk, /*!< SOSCDIV2_CLK.                                   */
    kCLOCK_ScgSysOscAsyncDiv3Clk, /*!< SOSCDIV3_CLK.                                   */

    /* For SCG_CLKOUT output */
    /* kCLOCK_ExtClk, */
    kCLOCK_ScgSysOscClk, /*!< SCG system OSC clock. (SYSOSC)                      */
    kCLOCK_ScgSircClk,   /*!< SCG SIRC clock.                                     */
    kCLOCK_ScgFircClk,   /*!< SCG FIRC clock.                                     */
    kCLOCK_RtcOscClk,    /*!< RTC OSC clock. */
    kCLOCK_ScgLpFllClk,  /*!< SCG Low-power FLL clock. (LPFLL)                     */

    /* --------------------------------- Other clock ----------------------------------*/
    kCLOCK_LpoClk,    /*!< LPO clock                                           */
    kCLOCK_Osc32kClk, /*!< External OSC 32K clock (OSC32KCLK)                  */
} clock_name_t;

#define kCLOCK_FlashClk kCLOCK_SlowClk
#define kCLOCK_RfClk kCLOCK_ScgSysOscClk
#define LPO_CLK_FREQ 1000U

/*!
 * @brief Clock source for peripherals that support various clock selections.
 *
 * These options are for PCC->CLKCFG[PCS].
 */
typedef enum _clock_ip_src
{
    kCLOCK_IpSrcNoneOrExt = 0U,   /*!< Clock is off or external clock is used. */
    kCLOCK_IpSrcSysOscAsync = 1U, /*!< System Oscillator async clock.          */
    kCLOCK_IpSrcSircAsync = 2U,   /*!< Slow IRC async clock.                   */
    kCLOCK_IpSrcFircAsync = 3U,   /*!< Fast IRC async clock.                   */
    kCLOCK_IpSrcLpFllAsync = 6U   /*!< System LPFLL async clock.               */
} clock_ip_src_t;

/*!
 * @brief Peripheral clock name difinition used for clock gate, clock source
 * and clock divider setting. It is defined as the corresponding register address.
 */
#define MAKE_PCC_REGADDR(base, offset) ((base) + (offset))
typedef enum _clock_ip_name
{
    kCLOCK_IpInvalid = 0U,
    /* PCC 0 */
    kCLOCK_Mscm = MAKE_PCC_REGADDR(PCC0_BASE, 0x4),
    kCLOCK_Syspm = MAKE_PCC_REGADDR(PCC0_BASE, 0xC),
    kCLOCK_Max0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x10),
    kCLOCK_Edma0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x20),
    kCLOCK_Flexbus = MAKE_PCC_REGADDR(PCC0_BASE, 0x30),
    kCLOCK_Xrdc0Mgr = MAKE_PCC_REGADDR(PCC0_BASE, 0x50),
    kCLOCK_Xrdc0Pac = MAKE_PCC_REGADDR(PCC0_BASE, 0x58),
    kCLOCK_Xrdc0Mrc = MAKE_PCC_REGADDR(PCC0_BASE, 0x5C),
    kCLOCK_Sema420 = MAKE_PCC_REGADDR(PCC0_BASE, 0x6C),
    kCLOCK_Dmamux0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x84),
    kCLOCK_Ewm0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x88),
    kCLOCK_MuA = MAKE_PCC_REGADDR(PCC0_BASE, 0x94),
    kCLOCK_Crc0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xBC),
    kCLOCK_Lpit0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xC0),
    kCLOCK_Tpm0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xD4),
    kCLOCK_Tpm1 = MAKE_PCC_REGADDR(PCC0_BASE, 0xD8),
    kCLOCK_Tpm2 = MAKE_PCC_REGADDR(PCC0_BASE, 0xDC),
    kCLOCK_Emvsim0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xE0),
    kCLOCK_Flexio0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xE4),
    kCLOCK_Lpi2c0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xE8),
    kCLOCK_Lpi2c1 = MAKE_PCC_REGADDR(PCC0_BASE, 0xEC),
    kCLOCK_Lpi2c2 = MAKE_PCC_REGADDR(PCC0_BASE, 0xF0),
    kCLOCK_Sai0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xF4),
    kCLOCK_Sdhc0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xF8),
    kCLOCK_Lpspi0 = MAKE_PCC_REGADDR(PCC0_BASE, 0xFC),
    kCLOCK_Lpspi1 = MAKE_PCC_REGADDR(PCC0_BASE, 0x100),
    kCLOCK_Lpspi2 = MAKE_PCC_REGADDR(PCC0_BASE, 0x104),
    kCLOCK_Lpuart0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x108),
    kCLOCK_Lpuart1 = MAKE_PCC_REGADDR(PCC0_BASE, 0x10C),
    kCLOCK_Lpuart2 = MAKE_PCC_REGADDR(PCC0_BASE, 0x110),
    kCLOCK_Usb0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x114),
    kCLOCK_PortA = MAKE_PCC_REGADDR(PCC0_BASE, 0x118),
    kCLOCK_PortB = MAKE_PCC_REGADDR(PCC0_BASE, 0x11C),
    kCLOCK_PortC = MAKE_PCC_REGADDR(PCC0_BASE, 0x120),
    kCLOCK_PortD = MAKE_PCC_REGADDR(PCC0_BASE, 0x124),
    kCLOCK_Lpadc0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x128),
    kCLOCK_Dac0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x130),
    kCLOCK_Vref = MAKE_PCC_REGADDR(PCC0_BASE, 0x134),
    kCLOCK_Atx = MAKE_PCC_REGADDR(PCC0_BASE, 0x138),
#if (defined(RV32M1_ri5cy_SERIES) || defined(RV32M1_zero_riscy_SERIES))
    kCLOCK_Intmux0 = MAKE_PCC_REGADDR(PCC0_BASE, 0x13C),
#endif
    kCLOCK_Trace = MAKE_PCC_REGADDR(PCC0_BASE, 0x200),
    /* PCC1. */
    kCLOCK_Edma1 = MAKE_PCC_REGADDR(PCC1_BASE, 0x20),
    kCLOCK_Rgpio1 = MAKE_PCC_REGADDR(PCC1_BASE, 0x3C),
    kCLOCK_Xrdc0PacB = MAKE_PCC_REGADDR(PCC1_BASE, 0x58),
    kCLOCK_Xrdc0MrcB = MAKE_PCC_REGADDR(PCC1_BASE, 0x5C),
    kCLOCK_Sema421 = MAKE_PCC_REGADDR(PCC1_BASE, 0x6C),
    kCLOCK_Dmamux1 = MAKE_PCC_REGADDR(PCC1_BASE, 0x84),
#if (defined(RV32M1_ri5cy_SERIES) || defined(RV32M1_zero_riscy_SERIES))
    kCLOCK_Intmux1 = MAKE_PCC_REGADDR(PCC1_BASE, 0x88),
#else
    kCLOCK_Intmux0 = MAKE_PCC_REGADDR(PCC1_BASE, 0x88),
#endif
    kCLOCK_MuB = MAKE_PCC_REGADDR(PCC1_BASE, 0x90),
    kCLOCK_Cau3 = MAKE_PCC_REGADDR(PCC1_BASE, 0xA0),
    kCLOCK_Trng = MAKE_PCC_REGADDR(PCC1_BASE, 0xA4),
    kCLOCK_Lpit1 = MAKE_PCC_REGADDR(PCC1_BASE, 0xA8),
    kCLOCK_Tpm3 = MAKE_PCC_REGADDR(PCC1_BASE, 0xB4),
    kCLOCK_Lpi2c3 = MAKE_PCC_REGADDR(PCC1_BASE, 0xB8),
    kCLOCK_Lpspi3 = MAKE_PCC_REGADDR(PCC1_BASE, 0xD4),
    kCLOCK_Lpuart3 = MAKE_PCC_REGADDR(PCC1_BASE, 0xD8),
    kCLOCK_PortE = MAKE_PCC_REGADDR(PCC1_BASE, 0xDC),
    kCLOCK_Ext0 = MAKE_PCC_REGADDR(PCC1_BASE, 0x200),
    kCLOCK_Ext1 = MAKE_PCC_REGADDR(PCC1_BASE, 0x204),
} clock_ip_name_t;

/*!
 * @brief USB clock source definition.
 */
typedef enum _clock_usb_src
{
    kCLOCK_UsbSrcIrc48M = 1,           /*!< Use IRC48M.    */
    kCLOCK_UsbSrcUnused = 0xFFFFFFFFU, /*!< Used when the function does not
                                               care the clock source. */
} clock_usb_src_t;

/*!
 * @brief SCG status return codes.
 */
enum _scg_status
{
    kStatus_SCG_Busy = MAKE_STATUS(kStatusGroup_SCG, 1),      /*!< Clock is busy.  */
    kStatus_SCG_InvalidSrc = MAKE_STATUS(kStatusGroup_SCG, 2) /*!< Invalid source. */
};

/*!
 * @brief SCG system clock type.
 */
typedef enum _scg_sys_clk
{
    kSCG_SysClkSlow, /*!< System slow clock. */
    kSCG_SysClkBus,  /*!< Bus clock.         */
    kSCG_SysClkExt,  /*!< External clock.    */
    kSCG_SysClkCore, /*!< Core clock.        */
} scg_sys_clk_t;

/*!
 * @brief SCG system clock source.
 */
typedef enum _scg_sys_clk_src
{
    kSCG_SysClkSrcSysOsc = 1U, /*!< System OSC. */
    kSCG_SysClkSrcSirc = 2U,   /*!< Slow IRC.   */
    kSCG_SysClkSrcFirc = 3U,   /*!< Fast IRC.   */
    kSCG_SysClkSrcRosc = 4U,   /*!< RTC OSC. */
    kSCG_SysClkSrcLpFll = 5U,  /*!< Low power FLL. */
} scg_sys_clk_src_t;

/*!
 * @brief SCG system clock divider value.
 */
typedef enum _scg_sys_clk_div
{
    kSCG_SysClkDivBy1 = 0U,   /*!< Divided by 1.  */
    kSCG_SysClkDivBy2 = 1U,   /*!< Divided by 2.  */
    kSCG_SysClkDivBy3 = 2U,   /*!< Divided by 3.  */
    kSCG_SysClkDivBy4 = 3U,   /*!< Divided by 4.  */
    kSCG_SysClkDivBy5 = 4U,   /*!< Divided by 5.  */
    kSCG_SysClkDivBy6 = 5U,   /*!< Divided by 6.  */
    kSCG_SysClkDivBy7 = 6U,   /*!< Divided by 7.  */
    kSCG_SysClkDivBy8 = 7U,   /*!< Divided by 8.  */
    kSCG_SysClkDivBy9 = 8U,   /*!< Divided by 9.  */
    kSCG_SysClkDivBy10 = 9U,  /*!< Divided by 10. */
    kSCG_SysClkDivBy11 = 10U, /*!< Divided by 11. */
    kSCG_SysClkDivBy12 = 11U, /*!< Divided by 12. */
    kSCG_SysClkDivBy13 = 12U, /*!< Divided by 13. */
    kSCG_SysClkDivBy14 = 13U, /*!< Divided by 14. */
    kSCG_SysClkDivBy15 = 14U, /*!< Divided by 15. */
    kSCG_SysClkDivBy16 = 15U  /*!< Divided by 16. */
} scg_sys_clk_div_t;

/*!
 * @brief SCG system clock configuration.
 */
typedef struct _scg_sys_clk_config
{
    uint32_t divSlow : 4; /*!< Slow clock divider, see @ref scg_sys_clk_div_t. */
    uint32_t divBus : 4;  /*!< Bus clock divider, see @ref scg_sys_clk_div_t.  */
    uint32_t divExt : 4;  /*!< External clock divider, see @ref scg_sys_clk_div_t.  */
    uint32_t : 4;         /*!< Reserved. */
    uint32_t divCore : 4; /*!< Core clock divider, see @ref scg_sys_clk_div_t. */
    uint32_t : 4;         /*!< Reserved. */
    uint32_t src : 4;     /*!< System clock source, see @ref scg_sys_clk_src_t. */
    uint32_t : 4;         /*!< reserved. */
} scg_sys_clk_config_t;

/*!
 * @brief SCG clock out configuration (CLKOUTSEL).
 */
typedef enum _clock_clkout_src
{
    kClockClkoutSelScgExt = 0U,    /*!< SCG external clock. */
    kClockClkoutSelSysOsc = 1U,    /*!< System OSC.     */
    kClockClkoutSelSirc = 2U,      /*!< Slow IRC.       */
    kClockClkoutSelFirc = 3U,      /*!< Fast IRC.       */
    kClockClkoutSelScgRtcOsc = 4U, /*!< SCG RTC OSC clock. */
    kClockClkoutSelLpFll = 5U,     /*!< Low power FLL.  */
} clock_clkout_src_t;

/*!
 * @brief SCG asynchronous clock type.
 */
typedef enum _scg_async_clk
{
    kSCG_AsyncDiv1Clk, /*!< The async clock by DIV1, e.g. SOSCDIV1_CLK, SIRCDIV1_CLK. */
    kSCG_AsyncDiv2Clk, /*!< The async clock by DIV2, e.g. SOSCDIV2_CLK, SIRCDIV2_CLK. */
    kSCG_AsyncDiv3Clk  /*!< The async clock by DIV3, e.g. SOSCDIV3_CLK, SIRCDIV3_CLK. */
} scg_async_clk_t;

/*!
 * @brief SCG asynchronous clock divider value.
 */
typedef enum scg_async_clk_div
{
    kSCG_AsyncClkDisable = 0U, /*!< Clock output is disabled.  */
    kSCG_AsyncClkDivBy1 = 1U,  /*!< Divided by 1.              */
    kSCG_AsyncClkDivBy2 = 2U,  /*!< Divided by 2.              */
    kSCG_AsyncClkDivBy4 = 3U,  /*!< Divided by 4.              */
    kSCG_AsyncClkDivBy8 = 4U,  /*!< Divided by 8.              */
    kSCG_AsyncClkDivBy16 = 5U, /*!< Divided by 16.             */
    kSCG_AsyncClkDivBy32 = 6U, /*!< Divided by 32.             */
    kSCG_AsyncClkDivBy64 = 7U  /*!< Divided by 64.             */
} scg_async_clk_div_t;

/*!
 * @brief SCG system OSC monitor mode.
 */
typedef enum _scg_sosc_monitor_mode
{
    kSCG_SysOscMonitorDisable = 0U,                  /*!< Monitor disabled.                          */
    kSCG_SysOscMonitorInt = SCG_SOSCCSR_SOSCCM_MASK, /*!< Interrupt when the system OSC error is detected. */
    kSCG_SysOscMonitorReset =
        SCG_SOSCCSR_SOSCCM_MASK | SCG_SOSCCSR_SOSCCMRE_MASK /*!< Reset when the system OSC error is detected.     */
} scg_sosc_monitor_mode_t;

/*! @brief OSC enable mode. */
enum _scg_sosc_enable_mode
{
    kSCG_SysOscEnable = SCG_SOSCCSR_SOSCEN_MASK,             /*!< Enable OSC clock. */
    kSCG_SysOscEnableInStop = SCG_SOSCCSR_SOSCSTEN_MASK,     /*!< Enable OSC in stop mode. */
    kSCG_SysOscEnableInLowPower = SCG_SOSCCSR_SOSCLPEN_MASK, /*!< Enable OSC in low power mode. */
};

/*!
 * @brief SCG system OSC configuration.
 */
typedef struct _scg_sosc_config
{
    uint32_t freq;                       /*!< System OSC frequency.                    */
    scg_sosc_monitor_mode_t monitorMode; /*!< Clock monitor mode selected.     */
    uint8_t enableMode;                  /*!< Enable mode, OR'ed value of _scg_sosc_enable_mode.  */

    scg_async_clk_div_t div1; /*!< SOSCDIV1 value.                          */
    scg_async_clk_div_t div2; /*!< SOSCDIV2 value.                          */
    scg_async_clk_div_t div3; /*!< SOSCDIV3 value.                          */

} scg_sosc_config_t;

/*!
 * @brief SCG slow IRC clock frequency range.
 */
typedef enum _scg_sirc_range
{
    kSCG_SircRangeLow, /*!< Slow IRC low range clock (2 MHz, 4 MHz for i.MX 7 ULP).  */
    kSCG_SircRangeHigh /*!< Slow IRC high range clock (8 MHz, 16 MHz for i.MX 7 ULP). */
} scg_sirc_range_t;

/*! @brief SIRC enable mode. */
enum _scg_sirc_enable_mode
{
    kSCG_SircEnable = SCG_SIRCCSR_SIRCEN_MASK,            /*!< Enable SIRC clock.             */
    kSCG_SircEnableInStop = SCG_SIRCCSR_SIRCSTEN_MASK,    /*!< Enable SIRC in stop mode.      */
    kSCG_SircEnableInLowPower = SCG_SIRCCSR_SIRCLPEN_MASK /*!< Enable SIRC in low power mode. */
};

/*!
 * @brief SCG slow IRC clock configuration.
 */
typedef struct _scg_sirc_config
{
    uint32_t enableMode;      /*!< Enable mode, OR'ed value of _scg_sirc_enable_mode. */
    scg_async_clk_div_t div1; /*!< SIRCDIV1 value.                          */
    scg_async_clk_div_t div2; /*!< SIRCDIV2 value.                          */
    scg_async_clk_div_t div3; /*!< SIRCDIV3 value.                          */

    scg_sirc_range_t range; /*!< Slow IRC frequency range.                */
} scg_sirc_config_t;

/*!
 * @brief SCG fast IRC trim mode.
 */
typedef enum _scg_firc_trim_mode
{
    kSCG_FircTrimNonUpdate = SCG_FIRCCSR_FIRCTREN_MASK,
    /*!< FIRC trim enable but not enable trim value update. In this mode, the
     trim value is fixed to the initialized value which is defined by
     trimCoar and trimFine in configure structure \ref scg_firc_trim_config_t.*/

    kSCG_FircTrimUpdate = SCG_FIRCCSR_FIRCTREN_MASK | SCG_FIRCCSR_FIRCTRUP_MASK
    /*!< FIRC trim enable and trim value update enable. In this mode, the trim
     value is auto update. */

} scg_firc_trim_mode_t;

/*!
 * @brief SCG fast IRC trim predivided value for system OSC.
 */
typedef enum _scg_firc_trim_div
{
    kSCG_FircTrimDivBy1,    /*!< Divided by 1.    */
    kSCG_FircTrimDivBy128,  /*!< Divided by 128.  */
    kSCG_FircTrimDivBy256,  /*!< Divided by 256.  */
    kSCG_FircTrimDivBy512,  /*!< Divided by 512.  */
    kSCG_FircTrimDivBy1024, /*!< Divided by 1024. */
    kSCG_FircTrimDivBy2048  /*!< Divided by 2048. */
} scg_firc_trim_div_t;

/*!
 * @brief SCG fast IRC trim source.
 */
typedef enum _scg_firc_trim_src
{
    kSCG_FircTrimSrcSysOsc = 2U, /*!< System OSC.                 */
    kSCG_FircTrimSrcRtcOsc = 3U, /*!< RTC OSC (32.768 kHz).       */
} scg_firc_trim_src_t;

/*!
 * @brief SCG fast IRC clock trim configuration.
 */
typedef struct _scg_firc_trim_config
{
    scg_firc_trim_mode_t trimMode; /*!< FIRC trim mode.                       */
    scg_firc_trim_src_t trimSrc;   /*!< Trim source.                          */
    scg_firc_trim_div_t trimDiv;   /*!< Trim predivided value for the system OSC.  */

    uint8_t trimCoar; /*!< Trim coarse value; Irrelevant if trimMode is kSCG_FircTrimUpdate. */
    uint8_t trimFine; /*!< Trim fine value; Irrelevant if trimMode is kSCG_FircTrimUpdate. */
} scg_firc_trim_config_t;

/*!
 * @brief SCG fast IRC clock frequency range.
 */
typedef enum _scg_firc_range
{
    kSCG_FircRange48M, /*!< Fast IRC is trimmed to 48 MHz.  */
    kSCG_FircRange52M, /*!< Fast IRC is trimmed to 52 MHz.  */
    kSCG_FircRange56M, /*!< Fast IRC is trimmed to 56 MHz.  */
    kSCG_FircRange60M  /*!< Fast IRC is trimmed to 60 MHz.  */
} scg_firc_range_t;

/*! @brief FIRC enable mode. */
enum _scg_firc_enable_mode
{
    kSCG_FircEnable = SCG_FIRCCSR_FIRCEN_MASK,              /*!< Enable FIRC clock.             */
    kSCG_FircEnableInStop = SCG_FIRCCSR_FIRCSTEN_MASK,      /*!< Enable FIRC in stop mode.      */
    kSCG_FircEnableInLowPower = SCG_FIRCCSR_FIRCLPEN_MASK,  /*!< Enable FIRC in low power mode. */
    kSCG_FircDisableRegulator = SCG_FIRCCSR_FIRCREGOFF_MASK /*!< Disable regulator.             */
};

/*!
 * @brief SCG fast IRC clock configuration.
 */
typedef struct _scg_firc_config_t
{
    uint32_t enableMode; /*!< Enable mode, OR'ed value of _scg_firc_enable_mode. */

    scg_async_clk_div_t div1; /*!< FIRCDIV1 value.                          */
    scg_async_clk_div_t div2; /*!< FIRCDIV2 value.                          */
    scg_async_clk_div_t div3; /*!< FIRCDIV3 value.                          */

    scg_firc_range_t range; /*!< Fast IRC frequency range.                 */

    const scg_firc_trim_config_t *trimConfig; /*!< Pointer to the FIRC trim configuration; set NULL to disable trim. */
} scg_firc_config_t;

/*! @brief LPFLL enable mode. */
enum _scg_lpfll_enable_mode
{
    kSCG_LpFllEnable = SCG_LPFLLCSR_LPFLLEN_MASK, /*!< Enable LPFLL clock.             */
};

/*!
 * @brief SCG LPFLL clock frequency range.
 */
typedef enum _scg_lpfll_range
{
    kSCG_LpFllRange48M, /*!< LPFLL is trimmed to 48MHz.  */
    kSCG_LpFllRange72M, /*!< LPFLL is trimmed to 72MHz.  */
    kSCG_LpFllRange96M, /*!< LPFLL is trimmed to 96MHz.  */
    kSCG_LpFllRange120M /*!< LPFLL is trimmed to 120MHz. */
} scg_lpfll_range_t;

/*!
 * @brief SCG LPFLL trim mode.
 */
typedef enum _scg_lpfll_trim_mode
{
    kSCG_LpFllTrimNonUpdate = SCG_LPFLLCSR_LPFLLTREN_MASK,
    /*!< LPFLL trim is enabled but the trim value update is not enabled. In this mode, the
     trim value is fixed to the initialized value, which is defined by the @ref trimValue
     in the structure @ref scg_lpfll_trim_config_t.*/

    kSCG_LpFllTrimUpdate = SCG_LPFLLCSR_LPFLLTREN_MASK | SCG_LPFLLCSR_LPFLLTRUP_MASK
    /*!< FIRC trim is enabled and trim value update is enabled. In this mode, the trim
     value is automatically updated. */
} scg_lpfll_trim_mode_t;

/*!
 * @brief SCG LPFLL trim source.
 */
typedef enum _scg_lpfll_trim_src
{
    kSCG_LpFllTrimSrcSirc = 0U,   /*!< SIRC.                 */
    kSCG_LpFllTrimSrcFirc = 1U,   /*!< FIRC.                 */
    kSCG_LpFllTrimSrcSysOsc = 2U, /*!< System OSC.           */
    kSCG_LpFllTrimSrcRtcOsc = 3U, /*!< RTC OSC (32.768 kHz). */
} scg_lpfll_trim_src_t;

/*!
 * @brief SCG LPFLL lock mode.
 */
typedef enum _scg_lpfll_lock_mode
{
    kSCG_LpFllLock1Lsb = 0U, /*!< Lock with 1 LSB. */
    kSCG_LpFllLock2Lsb = 1U  /*!< Lock with 2 LSB. */
} scg_lpfll_lock_mode_t;

/*!
 * @brief SCG LPFLL clock trim configuration.
 */
typedef struct _scg_lpfll_trim_config
{
    scg_lpfll_trim_mode_t trimMode; /*!< Trim mode.            */
    scg_lpfll_lock_mode_t lockMode; /*!< Lock mode; Irrelevant if the trimMode is kSCG_LpFllTrimNonUpdate. */

    scg_lpfll_trim_src_t trimSrc; /*!< Trim source.          */
    uint8_t trimDiv;              /*!< Trim predivideds value, which can be 0 ~ 31.
                                    [ Trim source frequency / (trimDiv + 1) ] must be 2 MHz or 32768 Hz. */

    uint8_t trimValue; /*!< Trim value; Irrelevant if trimMode is the kSCG_LpFllTrimUpdate. */
} scg_lpfll_trim_config_t;

/*!
 * @brief SCG low power FLL configuration.
 */
typedef struct _scg_lpfll_config
{
    uint8_t enableMode; /*!< Enable mode, OR'ed value of _scg_lpfll_enable_mode */

    scg_async_clk_div_t div1; /*!< LPFLLDIV1 value.                          */
    scg_async_clk_div_t div2; /*!< LPFLLDIV2 value.                          */
    scg_async_clk_div_t div3; /*!< LPFLLDIV3 value.                          */

    scg_lpfll_range_t range; /*!< LPFLL frequency range.                     */

    const scg_lpfll_trim_config_t *trimConfig; /*!< Trim configuration; set NULL to disable trim. */
} scg_lpfll_config_t;

/*!
 * @brief SCG RTC OSC monitor mode.
 */
typedef enum _scg_rosc_monitor_mode
{
    kSCG_rtcOscMonitorDisable = 0U,                  /*!< Monitor disable.                          */
    kSCG_rtcOscMonitorInt = SCG_ROSCCSR_ROSCCM_MASK, /*!< Interrupt when the RTC OSC error is detected. */
    kSCG_rtcOscMonitorReset =
        SCG_ROSCCSR_ROSCCM_MASK | SCG_ROSCCSR_ROSCCMRE_MASK /*!< Reset when the RTC OSC error is detected.     */
} scg_rosc_monitor_mode_t;

/*!
 * @brief SCG RTC OSC configuration.
 */
typedef struct _scg_rosc_config
{
    scg_rosc_monitor_mode_t monitorMode; /*!< Clock monitor mode selected.     */
} scg_rosc_config_t;

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
    assert((*(volatile uint32_t *)name) & PCC_CLKCFG_PR_MASK);

    (*(volatile uint32_t *)name) |= PCC_CLKCFG_CGC_MASK;
}

/*!
 * @brief Disable the clock for specific IP.
 *
 * @param name  Which clock to disable, see \ref clock_ip_name_t.
 */
static inline void CLOCK_DisableClock(clock_ip_name_t name)
{
    assert((*(volatile uint32_t *)name) & PCC_CLKCFG_PR_MASK);

    (*(volatile uint32_t *)name) &= ~PCC_CLKCFG_CGC_MASK;
}

/*!
 * @brief Check whether the clock is already enabled and configured by
 * any other core.
 *
 * @param name Which peripheral to check, see \ref clock_ip_name_t.
 * @return True if clock is already enabled, otherwise false.
 */
static inline bool CLOCK_IsEnabledByOtherCore(clock_ip_name_t name)
{
    assert((*(volatile uint32_t *)name) & PCC_CLKCFG_PR_MASK);

    return ((*(volatile uint32_t *)name) & PCC_CLKCFG_INUSE_MASK) ? true : false;
}

/*!
 * @brief Set the clock source for specific IP module.
 *
 * Set the clock source for specific IP, not all modules need to set the
 * clock source, should only use this function for the modules need source
 * setting.
 *
 * @param name Which peripheral to check, see \ref clock_ip_name_t.
 * @param src Clock source to set.
 */
static inline void CLOCK_SetIpSrc(clock_ip_name_t name, clock_ip_src_t src)
{
    uint32_t reg = (*(volatile uint32_t *)name);

    assert(reg & PCC_CLKCFG_PR_MASK);
    assert(!(reg & PCC_CLKCFG_INUSE_MASK)); /* Should not change if clock has been enabled by other core. */

    reg = (reg & ~PCC_CLKCFG_PCS_MASK) | PCC_CLKCFG_PCS(src);

    /*
     * If clock is already enabled, first disable it, then set the clock
     * source and re-enable it.
     */
    (*(volatile uint32_t *)name) = reg & ~PCC_CLKCFG_CGC_MASK;
    (*(volatile uint32_t *)name) = reg;
}

/*!
 * @brief Set the clock source and divider for specific IP module.
 *
 * Set the clock source and divider for specific IP, not all modules need to
 * set the clock source and divider, should only use this function for the
 * modules need source and divider setting.
 *
 * Divider output clock = Divider input clock x [(fracValue+1)/(divValue+1)]).
 *
 * @param name Which peripheral to check, see \ref clock_ip_name_t.
 * @param src Clock source to set.
 * @param divValue  The divider value.
 * @param fracValue The fraction multiply value.
 */
static inline void CLOCK_SetIpSrcDiv(clock_ip_name_t name, clock_ip_src_t src, uint8_t divValue, uint8_t fracValue)
{
    uint32_t reg = (*(volatile uint32_t *)name);

    assert(reg & PCC_CLKCFG_PR_MASK);
    assert(!(reg & PCC_CLKCFG_INUSE_MASK)); /* Should not change if clock has been enabled by other core. */

    reg = (reg & ~(PCC_CLKCFG_PCS_MASK | PCC_CLKCFG_FRAC_MASK | PCC_CLKCFG_PCD_MASK)) | PCC_CLKCFG_PCS(src) |
          PCC_CLKCFG_PCD(divValue) | PCC_CLKCFG_FRAC(fracValue);

    /*
     * If clock is already enabled, first disable it, then set the clock
     * source and re-enable it.
     */
    (*(volatile uint32_t *)name) = reg & ~PCC_CLKCFG_CGC_MASK;
    (*(volatile uint32_t *)name) = reg;
}

/*!
 * @brief Gets the clock frequency for a specific clock name.
 *
 * This function checks the current clock configurations and then calculates
 * the clock frequency for a specific clock name defined in clock_name_t.
 *
 * @param clockName Clock names defined in clock_name_t
 * @return Clock frequency value in hertz
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
 * @brief Get the OSC 32K clock frequency (OSC32KCLK).
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetOsc32kClkFreq(void);

/*!
 * @brief Get the external clock frequency (EXTCLK).
 *
 * @return Clock frequency in Hz.
 */
uint32_t CLOCK_GetExtClkFreq(void);

/*!
 * @brief Get the LPO clock frequency.
 *
 * @return Clock frequency in Hz.
 */
static inline uint32_t CLOCK_GetLpoClkFreq(void)
{
    return LPO_CLK_FREQ; /* 1k Hz. */
}

/*!
 * @brief Gets the functional clock frequency for a specific IP module.
 *
 * This function gets the IP module's functional clock frequency based on PCC
 * registers. It is only used for the IP modules which could select clock source
 * by PCC[PCS].
 *
 * @param name Which peripheral to get, see \ref clock_ip_name_t.
 * @return Clock frequency value in Hz
 */
uint32_t CLOCK_GetIpFreq(clock_ip_name_t name);

/*!
* @brief Enable the RTC Oscillator.
*
* This function enables the Oscillator for RTC external crystal.
*
* @param enable Enable the Oscillator or not.
*/
static inline void CLOCK_EnableRtcOsc(bool enable)
{
    if (enable)
    {
        RTC->CR |= RTC_CR_OSCE_MASK;
    }
    else
    {
        RTC->CR &= ~RTC_CR_OSCE_MASK;
    }
}

/*!
* @brief Enable the RSIM Run Regulator Request.
*
* When this function is enabled, the RSIM will request the SPM Run Regulator to be turned on and the
* RSIM will then stop requesting Stop after the Run Regulator Acknowledge signal is received from
* the SPM module.
* The RF OSC would be only available while this function is enabled.
*
* @param enable Enable the function or not.
*/
static inline void CLOCK_EnableRSIMRunRequest(bool enable)
{
    if (enable)
    {
        RSIM->POWER |= RSIM_POWER_RSIM_RUN_REQUEST_MASK;
    }
    else
    {
        RSIM->POWER &= ~RSIM_POWER_RSIM_RUN_REQUEST_MASK;
    }
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
    CLOCK_DisableClock(kCLOCK_Usb0);
}

/*!
 * @name MCU System Clock.
 * @{
 */

/*!
 * @brief Gets the SCG system clock frequency.
 *
 * This function gets the SCG system clock frequency. These clocks are used for
 * core, platform, external, and bus clock domains.
 *
 * @param type     Which type of clock to get, core clock or slow clock.
 * @return  Clock frequency.
 */
uint32_t CLOCK_GetSysClkFreq(scg_sys_clk_t type);

/*!
 * @brief Sets the system clock configuration for VLPR mode.
 *
 * This function sets the system clock configuration for VLPR mode.
 *
 * @param config Pointer to the configuration.
 */
static inline void CLOCK_SetVlprModeSysClkConfig(const scg_sys_clk_config_t *config)
{
    assert(config);

    SCG->VCCR = *(const uint32_t *)config;
}

/*!
 * @brief Sets the system clock configuration for RUN mode.
 *
 * This function sets the system clock configuration for RUN mode.
 *
 * @param config Pointer to the configuration.
 */
static inline void CLOCK_SetRunModeSysClkConfig(const scg_sys_clk_config_t *config)
{
    assert(config);

    SCG->RCCR = *(const uint32_t *)config;
}

/*!
 * @brief Sets the system clock configuration for HSRUN mode.
 *
 * This function sets the system clock configuration for HSRUN mode.
 *
 * @param config Pointer to the configuration.
 */
static inline void CLOCK_SetHsrunModeSysClkConfig(const scg_sys_clk_config_t *config)
{
    assert(config);

    SCG->HCCR = *(const uint32_t *)config;
}

/*!
 * @brief Gets the system clock configuration in the current power mode.
 *
 * This function gets the system configuration in the current power mode.
 *
 * @param config Pointer to the configuration.
 */
static inline void CLOCK_GetCurSysClkConfig(scg_sys_clk_config_t *config)
{
    assert(config);

    *(uint32_t *)config = SCG->CSR;
}

/*!
 * @brief Sets the clock out selection.
 *
 * This function sets the clock out selection (CLKOUTSEL).
 *
 * @param setting The selection to set.
 * @return  The current clock out selection.
 */
static inline void CLOCK_SetClkOutSel(clock_clkout_src_t setting)
{
    SCG->CLKOUTCNFG = SCG_CLKOUTCNFG_CLKOUTSEL(setting);
}
/* @} */

/*!
 * @name SCG System OSC Clock.
 * @{
 */

/*!
 * @brief Initializes the SCG system OSC.
 *
 * This function enables the SCG system OSC clock according to the
 * configuration.
 *
 * @param config   Pointer to the configuration structure.
 * @retval kStatus_Success System OSC is initialized.
 * @retval kStatus_SCG_Busy System OSC has been enabled and is used by the system clock.
 * @retval kStatus_ReadOnly System OSC control register is locked.
 *
 * @note This function can't detect whether the system OSC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitSysOsc(const scg_sosc_config_t *config);

/*!
 * @brief De-initializes the SCG system OSC.
 *
 * This function disables the SCG system OSC clock.
 *
 * @retval kStatus_Success System OSC is deinitialized.
 * @retval kStatus_SCG_Busy System OSC is used by the system clock.
 * @retval kStatus_ReadOnly System OSC control register is locked.
 *
 * @note This function can't detect whether the system OSC is used by an IP.
 */
status_t CLOCK_DeinitSysOsc(void);

/*!
 * @brief Set the asynchronous clock divider.
 *
 * @param asyncClk Which asynchronous clock to configure.
 * @param divider The divider value to set.
 *
 * @note There might be glitch when changing the asynchronous divider, so make sure
 * the asynchronous clock is not used while changing divider.
 */
static inline void CLOCK_SetSysOscAsyncClkDiv(scg_async_clk_t asyncClk, scg_async_clk_div_t divider)
{
    uint32_t reg = SCG->SOSCDIV;

    switch (asyncClk)
    {
        case kSCG_AsyncDiv3Clk:
            reg = (reg & ~SCG_SOSCDIV_SOSCDIV3_MASK) | SCG_SOSCDIV_SOSCDIV3(divider);
            break;
        case kSCG_AsyncDiv2Clk:
            reg = (reg & ~SCG_SOSCDIV_SOSCDIV2_MASK) | SCG_SOSCDIV_SOSCDIV2(divider);
            break;
        default:
            reg = (reg & ~SCG_SOSCDIV_SOSCDIV1_MASK) | SCG_SOSCDIV_SOSCDIV1(divider);
            break;
    }

    SCG->SOSCDIV = reg;
}

/*!
 * @brief Gets the SCG system OSC clock frequency (SYSOSC).
 *
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysOscFreq(void);

/*!
 * @brief Gets the SCG asynchronous clock frequency from the system OSC.
 *
 * @param type     The asynchronous clock type.
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysOscAsyncFreq(scg_async_clk_t type);

/*!
 * @brief Checks whether the system OSC clock error occurs.
 *
 * @return  True if the error occurs, false if not.
 */
static inline bool CLOCK_IsSysOscErr(void)
{
    return (bool)(SCG->SOSCCSR & SCG_SOSCCSR_SOSCERR_MASK);
}

/*!
 * @brief Clears the system OSC clock error.
 */
static inline void CLOCK_ClearSysOscErr(void)
{
    SCG->SOSCCSR |= SCG_SOSCCSR_SOSCERR_MASK;
}

/*!
 * @brief Sets the system OSC monitor mode.
 *
 * This function sets the system OSC monitor mode. The mode can be disabled,
 * it can generate an interrupt when the error is disabled, or reset when the error is detected.
 *
 * @param mode Monitor mode to set.
 */
static inline void CLOCK_SetSysOscMonitorMode(scg_sosc_monitor_mode_t mode)
{
    uint32_t reg = SCG->SOSCCSR;

    reg &= ~(SCG_SOSCCSR_SOSCCM_MASK | SCG_SOSCCSR_SOSCCMRE_MASK);

    reg |= (uint32_t)mode;

    SCG->SOSCCSR = reg;
}

/*!
 * @brief Checks whether the system OSC clock is valid.
 *
 * @return  True if clock is valid, false if not.
 */
static inline bool CLOCK_IsSysOscValid(void)
{
    return (bool)(SCG->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK);
}
/* @} */

/*!
 * @name SCG Slow IRC Clock.
 * @{
 */

/*!
 * @brief Initializes the SCG slow IRC clock.
 *
 * This function enables the SCG slow IRC clock according to the
 * configuration.
 *
 * @param config   Pointer to the configuration structure.
 * @retval kStatus_Success SIRC is initialized.
 * @retval kStatus_SCG_Busy SIRC has been enabled and is used by system clock.
 * @retval kStatus_ReadOnly SIRC control register is locked.
 *
 * @note This function can't detect whether the system OSC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitSirc(const scg_sirc_config_t *config);

/*!
 * @brief De-initializes the SCG slow IRC.
 *
 * This function disables the SCG slow IRC.
 *
 * @retval kStatus_Success SIRC is deinitialized.
 * @retval kStatus_SCG_Busy SIRC is used by system clock.
 * @retval kStatus_ReadOnly SIRC control register is locked.
 *
 * @note This function can't detect whether the SIRC is used by an IP.
 */
status_t CLOCK_DeinitSirc(void);

/*!
 * @brief Set the asynchronous clock divider.
 *
 * @param asyncClk Which asynchronous clock to configure.
 * @param divider The divider value to set.
 *
 * @note There might be glitch when changing the asynchronous divider, so make sure
 * the asynchronous clock is not used while changing divider.
 */
static inline void CLOCK_SetSircAsyncClkDiv(scg_async_clk_t asyncClk, scg_async_clk_div_t divider)
{
    uint32_t reg = SCG->SIRCDIV;

    switch (asyncClk)
    {
        case kSCG_AsyncDiv3Clk:
            reg = (reg & ~SCG_SIRCDIV_SIRCDIV3_MASK) | SCG_SIRCDIV_SIRCDIV3(divider);
            break;
        case kSCG_AsyncDiv2Clk:
            reg = (reg & ~SCG_SIRCDIV_SIRCDIV2_MASK) | SCG_SIRCDIV_SIRCDIV2(divider);
            break;
        default:
            reg = (reg & ~SCG_SIRCDIV_SIRCDIV1_MASK) | SCG_SIRCDIV_SIRCDIV1(divider);
            break;
    }

    SCG->SIRCDIV = reg;
}

/*!
 * @brief Gets the SCG SIRC clock frequency.
 *
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSircFreq(void);

/*!
 * @brief Gets the SCG asynchronous clock frequency from the SIRC.
 *
 * @param type     The asynchronous clock type.
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSircAsyncFreq(scg_async_clk_t type);

/*!
 * @brief Checks whether the SIRC clock is valid.
 *
 * @return  True if clock is valid, false if not.
 */
static inline bool CLOCK_IsSircValid(void)
{
    return (bool)(SCG->SIRCCSR & SCG_SIRCCSR_SIRCVLD_MASK);
}
/* @} */

/*!
 * @name SCG Fast IRC Clock.
 * @{
 */

/*!
 * @brief Initializes the SCG fast IRC clock.
 *
 * This function enables the SCG fast IRC clock according to the configuration.
 *
 * @param config   Pointer to the configuration structure.
 * @retval kStatus_Success FIRC is initialized.
 * @retval kStatus_SCG_Busy FIRC has been enabled and is used by the system clock.
 * @retval kStatus_ReadOnly FIRC control register is locked.
 *
 * @note This function can't detect whether the FIRC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitFirc(const scg_firc_config_t *config);

/*!
 * @brief De-initializes the SCG fast IRC.
 *
 * This function disables the SCG fast IRC.
 *
 * @retval kStatus_Success FIRC is deinitialized.
 * @retval kStatus_SCG_Busy FIRC is used by the system clock.
 * @retval kStatus_ReadOnly FIRC control register is locked.
 *
 * @note This function can't detect whether the FIRC is used by an IP.
 */
status_t CLOCK_DeinitFirc(void);

/*!
 * @brief Set the asynchronous clock divider.
 *
 * @param asyncClk Which asynchronous clock to configure.
 * @param divider The divider value to set.
 *
 * @note There might be glitch when changing the asynchronous divider, so make sure
 * the asynchronous clock is not used while changing divider.
 */
static inline void CLOCK_SetFircAsyncClkDiv(scg_async_clk_t asyncClk, scg_async_clk_div_t divider)
{
    uint32_t reg = SCG->FIRCDIV;

    switch (asyncClk)
    {
        case kSCG_AsyncDiv3Clk:
            reg = (reg & ~SCG_FIRCDIV_FIRCDIV3_MASK) | SCG_FIRCDIV_FIRCDIV3(divider);
            break;
        case kSCG_AsyncDiv2Clk:
            reg = (reg & ~SCG_FIRCDIV_FIRCDIV2_MASK) | SCG_FIRCDIV_FIRCDIV2(divider);
            break;
        default:
            reg = (reg & ~SCG_FIRCDIV_FIRCDIV1_MASK) | SCG_FIRCDIV_FIRCDIV1(divider);
            break;
    }

    SCG->FIRCDIV = reg;
}

/*!
 * @brief Gets the SCG FIRC clock frequency.
 *
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetFircFreq(void);

/*!
 * @brief Gets the SCG asynchronous clock frequency from the FIRC.
 *
 * @param type     The asynchronous clock type.
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetFircAsyncFreq(scg_async_clk_t type);

/*!
 * @brief Checks whether the FIRC clock error occurs.
 *
 * @return  True if the error occurs, false if not.
 */
static inline bool CLOCK_IsFircErr(void)
{
    return (bool)(SCG->FIRCCSR & SCG_FIRCCSR_FIRCERR_MASK);
}

/*!
 * @brief Clears the FIRC clock error.
 */
static inline void CLOCK_ClearFircErr(void)
{
    SCG->FIRCCSR |= SCG_FIRCCSR_FIRCERR_MASK;
}

/*!
 * @brief Checks whether the FIRC clock is valid.
 *
 * @return  True if clock is valid, false if not.
 */
static inline bool CLOCK_IsFircValid(void)
{
    return (bool)(SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK);
}
/* @} */

/*!
 * @brief Gets the SCG RTC OSC clock frequency.
 *
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetRtcOscFreq(void);

/*!
 * @brief Checks whether the RTC OSC clock error occurs.
 *
 * @return  True if error occurs, false if not.
 */
static inline bool CLOCK_IsRtcOscErr(void)
{
    return (bool)(SCG->ROSCCSR & SCG_ROSCCSR_ROSCERR_MASK);
}

/*!
 * @brief Clears the RTC OSC clock error.
 */
static inline void CLOCK_ClearRtcOscErr(void)
{
    SCG->ROSCCSR |= SCG_ROSCCSR_ROSCERR_MASK;
}

/*!
 * @brief Sets the RTC OSC monitor mode.
 *
 * This function sets the RTC OSC monitor mode. The mode can be disabled.
 * It can generate an interrupt when the error is disabled, or reset when the error is detected.
 *
 * @param mode Monitor mode to set.
 */
static inline void CLOCK_SetRtcOscMonitorMode(scg_rosc_monitor_mode_t mode)
{
    uint32_t reg = SCG->ROSCCSR;

    reg &= ~(SCG_ROSCCSR_ROSCCM_MASK | SCG_ROSCCSR_ROSCCMRE_MASK);

    reg |= (uint32_t)mode;

    SCG->ROSCCSR = reg;
}

/*!
 * @brief Checks whether the RTC OSC clock is valid.
 *
 * @return  True if the clock is valid, false if not.
 */
static inline bool CLOCK_IsRtcOscValid(void)
{
    return (bool)(SCG->ROSCCSR & SCG_ROSCCSR_ROSCVLD_MASK);
}
/* @} */

/*!
 * @name SCG Low Power FLL Clock.
 * @{
 */
/*!
 * @brief Initializes the SCG LPFLL clock.
 *
 * This function enables the SCG LPFLL clock according to the configuration.
 *
 * @param config   Pointer to the configuration structure.
 * @retval kStatus_Success LPFLL is initialized.
 * @retval kStatus_SCG_Busy LPFLL has been enabled and is used by the system clock.
 * @retval kStatus_ReadOnly LPFLL control register is locked.
 *
 * @note This function can't detect whether the LPFLL has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitLpFll(const scg_lpfll_config_t *config);

/*!
 * @brief De-initializes the SCG LPFLL.
 *
 * This function disables the SCG LPFLL.
 *
 * @retval kStatus_Success LPFLL is deinitialized.
 * @retval kStatus_SCG_Busy LPFLL is used by the system clock.
 * @retval kStatus_ReadOnly LPFLL control register is locked.
 *
 * @note This function can't detect whether the LPFLL is used by an IP.
 */
status_t CLOCK_DeinitLpFll(void);

/*!
 * @brief Set the asynchronous clock divider.
 *
 * @param asyncClk Which asynchronous clock to configure.
 * @param divider The divider value to set.
 *
 * @note There might be glitch when changing the asynchronous divider, so make sure
 * the asynchronous clock is not used while changing divider.
 */
static inline void CLOCK_SetLpFllAsyncClkDiv(scg_async_clk_t asyncClk, scg_async_clk_div_t divider)
{
    uint32_t reg = SCG->LPFLLDIV;

    switch (asyncClk)
    {
        case kSCG_AsyncDiv2Clk:
            reg = (reg & ~SCG_LPFLLDIV_LPFLLDIV2_MASK) | SCG_LPFLLDIV_LPFLLDIV2(divider);
            break;
        default:
            reg = (reg & ~SCG_LPFLLDIV_LPFLLDIV1_MASK) | SCG_LPFLLDIV_LPFLLDIV1(divider);
            break;
    }

    SCG->LPFLLDIV = reg;
}

/*!
 * @brief Gets the SCG LPFLL clock frequency.
 *
 * @return  Clock frequency in Hz; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetLpFllFreq(void);

/*!
 * @brief Gets the SCG asynchronous clock frequency from the LPFLL.
 *
 * @param type     The asynchronous clock type.
 * @return  Clock frequency in Hz; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetLpFllAsyncFreq(scg_async_clk_t type);

/*!
 * @brief Checks whether the LPFLL clock is valid.
 *
 * @return  True if the clock is valid, false if not.
 */
static inline bool CLOCK_IsLpFllValid(void)
{
    return (bool)(SCG->LPFLLCSR & SCG_LPFLLCSR_LPFLLVLD_MASK);
}
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
 * @brief Sets the XTAL32 frequency based on board settings.
 *
 * @param freq The XTAL32/EXTAL32 input clock frequency in Hz.
 */
static inline void CLOCK_SetXtal32Freq(uint32_t freq)
{
    g_xtal32Freq = freq;
}

/* @} */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @} */

#endif /* _FSL_CLOCK_H_ */
