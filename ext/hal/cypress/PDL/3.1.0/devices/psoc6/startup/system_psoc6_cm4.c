/***************************************************************************//**
* \file system_psoc6_cm4.c
* \version 2.20
*
* The device system-source file.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "cyip_cpuss.h"
#include "cyip_cpuss_v2.h"
#include "system_psoc6.h"
#include "cy_device_headers.h"
#include "cy_device.h"

#if !defined(CY_IPC_DEFAULT_CFG_DISABLE)
    #include "cy_ipc_drv.h"
    
    #if defined(CY_DEVICE_PSOC6ABLE2)
        #include "cy_flash.h"
    #endif /* defined(CY_DEVICE_PSOC6ABLE2) */
#endif /* !defined(CY_IPC_DEFAULT_CFG_DISABLE) */


/*******************************************************************************
* SystemCoreClockUpdate()
*******************************************************************************/

/** Default HFClk frequency in Hz */
#define CY_CLK_HFCLK0_FREQ_HZ_DEFAULT       (8000000UL)

/** Default PeriClk frequency in Hz */
#define CY_CLK_PERICLK_FREQ_HZ_DEFAULT      (4000000UL)

/** Default SlowClk system core frequency in Hz */
#define CY_CLK_SYSTEM_FREQ_HZ_DEFAULT       (8000000UL)

/** IMO frequency in Hz */
#define CY_CLK_IMO_FREQ_HZ                  (8000000UL)

/** HVILO frequency in Hz */
#define CY_CLK_HVILO_FREQ_HZ                (32000UL)

#if (SRSS_PILO_PRESENT == 1U) || defined(CY_DOXYGEN)
    /** PILO frequency in Hz */
    #define CY_CLK_PILO_FREQ_HZ             (32768UL)
#endif /* (SRSS_PILO_PRESENT == 1U) || defined(CY_DOXYGEN) */

/** WCO frequency in Hz */
#define CY_CLK_WCO_FREQ_HZ                  (32768UL)

#if (SRSS_ALTLF_PRESENT == 1U) || defined(CY_DOXYGEN)
    /** ALTLF frequency in Hz */
    #define CY_CLK_ALTLF_FREQ_HZ            (32768UL)
#endif /* (SRSS_ALTLF_PRESENT == 1U) || defined(CY_DOXYGEN) */


/*
* Holds the FastClk system core clock, which is the system clock frequency 
* supplied to the SysTick timer and the processor core clock.
* This variable implements CMSIS Core global variable.
* Refer to the [CMSIS documentation]
* (http://www.keil.com/pack/doc/CMSIS/Core/html/group__system__init__gr.html "System and Clock Configuration") 
* for more details.
* This variable can be used by debuggers to query the frequency 
* of the debug timer or to configure the trace clock speed.
*
* \attention Compilers must be configured to avoid removing this variable in case
* the application program is not using it. Debugging systems require the variable
* to be physically present in memory so that it can be examined to configure the debugger. */
uint32_t SystemCoreClock = CY_CLK_SYSTEM_FREQ_HZ_DEFAULT;

/** Holds the HFClk0 clock frequency. Updated by \ref SystemCoreClockUpdate(). */
uint32_t cy_Hfclk0FreqHz  = CY_CLK_HFCLK0_FREQ_HZ_DEFAULT;

/** Holds the PeriClk clock frequency. Updated by \ref SystemCoreClockUpdate(). */
uint32_t cy_PeriClkFreqHz = CY_CLK_PERICLK_FREQ_HZ_DEFAULT;

#if (defined (CY_IP_MXBLESS) && (CY_IP_MXBLESS == 1UL)) || defined (CY_DOXYGEN)
    /** Holds the Alternate high frequency clock in Hz. Updated by \ref SystemCoreClockUpdate(). */
    uint32_t cy_BleEcoClockFreqHz = CY_CLK_ALTHF_FREQ_HZ;
#endif /* (defined (CY_IP_MXBLESS) && (CY_IP_MXBLESS == 1UL)) || defined (CY_DOXYGEN) */


/* SCB->CPACR */
#define SCB_CPACR_CP10_CP11_ENABLE      (0xFUL << 20u)


/*******************************************************************************
* SystemInit()
*******************************************************************************/
#if (__CM0P_PRESENT == 0)
    /* CLK_FLL_CONFIG default values */
    #define CY_FB_CLK_FLL_CONFIG_VALUE      (0x01000000u)
    #define CY_FB_CLK_FLL_CONFIG2_VALUE     (0x00020001u)
    #define CY_FB_CLK_FLL_CONFIG3_VALUE     (0x00002800u)
    #define CY_FB_CLK_FLL_CONFIG4_VALUE     (0x000000FFu)
#endif /* (__CM0P_PRESENT == 0) */

/* Platform and peripheral block configuration */
#if defined (CY_DEVICE_PSOC6ABLE2)
#define CY_DEVICE_CFG                   (&cy_deviceIpBlockCfg1)
#else
#define CY_DEVICE_CFG                   (&cy_deviceIpBlockCfg2)
#endif /* CY_DEVICE_PSOC6ABLE2 */


/*******************************************************************************
* SystemCoreClockUpdate (void)
*******************************************************************************/
/* Do not use these definitions directly in your application */
#define CY_DELAY_MS_OVERFLOW_THRESHOLD  (0x8000u)
#define CY_DELAY_1K_THRESHOLD           (1000u)
#define CY_DELAY_1K_MINUS_1_THRESHOLD   (CY_DELAY_1K_THRESHOLD - 1u)
#define CY_DELAY_1M_THRESHOLD           (1000000u)
#define CY_DELAY_1M_MINUS_1_THRESHOLD   (CY_DELAY_1M_THRESHOLD - 1u)
uint32_t cy_delayFreqHz   = CY_CLK_SYSTEM_FREQ_HZ_DEFAULT;

uint32_t cy_delayFreqKhz  = (CY_CLK_SYSTEM_FREQ_HZ_DEFAULT + CY_DELAY_1K_MINUS_1_THRESHOLD) /
                            CY_DELAY_1K_THRESHOLD;

uint8_t cy_delayFreqMhz  = (uint8_t)((CY_CLK_SYSTEM_FREQ_HZ_DEFAULT + CY_DELAY_1M_MINUS_1_THRESHOLD) /
                            CY_DELAY_1M_THRESHOLD);

uint32_t cy_delay32kMs    = CY_DELAY_MS_OVERFLOW_THRESHOLD *
                            ((CY_CLK_SYSTEM_FREQ_HZ_DEFAULT + CY_DELAY_1K_MINUS_1_THRESHOLD) / CY_DELAY_1K_THRESHOLD);

#define CY_ROOT_PATH_SRC_IMO                (0UL)
#define CY_ROOT_PATH_SRC_EXT                (1UL)
#if (SRSS_ECO_PRESENT == 1U)
    #define CY_ROOT_PATH_SRC_ECO            (2UL)
#endif /* (SRSS_ECO_PRESENT == 1U) */
#if (SRSS_ALTHF_PRESENT == 1U)
    #define CY_ROOT_PATH_SRC_ALTHF          (3UL)
#endif /* (SRSS_ALTHF_PRESENT == 1U) */
#define CY_ROOT_PATH_SRC_DSI_MUX            (4UL)
#define CY_ROOT_PATH_SRC_DSI_MUX_HVILO      (16UL)
#define CY_ROOT_PATH_SRC_DSI_MUX_WCO        (17UL)
#if (SRSS_ALTLF_PRESENT == 1U)
    #define CY_ROOT_PATH_SRC_DSI_MUX_ALTLF  (18UL)
#endif /* (SRSS_ALTLF_PRESENT == 1U) */
#if (SRSS_PILO_PRESENT == 1U)
    #define CY_ROOT_PATH_SRC_DSI_MUX_PILO   (19UL)
#endif /* (SRSS_PILO_PRESENT == 1U) */


/*******************************************************************************
* Function Name: SystemInit
****************************************************************************//**
* \cond
* Initializes the system:
* - Restores FLL registers to the default state for single core devices.
* - Calls the Cy_SystemInit() function, if compiled from PSoC Creator.
* - Calls \ref SystemCoreClockUpdate().
* \endcond
*******************************************************************************/
void SystemInit(void)
{
    /* Set the platform and peripheral block configuration */
    cy_device = CY_DEVICE_CFG;
    
#if (__CM0P_PRESENT == 0)
    /* Restore FLL registers to the default state as they are not restored by the ROM code */
    uint32_t copy = SRSS->CLK_FLL_CONFIG;
    copy &= ~SRSS_CLK_FLL_CONFIG_FLL_ENABLE_Msk;
    SRSS->CLK_FLL_CONFIG = copy;

    copy = SRSS->CLK_ROOT_SELECT[0u];
    copy &= ~SRSS_CLK_ROOT_SELECT_ROOT_DIV_Msk; /* Set ROOT_DIV = 0*/
    SRSS->CLK_ROOT_SELECT[0u] = copy;

    SRSS->CLK_FLL_CONFIG  = CY_FB_CLK_FLL_CONFIG_VALUE;
    SRSS->CLK_FLL_CONFIG2 = CY_FB_CLK_FLL_CONFIG2_VALUE;
    SRSS->CLK_FLL_CONFIG3 = CY_FB_CLK_FLL_CONFIG3_VALUE;
    SRSS->CLK_FLL_CONFIG4 = CY_FB_CLK_FLL_CONFIG4_VALUE;
#endif /* (__CM0P_PRESENT == 0) */

    Cy_SystemInit();
    SystemCoreClockUpdate();

#if !defined(CY_IPC_DEFAULT_CFG_DISABLE)
    /* Allocate and initialize semaphores for the system operations. */
    Cy_IPC_SystemSemaInit();
    Cy_IPC_SystemPipeInit();
    
    #if defined(CY_DEVICE_PSOC6ABLE2)
        Cy_Flash_Init();
    #endif /* defined(CY_DEVICE_PSOC6ABLE2) */
#endif /* !defined(CY_IPC_DEFAULT_CFG_DISABLE) */
}


/*******************************************************************************
* Function Name: Cy_SystemInit
****************************************************************************//**
*
* The function is called during device startup. Once project compiled as part of
* the PSoC Creator project, the Cy_SystemInit() function is generated by the
* PSoC Creator.
*
* The function generated by PSoC Creator performs all of the necessary device
* configuration based on the design settings.  This includes settings from the
* Design Wide Resources (DWR) such as Clocks and Pins as well as any component
* configuration that is necessary.
*
*******************************************************************************/
__WEAK void Cy_SystemInit(void)
{
     /* Empty weak function. The actual implementation to be in the PSoC Creator
      * generated strong function.
     */
}


/*******************************************************************************
* Function Name: SystemCoreClockUpdate
****************************************************************************//**
*
* Gets core clock frequency and updates \ref SystemCoreClock, \ref
* cy_Hfclk0FreqHz, and \ref cy_PeriClkFreqHz.
*
* Updates global variables used by the \ref Cy_SysLib_Delay(), \ref
* Cy_SysLib_DelayUs(), and \ref Cy_SysLib_DelayCycles().
*
*******************************************************************************/
void SystemCoreClockUpdate (void)
{
    uint32_t srcFreqHz;
    uint32_t pathFreqHz;
    uint32_t fastClkDiv;
    uint32_t periClkDiv;
    uint32_t rootPath;
    uint32_t srcClk;

    /* Get root path clock for the high-frequency clock # 0 */
    rootPath = _FLD2VAL(SRSS_CLK_ROOT_SELECT_ROOT_MUX, SRSS->CLK_ROOT_SELECT[0u]);

    /* Get source of the root path clock */
    srcClk = _FLD2VAL(SRSS_CLK_PATH_SELECT_PATH_MUX, SRSS->CLK_PATH_SELECT[rootPath]);

    /* Get frequency of the source */
    switch (srcClk)
    {
    case CY_ROOT_PATH_SRC_IMO:
        srcFreqHz = CY_CLK_IMO_FREQ_HZ;
    break;

    case CY_ROOT_PATH_SRC_EXT:
        srcFreqHz = CY_CLK_EXT_FREQ_HZ;
    break;

    #if (SRSS_ECO_PRESENT == 1U)
        case CY_ROOT_PATH_SRC_ECO:
            srcFreqHz = CY_CLK_ECO_FREQ_HZ;
        break;
    #endif /* (SRSS_ECO_PRESENT == 1U) */

#if defined (CY_IP_MXBLESS) && (CY_IP_MXBLESS == 1UL) && (SRSS_ALTHF_PRESENT == 1U)
    case CY_ROOT_PATH_SRC_ALTHF:
        srcFreqHz = cy_BleEcoClockFreqHz;
    break;
#endif /* defined (CY_IP_MXBLESS) && (CY_IP_MXBLESS == 1UL) && (SRSS_ALTHF_PRESENT == 1U) */

    case CY_ROOT_PATH_SRC_DSI_MUX:
    {
        uint32_t dsi_src;
        dsi_src = _FLD2VAL(SRSS_CLK_DSI_SELECT_DSI_MUX,  SRSS->CLK_DSI_SELECT[rootPath]);
        switch (dsi_src)
        {
        case CY_ROOT_PATH_SRC_DSI_MUX_HVILO:
            srcFreqHz = CY_CLK_HVILO_FREQ_HZ;
        break;

        case CY_ROOT_PATH_SRC_DSI_MUX_WCO:
            srcFreqHz = CY_CLK_WCO_FREQ_HZ;
        break;

        #if (SRSS_ALTLF_PRESENT == 1U)
            case CY_ROOT_PATH_SRC_DSI_MUX_ALTLF:
                srcFreqHz = CY_CLK_ALTLF_FREQ_HZ;
            break;
        #endif /* (SRSS_ALTLF_PRESENT == 1U) */

        #if (SRSS_PILO_PRESENT == 1U)
            case CY_ROOT_PATH_SRC_DSI_MUX_PILO:
                srcFreqHz = CY_CLK_PILO_FREQ_HZ;
            break;
        #endif /* (SRSS_PILO_PRESENT == 1U) */

        default:
            srcFreqHz = CY_CLK_HVILO_FREQ_HZ;
        break;
        }
    }
    break;

    default:
        srcFreqHz = CY_CLK_EXT_FREQ_HZ;
    break;
    }

    if (rootPath == 0UL)
    {
        /* FLL */
        bool fllLocked       = ( 0UL != _FLD2VAL(SRSS_CLK_FLL_STATUS_LOCKED, SRSS->CLK_FLL_STATUS));
        bool fllOutputOutput = ( 3UL == _FLD2VAL(SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, SRSS->CLK_FLL_CONFIG3));
        bool fllOutputAuto   = ((0UL == _FLD2VAL(SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, SRSS->CLK_FLL_CONFIG3)) ||
                                (1UL == _FLD2VAL(SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, SRSS->CLK_FLL_CONFIG3)));
        if ((fllOutputAuto && fllLocked) || fllOutputOutput)
        {
            uint32_t fllMult;
            uint32_t refDiv;
            uint32_t outputDiv;

            fllMult = _FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_MULT, SRSS->CLK_FLL_CONFIG);
            refDiv  = _FLD2VAL(SRSS_CLK_FLL_CONFIG2_FLL_REF_DIV, SRSS->CLK_FLL_CONFIG2);
            outputDiv = _FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_OUTPUT_DIV, SRSS->CLK_FLL_CONFIG) + 1UL;

            pathFreqHz = ((srcFreqHz / refDiv) * fllMult) / outputDiv;
        }
        else
        {
            pathFreqHz = srcFreqHz;
        }
    }
    else if (rootPath == 1UL)
    {
        /* PLL */
        bool pllLocked       = ( 0UL != _FLD2VAL(SRSS_CLK_PLL_STATUS_LOCKED,     SRSS->CLK_PLL_STATUS[0UL]));
        bool pllOutputOutput = ( 3UL == _FLD2VAL(SRSS_CLK_PLL_CONFIG_BYPASS_SEL, SRSS->CLK_PLL_CONFIG[0UL]));
        bool pllOutputAuto   = ((0UL == _FLD2VAL(SRSS_CLK_PLL_CONFIG_BYPASS_SEL, SRSS->CLK_PLL_CONFIG[0UL])) ||
                                (1UL == _FLD2VAL(SRSS_CLK_PLL_CONFIG_BYPASS_SEL, SRSS->CLK_PLL_CONFIG[0UL])));
        if ((pllOutputAuto && pllLocked) || pllOutputOutput)
        {
            uint32_t feedbackDiv;
            uint32_t referenceDiv;
            uint32_t outputDiv;

            feedbackDiv  = _FLD2VAL(SRSS_CLK_PLL_CONFIG_FEEDBACK_DIV,  SRSS->CLK_PLL_CONFIG[0UL]);
            referenceDiv = _FLD2VAL(SRSS_CLK_PLL_CONFIG_REFERENCE_DIV, SRSS->CLK_PLL_CONFIG[0UL]);
            outputDiv    = _FLD2VAL(SRSS_CLK_PLL_CONFIG_OUTPUT_DIV,    SRSS->CLK_PLL_CONFIG[0UL]);

            pathFreqHz = ((srcFreqHz * feedbackDiv) / referenceDiv) / outputDiv;

        }
        else
        {
            pathFreqHz = srcFreqHz;
        }
    }
    else
    {
        /* Direct */
        pathFreqHz = srcFreqHz;
    }

    /* Get frequency after hf_clk pre-divider */
    pathFreqHz = pathFreqHz >> _FLD2VAL(SRSS_CLK_ROOT_SELECT_ROOT_DIV, SRSS->CLK_ROOT_SELECT[0u]);
    cy_Hfclk0FreqHz = pathFreqHz;

    /* Fast Clock Divider */
    fastClkDiv = 1u + _FLD2VAL(CPUSS_CM4_CLOCK_CTL_FAST_INT_DIV, CPUSS->CM4_CLOCK_CTL);

    /* Peripheral Clock Divider */
    periClkDiv = 1u + _FLD2VAL(CPUSS_CM0_CLOCK_CTL_PERI_INT_DIV, CPUSS->CM0_CLOCK_CTL);
    cy_PeriClkFreqHz = pathFreqHz / periClkDiv;

    pathFreqHz = pathFreqHz / fastClkDiv;
    SystemCoreClock = pathFreqHz;

    /* Sets clock frequency for Delay API */
    cy_delayFreqHz = SystemCoreClock;
    cy_delayFreqMhz = (uint8_t)((cy_delayFreqHz + CY_DELAY_1M_MINUS_1_THRESHOLD) / CY_DELAY_1M_THRESHOLD);
    cy_delayFreqKhz = (cy_delayFreqHz + CY_DELAY_1K_MINUS_1_THRESHOLD) / CY_DELAY_1K_THRESHOLD;
    cy_delay32kMs   = CY_DELAY_MS_OVERFLOW_THRESHOLD * cy_delayFreqKhz;
}


/*******************************************************************************
* Function Name: Cy_SystemInitFpuEnable
****************************************************************************//**
*
* Enables the FPU if it is used. The function is called from the startup file.
*
*******************************************************************************/
void Cy_SystemInitFpuEnable(void)
{
    #if defined (__FPU_USED) && (__FPU_USED == 1U)
        uint32_t  interruptState;
        interruptState = Cy_SaveIRQ();
        SCB->CPACR |= SCB_CPACR_CP10_CP11_ENABLE;
        __DSB();
        __ISB();
        Cy_RestoreIRQ(interruptState);
    #endif /* (__FPU_USED) && (__FPU_USED == 1U) */
}


/*******************************************************************************
* Function Name: Cy_MemorySymbols
****************************************************************************//**
*
* The intention of the function is to declare boundaries of the memories for the
* MDK compilers. For the rest of the supported compilers, this is done using
* linker configuration files. The following symbols used by the cymcuelftool.
*
*******************************************************************************/
#if defined (__ARMCC_VERSION)
__asm void Cy_MemorySymbols(void)
{
    /* Flash */
    EXPORT __cy_memory_0_start
    EXPORT __cy_memory_0_length
    EXPORT __cy_memory_0_row_size

    /* Working Flash */
    EXPORT __cy_memory_1_start
    EXPORT __cy_memory_1_length
    EXPORT __cy_memory_1_row_size

    /* Supervisory Flash */
    EXPORT __cy_memory_2_start
    EXPORT __cy_memory_2_length
    EXPORT __cy_memory_2_row_size

    /* XIP */
    EXPORT __cy_memory_3_start
    EXPORT __cy_memory_3_length
    EXPORT __cy_memory_3_row_size

    /* eFuse */
    EXPORT __cy_memory_4_start
    EXPORT __cy_memory_4_length
    EXPORT __cy_memory_4_row_size

    /* Flash */
__cy_memory_0_start     EQU __cpp(CY_FLASH_BASE)
__cy_memory_0_length    EQU __cpp(CY_FLASH_SIZE)
__cy_memory_0_row_size  EQU 0x200

    /* Flash region for EEPROM emulation */
__cy_memory_1_start     EQU __cpp(CY_EM_EEPROM_BASE)
__cy_memory_1_length    EQU __cpp(CY_EM_EEPROM_SIZE)
__cy_memory_1_row_size  EQU 0x200

    /* Supervisory Flash */
__cy_memory_2_start     EQU __cpp(CY_SFLASH_BASE)
__cy_memory_2_length    EQU __cpp(CY_SFLASH_SIZE)
__cy_memory_2_row_size  EQU 0x200

    /* XIP */
__cy_memory_3_start     EQU __cpp(CY_XIP_BASE)
__cy_memory_3_length    EQU __cpp(CY_XIP_SIZE)
__cy_memory_3_row_size  EQU 0x200

    /* eFuse */
__cy_memory_4_start     EQU __cpp(0x90700000)
__cy_memory_4_length    EQU __cpp(0x100000)
__cy_memory_4_row_size  EQU __cpp(1)
}
#endif /* defined (__ARMCC_VERSION) */


/* [] END OF FILE */
