/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file KW4xXcvrDrv.c
* This is a source file for the transceiver driver.
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

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "BLEDefaults.h"
#include "ZigbeeDefaults.h"
#include "KW4xXcvrDrv.h"
#include "fsl_os_abstraction.h"
#include "fsl_device_registers.h"

#include "tsm_ll_timing.h"
#include "ifr_mkw40z4_radio.h"

#if !USE_DCOC_MAGIC_NUMBERS
#include <math.h>
#endif

#ifdef gXcvrXtalTrimEnabled_d
#include "Flash_Adapter.h"
#endif

#define INCLUDE_OLD_DRV_CODE            (0)
#define wait(param) while(param)
#define ASSERT(condition) if(condition) while(1);

/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */
typedef enum
{
    FIRST_INIT = 0,
    MODE_CHANGE = 1
} MODE_CHG_SEL_T;

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */
/* Channel Filter coeff for BLE */
const uint8_t gBLERxChfCoeff[8] = {
    RX_CHF_COEF0_def_c,
    RX_CHF_COEF1_def_c,
    RX_CHF_COEF2_def_c,
    RX_CHF_COEF3_def_c,
    RX_CHF_COEF4_def_c,
    RX_CHF_COEF5_def_c,
    RX_CHF_COEF6_def_c,
    RX_CHF_COEF7_def_c
};

/* Channel Filter coeff for Zigbee */
const uint8_t gZigbeeRxChfCoeff[8] = {
    RX_CHF_COEF0_Zigbee_def_c,
    RX_CHF_COEF1_Zigbee_def_c,
    RX_CHF_COEF2_Zigbee_def_c,
    RX_CHF_COEF3_Zigbee_def_c,
    RX_CHF_COEF4_Zigbee_def_c,
    RX_CHF_COEF5_Zigbee_def_c,
    RX_CHF_COEF6_Zigbee_def_c,
    RX_CHF_COEF7_Zigbee_def_c,
};

const uint8_t gPABiasTbl[8] = PA_BIAS_ENTRIES; /* See tsm_timing_ble.h for PA_BIAS_ENTRIES */
panic_fptr panic_function_ptr = NULL;
uint8_t panic_fptr_is_valid = 0; /* Flag to store validity of the panic function pointer */
static uint8_t gen1_dcgain_trims_enabled = 0;
static uint8_t HWDCoffsetCal = 0;
static uint32_t trim_status = 0xFFFF; /* Status of trims from IFR, default to all failed */
static uint16_t ifr_version = 0xFFFF; /* IFR data format version number, default to maxint. */

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
const pllChannel_t mapTable [channelMapTableSize] =
{
    {0x00000025, 0x07C00000},  /* 0 */
    {0x00000025, 0x07C80000},  /* 1 */
    {0x00000025, 0x07D00000},  /* 2 */
    {0x00000025, 0x07D80000},  /* 3 */
    {0x00000025, 0x07E00000},  /* 4 */
    {0x00000025, 0x07E80000},  /* 5 */
    {0x00000025, 0x07F00000},  /* 6 */
    {0x00000025, 0x07F80000},  /* 7 */
    {0x00000025, 0x00000000},  /* 8 */
    {0x00000025, 0x00080000},  /* 9 */
    {0x00000025, 0x00100000},  /* 10 */
    {0x00000025, 0x00180000},  /* 11 */
    {0x00000025, 0x00200000},  /* 12 */
    {0x00000025, 0x00280000},  /* 13 */
    {0x00000025, 0x00300000},  /* 14 */
    {0x00000025, 0x00380000},  /* 15 */
    {0x00000025, 0x00400000},  /* 16 */
    {0x00000025, 0x00480000},  /* 17 */
    {0x00000025, 0x00500000},  /* 18 */
    {0x00000025, 0x00580000},  /* 19 */
    {0x00000025, 0x00600000},  /* 20 */
    {0x00000025, 0x00680000},  /* 21 */
    {0x00000025, 0x00700000},  /* 22 */
    {0x00000025, 0x00780000},  /* 23 */
    {0x00000025, 0x00800000},  /* 24 */
    {0x00000025, 0x00880000},  /* 25 */
    {0x00000025, 0x00900000},  /* 26 */
    {0x00000025, 0x00980000},  /* 27 */
    {0x00000025, 0x00A00000},  /* 28 */
    {0x00000025, 0x00A80000},  /* 29 */
    {0x00000025, 0x00B00000},  /* 30 */
    {0x00000025, 0x00B80000},  /* 31 */
    {0x00000025, 0x00C00000},  /* 32 */
    {0x00000025, 0x00C80000},  /* 33 */
    {0x00000025, 0x00D00000},  /* 34 */
    {0x00000025, 0x00D80000},  /* 35 */
    {0x00000025, 0x00E00000},  /* 36 */
    {0x00000025, 0x00E80000},  /* 37 */
    {0x00000025, 0x00F00000},  /* 38 */
    {0x00000025, 0x00F80000},  /* 39 */
    {0x00000025, 0x01000000},  /* 40 */
    {0x00000026, 0x07080000},  /* 41 */
    {0x00000026, 0x07100000},  /* 42 */
    {0x00000026, 0x07180000},  /* 43 */
    {0x00000026, 0x07200000},  /* 44 */
    {0x00000026, 0x07280000},  /* 45 */
    {0x00000026, 0x07300000},  /* 46 */
    {0x00000026, 0x07380000},  /* 47 */
    {0x00000026, 0x07400000},  /* 48 */
    {0x00000026, 0x07480000},  /* 49 */
    {0x00000026, 0x07500000},  /* 50 */
    {0x00000026, 0x07580000},  /* 51 */
    {0x00000026, 0x07600000},  /* 52 */
    {0x00000026, 0x07680000},  /* 53 */
    {0x00000026, 0x07700000},  /* 54 */
    {0x00000026, 0x07780000},  /* 55 */
    {0x00000026, 0x07800000},  /* 56 */
    {0x00000026, 0x07880000},  /* 57 */
    {0x00000026, 0x07900000},  /* 58 */
    {0x00000026, 0x07980000},  /* 59 */
    {0x00000026, 0x07A00000},  /* 60 */
    {0x00000026, 0x07A80000},  /* 61 */
    {0x00000026, 0x07B00000},  /* 62 */
    {0x00000026, 0x07B80000},  /* 63 */
    {0x00000026, 0x07C00000},  /* 64 */
    {0x00000026, 0x07C80000},  /* 65 */
    {0x00000026, 0x07D00000},  /* 66 */
    {0x00000026, 0x07D80000},  /* 67 */
    {0x00000026, 0x07E00000},  /* 68 */
    {0x00000026, 0x07E80000},  /* 69 */
    {0x00000026, 0x07F00000},  /* 70 */
    {0x00000026, 0x07F80000},  /* 71 */
    {0x00000026, 0x00000000},  /* 72 */
    {0x00000026, 0x00080000},  /* 73 */
    {0x00000026, 0x00100000},  /* 74 */
    {0x00000026, 0x00180000},  /* 75 */
    {0x00000026, 0x00200000},  /* 76 */
    {0x00000026, 0x00280000},  /* 77 */
    {0x00000026, 0x00300000},  /* 78 */
    {0x00000026, 0x00380000},  /* 79 */
    {0x00000026, 0x00400000},  /* 80 */
    {0x00000026, 0x00480000},  /* 81 */
    {0x00000026, 0x00500000},  /* 82 */
    {0x00000026, 0x00580000},  /* 83 */
    {0x00000026, 0x00600000},  /* 84 */
    {0x00000026, 0x00680000},  /* 85 */
    {0x00000026, 0x00700000},  /* 86 */
    {0x00000026, 0x00780000},  /* 87 */
    {0x00000026, 0x00800000},  /* 88 */
    {0x00000026, 0x00880000},  /* 89 */
    {0x00000026, 0x00900000},  /* 90 */
    {0x00000026, 0x00980000},  /* 91 */
    {0x00000026, 0x00A00000},  /* 92 */
    {0x00000026, 0x00A80000},  /* 93 */
    {0x00000026, 0x00B00000},  /* 94 */
    {0x00000026, 0x00B80000},  /* 95 */
    {0x00000026, 0x00C00000},  /* 96 */
    {0x00000026, 0x00C80000},  /* 97 */
    {0x00000026, 0x00D00000},  /* 98 */
    {0x00000026, 0x00D80000},  /* 99 */
    {0x00000026, 0x00E00000},  /* 100 */
    {0x00000026, 0x00E80000},  /* 101 */
    {0x00000026, 0x00F00000},  /* 102 */
    {0x00000026, 0x00F80000},  /* 103 */
    {0x00000026, 0x01000000},  /* 104 */
    {0x00000027, 0x07080000},  /* 105 */
    {0x00000027, 0x07100000},  /* 106 */
    {0x00000027, 0x07180000},  /* 107 */
    {0x00000027, 0x07200000},  /* 108 */
    {0x00000027, 0x07280000},  /* 109 */
    {0x00000027, 0x07300000},  /* 110 */
    {0x00000027, 0x07380000},  /* 111 */
    {0x00000027, 0x07400000},  /* 112 */
    {0x00000027, 0x07480000},  /* 113 */
    {0x00000027, 0x07500000},  /* 114 */
    {0x00000027, 0x07580000},  /* 115 */
    {0x00000027, 0x07600000},  /* 116 */
    {0x00000027, 0x07680000},  /* 117 */
    {0x00000027, 0x07700000},  /* 118 */
    {0x00000027, 0x07780000},  /* 119 */
    {0x00000027, 0x07800000},  /* 120 */
    {0x00000027, 0x07880000},  /* 121 */
    {0x00000027, 0x07900000},  /* 122 */
    {0x00000027, 0x07980000},  /* 123 */
    {0x00000027, 0x07A00000},  /* 124 */
    {0x00000027, 0x07A80000},  /* 125 */
    {0x00000027, 0x07B00000},  /* 126 */
    {0x00000027, 0x07B80000}   /* 127 */
};

/* Following are software trimmed values. They are initialized to potential
 * blind trim values with the intent that IFR trims overwrite these blind
 * trim values 
 */
float adc_gain_trimmed = ADC_SCALE_FACTOR;
uint8_t zb_tza_cap_tune = ZGBE_TZA_CAP_TUNE_def_c; 
uint8_t zb_bbf_cap_tune = ZGBE_BBF_CAP_TUNE_def_c; 
uint8_t zb_bbf_res_tune2 = ZGBE_BBF_RES_TUNE2_def_c;
uint8_t ble_tza_cap_tune = BLE_TZA_CAP_TUNE_def_c;
uint8_t ble_bbf_cap_tune = BLE_BBF_CAP_TUNE_def_c; 
uint8_t ble_bbf_res_tune2 = BLE_BBF_RES_TUNE2_def_c; 

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */
/* Common initialization and mode change routine, called by XcvrInit() and XcvRModeChange() */
void XcvrInit_ModeChg_Common ( radio_mode_t radioMode, MODE_CHG_SEL_T mode_change );

xcvrStatus_t XcvrCalcSetupDcoc ( void );
void XcvrManualDCOCCal (uint8_t chnum);

void XcvrSetTsmDefaults ( radio_mode_t radioMode );
void XcvrSetRxDigDefaults (  const uint8_t * filt_coeff_ptr, uint8_t iir3a_idx, uint8_t iir2a_idx, uint8_t iir1a_idx, uint8_t rssi_hold_src );
void XcvrSetTxDigPLLDefaults( radio_mode_t radioMode );
void XcvrSetAnalogDefaults ( radio_mode_t radioMode );

/* Separate out Mode Switch portion of init routines
 * call these routines with the target mode during a mode switch 
 */
void XcvrSetTsmDef_ModeSwitch ( radio_mode_t radioMode );
void XcvrSetRxDigDef_ModeSwitch (  const uint8_t * filt_coeff_ptr, uint8_t iir3a_idx, uint8_t iir2a_idx, uint8_t iir1a_idx, uint8_t rssi_hold_src );
void XcvrSetTxDigPLLDef_ModeSwitch( radio_mode_t radioMode );
void XcvrSetAnalogDef_ModeSwitch ( radio_mode_t radioMode );

void XcvrPanic (uint32_t panic_id, uint32_t location);
void XcvrDelay(uint32_t time);

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief  This function initializes the transceiver for operation in a particular
*  radioMode (BLE or Zigbee)
*
* \param[in] radioMode - the operating mode to which the radio should be initialized.
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
void XcvrInit ( radio_mode_t radioMode )
{
    XcvrInit_ModeChg_Common(radioMode,FIRST_INIT);
#ifdef gXcvrXtalTrimEnabled_d
  if( 0xFFFFFFFF != gHardwareParameters.xtalTrim )
  {
      XcvrSetXtalTrim( (uint8_t)gHardwareParameters.xtalTrim );
  }
#endif
}

/*! *********************************************************************************
* \brief  This function changes the radio operating mode between BLE and Zigbee 
*
* \param[in] radioMode - the new operating mode for the radio
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
xcvrStatus_t XcvrChangeMode ( radio_mode_t radioMode )
{
    XcvrInit_ModeChg_Common(radioMode, MODE_CHANGE);
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function allows a callback function to be registered allowing the
*  transceiver software to call a PANIC function in case of software error
*
* \param[in] fptr - a pointer to a function which implements system PANIC
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
void XcvrRegisterPanicCb ( panic_fptr fptr )
{
    panic_function_ptr = fptr;
    panic_fptr_is_valid = 1;
}

/*! *********************************************************************************
* \brief  This function allows a upper layer software to poll the health of the
*  transceiver to detect problems in the radio operation.
*
* \return radio_status - current status of the radio, 0 = no failures. Any other
*       value indicates a failure, see ::healthStatus_t for the possible values
*       which may all be OR'd together as needed.        
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
healthStatus_t XcvrHealthCheck ( void )
{
    healthStatus_t retval = NO_ERRORS;
    /* Read PLL status bits and set return values */
    if (XCVR_BRD_PLL_LOCK_DETECT_CTFF(XCVR) == 1)
    {
        retval |= PLL_CTUNE_FAIL;
    }
    if (XCVR_BRD_PLL_LOCK_DETECT_CSFF(XCVR) == 1)
    {
        retval |= PLL_CYCLE_SLIP_FAIL;
    }
    if (XCVR_BRD_PLL_LOCK_DETECT_FTFF(XCVR) == 1)
    {
        retval |= PLL_FREQ_TARG_FAIL;
    }
    if (XCVR_BRD_PLL_LOCK_DETECT_TAFF(XCVR) == 1)
    {
        retval |= PLL_TSM_ABORT_FAIL;
    }
    /* Once errors have been captured, clear the sticky flags.
     * Sticky bits are W1C so this clears them. 
     */
    XCVR_PLL_LOCK_DETECT = (uint32_t)(XCVR_PLL_LOCK_DETECT);
    return retval;
}

/*! *********************************************************************************
* \brief  This function allows a upper layer software to control the state of the
* Fast Antenna Diversity (FAD) and Low Power Preamble Search (LPPS) features for
* the Zigbee radio.
*
* \param control - desired setting for combined FAD/LPPS feature.        
*
* \ingroup PublicAPIs
*
* \details
*       FAD and LPPS are mutually exclusive features, they cannot be enabled at 
*       the same time so this API enforces that restriction.
*       This API only controls the transceiver functionality related to FAD and 
*       LPPS, it does not affect any link layer functionality that may be required.
* \note
*       This code does NOT set the pin muxing for the GPIO pins required for this
*       feature. TSM_GPIO2 and TSM_GPIO3 pins are used for TX and RX respectively.
***********************************************************************************/
void XcvrFadLppsControl(FAD_LPPS_CTRL_T control)
{
    switch (control)
    {
    case NONE:
        /* Disable FAD */
        XCVR_TSM_TIMING42 = (uint32_t)(TSM_REG_VALUE(0xFF, 0xFF, 0xFF, 0xFF)); /* Disabled TSM signal */
        XCVR_TSM_TIMING43 = (uint32_t)(TSM_REG_VALUE(0xFF, 0xFF, 0xFF, 0xFF)); /* Disabled TSM signal */
        /* Disable LPPS */
        XCVR_BWR_LPPS_CTRL_LPPS_ENABLE(XCVR, 0);
        break;
    case FAD_ENABLED:
        /* Enable FAD */
        XCVR_TSM_TIMING42 = (uint32_t)(TSM_REG_VALUE(0xFF, 0xFF, 0xFF, END_OF_TX_WU_BLE));
        XCVR_TSM_TIMING43 = (uint32_t)(TSM_REG_VALUE(0xFF, END_OF_RX_WU_BLE, 0xFF, 0xFF));
        /* Disable LPPS */
        XCVR_BWR_LPPS_CTRL_LPPS_ENABLE(XCVR, 0);
        break;
    case LPPS_ENABLED:
        /* Disable FAD */
        XCVR_TSM_TIMING42 = (uint32_t)(TSM_REG_VALUE(0xFF, 0xFF, 0xFF, 0xFF)); /* Disabled TSM signal */
        XCVR_TSM_TIMING43 = (uint32_t)(TSM_REG_VALUE(0xFF, 0xFF, 0xFF, 0xFF)); /* Disabled TSM signal */
        /* Enable LPPS */
        XCVR_LPPS_CTRL = (uint32_t)(XCVR_LPPS_CTRL_LPPS_QGEN25_ALLOW_MASK |
                                    XCVR_LPPS_CTRL_LPPS_ADC_ALLOW_MASK |
                                        XCVR_LPPS_CTRL_LPPS_ADC_CLK_ALLOW_MASK |
                                            XCVR_LPPS_CTRL_LPPS_ADC_I_Q_ALLOW_MASK |
                                                XCVR_LPPS_CTRL_LPPS_ADC_DAC_ALLOW_MASK |
                                                    XCVR_LPPS_CTRL_LPPS_BBF_ALLOW_MASK |
                                                        XCVR_LPPS_CTRL_LPPS_TCA_ALLOW_MASK
                                                            );
        XCVR_BWR_LPPS_CTRL_LPPS_ENABLE(XCVR, 1);
        break;
    default:
        /* PANIC? */
        break;
    }
}

/*! *********************************************************************************
* \brief  This function calls the system panic function for radio errors.
*
* \param[in] panic_id The identifier for the radio error.
* \param[in] location The location (address) in the code where the error happened.
*
*
* \ingroup PrivateAPIs
*
* \details This function is a common flow for both first init of the radio as well as 
* mode change of the radio. 
*
***********************************************************************************/
void XcvrPanic (uint32_t panic_id, uint32_t location)
{
    if (panic_fptr_is_valid)
    {
        (*panic_function_ptr)(panic_id, location, (uint32_t)0, (uint32_t)0);
    }
    else
    {
        while(1); /* No panic function registered. */
    }
}

/*! *********************************************************************************
* \brief  This function initializes or changes the mode of the transceiver.
*
* \param[in] radioMode The target operating mode of the radio.
* \param[in] mode_change Whether this is an initialization or a mode change (FIRST_INIT == Init, MODE_CHANGE==mode change).
*
*
* \ingroup PrivateAPIs
*
* \details This function is a common flow for both first init of the radio as well as 
* mode change of the radio. 
*
***********************************************************************************/
void XcvrInit_ModeChg_Common ( radio_mode_t radioMode, MODE_CHG_SEL_T mode_change )
{
    static radio_mode_t last_mode = INVALID_MODE;
    
    uint8_t osr;
    uint8_t norm_en;
    uint8_t chf_bypass;
    IFR_SW_TRIM_TBL_ENTRY_T sw_trim_tbl[] =
    {
        {ADC_GAIN, 0, FALSE},      /* Fetch the ADC GAIN parameter if available. */
        {ZB_FILT_TRIM, 0, FALSE},  /* Fetch the Zigbee BBW filter trim if available. */
        {BLE_FILT_TRIM, 0, FALSE}, /* Fetch the BLE BBW filter trim if available. */
        {TRIM_STATUS, 0, FALSE},   /* Fetch the trim status word if available. */
        {TRIM_VERSION, 0, FALSE}   /* Fetch the trim version number if available. */
    };
    
#define NUM_TRIM_TBL_ENTRIES    sizeof(sw_trim_tbl)/sizeof(IFR_SW_TRIM_TBL_ENTRY_T)  
    
    if( radioMode == last_mode )
    {
        return;
    }
    else
    {
        last_mode = radioMode;
    }
    
    /* Check that this is the proper chip version */
    if ((RSIM_ANA_TEST >> 24) & 0xF != 0x2)
    {
        XcvrPanic(WRONG_RADIO_ID_DETECTED,(uint32_t)&XcvrInit_ModeChg_Common);
    }
    
    
    /* Enable XCVR_DIG, LTC and DCDC clock always, independent of radio mode.
    * Only enable one of BTLL or Zigbee.
    */    
    switch (radioMode) 
    {
    case BLE:
        SIM_SCGC5 |=  SIM_SCGC5_BTLL_MASK | SIM_SCGC5_PHYDIG_MASK;
        break;
    case ZIGBEE:
        SIM_SCGC5 |=  SIM_SCGC5_ZigBee_MASK | SIM_SCGC5_PHYDIG_MASK;
        break;
    default:
        break;
    }   
    
    HWDCoffsetCal = 0;  /* Default to using SW DCOC calibration */
    
    gen1_dcgain_trims_enabled = 0;
    handle_ifr(&sw_trim_tbl[0], NUM_TRIM_TBL_ENTRIES);
    
    /* Transfer values from sw_trim_tbl[] to static variables used in radio operation */
    {
        uint16_t i;
        for (i=0;i<NUM_TRIM_TBL_ENTRIES;i++)
        {
            if (sw_trim_tbl[i].valid == TRUE)
            {
                switch (sw_trim_tbl[i].trim_id)
                {
                    /* ADC_GAIN */
                case ADC_GAIN:
                    adc_gain_trimmed = (float)sw_trim_tbl[i].trim_value/(float)(0x400);
                    gen1_dcgain_trims_enabled = 1; /* Only enable code that handles these trims if the ADC_GAIN is present. */
                    break;
                    /* ZB_FILT_TRIM */
                case ZB_FILT_TRIM:
                    zb_tza_cap_tune = ((sw_trim_tbl[i].trim_value & IFR_TZA_CAP_TUNE_MASK)>>IFR_TZA_CAP_TUNE_SHIFT);
                    zb_bbf_cap_tune = ((sw_trim_tbl[i].trim_value & IFR_BBF_CAP_TUNE_MASK)>>IFR_BBF_CAP_TUNE_SHIFT);
                    zb_bbf_res_tune2 = ((sw_trim_tbl[i].trim_value & IFR_RES_TUNE2_MASK)>>IFR_RES_TUNE2_SHIFT); 
                    break;
                    /* BLE_FILT_TRIM */
                case BLE_FILT_TRIM:
                    ble_tza_cap_tune = ((sw_trim_tbl[i].trim_value & IFR_TZA_CAP_TUNE_MASK)>>IFR_TZA_CAP_TUNE_SHIFT);
                    ble_bbf_cap_tune = ((sw_trim_tbl[i].trim_value & IFR_BBF_CAP_TUNE_MASK)>>IFR_BBF_CAP_TUNE_SHIFT);
                    ble_bbf_res_tune2 = ((sw_trim_tbl[i].trim_value & IFR_RES_TUNE2_MASK)>>IFR_RES_TUNE2_SHIFT); 
                    break;
                case TRIM_STATUS:
                    trim_status = sw_trim_tbl[i].trim_value;
                    break;
                case TRIM_VERSION:
                    ifr_version = sw_trim_tbl[i].trim_value & 0xFFFF; 
                    break;
                default:
                    break;
                }
            }
        }
    }
    
    {
        static uint32_t tempstatus = 0;
        tempstatus = (trim_status & (BGAP_VOLTAGE_TRIM_FAILED | IQMC_GAIN_ADJ_FAILED | IQMC_GAIN_ADJ_FAILED | IQMC_DC_GAIN_ADJ_FAILED));
        /* Determine whether this IC can support HW DC Offset Calibration.
        * Must be v3 or greater trim version and != 0xFFFF
        * Must have BGAP, IQMC and IQ DC GAIN calibration SUCCESS 
        * OR - may be over-ridden by a compile time flag to force HW DCOC cal
        */
        if (((ifr_version >= 3) && 
             (ifr_version != 0xFFFF) &&
                 (ifr_version != 0xFFFE) && /* Special version number indicating fallback IFR table used due to untrimmed part. */
                     (tempstatus == 0)) ||
            (ENGINEERING_TRIM_BYPASS))
        {
            HWDCoffsetCal = 1;
        }
    }
    
    XCVR_BWR_BGAP_CTRL_BGAP_ATST_ON(XCVR, 0);    
    
    if (mode_change == MODE_CHANGE)
    {
        /* Change only mode dependent analog & TSM settings */
        XcvrSetAnalogDef_ModeSwitch(radioMode);
        XcvrSetTsmDef_ModeSwitch(radioMode);
    }
    else
    {
        /* Setup initial analog & TSM settings */
        XcvrSetAnalogDefaults(radioMode);    
        XcvrSetTsmDefaults(radioMode);    
    }
    
    /* RX Channel filter coeffs and TSM settings are specific to modes */
    switch (radioMode) 
    {
    case BLE:
        if (mode_change == MODE_CHANGE)
        {
            XcvrSetRxDigDef_ModeSwitch(&gBLERxChfCoeff[0], DCOC_CAL_IIR3A_IDX_def_c, DCOC_CAL_IIR2A_IDX_def_c, DCOC_CAL_IIR1A_IDX_def_c, RSSI_HOLD_SRC_def_c);
        }
        else
        {
            XcvrSetRxDigDefaults(&gBLERxChfCoeff[0], DCOC_CAL_IIR3A_IDX_def_c, DCOC_CAL_IIR2A_IDX_def_c, DCOC_CAL_IIR1A_IDX_def_c, RSSI_HOLD_SRC_def_c);
        }
        XCVR_BWR_CTRL_PROTOCOL(XCVR, BLE_PROTOCOL_def_c);              /* BLE protocol selection */
        XCVR_BWR_CTRL_TGT_PWR_SRC(XCVR, BLE_TGT_PWR_SRC_def_c);
        XCVR_BWR_AGC_CTRL_0_FREEZE_AGC_SRC(XCVR, FREEZE_AGC_SRC_def_c); /* Freeze AGC */
        osr = RX_DEC_FILT_OSR_BLE_def_c;      /* OSR 4 */
        norm_en = RX_NORM_EN_BLE_def_c;       /* Normalizer OFF */
        chf_bypass = RX_CH_FILT_BYPASS_def_c; /* BLE Channel filter setting */
        
        /* DCOC_CTRL_0 & RX_DIG_CTRL settings done separately from other RX_DIG*/
        /* DCOC_CTRL_0 */ 
        XCVR_DCOC_CTRL_0 = (uint32_t)(
                                      (ALPHAC_SCALE_IDX_def_c << XCVR_DCOC_CTRL_0_DCOC_ALPHAC_SCALE_IDX_SHIFT) |
                                      (ALPHA_RADIUS_IDX_def_c << XCVR_DCOC_CTRL_0_DCOC_ALPHA_RADIUS_IDX_SHIFT) |
                                      (SIGN_SCALE_IDX_def_c << XCVR_DCOC_CTRL_0_DCOC_SIGN_SCALE_IDX_SHIFT) |
                                      (DCOC_CAL_DURATION_def_c << XCVR_DCOC_CTRL_0_DCOC_CAL_DURATION_SHIFT) |
                                      (DCOC_CORR_DLY_def_c << XCVR_DCOC_CTRL_0_DCOC_CORR_DLY_SHIFT) |
                                      (DCOC_CORR_HOLD_TIME_def_c << XCVR_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_SHIFT) |
                                      (DCOC_MAN_def_c << XCVR_DCOC_CTRL_0_DCOC_MAN_SHIFT) |
                                      (DCOC_TRACK_EN_def_c << XCVR_DCOC_CTRL_0_DCOC_TRACK_EN_SHIFT) |
                                      (DCOC_CORRECT_EN_def_c << XCVR_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT) 
                                     );  
        break;
    case ZIGBEE:
        if (mode_change == MODE_CHANGE)
        {
            XcvrSetRxDigDef_ModeSwitch(&gZigbeeRxChfCoeff[0], IIR3A_IDX_Zigbee_def_c, IIR2A_IDX_Zigbee_def_c, IIR1A_IDX_Zigbee_def_c, RSSI_HOLD_SRC_Zigbee_def_c);
        }
        else
        {
            XcvrSetRxDigDefaults(&gZigbeeRxChfCoeff[0], IIR3A_IDX_Zigbee_def_c, IIR2A_IDX_Zigbee_def_c, IIR1A_IDX_Zigbee_def_c, RSSI_HOLD_SRC_Zigbee_def_c);
        }
        XCVR_BWR_CTRL_PROTOCOL(XCVR, Zigbee_PROTOCOL_def_c); /* Zigbee protocol selection */
        XCVR_BWR_CTRL_TGT_PWR_SRC(XCVR, Zigbee_TGT_PWR_SRC_def_c);
        XCVR_BWR_AGC_CTRL_0_FREEZE_AGC_SRC(XCVR, FREEZE_AGC_SRC_Zigbee_def_c);  /* Freeze AGC */
        /* Correlation zero count for Zigbee preamble detection */
        XCVR_BWR_CORR_CTRL_CORR_NVAL(XCVR, CORR_NVAL_Zigbee_def_c);
        /* Correlation threshold */
        XCVR_BWR_CORR_CTRL_CORR_VT(XCVR, CORR_VT_Zigbee_def_c);
        osr = RX_DEC_FILT_OSR_Zigbee_def_c;          /* OSR 8 */
        norm_en = RX_NORM_EN_Zigbee_def_c;           /* Normalizer On */
        chf_bypass = RX_CH_FILT_BYPASS_Zigbee_def_c; /* Zigbee Channel filter setting */
        
        /* DCOC_CTRL_0 & RX_DIG_CTRL settings done separately from other RX_DIG*/
        /* DCOC_CTRL_0 */
        XCVR_DCOC_CTRL_0 = (uint32_t)(
                                      (ALPHAC_SCALE_IDX_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_ALPHAC_SCALE_IDX_SHIFT) |
                                      (ALPHA_RADIUS_IDX_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_ALPHA_RADIUS_IDX_SHIFT) |
                                      (SIGN_SCALE_IDX_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_SIGN_SCALE_IDX_SHIFT) |
                                      (DCOC_CAL_DURATION_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_CAL_DURATION_SHIFT) |
                                      (DCOC_CORR_DLY_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_CORR_DLY_SHIFT) |
                                      (DCOC_CORR_HOLD_TIME_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_SHIFT) |
                                      (DCOC_MAN_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_MAN_SHIFT) |
                                      (DCOC_TRACK_EN_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_TRACK_EN_SHIFT) |
                                      (DCOC_CORRECT_EN_Zigbee_def_c << XCVR_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT) 
                                     );
        break;
    default:
        break;
    }
    
    if (mode_change == MODE_CHANGE)
    {
        XcvrSetTxDigPLLDef_ModeSwitch(radioMode);
    }
    else
    {
        XcvrSetTxDigPLLDefaults(radioMode);
    }
    
    XCVR_DCOC_CTRL_0 = (uint32_t)((XCVR_DCOC_CTRL_0 & (uint32_t)~(uint32_t)(
                           XCVR_DCOC_CTRL_0_DCOC_MAN_MASK |
                           XCVR_DCOC_CTRL_0_DCOC_TRACK_EN_MASK |
                           XCVR_DCOC_CTRL_0_DCOC_CORRECT_EN_MASK 
                          )) | (uint32_t)(
                           (DCOC_MAN_def_c << XCVR_DCOC_CTRL_0_DCOC_MAN_SHIFT) |
                           (DCOC_TRACK_EN_def_c << XCVR_DCOC_CTRL_0_DCOC_TRACK_EN_SHIFT) |
                           (DCOC_CORRECT_EN_def_c << XCVR_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT) 
                          ));    
  
    /* DCOC_CTRL_0 & RX_DIG_CTRL settings done separately from other RX_DIG*/
    /* RX_DIG_CTRL */
    XCVR_RX_DIG_CTRL = (uint32_t)((XCVR_RX_DIG_CTRL & (uint32_t)~(uint32_t)(
                           XCVR_RX_DIG_CTRL_RX_RSSI_EN_MASK |
                           XCVR_RX_DIG_CTRL_RX_AGC_EN_MASK |
                           XCVR_RX_DIG_CTRL_RX_DCOC_EN_MASK |
                           XCVR_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK |
                           XCVR_RX_DIG_CTRL_RX_NORM_EN_MASK |
                           XCVR_RX_DIG_CTRL_RX_INTERP_EN_MASK |
                           XCVR_RX_DIG_CTRL_RX_CH_FILT_BYPASS_MASK |
                           XCVR_RX_DIG_CTRL_RX_DCOC_CAL_EN_MASK 
                          )) | (uint32_t)(
                           (RX_RSSI_EN_def_c << XCVR_RX_DIG_CTRL_RX_RSSI_EN_SHIFT) |
                           (RX_AGC_EN_def_c << XCVR_RX_DIG_CTRL_RX_AGC_EN_SHIFT) |
                           (RX_DCOC_EN_def_c << XCVR_RX_DIG_CTRL_RX_DCOC_EN_SHIFT) |
                           (osr << XCVR_RX_DIG_CTRL_RX_DEC_FILT_OSR_SHIFT) |
                           (norm_en << XCVR_RX_DIG_CTRL_RX_NORM_EN_SHIFT) |
                           (RX_INTERP_EN_def_c << XCVR_RX_DIG_CTRL_RX_INTERP_EN_SHIFT) |
                           (chf_bypass << XCVR_RX_DIG_CTRL_RX_CH_FILT_BYPASS_SHIFT) |
                           (0 << XCVR_RX_DIG_CTRL_RX_DCOC_CAL_EN_SHIFT)  /* DCOC_CAL_EN set to zero here, changed below if appropriate */
                          ));    
    
    if (HWDCoffsetCal)
    {
        XCVR_BWR_RX_DIG_CTRL_RX_DCOC_CAL_EN(XCVR, 1); /* DCOC_CAL 1=Enabled */
    }
    else
    {
        /* DCOC Calibration issue workaround. Performs a manual DCOC calibration and disables RX_DCOC_CAL_EN
        * Manipulates AGC settings during the calibration and then reverts them
        */
        if (mode_change == FIRST_INIT)
        {
            switch (radioMode) 
            {
            case BLE:
                XcvrManualDCOCCal(20);  /* Use channel 20 for BLE calibration @ ~mid-band */
                break;
            case ZIGBEE:        
                XcvrManualDCOCCal(18);  /* Use channel 18 for Zigbee calibration @ ~mid-band */
                break;
            default:
                break;
            } 
        }
    }
}

/*! *********************************************************************************
* \brief  This function sets the default vaues of the TSM registers.
*
* \param[in] radioMode Radio mode is used to enable mode specific settings.
*
* \ingroup PrivateAPIs
*
* \details This function also sets link layer registers to account for TX and
*       delays.
*
***********************************************************************************/
void XcvrSetTsmDefaults ( radio_mode_t radioMode )
{ 
    /*TSM_CTRL*/
    XCVR_TSM_CTRL = (uint32_t)((XCVR_TSM_CTRL & (uint32_t)~(uint32_t)(    
                        XCVR_TSM_CTRL_BKPT_MASK | 
                        XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_MASK |
                        XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_MASK |
                        XCVR_TSM_CTRL_ABORT_ON_CTUNE_MASK |
                        XCVR_TSM_CTRL_RX_ABORT_DIS_MASK |
                        XCVR_TSM_CTRL_TX_ABORT_DIS_MASK |
                        XCVR_TSM_CTRL_DATA_PADDING_EN_MASK |
                        XCVR_TSM_CTRL_PA_RAMP_SEL_MASK |
                        XCVR_TSM_CTRL_FORCE_RX_EN_MASK |
                        XCVR_TSM_CTRL_FORCE_TX_EN_MASK 
                       )) | (uint32_t)(
                        (BKPT_def_c << XCVR_TSM_CTRL_BKPT_SHIFT) |
                        (ABORT_ON_FREQ_TARG_def_c << XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_SHIFT) |
                        (ABORT_ON_CYCLE_SLIP_def_c << XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_SHIFT) |
                        (ABORT_ON_CTUNE_def_c << XCVR_TSM_CTRL_ABORT_ON_CTUNE_SHIFT) |
                        (RX_ABORT_DIS_def_c << XCVR_TSM_CTRL_RX_ABORT_DIS_SHIFT) |
                        (TX_ABORT_DIS_def_c << XCVR_TSM_CTRL_TX_ABORT_DIS_SHIFT) |
                        (DATA_PADDING_EN_def_c << XCVR_TSM_CTRL_DATA_PADDING_EN_SHIFT) |
                        (PA_RAMP_SEL_def_c << XCVR_TSM_CTRL_PA_RAMP_SEL_SHIFT) |
                        (FORCE_RX_EN_def_c  << XCVR_TSM_CTRL_FORCE_RX_EN_SHIFT) |
                        (FORCE_TX_EN_def_c  << XCVR_TSM_CTRL_FORCE_TX_EN_SHIFT)
                       ));                       
    
    /* Perform mode specific portion of TSM setup using mode switch routine */
    XcvrSetTsmDef_ModeSwitch(radioMode);
    
    /* Only init OVRD registers if a non-default setting is needed */  
    /*PA_POWER*/
    XCVR_BWR_PA_POWER_PA_POWER(XCVR, PA_POWER_def_c);  
    
    /*PA_BIAS_TBL0*/
    XCVR_PA_BIAS_TBL0 = (uint32_t)((XCVR_PA_BIAS_TBL0 & (uint32_t)~(uint32_t)(    
                        XCVR_PA_BIAS_TBL0_PA_BIAS3_MASK | 
                        XCVR_PA_BIAS_TBL0_PA_BIAS2_MASK |
                        XCVR_PA_BIAS_TBL0_PA_BIAS1_MASK |
                        XCVR_PA_BIAS_TBL0_PA_BIAS0_MASK
                       )) | (uint32_t)(
                        (gPABiasTbl[3] << XCVR_PA_BIAS_TBL0_PA_BIAS3_SHIFT) |
                        (gPABiasTbl[2] << XCVR_PA_BIAS_TBL0_PA_BIAS2_SHIFT) |
                        (gPABiasTbl[1] << XCVR_PA_BIAS_TBL0_PA_BIAS1_SHIFT) |
                        (gPABiasTbl[0] << XCVR_PA_BIAS_TBL0_PA_BIAS0_SHIFT)
                       ));    

    /*PA_BIAS_TBL1*/
    XCVR_PA_BIAS_TBL1 = (uint32_t)((XCVR_PA_BIAS_TBL1 & (uint32_t)~(uint32_t)(    
                        XCVR_PA_BIAS_TBL1_PA_BIAS7_MASK | 
                        XCVR_PA_BIAS_TBL1_PA_BIAS6_MASK |
                        XCVR_PA_BIAS_TBL1_PA_BIAS5_MASK |
                        XCVR_PA_BIAS_TBL1_PA_BIAS4_MASK
                       )) | (uint32_t)(
                        (gPABiasTbl[7] << XCVR_PA_BIAS_TBL1_PA_BIAS7_SHIFT) |
                        (gPABiasTbl[6] << XCVR_PA_BIAS_TBL1_PA_BIAS6_SHIFT) |
                        (gPABiasTbl[5] << XCVR_PA_BIAS_TBL1_PA_BIAS5_SHIFT) |
                        (gPABiasTbl[4] << XCVR_PA_BIAS_TBL1_PA_BIAS4_SHIFT)
                       ));  
}

/*! *********************************************************************************
* \brief  This function sets the radio mode specific default vaues of the TSM registers.
*
* \param[in] radioMode Radio mode is used to enable mode specific settings.
*
* \ingroup PrivateAPIs
*
* \details This function also sets link layer registers to account for TX and
*       delays. 
*
***********************************************************************************/
void XcvrSetTsmDef_ModeSwitch ( radio_mode_t radioMode )
{
    /* Setup TSM timing registers and LL timing registers at the same time */
    switch (radioMode)
    {
    case BLE:
        tsm_ll_timing_init(BLE_RADIO);
        break;
    case ZIGBEE:
        tsm_ll_timing_init(ZIGBEE_RADIO);
        break;
    default:
        break;
    }
}

/*! *********************************************************************************
* \brief  This function sets the default vaues of the RX DIG registers.
*
* \param[in] filt_coeff_ptr pointer to an array of 8 UINT8_T receive filter coefficients.
* \param[in] iir3a_idx value of the 3rd IIR index
* \param[in] iir2a_idx value of the 2nd IIR index
* \param[in] iir1a_idx value of the 1st IIR index
*
* \ingroup PrivateAPIs
*
* \details Sets up RX digital registers with command and mode specific settings.
*
* \note Only uses read-modify-write when only part of the register is being written.
*
***********************************************************************************/
void XcvrSetRxDigDefaults (  const uint8_t * filt_coeff_ptr, uint8_t iir3a_idx, uint8_t iir2a_idx, uint8_t iir1a_idx, uint8_t rssi_hold_src )
{
    /* Program RSSI registers */
    /* RSSI_CTRL_0 */
    XCVR_RSSI_CTRL_0 = (uint32_t)((XCVR_RSSI_CTRL_0 & (uint32_t)~(uint32_t)(
                           XCVR_RSSI_CTRL_0_RSSI_USE_VALS_MASK |
                           XCVR_RSSI_CTRL_0_RSSI_HOLD_EN_MASK |
                           XCVR_RSSI_CTRL_0_RSSI_DEC_EN_MASK |
                           XCVR_RSSI_CTRL_0_RSSI_IIR_WEIGHT_MASK |
                           XCVR_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_MASK |
                           XCVR_RSSI_CTRL_0_RSSI_ADJ_MASK
                          )) | (uint32_t)(
                           (RSSI_USE_VALS_def_c << XCVR_RSSI_CTRL_0_RSSI_USE_VALS_SHIFT) |
                           (RSSI_HOLD_EN_def_c << XCVR_RSSI_CTRL_0_RSSI_HOLD_EN_SHIFT) |
                           (RSSI_DEC_EN_def_c << XCVR_RSSI_CTRL_0_RSSI_DEC_EN_SHIFT) |
                           (RSSI_IIR_WEIGHT_def_c << XCVR_RSSI_CTRL_0_RSSI_IIR_WEIGHT_SHIFT) |
                           (RSSI_IIR_CW_WEIGHT_BYPASSEDdef_c << XCVR_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_SHIFT) |
                           (RSSI_ADJ_def_c << XCVR_RSSI_CTRL_0_RSSI_ADJ_SHIFT)
                          ));
    
    /* RSSI_CTRL_1 */
    XCVR_RSSI_CTRL_1 = (uint32_t)((XCVR_RSSI_CTRL_1 & (uint32_t)~(uint32_t)(
                           XCVR_RSSI_CTRL_1_RSSI_ED_THRESH1_H_MASK |
                           XCVR_RSSI_CTRL_1_RSSI_ED_THRESH0_H_MASK |
                           XCVR_RSSI_CTRL_1_RSSI_ED_THRESH1_MASK |
                           XCVR_RSSI_CTRL_1_RSSI_ED_THRESH0_MASK 
                          )) | (uint32_t)(
                           (RSSI_ED_THRESH1_H_def_c << XCVR_RSSI_CTRL_1_RSSI_ED_THRESH1_H_SHIFT) |
                           (RSSI_ED_THRESH0_H_def_c << XCVR_RSSI_CTRL_1_RSSI_ED_THRESH0_H_SHIFT) |
                           (RSSI_ED_THRESH1_def_c << XCVR_RSSI_CTRL_1_RSSI_ED_THRESH1_SHIFT) |
                           (RSSI_ED_THRESH0_def_c << XCVR_RSSI_CTRL_1_RSSI_ED_THRESH0_SHIFT) 
                          ));
        
    /* Program AGC registers */
    /* AGC_CTRL_0 */
    XCVR_AGC_CTRL_0 = (uint32_t)((XCVR_AGC_CTRL_0 & (uint32_t)~(uint32_t)(
                           XCVR_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_MASK |
                           XCVR_AGC_CTRL_0_AGC_UP_RSSI_THRESH_MASK |
                           XCVR_AGC_CTRL_0_AGC_DOWN_BBF_STEP_SZ_MASK |
                           XCVR_AGC_CTRL_0_AGC_DOWN_TZA_STEP_SZ_MASK |
                           XCVR_AGC_CTRL_0_AGC_UP_SRC_MASK |
                           XCVR_AGC_CTRL_0_AGC_UP_EN_MASK |
                           XCVR_AGC_CTRL_0_FREEZE_AGC_SRC_MASK |
                           XCVR_AGC_CTRL_0_AGC_FREEZE_EN_MASK |
                           XCVR_AGC_CTRL_0_SLOW_AGC_SRC_MASK |
                           XCVR_AGC_CTRL_0_SLOW_AGC_EN_MASK 
                          )) | (uint32_t)(
                           (AGC_DOWN_RSSI_THRESH_def_c << XCVR_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_SHIFT) |
                           (AGC_UP_RSSI_THRESH_def_c << XCVR_AGC_CTRL_0_AGC_UP_RSSI_THRESH_SHIFT) |
                           (AGC_DOWN_BBF_STEP_SZ_def_c << XCVR_AGC_CTRL_0_AGC_DOWN_BBF_STEP_SZ_SHIFT) |
                           (AGC_DOWN_TZA_STEP_SZ_def_c << XCVR_AGC_CTRL_0_AGC_DOWN_TZA_STEP_SZ_SHIFT) |
                           (AGC_UP_SRC_def_c << XCVR_AGC_CTRL_0_AGC_UP_SRC_SHIFT) |
                           (AGC_UP_EN_def_c << XCVR_AGC_CTRL_0_AGC_UP_EN_SHIFT) |
                           (FREEZE_AGC_SRC_def_c << XCVR_AGC_CTRL_0_FREEZE_AGC_SRC_SHIFT) |
                           (AGC_FREEZE_EN_def_c << XCVR_AGC_CTRL_0_AGC_FREEZE_EN_SHIFT) |
                           (SLOW_AGC_SRC_def_c << XCVR_AGC_CTRL_0_SLOW_AGC_SRC_SHIFT) |
                           (SLOW_AGC_EN_def_c << XCVR_AGC_CTRL_0_SLOW_AGC_EN_SHIFT)
                          ));
    
    /* AGC_CTRL_1 */
    XCVR_AGC_CTRL_1 = (uint32_t)((XCVR_AGC_CTRL_1 & (uint32_t)~(uint32_t)(
                           XCVR_AGC_CTRL_1_TZA_GAIN_SETTLE_TIME_MASK |
                           XCVR_AGC_CTRL_1_PRESLOW_EN_MASK |
                           XCVR_AGC_CTRL_1_USER_BBF_GAIN_EN_MASK |                             
                           XCVR_AGC_CTRL_1_USER_LNM_GAIN_EN_MASK |                             
                           XCVR_AGC_CTRL_1_BBF_USER_GAIN_MASK |                             
                           XCVR_AGC_CTRL_1_LNM_USER_GAIN_MASK |                             
                           XCVR_AGC_CTRL_1_LNM_ALT_CODE_MASK |
                           XCVR_AGC_CTRL_1_BBF_ALT_CODE_MASK                                                           
                          )) | (uint32_t)(
                           (TZA_GAIN_SETTLE_TIME_def_c << XCVR_AGC_CTRL_1_TZA_GAIN_SETTLE_TIME_SHIFT) |
                           (PRESLOW_EN_def_c << XCVR_AGC_CTRL_1_PRESLOW_EN_SHIFT) |
                           (USER_BBF_GAIN_EN_def_c << XCVR_AGC_CTRL_1_USER_BBF_GAIN_EN_SHIFT) |
                           (USER_LNM_GAIN_EN_def_c << XCVR_AGC_CTRL_1_USER_LNM_GAIN_EN_SHIFT) |
                           (BBF_USER_GAIN_def_c << XCVR_AGC_CTRL_1_BBF_USER_GAIN_SHIFT) |
                           (LNM_USER_GAIN_def_c << XCVR_AGC_CTRL_1_LNM_USER_GAIN_SHIFT) |
                           (LNM_ALT_CODE_def_c << XCVR_AGC_CTRL_1_LNM_ALT_CODE_SHIFT) |
                           (BBF_ALT_CODE_def_c << XCVR_AGC_CTRL_1_BBF_ALT_CODE_SHIFT)
                          ));
    
    /* AGC_CTRL_2 */
    XCVR_AGC_CTRL_2 = (uint32_t)((XCVR_AGC_CTRL_2 & (uint32_t)~(uint32_t)(
                           XCVR_AGC_CTRL_2_AGC_FAST_EXPIRE_MASK |
                           XCVR_AGC_CTRL_2_TZA_PDET_THRESH_HI_MASK |
                           XCVR_AGC_CTRL_2_TZA_PDET_THRESH_LO_MASK |
                           XCVR_AGC_CTRL_2_BBF_PDET_THRESH_HI_MASK |
                           XCVR_AGC_CTRL_2_BBF_PDET_THRESH_LO_MASK |
                           XCVR_AGC_CTRL_2_BBF_GAIN_SETTLE_TIME_MASK
                          )) | (uint32_t)(
                           (AGC_FAST_EXPIRE_def_c << XCVR_AGC_CTRL_2_AGC_FAST_EXPIRE_SHIFT) |
                           (TZA_PDET_THRESH_HI_def_c << XCVR_AGC_CTRL_2_TZA_PDET_THRESH_HI_SHIFT) |
                           (TZA_PDET_THRESH_LO_def_c << XCVR_AGC_CTRL_2_TZA_PDET_THRESH_LO_SHIFT) |
                           (BBF_PDET_THRESH_HI_def_c << XCVR_AGC_CTRL_2_BBF_PDET_THRESH_HI_SHIFT) |
                           (BBF_PDET_THRESH_LO_def_c << XCVR_AGC_CTRL_2_BBF_PDET_THRESH_LO_SHIFT) |
                           (BBF_GAIN_SETTLE_TIME_def_c << XCVR_AGC_CTRL_2_BBF_GAIN_SETTLE_TIME_SHIFT)
                          ));
    
    /* AGC_CTRL_3 */
    XCVR_AGC_CTRL_3 = (uint32_t)((XCVR_AGC_CTRL_3 & (uint32_t)~(uint32_t)(
                           XCVR_AGC_CTRL_3_AGC_UP_STEP_SZ_MASK |
                           XCVR_AGC_CTRL_3_AGC_H2S_STEP_SZ_MASK |
                           XCVR_AGC_CTRL_3_AGC_RSSI_DELT_H2S_MASK |
                           XCVR_AGC_CTRL_3_AGC_PDET_LO_DLY_MASK |
                           XCVR_AGC_CTRL_3_AGC_UNFREEZE_TIME_MASK
                          )) | (uint32_t)(
                           (AGC_UP_STEP_SZ_def_c << XCVR_AGC_CTRL_3_AGC_UP_STEP_SZ_SHIFT) |
                           (AGC_H2S_STEP_SZ_def_c << XCVR_AGC_CTRL_3_AGC_H2S_STEP_SZ_SHIFT) |
                           (AGC_RSSI_DELT_H2S_def_c << XCVR_AGC_CTRL_3_AGC_RSSI_DELT_H2S_SHIFT) |
                           (AGC_PDET_LO_DLY_def_c << XCVR_AGC_CTRL_3_AGC_PDET_LO_DLY_SHIFT) |
                           (AGC_UNFREEZE_TIME_def_c << XCVR_AGC_CTRL_3_AGC_UNFREEZE_TIME_SHIFT)
                          ));
    
    /* AGC_GAIN_TBL*** registers */
    XCVR_AGC_GAIN_TBL_03_00 = (uint32_t)(
                           (BBF_GAIN_00_def_c << XCVR_AGC_GAIN_TBL_03_00_BBF_GAIN_00_SHIFT) |
                           (LNM_GAIN_00_def_c << XCVR_AGC_GAIN_TBL_03_00_LNM_GAIN_00_SHIFT) |
                           (BBF_GAIN_01_def_c << XCVR_AGC_GAIN_TBL_03_00_BBF_GAIN_01_SHIFT) |
                           (LNM_GAIN_01_def_c << XCVR_AGC_GAIN_TBL_03_00_LNM_GAIN_01_SHIFT) |
                           (BBF_GAIN_02_def_c << XCVR_AGC_GAIN_TBL_03_00_BBF_GAIN_02_SHIFT) |
                           (LNM_GAIN_02_def_c << XCVR_AGC_GAIN_TBL_03_00_LNM_GAIN_02_SHIFT) |
                           (BBF_GAIN_03_def_c << XCVR_AGC_GAIN_TBL_03_00_BBF_GAIN_03_SHIFT) |
                           (LNM_GAIN_03_def_c << XCVR_AGC_GAIN_TBL_03_00_LNM_GAIN_03_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_07_04 = (uint32_t)(
                           (BBF_GAIN_04_def_c << XCVR_AGC_GAIN_TBL_07_04_BBF_GAIN_04_SHIFT) |
                           (LNM_GAIN_04_def_c << XCVR_AGC_GAIN_TBL_07_04_LNM_GAIN_04_SHIFT) |
                           (BBF_GAIN_05_def_c << XCVR_AGC_GAIN_TBL_07_04_BBF_GAIN_05_SHIFT) |
                           (LNM_GAIN_05_def_c << XCVR_AGC_GAIN_TBL_07_04_LNM_GAIN_05_SHIFT) |
                           (BBF_GAIN_06_def_c << XCVR_AGC_GAIN_TBL_07_04_BBF_GAIN_06_SHIFT) |
                           (LNM_GAIN_06_def_c << XCVR_AGC_GAIN_TBL_07_04_LNM_GAIN_06_SHIFT) |
                           (BBF_GAIN_07_def_c << XCVR_AGC_GAIN_TBL_07_04_BBF_GAIN_07_SHIFT) |
                           (LNM_GAIN_07_def_c << XCVR_AGC_GAIN_TBL_07_04_LNM_GAIN_07_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_11_08 = (uint32_t)(
                           (BBF_GAIN_08_def_c << XCVR_AGC_GAIN_TBL_11_08_BBF_GAIN_08_SHIFT) |
                           (LNM_GAIN_08_def_c << XCVR_AGC_GAIN_TBL_11_08_LNM_GAIN_08_SHIFT) |
                           (BBF_GAIN_09_def_c << XCVR_AGC_GAIN_TBL_11_08_BBF_GAIN_09_SHIFT) |
                           (LNM_GAIN_09_def_c << XCVR_AGC_GAIN_TBL_11_08_LNM_GAIN_09_SHIFT) |
                           (BBF_GAIN_10_def_c << XCVR_AGC_GAIN_TBL_11_08_BBF_GAIN_10_SHIFT) |
                           (LNM_GAIN_10_def_c << XCVR_AGC_GAIN_TBL_11_08_LNM_GAIN_10_SHIFT) |
                           (BBF_GAIN_11_def_c << XCVR_AGC_GAIN_TBL_11_08_BBF_GAIN_11_SHIFT) |
                           (LNM_GAIN_11_def_c << XCVR_AGC_GAIN_TBL_11_08_LNM_GAIN_11_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_15_12 =  (uint32_t)(
                           (BBF_GAIN_12_def_c << XCVR_AGC_GAIN_TBL_15_12_BBF_GAIN_12_SHIFT) |
                           (LNM_GAIN_12_def_c << XCVR_AGC_GAIN_TBL_15_12_LNM_GAIN_12_SHIFT) |
                           (BBF_GAIN_13_def_c << XCVR_AGC_GAIN_TBL_15_12_BBF_GAIN_13_SHIFT) |
                           (LNM_GAIN_13_def_c << XCVR_AGC_GAIN_TBL_15_12_LNM_GAIN_13_SHIFT) |
                           (BBF_GAIN_14_def_c << XCVR_AGC_GAIN_TBL_15_12_BBF_GAIN_14_SHIFT) |
                           (LNM_GAIN_14_def_c << XCVR_AGC_GAIN_TBL_15_12_LNM_GAIN_14_SHIFT) |
                           (BBF_GAIN_15_def_c << XCVR_AGC_GAIN_TBL_15_12_BBF_GAIN_15_SHIFT) |
                           (LNM_GAIN_15_def_c << XCVR_AGC_GAIN_TBL_15_12_LNM_GAIN_15_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_19_16 = (uint32_t)(
                           (BBF_GAIN_16_def_c << XCVR_AGC_GAIN_TBL_19_16_BBF_GAIN_16_SHIFT) |
                           (LNM_GAIN_16_def_c << XCVR_AGC_GAIN_TBL_19_16_LNM_GAIN_16_SHIFT) |
                           (BBF_GAIN_17_def_c << XCVR_AGC_GAIN_TBL_19_16_BBF_GAIN_17_SHIFT) |
                           (LNM_GAIN_17_def_c << XCVR_AGC_GAIN_TBL_19_16_LNM_GAIN_17_SHIFT) |
                           (BBF_GAIN_18_def_c << XCVR_AGC_GAIN_TBL_19_16_BBF_GAIN_18_SHIFT) |
                           (LNM_GAIN_18_def_c << XCVR_AGC_GAIN_TBL_19_16_LNM_GAIN_18_SHIFT) |
                           (BBF_GAIN_19_def_c << XCVR_AGC_GAIN_TBL_19_16_BBF_GAIN_19_SHIFT) |
                           (LNM_GAIN_19_def_c << XCVR_AGC_GAIN_TBL_19_16_LNM_GAIN_19_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_23_20 = (uint32_t)(
                           (BBF_GAIN_20_def_c << XCVR_AGC_GAIN_TBL_23_20_BBF_GAIN_20_SHIFT) |
                           (LNM_GAIN_20_def_c << XCVR_AGC_GAIN_TBL_23_20_LNM_GAIN_20_SHIFT) |
                           (BBF_GAIN_21_def_c << XCVR_AGC_GAIN_TBL_23_20_BBF_GAIN_21_SHIFT) |
                           (LNM_GAIN_21_def_c << XCVR_AGC_GAIN_TBL_23_20_LNM_GAIN_21_SHIFT) |
                           (BBF_GAIN_22_def_c << XCVR_AGC_GAIN_TBL_23_20_BBF_GAIN_22_SHIFT) |
                           (LNM_GAIN_22_def_c << XCVR_AGC_GAIN_TBL_23_20_LNM_GAIN_22_SHIFT) |
                           (BBF_GAIN_23_def_c << XCVR_AGC_GAIN_TBL_23_20_BBF_GAIN_23_SHIFT) |
                           (LNM_GAIN_23_def_c << XCVR_AGC_GAIN_TBL_23_20_LNM_GAIN_23_SHIFT) 
                          );
    
    XCVR_AGC_GAIN_TBL_26_24 = (uint32_t)(
                           (BBF_GAIN_24_def_c << XCVR_AGC_GAIN_TBL_26_24_BBF_GAIN_24_SHIFT) |
                           (LNM_GAIN_24_def_c << XCVR_AGC_GAIN_TBL_26_24_LNM_GAIN_24_SHIFT) |
                           (BBF_GAIN_25_def_c << XCVR_AGC_GAIN_TBL_26_24_BBF_GAIN_25_SHIFT) |
                           (LNM_GAIN_25_def_c << XCVR_AGC_GAIN_TBL_26_24_LNM_GAIN_25_SHIFT) |
                           (BBF_GAIN_26_def_c << XCVR_AGC_GAIN_TBL_26_24_BBF_GAIN_26_SHIFT) |
                           (LNM_GAIN_26_def_c << XCVR_AGC_GAIN_TBL_26_24_LNM_GAIN_26_SHIFT) 
                          );
    
    XCVR_TCA_AGC_VAL_3_0 = (uint32_t)(
                           (TCA_AGC_VAL_3_def_c<< XCVR_TCA_AGC_VAL_3_0_TCA_AGC_VAL_3_SHIFT) |
                           (TCA_AGC_VAL_2_def_c<< XCVR_TCA_AGC_VAL_3_0_TCA_AGC_VAL_2_SHIFT) |
                           (TCA_AGC_VAL_1_def_c<< XCVR_TCA_AGC_VAL_3_0_TCA_AGC_VAL_1_SHIFT) |
                           (TCA_AGC_VAL_0_def_c<< XCVR_TCA_AGC_VAL_3_0_TCA_AGC_VAL_0_SHIFT) 
                          );
    
    XCVR_TCA_AGC_VAL_7_4 = (uint32_t)(
                           (TCA_AGC_VAL_7_def_c<< XCVR_TCA_AGC_VAL_7_4_TCA_AGC_VAL_7_SHIFT) |
                           (TCA_AGC_VAL_6_def_c<< XCVR_TCA_AGC_VAL_7_4_TCA_AGC_VAL_6_SHIFT) |
                           (TCA_AGC_VAL_5_def_c<< XCVR_TCA_AGC_VAL_7_4_TCA_AGC_VAL_5_SHIFT) |
                           (TCA_AGC_VAL_4_def_c<< XCVR_TCA_AGC_VAL_7_4_TCA_AGC_VAL_4_SHIFT) 
                          );
    
    XCVR_TCA_AGC_VAL_8 = (uint32_t)(
                           (TCA_AGC_VAL_8_def_c<< XCVR_TCA_AGC_VAL_8_TCA_AGC_VAL_8_SHIFT) 
                          );
    
    XCVR_BBF_RES_TUNE_VAL_7_0 = (uint32_t)(
                           (BBF_RES_TUNE_VAL_7_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_7_SHIFT) |
                           (BBF_RES_TUNE_VAL_6_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_6_SHIFT) |
                           (BBF_RES_TUNE_VAL_5_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_5_SHIFT) |
                           (BBF_RES_TUNE_VAL_4_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_4_SHIFT) |
                           (BBF_RES_TUNE_VAL_3_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_3_SHIFT) |
                           (BBF_RES_TUNE_VAL_2_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_2_SHIFT) |
                           (BBF_RES_TUNE_VAL_1_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_1_SHIFT) |
                           (BBF_RES_TUNE_VAL_0_def_c<< XCVR_BBF_RES_TUNE_VAL_7_0_BBF_RES_TUNE_VAL_0_SHIFT) 
                          );
  
    XCVR_BBF_RES_TUNE_VAL_10_8 = (uint32_t)(
                           (BBF_RES_TUNE_VAL_10_def_c<< XCVR_BBF_RES_TUNE_VAL_10_8_BBF_RES_TUNE_VAL_10_SHIFT) |
                           (BBF_RES_TUNE_VAL_9_def_c<< XCVR_BBF_RES_TUNE_VAL_10_8_BBF_RES_TUNE_VAL_9_SHIFT) |
                           (BBF_RES_TUNE_VAL_8_def_c<< XCVR_BBF_RES_TUNE_VAL_10_8_BBF_RES_TUNE_VAL_8_SHIFT) 
                          );
    
    /* Program DCOC registers */
    XcvrCalcSetupDcoc();
    
    /* Use mode switch routine to setup the defaults that depend on radio mode */
    XcvrSetRxDigDef_ModeSwitch(filt_coeff_ptr, iir3a_idx, iir2a_idx, iir1a_idx, rssi_hold_src);
}

/*! *********************************************************************************
* \brief  This function sets the radio mode dependent default values of the RX DIG registers.
*
* \param[in] in_filt_coeff_ptr pointer to an array of 8 UINT8_T receive filter coefficients.
* \param[in] iir3a_idx value of the 3rd IIR index
* \param[in] iir2a_idx value of the 2nd IIR index
* \param[in] iir1a_idx value of the 1st IIR index
*
* \ingroup PrivateAPIs
*
* \details Sets up RX digital registers with command and mode specific settings.
*
* \note Only uses read-modify-write when only part of the register is being written.
*
**********************************************************************************/
void XcvrSetRxDigDef_ModeSwitch (  const uint8_t * in_filt_coeff_ptr, uint8_t iir3a_idx, uint8_t iir2a_idx, uint8_t iir1a_idx, uint8_t rssi_hold_src )
{
    uint8_t * filt_coeff_ptr = (uint8_t *)in_filt_coeff_ptr;
    
    /*RX_CHF_COEFn*/
    XCVR_RX_CHF_COEF0 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF1 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF2 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF3 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF4 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF5 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF6 = *filt_coeff_ptr++;
    XCVR_RX_CHF_COEF7 = *filt_coeff_ptr;    
    
    /* DCOC_CAL_IIR */
    XCVR_DCOC_CAL_IIR = (uint32_t)((XCVR_DCOC_CAL_IIR & (uint32_t)~(uint32_t)(
                         XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_MASK |
                         XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_MASK |
                         XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_MASK 
                        )) | (uint32_t)(
                         (iir1a_idx << XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_SHIFT) |
                         (iir2a_idx << XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_SHIFT) |
                         (iir3a_idx << XCVR_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_SHIFT) 
                        ));    
  
    XCVR_BWR_RSSI_CTRL_0_RSSI_HOLD_SRC(XCVR, rssi_hold_src);
}

/*! *********************************************************************************
* \brief  This function programs a set of DCOC registers either from raw values
*    or from calculated (equations) values.
*
* \return Status of the operation.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
xcvrStatus_t XcvrCalcSetupDcoc ( void )
{
#if !USE_DCOC_MAGIC_NUMBERS 
    
    /* Define variables to replace all of the #defined constants which are used
     * below and which are defined in BLEDefaults.h for the case where
     * USE_DCOC_MAGIC_NUMBERS == 1 
     */
    
    /* DCOC_CAL_RCP */
#define ALPHA_CALC_RECIP_def_c 0x00
#define TMP_CALC_RECIP_def_c   0x00
    
    /* TCA_AGC_LIN_VAL_2_0 */
    uint16_t TCA_AGC_LIN_VAL_0_def_c;
    uint16_t TCA_AGC_LIN_VAL_1_def_c;
    uint16_t TCA_AGC_LIN_VAL_2_def_c;
    
    /* TCA_AGC_LIN_VAL_5_3 */
    uint16_t TCA_AGC_LIN_VAL_3_def_c;
    uint16_t TCA_AGC_LIN_VAL_4_def_c;
    uint16_t TCA_AGC_LIN_VAL_5_def_c;
    
    /* TCA_AGC_LIN_VAL_8_6 */
    uint16_t TCA_AGC_LIN_VAL_6_def_c;
    uint16_t TCA_AGC_LIN_VAL_7_def_c;
    uint16_t TCA_AGC_LIN_VAL_8_def_c;
    
    /* BBF_RES_TUNE_LIN_VAL_3_0 */
    uint8_t BBF_RES_TUNE_LIN_VAL_0_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_1_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_2_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_3_def_c;
    
    /* BBF_RES_TUNE_LIN_VAL_7_4 */
    uint8_t BBF_RES_TUNE_LIN_VAL_4_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_5_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_6_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_7_def_c;
    
    /* BBF_RES_TUNE_LIN_VAL_10_8 */
    uint8_t BBF_RES_TUNE_LIN_VAL_8_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_9_def_c;
    uint8_t BBF_RES_TUNE_LIN_VAL_10_def_c;
    
    /* DCOC_TZA_STEP */
    uint16_t DCOC_TZA_STEP_GAIN_00_def_c;
    uint16_t DCOC_TZA_STEP_RCP_00_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_01_def_c;
    uint16_t DCOC_TZA_STEP_RCP_01_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_02_def_c;
    uint16_t DCOC_TZA_STEP_RCP_02_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_03_def_c;
    uint16_t DCOC_TZA_STEP_RCP_03_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_04_def_c;
    uint16_t DCOC_TZA_STEP_RCP_04_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_05_def_c;
    uint16_t DCOC_TZA_STEP_RCP_05_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_06_def_c;
    uint16_t DCOC_TZA_STEP_RCP_06_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_07_def_c;
    uint16_t DCOC_TZA_STEP_RCP_07_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_08_def_c;
    uint16_t DCOC_TZA_STEP_RCP_08_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_09_def_c;
    uint16_t DCOC_TZA_STEP_RCP_09_def_c;
    uint16_t DCOC_TZA_STEP_GAIN_10_def_c;
    uint16_t DCOC_TZA_STEP_RCP_10_def_c;
    
    /* DCOC_CTRL_1 DCOC_CTRL_2 */
    uint16_t BBF_STEP_def_c;
    uint16_t BBF_STEP_RECIP_def_c;
    
    /* DCOC_CAL_RCP */
    uint16_t RCP_GLHmGLLxGBL_def_c;
    uint16_t RCP_GBHmGBL_def_c;
    
    /* Equations to calculate these values */
    TCA_AGC_LIN_VAL_0_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_0_def_c));
    TCA_AGC_LIN_VAL_1_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_1_def_c));
    TCA_AGC_LIN_VAL_2_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_2_def_c));
    
    TCA_AGC_LIN_VAL_3_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_3_def_c));
    TCA_AGC_LIN_VAL_4_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_4_def_c));
    TCA_AGC_LIN_VAL_5_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_5_def_c));
    
    TCA_AGC_LIN_VAL_6_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_6_def_c));
    TCA_AGC_LIN_VAL_7_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_7_def_c));
    TCA_AGC_LIN_VAL_8_def_c = (uint16_t)round(0x4*DB_TO_LINEAR(TCA_GAIN_DB_8_def_c));
    
    BBF_RES_TUNE_LIN_VAL_0_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_0_def_c));
    BBF_RES_TUNE_LIN_VAL_1_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_1_def_c));
    BBF_RES_TUNE_LIN_VAL_2_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_2_def_c));
    BBF_RES_TUNE_LIN_VAL_3_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_3_def_c));
    
    BBF_RES_TUNE_LIN_VAL_4_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_4_def_c));
    BBF_RES_TUNE_LIN_VAL_5_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_5_def_c));
    BBF_RES_TUNE_LIN_VAL_6_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_6_def_c));
    BBF_RES_TUNE_LIN_VAL_7_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_7_def_c));
    
    BBF_RES_TUNE_LIN_VAL_8_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_8_def_c));
    BBF_RES_TUNE_LIN_VAL_9_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_9_def_c));
    
    if ((0x8*DB_TO_LINEAR(BBF_GAIN_DB_10_def_c)) > 255)
    {
        BBF_RES_TUNE_LIN_VAL_10_def_c = (uint8_t)(0xFF);
    }
    else
    {
        BBF_RES_TUNE_LIN_VAL_10_def_c = (uint8_t)round(0x8*DB_TO_LINEAR(BBF_GAIN_DB_10_def_c));
    }
    
    if (gen1_dcgain_trims_enabled)
    {
        double temp_prd = TZA_DCOC_STEP_RAW * adc_gain_trimmed;
        DCOC_TZA_STEP_GAIN_00_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_0_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_00_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_0_def_c)));
        DCOC_TZA_STEP_GAIN_01_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_1_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_01_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_1_def_c)));
        DCOC_TZA_STEP_GAIN_02_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_2_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_02_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_2_def_c)));
        DCOC_TZA_STEP_GAIN_03_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_3_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_03_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_3_def_c)));
        DCOC_TZA_STEP_GAIN_04_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_4_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_04_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_4_def_c)));
        DCOC_TZA_STEP_GAIN_05_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_5_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_05_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_5_def_c)));
        DCOC_TZA_STEP_GAIN_06_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_6_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_06_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_6_def_c)));
        DCOC_TZA_STEP_GAIN_07_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_7_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_07_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_7_def_c)));
        DCOC_TZA_STEP_GAIN_08_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_8_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_08_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_8_def_c)));
        DCOC_TZA_STEP_GAIN_09_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_9_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_09_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_9_def_c)));
        DCOC_TZA_STEP_GAIN_10_def_c = (uint16_t)round(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_10_def_c) * 0x8);
        DCOC_TZA_STEP_RCP_10_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(temp_prd * DB_TO_LINEAR(BBF_GAIN_DB_10_def_c)));
        
        BBF_STEP_def_c = (uint16_t)round(BBF_DCOC_STEP_RAW * adc_gain_trimmed *0x8);
        BBF_STEP_RECIP_def_c = (uint16_t)round((((double)(1.0))*(0x8000))/(BBF_DCOC_STEP_RAW * adc_gain_trimmed));
    }
    
    RCP_GLHmGLLxGBL_def_c = (uint16_t)(round(((double)(2048*1.0))/((DB_TO_LINEAR(TCA_GAIN_DB_8_def_c)-DB_TO_LINEAR(TCA_GAIN_DB_4_def_c))*DB_TO_LINEAR(BBF_GAIN_DB_4_def_c))));
    RCP_GBHmGBL_def_c = (uint16_t)(round(((double)(1024*1.0))/((DB_TO_LINEAR(BBF_GAIN_DB_8_def_c)-DB_TO_LINEAR(BBF_GAIN_DB_4_def_c)))));
    
#endif /* !USE_DCOC_MAGIC_NUMBERS */
    
    /* Write the registers with either the raw or calculated values.
    * NOTE: Values will be either #define constants or local variables.
    */
    
    /* Miscellaneous DCOC Tracking & GearShift Control Settings (Misc Registers) */
    XCVR_ADC_TEST_CTRL = (uint32_t)((XCVR_ADC_TEST_CTRL & (uint32_t)~(uint32_t)(
                          XCVR_ADC_TEST_CTRL_DCOC_ALPHA_RADIUS_GS_IDX_MASK
                         )) | (uint32_t)(
                          (DCOC_ALPHA_RADIUS_GS_IDX_def_c << XCVR_ADC_TEST_CTRL_DCOC_ALPHA_RADIUS_GS_IDX_SHIFT)
                         ));
    
    XCVR_RX_ANA_CTRL = (uint32_t)((XCVR_RX_ANA_CTRL & (uint32_t)~(uint32_t)(
                          XCVR_RX_ANA_CTRL_IQMC_DC_GAIN_ADJ_EN_MASK
                         )) | (uint32_t)(
                          (IQMC_DC_GAIN_ADJ_EN_def_c << XCVR_RX_ANA_CTRL_IQMC_DC_GAIN_ADJ_EN_SHIFT)
                         ));
    
    XCVR_ANA_SPARE = (uint32_t)((XCVR_ANA_SPARE & (uint32_t)~(uint32_t)(
                          XCVR_ANA_SPARE_DCOC_TRK_EST_GS_CNT_MASK |
                          XCVR_ANA_SPARE_HPM_LSB_INVERT_MASK
                         )) | (uint32_t)(
                          (DCOC_TRK_EST_GS_CNT_def_c << XCVR_ANA_SPARE_DCOC_TRK_EST_GS_CNT_SHIFT) |
                          (HPM_LSB_INVERT_def_c << XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT)
                         ));
      
    /* DCOC_CAL_GAIN */
    XCVR_DCOC_CAL_GAIN = (uint32_t)((XCVR_DCOC_CAL_GAIN & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN1_MASK |
                          XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN1_MASK |
                          XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN2_MASK |
                          XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN2_MASK |
                          XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN3_MASK |
                          XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN3_MASK 
                         )) | (uint32_t)(
                          (DCOC_BBF_CAL_GAIN1_def_c << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN1_SHIFT) |
                          (DCOC_TZA_CAL_GAIN1_def_c << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN1_SHIFT) |
                          (DCOC_BBF_CAL_GAIN2_def_c << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN2_SHIFT) |
                          (DCOC_TZA_CAL_GAIN2_def_c << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN2_SHIFT) |
                          (DCOC_BBF_CAL_GAIN3_def_c << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN3_SHIFT) |
                          (DCOC_TZA_CAL_GAIN3_def_c << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN3_SHIFT) 
                         ));    
  
    /* DCOC_CALC_RCP */
    XCVR_DCOC_CAL_RCP = (uint32_t)((XCVR_DCOC_CAL_RCP & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_MASK |
                          XCVR_DCOC_CAL_RCP_ALPHA_CALC_RECIP_MASK 
                         )) | (uint32_t)(
                          (TMP_CALC_RECIP_def_c << XCVR_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_SHIFT) |
                          (ALPHA_CALC_RECIP_def_c << XCVR_DCOC_CAL_RCP_ALPHA_CALC_RECIP_SHIFT) 
                         ));    

    /* TCA_AGC_LIN_VAL_2_0 */
    XCVR_TCA_AGC_LIN_VAL_2_0 = (uint32_t)((XCVR_TCA_AGC_LIN_VAL_2_0 & (uint32_t)~(uint32_t)(
                          XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_0_MASK |
                          XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_1_MASK |
                          XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_2_MASK 
                         )) | (uint32_t)(
                          (TCA_AGC_LIN_VAL_0_def_c << XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_0_SHIFT) |
                          (TCA_AGC_LIN_VAL_1_def_c << XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_1_SHIFT) |
                          (TCA_AGC_LIN_VAL_2_def_c << XCVR_TCA_AGC_LIN_VAL_2_0_TCA_AGC_LIN_VAL_2_SHIFT)
                         ));    

    /* TCA_AGC_LIN_VAL_5_3 */    
    XCVR_TCA_AGC_LIN_VAL_5_3 = (uint32_t)((XCVR_TCA_AGC_LIN_VAL_5_3 & (uint32_t)~(uint32_t)(
                          XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_3_MASK |
                          XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_4_MASK |
                          XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_5_MASK 
                         )) | (uint32_t)(
                          (TCA_AGC_LIN_VAL_3_def_c << XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_3_SHIFT) |
                          (TCA_AGC_LIN_VAL_4_def_c << XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_4_SHIFT) |
                          (TCA_AGC_LIN_VAL_5_def_c << XCVR_TCA_AGC_LIN_VAL_5_3_TCA_AGC_LIN_VAL_5_SHIFT)
                         ));    

    /* TCA_AGC_LIN_VAL_8_6 */        
    XCVR_TCA_AGC_LIN_VAL_8_6 = (uint32_t)((XCVR_TCA_AGC_LIN_VAL_8_6 & (uint32_t)~(uint32_t)(
                          XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_6_MASK |
                          XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_7_MASK |
                          XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_8_MASK 
                         )) | (uint32_t)(
                          (TCA_AGC_LIN_VAL_6_def_c << XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_6_SHIFT) |
                          (TCA_AGC_LIN_VAL_7_def_c << XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_7_SHIFT) |
                          (TCA_AGC_LIN_VAL_8_def_c << XCVR_TCA_AGC_LIN_VAL_8_6_TCA_AGC_LIN_VAL_8_SHIFT)
                         ));    

    /* BBF_RES_TUNE_LIN_VAL_3_0 */
    XCVR_BBF_RES_TUNE_LIN_VAL_3_0 = (uint32_t)((XCVR_BBF_RES_TUNE_LIN_VAL_3_0 & (uint32_t)~(uint32_t)(
                          XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_0_MASK  |
                          XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_1_MASK  |
                          XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_2_MASK  |
                          XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_3_MASK 
                         )) | (uint32_t)(
                          (BBF_RES_TUNE_LIN_VAL_0_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_0_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_1_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_1_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_2_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_2_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_3_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_3_0_BBF_RES_TUNE_LIN_VAL_3_SHIFT)
                         ));    
      
    /* BBF_RES_TUNE_LIN_VAL_7_4 */  
    XCVR_BBF_RES_TUNE_LIN_VAL_7_4 = (uint32_t)((XCVR_BBF_RES_TUNE_LIN_VAL_7_4 & (uint32_t)~(uint32_t)(
                          XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_4_MASK |
                          XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_5_MASK |
                          XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_6_MASK |
                          XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_7_MASK
                         )) | (uint32_t)(
                          (BBF_RES_TUNE_LIN_VAL_4_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_4_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_5_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_5_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_6_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_6_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_7_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_7_4_BBF_RES_TUNE_LIN_VAL_7_SHIFT)
                         ));    

    /* BBF_RES_TUNE_LIN_VAL_10_8 */
    XCVR_BBF_RES_TUNE_LIN_VAL_10_8 = (uint32_t)((XCVR_BBF_RES_TUNE_LIN_VAL_10_8 & (uint32_t)~(uint32_t)(
                          XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_8_MASK |
                          XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_9_MASK |
                          XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_10_MASK
                         )) | (uint32_t)(
                          (BBF_RES_TUNE_LIN_VAL_8_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_8_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_9_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_9_SHIFT) |
                          (BBF_RES_TUNE_LIN_VAL_10_def_c << XCVR_BBF_RES_TUNE_LIN_VAL_10_8_BBF_RES_TUNE_LIN_VAL_10_SHIFT)
                         ));    

    if (gen1_dcgain_trims_enabled) /* Only run this code when Gen 1 DC GAIN trims are being used. */
    {
        /* DCOC_TZA_STEP_GAIN & RCP 00-10 */
        XCVR_DCOC_TZA_STEP_00 = (uint32_t)((XCVR_DCOC_TZA_STEP_00 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_00_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_00_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_01 = (uint32_t)((XCVR_DCOC_TZA_STEP_01 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_01_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_01_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_02 = (uint32_t)((XCVR_DCOC_TZA_STEP_02 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_02_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_02_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_03 = (uint32_t)((XCVR_DCOC_TZA_STEP_03 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_03_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_03_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_04 = (uint32_t)((XCVR_DCOC_TZA_STEP_04 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_04_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_04_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_05 = (uint32_t)((XCVR_DCOC_TZA_STEP_05 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_05_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_05_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_06 = (uint32_t)((XCVR_DCOC_TZA_STEP_06 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_06_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_06_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_07 = (uint32_t)((XCVR_DCOC_TZA_STEP_07 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_07_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_07_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_08 = (uint32_t)((XCVR_DCOC_TZA_STEP_08 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_08_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_08_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_09 = (uint32_t)((XCVR_DCOC_TZA_STEP_09 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_09_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_09_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    

        XCVR_DCOC_TZA_STEP_10 = (uint32_t)((XCVR_DCOC_TZA_STEP_10 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_MASK |
                          XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_MASK 
                         )) | (uint32_t)(
                          (DCOC_TZA_STEP_RCP_10_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_RCP_SHIFT) |
                          (DCOC_TZA_STEP_GAIN_10_def_c << XCVR_DCOC_TZA_STEP__DCOC_TZA_STEP_GAIN_SHIFT) 
                         ));    
    }
   
    /* DCOC_CTRL_1 */
    XCVR_DCOC_CTRL_1 = (uint32_t)((XCVR_DCOC_CTRL_1 & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_CTRL_1_TRACK_FROM_ZERO_MASK |
                          XCVR_DCOC_CTRL_1_BBA_CORR_POL_MASK |
                          XCVR_DCOC_CTRL_1_TZA_CORR_POL_MASK 
                         )) | (uint32_t)(
                          (TRACK_FROM_ZERO_def_c << XCVR_DCOC_CTRL_1_TRACK_FROM_ZERO_SHIFT) |
                          (BBF_CORR_POL_def_c << XCVR_DCOC_CTRL_1_BBA_CORR_POL_SHIFT) |
                          (TZA_CORR_POL_def_c << XCVR_DCOC_CTRL_1_TZA_CORR_POL_SHIFT)
                         ));

    if (gen1_dcgain_trims_enabled)
    {
        XCVR_BWR_DCOC_CTRL_1_BBF_DCOC_STEP(XCVR, BBF_STEP_def_c); /* Only write this field if Gen1 DC GAIN trims being used. */
    }
    
    /* DCOC_CTRL_2 */
    if (gen1_dcgain_trims_enabled)
    {
        XCVR_BWR_DCOC_CTRL_2_BBF_DCOC_STEP_RECIP(XCVR, BBF_STEP_RECIP_def_c);
    }
    
    /* DCOC_CAL_RCP */      
    XCVR_DCOC_CAL_RCP = (uint32_t)((XCVR_DCOC_CAL_RCP & (uint32_t)~(uint32_t)(
                          XCVR_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_MASK |
                          XCVR_DCOC_CAL_RCP_ALPHA_CALC_RECIP_MASK 
                         )) | (uint32_t)(
                          (RCP_GBHmGBL_def_c << XCVR_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_SHIFT) |
                          (RCP_GLHmGLLxGBL_def_c << XCVR_DCOC_CAL_RCP_ALPHA_CALC_RECIP_SHIFT) 
                         ));    
    
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function sets up the TX_DIG and PLL settings 
*
* \param[in] radioMode - the operating mode for the radio to be configured
*
* \return status of the operation.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrSetTxDigPLLDefaults( radio_mode_t radioMode )
{
    /* Configure the High Port Sigma Delta */
    XCVR_BWR_ANA_SPARE_HPM_LSB_INVERT(XCVR, HPM_LSB_INVERT_def_c);
    XCVR_BWR_PLL_HPM_SDM_FRACTION_HPM_DENOM(XCVR, HPM_DENOM_def_c);
    
    /* Configure the PLL Modulation Delays */
    XCVR_BWR_PLL_DELAY_MATCH_HPM_BANK_DELAY(XCVR, HPM_BANK_DELAY_def_c);
    XCVR_BWR_PLL_DELAY_MATCH_HPM_SDM_DELAY(XCVR, HPM_SDM_DELAY_def_c);
    XCVR_BWR_PLL_DELAY_MATCH_LP_SDM_DELAY(XCVR, LP_SDM_DELAY_def_c);
    
    /* Configure PLL Loop Filter Configuration from Default value */
    XCVR_BWR_PLL_CTRL_PLL_LFILT_CNTL(XCVR, PLL_LFILT_CNTL_def_c);                       
    
    /* Change FSK_MODULATION_SCALE_1 from it Default value */
    XCVR_BWR_TX_FSK_MOD_SCALE_FSK_MODULATION_SCALE_1(XCVR, FSK_MODULATION_SCALE_1_def_c);     
    
    /* PLL Dithering */
    XCVR_BWR_PLL_LP_MOD_CTRL_LPM_D_OVRD(XCVR, 1); /* Enable over-riding dither control */
    XCVR_BWR_PLL_LP_MOD_CTRL_LPM_DTH_SCL(XCVR, 0x8); /* Set dither range */
    XCVR_BWR_PLL_LP_MOD_CTRL_LPM_D_CTRL(XCVR, 1); /* Turn on dither all the time */

    /* Use mode switch routine to setup the defaults that depend on radio mode (none at this time) */
    XcvrSetTxDigPLLDef_ModeSwitch(radioMode);
}

/*! *********************************************************************************
* \brief  This function sets up the TX_DIG and PLL radio mode specific settings 
*
* \param[in] radioMode - the operating mode for the radio to be configured
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrSetTxDigPLLDef_ModeSwitch( radio_mode_t radioMode )
{
    
}

/*! *********************************************************************************
* \brief  This function sets up the analog settings 
*
* \param[in] radioMode - the operating mode for the radio to be configured
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrSetAnalogDefaults ( radio_mode_t radioMode )
{
    /* Use mode switch routine to setup the defaults that depend on radio mode */
    XcvrSetAnalogDef_ModeSwitch(radioMode);
    
    /* Regulator trim and ADC trims */
    XCVR_BWR_PLL_CTRL2_PLL_VCO_REG_SUPPLY(XCVR, 1);
    XCVR_BWR_ADC_TUNE_ADC_R1_TUNE(XCVR, 5);
    XCVR_BWR_ADC_TUNE_ADC_R2_TUNE(XCVR, 5);
    XCVR_BWR_ADC_TUNE_ADC_C1_TUNE(XCVR, 8);
    XCVR_BWR_ADC_TUNE_ADC_C2_TUNE(XCVR, 8);
    
    XCVR_BWR_ADC_ADJ_ADC_IB_OPAMP1_ADJ(XCVR, 5);
    XCVR_BWR_ADC_ADJ_ADC_IB_OPAMP2_ADJ(XCVR, 7);
    XCVR_BWR_ADC_ADJ_ADC_IB_DAC1_ADJ(XCVR, 2);
    XCVR_BWR_ADC_ADJ_ADC_IB_DAC2_ADJ(XCVR, 2);
    XCVR_BWR_ADC_ADJ_ADC_IB_FLSH_ADJ(XCVR, 6);
    XCVR_BWR_ADC_ADJ_ADC_FLSH_RES_ADJ(XCVR, 0);    
  
    XCVR_BWR_ADC_TRIMS_ADC_IREF_OPAMPS_RES_TRIM(XCVR, 2);
    XCVR_BWR_ADC_TRIMS_ADC_IREF_FLSH_RES_TRIM(XCVR, 4);
    XCVR_BWR_ADC_TRIMS_ADC_VCM_TRIM(XCVR, 4);
  
    /* Raise QGEN and ADC_ANA and ADC_DIG supplies */
    XCVR_BWR_ADC_REGS_ADC_ANA_REG_SUPPLY(XCVR, 9);
    XCVR_BWR_ADC_REGS_ADC_REG_DIG_SUPPLY(XCVR, 9);
    XCVR_BWR_QGEN_CTRL_QGEN_REG_SUPPLY(XCVR, 9);
}

/*! *********************************************************************************
* \brief  This function sets up the radio mode specific analog settings 
*
* \param[in] radioMode - the operating mode for the radio to be configured
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrSetAnalogDef_ModeSwitch ( radio_mode_t radioMode )
{
    switch (radioMode)
    {
    case BLE:
        /* TZA_CTRL for BLE */
        XCVR_BWR_TZA_CTRL_TZA_CAP_TUNE(XCVR, ble_tza_cap_tune);
        
        /* BBF_CTRL for BLE */
        XCVR_BBF_CTRL = (uint32_t)((XCVR_BBF_CTRL & (uint32_t)~(uint32_t)(
                      XCVR_BBF_CTRL_BBF_CAP_TUNE_MASK |
                      XCVR_BBF_CTRL_BBF_RES_TUNE2_MASK |
                      XCVR_BBF_CTRL_DCOC_ALPHAC_SCALE_GS_IDX_MASK
                     )) | (uint32_t)(
                      (ble_bbf_cap_tune << XCVR_BBF_CTRL_BBF_CAP_TUNE_SHIFT) |
                      (ble_bbf_res_tune2 << XCVR_BBF_CTRL_BBF_RES_TUNE2_SHIFT) |
                      (DCOC_ALPHAC_SCALE_GS_IDX_def_c << XCVR_BBF_CTRL_DCOC_ALPHAC_SCALE_GS_IDX_SHIFT)
                     )); 
        break;
    case ZIGBEE:
        /* TZA_CTRL for Zigbee */
        XCVR_BWR_TZA_CTRL_TZA_CAP_TUNE(XCVR, zb_tza_cap_tune);
        
        /* BBF_CTRL for Zigbee */
        XCVR_BBF_CTRL = (uint32_t)((XCVR_BBF_CTRL & (uint32_t)~(uint32_t)(
                      XCVR_BBF_CTRL_BBF_CAP_TUNE_MASK |
                      XCVR_BBF_CTRL_BBF_RES_TUNE2_MASK |
                      XCVR_BBF_CTRL_DCOC_ALPHAC_SCALE_GS_IDX_MASK
                     )) | (uint32_t)(
                      (zb_bbf_cap_tune << XCVR_BBF_CTRL_BBF_CAP_TUNE_SHIFT) |
                      (zb_bbf_res_tune2 << XCVR_BBF_CTRL_BBF_RES_TUNE2_SHIFT) |
                      (DCOC_ALPHAC_SCALE_GS_IDX_def_c << XCVR_BBF_CTRL_DCOC_ALPHAC_SCALE_GS_IDX_SHIFT)
                     ));
        break;
    default:
        break;
    }
}

/*! *********************************************************************************
* \brief  This function controls the enable/disable of RSSI IIR narrowband filter
*  used in Zigbee CCA tests for narrowband RSSI measurement.
*
* \param[in] IIRnbEnable - enable or disable the narrowband IIR filter
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
void XcvrEnaNBRSSIMeas( bool_t IIRnbEnable )
{
    if (IIRnbEnable)
    {
        /* enable narrowband IIR filter for RSSI */
        XCVR_BWR_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT(XCVR, RSSI_IIR_CW_WEIGHT_ENABLEDdef_c);
    }
    else
    {
        /* Disable narrowband IIR filter for RSSI */
        XCVR_BWR_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT(XCVR, RSSI_IIR_CW_WEIGHT_BYPASSEDdef_c);    
    }
}

/* Customer level trim functions */
/*! *********************************************************************************
* \brief  This function sets the XTAL trim field to control crystal osc frequency.
*
* \param[in] xtalTrim - value to control the trim of the crystal osc.
*
* \return status of the operation.
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
xcvrStatus_t XcvrSetXtalTrim(int8_t xtalTrim)
{
    XCVR_BWR_XTAL_CTRL_XTAL_TRIM(XCVR, xtalTrim);
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function reads the XTAL trim field
*
* \return the current value of the crystal osc trim.
*
* \ingroup PublicAPIs
*
* \details
*
***********************************************************************************/
int8_t  XcvrGetXtalTrim(void)
{
    return (int8_t)XCVR_BRD_XTAL_CTRL_XTAL_TRIM(XCVR);
}

/*! *********************************************************************************
* \brief  This function sets the AGC gain setting table entry and the DCOC table 
*         entry to be used.
*
* \param[in] entry: Entry number from AGC table to use.
*
* \return status of the operation.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
xcvrStatus_t XcvrSetGain ( uint8_t entry )
{
#if INCLUDE_OLD_DRV_CODE
    if (entry > 26)
    {
        return gXcvrInvalidParameters_c;
    }
    
    XCVR_AGC_CTRL_1_WR((uint32_t)((XCVR_AGC_CTRL_1_RD & (uint32_t)~(uint32_t)(    
                          XCVR_AGC_CTRL_1_AGC_IDLE_GAIN_MASK
                         )) | (uint32_t)(
                          (entry << XCVR_AGC_CTRL_1_AGC_IDLE_GAIN_SHIFT)     
                         )));  
#endif  
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function sets the channel for test mode and overrides the BLE and Zigbee channel. 
*
* \param[in] channel: Channel number.
* \param[in] useMappedChannel: Selects to use the channel map table (TRUE) or to use the manual frequency setting based on a SW table of numerator and denominator values for the PLL.
*
* \return status of the operation.
*
* \ingroup PrivateAPIs
*
* \details This function overrides both the BLE and Zigbee channel values from the respective LLs. 
*
***********************************************************************************/
xcvrStatus_t XcvrOverrideChannel ( uint8_t channel, uint8_t useMappedChannel )
{
    if(channel == 0xFF)
    {
        /* Clear all of the overrides and restore to LL channel control */
        
        XCVR_PLL_CHAN_MAP = (uint32_t)((XCVR_PLL_CHAN_MAP & (uint32_t)~(uint32_t)(    
                          XCVR_PLL_CHAN_MAP_CHANNEL_NUM_MASK | 
                          XCVR_PLL_CHAN_MAP_ZOC_MASK | 
                          XCVR_PLL_CHAN_MAP_BOC_MASK
                         )));
        
        /* Stop using the manual frequency setting */
        XCVR_BWR_PLL_LP_SDM_CTRL1_SDM_MAP_DIS(XCVR, 0);
        
        return gXcvrSuccess_c;
    }
    
    if(channel >= 128)
    {
        return gXcvrInvalidParameters_c;
    }
    
    if(useMappedChannel == TRUE)
    {
        XCVR_PLL_CHAN_MAP = (uint32_t)((XCVR_PLL_CHAN_MAP & (uint32_t)~(uint32_t)(    
                          XCVR_PLL_CHAN_MAP_CHANNEL_NUM_MASK | 
                          XCVR_PLL_CHAN_MAP_BOC_MASK |
                          XCVR_PLL_CHAN_MAP_ZOC_MASK
                         )) | (uint32_t)(
                          (channel << XCVR_PLL_CHAN_MAP_CHANNEL_NUM_SHIFT) |
                          (0x01 << XCVR_PLL_CHAN_MAP_BOC_SHIFT) |
                          (0x01 << XCVR_PLL_CHAN_MAP_ZOC_SHIFT)
                         ));         
    }
    else
    {
        XCVR_PLL_CHAN_MAP =  (uint32_t)((XCVR_PLL_CHAN_MAP & (uint32_t)~(uint32_t)(                                
                          XCVR_PLL_CHAN_MAP_BOC_MASK |
                          XCVR_PLL_CHAN_MAP_ZOC_MASK
                         )) | (uint32_t)(
                          (0x01 << XCVR_PLL_CHAN_MAP_BOC_SHIFT) |
                          (0x01 << XCVR_PLL_CHAN_MAP_ZOC_SHIFT)
                         ));  
        
        XCVR_BWR_PLL_LP_SDM_CTRL3_LPM_DENOM(XCVR, gPllDenom_c);
        XCVR_BWR_PLL_LP_SDM_CTRL2_LPM_NUM(XCVR, mapTable[channel].numerator);   
        XCVR_BWR_PLL_LP_SDM_CTRL1_LPM_INTG(XCVR, mapTable[channel].integer);
        
        /* Stop using the LL channel map and use the manual frequency setting */
        XCVR_BWR_PLL_LP_SDM_CTRL1_SDM_MAP_DIS(XCVR, 1);
    }
    
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function sets the frequency at the PLL for test mode and overrides the BLE/ZB channel
*
* \param[in] freq: Frequency in KHz.
* \param[in] refOsc: Osc in MHz.
*
* \return status of the operation.
*
* \ingroup PrivateAPIs
*
* \details

The Manual carrier frequency selected can be calculated using the formula below:
Radio Carrier Frequency = ((Reference Clock Frequency x 2) x (LPM_INTG +
(LPM_NUM / LPM_DENOM))
WARNING : The fraction (LPM_NUM / LPM_DENOM) must be in the range of -0.55
to +0.55 for valid Sigma Delta Modulator operation.

*
***********************************************************************************/
xcvrStatus_t XcvrOverrideFrequency ( uint32_t freq , uint32_t refOsc) 
{ 
    int32_t intg;
    int32_t num;
    int32_t denom = 0x04000000;
    uint32_t sdRate = 64000000;
    double fract_check;
    
    intg = (uint32_t) freq / sdRate;
    
    /* CTUNE Target must be loaded manually +/- 6MHz should be ok for PLL Lock. */
    XCVR_BWR_PLL_CTUNE_CTRL_CTUNE_TARGET_MANUAL(XCVR, freq/1000000);
    XCVR_BWR_PLL_CTUNE_CTRL_CTUNE_TD(XCVR,1);  /*CTUNE Target Disable */
    
    XCVR_PLL_CHAN_MAP |=  XCVR_PLL_CHAN_MAP_BOC_MASK | XCVR_PLL_CHAN_MAP_ZOC_MASK;
    
    XCVR_BWR_PLL_LP_SDM_CTRL1_SDM_MAP_DIS(XCVR, 1);
    
    fract_check = (freq % sdRate);
    fract_check /= sdRate;
    
    if (fract_check >= 0.55) 
    {
        fract_check--;
        intg++;
    }
    
    num = refOsc;
    num = (int32_t) (fract_check * denom);
    
    if (num < 0)
        num = (((1 << 28) - 1) & ((1 << 28) - 1) - ABS(num)) + 1;
    
    XCVR_BWR_PLL_LP_SDM_CTRL1_LPM_INTG(XCVR, intg);
    XCVR_BWR_PLL_LP_SDM_CTRL2_LPM_NUM(XCVR, num);
    XCVR_BWR_PLL_LP_SDM_CTRL3_LPM_DENOM(XCVR, denom);
    
    return gXcvrSuccess_c;
}

/*! *********************************************************************************
* \brief  This function reads the RF PLL values and returns the programmed frequency  
*
* \return Frequency generated by the PLL.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
uint32_t XcvrGetFreq ( void )
{
    uint32_t pll_int, pll_denom;
    int64_t pll_num;
    
    pll_int = XCVR_BRD_PLL_LP_SDM_CTRL1_LPM_INTG(XCVR);                 
    pll_num = XCVR_BRD_PLL_LP_SDM_CTRL2_LPM_NUM(XCVR);
    
    if ( (pll_num & 0x0000000004000000) == 0x0000000004000000)
        pll_num = 0xFFFFFFFFF8000000 + pll_num; /* Sign extend the numerator */  
    
    pll_denom = XCVR_BRD_PLL_LP_SDM_CTRL3_LPM_DENOM(XCVR);   
    
    /* Original formula: ((float) pll_int + ((float)pll_num / (float)pll_denom)) * 64 */
    return  (uint32_t)(pll_int << 6) + ((pll_num << 6) / pll_denom); /* Calculate the frequency in MHz */
}

/*! *********************************************************************************
* \brief  This function overrides the TSM module and starts the RX warmup procedure 
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrForceRxWu ( void )
{
    /* Set "FORCE_RX_EN" in TSM Control register */
    XCVR_BWR_TSM_CTRL_FORCE_RX_EN(XCVR, 1); 
}

/*! *********************************************************************************
* \brief  This function overrides the TSM module and starts the RX warmdown procedure 
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrForceRxWd ( void )
{
    /* Clear "FORCE_RX_EN" in TSM Control register */
    XCVR_BWR_TSM_CTRL_FORCE_RX_EN(XCVR, 0); 
}

/*! *********************************************************************************
* \brief  This function overrides the TSM module and starts the TX warmup procedure 
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrForceTxWu ( void )
{
    XCVR_BWR_TSM_CTRL_FORCE_TX_EN(XCVR, 1);
}

/*! *********************************************************************************
* \brief  This function overrides the TSM module and starts the TX warmdown procedure 
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrForceTxWd ( void )
{
    XCVR_BWR_TSM_CTRL_FORCE_TX_EN(XCVR, 0);
    
    /* Clear "TX_CW_NOMOD" bit in TX Digital Control register */
    XCVR_BWR_TX_DIG_CTRL_DFT_MODE(XCVR, 0); /* Normal radio operation */
}

/*! *********************************************************************************
* \brief  This function performs an unmodulated TX test.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrTxTest ( void )
{
    volatile uint32_t rez[128], i;
    
    for(i=0;i<=127;i++)
    {
        XcvrOverrideChannel (i, 0);
        rez[i] = XcvrGetFreq ();
        XcvrForceTxWu();
        XcvrDelay(30000);
        XcvrForceTxWd();
    }
    
    XcvrOverrideChannel (0xFF, 1);
}

/*! *********************************************************************************
* \brief  Temporary delay function 
*
* \param[in] time the number of counts to decrement through in a wait loop.
*
* \return none.
*
* \ingroup PrivateAPIs
*
* \details
*
***********************************************************************************/
void XcvrDelay(volatile uint32_t time){
    while(time>0){
        time--;
    }
}

/*! *********************************************************************************
* \brief  Manual DCOC calibration function to support board level calibration.
*
* \param[in] chnum Channel number.
*
* \ingroup PrivateAPIs
*
* \details
*   Performs manual DCOC calibration and sets TZA and BBF gains per this calibration.
*   Disables DCOC_CAL_EN to prevent TSM signals from triggering calibration.
*   Intended to enable software development to continue during DCOC cal debug.
*
***********************************************************************************/
void XcvrManualDCOCCal (uint8_t chnum)
{
    static uint8_t DAC_idx;
    static int16_t dc_meas_i;
    static int16_t dc_meas_q;
    static int16_t dc_meas_total;
    static uint8_t curr_min_dc_tza_i_idx, curr_min_dc_tza_q_idx;
    static int16_t curr_min_dc_tza_i, curr_min_dc_tza_q;
    static int16_t curr_min_dc_total;
    static uint8_t curr_min_dc_bbf_i_idx, curr_min_dc_bbf_q_idx;
    static int16_t curr_min_dc_bbf_i, curr_min_dc_bbf_q;
    uint32_t dcoc_ctrl_0_stack;
    uint32_t dcoc_ctrl_1_stack;
    uint32_t dcoc_cal_gain_state;
    uint8_t gearshift_state;
    
    XcvrOverrideChannel(chnum,TRUE);
    
    dcoc_ctrl_0_stack = XCVR_DCOC_CTRL_0; /* Save state of DCOC_CTRL_0 for later restore */
    dcoc_ctrl_1_stack = XCVR_DCOC_CTRL_1; /* Save state of DCOC_CTRL_1 for later restore */
    dcoc_cal_gain_state = XCVR_DCOC_CAL_GAIN; /* Save state of DCOC_CAL_GAIN for later restore */
    gearshift_state = XCVR_BRD_ANA_SPARE_DCOC_TRK_EST_GS_CNT(XCVR); /* Save state of gearshift control for later restore */
    
    /* Set DCOC_CTRL_0, DCOC_CAL_GAIN, and GEARSHIFT to appropriate values for manual nulling of DC */
    XCVR_DCOC_CTRL_0 = ((0x2 << XCVR_DCOC_CTRL_0_DCOC_ALPHAC_SCALE_IDX_SHIFT) |
                        (0x3 << XCVR_DCOC_CTRL_0_DCOC_ALPHA_RADIUS_IDX_SHIFT) |
                        (0x3 << XCVR_DCOC_CTRL_0_DCOC_SIGN_SCALE_IDX_SHIFT) |
                        (0x12 << XCVR_DCOC_CTRL_0_DCOC_CAL_DURATION_SHIFT) |
                        (0x52 << XCVR_DCOC_CTRL_0_DCOC_CORR_DLY_SHIFT) |
                        (0x10 << XCVR_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_SHIFT) |
                        (0x1 << XCVR_DCOC_CTRL_0_DCOC_MAN_SHIFT) |
                        (0x1 << XCVR_DCOC_CTRL_0_DCOC_TRACK_EN_SHIFT) |
                        (0x1 << XCVR_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT) 
                       );   
    XCVR_DCOC_CTRL_1 = ((0x0 << XCVR_DCOC_CTRL_1_TRACK_FROM_ZERO_SHIFT) |
                        (0x1 << XCVR_DCOC_CTRL_1_BBA_CORR_POL_SHIFT) |
                        (0x1 << XCVR_DCOC_CTRL_1_TZA_CORR_POL_SHIFT) 
                       );   
    XCVR_DCOC_CAL_GAIN = ((0x03 << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN1_SHIFT) |
                          (0x02 << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN1_SHIFT) |
                          (0x08 << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN2_SHIFT) |
                          (0x02 << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN2_SHIFT) |
                          (0x03 << XCVR_DCOC_CAL_GAIN_DCOC_TZA_CAL_GAIN3_SHIFT) |
                          (0x08 << XCVR_DCOC_CAL_GAIN_DCOC_BBF_CAL_GAIN3_SHIFT) 
                         );   
    
    XCVR_BWR_ANA_SPARE_DCOC_TRK_EST_GS_CNT(XCVR, 0x00);
    
    /* Search for optimal DC DAC settings */
    XCVR_BWR_DCOC_CTRL_0_DCOC_TRACK_EN(XCVR, 1);   /* DCOC track */
    XCVR_BWR_RX_DIG_CTRL_RX_AGC_EN(XCVR, 0);       /* AGC Control */
    XCVR_BWR_RX_DIG_CTRL_RX_DCOC_EN(XCVR, 1);      /* DCOC_EN */
    XCVR_BWR_RX_DIG_CTRL_RX_DCOC_CAL_EN(XCVR, 0);  /* DCOC_CAL 0=disabled; 1=Enabled */
    XcvrForceRxWu();
    XcvrDelay(2000);
    
    XCVR_BWR_DCOC_CTRL_0_DCOC_MAN(XCVR, 1);        /* Force dcoc dacs to use manual override */
    
    /* Go through gain table to get each setting */
    
    static uint8_t tbl_idx;
    uint8_t * tbl_ptr=(uint8_t *)(&XCVR_AGC_GAIN_TBL_03_00);
    static uint8_t tbl_val;
    for (tbl_idx = 0; tbl_idx <27; tbl_idx++)
    {
        tbl_val = *tbl_ptr;
        tbl_ptr++;
        XCVR_BWR_AGC_CTRL_1_LNM_USER_GAIN(XCVR, ((0xF0&tbl_val)>>4));    /* Set Manual Gain Index for LNM */
        XCVR_BWR_AGC_CTRL_1_BBF_USER_GAIN(XCVR, (0x0F&tbl_val));         /* Set Manual Gain Index for BBF */
        XCVR_BWR_AGC_CTRL_1_USER_LNM_GAIN_EN(XCVR, 1);  /* Use Manual Gain for LNM */
        XCVR_BWR_AGC_CTRL_1_USER_BBF_GAIN_EN(XCVR, 1);  /* Use Manual Gain for BBF */
        
        XcvrDelay(32*3);
        
        /* Init fixed, mid-point values for BBF while sweeping TZA */
        XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_I(XCVR, 0x20); /* Set bbf I to mid */
        XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_Q(XCVR, 0x20); /* Set bbf Q to mid */
        
        /* Measure optimal TZA DAC setting */
        curr_min_dc_tza_i = 2000;
        curr_min_dc_tza_q = 2000;
        curr_min_dc_total = 4000;
        DAC_idx=0;
        
        if(0)  /* Set to 1 for brute force, takes a long time */
        {
            do
            {
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_I(XCVR, (0x00FF & DAC_idx));
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_Q(XCVR, (0xFF00 & DAC_idx)>>8);
                XcvrDelay(32*2);
                /* Take I measurement */
                dc_meas_i = XCVR_BRD_DCOC_DC_EST_DC_EST_I(XCVR);
                dc_meas_i = dc_meas_i | ((dc_meas_i & 0x800) ? 0xF000 : 0x0);
                dc_meas_i = (dc_meas_i < 0) ? (-1*dc_meas_i) : (dc_meas_i);
                
                /* Take Q measurement */
                dc_meas_q = XCVR_BRD_DCOC_DC_EST_DC_EST_Q(XCVR);
                dc_meas_q = dc_meas_q | ((dc_meas_q & 0x800) ? 0xF000 : 0x0);
                dc_meas_q = (dc_meas_q < 0) ? (-1*dc_meas_q) : (dc_meas_q);
                dc_meas_total = dc_meas_i + dc_meas_q;
                if(dc_meas_total < curr_min_dc_total)
                {
                    curr_min_dc_total = dc_meas_total;
                    curr_min_dc_tza_i_idx = (0x00FF & DAC_idx);
                    curr_min_dc_tza_q_idx = (0xFF00 & DAC_idx)>>8;
                }
                
                if(dc_meas_i < curr_min_dc_tza_i)
                {
                    curr_min_dc_tza_i = dc_meas_i;
                }
                if(dc_meas_q < curr_min_dc_tza_q)
                {
                    curr_min_dc_tza_q = dc_meas_q;
                }
                DAC_idx++;
            } while (DAC_idx > 0); /* Relies on 8 bit increment rolling over to zero. */
        }
        else 
        {
            do /* First do I channel because it is aggressor */
            {
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_I(XCVR, DAC_idx);
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_Q(XCVR, 0x80);
                XcvrDelay(32*2);
                
                /* Take I measurement */
                dc_meas_i = XCVR_BRD_DCOC_DC_EST_DC_EST_I(XCVR);
                dc_meas_i = dc_meas_i | ((dc_meas_i & 0x800) ? 0xF000 : 0x0);
                dc_meas_i = (dc_meas_i < 0) ? (-1*dc_meas_i) : (dc_meas_i);
                if(dc_meas_i < curr_min_dc_tza_i)
                {
                    curr_min_dc_tza_i = dc_meas_i;
                    curr_min_dc_tza_i_idx = DAC_idx;
                }
                DAC_idx++;
            }  while (DAC_idx > 0); /* Relies on 8 bit increment rolling over to zero. */
            DAC_idx=0;
            do /* First do Q channel */
            {
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_I(XCVR, curr_min_dc_tza_i_idx);
                XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_Q(XCVR, DAC_idx);
                XcvrDelay(32*2);
                /* Take Q measurement */
                dc_meas_q = XCVR_BRD_DCOC_DC_EST_DC_EST_Q(XCVR);
                dc_meas_q = dc_meas_q | ((dc_meas_q & 0x800) ? 0xF000 : 0x0);
                dc_meas_q = (dc_meas_q < 0) ? (-1*dc_meas_q) : (dc_meas_q);
                if(dc_meas_q < curr_min_dc_tza_q)
                {
                    curr_min_dc_tza_q = dc_meas_q;
                    curr_min_dc_tza_q_idx = DAC_idx;
                }
                DAC_idx++;
            }  while (DAC_idx > 0); /* relies on 8 bit increment rolling over to zero. */
        }
        /* Now set the manual TZA settings from this sweep */
        XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_I(XCVR, curr_min_dc_tza_i_idx);
        XCVR_BWR_DCOC_CTRL_3_TZA_DCOC_INIT_Q(XCVR, curr_min_dc_tza_q_idx);
        curr_min_dc_tza_i_idx = (curr_min_dc_tza_i_idx >= 0x80) ? ((curr_min_dc_tza_i_idx-0x80)) : ((0x80+curr_min_dc_tza_i_idx));
        curr_min_dc_tza_q_idx = (curr_min_dc_tza_q_idx >= 0x80) ? ((curr_min_dc_tza_q_idx-0x80)) : ((0x80+curr_min_dc_tza_q_idx));
        
        /* Measure optimal BBF I DAC setting (I and Q split as there are I->Q DC changes) */
        curr_min_dc_bbf_i = curr_min_dc_tza_i;
        curr_min_dc_bbf_i_idx = 0x20;
        for (DAC_idx=0;DAC_idx<=63;DAC_idx+=1)
        {
            XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_I(XCVR, DAC_idx); /* Adjust I bbf DAC */
            XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_Q(XCVR, 0x20); //set bbf Q to mid
            XcvrDelay(32*2);
            dc_meas_i = XCVR_BRD_DCOC_DC_EST_DC_EST_I(XCVR);    /* Take I measurement */
            dc_meas_i = dc_meas_i | ((dc_meas_i & 0x800) ? 0xF000 : 0x0);
            dc_meas_i = (dc_meas_i < 0) ? (-1*dc_meas_i) : (dc_meas_i);
            if(dc_meas_i < curr_min_dc_bbf_i)
            {
                curr_min_dc_bbf_i = dc_meas_i;
                curr_min_dc_bbf_i_idx = DAC_idx;
            }
        }
        
        /* Measure optimal BBF Q DAC setting (I and Q split as there are I->Q DC changes) */
        curr_min_dc_bbf_q = curr_min_dc_tza_q;
        curr_min_dc_bbf_q_idx = 0x20;
        for (DAC_idx=0;DAC_idx<=63;DAC_idx+=1)
        {
            XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_I(XCVR, curr_min_dc_bbf_i_idx); /* Set bbf I to calibrated value */
            XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_Q(XCVR, DAC_idx);               /* Adjust bbf Q */
            XcvrDelay(32*2);
            dc_meas_q = XCVR_BRD_DCOC_DC_EST_DC_EST_Q(XCVR);  /* Take Q measurement */
            dc_meas_q = dc_meas_q | ((dc_meas_q & 0x800) ? 0xF000 : 0x0);
            XcvrDelay(32*2);
            dc_meas_q = (dc_meas_q < 0) ? (-1*dc_meas_q) : (dc_meas_q);
            if(dc_meas_q < curr_min_dc_bbf_q)
            {
                curr_min_dc_bbf_q = dc_meas_q;
                curr_min_dc_bbf_q_idx = DAC_idx;
            }
        }
        /* Now set the manual BBF Q settings from this sweep */
        XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_Q(XCVR, curr_min_dc_bbf_q_idx); /* Set bbf Q to new manual value */
        XcvrDelay(32*15);
        
        dc_meas_i = XCVR_BRD_DCOC_DC_EST_DC_EST_I(XCVR); /* Take final I measurement */
        XcvrDelay(32*10);
        dc_meas_q = XCVR_BRD_DCOC_DC_EST_DC_EST_Q(XCVR); /* Take final Q measurement */
        dc_meas_q = dc_meas_q | ((dc_meas_q & 0x800) ? 0xF000 : 0x0);
        dc_meas_i = dc_meas_i | ((dc_meas_i & 0x800) ? 0xF000 : 0x0);
        XcvrDelay(32*2);
        
        /* Store this gain setting's dc dac values */
        XCVR_BWR_DCOC_OFFSET__DCOC_TZA_OFFSET_Q(XCVR, tbl_idx,curr_min_dc_tza_q_idx);
        XCVR_BWR_DCOC_OFFSET__DCOC_TZA_OFFSET_I(XCVR, tbl_idx,curr_min_dc_tza_i_idx);
        curr_min_dc_bbf_i_idx = (curr_min_dc_bbf_i_idx >= 0x20) ? ((curr_min_dc_bbf_i_idx-0x20)) : (0x20+(curr_min_dc_bbf_i_idx));
        curr_min_dc_bbf_q_idx = (curr_min_dc_bbf_q_idx >= 0x20) ? ((curr_min_dc_bbf_q_idx-0x20)) : (0x20+(curr_min_dc_bbf_q_idx));
        XCVR_BWR_DCOC_OFFSET__DCOC_BBF_OFFSET_Q(XCVR, tbl_idx,curr_min_dc_bbf_q_idx);
        XCVR_BWR_DCOC_OFFSET__DCOC_BBF_OFFSET_I(XCVR, tbl_idx,curr_min_dc_bbf_i_idx);
    }
    
    /* Now set the manual BBF settings from this sweep */
    XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_I(XCVR, curr_min_dc_bbf_i_idx); /* Set bbf I to new manual value */
    XCVR_BWR_DCOC_CTRL_3_BBF_DCOC_INIT_Q(XCVR, curr_min_dc_bbf_q_idx); /* Set bbf Q to new manual value */
    XCVR_BWR_DCOC_CTRL_0_DCOC_TRACK_EN(XCVR, DCOC_TRACK_EN_def_c); /* Disable tracking */
    XCVR_BWR_DCOC_CTRL_0_DCOC_MAN(XCVR, 0); /* Force dcoc dacs to not use manual override */
    
    XCVR_BWR_DCOC_CTRL_1_BBA_CORR_POL(XCVR, 1);
    XCVR_BWR_DCOC_CTRL_1_TZA_CORR_POL(XCVR, 1);
    XcvrForceRxWd();
    XcvrDelay(200);
    
    /* Revert AGC settings to normal values for usage */
    XCVR_BWR_AGC_CTRL_1_USER_LNM_GAIN_EN(XCVR, 0); /* Use Manual Gain for LNM */
    XCVR_BWR_AGC_CTRL_1_USER_BBF_GAIN_EN(XCVR, 0); /* Use Manual Gain for BBF */
    XCVR_BWR_RX_DIG_CTRL_RX_AGC_EN(XCVR, RX_AGC_EN_def_c);       /* AGC Control enabled */
    XCVR_DCOC_CTRL_0 = dcoc_ctrl_0_stack; /* Restore DCOC_CTRL_0 state to prior settings */
    XCVR_DCOC_CTRL_1 = dcoc_ctrl_1_stack; /* Restore DCOC_CTRL_1 state to prior settings */
    XCVR_BWR_ANA_SPARE_DCOC_TRK_EST_GS_CNT(XCVR, gearshift_state); /* Restore gearshift state to prior setting */
    XCVR_DCOC_CAL_GAIN = dcoc_cal_gain_state;  /* Restore DCOC_CAL_GAIN state to prior setting */
    XcvrOverrideChannel(0xFF,TRUE);     /* Release channel overrides */    
}

/*! *********************************************************************************
* \brief  IQ Mismatch Calibration. Performs IQMCalibrationIter calibrations, averages and set the calibration values in the XCVR_IQMC_CAL register
*
* \ingroup PublicAPIs
*
* \details  IQMC requires a tone input of 250 kHz (+/-75 kHz) to be applied at the RF port and RF/Ana gains set up to for a 0-5 dBm signal at ADC
*
***********************************************************************************/
void XcvrIQMCal ( void )
{
    uint8_t CH_filt_bypass_state;
    uint8_t Decimator_OSR_state;
    uint16_t IQMC_gain_cal_trials[IQMCalibrationTrials]={0};
    uint16_t IQMC_phase_cal_trials[IQMCalibrationTrials]={0};
    uint8_t cnt;
    uint32_t cal_wait_time;  
    uint32_t IQMC_gain_adj_sum = 0;  
    uint32_t IQMC_phase_adj_sum = 0;  
    uint16_t IQMC_gain_adj_mean = 0;  
    uint16_t IQMC_phase_adj_mean = 0; 
    
    /* Read current Rx Decimation OSR Value and Channel Filter State. Set Decimation Filter OSR to 2 and Bypass Rx Channel Filter */
    Decimator_OSR_state = XCVR_BRD_RX_DIG_CTRL_RX_DEC_FILT_OSR(XCVR);
    CH_filt_bypass_state = XCVR_BRD_RX_DIG_CTRL_RX_CH_FILT_BYPASS(XCVR);
    XCVR_BWR_RX_DIG_CTRL_RX_DEC_FILT_OSR(XCVR, 2);     /* Set Decimation OSR to 2 */
    XCVR_BWR_RX_DIG_CTRL_RX_CH_FILT_BYPASS(XCVR, 1);   /* Bypass Channel Filter */
    
    for (cnt=0;cnt<IQMCalibrationTrials;cnt+=1)
    {
        /* Set up for IQMC calibration trial */ 
        XCVR_BWR_IQMC_CAL_IQMC_GAIN_ADJ(XCVR, 0x400);          /* Set IQ gain mismatch to default (1.0) */
        XCVR_BWR_IQMC_CAL_IQMC_PHASE_ADJ(XCVR, 0x0);           /* Set IQ phase mismatch to default (0) */
        XCVR_BWR_IQMC_CTRL_IQMC_NUM_ITER(XCVR, IQMCalibrationIter);    /* Set number of iterations to compute IQMC. Default: 0x80. Max: 0xFF */
        XCVR_BWR_IQMC_CTRL_IQMC_CAL_EN(XCVR, 1);               /* Enable IQMC HW Calibration */
        
        /* Wait for IQMCalibrationIter * 13 microseconds */
        cal_wait_time = IQMCalibrationIter * 13 * 32;
        XcvrDelay(cal_wait_time);
        
        /* Read Calibration Trial Results and save in Trial Value Buffers */
        IQMC_gain_cal_trials[cnt] = XCVR_BRD_IQMC_CAL_IQMC_GAIN_ADJ(XCVR);
        IQMC_phase_cal_trials[cnt] = XCVR_BRD_IQMC_CAL_IQMC_PHASE_ADJ(XCVR);
        
        /* Compute Sum of gain/phase adjustment values */
        IQMC_gain_adj_sum = IQMC_gain_adj_sum + IQMC_gain_cal_trials[cnt] ;
        IQMC_phase_adj_sum = IQMC_phase_adj_sum + IQMC_phase_cal_trials[cnt] ;
    }
    
    /* Average Trial Values and load in XCVR_IQMC_CAL register */
    IQMC_gain_adj_mean = IQMC_gain_adj_sum/IQMCalibrationTrials;
    IQMC_phase_adj_mean = IQMC_phase_adj_sum/IQMCalibrationTrials;
    
    XCVR_BWR_IQMC_CAL_IQMC_GAIN_ADJ(XCVR, IQMC_gain_adj_mean);     /* Set IQ gain mismatch to default (1.0) */
    XCVR_BWR_IQMC_CAL_IQMC_PHASE_ADJ(XCVR, IQMC_phase_adj_mean);   /* Set IQ phase mismatch to default (0) */
    
    /* Restore Decimation OSR Value and Channel Filter State. */
    XCVR_BWR_RX_DIG_CTRL_RX_DEC_FILT_OSR(XCVR, Decimator_OSR_state);     
    XCVR_BWR_RX_DIG_CTRL_RX_CH_FILT_BYPASS(XCVR, CH_filt_bypass_state);     
}
