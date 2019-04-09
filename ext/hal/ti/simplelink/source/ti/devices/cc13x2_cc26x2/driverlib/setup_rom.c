/******************************************************************************
*  Filename:       setup_rom.c
*  Revised:        2017-11-02 11:31:15 +0100 (Thu, 02 Nov 2017)
*  Revision:       50143
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
#include "../inc/hw_aon_batmon.h"
#include "../inc/hw_aux_sysif.h"
#include "../inc/hw_ccfg.h"
#include "../inc/hw_ddi_0_osc.h"
#include "../inc/hw_fcfg1.h"
// Driverlib headers
#include "ddi.h"
#include "ioc.h"
#include "osc.h"
#include "sys_ctrl.h"
#include "setup_rom.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SetupAfterColdResetWakeupFromShutDownCfg1
    #define SetupAfterColdResetWakeupFromShutDownCfg1 NOROM_SetupAfterColdResetWakeupFromShutDownCfg1
    #undef  SetupAfterColdResetWakeupFromShutDownCfg2
    #define SetupAfterColdResetWakeupFromShutDownCfg2 NOROM_SetupAfterColdResetWakeupFromShutDownCfg2
    #undef  SetupAfterColdResetWakeupFromShutDownCfg3
    #define SetupAfterColdResetWakeupFromShutDownCfg3 NOROM_SetupAfterColdResetWakeupFromShutDownCfg3
    #undef  SetupGetTrimForAdcShModeEn
    #define SetupGetTrimForAdcShModeEn      NOROM_SetupGetTrimForAdcShModeEn
    #undef  SetupGetTrimForAdcShVbufEn
    #define SetupGetTrimForAdcShVbufEn      NOROM_SetupGetTrimForAdcShVbufEn
    #undef  SetupGetTrimForAmpcompCtrl
    #define SetupGetTrimForAmpcompCtrl      NOROM_SetupGetTrimForAmpcompCtrl
    #undef  SetupGetTrimForAmpcompTh1
    #define SetupGetTrimForAmpcompTh1       NOROM_SetupGetTrimForAmpcompTh1
    #undef  SetupGetTrimForAmpcompTh2
    #define SetupGetTrimForAmpcompTh2       NOROM_SetupGetTrimForAmpcompTh2
    #undef  SetupGetTrimForAnabypassValue1
    #define SetupGetTrimForAnabypassValue1  NOROM_SetupGetTrimForAnabypassValue1
    #undef  SetupGetTrimForDblrLoopFilterResetVoltage
    #define SetupGetTrimForDblrLoopFilterResetVoltage NOROM_SetupGetTrimForDblrLoopFilterResetVoltage
    #undef  SetupGetTrimForRadcExtCfg
    #define SetupGetTrimForRadcExtCfg       NOROM_SetupGetTrimForRadcExtCfg
    #undef  SetupGetTrimForRcOscLfIBiasTrim
    #define SetupGetTrimForRcOscLfIBiasTrim NOROM_SetupGetTrimForRcOscLfIBiasTrim
    #undef  SetupGetTrimForRcOscLfRtuneCtuneTrim
    #define SetupGetTrimForRcOscLfRtuneCtuneTrim NOROM_SetupGetTrimForRcOscLfRtuneCtuneTrim
    #undef  SetupGetTrimForXoscHfCtl
    #define SetupGetTrimForXoscHfCtl        NOROM_SetupGetTrimForXoscHfCtl
    #undef  SetupGetTrimForXoscHfFastStart
    #define SetupGetTrimForXoscHfFastStart  NOROM_SetupGetTrimForXoscHfFastStart
    #undef  SetupGetTrimForXoscHfIbiastherm
    #define SetupGetTrimForXoscHfIbiastherm NOROM_SetupGetTrimForXoscHfIbiastherm
    #undef  SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
    #define SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio NOROM_SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
    #undef  SetupSetCacheModeAccordingToCcfgSetting
    #define SetupSetCacheModeAccordingToCcfgSetting NOROM_SetupSetCacheModeAccordingToCcfgSetting
    #undef  SetupSetAonRtcSubSecInc
    #define SetupSetAonRtcSubSecInc         NOROM_SetupSetAonRtcSubSecInc
    #undef  SetupStepVddrTrimTo
    #define SetupStepVddrTrimTo             NOROM_SetupStepVddrTrimTo
#endif

//*****************************************************************************
//
// Function declarations
//
//*****************************************************************************

//*****************************************************************************
//
// SetupStepVddrTrimTo
//
//*****************************************************************************
void
SetupStepVddrTrimTo( uint32_t toCode )
{
    uint32_t    pmctlResetctl_reg   ;
    int32_t     targetTrim          ;
    int32_t     currentTrim         ;

    targetTrim  = SetupSignExtendVddrTrimValue( toCode & ( ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_M >> ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_S ));
    currentTrim = SetupSignExtendVddrTrimValue((
        HWREGB( ADI3_BASE + ADI_3_REFSYS_O_DCDCCTL0 ) &
        ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_M ) >>
        ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_S ) ;

    if ( targetTrim != currentTrim ) {
        pmctlResetctl_reg = ( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) & ~AON_PMCTL_RESETCTL_MCU_WARM_RESET_M );
        if ( pmctlResetctl_reg & AON_PMCTL_RESETCTL_VDDR_LOSS_EN_M ) {
            HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) = ( pmctlResetctl_reg & ~AON_PMCTL_RESETCTL_VDDR_LOSS_EN_M );
            HWREG( AON_RTC_BASE + AON_RTC_O_SYNC );      // Wait for VDDR_LOSS_EN setting to propagate
        }

        while ( targetTrim != currentTrim ) {
            HWREG( AON_RTC_BASE + AON_RTC_O_SYNCLF );    // Wait for next edge on SCLK_LF (positive or negative)

            if ( targetTrim > currentTrim )  currentTrim++;
            else                             currentTrim--;

            HWREGB( ADI3_BASE + ADI_3_REFSYS_O_DCDCCTL0 ) = (
                ( HWREGB( ADI3_BASE + ADI_3_REFSYS_O_DCDCCTL0 ) & ~ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_M ) |
                ((((uint32_t)currentTrim) << ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_S ) &
                                             ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_M ) );
        }

        HWREG( AON_RTC_BASE + AON_RTC_O_SYNCLF );        // Wait for next edge on SCLK_LF (positive or negative)

        if ( pmctlResetctl_reg & AON_PMCTL_RESETCTL_VDDR_LOSS_EN_M ) {
            HWREG( AON_RTC_BASE + AON_RTC_O_SYNCLF );    // Wait for next edge on SCLK_LF (positive or negative)
            HWREG( AON_RTC_BASE + AON_RTC_O_SYNCLF );    // Wait for next edge on SCLK_LF (positive or negative)
            HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) = pmctlResetctl_reg;
            HWREG( AON_RTC_BASE + AON_RTC_O_SYNC );      // And finally wait for VDDR_LOSS_EN setting to propagate
        }
    }
}

//*****************************************************************************
//
// SetupAfterColdResetWakeupFromShutDownCfg1
//
//*****************************************************************************
void
SetupAfterColdResetWakeupFromShutDownCfg1( uint32_t ccfg_ModeConfReg )
{
    // Check for CC1352 boost mode
    // The combination VDDR_EXT_LOAD=0 and VDDS_BOD_LEVEL=1 is defined to select boost mode
    if ((( ccfg_ModeConfReg & CCFG_MODE_CONF_VDDR_EXT_LOAD  ) == 0 ) &&
        (( ccfg_ModeConfReg & CCFG_MODE_CONF_VDDS_BOD_LEVEL ) != 0 )    )
    {
        // Set VDDS_BOD trim - using masked write {MASK8:DATA8}
        // - TRIM_VDDS_BOD is bits[7:3] of ADI3..REFSYSCTL1
        // - Needs a positive transition on BOD_BG_TRIM_EN (bit[7] of REFSYSCTL3) to
        //   latch new VDDS BOD. Set to 0 first to guarantee a positive transition.
        HWREGB( ADI3_BASE + ADI_O_CLR + ADI_3_REFSYS_O_REFSYSCTL3 ) = ADI_3_REFSYS_REFSYSCTL3_BOD_BG_TRIM_EN;
        //
        // VDDS_BOD_LEVEL = 1 means that boost mode is selected
        // - Max out the VDDS_BOD trim (=VDDS_BOD_POS_31)
        HWREGH( ADI3_BASE + ADI_O_MASK8B + ( ADI_3_REFSYS_O_REFSYSCTL1 * 2 )) =
            ( ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_M << 8 ) |
            ( ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_31 ) ;
        HWREGB( ADI3_BASE + ADI_O_SET + ADI_3_REFSYS_O_REFSYSCTL3 ) = ADI_3_REFSYS_REFSYSCTL3_BOD_BG_TRIM_EN;

        SetupStepVddrTrimTo(( HWREG( FCFG1_BASE + FCFG1_O_VOLT_TRIM ) &
            FCFG1_VOLT_TRIM_VDDR_TRIM_HH_M ) >>
            FCFG1_VOLT_TRIM_VDDR_TRIM_HH_S ) ;
    }

    // 1.
    // Do not allow DCDC to be enabled if in external regulator mode.
    // Preventing this by setting both the RECHARGE and the ACTIVE bits bit in the CCFG_MODE_CONF copy register (ccfg_ModeConfReg).
    //
    // 2.
    // Adjusted battery monitor low limit in internal regulator mode.
    // This is done by setting AON_BATMON_FLASHPUMPP0_LOWLIM=0 in internal regulator mode.
    if ( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL ) & AON_PMCTL_PWRCTL_EXT_REG_MODE ) {
        ccfg_ModeConfReg |= ( CCFG_MODE_CONF_DCDC_RECHARGE_M | CCFG_MODE_CONF_DCDC_ACTIVE_M );
    } else {
        HWREGBITW( AON_BATMON_BASE + AON_BATMON_O_FLASHPUMPP0, AON_BATMON_FLASHPUMPP0_LOWLIM_BITN ) = 0;
    }

    // set the RECHARGE source based upon CCFG:MODE_CONF:DCDC_RECHARGE
    // Note: Inverse polarity
    HWREGBITW( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL, AON_PMCTL_PWRCTL_DCDC_EN_BITN ) =
        ((( ccfg_ModeConfReg >> CCFG_MODE_CONF_DCDC_RECHARGE_S ) & 1 ) ^ 1 );

    // set the ACTIVE source based upon CCFG:MODE_CONF:DCDC_ACTIVE
    // Note: Inverse polarity
    HWREGBITW( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL, AON_PMCTL_PWRCTL_DCDC_ACTIVE_BITN ) =
        ((( ccfg_ModeConfReg >> CCFG_MODE_CONF_DCDC_ACTIVE_S ) & 1 ) ^ 1 );
}

//*****************************************************************************
//
// SetupAfterColdResetWakeupFromShutDownCfg2
//
//*****************************************************************************
void
SetupAfterColdResetWakeupFromShutDownCfg2( uint32_t ui32Fcfg1Revision, uint32_t ccfg_ModeConfReg )
{
    uint32_t   ui32Trim;

    // Following sequence is required for using XOSCHF, if not included
    // devices crashes when trying to switch to XOSCHF.
    //
    // Trim CAP settings. Get and set trim value for the ANABYPASS_VALUE1
    // register
    ui32Trim = SetupGetTrimForAnabypassValue1( ccfg_ModeConfReg );
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_ANABYPASSVAL1, ui32Trim);

    // Trim RCOSC_LF. Get and set trim values for the RCOSCLF_RTUNE_TRIM and
    // RCOSCLF_CTUNE_TRIM fields in the XOSCLF_RCOSCLF_CTRL register.
    ui32Trim = SetupGetTrimForRcOscLfRtuneCtuneTrim();
    DDI16BitfieldWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_LFOSCCTL,
                       (DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_M |
                        DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_M),
                       DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_S,
                       ui32Trim);

    // Trim XOSCHF IBIAS THERM. Get and set trim value for the
    // XOSCHF IBIAS THERM bit field in the ANABYPASS_VALUE2 register. Other
    // register bit fields are set to 0.
    ui32Trim = SetupGetTrimForXoscHfIbiastherm();
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_ANABYPASSVAL2,
                  ui32Trim<<DDI_0_OSC_ANABYPASSVAL2_XOSC_HF_IBIASTHERM_S);

    // Trim AMPCOMP settings required before switch to XOSCHF
    ui32Trim = SetupGetTrimForAmpcompTh2();
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_AMPCOMPTH2, ui32Trim);
    ui32Trim = SetupGetTrimForAmpcompTh1();
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_AMPCOMPTH1, ui32Trim);
#if ( CCFG_BASE == CCFG_BASE_DEFAULT )
    ui32Trim = SetupGetTrimForAmpcompCtrl( ui32Fcfg1Revision );
#else
    ui32Trim = NOROM_SetupGetTrimForAmpcompCtrl( ui32Fcfg1Revision );
#endif
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_AMPCOMPCTL, ui32Trim);

    // Set trim for DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN in accordance to FCFG1 setting
    // This is bit[5] in the DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL register
    // Using MASK4 write + 1 => writing to bits[7:4]
    ui32Trim = SetupGetTrimForAdcShModeEn( ui32Fcfg1Revision );
    HWREGB( AUX_DDI0_OSC_BASE + DDI_O_MASK4B + ( DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL * 2 ) + 1 ) =
      ( 0x20 | ( ui32Trim << 1 ));

    // Set trim for DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN in accordance to FCFG1 setting
    // This is bit[4] in the DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL register
    // Using MASK4 write + 1 => writing to bits[7:4]
    ui32Trim = SetupGetTrimForAdcShVbufEn( ui32Fcfg1Revision );
    HWREGB( AUX_DDI0_OSC_BASE + DDI_O_MASK4B + ( DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL * 2 ) + 1 ) =
      ( 0x10 | ( ui32Trim ));

    // Set trim for the PEAK_DET_ITRIM, HP_BUF_ITRIM and LP_BUF_ITRIM bit fields
    // in the DDI0_OSC_O_XOSCHFCTL register in accordance to FCFG1 setting.
    // Remaining register bit fields are set to their reset values of 0.
    ui32Trim = SetupGetTrimForXoscHfCtl(ui32Fcfg1Revision);
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_XOSCHFCTL, ui32Trim);

    // Set trim for DBLR_LOOP_FILTER_RESET_VOLTAGE in accordance to FCFG1 setting
    // (This is bits [18:17] in DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL)
    // (Using MASK4 write + 4 => writing to bits[19:16] => (4*4))
    // (Assuming: DDI_0_OSC_ADCDOUBLERNANOAMPCTL_DBLR_LOOP_FILTER_RESET_VOLTAGE_S = 17 and
    //  that DDI_0_OSC_ADCDOUBLERNANOAMPCTL_DBLR_LOOP_FILTER_RESET_VOLTAGE_M = 0x00060000)
    ui32Trim = SetupGetTrimForDblrLoopFilterResetVoltage( ui32Fcfg1Revision );
    HWREGB( AUX_DDI0_OSC_BASE + DDI_O_MASK4B + ( DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL * 2 ) + 4 ) =
      ( 0x60 | ( ui32Trim << 1 ));

    // Update DDI_0_OSC_ATESTCTL_ATESTLF_RCOSCLF_IBIAS_TRIM with data from
    // FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM
    // This is DDI_0_OSC_O_ATESTCTL bit[7]
    // ( DDI_0_OSC_O_ATESTCTL is currently hidden (but=0x00000020))
    // Using MASK4 write + 1 => writing to bits[7:4]
    ui32Trim = SetupGetTrimForRcOscLfIBiasTrim( ui32Fcfg1Revision );
    HWREGB( AUX_DDI0_OSC_BASE + DDI_O_MASK4B + ( 0x00000020 * 2 ) + 1 ) =
      ( 0x80 | ( ui32Trim << 3 ));

    // Update DDI_0_OSC_LFOSCCTL_XOSCLF_REGULATOR_TRIM and
    //        DDI_0_OSC_LFOSCCTL_XOSCLF_CMIRRWR_RATIO in one write
    // This can be simplified since the registers are packed together in the same
    // order both in FCFG1 and in the HW register.
    // This spans DDI_0_OSC_O_LFOSCCTL bits[23:18]
    // Using MASK8 write + 4 => writing to bits[23:16]
    ui32Trim = SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio( ui32Fcfg1Revision );
    HWREGH( AUX_DDI0_OSC_BASE + DDI_O_MASK8B + ( DDI_0_OSC_O_LFOSCCTL * 2 ) + 4 ) =
      ( 0xFC00 | ( ui32Trim << 2 ));

    // Set trim the HPM_IBIAS_WAIT_CNT, LPM_IBIAS_WAIT_CNT and IDAC_STEP bit
    // fields in the DDI0_OSC_O_RADCEXTCFG register in accordance to FCFG1 setting.
    // Remaining register bit fields are set to their reset values of 0.
    ui32Trim = SetupGetTrimForRadcExtCfg(ui32Fcfg1Revision);
    DDI32RegWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_RADCEXTCFG, ui32Trim);

}

//*****************************************************************************
//
// SetupAfterColdResetWakeupFromShutDownCfg3
//
//*****************************************************************************
void
SetupAfterColdResetWakeupFromShutDownCfg3( uint32_t ccfg_ModeConfReg )
{
    uint32_t   fcfg1OscConf;
    uint32_t   ui32Trim;
    uint32_t   currentHfClock;
    uint32_t   ccfgExtLfClk;

    // Examine the XOSC_FREQ field to select 0x1=HPOSC, 0x2=48MHz XOSC, 0x3=24MHz XOSC
    switch (( ccfg_ModeConfReg & CCFG_MODE_CONF_XOSC_FREQ_M ) >> CCFG_MODE_CONF_XOSC_FREQ_S ) {
    case 2 :
        // XOSC source is a 48 MHz crystal
        // Do nothing (since this is the reset setting)
        break;
    case 1 :
        // XOSC source is HPOSC (trim the HPOSC if this is a chip with HPOSC, otherwise skip trimming and default to 24 MHz XOSC)

        fcfg1OscConf = HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF );

        if (( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_OPTION ) == 0 ) {
            // This is a HPOSC chip, apply HPOSC settings
            // Set bit DDI_0_OSC_CTL0_HPOSC_MODE_EN (this is bit 14 in DDI_0_OSC_O_CTL0)
            HWREG( AUX_DDI0_OSC_BASE + DDI_O_SET + DDI_0_OSC_O_CTL0 ) = DDI_0_OSC_CTL0_HPOSC_MODE_EN;

            // ADI_2_REFSYS_HPOSCCTL2_BIAS_HOLD_MODE_EN = FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN   (1 bit)
            // ADI_2_REFSYS_HPOSCCTL2_CURRMIRR_RATIO    = FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO      (4 bits)
            // ADI_2_REFSYS_HPOSCCTL1_BIAS_RES_SET      = FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET        (4 bits)
            // ADI_2_REFSYS_HPOSCCTL0_FILTER_EN         = FCFG1_OSC_CONF_HPOSC_FILTER_EN           (1 bit)
            // ADI_2_REFSYS_HPOSCCTL0_BIAS_RECHARGE_DLY = FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY (2 bits)
            // ADI_2_REFSYS_HPOSCCTL0_SERIES_CAP        = FCFG1_OSC_CONF_HPOSC_SERIES_CAP          (2 bits)
            // ADI_2_REFSYS_HPOSCCTL0_DIV3_BYPASS       = FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS         (1 bit)

            HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL2 ) = (( HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL2 ) &
                  ~( ADI_2_REFSYS_HPOSCCTL2_BIAS_HOLD_MODE_EN_M | ADI_2_REFSYS_HPOSCCTL2_CURRMIRR_RATIO_M  )                                                                       ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN_M   ) >> FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN_S   ) << ADI_2_REFSYS_HPOSCCTL2_BIAS_HOLD_MODE_EN_S   ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO_M      ) >> FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO_S      ) << ADI_2_REFSYS_HPOSCCTL2_CURRMIRR_RATIO_S      )   );
            HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL1 ) = (( HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL1 ) & ~( ADI_2_REFSYS_HPOSCCTL1_BIAS_RES_SET_M )                          ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET_M        ) >> FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET_S        ) << ADI_2_REFSYS_HPOSCCTL1_BIAS_RES_SET_S        )   );
            HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL0 ) = (( HWREG( ADI2_BASE + ADI_2_REFSYS_O_HPOSCCTL0 ) &
                  ~( ADI_2_REFSYS_HPOSCCTL0_FILTER_EN_M | ADI_2_REFSYS_HPOSCCTL0_BIAS_RECHARGE_DLY_M | ADI_2_REFSYS_HPOSCCTL0_SERIES_CAP_M | ADI_2_REFSYS_HPOSCCTL0_DIV3_BYPASS_M )) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_FILTER_EN_M           ) >> FCFG1_OSC_CONF_HPOSC_FILTER_EN_S           ) << ADI_2_REFSYS_HPOSCCTL0_FILTER_EN_S           ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY_M ) >> FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY_S ) << ADI_2_REFSYS_HPOSCCTL0_BIAS_RECHARGE_DLY_S   ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_SERIES_CAP_M          ) >> FCFG1_OSC_CONF_HPOSC_SERIES_CAP_S          ) << ADI_2_REFSYS_HPOSCCTL0_SERIES_CAP_S          ) |
                   ((( fcfg1OscConf & FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS_M         ) >> FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS_S         ) << ADI_2_REFSYS_HPOSCCTL0_DIV3_BYPASS_S         )   );
            break;
        }
        // Not a HPOSC chip - fall through to default
    default :
        // XOSC source is a 24 MHz crystal (default)
        // Set bit DDI_0_OSC_CTL0_XTAL_IS_24M (this is bit 31 in DDI_0_OSC_O_CTL0)
        HWREG( AUX_DDI0_OSC_BASE + DDI_O_SET + DDI_0_OSC_O_CTL0 ) = DDI_0_OSC_CTL0_XTAL_IS_24M;
        break;
    }

    // Set XOSC_HF in bypass mode if CCFG is configured for external TCXO
    // Please note that it is up to the customer to make sure that the external clock source is up and running before XOSC_HF can be used.
    if (( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_TCXO ) == 0 ) {
        HWREG( AUX_DDI0_OSC_BASE + DDI_O_SET + DDI_0_OSC_O_XOSCHFCTL ) = DDI_0_OSC_XOSCHFCTL_BYPASS;
    }

    // Clear DDI_0_OSC_CTL0_CLK_LOSS_EN (ClockLossEventEnable()). This is bit 9 in DDI_0_OSC_O_CTL0.
    // This is typically already 0 except on Lizard where it is set in ROM-boot
    HWREG( AUX_DDI0_OSC_BASE + DDI_O_CLR + DDI_0_OSC_O_CTL0 ) = DDI_0_OSC_CTL0_CLK_LOSS_EN;

    // Setting DDI_0_OSC_CTL1_XOSC_HF_FAST_START according to value found in FCFG1
    ui32Trim = SetupGetTrimForXoscHfFastStart();
    HWREGB( AUX_DDI0_OSC_BASE + DDI_O_MASK4B + ( DDI_0_OSC_O_CTL1 * 2 )) = ( 0x30 | ui32Trim );

    // setup the LF clock based upon CCFG:MODE_CONF:SCLK_LF_OPTION
    switch (( ccfg_ModeConfReg & CCFG_MODE_CONF_SCLK_LF_OPTION_M ) >> CCFG_MODE_CONF_SCLK_LF_OPTION_S ) {
    case 0 : // XOSC_HF_DLF (XOSCHF/1536) -> SCLK_LF (=31250 Hz)
        OSCClockSourceSet( OSC_SRC_CLK_LF, OSC_XOSC_HF );
        SetupSetAonRtcSubSecInc( 0x8637BD ); // RTC_INCREMENT = 2^38 / frequency
        break;
    case 1 : // EXTERNAL signal -> SCLK_LF (frequency=2^38/CCFG_EXT_LF_CLK_RTC_INCREMENT)
        // Set SCLK_LF to use the same source as SCLK_HF
        // Can be simplified a bit since possible return values for HF matches LF settings
        currentHfClock = OSCClockSourceGet( OSC_SRC_CLK_HF );
        OSCClockSourceSet( OSC_SRC_CLK_LF, currentHfClock );
        while( OSCClockSourceGet( OSC_SRC_CLK_LF ) != currentHfClock ) {
            // Wait until switched
        }
        ccfgExtLfClk = HWREG( CCFG_BASE + CCFG_O_EXT_LF_CLK );
        SetupSetAonRtcSubSecInc(( ccfgExtLfClk & CCFG_EXT_LF_CLK_RTC_INCREMENT_M ) >> CCFG_EXT_LF_CLK_RTC_INCREMENT_S );
        IOCPortConfigureSet(( ccfgExtLfClk & CCFG_EXT_LF_CLK_DIO_M ) >> CCFG_EXT_LF_CLK_DIO_S,
                              IOC_PORT_AON_CLK32K,
                              IOC_STD_INPUT | IOC_HYST_ENABLE );   // Route external clock to AON IOC w/hysteresis
                                                                   // Set XOSC_LF in bypass mode to allow external 32 kHz clock
        HWREG( AUX_DDI0_OSC_BASE + DDI_O_SET + DDI_0_OSC_O_CTL0 ) = DDI_0_OSC_CTL0_XOSC_LF_DIG_BYPASS;
        // Fall through to set XOSC_LF as SCLK_LF source
    case 2 : // XOSC_LF -> SLCK_LF (32768 Hz)
        OSCClockSourceSet( OSC_SRC_CLK_LF, OSC_XOSC_LF );
        break;
    default : // (=3) RCOSC_LF
        OSCClockSourceSet( OSC_SRC_CLK_LF, OSC_RCOSC_LF );
        break;
    }

    // Update ADI_4_AUX_ADCREF1_VTRIM with value from FCFG1
    HWREGB( AUX_ADI4_BASE + ADI_4_AUX_O_ADCREF1 ) =
      ((( HWREG( FCFG1_BASE + FCFG1_O_SOC_ADC_REF_TRIM_AND_OFFSET_EXT ) >>
      FCFG1_SOC_ADC_REF_TRIM_AND_OFFSET_EXT_SOC_ADC_REF_VOLTAGE_TRIM_TEMP1_S ) <<
      ADI_4_AUX_ADCREF1_VTRIM_S ) &
      ADI_4_AUX_ADCREF1_VTRIM_M );

    // Sync with AON
    SysCtrlAonSync();
}

//*****************************************************************************
//
// SetupGetTrimForAnabypassValue1
//
//*****************************************************************************
uint32_t
SetupGetTrimForAnabypassValue1( uint32_t ccfg_ModeConfReg )
{
    uint32_t ui32Fcfg1Value            ;
    uint32_t ui32XoscHfRow             ;
    uint32_t ui32XoscHfCol             ;
    uint32_t ui32TrimValue             ;

    // Use device specific trim values located in factory configuration
    // area for the XOSC_HF_COLUMN_Q12 and XOSC_HF_ROW_Q12 bit fields in
    // the ANABYPASS_VALUE1 register. Value for the other bit fields
    // are set to 0.

    ui32Fcfg1Value = HWREG(FCFG1_BASE + FCFG1_O_CONFIG_OSC_TOP);
    ui32XoscHfRow = (( ui32Fcfg1Value &
        FCFG1_CONFIG_OSC_TOP_XOSC_HF_ROW_Q12_M ) >>
        FCFG1_CONFIG_OSC_TOP_XOSC_HF_ROW_Q12_S );
    ui32XoscHfCol = (( ui32Fcfg1Value &
        FCFG1_CONFIG_OSC_TOP_XOSC_HF_COLUMN_Q12_M ) >>
        FCFG1_CONFIG_OSC_TOP_XOSC_HF_COLUMN_Q12_S );

    if (( ccfg_ModeConfReg & CCFG_MODE_CONF_XOSC_CAP_MOD ) == 0 ) {
        // XOSC_CAP_MOD = 0 means: CAP_ARRAY_DELTA is in use -> Apply compensation
        // XOSC_CAPARRAY_DELTA is located in bit[15:8] of ccfg_ModeConfReg
        // Note: HW_REV_DEPENDENT_IMPLEMENTATION. Field width is not given by
        // a define and sign extension must therefore be hard coded.
        // ( A small test program is created verifying the code lines below:
        //   Ref.: ..\test\small_standalone_test_programs\CapArrayDeltaAdjust_test.c)
        int32_t i32CustomerDeltaAdjust =
            (((int32_t)( ccfg_ModeConfReg << ( 32 - CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA_W - CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA_S )))
                                          >> ( 32 - CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA_W ));

        while ( i32CustomerDeltaAdjust < 0 ) {
            ui32XoscHfCol >>= 1;                              // COL 1 step down
            if ( ui32XoscHfCol == 0 ) {                       // if COL below minimum
                ui32XoscHfCol = 0xFFFF;                       //   Set COL to maximum
                ui32XoscHfRow >>= 1;                          //   ROW 1 step down
                if ( ui32XoscHfRow == 0 ) {                   // if ROW below minimum
                   ui32XoscHfRow = 1;                         //   Set both ROW and COL
                   ui32XoscHfCol = 1;                         //   to minimum
                }
            }
            i32CustomerDeltaAdjust++;
        }
        while ( i32CustomerDeltaAdjust > 0 ) {
            ui32XoscHfCol = ( ui32XoscHfCol << 1 ) | 1;       // COL 1 step up
            if ( ui32XoscHfCol > 0xFFFF ) {                   // if COL above maximum
                ui32XoscHfCol = 1;                            //   Set COL to minimum
                ui32XoscHfRow = ( ui32XoscHfRow << 1 ) | 1;   //   ROW 1 step up
                if ( ui32XoscHfRow > 0xF ) {                  // if ROW above maximum
                   ui32XoscHfRow = 0xF;                       //   Set both ROW and COL
                   ui32XoscHfCol = 0xFFFF;                    //   to maximum
                }
            }
            i32CustomerDeltaAdjust--;
        }
    }

    ui32TrimValue = (( ui32XoscHfRow << DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_ROW_Q12_S    ) |
                     ( ui32XoscHfCol << DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_COLUMN_Q12_S )   );

    return (ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForRcOscLfRtuneCtuneTrim
//
//*****************************************************************************
uint32_t
SetupGetTrimForRcOscLfRtuneCtuneTrim( void )
{
    uint32_t ui32TrimValue;

    // Use device specific trim values located in factory configuration
    // area
    ui32TrimValue =
        ((HWREG(FCFG1_BASE + FCFG1_O_CONFIG_OSC_TOP) &
          FCFG1_CONFIG_OSC_TOP_RCOSCLF_CTUNE_TRIM_M)>>
          FCFG1_CONFIG_OSC_TOP_RCOSCLF_CTUNE_TRIM_S)<<
            DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_S;

    ui32TrimValue |=
        ((HWREG(FCFG1_BASE + FCFG1_O_CONFIG_OSC_TOP) &
          FCFG1_CONFIG_OSC_TOP_RCOSCLF_RTUNE_TRIM_M)>>
          FCFG1_CONFIG_OSC_TOP_RCOSCLF_RTUNE_TRIM_S)<<
            DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_S;

    return(ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForXoscHfIbiastherm
//
//*****************************************************************************
uint32_t
SetupGetTrimForXoscHfIbiastherm( void )
{
    uint32_t ui32TrimValue;

    // Use device specific trim value located in factory configuration
    // area
    ui32TrimValue =
        (HWREG(FCFG1_BASE + FCFG1_O_ANABYPASS_VALUE2) &
         FCFG1_ANABYPASS_VALUE2_XOSC_HF_IBIASTHERM_M)>>
         FCFG1_ANABYPASS_VALUE2_XOSC_HF_IBIASTHERM_S;

    return(ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForAmpcompTh2
//
//*****************************************************************************
uint32_t
SetupGetTrimForAmpcompTh2( void )
{
    uint32_t ui32TrimValue;
    uint32_t ui32Fcfg1Value;

    // Use device specific trim value located in factory configuration
    // area. All defined register bit fields have corresponding trim
    // value in the factory configuration area
    ui32Fcfg1Value = HWREG(FCFG1_BASE + FCFG1_O_AMPCOMP_TH2);
    ui32TrimValue = ((ui32Fcfg1Value &
                      FCFG1_AMPCOMP_TH2_LPMUPDATE_LTH_M)>>
                      FCFG1_AMPCOMP_TH2_LPMUPDATE_LTH_S)<<
                   DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_LTH_S;
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH2_LPMUPDATE_HTM_M)>>
                        FCFG1_AMPCOMP_TH2_LPMUPDATE_HTM_S)<<
                     DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_HTH_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_LPM_M)>>
                        FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_LPM_S)<<
                     DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_LPM_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_HPM_M)>>
                        FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_HPM_S)<<
                     DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_HPM_S);

    return(ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForAmpcompTh1
//
//*****************************************************************************
uint32_t
SetupGetTrimForAmpcompTh1( void )
{
    uint32_t ui32TrimValue;
    uint32_t ui32Fcfg1Value;

    // Use device specific trim values located in factory configuration
    // area. All defined register bit fields have a corresponding trim
    // value in the factory configuration area
    ui32Fcfg1Value = HWREG(FCFG1_BASE + FCFG1_O_AMPCOMP_TH1);
    ui32TrimValue = (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH1_HPMRAMP3_LTH_M)>>
                        FCFG1_AMPCOMP_TH1_HPMRAMP3_LTH_S)<<
                     DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_LTH_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH1_HPMRAMP3_HTH_M)>>
                        FCFG1_AMPCOMP_TH1_HPMRAMP3_HTH_S)<<
                     DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_HTH_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH1_IBIASCAP_LPTOHP_OL_CNT_M)>>
                        FCFG1_AMPCOMP_TH1_IBIASCAP_LPTOHP_OL_CNT_S)<<
                     DDI_0_OSC_AMPCOMPTH1_IBIASCAP_LPTOHP_OL_CNT_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_TH1_HPMRAMP1_TH_M)>>
                        FCFG1_AMPCOMP_TH1_HPMRAMP1_TH_S)<<
                     DDI_0_OSC_AMPCOMPTH1_HPMRAMP1_TH_S);

    return(ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForAmpcompCtrl
//
//*****************************************************************************
uint32_t
SetupGetTrimForAmpcompCtrl( uint32_t ui32Fcfg1Revision )
{
    uint32_t ui32TrimValue    ;
    uint32_t ui32Fcfg1Value   ;
    uint32_t ibiasOffset      ;
    uint32_t ibiasInit        ;
    uint32_t modeConf1        ;
    int32_t  deltaAdjust      ;

    // Use device specific trim values located in factory configuration
    // area. Register bit fields without trim values in the factory
    // configuration area will be set to the value of 0.
    ui32Fcfg1Value = HWREG( FCFG1_BASE + FCFG1_O_AMPCOMP_CTRL1 );

    ibiasOffset    = ( ui32Fcfg1Value &
                       FCFG1_AMPCOMP_CTRL1_IBIAS_OFFSET_M ) >>
                       FCFG1_AMPCOMP_CTRL1_IBIAS_OFFSET_S ;
    ibiasInit      = ( ui32Fcfg1Value &
                       FCFG1_AMPCOMP_CTRL1_IBIAS_INIT_M ) >>
                       FCFG1_AMPCOMP_CTRL1_IBIAS_INIT_S ;

    if (( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_XOSC_OVR_M ) == 0 ) {
        // Adjust with DELTA_IBIAS_OFFSET and DELTA_IBIAS_INIT from CCFG
        modeConf1   = HWREG( CCFG_BASE + CCFG_O_MODE_CONF_1 );

        // Both fields are signed 4-bit values. This is an assumption when doing the sign extension.
        deltaAdjust =
            (((int32_t)( modeConf1 << ( 32 - CCFG_MODE_CONF_1_DELTA_IBIAS_OFFSET_W - CCFG_MODE_CONF_1_DELTA_IBIAS_OFFSET_S )))
                                   >> ( 32 - CCFG_MODE_CONF_1_DELTA_IBIAS_OFFSET_W ));
        deltaAdjust += (int32_t)ibiasOffset;
        if ( deltaAdjust < 0 ) {
            deltaAdjust  = 0;
        }
        if ( deltaAdjust > ( DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_M >> DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_S )) {
            deltaAdjust  = ( DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_M >> DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_S );
        }
        ibiasOffset = (uint32_t)deltaAdjust;

        deltaAdjust =
            (((int32_t)( modeConf1 << ( 32 - CCFG_MODE_CONF_1_DELTA_IBIAS_INIT_W - CCFG_MODE_CONF_1_DELTA_IBIAS_INIT_S )))
                                   >> ( 32 - CCFG_MODE_CONF_1_DELTA_IBIAS_INIT_W ));
        deltaAdjust += (int32_t)ibiasInit;
        if ( deltaAdjust < 0 ) {
            deltaAdjust  = 0;
        }
        if ( deltaAdjust > ( DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_M >> DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_S )) {
            deltaAdjust  = ( DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_M >> DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_S );
        }
        ibiasInit = (uint32_t)deltaAdjust;
    }
    ui32TrimValue = ( ibiasOffset << DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_S ) |
                    ( ibiasInit   << DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_S   ) ;

    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_CTRL1_LPM_IBIAS_WAIT_CNT_FINAL_M)>>
                        FCFG1_AMPCOMP_CTRL1_LPM_IBIAS_WAIT_CNT_FINAL_S)<<
                       DDI_0_OSC_AMPCOMPCTL_LPM_IBIAS_WAIT_CNT_FINAL_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_CTRL1_CAP_STEP_M)>>
                        FCFG1_AMPCOMP_CTRL1_CAP_STEP_S)<<
                       DDI_0_OSC_AMPCOMPCTL_CAP_STEP_S);
    ui32TrimValue |= (((ui32Fcfg1Value &
                        FCFG1_AMPCOMP_CTRL1_IBIASCAP_HPTOLP_OL_CNT_M)>>
                        FCFG1_AMPCOMP_CTRL1_IBIASCAP_HPTOLP_OL_CNT_S)<<
                       DDI_0_OSC_AMPCOMPCTL_IBIASCAP_HPTOLP_OL_CNT_S);

    if ( ui32Fcfg1Revision >= 0x00000022 ) {
        ui32TrimValue |= ((( ui32Fcfg1Value &
            FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE_M ) >>
            FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE_S ) <<
           DDI_0_OSC_AMPCOMPCTL_AMPCOMP_REQ_MODE_S );
    }

    return(ui32TrimValue);
}

//*****************************************************************************
//
// SetupGetTrimForDblrLoopFilterResetVoltage
//
//*****************************************************************************
uint32_t
SetupGetTrimForDblrLoopFilterResetVoltage( uint32_t ui32Fcfg1Revision )
{
   uint32_t dblrLoopFilterResetVoltageValue = 0; // Reset value

   if ( ui32Fcfg1Revision >= 0x00000020 ) {
      dblrLoopFilterResetVoltageValue = ( HWREG( FCFG1_BASE + FCFG1_O_MISC_OTP_DATA_1 ) &
         FCFG1_MISC_OTP_DATA_1_DBLR_LOOP_FILTER_RESET_VOLTAGE_M ) >>
         FCFG1_MISC_OTP_DATA_1_DBLR_LOOP_FILTER_RESET_VOLTAGE_S;
   }

   return ( dblrLoopFilterResetVoltageValue );
}

//*****************************************************************************
//
// SetupGetTrimForAdcShModeEn
//
//*****************************************************************************
uint32_t
SetupGetTrimForAdcShModeEn( uint32_t ui32Fcfg1Revision )
{
   uint32_t getTrimForAdcShModeEnValue = 1; // Recommended default setting

   if ( ui32Fcfg1Revision >= 0x00000022 ) {
      getTrimForAdcShModeEnValue = ( HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF ) &
         FCFG1_OSC_CONF_ADC_SH_MODE_EN_M ) >>
         FCFG1_OSC_CONF_ADC_SH_MODE_EN_S;
   }

   return ( getTrimForAdcShModeEnValue );
}

//*****************************************************************************
//
// SetupGetTrimForAdcShVbufEn
//
//*****************************************************************************
uint32_t
SetupGetTrimForAdcShVbufEn( uint32_t ui32Fcfg1Revision )
{
   uint32_t getTrimForAdcShVbufEnValue = 1; // Recommended default setting

   if ( ui32Fcfg1Revision >= 0x00000022 ) {
      getTrimForAdcShVbufEnValue = ( HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF ) &
         FCFG1_OSC_CONF_ADC_SH_VBUF_EN_M ) >>
         FCFG1_OSC_CONF_ADC_SH_VBUF_EN_S;
   }

   return ( getTrimForAdcShVbufEnValue );
}

//*****************************************************************************
//
// SetupGetTrimForXoscHfCtl
//
//*****************************************************************************
uint32_t
SetupGetTrimForXoscHfCtl( uint32_t ui32Fcfg1Revision )
{
   uint32_t getTrimForXoschfCtlValue = 0; // Recommended default setting
   uint32_t fcfg1Data;

   if ( ui32Fcfg1Revision >= 0x00000020 ) {
      fcfg1Data = HWREG( FCFG1_BASE + FCFG1_O_MISC_OTP_DATA_1 );
      getTrimForXoschfCtlValue =
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_PEAK_DET_ITRIM_M ) >>
             FCFG1_MISC_OTP_DATA_1_PEAK_DET_ITRIM_S ) <<
           DDI_0_OSC_XOSCHFCTL_PEAK_DET_ITRIM_S);

      getTrimForXoschfCtlValue |=
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_HP_BUF_ITRIM_M ) >>
             FCFG1_MISC_OTP_DATA_1_HP_BUF_ITRIM_S ) <<
           DDI_0_OSC_XOSCHFCTL_HP_BUF_ITRIM_S);

      getTrimForXoschfCtlValue |=
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_LP_BUF_ITRIM_M ) >>
             FCFG1_MISC_OTP_DATA_1_LP_BUF_ITRIM_S ) <<
           DDI_0_OSC_XOSCHFCTL_LP_BUF_ITRIM_S);
   }

   return ( getTrimForXoschfCtlValue );
}

//*****************************************************************************
//
// SetupGetTrimForXoscHfFastStart
//
//*****************************************************************************
uint32_t
SetupGetTrimForXoscHfFastStart( void )
{
   uint32_t ui32XoscHfFastStartValue   ;

   // Get value from FCFG1
   ui32XoscHfFastStartValue = ( HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF ) &
      FCFG1_OSC_CONF_XOSC_HF_FAST_START_M ) >>
      FCFG1_OSC_CONF_XOSC_HF_FAST_START_S;

   return ( ui32XoscHfFastStartValue );
}

//*****************************************************************************
//
// SetupGetTrimForRadcExtCfg
//
//*****************************************************************************
uint32_t
SetupGetTrimForRadcExtCfg( uint32_t ui32Fcfg1Revision )
{
   uint32_t getTrimForRadcExtCfgValue = 0x403F8000; // Recommended default setting
   uint32_t fcfg1Data;

   if ( ui32Fcfg1Revision >= 0x00000020 ) {
      fcfg1Data = HWREG( FCFG1_BASE + FCFG1_O_MISC_OTP_DATA_1 );
      getTrimForRadcExtCfgValue =
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_HPM_IBIAS_WAIT_CNT_M ) >>
             FCFG1_MISC_OTP_DATA_1_HPM_IBIAS_WAIT_CNT_S ) <<
           DDI_0_OSC_RADCEXTCFG_HPM_IBIAS_WAIT_CNT_S);

      getTrimForRadcExtCfgValue |=
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_LPM_IBIAS_WAIT_CNT_M ) >>
             FCFG1_MISC_OTP_DATA_1_LPM_IBIAS_WAIT_CNT_S ) <<
           DDI_0_OSC_RADCEXTCFG_LPM_IBIAS_WAIT_CNT_S);

      getTrimForRadcExtCfgValue |=
         ( ( ( fcfg1Data & FCFG1_MISC_OTP_DATA_1_IDAC_STEP_M ) >>
             FCFG1_MISC_OTP_DATA_1_IDAC_STEP_S ) <<
           DDI_0_OSC_RADCEXTCFG_IDAC_STEP_S);
   }

   return ( getTrimForRadcExtCfgValue );
}

//*****************************************************************************
//
// SetupGetTrimForRcOscLfIBiasTrim
//
//*****************************************************************************
uint32_t
SetupGetTrimForRcOscLfIBiasTrim( uint32_t ui32Fcfg1Revision )
{
   uint32_t trimForRcOscLfIBiasTrimValue = 0; // Default value

   if ( ui32Fcfg1Revision >= 0x00000022 ) {
      trimForRcOscLfIBiasTrimValue = ( HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF ) &
         FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM_M ) >>
         FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM_S ;
   }

   return ( trimForRcOscLfIBiasTrimValue );
}

//*****************************************************************************
//
// SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
//
//*****************************************************************************
uint32_t
SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio( uint32_t ui32Fcfg1Revision )
{
   uint32_t trimForXoscLfRegulatorAndCmirrwrRatioValue = 0; // Default value for both fields

   if ( ui32Fcfg1Revision >= 0x00000022 ) {
      trimForXoscLfRegulatorAndCmirrwrRatioValue = ( HWREG( FCFG1_BASE + FCFG1_O_OSC_CONF ) &
         ( FCFG1_OSC_CONF_XOSCLF_REGULATOR_TRIM_M |
           FCFG1_OSC_CONF_XOSCLF_CMIRRWR_RATIO_M  )) >>
           FCFG1_OSC_CONF_XOSCLF_CMIRRWR_RATIO_S  ;
   }

   return ( trimForXoscLfRegulatorAndCmirrwrRatioValue );
}

//*****************************************************************************
//
// SetupSetCacheModeAccordingToCcfgSetting
//
//*****************************************************************************
void
SetupSetCacheModeAccordingToCcfgSetting( void )
{
    // - Make sure to enable aggressive VIMS clock gating for power optimization
    //   Only for PG2 devices.
    // - Enable cache prefetch enable as default setting
    //   (Slightly higher power consumption, but higher CPU performance)
    // - IF ( CCFG_..._DIS_GPRAM == 1 )
    //   then: Enable cache (set cache mode = 1), even if set by ROM boot code
    //         (This is done because it's not set by boot code when running inside
    //         a debugger supporting the Halt In Boot (HIB) functionality).
    //   else: Set MODE_GPRAM if not already set (see inline comments as well)
    uint32_t vimsCtlMode0 ;

    while ( HWREGBITW( VIMS_BASE + VIMS_O_STAT, VIMS_STAT_MODE_CHANGING_BITN )) {
        // Do nothing - wait for an eventual ongoing mode change to complete.
        // (There should typically be no wait time here, but need to be sure)
    }

    // Note that Mode=0 is equal to MODE_GPRAM
    vimsCtlMode0 = (( HWREG( VIMS_BASE + VIMS_O_CTL ) & ~VIMS_CTL_MODE_M ) | VIMS_CTL_DYN_CG_EN_M | VIMS_CTL_PREF_EN_M );


    if ( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_GPRAM ) {
        // Enable cache (and hence disable GPRAM)
        HWREG( VIMS_BASE + VIMS_O_CTL ) = ( vimsCtlMode0 | VIMS_CTL_MODE_CACHE );
    } else if (( HWREG( VIMS_BASE + VIMS_O_STAT ) & VIMS_STAT_MODE_M ) != VIMS_STAT_MODE_GPRAM ) {
        // GPRAM is enabled in CCFG but not selected
        // Note: It is recommended to go via MODE_OFF when switching to MODE_GPRAM
        HWREG( VIMS_BASE + VIMS_O_CTL ) = ( vimsCtlMode0 | VIMS_CTL_MODE_OFF );
        while (( HWREG( VIMS_BASE + VIMS_O_STAT ) & VIMS_STAT_MODE_M ) != VIMS_STAT_MODE_OFF ) {
            // Do nothing - wait for an eventual mode change to complete (This goes fast).
        }
        HWREG( VIMS_BASE + VIMS_O_CTL ) = vimsCtlMode0;
    } else {
        // Correct mode, but make sure PREF_EN and DYN_CG_EN always are set
        HWREG( VIMS_BASE + VIMS_O_CTL ) = vimsCtlMode0;
    }
}

//*****************************************************************************
//
// SetupSetAonRtcSubSecInc
//
//*****************************************************************************
void
SetupSetAonRtcSubSecInc( uint32_t subSecInc )
{
   // Loading a new RTCSUBSECINC value is done in 5 steps:
   // 1. Write bit[15:0] of new SUBSECINC value to AUX_SYSIF_O_RTCSUBSECINC0
   // 2. Write bit[23:16] of new SUBSECINC value to AUX_SYSIF_O_RTCSUBSECINC1
   // 3. Set AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ
   // 4. Wait for AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK
   // 5. Clear AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ
   HWREG( AUX_SYSIF_BASE + AUX_SYSIF_O_RTCSUBSECINC0 ) = (( subSecInc       ) & AUX_SYSIF_RTCSUBSECINC0_INC15_0_M  );
   HWREG( AUX_SYSIF_BASE + AUX_SYSIF_O_RTCSUBSECINC1 ) = (( subSecInc >> 16 ) & AUX_SYSIF_RTCSUBSECINC1_INC23_16_M );

   HWREG( AUX_SYSIF_BASE + AUX_SYSIF_O_RTCSUBSECINCCTL ) = AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ;
   while( ! ( HWREGBITW( AUX_SYSIF_BASE + AUX_SYSIF_O_RTCSUBSECINCCTL, AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK_BITN )));
   HWREG( AUX_SYSIF_BASE + AUX_SYSIF_O_RTCSUBSECINCCTL ) = 0;
}
