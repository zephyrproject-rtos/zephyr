/*
 * Copyright 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MICROCHIP_MEC_COMMON_XEC_I2C_REGS_H_
#define ZEPHYR_SOC_MICROCHIP_MEC_COMMON_XEC_I2C_REGS_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XEC_I2C_SMB0_ID    0
#define XEC_I2C_SMB1_ID    1
#define XEC_I2C_SMB2_ID    2
#define XEC_I2C_SMB3_ID    3
#define XEC_I2C_SMB4_ID    4
#define XEC_I2C_SMB_MAX_ID 5

#define XEC_I2C_SMB_GIRQ    13U
#define XEC_I2C_SMB_WK_GIRQ 26U

#define XEC_I2C_SMB_INST_SIZE 0x400U

#define XEC_I2C_SMB_BASE(base, instance)                                                           \
	((uint32_t)(base) + ((uint32_t)(instance) * (XEC_I2C_SMB_INST_SIZE)))

/* Hardware only supports 7-bit I2C addressing */
#define XEC_I2C_TARGET_ADDR_MSK GENMASK(6, 0)

/* Hardware supports two target 7-bit addresses */
#define XEC_I2C_MAX_TARGETS 2

#define XEC_I2C_CR_OFS     0 /* WO */
#define XEC_I2C_CR_MSK     0xcfU
#define XEC_I2C_CR_ACK_POS 0
#define XEC_I2C_CR_STO_POS 1
#define XEC_I2C_CR_STA_POS 2
#define XEC_I2C_CR_ENI_POS 3
#define XEC_I2C_CR_ESO_POS 6
#define XEC_I2C_CR_PIN_POS 7

#define XEC_I2C_SR_OFS         0 /* RO */
#define XEC_I2C_SR_MSK         0xffU
#define XEC_I2C_SR_NBB_POS     0
#define XEC_I2C_SR_LAB_POS     1
#define XEC_I2C_SR_AAT_POS     2
#define XEC_I2C_SR_LRB_AD0_POS 3
#define XEC_I2C_SR_BER_POS     4
#define XEC_I2C_SR_STO_POS     5
#define XEC_I2C_SR_SAD_POS     6
#define XEC_I2C_SR_PIN_POS     7

/* XEC I2C hardware can match the following addresses in addition
 * to the two programmable 7-bit OWN addresses.
 * I2C general call 7-bit address 0x00. I2C.CONFIG.GC_DIS bit = 0
 * SMBus Host and Device addresses 0x08 and 0x61. I2C.CONFIG.DSA bit = 1
 * Detection using I2C.STATUS register.
 * AAT SAD LRB/AD0 address
 *  1   1   0      0x08 SMBus Host address
 *  1   1   1      0x61 SMBus Device address
 *  1   0   1      0x00 I2C general call address
 *  1   0   0      One of the two programmable OWN addresses.
 * The actual address can be viewed without side-effect by reading I2C.SHAD_ADDR
 * register. It can be read from I2C.DATA with side-effect of clearing AAT, SAD, and LRB/AD0.
 * In network layer mode the HW stores the address via DMA into the configured memory buffer.
 */
#define XEC_I2C_GEN_CALL_ADDR   0u
#define XEC_I2C_SMB_HOST_ADDR   0x08U
#define XEC_I2C_SMB_DEVICE_ADDR 0x61U

#define XEC_I2C_SR_ADDR_MATCH_MSK                                                                  \
	(BIT(XEC_I2C_SR_PIN_POS) | BIT(XEC_I2C_SR_SAD_POS) | BIT(XEC_I2C_SR_LRB_AD0_POS) |         \
	 BIT(XEC_I2C_SR_AAT_POS))
#define XEC_I2C_SR_ADDR_MATCH_GEN_CALL (BIT(XEC_I2C_SR_LRB_AD0_POS) | BIT(XEC_I2C_SR_AAT_POS))
#define XEC_I2C_SR_ADDR_MATCH_SMB_HOST (BIT(XEC_I2C_SR_SAD_POS) | BIT(XEC_I2C_SR_AAT_POS))
#define XEC_I2C_SR_ADDR_MATCH_SMB_DEV                                                              \
	(BIT(XEC_I2C_SR_SAD_POS) | BIT(XEC_I2C_SR_LRB_AD0_POS) | BIT(XEC_I2C_SR_AAT_POS))

#define XEC_I2C_OA_OFS       0x4U /* own(target) address */
#define XEC_I2C_OA_1_POS     0
#define XEC_I2C_OA_2_POS     8
#define XEC_I2C_OA_1_MSK     GENMASK(6, 0)
#define XEC_I2C_OA_2_MSK     GENMASK(14, 8)
#define XEC_I2C_OA_1_SET(a)  FIELD_PREP(XEC_I2C_OA_1_MSK, (a))
#define XEC_I2C_OA_2_SET(a)  FIELD_PREP(XEC_I2C_OA_2_MSK, (a))
#define XEC_I2C_OA_1_GET(r)  FIELD_GET(XEC_I2C_OA_1_MSK, (r))
#define XEC_I2C_OA_2_GET(r)  FIELD_GET(XEC_I2C_OA_2_MSK, (r))
#define XEC_I2C_OA_POS(n)    ((n) * 8U)
#define XEC_I2C_OA_MSK(n)    (0x7FU << XEC_I2C_OA_POS(n))
#define XEC_I2C_OA_SET(n, a) (((uint32_t)(a) << XEC_I2C_OA_POS(n)) & XEC_I2C_OA_MSK(n))
#define XEC_I2C_OA_GET(n, r) (((uint32_t)(r) & XEC_I2C_OA_MSK(n)) >> XEC_I2C_OA_POS(n))

#define XEC_I2C_OA_NUM_TARGETS 2U

#define XEC_I2C_DATA_OFS 0x8U
#define XEC_I2C_DATA_MSK GENMASK(7, 0)

#define XEC_I2C_HCMD_OFS        0x0cU /* network layer host command */
#define XEC_I2C_HCMD_RUN_POS    0
#define XEC_I2C_HCMD_PROC_POS   1
#define XEC_I2C_HCMD_START0_POS 8
#define XEC_I2C_HCMD_STARTN_POS 9
#define XEC_I2C_HCMD_STOP_POS   10
#define XEC_I2C_HCMD_PEC_TX_POS 11
#define XEC_I2C_HCMD_RDM_POS    12
#define XEC_I2C_HCMD_PEC_RD_POS 13
#define XEC_I2C_HCMD_WCL_POS    16
#define XEC_I2C_HCMD_WCL_MSK    GENMASK(23, 16)
#define XEC_I2C_HCMD_WCL_SET(n) FIELD_PREP(XEC_I2C_HCMD_WCL_MSK, (n))
#define XEC_I2C_HCMD_WCL_GET(r) FIELD_GET(XEC_I2C_HCMD_WCL_MSK, (r))
#define XEC_I2C_HCMD_RCL_POS    24
#define XEC_I2C_HCMD_RCL_MSK    GENMASK(31, 24)
#define XEC_I2C_HCMD_RCL_SET(n) FIELD_PREP(XEC_I2C_HCMD_RCL_MSK, (n))
#define XEC_I2C_HCMD_RCL_GET(r) FIELD_GET(XEC_I2C_HCMD_RCL_MSK, (r))

#define XEC_I2C_TCMD_OFS        0x10U /* network layer target command */
#define XEC_I2C_TCMD_RUN_POS    0
#define XEC_I2C_TCMD_PROC_POS   1
#define XEC_I2C_TCMD_TX_PEC_POS 2
#define XEC_I2C_TCMD_WCL_POS    8
#define XEC_I2C_TCMD_WCL_MSK    GENMASK(15, 8)
#define XEC_I2C_TCMD_WCL_SET(n) FIELD_PREP(XEC_I2C_TCMD_WCL_MSK, (n))
#define XEC_I2C_TCMD_WCL_GET(r) FIELD_GET(XEC_I2C_TCMD_WCL_MSK, (r))
#define XEC_I2C_TCMD_RCL_POS    16
#define XEC_I2C_TCMD_RCL_MSK    GENMASK(23, 16)
#define XEC_I2C_TCMD_RCL_SET(n) FIELD_PREP(XEC_I2C_TCMD_RCL_MSK, (n))
#define XEC_I2C_TCMD_RCL_GET(r) FIELD_GET(XEC_I2C_TCMD_RCL_MSK, (r))

#define XEC_I2C_PEC_OFS 0x14U
#define XEC_I2C_PEC_MSK GENMASK(7, 0)

#define XEC_I2C_RSHT_OFS 0x18U /* Repeated-START hold time */
#define XEC_I2C_RSHT_MSK GENMASK(7, 0)

/* Network layer mode extended length. Contains bit[15:8] of write and read counts */
#define XEC_I2C_ELEN_OFS        0x1cU
#define XEC_I2C_ELEN_HWR_POS    0
#define XEC_I2C_ELEN_HWR_MSK    GENMASK(7, 0)
#define XEC_I2C_ELEN_HWR_SET(n) FIELD_PREP(XEC_I2C_ELEN_HWR_MSK, (n))
#define XEC_I2C_ELEN_HWR_GET(r) FIELD_GET(XEC_I2C_ELEN_HWR_MSK, (r))
#define XEC_I2C_ELEN_HRD_POS    8
#define XEC_I2C_ELEN_HRD_MSK    GENMASK(15, 8)
#define XEC_I2C_ELEN_HRD_SET(n) FIELD_PREP(XEC_I2C_ELEN_HRD_MSK, (n))
#define XEC_I2C_ELEN_HRD_GET(r) FIELD_GET(XEC_I2C_ELEN_HRD_MSK, (r))
#define XEC_I2C_ELEN_TWR_POS    16
#define XEC_I2C_ELEN_TWR_MSK    GENMASK(23, 16)
#define XEC_I2C_ELEN_TWR_SET(n) FIELD_PREP(XEC_I2C_ELEN_TWR_MSK, (n))
#define XEC_I2C_ELEN_TWR_GET(r) FIELD_GET(XEC_I2C_ELEN_TWR_MSK, (r))
#define XEC_I2C_ELEN_TRD_POS    24
#define XEC_I2C_ELEN_TRD_MSK    GENMASK(31, 24)
#define XEC_I2C_ELEN_TRD_SET(n) FIELD_PREP(XEC_I2C_ELEN_TRD_MSK, (n))
#define XEC_I2C_ELEN_TRD_GET(r) FIELD_GET(XEC_I2C_ELEN_TRD_MSK, (r))

#define XEC_I2C_CMPL_OFS 0x20U /* Completion: R/W1C status and timeout check enables */
#define XEC_I2C_CMPL_MSK                                                                           \
	(GENMASK(6, 2) | GENMASK(14, 8) | GENMASK(17, 16) | GENMASK(21, 19) | GENMASK(25, 24) |    \
	 GENMASK(31, 29))
#define XEC_I2C_CMPL_RW_MSK GENMASK(5, 2)
#define XEC_I2C_CMPL_RO_MSK (BIT(6) | BIT(17) | BIT(25))
#define XEC_I2C_CMPL_RW1C_MSK                                                                      \
	(GENMASK(14, 8) | BIT(16) | GENMASK(21, 19) | BIT(24) | GENMASK(31, 29))
#define XEC_I2C_CMPL_DTEN_POS      2
#define XEC_I2C_CMPL_HCEN_POS      3
#define XEC_I2C_CMPL_TCEN_POS      4
#define XEC_I2C_CMPL_BIDEN_POS     5
#define XEC_I2C_CMPL_TMO_STS_POS   6 /* RO - any of 4 timeouts detected */
#define XEC_I2C_CMPL_DTS_STS_POS   8
#define XEC_I2C_CMPL_HCTO_STS_POS  9
#define XEC_I2C_CMPL_TCTO_STS_POS  10
#define XEC_I2C_CMPL_CHDL_STS_POS  11
#define XEC_I2C_CMPL_CHDH_STS_POS  12
#define XEC_I2C_CMPL_BER_STS_POS   13
#define XEC_I2C_CMPL_LAB_STS_POS   14
#define XEC_I2C_CMPL_TNAKR_STS_POS 16
#define XEC_I2C_CMPL_TTR_POS       17 /* RO */
#define XEC_I2C_CMPL_TPROT_POS     19
#define XEC_I2C_CMPL_RPT_RD_POS    20
#define XEC_I2C_CMPL_RPT_WR_POS    21
#define XEC_I2C_CMPL_HNAKX_POS     24
#define XEC_I2C_CMPL_HTR_POS       25 /* RO */
#define XEC_I2C_CMPL_IDLE_POS      29
#define XEC_I2C_CMPL_HDONE_POS     30
#define XEC_I2C_CMPL_TDONE_POS     31

#define XEC_I2C_ISC_OFS         0x24U /* fairness idle time scaling */
#define XEC_I2C_ISC_FBI_POS     0
#define XEC_I2C_ISC_FBI_MSK     GENMASK(11, 0)
#define XEC_I2C_ISC_FBI_SET(n)  FIELD_PREP(XEC_I2C_ISC_FBI_MSK, (n))
#define XEC_I2C_ISC_FBI_GET(r)  FIELD_GET(XEC_I2C_ISC_FBI_MSK, (r))
#define XEC_I2C_ISC_FIDD_POS    16
#define XEC_I2C_ISC_FIDD_MSK    GENMASK(27, 16)
#define XEC_I2C_ISC_FIDD_SET(n) FIELD_PREP(XEC_I2C_ISC_FIDD_MSK, (n))
#define XEC_I2C_ISC_FIDD_GET(r) FIELD_GET(XEC_I2C_ISC_FIDD_MSK, (r))

#define XEC_I2C_CFG_OFS            0x28U
#define XEC_I2C_CFG_PORT_POS       0
#define XEC_I2C_CFG_PORT_MSK       GENMASK(3, 0)
#define XEC_I2C_CFG_PORT_SET(p)    FIELD_PREP(XEC_I2C_CFG_PORT_MSK, (p))
#define XEC_I2C_CFG_PORT_GET(r)    FIELD_GET(XEC_I2C_CFG_PORT_MSK, (r))
#define XEC_I2C_CFG_TCEN_POS       4
#define XEC_I2C_CFG_SLOW_CLK_POS   5
#define XEC_I2C_CFG_PECEN_POS      7
#define XEC_I2C_CFG_FEN_POS        8
#define XEC_I2C_CFG_RST_POS        9
#define XEC_I2C_CFG_ENAB_POS       10
#define XEC_I2C_CFG_DSA_POS        11
#define XEC_I2C_CFG_FAIR_POS       12
#define XEC_I2C_CFG_GC_DIS_POS     14
#define XEC_I2C_CFG_PROM_EN_POS    15
#define XEC_I2C_CFG_FTTX_POS       16 /* WO */
#define XEC_I2C_CFG_FTRX_POS       17 /* WO */
#define XEC_I2C_CFG_FHTX_POS       18 /* WO */
#define XEC_I2C_CFG_FHRX_POS       19 /* WO */
#define XEC_I2C_CFG_STD_IEN_POS    24 /* v3.8 HW only */
#define XEC_I2C_CFG_STD_NL_IEN_POS 27 /* v3.8 HW only */
#define XEC_I2C_CFG_AAT_IEN_POS    28
#define XEC_I2C_CFG_IDLE_IEN_POS   29
#define XEC_I2C_CFG_HD_IEN_POS     30
#define XEC_I2C_CFG_TD_IEN_POS     31

#define XEC_I2C_CFG_MAX_PORT 16

#define XEC_I2C_BCLK_OFS        0x2cU /* bus clock */
#define XEC_I2C_BCLK_LOP_POS    0
#define XEC_I2C_BCLK_LOP_MSK    GENMASK(7, 0)
#define XEC_I2C_BCLK_LOP_SET(n) FIELD_PREP(XEC_I2C_BCLK_LOP_MSK, (n))
#define XEC_I2C_BCLK_LOP_GET(r) FIELD_GET(XEC_I2C_BCLK_LOP_MSK, (r))
#define XEC_I2C_BCLK_HIP_POS    8
#define XEC_I2C_BCLK_HIP_MSK    GENMASK(15, 8)
#define XEC_I2C_BCLK_HIP_SET(n) FIELD_PREP(XEC_I2C_BCLK_HIP_MSK, (n))
#define XEC_I2C_BCLK_HIP_GET(r) FIELD_GET(XEC_I2C_BCLK_HIP_MSK, (r))

#define XEC_I2C_BLKID_OFS 0x30U /* RO HW block ID */
#define XEC_I2C_REV_OFS   0x34U /* RO HW revision */

#define XEC_I2C_BBCR_OFS        0x38U /* bit-bang control */
#define XEC_I2C_BBCR_EN_POS     0     /* MUX SCL/SDA pins from I2C to BB logic */
#define XEC_I2C_BBCR_CD_POS     1
#define XEC_I2C_BBCR_DD_POS     2
#define XEC_I2C_BBCR_SCL_POS    3
#define XEC_I2C_BBCR_SDA_POS    4
#define XEC_I2C_BBCR_SCL_IN_POS 5
#define XEC_I2C_BBCR_SDA_IN_POS 6
#define XEC_I2C_BBCR_CM_POS     7 /* ver3.8 only */

#define XEC_I2C_MR0_OFS 0x3cU /* MCHP reserved 0 */

#define XEC_I2C_DT_OFS         0x40U /* data timing */
#define XEC_I2C_DT_DH_POS      0
#define XEC_I2C_DT_DH_MSK      GENMASK(7, 0)
#define XEC_I2C_DT_DH_SET(n)   FIELD_PREP(XEC_I2C_DT_DH_MSK, (n))
#define XEC_I2C_DT_DH_GET(r)   FIELD_GET(XEC_I2C_DT_DH_MSK, (r))
#define XEC_I2C_DT_RSS_POS     8
#define XEC_I2C_DT_RSS_MSK     GENMASK(15, 8)
#define XEC_I2C_DT_RSS_SET(n)  FIELD_PREP(XEC_I2C_DT_RSS_MSK, (n))
#define XEC_I2C_DT_RSS_GET(r)  FIELD_GET(XEC_I2C_DT_RSS_MSK, (r))
#define XEC_I2C_DT_STPS_POS    16
#define XEC_I2C_DT_STPS_MSK    GENMASK(23, 16)
#define XEC_I2C_DT_STPS_SET(n) FIELD_PREP(XEC_I2C_DT_STPS_MSK, (n))
#define XEC_I2C_DT_STPS_GET(r) FIELD_GET(XEC_I2C_DT_STPS_MSK, (r))
#define XEC_I2C_DT_FSH_POS     24
#define XEC_I2C_DT_FSH_MSK     GENMASK(31, 24)
#define XEC_I2C_DT_FSH_SET(n)  FIELD_PREP(XEC_I2C_DT_FSH_MSK, (n))
#define XEC_I2C_DT_FSH_GET(r)  FIELD_GET(XEC_I2C_DT_FSH_MSK, (r))

#define XEC_I2C_TMOUT_SC_OFS         0x44U /* timeout scaling */
#define XEC_I2C_TMOUT_SC_CHTO_POS    0
#define XEC_I2C_TMOUT_SC_CHTO_MSK    GENMASK(7, 0)
#define XEC_I2C_TMOUT_SC_CHTO_SET(n) FIELD_PREP(XEC_I2C_TMOUT_SC_CHTO_MSK, (n))
#define XEC_I2C_TMOUT_SC_CHTO_GET(r) FIELD_GET(XEC_I2C_TMOUT_SC_CHTO_MSK, (r))
#define XEC_I2C_TMOUT_SC_TCTO_POS    8
#define XEC_I2C_TMOUT_SC_TCTO_MSK    GENMASK(15, 8)
#define XEC_I2C_TMOUT_SC_TCTO_SET(n) FIELD_PREP(XEC_I2C_TMOUT_SC_TCTO_MSK, (n))
#define XEC_I2C_TMOUT_SC_TCTO_GET(r) FIELD_GET(XEC_I2C_TMOUT_SC_TCTO_MSK, (r))
#define XEC_I2C_TMOUT_SC_HCTO_POS    16
#define XEC_I2C_TMOUT_SC_HCTO_MSK    GENMASK(23, 16)
#define XEC_I2C_TMOUT_SC_HCTO_SET(n) FIELD_PREP(XEC_I2C_TMOUT_SC_HCTO_MSK, (n))
#define XEC_I2C_TMOUT_SC_HCTO_GET(r) FIELD_GET(XEC_I2C_TMOUT_SC_HCTO_MSK, (r))
#define XEC_I2C_TMOUT_SC_BIM_POS     24
#define XEC_I2C_TMOUT_SC_BIM_MSK     GENMASK(31, 24)
#define XEC_I2C_TMOUT_SC_BIM_SET(n)  FIELD_PREP(XEC_I2C_TMOUT_SC_BIM_MSK, (n))
#define XEC_I2C_TMOUT_SC_BIM_GET(r)  FIELD_GET(XEC_I2C_TMOUT_SC_BIM_MSK, (r))

/* 8-bit data register for network layer mode */
#define XEC_I2C_TTX_OFS 0x48U /* network layer mode target transmit */
#define XEC_I2C_TRX_OFS 0x4cU /* network layer mode target receive */
#define XEC_I2C_HTX_OFS 0x50U /* network layer mode host transmit */
#define XEC_I2C_HRX_OFS 0x54U /* network layer mode host receive */

#define XEC_I2C_IFSM_OFS                  0x58U /* RO I2C HW FSM */
#define XEC_I2C_IFSM_HM_ST_POS            0
#define XEC_I2C_IFSM_HM_ST_MSK            GENMASK(7, 0)
#define XEC_I2C_IFSM_HM_ST_IDLE           0
#define XEC_I2C_IFSM_HM_ST_W4STA          1U
#define XEC_I2C_IFSM_HM_ST_TXADDR         2U
#define XEC_I2C_IFSM_HM_ST_CHK_AACK       3U
#define XEC_I2C_IFSM_HM_ST_RXDATA         4U /* debug I2C0 was here */
#define XEC_I2C_IFSM_HM_ST_CHK_DACK       5U
#define XEC_I2C_IFSM_HM_ST_TXDATA         6U
#define XEC_I2C_IFSM_HM_ST_TXDATA_LD      7U
#define XEC_I2C_IFSM_HM_ST_W4ACK          8U
#define XEC_I2C_IFSM_HM_ST_W4STO          9U
#define XEC_I2C_IFSM_HM_ST_LARB           10U
#define XEC_I2C_IFSM_HM_ST_LARB_RSTA      11U
#define XEC_I2C_IFSM_HM_ST_LARB_RSTA_DLY1 12U
#define XEC_I2C_IFSM_HM_ST_LARB_RSTA_DLY2 13U
#define XEC_I2C_IFSM_HM_ST_W4STA_HLD      14U
#define XEC_I2C_IFSM_HM_ST_GET(r)         FIELD_GET(XEC_I2C_IFSM_HM_ST_MSK, (r))

#define XEC_I2C_IFSM_TM_ST_POS     8
#define XEC_I2C_IFSM_TM_ST_MSK     GENMASK(15, 8)
#define XEC_I2C_IFSM_TM_ST_IDLE    0
#define XEC_I2C_IFSM_TM_ST_HDRACK  1U
#define XEC_I2C_IFSM_TM_ST_TXDATA  2U
#define XEC_I2C_IFSM_TM_ST_W4ACK   3U
#define XEC_I2C_IFSM_TM_ST_RXDATA  4U
#define XEC_I2C_IFSM_TM_ST_ACKDATA 5U
#define XEC_I2C_IFSM_TM_ST_GET(r)  FIELD_GET(XEC_I2C_IFSM_TM_ST_MSK, (r))

#define XEC_I2C_IFSM_PHY_POS     16
#define XEC_I2C_IFSM_PHY_MSK     GENMASK(19, 16)
#define XEC_I2C_IFSM_PHY_IDLE    0
#define XEC_I2C_IFSM_PHY_CLKHI   1U
#define XEC_I2C_IFSM_PHY_STA_STO 2U
#define XEC_I2C_IFSM_PHY_CLKLO   3U
#define XEC_I2C_IFSM_PHY_SDATCR  4U
#define XEC_I2C_IFSM_PHY_ARBLOSS 5U
#define XEC_I2C_IFSM_PHY_GET(r)  FIELD_GET(XEC_I2C_IFSM_TM_PHY_MSK, (r))

#define XEC_I2C_IFSM_HMCTO_POS    20
#define XEC_I2C_IFSM_HMCTO_MSK    GENMASK(23, 20)
#define XEC_I2C_IFSM_HMCTO_IDLE   0
#define XEC_I2C_IFSM_HMCTO_CNT    1U
#define XEC_I2C_IFSM_HMCTO_GET(r) FIELD_GET(XEC_I2C_IFSM_HMCTO_MSK, (r))

#define XEC_I2C_IFSM_SMCTO_POS    24
#define XEC_I2C_IFSM_SMCTO_MSK    GENMASK(27, 24)
#define XEC_I2C_IFSM_SMCTO_IDLE   0
#define XEC_I2C_IFSM_SMCTO_CNT    1U
#define XEC_I2C_IFSM_SMCTO_GET(r) FIELD_GET(XEC_I2C_IFSM_SMCTO_MSK, (r))

#define XEC_I2C_IFSM_TMBI_POS    28
#define XEC_I2C_IFSM_TMBI_MSK    GENMASK(31, 28)
#define XEC_I2C_IFSM_TMBI_IDLE   0
#define XEC_I2C_IFSM_TMBI_CNT    1U
#define XEC_I2C_IFSM_TMBI_GET(r) FIELD_GET(XEC_I2C_IFSM_TMBI_MSK, (r))

#define XEC_I2C_NFSM_OFS               0x5cu /* RO Network layer mode HW FSM */
#define XEC_I2C_NFSM_HC_STATE_POS      0
#define XEC_I2C_NFSM_HC_STATE_MSK      GENMASK(7, 0)
#define XEC_I2C_NFSM_HC_STATE_IDLE     0
#define XEC_I2C_NFSM_HC_STATE_SOP      1U
#define XEC_I2C_NFSM_HC_STATE_STA      2U
#define XEC_I2C_NFSM_HC_STATE_STA_PIN  3U
#define XEC_I2C_NFSM_HC_STATE_WDATA    4U
#define XEC_I2C_NFSM_HC_STATE_WPEC     5U
#define XEC_I2C_NFSM_HC_STATE_RSTA     6U
#define XEC_I2C_NFSM_HC_STATE_RSTA_PIN 7U
#define XEC_I2C_NFSM_HC_STATE_RDN      8U
#define XEC_I2C_NFSM_HC_STATE_RD_PEC   9U
#define XEC_I2C_NFSM_HC_STATE_RPEC     10U
#define XEC_I2C_NFSM_HC_STATE_PAUSE    11U
#define XEC_I2C_NFSM_HC_STATE_STO      12U
#define XEC_I2C_NFSM_HC_STATE_EOP      13U
#define XEC_I2C_NFSM_HC_STATE_GET(r)   FIELD_GET(XEC_I2C_NFSM_HC_STATE_MSK, (r))
#define XEC_I2C_NFSM_TC_STATE_POS      8
#define XEC_I2C_NFSM_TC_STATE_MSK      GENMASK(15, 8)
#define XEC_I2C_NFSM_TC_STATE_IDLE     0
#define XEC_I2C_NFSM_TC_STATE_ADDR     1U
#define XEC_I2C_NFSM_TC_STATE_WPIN     2U
#define XEC_I2C_NFSM_TC_STATE_RPIN     3U
#define XEC_I2C_NFSM_TC_STATE_WDATA    4U
#define XEC_I2C_NFSM_TC_STATE_RDATA    5U
#define XEC_I2C_NFSM_TC_STATE_RBE      6U
#define XEC_I2C_NFSM_TC_STATE_RPEC     7U
#define XEC_I2C_NFSM_TC_STATE_RPECRPT  8U
#define XEC_I2C_NFSM_TC_STATE_GET(r)   FIELD_GET(XEC_I2C_NFSM_TC_STATE_MSK, (r))
#define XEC_I2C_NFSM_FAIR_POS          16
#define XEC_I2C_NFSM_FAIR_MSK          GENMASK(23, 16)
#define XEC_I2C_NFSM_FAIR_IDLE         0
#define XEC_I2C_NFSM_FAIR_BUSY         1U
#define XEC_I2C_NFSM_FAIR_WIN          2U
#define XEC_I2C_NFSM_FAIR_DLY          3U
#define XEC_I2C_NFSM_FAIR_WAIT         4U
#define XEC_I2C_NFSM_FAIR_WAIT_DONE    5U
#define XEC_I2C_NFSM_FAIR_ACTIVE       6U

#define XEC_I2C_WKSR_OFS    0x60U /* wake status */
#define XEC_I2C_WKSR_SB_POS 0     /* start bit detected R/W1C */

#define XEC_I2C_WKCR_OFS      0x64U /* wake control */
#define XEC_I2C_WKCR_SBEN_POS 0     /* start bit detection interrupt enable */

#define XEC_I2C_MR1_OFS 0x68U /* MCHP reserved 1 */

#define XEC_I2C_IAS_OFS 0x6cU /* RO I2C address shadow */

#define XEC_I2C_PIS_OFS     0x70U /* I2C promiscuous mode interrupt status */
/* I2C has captured 8 address bits and is stretching the clock
 * software sets HW to (n)ACK
 */
#define XEC_I2C_PIS_CAP_POS 0

#define XEC_I2C_PIE_OFS     0x74U /* I2C promiscuous mode interrupt enable */
#define XEC_I2C_PIE_CAP_POS 0

#define XEC_I2C_PCR_OFS     0x78U /* I2C promiscuous mode control */
#define XEC_I2C_PCR_ACK_POS 0     /* write 1 to ACK the captured address */

#define XEC_I2C_IDS_OFS 0x7cU /* RO I2C data shadow */

#define XEC_I2C_SMB_DATA_TM_100K 0x0C4D5006U
#define XEC_I2C_SMB_IDLE_SC_100K 0x01FC01EDU
#define XEC_I2C_SMB_TMO_SC_100K  0x4B9CC2C7U
#define XEC_I2C_SMB_BUS_CLK_100K 0x4F4FU
#define XEC_I2C_SMB_RSHT_100K    0x4DU

#define XEC_I2C_SMB_DATA_TM_400K 0x040A0A06U
#define XEC_I2C_SMB_IDLE_SC_400K 0x01000050U
#define XEC_I2C_SMB_TMO_SC_400K  0x159CC2C7U
#define XEC_I2C_SMB_BUS_CLK_400K 0x0F17U
#define XEC_I2C_SMB_RSHT_400K    0x0AU

#define XEC_I2C_SMB_DATA_TM_1M 0x04060601U
#define XEC_I2C_SMB_IDLE_SC_1M 0x10000050U
#define XEC_I2C_SMB_TMO_SC_1M  0x089CC2C7U
#define XEC_I2C_SMB_BUS_CLK_1M 0x0509U
#define XEC_I2C_SMB_RSHT_1M    0x06U

#define XEC_I2C_NL_MAX_LEN 0xfff8U

#define XEC_I2C_MAX_PORTS (((XEC_I2C_CFG_PORT_MSK) >> (XEC_I2C_CFG_PORT_POS)) + 1U)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_MICROCHIP_MEC_COMMON_XEC_I2C_REGS_H_ */
