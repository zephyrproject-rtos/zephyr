/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_MEC_ESPI_IOM_V2_H
#define _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_MEC_ESPI_IOM_V2_H

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

/* Offsets from eSPI base for various register groups */
#define MCHP_ESPI_IO_PC_OFS       0x0100U
#define MCHP_ESPI_IO_HOST_BAR_OFS 0x0120U
#define MCHP_ESPI_IO_LTR_OFS      0x0220U
#define MCHP_ESPI_IO_OOB_OFS      0x0240U
#define MCHP_ESPI_IO_FC_OFS       0x0280U
#define MCHP_ESPI_IO_CAP_OFS      0x02b0U
#define MCHP_ESPI_IO_CFG_OFS      0x0300U
#define MCHP_ESPI_MC_OFS          0x0400U /* memory component offset from IOC base */

/* eSPI memory component base offsets */
#define MCHP_ESPI_MC_BAR_LDM_OFS      0x130U
#define MCHP_ESPI_MC_BAR_SRAM_ACC_OFS 0x1ACU
#define MCHP_ESPI_MC_BM_OFS           0x200U
#define MCHP_ESPI_MC_CFG_OFS          0x330U

/* eSPI Global Capabilities 0 */
#define MCHP_ESPI_GBL_CAP0_MASK     0x0fU
#define MCHP_ESPI_GBL_CAP0_PC_SUPP  BIT(0)
#define MCHP_ESPI_GBL_CAP0_VW_SUPP  BIT(1)
#define MCHP_ESPI_GBL_CAP0_OOB_SUPP BIT(2)
#define MCHP_ESPI_GBL_CAP0_FC_SUPP  BIT(3)

/* eSPI Global Capabilities 1 */
#define MCHP_ESPI_GBL_CAP1_MASK          0xffU
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_POS  0U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_MASK 0x07U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_20M  0x00U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_25M  0x01U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_33M  0x02U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_50M  0x03U
#define MCHP_ESPI_GBL_CAP1_MAX_FREQ_66M  0x04U
#define MCHP_ESPI_GBL_CAP1_ALERT_POS     3U /* Read-Only */
#define MCHP_ESPI_GBL_CAP1_ALERT_DED_PIN BIT(MCHP_ESPI_GBL_CAP1_ALERT_POS)
#define MCHP_ESPI_GBL_CAP1_ALERT_ON_IO1  0U
#define MCHP_ESPI_GBL_CAP1_IO_MODE_POS   4U
#define MCHP_ESPI_GBL_CAP1_IO_MODE_MASK0 GENMASK(1, 0)
#define MCHP_ESPI_GBL_CAP1_IO_MODE_MASK  GENMASK(5, 4)
#define MCHP_ESPI_GBL_CAP1_IO_MODE0_1    0U
#define MCHP_ESPI_GBL_CAP1_IO_MODE0_12   1U
#define MCHP_ESPI_GBL_CAP1_IO_MODE0_14   2U
#define MCHP_ESPI_GBL_CAP1_IO_MODE0_124  3U
#define MCHP_ESPI_GBL_CAP1_IO_MODE_1     FIELD_PREP(MCHP_ESPI_GBL_CAP1_IO_MODE_MASK, 0)
#define MCHP_ESPI_GBL_CAP1_IO_MODE_12    FIELD_PREP(MCHP_ESPI_GBL_CAP1_IO_MODE_MASK, 1U)
#define MCHP_ESPI_GBL_CAP1_IO_MODE_14    FIELD_PREP(MCHP_ESPI_GBL_CAP1_IO_MODE_MASK, 2U)
#define MCHP_ESPI_GBL_CAP1_IO_MODE_124   FIELD_PREP(MCHP_ESPI_GBL_CAP1_IO_MODE_MASK, 3U)

/*
 * Support Open Drain ALERT pin configuration
 * EC sets this bit if it can support open-drain ESPI_ALERT#
 */
#define MCHP_ESPI_GBL_CAP1_ALERT_ODS_POS 6U
#define MCHP_ESPI_GBL_CAP1_ALERT_ODS     BIT(MCHP_ESPI_GBL_CAP1_ALERT_ODS_POS)

/*
 * Read-Only ALERT Open Drain select.
 * If EC has indicated it can support open-drain ESPI_ALERT# then
 * the Host can enable open-drain ESPI_ALERT# by sending a configuration
 * message. This read-only bit reflects the configuration selection.
 */
#define MCHP_ESPI_GBL_CAP1_ALERT_ODS_SEL_POS 7U
#define MCHP_ESPI_GBL_CAP1_ALERT_SEL_ODS     BIT(MCHP_ESPI_GBL_CAP1_ALERT_ODS_SEL_POS)

/* Peripheral Channel(PC) Capabilities */
#define MCHP_ESPI_PC_CAP_MASK            0x07U
#define MCHP_ESPI_PC_CAP_MAX_PLD_SZ_MASK 0x07U
#define MCHP_ESPI_PC_CAP_MAX_PLD_SZ_64   0x01U
#define MCHP_ESPI_PC_CAP_MAX_PLD_SZ_128  0x02U
#define MCHP_ESPI_PC_CAP_MAX_PLD_SZ_256  0x03U

/* Virtual Wire(VW) Capabilities */
#define MCHP_ESPI_VW_CAP_MASK            0x3fU
#define MCHP_ESPI_VW_CAP_MAX_VW_CNT_MASK 0x3fU

/* Out-of-Band(OOB) Capabilities */
#define MCHP_ESPI_OOB_CAP_MASK            0x07U
#define MCHP_ESPI_OOB_CAP_MAX_PLD_SZ_MASK 0x07U
#define MCHP_ESPI_OOB_CAP_MAX_PLD_SZ_73   0x01U
#define MCHP_ESPI_OOB_CAP_MAX_PLD_SZ_137  0x02U
#define MCHP_ESPI_OOB_CAP_MAX_PLD_SZ_265  0x03U

/* Flash Channel(FC) Capabilities */
#define MCHP_ESPI_FC_CAP_MASK            0xffU
#define MCHP_ESPI_FC_CAP_MAX_PLD_SZ_MASK 0x07U
#define MCHP_ESPI_FC_CAP_MAX_PLD_SZ_64   0x01U
#define MCHP_ESPI_FC_CAP_SHARE_POS       3U
#define MCHP_ESPI_FC_CAP_SHARE_MASK0     GENMASK(1, 0)
#define MCHP_ESPI_FC_CAP_SHARE_MASK      GENMASK(4, 3)

#define MCHP_ESPI_FC_CAP_SHARE_CAF_ONLY  0U
#define MCHP_ESPI_FC_CAP_SHARE_CAF2_ONLY BIT(MCHP_ESPI_FC_CAP_SHARE_POS)
#define MCHP_ESPI_FC_CAP_SHARE_TAF_ONLY  FIELD_PREP(MCHP_ESPI_FC_CAP_SHARE_MASK, 2U)
#define MCHP_ESPI_FC_CAP_SHARE_CAF_TAF   FIELD_PREP(MCHP_ESPI_FC_CAP_SHARE_MASK, 3U)

/* Temporary until SAF to TAF converstion done */
#define MCHP_ESPI_FC_CAP_SHARE_MAF_SAF MCHP_ESPI_FC_CAP_SHARE_CAF_TAF

#define MCHP_ESPI_FC_CAP_MAX_RD_SZ_POS   5U
#define MCHP_ESPI_FC_CAP_MAX_RD_SZ_MASK0 GENMASK(2, 0)
#define MCHP_ESPI_FC_CAP_MAX_RD_SZ_MASK  GENMASK(7, 5)

#define MCHP_ESPI_FC_CAP_MAX_RD_SZ_64 BIT(MCHP_ESPI_FC_CAP_MAX_RD_SZ_POS)

/* PC Ready */
#define MCHP_ESPI_PC_READY_MASK 0x01U;
#define MCHP_ESPI_PC_READY      0x01U;

/* OOB Ready */
#define MCHP_ESPI_OOB_READY_MASK 0x01U;
#define MCHP_ESPI_OOB_READY      0x01U;

/* FC Ready */
#define MCHP_ESPI_FC_READY_MASK 0x01U;
#define MCHP_ESPI_FC_READY      0x01U;

/* ESPI_RESET# Interrupt Status */
#define MCHP_ESPI_RST_ISTS_MASK       0x03U;
#define MCHP_ESPI_RST_ISTS_POS        0U
#define MCHP_ESPI_RST_ISTS            BIT(MCHP_ESPI_RST_ISTS_POS)
#define MCHP_ESPI_RST_ISTS_PIN_RO_POS 1U
#define MCHP_ESPI_RST_ISTS_PIN_RO_HI  BIT(MCHP_ESPI_RST_ISTS_PIN_RO_POS)

/* ESPI_RESET# Interrupt Enable */
#define MCHP_ESPI_RST_IEN_MASK 0x01U
#define MCHP_ESPI_RST_IEN      0x01U

/* eSPI Platform Reset Source */
#define MCHP_ESPI_PLTRST_SRC_MASK   0x01U
#define MCHP_ESPI_PLTRST_SRC_POS    0U
#define MCHP_ESPI_PLTRST_SRC_IS_PIN 0x01U
#define MCHP_ESPI_PLTRST_SRC_IS_VW  0x00U

/* VW Ready */
#define MCHP_ESPI_VW_READY_MASK 0x01U
#define MCHP_ESPI_VW_READY      0x01U

/* SAF Erase Block size */
#define MCHP_ESPI_SERASE_SZ_1K_BITPOS   0
#define MCHP_ESPI_SERASE_SZ_2K_BITPOS   1
#define MCHP_ESPI_SERASE_SZ_4K_BITPOS   2
#define MCHP_ESPI_SERASE_SZ_8K_BITPOS   3
#define MCHP_ESPI_SERASE_SZ_16K_BITPOS  4
#define MCHP_ESPI_SERASE_SZ_32K_BITPOS  5
#define MCHP_ESPI_SERASE_SZ_64K_BITPOS  6
#define MCHP_ESPI_SERASE_SZ_128K_BITPOS 7
#define MCHP_ESPI_SERASE_SZ_1K          BIT(0)
#define MCHP_ESPI_SERASE_SZ_2K          BIT(1)
#define MCHP_ESPI_SERASE_SZ_4K          BIT(2)
#define MCHP_ESPI_SERASE_SZ_8K          BIT(3)
#define MCHP_ESPI_SERASE_SZ_16K         BIT(4)
#define MCHP_ESPI_SERASE_SZ_32K         BIT(5)
#define MCHP_ESPI_SERASE_SZ_64K         BIT(6)
#define MCHP_ESPI_SERASE_SZ_128K        BIT(7)
#define MCHP_ESPI_SERASE_SZ(bitpos)     BIT((bitpos) + 10U)

/* VW Error Status */
#define MCHP_ESPI_VW_ERR_STS_MASK      0x33U
#define MCHP_ESPI_VW_ERR_STS_FATAL_POS 0U
#define MCHP_ESPI_VW_ERR_STS_FATAL_RO  BIT(MCHP_ESPI_VW_ERR_STS_FATAL_POS)

#define MCHP_ESPI_VW_ERR_STS_FATAL_CLR_POS 1U
#define MCHP_ESPI_VW_ERR_STS_FATAL_CLR_WO  BIT(MCHP_ESPI_VW_ERR_STS_FATAL_CLR_POS)

#define MCHP_ESPI_VW_ERR_STS_NON_FATAL_POS 4U
#define MCHP_ESPI_VW_ERR_STS_NON_FATAL_RO  BIT(MCHP_ESPI_VW_ERR_STS_NON_FATAL_POS)

#define MCHP_ESPI_VW_ERR_STS_NON_FATAL_CLR_POS 5U
#define MCHP_ESPI_VW_ERR_STS_NON_FATAL_CLR_WO  BIT(MCHP_ESPI_VW_ERR_STS_NON_FATAL_CLR_POS)

/* VW Channel Enable Status */
#define MCHP_ESPI_VW_EN_STS_MASK 0x01U
#define MCHP_ESPI_VW_EN_STS_RO   0x01U

/*
 * MCHP_ESPI_IO_PC - eSPI IO Peripheral Channel registers @ 0x400F3500
 */

/* Peripheral Channel Last Cycle length, type, and tag. */
#define MCHP_ESPI_PC_LC_LEN_POS    0U
#define MCHP_ESPI_PC_LC_LEN_MASK0  0x0fffU
#define MCHP_ESPI_PC_LC_LEN_MASK   0x0fffU
#define MCHP_ESPI_PC_LC_TYPE_POS   12U
#define MCHP_ESPI_PC_LC_TYPE_MASK0 0xffU
#define MCHP_ESPI_PC_LC_TYPE_MASK  (0xffU << 12)
#define MCHP_ESPI_PC_LC_TAG_POS    20U
#define MCHP_ESPI_PC_LC_TAG_MASK0  0x0fU
#define MCHP_ESPI_PC_LC_TAG_MASK   (0x0fU << 20)

/*
 * Peripheral Channel Status
 * Bus error, Channel enable change, and Bus master enable change.
 */
#define MCHP_ESPI_PC_STS_BUS_ERR_POS   16U /* RW1C */
#define MCHP_ESPI_PC_STS_BUS_ERR       BIT(MCHP_ESPI_PC_STS_BUS_ERR_POS)
#define MCHP_ESPI_PC_STS_EN_POS        24U /* RO */
#define MCHP_ESPI_PC_STS_EN            BIT(MCHP_ESPI_PC_STS_EN_POS)
#define MCHP_ESPI_PC_STS_EN_CHG_POS    25U /* RW1C */
#define MCHP_ESPI_PC_STS_EN_CHG        BIT(MCHP_ESPI_PC_STS_EN_CHG_POS)
#define MCHP_ESPI_PC_STS_BM_EN_POS     27U /* RO */
#define MCHP_ESPI_PC_STS_BM_EN         BIT(MCHP_ESPI_PC_STS_BM_EN_POS)
#define MCHP_ESPI_PC_STS_BM_EN_CHG_POS 28U /* RW1C */
#define MCHP_ESPI_PC_STS_BM_EN_CHG     BIT(MCHP_ESPI_PC_STS_BM_EN_CHG_POS)

/*
 * Peripheral Channel Interrupt Enables for
 * Bus error, Channel enable change, and Bus master enable change.
 * PC_LC_ADDR_LSW (@ 0x0000) Periph Chan Last Cycle address LSW
 * PC_LC_ADDR_MSW (@ 0x0004) Periph Chan Last Cycle address MSW
 * PC_LC_LEN_TYPE_TAG (@ 0x0008) Periph Chan Last Cycle length/type/tag
 * PC_ERR_ADDR_LSW (@ 0x000C) Periph Chan Error Address LSW
 * PC_ERR_ADDR_MSW (@ 0x0010) Periph Chan Error Address MSW
 * PC_STATUS (@ 0x0014) Periph Chan Status
 * PC_IEN (@ 0x0018) Periph Chan IEN
 */
#define MCHP_ESPI_PC_IEN_BUS_ERR_POS   16U
#define MCHP_ESPI_PC_IEN_BUS_ERR       BIT(MCHP_ESPI_PC_IEN_BUS_ERR_POS)
#define MCHP_ESPI_PC_IEN_EN_CHG_POS    25U
#define MCHP_ESPI_PC_IEN_EN_CHG        BIT(MCHP_ESPI_PC_IEN_BUS_ERR_POS)
#define MCHP_ESPI_PC_IEN_BM_EN_CHG_POS 28U
#define MCHP_ESPI_PC_IEN_BM_EN_CHG     BIT(MCHP_ESPI_PC_IEN_BUS_ERR_POS)

/*---- ESPI_IO_LTR - eSPI IO LTR registers ----*/
#define MCHP_ESPI_LTR_STS_TX_DONE_POS 0U /* RW1C */
#define MCHP_ESPI_LTR_STS_TX_DONE     BIT(MCHP_ESPI_LTR_STS_TX_DONE_POS)
#define MCHP_ESPI_LTR_STS_OVRUN_POS   3U /* RW1C */
#define MCHP_ESPI_LTR_STS_OVRUN       BIT(MCHP_ESPI_LTR_STS_OVRUN_POS)
#define MCHP_ESPI_LTR_STS_HDIS_POS    4U /* RW1C */
#define MCHP_ESPI_LTR_STS_HDIS        BIT(MCHP_ESPI_LTR_STS_HDIS_POS)
#define MCHP_ESPI_LTR_STS_TX_BUSY_POS 8U /* RO */
#define MCHP_ESPI_LTR_STS_TX_BUSY     BIT(MCHP_ESPI_LTR_STS_TX_BUSY_POS)

#define MCHP_ESPI_LTR_IEN_TX_DONE_POS 0U
#define MCHP_ESPI_LTR_IEN_TX_DONE     BIT(MCHP_ESPI_LTR_IEN_TX_DONE_POS)

#define MCHP_ESPI_LTR_CTRL_START_POS 0U
#define MCHP_ESPI_LTR_CTRL_START     BIT(MCHP_ESPI_LTR_CTRL_START_POS)
#define MCHP_ESPI_LTR_CTRL_TAG_POS   8U
#define MCHP_ESPI_LTR_CTRL_TAG_MASK0 GENMASK(3, 0)
#define MCHP_ESPI_LTR_CTRL_TAG_MASK  GENMASK(11, 8)

#define MCHP_ESPI_LTR_MSG_VAL_POS   0U
#define MCHP_ESPI_LTR_MSG_VAL_MASK0 GENMASK(9, 0)
#define MCHP_ESPI_LTR_MSG_VAL_MASK  GENMASK(9, 0)
#define MCHP_ESPI_LTR_MSG_SC_POS    10U
#define MCHP_ESPI_LTR_MSG_SC_MASK0  GENMASK(2, 0)
#define MCHP_ESPI_LTR_MSG_SC_MASK   GENMASK(12, 10)
#define MCHP_ESPI_LTR_MSG_RT_POS    13U
#define MCHP_ESPI_LTR_MSG_RT_MASK0  GENMASK(1, 0)
#define MCHP_ESPI_LTR_MSG_RT_MASK   GENMASK(14, 13)
/* eSPI specification indicates RT field must be 00b */
#define MCHP_ESPI_LTR_MSG_RT_VAL    0U
#define MCHP_ESPI_LTR_MSG_REQ_POS   15U
/* infinite latency(default) */
#define MCHP_ESPI_LTR_MSG_REQ_INF   0U
/* latency computed from VAL and SC(scale) fields */
#define MCHP_ESPI_LTR_MSG_REQ_VAL   BIT(MCHP_ESPI_LTR_MSG_REQ_POS)

/*---- ESPI_IO_OOB - eSPI IO OOB registers ----*/
#define MCHP_ESPI_OOB_RX_ADDR_LSW_MASK 0xfffffffcU
#define MCHP_ESPI_OOB_TX_ADDR_LSW_MASK 0xfffffffcU

/* OOB RX_LEN register */
/* Number of bytes received (RO) */
#define MCHP_ESPI_OOB_RX_LEN_POS       0U
#define MCHP_ESPI_OOB_RX_LEN_MASK      GENMASK(12, 0)
/* Receive buffer length field (RW) */
#define MCHP_ESPI_OOB_RX_BUF_LEN_POS   16U
#define MCHP_ESPI_OOB_RX_BUF_LEN_MASK0 GENMASK(12, 0)
#define MCHP_ESPI_OOB_RX_BUF_LEN_MASK  GENMASK(28, 16)

/* OOB TX_LEN register */
#define MCHP_ESPI_OOB_TX_MSG_LEN_POS  0U
#define MCHP_ESPI_OOB_TX_MSG_LEN_MASK GENMASK(12, 0)

/* OOB RX_CTRL */
/* Set AVAIL bit to indicate SRAM Buffer and size has been configured */
#define MCHP_ESPI_OOB_RX_CTRL_AVAIL_POS    0U /* WO */
#define MCHP_ESPI_OOB_RX_CTRL_AVAIL        BIT(MCHP_ESPI_OOB_RX_CTRL_AVAIL_POS)
#define MCHP_ESPI_OOB_RX_CTRL_CHEN_POS     9U /* RO */
#define MCHP_ESPI_OOB_RX_CTRL_CHEN         BIT(MCHP_ESPI_OOB_RX_CTRL_CHEN_POS)
/* Copy of eSPI OOB Capabilities max. payload size */
#define MCHP_ESPI_OOB_RX_CTRL_MAX_SZ_POS   16U /* RO */
#define MCHP_ESPI_OOB_RX_CTRL_MAX_SZ_MASK0 GENMASK(2, 0)
#define MCHP_ESPI_OOB_RX_CTRL_MAX_SZ_MASK  GENMASK(18, 16)

/* OOB RX_IEN */
#define MCHP_ESPI_OOB_RX_IEN_POS 0U
#define MCHP_ESPI_OOB_RX_IEN     BIT(MCHP_ESPI_OOB_RX_IEN_POS)

/* OOB RX_STS */
#define MCHP_ESPI_OOB_RX_STS_DONE_POS  0U /* RW1C */
#define MCHP_ESPI_OOB_RX_STS_DONE      BIT(MCHP_ESPI_OOB_RX_STS_DONE_POS)
#define MCHP_ESPI_OOB_RX_STS_IBERR_POS 1U /* RW1C */
#define MCHP_ESPI_OOB_RX_STS_IBERR     BIT(MCHP_ESPI_OOB_RX_STS_IBERR_POS)
#define MCHP_ESPI_OOB_RX_STS_OVRUN_POS 2U /* RW1C */
#define MCHP_ESPI_OOB_RX_STS_OVRUN     BIT(MCHP_ESPI_OOB_RX_STS_OVRUN_POS)
#define MCHP_ESPI_OOB_RX_STS_RXEN_POS  3U /* RO */
#define MCHP_ESPI_OOB_RX_STS_RXEN      BIT(MCHP_ESPI_OOB_RX_STS_RXEN_POS)
#define MCHP_ESPI_OOB_RX_STS_TAG_POS   8U /* RO */
#define MCHP_ESPI_OOB_RX_STS_TAG_MASK0 GENMASK(3, 0)
#define MCHP_ESPI_OOB_RX_STS_TAG_MASK  GENMASK(11, 8)

#define MCHP_ESPI_OOB_RX_STS_ALL_RW1C 0x07U
#define MCHP_ESPI_OOB_RX_STS_ALL      0x0fU

/* OOB TX_CTRL */
#define MCHP_ESPI_OOB_TX_CTRL_START_POS 0U /* WO */
#define MCHP_ESPI_OOB_TX_CTRL_START     BIT(MCHP_ESPI_OOB_TX_CTRL_START_POS)
#define MCHP_ESPI_OOB_TX_CTRL_TAG_POS   8U /* RW */
#define MCHP_ESPI_OOB_TX_CTRL_TAG_MASK0 GENMASK(3, 0)
#define MCHP_ESPI_OOB_TX_CTRL_TAG_MASK  GENMASK(11, 8)

/* OOB TX_IEN */
#define MCHP_ESPI_OOB_TX_IEN_DONE_POS   0U
#define MCHP_ESPI_OOB_TX_IEN_DONE       BIT(MCHP_ESPI_OOB_TX_IEN_DONE_POS)
#define MCHP_ESPI_OOB_TX_IEN_CHG_EN_POS 1U
#define MCHP_ESPI_OOB_TX_IEN_CHG_EN     BIT(MCHP_ESPI_OOB_TX_IEN_CHG_EN_POS)
#define MCHP_ESPI_OOB_TX_IEN_ALL        0x03U

/* OOB TX_STS */
#define MCHP_ESPI_OOB_TX_STS_DONE_POS   0U /* RW1C */
#define MCHP_ESPI_OOB_TX_STS_DONE       BIT(MCHP_ESPI_OOB_TX_STS_DONE_POS)
#define MCHP_ESPI_OOB_TX_STS_CHG_EN_POS 1U /* RW1C */
#define MCHP_ESPI_OOB_TX_STS_CHG_EN     BIT(MCHP_ESPI_OOB_TX_STS_CHG_EN_POS)
#define MCHP_ESPI_OOB_TX_STS_IBERR_POS  2U /* RW1C */
#define MCHP_ESPI_OOB_TX_STS_IBERR      BIT(MCHP_ESPI_OOB_TX_STS_IBERR_POS)
#define MCHP_ESPI_OOB_TX_STS_OVRUN_POS  3U /* RW1C */
#define MCHP_ESPI_OOB_TX_STS_OVRUN      BIT(MCHP_ESPI_OOB_TX_STS_OVRUN_POS)
#define MCHP_ESPI_OOB_TX_STS_BADREQ_POS 5U /* RW1C */
#define MCHP_ESPI_OOB_TX_STS_BADREQ     BIT(MCHP_ESPI_OOB_TX_STS_BADREQ_POS)
#define MCHP_ESPI_OOB_TX_STS_BUSY_POS   8U /* RO */
#define MCHP_ESPI_OOB_TX_STS_BUSY       BIT(MCHP_ESPI_OOB_TX_STS_BUSY_POS)
/* Read-only copy of OOB Channel Enabled bit */
#define MCHP_ESPI_OOB_TX_STS_CHEN_POS   9U /* RO */
#define MCHP_ESPI_OOB_TX_STS_CHEN       BIT(MCHP_ESPI_OOB_TX_STS_CHEN_POS)

#define MCHP_ESPI_OOB_TX_STS_ALL_RW1C 0x2fU

/*---- MCHP_ESPI_IO_FC - eSPI IO Flash channel registers ----*/
/* FC MEM_ADDR_LSW */
#define MCHP_ESPI_FC_MEM_ADDR_LSW_MASK GENMASK(31, 2)

/* FC CTRL */
#define MCHP_ESPI_FC_CTRL_START_POS   0U /* WO */
#define MCHP_ESPI_FC_CTRL_START       BIT(MCHP_ESPI_FC_CTRL_START_POS)
#define MCHP_ESPI_FC_CTRL_FUNC_POS    2U /* RW */
#define MCHP_ESPI_FC_CTRL_FUNC_MASK0  GENMASK(1, 0)
#define MCHP_ESPI_FC_CTRL_FUNC_MASK   GENMASK(3, 2)
#define MCHP_ESPI_FC_CTRL_RD0         0x00U
#define MCHP_ESPI_FC_CTRL_WR0         0x01U
#define MCHP_ESPI_FC_CTRL_ERS0        0x02U
#define MCHP_ESPI_FC_CTRL_ERL0        0x03U
#define MCHP_ESPI_FC_CTRL_FUNC_SET(f) FIELD_PREP(MCHP_ESPI_FC_CTRL_FUNC_MASK, (f))
#define MCHP_ESPI_FC_CTRL_FUNC_GET(r) FIELD_GET(MCHP_ESPI_FC_CTRL_FUNC_MASK, (r))

#define MCHP_ESPI_FC_CTRL_TAG_POS    4U
#define MCHP_ESPI_FC_CTRL_TAG_MASK0  GENMASK(3, 0)
#define MCHP_ESPI_FC_CTRL_TAG_MASK   GENMASK(7, 4)
#define MCHP_ESPI_FC_CTRL_TAG_SET(t) FIELD_PREP(MCHP_ESPI_FC_CTRL_TAG_MASK, (t))
#define MCHP_ESPI_FC_CTRL_TAG_GET(r) FIELD_GET(MCHP_ESPI_FC_CTRL_TAG_MASK, (r))

#define MCHP_ESPI_FC_CTRL_ABORT_POS 16U /* WO */
#define MCHP_ESPI_FC_CTRL_ABORT     BIT(MCHP_ESPI_FC_CTRL_ABORT_POS)

/* FC IEN */
#define MCHP_ESPI_FC_IEN_DONE_POS   0U
#define MCHP_ESPI_FC_IEN_DONE       BIT(MCHP_ESPI_FC_IEN_DONE_POS)
#define MCHP_ESPI_FC_IEN_CHG_EN_POS 1U
#define MCHP_ESPI_FC_IEN_CHG_EN     BIT(MCHP_ESPI_FC_IEN_CHG_EN_POS)

/* FC CFG */
#define MCHP_ESPI_FC_CFG_BUSY_POS       0U /* RO */
#define MCHP_ESPI_FC_CFG_BUSY           BIT(MCHP_ESPI_FC_CFG_BUSY_POS)
#define MCHP_ESPI_FC_CFG_ERBSZ_POS      2U /* RO */
#define MCHP_ESPI_FC_CFG_ERBSZ_MASK0    GENMASK(2, 0)
#define MCHP_ESPI_FC_CFG_ERBSZ_MASK     GENMASK(4, 2)
#define MCHP_ESPI_FC_CFG_ERBSZ_4K       FIELD_PREP(MCHP_ESPI_FC_CFG_ERBSZ_MASK, 1U)
#define MCHP_ESPI_FC_CFG_ERBSZ_64K      FIELD_PREP(MCHP_ESPI_FC_CFG_ERBSZ_MASK, 2U)
#define MCHP_ESPI_FC_CFG_ERBSZ_4K_64K   FIELD_PREP(MCHP_ESPI_FC_CFG_ERBSZ_MASK, 3U)
#define MCHP_ESPI_FC_CFG_ERBSZ_128K     FIELD_PREP(MCHP_ESPI_FC_CFG_ERBSZ_MASK, 4U)
#define MCHP_ESPI_FC_CFG_ERBSZ_256K     FIELD_PREP(MCHP_ESPI_FC_CFG_ERBSZ_MASK, 5U)
#define MCHP_ESPI_FC_CFG_MAXPLD_POS     8U /* RO */
#define MCHP_ESPI_FC_CFG_MAXPLD_MASK0   GENMASK(2, 0)
#define MCHP_ESPI_FC_CFG_MAXPLD_MASK    GENMASK(10, 8)
#define MCHP_ESPI_FC_CFG_MAXPLD_64B     FIELD_PREP(MCHP_ESPI_FC_CFG_MAXPLD_MASK, 1U)
#define MCHP_ESPI_FC_CFG_MAXPLD_128B    FIELD_PREP(MCHP_ESPI_FC_CFG_MAXPLD_MASK, 2U)
#define MCHP_ESPI_FC_CFG_MAXPLD_256B    FIELD_PREP(MCHP_ESPI_FC_CFG_MAXPLD_MASK, 3U)
#define MCHP_ESPI_FC_CFG_SAFS_SEL_POS   11U
#define MCHP_ESPI_FC_CFG_SAFS_SEL       BIT(MCHP_ESPI_FC_CFG_SAFS_SEL_POS)
#define MCHP_ESPI_FC_CFG_MAXRD_POS      12U /* RO */
#define MCHP_ESPI_FC_CFG_MAXRD_MASK0    GENMASK(2, 0)
#define MCHP_ESPI_FC_CFG_MAXRD_MASK     GENMASK(14, 12)
#define MCHP_ESPI_FC_CFG_MAXRD_64B      FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 1U)
#define MCHP_ESPI_FC_CFG_MAXRD_128B     FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 2U)
#define MCHP_ESPI_FC_CFG_MAXRD_256B     FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 3U)
#define MCHP_ESPI_FC_CFG_MAXRD_512B     FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 4U)
#define MCHP_ESPI_FC_CFG_MAXRD_1K       FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 5U)
#define MCHP_ESPI_FC_CFG_MAXRD_2K       FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 6U)
#define MCHP_ESPI_FC_CFG_MAXRD_4K       FIELD_PREP(MCHP_ESPI_FC_CFG_MAXRD_MASK, 7U)
#define MCHP_ESPI_FC_CFG_FORCE_MS_POS   28U /* RW */
#define MCHP_ESPI_FC_CFG_FORCE_MS_MASK0 GENMASK(1, 0)
#define MCHP_ESPI_FC_CFG_FORCE_MS_MASK  GENMASK(29, 28)
/* Host (eSPI Master) can select MAFS or SAFS */
#define MCHP_ESPI_FC_CFG_FORCE_NONE     0U
/* EC forces eSPI slave HW to only allow MAFS */
#define MCHP_ESPI_FC_CFG_FORCE_MAFS     FIELD_PREP(MCHP_ESPI_FC_CFG_FORCE_MS_MASK, 2U)
/* EC forces eSPI slave HW to only allow SAFS */
#define MCHP_ESPI_FC_CFG_FORCE_SAFS     FIELD_PREP(MCHP_ESPI_FC_CFG_FORCE_MS_MASK, 3U)

/* FC STS */
#define MCHP_ESPI_FC_STS_CHAN_EN_POS 0U /* RO */
#define MCHP_ESPI_FC_STS_CHAN_EN     BIT(MCHP_ESPI_FC_STS_CHAN_EN_POS)

#define MCHP_ESPI_FC_STS_CHAN_EN_CHG_POS 1U /* RW1C */
#define MCHP_ESPI_FC_STS_CHAN_EN_CHG     BIT(MCHP_ESPI_FC_STS_CHAN_EN_CHG_POS)

#define MCHP_ESPI_FC_STS_DONE_POS   2U /* RW1C */
#define MCHP_ESPI_FC_STS_DONE       BIT(MCHP_ESPI_FC_STS_DONE_POS)
#define MCHP_ESPI_FC_STS_MDIS_POS   3U /* RW1C */
#define MCHP_ESPI_FC_STS_MDIS       BIT(MCHP_ESPI_FC_STS_MDIS_POS)
#define MCHP_ESPI_FC_STS_IBERR_POS  4U /* RW1C */
#define MCHP_ESPI_FC_STS_IBERR      BIT(MCHP_ESPI_FC_STS_IBERR_POS)
#define MCHP_ESPI_FC_STS_ABS_POS    5U /* RW1C */
#define MCHP_ESPI_FC_STS_ABS        BIT(MCHP_ESPI_FC_STS_ABS_POS)
#define MCHP_ESPI_FC_STS_OVRUN_POS  6U /* RW1C */
#define MCHP_ESPI_FC_STS_OVRUN      BIT(MCHP_ESPI_FC_STS_OVRUN_POS)
#define MCHP_ESPI_FC_STS_INC_POS    7U /* RW1C */
#define MCHP_ESPI_FC_STS_INC        BIT(MCHP_ESPI_FC_STS_INC_POS)
#define MCHP_ESPI_FC_STS_FAIL_POS   8U /* RW1C */
#define MCHP_ESPI_FC_STS_FAIL       BIT(MCHP_ESPI_FC_STS_FAIL_POS)
#define MCHP_ESPI_FC_STS_OVFL_POS   9U /* RW1C */
#define MCHP_ESPI_FC_STS_OVFL       BIT(MCHP_ESPI_FC_STS_OVFL_POS)
#define MCHP_ESPI_FC_STS_BADREQ_POS 11U /* RW1C */
#define MCHP_ESPI_FC_STS_BADREQ     BIT(MCHP_ESPI_FC_STS_BADREQ_POS)

#define MCHP_ESPI_FC_STS_ALL_RW1C 0x0bfeU

/*---- MCHP_ESPI_IO_BAR_HOST - eSPI IO Host visible BAR registers ----*/

/*
 * IOBAR_INH_LSW/MSW 64-bit register: each bit = 1 inhibits an I/O BAR
 * independent of the BAR's Valid bit.
 * Logical Device Number = bit position.
 */
#define MCHP_ESPI_IOBAR_LDN_MBOX      0U
#define MCHP_ESPI_IOBAR_LDN_KBC       1U
#define MCHP_ESPI_IOBAR_LDN_ACPI_EC_0 2U
#define MCHP_ESPI_IOBAR_LDN_ACPI_EC_1 3U
#define MCHP_ESPI_IOBAR_LDN_ACPI_EC_2 4U
#define MCHP_ESPI_IOBAR_LDN_ACPI_EC_3 5U
#define MCHP_ESPI_IOBAR_LDN_ACPI_EC_4 6U
#define MCHP_ESPI_IOBAR_LDN_ACPI_PM1  7U
#define MCHP_ESPI_IOBAR_LDN_PORT92    8U
#define MCHP_ESPI_IOBAR_LDN_UART_0    9U
#define MCHP_ESPI_IOBAR_LDN_UART_1    10U
#define MCHP_ESPI_IOBAR_LDN_IOC       13U
#define MCHP_ESPI_IOBAR_LDN_MEM       14U
#define MCHP_ESPI_IOBAR_LDN_GLUE_LOG  15U
#define MCHP_ESPI_IOBAR_LDN_EMI_0     16U
#define MCHP_ESPI_IOBAR_LDN_EMI_1     17U
#define MCHP_ESPI_IOBAR_LDN_EMI_2     18U
#define MCHP_ESPI_IOBAR_LDN_RTC       20U
#define MCHP_ESPI_IOBAR_LDN_P80CAP_0  32U /* BDP Port80 Capture */
#define MCHP_ESPI_IOBAR_LDN_P80CAP_1  31U /* BDP Alias Capture */
#define MCHP_ESPI_IOBAR_LDN_T32B      47U
#define MCHP_ESPI_IOBAR_LDN_LASIC     48U

/*
 * IOBAR_INIT: Default address of I/O Plug and Play Super-IO index/data
 * configuration registers. (Defaults to 0x2E/0x2F)
 */
#define MCHP_ESPI_IOBAR_INIT_DFLT 0x2eU

/*
 * EC_IRQ: A write to bit[0] triggers EC SERIRQ. The actual
 * SERIRQ slot is configured in MCHP_ESPI_IO_SIRQ.EC_SIRQ
 */
#define MCHP_ESPI_EC_IRQ_GEN (1U << 0)

/* 32-bit Host IO BAR */
#define MCHP_ESPI_IO_BAR_HOST_VALID_POS   0U
#define MCHP_ESPI_IO_BAR_HOST_VALID       BIT(MCHP_ESPI_IO_BAR_HOST_VALID_POS)
#define MCHP_ESPI_IO_BAR_HOST_ADDR_POS    16U
#define MCHP_ESPI_IO_BAR_HOST_ADDR_MASK0  GENMASK(15, 0)
#define MCHP_ESPI_IO_BAR_HOST_ADDR_MASK   GENMASK(31, 16)
#define MCHP_ESPI_IO_BAR_HOST_ADDR_SET(a) FIELD_PREP(MCHP_ESPI_IO_BAR_HOST_ADDR_MASK, (a))
#define MCHP_ESPI_IO_BAR_HOST_ADDR_GET(r) FIELD_GET(MCHP_ESPI_IO_BAR_HOST_ADDR_MASK, (r))

/* Offsets from first SIRQ */
#define MCHP_ESPI_SIRQ_MBOX_SIRQ 0U
#define MCHP_ESPI_SIRQ_MBOX_SMI  1U
#define MCHP_ESPI_SIRQ_KBC_KIRQ  2U
#define MCHP_ESPI_SIRQ_KBC_MIRQ  3U
#define MCHP_ESPI_SIRQ_ACPI_EC0  4U
#define MCHP_ESPI_SIRQ_ACPI_EC1  5U
#define MCHP_ESPI_SIRQ_ACPI_EC2  6U
#define MCHP_ESPI_SIRQ_ACPI_EC3  7U
#define MCHP_ESPI_SIRQ_ACPI_EC4  8U
#define MCHP_ESPI_SIRQ_UART0     9U
#define MCHP_ESPI_SIRQ_UART1     10U
#define MCHP_ESPI_SIRQ_EMI0_HOST 11U
#define MCHP_ESPI_SIRQ_EMI0_E2H  12U
#define MCHP_ESPI_SIRQ_EMI1_HOST 13U
#define MCHP_ESPI_SIRQ_EMI1_E2H  14U
#define MCHP_ESPI_SIRQ_EMI2_HOST 15U
#define MCHP_ESPI_SIRQ_EMI2_E2H  16U
#define MCHP_ESPI_SIRQ_RTC       17U
#define MCHP_ESPI_SIRQ_EC        18U
#define MCHP_ESPI_SIRQ_RSVD19    19U
#define MCHP_ESPI_SIRQ_MAX       20U

/*
 * Values for Logical Device SIRQ registers.
 * Unless disabled each logical device must have a unique value
 * programmed to its SIRQ register.
 * Values 0x00u through 0x7Fu are sent using VWire host index 0x00
 * Values 0x80h through 0xFEh are sent using VWire host index 0x01
 * All registers reset default is 0xFFu (disabled).
 */
#define MCHP_ESPI_IO_SIRQ_DIS 0xFFU

/* eSPI Memory component registers */
/* BM_STS */
#define MCHP_ESPI_BM_STS_DONE_1_POS    0U /* RW1C */
#define MCHP_ESPI_BM_STS_DONE_1        BIT(MCHP_ESPI_BM_STS_DONE_1_POS)
#define MCHP_ESPI_BM_STS_BUSY_1_POS    1U /* RO */
#define MCHP_ESPI_BM_STS_BUSY_1        BIT(MCHP_ESPI_BM_STS_BUSY_1_POS)
#define MCHP_ESPI_BM_STS_AB_EC_1_POS   2U /* RW1C */
#define MCHP_ESPI_BM_STS_AB_EC_1       BIT(MCHP_ESPI_BM_STS_AB_EC_1_POS)
#define MCHP_ESPI_BM_STS_AB_HOST_1_POS 3U /* RW1C */
#define MCHP_ESPI_BM_STS_AB_HOST_1     BIT(MCHP_ESPI_BM_STS_AB_HOST_1_POS)
#define MCHP_ESPI_BM_STS_AB_CH2_1_POS  4U /* RW1C */
#define MCHP_ESPI_BM_STS_CH2_AB_1      BIT(MCHP_ESPI_BM_STS_AB_CH2_1_POS)
#define MCHP_ESPI_BM_STS_OVFL_1_POS    5U /* RW1C */
#define MCHP_ESPI_BM_STS_OVFL_1_CH2    BIT(MCHP_ESPI_BM_STS_OVFL_1_POS)
#define MCHP_ESPI_BM_STS_OVRUN_1_POS   6U /* RW1C */
#define MCHP_ESPI_BM_STS_OVRUN_1_CH2   BIT(MCHP_ESPI_BM_STS_OVRUN_1_POS)
#define MCHP_ESPI_BM_STS_INC_1_POS     7U /* RW1C */
#define MCHP_ESPI_BM_STS_INC_1         BIT(MCHP_ESPI_BM_STS_INC_1_POS)
#define MCHP_ESPI_BM_STS_FAIL_1_POS    8U /* RW1C */
#define MCHP_ESPI_BM_STS_FAIL_1        BIT(MCHP_ESPI_BM_STS_FAIL_1_POS)
#define MCHP_ESPI_BM_STS_IBERR_1_POS   9U /* RW1C */
#define MCHP_ESPI_BM_STS_IBERR_1       BIT(MCHP_ESPI_BM_STS_IBERR_1_POS)
#define MCHP_ESPI_BM_STS_BADREQ_1_POS  11U /* RW1C */
#define MCHP_ESPI_BM_STS_BADREQ_1      BIT(MCHP_ESPI_BM_STS_BADREQ_1_POS)
#define MCHP_ESPI_BM_STS_DONE_2_POS    16U /* RW1C */
#define MCHP_ESPI_BM_STS_DONE_2        BIT(MCHP_ESPI_BM_STS_DONE_2_POS)
#define MCHP_ESPI_BM_STS_BUSY_2_POS    17U /* RO */
#define MCHP_ESPI_BM_STS_BUSY_2        BIT(MCHP_ESPI_BM_STS_BUSY_2_POS)
#define MCHP_ESPI_BM_STS_AB_EC_2_POS   18U /* RW1C */
#define MCHP_ESPI_BM_STS_AB_EC_2       BIT(MCHP_ESPI_BM_STS_AB_EC_2_POS)
#define MCHP_ESPI_BM_STS_AB_HOST_2_POS 19U /* RW1C */
#define MCHP_ESPI_BM_STS_AB_HOST_2     BIT(MCHP_ESPI_BM_STS_AB_HOST_2_POS)
#define MCHP_ESPI_BM_STS_AB_CH1_2_POS  20U /* RW1C */
#define MCHP_ESPI_BM_STS_AB_CH1_2      BIT(MCHP_ESPI_BM_STS_AB_CH1_2_POS)
#define MCHP_ESPI_BM_STS_OVFL_2_POS    21U /* RW1C */
#define MCHP_ESPI_BM_STS_OVFL_2_CH2    BIT(MCHP_ESPI_BM_STS_OVFL_2_POS)
#define MCHP_ESPI_BM_STS_OVRUN_2_POS   22U /* RW1C */
#define MCHP_ESPI_BM_STS_OVRUN_CH2_2   BIT(MCHP_ESPI_BM_STS_OVRUN_2_POS)
#define MCHP_ESPI_BM_STS_INC_2_POS     23U /* RW1C */
#define MCHP_ESPI_BM_STS_INC_2         BIT(MCHP_ESPI_BM_STS_INC_2_POS)
#define MCHP_ESPI_BM_STS_FAIL_2_POS    24U /* RW1C */
#define MCHP_ESPI_BM_STS_FAIL_2        BIT(MCHP_ESPI_BM_STS_FAIL_2_POS)
#define MCHP_ESPI_BM_STS_IBERR_2_POS   25U /* RW1C */
#define MCHP_ESPI_BM_STS_IBERR_2       BIT(MCHP_ESPI_BM_STS_IBERR_2_POS)
#define MCHP_ESPI_BM_STS_BADREQ_2_POS  27U /* RW1C */
#define MCHP_ESPI_BM_STS_BADREQ_2      BIT(MCHP_ESPI_BM_STS_BADREQ_2_POS)

#define MCHP_ESPI_BM_STS_ALL_RW1C_1 0x0bfdU
#define MCHP_ESPI_BM_STS_ALL_RW1C_2 0x0bfd0000U

/* BM_IEN */
#define MCHP_ESPI_BM1_IEN_DONE_POS 0U
#define MCHP_ESPI_BM1_IEN_DONE     BIT(MCHP_ESPI_BM1_IEN_DONE_POS)
#define MCHP_ESPI_BM2_IEN_DONE_POS 16U
#define MCHP_ESPI_BM2_IEN_DONE     BIT(MCHP_ESPI_BM2_IEN_DONE_POS)

/* BM_CFG */
#define MCHP_ESPI_BM1_CFG_TAG_POS   0U
#define MCHP_ESPI_BM1_CFG_TAG_MASK0 0x0fU
#define MCHP_ESPI_BM1_CFG_TAG_MASK  0x0fU
#define MCHP_ESPI_BM2_CFG_TAG_POS   16U
#define MCHP_ESPI_BM2_CFG_TAG_MASK0 0x0fU
#define MCHP_ESPI_BM2_CFG_TAG_MASK  0x0f0000U

/* BM1_CTRL */
#define MCHP_ESPI_BM1_CTRL_START_POS       0U /* WO */
#define MCHP_ESPI_BM1_CTRL_START           BIT(MCHP_ESPI_BM1_CTRL_START_POS)
#define MCHP_ESPI_BM1_CTRL_ABORT_POS       1U /* WO */
#define MCHP_ESPI_BM1_CTRL_ABORT           BIT(MCHP_ESPI_BM1_CTRL_ABORT_POS)
#define MCHP_ESPI_BM1_CTRL_EN_INC_POS      2U /* RW */
#define MCHP_ESPI_BM1_CTRL_EN_INC          BIT(MCHP_ESPI_BM1_CTRL_EN_INC_POS)
#define MCHP_ESPI_BM1_CTRL_WAIT_NB2_POS    3U /* RW */
#define MCHP_ESPI_BM1_CTRL_WAIT_NB2        BIT(MCHP_ESPI_BM1_CTRL_WAIT_NB2_POS)
#define MCHP_ESPI_BM1_CTRL_CTYPE_POS       8U
#define MCHP_ESPI_BM1_CTRL_CTYPE_MASK0     GENMASK(1, 0)
#define MCHP_ESPI_BM1_CTRL_CTYPE_MASK      GENMASK(9, 8)
#define MCHP_ESPI_BM1_CTRL_CTYPE_RD_ADDR32 0x00U
#define MCHP_ESPI_BM1_CTRL_CTYPE_WR_ADDR32 FIELD_PREP(MCHP_ESPI_BM1_CTRL_CTYPE_MASK, 1U)
#define MCHP_ESPI_BM1_CTRL_CTYPE_RD_ADDR64 FIELD_PREP(MCHP_ESPI_BM1_CTRL_CTYPE_MASK, 2U)
#define MCHP_ESPI_BM1_CTRL_CTYPE_WR_ADDR64 FIELD_PREP(MCHP_ESPI_BM1_CTRL_CTYPE_MASK, 3U)
#define MCHP_ESPI_BM1_CTRL_LEN_POS         16U
#define MCHP_ESPI_BM1_CTRL_LEN_MASK0       GENMASK(12, 0)
#define MCHP_ESPI_BM1_CTRL_LEN_MASK        GENMASK(28, 16)

/* BM1_EC_ADDR_LSW */
#define MCHP_ESPI_BM1_EC_ADDR_LSW_MASK GENMASK(31, 2)

/* BM2_CTRL */
#define MCHP_ESPI_BM2_CTRL_START_POS       0U /* WO */
#define MCHP_ESPI_BM2_CTRL_START           BIT(MCHP_ESPI_BM2_CTRL_START_POS)
#define MCHP_ESPI_BM2_CTRL_ABORT_POS       1U /* WO */
#define MCHP_ESPI_BM2_CTRL_ABORT           BIT(MCHP_ESPI_BM2_CTRL_ABORT_POS)
#define MCHP_ESPI_BM2_CTRL_EN_INC_POS      2U /* RW */
#define MCHP_ESPI_BM2_CTRL_EN_INC          BIT(MCHP_ESPI_BM2_CTRL_EN_INC_POS)
#define MCHP_ESPI_BM2_CTRL_WAIT_NB2_POS    3U /* RW */
#define MCHP_ESPI_BM2_CTRL_WAIT_NB2        BIT(MCHP_ESPI_BM2_CTRL_WAIT_NB2_POS)
#define MCHP_ESPI_BM2_CTRL_CTYPE_POS       8U
#define MCHP_ESPI_BM2_CTRL_CTYPE_MASK0     GENMASK(1, 0)
#define MCHP_ESPI_BM2_CTRL_CTYPE_MASK      GENMASK(9, 8)
#define MCHP_ESPI_BM2_CTRL_CTYPE_RD_ADDR32 0x00U
#define MCHP_ESPI_BM2_CTRL_CTYPE_WR_ADDR32 FIELD_PREP(MCHP_ESPI_BM2_CTRL_CTYPE_MASK, 1U)
#define MCHP_ESPI_BM2_CTRL_CTYPE_RD_ADDR64 FIELD_PREP(MCHP_ESPI_BM2_CTRL_CTYPE_MASK, 2U)
#define MCHP_ESPI_BM2_CTRL_CTYPE_WR_ADDR64 FIELD_PREP(MCHP_ESPI_BM2_CTRL_CTYPE_MASK, 3U)
#define MCHP_ESPI_BM2_CTRL_LEN_POS         16U
#define MCHP_ESPI_BM2_CTRL_LEN_MASK0       GENMASK(12, 0)
#define MCHP_ESPI_BM2_CTRL_LEN_MASK        GENMASK(28, 16)

/* BM2_EC_ADDR_LSW */
#define MCHP_ESPI_BM2_EC_ADDR_LSW_MASK GENMASK(31, 2)

/*
 * MCHP_ESPI_MEM_BAR_EC @ 0x400F3930
 * Half-word H0 of each EC Memory BAR contains
 * Memory BAR memory address mask bits in bits[7:0]
 * Logical Device Number in bits[13:8]
 */
#define MCHP_ESPI_EBAR_H0_MEM_MASK_POS  0U
#define MCHP_ESPI_EBAR_H0_MEM_MASK_MASK 0xffU
#define MCHP_ESPI_EBAR_H0_LDN_POS       8U
#define MCHP_ESPI_EBAR_H0_LDN_MASK0     0x3fU
#define MCHP_ESPI_EBAR_H0_LDN_MASK      0x3f00U

/*
 * MCHP_ESPI_MEM_BAR_HOST @ 0x400F3B30
 * Each Host BAR contains:
 * bit[0] (RW) = Valid bit
 * bits[15:1] = Reserved, read-only 0
 * bits[47:16] (RW) = bits[31:0] of the Host Memory address.
 */

/* Memory BAR Host address valid */
#define MCHP_ESPI_HBAR_VALID_POS   0U
#define MCHP_ESPI_HBAR_VALID_MASK  0x01U
/*
 * Host address is in bits[47:16] of the HBAR
 * HBAR's are spaced every 10 bytes (80 bits) but
 * only implement bits[47:0]
 */
#define MCHP_ESPI_HBAR_VALID_OFS   0x00U /* byte 0 */
/* 32-bit Host Address */
#define MCHP_ESPI_HBAR_ADDR_B0_OFS 0x02U /* byte 2 */
#define MCHP_ESPI_HBAR_ADDR_B1_OFS 0x03U /* byte 3 */
#define MCHP_ESPI_HBAR_ADDR_B2_OFS 0x04U /* byte 4 */
#define MCHP_ESPI_HBAR_ADDR_B3_OFS 0x05U /* byte 5 */

/* eSPI memory component SRAM BAR register bits */
#define MCHP_ESPI_SRAM_BAR_ID0    0
#define MCHP_ESPI_SRAM_BAR_ID1    1U
#define MCHP_ESPI_SRAM_BAR_ID_MAX 2U

#define MCHP_EC_SRAM_BAR_H0_VALID_POS    0U
#define MCHP_EC_SRAM_BAR_H0_VALID_MASK0  0x01U
#define MCHP_EC_SRAM_BAR_H0_VALID_MASK   0x01U
#define MCHP_EC_SRAM_BAR_H0_VALID        0x01U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_POS   1U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_MASK0 0x03U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_MASK  0x06U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_NONE  0x00U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_RO    0x02U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_WO    0x04U
#define MCHP_EC_SRAM_BAR_H0_ACCESS_RW    0x06U
#define MCHP_EC_SRAM_BAR_H0_SIZE_POS     4U
#define MCHP_EC_SRAM_BAR_H0_SIZE_MASK0   0x0fU
#define MCHP_EC_SRAM_BAR_H0_SIZE_MASK    0xf0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_1B      0x00U
#define MCHP_EC_SRAM_BAR_H0_SIZE_2B      0x10U
#define MCHP_EC_SRAM_BAR_H0_SIZE_4B      0x20U
#define MCHP_EC_SRAM_BAR_H0_SIZE_8B      0x30U
#define MCHP_EC_SRAM_BAR_H0_SIZE_16B     0x40U
#define MCHP_EC_SRAM_BAR_H0_SIZE_32B     0x50U
#define MCHP_EC_SRAM_BAR_H0_SIZE_64B     0x60U
#define MCHP_EC_SRAM_BAR_H0_SIZE_128B    0x70U
#define MCHP_EC_SRAM_BAR_H0_SIZE_256B    0x80U
#define MCHP_EC_SRAM_BAR_H0_SIZE_512B    0x90U
#define MCHP_EC_SRAM_BAR_H0_SIZE_1KB     0xa0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_2KB     0xb0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_4KB     0xc0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_8KB     0xd0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_16KB    0xe0U
#define MCHP_EC_SRAM_BAR_H0_SIZE_32KB    0xf0U
/* EC and Host SRAM BAR start offset of EC or Host memory address */
#define MCHP_EC_SRAM_BAR_MADDR_OFS1      2U
#define MCHP_EC_SRAM_BAR_MADDR_OFS2      4U

/* Interfaces to any C modules */
#ifdef __cplusplus
extern "C" {
#endif

/* Array indices for eSPI IO BAR Host and EC-only register structures */
enum espi_io_bar_idx {
	IOB_IOC = 0,
	IOB_MEM,
	IOB_MBOX,
	IOB_KBC,
	IOB_ACPI_EC0,
	IOB_ACPI_EC1,
	IOB_ACPI_EC2,
	IOB_ACPI_EC3,
	IOB_ACPI_EC4,
	IOB_ACPI_PM1,
	IOB_PORT92,
	IOB_UART0,
	IOB_UART1,
	IOB_EMI0,
	IOB_EMI1,
	IOB_EMI2,
	IOB_P80BD,       /* MEC152x IOB_P80_CAP0 */
	IOB_P80BD_ALIAS, /* MEC152x IOB_P80_CAP1 */
	IOB_RTC,
	IOB_RSVD19,
	IOB_T32B,
	IOB_UART2,
	IOB_GLUE,
	IOB_UART3,
	IOB_MAX
};

/** @brief Serial IRQ byte register indices */
enum espi_io_sirq_idx {
	SIRQ_MBOX = 0,
	SIRQ_MBOX_SMI,
	SIRQ_KBC_KIRQ,
	SIRQ_KBC_MIRQ,
	SIRQ_ACPI_EC0_OBF,
	SIRQ_ACPI_EC1_OBF,
	SIRQ_ACPI_EC2_OBF,
	SIRQ_ACPI_EC3_OBF,
	SIRQ_ACPI_EC4_OBF,
	SIRQ_UART0,
	SIRQ_UART1,
	SIRQ_EMI0_HEV,
	SIRQ_EMI0_E2H,
	SIRQ_EMI1_HEV,
	SIRQ_EMI1_E2H,
	SIRQ_EMI2_HEV,
	SIRQ_EMI2_E2H,
	SIRQ_RTC,
	SIRQ_EC,
	SIRQ_UART2,
	SIRQ_RSVD20,
	SIRQ_UART3,
	SIRQ_MAX
};

enum espi_mem_bar_idx {
	MEMB_MBOX = 0,
	MEMB_ACPI_EC0,
	MEMB_ACPI_EC1,
	MEMB_ACPI_EC2,
	MEMB_ACPI_EC3,
	MEMB_ACPI_EC4,
	MEMB_EMI0,
	MEMB_EMI1,
	MEMB_EMI2,
	MEMB_T32B,
	MEMB_MAX
};

/* eSPI */
struct espi_io_mbar { /* 80-bit register on 16-bit alignment */
	volatile uint16_t LDN_MASK;
	volatile uint16_t RESERVED[4];
}; /* Size = 10 (0xa) */

struct espi_mbar_host { /* 80-bit register on 16-bit alignment */
	volatile uint16_t VALID;
	volatile uint16_t HADDR_LSH;
	volatile uint16_t HADDR_MSH;
	volatile uint16_t RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_bar { /* 64-bit register on 16-bit alignment */
	volatile uint16_t VACCSZ;
	volatile uint16_t EC_SRAM_BASE_LSH;
	volatile uint16_t EC_SRAM_BASE_MSH;
	volatile uint16_t RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_host_bar {      /* 64-bit register on 16-bit alignment */
	volatile uint16_t ACCSZ; /* RO copy of VACCSZ */
	volatile uint16_t HBASE_LSH;
	volatile uint16_t HBASE_MSH;
	volatile uint16_t RESERVED[2];
}; /* Size = 10 (0xa) */

/* eSPI Capabilities, I/O and Memory components in one structure
 * DEPRECATED do not use for new modifications or new eSPI drivers.
 * Refer to structures at the end of this file where we have broken
 * up this monster into functionality based structures.
 */
struct espi_iom_regs {          /* @ 0x400F3400 */
	volatile uint8_t RTIDX; /* @ 0x0000 */
	volatile uint8_t RTDAT; /* @ 0x0001 */
	volatile uint16_t RESERVED;
	volatile uint32_t RESERVED1[63];
	volatile uint32_t PCLC[3]; /* @ 0x0100 */ /* struct xec_espi_ioc_pc_regs */
	volatile uint32_t PCERR[2];               /* @ 0x010C */
	volatile uint32_t PCSTS;                  /* @ 0x0114 */
	volatile uint32_t PCIEN; /* @ 0x0118 */   /* end struct xec_espi_ioc_pc_regs */
	volatile uint32_t RESERVED2;
	volatile uint32_t PCBINH[2]; /* @ 0x0120 */  /* struct xec_espi_ioc_bar_ldm */
	volatile uint32_t PCBINIT;                   /* @ 0x0128 */
	volatile uint32_t PCECIRQ;                   /* @ 0x012C */
	volatile uint32_t PCCKNP;                    /* @ 0x0130 */
	volatile uint32_t PCBARI[29]; /* @ 0x0134 */ /* end struct xec_espi_ioc_bar_ldm */
	volatile uint32_t RESERVED3[30];
	volatile uint32_t PCLTRSTS; /* @ 0x0220 */ /* struct xec_espi_ioc_pc_ltr_regs */
	volatile uint32_t PCLTREN;                 /* @ 0x0224 */
	volatile uint32_t PCLTRCTL;                /* @ 0x0228 */
	volatile uint32_t PCLTRM; /* @ 0x022C */   /* end struct xec_espi_ioc_pc_ltr_regs */
	volatile uint32_t RESERVED4[4];
	volatile uint32_t OOBRXA[2]; /* @ 0x0240 */ /* struct xec_espi_ioc_oob_regs */
	volatile uint32_t OOBTXA[2];                /* @ 0x0248 */
	volatile uint32_t OOBRXL;                   /* @ 0x0250 */
	volatile uint32_t OOBTXL;                   /* @ 0x0254 */
	volatile uint32_t OOBRXC;                   /* @ 0x0258 */
	volatile uint32_t OOBRXIEN;                 /* @ 0x025C */
	volatile uint32_t OOBRXSTS;                 /* @ 0x0260 */
	volatile uint32_t OOBTXC;                   /* @ 0x0264 */
	volatile uint32_t OOBTXIEN;                 /* @ 0x0268 */
	volatile uint32_t OOBTXSTS; /* @ 0x026C */  /* end struct xec_espi_ioc_oob_regs */
	volatile uint32_t RESERVED5[4];
	volatile uint32_t FCFA[2]; /* @ 0x0280 */ /* struct xec_espi_ioc_fc_regs */
	volatile uint32_t FCBA[2];                /* @ 0x0288 */
	volatile uint32_t FCLEN;                  /* @ 0x0290 */
	volatile uint32_t FCCTL;                  /* @ 0x0294 */
	volatile uint32_t FCIEN;                  /* @ 0x0298 */
	volatile uint32_t FCCFG;                  /* @ 0x029C */
	volatile uint32_t FCSTS; /* @ 0x02A0 */   /* end struct xec_espi_ioc_fc_regs */
	volatile uint32_t RESERVED6[3];
	volatile uint32_t VWSTS; /* @ 0x02B0 */ /* struct xec_espi_cap_regs */
	volatile uint32_t RESERVED7[11];
	volatile uint8_t CAPID;                 /* @ 0x02E0 */
	volatile uint8_t CAP0;                  /* @ 0x02E1 */
	volatile uint8_t CAP1;                  /* @ 0x02E2 */
	volatile uint8_t CAPPC;                 /* @ 0x02E3 */
	volatile uint8_t CAPVW;                 /* @ 0x02E4 */
	volatile uint8_t CAPOOB;                /* @ 0x02E5 */
	volatile uint8_t CAPFC;                 /* @ 0x02E6 */
	volatile uint8_t PCRDY;                 /* @ 0x02E7 */
	volatile uint8_t OOBRDY;                /* @ 0x02E8 */
	volatile uint8_t FCRDY;                 /* @ 0x02E9 */
	volatile uint8_t ERIS;                  /* @ 0x02EA */
	volatile uint8_t ERIE;                  /* @ 0x02EB */
	volatile uint8_t PLTSRC;                /* @ 0x02EC */
	volatile uint8_t VWRDY;                 /* @ 0x02ED */
	volatile uint8_t SAFEBS; /* @ 0x02EE */ /* end struct xec_espi_cap_regs */
	volatile uint8_t RESERVED8;
	volatile uint32_t RESERVED9[16];
	volatile uint32_t ACTV; /* @ 0x0330 */ /* struct xec_espi_ioc_cfg_regs */
	volatile uint32_t IOHBAR[29];          /* @ 0x0334 */
	volatile uint32_t RESERVED10;
	volatile uint8_t SIRQ[19]; /* @ 0x03ac */
	volatile uint8_t RESERVED11;
	volatile uint32_t RESERVED12[12];
	volatile uint32_t VWERREN; /* @ 0x03f0 */ /* end struct xec_espi_ioc_cfg_regs */
	volatile uint32_t RESERVED13[79];
	struct espi_io_mbar MBAR[10]; /* @ 0x0530 */ /* struct xec_espi_mc_bar_cfg_regs */
	volatile uint32_t RESERVED14[6];
	struct espi_sram_bar SRAMBAR[2]; /* @ 0x05AC */ /* struct xec_espi_mc_bar_host_regs */
	volatile uint32_t RESERVED15[16];
	volatile uint32_t BM_STATUS; /* @ 0x0600 */
	volatile uint32_t BM_IEN;    /* @ 0x0604 */
	volatile uint32_t BM_CONFIG; /* @ 0x0608 */
	volatile uint32_t RESERVED16;
	volatile uint32_t BM_CTRL1;        /* @ 0x0610 */
	volatile uint32_t BM_HADDR1_LSW;   /* @ 0x0614 */
	volatile uint32_t BM_HADDR1_MSW;   /* @ 0x0618 */
	volatile uint32_t BM_EC_ADDR1_LSW; /* @ 0x061C */
	volatile uint32_t BM_EC_ADDR1_MSW; /* @ 0x0620 */
	volatile uint32_t BM_CTRL2;        /* @ 0x0624 */
	volatile uint32_t BM_HADDR2_LSW;   /* @ 0x0628 */
	volatile uint32_t BM_HADDR2_MSW;   /* @ 0x062C */
	volatile uint32_t BM_EC_ADDR2_LSW; /* @ 0x0630 */
	volatile uint32_t BM_EC_ADDR2_MSW; /* @ 0x0634 */
	volatile uint32_t RESERVED17[62];
	struct espi_mbar_host HMBAR[10]; /* @ 0x0730 */
	volatile uint32_t RESERVED18[6];
	struct espi_sram_host_bar HSRAMBAR[2]; /* @ 0x07AC */
}; /* Size = 1984 (0x7c0) */

struct xec_espi_ioc_pc_regs { /* 0x4000F400 + 0x100 */
	volatile uint32_t PCLC[3];
	volatile uint32_t PCERR[2];
	volatile uint32_t PCSTS;
	volatile uint32_t PCIEN;
};

struct xec_espi_ioc_bar_ldm { /* 0x4000F400 + 0x120 */
	volatile uint32_t PCBINH[2];
	volatile uint32_t PCBINIT;
	volatile uint32_t PCECIRQ;
	volatile uint32_t PCCKNP;
	volatile uint32_t PCBARI[29];
};

struct xec_espi_ioc_pc_ltr_regs { /* 0x4000F400 + 0x220 */
	volatile uint32_t PCLTRSTS;
	volatile uint32_t PCLTREN;
	volatile uint32_t PCLTRCTL;
	volatile uint32_t PCLTRM;
};

struct xec_espi_ioc_oob_regs { /* 0x4000F400 + 0x240 */
	volatile uint32_t OOBRXA[2];
	volatile uint32_t OOBTXA[2];
	volatile uint32_t OOBRXL;
	volatile uint32_t OOBTXL;
	volatile uint32_t OOBRXC;
	volatile uint32_t OOBRXIEN;
	volatile uint32_t OOBRXSTS;
	volatile uint32_t OOBTXC;
	volatile uint32_t OOBTXIEN;
	volatile uint32_t OOBTXSTS;
};

struct xec_espi_ioc_fc_regs { /* 0x4000F400 + 0x280 */
	volatile uint32_t FCFA[2];
	volatile uint32_t FCBA[2];
	volatile uint32_t FCLEN;
	volatile uint32_t FCCTL;
	volatile uint32_t FCIEN;
	volatile uint32_t FCCFG;
	volatile uint32_t FCSTS;
};

struct xec_espi_cap_regs { /* 0x4000F400 + 0x2B0 */
	volatile uint32_t VW_CHEN_SR;
	volatile uint32_t RESERVED7[11];
	volatile uint8_t CAPID; /* @ 0x02E0 */
	volatile uint8_t CAP0;
	volatile uint8_t CAP1;
	volatile uint8_t CAPPC;
	volatile uint8_t CAPVW;
	volatile uint8_t CAPOOB;
	volatile uint8_t CAPFC;
	volatile uint8_t PCRDY;
	volatile uint8_t OOBRDY;
	volatile uint8_t FCRDY;
	volatile uint8_t ERIS;
	volatile uint8_t ERIE;
	volatile uint8_t PLTSRC;
	volatile uint8_t VWRDY;
	volatile uint8_t TAFEBS;
};

/* Accessible by EC and Host
 * Host access is via legacy I/O index and data pair defaulting to I/O 2Eh
 * Host uses bits [7:0] of the register offset as the index.
 * For example, accessing the SIRQ registers the Host would use 0xAC - 0xC1 as the index.
 */
struct xec_espi_ioc_cfg_regs {         /* 0x4000F400 + 0x300 */
	volatile uint32_t RPMC_OP1_DC; /* @ 0x300 v1.5 only */
	volatile uint32_t RPMC_OP1_NC; /* @ 0x304 v1.5 only */
	uint32_t RESERVED1[10];        /* @ 0x308 */
	volatile uint32_t ACTV;        /* @ 0x330 */
	volatile uint32_t IOHBAR[29];  /* @ 0x334 */
	volatile uint32_t RESERVED2;
	volatile uint8_t SIRQ[24];      /* @ 0x3ac - 0x3c3 */
	volatile uint32_t RESERVED4[8]; /* @ 0x3c4 - 0x3e3 */
	volatile uint32_t RPMC_OP1_IMG; /* @ 0x3E4 RO v1.5 only */
	uint32_t RESERVED5[2];
	volatile uint32_t VWERREN; /* @ 0x03f0 */
};

/* eSPI Memory Compment @ 0x4000F3800 */

struct xec_espi_mc_bar_ldm_regs { /* 0x400F3800 + 0x130 */
	struct espi_io_mbar MBAR[MEMB_MAX];
};

struct xec_espi_mc_sram_bar_acc_regs { /* 0x400F3800 + 0x1AC */
	struct espi_sram_host_bar HSRAMBAR[MCHP_ESPI_SRAM_BAR_ID_MAX];
};

/* memory component bus master channels 1 and 2
 * Use by the EC to read/write Host memory.
 */
struct xec_espi_mc_bm_regs { /* 0x400F3800 + 0x200 */
	volatile uint32_t BM_STATUS;
	volatile uint32_t BM_IEN;
	volatile uint32_t BM_CONFIG;
	volatile uint32_t RESERVED1;
	volatile uint32_t BM_CTRL1;
	volatile uint32_t BM_HADDR1_LSW;
	volatile uint32_t BM_HADDR1_MSW;
	volatile uint32_t BM_EC_ADDR1_LSW;
	volatile uint32_t BM_EC_ADDR1_MSW;
	volatile uint32_t BM_CTRL2;
	volatile uint32_t BM_HADDR2_LSW;
	volatile uint32_t BM_HADDR2_MSW;
	volatile uint32_t BM_EC_ADDR2_LSW;
	volatile uint32_t BM_EC_ADDR2_MSW;
};

/* eSPI memory component configuration
 * These registers are accessible by EC and Host.
 * BAR registers contain Host addresses for peripheral channel devices which implement memory BARs.
 * SRAM register contain Host address to map each SRAM bar.
 * HBAR_EXT and SRAM_EXT are 16-bit register containing Host address bits[47:32] for all peripheral
 * memory BARs and SRAM BARs respectively.
 */
struct xec_espi_mc_cfg_regs {                  /* 0x400F3800 + 0x330 */
	struct espi_mbar_host HMBAR[MEMB_MAX]; /* @ 0x330 - 0x393 */
	uint32_t RESERVED1[5];                 /* @ 0x394 - 0x3A7 */
	volatile uint16_t HBAR_EXT;            /* @ 0x3A8 */
	uint16_t RESERVED2;
	struct espi_sram_host_bar HSRAMBAR[MCHP_ESPI_SRAM_BAR_ID_MAX]; /* @ 0x3AC - 0x3BF */
	uint32_t RESERVED3[15];                                        /* @ 0x3C0 - 0x3FB */
	volatile uint16_t SRAM_EXT;                                    /* @ 0x3FC */
};

/* ---- Helpers ---- */

static inline int xec_espi_mbar_host_set(mm_reg_t espi_mc_base, uint8_t mc_bar_host_idx,
					 uint32_t host_addr_lsw, bool enable)
{
	struct xec_espi_mc_cfg_regs *cfgregs =
		(struct xec_espi_mc_cfg_regs *)(espi_mc_base + MCHP_ESPI_MC_CFG_OFS);
	struct espi_mbar_host *bregs = NULL;

	if (mc_bar_host_idx >= MEMB_MAX) {
		return -EINVAL;
	}

	bregs = &cfgregs->HMBAR[mc_bar_host_idx];

	bregs->VALID &= ~BIT(MCHP_ESPI_HBAR_VALID_POS);
	bregs->HADDR_LSH = (uint16_t)host_addr_lsw;
	bregs->HADDR_MSH = (uint16_t)(host_addr_lsw >> 16);
	if (enable == true) {
		bregs->VALID |= BIT(MCHP_ESPI_HBAR_VALID_POS);
	}

	return 0;
};

#ifdef __cplusplus
}
#endif

#endif /* _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_MEC_ESPI_IOM_V2_H */
