/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
#include "EmbeddedTypes.h"
#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_xcvr.h"
#include "fsl_xcvr_trim.h"
#include <math.h>
#include "ifr_radio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define channelMapTableSize (128U)
#define gPllDenom_c         0x02000000U /* Denominator is a constant value */
#define ABS(x) ((x) > 0 ? (x) : -(x))

#ifndef TRUE
#define TRUE                        (true)
#endif

#ifndef FALSE
#define FALSE                       (false)
#endif
#define RF_OSCILLATOR_STAYS_ON     (false) /* Control whether RF_OSC can be left on all the time. */
#define RF_OSCILLATOR_READY        ((RSIM->CONTROL & RSIM_CONTROL_RF_OSC_READY_MASK) != 0x0U)

#ifndef EXTERNAL_CLOCK_GEN
#define EXTERNAL_CLOCK_GEN   0
#endif

#define ANT_A   1
#define ANT_B   0

#ifndef XCVR_COEX_RF_ACTIVE_PIN
#define XCVR_COEX_RF_ACTIVE_PIN ANT_B
#endif /* XCVR_COEX_RF_ACTIVE_PIN */

typedef struct xcvr_pllChannel_tag
{
    unsigned int integer;
    unsigned int numerator;
} xcvr_pllChannel_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void XcvrPanic(XCVR_PANIC_ID_T panic_id, uint32_t panic_address);
void rf_osc_startup(void);
void rf_osc_shutdown(void);
extern double trunc (double);
extern double round (double);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static panic_fptr s_PanicFunctionPtr = NULL;
const xcvr_pllChannel_t mapTable [channelMapTableSize] =
{
    {0x00000025, 0x07C00000}, /* 0 */
    {0x00000025, 0x07C80000}, /* 1 */
    {0x00000025, 0x07D00000}, /* 2 */
    {0x00000025, 0x07D80000}, /* 3 */
    {0x00000025, 0x07E00000}, /* 4 */
    {0x00000025, 0x07E80000}, /* 5 */
    {0x00000025, 0x07F00000}, /* 6 */
    {0x00000025, 0x07F80000}, /* 7 */
    {0x00000025, 0x00000000}, /* 8 */
    {0x00000025, 0x00080000}, /* 9 */
    {0x00000025, 0x00100000}, /* 10 */
    {0x00000025, 0x00180000}, /* 11 */
    {0x00000025, 0x00200000}, /* 12 */
    {0x00000025, 0x00280000}, /* 13 */
    {0x00000025, 0x00300000}, /* 14 */
    {0x00000025, 0x00380000}, /* 15 */
    {0x00000025, 0x00400000}, /* 16 */
    {0x00000025, 0x00480000}, /* 17 */
    {0x00000025, 0x00500000}, /* 18 */
    {0x00000025, 0x00580000}, /* 19 */
    {0x00000025, 0x00600000}, /* 20 */
    {0x00000025, 0x00680000}, /* 21 */
    {0x00000025, 0x00700000}, /* 22 */
    {0x00000025, 0x00780000}, /* 23 */
    {0x00000025, 0x00800000}, /* 24 */
    {0x00000025, 0x00880000}, /* 25 */
    {0x00000025, 0x00900000}, /* 26 */
    {0x00000025, 0x00980000}, /* 27 */
    {0x00000025, 0x00A00000}, /* 28 */
    {0x00000025, 0x00A80000}, /* 29 */
    {0x00000025, 0x00B00000}, /* 30 */
    {0x00000025, 0x00B80000}, /* 31 */
    {0x00000025, 0x00C00000}, /* 32 */
    {0x00000025, 0x00C80000}, /* 33 */
    {0x00000025, 0x00D00000}, /* 34 */
    {0x00000025, 0x00D80000}, /* 35 */
    {0x00000025, 0x00E00000}, /* 36 */
    {0x00000025, 0x00E80000}, /* 37 */
    {0x00000025, 0x00F00000}, /* 38 */
    {0x00000025, 0x00F80000}, /* 39 */
    {0x00000025, 0x01000000}, /* 40 */
    {0x00000026, 0x07080000}, /* 41 */
    {0x00000026, 0x07100000}, /* 42 */
    {0x00000026, 0x07180000}, /* 43 */
    {0x00000026, 0x07200000}, /* 44 */
    {0x00000026, 0x07280000}, /* 45 */
    {0x00000026, 0x07300000}, /* 46 */
    {0x00000026, 0x07380000}, /* 47 */
    {0x00000026, 0x07400000}, /* 48 */
    {0x00000026, 0x07480000}, /* 49 */
    {0x00000026, 0x07500000}, /* 50 */
    {0x00000026, 0x07580000}, /* 51 */
    {0x00000026, 0x07600000}, /* 52 */
    {0x00000026, 0x07680000}, /* 53 */
    {0x00000026, 0x07700000}, /* 54 */
    {0x00000026, 0x07780000}, /* 55 */
    {0x00000026, 0x07800000}, /* 56 */
    {0x00000026, 0x07880000}, /* 57 */
    {0x00000026, 0x07900000}, /* 58 */
    {0x00000026, 0x07980000}, /* 59 */
    {0x00000026, 0x07A00000}, /* 60 */
    {0x00000026, 0x07A80000}, /* 61 */
    {0x00000026, 0x07B00000}, /* 62 */
    {0x00000026, 0x07B80000}, /* 63 */
    {0x00000026, 0x07C00000}, /* 64 */
    {0x00000026, 0x07C80000}, /* 65 */
    {0x00000026, 0x07D00000}, /* 66 */
    {0x00000026, 0x07D80000}, /* 67 */
    {0x00000026, 0x07E00000}, /* 68 */
    {0x00000026, 0x07E80000}, /* 69 */
    {0x00000026, 0x07F00000}, /* 70 */
    {0x00000026, 0x07F80000}, /* 71 */
    {0x00000026, 0x00000000}, /* 72 */
    {0x00000026, 0x00080000}, /* 73 */
    {0x00000026, 0x00100000}, /* 74 */
    {0x00000026, 0x00180000}, /* 75 */
    {0x00000026, 0x00200000}, /* 76 */
    {0x00000026, 0x00280000}, /* 77 */
    {0x00000026, 0x00300000}, /* 78 */
    {0x00000026, 0x00380000}, /* 79 */
    {0x00000026, 0x00400000}, /* 80 */
    {0x00000026, 0x00480000}, /* 81 */
    {0x00000026, 0x00500000}, /* 82 */
    {0x00000026, 0x00580000}, /* 83 */
    {0x00000026, 0x00600000}, /* 84 */
    {0x00000026, 0x00680000}, /* 85 */
    {0x00000026, 0x00700000}, /* 86 */
    {0x00000026, 0x00780000}, /* 87 */
    {0x00000026, 0x00800000}, /* 88 */
    {0x00000026, 0x00880000}, /* 89 */
    {0x00000026, 0x00900000}, /* 90 */
    {0x00000026, 0x00980000}, /* 91 */
    {0x00000026, 0x00A00000}, /* 92 */
    {0x00000026, 0x00A80000}, /* 93 */
    {0x00000026, 0x00B00000}, /* 94 */
    {0x00000026, 0x00B80000}, /* 95 */
    {0x00000026, 0x00C00000}, /* 96 */
    {0x00000026, 0x00C80000}, /* 97 */
    {0x00000026, 0x00D00000}, /* 98 */
    {0x00000026, 0x00D80000}, /* 99 */
    {0x00000026, 0x00E00000}, /* 100 */
    {0x00000026, 0x00E80000}, /* 101 */
    {0x00000026, 0x00F00000}, /* 102 */
    {0x00000026, 0x00F80000}, /* 103 */
    {0x00000026, 0x01000000}, /* 104 */
    {0x00000027, 0x07080000}, /* 105 */
    {0x00000027, 0x07100000}, /* 106 */
    {0x00000027, 0x07180000}, /* 107 */
    {0x00000027, 0x07200000}, /* 108 */
    {0x00000027, 0x07280000}, /* 109 */
    {0x00000027, 0x07300000}, /* 110 */
    {0x00000027, 0x07380000}, /* 111 */
    {0x00000027, 0x07400000}, /* 112 */
    {0x00000027, 0x07480000}, /* 113 */
    {0x00000027, 0x07500000}, /* 114 */
    {0x00000027, 0x07580000}, /* 115 */
    {0x00000027, 0x07600000}, /* 116 */
    {0x00000027, 0x07680000}, /* 117 */
    {0x00000027, 0x07700000}, /* 118 */
    {0x00000027, 0x07780000}, /* 119 */
    {0x00000027, 0x07800000}, /* 120 */
    {0x00000027, 0x07880000}, /* 121 */
    {0x00000027, 0x07900000}, /* 122 */
    {0x00000027, 0x07980000}, /* 123 */
    {0x00000027, 0x07A00000}, /* 124 */
    {0x00000027, 0x07A80000}, /* 125 */
    {0x00000027, 0x07B00000}, /* 126 */
    {0x00000027, 0x07B80000}  /* 127 */
};

/* Registers for timing of TX & RX */
#if RADIO_IS_GEN_3P0
uint16_t tx_rx_on_delay = TX_RX_ON_DELinit;
uint16_t tx_rx_synth_delay = TX_RX_SYNTH_init;
#else
#if RF_OSC_26MHZ == 1
uint16_t tx_rx_on_delay = TX_RX_ON_DELAY_VAL_26MHZ;
#else
uint16_t tx_rx_on_delay = TX_RX_ON_DELAY_VAL;
#endif /* RF_OSC_26MHZ == 1 */
uint16_t tx_rx_synth_delay = TX_RX_SYNTH_DELAY_VAL;
#endif /* RADIO_IS_GEN_3P0 */

/* NOTE: These arrays MUST be ordered in the same order as the radio_mode_t enumeration. */
#if RADIO_IS_GEN_3P0
const xcvr_mode_datarate_config_t * mode_configs_dr_2mbps[NUM_RADIO_MODES] =
{
    (xcvr_mode_datarate_config_t *)NULL, /* 2Mbps rate not supported for this mode */
    (xcvr_mode_datarate_config_t *)NULL, /* 2Mbps rate not supported for this mode */
    (xcvr_mode_datarate_config_t *)NULL, /* 2Mbps rate not supported for this mode */
    &xcvr_GFSK_BT_0p5_h_0p5_2mbps_config,
    &xcvr_GFSK_BT_0p5_h_0p32_2mbps_config,
    (xcvr_mode_datarate_config_t *)NULL, /* 2Mbps rate not supported for this mode */
    (xcvr_mode_datarate_config_t *)NULL, /* 2Mbps rate not supported for this mode */
    &xcvr_GFSK_BT_0p3_h_0p5_2mbps_config,
    &xcvr_GFSK_BT_0p7_h_0p5_2mbps_config,
    &xcvr_MSK_2mbps_config,
};
#endif /* RADIO_IS_GEN_3P0 */

const xcvr_mode_datarate_config_t * mode_configs_dr_1mbps[NUM_RADIO_MODES] =
{
    &xcvr_BLE_1mbps_config,
#if RADIO_IS_GEN_2P1
    NULL,
    NULL,
#else
    &xcvr_ZIGBEE_500kbps_config, /* 802.15.4 only supports one configuration */
    &xcvr_ANT_1mbps_config,
#endif /* RADIO_IS_GEN_2P1 */
    &xcvr_GFSK_BT_0p5_h_0p5_1mbps_config,
    &xcvr_GFSK_BT_0p5_h_0p32_1mbps_config,
    &xcvr_GFSK_BT_0p5_h_0p7_1mbps_config,
    &xcvr_GFSK_BT_0p5_h_1p0_1mbps_config,
    &xcvr_GFSK_BT_0p3_h_0p5_1mbps_config,
    &xcvr_GFSK_BT_0p7_h_0p5_1mbps_config,
    &xcvr_MSK_1mbps_config,
};

const xcvr_mode_datarate_config_t * mode_configs_dr_500kbps[NUM_RADIO_MODES] =
{
    &xcvr_BLE_1mbps_config, /* Invalid option */
#if RADIO_IS_GEN_2P1
    NULL,
    NULL,
#else
    &xcvr_ZIGBEE_500kbps_config, /* 802.15.4 setting */
    &xcvr_ANT_1mbps_config, /* Invalid option */
#endif /* RADIO_IS_GEN_2P1 */
    &xcvr_GFSK_BT_0p5_h_0p5_500kbps_config,
    &xcvr_GFSK_BT_0p5_h_0p32_500kbps_config,
    &xcvr_GFSK_BT_0p5_h_0p7_500kbps_config,
    &xcvr_GFSK_BT_0p5_h_1p0_500kbps_config,
    &xcvr_GFSK_BT_0p3_h_0p5_500kbps_config,
    &xcvr_GFSK_BT_0p7_h_0p5_500kbps_config,
    &xcvr_MSK_500kbps_config,
};
const xcvr_mode_datarate_config_t * mode_configs_dr_250kbps[NUM_RADIO_MODES] =
{
    &xcvr_BLE_1mbps_config, /* Invalid option */
#if RADIO_IS_GEN_2P1
    NULL,
    NULL,
#else
    &xcvr_ZIGBEE_500kbps_config, /* 802.15.4 only supports one configuration */
    &xcvr_ANT_1mbps_config, /* Invalid option */
#endif /* RADIO_IS_GEN_2P1 */
    &xcvr_GFSK_BT_0p5_h_0p5_250kbps_config,
    &xcvr_GFSK_BT_0p5_h_0p32_250kbps_config,
    &xcvr_GFSK_BT_0p5_h_0p7_250kbps_config,
    &xcvr_GFSK_BT_0p5_h_1p0_250kbps_config,
    &xcvr_GFSK_BT_0p3_h_0p5_250kbps_config,
    &xcvr_GFSK_BT_0p7_h_0p5_250kbps_config,
    &xcvr_MSK_250kbps_config,
};

static xcvr_currConfig_t current_xcvr_config;

void rf_osc_startup(void)
{
    if (!RF_OSCILLATOR_READY)
    {
        RSIM->CONTROL |= RSIM_CONTROL_RF_OSC_EN_MASK;
    }
    while (!RF_OSCILLATOR_READY)
    {
        /* Wait for RF_OSC_READY to be asserted before continuing */
    }
}

void rf_osc_shutdown(void)
{
    if (!RF_OSCILLATOR_STAYS_ON)
    {
        RSIM->CONTROL &= ~RSIM_CONTROL_RF_OSC_EN_MASK;
    }
}
    
/*******************************************************************************
 * Code
 ******************************************************************************/
xcvrStatus_t XCVR_Init(radio_mode_t radio_mode, data_rate_t data_rate)
{
    const xcvr_mode_datarate_config_t * mode_datarate_config;
    const xcvr_datarate_config_t * datarate_config ;
    const xcvr_mode_config_t * radio_mode_cfg;
    const xcvr_common_config_t * radio_common_config;

    xcvrStatus_t status;

    IFR_SW_TRIM_TBL_ENTRY_T sw_trim_tbl[] =
    {
        {TRIM_STATUS, 0, FALSE}, /*< Fetch the trim status word if available.*/
        {TRIM_VERSION, 0, FALSE} /*< Fetch the trim version number if available.*/
    };
    const uint8_t NUM_TRIM_TBL_ENTRIES = sizeof(sw_trim_tbl)/sizeof(IFR_SW_TRIM_TBL_ENTRY_T);

#ifndef SIMULATION

#if (EXTERNAL_CLOCK_GEN)
    RSIM->RF_OSC_CTRL |= RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN_MASK; /* Only when external clock is being used */
#endif /* EXTERNAL_CLOCK_GEN */

#if RADIO_IS_GEN_2P0
    RSIM->RF_OSC_CTRL &= ~RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_MASK; /* Set EXT_OSC_OVRD value to zero */
    RSIM->RF_OSC_CTRL |= RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK; /* Enable over-ride with zero value */   
#endif /* RADIO_IS_GEN_2P0 */

    /* Check that this is the proper radio version */
    {
        uint8_t radio_id = ((RSIM->MISC & RSIM_MISC_RADIO_VERSION_MASK)>>RSIM_MISC_RADIO_VERSION_SHIFT);

        if (
#if RADIO_IS_GEN_3P0
            (radio_id != 0x5) /* KW3 Gen3 */
#elif RADIO_IS_GEN_2P1
            (radio_id != 0x5) /* KW35 Gen2.1 */
#else
            (radio_id != 0x3) &&  /* KW41/31/21 v1 */
            (radio_id != 0xB) /* KW41/31/21 v1.1 */
#endif /* RADIO_IS_GEN_3P0 */
            )
        {
            XcvrPanic(WRONG_RADIO_ID_DETECTED, (uint32_t)&XCVR_Init);
        }
    }

#if RADIO_IS_GEN_3P0
    /* Assert Radio Run Request and wait for ack from SPM. */
    RSIM->POWER |= RSIM_POWER_RSIM_RUN_REQUEST_MASK;
    while ((RSIM->POWER & RSIM_POWER_SPM_RUN_ACK_STAT_MASK) == 0)
    {
    }
    RSIM->CONTROL |= RSIM_CONTROL_RSIM_CGC_XCVR_EN_MASK; 
    rf_osc_startup(); /* Start RF_OSC to allow radio registers access */
#else
    SIM->SCGC5 |= SIM_SCGC5_PHYDIG_MASK;

    /* Load IFR trim values */
    handle_ifr(&sw_trim_tbl[0], NUM_TRIM_TBL_ENTRIES);
#endif /* RADIO_IS_GEN_3P0 */

#endif /* ifndef SIMULATION */

    /* Perform the desired XCVR initialization and configuration */
    status = XCVR_GetDefaultConfig(radio_mode, data_rate, 
                                   (const xcvr_common_config_t **)&radio_common_config, 
                                   (const xcvr_mode_config_t **)&radio_mode_cfg, 
                                   (const xcvr_mode_datarate_config_t **)&mode_datarate_config, 
                                   (const xcvr_datarate_config_t **)&datarate_config);
								  
    if (status == gXcvrSuccess_c)
    {
        status = XCVR_Configure((const xcvr_common_config_t *)radio_common_config, 
                                (const xcvr_mode_config_t *)radio_mode_cfg, 
                                (const xcvr_mode_datarate_config_t *)mode_datarate_config, 
                                (const xcvr_datarate_config_t *)datarate_config, 25, XCVR_FIRST_INIT);
        current_xcvr_config.radio_mode = radio_mode;
        current_xcvr_config.data_rate = data_rate;
    }

    return status;
}

void XCVR_Deinit(void)
{
#if RADIO_IS_GEN_3P0
    rf_osc_shutdown();
    RSIM->POWER |= RSIM_POWER_RSIM_STOP_MODE_MASK; /* Set radio stop mode to RVLLS */
    RSIM->POWER &= ~RSIM_POWER_RSIM_RUN_REQUEST_MASK; /* Clear RUN request */
#else

#endif /* RADIO_IS_GEN_3P0 */
}

xcvrStatus_t XCVR_GetDefaultConfig(radio_mode_t radio_mode, 
                                   data_rate_t data_rate, 
                                   const xcvr_common_config_t ** com_config, 
                                   const xcvr_mode_config_t ** mode_config, 
                                   const xcvr_mode_datarate_config_t ** mode_datarate_config, 
                                   const xcvr_datarate_config_t ** datarate_config)
{
    xcvrStatus_t status = gXcvrSuccess_c;
    /* Common configuration pointer */
    *com_config = (const xcvr_common_config_t *)&xcvr_common_config;

    /* Mode dependent configuration pointer */
    switch (radio_mode)
    {
#if !RADIO_IS_GEN_2P1
        case ZIGBEE_MODE:
            *mode_config = ( const xcvr_mode_config_t *)&zgbe_mode_config; /* Zigbee configuration */
            break;
        case ANT_MODE:
            *mode_config = ( const xcvr_mode_config_t *)&ant_mode_config; /* ANT configuration */
            break;
#endif /* !RADIO_IS_GEN_2P1 */
        case BLE_MODE:
            *mode_config = ( const xcvr_mode_config_t *)&ble_mode_config; /* BLE configuration */
            break;
        case GFSK_BT_0p5_h_0p5:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p5_h_0p5_mode_config; /* GFSK_BT_0p5_h_0p5 configuration */
            break;
        case GFSK_BT_0p5_h_0p32:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p5_h_0p32_mode_config; /* GFSK_BT_0p5_h_0p32 configuration */
            break;
        case GFSK_BT_0p5_h_0p7:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p5_h_0p7_mode_config; /* GFSK_BT_0p5_h_0p7 configuration */
            break;
        case GFSK_BT_0p5_h_1p0:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p5_h_1p0_mode_config; /* GFSK_BT_0p5_h_1p0 configuration */
            break;
        case GFSK_BT_0p3_h_0p5:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p3_h_0p5_mode_config; /* GFSK_BT_0p3_h_0p5 configuration */
            break;
        case GFSK_BT_0p7_h_0p5:
            *mode_config = ( const xcvr_mode_config_t *)&gfsk_bt_0p7_h_0p5_mode_config; /* GFSK_BT_0p7_h_0p5 configuration */
            break;
        case MSK:
            *mode_config = ( const xcvr_mode_config_t *)&msk_mode_config; /* MSK configuration */
            break;
        default:    
            status = gXcvrInvalidParameters_c;
            break;
    }

    /* Data rate dependent and modeXdatarate dependent configuration pointers */
    if (status == gXcvrSuccess_c) /* Only attempt this pointer assignment process if prior switch() statement completed successfully */
    {
        switch (data_rate)
        {
#if RADIO_IS_GEN_3P0
            case DR_2MBPS:
                if ((radio_mode == GFSK_BT_0p5_h_0p7) || (radio_mode == GFSK_BT_0p5_h_1p0) || (radio_mode == ZIGBEE_MODE) || (radio_mode == BLE_MODE) || (radio_mode == ANT_MODE))
                {
                    status = gXcvrInvalidParameters_c;
                }
                else
                {
                    *datarate_config = (const xcvr_datarate_config_t *)&xcvr_2mbps_config; /* 2Mbps datarate configurations */
                    *mode_datarate_config = (const xcvr_mode_datarate_config_t *)mode_configs_dr_2mbps[radio_mode];
                }
                break;
#endif /* RADIO_IS_GEN_3P0 */
            case DR_1MBPS:
                *datarate_config = (const xcvr_datarate_config_t *)&xcvr_1mbps_config; /* 1Mbps datarate configurations */
                *mode_datarate_config = (const xcvr_mode_datarate_config_t *)mode_configs_dr_1mbps[radio_mode];
                break;
            case DR_500KBPS:
                if (radio_mode == ZIGBEE_MODE)
                {
                    /* See fsl_xcvr_zgbe_config.c for settings */
#if !RADIO_IS_GEN_2P1
                    *datarate_config = (const xcvr_datarate_config_t *)&xcvr_802_15_4_500kbps_config; /* 500Kbps datarate configurations */
#endif /* !RADIO_IS_GEN_2P1 */
                }
                else
                {
                    *datarate_config = (const xcvr_datarate_config_t *)&xcvr_500kbps_config; /* 500Kbps datarate configurations */
                }
                *mode_datarate_config = (const xcvr_mode_datarate_config_t *)mode_configs_dr_500kbps[radio_mode];
                break;
            case DR_250KBPS:
                *datarate_config = (const xcvr_datarate_config_t *)&xcvr_250kbps_config; /* 250Kbps datarate configurations */
                *mode_datarate_config = (const xcvr_mode_datarate_config_t *)mode_configs_dr_250kbps[radio_mode];
                break;
            default:
                status = gXcvrInvalidParameters_c;
                break;
        }
    }

    return status;
}

xcvrStatus_t XCVR_Configure(const xcvr_common_config_t *com_config, 
                            const xcvr_mode_config_t *mode_config, 
                            const xcvr_mode_datarate_config_t *mode_datarate_config, 
                            const xcvr_datarate_config_t *datarate_config, 
                            int16_t tempDegC, 
                            XCVR_INIT_MODE_CHG_T first_init)
{
    xcvrStatus_t config_status = gXcvrSuccess_c;
    uint32_t temp; 

    /* Turn on the module clocks before doing anything */
#if RADIO_IS_GEN_3P0
    RSIM->CONTROL |= mode_config->scgc5_clock_ena_bits; /* Same bit storage is used but RSIM bit assignments are applied */
#else
    SIM->SCGC5 |= mode_config->scgc5_clock_ena_bits;
#endif /* RADIO_IS_GEN_3P0 */

    /*******************************************************************************/
    /* XCVR_ANA configs */
    /*******************************************************************************/

    /* Configure PLL Loop Filter */
    if (first_init)
    {
        XCVR_ANA->SY_CTRL_1 &= ~com_config->ana_sy_ctrl1.mask;
        XCVR_ANA->SY_CTRL_1 |= com_config->ana_sy_ctrl1.init;
    }

    /* Configure VCO KVM */
    XCVR_ANA->SY_CTRL_2 &= ~mode_datarate_config->ana_sy_ctrl2.mask;
    XCVR_ANA->SY_CTRL_2 |= mode_datarate_config->ana_sy_ctrl2.init;

    /* Configure analog filter bandwidth */
    XCVR_ANA->RX_BBA &= ~mode_datarate_config->ana_rx_bba.mask;
    XCVR_ANA->RX_BBA |= mode_datarate_config->ana_rx_bba.init;
    XCVR_ANA->RX_TZA &= ~mode_datarate_config->ana_rx_tza.mask;
    XCVR_ANA->RX_TZA |= mode_datarate_config->ana_rx_tza.init;

#if RADIO_IS_GEN_2P0
    if (first_init)
    {
        temp = XCVR_ANA->TX_DAC_PA;
        temp &= ~XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS_MASK;
        temp |= XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS(4);
        XCVR_ANA->TX_DAC_PA = temp;

        temp = XCVR_ANA->BB_LDO_2;
        temp &= ~XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM_MASK;
        temp |= XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM(0);
        XCVR_ANA->BB_LDO_2 = temp;

        temp = XCVR_ANA->RX_LNA;
        temp &= ~XCVR_ANALOG_RX_LNA_RX_LNA_BUMP_MASK;
        temp |= XCVR_ANALOG_RX_LNA_RX_LNA_BUMP(1);
        XCVR_ANA->RX_LNA = temp;

        temp = XCVR_ANA->BB_LDO_1;
        temp &= ~XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM_MASK;
        temp |= XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM(1);
        XCVR_ANA->BB_LDO_1 = temp;
    }
#endif /* RADIO_IS_GEN_2P0 */

    /*******************************************************************************/
    /* XCVR_MISC configs */
    /*******************************************************************************/
    temp = XCVR_MISC->XCVR_CTRL;
    temp &= ~(mode_config->xcvr_ctrl.mask | XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ_MASK);
    temp |= mode_config->xcvr_ctrl.init;
	
#if RF_OSC_26MHZ == 1
    {
        temp |= XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ(1);
    }
#endif /* RF_OSC_26MHZ == 1 */

    XCVR_MISC->XCVR_CTRL = temp;

#if RADIO_IS_GEN_2P1
    XCVR_MISC->FAD_CTRL &= ~XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_MASK;
#endif /* RADIO_IS_GEN_2P1 */

    /*******************************************************************************/
    /* XCVR_PHY configs */
    /*******************************************************************************/
#if RADIO_IS_GEN_3P0
    XCVR_PHY->PHY_FSK_PD_CFG0 = mode_config->phy_fsk_pd_cfg0;
    XCVR_PHY->PHY_FSK_PD_CFG1 = mode_config->phy_fsk_pd_cfg1;
    XCVR_PHY->PHY_FSK_CFG = mode_config->phy_fsk_cfg;
    XCVR_PHY->PHY_FSK_MISC = mode_config->phy_fsk_misc | mode_datarate_config->phy_fsk_misc_mode_datarate;
    XCVR_PHY->FSK_FAD_CTRL = mode_config->phy_fad_ctrl;
#else
    XCVR_PHY->PHY_PRE_REF0 = mode_config->phy_pre_ref0_init;
    XCVR_PHY->PRE_REF1 = mode_config->phy_pre_ref1_init;
    XCVR_PHY->PRE_REF2 = mode_config->phy_pre_ref2_init;
    XCVR_PHY->CFG1 = mode_config->phy_cfg1_init;
    XCVR_PHY->CFG2 = mode_datarate_config->phy_cfg2_init;
    XCVR_PHY->EL_CFG = mode_config->phy_el_cfg_init | datarate_config->phy_el_cfg_init; /* EL_WIN_SIZE and EL_INTERVAL are datarate dependent, */
#endif /* RADIO_IS_GEN_3P0 */

    /*******************************************************************************/
    /* XCVR_PLL_DIG configs */
    /*******************************************************************************/
    if (first_init)
    {
        XCVR_PLL_DIG->HPM_BUMP = com_config->pll_hpm_bump;
        XCVR_PLL_DIG->MOD_CTRL = com_config->pll_mod_ctrl;
        XCVR_PLL_DIG->CHAN_MAP = com_config->pll_chan_map;
        XCVR_PLL_DIG->LOCK_DETECT = com_config->pll_lock_detect;
        XCVR_PLL_DIG->HPM_CTRL = com_config->pll_hpm_ctrl;
#if !RADIO_IS_GEN_2P1
        XCVR_PLL_DIG->HPMCAL_CTRL = com_config->pll_hpmcal_ctrl;
#endif /* !RADIO_IS_GEN_2P1 */
        XCVR_PLL_DIG->HPM_SDM_RES = com_config->pll_hpm_sdm_res;
        XCVR_PLL_DIG->LPM_CTRL = com_config->pll_lpm_ctrl;
        XCVR_PLL_DIG->LPM_SDM_CTRL1 = com_config->pll_lpm_sdm_ctrl1;
        XCVR_PLL_DIG->DELAY_MATCH = com_config->pll_delay_match;
        XCVR_PLL_DIG->CTUNE_CTRL = com_config->pll_ctune_ctrl;
    }

    /*******************************************************************************/
    /* XCVR_RX_DIG configs */
    /*******************************************************************************/

    /* Configure RF Aux PLL for proper operation based on external clock frequency */
    if (first_init)
    {
        temp = XCVR_ANA->RX_AUXPLL;
        temp &= ~XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST_MASK;
#if RF_OSC_26MHZ == 1
        {
            temp |= XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST(4);
        }
#else
        {
            temp |= XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST(7);
        }
#endif /* RF_OSC_26MHZ == 1 */
        XCVR_ANA->RX_AUXPLL = temp;
    }
    
    /* Configure RX_DIG_CTRL */
#if RF_OSC_26MHZ == 1
    {
        temp = com_config->rx_dig_ctrl_init | /* Common portion of RX_DIG_CTRL init */
               mode_config->rx_dig_ctrl_init_26mhz | /* Mode  specific portion of RX_DIG_CTRL init */
               datarate_config->rx_dig_ctrl_init_26mhz | /* Datarate specific portion of RX_DIG_CTRL init */
               XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN_MASK; /* Always enable the sample rate converter for 26MHz */
    }
#else
    {
        temp = com_config->rx_dig_ctrl_init | /* Common portion of RX_DIG_CTRL init */
               mode_config->rx_dig_ctrl_init_32mhz | /* Mode specific portion of RX_DIG_CTRL init */
               datarate_config->rx_dig_ctrl_init_32mhz | /* Datarate specific portion of RX_DIG_CTRL init */
               0; /* Always disable the sample rate converter for 32MHz */
    }
#endif /* RF_OSC_26MHZ == 1 */

    temp |= com_config->rx_dig_ctrl_init;  /* Common portion of RX_DIG_CTRL init */
    XCVR_RX_DIG->RX_DIG_CTRL = temp;

    /* DCOC_CAL_IIR */
#if RF_OSC_26MHZ == 1
    {
        XCVR_RX_DIG->DCOC_CAL_IIR = datarate_config->dcoc_cal_iir_init_26mhz;
    }
#else
    {
        XCVR_RX_DIG->DCOC_CAL_IIR = datarate_config->dcoc_cal_iir_init_32mhz;
    }
#endif /* RF_OSC_26MHZ == 1 */

    /* DC_RESID_CTRL */
#if RF_OSC_26MHZ == 1
    {
        XCVR_RX_DIG->DC_RESID_CTRL = com_config->dc_resid_ctrl_init | datarate_config->dc_resid_ctrl_26mhz;
    }
#else
    {
        XCVR_RX_DIG->DC_RESID_CTRL = com_config->dc_resid_ctrl_init | datarate_config->dc_resid_ctrl_32mhz;
    }
#endif /* RF_OSC_26MHZ == 1 */

    /* DCOC_CTRL_0  & _1 */
#if RF_OSC_26MHZ == 1
    {
        XCVR_RX_DIG->DCOC_CTRL_0 = com_config->dcoc_ctrl_0_init_26mhz | datarate_config->dcoc_ctrl_0_init_26mhz; /* Combine common and datarate specific settings */
        XCVR_RX_DIG->DCOC_CTRL_1 = com_config->dcoc_ctrl_1_init | datarate_config->dcoc_ctrl_1_init_26mhz; /* Combine common and datarate specific settings */
#if RADIO_IS_GEN_3P0
        XCVR_RX_DIG->DCOC_CTRL_2 = datarate_config->dcoc_ctrl_2_init_26mhz;
#endif /* RADIO_IS_GEN_3P0 */

    }
#else
    {
        XCVR_RX_DIG->DCOC_CTRL_0 = com_config->dcoc_ctrl_0_init_32mhz | datarate_config->dcoc_ctrl_0_init_32mhz; /* Combine common and datarate specific settings */
        XCVR_RX_DIG->DCOC_CTRL_1 = com_config->dcoc_ctrl_1_init | datarate_config->dcoc_ctrl_1_init_32mhz; /* Combine common and datarate specific settings */
#if RADIO_IS_GEN_3P0
        XCVR_RX_DIG->DCOC_CTRL_2 = datarate_config->dcoc_ctrl_2_init_32mhz;
#endif /* RADIO_IS_GEN_3P0 */
    }
#endif /* RF_OSC_26MHZ == 1 */
    if (first_init)
    {
        /* DCOC_CAL_GAIN */
        XCVR_RX_DIG->DCOC_CAL_GAIN = com_config->dcoc_cal_gain_init;

        /* DCOC_CAL_RCP */
        XCVR_RX_DIG->DCOC_CAL_RCP = com_config->dcoc_cal_rcp_init;
        XCVR_RX_DIG->LNA_GAIN_VAL_3_0 = com_config->lna_gain_val_3_0;
        XCVR_RX_DIG->LNA_GAIN_VAL_7_4 = com_config->lna_gain_val_7_4;
        XCVR_RX_DIG->LNA_GAIN_VAL_8 = com_config->lna_gain_val_8;
        XCVR_RX_DIG->BBA_RES_TUNE_VAL_7_0 = com_config->bba_res_tune_val_7_0;
        XCVR_RX_DIG->BBA_RES_TUNE_VAL_10_8 = com_config->bba_res_tune_val_10_8;

        /* LNA_GAIN_LIN_VAL */
        XCVR_RX_DIG->LNA_GAIN_LIN_VAL_2_0 = com_config->lna_gain_lin_val_2_0_init;
        XCVR_RX_DIG->LNA_GAIN_LIN_VAL_5_3 = com_config->lna_gain_lin_val_5_3_init;
        XCVR_RX_DIG->LNA_GAIN_LIN_VAL_8_6 = com_config->lna_gain_lin_val_8_6_init;
        XCVR_RX_DIG->LNA_GAIN_LIN_VAL_9 = com_config->lna_gain_lin_val_9_init;

        /* BBA_RES_TUNE_LIN_VAL */
        XCVR_RX_DIG->BBA_RES_TUNE_LIN_VAL_3_0 = com_config->bba_res_tune_lin_val_3_0_init;
        XCVR_RX_DIG->BBA_RES_TUNE_LIN_VAL_7_4 = com_config->bba_res_tune_lin_val_7_4_init;
        XCVR_RX_DIG->BBA_RES_TUNE_LIN_VAL_10_8 = com_config->bba_res_tune_lin_val_10_8_init;

        /* BBA_STEP */
        XCVR_RX_DIG->DCOC_BBA_STEP = com_config->dcoc_bba_step_init;

        /* DCOC_TZA_STEP */
        XCVR_RX_DIG->DCOC_TZA_STEP_0 = com_config->dcoc_tza_step_00_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_1 = com_config->dcoc_tza_step_01_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_2 = com_config->dcoc_tza_step_02_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_3 = com_config->dcoc_tza_step_03_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_4 = com_config->dcoc_tza_step_04_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_5 = com_config->dcoc_tza_step_05_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_6 = com_config->dcoc_tza_step_06_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_7 = com_config->dcoc_tza_step_07_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_8 = com_config->dcoc_tza_step_08_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_9 = com_config->dcoc_tza_step_09_init;
        XCVR_RX_DIG->DCOC_TZA_STEP_10 = com_config->dcoc_tza_step_10_init;

#if (RADIO_IS_GEN_3P0 || RADIO_IS_GEN_2P1)
        /* DCOC_CAL_FAIL and DCOC_CAL_PASS */
        XCVR_RX_DIG->DCOC_CAL_FAIL_TH = com_config->dcoc_cal_fail_th_init;
        XCVR_RX_DIG->DCOC_CAL_PASS_TH = com_config->dcoc_cal_pass_th_init;
#endif /* (RADIO_IS_GEN_3P0 || RADIO_IS_GEN_2P1) */
    }

    /* AGC_CTRL_0 .. _3 */
    XCVR_RX_DIG->AGC_CTRL_0 = com_config->agc_ctrl_0_init | mode_config->agc_ctrl_0_init;

#if RF_OSC_26MHZ == 1
    {
        XCVR_RX_DIG->AGC_CTRL_1 = com_config->agc_ctrl_1_init_26mhz | datarate_config->agc_ctrl_1_init_26mhz; /* Combine common and datarate specific settings */
        XCVR_RX_DIG->AGC_CTRL_2 = mode_datarate_config->agc_ctrl_2_init_26mhz;
    }
#else
    {
        XCVR_RX_DIG->AGC_CTRL_1 = com_config->agc_ctrl_1_init_32mhz | datarate_config->agc_ctrl_1_init_32mhz; /* Combine common and datarate specific settings */
        XCVR_RX_DIG->AGC_CTRL_2 = mode_datarate_config->agc_ctrl_2_init_32mhz;
    }
#endif /* RF_OSC_26MHZ == 1 */

    if (first_init)
    {
        XCVR_RX_DIG->AGC_CTRL_3 = com_config->agc_ctrl_3_init;

        /* AGC_GAIN_TBL_** */
        XCVR_RX_DIG->AGC_GAIN_TBL_03_00 = com_config->agc_gain_tbl_03_00_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_07_04 = com_config->agc_gain_tbl_07_04_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_11_08 = com_config->agc_gain_tbl_11_08_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_15_12 = com_config->agc_gain_tbl_15_12_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_19_16 = com_config->agc_gain_tbl_19_16_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_23_20 = com_config->agc_gain_tbl_23_20_init;
        XCVR_RX_DIG->AGC_GAIN_TBL_26_24 = com_config->agc_gain_tbl_26_24_init;

        /* RSSI_CTRL_0 */
        XCVR_RX_DIG->RSSI_CTRL_0 = com_config->rssi_ctrl_0_init;

#if RADIO_IS_GEN_3P0
        XCVR_RX_DIG->RSSI_CTRL_1 = com_config->rssi_ctrl_1_init;
#endif /* RADIO_IS_GEN_3P0 */

        /* CCA_ED_LQI_0 and _1 */
        XCVR_RX_DIG->CCA_ED_LQI_CTRL_0 = com_config->cca_ed_lqi_ctrl_0_init;
        XCVR_RX_DIG->CCA_ED_LQI_CTRL_1 = com_config->cca_ed_lqi_ctrl_1_init;
    }

    /* Channel filter coefficients */
#if RF_OSC_26MHZ == 1
    {
        XCVR_RX_DIG->RX_CHF_COEF_0 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_0;
        XCVR_RX_DIG->RX_CHF_COEF_1 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_1;
        XCVR_RX_DIG->RX_CHF_COEF_2 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_2;
        XCVR_RX_DIG->RX_CHF_COEF_3 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_3;
        XCVR_RX_DIG->RX_CHF_COEF_4 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_4;
        XCVR_RX_DIG->RX_CHF_COEF_5 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_5;
        XCVR_RX_DIG->RX_CHF_COEF_6 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_6;
        XCVR_RX_DIG->RX_CHF_COEF_7 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_7;
        XCVR_RX_DIG->RX_CHF_COEF_8 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_8;
        XCVR_RX_DIG->RX_CHF_COEF_9 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_9;
        XCVR_RX_DIG->RX_CHF_COEF_10 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_10;
        XCVR_RX_DIG->RX_CHF_COEF_11 = mode_datarate_config->rx_chf_coeffs_26mhz.rx_chf_coef_11;
    }
#else
    {
        XCVR_RX_DIG->RX_CHF_COEF_0 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_0;
        XCVR_RX_DIG->RX_CHF_COEF_1 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_1;
        XCVR_RX_DIG->RX_CHF_COEF_2 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_2;
        XCVR_RX_DIG->RX_CHF_COEF_3 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_3;
        XCVR_RX_DIG->RX_CHF_COEF_4 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_4;
        XCVR_RX_DIG->RX_CHF_COEF_5 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_5;
        XCVR_RX_DIG->RX_CHF_COEF_6 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_6;
        XCVR_RX_DIG->RX_CHF_COEF_7 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_7;
        XCVR_RX_DIG->RX_CHF_COEF_8 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_8;
        XCVR_RX_DIG->RX_CHF_COEF_9 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_9;
        XCVR_RX_DIG->RX_CHF_COEF_10 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_10;
        XCVR_RX_DIG->RX_CHF_COEF_11 = mode_datarate_config->rx_chf_coeffs_32mhz.rx_chf_coef_11;
    }
#endif /* RF_OSC_26MHZ == 1 */

    XCVR_RX_DIG->RX_RCCAL_CTRL0 = mode_datarate_config->rx_rccal_ctrl_0;
    XCVR_RX_DIG->RX_RCCAL_CTRL1 = mode_datarate_config->rx_rccal_ctrl_1;

    /*******************************************************************************/
    /* XCVR_TSM configs */
    /*******************************************************************************/
    XCVR_TSM->CTRL = com_config->tsm_ctrl;

#if RADIO_IS_GEN_2P0
    if ((mode_config->radio_mode != ZIGBEE_MODE) && (mode_config->radio_mode != BLE_MODE))
    {
        XCVR_TSM->CTRL &= ~XCVR_TSM_CTRL_DATA_PADDING_EN_MASK;
    }
#endif /* RADIO_IS_GEN_2P0 */

    if (first_init)
    {
#if !RADIO_IS_GEN_2P1
        XCVR_MISC->LPPS_CTRL = com_config->lpps_ctrl_init; /* Register is in XCVR_MISC but grouped with TSM for intialization */
#endif /* !RADIO_IS_GEN_2P1 */

        XCVR_TSM->OVRD2 = com_config->tsm_ovrd2_init;
        /* TSM registers and timings - dependent upon clock frequency */
#if RF_OSC_26MHZ == 1
        {
            XCVR_TSM->END_OF_SEQ = com_config->end_of_seq_init_26mhz;
            XCVR_TSM->FAST_CTRL2 = com_config->tsm_fast_ctrl2_init_26mhz;
            XCVR_TSM->RECYCLE_COUNT = com_config->recycle_count_init_26mhz;
            XCVR_TSM->TIMING14 = com_config->tsm_timing_14_init_26mhz;
            XCVR_TSM->TIMING16 = com_config->tsm_timing_16_init_26mhz;
            XCVR_TSM->TIMING25 = com_config->tsm_timing_25_init_26mhz;
            XCVR_TSM->TIMING27 = com_config->tsm_timing_27_init_26mhz;
            XCVR_TSM->TIMING28 = com_config->tsm_timing_28_init_26mhz;
            XCVR_TSM->TIMING29 = com_config->tsm_timing_29_init_26mhz;
            XCVR_TSM->TIMING30 = com_config->tsm_timing_30_init_26mhz;
            XCVR_TSM->TIMING31 = com_config->tsm_timing_31_init_26mhz;
            XCVR_TSM->TIMING32 = com_config->tsm_timing_32_init_26mhz;
            XCVR_TSM->TIMING33 = com_config->tsm_timing_33_init_26mhz;
            XCVR_TSM->TIMING36 = com_config->tsm_timing_36_init_26mhz;
            XCVR_TSM->TIMING37 = com_config->tsm_timing_37_init_26mhz;
            XCVR_TSM->TIMING39 = com_config->tsm_timing_39_init_26mhz;
            XCVR_TSM->TIMING40 = com_config->tsm_timing_40_init_26mhz;
            XCVR_TSM->TIMING41 = com_config->tsm_timing_41_init_26mhz;
            XCVR_TSM->TIMING52 = com_config->tsm_timing_52_init_26mhz;
            XCVR_TSM->TIMING54 = com_config->tsm_timing_54_init_26mhz;
            XCVR_TSM->TIMING55 = com_config->tsm_timing_55_init_26mhz;
            XCVR_TSM->TIMING56 = com_config->tsm_timing_56_init_26mhz;
        }
#else
        {
            XCVR_TSM->END_OF_SEQ = com_config->end_of_seq_init_32mhz;
            XCVR_TSM->FAST_CTRL2 = com_config->tsm_fast_ctrl2_init_32mhz;
            XCVR_TSM->RECYCLE_COUNT = com_config->recycle_count_init_32mhz;
            XCVR_TSM->TIMING14 = com_config->tsm_timing_14_init_32mhz;
            XCVR_TSM->TIMING16 = com_config->tsm_timing_16_init_32mhz;
            XCVR_TSM->TIMING25 = com_config->tsm_timing_25_init_32mhz;
            XCVR_TSM->TIMING27 = com_config->tsm_timing_27_init_32mhz;
            XCVR_TSM->TIMING28 = com_config->tsm_timing_28_init_32mhz;
            XCVR_TSM->TIMING29 = com_config->tsm_timing_29_init_32mhz;
            XCVR_TSM->TIMING30 = com_config->tsm_timing_30_init_32mhz;
            XCVR_TSM->TIMING31 = com_config->tsm_timing_31_init_32mhz;
            XCVR_TSM->TIMING32 = com_config->tsm_timing_32_init_32mhz;
            XCVR_TSM->TIMING33 = com_config->tsm_timing_33_init_32mhz;
            XCVR_TSM->TIMING36 = com_config->tsm_timing_36_init_32mhz;
            XCVR_TSM->TIMING37 = com_config->tsm_timing_37_init_32mhz;
            XCVR_TSM->TIMING39 = com_config->tsm_timing_39_init_32mhz;
            XCVR_TSM->TIMING40 = com_config->tsm_timing_40_init_32mhz;
            XCVR_TSM->TIMING41 = com_config->tsm_timing_41_init_32mhz;
            XCVR_TSM->TIMING52 = com_config->tsm_timing_52_init_32mhz;
            XCVR_TSM->TIMING54 = com_config->tsm_timing_54_init_32mhz;
            XCVR_TSM->TIMING55 = com_config->tsm_timing_55_init_32mhz;
            XCVR_TSM->TIMING56 = com_config->tsm_timing_56_init_32mhz;
        }
#endif /* RF_OSC_26MHZ == 1 */

        /* TSM timings independent of clock frequency */
        XCVR_TSM->TIMING00 = com_config->tsm_timing_00_init;
        XCVR_TSM->TIMING01 = com_config->tsm_timing_01_init;
        XCVR_TSM->TIMING02 = com_config->tsm_timing_02_init;
        XCVR_TSM->TIMING03 = com_config->tsm_timing_03_init;
        XCVR_TSM->TIMING04 = com_config->tsm_timing_04_init;
        XCVR_TSM->TIMING05 = com_config->tsm_timing_05_init;
        XCVR_TSM->TIMING06 = com_config->tsm_timing_06_init;
        XCVR_TSM->TIMING07 = com_config->tsm_timing_07_init;
        XCVR_TSM->TIMING08 = com_config->tsm_timing_08_init;
        XCVR_TSM->TIMING09 = com_config->tsm_timing_09_init;
        XCVR_TSM->TIMING10 = com_config->tsm_timing_10_init;
        XCVR_TSM->TIMING11 = com_config->tsm_timing_11_init;
        XCVR_TSM->TIMING12 = com_config->tsm_timing_12_init;
        XCVR_TSM->TIMING13 = com_config->tsm_timing_13_init;
        XCVR_TSM->TIMING15 = com_config->tsm_timing_15_init;
        XCVR_TSM->TIMING17 = com_config->tsm_timing_17_init;
        XCVR_TSM->TIMING18 = com_config->tsm_timing_18_init;
        XCVR_TSM->TIMING19 = com_config->tsm_timing_19_init;
        XCVR_TSM->TIMING20 = com_config->tsm_timing_20_init;
        XCVR_TSM->TIMING21 = com_config->tsm_timing_21_init;
        XCVR_TSM->TIMING22 = com_config->tsm_timing_22_init;
        XCVR_TSM->TIMING23 = com_config->tsm_timing_23_init;
        XCVR_TSM->TIMING24 = com_config->tsm_timing_24_init;
        XCVR_TSM->TIMING26 = com_config->tsm_timing_26_init;
        XCVR_TSM->TIMING34 = com_config->tsm_timing_34_init;
        XCVR_TSM->TIMING35 = com_config->tsm_timing_35_init;
        XCVR_TSM->TIMING38 = com_config->tsm_timing_38_init;
        XCVR_TSM->TIMING51 = com_config->tsm_timing_51_init;
        XCVR_TSM->TIMING53 = com_config->tsm_timing_53_init;
        XCVR_TSM->TIMING57 = com_config->tsm_timing_57_init;
        XCVR_TSM->TIMING58 = com_config->tsm_timing_58_init;
		
#if RF_OSC_26MHZ == 1
        {
            XCVR_TSM->END_OF_SEQ = XCVR_TSM_END_OF_SEQ_END_OF_TX_WU(END_OF_TX_WU) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_TX_WD(END_OF_TX_WD) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_RX_WU(END_OF_RX_WU_26MHZ) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_RX_WD(END_OF_RX_WD_26MHZ);
        }
#else
        {
            XCVR_TSM->END_OF_SEQ = XCVR_TSM_END_OF_SEQ_END_OF_TX_WU(END_OF_TX_WU) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_TX_WD(END_OF_TX_WD) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_RX_WU(END_OF_RX_WU) |
                                   XCVR_TSM_END_OF_SEQ_END_OF_RX_WD(END_OF_RX_WD);
        }
#endif /* RF_OSC_26MHZ == 1 */

        XCVR_TSM->PA_RAMP_TBL0 = com_config->pa_ramp_tbl_0_init;
        XCVR_TSM->PA_RAMP_TBL1 = com_config->pa_ramp_tbl_1_init;
		
#if RADIO_IS_GEN_3P0
        XCVR_TSM->PA_RAMP_TBL2 = com_config->pa_ramp_tbl_2_init;
        XCVR_TSM->PA_RAMP_TBL3 = com_config->pa_ramp_tbl_3_init;

        /* Apply PA_RAMP_TIME == 4usec adjustments to TX_WD signals */
#if (PA_RAMP_TIME == 4)
        XCVR_TSM->TIMING00 += B1(2); /* (bb_ldo_hf_en) */
        XCVR_TSM->TIMING01 += B1(2); /* (bb_ldo_adcdac_en) */
        XCVR_TSM->TIMING03 += B1(2); /* (bb_ldo_pd_en) */
        XCVR_TSM->TIMING04 += B1(2); /* (bb_ldo_fdbk_en) */
        XCVR_TSM->TIMING05 += B1(2); /* (bb_ldo_vcolo_en) */
        XCVR_TSM->TIMING06 += B1(2); /* (bb_ldo_vtref_en) */
        XCVR_TSM->TIMING10 += B1(2); /* (bb_xtal_pll_ref_clk_en) */
        XCVR_TSM->TIMING11 += B1(2); /* (bb_xtal_dac_ref_clk_en) */
        XCVR_TSM->TIMING15 += B1(2); /* (sy_vco_en) */
        XCVR_TSM->TIMING17 += B1(2); /* (sy_lo_tx_buf_en) */
        XCVR_TSM->TIMING18 += B1(2); /* (sy_divn_en) */
        XCVR_TSM->TIMING20 += B1(2); /*  (sy_pd_en) */
        XCVR_TSM->TIMING21 += B1(2); /* (sy_lo_divn_en) */
        XCVR_TSM->TIMING23 += B1(2); /* (sy_lo_tx_en) */
        XCVR_TSM->TIMING26 += B1(2); /* (tx_pa_en) */ 
        XCVR_TSM->TIMING34 += B1(2); /* (pll_dig_en) */
        XCVR_TSM->TIMING35 += B1(2); /* (tx_dig_en) */
        XCVR_TSM->TIMING38 += B1(2); /* (sigma_delta_en) */
        XCVR_TSM->TIMING58 += B1(2)  /* (tx_hpm_dac_en) */
        temp = XCVR_TSM->TIMING14;
        temp &= 0xFFFF0000; 
        temp |= B0(END_OF_TX_WU - 4) | B1(END_OF_TX_WU + 1); /* (sy_pd_cycle_slip_ld_ft_en) */
        XCVR_TSM->TIMING14 = temp;
#endif /* (PA_RAMP_TIME == 4) */
#endif /* RADIO_IS_GEN_3P0 */
    }
    
#if RADIO_IS_GEN_3P0
    if (mode_config->radio_mode == ZIGBEE_MODE)
    {
        temp = XCVR_TSM->TIMING35;
        temp &= ~(B0(0xFF));
        if (DATA_PADDING_EN == 1)
        {
            temp |= B0(END_OF_TX_WU - 2 - 8); /* Adjust for data padding time */
        }
        else
        {
            temp |= B0(END_OF_TX_WU - 2); /* No data padding adjustment */
        }
        XCVR_TSM->TIMING35 = temp;
    }
#else

    if ((mode_datarate_config->radio_mode == MSK) && ((mode_datarate_config->data_rate == DR_500KBPS) || (mode_datarate_config->data_rate == DR_250KBPS)))
    {
        /* Apply a specific value of TX_DIG_EN which assumes no DATA PADDING */
        XCVR_TSM->TIMING35 = com_config->tsm_timing_35_init | B0(TX_DIG_EN_ASSERT_MSK500); /* LSbyte is mode specific */
    }
    else
    {
        XCVR_TSM->TIMING35 = com_config->tsm_timing_35_init | mode_config->tsm_timing_35_init; /* LSbyte is mode specific, other bytes are common */
    }
#endif /* RADIO_IS_GEN_3P0 */
    
    /*******************************************************************************/
    /* XCVR_TX_DIG configs */
    /*******************************************************************************/
#if RF_OSC_26MHZ == 1
    {
        XCVR_TX_DIG->FSK_SCALE = mode_datarate_config->tx_fsk_scale_26mhz; /* Applies only to 802.15.4 & MSK but won't harm other protocols */
        XCVR_TX_DIG->GFSK_COEFF1 = mode_config->tx_gfsk_coeff1_26mhz;
        XCVR_TX_DIG->GFSK_COEFF2 = mode_config->tx_gfsk_coeff2_26mhz;
    }
#else
    {
        XCVR_TX_DIG->FSK_SCALE = mode_datarate_config->tx_fsk_scale_32mhz; /* Applies only to 802.15.4 & MSK but won't harm other protocols */
        XCVR_TX_DIG->GFSK_COEFF1 = mode_config->tx_gfsk_coeff1_32mhz;
        XCVR_TX_DIG->GFSK_COEFF2 = mode_config->tx_gfsk_coeff2_32mhz;
    }
#endif /* RF_OSC_26MHZ == 1 */

    if (first_init)
    {
        XCVR_TX_DIG->CTRL = com_config->tx_ctrl;
        XCVR_TX_DIG->DATA_PADDING = com_config->tx_data_padding;
        XCVR_TX_DIG->DFT_PATTERN = com_config->tx_dft_pattern;
		
#if !RADIO_IS_GEN_2P1
        XCVR_TX_DIG->RF_DFT_BIST_1 = com_config->rf_dft_bist_1;
        XCVR_TX_DIG->RF_DFT_BIST_2 = com_config->rf_dft_bist_2;
#endif /* !RADIO_IS_GEN_2P1 */
    }

    XCVR_TX_DIG->GFSK_CTRL = mode_config->tx_gfsk_ctrl;

#ifndef SIMULATION
#if (TRIM_BBA_DCOC_DAC_AT_INIT)
    if (first_init)
    {
        uint32_t end_of_rx_wu = 0;
        XCVR_ForceRxWu();
        /* Wait for TSM to reach the end of warmup (unless you want to capture some samples during DCOC cal phase) */
        temp = XCVR_TSM->END_OF_SEQ;
        end_of_rx_wu = (temp & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >> XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
        while ((( XCVR_MISC->XCVR_STATUS & XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK) >> XCVR_CTRL_XCVR_STATUS_TSM_COUNT_SHIFT ) != end_of_rx_wu) {};

//        if (!rx_bba_dcoc_dac_trim_shortIQ())
        if (!rx_bba_dcoc_dac_trim_DCest())
        {
            config_status = gXcvrTrimFailure_c;
        }

        XCVR_ForceRxWd();
        DCOC_DAC_INIT_Cal(1);
    }
#endif /* TRIM_BBA_DCOC_DAC_AT_INIT */
#endif /* ifndef SIMULATION */
    return config_status;
}

void XCVR_Reset(void)
{
#if RADIO_IS_GEN_3P0
#else
    RSIM->CONTROL |= RSIM_CONTROL_RADIO_RESET_BIT_MASK; /* Assert radio software reset */
    RSIM->CONTROL &= ~RSIM_CONTROL_RADIO_RESET_BIT_MASK; /* De-assert radio software reset */
    RSIM->CONTROL &= ~RSIM_CONTROL_RADIO_RESET_BIT_MASK; /* De-assert radio software reset a second time per RADIO_RESET bit description */
#endif /* RADIO_IS_GEN_3P0 */
}

xcvrStatus_t XCVR_ChangeMode (radio_mode_t new_radio_mode, data_rate_t new_data_rate) /* Change from one radio mode to another */
{
    xcvrStatus_t status;
    const xcvr_mode_datarate_config_t * mode_datarate_config;
    const xcvr_datarate_config_t * datarate_config ;
    const xcvr_mode_config_t * radio_mode_cfg;
    const xcvr_common_config_t * radio_common_config;

    status = XCVR_GetDefaultConfig(new_radio_mode, new_data_rate, (void *)&radio_common_config, (void *)&radio_mode_cfg, (void *)&mode_datarate_config, (void *)&datarate_config );
    
    if (status == gXcvrSuccess_c)
    {
        status = XCVR_Configure((const xcvr_common_config_t *)radio_common_config, 
                                (const xcvr_mode_config_t *)radio_mode_cfg, 
                                (const xcvr_mode_datarate_config_t *)mode_datarate_config, 
                                (const xcvr_datarate_config_t *)datarate_config, 25, XCVR_MODE_CHANGE);
        current_xcvr_config.radio_mode = new_radio_mode;
        current_xcvr_config.data_rate = new_data_rate;
    }

    return status;
}

void XCVR_EnaNBRSSIMeas( uint8_t IIRnbEnable )
{
    if (IIRnbEnable)
    {
        XCVR_RX_DIG->RSSI_CTRL_0 |= XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_MASK;
    }
    else
    {
        XCVR_RX_DIG->RSSI_CTRL_0 &= ~XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_MASK;
    }
}

xcvrStatus_t XCVR_OverrideFrequency ( uint32_t freq, uint32_t refOsc )
{
    double integer_used_in_Hz, 
           integer_used_in_LSB, 
           numerator_fraction, 
           numerator_in_Hz, 
           numerator_in_LSB, 
           numerator_unrounded,
           real_int_and_fraction, 
           real_fraction, 
           requested_freq_in_LSB,
           sdm_lsb;
    uint32_t temp;
    static uint32_t integer_truncated,
                    integer_to_use;
    static int32_t numerator_rounded;

    /* Configure for Coarse Tune */
    uint32_t coarse_tune_target = freq / 1000000;

    temp = XCVR_PLL_DIG->CTUNE_CTRL;
    temp &= ~XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL_MASK;
    temp |= XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL(coarse_tune_target);
    XCVR_PLL_DIG->CTUNE_CTRL = temp;

    /* Calculate the Low Port values */
    sdm_lsb = refOsc / 131072.0;

    real_int_and_fraction = freq / (refOsc * 2.0);

    integer_truncated = (uint32_t) trunc(real_int_and_fraction);

    real_fraction = real_int_and_fraction - integer_truncated;

    if (real_fraction > 0.5) 
    {
        integer_to_use = integer_truncated + 1;
    } 
    else                     
    {
        integer_to_use = integer_truncated;
    }

    numerator_fraction = real_int_and_fraction - integer_to_use;

    integer_used_in_Hz  = integer_to_use * refOsc * 2;
    integer_used_in_LSB = integer_used_in_Hz / sdm_lsb;

    numerator_in_Hz  = numerator_fraction * refOsc * 2;
    numerator_in_LSB = numerator_in_Hz    / sdm_lsb;

    requested_freq_in_LSB = integer_used_in_LSB + numerator_in_LSB;

    numerator_unrounded = (requested_freq_in_LSB - integer_used_in_LSB) * 256;

    numerator_rounded = (int32_t)round(numerator_unrounded);

    /* Write the Low Port Integer and Numerator */
    temp = XCVR_PLL_DIG->LPM_SDM_CTRL1;
    temp &= ~XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_MASK;
    temp |= (XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG(integer_to_use) | 
             XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK);
    XCVR_PLL_DIG->LPM_SDM_CTRL1 = temp;

    XCVR_PLL_DIG->LPM_SDM_CTRL2 = numerator_rounded;

    return gXcvrSuccess_c;
}

void XCVR_RegisterPanicCb ( panic_fptr fptr ) /* Allow upper layers to provide PANIC callback */
{
    s_PanicFunctionPtr = fptr;
}

void XcvrPanic(XCVR_PANIC_ID_T panic_id, uint32_t panic_address)
{
    if ( s_PanicFunctionPtr != NULL)
    {
        s_PanicFunctionPtr(panic_id, panic_address, 0, 0);
    }
    else
    {
        uint8_t dummy;

        while(1)
        {
            dummy = dummy;
        }
    }
}

healthStatus_t XCVR_HealthCheck ( void ) /* Allow upper layers to poll the radio health */
{
    return (healthStatus_t)NO_ERRORS;
}

void XCVR_FadLppsControl(FAD_LPPS_CTRL_T control)
{

}

/* Helper function to map radio mode to LL usage */
link_layer_t map_mode_to_ll(radio_mode_t mode)
{
    link_layer_t llret;
    switch (mode)
    {
        case BLE_MODE:
            llret = BLE_LL;
            break;
        case ZIGBEE_MODE:
            llret = ZIGBEE_LL;
            break;
        case ANT_MODE:
            llret = ANT_LL;
            break;
        case GFSK_BT_0p5_h_0p5:
        case GFSK_BT_0p5_h_0p32:
        case GFSK_BT_0p5_h_0p7:
        case GFSK_BT_0p5_h_1p0:
        case GFSK_BT_0p3_h_0p5:
        case GFSK_BT_0p7_h_0p5:
        case MSK: 
            llret = GENFSK_LL;
            break;
        default:
            llret = UNASSIGNED_LL;
            break;
    }
    return llret;
}

#if RADIO_IS_GEN_3P0
void XCVR_SetBSM_NTW_Address(uint32_t bsm_ntw_address)
{
    XCVR_PHY->NTW_ADR_BSM = bsm_ntw_address;
}

uint32_t XCVR_GetBSM_NTW_Address(void)
{
    return XCVR_PHY->NTW_ADR_BSM;
}
#endif /* RADIO_IS_GEN_3P0 */

/* Setup IRQ mapping to LL interrupt outputs in XCVR_CTRL */
xcvrStatus_t XCVR_SetIRQMapping(radio_mode_t irq0_mapping, radio_mode_t irq1_mapping)
{
    link_layer_t int0 = map_mode_to_ll(irq0_mapping);
    link_layer_t int1 = map_mode_to_ll(irq1_mapping);
    xcvrStatus_t statusret;
    /* Make sure the two LL's requested aren't the same */
    if (int0 == int1)
    {
        statusret = gXcvrInvalidParameters_c;
    }
    else
    {
        uint32_t temp;
        temp = XCVR_MISC->XCVR_CTRL;
        temp &= ~(XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_MASK | XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_MASK);
        temp |= (XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL(int0) | XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL(int1));
        XCVR_MISC->XCVR_CTRL = temp;
        statusret = gXcvrSuccess_c;
    }
    return statusret;
}

/* Get current state of IRQ mapping for either  radio INT0 or INT1 */
link_layer_t XCVR_GetIRQMapping(uint8_t int_num)
{
    if (int_num == 0)
    {
        return (link_layer_t)((XCVR_MISC->XCVR_CTRL & XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_MASK)>>XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_SHIFT);
    }
    else
    {
        return (link_layer_t)((XCVR_MISC->XCVR_CTRL & XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_MASK)>>XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_SHIFT);
    }
}

/* Get current state of radio mode and data rate */
xcvrStatus_t XCVR_GetCurrentConfig(xcvr_currConfig_t * curr_config)
{
    xcvrStatus_t status = gXcvrInvalidParameters_c;
    if (curr_config != NULL)
    {
        curr_config->radio_mode = current_xcvr_config.radio_mode;
        curr_config->data_rate = current_xcvr_config.data_rate;
        status = gXcvrSuccess_c;
    }
    return status;
}

/* Customer level trim functions */
xcvrStatus_t XCVR_SetXtalTrim(uint8_t xtalTrim)
{
    xcvrStatus_t status = gXcvrInvalidParameters_c;

    if ((xtalTrim & 0x80) == 0)
    {
        uint32_t temp;
        temp = RSIM->ANA_TRIM;
        temp &= ~RSIM_ANA_TRIM_BB_XTAL_TRIM_MASK;
        RSIM->ANA_TRIM = temp | RSIM_ANA_TRIM_BB_XTAL_TRIM(xtalTrim);
        status = gXcvrSuccess_c;
    }
    return status;
}

uint8_t  XCVR_GetXtalTrim(void)
{
    uint8_t temp_xtal;
    temp_xtal = ((RSIM->ANA_TRIM & RSIM_ANA_TRIM_BB_XTAL_TRIM_MASK)>>RSIM_ANA_TRIM_BB_XTAL_TRIM_SHIFT);
    return temp_xtal;
}

/* RSSI adjustment */
xcvrStatus_t XCVR_SetRssiAdjustment(int8_t adj)
{
    XCVR_RX_DIG->RSSI_CTRL_0 &= ~XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_MASK;
    XCVR_RX_DIG->RSSI_CTRL_0 |= XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ(adj);
    return gXcvrSuccess_c;
}

int8_t  XCVR_GetRssiAdjustment(void)
{
    int8_t adj;
    adj = (XCVR_RX_DIG->RSSI_CTRL_0 & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_MASK) >> XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_SHIFT;
    return adj;
}

/* Radio debug functions */
xcvrStatus_t XCVR_OverrideChannel(uint8_t channel, uint8_t useMappedChannel)
{
    uint32_t temp;

    if (channel == 0xFF)
    {
        /* Clear all of the overrides and restore to LL channel control */
        temp = XCVR_PLL_DIG->CHAN_MAP;
        temp &= ~(XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_MASK | XCVR_PLL_DIG_CHAN_MAP_BOC_MASK 
#if !RADIO_IS_GEN_2P1
                 | XCVR_PLL_DIG_CHAN_MAP_ZOC_MASK
#endif /* !RADIO_IS_GEN_2P1 */
#if RADIO_IS_GEN_3P0
                 | XCVR_PLL_DIG_CHAN_MAP_HOP_TBL_CFG_OVRD_EN_MASK
#endif /* RADIO_IS_GEN_3P0 */
                 );

      XCVR_PLL_DIG->CHAN_MAP = temp; 

      /* Stop using the manual frequency setting */
      XCVR_PLL_DIG->LPM_SDM_CTRL1 &= ~XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK;

      return gXcvrSuccess_c;
    }

    if (channel >= 128)
    {
        return gXcvrInvalidParameters_c;
    }

    if (useMappedChannel)
    {
        temp = (XCVR_MISC->XCVR_CTRL & XCVR_CTRL_XCVR_CTRL_PROTOCOL_MASK)>>XCVR_CTRL_XCVR_CTRL_PROTOCOL_SHIFT; /* Extract PROTOCOL bitfield */

        switch (temp)
        {
#if !RADIO_IS_GEN_2P1
            case 0x3: /* ANT protocol */
                ANT->CHANNEL_NUM = channel;
                break;
#endif /* !RADIO_IS_GEN_2P1 */
            case 0x8: /* GENFSK protocol */
            case 0x9: /* MSK protocol */
                GENFSK->CHANNEL_NUM = channel;
                break;
            default: /* All other protocols */
                temp = XCVR_PLL_DIG->CHAN_MAP;
                temp &= ~(XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_MASK
#if RADIO_IS_GEN_3P0
                         | XCVR_PLL_DIG_CHAN_MAP_HOP_TBL_CFG_OVRD_EN_MASK
#endif /* RADIO_IS_GEN_3P0 */
                         );
                temp |= (XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM(channel) | XCVR_PLL_DIG_CHAN_MAP_BOC_MASK
#if !RADIO_IS_GEN_2P1
                        | XCVR_PLL_DIG_CHAN_MAP_ZOC_MASK
#endif /* !RADIO_IS_GEN_2P1 */
                        );
                XCVR_PLL_DIG->CHAN_MAP = temp; 
                break;
        }
    }
    else
    {
        XCVR_PLL_DIG->CHAN_MAP |= (XCVR_PLL_DIG_CHAN_MAP_BOC_MASK
#if !RADIO_IS_GEN_2P1
                                  | XCVR_PLL_DIG_CHAN_MAP_ZOC_MASK
#endif /* !RADIO_IS_GEN_2P1 */
                                  ); 

        XCVR_PLL_DIG->LPM_SDM_CTRL3 = XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM(gPllDenom_c);
        XCVR_PLL_DIG->LPM_SDM_CTRL2 = XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM(mapTable[channel].numerator);

        temp = XCVR_PLL_DIG->LPM_SDM_CTRL1;
        temp &= ~XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_MASK;
        temp |= XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG(mapTable[channel].integer);
        XCVR_PLL_DIG->LPM_SDM_CTRL1 = temp;

        /* Stop using the LL channel map and use the manual frequency setting */
        XCVR_PLL_DIG->LPM_SDM_CTRL1 |= XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK;
    }

    return gXcvrSuccess_c;
}

uint32_t XCVR_GetFreq ( void )
{
    uint32_t pll_int;
    uint32_t pll_num_unsigned;
    int32_t pll_num;
    uint32_t pll_denom;
    float freq_float;

    if (XCVR_PLL_DIG->LPM_SDM_CTRL1 & XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK) /* Not using mapped channels */
    {
        pll_int = (XCVR_PLL_DIG->LPM_SDM_CTRL1 & XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_MASK) >>
                   XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SHIFT;

        pll_num_unsigned = XCVR_PLL_DIG->LPM_SDM_CTRL2;
        pll_denom = XCVR_PLL_DIG->LPM_SDM_CTRL3;
    }
    else
    {
        /* Using mapped channels so need to read from the _SELECTED fields to get the values being used */
        pll_int = (XCVR_PLL_DIG->LPM_SDM_CTRL1 & XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_MASK) >>
                   XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_SHIFT;

        pll_num_unsigned = XCVR_PLL_DIG->LPM_SDM_RES1;
        pll_denom = XCVR_PLL_DIG->LPM_SDM_RES2;
    }

    uint32_t freq = 0;

#if RF_OSC_26MHZ == 1
    uint32_t ref_clk = 26U;
#else
    uint32_t ref_clk = 32U;
#endif /* RF_OSC_26MHZ == 1 */

    /* Check if sign bit is asserted */
    if (pll_num_unsigned & 0x04000000U)
    {
        /* Sign extend the numerator */
        pll_num = (~pll_num_unsigned + 1) & 0x03FFFFFFU; 

        /* Calculate the frequency in MHz */
        freq_float = (ref_clk * 2 * (pll_int - ((float)pll_num / pll_denom))); 
    }
    else
    {
        /* Calculate the frequency in MHz */
        pll_num = pll_num_unsigned;
        freq_float = (ref_clk * 2 * (pll_int + ((float)pll_num / (float)pll_denom))); 
    }

    freq = (uint32_t)freq_float;

    return freq;
}

void XCVR_ForceRxWu(void)
{
    XCVR_TSM->CTRL |= XCVR_TSM_CTRL_FORCE_RX_EN_MASK;
}

void XCVR_ForceRxWd(void)
{
    XCVR_TSM->CTRL &= ~XCVR_TSM_CTRL_FORCE_RX_EN_MASK;
}

void XCVR_ForceTxWu(void)
{
    XCVR_TSM->CTRL |= XCVR_TSM_CTRL_FORCE_TX_EN_MASK;
}

void XCVR_ForceTxWd(void)
{
    XCVR_TSM->CTRL &= ~XCVR_TSM_CTRL_FORCE_TX_EN_MASK;
}

xcvrStatus_t XCVR_DftTxCW(uint16_t rf_channel_freq, uint8_t protocol)
{
    uint32_t temp;
    if ((protocol != 6) && (protocol != 7))
    {
        return gXcvrInvalidParameters_c; /* Failure */
    }

    if ((rf_channel_freq < 2360) || (rf_channel_freq >2487))
    {
        return gXcvrInvalidParameters_c; /* failure */
    }

    /* Set the DFT Mode */
    temp  = XCVR_TX_DIG->CTRL;
    temp &= ~XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK;
    temp |= XCVR_TX_DIG_CTRL_RADIO_DFT_MODE(1);
    XCVR_TX_DIG->CTRL = temp;

    /* Choose Protocol 6 or 7 if using the Channel Number register */
    temp  = XCVR_MISC->XCVR_CTRL;
    temp &= ~XCVR_CTRL_XCVR_CTRL_PROTOCOL_MASK;
    temp |= XCVR_CTRL_XCVR_CTRL_PROTOCOL(protocol);
    XCVR_MISC->XCVR_CTRL = temp;

    /* Select the RF Channel, using the Channel Number register */
    XCVR_OverrideChannel(rf_channel_freq-2360,1);

    /* Warm-up the Radio */
    XCVR_ForceTxWu();

    return gXcvrSuccess_c; /* Success */
}

xcvrStatus_t XCVR_DftTxPatternReg(uint16_t channel_num, radio_mode_t radio_mode, data_rate_t data_rate, uint32_t tx_pattern)
{
    uint32_t temp;
    uint8_t dft_mode = 0;
    uint8_t dft_clk_sel = 0;
    xcvrStatus_t status = gXcvrSuccess_c;
    
    XCVR_ChangeMode(radio_mode, data_rate);
    
    /* Select the RF Channel, using the Channel Number register */
    XCVR_OverrideChannel(channel_num, 1);

    switch (radio_mode)
    {
        case ZIGBEE_MODE:
            dft_mode = 6; /* OQPSK configuration */
            break;
        case ANT_MODE:
        case BLE_MODE:
        case GFSK_BT_0p5_h_0p5:
        case GFSK_BT_0p5_h_0p32:
        case GFSK_BT_0p5_h_0p7:
        case GFSK_BT_0p5_h_1p0:
        case GFSK_BT_0p3_h_0p5:
        case GFSK_BT_0p7_h_0p5:
            dft_mode = 2; /* GFSK configuration */
            break;
        case MSK:
            dft_mode = 4; /* MSK configuration */
            break;
        default:
            status = gXcvrInvalidParameters_c;
            break;
    }

    if (status == gXcvrSuccess_c) /* Only attempt this pointer assignment process if prior switch() statement completed successfully */
    {
        switch (data_rate)
        {
            case DR_1MBPS:
                dft_clk_sel = 4;
                break;
            case DR_500KBPS:
                dft_clk_sel = 3;
                break;
            case DR_250KBPS:
                dft_clk_sel = 2;
                break;
            default:
                status = gXcvrInvalidParameters_c;
                break;
        }
    }

    temp = XCVR_TX_DIG->CTRL;
    temp &= ~(XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK | XCVR_TX_DIG_CTRL_DFT_CLK_SEL_MASK | XCVR_TX_DIG_CTRL_TX_DFT_EN_MASK | XCVR_TX_DIG_CTRL_LFSR_EN_MASK);
    temp |= XCVR_TX_DIG_CTRL_RADIO_DFT_MODE(dft_mode) |
            XCVR_TX_DIG_CTRL_DFT_CLK_SEL(dft_clk_sel) |
            XCVR_TX_DIG_CTRL_TX_DFT_EN(1) |
            XCVR_TX_DIG_CTRL_LFSR_EN(0);
    XCVR_TX_DIG->CTRL = temp;

    XCVR_TX_DIG->DFT_PATTERN = tx_pattern;

    if (status == gXcvrSuccess_c)
    {
        /* Warm-up the Radio */
        XCVR_ForceTxWu();
    }

    return status;
}

xcvrStatus_t XCVR_DftTxLfsrReg(uint16_t channel_num, radio_mode_t radio_mode, data_rate_t data_rate, uint8_t lfsr_length)
{
    uint32_t temp;
    uint8_t dft_mode = 0;
    uint8_t dft_clk_sel = 0;
    xcvrStatus_t status = gXcvrSuccess_c;
    uint8_t bitrate_setting = 0xFF;

    if (lfsr_length > 5)
    {
        return gXcvrInvalidParameters_c;
    }
    
    XCVR_ChangeMode(radio_mode, data_rate);
    
    /* Select the RF Channel, using the Channel Number register */
    XCVR_OverrideChannel(channel_num, 1);

    switch (radio_mode)
    {
        case ZIGBEE_MODE:
            dft_mode = 7; /* OQPSK configuration */
            break;
        case ANT_MODE:
        case BLE_MODE:
        case GFSK_BT_0p5_h_0p5:
        case GFSK_BT_0p5_h_0p32:
        case GFSK_BT_0p5_h_0p7:
        case GFSK_BT_0p5_h_1p0:
        case GFSK_BT_0p3_h_0p5:
        case GFSK_BT_0p7_h_0p5:
            dft_mode = 3; /* GFSK configuration */
            bitrate_setting = data_rate;
            break;
        case MSK:
            dft_mode = 5; /* MSK configuration */
            break;

        default:
            status = gXcvrInvalidParameters_c;
            break;
    }

    if (status == gXcvrSuccess_c)
    {
        switch (data_rate)
        {
            case DR_1MBPS:
                dft_clk_sel = 4;
                break;
            case DR_500KBPS:
                dft_clk_sel = 3;
                break;
            case DR_250KBPS:
                dft_clk_sel = 2;
                break;
            default:
                status = gXcvrInvalidParameters_c;
                break;
        }
    }

    if (bitrate_setting < 4)
    {
        GENFSK->BITRATE = bitrate_setting;
    }

    temp = XCVR_TX_DIG->CTRL;
    temp &= ~(XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK |
              XCVR_TX_DIG_CTRL_LFSR_LENGTH_MASK |
              XCVR_TX_DIG_CTRL_DFT_CLK_SEL_MASK |
              XCVR_TX_DIG_CTRL_TX_DFT_EN_MASK |
              XCVR_TX_DIG_CTRL_LFSR_EN_MASK);
    temp |= XCVR_TX_DIG_CTRL_RADIO_DFT_MODE(dft_mode) |
            XCVR_TX_DIG_CTRL_LFSR_LENGTH(lfsr_length) |
            XCVR_TX_DIG_CTRL_DFT_CLK_SEL(dft_clk_sel) |
            XCVR_TX_DIG_CTRL_TX_DFT_EN(0) |
            XCVR_TX_DIG_CTRL_LFSR_EN(1);
    XCVR_TX_DIG->CTRL = temp;

    if (status == gXcvrSuccess_c)
    {
        /* Warm-up the Radio */
        XCVR_ForceTxWu();
    }

    return status;
}

void XCVR_DftTxOff(void)
{
    XCVR_ForceTxWd();
    XCVR_MISC->XCVR_CTRL |= XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_MASK; /* Use PA_POWER in LL registers */
    /* Clear the RF Channel over-ride */
    XCVR_OverrideChannel(0xFF,1);
    XCVR_TX_DIG->CTRL &= ~(XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK | /* Clear DFT_MODE */
                           XCVR_TX_DIG_CTRL_DFT_CLK_SEL_MASK | /* Clear DFT_CLK_SEL */
                           XCVR_TX_DIG_CTRL_TX_DFT_EN_MASK | /* Clear DFT_EN */
                           XCVR_TX_DIG_CTRL_LFSR_EN_MASK);/* Clear LFSR_EN */
}

xcvrStatus_t XCVR_ForcePAPower(uint8_t pa_power)
{
    if (pa_power > 0x3F)
    {
        return gXcvrInvalidParameters_c; /* Failure */
    }
    
    if (pa_power != 1)
    {
        pa_power = pa_power & 0xFEU; /* Ensure LSbit is cleared */
    }
    
    XCVR_MISC->XCVR_CTRL &= ~XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_MASK; /* Use PA_POWER in TSM registers */
    XCVR_TSM->PA_POWER = pa_power;

    return gXcvrSuccess_c; /* Success */
}

xcvrStatus_t XCVR_CoexistenceInit(void)
{
#if gMWS_UseCoexistence_d  
    uint32_t temp = 0x00U;
    uint32_t end_of_tx_wu = 0x00U;
    uint32_t end_of_rx_wu = 0x00U;
    
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
#if (XCVR_COEX_RF_ACTIVE_PIN == ANT_A)
    uint32_t tsm_timing47 = 0x00U;
#else /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_B) */
    uint32_t tsm_timing48 = 0x00U;
#endif /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_A) */
    uint32_t tsm_timing50 = 0x00U;    
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
    
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    // RF_ACTIVE = ANT_B (PTC1, gpio1_trig_en)
    uint32_t tsm_timing48 = 0x00U;
    // RF_PRIORITY = ANT_A (PTC4, gpio0_trig_en)
    uint32_t tsm_timing47 = 0x00U;    
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */
    
    uint16_t tsm_timing43_rx = 0x00;
    uint16_t tsm_timing43_tx = 0x00;    

    /* Select GPIO mode for FAD pins */
    temp = XCVR_MISC->FAD_CTRL;
    temp &= ~(XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_MASK);
    XCVR_MISC->FAD_CTRL = temp;

    /* Read the END_OF_TX_WU and END_OF_RX_WU for XCVR */
    end_of_tx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK) >>
                    XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT;
    end_of_rx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >>
                    XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
    
/*****************
 *  TX SEQUENCE  *
 *****************/    

    if (end_of_tx_wu < gMWS_CoexRfActiveAssertTime_d)
    {
        temp = end_of_tx_wu;
    }
    else
    {
        temp = gMWS_CoexRfActiveAssertTime_d;
    }
    
    /* Save the TX RF_ACTIVE start time. */
    tsm_timing43_tx = end_of_tx_wu - temp; 

#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)    
    /* Set RF_ACTIVE pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any TX sequence. */
#if (XCVR_COEX_RF_ACTIVE_PIN == ANT_A)
    tsm_timing47 = (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT) & 
                                                        XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK);
#else
    tsm_timing48 = (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_SHIFT) & 
                                                        XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK);    
#endif /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_A) */

    /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any TX sequence. */
    tsm_timing50 = (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_SHIFT) & 
                                                        XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK);
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */

#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    /* Set RF_ACTIVE pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any TX sequence. */
    tsm_timing48 = (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_SHIFT) & 
                                                        XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK);

    /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any TX sequence. */
    tsm_timing47 = (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT) & 
                                                        XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK);        
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */  
    
/*****************
 *  RX SEQUENCE  *
 *****************/

    if (end_of_rx_wu < gMWS_CoexRfActiveAssertTime_d)
    {
        temp = end_of_rx_wu;
    }
    else
    {
        temp = gMWS_CoexRfActiveAssertTime_d;
    }

    /* Save the RX RF_ACTIVE start time. */
    tsm_timing43_rx = end_of_rx_wu - temp; 

#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)    
    /* Set RF_ACTIVE pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence. */
#if (XCVR_COEX_RF_ACTIVE_PIN == ANT_A)
    tsm_timing47 |= (((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT) & 
                                                         XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK);
#else
    tsm_timing48 |= (((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_SHIFT) & 
                                                         XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK);
#endif /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_A) */

    /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence and clear it gMWS_CoexPrioSignalTime_d uS before RX start. */
    tsm_timing50 |= ((((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_SHIFT) & 
                                                          XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK) |
                     (((uint32_t)(end_of_rx_wu - gMWS_CoexPrioSignalTime_d) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_SHIFT) & 
                                                                             XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK));

#if (XCVR_COEX_RF_ACTIVE_PIN == ANT_A)
    temp = XCVR_TSM->TIMING47;
    temp &= ~(XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK | XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK);
    temp |= tsm_timing47;
    XCVR_TSM->TIMING47 = temp;
#else
    temp = XCVR_TSM->TIMING48;
    temp &= ~(XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK | XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK);
    temp |= tsm_timing48;
    XCVR_TSM->TIMING48 = temp;    
#endif /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_A) */

    temp = XCVR_TSM->TIMING50;
    temp &= ~(XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK | 
              XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK |
              XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK);
    temp |= tsm_timing50;
    XCVR_TSM->TIMING50 = temp;

#if (XCVR_COEX_RF_ACTIVE_PIN == ANT_A)
    GPIOC->PDDR |= 0x18;
    PORTC->PCR[4] = (PORTC->PCR[4] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);
    PORTC->PCR[3] = (PORTC->PCR[3] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);
#else
    GPIOC->PDDR |= 0x0A;
    PORTC->PCR[1] = (PORTC->PCR[1] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);
    PORTC->PCR[3] = (PORTC->PCR[3] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);
#endif /* (XCVR_COEX_RF_ACTIVE_PIN == ANT_A) */
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
    
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)    
    /* Set RF_ACTIVE pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence. */
    tsm_timing48 |= (((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_SHIFT) & 
                                                         XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK);
    
    /* Set PRIORITY pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence and clear it gMWS_CoexPrioSignalTime_d uS before RX start. */
    tsm_timing47 |= (((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT) & 
                                                          XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK);
    
    /* RF_ACTIVE */
    temp = XCVR_TSM->TIMING48;
    temp &= ~(XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK | XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK);
    temp |= tsm_timing48;
    XCVR_TSM->TIMING48 = temp;
    
    /* RF_PRIORITY */
    temp = XCVR_TSM->TIMING47;
    temp &= ~(XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK | XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK);
    temp |= tsm_timing47;
    XCVR_TSM->TIMING47 = temp;
    
    /* Overwrite pins settings */
    GPIOC->PDDR |= 0x12;
    PORTC->PCR[4] = (PORTC->PCR[4] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);
    PORTC->PCR[1] = (PORTC->PCR[1] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(2);    
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */

    tsm_timing43_tx += gMWS_CoexConfirmWaitTime_d;

    if (tsm_timing43_tx > end_of_tx_wu - 1)
    {
        tsm_timing43_tx = end_of_tx_wu - 1;
    }

    tsm_timing43_rx += gMWS_CoexConfirmWaitTime_d;
    
    if (tsm_timing43_rx > end_of_rx_wu - 1)
    {
        tsm_timing43_rx = end_of_rx_wu - 1;
    }

    XCVR_TSM->TIMING43 = ((((uint32_t)(tsm_timing43_tx) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_SHIFT) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_MASK) |
                          (((uint32_t)(tsm_timing43_tx + 2) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_SHIFT) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_MASK) | 
                          (((uint32_t)(tsm_timing43_rx) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_SHIFT) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_MASK) |
                          (((uint32_t)(tsm_timing43_rx + 2) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_SHIFT) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_MASK));

    BTLE_RF->MISC_CTRL = 0x02;

    XCVR_TSM->CTRL |= XCVR_TSM_CTRL_TSM_IRQ0_EN_MASK;

    /* Save the updated registers values. */
    XCVR_CoexistenceSaveRestoreTimings(1);    
#endif /* gMWS_UseCoexistence_d */ 

    return gXcvrSuccess_c;
}

xcvrStatus_t XCVR_CoexistenceSetPriority(XCVR_COEX_PRIORITY_T rxPriority, XCVR_COEX_PRIORITY_T txPriority)
{
#if gMWS_UseCoexistence_d
    uint32_t temp = 0x00U;
    uint32_t end_of_tx_wu = 0x00U;
    uint32_t end_of_rx_wu = 0x00U;
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
    uint32_t tsm_timing50 = 0x00U;
#endif    
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    uint32_t tsm_timing47 = 0x00U;
#endif    

    /* Read the END_OF_TX_WU and END_OF_RX_WU for XCVR */
    end_of_tx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK) >>
                    XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT;
    end_of_rx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >>
                    XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;    

/*****************
 *      RX       *
 *****************/
    
    if (XCVR_COEX_HIGH_PRIO == rxPriority)
    {
        if (end_of_rx_wu < gMWS_CoexRfActiveAssertTime_d)
        {
            temp = end_of_rx_wu;
        }
        else
        {
            temp = gMWS_CoexRfActiveAssertTime_d;
        }

#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
        /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence and clear it gMWS_CoexPrioSignalTime_d uS before RX start for high priority RX. */
        tsm_timing50 = ((((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_SHIFT) & 
                                                             XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK) |
                        (((uint32_t)(end_of_rx_wu - gMWS_CoexPrioSignalTime_d) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_SHIFT) & 
                                                                                  XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK));
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
     /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any RX sequence */
        tsm_timing47 = (((uint32_t)(end_of_rx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT) & 
                                                            XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK);   
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */
    }
    else
    {
        /* Low priority RX */
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
        tsm_timing50 = (((0xFFU << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_SHIFT) & 
                                   XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK) |
                        ((0xFFU << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_SHIFT) & 
                                   XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK));
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)        
        tsm_timing47 = (((0xFFU << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT) & 
                                   XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK) |
                        ((0xFFU << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_SHIFT) & 
                                   XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_MASK));
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */
    }

/*****************
 *      TX       *
 *****************/
    if (XCVR_COEX_HIGH_PRIO == txPriority)
    {
        if (end_of_tx_wu < gMWS_CoexRfActiveAssertTime_d)
        {
            temp = end_of_tx_wu;
        }
        else
        {
            temp = gMWS_CoexRfActiveAssertTime_d;
        }

        /* Set STATUS pin HIGH gMWS_CoexRfActiveAssertTime_d uS prior to any TX sequence for HIGH priority TX. */
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
        tsm_timing50 |= (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_SHIFT) & 
                                                             XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK);
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)        
        tsm_timing47 |= (((uint32_t)(end_of_tx_wu - temp) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT) & 
                                                             XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK);
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */                
    }
    else
    {        
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
        /* Set STATUS pin HIGH at END_OF_TX_WU prior to any TX sequence for LOW priority TX. */
        tsm_timing50 |= (((uint32_t)(end_of_tx_wu) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_SHIFT) & 
                                                      XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK);
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
        /* Set STATUS pin LOW at END_OF_TX_WU prior to any TX sequence for LOW priority TX. */                
        tsm_timing47 = (((0xFFU << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT) & 
                                   XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK) |
                        ((0xFFU << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_SHIFT) & 
                                   XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_MASK));
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */
    }

#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
    temp = XCVR_TSM->TIMING50;
    temp &= ~(XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK |
              XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK |
              XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK);
    temp |= tsm_timing50;
    XCVR_TSM->TIMING50 = temp;
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d) */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    temp = XCVR_TSM->TIMING47;
    temp &= ~(XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK |
              XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_MASK |
              XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK |
              XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_MASK);
    temp |= tsm_timing47;
    XCVR_TSM->TIMING47 = temp;    
#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */
    
    /* Save the updated registers values. */
    XCVR_CoexistenceSaveRestoreTimings(1);
#endif /* gMWS_UseCoexistence_d */

    return gXcvrSuccess_c;
}

xcvrStatus_t XCVR_CoexistenceSaveRestoreTimings(uint8_t saveTimings)
{
#if gMWS_UseCoexistence_d
    static uint32_t tsm_ovrd0_saved = 0x00;
    static uint32_t tsm_ovrd1_saved = 0x00;
    static uint32_t tsm_ovrd2_saved = 0x00;
    static uint32_t tsm_ovrd3_saved = 0x00;
    static uint32_t tsm_timing47_saved = 0x00;
    static uint32_t tsm_timing48_saved = 0x00;
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)    
    static uint32_t tsm_timing49_saved = 0x00;
    static uint32_t tsm_timing50_saved = 0x00;
#endif    

    if (saveTimings == 0)
    {
        /* Restore registers values. */
        XCVR_TSM->OVRD0 = tsm_ovrd0_saved; 
        XCVR_TSM->OVRD1 = tsm_ovrd1_saved;
        XCVR_TSM->OVRD2 = tsm_ovrd2_saved;
        XCVR_TSM->OVRD3 = tsm_ovrd3_saved;

        XCVR_TSM->TIMING47 = tsm_timing47_saved;
        XCVR_TSM->TIMING48 = tsm_timing48_saved;
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)        
        XCVR_TSM->TIMING49 = tsm_timing49_saved;
        XCVR_TSM->TIMING50 = tsm_timing50_saved;
#endif        
    }
    else
    {
        /* Save registers values. */
        tsm_ovrd0_saved = XCVR_TSM->OVRD0;
        tsm_ovrd1_saved = XCVR_TSM->OVRD1;
        tsm_ovrd2_saved = XCVR_TSM->OVRD2;
        tsm_ovrd3_saved = XCVR_TSM->OVRD3;
        tsm_timing47_saved = XCVR_TSM->TIMING47;
        tsm_timing48_saved = XCVR_TSM->TIMING48;
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)        
        tsm_timing49_saved = XCVR_TSM->TIMING49;
        tsm_timing50_saved = XCVR_TSM->TIMING50;
#endif        
    }
#endif /* gMWS_UseCoexistence_d */

    return gXcvrSuccess_c;
}

