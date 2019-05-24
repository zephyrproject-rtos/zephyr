/*
 *  Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 */

//*****************************************************************************
//
//! \addtogroup PRCM_Power_Reset_Clock_Module_api
//! @{
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_apps_rcm.h"
#include "inc/hw_gprcm.h"
#include "inc/hw_hib1p2.h"
#include "inc/hw_hib3p3.h"
#include "inc/hw_ocp_shared.h"
#include "inc/hw_common_reg.h"
#include "prcm.h"
#include "interrupt.h"
#include "cpu.h"
#include "flash.h"
#include "utils.h"
#include "pin.h"


//*****************************************************************************
// Macro definition
//*****************************************************************************
#define PRCM_SOFT_RESET           0x00000001
#define PRCM_ENABLE_STATUS        0x00000002
#define SYS_CLK                   80000000
#define XTAL_CLK                  40000000
#define PLL_DIV_8                 30000000


//*****************************************************************************
//    CC3200 does not have a true RTC capability. However, API(s) in this file
//    provide an effective mechanism to support RTC feature in the device.
//
//    The implementation to support RTC has been kept very simple. A set of
//    HIB Memory Registers in conjunction with Slow Clock Counter are used
//    to render RTC information to users. Core principle of design involves
//    two steps (a) establish an association between user provided wall-clock
//    and slow clock counter. (b) store reference value of this associattion
//    in HIB Registers. This reference value and SCC value are then combined
//    to create real-world calendar time.
//
//    Across HIB cycles, value stored in HIB Registers is retained and slow
//    clock counter continues to tick, thereby, this arragement is relevant
//    and valid as long as device has a (tickle) battery power.
//
//    Further, provision also has been made to set an alarm. When it RTC value
//    matches that of set for alarm, an interrupt is generated.
//
//    HIB MEM REG0 and REG1 are reserved for TI.
//
//    If RTC feature is not used, then HIB REG2 & REG3 are available to user.
//
//    Lower half of REG0 is used for TI HW ECO.
//*****************************************************************************
#define RTC_U64MSEC_MK(u32Secs, u16Msec) (((unsigned long long)u32Secs << 10)|\
                                          (u16Msec & 0x3FF))

#define RTC_SECS_IN_U64MSEC(u64Msec)     ((unsigned long)(u64Msec  >>   10))
#define RTC_MSEC_IN_U64MSEC(u64Msec)     ((unsigned short)(u64Msec & 0x3FF))

#define RTC_SECS_U32_REG_ADDR            (HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG3)
#define RTC_MSEC_U16_REG_ADDR            (HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG2+2)

#define RTC_U32SECS_REG                 (HWREG(RTC_SECS_U32_REG_ADDR))
#define RTC_U16MSEC_REG                 (*(unsigned short*)RTC_MSEC_U16_REG_ADDR)

//*****************************************************************************
// Register Access and Updates
//
// Tick of SCC has a resolution of 32768Hz, meaning 1 sec is equal to 32768
// clock ticks. Ideal way of getting time in millisecond will involve floating
// point arithmetic (division by 32.768). To avoid this, we simply divide it by
// 32, which will give a range from 0 -1023(instead of 0-999). To use this
// output correctly we have to take care of this inaccuracy externally.
// following wrapper can be used to convert the value from cycles to
// millisecond:
//
// CYCLES_U16MS(cycles)	((cycles *1000)/ 1024),
//
// Similarly, before setting the value, it must be first converted (from ms to
// cycles).
//
// U16MS_CYCLES(msec)	((msec *1024)/1000)
//
// Note: There is a precision loss of 1 ms with the above scheme.
//
//*****************************************************************************
#define SCC_U64MSEC_GET()                (PRCMSlowClkCtrGet() >> 5)
#define SCC_U64MSEC_MATCH_SET(u64Msec)   (PRCMSlowClkCtrMatchSet(u64Msec << 5))
#define SCC_U64MSEC_MATCH_GET()          (PRCMSlowClkCtrMatchGet() >> 5)

//*****************************************************************************
//
// Bit:  31 is used to indicate use of RTC. If set as '1', RTC feature is used.
// Bits: 30 to 26 are reserved, available to software for use
// Bits: 25 to 16 are used to save millisecond part of RTC reference.
// Bits: 15 to 0 are being used for HW Changes / ECO
//
//*****************************************************************************

//*****************************************************************************
// Set RTC USE Bit
//*****************************************************************************
static void RTCUseSet(void)
{
  unsigned short usRegValue;

  usRegValue = RTC_U16MSEC_REG |  (1 << 15);

  UtilsDelay((80*200)/3);

  RTC_U16MSEC_REG = usRegValue;
}

//*****************************************************************************
// Checks if RTC-USE bit is set
//*****************************************************************************
static tBoolean IsRTCUsed(void)
{
  unsigned short usRegValue;

  usRegValue = RTC_U16MSEC_REG;

  UtilsDelay((80*200)/3);

  return ((usRegValue & (1 << 15))? true : false);
}

//*****************************************************************************
// Read 16-bit mSecs
//*****************************************************************************
static unsigned short RTCU16MSecRegRead(void)
{
  unsigned short usRegValue;

  usRegValue = RTC_U16MSEC_REG;

  UtilsDelay((80*200)/3);

  return (usRegValue & 0x3FF);
}

//*****************************************************************************
// Write 16-bit mSecs
//*****************************************************************************
static void RTCU16MSecRegWrite(unsigned short u16Msec)
{
   unsigned short usRegValue;

   usRegValue = RTC_U16MSEC_REG;

   UtilsDelay((80*200)/3);

   RTC_U16MSEC_REG = ((usRegValue & ~0x3FF) |u16Msec);
}

//*****************************************************************************
// Read 32-bit Secs
//*****************************************************************************
static unsigned long RTCU32SecRegRead(void)
{
  return (PRCMHIBRegRead(RTC_SECS_U32_REG_ADDR));
}

//*****************************************************************************
// Write 32-bit Secs
//*****************************************************************************
static void RTCU32SecRegWrite(unsigned long u32Msec)
{
   PRCMHIBRegWrite(RTC_SECS_U32_REG_ADDR, u32Msec);
}

//*****************************************************************************
// Macros
//*****************************************************************************
#define IS_RTC_USED()                   IsRTCUsed()
#define RTC_USE_SET()                   RTCUseSet()

#define RTC_U16MSEC_REG_RD()            RTCU16MSecRegRead()
#define RTC_U16MSEC_REG_WR(u16Msec)     RTCU16MSecRegWrite(u16Msec)

#define RTC_U32SECS_REG_RD()            RTCU32SecRegRead()
#define RTC_U32SECS_REG_WR(u32Secs)     RTCU32SecRegWrite(u32Secs)

#define SELECT_SCC_U42BITS(u64Msec)     (u64Msec & 0x3ffffffffff)

//*****************************************************************************
// Global Peripheral clock and rest Registers
//*****************************************************************************
static const PRCM_PeriphRegs_t PRCM_PeriphRegsList[] =
{

	{APPS_RCM_O_CAMERA_CLK_GATING,   APPS_RCM_O_CAMERA_SOFT_RESET   },
	{APPS_RCM_O_MCASP_CLK_GATING,    APPS_RCM_O_MCASP_SOFT_RESET    },
	{APPS_RCM_O_MMCHS_CLK_GATING,    APPS_RCM_O_MMCHS_SOFT_RESET    },
	{APPS_RCM_O_MCSPI_A1_CLK_GATING, APPS_RCM_O_MCSPI_A1_SOFT_RESET },
	{APPS_RCM_O_MCSPI_A2_CLK_GATING, APPS_RCM_O_MCSPI_A2_SOFT_RESET },
	{APPS_RCM_O_UDMA_A_CLK_GATING,   APPS_RCM_O_UDMA_A_SOFT_RESET   },
	{APPS_RCM_O_GPIO_A_CLK_GATING,   APPS_RCM_O_GPIO_A_SOFT_RESET   },
	{APPS_RCM_O_GPIO_B_CLK_GATING,   APPS_RCM_O_GPIO_B_SOFT_RESET   },
	{APPS_RCM_O_GPIO_C_CLK_GATING,   APPS_RCM_O_GPIO_C_SOFT_RESET   },
	{APPS_RCM_O_GPIO_D_CLK_GATING,   APPS_RCM_O_GPIO_D_SOFT_RESET   },
	{APPS_RCM_O_GPIO_E_CLK_GATING,   APPS_RCM_O_GPIO_E_SOFT_RESET   },
	{APPS_RCM_O_WDOG_A_CLK_GATING,   APPS_RCM_O_WDOG_A_SOFT_RESET   },
	{APPS_RCM_O_UART_A0_CLK_GATING,  APPS_RCM_O_UART_A0_SOFT_RESET  },
	{APPS_RCM_O_UART_A1_CLK_GATING,  APPS_RCM_O_UART_A1_SOFT_RESET  },
	{APPS_RCM_O_GPT_A0_CLK_GATING ,  APPS_RCM_O_GPT_A0_SOFT_RESET   },
	{APPS_RCM_O_GPT_A1_CLK_GATING,   APPS_RCM_O_GPT_A1_SOFT_RESET   },
	{APPS_RCM_O_GPT_A2_CLK_GATING,   APPS_RCM_O_GPT_A2_SOFT_RESET   },
	{APPS_RCM_O_GPT_A3_CLK_GATING,   APPS_RCM_O_GPT_A3_SOFT_RESET   },
	{APPS_RCM_O_CRYPTO_CLK_GATING,   APPS_RCM_O_CRYPTO_SOFT_RESET   },
	{APPS_RCM_O_MCSPI_S0_CLK_GATING, APPS_RCM_O_MCSPI_S0_SOFT_RESET },
	{APPS_RCM_O_I2C_CLK_GATING,      APPS_RCM_O_I2C_SOFT_RESET      }

};

//*****************************************************************************
//
//! Performs a software reset of a MCU and associated peripherals
//!
//! \param bIncludeSubsystem is \b true to reset associated peripherals.
//!
//! This function performs a software reset of a MCU and associated peripherals.
//! To reset the associated peripheral, the parameter \e bIncludeSubsystem
//! should be set to \b true.
//!
//! \return None.
//
//*****************************************************************************
void PRCMMCUReset(tBoolean bIncludeSubsystem)
{
  if(bIncludeSubsystem)
  {
    //
    // Reset Apps processor and associated peripheral
    //
    HWREG(GPRCM_BASE+ GPRCM_O_APPS_SOFT_RESET) = 0x2;
  }
  else
  {
    //
    // Reset Apps processor only
    //
    HWREG(GPRCM_BASE+ GPRCM_O_APPS_SOFT_RESET) = 0x1;
  }

  //
  // Wait for system to enter hibernate
  //
  __asm("    wfi\n");

  //
  // Infinite loop
  //
  while(1)
  {

  }
}

//*****************************************************************************
//
//! Gets the reason for a reset.
//!
//! This function returns the reason(s) for a reset. The reset reason are:-
//! -\b PRCM_POWER_ON  - Device is powering up.
//! -\b PRCM_LPDS_EXIT - Device is exiting from LPDS.
//! -\b PRCM_CORE_RESET - Device is exiting soft core only reset
//! -\b PRCM_MCU_RESET - Device is exiting soft subsystem reset.
//! -\b PRCM_WDT_RESET - Device was reset by watchdog.
//! -\b PRCM_SOC_RESET - Device is exting SOC reset.
//! -\b PRCM_HIB_EXIT - Device is exiting hibernate.
//!
//! \return Returns one of the cause defined above.
//
//*****************************************************************************
unsigned long PRCMSysResetCauseGet()
{
  unsigned long ulWakeupStatus;

  //
  // Read the Reset status
  //
  ulWakeupStatus = (HWREG(GPRCM_BASE+ GPRCM_O_APPS_RESET_CAUSE) & 0xFF);

  //
  // For hibernate do additional check.
  //
  if(ulWakeupStatus == PRCM_POWER_ON)
  {
    if(PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_WAKE_STATUS) & 0x1)
    {
      ulWakeupStatus = PRCM_HIB_EXIT;

      if( (HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) & (0x00000280)) == 0x00000280  )
      {
        ulWakeupStatus = PRCM_WDT_RESET;
      }
    }
  }
  else if((ulWakeupStatus == PRCM_LPDS_EXIT) &&
          !(HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG1) & (1 <<2)) )
  {
    if(HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) & (0x1<<8))
    {
      ulWakeupStatus = PRCM_POWER_ON;
    }
  }

  //
  // Return status.
  //
  return ulWakeupStatus;
}

//*****************************************************************************
//
//! Enable clock(s) to peripheral.
//!
//! \param ulPeripheral is one of the valid peripherals
//! \param ulClkFlags are bitmask of clock(s) to be enabled.
//!
//! This function enables the clock for the specified peripheral. Peripherals
//! are by default clock gated (disabled) and generates a bus fault if
//! accessed.
//!
//! The parameter \e ulClkFlags can be logical OR of the following:
//! -\b PRCM_RUN_MODE_CLK - Ungates clock to the peripheral
//! -\b PRCM_SLP_MODE_CLK - Keeps the clocks ungated in sleep.
//!
//! \return None.
//
//*****************************************************************************
void
PRCMPeripheralClkEnable(unsigned long ulPeripheral, unsigned long ulClkFlags)
{
  //
  // Enable the specified peripheral clocks, Nothing to be done for PRCM_ADC
  // as it is a dummy define for pinmux utility code generation
  //
  if(ulPeripheral != PRCM_ADC)
  {
    HWREG(ARCM_BASE + PRCM_PeriphRegsList[ulPeripheral].ulClkReg) |= ulClkFlags;
  }
  
  //
  // Checking ROM Version less than 2.x.x. 
  // Only for driverlib backward compatibility
  //
  if( (HWREG(0x00000400) & 0xFFFF) < 2 )
  {
    //
    // Set the default clock for camera
    //
    if(ulPeripheral == PRCM_CAMERA)
    {
      HWREG(ARCM_BASE + APPS_RCM_O_CAMERA_CLK_GEN) = 0x0404;
    }
  }
}

//*****************************************************************************
//
//! Disables clock(s) to peripheral.
//!
//! \param ulPeripheral is one of the valid peripherals
//! \param ulClkFlags are bitmask of clock(s) to be enabled.
//!
//! This function disable the clock for the specified peripheral. Peripherals
//! are by default clock gated (disabled) and generated a bus fault if
//! accessed.
//!
//! The parameter \e ulClkFlags can be logical OR bit fields as defined in
//! PRCMEnablePeripheral().
//!
//! \return None.
//
//*****************************************************************************
void
PRCMPeripheralClkDisable(unsigned long ulPeripheral, unsigned long ulClkFlags)
{
  //
  // Disable the specified peripheral clocks
  //
  HWREG(ARCM_BASE + PRCM_PeriphRegsList[ulPeripheral].ulClkReg) &= ~ulClkFlags;
}

//*****************************************************************************
//
//! Gets the input clock for the specified peripheral.
//!
//! \param ulPeripheral is one of the valid peripherals.
//!
//! This function gets the input clock for the specified peripheral.
//!
//! The parameter \e ulPeripheral has the same definition as that in
//! PRCMPeripheralClkEnable();
//!
//! \return Returns input clock frequency for specified peripheral.
//
//*****************************************************************************
unsigned long
PRCMPeripheralClockGet(unsigned long ulPeripheral)
{
  unsigned long ulClockFreq;
  unsigned long ulHiPulseDiv;
  unsigned long ulLoPulseDiv;

  //
  // Get the clock based on specified peripheral.
  //
  if((ulPeripheral == PRCM_GSPI) | (ulPeripheral == PRCM_SSPI))
  {
    return XTAL_CLK;
  }
  else if(ulPeripheral == PRCM_LSPI)
  {
    //
    // Check NWP generation.
    //
    if((HWREG(GPRCM_BASE + GPRCM_O_GPRCM_DIEID_READ_REG4) >> 24) & 0x02)
    {
       return PLL_DIV_8;
    }
    else
    {
       return XTAL_CLK;
    }
  }
  else if(ulPeripheral == PRCM_CAMERA)
  {
    ulHiPulseDiv = ((HWREG(ARCM_BASE + APPS_RCM_O_CAMERA_CLK_GEN) >> 8) & 0x07);
    ulLoPulseDiv = (HWREG(ARCM_BASE + APPS_RCM_O_CAMERA_CLK_GEN) & 0xFF);
  }
  else if(ulPeripheral == PRCM_SDHOST)
  {
    ulHiPulseDiv = ((HWREG(ARCM_BASE + APPS_RCM_O_MMCHS_CLK_GEN) >> 8) & 0x07);
    ulLoPulseDiv = (HWREG(ARCM_BASE + APPS_RCM_O_MMCHS_CLK_GEN) & 0xFF);
  }
  else
  {
    return SYS_CLK;
  }

  //
  // Compute the clock freq. from the divider value
  //
  ulClockFreq = (240000000/((ulHiPulseDiv + 1) + (ulLoPulseDiv + 1)));

  //
  // Return the clock rate.
  //
  return ulClockFreq;
}

//*****************************************************************************
//
//! Performs a software reset of a peripheral.
//!
//! \param ulPeripheral is one of the valid peripheral.
//!
//! This function does soft reset of the specified peripheral
//!
//! \return None.
//
//*****************************************************************************
void
PRCMPeripheralReset(unsigned long ulPeripheral)
{
  volatile unsigned long ulDelay;

  if( ulPeripheral != PRCM_DTHE)
  {
    //
    // Assert the reset
    //
    HWREG(ARCM_BASE + PRCM_PeriphRegsList[ulPeripheral].ulRstReg)
                                                         |= PRCM_SOFT_RESET;
    //
    // Delay a little bit.
    //
    for(ulDelay = 0; ulDelay < 16; ulDelay++)
    {
    }

    //
    // Deassert the reset
    //
    HWREG(ARCM_BASE+PRCM_PeriphRegsList[ulPeripheral].ulRstReg)
                                                          &= ~PRCM_SOFT_RESET;
  }
}

//*****************************************************************************
//
//! Determines if a peripheral is ready.
//!
//! \param ulPeripheral is one of the valid modules
//!
//! This function determines if a particular peripheral is ready to be
//! accessed. The peripheral may be in a non-ready state if it is not enabled,
//! is being held in reset, or is in the process of becoming ready after being
//! enabled or taken out of reset.
//!
//! \return Returns \b true if the  peripheral is ready, \b false otherwise.
//
//*****************************************************************************
tBoolean
PRCMPeripheralStatusGet(unsigned long ulPeripheral)
{
  unsigned long ReadyBit;

  //
  // Read the ready bit status
  //
  ReadyBit = HWREG(ARCM_BASE + PRCM_PeriphRegsList[ulPeripheral].ulRstReg);
  ReadyBit = ReadyBit & PRCM_ENABLE_STATUS;

  if (ReadyBit)
  {
    //
    // Module is ready
    //
    return(true);
  }
  else
  {
    //
    // Module is not ready
    //
    return(false);
  }
}

//*****************************************************************************
//
//! Configure I2S fracactional divider
//!
//! \param ulI2CClkFreq is the required input clock for McAPS module
//!
//! This function configures I2S fractional divider. By default this
//! divider is set to output 24 Mhz clock to I2S module.
//!
//! The minimum frequency that can be obtained by configuring this divider is
//!
//! (240000KHz/1023.99) =  234.377 KHz
//!
//! \return None.
//
//*****************************************************************************
void
PRCMI2SClockFreqSet(unsigned long ulI2CClkFreq)
{
  unsigned long long ullDiv;
  unsigned short usInteger;
  unsigned short usFrac;

  ullDiv = (((unsigned long long)240000000 * 65536)/ulI2CClkFreq);

  usInteger = (ullDiv/65536);
  usFrac    = (ullDiv%65536);

  HWREG(ARCM_BASE + APPS_RCM_O_MCASP_FRAC_CLK_CONFIG0) =
    ((usInteger & 0x3FF) << 16 | usFrac);
}

//*****************************************************************************
//
//! Sets the LPDS exit PC and SP restore vlaues.
//!
//! \param ulStackPtr is the SP restore value.
//! \param ulProgCntr is the PC restore value
//!
//! This function sets the LPDS exit PC and SP restore vlaues. Setting
//! \e ulProgCntr to a non-zero value, forces bootloader to jump to that
//! address with Stack Pointer initialized to \e ulStackPtr on LPDS exit,
//! otherwise the application's vector table entries are used.
//!
//! \return None.
//
//*****************************************************************************
void
PRCMLPDSRestoreInfoSet(unsigned long ulStackPtr, unsigned long ulProgCntr)
{
  //
  // ROM Version 2.x.x or greater
  //
  if( (HWREG(0x00000400) & 0xFFFF) >= 2 )
  {
    //
    // Set The SP Value
    //
    HWREG(0x4402E160) = ulStackPtr;

    //
    // Set The PC Value
    //
    HWREG(0x4402E198) = ulProgCntr;

  }
  else
  {
    //
    // Set The SP Value
    //
    HWREG(0x4402E18C) = ulStackPtr;

    //
    // Set The PC Value
    //
    HWREG(0x4402E190) = ulProgCntr;
  }
}

//*****************************************************************************
//
//! Puts the system into Low Power Deel Sleep (LPDS) power mode.
//!
//! This function puts the system into Low Power Deel Sleep (LPDS) power mode.
//! A call to this function never returns and the execution starts from Reset.
//! \sa PRCMLPDSRestoreInfoSet().
//!
//! \return None.
//!
//! \note  External debugger will always disconnect whenever the system
//!  enters LPDS and debug interface is shutdown until next POR reset. In order
//!  to avoid this and allow for connecting back the debugger after waking up
//!  from LPDS \sa PRCMLPDSEnterKeepDebugIf().
//!
//
//*****************************************************************************
void
PRCMLPDSEnter()
{
  unsigned long ulChipId;

  //
  // Read the Chip ID
  //
  ulChipId = ((HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2) >> 16) & 0x1F);

  //
  // Check if flash exists
  //
  if( (0x11 == ulChipId) || (0x19 == ulChipId))
  {

    //
    // Disable the flash
    //
    FlashDisable();
  }

#ifndef KEEP_TESTPD_ALIVE

  //
  // Disable TestPD
  //
  HWREG(0x4402E168) |= (1<<9);
#endif

  //
  // Set bandgap duty cycle to 1
  //
  HWREG(HIB1P2_BASE + HIB1P2_O_BGAP_DUTY_CYCLING_EXIT_CFG) = 0x1;

  //
  // Request LPDS
  //
  HWREG(ARCM_BASE + APPS_RCM_O_APPS_LPDS_REQ)
          = APPS_RCM_APPS_LPDS_REQ_APPS_LPDS_REQ;

  //
  // Wait for system to enter LPDS
  //
  __asm("    wfi\n");

  //
  // Infinite loop
  //
  while(1)
  {

  }

}


//*****************************************************************************
//
//! Puts the system into Low Power Deel Sleep (LPDS) power mode keeping
//! debug interface alive.
//!
//! This function puts the system into Low Power Deel Sleep (LPDS) power mode
//! keeping debug interface alive. A call to this function never returns and the
//! execution starts from Reset \sa PRCMLPDSRestoreInfoSet().
//!
//! \return None.
//!
//! \note External debugger will always disconnect whenever the system
//!  enters LPDS, using this API will allow connecting back the debugger after
//!  waking up from LPDS. This API is recommended for development purposes
//!  only as it adds to the current consumption of the system.
//!
//
//*****************************************************************************
void
PRCMLPDSEnterKeepDebugIf()
{
  unsigned long ulChipId;

  //
  // Read the Chip ID
  //
  ulChipId = ((HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2) >> 16) & 0x1F);

  //
  // Check if flash exists
  //
  if( (0x11 == ulChipId) || (0x19 == ulChipId))
  {

    //
    // Disable the flash
    //
    FlashDisable();
  }

  //
  // Set bandgap duty cycle to 1
  //
  HWREG(HIB1P2_BASE + HIB1P2_O_BGAP_DUTY_CYCLING_EXIT_CFG) = 0x1;

  //
  // Request LPDS
  //
  HWREG(ARCM_BASE + APPS_RCM_O_APPS_LPDS_REQ)
          = APPS_RCM_APPS_LPDS_REQ_APPS_LPDS_REQ;

  //
  // Wait for system to enter LPDS
  //
  __asm("    wfi\n");

  //
  // Infinite loop
  //
  while(1)
  {

  }

}

//*****************************************************************************
//
//! Enable the individual LPDS wakeup source(s).
//!
//! \param ulLpdsWakeupSrc is logical OR of wakeup sources.
//!
//! This function enable the individual LPDS wakeup source(s) and following
//! three wakeup sources (\e ulLpdsWakeupSrc ) are supported by the device.
//! -\b PRCM_LPDS_HOST_IRQ
//! -\b PRCM_LPDS_GPIO
//! -\b PRCM_LPDS_TIMER
//!
//! \return None.
//
//*****************************************************************************
void
PRCMLPDSWakeupSourceEnable(unsigned long ulLpdsWakeupSrc)
{
  unsigned long ulRegVal;

  //
  // Read the current wakup sources
  //
  ulRegVal = HWREG(GPRCM_BASE+ GPRCM_O_APPS_LPDS_WAKEUP_CFG);

  //
  // Enable individual wakeup source
  //
  ulRegVal = ((ulRegVal | ulLpdsWakeupSrc) & 0x91);

  //
  // Set the configuration in the register
  //
  HWREG(GPRCM_BASE+ GPRCM_O_APPS_LPDS_WAKEUP_CFG) = ulRegVal;
}

//*****************************************************************************
//
//! Disable the individual LPDS wakeup source(s).
//!
//! \param ulLpdsWakeupSrc is logical OR of wakeup sources.
//!
//! This function enable the individual LPDS wakeup source(s) and following
//! three wake up sources (\e ulLpdsWakeupSrc ) are supported by the device.
//! -\b PRCM_LPDS_HOST_IRQ
//! -\b PRCM_LPDS_GPIO
//! -\b PRCM_LPDS_TIMER
//!
//! \return None.
//
//*****************************************************************************
void
PRCMLPDSWakeupSourceDisable(unsigned long ulLpdsWakeupSrc)
{
  HWREG(GPRCM_BASE+ GPRCM_O_APPS_LPDS_WAKEUP_CFG) &= ~ulLpdsWakeupSrc;
}


//*****************************************************************************
//
//! Get LPDS wakeup cause
//!
//! This function gets LPDS wakeup caouse
//!
//! \return Returns values enumerated as described in
//! PRCMLPDSWakeupSourceEnable().
//
//*****************************************************************************
unsigned long
PRCMLPDSWakeupCauseGet()
{
  return (HWREG(GPRCM_BASE+ GPRCM_O_APPS_LPDS_WAKEUP_SRC));
}

//*****************************************************************************
//
//! Sets LPDS wakeup Timer
//!
//! \param ulTicks is number of 32.768 KHz clocks
//!
//! This function sets internal LPDS wakeup timer running at 32.768 KHz. The
//! timer is only configured if the parameter \e ulTicks is in valid range i.e.
//! from 21 to 2^32.
//!
//! \return Returns \b true on success, \b false otherwise.
//
//*****************************************************************************
void
PRCMLPDSIntervalSet(unsigned long ulTicks)
{
  //
  // Check sleep is atleast for 21 cycles
  // If not set the sleep time to 21 cycles
  //
  if( ulTicks < 21)
  {
      ulTicks = 21;
  }

  HWREG(GPRCM_BASE + GPRCM_O_APPS_LPDS_WAKETIME_WAKE_CFG) = ulTicks;
  HWREG(GPRCM_BASE + GPRCM_O_APPS_LPDS_WAKETIME_OPP_CFG) = ulTicks-20;
}

//*****************************************************************************
//
//! Selects the GPIO for LPDS wakeup
//!
//! \param ulGPIOPin is one of the valid GPIO fro LPDS wakeup.
//! \param ulType is the wakeup trigger type.
//!
//! This function setects the wakeup GPIO for LPDS wakeup and can be
//! used to select one out of 7 pre-defined GPIO(s).
//!
//! The parameter \e ulLpdsGPIOSel should be one of the following:-
//! -\b PRCM_LPDS_GPIO2
//! -\b PRCM_LPDS_GPIO4
//! -\b PRCM_LPDS_GPIO13
//! -\b PRCM_LPDS_GPIO17
//! -\b PRCM_LPDS_GPIO11
//! -\b PRCM_LPDS_GPIO24
//! -\b PRCM_LPDS_GPIO26
//!
//! The parameter \e ulType sets the trigger type and can be one of the
//! following:
//! - \b PRCM_LPDS_LOW_LEVEL
//! - \b PRCM_LPDS_HIGH_LEVEL
//! - \b PRCM_LPDS_FALL_EDGE
//! - \b PRCM_LPDS_RISE_EDGE
//!
//! \return None.
//
//*****************************************************************************
void
PRCMLPDSWakeUpGPIOSelect(unsigned long ulGPIOPin, unsigned long ulType)
{
  //
  // Set the wakeup GPIO
  //
  PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_LPDS_GPIO_SEL, ulGPIOPin);

  //
  // Set the trigger type.
  //
  HWREG(GPRCM_BASE + GPRCM_O_APPS_GPIO_WAKE_CONF) = (ulType & 0x3);
}

//*****************************************************************************
//
//! Puts the system into Sleep.
//!
//! This function puts the system into sleep power mode. System exits the power
//! state on any one of the available interrupt. On exit from sleep mode the
//! function returns to the calling function with all the processor core
//! registers retained.
//!
//! \return None.
//
//*****************************************************************************
void
PRCMSleepEnter()
{
  //
  // Request Sleep
  //
  CPUwfi();
}

//*****************************************************************************
//
//! Enable SRAM column retention during LPDS Power mode(s)
//!
//! \param ulSramColSel is bit mask of valid SRAM columns.
//! \param ulModeFlags is the bit mask of power modes.
//!
//! This functions enables the SRAM retention. The device supports configurable
//! SRAM column retention in Low Power Deep Sleep (LPDS). Each column is of
//! 64 KB size.
//!
//! The parameter \e ulSramColSel should be logical OR of the following:-
//! -\b PRCM_SRAM_COL_1
//! -\b PRCM_SRAM_COL_2
//! -\b PRCM_SRAM_COL_3
//! -\b PRCM_SRAM_COL_4
//!
//! The parameter \e ulModeFlags selects the power modes and sholud be logical
//! OR of one or more of the following
//! -\b PRCM_SRAM_LPDS_RET
//!
//! \return None.
//
//****************************************************************************
void
PRCMSRAMRetentionEnable(unsigned long ulSramColSel, unsigned long ulModeFlags)
{
  if(ulModeFlags & PRCM_SRAM_LPDS_RET)
  {
    //
    // Configure LPDS SRAM retention register
    //
    HWREG(GPRCM_BASE+ GPRCM_O_APPS_SRAM_LPDS_CFG) = (ulSramColSel & 0xF);
  }
}

//*****************************************************************************
//
//! Disable SRAM column retention during LPDS Power mode(s).
//!
//! \param ulSramColSel is bit mask of valid SRAM columns.
//! \param ulFlags is the bit mask of power modes.
//!
//! This functions disable the SRAM retention. The device supports configurable
//! SRAM column retention in Low Power Deep Sleep (LPDS). Each column is
//! of 64 KB size.
//!
//! The parameter \e ulSramColSel should be logical OR of the following:-
//! -\b PRCM_SRAM_COL_1
//! -\b PRCM_SRAM_COL_2
//! -\b PRCM_SRAM_COL_3
//! -\b PRCM_SRAM_COL_4
//!
//! The parameter \e ulFlags selects the power modes and sholud be logical OR
//! of one or more of the following
//! -\b PRCM_SRAM_LPDS_RET
//!
//! \return None.
//
//****************************************************************************
void
PRCMSRAMRetentionDisable(unsigned long ulSramColSel, unsigned long ulFlags)
{
  if(ulFlags & PRCM_SRAM_LPDS_RET)
  {
    //
    // Configure LPDS SRAM retention register
    //
    HWREG(GPRCM_BASE+ GPRCM_O_APPS_SRAM_LPDS_CFG) &= ~(ulSramColSel & 0xF);
  }
}


//*****************************************************************************
//
//! Enables individual HIB wakeup source(s).
//!
//! \param ulHIBWakupSrc is logical OR of valid HIB wakeup sources.
//!
//! This function enables individual HIB wakeup source(s). The paramter
//! \e ulHIBWakupSrc is the bit mask of HIB wakeup sources and should be
//! logical OR of one or more of the follwoing :-
//! -\b PRCM_HIB_SLOW_CLK_CTR
//! -\b PRCM_HIB_GPIO2
//! -\b PRCM_HIB_GPIO4
//! -\b PRCM_HIB_GPIO13
//! -\b PRCM_HIB_GPIO17
//! -\b PRCM_HIB_GPIO11
//! -\b PRCM_HIB_GPIO24
//! -\b PRCM_HIB_GPIO26
//!
//! \return None.
//
//*****************************************************************************
void
PRCMHibernateWakeupSourceEnable(unsigned long ulHIBWakupSrc)
{
  unsigned long ulRegValue;

  //
  // Read the RTC register
  //
  ulRegValue = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN);

  //
  // Enable the RTC as wakeup source if specified
  //
  ulRegValue |= (ulHIBWakupSrc & 0x1);

  //
  // Enable HIB wakeup sources
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN,ulRegValue);

  //
  // REad the GPIO wakeup configuration register
  //
  ulRegValue = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_EN);

  //
  // Enable the specified GPIOs a wakeup sources
  //
  ulRegValue |= ((ulHIBWakupSrc>>16)&0xFF);

  //
  // Write the new register configuration
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_EN,ulRegValue);
}

//*****************************************************************************
//
//! Disable individual HIB wakeup source(s).
//!
//! \param ulHIBWakupSrc is logical OR of valid HIB wakeup sources.
//!
//! This function disable individual HIB wakeup source(s). The paramter
//! \e ulHIBWakupSrc is same as bit fileds defined in
//! PRCMEnableHibernateWakeupSource()
//!
//! \return None.
//
//*****************************************************************************
void
PRCMHibernateWakeupSourceDisable(unsigned long ulHIBWakupSrc)
{
  unsigned long ulRegValue;

  //
  // Read the RTC register
  //
  ulRegValue = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN);

  //
  // Disable the RTC as wakeup source if specified
  //
  ulRegValue &= ~(ulHIBWakupSrc & 0x1);

  //
  // Disable HIB wakeup sources
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN,ulRegValue);

  //
  // Read the GPIO wakeup configuration register
  //
  ulRegValue = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_EN);

  //
  // Enable the specified GPIOs a wakeup sources
  //
  ulRegValue &= ~((ulHIBWakupSrc>>16)&0xFF);

  //
  // Write the new register configuration
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_EN,ulRegValue);
}


//*****************************************************************************
//
//! Get hibernate wakeup cause
//!
//! This function gets the hibernate wakeup cause.
//!
//! \return Returns \b PRCM_HIB_WAKEUP_CAUSE_SLOW_CLOCK or
//! \b PRCM_HIB_WAKEUP_CAUSE_GPIO
//
//*****************************************************************************
unsigned long
PRCMHibernateWakeupCauseGet()
{
  //
  // Supported only in ES2.00 and Later devices i.e. ROM Version 2.x.x or greater
  //
  if( (HWREG(0x00000400) & 0xFFFF) >= 2 )
  {
      return ((PRCMHIBRegRead((OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8))>>2)&0x7);
  }
  else
  {
      return(0);
  }
}

//*****************************************************************************
//
//! Sets Hibernate wakeup Timer
//!
//! \param ullTicks is number of 32.768 KHz clocks
//!
//! This function sets internal hibernate wakeup timer running at 32.768 KHz.
//!
//! \return Returns \b true on success, \b false otherwise.
//
//*****************************************************************************
void
PRCMHibernateIntervalSet(unsigned long long ullTicks)
{
  unsigned long long ullRTCVal;

  //
  // Latch the RTC vlaue
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_READ ,0x1);

  //
  // Read latched values as 2 32-bit vlaues
  //
  ullRTCVal  = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_TIMER_MSW);
  ullRTCVal  = ullRTCVal << 32;
  ullRTCVal |= PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_LSW);

  //
  // Add the interval
  //
  ullRTCVal = ullRTCVal + ullTicks;

  //
  // Set RTC match value
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_LSW_CONF,
                                            (unsigned long)(ullRTCVal));
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_MSW_CONF,
                                           (unsigned long)(ullRTCVal>>32));
}


//*****************************************************************************
//
//! Selects the GPIO(s) for hibernate wakeup
//!
//! \param ulGPIOBitMap is the bit-map of valid hibernate wakeup GPIO.
//! \param ulType is the wakeup trigger type.
//!
//! This function setects the wakeup GPIO for hibernate and can be
//! used to select any combination of 7 pre-defined GPIO(s).
//!
//! This function enables individual HIB wakeup source(s). The paramter
//! \e ulGPIOBitMap should be one of the follwoing :-
//! -\b PRCM_HIB_GPIO2
//! -\b PRCM_HIB_GPIO4
//! -\b PRCM_HIB_GPIO13
//! -\b PRCM_HIB_GPIO17
//! -\b PRCM_HIB_GPIO11
//! -\b PRCM_HIB_GPIO24
//! -\b PRCM_HIB_GPIO26
//!
//! The parameter \e ulType sets the trigger type and can be one of the
//! following:
//! - \b PRCM_HIB_LOW_LEVEL
//! - \b PRCM_HIB_HIGH_LEVEL
//! - \b PRCM_HIB_FALL_EDGE
//! - \b PRCM_HIB_RISE_EDGE
//!
//! \return None.
//
//*****************************************************************************
void
PRCMHibernateWakeUpGPIOSelect(unsigned long ulGPIOBitMap, unsigned long ulType)
{
  unsigned char ucLoop;
  unsigned long ulRegValue;

  //
  // Shift the bits to extract the GPIO selection
  //
  ulGPIOBitMap >>= 16;

  //
  // Set the configuration for each GPIO
  //
  for(ucLoop=0; ucLoop < 7; ucLoop++)
  {
    if(ulGPIOBitMap & (1<<ucLoop))
    {
      ulRegValue  = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_CONF);
      ulRegValue = (ulRegValue & (~(0x3 << (ucLoop*2)))) | (ulType <<(ucLoop*2));
      PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_GPIO_WAKE_CONF, ulRegValue);
    }
  }
}

//*****************************************************************************
//
//! Puts the system into Hibernate
//!
//! This function puts the system into Hibernate. The device enters HIB
//! immediately and on exit from HIB device core starts its execution from
//! reset thus the function never returns.
//!
//! \return None.
//
//*****************************************************************************
void
PRCMHibernateEnter()
{

  //
  // Request hibernate.
  //
  PRCMHIBRegWrite((HIB3P3_BASE+HIB3P3_O_MEM_HIB_REQ),0x1);

  //
  // Wait for system to enter hibernate
  //
  __asm("    wfi\n");

  //
  // Infinite loop
  //
  while(1)
  {

  }
}

//*****************************************************************************
//
//! Gets the current value of the internal slow clock counter
//!
//! This function latches and reads the internal RTC running at 32.768 Khz
//!
//! \return 64-bit current counter vlaue.
//
//*****************************************************************************
unsigned long long
PRCMSlowClkCtrGet()
{
  unsigned long long ullRTCVal;

  //
  // Latch the RTC vlaue
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_READ, 0x1);

  //
  // Read latched values as 2 32-bit vlaues
  //
  ullRTCVal  = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_TIMER_MSW);
  ullRTCVal  = ullRTCVal << 32;
  ullRTCVal |= PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_LSW);

  return ullRTCVal;
}

//*****************************************************************************
//
//! Gets the current value of the internal slow clock counter
//!
//! This function is similar to \sa PRCMSlowClkCtrGet() but reads the counter
//! value from a relatively faster interface using an auto-latch mechainsm.
//!
//! \note Due to the nature of implemetation of auto latching, when using this
//! API, the recommendation is to read the value thrice and identify the right
//! value (as 2 out the 3 read values will always be correct and with a max. of
//! 1 LSB change)
//!
//! \return 64-bit current counter vlaue.
//
//*****************************************************************************
unsigned long long PRCMSlowClkCtrFastGet(void)
{
  unsigned long long ullRTCVal;

  //
  // Read as 2 32-bit values
  //
  ullRTCVal = HWREG(HIB1P2_BASE + HIB1P2_O_HIB_RTC_TIMER_MSW_1P2);
  ullRTCVal = ullRTCVal << 32;
  ullRTCVal |= HWREG(HIB1P2_BASE + HIB1P2_O_HIB_RTC_TIMER_LSW_1P2);

  return ullRTCVal;

}

//*****************************************************************************
//
//! Sets slow clock counter match value to interrupt the processor.
//!
//! \param ullValue is the match value.
//!
//! This function sets the match value for  slow clock counter. This is use
//! to interrupt the processor when RTC counts to the specified value.
//!
//! \return None.
//
//*****************************************************************************
void PRCMSlowClkCtrMatchSet(unsigned long long ullValue)
{
  //
  // Set RTC match value
  //
  PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_LSW_CONF,
                                           (unsigned long)(ullValue));
  PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_MSW_CONF,
                                           (unsigned long)(ullValue>>32));
}

//*****************************************************************************
//
//! Gets slow clock counter match value.
//!
//! This function gets the match value for  slow clock counter. This is use
//! to interrupt the processor when RTC counts to the specified value.
//!
//! \return None.
//
//*****************************************************************************
unsigned long long PRCMSlowClkCtrMatchGet()
{
  unsigned long long ullValue;

  //
  // Get RTC match value
  //
  ullValue = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_MSW_CONF);
  ullValue = ullValue<<32;
  ullValue |= PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_LSW_CONF);

  //
  // Return the value
  //
  return ullValue;
}


//*****************************************************************************
//
//! Write to On-Chip Retention (OCR) register.
//!
//! This function writes to On-Chip retention register. The device supports two
//! 4-byte OCR register which are retained across all power mode.
//!
//! The parameter \e ucIndex is an index of the OCR and can be \b 0 or \b 1.
//!
//! These registers are shared by the RTC implementation (if Driverlib RTC
//! APIs are used), ROM, and user application.
//!
//! When RTC APIs in use:
//!
//!     |-----------------------------------------------|
//!     |                  INDEX 1                      |
//!     |-----------------------------------------------|
//!     |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|
//!     |-----------------------------------------------|
//!     |           Reserved by RTC APIs - YY           |
//!     |-----------------------------------------------|
//!     |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
//!     |-----------------------------------------------|
//!     |           Reserved by RTC APIs - YY           |
//!     |-----------------------------------------------|
//!
//!
//!     |-----------------------------------------------|
//!     |                  INDEX 0                      |
//!     |-----------------------------------------------|
//!     |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|
//!     |-----------------------------------------------|
//!     |           Reserved by RTC APIs - YY           |
//!     |-----------------------------------------------|
//!     |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
//!     |-----------------------------------------------|
//!     |YY|        For User Application             |XX|
//!     |-----------------------------------------------|
//!
//!     YY => Reserved by RTC APIs. If Driverlib RTC APIs are used
//!     XX => Reserved by ROM
//!
//!
//! When RTC APIs are not in use:
//!
//!     |-----------------------------------------------|
//!     |                  INDEX 1                      |
//!     |-----------------------------------------------|
//!     |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|
//!     |-----------------------------------------------|
//!     |            For User Application               |
//!     |-----------------------------------------------|
//!     |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
//!     |-----------------------------------------------|
//!     |            For User Application               |
//!     |-----------------------------------------------|
//!
//!
//!     |-----------------------------------------------|
//!     |                  INDEX 0                      |
//!     |-----------------------------------------------|
//!     |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|
//!     |-----------------------------------------------|
//!     |            For User Application               |
//!     |-----------------------------------------------|
//!     |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
//!     |-----------------------------------------------|
//!     |           For User Application             |XX|
//!     |-----------------------------------------------|
//!
//!     XX => Reserved by ROM
//!
//!
//!
//! \return None.
//
//*****************************************************************************
void PRCMOCRRegisterWrite(unsigned char ucIndex, unsigned long ulRegValue)
{
  unsigned long ulVal;

  //
  // Compuitr the offset
  //
  ucIndex = ucIndex << 2;

  //
  // If bit 0 is reserved
  //
  if( (HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) & (0x00000080)) &&
      (ucIndex == 0) )
  {
    ulVal = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG2 + ucIndex);
    ulRegValue = ((ulRegValue << 0x1) | (ulVal & (0x1)));
  }

  //
  // Write thr value
  //
  PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG2 + ucIndex,ulRegValue);

}

//*****************************************************************************
//
//! Read from On-Chip Retention (OCR) register.
//!
//! This function reads from On-Chip retention register. The device supports two
//! 4-byte OCR register which are retained across all power mode.
//!
//! The parameter \e ucIndex is an index of the OCR and can be \b 0 or \b 1.
//!
//! \sa PRCMOCRRegisterWrite() for the register usage details.
//!
//! \return None.
//
//*****************************************************************************
unsigned long PRCMOCRRegisterRead(unsigned char ucIndex)
{
  unsigned long ulRet;

  //
  // Read the OCR register
  //
  ulRet = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_REG2 + (ucIndex << 2));

  //
  // If bit 0 is reserved
  //
  if( (HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) & (0x00000080)) &&
      (ucIndex == 0) )
  {
     ulRet = ulRet >> 0x1;
  }

  //
  // Return the read value.
  //
  return ulRet;
}

//*****************************************************************************
//
//! Registers an interrupt handler for the PRCM.
//!
//! \param pfnHandler is a pointer to the function to be called when the
//! interrupt is activated.
//!
//! This function does the actual registering of the interrupt handler.  This
//! function enables the global interrupt in the interrupt controller;
//!
//! \return None.
//
//*****************************************************************************
void PRCMIntRegister(void (*pfnHandler)(void))
{
  //
  // Register the interrupt handler.
  //
  IntRegister(INT_PRCM, pfnHandler);

  //
  // Enable the PRCM interrupt.
  //
  IntEnable(INT_PRCM);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the PRCM.
//!
//! This function does the actual unregistering of the interrupt handler.  It
//! clears the handler to be called when a PRCM interrupt occurs.  This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \return None.
//
//*****************************************************************************
void PRCMIntUnregister()
{
  //
  // Enable the UART interrupt.
  //
  IntDisable(INT_PRCM);

  //
  // Register the interrupt handler.
  //
  IntUnregister(INT_PRCM);
}

//*****************************************************************************
//
//! Enables individual PRCM interrupt sources.
//!
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! This function enables the indicated ARCM interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! The \e ulIntFlags parameter is the logical OR of any of the following:
//! -\b PRCM_INT_SLOW_CLK_CTR
//!
//
//*****************************************************************************
void PRCMIntEnable(unsigned long ulIntFlags)
{
  unsigned long ulRegValue;

  if(ulIntFlags & PRCM_INT_SLOW_CLK_CTR )
  {
    //
    // Enable PRCM interrupt
    //
    HWREG(ARCM_BASE + APPS_RCM_O_APPS_RCM_INTERRUPT_ENABLE) |= 0x4;

    //
    // Enable RTC interrupt
    //
    ulRegValue = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_ENABLE);
    ulRegValue |= 0x1;
    PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_ENABLE, ulRegValue);
  }
}

//*****************************************************************************
//
//! Disables individual PRCM interrupt sources.
//!
//! \param ulIntFlags is the bit mask of the interrupt sources to be disabled.
//!
//! This function disables the indicated ARCM interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to PRCMEnableInterrupt().
//!
//! \return None.
//
//*****************************************************************************
void PRCMIntDisable(unsigned long ulIntFlags)
{
  unsigned long ulRegValue;

  if(ulIntFlags & PRCM_INT_SLOW_CLK_CTR )
  {
    //
    // Disable PRCM interrupt
    //
    HWREG(ARCM_BASE + APPS_RCM_O_APPS_RCM_INTERRUPT_ENABLE) &= ~0x4;

    //
    // Disable RTC interrupt
    //
    ulRegValue = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_ENABLE);
    ulRegValue &= ~0x1;
    PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_IRQ_ENABLE, ulRegValue);
  }
}

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! This function returns the PRCM interrupt status of interrupts that are
//! allowed to reflect to the processor. The interrupts are cleared on read.
//!
//! \return Returns the current interrupt status.
//
//*****************************************************************************
unsigned long PRCMIntStatus()
{
    return HWREG(ARCM_BASE + APPS_RCM_O_APPS_RCM_INTERRUPT_STATUS);
}

//*****************************************************************************
//
//! Mark the function of RTC as being used
//!
//! This function marks in HW that feature to maintain calendar time in device
//! is being used.
//!
//! Specifically, this feature reserves user's HIB Register-1 accessed through
//! PRCMOCRRegisterWrite(1) for internal work / purpose, therefore, the stated
//! register is not available to user. Also, users must not excercise the Slow
//! Clock Counter API(s), if RTC has been set for use.
//!
//! The RTC feature, if set or marked, can be only reset either through reboot
//! or power cycle.
//!
//! \return None.
//
//*****************************************************************************
void PRCMRTCInUseSet()
{
        RTC_USE_SET();
        return;
}

//*****************************************************************************
//
//! Ascertain whether function of RTC is being used
//!
//! This function indicates whether function of RTC is being used on the device
//! or not.
//!
//! This routine should be utilized by the application software, when returning
//! from low-power, to confirm that RTC has been put to use and may not need to
//! set the value of the RTC.
//!
//! The RTC feature, if set or marked, can be only reset either through reboot
//! or power cycle.
//!
//! \return None.
//
//*****************************************************************************
tBoolean PRCMRTCInUseGet()
{
        return IS_RTC_USED()? true : false;
}

//*****************************************************************************
//
//! Set the calendar time in the device.
//!
//! \param ulSecs refers to the seconds part of the  calendar time
//! \param usMsec refers to the fractional (ms) part of the second
//!
//! This function sets the specified calendar time in the device. The calendar
//! time is outlined in terms of seconds and milliseconds. However, the device
//! makes no assumption about the origin or reference of the calendar time.
//!
//! The device uses the indicated calendar value to update and maintain the
//! wall-clock time across active and low power states.
//!
//! The function PRCMRTCInUseSet() must be invoked prior to use of this feature.
//!
//! \return None.
//
//*****************************************************************************
void PRCMRTCSet(unsigned long ulSecs, unsigned short usMsec)
{
        unsigned long long ullMsec = 0;

        if(IS_RTC_USED()) {
                ullMsec = RTC_U64MSEC_MK(ulSecs, usMsec) - SCC_U64MSEC_GET();

                 RTC_U32SECS_REG_WR(RTC_SECS_IN_U64MSEC(ullMsec));
                 RTC_U16MSEC_REG_WR(RTC_MSEC_IN_U64MSEC(ullMsec));
        }

        return;
}

//*****************************************************************************
//
//! Get the instantaneous calendar time from the device.
//!
//! \param ulSecs refers to the seconds part of the  calendar time
//! \param usMsec refers to the fractional (ms) part of the second
//!
//! This function fetches the instantaneous value of the ticking calendar time
//! from the device. The calendar time is outlined in terms of seconds and
//! milliseconds.
//!
//! The device provides the calendar value that has been maintained across
//! active and low power states.
//!
//! The function PRCMRTCSet() must have been invoked once to set a reference.
//!
//! \return None.
//
//*****************************************************************************
void PRCMRTCGet(unsigned long *ulSecs, unsigned short *usMsec)
{
        unsigned long long ullMsec = 0;

        if(IS_RTC_USED()) {
                ullMsec  = RTC_U64MSEC_MK(RTC_U32SECS_REG_RD(),
                                          RTC_U16MSEC_REG_RD());
                ullMsec += SCC_U64MSEC_GET();
        }

        *ulSecs = RTC_SECS_IN_U64MSEC(ullMsec);
        *usMsec = RTC_MSEC_IN_U64MSEC(ullMsec);

        return;
}

//*****************************************************************************
//
//! Set a calendar time alarm.
//!
//! \param ulSecs refers to the seconds part of the  calendar time
//! \param usMsec refers to the fractional (ms) part of the second
//!
//! This function sets an wall-clock alarm in the device to be reported for  a
//! futuristic calendar time. The calendar time is outlined in terms of seconds
//! and milliseconds.
//!
//! The device provides uses the calendar value that has been maintained across
//! active and low power states to report attainment of alarm time.
//!
//! The function PRCMRTCSet() must have been invoked once to set a reference.
//!
//! \return None.
//
//*****************************************************************************
void PRCMRTCMatchSet(unsigned long ulSecs, unsigned short usMsec)
{
        unsigned long long ullMsec = 0;

        if(IS_RTC_USED()) {
                ullMsec  = RTC_U64MSEC_MK(ulSecs, usMsec);
                ullMsec -= RTC_U64MSEC_MK(RTC_U32SECS_REG_RD(),
                                          RTC_U16MSEC_REG_RD());
                SCC_U64MSEC_MATCH_SET(SELECT_SCC_U42BITS(ullMsec));
        }

        return;
}

//*****************************************************************************
//
//! Get a previously set calendar time alarm.
//!
//! \param ulSecs refers to the seconds part of the  calendar time
//! \param usMsec refers to the fractional (ms) part of the second
//!
//! This function fetches from the device a wall-clock alarm that would  have
//! been previously set in the device. The calendar time is outlined in terms
//! of seconds and milliseconds.
//!
//! If no alarm was set in the past, then this function would fetch a random
//! information.
//!
//! The function PRCMRTCMatchSet() must have been invoked once to set an alarm.
//!
//! \return None.
//
//*****************************************************************************
void PRCMRTCMatchGet(unsigned long *ulSecs, unsigned short *usMsec)
{
        unsigned long long ullMsec = 0;

        if(IS_RTC_USED()) {
                ullMsec  = SCC_U64MSEC_MATCH_GET();
                ullMsec += RTC_U64MSEC_MK(RTC_U32SECS_REG_RD(),
                                          RTC_U16MSEC_REG_RD());
        }

        *ulSecs = RTC_SECS_IN_U64MSEC(ullMsec);
        *usMsec = RTC_MSEC_IN_U64MSEC(ullMsec);

        return;
}

//*****************************************************************************
//
//! MCU Initialization Routine
//!
//! This function contains all the mandatory bug fixes, ECO enables,
//! initializations for both CC3200 and CC3220.
//!
//! \note \b ###IMPORTANT### : This is a routine which should be one of the
//! first things to be executed after control comes to MCU Application code.
//!
//! \return None
//
//*****************************************************************************
void PRCMCC3200MCUInit()
{

  if( PRCMSysResetCauseGet() != PRCM_LPDS_EXIT )
  {
    if( 0x00010001 == HWREG(0x00000400) )
    {

#ifndef REMOVE_CC3200_ES_1_2_1_CODE

      unsigned long ulRegVal;

      //
      // DIG DCDC NFET SEL and COT mode disable
      //
      HWREG(0x4402F010) = 0x30031820;
      HWREG(0x4402F00C) = 0x04000000;

      UtilsDelay(32000);

      //
      // ANA DCDC clock config
      //
      HWREG(0x4402F11C) = 0x099;
      HWREG(0x4402F11C) = 0x0AA;
      HWREG(0x4402F11C) = 0x1AA;

      //
      // PA DCDC clock config
      //
      HWREG(0x4402F124) = 0x099;
      HWREG(0x4402F124) = 0x0AA;
      HWREG(0x4402F124) = 0x1AA;

      //
      // TD Flash timing configurations in case of MCU WDT reset
      //
      if((HWREG(0x4402D00C) & 0xFF) == 0x00000005)
      {
          HWREG(0x400F707C) |= 0x01840082;
          HWREG(0x400F70C4)= 0x1;
          HWREG(0x400F70C4)= 0x0;
      }

      //
      // Take I2C semaphore
      //
      ulRegVal = HWREG(0x400F7000);
      ulRegVal = (ulRegVal & ~0x3) | 0x1;
      HWREG(0x400F7000) = ulRegVal;

      //
      // Take GPIO semaphore
      //
      ulRegVal = HWREG(0x400F703C);
      ulRegVal = (ulRegVal & ~0x3FF) | 0x155;
      HWREG(0x400F703C) = ulRegVal;

      //
      // Enable 32KHz internal RC oscillator
      //
      PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_INT_OSC_CONF, 0x00000101);

      //
      // Delay for a little bit.
      //
      UtilsDelay(8000);

      //
      // Enable 16MHz clock
      //
      HWREG(HIB1P2_BASE+HIB1P2_O_CM_OSC_16M_CONFIG) = 0x00010008;

      //
      // Delay for a little bit.
      //
      UtilsDelay(8000);

#endif // REMOVE_CC3200_ES_1_2_1_CODE

    }
    else
    {

      unsigned long ulRegValue;

      //
      // DIG DCDC LPDS ECO Enable
      //
      HWREG(0x4402F064) |= 0x800000;

      //
      // Enable hibernate ECO for PG 1.32 devices only. With this ECO enabled,
      // any hibernate wakeup source will be kept maked until the device enters
      // hibernate completely (analog + digital)
      //
      ulRegValue = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_HIB_REG0);
      PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG0, ulRegValue | (1<<4));

      //
      // Handling the clock switching (for 1.32 only)
      //
      HWREG(0x4402E16C) |= 0x3C;
    }


    //
    // Enable uDMA
    //
    PRCMPeripheralClkEnable(PRCM_UDMA,PRCM_RUN_MODE_CLK);

    //
    // Reset uDMA
    //
    PRCMPeripheralReset(PRCM_UDMA);

    //
    // Disable uDMA
    //
    PRCMPeripheralClkDisable(PRCM_UDMA,PRCM_RUN_MODE_CLK);

    //
    // Enable RTC
    //
    if(PRCMSysResetCauseGet()== PRCM_POWER_ON)
    {
        PRCMHIBRegWrite(0x4402F804,0x1);
    }

    //
    // SWD mode
    //
    if(((HWREG(0x4402F0C8) & 0xFF) == 0x2))
    {
        HWREG(0x4402E110) = ((HWREG(0x4402E110) & ~0xC0F) | 0x2);
        HWREG(0x4402E114) = ((HWREG(0x4402E114) & ~0xC0F) | 0x2);
    }

    //
    // Override JTAG mux
    //
    HWREG(0x4402E184) |= 0x2;

    //
    // Change UART pins(55,57) mode to PIN_MODE_0 if they are in PIN_MODE_1
    //
    if (PinModeGet(PIN_55) == PIN_MODE_1)
    {
        PinModeSet(PIN_55,PIN_MODE_0);
    }
    if (PinModeGet(PIN_57) == PIN_MODE_1)
    {
        PinModeSet(PIN_57,PIN_MODE_0);
    }

    //
    // Change I2C pins(1,2) mode to PIN_MODE_0 if they are in PIN_MODE_1
    //
    if (PinModeGet(PIN_01) == PIN_MODE_1)
    {
        PinModeSet(PIN_01,PIN_MODE_0);
    }
    if (PinModeGet(PIN_02) == PIN_MODE_1)
    {
        PinModeSet(PIN_02,PIN_MODE_0);
    }

    //
    // DIG DCDC VOUT trim settings based on PROCESS INDICATOR
    //
    if(((HWREG(0x4402DC78) >> 22) & 0xF) == 0xE)
    {
        HWREG(0x4402F0B0) = ((HWREG(0x4402F0B0) & ~(0x00FC0000))|(0x32 << 18));
    }
    else
    {
        HWREG(0x4402F0B0) = ((HWREG(0x4402F0B0) & ~(0x00FC0000))|(0x29 << 18));
    }

    //
    // Enable SOFT RESTART in case of DIG DCDC collapse
    //
    HWREG(0x4402FC74) &= ~(0x10000000);

    //
    // Required only if ROM version is lower than 2.x.x
    //
    if( (HWREG(0x00000400) & 0xFFFF) < 2 )
    {
      //
      // Disable the sleep for ANA DCDC
      //
      HWREG(0x4402F0A8) |= 0x00000004 ;
    }
    else if( (HWREG(0x00000400) >> 16)  >= 1 )
    {
      //
      // Enable NWP force reset and HIB on WDT reset
      // Enable direct boot path for flash
      //
      HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) |= ((7<<5) | 0x1);
      if((HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG2) & 0x1) )
      {
          HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG2) &= ~0x1;
          HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_8) |= (1<<9);

          //
          // Clear the RTC hib wake up source
          //
          HWREG(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN) &= ~0x1;

          //
          // Reset RTC match value
          //
          HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_WAKE_LSW_CONF) = 0;
          HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_WAKE_MSW_CONF) = 0;

      }
    }
    
    unsigned long efuse_reg2;
    unsigned long ulDevMajorVer, ulDevMinorVer;
    //
    // Read the device identification register
    //
    efuse_reg2= HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2);
    
    //
    // Read the ROM mojor and minor version
    //
    ulDevMajorVer = ((efuse_reg2 >> 28) & 0xF);
    ulDevMinorVer = ((efuse_reg2 >> 24) & 0xF);
  
    if(((ulDevMajorVer == 0x3) && (ulDevMinorVer == 0)) || (ulDevMajorVer < 0x3))
    {
      unsigned int Scratch, PreRegulatedMode;

      // 0x4402F840 => 6th bit “1” indicates device is in pre-regulated mode.
      PreRegulatedMode = (HWREG(0x4402F840) >> 6) & 1;
    
      if( PreRegulatedMode)
      {
        Scratch = HWREG(0x4402F028);
        Scratch &= 0xFFFFFF7F; // <7:7> = 0
        HWREG(0x4402F028) = Scratch;
        
        Scratch = HWREG(0x4402F010);
        Scratch &= 0x0FFFFFFF; // <31:28> = 0
        Scratch |= 0x10000000; // <31:28> = 1
        HWREG(0x4402F010) = Scratch;
      }
      else
      {
        Scratch = HWREG(0x4402F024);

        Scratch &= 0xFFFFFFF0; // <3:0> = 0
        Scratch |= 0x00000001; // <3:0> = 1
        Scratch &= 0xFFFFF0FF; // <11:8> = 0000
        Scratch |= 0x00000500; // <11:8> = 0101
        Scratch &= 0xFFFE7FFF; // <16:15> = 0000
        Scratch |= 0x00010000; // <16:15> = 10

        HWREG(0x4402F024) = Scratch;

        Scratch = HWREG(0x4402F028);

        Scratch &= 0xFFFFFF7F; // <7:7> = 0
        Scratch &= 0x0FFFFFFF; // <31:28> = 0
        Scratch &= 0xFF0FFFFF; // <23:20> = 0
        Scratch |= 0x00300000; // <23:20> = 0011
        Scratch &= 0xFFF0FFFF; // <19:16> = 0
        Scratch |= 0x00030000; // <19:16> = 0011

        HWREG(0x4402F028) = Scratch;
        HWREG(0x4402F010) &= 0x0FFFFFFF; // <31:28> = 0
      }
    }
    else
    {
      unsigned int Scratch, PreRegulatedMode;

      // 0x4402F840 => 6th bit “1” indicates device is in pre-regulated mode.
      PreRegulatedMode = (HWREG(0x4402F840) >> 6) & 1;
    
      Scratch = HWREG(0x4402F028);
      Scratch &= 0xFFFFFF7F; // <7:7> = 0
      HWREG(0x4402F028) = Scratch;
    
      HWREG(0x4402F010) &= 0x0FFFFFFF; // <31:28> = 0
      if( PreRegulatedMode)
      {
        HWREG(0x4402F010) |= 0x10000000; // <31:28> = 1
      }  
    }
  }
  else
  {
    unsigned long ulRegVal;

    //
    // I2C Configuration
    //
    ulRegVal = HWREG(COMMON_REG_BASE + COMMON_REG_O_I2C_Properties_Register);
    ulRegVal = (ulRegVal & ~0x3) | 0x1;
    HWREG(COMMON_REG_BASE + COMMON_REG_O_I2C_Properties_Register) = ulRegVal;

    //
    // GPIO configuration
    //
    ulRegVal = HWREG(COMMON_REG_BASE + COMMON_REG_O_GPIO_properties_register);
    ulRegVal = (ulRegVal & ~0x3FF) | 0x155;
    HWREG(COMMON_REG_BASE + COMMON_REG_O_GPIO_properties_register) = ulRegVal;

  }
}

//*****************************************************************************
//
//! Reads 32-bit value from register at specified address
//!
//! \param ulRegAddr is the address of register to be read.
//!
//! This function reads 32-bit value from the register as specified by
//! \e ulRegAddr.
//!
//! \return Return the value of the register.
//
//*****************************************************************************
unsigned long PRCMHIBRegRead(unsigned long ulRegAddr)
{
  unsigned long ulValue;

  //
  // Read the Reg value
  //
  ulValue = HWREG(ulRegAddr);

  //
  // Wait for 200 uSec
  //
  UtilsDelay((80*200)/3);

  //
  // Return the value
  //
  return ulValue;
}

//*****************************************************************************
//
//! Writes 32-bit value to register at specified address
//!
//! \param ulRegAddr is the address of register to be read.
//! \param ulValue is the 32-bit value to be written.
//!
//! This function writes 32-bit value passed as \e ulValue to the register as
//! specified by \e ulRegAddr
//!
//! \return None
//
//*****************************************************************************
void PRCMHIBRegWrite(unsigned long ulRegAddr, unsigned long ulValue)
{
  //
  // Read the Reg value
  //
  HWREG(ulRegAddr) = ulValue;

  //
  // Wait for 200 uSec
  //
  UtilsDelay((80*200)/3);
}

//*****************************************************************************
//
//! \param ulDivider is clock frequency divider value
//! \param ulWidth is the width of the high pulse
//!
//! This function sets the input frequency for camera module.
//!
//! The frequency is calculated as follows:
//!
//!        f_out = 240MHz/ulDivider;
//!
//! The parameter \e ulWidth sets the width of the high pulse.
//!
//! For e.g.:
//!
//!     ulDivider = 4;
//!     ulWidth   = 2;
//!
//!     f_out = 30 MHz and 50% duty cycle
//!
//! And,
//!
//!     ulDivider = 4;
//!     ulWidth   = 1;
//!
//!     f_out = 30 MHz and 25% duty cycle
//!
//! \return 0 on success, 1 on error
//
//*****************************************************************************
unsigned long PRCMCameraFreqSet(unsigned char ulDivider, unsigned char ulWidth)
{
    if(ulDivider > ulWidth && ulWidth != 0 )
    {
      //
      // Set  the hifh pulse width
      //
      HWREG(ARCM_BASE +
            APPS_RCM_O_CAMERA_CLK_GEN) = (((ulWidth & 0x07) -1) << 8);

      //
      // Set the low pulse width
      //
      HWREG(ARCM_BASE +
            APPS_RCM_O_CAMERA_CLK_GEN) = ((ulDivider - ulWidth - 1) & 0x07);
      //
      // Return success
      //
      return 0;
    }

    //
    // Success;
    //
    return 1;
}

//*****************************************************************************
//
//! Enable the IO value retention
//!
//! \param ulIORetGrpFlags is one of the valid IO groups.
//!
//! This function enables the IO retention for group of pins as specified by
//! \e ulIORetGrpFlags parameter. Enabling retention will immediately lock the
//! digital pins, in the specified group, to their current state (0 or 1).
//! Output pins can only be driven when retention is disabled.
//!
//! The parameter \e ulIORetGrpFlags can be logical OR of one or
//! more of the following:
//! -\b PRCM_IO_RET_GRP_0 - All the pins except sFlash and JTAG interface
//! -\b PRCM_IO_RET_GRP_1 - sFlash interface pins 11,12,13,14
//! -\b PRCM_IO_RET_GRP_2 - JTAG TDI and TDO interface pins 16,17
//! -\b PRCM_IO_RET_GRP_3 - JTAG TCK and TMS interface pins 19,20
//!
//! \note Use case is to park the pins when entering HIB.
//!
//! \return None.
//
//*****************************************************************************
void PRCMIORetentionEnable(unsigned long ulIORetGrpFlags)
{
  unsigned long ulRegVal;
  
  //
  // Supported only in ES2.00 and Later devices i.e. ROM Version 2.x.x or greater
  //
  if( (HWREG(0x00000400) & 0xFFFF) >= 2 )
  {
    //
    // Disable IO Pad to ODI Path
    //
    HWREG(OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CMN_CONFIG) |= 0x00001D00;

    //
    // 0b'0 in bit 5 for JTAG PADS
    // 0b'0 in bit 0 for all other IOs
    //
    HWREG(OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CMN_CONFIG) &= ~(0x00000023);

    //
    // Enable retention for GRP0
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_0 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_PAD_OEN_RET33_CONF);
      ulRegVal |= 0x5;
      PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_PAD_OEN_RET33_CONF,ulRegVal);
    }

    //
    // Enable retention for GRP1
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_1 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_HIB_REG0);
      ulRegVal |= ((0x3<<5));
      PRCMHIBRegWrite(HIB3P3_BASE  + HIB3P3_O_MEM_HIB_REG0,ulRegVal);
    }

    //
    // Enable retention for GRP2
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_2 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF);
      ulRegVal |= 0x00000101;
      PRCMHIBRegWrite(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF,ulRegVal);
    }

    //
    // Enable retention for GRP3
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_3 )
    {
       ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF);
       ulRegVal |= 0x00000204;
       PRCMHIBRegWrite(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF,ulRegVal);
    }
  }
}

//*****************************************************************************
//
//! Disable the IO value retention
//!
//! \param ulIORetGrpFlags is one of the valid IO groups.
//!
//! This function disable the IO retention for group of pins as specified by
//! \e ulIORetGrpFlags parameter. Disabling retention will unlock the
//! digital pins in the specified group. Output pins can only be driven when
//! retention is disabled.
//!
//! The parameter \e ulIORetGrpFlags can be logical OR of one or
//! more of the following:
//! -\b PRCM_IO_RET_GRP_0 - All the pins except sFlash and JTAG interface
//! -\b PRCM_IO_RET_GRP_1 - sFlash interface pins 11,12,13,14
//! -\b PRCM_IO_RET_GRP_2 - JTAG TDI and TDO interface pins 16,17
//! -\b PRCM_IO_RET_GRP_3 - JTAG TCK and TMS interface pins 19,20
//!
//! \note Use case is to un-park the pins when exiting HIB
//!
//! \return None.
//
//*****************************************************************************
void PRCMIORetentionDisable(unsigned long ulIORetGrpFlags)
{
  unsigned long ulRegVal;
  
  //
  // Supported only in ES2.00 and Later devices i.e. ROM Version 2.x.x or greater
  //
  if( (HWREG(0x00000400) & 0xFFFF) >= 2 )
  {

    //
    // Enable IO Pad to ODI Path
    //
    HWREG(OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CMN_CONFIG) &= ~(0x00001D00);

    //
    // 0b'1 in bit 5 for JTAG PADS
    // 0b'1 in bit 0 for all other IOs
    //
    HWREG(OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CMN_CONFIG) |= 0x00000023;

    //
    // Disable retention for GRP0
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_0 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_PAD_OEN_RET33_CONF);
      ulRegVal &= ~0x5;
      PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_PAD_OEN_RET33_CONF,ulRegVal);
    }

    //
    // Disable retention for GRP1
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_1 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG0);
      ulRegVal &= ~((0x3<<5));
      PRCMHIBRegWrite(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG0,ulRegVal);
    }

    //
    // Disable retention for GRP2
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_2 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF);
      ulRegVal &= ~0x00000101;
      PRCMHIBRegWrite(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF,ulRegVal);
      
    }

    //
    // Disable retention for GRP3
    //
    if( ulIORetGrpFlags & PRCM_IO_RET_GRP_3 )
    {
      ulRegVal  = PRCMHIBRegRead(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF);
      ulRegVal &= ~0x00000204;
      PRCMHIBRegWrite(HIB3P3_BASE  + HIB3P3_O_MEM_JTAG_CONF,ulRegVal);
    }

  }
}

//*****************************************************************************
//
//! Gets the device type
//!
//! This function returns bit-packed value representing the device type
//!
//! The returned value is logical OR of one or more of the following:-
//!
//! -\b PRCM_DEV_TYPE_FLAG_R        - R variant
//! -\b PRCM_DEV_TYPE_FLAG_F        - F variant
//! -\b PRCM_DEV_TYPE_FLAG_Z        - Z variant
//! -\b PRCM_DEV_TYPE_FLAG_SECURE   - Device is secure
//! -\b PRCM_DEV_TYPE_FLAG_PRE_PROD - Device is a pre-production part
//! -\b PRCM_DEV_TYPE_FLAG_3200     - Device is CC3200
//! -\b PRCM_DEV_TYPE_FLAG_3220     - Device is CC3220
//! -\b PRCM_DEV_TYPE_FLAG_REV1     - Device Rev 1
//! -\b PRCM_DEV_TYPE_FLAG_REV2     - Device Rev 2
//!
//! Pre-defined helper macros:-
//!
//! -\b PRCM_DEV_TYPE_PRE_CC3200R   - Pre-Production CC3200R
//! -\b PRCM_DEV_TYPE_PRE_CC3200F   - Pre-Production CC3200F
//! -\b PRCM_DEV_TYPE_PRE_CC3200Z   - Pre-Production CC3200Z
//! -\b PRCM_DEV_TYPE_CC3200R       - Production CC3200R
//! -\b PRCM_DEV_TYPE_PRE_CC3220R   - Pre-Production CC3220R
//! -\b PRCM_DEV_TYPE_PRE_CC3220F   - Pre-Production CC3220F
//! -\b PRCM_DEV_TYPE_PRE_CC3220Z   - Pre-Production CC3220Z
//! -\b PRCM_DEV_TYPE_CC3220R       - Production CC3220R
//! -\b PRCM_DEV_TYPE_PRE_CC3220RS  - Pre-Production CC3220RS
//! -\b PRCM_DEV_TYPE_PRE_CC3220FS  - Pre-Production CC3220FS
//! -\b PRCM_DEV_TYPE_PRE_CC3220ZS  - Pre-Production CC3220ZS
//! -\b PRCM_DEV_TYPE_CC3220RS      - Production CC3220RS
//! -\b PRCM_DEV_TYPE_CC3220FS      - Production CC3220FS
//!
//! \return  Returns, bit-packed value representing the device type,
//! or 0 if device is unknown
//
//*****************************************************************************
unsigned long PRCMDeviceTypeGet()
{
  unsigned long ulDevType;
  unsigned long ulChipId;
  unsigned long ulDevMajorVer;
  unsigned long ulDevMinorVer;

  //
  // Read the device identification register
  //
  ulChipId = HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2);

  //
  // Read the ROM mojor and minor version
  //
  ulDevMajorVer = ((ulChipId >> 28) & 0xF);
  ulDevMinorVer = ((ulChipId >> 24) & 0xF);

  
  ulChipId = ((HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2) >> 16) & 0x1F);
   
  //
  // Get the device variant from the chip id
  //
  switch((ulChipId & 0xF))
  {
    //
    // It is R variant
    //
    case 0x0:
      ulDevType = PRCM_DEV_TYPE_FLAG_R;
      break;

    //
    // It is F variant, non secure F variant is always Pre-Production
    //
    case 0x1:
      ulDevType = PRCM_DEV_TYPE_FLAG_F|PRCM_DEV_TYPE_FLAG_PRE_PROD;
      break;

    //
    // It is Z variant and is always Pre-Production
    //
    case 0x3:
      ulDevType = PRCM_DEV_TYPE_FLAG_Z|PRCM_DEV_TYPE_FLAG_PRE_PROD;
      break;

    //
    // It is Secure R
    //
    case 0x8:
      ulDevType = PRCM_DEV_TYPE_FLAG_R|PRCM_DEV_TYPE_FLAG_SECURE;
      break;

    //
    // It is Secure F
    //
    case 0x9:
      ulDevType = PRCM_DEV_TYPE_FLAG_F|PRCM_DEV_TYPE_FLAG_SECURE;
      break;

    //
    // It is secure Z variant and variant is always Pre-Production
    //
    case 0xB:
      ulDevType = PRCM_DEV_TYPE_FLAG_Z|PRCM_DEV_TYPE_FLAG_SECURE|
                  PRCM_DEV_TYPE_FLAG_PRE_PROD;
      break;

    //
    // Undefined variant
    //
    default:
      ulDevType = 0x0;
  }
  
  if( ulDevType != 0 )
  {
    if( ulDevMajorVer == 0x3 )
    {
      ulDevType |= PRCM_DEV_TYPE_FLAG_3220;
    }
    else if( ulDevMajorVer == 0x2 )
    {
      ulDevType  |= (PRCM_DEV_TYPE_FLAG_PRE_PROD|PRCM_DEV_TYPE_FLAG_3220);
      
      if( ((ulDevType & PRCM_DEV_TYPE_FLAG_Z) != 0) )
      {
        if((ulDevMinorVer == 0x0))
        {
          ulDevType |= PRCM_DEV_TYPE_FLAG_REV1; 
        }
        else
        {
          ulDevType |= PRCM_DEV_TYPE_FLAG_REV2;
        }
      }
      else
      {
        if((ulDevMinorVer == 0x1))
        {
          ulDevType |= PRCM_DEV_TYPE_FLAG_REV1;
        }
      }     
    }
    else
    {
      if( (ulDevMinorVer == 0x4))
      {
        if( ((ulDevType & PRCM_DEV_TYPE_FLAG_Z) != 0))
        {
          ulDevType |= (PRCM_DEV_TYPE_FLAG_PRE_PROD|PRCM_DEV_TYPE_FLAG_3220);
        }
        else
        {
          ulDevType |= PRCM_DEV_TYPE_FLAG_3200;
        }
      }
      else
      {
        ulDevType |= (PRCM_DEV_TYPE_FLAG_PRE_PROD|PRCM_DEV_TYPE_FLAG_3200);
      }
    }	  
  }
 

  return ulDevType;
}



//****************************************************************************
//
//! Used to trigger a hibernate cycle for the device using RTC
//!
//! This API can be used to do a clean reboot of device.
//!
//! \note This routine should only be exercised after all the network processing
//! has been stopped. To stop network processing use \b sl_stop API from
//! simplelink library.
//!
//! \return None
//
//****************************************************************************
void PRCMHibernateCycleTrigger()
{
  unsigned long ulRegValue;
  unsigned long long ullRTCVal;

  //
  // Read the RTC register
  //
  ulRegValue = PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN);

  //
  // Enable the RTC as wakeup source if specified
  //
  ulRegValue |= (PRCM_HIB_SLOW_CLK_CTR & 0x1);

  //
  // Enable HIB wakeup sources
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_EN,ulRegValue);

  //
  // Latch the RTC vlaue
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_READ ,0x1);

  //
  // Read latched values as 2 32-bit vlaues
  //
  ullRTCVal  = PRCMHIBRegRead(HIB3P3_BASE + HIB3P3_O_MEM_HIB_RTC_TIMER_MSW);
  ullRTCVal  = ullRTCVal << 32;
  ullRTCVal |= PRCMHIBRegRead(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_TIMER_LSW);

  //
  //Considering worst case execution times of ROM,RAM,Flash value of 160 is used
  //
  ullRTCVal = ullRTCVal + 160;

  //
  // Set RTC match value
  //
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_LSW_CONF,
                                            (unsigned long)(ullRTCVal));
  PRCMHIBRegWrite(HIB3P3_BASE+HIB3P3_O_MEM_HIB_RTC_WAKE_MSW_CONF,
                                           (unsigned long)(ullRTCVal>>32));
  //
  // Note : Any addition of code after this line would need a change in
  // ullTicks Interval currently set to 160
  //

  //
  // Request hibernate.
  //
  PRCMHIBRegWrite((HIB3P3_BASE+HIB3P3_O_MEM_HIB_REQ),0x1);

  //
  // Wait for system to enter hibernate
  //
  __asm("    wfi\n");

  //
  // Infinite loop
  //
  while(1)
  {

  }
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
