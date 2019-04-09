/******************************************************************************
*  Filename:       setup.c
*  Revised:        2018-11-06 15:08:57 +0100 (Tue, 06 Nov 2018)
*  Revision:       53239
*
*  Description:    Setup file for CC13xx/CC26xx devices.
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
#include "../inc/hw_memmap.h"
#include "../inc/hw_adi.h"
#include "../inc/hw_adi_2_refsys.h"
#include "../inc/hw_adi_3_refsys.h"
#include "../inc/hw_adi_4_aux.h"
// Temporarily adding these defines as they are missing in hw_adi_4_aux.h
#define ADI_4_AUX_O_LPMBIAS                                         0x0000000E
#define ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_M                           0x0000003F
#define ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_S                                    0
#define ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_M                        0x00000038
#define ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_S                                 3
#include "../inc/hw_aon_ioc.h"
#include "../inc/hw_aon_pmctl.h"
#include "../inc/hw_aon_rtc.h"
#include "../inc/hw_ddi_0_osc.h"
#include "../inc/hw_ddi.h"
#include "../inc/hw_ccfg.h"
#include "../inc/hw_fcfg1.h"
#include "../inc/hw_flash.h"
#include "../inc/hw_prcm.h"
#include "../inc/hw_vims.h"
// Driverlib headers
#include "aux_sysif.h"
#include "chipinfo.h"
#include "setup.h"
#include "setup_rom.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SetupTrimDevice
    #define SetupTrimDevice                 NOROM_SetupTrimDevice
#endif



//*****************************************************************************
//
// Defined CPU delay macro with microseconds as input
// Quick check shows: (To be further investigated)
// At 48 MHz RCOSC and VIMS.CONTROL.PREFETCH = 0, there is 5 cycles
// At 48 MHz RCOSC and VIMS.CONTROL.PREFETCH = 1, there is 4 cycles
// At 24 MHz RCOSC and VIMS.CONTROL.PREFETCH = 0, there is 3 cycles
//
//*****************************************************************************
#define CPU_DELAY_MICRO_SECONDS( x ) \
   CPUdelay(((uint32_t)((( x ) * 48.0 ) / 5.0 )) - 1 )


//*****************************************************************************
//
// Function declarations
//
//*****************************************************************************
static void     TrimAfterColdReset( void );
static void     TrimAfterColdResetWakeupFromShutDown( uint32_t ui32Fcfg1Revision );
static void     TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown( void );

//*****************************************************************************
//
// Perform the necessary trim of the device which is not done in boot code
//
// This function should only execute coming from ROM boot. The current
// implementation does not take soft reset into account. However, it does no
// damage to execute it again. It only consumes time.
//
//*****************************************************************************
void
SetupTrimDevice(void)
{
    uint32_t ui32Fcfg1Revision;
    uint32_t ui32AonSysResetctl;

    // Get layout revision of the factory configuration area
    // (Handle undefined revision as revision = 0)
    ui32Fcfg1Revision = HWREG(FCFG1_BASE + FCFG1_O_FCFG1_REVISION);
    if ( ui32Fcfg1Revision == 0xFFFFFFFF ) {
        ui32Fcfg1Revision = 0;
    }

    // This driverlib version and setup file is for the CC13x2, CC26x2 PG2.0 or later chips.
    // Halt if violated
    ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated();

    // Enable standby in flash bank
    HWREGBITW( FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_STANDBY_BITN ) = 0;

    // Select correct CACHE mode and set correct CACHE configuration
#if ( CCFG_BASE == CCFG_BASE_DEFAULT )
    SetupSetCacheModeAccordingToCcfgSetting();
#else
    NOROM_SetupSetCacheModeAccordingToCcfgSetting();
#endif

    // 1. Check for powerdown
    // 2. Check for shutdown
    // 3. Assume cold reset if none of the above.
    //
    // It is always assumed that the application will freeze the latches in
    // AON_IOC when going to powerdown in order to retain the values on the IOs.
    //
    // NB. If this bit is not cleared before proceeding to powerdown, the IOs
    //     will all default to the reset configuration when restarting.
    if( ! ( HWREGBITW( AON_IOC_BASE + AON_IOC_O_IOCLATCH, AON_IOC_IOCLATCH_EN_BITN )))
    {
        // NB. This should be calling a ROM implementation of required trim and
        // compensation
        // e.g. TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown()
        TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown();
    }
    // Check for shutdown
    //
    // When device is going to shutdown the hardware will automatically clear
    // the SLEEPDIS bit in the SLEEP register in the AON_PMCTL module.
    // It is left for the application to assert this bit when waking back up,
    // but not before the desired IO configuration has been re-established.
    else if( ! ( HWREGBITW( AON_PMCTL_BASE + AON_PMCTL_O_SLEEPCTL, AON_PMCTL_SLEEPCTL_IO_PAD_SLEEP_DIS_BITN )))
    {
        // NB. This should be calling a ROM implementation of required trim and
        // compensation
        // e.g. TrimAfterColdResetWakeupFromShutDown()    -->
        //      TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown();
        TrimAfterColdResetWakeupFromShutDown(ui32Fcfg1Revision);
        TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown();
    }
    else
    {
        // Consider adding a check for soft reset to allow debugging to skip
        // this section!!!
        //
        // NB. This should be calling a ROM implementation of required trim and
        // compensation
        // e.g. TrimAfterColdReset()   -->
        //      TrimAfterColdResetWakeupFromShutDown()    -->
        //      TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown()
        TrimAfterColdReset();
        TrimAfterColdResetWakeupFromShutDown(ui32Fcfg1Revision);
        TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown();

    }

    // Set VIMS power domain control.
    // PDCTL1VIMS = 0 ==> VIMS power domain is only powered when CPU power domain is powered
    HWREG( PRCM_BASE + PRCM_O_PDCTL1VIMS ) = 0;

    // Configure optimal wait time for flash FSM in cases where flash pump
    // wakes up from sleep
    HWREG(FLASH_BASE + FLASH_O_FPAC1) = (HWREG(FLASH_BASE + FLASH_O_FPAC1) &
                                         ~FLASH_FPAC1_PSLEEPTDIS_M) |
                                        (0x139<<FLASH_FPAC1_PSLEEPTDIS_S);

    // And finally at the end of the flash boot process:
    // SET BOOT_DET bits in AON_PMCTL to 3 if already found to be 1
    // Note: The BOOT_DET_x_CLR/SET bits must be manually cleared
    if ((( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) &
        ( AON_PMCTL_RESETCTL_BOOT_DET_1_M | AON_PMCTL_RESETCTL_BOOT_DET_0_M )) >>
        AON_PMCTL_RESETCTL_BOOT_DET_0_S ) == 1 )
    {
        ui32AonSysResetctl = ( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) &
        ~( AON_PMCTL_RESETCTL_BOOT_DET_1_CLR_M | AON_PMCTL_RESETCTL_BOOT_DET_0_CLR_M |
           AON_PMCTL_RESETCTL_BOOT_DET_1_SET_M | AON_PMCTL_RESETCTL_BOOT_DET_0_SET_M | AON_PMCTL_RESETCTL_MCU_WARM_RESET_M ));
        HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) = ui32AonSysResetctl | AON_PMCTL_RESETCTL_BOOT_DET_1_SET_M;
        HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) = ui32AonSysResetctl;
    }

    // Make sure there are no ongoing VIMS mode change when leaving SetupTrimDevice()
    // (There should typically be no wait time here, but need to be sure)
    while ( HWREGBITW( VIMS_BASE + VIMS_O_STAT, VIMS_STAT_MODE_CHANGING_BITN )) {
        // Do nothing - wait for an eventual ongoing mode change to complete.
    }
}

//*****************************************************************************
//
//! \brief Trims to be applied when coming from POWER_DOWN (also called when
//! coming from SHUTDOWN and PIN_RESET).
//!
//! \return None
//
//*****************************************************************************
static void
TrimAfterColdResetWakeupFromShutDownWakeupFromPowerDown( void )
{
    // Currently no specific trim for Powerdown
}

//*****************************************************************************
//
//! \brief Trims to be applied when coming from SHUTDOWN (also called when
//! coming from PIN_RESET).
//!
//! \param ui32Fcfg1Revision
//!
//! \return None
//
//*****************************************************************************
static void
TrimAfterColdResetWakeupFromShutDown(uint32_t ui32Fcfg1Revision)
{
    uint32_t   ccfg_ModeConfReg  ;

    // Check in CCFG for alternative DCDC setting
    if (( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_ALT_DCDC_SETTING ) == 0 ) {
        // ADI_3_REFSYS:DCDCCTL5[3]  (=DITHER_EN) = CCFG_MODE_CONF_1[19]   (=ALT_DCDC_DITHER_EN)
        // ADI_3_REFSYS:DCDCCTL5[2:0](=IPEAK    ) = CCFG_MODE_CONF_1[18:16](=ALT_DCDC_IPEAK    )
        // Using a single 4-bit masked write since layout is equal for both source and destination
        HWREGB( ADI3_BASE + ADI_O_MASK4B + ( ADI_3_REFSYS_O_DCDCCTL5 * 2 )) = ( 0xF0 |
            ( HWREG( CCFG_BASE + CCFG_O_MODE_CONF_1 ) >> CCFG_MODE_CONF_1_ALT_DCDC_IPEAK_S ));

    }

    // TBD - Temporarily removed for CC13x2 / CC26x2

        // Force DCDC to use RCOSC before starting up XOSC.
        // Clock loss detector does not use XOSC until SCLK_HF actually switches
        // and thus DCDC is not protected from clock loss on XOSC in that time frame.
        // The force must be released when the switch to XOSC has happened. This is done
        // in OSCHfSourceSwitch().
        HWREG(AUX_DDI0_OSC_BASE + DDI_O_MASK16B + (DDI_0_OSC_O_CTL0 << 1) + 4) = DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M | (DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M >> 16);
        // Dummy read to ensure that the write has propagated
        HWREGH(AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0);

    // read the MODE_CONF register in CCFG
    ccfg_ModeConfReg = HWREG( CCFG_BASE + CCFG_O_MODE_CONF );

    // First part of trim done after cold reset and wakeup from shutdown:
    // -Adjust the VDDR_TRIM_SLEEP value.
    // -Configure DCDC.
    SetupAfterColdResetWakeupFromShutDownCfg1( ccfg_ModeConfReg );

    // Addition to the CC1352 boost mode for HWREV >= 2.0
    // The combination VDDR_EXT_LOAD=0 and VDDS_BOD_LEVEL=1 is defined to select boost mode
    if ((( ccfg_ModeConfReg & CCFG_MODE_CONF_VDDR_EXT_LOAD  ) == 0 ) &&
        (( ccfg_ModeConfReg & CCFG_MODE_CONF_VDDS_BOD_LEVEL ) != 0 )    )
    {
        HWREGB( ADI3_BASE + ADI_3_REFSYS_O_DCDCCTL3 ) = ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_BOOST ;
    }

    // Second part of trim done after cold reset and wakeup from shutdown:
    // -Configure XOSC.
#if ( CCFG_BASE == CCFG_BASE_DEFAULT )
    SetupAfterColdResetWakeupFromShutDownCfg2( ui32Fcfg1Revision, ccfg_ModeConfReg );
#else
    NOROM_SetupAfterColdResetWakeupFromShutDownCfg2( ui32Fcfg1Revision, ccfg_ModeConfReg );
#endif

    {
        uint32_t  trimReg        ;
        uint32_t  ui32TrimValue  ;

        //--- Propagate the LPM_BIAS trim ---
        trimReg = HWREG( FCFG1_BASE + FCFG1_O_DAC_BIAS_CNF );
        ui32TrimValue = (( trimReg & FCFG1_DAC_BIAS_CNF_LPM_TRIM_IOUT_M ) >>
                                     FCFG1_DAC_BIAS_CNF_LPM_TRIM_IOUT_S ) ;
        HWREGB( AUX_ADI4_BASE + ADI_4_AUX_O_LPMBIAS ) = (( ui32TrimValue << ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_S ) &
                                                                            ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_M ) ;
        // Set LPM_BIAS_BACKUP_EN according to FCFG1 configuration
        if ( trimReg & FCFG1_DAC_BIAS_CNF_LPM_BIAS_BACKUP_EN ) {
            HWREGB( ADI3_BASE + ADI_O_SET + ADI_3_REFSYS_O_AUX_DEBUG ) = ADI_3_REFSYS_AUX_DEBUG_LPM_BIAS_BACKUP_EN;
        } else {
            HWREGB( ADI3_BASE + ADI_O_CLR + ADI_3_REFSYS_O_AUX_DEBUG ) = ADI_3_REFSYS_AUX_DEBUG_LPM_BIAS_BACKUP_EN;
        }
        // Set LPM_BIAS_WIDTH_TRIM according to FCFG1 configuration
        {
            uint32_t widthTrim = (( trimReg & FCFG1_DAC_BIAS_CNF_LPM_BIAS_WIDTH_TRIM_M ) >> FCFG1_DAC_BIAS_CNF_LPM_BIAS_WIDTH_TRIM_S );
            HWREGH( AUX_ADI4_BASE + ADI_O_MASK8B + ( ADI_4_AUX_O_COMP * 2 )) = // Set LPM_BIAS_WIDTH_TRIM = 3
                (( ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_M << 8         ) |       // Set mask (bits to be written) in [15:8]
                 ( widthTrim << ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_S )   );    // Set value (in correct bit pos) in [7:0]
        }
    }

    // Third part of trim done after cold reset and wakeup from shutdown:
    // -Configure HPOSC.
    // -Setup the LF clock.
#if ( CCFG_BASE == CCFG_BASE_DEFAULT )
    SetupAfterColdResetWakeupFromShutDownCfg3( ccfg_ModeConfReg );
#else
    NOROM_SetupAfterColdResetWakeupFromShutDownCfg3( ccfg_ModeConfReg );
#endif

    // Set AUX into power down active mode
    AUXSYSIFOpModeChange( AUX_SYSIF_OPMODE_TARGET_PDA );

    // Disable EFUSE clock
    HWREGBITW( FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_EFUSECLK_BITN ) = 1;
}


//*****************************************************************************
//
//! \brief Trims to be applied when coming from PIN_RESET.
//!
//! \return None
//
//*****************************************************************************
static void
TrimAfterColdReset( void )
{
    // Currently no specific trim for Cold Reset
}
