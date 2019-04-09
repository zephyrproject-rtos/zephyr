/******************************************************************************
*  Filename:       sys_ctrl.c
*  Revised:        2018-06-26 15:19:11 +0200 (Tue, 26 Jun 2018)
*  Revision:       52220
*
*  Description:    Driver for the System Control.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

// Hardware headers
#include "../inc/hw_types.h"
#include "../inc/hw_ccfg.h"
#include "../inc/hw_ioc.h"
// Driverlib headers
#include "aon_batmon.h"
#include "flash.h"
#include "gpio.h"
#include "setup_rom.h"
#include "sys_ctrl.h"


//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SysCtrlIdle
    #define SysCtrlIdle                     NOROM_SysCtrlIdle
    #undef  SysCtrlShutdownWithAbort
    #define SysCtrlShutdownWithAbort        NOROM_SysCtrlShutdownWithAbort
    #undef  SysCtrlShutdown
    #define SysCtrlShutdown                 NOROM_SysCtrlShutdown
    #undef  SysCtrlStandby
    #define SysCtrlStandby                  NOROM_SysCtrlStandby
    #undef  SysCtrlSetRechargeBeforePowerDown
    #define SysCtrlSetRechargeBeforePowerDown NOROM_SysCtrlSetRechargeBeforePowerDown
    #undef  SysCtrlAdjustRechargeAfterPowerDown
    #define SysCtrlAdjustRechargeAfterPowerDown NOROM_SysCtrlAdjustRechargeAfterPowerDown
    #undef  SysCtrl_DCDC_VoltageConditionalControl
    #define SysCtrl_DCDC_VoltageConditionalControl NOROM_SysCtrl_DCDC_VoltageConditionalControl
    #undef  SysCtrlResetSourceGet
    #define SysCtrlResetSourceGet           NOROM_SysCtrlResetSourceGet
#endif



//*****************************************************************************
//
// Force the system in to idle mode
//
//*****************************************************************************
void SysCtrlIdle(uint32_t vimsPdMode)
{
    // Configure the VIMS mode
    HWREG(PRCM_BASE + PRCM_O_PDCTL1VIMS) = vimsPdMode;

    // Always keep cache retention ON in IDLE
    PRCMCacheRetentionEnable();

    // Turn off the CPU power domain, will take effect when PRCMDeepSleep() executes
    PRCMPowerDomainOff(PRCM_DOMAIN_CPU);

    // Ensure any possible outstanding AON writes complete
    SysCtrlAonSync();

    // Invoke deep sleep to go to IDLE
    PRCMDeepSleep();
}

//*****************************************************************************
//
// Try to enter shutdown but abort if wakeup event happened before shutdown
//
//*****************************************************************************
void SysCtrlShutdownWithAbort(void)
{
    uint32_t wu_detect_vector = 0;
    uint32_t io_num = 0;

    // For all IO CFG registers check if wakeup detect is enabled
    for(io_num = 0; io_num < 32; io_num++)
    {
        // Read MSB from WU_CFG bit field
        if( HWREG(IOC_BASE + IOC_O_IOCFG0 + (io_num * 4) ) & (1 << (IOC_IOCFG0_WU_CFG_S + IOC_IOCFG0_WU_CFG_W - 1)) )
        {
            wu_detect_vector |= (1 << io_num);
        }
    }

    // Wakeup events are detected when pads are in sleep mode
    PowerCtrlPadSleepEnable();

    // Make sure all potential events have propagated before checking event flags
    SysCtrlAonUpdate();
    SysCtrlAonUpdate();

    // If no edge detect flags for wakeup enabled IOs are set then shut down the device
    if( GPIO_getEventMultiDio(wu_detect_vector) == 0 )
    {
        SysCtrlShutdown();
    }
    else
    {
        PowerCtrlPadSleepDisable();
    }
}

//*****************************************************************************
//
// Force the system into shutdown mode
//
//*****************************************************************************
void SysCtrlShutdown(void)
{
    // Request shutdown mode
    HWREG(AON_PMCTL_BASE + AON_PMCTL_O_SHUTDOWN) = AON_PMCTL_SHUTDOWN_EN;

    // Make sure System CPU does not continue beyond this point.
    // Shutdown happens when all shutdown conditions are met.
    while(1);
}

//*****************************************************************************
//
// Force the system in to standby mode
//
//*****************************************************************************
void SysCtrlStandby(bool retainCache, uint32_t vimsPdMode, uint32_t rechargeMode)
{
    uint32_t modeVIMS;

    // Freeze the IOs on the boundary between MCU and AON
    AONIOCFreezeEnable();

    // Ensure any possible outstanding AON writes complete before turning off the power domains
    SysCtrlAonSync();

    // Request power off of domains in the MCU voltage domain
    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE | PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH | PRCM_DOMAIN_CPU);

    // Ensure that no clocks are forced on in any modes for Crypto, DMA and I2S
    HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR) &= (~PRCM_SECDMACLKGR_CRYPTO_AM_CLK_EN & ~PRCM_SECDMACLKGR_DMA_AM_CLK_EN);
    HWREG(PRCM_BASE + PRCM_O_I2SCLKGR)    &= ~PRCM_I2SCLKGR_AM_CLK_EN;

    // Gate running deep sleep clocks for Crypto, DMA and I2S
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_CRYPTO);
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_UDMA);
    PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_I2S);

    // Load the new clock settings
    PRCMLoadSet();

    // Configure the VIMS power domain mode
    HWREG(PRCM_BASE + PRCM_O_PDCTL1VIMS) = vimsPdMode;

    // Request uLDO during standby
    PRCMMcuUldoConfigure(1);

   // Check the regulator mode
   if (HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL) & AON_PMCTL_PWRCTL_EXT_REG_MODE)
   {
       // In external regulator mode the recharge functionality is disabled
       HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RECHARGECFG) = 0x00000000;
   }
   else
   {
       // In internal regulator mode the recharge functionality is set up with
       // adaptive recharge mode and fixed parameter values
       if(rechargeMode == SYSCTRL_PREFERRED_RECHARGE_MODE)
       {
           // Enable the Recharge Comparator
           HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RECHARGECFG) = AON_PMCTL_RECHARGECFG_MODE_COMPARATOR;
       }
       else
       {
           // Set requested recharge mode
           HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RECHARGECFG) = rechargeMode;
       }
   }

    // Ensure all writes have taken effect
    SysCtrlAonSync();

    // Ensure UDMA, Crypto and I2C clocks are turned off
    while (!PRCMLoadGet()) {;}

    // Ensure power domains have been turned off.
    // CPU power domain will power down when PRCMDeepSleep() executes.
    while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE | PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_OFF) {;}

    // Turn off cache retention if requested
    if (retainCache == false) {

        // Get the current VIMS mode
        do {
            modeVIMS = VIMSModeGet(VIMS_BASE);
        } while (modeVIMS == VIMS_MODE_CHANGING);

        // If in a cache mode, turn VIMS off
        if (modeVIMS == VIMS_MODE_ENABLED) {
           VIMSModeSet(VIMS_BASE, VIMS_MODE_OFF);
        }

        // Disable retention of cache RAM
        PRCMCacheRetentionDisable();
    }

    // Invoke deep sleep to go to STANDBY
    PRCMDeepSleep();
}

//*****************************************************************************
//
// SysCtrlSetRechargeBeforePowerDown( xoscPowerMode )
//
//*****************************************************************************
void
SysCtrlSetRechargeBeforePowerDown( uint32_t xoscPowerMode )
{
   uint32_t          ccfg_ModeConfReg        ;

   // read the MODE_CONF register in CCFG
   ccfg_ModeConfReg = HWREG( CCFG_BASE + CCFG_O_MODE_CONF );
   // Do temperature compensation if enabled
   if (( ccfg_ModeConfReg & CCFG_MODE_CONF_VDDR_TRIM_SLEEP_TC ) == 0 ) {
      int32_t vddrSleepDelta  ;
      int32_t curTemp         ;
      int32_t tcDelta         ;
      int32_t vddrSleepTrim   ;

      // Get VDDR_TRIM_SLEEP_DELTA + 1 (sign extended) ==> vddrSleepDelta = -7..+8
      vddrSleepDelta = (((int32_t)( ccfg_ModeConfReg << ( 32 - CCFG_MODE_CONF_VDDR_TRIM_SLEEP_DELTA_W - CCFG_MODE_CONF_VDDR_TRIM_SLEEP_DELTA_S )))
                                                     >> ( 32 - CCFG_MODE_CONF_VDDR_TRIM_SLEEP_DELTA_W )) + 1 ;
      curTemp = AONBatMonTemperatureGetDegC();
      tcDelta = ( 62 - curTemp ) >> 3;
      if ( tcDelta > 7 ) {
         tcDelta = 7 ;
      }
      if ( tcDelta > vddrSleepDelta ) {
         vddrSleepDelta = tcDelta ;
      }
      vddrSleepTrim = (( HWREG( FLASH_CFG_BASE + FCFG1_OFFSET + FCFG1_O_MISC_TRIM ) & FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_REFLEVEL_M ) >>
                                                                                      FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_REFLEVEL_S ) ;
      vddrSleepTrim -= vddrSleepDelta ;
      if ( vddrSleepTrim >  15 ) vddrSleepTrim =  15 ;
      if ( vddrSleepTrim <   1 ) vddrSleepTrim =   1 ;
      // Write adjusted value using MASKED write (MASK8)
      HWREGB( ADI3_BASE + ADI_O_MASK4B + ( ADI_3_REFSYS_O_CTL_RECHARGE_CMP0 * 2 )) = (( ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_M << 4 ) |
        (( vddrSleepTrim << ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_S ) & ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_M )   );
      // Make a dummy read in order to make sure the write above is done before going into standby
      HWREGB( ADI3_BASE + ADI_3_REFSYS_O_CTL_RECHARGE_CMP0 );
   }
}


//*****************************************************************************
//
// SysCtrlAdjustRechargeAfterPowerDown()
//
//*****************************************************************************
void
SysCtrlAdjustRechargeAfterPowerDown( uint32_t vddrRechargeMargin )
{
   // Nothing to be done but keeping this function for platform compatibility.
}


//*****************************************************************************
//
// SysCtrl_DCDC_VoltageConditionalControl()
//
//*****************************************************************************
void
SysCtrl_DCDC_VoltageConditionalControl( void )
{
   uint32_t batThreshold     ;  // Fractional format with 8 fractional bits.
   uint32_t aonBatmonBat     ;  // Fractional format with 8 fractional bits.
   uint32_t ccfg_ModeConfReg ;  // Holds a copy of the CCFG_O_MODE_CONF register.
   uint32_t aonPmctlPwrctl   ;  // Reflect whats read/written to the AON_PMCTL_O_PWRCTL register.

   // We could potentially call this function before any battery voltage measurement
   // is made/available. In that case we must make sure that we do not turn off the DCDC.
   // This can be done by doing nothing as long as the battery voltage is 0 (Since the
   // reset value of the battery voltage register is 0).
   aonBatmonBat = HWREG( AON_BATMON_BASE + AON_BATMON_O_BAT );
   if ( aonBatmonBat != 0 ) {
      // Check if Voltage Conditional Control is enabled
      // It is enabled if all the following are true:
      // - DCDC in use (either in active or recharge mode), (in use if one of the corresponding CCFG bits are zero).
      // - Alternative DCDC settings are enabled ( DIS_ALT_DCDC_SETTING == 0 )
      // - Not in external regulator mode ( EXT_REG_MODE == 0 )
      ccfg_ModeConfReg = HWREG( CCFG_BASE + CCFG_O_MODE_CONF );

      if (((( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_RECHARGE_M ) == 0                                            ) ||
           (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_ACTIVE_M   ) == 0                                            )    ) &&
          (( HWREG( AON_PMCTL_BASE  + AON_PMCTL_O_PWRCTL  ) & AON_PMCTL_PWRCTL_EXT_REG_MODE  )               == 0      ) &&
          (( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_ALT_DCDC_SETTING ) == 0      )    )
      {
         aonPmctlPwrctl = HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL );
         batThreshold   = (((( HWREG( CCFG_BASE + CCFG_O_MODE_CONF_1 ) &
            CCFG_MODE_CONF_1_ALT_DCDC_VMIN_M ) >>
            CCFG_MODE_CONF_1_ALT_DCDC_VMIN_S ) + 28 ) << 4 );

         if ( aonPmctlPwrctl & ( AON_PMCTL_PWRCTL_DCDC_EN_M | AON_PMCTL_PWRCTL_DCDC_ACTIVE_M )) {
            // DCDC is ON, check if it should be switched off
            if ( aonBatmonBat < batThreshold ) {
               aonPmctlPwrctl &= ~( AON_PMCTL_PWRCTL_DCDC_EN_M | AON_PMCTL_PWRCTL_DCDC_ACTIVE_M );

               HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL ) = aonPmctlPwrctl;
            }
         } else {
            // DCDC is OFF, check if it should be switched on
            if ( aonBatmonBat > batThreshold ) {
               if (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_RECHARGE_M ) == 0 ) aonPmctlPwrctl |= AON_PMCTL_PWRCTL_DCDC_EN_M     ;
               if (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_ACTIVE_M   ) == 0 ) aonPmctlPwrctl |= AON_PMCTL_PWRCTL_DCDC_ACTIVE_M ;

               HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL ) = aonPmctlPwrctl;
            }
         }
      }
   }
}


//*****************************************************************************
//
// SysCtrlResetSourceGet()
//
//*****************************************************************************
uint32_t
SysCtrlResetSourceGet( void )
{
   uint32_t aonPmctlResetCtl = HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL );

   if ( aonPmctlResetCtl & AON_PMCTL_RESETCTL_WU_FROM_SD_M ) {
      if ( aonPmctlResetCtl & AON_PMCTL_RESETCTL_GPIO_WU_FROM_SD_M ) {
         return ( RSTSRC_WAKEUP_FROM_SHUTDOWN );
      } else {
         return ( RSTSRC_WAKEUP_FROM_TCK_NOISE );
      }
   } else {
      return (( aonPmctlResetCtl & AON_PMCTL_RESETCTL_RESET_SRC_M ) >> AON_PMCTL_RESETCTL_RESET_SRC_S );
   }
}
