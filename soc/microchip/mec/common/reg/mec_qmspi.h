/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_QMSPI_H
#define ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_QMSPI_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#define XEC_QSPI_MAX_CS 2

/* Mode register */
#define XEC_QSPI_MODE_OFS             0
#define XEC_QSPI_MODE_ACTV_POS        0
#define XEC_QSPI_MODE_SRST_POS        1
#define XEC_QSPI_MODE_TAF_POS         2
#define XEC_QSPI_MODE_LD_RX_EN_POS    3
#define XEC_QSPI_MODE_LD_TX_EN_POS    4
#define XEC_QSPI_MODE_CPOL_HI_POS     8
#define XEC_QSPI_MODE_CPHA_SDI_FE_POS 9
#define XEC_QSPI_MODE_CPHA_SDO_SE_POS 10
#define XEC_QSPI_MODE_CS_SEL_POS      12
#define XEC_QSPI_MODE_CS_SEL_MSK      GENMASK(13, 12)
#define XEC_QSPI_MODE_CS_SEL_0        0
#define XEC_QSPI_MODE_CS_SEL_1        1u
#define XEC_QSPI_MODE_CS_SEL_SET(cs)  FIELD_PREP(XEC_QSPI_MODE_CS_SEL_MSK, (cs))
#define XEC_QSPI_MODE_CS_SEL_GET(r)   FIELD_GET(XEC_QSPI_MODE_CS_SEL_MSK, (r))
#define XEC_QSPI_MODE_CK_DIV_POS      16
#define XEC_QSPI_MODE_CK_DIV_MSK      GENMASK(31, 16)
#define XEC_QSPI_MODE_CK_DIV_SET(d)   FIELD_PREP(XEC_QSPI_MODE_CK_DIV_MSK, (d))
#define XEC_QSPI_MODE_CK_DIV_GET(r)   FIELD_GET(XEC_QSPI_MODE_CK_DIV_MSK, (r))

/* CPOL, CPHA_SDI, and CPHA_SDO bit mask */
#define XEC_QSPI_MODE_CP_MSK 0x700u

/* Control register */
#define XEC_QSPI_CR_OFS            4u
#define XEC_QSPI_CR_IFM_POS        0
#define XEC_QSPI_CR_IFM_MSK        GENMASK(1, 0)
#define XEC_QSPI_CR_IFM_FD         0
#define XEC_QSPI_CR_IFM_DUAL       1u
#define XEC_QSPI_CR_IFM_QUAD       2u
#define XEC_QSPI_CR_IFM_SET(m)     FIELD_PREP(XEC_QSPI_CR_IFM_MSK, (m))
#define XEC_QSPI_CR_IFM_GET(r)     FIELD_GET(XEC_QSPI_CR_IFM_MSK, (r))
#define XEC_QSPI_CR_TXM_POS        2
#define XEC_QSPI_CR_TXM_MSK        GENMASK(3, 2)
#define XEC_QSPI_CR_TXM_DIS        0
#define XEC_QSPI_CR_TXM_DATA       1u
#define XEC_QSPI_CR_TXM_ZEROS      2u
#define XEC_QSPI_CR_TXM_ONES       3u
#define XEC_QSPI_CR_TXM_SET(tm)    FIELD_PREP(XEC_QSPI_CR_TXM_MSK, (tm))
#define XEC_QSPI_CR_TXM_GET(r)     FIELD_GET(XEC_QSPI_CR_TXM_MSK, (r))
#define XEC_QSPI_CR_TXDMA_POS      4
#define XEC_QSPI_CR_TXDMA_MSK      GENMASK(5, 4)
#define XEC_QSPI_CR_TXDMA_DIS      0
#define XEC_QSPI_CR_TXDMA_CD1B     1u
#define XEC_QSPI_CR_TXDMA_CD2B     2u
#define XEC_QSPI_CR_TXDMA_CD4B     4u
#define XEC_QSPI_CR_TXDMA_TLDCH0   1u
#define XEC_QSPI_CR_TXDMA_TLDCH1   2u
#define XEC_QSPI_CR_TXDMA_TLDCH2   3u
#define XEC_QSPI_CR_TXDMA_SET(m)   FIELD_PREP(XEC_QSPI_CR_TXDMA_MSK, (m))
#define XEC_QSPI_CR_TXDMA_GET(r)   FIELD_GET(XEC_QSPI_CR_TXDMA_MSK, (r))
#define XEC_QSPI_CR_RX_EN_POS      6
#define XEC_QSPI_CR_RXDMA_POS      7
#define XEC_QSPI_CR_RXDMA_MSK      GENMASK(8, 7)
#define XEC_QSPI_CR_RXDMA_DIS      0
#define XEC_QSPI_CR_RXDMA_CD1B     1u
#define XEC_QSPI_CR_RXDMA_CD2B     2u
#define XEC_QSPI_CR_RXDMA_CD4B     3u
#define XEC_QSPI_CR_RXDMA_RLDCH0   1u
#define XEC_QSPI_CR_RXDMA_RLDCH1   2u
#define XEC_QSPI_CR_RXDMA_RLDCH2   3u
#define XEC_QSPI_CR_RXDMA_SET(m)   FIELD_PREP(XEC_QSPI_CR_RXDMA_MSK, (m))
#define XEC_QSPI_CR_RXDMA_GET(r)   FIELD_GET(XEC_QSPI_CR_RXDMA_MSK, (r))
#define XEC_QSPI_CR_CLOSE_EN_POS   9
#define XEC_QSPI_CR_QUNIT_POS      10
#define XEC_QSPI_CR_QUNIT_MSK      GENMASK(11, 10)
#define XEC_QSPI_CR_QUNIT_BITS     0
#define XEC_QSPI_CR_QUNIT_1B       1u
#define XEC_QSPI_CR_QUNIT_4B       2u
#define XEC_QSPI_CR_QUNIT_16B      3u
#define XEC_QSPI_CR_QUNIT_SET(u)   FIELD_PREP(XEC_QSPI_CR_QUNIT_MSK, (u))
#define XEC_QSPI_CR_QUNIT_GET(r)   FIELD_GET(XEC_QSPI_CR_QUNIT_MSK, (r))
#define XEC_QSPI_CR_FD_POS         12
#define XEC_QSPI_CR_FD_MSK         GENMASK(15, 12)
#define XEC_QSPI_CR_FD_SET(fd)     FIELD_PREP(XEC_QSPI_CR_FD_MSK, (fd))
#define XEC_QSPI_CR_FD_GET(r)      FIELD_GET(XEC_QSPI_CR_FD_MSK, (r))
#define XEC_QSPI_CR_DESCR_EN_POS   16
#define XEC_QSPI_CR_NQUNITS_POS    17
#define XEC_QSPI_CR_NQUNITS_MSK    GENMASK(31, 17)
#define XEC_QSPI_CR_NQUNITS_SET(n) FIELD_PREP(XEC_QSPI_CR_NQUNITS_MSK, (n))
#define XEC_QSPI_CR_NQUNITS_GET(r) FIELD_GET(XEC_QSPI_CR_NQUNITS_MSK, (r))

/* Execute register: write-only */
#define XEC_QSPI_EXE_OFS           8u
#define XEC_QSPI_EXE_START_POS     0
#define XEC_QSPI_EXE_STOP_POS      1
#define XEC_QSPI_EXE_CLR_FIFOS_POS 2

/* Interface control */
#define XEC_QSPI_IFCR_OFS             0xcu
#define XEC_QSPI_IFCR_WP_HI_POS       0
#define XEC_QSPI_IFCR_WP_OUT_EN_POS   1
#define XEC_QSPI_IFCR_HOLD_HI_POS     2
#define XEC_QSPI_IFCR_HOLD_OUT_EN_POS 3

/* Status register all bits are read/write-1-to-clear except marked otherwise */
#define XEC_QSPI_SR_OFS             0x10u
#define XEC_QSPI_SR_XFR_DONE_POS    0
#define XEC_QSPI_SR_DMA_DONE_POS    1
#define XEC_QSPI_SR_TXB_ERR_POS     2
#define XEC_QSPI_SR_RXB_ERR_POS     3
#define XEC_QSPI_SR_PROG_ERR_POS    4
#define XEC_QSPI_SR_LDMA_RX_ERR_POS 5 /* read-only */
#define XEC_QSPI_SR_LDMA_TX_ERR_POS 6 /* read-only */
#define XEC_QSPI_SR_TXB_FULL_POS    8 /* read-only */
#define XEC_QSPI_SR_TXB_EMPTY_POS   9 /* read-only */
#define XEC_QSPI_SR_TXB_REQ_POS     10
#define XEC_QSPI_SR_TXB_STALL_POS   11
#define XEC_QSPI_SR_RXB_FULL_POS    12 /* read-only */
#define XEC_QSPI_SR_RXB_EMPTY_POS   13 /* read-only */
#define XEC_QSPI_SR_RXB_REQ_POS     14
#define XEC_QSPI_SR_RXB_STALL_POS   15
#define XEC_QSPI_SR_CS_ASSERTED_POS 16 /* read-only */
#define XEC_QSPI_SR_CDESCR_POS      24
#define XEC_QSPI_SR_CDESCR_MSK      GENMASK(27, 24) /* read-only */
#define XEC_QSPI_SR_CDESCR_GET(r)   FIELD_GET(XEC_QSPI_SR_CDESCR_MSK, (r))

/* Buffer count register: read-only */
#define XEC_QSPI_BCNT_SR_OFS        0x14u
#define XEC_QSPI_BCNT_SR_TXB_POS    0
#define XEC_QSPI_BCNT_SR_TXB_MSK    GENMASK(15, 0)
#define XEC_QSPI_BCNT_SR_TXB_GET(r) FIELD_GET(XEC_QSPI_BCNT_SR_TXB_MSK, (r))
#define XEC_QSPI_BCNT_SR_RXB_POS    16
#define XEC_QSPI_BCNT_SR_RXB_MSK    GENMASK(31, 16)
#define XEC_QSPI_BCNT_SR_RXB_GET(r) FIELD_GET(XEC_QSPI_BCNT_SR_RXB_MSK, (r))

/* Interrupt enable register */
#define XEC_QSPI_IER_OFS             0x18u
#define XEC_QSPI_IER_XFR_DONE_POS    0
#define XEC_QSPI_IER_DMA_DONE_POS    1
#define XEC_QSPI_IER_TXB_ERR_POS     2
#define XEC_QSPI_IER_RXB_ERR_POS     3
#define XEC_QSPI_IER_PROG_ERR_POS    4
#define XEC_QSPI_IER_LDMA_RX_ERR_POS 5
#define XEC_QSPI_IER_LDMA_TX_ERR_POS 6
#define XEC_QSPI_IER_TXB_FULL_POS    8
#define XEC_QSPI_IER_TXB_EMPTY_POS   9
#define XEC_QSPI_IER_TXB_REQ_POS     10
#define XEC_QSPI_IER_RXB_FULL_POS    12
#define XEC_QSPI_IER_RXB_EMPTY_POS   13
#define XEC_QSPI_IER_RXB_REQ_POS     14

/* Buffer count trigger register */
#define XEC_QSPI_BCNT_TR_OFS        0x1cu
#define XEC_QSPI_BCNT_TR_TXB_POS    0
#define XEC_QSPI_BCNT_TR_TXB_MSK    GENMASK(15, 0)
#define XEC_QSPI_BCNT_TR_TXB_SET(n) FIELD_PREP(XEC_QSPI_BCNT_TR_TXB_MSK, (n))
#define XEC_QSPI_BCNT_TR_TXB_GET(r) FIELD_GET(XEC_QSPI_BCNT_TR_TXB_MSK, (r))
#define XEC_QSPI_BCNT_TR_RXB_POS    16
#define XEC_QSPI_BCNT_TR_RXB_MSK    GENMASK(31, 16)
#define XEC_QSPI_BCNT_TR_RXB_SET(n) FIELD_PREP(XEC_QSPI_BCNT_TR_RXB_MSK, (n))
#define XEC_QSPI_BCNT_TR_RXB_GET(r) FIELD_GET(XEC_QSPI_BCNT_TR_RXB_MSK, (r))

/* TX and RX buffer(FIFO) registers */
#define XEC_QSPI_TXB_OFS 0x20u /* write-only */
#define XEC_QSPI_RXB_OFS 0x24u /* read-only */

/* Chip select timing register */
#define XEC_QSPI_CSTM_OFS            0x28u
/* delay from CS assertion to first clock edge */
#define XEC_QSPI_CSTM_CSA_FCK_POS    0
#define XEC_QSPI_CSTM_CSA_FCK_MSK    GENMASK(3, 0)
#define XEC_QSPI_CSTM_CSA_FCK_DFLT   6u
#define XEC_QSPI_CSTM_CSA_FCK_SET(d) FIELD_PREP(XEC_QSPI_CSTM_CSA_FCK_MSK, (d))
#define XEC_QSPI_CSTM_CSA_FCK_GET(r) FIELD_GET(XEC_QSPI_CSTM_CSA_FCK_MSK, (r))
/* delay from last clock edge to CS de-assertion */
#define XEC_QSPI_CSTM_LCK_CSD_POS    8
#define XEC_QSPI_CSTM_LCK_CSD_MSK    GENMASK(11, 8)
#define XEC_QSPI_CSTM_LCK_CSD_DFLT   4u
#define XEC_QSPI_CSTM_LCK_CSD_SET(d) FIELD_PREP(XEC_QSPI_CSTM_LCK_CSD_MSK, (d))
#define XEC_QSPI_CSTM_LCK_CSD_GET(r) FIELD_GET(XEC_QSPI_CSTM_LCK_CSD_MSK, (r))
/* last data hold delay */
#define XEC_QSPI_CSTM_LDHD_POS       16
#define XEC_QSPI_CSTM_LDHD_MSK       GENMASK(19, 16)
#define XEC_QSPI_CSTM_LDHD_DFLT      6u
#define XEC_QSPI_CSTM_LDHD_SET(d)    FIELD_PREP(XEC_QSPI_CSTM_LDHD_MSK, (d))
#define XEC_QSPI_CSTM_LDHD_GET(r)    FIELD_GET(XEC_QSPI_CSTM_LDHD_MSK, (r))
/* CS de-assertion to CS assertion */
#define XEC_QSPI_CSTM_CSD_CSA_POS    24
#define XEC_QSPI_CSTM_CSD_CSA_MSK    GENMASK(31, 24)
#define XEC_QSPI_CSTM_CSD_CSA_DFLT   6u
#define XEC_QSPI_CSTM_CSD_CSA_SET(d) FIELD_PREP(XEC_QSPI_CSTM_CSD_CSA_MSK, (d))
#define XEC_QSPI_CSTM_CSD_CSA_GET(r) FIELD_GET(XEC_QSPI_CSTM_CSD_CSA_MSK, (r))

/* Description mode registers
 * Fields are the same as the control register except for
 * bits[15:12] is next descriptor index
 * bit[16] is last descriptor flag
 */
#define XEC_QSPI_MAX_DESCR_IDX 16
#define XEC_QSPI_DESCR_OFS(n)  (0x30u + ((uint32_t)(n) * 4u))
#define XEC_QSPI_DR_ND_POS     12
#define XEC_QSPI_DR_ND_MSK     GENMASK(15, 12)
#define XEC_QSPI_DR_ND_SET(fd) FIELD_PREP(XEC_QSPI_DR_ND_MSK, (fd))
#define XEC_QSPI_DR_ND_GET(r)  FIELD_GET(XEC_QSPI_DR_ND_MSK, (r))
#define XEC_QSPI_DR_LD_POS     16

/* ---- All register past this point are for MEC172x and onwards ---- */

/* Alternate clock divider for chip select 1 */
#define XEC_QSPI_ALTM1_OFS              0xc0u
#define XEC_QSPI_ALTM1_EN_POS           0
#define XEC_QSPI_ALTM1_CS1_CKDIV_POS    16
#define XEC_QSPI_ALTM1_CS1_CKDIV_MSK    GENMASK(31, 16)
#define XEC_QSPI_ALTM1_CS1_CKDIV_SET(d) FIELD_PREP(XEC_QSPI_ALTM1_CS1_CKDIV_MSK, (d))
#define XEC_QSPI_ALTM1_CS1_CKDIV_GET(r) FIELD_GET(XEC_QSPI_ALTM1_CS1_CKDIV_MSK, (r))

/* ---- Local DMA exists in QMSPI for MEC172x and onwards ---- */

/* TAPS register */
#define XEC_QSPI_TAPS_OFS           0xd0u
#define XEC_QSPI_TAPS_CK_SEL_POS    0
#define XEC_QSPI_TAPS_CK_SEL_MSK    GENMASK(7, 0)
#define XEC_QSPI_TAPS_CK_SEL_SET(t) FIELD_PREP(XEC_QSPI_TAPS_CK_SEL_MSK, (t))
#define XEC_QSPI_TAPS_CK_SEL_GET(r) FIELD_GET(XEC_QSPI_TAPS_CK_SEL_MSK, (r))
#define XEC_QSPI_TAPS_CR_SEL_POS    8
#define XEC_QSPI_TAPS_CR_SEL_MSK    GENMASK(15, 8)
#define XEC_QSPI_TAPS_CR_SEL_SET(t) FIELD_PREP(XEC_QSPI_TAPS_CR_SEL_MSK, (t))
#define XEC_QSPI_TAPS_CR_SEL_GET(r) FIELD_GET(XEC_QSPI_TAPS_CR_SEL_MSK, (r))

/* TAPS adjust register */
#define XEC_QSPI_TAPS_ADJ_OFS       0xd4u
#define XEC_QSPI_TAPS_ADJ_CK_POS    0
#define XEC_QSPI_TAPS_ADJ_CK_MSK    GENMASK(7, 0)
#define XEC_QSPI_TAPS_ADJ_CK_SET(a) FIELD_PREP(XEC_QSPI_TAPS_ADJ_CK_MSK, (a))
#define XEC_QSPI_TAPS_ADJ_CK_GET(r) FIELD_GET(XEC_QSPI_TAPS_ADJ_CK_MSK, (r))
#define XEC_QSPI_TAPS_ADJ_CR_POS    8
#define XEC_QSPI_TAPS_ADJ_CR_MSK    GENMASK(15, 8)
#define XEC_QSPI_TAPS_ADJ_CR_SET(a) FIELD_PREP(XEC_QSPI_TAPS_ADJ_CR_MSK, (a))
#define XEC_QSPI_TAPS_ADJ_CR_GET(r) FIELD_GET(XEC_QSPI_TAPS_ADJ_CR_MSK, (r))

#define XEC_QSPI_TAPS_CR2_OFS 0xd8u

/* Local DMA RX enable descriptor bit map register */
#define XEC_QSPI_LDMA_RX_EN_OFS 0x100u

/* Local DMA TX enable descriptor bit map register */
#define XEC_QSPI_LDMA_TX_EN_OFS 0x104u

/* Local DMA channels: first 3 are HW dedicated to RX, second 3 are HW dedicated to TX */
#define XEC_QSPI_LDMA_RX_CH0  0
#define XEC_QSPI_LDMA_RX_CH1  1u
#define XEC_QSPI_LDMA_RX_CH2  2u
#define XEC_QSPI_LDMA_TX_CH0  3u
#define XEC_QSPI_LDMA_TX_CH1  4u
#define XEC_QSPI_LDMA_TX_CH2  5u
#define XEC_QSPI_LDMA_CHX_MAX 6u

/* Local DMA channel control register */
#define XEC_QSPI_LDMA_CHX_CR_OFS(x)    (0x110u + ((uint32_t)(x) * 0x10u))
#define XEC_QSPI_LDMA_CHX_CR_EN_POS    0 /* channel enable */
#define XEC_QSPI_LDMA_CHX_CR_RSE_POS   1 /* re-enable channel on completion */
#define XEC_QSPI_LDMA_CHX_CR_RSA_POS   2 /* restore original start address on completion */
#define XEC_QSPI_LDMA_CHX_CR_OVRL_POS  3 /* use channel length instead of descriptor QUNITS */
#define XEC_QSPI_LDMA_CHX_CR_SZ_POS    4 /* access size */
#define XEC_QSPI_LDMA_CHX_CR_SZ_MSK    GENMASK(5, 4)
#define XEC_QSPI_LDMA_CHX_CR_SZ_1B     0
#define XEC_QSPI_LDMA_CHX_CR_SZ_2B     1u
#define XEC_QSPI_LDMA_CHX_CR_SZ_4B     2u
#define XEC_QSPI_LDMA_CHX_CR_SZ_SET(s) FIELD_PREP(XEC_QSPI_LDMA_CHX_CR_SZ_MSK, (s))
#define XEC_QSPI_LDMA_CHX_CR_SZ_GET(r) FIELD_GET(XEC_QSPI_LDMA_CHX_CR_SZ_MSK, (r))
#define XEC_QSPI_LDMA_CHX_CR_INCRA_POS 6 /* increment start address */

/* Local DMA channel memory buffer start address register b[31:0] */
#define XEC_QSPI_LDMA_CHX_SA_OFS(x) (0x114u + ((uint32_t)(x) * 0x10u))

/* Local DMA channel data length in bytes register b[31:0] */
#define XEC_QSPI_LDMA_CHX_LR_OFS(x) (0x118u + ((uint32_t)(x) * 0x10u))

#endif /* ZEPHYR_SOC_MICROCHIP_MEC_COMMON_REG_QMSPI_H */
