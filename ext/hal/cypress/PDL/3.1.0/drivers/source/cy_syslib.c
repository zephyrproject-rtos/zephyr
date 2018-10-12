/***************************************************************************//**
* \file cy_syslib.c
* \version 2.10.1
*
*  Description:
*   Provides system API implementation for the SysLib driver.
*
********************************************************************************
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_syslib.h"
#include "cy_ipc_drv.h"
#if !defined(NDEBUG)
    #include <string.h>
#endif /* NDEBUG */

/* Flash wait states (ULP mode at 0.9v) */
#define CY_SYSLIB_FLASH_ULP_WS_0_FREQ_MAX    ( 16UL)
#define CY_SYSLIB_FLASH_ULP_WS_1_FREQ_MAX    ( 33UL)
#define CY_SYSLIB_FLASH_ULP_WS_2_FREQ_MAX    ( 50UL)

/* ROM and SRAM wait states for the slow clock domain (LP mode at 1.1v) */
#define CY_SYSLIB_LP_SLOW_WS_0_FREQ_MAX      (100UL)
#define CY_SYSLIB_LP_SLOW_WS_1_FREQ_MAX      (120UL)

/* ROM and SRAM wait states for the slow clock domain (ULP mode at 0.9v) */
#define CY_SYSLIB_ULP_SLOW_WS_0_FREQ_MAX     ( 25UL)
#define CY_SYSLIB_ULP_SLOW_WS_1_FREQ_MAX     ( 50UL)


#if !defined(NDEBUG)
    CY_NOINIT char_t cy_assertFileName[CY_MAX_FILE_NAME_SIZE];
    CY_NOINIT uint32_t cy_assertLine;
#endif /* NDEBUG */

#if (CY_ARM_FAULT_DEBUG == CY_ARM_FAULT_DEBUG_ENABLED)
    CY_NOINIT cy_stc_fault_frame_t cy_faultFrame;
#endif /* (CY_ARM_FAULT_DEBUG == CY_ARM_FAULT_DEBUG_ENABLED) */

#if defined(__ARMCC_VERSION)
    static __ASM void Cy_SysLib_AsmInfiniteLoop(void) { b . };
#endif  /* (__ARMCC_VERSION) */


/*******************************************************************************
* Function Name: Cy_SysLib_Delay
****************************************************************************//**
*
* The function delays by the specified number of milliseconds.
* By default, the number of cycles to delay is calculated based on the
* \ref SystemCoreClock.
*
* \param milliseconds  The number of milliseconds to delay.
*
* \note The function calls \ref Cy_SysLib_DelayCycles() API to generate a delay.
*       If the function parameter (milliseconds) is bigger than 
*       CY_DELAY_MS_OVERFLOW constant, then an additional loop runs to prevent
*       an overflow in parameter passed to \ref Cy_SysLib_DelayCycles() API.
*
*******************************************************************************/
void Cy_SysLib_Delay(uint32_t milliseconds)
{
    while(milliseconds > CY_DELAY_MS_OVERFLOW)
    {
        /* This loop prevents an overflow in value passed to Cy_SysLib_DelayCycles() API.
         * At 100 MHz, (milliseconds * cy_delayFreqKhz) product overflows
         * in case if milliseconds parameter is more than 42 seconds.
         */
        Cy_SysLib_DelayCycles(cy_delay32kMs);
        milliseconds -= CY_DELAY_MS_OVERFLOW;
    }

    Cy_SysLib_DelayCycles(milliseconds * cy_delayFreqKhz);
}


/*******************************************************************************
* Function Name: Cy_SysLib_DelayUs
****************************************************************************//**
*
* The function delays by the specified number of microseconds.
* By default, the number of cycles to delay is calculated based on the
* \ref SystemCoreClock.
*
* \param microseconds  The number of microseconds to delay.
*
* \note If the CPU frequency is a small non-integer number, the actual delay
*       can be up to twice as long as the nominal value. The actual delay
*       cannot be shorter than the nominal one.
*
*******************************************************************************/
void Cy_SysLib_DelayUs(uint16_t microseconds)
{
    Cy_SysLib_DelayCycles((uint32_t) microseconds * cy_delayFreqMhz);
}


/*******************************************************************************
* Function Name: Cy_SysLib_Halt
****************************************************************************//**
*
* This function halts the CPU but only the CPU which calls the function.
* It doesn't affect other CPUs.
*
* \param reason  The value to be used during debugging.
*
* \note The function executes the BKPT instruction for halting CPU and is
*       intended to be used for the debug purpose. A regular use case requires
*       Debugger attachment before the function call.
*       The BKPT instruction causes the CPU to enter the Debug state. Debug
*       tools can use this to investigate the system state, when the
*       instruction at a particular address is reached.
*
* \note Execution of a BKPT instruction without a debugger attached produces
*       a fault. The fault results in the HardFault exception being taken
*       or causes a Lockup state if it occurs in the NMI or HardFault handler.
*       The default HardFault handler make a software reset if the build option
*       is the release mode (NDEBUG). If the build option is the debug mode,
*       the system will stay in the infinite loop of the
*       \ref Cy_SysLib_ProcessingFault() function.
*
*******************************************************************************/
__NO_RETURN void Cy_SysLib_Halt(uint32_t reason)
{
    if(0U != reason)
    {
        /* To remove an unreferenced local variable warning */
    }

    #if defined (__ARMCC_VERSION)
        __breakpoint(0x0);
    #elif defined(__GNUC__)
        __asm("    bkpt    1");
    #elif defined (__ICCARM__)
        __asm("    bkpt    1");
    #else
        #error "An unsupported toolchain"
    #endif  /* (__ARMCC_VERSION) */

    while(1) {}
}


#if !defined(NDEBUG) || defined(CY_DOXYGEN)
/*******************************************************************************
* Macro Name: Cy_SysLib_AssertFailed
****************************************************************************//**
*
* This function stores the ASSERT location of the file name (including path
* to file) and line number in a non-zero init area for debugging. Also it calls
* the \ref Cy_SysLib_Halt() function to halt the processor.
*
* \param file  The file name of the ASSERT location.
* \param line  The line number of the ASSERT location.
*
* \note A stored file name and line number could be accessed by
*       cy_assertFileName and cy_assertLine global variables.
* \note This function has the WEAK option, so the user can redefine
*       the function for a custom processing.
*
*******************************************************************************/
__WEAK void Cy_SysLib_AssertFailed(const char_t * file, uint32_t line)
{
    (void) strncpy(cy_assertFileName, file, CY_MAX_FILE_NAME_SIZE);
    cy_assertLine = line;
    Cy_SysLib_Halt(0UL);
}
#endif  /* !defined(NDEBUG) || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysLib_ClearFlashCacheAndBuffer
****************************************************************************//**
*
* This function invalidates the flash cache and buffer. It ensures the valid
* data is read from flash instead of using outdated data from the cache.
* The caches' LRU structure is also reset to their default state.
*
* \note The operation takes a maximum of three clock cycles on the slowest of
*       the clk_slow and clk_fast clocks.
*
*******************************************************************************/
void Cy_SysLib_ClearFlashCacheAndBuffer(void)
{
    FLASHC->FLASH_CMD = FLASHC_FLASH_CMD_INV_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysLib_ResetBackupDomain
****************************************************************************//**
*
* This function resets the backup domain power to avoid the ILO glitch. The
* glitch can occur when the device is reset due to POR/BOD/XRES while
* the backup voltage is supplied into the system.
*
* \note Writing 1 to BACKUP->RESET resets the backup logic. Hardware clears it
*       when the reset is complete. After setting the register, this function
*       reads the register immediately for returning the result of the backup
*       domain reset state. The reading register is important because the Read
*       itself takes multiple AHB clock cycles, and the reset is actually
*       finishing during that time.
*
* \return CY_SYSLIB_SUCCESS, if BACKUP->RESET read-back is 0.
*         Otherwise returns CY_SYSLIB_INVALID_STATE.
*
*******************************************************************************/
cy_en_syslib_status_t Cy_SysLib_ResetBackupDomain(void)
{
    BACKUP->RESET = BACKUP_RESET_RESET_Msk;

    return ( ((BACKUP->RESET & BACKUP_RESET_RESET_Msk) == 0UL) ? CY_SYSLIB_SUCCESS : CY_SYSLIB_INVALID_STATE );
}


/*******************************************************************************
* Function Name: Cy_SysLib_GetResetReason
****************************************************************************//**
*
* The function returns the cause for the latest reset(s) that occurred in
* the system. The reset causes include an HFCLK error, system faults, and
* device reset on a wakeup from Hibernate mode.
* The return results are consolidated reset causes from reading RES_CAUSE,
* RES_CAUSE2 and PWR_HIBERNATE token registers.
*
* \return The cause of a system reset.
*
* | Name                          | Value
* |-------------------------------|---------------------
* | CY_SYSLIB_RESET_HWWDT         | 0x00001 (bit0)
* | CY_SYSLIB_RESET_ACT_FAULT     | 0x00002 (bit1)
* | CY_SYSLIB_RESET_DPSLP_FAULT   | 0x00004 (bit2)
* | CY_SYSLIB_RESET_CSV_WCO_LOSS  | 0x00008 (bit3)
* | CY_SYSLIB_RESET_SOFT          | 0x00010 (bit4)
* | CY_SYSLIB_RESET_SWWDT0        | 0x00020 (bit5)
* | CY_SYSLIB_RESET_SWWDT1        | 0x00040 (bit6)
* | CY_SYSLIB_RESET_SWWDT2        | 0x00080 (bit7)
* | CY_SYSLIB_RESET_SWWDT3        | 0x00100 (bit8)
* | CY_SYSLIB_RESET_HFCLK_LOSS    | 0x10000 (bit16)
* | CY_SYSLIB_RESET_HFCLK_ERR     | 0x20000 (bit17)
* | CY_SYSLIB_RESET_HIB_WAKEUP    | 0x40000 (bit18)
*
* \note CY_SYSLIB_RESET_CSV_WCO_LOSS, CY_SYSLIB_RESET_HFCLK_LOSS and
*       CY_SYSLIB_RESET_HFCLK_ERR causes of a system reset available only if
*       WCO CSV present in the device.
*
*******************************************************************************/
uint32_t Cy_SysLib_GetResetReason(void)
{
    uint32_t retVal = SRSS->RES_CAUSE;

#if (SRSS_WCOCSV_PRESENT != 0U)
    uint32_t resCause2 = SRSS->RES_CAUSE2;

    retVal |= ((CY_LO16(resCause2) > 0U) ? CY_SYSLIB_RESET_HFCLK_LOSS : 0U) |
              ((CY_HI16(resCause2) > 0U) ? CY_SYSLIB_RESET_HFCLK_ERR  : 0U);
#endif /* (SRSS_WCOCSV_PRESENT != 0U) */

    if(0U != _FLD2VAL(SRSS_PWR_HIBERNATE_TOKEN, SRSS->PWR_HIBERNATE))
    {
        retVal |= CY_SYSLIB_RESET_HIB_WAKEUP;
    }

    return (retVal);
}


#if (SRSS_WCOCSV_PRESENT != 0U) || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysLib_GetNumHfclkResetCause
****************************************************************************//**
*
* This function returns the number of HF_CLK which is a reset cause (RES_CAUSE2)
* by a loss or an error of the high frequency clock.
*
* The Clock supervisors (CSV) can make a reset as CSV_FREQ_ACTION setting
* when a CSV frequency anomaly is detected. The function returns the index
* of HF_CLK, if a reset occurred due to an anomaly HF_CLK. There are two
* different options for monitoring on HF_CLK which are a frequency loss
* and a frequency error.
*
* \return
*   - The number HF_CLK from Clock Supervisor High Frequency Loss:  Bits[15:0]
*   - The number HF_CLK from Clock Supervisor High Frequency error: Bits[31:16]
*
*******************************************************************************/
uint32_t Cy_SysLib_GetNumHfclkResetCause(void)
{
    return (SRSS->RES_CAUSE2);
}
#endif /* (SRSS_WCOCSV_PRESENT != 0U) || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysLib_ClearResetReason
****************************************************************************//**
*
* This function clears the values of RES_CAUSE and RES_CAUSE2. Also it clears
* PWR_HIBERNATE token, which indicates reset event on waking up from HIBERNATE.
*
*******************************************************************************/
void Cy_SysLib_ClearResetReason(void)
{
    /* RES_CAUSE and RES_CAUSE2 register's bits are RW1C (every bit is cleared upon writing 1),
     * so write all ones to clear all the reset reasons.
     */
    SRSS->RES_CAUSE  = 0xFFFFFFFFU;
    SRSS->RES_CAUSE2 = 0xFFFFFFFFU;
    
    if(0U != _FLD2VAL(SRSS_PWR_HIBERNATE_TOKEN, SRSS->PWR_HIBERNATE))
    {
        /* Clears PWR_HIBERNATE token */
        SRSS->PWR_HIBERNATE &= ~SRSS_PWR_HIBERNATE_TOKEN_Msk;
    }
}


#if (CY_CPU_CORTEX_M0P) || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysLib_SoftResetCM4
****************************************************************************//**
*
* This function performs a CM4 Core software reset using the CM4_PWR_CTL
* register. The register is accessed by CM0 Core by using a command transferred
* to SROM API through the IPC channel. When the command is sent, the API waits
* for the IPC channel release.
*
* \note This function should be called only when the CM4 core is in Deep
*       Sleep mode.
* \note This function will not reset CM0+ Core.
* \note This function waits for an IPC channel release state.
*
*******************************************************************************/
void Cy_SysLib_SoftResetCM4(void)
{
    static uint32_t msg = CY_IPC_DATA_FOR_CM4_SOFT_RESET;

    /* Tries to acquire the IPC structure and pass the arguments to SROM API.
    *  SROM API parameters:
    *   ipcPtr: IPC_STRUCT0 - IPC Structure 0 reserved for M0+ Secure Access.
    *   notifyEvent_Intr: 1U - IPC Interrupt Structure 1 is used for Releasing IPC 0 (M0+ NMI Handler).
    *   msgPtr: &msg - The address of SRAM with the API's parameters.
    */
    if(CY_IPC_DRV_SUCCESS != Cy_IPC_Drv_SendMsgPtr(IPC_STRUCT0, 1U, (void *) &msg))
    {
        CY_ASSERT(0U != 0U);
    }

    while(Cy_IPC_Drv_IsLockAcquired(IPC_STRUCT0))
    {
        /* Waits until SROM API runs the command (sent over the IPC channel) and releases the IPC_STRUCT0. */
    }
}
#endif /* CY_CPU_CORTEX_M0P || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysLib_GetUniqueId
****************************************************************************//**
*
* This function returns the silicon unique ID.
* The ID includes Die lot[3]#, Die Wafer#, Die X, Die Y, Die Sort#, Die Minor
* and Die Year.
*
* \return  A combined 64-bit unique ID.
*          [63:57] - DIE_YEAR
*          [56:56] - DIE_MINOR
*          [55:48] - DIE_SORT
*          [47:40] - DIE_Y
*          [39:32] - DIE_X
*          [31:24] - DIE_WAFER
*          [23:16] - DIE_LOT[2]
*          [15: 8] - DIE_LOT[1]
*          [ 7: 0] - DIE_LOT[0]
*
*******************************************************************************/
uint64_t Cy_SysLib_GetUniqueId(void)
{
    uint32_t uniqueIdHi;
    uint32_t uniqueIdLo;

    uniqueIdHi = ((uint32_t) SFLASH->DIE_YEAR        << (CY_UNIQUE_ID_DIE_YEAR_Pos  - CY_UNIQUE_ID_DIE_X_Pos)) |
                 (((uint32_t)SFLASH->DIE_MINOR & 1U) << (CY_UNIQUE_ID_DIE_MINOR_Pos - CY_UNIQUE_ID_DIE_X_Pos)) |
                 ((uint32_t) SFLASH->DIE_SORT        << (CY_UNIQUE_ID_DIE_SORT_Pos  - CY_UNIQUE_ID_DIE_X_Pos)) |
                 ((uint32_t) SFLASH->DIE_Y           << (CY_UNIQUE_ID_DIE_Y_Pos     - CY_UNIQUE_ID_DIE_X_Pos)) |
                 ((uint32_t) SFLASH->DIE_X);

    uniqueIdLo = ((uint32_t) SFLASH->DIE_WAFER       << CY_UNIQUE_ID_DIE_WAFER_Pos) |
                 ((uint32_t) SFLASH->DIE_LOT[2U]     << CY_UNIQUE_ID_DIE_LOT_2_Pos) |
                 ((uint32_t) SFLASH->DIE_LOT[1U]     << CY_UNIQUE_ID_DIE_LOT_1_Pos) |
                 ((uint32_t) SFLASH->DIE_LOT[0U]);

    return (((uint64_t) uniqueIdHi << CY_UNIQUE_ID_DIE_X_Pos) | uniqueIdLo);
}


#if (CY_ARM_FAULT_DEBUG == CY_ARM_FAULT_DEBUG_ENABLED) || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysLib_FaultHandler
****************************************************************************//**
*
* This function stores the ARM Cortex registers into a non-zero init area for
* debugging. This function calls Cy_SysLib_ProcessingFault() after storing all
* information.
*
* \param faultStackAddr The address of the stack pointer, indicates the lowest
*                       address in the fault stack frame to be stored.
* \note This function stores the fault stack frame only for the first occurred
*       fault.
* \note The PDL doesn't provide an API to analyze the stored register
*       values. The user has to add additional functions for the analysis,
*       if necessary.
* \note The CY_ARM_FAULT_DEBUG macro defines if the Fault Handler is enabled.
*       By default it is set to CY_ARM_FAULT_DEBUG_ENABLED and enables the
*       Fault Handler.
*       If there is a necessity to save memory or have some specific custom
*       handler, etc. then CY_ARM_FAULT_DEBUG should be redefined as
*       CY_ARM_FAULT_DEBUG_DISABLED. To do this, the following definition should
*       be added to the compiler Command Line (through the project Build
*       Settings): "-D CY_ARM_FAULT_DEBUG=0".
*
*******************************************************************************/
void Cy_SysLib_FaultHandler(uint32_t const *faultStackAddr)
{
    /* Stores general registers */
    cy_faultFrame.r0  = faultStackAddr[CY_R0_Pos];
    cy_faultFrame.r1  = faultStackAddr[CY_R1_Pos];
    cy_faultFrame.r2  = faultStackAddr[CY_R2_Pos];
    cy_faultFrame.r3  = faultStackAddr[CY_R3_Pos];
    cy_faultFrame.r12 = faultStackAddr[CY_R12_Pos];
    cy_faultFrame.lr  = faultStackAddr[CY_LR_Pos];
    cy_faultFrame.pc  = faultStackAddr[CY_PC_Pos];
    cy_faultFrame.psr = faultStackAddr[CY_PSR_Pos];

    #if (CY_CPU_CORTEX_M4)
        /* Stores the Configurable Fault Status Register state with the fault cause */
        cy_faultFrame.cfsr.cfsrReg = SCB->CFSR;
        /* Stores the Hard Fault Status Register */
        cy_faultFrame.hfsr.hfsrReg = SCB->HFSR;
        /* Stores the System Handler Control and State Register */
        cy_faultFrame.shcsr.shcsrReg = SCB->SHCSR;
        /* Store MemMange fault address */
        cy_faultFrame.mmfar = SCB->MMFAR;
        /* Store Bus fault address */
        cy_faultFrame.bfar = SCB->BFAR;

        #if ((defined (__FPU_PRESENT) && (__FPU_PRESENT == 1U)) && \
             (defined (__FPU_USED   ) && (__FPU_USED    == 1U)))
            /* Checks cumulative exception bits for floating-point exceptions */
            if(0U != (__get_FPSCR() & (CY_FPSCR_IXC_Msk | CY_FPSCR_IDC_Msk)))
            {
                cy_faultFrame.s0    = faultStackAddr[CY_S0_Pos];
                cy_faultFrame.s1    = faultStackAddr[CY_S1_Pos];
                cy_faultFrame.s2    = faultStackAddr[CY_S2_Pos];
                cy_faultFrame.s3    = faultStackAddr[CY_S3_Pos];
                cy_faultFrame.s4    = faultStackAddr[CY_S4_Pos];
                cy_faultFrame.s5    = faultStackAddr[CY_S5_Pos];
                cy_faultFrame.s6    = faultStackAddr[CY_S6_Pos];
                cy_faultFrame.s7    = faultStackAddr[CY_S7_Pos];
                cy_faultFrame.s8    = faultStackAddr[CY_S8_Pos];
                cy_faultFrame.s9    = faultStackAddr[CY_S9_Pos];
                cy_faultFrame.s10   = faultStackAddr[CY_S10_Pos];
                cy_faultFrame.s11   = faultStackAddr[CY_S11_Pos];
                cy_faultFrame.s12   = faultStackAddr[CY_S12_Pos];
                cy_faultFrame.s13   = faultStackAddr[CY_S13_Pos];
                cy_faultFrame.s14   = faultStackAddr[CY_S14_Pos];
                cy_faultFrame.s15   = faultStackAddr[CY_S15_Pos];
                cy_faultFrame.fpscr = faultStackAddr[CY_FPSCR_Pos];
            }
        #endif /* __FPU_PRESENT */
    #endif /* CY_CPU_CORTEX_M4 */

    Cy_SysLib_ProcessingFault();
}


/*******************************************************************************
* Function Name: Cy_SysLib_ProcessingFault
****************************************************************************//**
*
* This function determines how to process the current fault state. By default
* in case of exception the system will stay in the infinite loop of this
* function.
*
* \note This function has the WEAK option, so the user can redefine the function
*       behavior for a custom processing.
*       For example, the function redefinition could be constructed from fault
*       stack processing and NVIC_SystemReset() function call.
*
*******************************************************************************/
__WEAK void Cy_SysLib_ProcessingFault(void)
{
    #if defined(__ARMCC_VERSION)
        /* Assembly implementation of an infinite loop
         * is used for the armcc compiler to preserve the call stack.
         * Otherwise, the compiler destroys the call stack,
         * because treats this API as a no return function.
         */
        Cy_SysLib_AsmInfiniteLoop();
    #else
        while(1) {}
    #endif  /* (__ARMCC_VERSION) */
}
#endif /* (CY_ARM_FAULT_DEBUG == CY_ARM_FAULT_DEBUG_ENABLED) || defined(CY_DOXYGEN) */


/*******************************************************************************
* Function Name: Cy_SysLib_SetWaitStates
****************************************************************************//**
*
* Sets the number of clock cycles the cache will wait for, before it samples
* data coming back from ROM, SRAM, and Flash.
*
* Call this function before increasing the HFClk0 High Frequency clock.
* Call this function optionally after lowering the HFClk0 High Frequency clock
* in order to improve the CPU performance.
*
* Also, call this function before switching the core supply regulator voltage
* (LDO or SIMO Buck) from 1.1V to 0.9V.
* Call this function optionally after switching the core supply regulator
* voltage from 0.9V to 1.1V in order to improve the CPU performance.
*
* \param ulpMode  The device power mode.
*        true  if the device should be switched to the ULP mode (nominal
*              voltage of the core supply regulator should be switched to 0.9V);
*        false if the device should be switched to the LP mode (nominal
*              voltage of the core supply regulator should be switched to 1.1V).
*
* \note Refer to the device TRM for the low power modes description.
*
* \param clkHfMHz  The HFClk0 clock frequency in MHz. Specifying a frequency
*                  above the supported maximum will set the wait states as for
*                  the maximum frequency.
*
*******************************************************************************/
void Cy_SysLib_SetWaitStates(bool ulpMode, uint32_t clkHfMHz)
{
    uint32_t waitStates;
    uint32_t freqMax;

    freqMax = ulpMode ? CY_SYSLIB_ULP_SLOW_WS_0_FREQ_MAX : CY_SYSLIB_LP_SLOW_WS_0_FREQ_MAX;
    waitStates = (clkHfMHz <= freqMax) ? 0UL : 1UL;

    /* ROM */
    CPUSS->ROM_CTL = _CLR_SET_FLD32U(CPUSS->ROM_CTL, CPUSS_ROM_CTL_SLOW_WS, waitStates);
    CPUSS->ROM_CTL = _CLR_SET_FLD32U(CPUSS->ROM_CTL, CPUSS_ROM_CTL_FAST_WS, 0UL);

    /* SRAM */
    CPUSS->RAM0_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM0_CTL0, CPUSS_RAM0_CTL0_SLOW_WS, waitStates);
    CPUSS->RAM0_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM0_CTL0, CPUSS_RAM0_CTL0_FAST_WS, 0UL);
    #if defined (RAMC1_PRESENT) && (RAMC1_PRESENT == 1UL)
        CPUSS->RAM1_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM1_CTL0, CPUSS_RAM1_CTL0_SLOW_WS, waitStates);
        CPUSS->RAM1_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM1_CTL0, CPUSS_RAM1_CTL0_FAST_WS, 0UL);
    #endif /* defined (RAMC1_PRESENT) && (RAMC1_PRESENT == 1UL) */
    #if defined (RAMC2_PRESENT) && (RAMC2_PRESENT == 1UL)
        CPUSS->RAM2_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM2_CTL0, CPUSS_RAM2_CTL0_SLOW_WS, waitStates);
        CPUSS->RAM2_CTL0 = _CLR_SET_FLD32U(CPUSS->RAM2_CTL0, CPUSS_RAM2_CTL0_FAST_WS, 0UL);
    #endif /* defined (RAMC2_PRESENT) && (RAMC2_PRESENT == 1UL) */

    /* Flash */
    if (ulpMode)
    {
        waitStates =  (clkHfMHz <= CY_SYSLIB_FLASH_ULP_WS_0_FREQ_MAX) ? 0UL :
                     ((clkHfMHz <= CY_SYSLIB_FLASH_ULP_WS_1_FREQ_MAX) ? 1UL : 2UL);
    }
    else
    {
        waitStates =  (clkHfMHz <= cy_device->flashCtlMainWs0Freq) ? 0UL :
                     ((clkHfMHz <= cy_device->flashCtlMainWs1Freq) ? 1UL :
                     ((clkHfMHz <= cy_device->flashCtlMainWs2Freq) ? 2UL :
                     ((clkHfMHz <= cy_device->flashCtlMainWs3Freq) ? 3UL : 
                     ((clkHfMHz <= cy_device->flashCtlMainWs4Freq) ? 4UL : 5UL))));
    }

    FLASHC->FLASH_CTL = _CLR_SET_FLD32U(FLASHC->FLASH_CTL, FLASHC_FLASH_CTL_MAIN_WS, waitStates);
}


/* [] END OF FILE */
