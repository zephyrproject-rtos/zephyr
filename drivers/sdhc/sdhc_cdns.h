/*
 * Copyright (C) 2023 Intel Corporation
 * Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H
#define ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H

#include <stdio.h>

#include <zephyr/sd/sd.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* SDMA System Address / Argument2 */
#define SDHC_CDNS_DMA_ADDRESS 0x00

/* SRS01 - Block Size / Block Count */
#define SDHC_CDNS_BLOCK_SIZE     0x04
#define SDHC_CDNS_BLOCK_SIZE_512 0X200U

#define SDHC_CDNS_BLOCK_SIZE_BCCT_POS    16
#define SDHC_CDNS_BLOCK_SIZE_BCCT_MASK   GENMASK(31, 16) /* Block Count for Current Transfer */
#define SDHC_CDNS_BLOCK_SIZE_SDMABB_POS  12
#define SDHC_CDNS_BLOCK_SIZE_SDMABB_MASK GENMASK(14, 12) /* SDMA Buffer Boundary             */
#define SDHC_CDNS_BLOCK_SIZE_TBS_POS     0
#define SDHC_CDNS_BLOCK_SIZE_TBS_MASK    GENMASK(11, 0) /* Transfer Block Size              */

/* Argument1 */
#define SDHC_CDNS_ARGUMENT 0x08

/* Command / Transfer Mode */
#define SDHC_CDNS_XFER_MODE 0x0C

#define SDHC_CDNS_XFER_MODE_CIDX_POS  24
#define SDHC_CDNS_XFER_MODE_CIDX_MASK GENMASK(29, 24) /* Command Index               */
#define SDHC_CDNS_XFER_MODE_CT_POS    22
#define SDHC_CDNS_XFER_MODE_CT_MASK   GENMASK(23, 22) /* Command Type                */
#define SDHC_CDNS_XFER_MODE_DPS       BIT(21)         /* Data Present Select         */
#define SDHC_CDNS_XFER_MODE_CICE      BIT(20)         /* Cmd Index Check Enable      */
#define SDHC_CDNS_XFER_MODE_CRCCE     BIT(19)         /* Cmd CRC Check Enable        */
#define SDHC_CDNS_XFER_MODE_SCF       BIT(18)         /* Sub Command Flag            */
#define SDHC_CDNS_XFER_MODE_RTS_POS   16
#define SDHC_CDNS_XFER_MODE_RTS_MASK  GENMASK(17, 16) /* Response Type Select        */
#define SDHC_CDNS_CMD_RESP_L48_MASK   BIT(17)
#define SDHC_CDNS_CMD_RESP_L136_MASK  BIT(16)
#define SDHC_CDNS_XFER_MODE_RID       BIT(8) /* Response Interrupt Disable  */
#define SDHC_CDNS_XFER_MODE_RECE      BIT(7) /* Response Error Check Enable */
#define SDHC_CDNS_XFER_MODE_RECT      BIT(6) /* Response Error Check Type   */
#define SDHC_CDNS_XFER_MODE_MSBS      BIT(5) /* Multi/Single Block Select   */
#define SDHC_CDNS_XFER_MODE_DTDS      BIT(4) /* Data Transfer Dir Select    */
#define SDHC_CDNS_XFER_MODE_ACE_POS   2
#define SDHC_CDNS_XFER_MODE_ACE_MASK  GENMASK(3, 2) /* Auto CMD Enable             */
#define SDHC_CDNS_XFER_MODE_CMD12_EN  BIT(2)
#define SDHC_CDNS_XFER_MODE_BCE       BIT(1) /* Block Count Enable          */
#define SDHC_CDNS_XFER_MODE_DMAE      BIT(0) /* DMA Enable                  */

/* Bit map for Transfer Mode */
#define SDHC_CDNS_RESP_NONE 0

#define SDHC_CDNS_RESP_R1B                                                                        \
	SDHC_CDNS_XFER_MODE_RTS_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

#define SDHC_CDNS_RESP_R1                                                                         \
	SDHC_CDNS_CMD_RESP_L48_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

#define SDHC_CDNS_RESP_R2 SDHC_CDNS_CMD_RESP_L136_MASK | SDHC_CDNS_XFER_MODE_CRCCE

#define SDHC_CDNS_RESP_R3 SDHC_CDNS_CMD_RESP_L48_MASK

#define SDHC_CDNS_RESP_R6                                                                         \
	SDHC_CDNS_XFER_MODE_RTS_MASK | SDHC_CDNS_XFER_MODE_CRCCE | SDHC_CDNS_XFER_MODE_CICE

/* Response 0..3 (read-only) */
#define SDHC_CDNS_RESPONSE0 0x10
#define SDHC_CDNS_RESPONSE1 0x14
#define SDHC_CDNS_RESPONSE2 0x18
#define SDHC_CDNS_RESPONSE3 0x1C

/* Bit map to update response type */
#define SDHC_CDNS_CRC_LEFT_SHIFT  0x8U
#define SDHC_CDNS_CRC_RIGHT_SHIFT 0x18U

/* Buffer Data Port */
#define SDHC_CDNS_BUFFER 0x20

#define SDHC_CDNS_BUFFER_BDP_POS  0
#define SDHC_CDNS_BUFFER_BDP_MASK GENMASK(31, 0)

/* Present State */
#define SDHC_CDNS_PRESENT_STATE 0x24

#define SDHC_CDNS_PRESENT_STATE_SCMDS       BIT(28) /* Sub Command Status         */
#define SDHC_CDNS_PRESENT_STATE_CNIBE       BIT(27) /* Cmd Not Issued by Err      */
#define SDHC_CDNS_PRESENT_STATE_LVSIRSLT    BIT(26) /* Host Reg Volt Switch Rslt  */
#define SDHC_CDNS_PRESENT_STATE_CMDSL       BIT(24) /* CMD Line Signal Level      */
#define SDHC_CDNS_PRESENT_STATE_DATSL1_POS  20
#define SDHC_CDNS_PRESENT_STATE_DATSL1_MASK GENMASK(23, 20) /* DAT[3:0] Line Signal Level */
#define SDHC_CDNS_PRESENT_STATE_DATSL1_D0   BIT(20)         /* DAT[0] Line Signal Level */
#define SDHC_CDNS_PRESENT_STATE_WPSL        BIT(19)         /* Write Protect Switch Level */
#define SDHC_CDNS_PRESENT_STATE_CDSL        BIT(18)         /* Card Detect Pin Level      */
#define SDHC_CDNS_PRESENT_STATE_CSS         BIT(17)         /* Card State Stable          */
#define SDHC_CDNS_PRESENT_STATE_CI          BIT(16)         /* Card Inserted              */
#define SDHC_CDNS_PRESENT_STATE_BRE         BIT(11)         /* Buffer Read Enable         */
#define SDHC_CDNS_PRESENT_STATE_BWE         BIT(10)         /* Buffer Write Enable        */
#define SDHC_CDNS_PRESENT_STATE_RTA         BIT(9)          /* Read Transfer Active       */
#define SDHC_CDNS_PRESENT_STATE_WTA         BIT(8)          /* Write Transfer Active      */
#define SDHC_CDNS_PRESENT_STATE_DATSL2_POS  4
#define SDHC_CDNS_PRESENT_STATE_DATSL2_MASK GENMASK(7, 4) /* DAT[7:4] Line Signal Level */
#define SDHC_CDNS_PRESENT_STATE_DLA         BIT(2)        /* Data Line Active           */
#define SDHC_CDNS_PRESENT_STATE_CIDAT       BIT(1)        /* Cmd Inhibit (DAT)          */
#define SDHC_CDNS_PRESENT_STATE_CICMD       BIT(0)        /* Cmd Inhibit (CMD)          */

/* Host Control 1 (General / Power / Block-Gap / Wake-Up) */
#define SDHC_CDNS_HOST_CTRL 0x28

#define SDHC_CDNS_HOST_CTRL_WORM        BIT(26) /* Wake-Up on Removal        */
#define SDHC_CDNS_HOST_CTRL_WOIS        BIT(25) /* Wake-Up on Insertion      */
#define SDHC_CDNS_HOST_CTRL_WOIQ        BIT(24) /* Wake-Up on Card Int       */
#define SDHC_CDNS_HOST_CTRL_IBG         BIT(19) /* Interrupt at Block Gap    */
#define SDHC_CDNS_HOST_CTRL_RWC         BIT(18) /* Read Wait Control         */
#define SDHC_CDNS_HOST_CTRL_CREQ        BIT(17) /* Continue Request          */
#define SDHC_CDNS_HOST_CTRL_SBGR        BIT(16) /* Stop at Block Gap Request */
#define SDHC_CDNS_HOST_CTRL_BVS_POS     9
#define SDHC_CDNS_HOST_CTRL_BVS_MASK    GENMASK(11, 9) /* Bus Voltage Select        */
#define SDHC_CDNS_HOST_CTRL_BP          BIT(8)         /* Bus Power                 */
#define SDHC_CDNS_HOST_CTRL_CDSS        BIT(7)         /* Card Detect Signal Select */
#define SDHC_CDNS_HOST_CTRL_CDTL        BIT(6)         /* Card Detect Test Level    */
#define SDHC_CDNS_HOST_CTRL_EDTW        BIT(5)         /* Extended Data Xfer Width  */
#define SDHC_CDNS_HOST_CTRL_DMASEL_POS  3
#define SDHC_CDNS_HOST_CTRL_DMASEL_MASK GENMASK(4, 3) /* DMA Select                */
#define SDHC_CDNS_HOST_CTRL_HSE         BIT(2)        /* High Speed Enable         */
#define SDHC_CDNS_HOST_CTRL_DTW         BIT(1)        /* Data Transfer Width       */
#define SDHC_CDNS_HOST_CTRL_LEDC        BIT(0)        /* LED Control               */

/* Bit map for Host Control 1 Bus Voltage Select */
#define SDHC_CDNS_HOST_VOL_3_3_V_SELECT (0x7U << SDHC_CDNS_HOST_CTRL_BVS_POS)
#define SDHC_CDNS_HOST_VOL_3_0_V_SELECT (0x6U << SDHC_CDNS_HOST_CTRL_BVS_POS)
#define SDHC_CDNS_HOST_VOL_1_8_V_SELECT (0x5U << SDHC_CDNS_HOST_CTRL_BVS_POS)

/* Bit map for Host Control 1 DMA Select */
#define SDHC_CDNS_HOST_CTRL_DMASEL_ADMA2_64  (0x3 << SDHC_CDNS_HOST_CTRL_DMASEL_POS)
#define SDHC_CDNS_HOST_CTRL_DMASEL_ADMA2_32  (0x2 << SDHC_CDNS_HOST_CTRL_DMASEL_POS)
#define SDHC_CDNS_HOST_CTRL_DMASEL_RESERVED  (0x1 << SDHC_CDNS_HOST_CTRL_DMASEL_POS)
#define SDHC_CDNS_HOST_CTRL_DMASEL_SDMA_MODE (0x0 << SDHC_CDNS_HOST_CTRL_DMASEL_POS)

/* Bit map for ADMA2 attribute */
#define SDHC_CDNS_ADMA2_DESC_TRAN  BIT(5)
#define SDHC_CDNS_ADMA2_DESC_END   BIT(1)
#define SDHC_CDNS_ADMA2_DESC_VALID BIT(0)

/* Clock Control / Timeout Control / Software Reset */
#define SDHC_CDNS_CLOCK_CTRL 0x2C

#define SDHC_CDNS_CLOCK_CTRL_SRDAT       BIT(26) /* Software Reset for DAT     */
#define SDHC_CDNS_CLOCK_CTRL_SRCMD       BIT(25) /* Software Reset for CMD     */
#define SDHC_CDNS_CLOCK_CTRL_SRFA        BIT(24) /* Software Reset for All     */
#define SDHC_CDNS_CLOCK_CTRL_DTCV_POS    16
#define SDHC_CDNS_CLOCK_CTRL_DTCV_MASK   GENMASK(19, 16) /* Data Timeout Counter Value */
#define SDHC_CDNS_CLOCK_CTRL_SDCFSL_POS  8
#define SDHC_CDNS_CLOCK_CTRL_SDCFSL_MASK GENMASK(15, 8) /* SDCLK Freq Select (low)    */
#define SDHC_CDNS_CLOCK_CTRL_SDCFSH_POS  6
#define SDHC_CDNS_CLOCK_CTRL_SDCFSH_MASK GENMASK(7, 6) /* SDCLK Freq Select (high)   */
#define SDHC_CDNS_CLOCK_CTRL_SDCE        BIT(2)        /* SD Clock Enable            */
#define SDHC_CDNS_CLOCK_CTRL_ICS         BIT(1)        /* Internal Clock Stable      */
#define SDHC_CDNS_CLOCK_CTRL_ICE         BIT(0)        /* Internal Clock Enable      */

#define SDHC_CDNS_CLOCK_CTRL_DTCV 0x0E

/* Error / Normal Interrupt Status */
#define SDHC_CDNS_INT_STATUS 0x30

#define SDHC_CDNS_INT_STATUS_ERSP   BIT(27) /* Error Response            */
#define SDHC_CDNS_INT_STATUS_EADMA  BIT(25) /* ADMA Error                */
#define SDHC_CDNS_INT_STATUS_EAC    BIT(24) /* Auto CMD Error            */
#define SDHC_CDNS_INT_STATUS_ECL    BIT(23) /* Current Limit Error       */
#define SDHC_CDNS_INT_STATUS_EDEB   BIT(22) /* Data End Bit Error        */
#define SDHC_CDNS_INT_STATUS_EDCRC  BIT(21) /* Data CRC Error            */
#define SDHC_CDNS_INT_STATUS_EDT    BIT(20) /* Data Timeout Error        */
#define SDHC_CDNS_INT_STATUS_ECI    BIT(19) /* Command Index Error       */
#define SDHC_CDNS_INT_STATUS_ECEB   BIT(18) /* Command End Bit Error     */
#define SDHC_CDNS_INT_STATUS_ECCRC  BIT(17) /* Command CRC Error         */
#define SDHC_CDNS_INT_STATUS_ECT    BIT(16) /* Command Timeout Error     */
#define SDHC_CDNS_INT_STATUS_EINT   BIT(15) /* Error Interrupt           */
#define SDHC_CDNS_INT_STATUS_CQINT  BIT(14) /* Command Queuing Interrupt */
#define SDHC_CDNS_INT_STATUS_FXE    BIT(13) /* Fixed Event               */
#define SDHC_CDNS_INT_STATUS_CINT   BIT(8)  /* Card Interrupt            */
#define SDHC_CDNS_INT_STATUS_CR     BIT(7)  /* Card Removal              */
#define SDHC_CDNS_INT_STATUS_CIN    BIT(6)  /* Card Insertion            */
#define SDHC_CDNS_INT_STATUS_BRR    BIT(5)  /* Buffer Read Ready         */
#define SDHC_CDNS_INT_STATUS_BWR    BIT(4)  /* Buffer Write Ready        */
#define SDHC_CDNS_INT_STATUS_DMAINT BIT(3)  /* DMA Interrupt             */
#define SDHC_CDNS_INT_STATUS_BGE    BIT(2)  /* Block Gap Event           */
#define SDHC_CDNS_INT_STATUS_TC     BIT(1)  /* Transfer Complete         */
#define SDHC_CDNS_INT_STATUS_CC     BIT(0)  /* Command Complete          */

/* Error / Normal Interrupt Status Enable */
#define SDHC_CDNS_INT_ENABLE 0x34

#define SDHC_CDNS_INT_ENABLE_ERSP_SE   BIT(27)
#define SDHC_CDNS_INT_ENABLE_EADMA_SE  BIT(25)
#define SDHC_CDNS_INT_ENABLE_EAC_SE    BIT(24)
#define SDHC_CDNS_INT_ENABLE_ECL_SE    BIT(23)
#define SDHC_CDNS_INT_ENABLE_EDEB_SE   BIT(22)
#define SDHC_CDNS_INT_ENABLE_EDCRC_SE  BIT(21)
#define SDHC_CDNS_INT_ENABLE_EDT_SE    BIT(20)
#define SDHC_CDNS_INT_ENABLE_ECI_SE    BIT(19)
#define SDHC_CDNS_INT_ENABLE_ECEB_SE   BIT(18)
#define SDHC_CDNS_INT_ENABLE_ECCRC_SE  BIT(17)
#define SDHC_CDNS_INT_ENABLE_ECT_SE    BIT(16)
#define SDHC_CDNS_INT_ENABLE_CQINT_SE  BIT(14)
#define SDHC_CDNS_INT_ENABLE_FXE_SE    BIT(13)
#define SDHC_CDNS_INT_ENABLE_CINT_SE   BIT(8)
#define SDHC_CDNS_INT_ENABLE_CR_SE     BIT(7)
#define SDHC_CDNS_INT_ENABLE_CIN_SE    BIT(6)
#define SDHC_CDNS_INT_ENABLE_BRR_SE    BIT(5)
#define SDHC_CDNS_INT_ENABLE_BWR_SE    BIT(4)
#define SDHC_CDNS_INT_ENABLE_DMAINT_SE BIT(3)
#define SDHC_CDNS_INT_ENABLE_BGE_SE    BIT(2)
#define SDHC_CDNS_INT_ENABLE_TC_SE     BIT(1)
#define SDHC_CDNS_INT_ENABLE_CC_SE     BIT(0)

/* Error / Normal Interrupt Signal Enable */
#define SDHC_CDNS_INT_SIGNAL_ENABLE 0x38

#define SDHC_CDNS_INT_SIGNAL_ENABLE_ERSP_IE   BIT(27)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_EADMA_IE  BIT(25)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_EAC_IE    BIT(24)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_ECL_IE    BIT(23)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_EDEB_IE   BIT(22)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_EDCRC_IE  BIT(21)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_EDT_IE    BIT(20)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_ECI_IE    BIT(19)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_ECEB_IE   BIT(18)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_ECCRC_IE  BIT(17)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_ECT_IE    BIT(16)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_CQINT_IE  BIT(14)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_FXE_IE    BIT(13)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_CINT_IE   BIT(8)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_CR_IE     BIT(7)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_CIN_IE    BIT(6)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_BRR_IE    BIT(5)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_BWR_IE    BIT(4)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_DMAINT_IE BIT(3)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_BGE_IE    BIT(2)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_TC_IE     BIT(1)
#define SDHC_CDNS_INT_SIGNAL_ENABLE_CC_IE     BIT(0)

/* SRS15 - Auto CMD Error Status / Host Control 2 */
#define SDHC_CDNS_AUTO_CMD_HOST_CTRL2 0x3C

#define SDHC_CDNS_HOST_CTRL2_PVE      BIT(31) /* Preset Value Enable              */
#define SDHC_CDNS_HOST_CTRL2_A64B     BIT(29) /* 64-bit Addressing                */
#define SDHC_CDNS_HOST_CTRL2_HV4E     BIT(28) /* Host Version 4 Enable            */
#define SDHC_CDNS_HOST_CTRL2_CMD23E   BIT(27) /* CMD23 Enable                     */
#define SDHC_CDNS_HOST_CTRL2_ADMA2LM  BIT(26) /* ADMA2 Length Mode                */
#define SDHC_CDNS_HOST_CTRL2_LVSIEXEC BIT(24) /* Voltage Switch Execute           */
#define SDHC_CDNS_HOST_CTRL2_SCS      BIT(23) /* Sample Clock Select              */
#define SDHC_CDNS_HOST_CTRL2_EXTNG    BIT(22) /* Execute Tuning                   */
#define SDHC_CDNS_HOST_CTRL2_DSS_POS  20
#define SDHC_CDNS_HOST_CTRL2_DSS_MASK GENMASK(21, 20) /* Driver Strength Select           */
#define SDHC_CDNS_HOST_CTRL2_V18SE    BIT(19)         /* 1.8V Signaling Enable            */
#define SDHC_CDNS_HOST_CTRL2_UMS_POS  16
#define SDHC_CDNS_HOST_CTRL2_UMS_MASK GENMASK(18, 16) /* UHS Mode Select                  */
#define SDHC_CDNS_AUTO_CMD_CNIACE     BIT(7)          /* Cmd Not Issued by Auto CMD12 Err */
#define SDHC_CDNS_AUTO_CMD_ACRE       BIT(5)          /* Auto CMD Response Error          */
#define SDHC_CDNS_AUTO_CMD_ACIE       BIT(4)          /* Auto CMD Index Error             */
#define SDHC_CDNS_AUTO_CMD_ACEBE      BIT(3)          /* Auto CMD End Bit Error           */
#define SDHC_CDNS_AUTO_CMD_ACCE       BIT(2)          /* Auto CMD CRC Error               */
#define SDHC_CDNS_AUTO_CMD_ACTE       BIT(1)          /* Auto CMD Timeout Error           */
#define SDHC_CDNS_AUTO_CMD_ACNE       BIT(0)          /* Auto CMD12 Not Executed          */

/* Bit map for Host Control 2 UHS Mode Select */
#define SDHC_CDNS_UHS_SPEED_MODE_DDR200 (0x5U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_DDR50  (0x4U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_SDR104 (0x3U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_SDR50  (0x2U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_SDR25  (0x1U << SDHC_CDNS_HOST_CTRL2_UMS_POS)
#define SDHC_CDNS_UHS_SPEED_MODE_SDR12  (0x0U << SDHC_CDNS_HOST_CTRL2_UMS_POS)

/* Capabilities */
#define SDHC_CDNS_CAPS1 0x40

#define SDHC_CDNS_CAPS1_SLT_POS      30
#define SDHC_CDNS_CAPS1_SLT_MASK     GENMASK(31, 30) /* Slot Type                 */
#define SDHC_CDNS_CAPS1_AIS          BIT(29)         /* Async Interrupt Support   */
#define SDHC_CDNS_CAPS1_A64SV3       BIT(28)         /* 64-bit Sys Addr (V3)      */
#define SDHC_CDNS_CAPS1_A64SV4       BIT(27)         /* 64-bit Sys Addr (V4)      */
#define SDHC_CDNS_CAPS1_VS18         BIT(26)         /* Voltage Support 1.8V      */
#define SDHC_CDNS_CAPS1_VS30         BIT(25)         /* Voltage Support 3.0V      */
#define SDHC_CDNS_CAPS1_VS33         BIT(24)         /* Voltage Support 3.3V      */
#define SDHC_CDNS_CAPS1_SRS          BIT(23)         /* Suspend/Resume Support    */
#define SDHC_CDNS_CAPS1_DMAS         BIT(22)         /* SDMA Support              */
#define SDHC_CDNS_CAPS1_HSS          BIT(21)         /* High Speed Support        */
#define SDHC_CDNS_CAPS1_ADMA1S       BIT(20)         /* ADMA1 Support             */
#define SDHC_CDNS_CAPS1_ADMA2S       BIT(19)         /* ADMA2 Support             */
#define SDHC_CDNS_CAPS1_EDS8         BIT(18)         /* 8-bit Embedded Support    */
#define SDHC_CDNS_CAPS1_MBL_POS      16
#define SDHC_CDNS_CAPS1_MBL_MASK     GENMASK(17, 16) /* Max Block Length          */
#define SDHC_CDNS_CAPS1_BCSDCLK_POS  8
#define SDHC_CDNS_CAPS1_BCSDCLK_MASK GENMASK(15, 8) /* Base Clock Freq for SDCLK */
#define SDHC_CDNS_CAPS1_TCU          BIT(7)         /* Timeout Clock Unit        */
#define SDHC_CDNS_CAPS1_TCF_POS      0
#define SDHC_CDNS_CAPS1_TCF_MASK     GENMASK(5, 0) /* Timeout Clock Frequency   */

/* Capabilities 2 */
#define SDHC_CDNS_CAPS2 0x44

#define SDHC_CDNS_CAPS2_LVSH         BIT(31) /* 1.8V Signaling Support */
#define SDHC_CDNS_CAPS2_VDD2S        BIT(28) /* VDD2 Support           */
#define SDHC_CDNS_CAPS2_ADMA3SUP     BIT(27) /* ADMA3 Support          */
#define SDHC_CDNS_CAPS2_CLKMPR_POS   16
#define SDHC_CDNS_CAPS2_CLKMPR_MASK  GENMASK(23, 16) /* Clock Multiplier       */
#define SDHC_CDNS_CAPS2_RTNGM_POS    14
#define SDHC_CDNS_CAPS2_RTNGM_MASK   GENMASK(15, 14) /* Retuning Modes         */
#define SDHC_CDNS_CAPS2_UTSM50       BIT(13)         /* Use Tuning for SDR50   */
#define SDHC_CDNS_CAPS2_RTNGCNT_POS  8
#define SDHC_CDNS_CAPS2_RTNGCNT_MASK GENMASK(11, 8) /* Retuning Count         */
#define SDHC_CDNS_CAPS2_DRVD         BIT(6)         /* Driver Type D Support  */
#define SDHC_CDNS_CAPS2_DRVC         BIT(5)         /* Driver Type C Support  */
#define SDHC_CDNS_CAPS2_DRVA         BIT(4)         /* Driver Type A Support  */
#define SDHC_CDNS_CAPS2_UHSII        BIT(3)         /* UHS-II Support         */
#define SDHC_CDNS_CAPS2_DDR50        BIT(2)         /* DDR50 Support          */
#define SDHC_CDNS_CAPS2_SDR104       BIT(1)         /* SDR104 Support         */
#define SDHC_CDNS_CAPS2_SDR50        BIT(0)         /* SDR50 Support          */

/* Max Current 1 Capabilities */
#define SDHC_CDNS_MAX_CURRENT1 0x48

#define SDHC_CDNS_MAX_CURRENT1_MC18_POS  16
#define SDHC_CDNS_MAX_CURRENT1_MC18_MASK GENMASK(23, 16) /* Max Current for 1.8V */
#define SDHC_CDNS_MAX_CURRENT1_MC30_POS  8
#define SDHC_CDNS_MAX_CURRENT1_MC30_MASK GENMASK(15, 8) /* Max Current for 3.0V */
#define SDHC_CDNS_MAX_CURRENT1_MC33_POS  0
#define SDHC_CDNS_MAX_CURRENT1_MC33_MASK GENMASK(7, 0) /* Max Current for 3.3V */

/* Max Current 2 Capabilities */
#define SDHC_CDNS_MAX_CURRENT2 0x4C

#define SDHC_CDNS_MAX_CURRENT2_MC18V2_POS  0
#define SDHC_CDNS_MAX_CURRENT2_MC18V2_MASK GENMASK(7, 0) /* Max Current for 1.8V VDD2 */

/* Force Event for Auto CMD / Error Status */
#define SDHC_CDNS_FORCE_EVENT 0x50

#define SDHC_CDNS_FORCE_EVENT_ERESP_FE  BIT(27)
#define SDHC_CDNS_FORCE_EVENT_ETUNE_FE  BIT(26)
#define SDHC_CDNS_FORCE_EVENT_EADMA_FE  BIT(25)
#define SDHC_CDNS_FORCE_EVENT_EAC_FE    BIT(24)
#define SDHC_CDNS_FORCE_EVENT_ECL_FE    BIT(23)
#define SDHC_CDNS_FORCE_EVENT_EDEB_FE   BIT(22)
#define SDHC_CDNS_FORCE_EVENT_EDCRC_FE  BIT(21)
#define SDHC_CDNS_FORCE_EVENT_EDT_FE    BIT(20)
#define SDHC_CDNS_FORCE_EVENT_ECI_FE    BIT(19)
#define SDHC_CDNS_FORCE_EVENT_ECEB_FE   BIT(18)
#define SDHC_CDNS_FORCE_EVENT_ECCRC_FE  BIT(17)
#define SDHC_CDNS_FORCE_EVENT_ECT_FE    BIT(16)
#define SDHC_CDNS_FORCE_EVENT_CNIACE_FE BIT(7)
#define SDHC_CDNS_FORCE_EVENT_ACIE_FE   BIT(4)
#define SDHC_CDNS_FORCE_EVENT_ACEBE_FE  BIT(3)
#define SDHC_CDNS_FORCE_EVENT_ACCE_FE   BIT(2)
#define SDHC_CDNS_FORCE_EVENT_ACTE_FE   BIT(1)
#define SDHC_CDNS_FORCE_EVENT_ACNE_FE   BIT(0)

/* ADMA Error Status              */
#define SDHC_CDNS_ADMA_ERROR 0x54

#define SDHC_CDNS_ADMA_ERROR_EADMAL      BIT(2) /* ADMA Length Mismatch Error */
#define SDHC_CDNS_ADMA_ERROR_EADMAS_POS  0
#define SDHC_CDNS_ADMA_ERROR_EADMAS_MASK GENMASK(1, 0) /* ADMA Error State           */

/* ADMA System Address      */
#define SDHC_CDNS_ADMA_SYS_ADDR1 0x58
#define SDHC_CDNS_ADMA_SYS_ADDR2 0x5C

#define SDHC_CDNS_PRESET_VALUE0 0x60 /* Initial / Default Speed  */
#define SDHC_CDNS_PRESET_VALUE1 0x64 /* High Speed / SDR12       */
#define SDHC_CDNS_PRESET_VALUE2 0x68 /* SDR25 / SDR50            */
#define SDHC_CDNS_PRESET_VALUE3 0x6C /* SDR104 / DDR50           */

#define SDHC_CDNS_PRESET_DSSPV_LO_POS    14
#define SDHC_CDNS_PRESET_DSSPV_LO_MASK   GENMASK(15, 14) /* Driver Strength Select (low preset)  */
#define SDHC_CDNS_PRESET_SDCFSPV_LO_POS  0
#define SDHC_CDNS_PRESET_SDCFSPV_LO_MASK GENMASK(9, 0) /* SDCLK Frequency Select (low preset)  */
#define SDHC_CDNS_PRESET_DSSPV_HI_POS    30
#define SDHC_CDNS_PRESET_DSSPV_HI_MASK   GENMASK(31, 30) /* Driver Strength Select (high preset) */
#define SDHC_CDNS_PRESET_SDCFSPV_HI_POS  16
#define SDHC_CDNS_PRESET_SDCFSPV_HI_MASK GENMASK(25, 16) /* SDCLK Frequency Select (high preset)   \
							  */
#define SDHC_CDNS_PRESET_CGSPV_HI        BIT(26)         /* Clock Generator Select (high preset) */

/* ADMA3 Integrated Descriptor Address */
#define SDHC_CDNS_ADMA3_ID_ADDR1      0x78
#define SDHC_CDNS_ADMA3_ID_ADDR1_POS  0
#define SDHC_CDNS_ADMA3_ID_ADDR1_MASK GENMASK(31, 0)

#define SDHC_CDNS_ADMA3_ID_ADDR2      0x7C
#define SDHC_CDNS_ADMA3_ID_ADDR2_POS  0
#define SDHC_CDNS_ADMA3_ID_ADDR2_MASK GENMASK(31, 0)

/*
 * Bitmap for Interrupt Masking
 */
#define SDHC_CDNS_NORMAL_INT_MASK                                                                  \
	(SDHC_CDNS_INT_ENABLE_CINT_SE | SDHC_CDNS_INT_ENABLE_CR_SE | SDHC_CDNS_INT_ENABLE_CIN_SE | \
	 SDHC_CDNS_INT_ENABLE_BWR_SE | SDHC_CDNS_INT_ENABLE_BRR_SE |                               \
	 SDHC_CDNS_INT_ENABLE_DMAINT_SE | SDHC_CDNS_INT_ENABLE_BGE_SE |                            \
	 SDHC_CDNS_INT_ENABLE_TC_SE | SDHC_CDNS_INT_ENABLE_CC_SE)

#define SDHC_CDNS_ERR_INT_MASK                                                                     \
	(SDHC_CDNS_INT_ENABLE_ERSP_SE | SDHC_CDNS_INT_ENABLE_EADMA_SE |                            \
	 SDHC_CDNS_INT_ENABLE_EAC_SE | SDHC_CDNS_INT_ENABLE_ECL_SE |                               \
	 SDHC_CDNS_INT_ENABLE_EDEB_SE | SDHC_CDNS_INT_ENABLE_EDCRC_SE |                            \
	 SDHC_CDNS_INT_ENABLE_EDT_SE | SDHC_CDNS_INT_ENABLE_ECI_SE |                               \
	 SDHC_CDNS_INT_ENABLE_ECEB_SE | SDHC_CDNS_INT_ENABLE_ECCRC_SE |                            \
	 SDHC_CDNS_INT_ENABLE_ECT_SE)

#define SDHC_CDNS_TXFR_INTR_EN_MASK                                                                \
	(SDHC_CDNS_INT_STATUS_CC | SDHC_CDNS_INT_STATUS_TC | SDHC_CDNS_INT_STATUS_BRR |            \
	 SDHC_CDNS_ERR_INT_MASK)

#define SDHC_CDNS_TUNING_CMD_BLKCOUNT	0x1U
#define SDHC_CDNS_TUNING_CMD_BLKSIZE	0x40U
#define SDHC_CDNS_MAX_TUNING_COUNT	0x28U

#define SDHC_CDNS_SD_SLOT   0x0U
#define SDHC_CDNS_EMMC_SLOT 0X1U

/* Host Version 4 requires an 8-byte aligned ADMA2 descriptor table */
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 8)
#define SDHC_CDNS_DESC_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define SDHC_CDNS_DESC_ALIGN 8
#endif

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(dev)  ((const struct sdhc_cdns_config *)(dev)->config)
#define DEV_DATA(dev) ((struct sdhc_cdns_data *)(dev)->data)

/**
 * @brief ADMA2 descriptor table structure (Host Version 4).
 *
 * 128-bit descriptor with 64-bit addressing, 64-bit descriptor with 32-bit
 * addressing.
 */
#ifdef CONFIG_64BIT
typedef struct {
	/**< attrs of descriptor */
	uint16_t attr;
	/**< Length of current dma transfer max 64kb */
	uint16_t len;
	/**< source/destination addr for current dma transfer */
	uint64_t addr;
	/**< reserved, must be zero */
	uint32_t reserved;
} __packed adma2_descriptor;
#else
typedef struct {
	/**< attrs of descriptor */
	uint16_t attr;
	/**< Length of current dma transfer max 64kb */
	uint16_t len;
	/**< source/destination addr for current dma transfer */
	uint32_t addr;
} __packed adma2_descriptor;
#endif

/**
 * @brief Holds device private data.
 */
struct sdhc_cdns_data {
	/* MMIO mapping information for SDHC slot registers set */
	DEVICE_MMIO_NAMED_RAM(slot);
	/* MMIO mapping information for SDHC host registers set */
	DEVICE_MMIO_NAMED_RAM(host);
	/**< Current I/O settings of SDHC */
	struct sdhc_io host_io;
	/**< Supported properties of SDHC */
	struct sdhc_host_props props;
	/**< SDHC IRQ events */
	struct k_event irq_event;
	/**< transfer mode and data direction (lower 16 bits of SDHC_CDNS_XFER_MODE) */
	uint16_t transfermode;
	/**< SD host controller timing mode */
	uint32_t timing_mode;
	/**< Bounce buffer for small DMA reads */
	uint8_t read_bounce_buffer[CONFIG_SDHC_BUFFER_ALIGNMENT]
		__aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	/**< ADMA descriptor table */
	adma2_descriptor adma2_desc[MAX(1, CONFIG_SDHC_CDNS_DESC_SIZE)]
		__aligned(SDHC_CDNS_DESC_ALIGN);
};

/**
 * @brief Holds SDHC configuration data.
 */
struct sdhc_cdns_config {
	/* MMIO mapping information for SDHC slot registers set */
	DEVICE_MMIO_NAMED_ROM(slot);
	/* MMIO mapping information for SDHC host registers set */
	DEVICE_MMIO_NAMED_ROM(host);
	/**< Pointer to the device structure representing the clock bus */
	const struct device *clock_dev;
	/**< device structure representing the clock ID */
	clock_control_subsys_t clock_id;
	/**< Callback to the device interrupt configuration api */
	void (*irq_config_fn)(const struct device *dev);
	/**< Card detection pin available or not */
	bool broken_cd;
	/**< delay given to card to power up or down fully */
	uint16_t powerdelay;
	/**< 1.8V signaling support */
	bool mmc_hs200_1_8v;
	/**< Indicates if driver  has a separate "host" MMIO region. */
	bool has_host;
	/* Cadence specific quirks */
	const struct sdhc_cdns_quirks *const quirks;
	/* Cadence specific quirks data */
	void *quirk_data;
	/* Cadence specific quirks config */
	const void *quirk_config;
};

/* Vendor quirks per driver instance */
struct sdhc_cdns_quirks {
	/* One-time PHY bring-up at device init, before any mode is selected. */
	int (*phy_init)(const struct device *dev);
	/* Apply PHY/HRS tuning for a negotiated SDHC_TIMING_* mode. */
	int (*phy_config)(const struct device *dev);
};

/* clang-format off */
#define SDHC_CDNS_QUIRK_CONFIG(dev)                                                                \
	(((const struct sdhc_cdns_config *)(dev)->config)->quirk_config)

#define SDHC_CDNS_QUIRK_DATA(dev)                                                                  \
	(((const struct sdhc_cdns_config *)(dev)->config)->quirk_data)

#if DT_HAS_COMPAT_STATUS_OKAY(cdns_sd6hc)
#include "sdhc_cdns6.h"
#endif

#define SDHC_CDNS_HAS_QUIRK(n)                                                                     \
	DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1)

#define SDHC_CDNS_QUIRK_GET(n) COND_CODE_1(SDHC_CDNS_HAS_QUIRK(n),                                 \
			(&sdhc_cdns_quirks_##n),                                                   \
			(NULL))

#define SDHC_CDNS_QUIRK_DATA_GET(n)                                                                \
	COND_CODE_1(SDHC_CDNS_HAS_QUIRK(n),                                                        \
			(&sdhc_cdns_quirk_data_##n),                                               \
			(NULL))

#define SDHC_CDNS_QUIRK_CONFIG_GET(n)                                                              \
	COND_CODE_1(SDHC_CDNS_HAS_QUIRK(n),                                                        \
			(&sdhc_cdns_quirk_config_##n),                                             \
			(NULL))

#define CDNS_QUIRK_FUNC_DEFINE(fname)                                                              \
	static inline int sdhc_cdns_quirk_##fname(const struct device *dev)                        \
	{                                                                                          \
		const struct sdhc_cdns_config *const config = dev->config;                         \
		const struct sdhc_cdns_quirks *const quirks = config->quirks;                      \
	                                                                                           \
		if (quirks != NULL && quirks->fname != NULL) {                                     \
			return config->quirks->fname(dev);                                         \
		}                                                                                  \
		return 0;                                                                          \
	}

CDNS_QUIRK_FUNC_DEFINE(phy_init)
CDNS_QUIRK_FUNC_DEFINE(phy_config)
/* clang-format on */

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_CDNS_H */
