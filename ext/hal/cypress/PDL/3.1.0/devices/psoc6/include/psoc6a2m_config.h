/***************************************************************************//**
* \file psoc6a2m_config.h
*
* \brief
* PSoC6A-2M device configuration header
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _PSOC6A2M_CONFIG_H_
#define _PSOC6A2M_CONFIG_H_

/* Clock Connections */
typedef enum
{
    PCLK_SCB0_CLOCK                 = 0x0000u,  /* scb[0].clock */
    PCLK_SCB1_CLOCK                 = 0x0001u,  /* scb[1].clock */
    PCLK_SCB2_CLOCK                 = 0x0002u,  /* scb[2].clock */
    PCLK_SCB3_CLOCK                 = 0x0003u,  /* scb[3].clock */
    PCLK_SCB4_CLOCK                 = 0x0004u,  /* scb[4].clock */
    PCLK_SCB5_CLOCK                 = 0x0005u,  /* scb[5].clock */
    PCLK_SCB6_CLOCK                 = 0x0006u,  /* scb[6].clock */
    PCLK_SCB7_CLOCK                 = 0x0007u,  /* scb[7].clock */
    PCLK_SCB8_CLOCK                 = 0x0008u,  /* scb[8].clock */
    PCLK_SCB9_CLOCK                 = 0x0009u,  /* scb[9].clock */
    PCLK_SCB10_CLOCK                = 0x000Au,  /* scb[10].clock */
    PCLK_SCB11_CLOCK                = 0x000Bu,  /* scb[11].clock */
    PCLK_SCB12_CLOCK                = 0x000Cu,  /* scb[12].clock */
    PCLK_SMARTIO8_CLOCK             = 0x000Du,  /* smartio[8].clock */
    PCLK_SMARTIO9_CLOCK             = 0x000Eu,  /* smartio[9].clock */
    PCLK_TCPWM0_CLOCKS0             = 0x000Fu,  /* tcpwm[0].clocks[0] */
    PCLK_TCPWM0_CLOCKS1             = 0x0010u,  /* tcpwm[0].clocks[1] */
    PCLK_TCPWM0_CLOCKS2             = 0x0011u,  /* tcpwm[0].clocks[2] */
    PCLK_TCPWM0_CLOCKS3             = 0x0012u,  /* tcpwm[0].clocks[3] */
    PCLK_TCPWM0_CLOCKS4             = 0x0013u,  /* tcpwm[0].clocks[4] */
    PCLK_TCPWM0_CLOCKS5             = 0x0014u,  /* tcpwm[0].clocks[5] */
    PCLK_TCPWM0_CLOCKS6             = 0x0015u,  /* tcpwm[0].clocks[6] */
    PCLK_TCPWM0_CLOCKS7             = 0x0016u,  /* tcpwm[0].clocks[7] */
    PCLK_TCPWM1_CLOCKS0             = 0x0017u,  /* tcpwm[1].clocks[0] */
    PCLK_TCPWM1_CLOCKS1             = 0x0018u,  /* tcpwm[1].clocks[1] */
    PCLK_TCPWM1_CLOCKS2             = 0x0019u,  /* tcpwm[1].clocks[2] */
    PCLK_TCPWM1_CLOCKS3             = 0x001Au,  /* tcpwm[1].clocks[3] */
    PCLK_TCPWM1_CLOCKS4             = 0x001Bu,  /* tcpwm[1].clocks[4] */
    PCLK_TCPWM1_CLOCKS5             = 0x001Cu,  /* tcpwm[1].clocks[5] */
    PCLK_TCPWM1_CLOCKS6             = 0x001Du,  /* tcpwm[1].clocks[6] */
    PCLK_TCPWM1_CLOCKS7             = 0x001Eu,  /* tcpwm[1].clocks[7] */
    PCLK_TCPWM1_CLOCKS8             = 0x001Fu,  /* tcpwm[1].clocks[8] */
    PCLK_TCPWM1_CLOCKS9             = 0x0020u,  /* tcpwm[1].clocks[9] */
    PCLK_TCPWM1_CLOCKS10            = 0x0021u,  /* tcpwm[1].clocks[10] */
    PCLK_TCPWM1_CLOCKS11            = 0x0022u,  /* tcpwm[1].clocks[11] */
    PCLK_TCPWM1_CLOCKS12            = 0x0023u,  /* tcpwm[1].clocks[12] */
    PCLK_TCPWM1_CLOCKS13            = 0x0024u,  /* tcpwm[1].clocks[13] */
    PCLK_TCPWM1_CLOCKS14            = 0x0025u,  /* tcpwm[1].clocks[14] */
    PCLK_TCPWM1_CLOCKS15            = 0x0026u,  /* tcpwm[1].clocks[15] */
    PCLK_TCPWM1_CLOCKS16            = 0x0027u,  /* tcpwm[1].clocks[16] */
    PCLK_TCPWM1_CLOCKS17            = 0x0028u,  /* tcpwm[1].clocks[17] */
    PCLK_TCPWM1_CLOCKS18            = 0x0029u,  /* tcpwm[1].clocks[18] */
    PCLK_TCPWM1_CLOCKS19            = 0x002Au,  /* tcpwm[1].clocks[19] */
    PCLK_TCPWM1_CLOCKS20            = 0x002Bu,  /* tcpwm[1].clocks[20] */
    PCLK_TCPWM1_CLOCKS21            = 0x002Cu,  /* tcpwm[1].clocks[21] */
    PCLK_TCPWM1_CLOCKS22            = 0x002Du,  /* tcpwm[1].clocks[22] */
    PCLK_TCPWM1_CLOCKS23            = 0x002Eu,  /* tcpwm[1].clocks[23] */
    PCLK_CSD_CLOCK                  = 0x002Fu,  /* csd.clock */
    PCLK_LCD_CLOCK                  = 0x0030u,  /* lcd.clock */
    PCLK_PROFILE_CLOCK_PROFILE      = 0x0031u,  /* profile.clock_profile */
    PCLK_CPUSS_CLOCK_TRACE_IN       = 0x0032u,  /* cpuss.clock_trace_in */
    PCLK_PASS_CLOCK_PUMP_PERI       = 0x0033u,  /* pass.clock_pump_peri */
    PCLK_PASS_CLOCK_SAR             = 0x0034u,  /* pass.clock_sar */
    PCLK_USB_CLOCK_DEV_BRS          = 0x0035u   /* usb.clock_dev_brs */
} en_clk_dst_t;

/* Trigger Group */
/* This section contains the enums related to the Trigger multiplexer (TrigMux) driver. 
* The constants are divided into four types because each signal of the TrigMux driver has a path
* through two multiplexers: the reduction multiplexer and the distribution multiplexer. This 
* requires two calls for Cy_TrigMux_Connect() function. The first call - for the reduction 
* multiplexer, the second call - for the distribution multiplexer.
*
* The four types of inputs/output parameters:
* 1) Parameters for reduction multiplexer's inputs (input signals of TrigMux)
* 2) Parameters for reduction multiplexer's outputs (intermediate signals);
* 3) Parameters for distribution multiplexer's inputs (intermediate signals);
* 4) Parameters for distribution multiplexer's outputs (output signals of TrigMux).
*
* The Cy_TrigMux_Connect() inTrig parameter can have 1) and 3) types parameters. The outTrig 
* parameter can have 2) and 4) types parameters.
* The names of the constants for these parameters have the following format:
*
* 1) For reduction multiplexer's inputs:
* TRIG<REDMULT>_IN_<IPSOURCE><IPNUM>
* <REDMULT> the reduction multiplexer number;
* <IPSOURCE> - the name of the IP block which is the source of the signal;
* <IPNUM> - the source signal number in the IP block.
*
* Example:
* TRIG11_IN_TCPWM0_TR_OVERFLOW3 - the TCPWM0 tr_overflow[3] input of reduction multiplexer#11.
* 
* 2) For reduction multiplexer's outputs:
* TRIG<REDMULT>_OUT_TR_GROUP<DISTMULT >_INPUT<DISTMULTINPUT>
* <REDMULT> - the reduction multiplexer number;
* <DISTMULT> - the distribution multiplexer number;
* <DISTMULTINPUT> - the input number of the distribution multiplexer.
*
* Example:
* TRIG11_OUT_TR_GROUP0_INPUT23 - Input#23 of the distribution multiplexer#0 is the destination 
* of the reduction multiplexer#11.
*
* 3) For distribution multiplexer's inputs:
* TRIG<DISTMULT>_IN_TR_GROUP<REDMULT >_OUTPUT<REDMULTOUTPUT>
* <REDMULT> - the reduction multiplexer number;
* <DISTMULT> - the distribution multiplexer number;
* <REDMULTOUTPUT> - the output number of the reduction multiplexer;
*
* Example:
* TRIG0_IN_TR_GROUP11_OUTPUT15 - Output#15 of the reduction multiplexer#11 is the source of the 
* distribution multiplexer#0.
* 
* 4) For distribution multiplexer's outputs:
* TRIG<DISTMULT>_OUT_<IPDEST><IPNUM>
* <REDMULT> - the distribution multiplexer number;
* <IPDEST> - the name of the IP block which is the destination of the signal;
* <IPNUM> - the input signal number in the IP block.
*
* Example:
* TRIG0_OUT_CPUSS_DW0_TR_IN3 - the DW0 tr_out[3] ouput of the distribution multiplexer 0.*/
/* Trigger Group Inputs */
/* Trigger Input Group 0 - P-DMA0 Request Assignments */
typedef enum
{
    TRIG_IN_MUX_0_PDMA0_TR_OUT0     = 0x00000001u, /* cpuss.dw0_tr_out[0] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT1     = 0x00000002u, /* cpuss.dw0_tr_out[1] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT2     = 0x00000003u, /* cpuss.dw0_tr_out[2] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT3     = 0x00000004u, /* cpuss.dw0_tr_out[3] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT4     = 0x00000005u, /* cpuss.dw0_tr_out[4] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT5     = 0x00000006u, /* cpuss.dw0_tr_out[5] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT6     = 0x00000007u, /* cpuss.dw0_tr_out[6] */
    TRIG_IN_MUX_0_PDMA0_TR_OUT7     = 0x00000008u, /* cpuss.dw0_tr_out[7] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT0     = 0x00000009u, /* cpuss.dw1_tr_out[0] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT1     = 0x0000000Au, /* cpuss.dw1_tr_out[1] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT2     = 0x0000000Bu, /* cpuss.dw1_tr_out[2] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT3     = 0x0000000Cu, /* cpuss.dw1_tr_out[3] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT4     = 0x0000000Du, /* cpuss.dw1_tr_out[4] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT5     = 0x0000000Eu, /* cpuss.dw1_tr_out[5] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT6     = 0x0000000Fu, /* cpuss.dw1_tr_out[6] */
    TRIG_IN_MUX_0_PDMA1_TR_OUT7     = 0x00000010u, /* cpuss.dw1_tr_out[7] */
    TRIG_IN_MUX_0_TCPWM0_TR_OVERFLOW0 = 0x00000011u, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_0_TCPWM0_TR_COMPARE_MATCH0 = 0x00000012u, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_0_TCPWM0_TR_UNDERFLOW0 = 0x00000013u, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_0_TCPWM0_TR_OVERFLOW1 = 0x00000014u, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_0_TCPWM0_TR_COMPARE_MATCH1 = 0x00000015u, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_0_TCPWM0_TR_UNDERFLOW1 = 0x00000016u, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_0_TCPWM0_TR_OVERFLOW2 = 0x00000017u, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_0_TCPWM0_TR_COMPARE_MATCH2 = 0x00000018u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_0_TCPWM0_TR_UNDERFLOW2 = 0x00000019u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_0_TCPWM0_TR_OVERFLOW3 = 0x0000001Au, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_0_TCPWM0_TR_COMPARE_MATCH3 = 0x0000001Bu, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_0_TCPWM0_TR_UNDERFLOW3 = 0x0000001Cu, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW0 = 0x0000001Du, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH0 = 0x0000001Eu, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW0 = 0x0000001Fu, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW1 = 0x00000020u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH1 = 0x00000021u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW1 = 0x00000022u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW2 = 0x00000023u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH2 = 0x00000024u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW2 = 0x00000025u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW3 = 0x00000026u, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH3 = 0x00000027u, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW3 = 0x00000028u, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW4 = 0x00000029u, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH4 = 0x0000002Au, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW4 = 0x0000002Bu, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW5 = 0x0000002Cu, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH5 = 0x0000002Du, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW5 = 0x0000002Eu, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW6 = 0x0000002Fu, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH6 = 0x00000030u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW6 = 0x00000031u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW7 = 0x00000032u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH7 = 0x00000033u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW7 = 0x00000034u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW8 = 0x00000035u, /* tcpwm[1].tr_overflow[8] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH8 = 0x00000036u, /* tcpwm[1].tr_compare_match[8] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW8 = 0x00000037u, /* tcpwm[1].tr_underflow[8] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW9 = 0x00000038u, /* tcpwm[1].tr_overflow[9] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH9 = 0x00000039u, /* tcpwm[1].tr_compare_match[9] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW9 = 0x0000003Au, /* tcpwm[1].tr_underflow[9] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW10 = 0x0000003Bu, /* tcpwm[1].tr_overflow[10] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH10 = 0x0000003Cu, /* tcpwm[1].tr_compare_match[10] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW10 = 0x0000003Du, /* tcpwm[1].tr_underflow[10] */
    TRIG_IN_MUX_0_TCPWM1_TR_OVERFLOW11 = 0x0000003Eu, /* tcpwm[1].tr_overflow[11] */
    TRIG_IN_MUX_0_TCPWM1_TR_COMPARE_MATCH11 = 0x0000003Fu, /* tcpwm[1].tr_compare_match[11] */
    TRIG_IN_MUX_0_TCPWM1_TR_UNDERFLOW11 = 0x00000040u, /* tcpwm[1].tr_underflow[11] */
    TRIG_IN_MUX_0_MDMA_TR_OUT0      = 0x00000041u, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_0_MDMA_TR_OUT1      = 0x00000042u, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_0_MDMA_TR_OUT2      = 0x00000043u, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_0_MDMA_TR_OUT3      = 0x00000044u, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT0     = 0x00000045u, /* peri.tr_io_input[0] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT1     = 0x00000046u, /* peri.tr_io_input[1] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT2     = 0x00000047u, /* peri.tr_io_input[2] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT3     = 0x00000048u, /* peri.tr_io_input[3] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT4     = 0x00000049u, /* peri.tr_io_input[4] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT5     = 0x0000004Au, /* peri.tr_io_input[5] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT6     = 0x0000004Bu, /* peri.tr_io_input[6] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT7     = 0x0000004Cu, /* peri.tr_io_input[7] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT8     = 0x0000004Du, /* peri.tr_io_input[8] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT9     = 0x0000004Eu, /* peri.tr_io_input[9] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT10    = 0x0000004Fu, /* peri.tr_io_input[10] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT11    = 0x00000050u, /* peri.tr_io_input[11] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT12    = 0x00000051u, /* peri.tr_io_input[12] */
    TRIG_IN_MUX_0_HSIOM_TR_OUT13    = 0x00000052u, /* peri.tr_io_input[13] */
    TRIG_IN_MUX_0_CTI_TR_OUT0       = 0x00000053u, /* cpuss.cti_tr_out[0] */
    TRIG_IN_MUX_0_CTI_TR_OUT1       = 0x00000054u, /* cpuss.cti_tr_out[1] */
    TRIG_IN_MUX_0_FAULT_TR_OUT0     = 0x00000055u, /* cpuss.tr_fault[0] */
    TRIG_IN_MUX_0_FAULT_TR_OUT1     = 0x00000056u /* cpuss.tr_fault[1] */
} en_trig_input_pdma0_tr_t;

/* Trigger Input Group 1 - P-DMA1 Request Assignments */
typedef enum
{
    TRIG_IN_MUX_1_PDMA0_TR_OUT0     = 0x00000101u, /* cpuss.dw0_tr_out[0] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT1     = 0x00000102u, /* cpuss.dw0_tr_out[1] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT2     = 0x00000103u, /* cpuss.dw0_tr_out[2] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT3     = 0x00000104u, /* cpuss.dw0_tr_out[3] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT4     = 0x00000105u, /* cpuss.dw0_tr_out[4] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT5     = 0x00000106u, /* cpuss.dw0_tr_out[5] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT6     = 0x00000107u, /* cpuss.dw0_tr_out[6] */
    TRIG_IN_MUX_1_PDMA0_TR_OUT7     = 0x00000108u, /* cpuss.dw0_tr_out[7] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT0     = 0x00000109u, /* cpuss.dw1_tr_out[0] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT1     = 0x0000010Au, /* cpuss.dw1_tr_out[1] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT2     = 0x0000010Bu, /* cpuss.dw1_tr_out[2] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT3     = 0x0000010Cu, /* cpuss.dw1_tr_out[3] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT4     = 0x0000010Du, /* cpuss.dw1_tr_out[4] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT5     = 0x0000010Eu, /* cpuss.dw1_tr_out[5] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT6     = 0x0000010Fu, /* cpuss.dw1_tr_out[6] */
    TRIG_IN_MUX_1_PDMA1_TR_OUT7     = 0x00000110u, /* cpuss.dw1_tr_out[7] */
    TRIG_IN_MUX_1_TCPWM0_TR_OVERFLOW4 = 0x00000111u, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_1_TCPWM0_TR_COMPARE_MATCH4 = 0x00000112u, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_1_TCPWM0_TR_UNDERFLOW4 = 0x00000113u, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_1_TCPWM0_TR_OVERFLOW5 = 0x00000114u, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_1_TCPWM0_TR_COMPARE_MATCH5 = 0x00000115u, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_1_TCPWM0_TR_UNDERFLOW5 = 0x00000116u, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_1_TCPWM0_TR_OVERFLOW6 = 0x00000117u, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_1_TCPWM0_TR_COMPARE_MATCH6 = 0x00000118u, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_1_TCPWM0_TR_UNDERFLOW6 = 0x00000119u, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_1_TCPWM0_TR_OVERFLOW7 = 0x0000011Au, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_1_TCPWM0_TR_COMPARE_MATCH7 = 0x0000011Bu, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_1_TCPWM0_TR_UNDERFLOW7 = 0x0000011Cu, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW12 = 0x0000011Du, /* tcpwm[1].tr_overflow[12] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH12 = 0x0000011Eu, /* tcpwm[1].tr_compare_match[12] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW12 = 0x0000011Fu, /* tcpwm[1].tr_underflow[12] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW13 = 0x00000120u, /* tcpwm[1].tr_overflow[13] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH13 = 0x00000121u, /* tcpwm[1].tr_compare_match[13] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW13 = 0x00000122u, /* tcpwm[1].tr_underflow[13] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW14 = 0x00000123u, /* tcpwm[1].tr_overflow[14] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH14 = 0x00000124u, /* tcpwm[1].tr_compare_match[14] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW14 = 0x00000125u, /* tcpwm[1].tr_underflow[14] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW15 = 0x00000126u, /* tcpwm[1].tr_overflow[15] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH15 = 0x00000127u, /* tcpwm[1].tr_compare_match[15] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW15 = 0x00000128u, /* tcpwm[1].tr_underflow[15] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW16 = 0x00000129u, /* tcpwm[1].tr_overflow[16] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH16 = 0x0000012Au, /* tcpwm[1].tr_compare_match[16] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW16 = 0x0000012Bu, /* tcpwm[1].tr_underflow[16] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW17 = 0x0000012Cu, /* tcpwm[1].tr_overflow[17] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH17 = 0x0000012Du, /* tcpwm[1].tr_compare_match[17] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW17 = 0x0000012Eu, /* tcpwm[1].tr_underflow[17] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW18 = 0x0000012Fu, /* tcpwm[1].tr_overflow[18] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH18 = 0x00000130u, /* tcpwm[1].tr_compare_match[18] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW18 = 0x00000131u, /* tcpwm[1].tr_underflow[18] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW19 = 0x00000132u, /* tcpwm[1].tr_overflow[19] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH19 = 0x00000133u, /* tcpwm[1].tr_compare_match[19] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW19 = 0x00000134u, /* tcpwm[1].tr_underflow[19] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW20 = 0x00000135u, /* tcpwm[1].tr_overflow[20] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH20 = 0x00000136u, /* tcpwm[1].tr_compare_match[20] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW20 = 0x00000137u, /* tcpwm[1].tr_underflow[20] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW21 = 0x00000138u, /* tcpwm[1].tr_overflow[21] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH21 = 0x00000139u, /* tcpwm[1].tr_compare_match[21] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW21 = 0x0000013Au, /* tcpwm[1].tr_underflow[21] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW22 = 0x0000013Bu, /* tcpwm[1].tr_overflow[22] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH22 = 0x0000013Cu, /* tcpwm[1].tr_compare_match[22] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW22 = 0x0000013Du, /* tcpwm[1].tr_underflow[22] */
    TRIG_IN_MUX_1_TCPWM1_TR_OVERFLOW23 = 0x0000013Eu, /* tcpwm[1].tr_overflow[23] */
    TRIG_IN_MUX_1_TCPWM1_TR_COMPARE_MATCH23 = 0x0000013Fu, /* tcpwm[1].tr_compare_match[23] */
    TRIG_IN_MUX_1_TCPWM1_TR_UNDERFLOW23 = 0x00000140u, /* tcpwm[1].tr_underflow[23] */
    TRIG_IN_MUX_1_MDMA_TR_OUT0      = 0x00000141u, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_1_MDMA_TR_OUT1      = 0x00000142u, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_1_MDMA_TR_OUT2      = 0x00000143u, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_1_MDMA_TR_OUT3      = 0x00000144u, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_1_CSD_DONE          = 0x00000145u, /* csd.tr_adc_done */
    TRIG_IN_MUX_1_HSIOM_TR_OUT14    = 0x00000146u, /* peri.tr_io_input[14] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT15    = 0x00000147u, /* peri.tr_io_input[15] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT16    = 0x00000148u, /* peri.tr_io_input[16] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT17    = 0x00000149u, /* peri.tr_io_input[17] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT18    = 0x0000014Au, /* peri.tr_io_input[18] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT19    = 0x0000014Bu, /* peri.tr_io_input[19] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT20    = 0x0000014Cu, /* peri.tr_io_input[20] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT21    = 0x0000014Du, /* peri.tr_io_input[21] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT22    = 0x0000014Eu, /* peri.tr_io_input[22] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT23    = 0x0000014Fu, /* peri.tr_io_input[23] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT24    = 0x00000150u, /* peri.tr_io_input[24] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT25    = 0x00000151u, /* peri.tr_io_input[25] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT26    = 0x00000152u, /* peri.tr_io_input[26] */
    TRIG_IN_MUX_1_HSIOM_TR_OUT27    = 0x00000153u, /* peri.tr_io_input[27] */
    TRIG_IN_MUX_1_LPCOMP_DSI_COMP0  = 0x00000154u, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_1_LPCOMP_DSI_COMP1  = 0x00000155u /* lpcomp.dsi_comp1 */
} en_trig_input_pdma1_tr_t;

/* Trigger Input Group 2 - TCPWM0 trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_2_PDMA0_TR_OUT0     = 0x00000201u, /* cpuss.dw0_tr_out[0] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT1     = 0x00000202u, /* cpuss.dw0_tr_out[1] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT2     = 0x00000203u, /* cpuss.dw0_tr_out[2] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT3     = 0x00000204u, /* cpuss.dw0_tr_out[3] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT4     = 0x00000205u, /* cpuss.dw0_tr_out[4] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT5     = 0x00000206u, /* cpuss.dw0_tr_out[5] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT6     = 0x00000207u, /* cpuss.dw0_tr_out[6] */
    TRIG_IN_MUX_2_PDMA0_TR_OUT7     = 0x00000208u, /* cpuss.dw0_tr_out[7] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW0 = 0x00000209u, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH0 = 0x0000020Au, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW0 = 0x0000020Bu, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW1 = 0x0000020Cu, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH1 = 0x0000020Du, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW1 = 0x0000020Eu, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW2 = 0x0000020Fu, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH2 = 0x00000210u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW2 = 0x00000211u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW3 = 0x00000212u, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH3 = 0x00000213u, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW3 = 0x00000214u, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW4 = 0x00000215u, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH4 = 0x00000216u, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW4 = 0x00000217u, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW5 = 0x00000218u, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH5 = 0x00000219u, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW5 = 0x0000021Au, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW6 = 0x0000021Bu, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH6 = 0x0000021Cu, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW6 = 0x0000021Du, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_2_TCPWM0_TR_OVERFLOW7 = 0x0000021Eu, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_2_TCPWM0_TR_COMPARE_MATCH7 = 0x0000021Fu, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_2_TCPWM0_TR_UNDERFLOW7 = 0x00000220u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW0 = 0x00000221u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH0 = 0x00000222u, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW0 = 0x00000223u, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW1 = 0x00000224u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH1 = 0x00000225u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW1 = 0x00000226u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW2 = 0x00000227u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH2 = 0x00000228u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW2 = 0x00000229u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW3 = 0x0000022Au, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH3 = 0x0000022Bu, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW3 = 0x0000022Cu, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW4 = 0x0000022Du, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH4 = 0x0000022Eu, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW4 = 0x0000022Fu, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW5 = 0x00000230u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH5 = 0x00000231u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW5 = 0x00000232u, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW6 = 0x00000233u, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH6 = 0x00000234u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW6 = 0x00000235u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_2_TCPWM1_TR_OVERFLOW7 = 0x00000236u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_2_TCPWM1_TR_COMPARE_MATCH7 = 0x00000237u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_2_TCPWM1_TR_UNDERFLOW7 = 0x00000238u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_2_MDMA_TR_OUT0      = 0x00000239u, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_2_MDMA_TR_OUT1      = 0x0000023Au, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_2_MDMA_TR_OUT2      = 0x0000023Bu, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_2_MDMA_TR_OUT3      = 0x0000023Cu, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_2_SCB_I2C_SCL0      = 0x0000023Du, /* scb[0].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX0           = 0x0000023Eu, /* scb[0].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX0           = 0x0000023Fu, /* scb[0].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL1      = 0x00000240u, /* scb[1].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX1           = 0x00000241u, /* scb[1].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX1           = 0x00000242u, /* scb[1].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL2      = 0x00000243u, /* scb[2].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX2           = 0x00000244u, /* scb[2].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX2           = 0x00000245u, /* scb[2].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL3      = 0x00000246u, /* scb[3].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX3           = 0x00000247u, /* scb[3].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX3           = 0x00000248u, /* scb[3].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL4      = 0x00000249u, /* scb[4].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX4           = 0x0000024Au, /* scb[4].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX4           = 0x0000024Bu, /* scb[4].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL5      = 0x0000024Cu, /* scb[5].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX5           = 0x0000024Du, /* scb[5].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX5           = 0x0000024Eu, /* scb[5].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL6      = 0x0000024Fu, /* scb[6].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX6           = 0x00000250u, /* scb[6].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX6           = 0x00000251u, /* scb[6].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL7      = 0x00000252u, /* scb[7].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX7           = 0x00000253u, /* scb[7].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX7           = 0x00000254u, /* scb[7].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL8      = 0x00000255u, /* scb[8].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX8           = 0x00000256u, /* scb[8].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX8           = 0x00000257u, /* scb[8].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL9      = 0x00000258u, /* scb[9].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX9           = 0x00000259u, /* scb[9].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX9           = 0x0000025Au, /* scb[9].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL10     = 0x0000025Bu, /* scb[10].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX10          = 0x0000025Cu, /* scb[10].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX10          = 0x0000025Du, /* scb[10].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL11     = 0x0000025Eu, /* scb[11].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX11          = 0x0000025Fu, /* scb[11].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX11          = 0x00000260u, /* scb[11].tr_rx_req */
    TRIG_IN_MUX_2_SCB_I2C_SCL12     = 0x00000261u, /* scb[12].tr_i2c_scl_filtered */
    TRIG_IN_MUX_2_SCB_TX12          = 0x00000262u, /* scb[12].tr_tx_req */
    TRIG_IN_MUX_2_SCB_RX12          = 0x00000263u, /* scb[12].tr_rx_req */
    TRIG_IN_MUX_2_SMIF_TX           = 0x00000264u, /* smif.tr_tx_req */
    TRIG_IN_MUX_2_SMIF_RX           = 0x00000265u, /* smif.tr_rx_req */
    TRIG_IN_MUX_2_USB_DMA0          = 0x00000266u, /* usb.dma_req[0] */
    TRIG_IN_MUX_2_USB_DMA1          = 0x00000267u, /* usb.dma_req[1] */
    TRIG_IN_MUX_2_USB_DMA2          = 0x00000268u, /* usb.dma_req[2] */
    TRIG_IN_MUX_2_USB_DMA3          = 0x00000269u, /* usb.dma_req[3] */
    TRIG_IN_MUX_2_USB_DMA4          = 0x0000026Au, /* usb.dma_req[4] */
    TRIG_IN_MUX_2_USB_DMA5          = 0x0000026Bu, /* usb.dma_req[5] */
    TRIG_IN_MUX_2_USB_DMA6          = 0x0000026Cu, /* usb.dma_req[6] */
    TRIG_IN_MUX_2_USB_DMA7          = 0x0000026Du, /* usb.dma_req[7] */
    TRIG_IN_MUX_2_I2S_TX0           = 0x0000026Eu, /* audioss[0].tr_i2s_tx_req */
    TRIG_IN_MUX_2_I2S_RX0           = 0x0000026Fu, /* audioss[0].tr_i2s_rx_req */
    TRIG_IN_MUX_2_PDM_RX0           = 0x00000270u, /* audioss[0].tr_pdm_rx_req */
    TRIG_IN_MUX_2_I2S_TX1           = 0x00000271u, /* audioss[1].tr_i2s_tx_req */
    TRIG_IN_MUX_2_I2S_RX1           = 0x00000272u, /* audioss[1].tr_i2s_rx_req */
    TRIG_IN_MUX_2_PASS_SAR_DONE     = 0x00000273u, /* pass.tr_sar_out */
    TRIG_IN_MUX_2_CSD_SENSE         = 0x00000274u, /* csd.dsi_sense_out */
    TRIG_IN_MUX_2_HSIOM_TR_OUT0     = 0x00000275u, /* peri.tr_io_input[0] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT1     = 0x00000276u, /* peri.tr_io_input[1] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT2     = 0x00000277u, /* peri.tr_io_input[2] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT3     = 0x00000278u, /* peri.tr_io_input[3] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT4     = 0x00000279u, /* peri.tr_io_input[4] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT5     = 0x0000027Au, /* peri.tr_io_input[5] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT6     = 0x0000027Bu, /* peri.tr_io_input[6] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT7     = 0x0000027Cu, /* peri.tr_io_input[7] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT8     = 0x0000027Du, /* peri.tr_io_input[8] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT9     = 0x0000027Eu, /* peri.tr_io_input[9] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT10    = 0x0000027Fu, /* peri.tr_io_input[10] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT11    = 0x00000280u, /* peri.tr_io_input[11] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT12    = 0x00000281u, /* peri.tr_io_input[12] */
    TRIG_IN_MUX_2_HSIOM_TR_OUT13    = 0x00000282u, /* peri.tr_io_input[13] */
    TRIG_IN_MUX_2_CTI_TR_OUT0       = 0x00000283u, /* cpuss.cti_tr_out[0] */
    TRIG_IN_MUX_2_CTI_TR_OUT1       = 0x00000284u, /* cpuss.cti_tr_out[1] */
    TRIG_IN_MUX_2_LPCOMP_DSI_COMP0  = 0x00000285u, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_2_LPCOMP_DSI_COMP1  = 0x00000286u /* lpcomp.dsi_comp1 */
} en_trig_input_tcpwm0_t;

/* Trigger Input Group 3 - TCPWM1 trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_3_PDMA1_TR_OUT0     = 0x00000301u, /* cpuss.dw1_tr_out[0] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT1     = 0x00000302u, /* cpuss.dw1_tr_out[1] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT2     = 0x00000303u, /* cpuss.dw1_tr_out[2] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT3     = 0x00000304u, /* cpuss.dw1_tr_out[3] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT4     = 0x00000305u, /* cpuss.dw1_tr_out[4] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT5     = 0x00000306u, /* cpuss.dw1_tr_out[5] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT6     = 0x00000307u, /* cpuss.dw1_tr_out[6] */
    TRIG_IN_MUX_3_PDMA1_TR_OUT7     = 0x00000308u, /* cpuss.dw1_tr_out[7] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW0 = 0x00000309u, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH0 = 0x0000030Au, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW0 = 0x0000030Bu, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW1 = 0x0000030Cu, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH1 = 0x0000030Du, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW1 = 0x0000030Eu, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW2 = 0x0000030Fu, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH2 = 0x00000310u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW2 = 0x00000311u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW3 = 0x00000312u, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH3 = 0x00000313u, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW3 = 0x00000314u, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW4 = 0x00000315u, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH4 = 0x00000316u, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW4 = 0x00000317u, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW5 = 0x00000318u, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH5 = 0x00000319u, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW5 = 0x0000031Au, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW6 = 0x0000031Bu, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH6 = 0x0000031Cu, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW6 = 0x0000031Du, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_3_TCPWM0_TR_OVERFLOW7 = 0x0000031Eu, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_3_TCPWM0_TR_COMPARE_MATCH7 = 0x0000031Fu, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_3_TCPWM0_TR_UNDERFLOW7 = 0x00000320u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW0 = 0x00000321u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH0 = 0x00000322u, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW0 = 0x00000323u, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW1 = 0x00000324u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH1 = 0x00000325u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW1 = 0x00000326u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW2 = 0x00000327u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH2 = 0x00000328u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW2 = 0x00000329u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW3 = 0x0000032Au, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH3 = 0x0000032Bu, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW3 = 0x0000032Cu, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW4 = 0x0000032Du, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH4 = 0x0000032Eu, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW4 = 0x0000032Fu, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW5 = 0x00000330u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH5 = 0x00000331u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW5 = 0x00000332u, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW6 = 0x00000333u, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH6 = 0x00000334u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW6 = 0x00000335u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_3_TCPWM1_TR_OVERFLOW7 = 0x00000336u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_3_TCPWM1_TR_COMPARE_MATCH7 = 0x00000337u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_3_TCPWM1_TR_UNDERFLOW7 = 0x00000338u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_3_MDMA_TR_OUT0      = 0x00000339u, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_3_MDMA_TR_OUT1      = 0x0000033Au, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_3_MDMA_TR_OUT2      = 0x0000033Bu, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_3_MDMA_TR_OUT3      = 0x0000033Cu, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_3_SCB_I2C_SCL0      = 0x0000033Du, /* scb[0].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX0           = 0x0000033Eu, /* scb[0].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX0           = 0x0000033Fu, /* scb[0].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL1      = 0x00000340u, /* scb[1].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX1           = 0x00000341u, /* scb[1].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX1           = 0x00000342u, /* scb[1].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL2      = 0x00000343u, /* scb[2].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX2           = 0x00000344u, /* scb[2].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX2           = 0x00000345u, /* scb[2].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL3      = 0x00000346u, /* scb[3].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX3           = 0x00000347u, /* scb[3].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX3           = 0x00000348u, /* scb[3].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL4      = 0x00000349u, /* scb[4].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX4           = 0x0000034Au, /* scb[4].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX4           = 0x0000034Bu, /* scb[4].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL5      = 0x0000034Cu, /* scb[5].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX5           = 0x0000034Du, /* scb[5].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX5           = 0x0000034Eu, /* scb[5].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL6      = 0x0000034Fu, /* scb[6].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX6           = 0x00000350u, /* scb[6].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX6           = 0x00000351u, /* scb[6].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL7      = 0x00000352u, /* scb[7].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX7           = 0x00000353u, /* scb[7].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX7           = 0x00000354u, /* scb[7].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL8      = 0x00000355u, /* scb[8].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX8           = 0x00000356u, /* scb[8].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX8           = 0x00000357u, /* scb[8].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL9      = 0x00000358u, /* scb[9].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX9           = 0x00000359u, /* scb[9].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX9           = 0x0000035Au, /* scb[9].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL10     = 0x0000035Bu, /* scb[10].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX10          = 0x0000035Cu, /* scb[10].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX10          = 0x0000035Du, /* scb[10].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL11     = 0x0000035Eu, /* scb[11].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX11          = 0x0000035Fu, /* scb[11].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX11          = 0x00000360u, /* scb[11].tr_rx_req */
    TRIG_IN_MUX_3_SCB_I2C_SCL12     = 0x00000361u, /* scb[12].tr_i2c_scl_filtered */
    TRIG_IN_MUX_3_SCB_TX12          = 0x00000362u, /* scb[12].tr_tx_req */
    TRIG_IN_MUX_3_SCB_RX12          = 0x00000363u, /* scb[12].tr_rx_req */
    TRIG_IN_MUX_3_SMIF_TX           = 0x00000364u, /* smif.tr_tx_req */
    TRIG_IN_MUX_3_SMIF_RX           = 0x00000365u, /* smif.tr_rx_req */
    TRIG_IN_MUX_3_USB_DMA0          = 0x00000366u, /* usb.dma_req[0] */
    TRIG_IN_MUX_3_USB_DMA1          = 0x00000367u, /* usb.dma_req[1] */
    TRIG_IN_MUX_3_USB_DMA2          = 0x00000368u, /* usb.dma_req[2] */
    TRIG_IN_MUX_3_USB_DMA3          = 0x00000369u, /* usb.dma_req[3] */
    TRIG_IN_MUX_3_USB_DMA4          = 0x0000036Au, /* usb.dma_req[4] */
    TRIG_IN_MUX_3_USB_DMA5          = 0x0000036Bu, /* usb.dma_req[5] */
    TRIG_IN_MUX_3_USB_DMA6          = 0x0000036Cu, /* usb.dma_req[6] */
    TRIG_IN_MUX_3_USB_DMA7          = 0x0000036Du, /* usb.dma_req[7] */
    TRIG_IN_MUX_3_I2S_TX0           = 0x0000036Eu, /* audioss[0].tr_i2s_tx_req */
    TRIG_IN_MUX_3_I2S_RX0           = 0x0000036Fu, /* audioss[0].tr_i2s_rx_req */
    TRIG_IN_MUX_3_PDM_RX0           = 0x00000370u, /* audioss[0].tr_pdm_rx_req */
    TRIG_IN_MUX_3_I2S_TX1           = 0x00000371u, /* audioss[1].tr_i2s_tx_req */
    TRIG_IN_MUX_3_I2S_RX1           = 0x00000372u, /* audioss[1].tr_i2s_rx_req */
    TRIG_IN_MUX_3_PASS_SAR_DONE     = 0x00000373u, /* pass.tr_sar_out */
    TRIG_IN_MUX_3_CSD_SENSE         = 0x00000374u, /* csd.dsi_sense_out */
    TRIG_IN_MUX_3_HSIOM_TR_OUT0     = 0x00000375u, /* peri.tr_io_input[14] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT1     = 0x00000376u, /* peri.tr_io_input[15] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT2     = 0x00000377u, /* peri.tr_io_input[16] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT3     = 0x00000378u, /* peri.tr_io_input[17] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT4     = 0x00000379u, /* peri.tr_io_input[18] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT5     = 0x0000037Au, /* peri.tr_io_input[19] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT6     = 0x0000037Bu, /* peri.tr_io_input[20] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT7     = 0x0000037Cu, /* peri.tr_io_input[21] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT8     = 0x0000037Du, /* peri.tr_io_input[22] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT9     = 0x0000037Eu, /* peri.tr_io_input[23] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT10    = 0x0000037Fu, /* peri.tr_io_input[24] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT11    = 0x00000380u, /* peri.tr_io_input[25] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT12    = 0x00000381u, /* peri.tr_io_input[26] */
    TRIG_IN_MUX_3_HSIOM_TR_OUT13    = 0x00000382u, /* peri.tr_io_input[27] */
    TRIG_IN_MUX_3_FAULT_TR_OUT0     = 0x00000383u, /* cpuss.tr_fault[0] */
    TRIG_IN_MUX_3_FAULT_TR_OUT1     = 0x00000384u, /* cpuss.tr_fault[1] */
    TRIG_IN_MUX_3_LPCOMP_DSI_COMP0  = 0x00000385u, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_3_LPCOMP_DSI_COMP1  = 0x00000386u /* lpcomp.dsi_comp1 */
} en_trig_input_tcpwm1_t;

/* Trigger Input Group 4 - HSIOM trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_4_PDMA0_TR_OUT0     = 0x00000401u, /* cpuss.dw0_tr_out[0] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT1     = 0x00000402u, /* cpuss.dw0_tr_out[1] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT2     = 0x00000403u, /* cpuss.dw0_tr_out[2] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT3     = 0x00000404u, /* cpuss.dw0_tr_out[3] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT4     = 0x00000405u, /* cpuss.dw0_tr_out[4] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT5     = 0x00000406u, /* cpuss.dw0_tr_out[5] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT6     = 0x00000407u, /* cpuss.dw0_tr_out[6] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT7     = 0x00000408u, /* cpuss.dw0_tr_out[7] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT8     = 0x00000409u, /* cpuss.dw0_tr_out[8] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT9     = 0x0000040Au, /* cpuss.dw0_tr_out[9] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT10    = 0x0000040Bu, /* cpuss.dw0_tr_out[10] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT11    = 0x0000040Cu, /* cpuss.dw0_tr_out[11] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT12    = 0x0000040Du, /* cpuss.dw0_tr_out[12] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT13    = 0x0000040Eu, /* cpuss.dw0_tr_out[13] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT14    = 0x0000040Fu, /* cpuss.dw0_tr_out[14] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT15    = 0x00000410u, /* cpuss.dw0_tr_out[15] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT16    = 0x00000411u, /* cpuss.dw0_tr_out[16] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT17    = 0x00000412u, /* cpuss.dw0_tr_out[17] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT18    = 0x00000413u, /* cpuss.dw0_tr_out[18] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT19    = 0x00000414u, /* cpuss.dw0_tr_out[19] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT20    = 0x00000415u, /* cpuss.dw0_tr_out[20] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT21    = 0x00000416u, /* cpuss.dw0_tr_out[21] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT22    = 0x00000417u, /* cpuss.dw0_tr_out[22] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT23    = 0x00000418u, /* cpuss.dw0_tr_out[23] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT24    = 0x00000419u, /* cpuss.dw0_tr_out[24] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT25    = 0x0000041Au, /* cpuss.dw0_tr_out[25] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT26    = 0x0000041Bu, /* cpuss.dw0_tr_out[26] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT27    = 0x0000041Cu, /* cpuss.dw0_tr_out[27] */
    TRIG_IN_MUX_4_PDMA0_TR_OUT28    = 0x0000041Du, /* cpuss.dw0_tr_out[28] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT0     = 0x0000041Eu, /* cpuss.dw1_tr_out[0] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT1     = 0x0000041Fu, /* cpuss.dw1_tr_out[1] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT2     = 0x00000420u, /* cpuss.dw1_tr_out[2] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT3     = 0x00000421u, /* cpuss.dw1_tr_out[3] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT4     = 0x00000422u, /* cpuss.dw1_tr_out[4] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT5     = 0x00000423u, /* cpuss.dw1_tr_out[5] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT6     = 0x00000424u, /* cpuss.dw1_tr_out[6] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT7     = 0x00000425u, /* cpuss.dw1_tr_out[7] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT8     = 0x00000426u, /* cpuss.dw1_tr_out[8] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT9     = 0x00000427u, /* cpuss.dw1_tr_out[9] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT10    = 0x00000428u, /* cpuss.dw1_tr_out[10] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT11    = 0x00000429u, /* cpuss.dw1_tr_out[11] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT12    = 0x0000042Au, /* cpuss.dw1_tr_out[12] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT13    = 0x0000042Bu, /* cpuss.dw1_tr_out[13] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT14    = 0x0000042Cu, /* cpuss.dw1_tr_out[14] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT15    = 0x0000042Du, /* cpuss.dw1_tr_out[15] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT16    = 0x0000042Eu, /* cpuss.dw1_tr_out[16] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT17    = 0x0000042Fu, /* cpuss.dw1_tr_out[17] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT18    = 0x00000430u, /* cpuss.dw1_tr_out[18] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT19    = 0x00000431u, /* cpuss.dw1_tr_out[19] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT20    = 0x00000432u, /* cpuss.dw1_tr_out[20] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT21    = 0x00000433u, /* cpuss.dw1_tr_out[21] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT22    = 0x00000434u, /* cpuss.dw1_tr_out[22] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT23    = 0x00000435u, /* cpuss.dw1_tr_out[23] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT24    = 0x00000436u, /* cpuss.dw1_tr_out[24] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT25    = 0x00000437u, /* cpuss.dw1_tr_out[25] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT26    = 0x00000438u, /* cpuss.dw1_tr_out[26] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT27    = 0x00000439u, /* cpuss.dw1_tr_out[27] */
    TRIG_IN_MUX_4_PDMA1_TR_OUT28    = 0x0000043Au, /* cpuss.dw1_tr_out[28] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW0 = 0x0000043Bu, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH0 = 0x0000043Cu, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW0 = 0x0000043Du, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW1 = 0x0000043Eu, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH1 = 0x0000043Fu, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW1 = 0x00000440u, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW2 = 0x00000441u, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH2 = 0x00000442u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW2 = 0x00000443u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW3 = 0x00000444u, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH3 = 0x00000445u, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW3 = 0x00000446u, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW4 = 0x00000447u, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH4 = 0x00000448u, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW4 = 0x00000449u, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW5 = 0x0000044Au, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH5 = 0x0000044Bu, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW5 = 0x0000044Cu, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW6 = 0x0000044Du, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH6 = 0x0000044Eu, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW6 = 0x0000044Fu, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_4_TCPWM0_TR_OVERFLOW7 = 0x00000450u, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_4_TCPWM0_TR_COMPARE_MATCH7 = 0x00000451u, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_4_TCPWM0_TR_UNDERFLOW7 = 0x00000452u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW0 = 0x00000453u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH0 = 0x00000454u, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW0 = 0x00000455u, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW1 = 0x00000456u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH1 = 0x00000457u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW1 = 0x00000458u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW2 = 0x00000459u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH2 = 0x0000045Au, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW2 = 0x0000045Bu, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW3 = 0x0000045Cu, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH3 = 0x0000045Du, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW3 = 0x0000045Eu, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW4 = 0x0000045Fu, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH4 = 0x00000460u, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW4 = 0x00000461u, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW5 = 0x00000462u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH5 = 0x00000463u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW5 = 0x00000464u, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW6 = 0x00000465u, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH6 = 0x00000466u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW6 = 0x00000467u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW7 = 0x00000468u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH7 = 0x00000469u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW7 = 0x0000046Au, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW8 = 0x0000046Bu, /* tcpwm[1].tr_overflow[8] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH8 = 0x0000046Cu, /* tcpwm[1].tr_compare_match[8] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW8 = 0x0000046Du, /* tcpwm[1].tr_underflow[8] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW9 = 0x0000046Eu, /* tcpwm[1].tr_overflow[9] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH9 = 0x0000046Fu, /* tcpwm[1].tr_compare_match[9] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW9 = 0x00000470u, /* tcpwm[1].tr_underflow[9] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW10 = 0x00000471u, /* tcpwm[1].tr_overflow[10] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH10 = 0x00000472u, /* tcpwm[1].tr_compare_match[10] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW10 = 0x00000473u, /* tcpwm[1].tr_underflow[10] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW11 = 0x00000474u, /* tcpwm[1].tr_overflow[11] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH11 = 0x00000475u, /* tcpwm[1].tr_compare_match[11] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW11 = 0x00000476u, /* tcpwm[1].tr_underflow[11] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW12 = 0x00000477u, /* tcpwm[1].tr_overflow[12] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH12 = 0x00000478u, /* tcpwm[1].tr_compare_match[12] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW12 = 0x00000479u, /* tcpwm[1].tr_underflow[12] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW13 = 0x0000047Au, /* tcpwm[1].tr_overflow[13] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH13 = 0x0000047Bu, /* tcpwm[1].tr_compare_match[13] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW13 = 0x0000047Cu, /* tcpwm[1].tr_underflow[13] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW14 = 0x0000047Du, /* tcpwm[1].tr_overflow[14] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH14 = 0x0000047Eu, /* tcpwm[1].tr_compare_match[14] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW14 = 0x0000047Fu, /* tcpwm[1].tr_underflow[14] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW15 = 0x00000480u, /* tcpwm[1].tr_overflow[15] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH15 = 0x00000481u, /* tcpwm[1].tr_compare_match[15] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW15 = 0x00000482u, /* tcpwm[1].tr_underflow[15] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW16 = 0x00000483u, /* tcpwm[1].tr_overflow[16] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH16 = 0x00000484u, /* tcpwm[1].tr_compare_match[16] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW16 = 0x00000485u, /* tcpwm[1].tr_underflow[16] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW17 = 0x00000486u, /* tcpwm[1].tr_overflow[17] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH17 = 0x00000487u, /* tcpwm[1].tr_compare_match[17] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW17 = 0x00000488u, /* tcpwm[1].tr_underflow[17] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW18 = 0x00000489u, /* tcpwm[1].tr_overflow[18] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH18 = 0x0000048Au, /* tcpwm[1].tr_compare_match[18] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW18 = 0x0000048Bu, /* tcpwm[1].tr_underflow[18] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW19 = 0x0000048Cu, /* tcpwm[1].tr_overflow[19] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH19 = 0x0000048Du, /* tcpwm[1].tr_compare_match[19] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW19 = 0x0000048Eu, /* tcpwm[1].tr_underflow[19] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW20 = 0x0000048Fu, /* tcpwm[1].tr_overflow[20] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH20 = 0x00000490u, /* tcpwm[1].tr_compare_match[20] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW20 = 0x00000491u, /* tcpwm[1].tr_underflow[20] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW21 = 0x00000492u, /* tcpwm[1].tr_overflow[21] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH21 = 0x00000493u, /* tcpwm[1].tr_compare_match[21] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW21 = 0x00000494u, /* tcpwm[1].tr_underflow[21] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW22 = 0x00000495u, /* tcpwm[1].tr_overflow[22] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH22 = 0x00000496u, /* tcpwm[1].tr_compare_match[22] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW22 = 0x00000497u, /* tcpwm[1].tr_underflow[22] */
    TRIG_IN_MUX_4_TCPWM1_TR_OVERFLOW23 = 0x00000498u, /* tcpwm[1].tr_overflow[23] */
    TRIG_IN_MUX_4_TCPWM1_TR_COMPARE_MATCH23 = 0x00000499u, /* tcpwm[1].tr_compare_match[23] */
    TRIG_IN_MUX_4_TCPWM1_TR_UNDERFLOW23 = 0x0000049Au, /* tcpwm[1].tr_underflow[23] */
    TRIG_IN_MUX_4_MDMA_TR_OUT0      = 0x0000049Bu, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_4_MDMA_TR_OUT1      = 0x0000049Cu, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_4_MDMA_TR_OUT2      = 0x0000049Du, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_4_MDMA_TR_OUT3      = 0x0000049Eu, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_4_SCB_I2C_SCL0      = 0x0000049Fu, /* scb[0].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX0           = 0x000004A0u, /* scb[0].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX0           = 0x000004A1u, /* scb[0].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL1      = 0x000004A2u, /* scb[1].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX1           = 0x000004A3u, /* scb[1].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX1           = 0x000004A4u, /* scb[1].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL2      = 0x000004A5u, /* scb[2].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX2           = 0x000004A6u, /* scb[2].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX2           = 0x000004A7u, /* scb[2].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL3      = 0x000004A8u, /* scb[3].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX3           = 0x000004A9u, /* scb[3].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX3           = 0x000004AAu, /* scb[3].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL4      = 0x000004ABu, /* scb[4].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX4           = 0x000004ACu, /* scb[4].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX4           = 0x000004ADu, /* scb[4].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL5      = 0x000004AEu, /* scb[5].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX5           = 0x000004AFu, /* scb[5].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX5           = 0x000004B0u, /* scb[5].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL6      = 0x000004B1u, /* scb[6].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX6           = 0x000004B2u, /* scb[6].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX6           = 0x000004B3u, /* scb[6].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL7      = 0x000004B4u, /* scb[7].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX7           = 0x000004B5u, /* scb[7].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX7           = 0x000004B6u, /* scb[7].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL8      = 0x000004B7u, /* scb[8].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX8           = 0x000004B8u, /* scb[8].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX8           = 0x000004B9u, /* scb[8].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL9      = 0x000004BAu, /* scb[9].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX9           = 0x000004BBu, /* scb[9].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX9           = 0x000004BCu, /* scb[9].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL10     = 0x000004BDu, /* scb[10].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX10          = 0x000004BEu, /* scb[10].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX10          = 0x000004BFu, /* scb[10].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL11     = 0x000004C0u, /* scb[11].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX11          = 0x000004C1u, /* scb[11].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX11          = 0x000004C2u, /* scb[11].tr_rx_req */
    TRIG_IN_MUX_4_SCB_I2C_SCL12     = 0x000004C3u, /* scb[12].tr_i2c_scl_filtered */
    TRIG_IN_MUX_4_SCB_TX12          = 0x000004C4u, /* scb[12].tr_tx_req */
    TRIG_IN_MUX_4_SCB_RX12          = 0x000004C5u, /* scb[12].tr_rx_req */
    TRIG_IN_MUX_4_SMIF_TX           = 0x000004C6u, /* smif.tr_tx_req */
    TRIG_IN_MUX_4_SMIF_RX           = 0x000004C7u, /* smif.tr_rx_req */
    TRIG_IN_MUX_4_USB_DMA0          = 0x000004C8u, /* usb.dma_req[0] */
    TRIG_IN_MUX_4_USB_DMA1          = 0x000004C9u, /* usb.dma_req[1] */
    TRIG_IN_MUX_4_USB_DMA2          = 0x000004CAu, /* usb.dma_req[2] */
    TRIG_IN_MUX_4_USB_DMA3          = 0x000004CBu, /* usb.dma_req[3] */
    TRIG_IN_MUX_4_USB_DMA4          = 0x000004CCu, /* usb.dma_req[4] */
    TRIG_IN_MUX_4_USB_DMA5          = 0x000004CDu, /* usb.dma_req[5] */
    TRIG_IN_MUX_4_USB_DMA6          = 0x000004CEu, /* usb.dma_req[6] */
    TRIG_IN_MUX_4_USB_DMA7          = 0x000004CFu, /* usb.dma_req[7] */
    TRIG_IN_MUX_4_I2S_TX0           = 0x000004D0u, /* audioss[0].tr_i2s_tx_req */
    TRIG_IN_MUX_4_I2S_RX0           = 0x000004D1u, /* audioss[0].tr_i2s_rx_req */
    TRIG_IN_MUX_4_PDM_RX0           = 0x000004D2u, /* audioss[0].tr_pdm_rx_req */
    TRIG_IN_MUX_4_I2S_TX1           = 0x000004D3u, /* audioss[1].tr_i2s_tx_req */
    TRIG_IN_MUX_4_I2S_RX1           = 0x000004D4u, /* audioss[1].tr_i2s_rx_req */
    TRIG_IN_MUX_4_CSD_SENSE         = 0x000004D5u, /* csd.dsi_sense_out */
    TRIG_IN_MUX_4_CSD_SAMPLE        = 0x000004D6u, /* csd.dsi_sample_out */
    TRIG_IN_MUX_4_CSD_ADC_DONE      = 0x000004D7u, /* csd.tr_adc_done */
    TRIG_IN_MUX_4_PASS_SAR_DONE     = 0x000004D8u, /* pass.tr_sar_out */
    TRIG_IN_MUX_4_FAULT_TR_OUT0     = 0x000004D9u, /* cpuss.tr_fault[0] */
    TRIG_IN_MUX_4_FAULT_TR_OUT1     = 0x000004DAu, /* cpuss.tr_fault[1] */
    TRIG_IN_MUX_4_CTI_TR_OUT0       = 0x000004DBu, /* cpuss.cti_tr_out[0] */
    TRIG_IN_MUX_4_CTI_TR_OUT1       = 0x000004DCu, /* cpuss.cti_tr_out[1] */
    TRIG_IN_MUX_4_LPCOMP_DSI_COMP0  = 0x000004DDu, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_4_LPCOMP_DSI_COMP1  = 0x000004DEu /* lpcomp.dsi_comp1 */
} en_trig_input_hsiom_t;

/* Trigger Input Group 5 - CPUSS Debug and Profiler trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_5_PDMA0_TR_OUT0     = 0x00000501u, /* cpuss.dw0_tr_out[0] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT1     = 0x00000502u, /* cpuss.dw0_tr_out[1] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT2     = 0x00000503u, /* cpuss.dw0_tr_out[2] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT3     = 0x00000504u, /* cpuss.dw0_tr_out[3] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT4     = 0x00000505u, /* cpuss.dw0_tr_out[4] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT5     = 0x00000506u, /* cpuss.dw0_tr_out[5] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT6     = 0x00000507u, /* cpuss.dw0_tr_out[6] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT7     = 0x00000508u, /* cpuss.dw0_tr_out[7] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT8     = 0x00000509u, /* cpuss.dw0_tr_out[8] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT9     = 0x0000050Au, /* cpuss.dw0_tr_out[9] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT10    = 0x0000050Bu, /* cpuss.dw0_tr_out[10] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT11    = 0x0000050Cu, /* cpuss.dw0_tr_out[11] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT12    = 0x0000050Du, /* cpuss.dw0_tr_out[12] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT13    = 0x0000050Eu, /* cpuss.dw0_tr_out[13] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT14    = 0x0000050Fu, /* cpuss.dw0_tr_out[14] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT15    = 0x00000510u, /* cpuss.dw0_tr_out[15] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT16    = 0x00000511u, /* cpuss.dw0_tr_out[16] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT17    = 0x00000512u, /* cpuss.dw0_tr_out[17] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT18    = 0x00000513u, /* cpuss.dw0_tr_out[18] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT19    = 0x00000514u, /* cpuss.dw0_tr_out[19] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT20    = 0x00000515u, /* cpuss.dw0_tr_out[20] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT21    = 0x00000516u, /* cpuss.dw0_tr_out[21] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT22    = 0x00000517u, /* cpuss.dw0_tr_out[22] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT23    = 0x00000518u, /* cpuss.dw0_tr_out[23] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT24    = 0x00000519u, /* cpuss.dw0_tr_out[24] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT25    = 0x0000051Au, /* cpuss.dw0_tr_out[25] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT26    = 0x0000051Bu, /* cpuss.dw0_tr_out[26] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT27    = 0x0000051Cu, /* cpuss.dw0_tr_out[27] */
    TRIG_IN_MUX_5_PDMA0_TR_OUT28    = 0x0000051Du, /* cpuss.dw0_tr_out[28] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT0     = 0x0000051Eu, /* cpuss.dw1_tr_out[0] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT1     = 0x0000051Fu, /* cpuss.dw1_tr_out[1] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT2     = 0x00000520u, /* cpuss.dw1_tr_out[2] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT3     = 0x00000521u, /* cpuss.dw1_tr_out[3] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT4     = 0x00000522u, /* cpuss.dw1_tr_out[4] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT5     = 0x00000523u, /* cpuss.dw1_tr_out[5] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT6     = 0x00000524u, /* cpuss.dw1_tr_out[6] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT7     = 0x00000525u, /* cpuss.dw1_tr_out[7] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT8     = 0x00000526u, /* cpuss.dw1_tr_out[8] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT9     = 0x00000527u, /* cpuss.dw1_tr_out[9] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT10    = 0x00000528u, /* cpuss.dw1_tr_out[10] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT11    = 0x00000529u, /* cpuss.dw1_tr_out[11] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT12    = 0x0000052Au, /* cpuss.dw1_tr_out[12] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT13    = 0x0000052Bu, /* cpuss.dw1_tr_out[13] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT14    = 0x0000052Cu, /* cpuss.dw1_tr_out[14] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT15    = 0x0000052Du, /* cpuss.dw1_tr_out[15] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT16    = 0x0000052Eu, /* cpuss.dw1_tr_out[16] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT17    = 0x0000052Fu, /* cpuss.dw1_tr_out[17] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT18    = 0x00000530u, /* cpuss.dw1_tr_out[18] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT19    = 0x00000531u, /* cpuss.dw1_tr_out[19] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT20    = 0x00000532u, /* cpuss.dw1_tr_out[20] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT21    = 0x00000533u, /* cpuss.dw1_tr_out[21] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT22    = 0x00000534u, /* cpuss.dw1_tr_out[22] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT23    = 0x00000535u, /* cpuss.dw1_tr_out[23] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT24    = 0x00000536u, /* cpuss.dw1_tr_out[24] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT25    = 0x00000537u, /* cpuss.dw1_tr_out[25] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT26    = 0x00000538u, /* cpuss.dw1_tr_out[26] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT27    = 0x00000539u, /* cpuss.dw1_tr_out[27] */
    TRIG_IN_MUX_5_PDMA1_TR_OUT28    = 0x0000053Au, /* cpuss.dw1_tr_out[28] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW0 = 0x0000053Bu, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH0 = 0x0000053Cu, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW0 = 0x0000053Du, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW1 = 0x0000053Eu, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH1 = 0x0000053Fu, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW1 = 0x00000540u, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW2 = 0x00000541u, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH2 = 0x00000542u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW2 = 0x00000543u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW3 = 0x00000544u, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH3 = 0x00000545u, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW3 = 0x00000546u, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW4 = 0x00000547u, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH4 = 0x00000548u, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW4 = 0x00000549u, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW5 = 0x0000054Au, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH5 = 0x0000054Bu, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW5 = 0x0000054Cu, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW6 = 0x0000054Du, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH6 = 0x0000054Eu, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW6 = 0x0000054Fu, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_5_TCPWM0_TR_OVERFLOW7 = 0x00000550u, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_5_TCPWM0_TR_COMPARE_MATCH7 = 0x00000551u, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_5_TCPWM0_TR_UNDERFLOW7 = 0x00000552u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW0 = 0x00000553u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH0 = 0x00000554u, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW0 = 0x00000555u, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW1 = 0x00000556u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH1 = 0x00000557u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW1 = 0x00000558u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW2 = 0x00000559u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH2 = 0x0000055Au, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW2 = 0x0000055Bu, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW3 = 0x0000055Cu, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH3 = 0x0000055Du, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW3 = 0x0000055Eu, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW4 = 0x0000055Fu, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH4 = 0x00000560u, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW4 = 0x00000561u, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW5 = 0x00000562u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH5 = 0x00000563u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW5 = 0x00000564u, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW6 = 0x00000565u, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH6 = 0x00000566u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW6 = 0x00000567u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW7 = 0x00000568u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH7 = 0x00000569u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW7 = 0x0000056Au, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW8 = 0x0000056Bu, /* tcpwm[1].tr_overflow[8] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH8 = 0x0000056Cu, /* tcpwm[1].tr_compare_match[8] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW8 = 0x0000056Du, /* tcpwm[1].tr_underflow[8] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW9 = 0x0000056Eu, /* tcpwm[1].tr_overflow[9] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH9 = 0x0000056Fu, /* tcpwm[1].tr_compare_match[9] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW9 = 0x00000570u, /* tcpwm[1].tr_underflow[9] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW10 = 0x00000571u, /* tcpwm[1].tr_overflow[10] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH10 = 0x00000572u, /* tcpwm[1].tr_compare_match[10] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW10 = 0x00000573u, /* tcpwm[1].tr_underflow[10] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW11 = 0x00000574u, /* tcpwm[1].tr_overflow[11] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH11 = 0x00000575u, /* tcpwm[1].tr_compare_match[11] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW11 = 0x00000576u, /* tcpwm[1].tr_underflow[11] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW12 = 0x00000577u, /* tcpwm[1].tr_overflow[12] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH12 = 0x00000578u, /* tcpwm[1].tr_compare_match[12] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW12 = 0x00000579u, /* tcpwm[1].tr_underflow[12] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW13 = 0x0000057Au, /* tcpwm[1].tr_overflow[13] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH13 = 0x0000057Bu, /* tcpwm[1].tr_compare_match[13] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW13 = 0x0000057Cu, /* tcpwm[1].tr_underflow[13] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW14 = 0x0000057Du, /* tcpwm[1].tr_overflow[14] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH14 = 0x0000057Eu, /* tcpwm[1].tr_compare_match[14] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW14 = 0x0000057Fu, /* tcpwm[1].tr_underflow[14] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW15 = 0x00000580u, /* tcpwm[1].tr_overflow[15] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH15 = 0x00000581u, /* tcpwm[1].tr_compare_match[15] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW15 = 0x00000582u, /* tcpwm[1].tr_underflow[15] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW16 = 0x00000583u, /* tcpwm[1].tr_overflow[16] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH16 = 0x00000584u, /* tcpwm[1].tr_compare_match[16] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW16 = 0x00000585u, /* tcpwm[1].tr_underflow[16] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW17 = 0x00000586u, /* tcpwm[1].tr_overflow[17] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH17 = 0x00000587u, /* tcpwm[1].tr_compare_match[17] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW17 = 0x00000588u, /* tcpwm[1].tr_underflow[17] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW18 = 0x00000589u, /* tcpwm[1].tr_overflow[18] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH18 = 0x0000058Au, /* tcpwm[1].tr_compare_match[18] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW18 = 0x0000058Bu, /* tcpwm[1].tr_underflow[18] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW19 = 0x0000058Cu, /* tcpwm[1].tr_overflow[19] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH19 = 0x0000058Du, /* tcpwm[1].tr_compare_match[19] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW19 = 0x0000058Eu, /* tcpwm[1].tr_underflow[19] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW20 = 0x0000058Fu, /* tcpwm[1].tr_overflow[20] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH20 = 0x00000590u, /* tcpwm[1].tr_compare_match[20] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW20 = 0x00000591u, /* tcpwm[1].tr_underflow[20] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW21 = 0x00000592u, /* tcpwm[1].tr_overflow[21] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH21 = 0x00000593u, /* tcpwm[1].tr_compare_match[21] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW21 = 0x00000594u, /* tcpwm[1].tr_underflow[21] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW22 = 0x00000595u, /* tcpwm[1].tr_overflow[22] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH22 = 0x00000596u, /* tcpwm[1].tr_compare_match[22] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW22 = 0x00000597u, /* tcpwm[1].tr_underflow[22] */
    TRIG_IN_MUX_5_TCPWM1_TR_OVERFLOW23 = 0x00000598u, /* tcpwm[1].tr_overflow[23] */
    TRIG_IN_MUX_5_TCPWM1_TR_COMPARE_MATCH23 = 0x00000599u, /* tcpwm[1].tr_compare_match[23] */
    TRIG_IN_MUX_5_TCPWM1_TR_UNDERFLOW23 = 0x0000059Au, /* tcpwm[1].tr_underflow[23] */
    TRIG_IN_MUX_5_MDMA_TR_OUT0      = 0x0000059Bu, /* cpuss.dmac_tr_out[0] */
    TRIG_IN_MUX_5_MDMA_TR_OUT1      = 0x0000059Cu, /* cpuss.dmac_tr_out[1] */
    TRIG_IN_MUX_5_MDMA_TR_OUT2      = 0x0000059Du, /* cpuss.dmac_tr_out[2] */
    TRIG_IN_MUX_5_MDMA_TR_OUT3      = 0x0000059Eu, /* cpuss.dmac_tr_out[3] */
    TRIG_IN_MUX_5_SCB_I2C_SCL0      = 0x0000059Fu, /* scb[0].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX0           = 0x000005A0u, /* scb[0].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX0           = 0x000005A1u, /* scb[0].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL1      = 0x000005A2u, /* scb[1].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX1           = 0x000005A3u, /* scb[1].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX1           = 0x000005A4u, /* scb[1].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL2      = 0x000005A5u, /* scb[2].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX2           = 0x000005A6u, /* scb[2].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX2           = 0x000005A7u, /* scb[2].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL3      = 0x000005A8u, /* scb[3].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX3           = 0x000005A9u, /* scb[3].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX3           = 0x000005AAu, /* scb[3].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL4      = 0x000005ABu, /* scb[4].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX4           = 0x000005ACu, /* scb[4].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX4           = 0x000005ADu, /* scb[4].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL5      = 0x000005AEu, /* scb[5].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX5           = 0x000005AFu, /* scb[5].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX5           = 0x000005B0u, /* scb[5].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL6      = 0x000005B1u, /* scb[6].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX6           = 0x000005B2u, /* scb[6].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX6           = 0x000005B3u, /* scb[6].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL7      = 0x000005B4u, /* scb[7].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX7           = 0x000005B5u, /* scb[7].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX7           = 0x000005B6u, /* scb[7].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL8      = 0x000005B7u, /* scb[8].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX8           = 0x000005B8u, /* scb[8].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX8           = 0x000005B9u, /* scb[8].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL9      = 0x000005BAu, /* scb[9].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX9           = 0x000005BBu, /* scb[9].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX9           = 0x000005BCu, /* scb[9].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL10     = 0x000005BDu, /* scb[10].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX10          = 0x000005BEu, /* scb[10].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX10          = 0x000005BFu, /* scb[10].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL11     = 0x000005C0u, /* scb[11].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX11          = 0x000005C1u, /* scb[11].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX11          = 0x000005C2u, /* scb[11].tr_rx_req */
    TRIG_IN_MUX_5_SCB_I2C_SCL12     = 0x000005C3u, /* scb[12].tr_i2c_scl_filtered */
    TRIG_IN_MUX_5_SCB_TX12          = 0x000005C4u, /* scb[12].tr_tx_req */
    TRIG_IN_MUX_5_SCB_RX12          = 0x000005C5u, /* scb[12].tr_rx_req */
    TRIG_IN_MUX_5_SMIF_TX           = 0x000005C6u, /* smif.tr_tx_req */
    TRIG_IN_MUX_5_SMIF_RX           = 0x000005C7u, /* smif.tr_rx_req */
    TRIG_IN_MUX_5_USB_DMA0          = 0x000005C8u, /* usb.dma_req[0] */
    TRIG_IN_MUX_5_USB_DMA1          = 0x000005C9u, /* usb.dma_req[1] */
    TRIG_IN_MUX_5_USB_DMA2          = 0x000005CAu, /* usb.dma_req[2] */
    TRIG_IN_MUX_5_USB_DMA3          = 0x000005CBu, /* usb.dma_req[3] */
    TRIG_IN_MUX_5_USB_DMA4          = 0x000005CCu, /* usb.dma_req[4] */
    TRIG_IN_MUX_5_USB_DMA5          = 0x000005CDu, /* usb.dma_req[5] */
    TRIG_IN_MUX_5_USB_DMA6          = 0x000005CEu, /* usb.dma_req[6] */
    TRIG_IN_MUX_5_USB_DMA7          = 0x000005CFu, /* usb.dma_req[7] */
    TRIG_IN_MUX_5_I2S_TX0           = 0x000005D0u, /* audioss[0].tr_i2s_tx_req */
    TRIG_IN_MUX_5_I2S_RX0           = 0x000005D1u, /* audioss[0].tr_i2s_rx_req */
    TRIG_IN_MUX_5_PDM_RX0           = 0x000005D2u, /* audioss[0].tr_pdm_rx_req */
    TRIG_IN_MUX_5_I2S_TX1           = 0x000005D3u, /* audioss[1].tr_i2s_tx_req */
    TRIG_IN_MUX_5_I2S_RX1           = 0x000005D4u, /* audioss[1].tr_i2s_rx_req */
    TRIG_IN_MUX_5_CSD_SENSE         = 0x000005D5u, /* csd.dsi_sense_out */
    TRIG_IN_MUX_5_CSD_SAMPLE        = 0x000005D6u, /* csd.dsi_sample_out */
    TRIG_IN_MUX_5_CSD_ADC_DONE      = 0x000005D7u, /* csd.tr_adc_done */
    TRIG_IN_MUX_5_PASS_SAR_DONE     = 0x000005D8u, /* pass.tr_sar_out */
    TRIG_IN_MUX_5_HSIOM_TR_OUT0     = 0x000005D9u, /* peri.tr_io_input[0] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT1     = 0x000005DAu, /* peri.tr_io_input[1] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT2     = 0x000005DBu, /* peri.tr_io_input[2] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT3     = 0x000005DCu, /* peri.tr_io_input[3] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT4     = 0x000005DDu, /* peri.tr_io_input[4] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT5     = 0x000005DEu, /* peri.tr_io_input[5] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT6     = 0x000005DFu, /* peri.tr_io_input[6] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT7     = 0x000005E0u, /* peri.tr_io_input[7] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT8     = 0x000005E1u, /* peri.tr_io_input[8] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT9     = 0x000005E2u, /* peri.tr_io_input[9] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT10    = 0x000005E3u, /* peri.tr_io_input[10] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT11    = 0x000005E4u, /* peri.tr_io_input[11] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT12    = 0x000005E5u, /* peri.tr_io_input[12] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT13    = 0x000005E6u, /* peri.tr_io_input[13] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT14    = 0x000005E7u, /* peri.tr_io_input[14] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT15    = 0x000005E8u, /* peri.tr_io_input[15] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT16    = 0x000005E9u, /* peri.tr_io_input[16] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT17    = 0x000005EAu, /* peri.tr_io_input[17] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT18    = 0x000005EBu, /* peri.tr_io_input[18] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT19    = 0x000005ECu, /* peri.tr_io_input[19] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT20    = 0x000005EDu, /* peri.tr_io_input[20] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT21    = 0x000005EEu, /* peri.tr_io_input[21] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT22    = 0x000005EFu, /* peri.tr_io_input[22] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT23    = 0x000005F0u, /* peri.tr_io_input[23] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT24    = 0x000005F1u, /* peri.tr_io_input[24] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT25    = 0x000005F2u, /* peri.tr_io_input[25] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT26    = 0x000005F3u, /* peri.tr_io_input[26] */
    TRIG_IN_MUX_5_HSIOM_TR_OUT27    = 0x000005F4u, /* peri.tr_io_input[27] */
    TRIG_IN_MUX_5_FAULT_TR_OUT0     = 0x000005F5u, /* cpuss.tr_fault[0] */
    TRIG_IN_MUX_5_FAULT_TR_OUT1     = 0x000005F6u, /* cpuss.tr_fault[1] */
    TRIG_IN_MUX_5_CTI_TR_OUT0       = 0x000005F7u, /* cpuss.cti_tr_out[0] */
    TRIG_IN_MUX_5_CTI_TR_OUT1       = 0x000005F8u, /* cpuss.cti_tr_out[1] */
    TRIG_IN_MUX_5_LPCOMP_DSI_COMP0  = 0x000005F9u, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_5_LPCOMP_DSI_COMP1  = 0x000005FAu /* lpcomp.dsi_comp1 */
} en_trig_input_cpuss_cti_t;

/* Trigger Input Group 6 - MDMA trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW0 = 0x00000601u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH0 = 0x00000602u, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW0 = 0x00000603u, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW1 = 0x00000604u, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH1 = 0x00000605u, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW1 = 0x00000606u, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW2 = 0x00000607u, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH2 = 0x00000608u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW2 = 0x00000609u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW3 = 0x0000060Au, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH3 = 0x0000060Bu, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW3 = 0x0000060Cu, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW4 = 0x0000060Du, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH4 = 0x0000060Eu, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW4 = 0x0000060Fu, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW5 = 0x00000610u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH5 = 0x00000611u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW5 = 0x00000612u, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW6 = 0x00000613u, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH6 = 0x00000614u, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW6 = 0x00000615u, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_6_TCPWM1_TR_OVERFLOW7 = 0x00000616u, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_6_TCPWM1_TR_COMPARE_MATCH7 = 0x00000617u, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_6_TCPWM1_TR_UNDERFLOW7 = 0x00000618u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_6_SMIF_TX           = 0x00000619u, /* smif.tr_tx_req */
    TRIG_IN_MUX_6_SMIF_RX           = 0x0000061Au /* smif.tr_rx_req */
} en_trig_input_mdma_t;

/* Trigger Input Group 7 - PERI Freeze trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_7_CTI_TR_OUT0       = 0x00000701u, /* cpuss.cti_tr_out[0] */
    TRIG_IN_MUX_7_CTI_TR_OUT1       = 0x00000702u /* cpuss.cti_tr_out[1] */
} en_trig_input_peri_freeze_t;

/* Trigger Input Group 8 - Capsense trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW0 = 0x00000801u, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH0 = 0x00000802u, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW0 = 0x00000803u, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW1 = 0x00000804u, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH1 = 0x00000805u, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW1 = 0x00000806u, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW2 = 0x00000807u, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH2 = 0x00000808u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW2 = 0x00000809u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW3 = 0x0000080Au, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH3 = 0x0000080Bu, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW3 = 0x0000080Cu, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW4 = 0x0000080Du, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH4 = 0x0000080Eu, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW4 = 0x0000080Fu, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW5 = 0x00000810u, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH5 = 0x00000811u, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW5 = 0x00000812u, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW6 = 0x00000813u, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH6 = 0x00000814u, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW6 = 0x00000815u, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_8_TCPWM0_TR_OVERFLOW7 = 0x00000816u, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_8_TCPWM0_TR_COMPARE_MATCH7 = 0x00000817u, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_8_TCPWM0_TR_UNDERFLOW7 = 0x00000818u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW0 = 0x00000819u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH0 = 0x0000081Au, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW0 = 0x0000081Bu, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW1 = 0x0000081Cu, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH1 = 0x0000081Du, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW1 = 0x0000081Eu, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW2 = 0x0000081Fu, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH2 = 0x00000820u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW2 = 0x00000821u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW3 = 0x00000822u, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH3 = 0x00000823u, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW3 = 0x00000824u, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW4 = 0x00000825u, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH4 = 0x00000826u, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW4 = 0x00000827u, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW5 = 0x00000828u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH5 = 0x00000829u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW5 = 0x0000082Au, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW6 = 0x0000082Bu, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH6 = 0x0000082Cu, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW6 = 0x0000082Du, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW7 = 0x0000082Eu, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH7 = 0x0000082Fu, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW7 = 0x00000830u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW8 = 0x00000831u, /* tcpwm[1].tr_overflow[8] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH8 = 0x00000832u, /* tcpwm[1].tr_compare_match[8] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW8 = 0x00000833u, /* tcpwm[1].tr_underflow[8] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW9 = 0x00000834u, /* tcpwm[1].tr_overflow[9] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH9 = 0x00000835u, /* tcpwm[1].tr_compare_match[9] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW9 = 0x00000836u, /* tcpwm[1].tr_underflow[9] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW10 = 0x00000837u, /* tcpwm[1].tr_overflow[10] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH10 = 0x00000838u, /* tcpwm[1].tr_compare_match[10] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW10 = 0x00000839u, /* tcpwm[1].tr_underflow[10] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW11 = 0x0000083Au, /* tcpwm[1].tr_overflow[11] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH11 = 0x0000083Bu, /* tcpwm[1].tr_compare_match[11] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW11 = 0x0000083Cu, /* tcpwm[1].tr_underflow[11] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW12 = 0x0000083Du, /* tcpwm[1].tr_overflow[12] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH12 = 0x0000083Eu, /* tcpwm[1].tr_compare_match[12] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW12 = 0x0000083Fu, /* tcpwm[1].tr_underflow[12] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW13 = 0x00000840u, /* tcpwm[1].tr_overflow[13] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH13 = 0x00000841u, /* tcpwm[1].tr_compare_match[13] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW13 = 0x00000842u, /* tcpwm[1].tr_underflow[13] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW14 = 0x00000843u, /* tcpwm[1].tr_overflow[14] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH14 = 0x00000844u, /* tcpwm[1].tr_compare_match[14] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW14 = 0x00000845u, /* tcpwm[1].tr_underflow[14] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW15 = 0x00000846u, /* tcpwm[1].tr_overflow[15] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH15 = 0x00000847u, /* tcpwm[1].tr_compare_match[15] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW15 = 0x00000848u, /* tcpwm[1].tr_underflow[15] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW16 = 0x00000849u, /* tcpwm[1].tr_overflow[16] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH16 = 0x0000084Au, /* tcpwm[1].tr_compare_match[16] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW16 = 0x0000084Bu, /* tcpwm[1].tr_underflow[16] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW17 = 0x0000084Cu, /* tcpwm[1].tr_overflow[17] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH17 = 0x0000084Du, /* tcpwm[1].tr_compare_match[17] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW17 = 0x0000084Eu, /* tcpwm[1].tr_underflow[17] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW18 = 0x0000084Fu, /* tcpwm[1].tr_overflow[18] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH18 = 0x00000850u, /* tcpwm[1].tr_compare_match[18] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW18 = 0x00000851u, /* tcpwm[1].tr_underflow[18] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW19 = 0x00000852u, /* tcpwm[1].tr_overflow[19] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH19 = 0x00000853u, /* tcpwm[1].tr_compare_match[19] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW19 = 0x00000854u, /* tcpwm[1].tr_underflow[19] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW20 = 0x00000855u, /* tcpwm[1].tr_overflow[20] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH20 = 0x00000856u, /* tcpwm[1].tr_compare_match[20] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW20 = 0x00000857u, /* tcpwm[1].tr_underflow[20] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW21 = 0x00000858u, /* tcpwm[1].tr_overflow[21] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH21 = 0x00000859u, /* tcpwm[1].tr_compare_match[21] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW21 = 0x0000085Au, /* tcpwm[1].tr_underflow[21] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW22 = 0x0000085Bu, /* tcpwm[1].tr_overflow[22] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH22 = 0x0000085Cu, /* tcpwm[1].tr_compare_match[22] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW22 = 0x0000085Du, /* tcpwm[1].tr_underflow[22] */
    TRIG_IN_MUX_8_TCPWM1_TR_OVERFLOW23 = 0x0000085Eu, /* tcpwm[1].tr_overflow[23] */
    TRIG_IN_MUX_8_TCPWM1_TR_COMPARE_MATCH23 = 0x0000085Fu, /* tcpwm[1].tr_compare_match[23] */
    TRIG_IN_MUX_8_TCPWM1_TR_UNDERFLOW23 = 0x00000860u, /* tcpwm[1].tr_underflow[23] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT0     = 0x00000861u, /* peri.tr_io_input[0] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT1     = 0x00000862u, /* peri.tr_io_input[1] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT2     = 0x00000863u, /* peri.tr_io_input[2] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT3     = 0x00000864u, /* peri.tr_io_input[3] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT4     = 0x00000865u, /* peri.tr_io_input[4] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT5     = 0x00000866u, /* peri.tr_io_input[5] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT6     = 0x00000867u, /* peri.tr_io_input[6] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT7     = 0x00000868u, /* peri.tr_io_input[7] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT8     = 0x00000869u, /* peri.tr_io_input[8] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT9     = 0x0000086Au, /* peri.tr_io_input[9] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT10    = 0x0000086Bu, /* peri.tr_io_input[10] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT11    = 0x0000086Cu, /* peri.tr_io_input[11] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT12    = 0x0000086Du, /* peri.tr_io_input[12] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT13    = 0x0000086Eu, /* peri.tr_io_input[13] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT14    = 0x0000086Fu, /* peri.tr_io_input[14] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT15    = 0x00000870u, /* peri.tr_io_input[15] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT16    = 0x00000871u, /* peri.tr_io_input[16] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT17    = 0x00000872u, /* peri.tr_io_input[17] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT18    = 0x00000873u, /* peri.tr_io_input[18] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT19    = 0x00000874u, /* peri.tr_io_input[19] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT20    = 0x00000875u, /* peri.tr_io_input[20] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT21    = 0x00000876u, /* peri.tr_io_input[21] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT22    = 0x00000877u, /* peri.tr_io_input[22] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT23    = 0x00000878u, /* peri.tr_io_input[23] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT24    = 0x00000879u, /* peri.tr_io_input[24] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT25    = 0x0000087Au, /* peri.tr_io_input[25] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT26    = 0x0000087Bu, /* peri.tr_io_input[26] */
    TRIG_IN_MUX_8_HSIOM_TR_OUT27    = 0x0000087Cu, /* peri.tr_io_input[27] */
    TRIG_IN_MUX_8_LPCOMP_DSI_COMP0  = 0x0000087Du, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_8_LPCOMP_DSI_COMP1  = 0x0000087Eu /* lpcomp.dsi_comp1 */
} en_trig_input_csd_t;

/* Trigger Input Group 9 - ADC trigger multiplexer */
typedef enum
{
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW0 = 0x00000901u, /* tcpwm[0].tr_overflow[0] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH0 = 0x00000902u, /* tcpwm[0].tr_compare_match[0] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW0 = 0x00000903u, /* tcpwm[0].tr_underflow[0] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW1 = 0x00000904u, /* tcpwm[0].tr_overflow[1] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH1 = 0x00000905u, /* tcpwm[0].tr_compare_match[1] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW1 = 0x00000906u, /* tcpwm[0].tr_underflow[1] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW2 = 0x00000907u, /* tcpwm[0].tr_overflow[2] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH2 = 0x00000908u, /* tcpwm[0].tr_compare_match[2] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW2 = 0x00000909u, /* tcpwm[0].tr_underflow[2] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW3 = 0x0000090Au, /* tcpwm[0].tr_overflow[3] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH3 = 0x0000090Bu, /* tcpwm[0].tr_compare_match[3] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW3 = 0x0000090Cu, /* tcpwm[0].tr_underflow[3] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW4 = 0x0000090Du, /* tcpwm[0].tr_overflow[4] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH4 = 0x0000090Eu, /* tcpwm[0].tr_compare_match[4] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW4 = 0x0000090Fu, /* tcpwm[0].tr_underflow[4] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW5 = 0x00000910u, /* tcpwm[0].tr_overflow[5] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH5 = 0x00000911u, /* tcpwm[0].tr_compare_match[5] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW5 = 0x00000912u, /* tcpwm[0].tr_underflow[5] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW6 = 0x00000913u, /* tcpwm[0].tr_overflow[6] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH6 = 0x00000914u, /* tcpwm[0].tr_compare_match[6] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW6 = 0x00000915u, /* tcpwm[0].tr_underflow[6] */
    TRIG_IN_MUX_9_TCPWM0_TR_OVERFLOW7 = 0x00000916u, /* tcpwm[0].tr_overflow[7] */
    TRIG_IN_MUX_9_TCPWM0_TR_COMPARE_MATCH7 = 0x00000917u, /* tcpwm[0].tr_compare_match[7] */
    TRIG_IN_MUX_9_TCPWM0_TR_UNDERFLOW7 = 0x00000918u, /* tcpwm[0].tr_underflow[7] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW0 = 0x00000919u, /* tcpwm[1].tr_overflow[0] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH0 = 0x0000091Au, /* tcpwm[1].tr_compare_match[0] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW0 = 0x0000091Bu, /* tcpwm[1].tr_underflow[0] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW1 = 0x0000091Cu, /* tcpwm[1].tr_overflow[1] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH1 = 0x0000091Du, /* tcpwm[1].tr_compare_match[1] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW1 = 0x0000091Eu, /* tcpwm[1].tr_underflow[1] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW2 = 0x0000091Fu, /* tcpwm[1].tr_overflow[2] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH2 = 0x00000920u, /* tcpwm[1].tr_compare_match[2] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW2 = 0x00000921u, /* tcpwm[1].tr_underflow[2] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW3 = 0x00000922u, /* tcpwm[1].tr_overflow[3] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH3 = 0x00000923u, /* tcpwm[1].tr_compare_match[3] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW3 = 0x00000924u, /* tcpwm[1].tr_underflow[3] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW4 = 0x00000925u, /* tcpwm[1].tr_overflow[4] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH4 = 0x00000926u, /* tcpwm[1].tr_compare_match[4] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW4 = 0x00000927u, /* tcpwm[1].tr_underflow[4] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW5 = 0x00000928u, /* tcpwm[1].tr_overflow[5] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH5 = 0x00000929u, /* tcpwm[1].tr_compare_match[5] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW5 = 0x0000092Au, /* tcpwm[1].tr_underflow[5] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW6 = 0x0000092Bu, /* tcpwm[1].tr_overflow[6] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH6 = 0x0000092Cu, /* tcpwm[1].tr_compare_match[6] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW6 = 0x0000092Du, /* tcpwm[1].tr_underflow[6] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW7 = 0x0000092Eu, /* tcpwm[1].tr_overflow[7] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH7 = 0x0000092Fu, /* tcpwm[1].tr_compare_match[7] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW7 = 0x00000930u, /* tcpwm[1].tr_underflow[7] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW8 = 0x00000931u, /* tcpwm[1].tr_overflow[8] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH8 = 0x00000932u, /* tcpwm[1].tr_compare_match[8] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW8 = 0x00000933u, /* tcpwm[1].tr_underflow[8] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW9 = 0x00000934u, /* tcpwm[1].tr_overflow[9] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH9 = 0x00000935u, /* tcpwm[1].tr_compare_match[9] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW9 = 0x00000936u, /* tcpwm[1].tr_underflow[9] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW10 = 0x00000937u, /* tcpwm[1].tr_overflow[10] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH10 = 0x00000938u, /* tcpwm[1].tr_compare_match[10] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW10 = 0x00000939u, /* tcpwm[1].tr_underflow[10] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW11 = 0x0000093Au, /* tcpwm[1].tr_overflow[11] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH11 = 0x0000093Bu, /* tcpwm[1].tr_compare_match[11] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW11 = 0x0000093Cu, /* tcpwm[1].tr_underflow[11] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW12 = 0x0000093Du, /* tcpwm[1].tr_overflow[12] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH12 = 0x0000093Eu, /* tcpwm[1].tr_compare_match[12] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW12 = 0x0000093Fu, /* tcpwm[1].tr_underflow[12] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW13 = 0x00000940u, /* tcpwm[1].tr_overflow[13] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH13 = 0x00000941u, /* tcpwm[1].tr_compare_match[13] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW13 = 0x00000942u, /* tcpwm[1].tr_underflow[13] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW14 = 0x00000943u, /* tcpwm[1].tr_overflow[14] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH14 = 0x00000944u, /* tcpwm[1].tr_compare_match[14] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW14 = 0x00000945u, /* tcpwm[1].tr_underflow[14] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW15 = 0x00000946u, /* tcpwm[1].tr_overflow[15] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH15 = 0x00000947u, /* tcpwm[1].tr_compare_match[15] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW15 = 0x00000948u, /* tcpwm[1].tr_underflow[15] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW16 = 0x00000949u, /* tcpwm[1].tr_overflow[16] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH16 = 0x0000094Au, /* tcpwm[1].tr_compare_match[16] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW16 = 0x0000094Bu, /* tcpwm[1].tr_underflow[16] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW17 = 0x0000094Cu, /* tcpwm[1].tr_overflow[17] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH17 = 0x0000094Du, /* tcpwm[1].tr_compare_match[17] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW17 = 0x0000094Eu, /* tcpwm[1].tr_underflow[17] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW18 = 0x0000094Fu, /* tcpwm[1].tr_overflow[18] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH18 = 0x00000950u, /* tcpwm[1].tr_compare_match[18] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW18 = 0x00000951u, /* tcpwm[1].tr_underflow[18] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW19 = 0x00000952u, /* tcpwm[1].tr_overflow[19] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH19 = 0x00000953u, /* tcpwm[1].tr_compare_match[19] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW19 = 0x00000954u, /* tcpwm[1].tr_underflow[19] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW20 = 0x00000955u, /* tcpwm[1].tr_overflow[20] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH20 = 0x00000956u, /* tcpwm[1].tr_compare_match[20] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW20 = 0x00000957u, /* tcpwm[1].tr_underflow[20] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW21 = 0x00000958u, /* tcpwm[1].tr_overflow[21] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH21 = 0x00000959u, /* tcpwm[1].tr_compare_match[21] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW21 = 0x0000095Au, /* tcpwm[1].tr_underflow[21] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW22 = 0x0000095Bu, /* tcpwm[1].tr_overflow[22] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH22 = 0x0000095Cu, /* tcpwm[1].tr_compare_match[22] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW22 = 0x0000095Du, /* tcpwm[1].tr_underflow[22] */
    TRIG_IN_MUX_9_TCPWM1_TR_OVERFLOW23 = 0x0000095Eu, /* tcpwm[1].tr_overflow[23] */
    TRIG_IN_MUX_9_TCPWM1_TR_COMPARE_MATCH23 = 0x0000095Fu, /* tcpwm[1].tr_compare_match[23] */
    TRIG_IN_MUX_9_TCPWM1_TR_UNDERFLOW23 = 0x00000960u, /* tcpwm[1].tr_underflow[23] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT0     = 0x00000961u, /* peri.tr_io_input[0] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT1     = 0x00000962u, /* peri.tr_io_input[1] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT2     = 0x00000963u, /* peri.tr_io_input[2] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT3     = 0x00000964u, /* peri.tr_io_input[3] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT4     = 0x00000965u, /* peri.tr_io_input[4] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT5     = 0x00000966u, /* peri.tr_io_input[5] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT6     = 0x00000967u, /* peri.tr_io_input[6] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT7     = 0x00000968u, /* peri.tr_io_input[7] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT8     = 0x00000969u, /* peri.tr_io_input[8] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT9     = 0x0000096Au, /* peri.tr_io_input[9] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT10    = 0x0000096Bu, /* peri.tr_io_input[10] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT11    = 0x0000096Cu, /* peri.tr_io_input[11] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT12    = 0x0000096Du, /* peri.tr_io_input[12] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT13    = 0x0000096Eu, /* peri.tr_io_input[13] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT14    = 0x0000096Fu, /* peri.tr_io_input[14] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT15    = 0x00000970u, /* peri.tr_io_input[15] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT16    = 0x00000971u, /* peri.tr_io_input[16] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT17    = 0x00000972u, /* peri.tr_io_input[17] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT18    = 0x00000973u, /* peri.tr_io_input[18] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT19    = 0x00000974u, /* peri.tr_io_input[19] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT20    = 0x00000975u, /* peri.tr_io_input[20] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT21    = 0x00000976u, /* peri.tr_io_input[21] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT22    = 0x00000977u, /* peri.tr_io_input[22] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT23    = 0x00000978u, /* peri.tr_io_input[23] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT24    = 0x00000979u, /* peri.tr_io_input[24] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT25    = 0x0000097Au, /* peri.tr_io_input[25] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT26    = 0x0000097Bu, /* peri.tr_io_input[26] */
    TRIG_IN_MUX_9_HSIOM_TR_OUT27    = 0x0000097Cu, /* peri.tr_io_input[27] */
    TRIG_IN_MUX_9_LPCOMP_DSI_COMP0  = 0x0000097Du, /* lpcomp.dsi_comp0 */
    TRIG_IN_MUX_9_LPCOMP_DSI_COMP1  = 0x0000097Eu /* lpcomp.dsi_comp1 */
} en_trig_input_sar_adc_start_t;

/* Trigger Input Group 0 - SCB PDMA0 Triggers (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA00 = 0x00001000u, /* scb[0].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA00 = 0x00001001u, /* scb[0].tr_rx_req */
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA01 = 0x00001002u, /* scb[1].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA01 = 0x00001003u, /* scb[1].tr_rx_req */
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA02 = 0x00001004u, /* scb[2].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA02 = 0x00001005u, /* scb[2].tr_rx_req */
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA03 = 0x00001006u, /* scb[3].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA03 = 0x00001007u, /* scb[3].tr_rx_req */
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA04 = 0x00001008u, /* scb[4].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA04 = 0x00001009u, /* scb[4].tr_rx_req */
    TRIG_IN_1TO1_0_SCB_TX_TO_PDMA05 = 0x0000100Au, /* scb[5].tr_tx_req */
    TRIG_IN_1TO1_0_SCB_RX_TO_PDMA05 = 0x0000100Bu /* scb[5].tr_rx_req */
} en_trig_input_1to1_scb_pdma0_tr_t;

/* Trigger Input Group 1 - SCB PDMA1 Triggers (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA16 = 0x00001100u, /* scb[6].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA16 = 0x00001101u, /* scb[6].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA17 = 0x00001102u, /* scb[7].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA17 = 0x00001103u, /* scb[7].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA18 = 0x00001104u, /* scb[8].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA18 = 0x00001105u, /* scb[8].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA19 = 0x00001106u, /* scb[9].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA19 = 0x00001107u, /* scb[9].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA110 = 0x00001108u, /* scb[10].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA110 = 0x00001109u, /* scb[10].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA111 = 0x0000110Au, /* scb[11].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA111 = 0x0000110Bu, /* scb[11].tr_rx_req */
    TRIG_IN_1TO1_1_SCB_TX_TO_PDMA112 = 0x0000110Cu, /* scb[12].tr_tx_req */
    TRIG_IN_1TO1_1_SCB_RX_TO_PDMA112 = 0x0000110Du /* scb[12].tr_rx_req */
} en_trig_input_1to1_scb_pdma1_tr_t;

/* Trigger Input Group 2 - PASS to PDMA0 direct connect (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_2_PASS_SAR_DONE_TO_PDMA0 = 0x00001200u /* pass.tr_sar_out */
} en_trig_input_1to1_sar_to_pdma0_t;

/* Trigger Input Group 3 -  (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_3_SMIF_TX_TO_PDMA1 = 0x00001300u, /* smif.tr_tx_req */
    TRIG_IN_1TO1_3_SMIF_RX_TO_PDMA1 = 0x00001301u /* smif.tr_rx_req */
} en_trig_input_1to1_smif_to_pdma1_t;

/* Trigger Input Group 4 - I2S and PDM PDMA triggers (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_4_I2S_TX_TO_PDMA10 = 0x00001400u, /* audioss[0].tr_i2s_tx_req */
    TRIG_IN_1TO1_4_I2S_RX_TO_PDMA10 = 0x00001401u, /* audioss[0].tr_i2s_rx_req */
    TRIG_IN_1TO1_4_PDM_RX_TO_PDMA10 = 0x00001402u, /* audioss[0].tr_pdm_rx_req */
    TRIG_IN_1TO1_4_I2S_TX_TO_PDMA11 = 0x00001403u, /* audioss[1].tr_i2s_tx_req */
    TRIG_IN_1TO1_4_I2S_RX_TO_PDMA11 = 0x00001404u /* audioss[1].tr_i2s_rx_req */
} en_trig_input_1to1_audioss_pdma1_tr_t;

/* Trigger Input Group 5 - USB PDMA0 Triggers (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA00 = 0x00001500u, /* usb.dma_req[0] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA01 = 0x00001501u, /* usb.dma_req[1] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA02 = 0x00001502u, /* usb.dma_req[2] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA03 = 0x00001503u, /* usb.dma_req[3] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA04 = 0x00001504u, /* usb.dma_req[4] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA05 = 0x00001505u, /* usb.dma_req[5] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA06 = 0x00001506u, /* usb.dma_req[6] */
    TRIG_IN_1TO1_5_USB_DMA_TO_PDMA07 = 0x00001507u /* usb.dma_req[7] */
} en_trig_input_1to1_usb_pdma0_tr_t;

/* Trigger Input Group 6 - USB PDMA0 Acknowledge Triggers (OneToOne) */
typedef enum
{
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK0 = 0x00001600u, /* cpuss.dw0_tr_out[8] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK1 = 0x00001601u, /* cpuss.dw0_tr_out[9] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK2 = 0x00001602u, /* cpuss.dw0_tr_out[10] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK3 = 0x00001603u, /* cpuss.dw0_tr_out[11] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK4 = 0x00001604u, /* cpuss.dw0_tr_out[12] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK5 = 0x00001605u, /* cpuss.dw0_tr_out[13] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK6 = 0x00001606u, /* cpuss.dw0_tr_out[14] */
    TRIG_IN_1TO1_6_PDMA0_TO_USB_ACK7 = 0x00001607u /* cpuss.dw0_tr_out[15] */
} en_trig_input_1to1_usb_pdma0_ack_tr_t;

/* Trigger Group Outputs */
/* Trigger Output Group 0 - P-DMA0 Request Assignments */
typedef enum
{
    TRIG_OUT_MUX_0_PDMA0_TR_IN0     = 0x40000000u, /* cpuss.dw0_tr_in[0] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN1     = 0x40000001u, /* cpuss.dw0_tr_in[1] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN2     = 0x40000002u, /* cpuss.dw0_tr_in[2] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN3     = 0x40000003u, /* cpuss.dw0_tr_in[3] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN4     = 0x40000004u, /* cpuss.dw0_tr_in[4] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN5     = 0x40000005u, /* cpuss.dw0_tr_in[5] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN6     = 0x40000006u, /* cpuss.dw0_tr_in[6] */
    TRIG_OUT_MUX_0_PDMA0_TR_IN7     = 0x40000007u /* cpuss.dw0_tr_in[7] */
} en_trig_output_pdma0_tr_t;

/* Trigger Output Group 1 - P-DMA1 Request Assignments */
typedef enum
{
    TRIG_OUT_MUX_1_PDMA1_TR_IN0     = 0x40000100u, /* cpuss.dw1_tr_in[0] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN1     = 0x40000101u, /* cpuss.dw1_tr_in[1] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN2     = 0x40000102u, /* cpuss.dw1_tr_in[2] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN3     = 0x40000103u, /* cpuss.dw1_tr_in[3] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN4     = 0x40000104u, /* cpuss.dw1_tr_in[4] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN5     = 0x40000105u, /* cpuss.dw1_tr_in[5] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN6     = 0x40000106u, /* cpuss.dw1_tr_in[6] */
    TRIG_OUT_MUX_1_PDMA1_TR_IN7     = 0x40000107u /* cpuss.dw1_tr_in[7] */
} en_trig_output_pdma1_tr_t;

/* Trigger Output Group 2 - TCPWM0 trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_2_TCPWM0_TR_IN0    = 0x40000200u, /* tcpwm[0].tr_in[0] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN1    = 0x40000201u, /* tcpwm[0].tr_in[1] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN2    = 0x40000202u, /* tcpwm[0].tr_in[2] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN3    = 0x40000203u, /* tcpwm[0].tr_in[3] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN4    = 0x40000204u, /* tcpwm[0].tr_in[4] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN5    = 0x40000205u, /* tcpwm[0].tr_in[5] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN6    = 0x40000206u, /* tcpwm[0].tr_in[6] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN7    = 0x40000207u, /* tcpwm[0].tr_in[7] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN8    = 0x40000208u, /* tcpwm[0].tr_in[8] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN9    = 0x40000209u, /* tcpwm[0].tr_in[9] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN10   = 0x4000020Au, /* tcpwm[0].tr_in[10] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN11   = 0x4000020Bu, /* tcpwm[0].tr_in[11] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN12   = 0x4000020Cu, /* tcpwm[0].tr_in[12] */
    TRIG_OUT_MUX_2_TCPWM0_TR_IN13   = 0x4000020Du /* tcpwm[0].tr_in[13] */
} en_trig_output_tcpwm0_t;

/* Trigger Output Group 3 - TCPWM1 trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_3_TCPWM1_TR_IN0    = 0x40000300u, /* tcpwm[1].tr_in[0] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN1    = 0x40000301u, /* tcpwm[1].tr_in[1] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN2    = 0x40000302u, /* tcpwm[1].tr_in[2] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN3    = 0x40000303u, /* tcpwm[1].tr_in[3] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN4    = 0x40000304u, /* tcpwm[1].tr_in[4] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN5    = 0x40000305u, /* tcpwm[1].tr_in[5] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN6    = 0x40000306u, /* tcpwm[1].tr_in[6] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN7    = 0x40000307u, /* tcpwm[1].tr_in[7] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN8    = 0x40000308u, /* tcpwm[1].tr_in[8] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN9    = 0x40000309u, /* tcpwm[1].tr_in[9] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN10   = 0x4000030Au, /* tcpwm[1].tr_in[10] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN11   = 0x4000030Bu, /* tcpwm[1].tr_in[11] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN12   = 0x4000030Cu, /* tcpwm[1].tr_in[12] */
    TRIG_OUT_MUX_3_TCPWM1_TR_IN13   = 0x4000030Du /* tcpwm[1].tr_in[13] */
} en_trig_output_tcpwm1_t;

/* Trigger Output Group 4 - HSIOM trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_4_HSIOM_TR_IO_OUTPUT0 = 0x40000400u, /* peri.tr_io_output[0] */
    TRIG_OUT_MUX_4_HSIOM_TR_IO_OUTPUT1 = 0x40000401u /* peri.tr_io_output[1] */
} en_trig_output_hsiom_t;

/* Trigger Output Group 5 - CPUSS Debug and Profiler trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_5_CPUSS_CTI_TR_IN0 = 0x40000500u, /* cpuss.cti_tr_in[0] */
    TRIG_OUT_MUX_5_CPUSS_CTI_TR_IN1 = 0x40000501u, /* cpuss.cti_tr_in[1] */
    TRIG_OUT_MUX_5_PROFILE_TR_START = 0x40000502u, /* profile.tr_start */
    TRIG_OUT_MUX_5_PROFILE_TR_STOP  = 0x40000503u /* profile.tr_stop */
} en_trig_output_cpuss_cti_t;

/* Trigger Output Group 6 - MDMA trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_6_MDMA_TR_IN0      = 0x40000600u, /* cpuss.dmac_tr_in[0] */
    TRIG_OUT_MUX_6_MDMA_TR_IN1      = 0x40000601u, /* cpuss.dmac_tr_in[1] */
    TRIG_OUT_MUX_6_MDMA_TR_IN2      = 0x40000602u, /* cpuss.dmac_tr_in[2] */
    TRIG_OUT_MUX_6_MDMA_TR_IN3      = 0x40000603u /* cpuss.dmac_tr_in[3] */
} en_trig_output_mdma_t;

/* Trigger Output Group 7 - PERI Freeze trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_7_DEBUG_FREEZE_TR_IN = 0x40000700u /* peri.tr_dbg_freeze */
} en_trig_output_peri_freeze_t;

/* Trigger Output Group 8 - Capsense trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_8_CSD_DSI_START    = 0x40000800u /* csd.dsi_start */
} en_trig_output_csd_t;

/* Trigger Output Group 9 - ADC trigger multiplexer */
typedef enum
{
    TRIG_OUT_MUX_9_PASS_TR_SAR_IN   = 0x40000900u /* pass.tr_sar_in */
} en_trig_output_sar_adc_start_t;

/* Trigger Output Group 0 - SCB PDMA0 Triggers (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA00 = 0x40001000u, /* cpuss.dw0_tr_in[16] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA00 = 0x40001001u, /* cpuss.dw0_tr_in[17] */
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA01 = 0x40001002u, /* cpuss.dw0_tr_in[18] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA01 = 0x40001003u, /* cpuss.dw0_tr_in[19] */
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA02 = 0x40001004u, /* cpuss.dw0_tr_in[20] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA02 = 0x40001005u, /* cpuss.dw0_tr_in[21] */
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA03 = 0x40001006u, /* cpuss.dw0_tr_in[22] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA03 = 0x40001007u, /* cpuss.dw0_tr_in[23] */
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA04 = 0x40001008u, /* cpuss.dw0_tr_in[24] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA04 = 0x40001009u, /* cpuss.dw0_tr_in[25] */
    TRIG_OUT_1TO1_0_SCB_TX_TO_PDMA05 = 0x4000100Au, /* cpuss.dw0_tr_in[26] */
    TRIG_OUT_1TO1_0_SCB_RX_TO_PDMA05 = 0x4000100Bu /* cpuss.dw0_tr_in[27] */
} en_trig_output_1to1_scb_pdma0_tr_t;

/* Trigger Output Group 1 - SCB PDMA1 Triggers (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA16 = 0x40001100u, /* cpuss.dw1_tr_in[8] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA16 = 0x40001101u, /* cpuss.dw1_tr_in[9] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA17 = 0x40001102u, /* cpuss.dw1_tr_in[10] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA17 = 0x40001103u, /* cpuss.dw1_tr_in[11] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA18 = 0x40001104u, /* cpuss.dw1_tr_in[12] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA18 = 0x40001105u, /* cpuss.dw1_tr_in[13] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA19 = 0x40001106u, /* cpuss.dw1_tr_in[14] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA19 = 0x40001107u, /* cpuss.dw1_tr_in[15] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA110 = 0x40001108u, /* cpuss.dw1_tr_in[16] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA110 = 0x40001109u, /* cpuss.dw1_tr_in[17] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA111 = 0x4000110Au, /* cpuss.dw1_tr_in[18] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA111 = 0x4000110Bu, /* cpuss.dw1_tr_in[19] */
    TRIG_OUT_1TO1_1_SCB_TX_TO_PDMA112 = 0x4000110Cu, /* cpuss.dw1_tr_in[20] */
    TRIG_OUT_1TO1_1_SCB_RX_TO_PDMA112 = 0x4000110Du /* cpuss.dw1_tr_in[21] */
} en_trig_output_1to1_scb_pdma1_tr_t;

/* Trigger Output Group 2 - PASS to PDMA0 direct connect (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_2_PASS_SAR_DONE_TO_PDMA0 = 0x40001200u /* cpuss.dw0_tr_in[28] */
} en_trig_output_1to1_sar_to_pdma0_t;

/* Trigger Output Group 3 -  (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_3_SMIF_TX_TO_PDMA1 = 0x40001300u, /* cpuss.dw1_tr_in[22] */
    TRIG_OUT_1TO1_3_SMIF_RX_TO_PDMA1 = 0x40001301u /* cpuss.dw1_tr_in[23] */
} en_trig_output_1to1_smif_to_pdma1_t;

/* Trigger Output Group 4 - I2S and PDM PDMA triggers (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_4_I2S_TX_TO_PDMA10 = 0x40001400u, /* cpuss.dw1_tr_in[24] */
    TRIG_OUT_1TO1_4_I2S_RX_TO_PDMA10 = 0x40001401u, /* cpuss.dw1_tr_in[25] */
    TRIG_OUT_1TO1_4_PDM_RX_TO_PDMA10 = 0x40001402u, /* cpuss.dw1_tr_in[26] */
    TRIG_OUT_1TO1_4_I2S_TX_TO_PDMA11 = 0x40001403u, /* cpuss.dw1_tr_in[27] */
    TRIG_OUT_1TO1_4_I2S_RX_TO_PDMA11 = 0x40001404u /* cpuss.dw1_tr_in[28] */
} en_trig_output_1to1_audioss_pdma1_tr_t;

/* Trigger Output Group 5 - USB PDMA0 Triggers (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA00 = 0x40001500u, /* cpuss.dw0_tr_in[8] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA01 = 0x40001501u, /* cpuss.dw0_tr_in[9] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA02 = 0x40001502u, /* cpuss.dw0_tr_in[10] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA03 = 0x40001503u, /* cpuss.dw0_tr_in[11] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA04 = 0x40001504u, /* cpuss.dw0_tr_in[12] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA05 = 0x40001505u, /* cpuss.dw0_tr_in[13] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA06 = 0x40001506u, /* cpuss.dw0_tr_in[14] */
    TRIG_OUT_1TO1_5_USB_DMA_TO_PDMA07 = 0x40001507u /* cpuss.dw0_tr_in[15] */
} en_trig_output_1to1_usb_pdma0_tr_t;

/* Trigger Output Group 6 - USB PDMA0 Acknowledge Triggers (OneToOne) */
typedef enum
{
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK0 = 0x40001600u, /* usb.dma_burstend[0] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK1 = 0x40001601u, /* usb.dma_burstend[1] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK2 = 0x40001602u, /* usb.dma_burstend[2] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK3 = 0x40001603u, /* usb.dma_burstend[3] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK4 = 0x40001604u, /* usb.dma_burstend[4] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK5 = 0x40001605u, /* usb.dma_burstend[5] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK6 = 0x40001606u, /* usb.dma_burstend[6] */
    TRIG_OUT_1TO1_6_PDMA0_TO_USB_ACK7 = 0x40001607u /* usb.dma_burstend[7] */
} en_trig_output_1to1_usb_pdma0_ack_tr_t;

/* Level or edge detection setting for a trigger mux */
typedef enum
{
    /* The trigger is a simple level output */
    TRIGGER_TYPE_LEVEL = 0u,
    /* The trigger is synchronized to the consumer blocks clock
       and a two cycle pulse is generated on this clock */
    TRIGGER_TYPE_EDGE = 1u
} en_trig_type_t;

/* Trigger Type Defines */
/* TCPWM Trigger Types */
#define TRIGGER_TYPE_TCPWM_LINE                 TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_TCPWM_LINE_COMPL           TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_TCPWM_TR_OVERFLOW          TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_TCPWM_TR_COMPARE_MATCH     TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_TCPWM_TR_UNDERFLOW         TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_TCPWM_TR_IN__LEVEL         TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_TCPWM_TR_IN__EDGE          TRIGGER_TYPE_EDGE
/* CSD Trigger Types */
#define TRIGGER_TYPE_CSD_DSI_SAMPLE_OUT         TRIGGER_TYPE_EDGE
/* SCB Trigger Types */
#define TRIGGER_TYPE_SCB_TR_I2C_SCL_FILTERED    TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_SCB_TR_TX_REQ              TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_SCB_TR_RX_REQ              TRIGGER_TYPE_LEVEL
/* PERI Trigger Types */
#define TRIGGER_TYPE_PERI_TR_IO_INPUT__LEVEL    TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_PERI_TR_IO_INPUT__EDGE     TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_PERI_TR_IO_OUTPUT__LEVEL   TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_PERI_TR_IO_OUTPUT__EDGE    TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_PERI_TR_DBG_FREEZE         TRIGGER_TYPE_LEVEL
/* CPUSS Trigger Types */
#define TRIGGER_TYPE_CPUSS_DW0_TR_IN__LEVEL     TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_CPUSS_DW0_TR_IN__EDGE      TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_DW1_TR_IN__LEVEL     TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_CPUSS_DW1_TR_IN__EDGE      TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_DMAC_TR_IN__LEVEL    TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_CPUSS_DMAC_TR_IN__EDGE     TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_DW0_TR_OUT           TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_DW1_TR_OUT           TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_DMAC_TR_OUT          TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_CTI_TR_OUT           TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_TR_FAULT             TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_CPUSS_CTI_TR_IN            TRIGGER_TYPE_EDGE
/* AUDIOSS Trigger Types */
#define TRIGGER_TYPE_AUDIOSS_TR_I2S_TX_REQ      TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_AUDIOSS_TR_I2S_RX_REQ      TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_AUDIOSS_TR_PDM_RX_REQ      TRIGGER_TYPE_LEVEL
/* LPCOMP Trigger Types */
#define TRIGGER_TYPE_LPCOMP_DSI_COMP0           TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_LPCOMP_DSI_COMP1           TRIGGER_TYPE_LEVEL
/* SMIF Trigger Types */
#define TRIGGER_TYPE_SMIF_TR_TX_REQ             TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_SMIF_TR_RX_REQ             TRIGGER_TYPE_LEVEL
/* USB Trigger Types */
#define TRIGGER_TYPE_USB_DMA_REQ                TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_USB_DMA_BURSTEND           TRIGGER_TYPE_EDGE
/* PASS Trigger Types */
#define TRIGGER_TYPE_PASS_TR_SAR_OUT            TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_PASS_TR_SAR_IN__LEVEL      TRIGGER_TYPE_LEVEL
#define TRIGGER_TYPE_PASS_TR_SAR_IN__EDGE       TRIGGER_TYPE_EDGE
/* PROFILE Trigger Types */
#define TRIGGER_TYPE_PROFILE_TR_START           TRIGGER_TYPE_EDGE
#define TRIGGER_TYPE_PROFILE_TR_STOP            TRIGGER_TYPE_EDGE

/* Monitor Signal Defines */
typedef enum
{
    PROFILE_ONE                     =  0,       /* profile.one */
    CPUSS_MONITOR_CM0               =  1,       /* cpuss.monitor_cm0 */
    CPUSS_MONITOR_CM4               =  2,       /* cpuss.monitor_cm4 */
    CPUSS_MONITOR_MAIN_FLASH        =  3,       /* cpuss.monitor_main_flash */
    CPUSS_MONITOR_WORK_FLASH        =  4,       /* cpuss.monitor_work_flash */
    CPUSS_MONITOR_DW0_AHB           =  5,       /* cpuss.monitor_dw0_ahb */
    CPUSS_MONITOR_DW1_AHB           =  6,       /* cpuss.monitor_dw1_ahb */
    CPUSS_MONITOR_DMAC_AHB          =  7,       /* cpuss.monitor_dmac_ahb */
    CPUSS_MONITOR_CRYPTO            =  8,       /* cpuss.monitor_crypto */
    USB_MONITOR_AHB                 =  9,       /* usb.monitor_ahb */
    SCB0_MONITOR_AHB                = 10,       /* scb[0].monitor_ahb */
    SCB1_MONITOR_AHB                = 11,       /* scb[1].monitor_ahb */
    SCB2_MONITOR_AHB                = 12,       /* scb[2].monitor_ahb */
    SCB3_MONITOR_AHB                = 13,       /* scb[3].monitor_ahb */
    SCB4_MONITOR_AHB                = 14,       /* scb[4].monitor_ahb */
    SCB5_MONITOR_AHB                = 15,       /* scb[5].monitor_ahb */
    SCB6_MONITOR_AHB                = 16,       /* scb[6].monitor_ahb */
    SCB7_MONITOR_AHB                = 17,       /* scb[7].monitor_ahb */
    SCB8_MONITOR_AHB                = 18,       /* scb[8].monitor_ahb */
    SCB9_MONITOR_AHB                = 19,       /* scb[9].monitor_ahb */
    SCB10_MONITOR_AHB               = 20,       /* scb[10].monitor_ahb */
    SCB11_MONITOR_AHB               = 21,       /* scb[11].monitor_ahb */
    SCB12_MONITOR_AHB               = 22,       /* scb[12].monitor_ahb */
    SMIF_MONITOR_SMIF_SPI_SELECT0   = 23,       /* smif.monitor_smif_spi_select[0] */
    SMIF_MONITOR_SMIF_SPI_SELECT1   = 24,       /* smif.monitor_smif_spi_select[1] */
    SMIF_MONITOR_SMIF_SPI_SELECT2   = 25,       /* smif.monitor_smif_spi_select[2] */
    SMIF_MONITOR_SMIF_SPI_SELECT3   = 26,       /* smif.monitor_smif_spi_select[3] */
    SMIF_MONITOR_SMIF_SPI_SELECT_ANY = 27,      /* smif.monitor_smif_spi_select_any */
    SDHC0_MONITOR_CORE_MASTER_WR    = 28,       /* sdhc[0].monitor_core_master_wr */
    SDHC0_MONITOR_CORE_MASTER_RD    = 29,       /* sdhc[0].monitor_core_master_rd */
    SDHC1_MONITOR_CORE_MASTER_WR    = 30,       /* sdhc[1].monitor_core_master_wr */
    SDHC1_MONITOR_CORE_MASTER_RD    = 31        /* sdhc[1].monitor_core_master_rd */
} en_ep_mon_sel_t;

/* Total count of Energy Profiler monitor signal connections */
#define EP_MONITOR_COUNT                32u

/* Bus masters */
typedef enum
{
    CPUSS_MS_ID_CM0                 =  0,
    CPUSS_MS_ID_CRYPTO              =  1,
    CPUSS_MS_ID_DW0                 =  2,
    CPUSS_MS_ID_DW1                 =  3,
    CPUSS_MS_ID_DMAC                =  4,
    CPUSS_MS_ID_SLOW0               =  5,
    CPUSS_MS_ID_SLOW1               =  6,
    CPUSS_MS_ID_CM4                 = 14,
    CPUSS_MS_ID_TC                  = 15
} en_prot_master_t;

/* Include IP definitions */
#include "ip/cyip_sflash.h"
#include "ip/cyip_peri_v2.h"
#include "ip/cyip_peri_ms_v2.h"
#include "ip/cyip_crypto_v2.h"
#include "ip/cyip_cpuss_v2.h"
#include "ip/cyip_fault_v2.h"
#include "ip/cyip_ipc_v2.h"
#include "ip/cyip_prot_v2.h"
#include "ip/cyip_flashc_v2.h"
#include "ip/cyip_srss.h"
#include "ip/cyip_backup.h"
#include "ip/cyip_dw_v2.h"
#include "ip/cyip_dmac_v2.h"
#include "ip/cyip_efuse.h"
#include "ip/cyip_efuse_data.h"
#include "ip/cyip_profile.h"
#include "ip/cyip_hsiom_v2.h"
#include "ip/cyip_gpio_v2.h"
#include "ip/cyip_smartio_v2.h"
#include "ip/cyip_lpcomp.h"
#include "ip/cyip_csd.h"
#include "ip/cyip_tcpwm.h"
#include "ip/cyip_lcd.h"
#include "ip/cyip_usbfs.h"
#include "ip/cyip_smif.h"
#include "ip/cyip_sdhc.h"
#include "ip/cyip_scb.h"
#include "ip/cyip_ctbm.h"
#include "ip/cyip_ctdac.h"
#include "ip/cyip_sar.h"
#include "ip/cyip_pass.h"
#include "ip/cyip_pdm.h"
#include "ip/cyip_i2s.h"

/* IP type definitions */
typedef SFLASH_V1_Type SFLASH_Type;
typedef PERI_GR_V2_Type PERI_GR_Type;
typedef PERI_TR_GR_V2_Type PERI_TR_GR_Type;
typedef PERI_TR_1TO1_GR_V2_Type PERI_TR_1TO1_GR_Type;
typedef PERI_V2_Type PERI_Type;
typedef PERI_MS_PPU_PR_V2_Type PERI_MS_PPU_PR_Type;
typedef PERI_MS_PPU_FX_V2_Type PERI_MS_PPU_FX_Type;
typedef PERI_MS_V2_Type PERI_MS_Type;
typedef CRYPTO_V2_Type CRYPTO_Type;
typedef CPUSS_V2_Type CPUSS_Type;
typedef FAULT_STRUCT_V2_Type FAULT_STRUCT_Type;
typedef FAULT_V2_Type FAULT_Type;
typedef IPC_STRUCT_V2_Type IPC_STRUCT_Type;
typedef IPC_INTR_STRUCT_V2_Type IPC_INTR_STRUCT_Type;
typedef IPC_V2_Type IPC_Type;
typedef PROT_SMPU_SMPU_STRUCT_V2_Type PROT_SMPU_SMPU_STRUCT_Type;
typedef PROT_SMPU_V2_Type PROT_SMPU_Type;
typedef PROT_MPU_MPU_STRUCT_V2_Type PROT_MPU_MPU_STRUCT_Type;
typedef PROT_MPU_V2_Type PROT_MPU_Type;
typedef PROT_V2_Type PROT_Type;
typedef FLASHC_FM_CTL_V2_Type FLASHC_FM_CTL_Type;
typedef FLASHC_V2_Type FLASHC_Type;
typedef MCWDT_STRUCT_V1_Type MCWDT_STRUCT_Type;
typedef SRSS_V1_Type SRSS_Type;
typedef BACKUP_V1_Type BACKUP_Type;
typedef DW_CH_STRUCT_V2_Type DW_CH_STRUCT_Type;
typedef DW_V2_Type DW_Type;
typedef DMAC_CH_V2_Type DMAC_CH_Type;
typedef DMAC_V2_Type DMAC_Type;
typedef EFUSE_V1_Type EFUSE_Type;
typedef PROFILE_CNT_STRUCT_V1_Type PROFILE_CNT_STRUCT_Type;
typedef PROFILE_V1_Type PROFILE_Type;
typedef HSIOM_PRT_V2_Type HSIOM_PRT_Type;
typedef HSIOM_V2_Type HSIOM_Type;
typedef GPIO_PRT_V2_Type GPIO_PRT_Type;
typedef GPIO_V2_Type GPIO_Type;
typedef SMARTIO_PRT_V2_Type SMARTIO_PRT_Type;
typedef SMARTIO_V2_Type SMARTIO_Type;
typedef LPCOMP_V1_Type LPCOMP_Type;
typedef CSD_V1_Type CSD_Type;
typedef TCPWM_CNT_V1_Type TCPWM_CNT_Type;
typedef TCPWM_V1_Type TCPWM_Type;
typedef LCD_V1_Type LCD_Type;
typedef USBFS_USBDEV_V1_Type USBFS_USBDEV_Type;
typedef USBFS_USBLPM_V1_Type USBFS_USBLPM_Type;
typedef USBFS_USBHOST_V1_Type USBFS_USBHOST_Type;
typedef USBFS_V1_Type USBFS_Type;
typedef SMIF_DEVICE_V1_Type SMIF_DEVICE_Type;
typedef SMIF_V1_Type SMIF_Type;
typedef SDHC_WRAP_V1_Type SDHC_WRAP_Type;
typedef SDHC_CORE_V1_Type SDHC_CORE_Type;
typedef SDHC_V1_Type SDHC_Type;
typedef CySCB_V1_Type CySCB_Type;
typedef CTBM_V1_Type CTBM_Type;
typedef CTDAC_V1_Type CTDAC_Type;
typedef SAR_V1_Type SAR_Type;
typedef PASS_AREF_V1_Type PASS_AREF_Type;
typedef PASS_V1_Type PASS_Type;
typedef PDM_V1_Type PDM_Type;
typedef I2S_V1_Type I2S_Type;

/* Parameter Defines */
/* Number of regulator modules instantiated within SRSS, start with estimate,
   update after CMR feedback */
#define SRSS_NUM_ACTREG_PWRMOD          2u
/* Number of shorting switches between vccd and vccact (target dynamic voltage
   drop < 10mV) */
#define SRSS_NUM_ACTIVE_SWITCH          3u
/* ULP linear regulator system is present */
#define SRSS_ULPLINREG_PRESENT          1u
/* HT linear regulator system is present */
#define SRSS_HTLINREG_PRESENT           0u
/* Low-current buck regulator present. Can be derived from S40S_SISOBUCKLC_PRESENT
   or SIMOBUCK_PRESENT. */
#define SRSS_BUCKCTL_PRESENT            1u
/* Low-current SISO buck core regulator is present. Only compatible with ULP
   linear regulator system (ULPLINREG_PRESENT==1). */
#define SRSS_S40S_SISOBUCKLC_PRESENT    1u
/* SIMO buck core regulator is present. Only compatible with ULP linear regulator
   system (ULPLINREG_PRESENT==1). */
#define SRSS_SIMOBUCK_PRESENT           0u
/* Precision ILO (PILO) is present */
#define SRSS_PILO_PRESENT               0u
/* External Crystal Oscillator is present (high frequency) */
#define SRSS_ECO_PRESENT                1u
/* System Buck-Boost is present */
#define SRSS_SYSBB_PRESENT              0u
/* Number of clock paths. Must be > 0 */
#define SRSS_NUM_CLKPATH                6u
/* Number of PLLs present. Must be <= NUM_CLKPATH */
#define SRSS_NUM_PLL                    2u
/* Number of HFCLK roots present. Must be > 0 */
#define SRSS_NUM_HFROOT                 6u
/* Number of PWR_HIB_DATA registers, should not be needed if BACKUP_PRESENT */
#define SRSS_NUM_HIBDATA                1u
/* Backup domain is present (includes RTC and WCO) */
#define SRSS_BACKUP_PRESENT             1u
/* Mask of HFCLK root clock supervisors (CSV). For each clock root i, bit[i] of
   mask indicates presence of a CSV. */
#define SRSS_MASK_HFCSV                 0u
/* Clock supervisor is present on WCO. Must be 0 if BACKUP_PRESENT==0. */
#define SRSS_WCOCSV_PRESENT             0u
/* Number of software watchdog timers. */
#define SRSS_NUM_MCWDT                  2u
/* Number of DSI inputs into clock muxes. This is used for logic optimization. */
#define SRSS_NUM_DSI                    0u
/* Alternate high-frequency clock is present. This is used for logic optimization. */
#define SRSS_ALTHF_PRESENT              0u
/* Alternate low-frequency clock is present. This is used for logic optimization. */
#define SRSS_ALTLF_PRESENT              0u
/* Use the hardened clkactfllmux block */
#define SRSS_USE_HARD_CLKACTFLLMUX      1u
/* Number of clock paths, including direct paths in hardened clkactfllmux block
   (Must be >= NUM_CLKPATH) */
#define SRSS_HARD_CLKPATH               6u
/* Number of clock paths with muxes in hardened clkactfllmux block (Must be >=
   NUM_PLL+1) */
#define SRSS_HARD_CLKPATHMUX            6u
/* Number of HFCLKS present in hardened clkactfllmux block (Must be >= NUM_HFROOT) */
#define SRSS_HARD_HFROOT                6u
/* ECO mux is present in hardened clkactfllmux block (Must be >= ECO_PRESENT) */
#define SRSS_HARD_ECOMUX_PRESENT        1u
/* ALTHF mux is present in hardened clkactfllmux block (Must be >= ALTHF_PRESENT) */
#define SRSS_HARD_ALTHFMUX_PRESENT      1u
/* Backup memory is present (only used when BACKUP_PRESENT==1) */
#define SRSS_BACKUP_BMEM_PRESENT        0u
/* Number of Backup registers to include (each is 32b). Only used when
   BACKUP_PRESENT==1. */
#define SRSS_BACKUP_NUM_BREG            16u
/* Number of AMUX splitter cells */
#define IOSS_HSIOM_AMUX_SPLIT_NR        8u
/* Number of HSIOM ports in device (same as GPIO.GPIO_PRT_NR) */
#define IOSS_HSIOM_HSIOM_PORT_NR        15u
/* Number of PWR/GND MONITOR CELLs in the device */
#define IOSS_HSIOM_MONITOR_NR           0u
/* Number of PWR/GND MONITOR CELLs in range 0..31 */
#define IOSS_HSIOM_MONITOR_NR_0_31      0u
/* Number of PWR/GND MONITOR CELLs in range 32..63 */
#define IOSS_HSIOM_MONITOR_NR_32_63     0u
/* Number of PWR/GND MONITOR CELLs in range 64..95 */
#define IOSS_HSIOM_MONITOR_NR_64_95     0u
/* Number of PWR/GND MONITOR CELLs in range 96..127 */
#define IOSS_HSIOM_MONITOR_NR_96_127    0u
/* Indicates the presence of alternate JTAG interface */
#define IOSS_HSIOM_ALTJTAG_PRESENT      0u
/* Number of GPIO ports in range 0..31 */
#define IOSS_GPIO_GPIO_PORT_NR_0_31     15u
/* Number of GPIO ports in range 32..63 */
#define IOSS_GPIO_GPIO_PORT_NR_32_63    0u
/* Number of GPIO ports in range 64..95 */
#define IOSS_GPIO_GPIO_PORT_NR_64_95    0u
/* Number of GPIO ports in range 96..127 */
#define IOSS_GPIO_GPIO_PORT_NR_96_127   0u
/* Number of ports in device */
#define IOSS_GPIO_GPIO_PORT_NR          15u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO6 0u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR0_GPIO_PRT_SLOW_IO7 0u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO6 0u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR1_GPIO_PRT_SLOW_IO7 0u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR2_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO6 0u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR3_GPIO_PRT_SLOW_IO7 0u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO4 0u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO5 0u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO6 0u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR4_GPIO_PRT_SLOW_IO7 0u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR5_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR6_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR7_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR8_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR9_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR10_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR11_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR12_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_GPIO 1u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO2 1u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO3 1u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO4 1u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO5 1u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO6 1u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR13_GPIO_PRT_SLOW_IO7 1u
/* Indicates port is either GPIO or SIO (i.e. all GPIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_GPIO 0u
/* Indicates port is an SIO port (i.e. both GPIO and SIO registers present) */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SIO 0u
/* Indicates port is a GPIO port including the "AUTO" input threshold */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_AUTOLVL 0u
/* Indicates that pin #0 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO0 1u
/* Indicates that pin #1 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO1 1u
/* Indicates that pin #2 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO2 0u
/* Indicates that pin #3 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO3 0u
/* Indicates that pin #4 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO4 0u
/* Indicates that pin #5 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO5 0u
/* Indicates that pin #6 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO6 0u
/* Indicates that pin #7 exists for this port with slew control feature */
#define IOSS_GPIO_GPIO_PORT_NR14_GPIO_PRT_SLOW_IO7 0u
/* Mask of SMARTIO instances presence */
#define IOSS_SMARTIO_SMARTIO_MASK       768u
/* The number of protection contexts ([2, 16]). */
#define PERI_PC_NR                      8u
/* Master interface presence mask (4 bits) */
#define PERI_MS_PRESENT                 15u
/* Protection structures SRAM ECC present or not ('0': no, '1': yes) */
#define PERI_ECC_PRESENT                0u
/* Protection structures SRAM address ECC present or not ('0': no, '1': yes) */
#define PERI_ECC_ADDR_PRESENT           0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_CLOCK_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL1_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT0_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_CLOCK_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT1_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_CLOCK_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL1_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL2_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL3_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL4_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL6_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL7_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL8_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL9_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL10_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL12_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL13_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT2_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL1_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL2_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL5_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL6_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL8_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL9_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL11_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT3_PERI_GROUP_STRUCT_SL15_PRESENT 1u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL2_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL6_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL7_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT4_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT5_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL1_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL2_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL3_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL4_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL5_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL6_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL7_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL8_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL9_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL10_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL11_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL12_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT6_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT7_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT8_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL0_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT9_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL1_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL2_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL3_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT10_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT11_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT12_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT13_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT14_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Clock control functionality present ('0': no, '1': yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_CLOCK_PRESENT 1u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL0_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL1_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL2_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL3_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL4_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL5_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL6_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL7_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL8_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL9_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL10_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL11_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL12_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL13_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL14_PRESENT 0u
/* Slave present (0:No, 1:Yes) */
#define PERI_GROUP_PRESENT15_PERI_GROUP_STRUCT_SL15_PRESENT 0u
/* Number of programmable clocks (outputs) */
#define PERI_CLOCK_NR                   54u
/* Number of 8.0 dividers */
#define PERI_DIV_8_NR                   8u
/* Number of 16.0 dividers */
#define PERI_DIV_16_NR                  16u
/* Number of 16.5 (fractional) dividers */
#define PERI_DIV_16_5_NR                4u
/* Number of 24.5 (fractional) dividers */
#define PERI_DIV_24_5_NR                1u
/* Divider number width: max(1,roundup(log2(max(DIV_*_NR))) */
#define PERI_DIV_ADDR_WIDTH             4u
/* Timeout functionality present ('0': no, '1': yes) */
#define PERI_TIMEOUT_PRESENT            1u
/* Trigger module present (0=No, 1=Yes) */
#define PERI_TR                         1u
/* Number of trigger groups */
#define PERI_TR_GROUP_NR                10u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR0_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR1_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR2_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR3_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR4_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR5_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR6_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR7_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR8_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_GROUP_NR9_TR_GROUP_TR_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR0_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR1_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR2_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR3_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR4_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR5_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Trigger 1-to-1 group trigger manipulation logic present ('0': no, '1': yes) */
#define PERI_TR_1TO1_GROUP_NR6_TR_1TO1_GROUP_TR_1TO1_MANIPULATION_PRESENT 1u
/* Number of AHB-Lite "hmaster[]" bits ([1, 8]). */
#define PERI_MASTER_WIDTH               8u
/* UDB present or not ('0': no, '1': yes) */
#define CPUSS_UDB_PRESENT               0u
/* MBIST MMIO for Synopsys MBIST ('0': no, '1': yes). Set this to '1' only for the
   chips which doesn't use mxdft. */
#define CPUSS_MBIST_MMIO_PRESENT        1u
/* System RAM 0 size in kilobytes */
#define CPUSS_SRAM0_SIZE                512u
/* Number of macros used to implement System RAM 0. Example: 8 if 256 KB System
   SRAM0 is implemented with 8 32KB macros. */
#define CPUSS_RAMC0_MACRO_NR            16u
/* System RAM 1 present or not (0=No, 1=Yes) */
#define CPUSS_RAMC1_PRESENT             1u
/* System RAM 1 size in kilobytes */
#define CPUSS_SRAM1_SIZE                256u
/* Number of macros used to implement System RAM 1. Example: 8 if 256 KB System
   RAM 1 is implemented with 8 32KB macros. */
#define CPUSS_RAMC1_MACRO_NR            8u
/* System RAM 2 present or not (0=No, 1=Yes) */
#define CPUSS_RAMC2_PRESENT             1u
/* System RAM 2 size in kilobytes */
#define CPUSS_SRAM2_SIZE                256u
/* Number of macros used to implement System RAM 2. Example: 8 if 256 KB System
   RAM 2 is implemented with 8 32KB macros. */
#define CPUSS_RAMC2_MACRO_NR            8u
/* System SRAM(s) ECC present or not ('0': no, '1': yes) */
#define CPUSS_RAMC_ECC_PRESENT          0u
/* System SRAM(s) address ECC present or not ('0': no, '1': yes) */
#define CPUSS_RAMC_ECC_ADDR_PRESENT     0u
/* ECC present in either system RAM or interrupt handler (RAMC_ECC_PRESENT) */
#define CPUSS_ECC_PRESENT               0u
/* DataWire SRAMs ECC present or not ('0': no, '1': yes) */
#define CPUSS_DW_ECC_PRESENT            0u
/* DataWire SRAMs address ECC present or not ('0': no, '1': yes) */
#define CPUSS_DW_ECC_ADDR_PRESENT       0u
/* System ROM size in KB */
#define CPUSS_ROM_SIZE                  64u
/* Number of macros used to implement system ROM. Example: 4 if 512 KB system ROM
   is implemented with 4 128KB macros. */
#define CPUSS_ROMC_MACRO_NR             1u
/* Flash memory type ('0' : SONOS, '1': ECT) */
#define CPUSS_FLASHC_ECT                0u
/* Flash main region size in KB */
#define CPUSS_FLASH_SIZE                2048u
/* Flash work region size in KB (EEPROM emulation, data) */
#define CPUSS_WFLASH_SIZE               32u
/* Flash supervisory region size in KB */
#define CPUSS_SFLASH_SIZE               32u
/* Flash data output word size (in Bytes) */
#define CPUSS_FLASHC_MAIN_DATA_WIDTH    16u
/* SONOS Flash RWW present or not ('0': no, '1': yes) When RWW is '0', No special
   sectors present in Flash. Part of main sector 0 is allowcated for Supervisory
   Flash, and no Work Flash present. */
#define CPUSS_FLASHC_SONOS_RWW          1u
/* SONOS Flash, number of main sectors. */
#define CPUSS_FLASHC_SONOS_MAIN_SECTORS 8u
/* SONOS Flash, number of rows per main sector. */
#define CPUSS_FLASHC_SONOS_MAIN_ROWS    512u
/* SONOS Flash, number of words per row of main sector. */
#define CPUSS_FLASHC_SONOS_MAIN_WORDS   128u
/* SONOS Flash, number of special sectors. */
#define CPUSS_FLASHC_SONOS_SPL_SECTORS  2u
/* SONOS Flash, number of rows per special sector. */
#define CPUSS_FLASHC_SONOS_SPL_ROWS     64u
/* Flash memory ECC present or not ('0': no, '1': yes) */
#define CPUSS_FLASHC_FLASH_ECC_PRESENT  0u
/* Flash cache SRAM(s) ECC present or not ('0': no, '1': yes) */
#define CPUSS_FLASHC_RAM_ECC_PRESENT    0u
/* Number of external slaves directly connected to slow AHB-Lite infrastructure.
   Maximum nubmer of slave supported is 4. Width of this parameter is 4-bits.
   1-bit mask for each slave indicating present or not. Example: 4'b0011 - slave
   0 and slave 1 are present. Note: The SLOW_SLx_ADDR and SLOW_SLx_MASK
   parameters (for the slaves present) should be derived from the Memory Map. */
#define CPUSS_SLOW_SL_PRESENT           1u
/* Number of external slaves directly connected to fast AHB-Lite infrastructure.
   Maximum nubmer of slave supported is 4. Width of this parameter is 4-bits.
   1-bit mask for each slave indicating present or not. Example: 4'b0011 - slave
   0 and slave 1 are present. Note: The FAST_SLx_ADDR and FAST_SLx_MASK
   parameters (for the slaves present) should be derived from the Memory Map. */
#define CPUSS_FAST_SL_PRESENT           1u
/* Number of external masters driving the slow AHB-Lite infrastructure. Maximum
   number of masters supported is 2. Width of this parameter is 2-bits. 1-bit
   mask for each master indicating present or not. Example: 2'b01 - master 0 is
   present. */
#define CPUSS_SLOW_MS_PRESENT           3u
/* System interrupt functionality present or not ('0': no; '1': yes). Not used for
   CM0+ PCU, which always uses system interrupt functionality. */
#define CPUSS_SYSTEM_IRQ_PRESENT        0u
/* Number of total interrupt request inputs to CPUSS */
#define CPUSS_SYSTEM_INT_NR             168u
/* Number of DeepSleep wakeup interrupt inputs to CPUSS */
#define CPUSS_SYSTEM_DPSLP_INT_NR       39u
/* Width of the CM4 interrupt priority bits. Legal range [3,8] Example: 3 = 8
   levels of priority 8 = 256 levels of priority */
#define CPUSS_CM4_LVL_WIDTH             3u
/* CM4 Floating point unit present or not (0=No, 1=Yes) */
#define CPUSS_CM4_FPU_PRESENT           1u
/* Debug level. Legal range [0,3] (0= No support, 1= Minimum: CM0/4 both 2
   breakpoints +1 watchpoint, 2= Full debug: CM0/4 have 4/6 breakpoints, 2/4
   watchpoints and 0/2 literal compare, 3= Full debug + data matching) */
#define CPUSS_DEBUG_LVL                 3u
/* Trace level. Legal range [0,2] (0= No tracing, 1= ITM + TPIU + SWO, 2= ITM +
   ETM + TPIU + SWO) Note: CM4 HTM is not supported. Hence vaule 3 for trace
   level is not supported in CPUSS. */
#define CPUSS_TRACE_LVL                 2u
/* Embedded Trace Buffer present or not (0=No, 1=Yes) */
#define CPUSS_ETB_PRESENT               0u
/* CM0+ MTB SRAM buffer size in kilobytes. Legal vaules 4, 8 or 16 */
#define CPUSS_MTB_SRAM_SIZE             4u
/* CM4 ETB SRAM buffer size in kilobytes. Legal vaules 4, 8 or 16 */
#define CPUSS_ETB_SRAM_SIZE             8u
/* PTM interface present (0=No, 1=Yes) */
#define CPUSS_PTM_PRESENT               0u
/* Width of the PTM interface in bits ([2,32]) */
#define CPUSS_PTM_WIDTH                 1u
/* Width of the TPIU interface in bits ([1,4]) */
#define CPUSS_TPIU_WIDTH                4u
/* CoreSight Part Identification Number */
#define CPUSS_JEPID                     52u
/* CoreSight Part Identification Number */
#define CPUSS_JEPCONTINUATION           0u
/* CoreSight Part Identification Number */
#define CPUSS_FAMILYID                  258u
/* ROM trim register width (for ARM 3, for Synopsys 5) */
#define CPUSS_ROM_TRIM_WIDTH            5u
/* ROM trim register default (for both ARM and Synopsys 0x0000_0012) */
#define CPUSS_ROM_TRIM_DEFAULT          18u
/* RAM trim register width (for ARM 5, for Synopsys 15) */
#define CPUSS_RAM_TRIM_WIDTH            15u
/* RAM trim register default (for ARM 0x0000_0002 and for Synopsys 0x0000_6012) */
#define CPUSS_RAM_TRIM_DEFAULT          24594u
/* Cryptography IP present or not (0=No, 1=Yes) */
#define CPUSS_CRYPTO_PRESENT            1u
/* DataWire 0 present or not (0=No, 1=Yes) */
#define CPUSS_DW0_PRESENT               1u
/* Number of DataWire 0 channels (8, 16 or 32) */
#define CPUSS_DW0_CH_NR                 29u
/* DataWire 1 present or not (0=No, 1=Yes) */
#define CPUSS_DW1_PRESENT               1u
/* Number of DataWire 1 channels (8, 16 or 32) */
#define CPUSS_DW1_CH_NR                 29u
/* DMA controller present or not ('0': no, '1': yes) */
#define CPUSS_DMAC_PRESENT              1u
/* Number of DMA controller channels ([1, 8]) */
#define CPUSS_DMAC_CH_NR                4u
/* Number of Flash BIST_DATA registers */
#define CPUSS_FLASHC_FLASHC_BIST_DATA_NR 4u
/* Page size in # of 32-bit words (1: 4 bytes, 2: 8 bytes, ... */
#define CPUSS_FLASHC_PA_SIZE            128u
/* SONOS Flash is used or not ('0': no, '1': yes) */
#define CPUSS_FLASHC_FLASHC_IS_SONOS    1u
/* eCT Flash is used or not ('0': no, '1': yes) */
#define CPUSS_FLASHC_FLASHC_IS_ECT      0u
/* Cryptography SRAMs ECC present or not ('0': no, '1': yes) */
#define CPUSS_CRYPTO_ECC_PRESENT        0u
/* Cryptography SRAMs address ECC present or not ('0': no, '1': yes) */
#define CPUSS_CRYPTO_ECC_ADDR_PRESENT   0u
/* AES cipher support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_AES                1u
/* (Tripple) DES cipher support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_DES                1u
/* Chacha support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_CHACHA             1u
/* Pseudo random number generation support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_PR                 1u
/* SHA1 hash support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_SHA1               1u
/* SHA2 hash support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_SHA2               1u
/* SHA3 hash support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_SHA3               1u
/* Cyclic Redundancy Check support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_CRC                1u
/* True random number generation support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_TR                 1u
/* Vector unit support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_VU                 1u
/* Galios/Counter Mode (GCM) support ('0': no, '1': yes) */
#define CPUSS_CRYPTO_GCM                1u
/* Number of 32-bit words in the IP internal memory buffer (from the set [64, 128,
   256, 512, 1024, 2048, 4096], to allow for a 256 B, 512 B, 1 kB, 2 kB, 4 kB, 8
   kB and 16 kB memory buffer) */
#define CPUSS_CRYPTO_BUFF_SIZE          1024u
/* Number of fault structures. Legal range [1, 4] */
#define CPUSS_FAULT_FAULT_NR            2u
/* Number of IPC structures. Legal range [1, 16] */
#define CPUSS_IPC_IPC_NR                16u
/* Number of IPC interrupt structures. Legal range [1, 16] */
#define CPUSS_IPC_IPC_IRQ_NR            16u
/* Master 0 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS0_PC_NR_MINUS1 7u
/* Master 1 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS1_PC_NR_MINUS1 0u
/* Master 2 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS2_PC_NR_MINUS1 0u
/* Master 3 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS3_PC_NR_MINUS1 0u
/* Master 4 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS4_PC_NR_MINUS1 0u
/* Master 5 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS5_PC_NR_MINUS1 7u
/* Master 6 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS6_PC_NR_MINUS1 7u
/* Master 7 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS7_PC_NR_MINUS1 0u
/* Master 8 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS8_PC_NR_MINUS1 0u
/* Master 9 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS9_PC_NR_MINUS1 0u
/* Master 10 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS10_PC_NR_MINUS1 0u
/* Master 11 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS11_PC_NR_MINUS1 0u
/* Master 12 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS12_PC_NR_MINUS1 0u
/* Master 13 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS13_PC_NR_MINUS1 0u
/* Master 14 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS14_PC_NR_MINUS1 7u
/* Master 15 protect contexts minus one */
#define CPUSS_PROT_SMPU_MS15_PC_NR_MINUS1 7u
/* Number of SMPU protection structures */
#define CPUSS_PROT_SMPU_STRUCT_NR       16u
/* Number of protection contexts supported minus 1. Legal range [1,16] */
#define CPUSS_SMPU_STRUCT_PC_NR_MINUS1  7u
/* Number of DataWire controllers present (max 2) */
#define CPUSS_DW_NR                     2u
/* DataWire SRAMs ECC present or not ('0': no, '1': yes) */
#define CPUSS_DW_ECC_PRESENT            0u
/* Number of DataWire controllers present (max 2) (same as DW.NR above) */
#define CPUSS_CPUSS_DW_DW_NR            2u
/* Number of channels in each DataWire controller */
#define CPUSS_CPUSS_DW_DW_NR0_DW_CH_NR  29u
/* Width of a channel number in bits */
#define CPUSS_CPUSS_DW_DW_NR0_DW_CH_NR_WIDTH 5u
/* Number of channels in each DataWire controller */
#define CPUSS_CPUSS_DW_DW_NR1_DW_CH_NR  29u
/* Width of a channel number in bits */
#define CPUSS_CPUSS_DW_DW_NR1_DW_CH_NR_WIDTH 5u
/* Number of DMA controller channels ([1, 8]) */
#define CPUSS_DMAC_CH_NR                4u
/* See MMIO2 instantiation or not */
#define CPUSS_CHIP_TOP_PROFILER_PRESENT 1u
/* ETAS Calibration support pin out present (automotive only) */
#define CPUSS_CHIP_TOP_CAL_SUP_NZ_PRESENT 0u
/* Number of profiling counters. Legal range [1, 32] */
#define PROFILE_PRFL_CNT_NR             8u
/* Number of monitor event signals. Legal range [1, 128] */
#define PROFILE_PRFL_MONITOR_NR         128u
/* Number of instantiated eFUSE macros (256 bit macros). Legal range [1, 16] */
#define EFUSE_EFUSE_NR                  4u
/* SONOS Flash is used or not ('0': no, '1': yes) */
#define SFLASH_FLASHC_IS_SONOS          1u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB0_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB0_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB0_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB0_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB0_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB0_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB0_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB0_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB0_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB0_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB0_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB0_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB0_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB0_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB0_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB0_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB0_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB0_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB0_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB0_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB0_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB0_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB0_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB0_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB1_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB1_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB1_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB1_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB1_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB1_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB1_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB1_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB1_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB1_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB1_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB1_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB1_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB1_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB1_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB1_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB1_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB1_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB1_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB1_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB1_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB1_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB1_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB1_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB2_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB2_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB2_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB2_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB2_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB2_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB2_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB2_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB2_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB2_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB2_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB2_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB2_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB2_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB2_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB2_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB2_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB2_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB2_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB2_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB2_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB2_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB2_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB2_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB3_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB3_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB3_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB3_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB3_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB3_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB3_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB3_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB3_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB3_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB3_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB3_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB3_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB3_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB3_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB3_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB3_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB3_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB3_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB3_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB3_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB3_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB3_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB3_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB4_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB4_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB4_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB4_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB4_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB4_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB4_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB4_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB4_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB4_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB4_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB4_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB4_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB4_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB4_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB4_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB4_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB4_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB4_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB4_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB4_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB4_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB4_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB4_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB5_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB5_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB5_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB5_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB5_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB5_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB5_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB5_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB5_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB5_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB5_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB5_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB5_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB5_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB5_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB5_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB5_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB5_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB5_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB5_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB5_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB5_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB5_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB5_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB6_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB6_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB6_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB6_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB6_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB6_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB6_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB6_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB6_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB6_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB6_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB6_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB6_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB6_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB6_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB6_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB6_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB6_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB6_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB6_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB6_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB6_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB6_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB6_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB7_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB7_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB7_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB7_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB7_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB7_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB7_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB7_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB7_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB7_SPI_M                      1u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB7_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB7_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB7_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB7_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB7_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB7_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB7_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB7_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB7_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB7_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB7_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB7_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB7_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB7_CHIP_TOP_SPI_SEL_NR        3u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB8_DEEPSLEEP                  1u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB8_EC                         1u
/* I2C master support? ('0': no, '1': yes) */
#define SCB8_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB8_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB8_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB8_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB8_I2C_EC                     1u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB8_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB8_I2C_S_EC                   1u
/* SPI master support? ('0': no, '1': yes) */
#define SCB8_SPI_M                      0u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB8_SPI_S                      1u
/* SPI support? (SPI_M | SPI_S) */
#define SCB8_SPI                        1u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB8_SPI_EC                     1u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB8_SPI_S_EC                   1u
/* UART support? ('0': no, '1': yes) */
#define SCB8_UART                       0u
/* SPI or UART (SPI | UART) */
#define SCB8_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB8_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB8_CMD_RESP                   1u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB8_EZ                         1u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB8_EZ_CMD_RESP                1u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB8_I2C_S_EZ                   1u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB8_SPI_S_EZ                   1u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB8_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB8_CHIP_TOP_SPI_SEL_NR        1u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB9_DEEPSLEEP                  0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB9_EC                         0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB9_I2C_M                      1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB9_I2C_S                      1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB9_I2C                        1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB9_I2C_GLITCH                 1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB9_I2C_EC                     0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB9_I2C_M_S                    1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB9_I2C_S_EC                   0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB9_SPI_M                      0u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB9_SPI_S                      0u
/* SPI support? (SPI_M | SPI_S) */
#define SCB9_SPI                        0u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB9_SPI_EC                     0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB9_SPI_S_EC                   0u
/* UART support? ('0': no, '1': yes) */
#define SCB9_UART                       1u
/* SPI or UART (SPI | UART) */
#define SCB9_SPI_UART                   1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB9_EZ_DATA_NR                 256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB9_CMD_RESP                   0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB9_EZ                         0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB9_EZ_CMD_RESP                0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB9_I2C_S_EZ                   0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB9_SPI_S_EZ                   0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB9_I2C_FAST_PLUS              1u
/* Number of used spi_select signals (max 4) */
#define SCB9_CHIP_TOP_SPI_SEL_NR        0u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB10_DEEPSLEEP                 0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB10_EC                        0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB10_I2C_M                     1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB10_I2C_S                     1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB10_I2C                       1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB10_I2C_GLITCH                1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB10_I2C_EC                    0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB10_I2C_M_S                   1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB10_I2C_S_EC                  0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB10_SPI_M                     0u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB10_SPI_S                     0u
/* SPI support? (SPI_M | SPI_S) */
#define SCB10_SPI                       0u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB10_SPI_EC                    0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB10_SPI_S_EC                  0u
/* UART support? ('0': no, '1': yes) */
#define SCB10_UART                      1u
/* SPI or UART (SPI | UART) */
#define SCB10_SPI_UART                  1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB10_EZ_DATA_NR                256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB10_CMD_RESP                  0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB10_EZ                        0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB10_EZ_CMD_RESP               0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB10_I2C_S_EZ                  0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB10_SPI_S_EZ                  0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB10_I2C_FAST_PLUS             1u
/* Number of used spi_select signals (max 4) */
#define SCB10_CHIP_TOP_SPI_SEL_NR       0u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB11_DEEPSLEEP                 0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB11_EC                        0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB11_I2C_M                     1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB11_I2C_S                     1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB11_I2C                       1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB11_I2C_GLITCH                1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB11_I2C_EC                    0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB11_I2C_M_S                   1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB11_I2C_S_EC                  0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB11_SPI_M                     0u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB11_SPI_S                     0u
/* SPI support? (SPI_M | SPI_S) */
#define SCB11_SPI                       0u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB11_SPI_EC                    0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB11_SPI_S_EC                  0u
/* UART support? ('0': no, '1': yes) */
#define SCB11_UART                      1u
/* SPI or UART (SPI | UART) */
#define SCB11_SPI_UART                  1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB11_EZ_DATA_NR                256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB11_CMD_RESP                  0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB11_EZ                        0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB11_EZ_CMD_RESP               0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB11_I2C_S_EZ                  0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB11_SPI_S_EZ                  0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB11_I2C_FAST_PLUS             1u
/* Number of used spi_select signals (max 4) */
#define SCB11_CHIP_TOP_SPI_SEL_NR       0u
/* DeepSleep support ('0':no, '1': yes) */
#define SCB12_DEEPSLEEP                 0u
/* Externally clocked support? ('0': no, '1': yes) */
#define SCB12_EC                        0u
/* I2C master support? ('0': no, '1': yes) */
#define SCB12_I2C_M                     1u
/* I2C slave support? ('0': no, '1': yes) */
#define SCB12_I2C_S                     1u
/* I2C support? (I2C_M | I2C_S) */
#define SCB12_I2C                       1u
/* I2C glitch filters present? ('0': no, '1': yes) */
#define SCB12_I2C_GLITCH                1u
/* I2C externally clocked support? ('0': no, '1': yes) */
#define SCB12_I2C_EC                    0u
/* I2C master and slave support? (I2C_M & I2C_S) */
#define SCB12_I2C_M_S                   1u
/* I2C slave with EC? (I2C_S & I2C_EC) */
#define SCB12_I2C_S_EC                  0u
/* SPI master support? ('0': no, '1': yes) */
#define SCB12_SPI_M                     0u
/* SPI slave support? ('0': no, '1': yes) */
#define SCB12_SPI_S                     0u
/* SPI support? (SPI_M | SPI_S) */
#define SCB12_SPI                       0u
/* SPI externally clocked support? ('0': no, '1': yes) */
#define SCB12_SPI_EC                    0u
/* SPI slave with EC? (SPI_S & SPI_EC) */
#define SCB12_SPI_S_EC                  0u
/* UART support? ('0': no, '1': yes) */
#define SCB12_UART                      1u
/* SPI or UART (SPI | UART) */
#define SCB12_SPI_UART                  1u
/* Number of EZ memory Bytes ([32, 256, 512]). This memory is used in EZ mode,
   CMD_RESP mode and FIFO mode. Note that in EZ mode, if EZ_DATA_NR is 512, only
   256 B are used. This is because the EZ mode uses 8-bit addresses. */
#define SCB12_EZ_DATA_NR                256u
/* Command/response mode support? ('0': no, '1': yes) */
#define SCB12_CMD_RESP                  0u
/* EZ mode support? ('0': no, '1': yes) */
#define SCB12_EZ                        0u
/* Command/response mode or EZ mode support? (CMD_RESP | EZ) */
#define SCB12_EZ_CMD_RESP               0u
/* I2C slave with EZ mode (I2C_S & EZ) */
#define SCB12_I2C_S_EZ                  0u
/* SPI slave with EZ mode (SPI_S & EZ) */
#define SCB12_SPI_S_EZ                  0u
/* Support I2C FM+/1Mbps speed ('0': no, '1': yes) */
#define SCB12_I2C_FAST_PLUS             1u
/* Number of used spi_select signals (max 4) */
#define SCB12_CHIP_TOP_SPI_SEL_NR       0u
/* Number of counters per IP (1..32) */
#define TCPWM0_CNT_NR                   8u
/* Counter width (in number of bits) */
#define TCPWM0_CNT_CNT_WIDTH            32u
/* Number of counters per IP (1..32) */
#define TCPWM1_CNT_NR                   24u
/* Counter width (in number of bits) */
#define TCPWM1_CNT_CNT_WIDTH            16u
/* Number of ports supoprting up to 4 COMs */
#define LCD_NUMPORTS                    8u
/* Number of ports supporting up to 8 COMs */
#define LCD_NUMPORTS8                   8u
/* Number of ports supporting up to 16 COMs */
#define LCD_NUMPORTS16                  0u
/* Max number of LCD commons supported */
#define LCD_CHIP_TOP_COM_NR             8u
/* Max number of LCD pins (total) supported */
#define LCD_CHIP_TOP_PIN_NR             62u
/* Number of IREF outputs from AREF */
#define PASS_NR_IREFS                   4u
/* Number of CTBs in the Subsystem */
#define PASS_NR_CTBS                    0u
/* Number of CTDACs in the Subsystem */
#define PASS_NR_CTDACS                  0u
/* CTB0 Exists */
#define PASS_CTB0_EXISTS                0u
/* CTB1 Exists */
#define PASS_CTB1_EXISTS                0u
/* CTB2 Exists */
#define PASS_CTB2_EXISTS                0u
/* CTB3 Exists */
#define PASS_CTB3_EXISTS                0u
/* CTDAC0 Exists */
#define PASS_CTDAC0_EXISTS              0u
/* CTDAC1 Exists */
#define PASS_CTDAC1_EXISTS              0u
/* CTDAC2 Exists */
#define PASS_CTDAC2_EXISTS              0u
/* CTDAC3 Exists */
#define PASS_CTDAC3_EXISTS              0u
/* Number of SAR channels */
#define PASS_SAR_SAR_CHANNELS           16u
/* Averaging logic present in SAR */
#define PASS_SAR_SAR_AVERAGE            1u
/* Range detect logic present in SAR */
#define PASS_SAR_SAR_RANGEDET           1u
/* Support for UAB sampling */
#define PASS_SAR_SAR_UAB                0u
#define PASS_CTBM_CTDAC_PRESENT         0u
/* Base address of the SMIF XIP memory region. This address must be a multiple of
   the SMIF XIP memory capacity. This address must be a multiple of 64 KB. This
   address must be in the [0x0000:0000, 0x1fff:ffff] memory region. The XIP
   memory region should NOT overlap with other memory regions. */
#define SMIF_SMIF_XIP_ADDR              402653184u
/* Capacity of the SMIF XIP memory region. The more significant bits of this
   parameter must be '1' and the lesser significant bits of this paramter must
   be '0'. E.g., 0xfff0:0000 specifies a 1 MB memory region. Legal values are
   {0xffff:0000, 0xfffe:0000, 0xfffc:0000, 0xfff8:0000, 0xfff0:0000,
   0xffe0:0000, ..., 0xe000:0000}. */
#define SMIF_SMIF_XIP_MASK              4160749568u
/* Cryptography (AES) support ('0' = no support, '1' = support) */
#define SMIF_CRYPTO                     1u
/* Number of external devices supported ([1,4]) */
#define SMIF_DEVICE_NR                  4u
/* External device write support. This is a 4-bit field. Each external device has
   a dedicated bit. E.g., if bit 2 is '1', external device 2 has write support. */
#define SMIF_DEVICE_WR_EN               15u
/* Number of AHB-Lite "hmaster[]" bits ([1, 8]). */
#define SMIF_MASTER_WIDTH               8u
/* Chip top connect all 8 data pins (0= connect 4 data pins, 1= connect 8 data
   pins) */
#define SMIF_CHIP_TOP_DATA8_PRESENT     1u
/* I2S capable? (0=No,1=Yes) */
#define AUDIOSS0_I2S                    1u
/* PDM capable? (0=No,1=Yes) */
#define AUDIOSS0_PDM                    1u
/* I2S capable? (0=No,1=Yes) */
#define AUDIOSS1_I2S                    1u
/* PDM capable? (0=No,1=Yes) */
#define AUDIOSS1_PDM                    0u
/* Basically the max packet size, which gets double buffered in RAM 0: 512B
   (implies 1KB of RAM space for data) 1: 1KB (implies 2KB of RAM space for
   data) */
#define SDHC0_MAX_BLK_SIZE              0u
/* 0: No Command Queuing Engine present 1: Command Queuing Engine present; this
   adds 288 bytes of space to the RAM for this purpose. */
#define SDHC0_CQE_PRESENT               0u
/* 0: no retention of any SDHC_CORE regs 1: retention of SDHC_CORE regs that have
   the Retention flag (Note, CTL.ENABLE is always retained irrespective of this
   parameter) */
#define SDHC0_RETENTION_PRESENT         1u
/* Basically the max packet size, which gets double buffered in RAM 0: 512B
   (implies 1KB of RAM space for data) 1: 1KB (implies 2KB of RAM space for
   data) */
#define SDHC0_CORE_MAX_BLK_SIZE         0u
/* 0: No Command Queuing Engine present 1: Command Queuing Engine present; this
   adds 288 bytes of space to the RAM for this purpose. */
#define SDHC0_CORE_CQE_PRESENT          0u
/* 0: no retention of any SDHC_CORE regs 1: retention of SDHC_CORE regs that have
   the Retention flag (Note, CTL.ENABLE is always retained irrespective of this
   parameter) */
#define SDHC0_CORE_RETENTION_PRESENT    1u
/* Chip top connect all 8 data pins (0= connect 4 data pins, 1= connect 8 data
   pins) */
#define SDHC0_CHIP_TOP_DATA8_PRESENT    0u
/* Chip top connect card_detect */
#define SDHC0_CHIP_TOP_CARD_DETECT_PRESENT 1u
/* Chip top connect card_mech_write_prot_in */
#define SDHC0_CHIP_TOP_CARD_WRITE_PROT_PRESENT 1u
/* Chip top connect led_ctrl_out and led_ctrl_out_en */
#define SDHC0_CHIP_TOP_LED_CTRL_PRESENT 0u
/* Chip top connect io_volt_sel_out and io_volt_sel_out_en */
#define SDHC0_CHIP_TOP_IO_VOLT_SEL_PRESENT 1u
/* Chip top connect io_drive_strength_out and io_drive_strength_out_en */
#define SDHC0_CHIP_TOP_IO_DRIVE_STRENGTH_PRESENT 0u
/* Chip top connect card_if_pwr_en_out and card_if_pwr_en_out_en */
#define SDHC0_CHIP_TOP_CARD_IF_PWR_EN_PRESENT 1u
/* Chip top connect card_emmc_reset_n_out and card_emmc_reset_n_out_en */
#define SDHC0_CHIP_TOP_CARD_EMMC_RESET_PRESENT 0u
/* Chip top connect interrupt_wakeup (not used for eMMC) */
#define SDHC0_CHIP_TOP_INTERRUPT_WAKEUP_PRESENT 1u
/* Basically the max packet size, which gets double buffered in RAM 0: 512B
   (implies 1KB of RAM space for data) 1: 1KB (implies 2KB of RAM space for
   data) */
#define SDHC1_MAX_BLK_SIZE              0u
/* 0: No Command Queuing Engine present 1: Command Queuing Engine present; this
   adds 288 bytes of space to the RAM for this purpose. */
#define SDHC1_CQE_PRESENT               0u
/* 0: no retention of any SDHC_CORE regs 1: retention of SDHC_CORE regs that have
   the Retention flag (Note, CTL.ENABLE is always retained irrespective of this
   parameter) */
#define SDHC1_RETENTION_PRESENT         1u
/* Basically the max packet size, which gets double buffered in RAM 0: 512B
   (implies 1KB of RAM space for data) 1: 1KB (implies 2KB of RAM space for
   data) */
#define SDHC1_CORE_MAX_BLK_SIZE         0u
/* 0: No Command Queuing Engine present 1: Command Queuing Engine present; this
   adds 288 bytes of space to the RAM for this purpose. */
#define SDHC1_CORE_CQE_PRESENT          0u
/* 0: no retention of any SDHC_CORE regs 1: retention of SDHC_CORE regs that have
   the Retention flag (Note, CTL.ENABLE is always retained irrespective of this
   parameter) */
#define SDHC1_CORE_RETENTION_PRESENT    1u
/* Chip top connect all 8 data pins (0= connect 4 data pins, 1= connect 8 data
   pins) */
#define SDHC1_CHIP_TOP_DATA8_PRESENT    1u
/* Chip top connect card_detect */
#define SDHC1_CHIP_TOP_CARD_DETECT_PRESENT 1u
/* Chip top connect card_mech_write_prot_in */
#define SDHC1_CHIP_TOP_CARD_WRITE_PROT_PRESENT 1u
/* Chip top connect led_ctrl_out and led_ctrl_out_en */
#define SDHC1_CHIP_TOP_LED_CTRL_PRESENT 1u
/* Chip top connect io_volt_sel_out and io_volt_sel_out_en */
#define SDHC1_CHIP_TOP_IO_VOLT_SEL_PRESENT 1u
/* Chip top connect io_drive_strength_out and io_drive_strength_out_en */
#define SDHC1_CHIP_TOP_IO_DRIVE_STRENGTH_PRESENT 0u
/* Chip top connect card_if_pwr_en_out and card_if_pwr_en_out_en */
#define SDHC1_CHIP_TOP_CARD_IF_PWR_EN_PRESENT 1u
/* Chip top connect card_emmc_reset_n_out and card_emmc_reset_n_out_en */
#define SDHC1_CHIP_TOP_CARD_EMMC_RESET_PRESENT 1u
/* Chip top connect interrupt_wakeup (not used for eMMC) */
#define SDHC1_CHIP_TOP_INTERRUPT_WAKEUP_PRESENT 1u

/* MMIO Targets Defines */
#define CY_MMIO_CRYPTO_GROUP_NR         1u
#define CY_MMIO_CRYPTO_SLAVE_NR         0u
#define CY_MMIO_CPUSS_GROUP_NR          2u
#define CY_MMIO_CPUSS_SLAVE_NR          0u
#define CY_MMIO_FAULT_GROUP_NR          2u
#define CY_MMIO_FAULT_SLAVE_NR          1u
#define CY_MMIO_IPC_GROUP_NR            2u
#define CY_MMIO_IPC_SLAVE_NR            2u
#define CY_MMIO_PROT_GROUP_NR           2u
#define CY_MMIO_PROT_SLAVE_NR           3u
#define CY_MMIO_FLASHC_GROUP_NR         2u
#define CY_MMIO_FLASHC_SLAVE_NR         4u
#define CY_MMIO_SRSS_GROUP_NR           2u
#define CY_MMIO_SRSS_SLAVE_NR           6u
#define CY_MMIO_BACKUP_GROUP_NR         2u
#define CY_MMIO_BACKUP_SLAVE_NR         7u
#define CY_MMIO_DW_GROUP_NR             2u
#define CY_MMIO_DW_SLAVE_NR             8u
#define CY_MMIO_DMAC_GROUP_NR           2u
#define CY_MMIO_DMAC_SLAVE_NR           10u
#define CY_MMIO_EFUSE_GROUP_NR          2u
#define CY_MMIO_EFUSE_SLAVE_NR          12u
#define CY_MMIO_PROFILE_GROUP_NR        2u
#define CY_MMIO_PROFILE_SLAVE_NR        13u
#define CY_MMIO_HSIOM_GROUP_NR          3u
#define CY_MMIO_HSIOM_SLAVE_NR          0u
#define CY_MMIO_GPIO_GROUP_NR           3u
#define CY_MMIO_GPIO_SLAVE_NR           1u
#define CY_MMIO_SMARTIO_GROUP_NR        3u
#define CY_MMIO_SMARTIO_SLAVE_NR        2u
#define CY_MMIO_LPCOMP_GROUP_NR         3u
#define CY_MMIO_LPCOMP_SLAVE_NR         5u
#define CY_MMIO_CSD0_GROUP_NR           3u
#define CY_MMIO_CSD0_SLAVE_NR           6u
#define CY_MMIO_TCPWM0_GROUP_NR         3u
#define CY_MMIO_TCPWM0_SLAVE_NR         8u
#define CY_MMIO_TCPWM1_GROUP_NR         3u
#define CY_MMIO_TCPWM1_SLAVE_NR         9u
#define CY_MMIO_LCD0_GROUP_NR           3u
#define CY_MMIO_LCD0_SLAVE_NR           11u
#define CY_MMIO_USBFS0_GROUP_NR         3u
#define CY_MMIO_USBFS0_SLAVE_NR         15u
#define CY_MMIO_SMIF0_GROUP_NR          4u
#define CY_MMIO_SMIF0_SLAVE_NR          2u
#define CY_MMIO_SDHC0_GROUP_NR          4u
#define CY_MMIO_SDHC0_SLAVE_NR          6u
#define CY_MMIO_SDHC1_GROUP_NR          4u
#define CY_MMIO_SDHC1_SLAVE_NR          7u
#define CY_MMIO_SCB0_GROUP_NR           6u
#define CY_MMIO_SCB0_SLAVE_NR           0u
#define CY_MMIO_SCB1_GROUP_NR           6u
#define CY_MMIO_SCB1_SLAVE_NR           1u
#define CY_MMIO_SCB2_GROUP_NR           6u
#define CY_MMIO_SCB2_SLAVE_NR           2u
#define CY_MMIO_SCB3_GROUP_NR           6u
#define CY_MMIO_SCB3_SLAVE_NR           3u
#define CY_MMIO_SCB4_GROUP_NR           6u
#define CY_MMIO_SCB4_SLAVE_NR           4u
#define CY_MMIO_SCB5_GROUP_NR           6u
#define CY_MMIO_SCB5_SLAVE_NR           5u
#define CY_MMIO_SCB6_GROUP_NR           6u
#define CY_MMIO_SCB6_SLAVE_NR           6u
#define CY_MMIO_SCB7_GROUP_NR           6u
#define CY_MMIO_SCB7_SLAVE_NR           7u
#define CY_MMIO_SCB8_GROUP_NR           6u
#define CY_MMIO_SCB8_SLAVE_NR           8u
#define CY_MMIO_SCB9_GROUP_NR           6u
#define CY_MMIO_SCB9_SLAVE_NR           9u
#define CY_MMIO_SCB10_GROUP_NR          6u
#define CY_MMIO_SCB10_SLAVE_NR          10u
#define CY_MMIO_SCB11_GROUP_NR          6u
#define CY_MMIO_SCB11_SLAVE_NR          11u
#define CY_MMIO_SCB12_GROUP_NR          6u
#define CY_MMIO_SCB12_SLAVE_NR          12u
#define CY_MMIO_PASS_GROUP_NR           9u
#define CY_MMIO_PASS_SLAVE_NR           0u
#define CY_MMIO_PDM0_GROUP_NR           10u
#define CY_MMIO_PDM0_SLAVE_NR           1u
#define CY_MMIO_I2S0_GROUP_NR           10u
#define CY_MMIO_I2S0_SLAVE_NR           2u
#define CY_MMIO_I2S1_GROUP_NR           10u
#define CY_MMIO_I2S1_SLAVE_NR           3u

#endif /* _PSOC6A2M_CONFIG_H_ */


/* [] END OF FILE */
