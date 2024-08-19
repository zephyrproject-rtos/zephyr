/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_qspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_qspi_memc, CONFIG_MEMC_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>

#include "memc_nxp_s32_qspi.h"

/* Module Configuration Register */
#define QSPI_MCR                              0x0
#define QSPI_MCR_SWRSTSD_MASK                 BIT(0)
#define QSPI_MCR_SWRSTSD(v)                   FIELD_PREP(QSPI_MCR_SWRSTSD_MASK, (v))
#define QSPI_MCR_SWRSTHD_MASK                 BIT(1)
#define QSPI_MCR_SWRSTHD(v)                   FIELD_PREP(QSPI_MCR_SWRSTHD_MASK, (v))
#define QSPI_MCR_DQS_OUT_EN_MASK              BIT(4)
#define QSPI_MCR_DQS_OUT_EN(v)                FIELD_PREP(QSPI_MCR_DQS_OUT_EN_MASK, (v))
#define QSPI_MCR_DQS_EN_MASK                  BIT(6)
#define QSPI_MCR_DQS_EN(v)                    FIELD_PREP(QSPI_MCR_DQS_EN_MASK, (v))
#define QSPI_MCR_DDR_EN_MASK                  BIT(7)
#define QSPI_MCR_DDR_EN(v)                    FIELD_PREP(QSPI_MCR_DDR_EN_MASK, (v))
#define QSPI_MCR_VAR_LAT_EN_MASK              BIT(8)
#define QSPI_MCR_VAR_LAT_EN(v)                FIELD_PREP(QSPI_MCR_VAR_LAT_EN_MASK, (v))
#define QSPI_MCR_CLR_RXF_MASK                 BIT(10)
#define QSPI_MCR_CLR_RXF(v)                   FIELD_PREP(QSPI_MCR_CLR_RXF_MASK, (v))
#define QSPI_MCR_CLR_TXF_MASK                 BIT(11)
#define QSPI_MCR_CLR_TXF(v)                   FIELD_PREP(QSPI_MCR_CLR_TXF_MASK, (v))
#define QSPI_MCR_DLPEN_MASK                   BIT(12)
#define QSPI_MCR_DLPEN(v)                     FIELD_PREP(QSPI_MCR_DLPEN_MASK, (v))
#define QSPI_MCR_MDIS_MASK                    BIT(14)
#define QSPI_MCR_MDIS(v)                      FIELD_PREP(QSPI_MCR_MDIS_MASK, (v))
#define QSPI_MCR_ISD2_MASK(n)                 (BIT(16) * BIT(2 * (n)))
#define QSPI_MCR_ISD2(n, v)                   FIELD_PREP(QSPI_MCR_ISD2_MASK(n), (v))
#define QSPI_MCR_ISD3_MASK(n)                 (BIT(17) * BIT(2 * (n)))
#define QSPI_MCR_ISD3(n, v)                   FIELD_PREP(QSPI_MCR_ISD3_MASK(n), (v))
#define QSPI_MCR_DQS_SEL_MASK(n)              (GENMASK(25, 24) * BIT(4 * (n)))
#define QSPI_MCR_DQS_SEL(n, v)                FIELD_PREP(QSPI_MCR_DQS_SEL_MASK(n), (v))
#define QSPI_MCR_CKN_EN_MASK(n)               (BIT(26) * BIT(4 * (n)))
#define QSPI_MCR_CKN_EN(n, v)                 FIELD_PREP(QSPI_MCR_CKN_EN_MASK(n), (v))
#define QSPI_MCR_CK2_DCARS_MASK(n)            (BIT(27) * BIT(4 * (n)))
#define QSPI_MCR_CK2_DCARS_FX(n, v)           FIELD_PREP(QSPI_MCR_CK2_DCARS_MASK(n), (v))
/* IP Configuration Register */
#define QSPI_IPCR                             0x8
#define QSPI_IPCR_IDATSZ_MASK                 GENMASK(15, 0)
#define QSPI_IPCR_IDATSZ(v)                   FIELD_PREP(QSPI_IPCR_IDATSZ_MASK, (v))
#define QSPI_IPCR_SEQID_MASK                  GENMASK(27, 24)
#define QSPI_IPCR_SEQID(v)                    FIELD_PREP(QSPI_IPCR_SEQID_MASK, (v))
/* Flash Memory Configuration Register */
#define QSPI_FLSHCR                           0xc
#define QSPI_FLSHCR_TCSS_MASK                 GENMASK(3, 0)
#define QSPI_FLSHCR_TCSS(v)                   FIELD_PREP(QSPI_FLSHCR_TCSS_MASK, (v))
#define QSPI_FLSHCR_TCSH_MASK                 GENMASK(11, 8)
#define QSPI_FLSHCR_TCSH(v)                   FIELD_PREP(QSPI_FLSHCR_TCSH_MASK, (v))
#define QSPI_FLSHCR_TDH_MASK                  GENMASK(17, 16)
#define QSPI_FLSHCR_TDH(v)                    FIELD_PREP(QSPI_FLSHCR_TDH_MASK, (v))
/* Buffer n Configuration Register */
#define QSPI_BUFCR(n)                         (0x10 + 0x4 * (n))
#define QSPI_BUFCR_MSTRID_MASK                GENMASK(5, 0)
#define QSPI_BUFCR_MSTRID(v)                  FIELD_PREP(QSPI_BUFCR_MSTRID_MASK, (v))
#define QSPI_BUFCR_ADATSZ_MASK                GENMASK(17, 8)
#define QSPI_BUFCR_ADATSZ(v)                  FIELD_PREP(QSPI_BUFCR_ADATSZ_MASK, (v))
#define QSPI_BUFCR_ALLMST_MASK                BIT(31)
#define QSPI_BUFCR_ALLMST(v)                  FIELD_PREP(QSPI_BUFCR_ALLMST_MASK, (v))
/* Buffer Generic Configuration Register */
#define QSPI_BFGENCR                          0x20
#define QSPI_BFGENCR_SEQID_MASK               GENMASK(15, 12)
#define QSPI_BFGENCR_SEQID(v)                 FIELD_PREP(QSPI_BFGENCR_SEQID_MASK, (v))
#define QSPI_BFGENCR_SEQID_WR_EN_MASK         BIT(17)
#define QSPI_BFGENCR_SEQID_WR_EN(v)           FIELD_PREP(QSPI_BFGENCR_SEQID_WR_EN_MASK, (v))
#define QSPI_BFGENCR_SEQID_WR_MASK            GENMASK(31, 28)
#define QSPI_BFGENCR_SEQID_WR(v)              FIELD_PREP(QSPI_BFGENCR_SEQID_WR_MASK, (v))
/* SOC Configuration Register */
#define QSPI_SOCCR                            0x24
#define QSPI_SOCCR_SOCCFG_MASK                GENMASK(31, 0)
#define QSPI_SOCCR_SOCCFG(v)                  FIELD_PREP(QSPI_SOCCR_SOCCFG_MASK, (v))
/* Buffer n Top Index Register */
#define QSPI_BUFIND(n)                        (0x30 + 0x4 * (n))
#define QSPI_BUFIND_TPINDX_MASK               GENMASK(10, 3)
#define QSPI_BUFIND_TPINDX(v)                 FIELD_PREP(QSPI_BUFIND_TPINDX_MASK, (v))
/* AHB Write Configuration Register */
#define QSPI_AWRCR                            0x50
#define QSPI_AWRCR_AWTRGLVL_MASK              GENMASK(7, 0)
#define QSPI_AWRCR_AWTRGLVL(v)                FIELD_PREP(QSPI_AWRCR_AWTRGLVL_MASK, (v))
#define QSPI_AWRCR_PPW_RD_DIS_MASK            BIT(14)
#define QSPI_AWRCR_PPW_RD_DIS(v)              FIELD_PREP(QSPI_AWRCR_PPW_RD_DIS_MASK, (v))
#define QSPI_AWRCR_PPW_WR_DIS_MASK            BIT(15)
#define QSPI_AWRCR_PPW_WR_DIS(v)              FIELD_PREP(QSPI_AWRCR_PPW_WR_DIS_MASK, (v))
/* DLL Flash Memory Port n Configuration Register */
#define QSPI_DLLCR(n)                         (0x60 + 0x4 * (n))
#define QSPI_DLLCR_SLV_UPD_MASK               BIT(0)
#define QSPI_DLLCR_SLV_UPD(v)                 FIELD_PREP(QSPI_DLLCR_SLV_UPD_MASK, (v))
#define QSPI_DLLCR_SLV_DLL_BYPASS_MASK        BIT(1)
#define QSPI_DLLCR_SLV_DLL_BYPASS(v)          FIELD_PREP(QSPI_DLLCR_SLV_DLL_BYPASS_MASK, (v))
#define QSPI_DLLCR_SLV_EN_MASK                BIT(2)
#define QSPI_DLLCR_SLV_EN(v)                  FIELD_PREP(QSPI_DLLCR_SLV_EN_MASK, (v))
#define QSPI_DLLCR_SLAVE_AUTO_UPDT_MASK       BIT(3)
#define QSPI_DLLCR_SLAVE_AUTO_UPDT(v)         FIELD_PREP(QSPI_DLLCR_SLAVE_AUTO_UPDT_MASK, (v))
#define QSPI_DLLCR_SLV_DLY_COARSE_MASK        GENMASK(11, 8)
#define QSPI_DLLCR_SLV_DLY_COARSE(v)          FIELD_PREP(QSPI_DLLCR_SLV_DLY_COARSE_MASK, (v))
#define QSPI_DLLCR_SLV_DLY_OFFSET_MASK        GENMASK(14, 12)
#define QSPI_DLLCR_SLV_DLY_OFFSET(v)          FIELD_PREP(QSPI_DLLCR_SLV_DLY_OFFSET_MASK, (v))
#define QSPI_DLLCR_SLV_FINE_OFFSET_MASK       GENMASK(19, 16)
#define QSPI_DLLCR_SLV_FINE_OFFSET(v)         FIELD_PREP(QSPI_DLLCR_SLV_FINE_OFFSET_MASK, (v))
#define QSPI_DLLCR_DLLRES_MASK                GENMASK(23, 20)
#define QSPI_DLLCR_DLLRES(v)                  FIELD_PREP(QSPI_DLLCR_DLLRES_MASK, (v))
#define QSPI_DLLCR_DLL_REFCNTR_MASK           GENMASK(27, 24)
#define QSPI_DLLCR_DLL_REFCNTR(v)             FIELD_PREP(QSPI_DLLCR_DLL_REFCNTR_MASK, (v))
#define QSPI_DLLCR_FREQEN_MASK                BIT(30)
#define QSPI_DLLCR_FREQEN(v)                  FIELD_PREP(QSPI_DLLCR_FREQEN_MASK, (v))
#define QSPI_DLLCR_DLLEN_MASK                 BIT(31)
#define QSPI_DLLCR_DLLEN(v)                   FIELD_PREP(QSPI_DLLCR_DLLEN_MASK, (v))
/* Parity Configuration Register */
#define QSPI_PARITYCR                         0x6c
#define QSPI_PARITYCR_CRCBIN_FA_MASK          BIT(5)
#define QSPI_PARITYCR_CRCBIN_FA(v)            FIELD_PREP(QSPI_PARITYCR_CRCBIN_FA_MASK, (v))
#define QSPI_PARITYCR_CRCBEN_FA_MASK          BIT(6)
#define QSPI_PARITYCR_CRCBEN_FA(v)            FIELD_PREP(QSPI_PARITYCR_CRCBEN_FA_MASK, (v))
#define QSPI_PARITYCR_CRCEN_FA_MASK           BIT(7)
#define QSPI_PARITYCR_CRCEN_FA(v)             FIELD_PREP(QSPI_PARITYCR_CRCEN_FA_MASK, (v))
#define QSPI_PARITYCR_BYTE_SIZE_FA_MASK       BIT(8)
#define QSPI_PARITYCR_BYTE_SIZE_FA(v)         FIELD_PREP(QSPI_PARITYCR_BYTE_SIZE_FA_MASK, (v))
#define QSPI_PARITYCR_CHUNKSIZE_FA_MASK       GENMASK(14, 9)
#define QSPI_PARITYCR_CHUNKSIZE_FA(v)         FIELD_PREP(QSPI_PARITYCR_CHUNKSIZE_FA_MASK, (v))
#define QSPI_PARITYCR_CRC_WNDW_FA_MASK        BIT(15)
#define QSPI_PARITYCR_CRC_WNDW_FA(v)          FIELD_PREP(QSPI_PARITYCR_CRC_WNDW_FA_MASK, (v))
#define QSPI_PARITYCR_CRCBIN_FB_MASK          BIT(21)
#define QSPI_PARITYCR_CRCBIN_FB(v)            FIELD_PREP(QSPI_PARITYCR_CRCBIN_FB_MASK, (v))
#define QSPI_PARITYCR_CRCBEN_FB_MASK          BIT(22)
#define QSPI_PARITYCR_CRCBEN_FB(v)            FIELD_PREP(QSPI_PARITYCR_CRCBEN_FB_MASK, (v))
#define QSPI_PARITYCR_CRCEN_FB_MASK           BIT(23)
#define QSPI_PARITYCR_CRCEN_FB(v)             FIELD_PREP(QSPI_PARITYCR_CRCEN_FB_MASK, (v))
#define QSPI_PARITYCR_BYTE_SIZE_FB_MASK       BIT(24)
#define QSPI_PARITYCR_BYTE_SIZE_FB(v)         FIELD_PREP(QSPI_PARITYCR_BYTE_SIZE_FB_MASK, (v))
#define QSPI_PARITYCR_CHUNKSIZE_FB_MASK       GENMASK(30, 25)
#define QSPI_PARITYCR_CHUNKSIZE_FB(v)         FIELD_PREP(QSPI_PARITYCR_CHUNKSIZE_FB_MASK, (v))
#define QSPI_PARITYCR_CRC_WNDW_FB_MASK        BIT(31)
#define QSPI_PARITYCR_CRC_WNDW_FB(v)          FIELD_PREP(QSPI_PARITYCR_CRC_WNDW_FB_MASK, (v))
/* Serial Flash Memory Address Register */
#define QSPI_SFAR                             0x100
#define QSPI_SFAR_SFADR_MASK                  GENMASK(31, 0)
#define QSPI_SFAR_SFADR(v)                    FIELD_PREP(QSPI_SFAR_SFADR_MASK, (v))
/* Serial Flash Memory Address Configuration Register */
#define QSPI_SFACR                            0x104
#define QSPI_SFACR_CAS_MASK                   GENMASK(3, 0)
#define QSPI_SFACR_CAS(v)                     FIELD_PREP(QSPI_SFACR_CAS_MASK, (v))
#define QSPI_SFACR_PPWB_MASK                  GENMASK(12, 8)
#define QSPI_SFACR_PPWB(v)                    FIELD_PREP(QSPI_SFACR_PPWB_MASK, (v))
#define QSPI_SFACR_WA_MASK                    BIT(16)
#define QSPI_SFACR_WA(v)                      FIELD_PREP(QSPI_SFACR_WA_MASK, (v))
#define QSPI_SFACR_BYTE_SWAP_MASK             BIT(17)
#define QSPI_SFACR_BYTE_SWAP(v)               FIELD_PREP(QSPI_SFACR_BYTE_SWAP_MASK, (v))
/* Sampling Register */
#define QSPI_SMPR                             0x108
#define QSPI_SMPR_FSPHS_MASK                  BIT(5)
#define QSPI_SMPR_FSPHS(v)                    FIELD_PREP(QSPI_SMPR_FSPHS_MASK, (v))
#define QSPI_SMPR_FSDLY_MASK                  BIT(6)
#define QSPI_SMPR_FSDLY(v)                    FIELD_PREP(QSPI_SMPR_FSDLY_MASK, (v))
#define QSPI_SMPR_DLLFSMP_MASK(n)             (GENMASK(26, 24) * BIT(4 * (n)))
#define QSPI_SMPR_DLLFSMP(n, v)               FIELD_PREP(QSPI_SMPR_DLLFSMP_MASK(n), (v))
/* RX Buffer Status Register */
#define QSPI_RBSR                             0x10c
#define QSPI_RBSR_RDBFL_MASK                  GENMASK(7, 0)
#define QSPI_RBSR_RDBFL(v)                    FIELD_PREP(QSPI_RBSR_RDBFL_MASK, (v))
#define QSPI_RBSR_RDCTR_MASK                  GENMASK(31, 16)
#define QSPI_RBSR_RDCTR(v)                    FIELD_PREP(QSPI_RBSR_RDCTR_MASK, (v))
/* RX Buffer Control Register */
#define QSPI_RBCT                             0x110
#define QSPI_RBCT_WMRK_MASK                   GENMASK(6, 0)
#define QSPI_RBCT_WMRK(v)                     FIELD_PREP(QSPI_RBCT_WMRK_MASK, (v))
#define QSPI_RBCT_RXBRD_MASK                  BIT(8)
#define QSPI_RBCT_RXBRD(v)                    FIELD_PREP(QSPI_RBCT_RXBRD_MASK, (v))
/* AHB Write Status Register */
#define QSPI_AWRSR                            0x120
#define QSPI_AWRSR_SEQAUJOIN_MASK             BIT(2)
#define QSPI_AWRSR_SEQAUJOIN(v)               FIELD_PREP(QSPI_AWRSR_SEQAUJOIN_MASK, (v))
/* DLL Status Register */
#define QSPI_DLLSR                            0x12c
#define QSPI_DLLSR_DLL_SLV_COARSE_VAL_MASK(n) (GENMASK(3, 0) * BIT(16 * (n)))
#define QSPI_DLLSR_DLL_SLV_COARSE_VAL(n, v)   FIELD_PREP(QSPI_DLLSR_DLL_SLV_COARSE_VAL_MASK(n), (v))
#define QSPI_DLLSR_DLL_SLV_FINE_VAL_MASK(n)   (GENMASK(7, 4) * BIT(16 * (n)))
#define QSPI_DLLSR_DLL_SLV_FINE_VAL(n, v)     FIELD_PREP(QSPI_DLLSR_DLL_SLV_FINE_VAL_MASK(n), (v))
#define QSPI_DLLSR_DLL_FINE_UNDERFLOW_MASK(n) (BIT(12) * BIT(16 * (n)))
#define QSPI_DLLSR_DLL_FINE_UNDERFLOW(n, v)   FIELD_PREP(QSPI_DLLSR_DLL_FINE_UNDERFLOW_MASK(n), (v))
#define QSPI_DLLSR_DLL_RANGE_ERR_MASK(n)      (BIT(13) * BIT(16 * (n)))
#define QSPI_DLLSR_DLL_RANGE_ERR(n, v)        FIELD_PREP(QSPI_DLLSR_DLL_RANGE_ERR_MASK(n), (v))
#define QSPI_DLLSR_SLV_LOCK_MASK(n)           (BIT(14) * BIT(16 * (n)))
#define QSPI_DLLSR_SLV_LOCK(n, v)             FIELD_PREP(QSPI_DLLSR_SLV_LOCK_MASK(n), (v))
#define QSPI_DLLSR_DLL_LOCK_MASK(n)           (BIT(15) * BIT(16 * (n)))
#define QSPI_DLLSR_DLL_LOCK(n, v)             FIELD_PREP(QSPI_DLLSR_DLL_LOCK_MASK(n), (v))
/* Data Learning Configuration Register */
#define QSPI_DLCR                             0x130
#define QSPI_DLCR_DLP_SEL_MASK(n)             (GENMASK(15, 14) * BIT(16 * (n)))
#define QSPI_DLCR_DLP_SEL(n, v)               FIELD_PREP(QSPI_DLCR_DLP_SEL_MASK(n), (v))
#define QSPI_DLCR_DL_NONDLP_FLSH_MASK         BIT(24)
#define QSPI_DLCR_DL_NONDLP_FLSH(v)           FIELD_PREP(QSPI_DLCR_DL_NONDLP_FLSH_MASK, (v))
/* Data Learning Status Flash Memory n Register */
#define QSPI_DLSR(n)                          (0x134 + 0x4 * (n))
#define QSPI_DLSR_NEG_EDGE_MASK               GENMASK(7, 0)
#define QSPI_DLSR_NEG_EDGE(v)                 FIELD_PREP(QSPI_DLSR_NEG_EDGE_MASK, (v))
#define QSPI_DLSR_POS_EDGE_MASK               GENMASK(15, 8)
#define QSPI_DLSR_POS_EDGE(v)                 FIELD_PREP(QSPI_DLSR_POS_EDGE_MASK, (v))
#define QSPI_DLSR_DLPFF_MASK                  BIT(31)
#define QSPI_DLSR_DLPFF(v)                    FIELD_PREP(QSPI_DLSR_DLPFF_MASK, (v))
/* TX Buffer Status Register */
#define QSPI_TBSR                             0x150
#define QSPI_TBSR_TRBFL_MASK                  GENMASK(8, 0)
#define QSPI_TBSR_TRBFL(v)                    FIELD_PREP(QSPI_TBSR_TRBFL_MASK, (v))
#define QSPI_TBSR_TRCTR_MASK                  GENMASK(31, 16)
#define QSPI_TBSR_TRCTR(v)                    FIELD_PREP(QSPI_TBSR_TRCTR_MASK, (v))
/* TX Buffer Data Register */
#define QSPI_TBDR                             0x154
#define QSPI_TBDR_TXDATA_MASK                 GENMASK(31, 0)
#define QSPI_TBDR_TXDATA(v)                   FIELD_PREP(QSPI_TBDR_TXDATA_MASK, (v))
/* TX Buffer Control Register */
#define QSPI_TBCT                             0x158
#define QSPI_TBCT_WMRK_MASK                   GENMASK(7, 0)
#define QSPI_TBCT_WMRK(v)                     FIELD_PREP(QSPI_TBCT_WMRK_MASK, (v))
/* Status Register */
#define QSPI_SR                               0x15c
#define QSPI_SR_BUSY_MASK                     BIT(0)
#define QSPI_SR_BUSY(v)                       FIELD_PREP(QSPI_SR_BUSY_MASK, (v))
#define QSPI_SR_IP_ACC_MASK                   BIT(1)
#define QSPI_SR_IP_ACC(v)                     FIELD_PREP(QSPI_SR_IP_ACC_MASK, (v))
#define QSPI_SR_AHB_ACC_MASK                  BIT(2)
#define QSPI_SR_AHB_ACC(v)                    FIELD_PREP(QSPI_SR_AHB_ACC_MASK, (v))
#define QSPI_SR_AWRACC_MASK                   BIT(4)
#define QSPI_SR_AWRACC(v)                     FIELD_PREP(QSPI_SR_AWRACC_MASK, (v))
#define QSPI_SR_AHBTRN_MASK                   BIT(6)
#define QSPI_SR_AHBTRN(v)                     FIELD_PREP(QSPI_SR_AHBTRN_MASK, (v))
#define QSPI_SR_AHB0NE_MASK                   BIT(7)
#define QSPI_SR_AHB0NE(v)                     FIELD_PREP(QSPI_SR_AHB0NE_MASK, (v))
#define QSPI_SR_AHB1NE_MASK                   BIT(8)
#define QSPI_SR_AHB1NE(v)                     FIELD_PREP(QSPI_SR_AHB1NE_MASK, (v))
#define QSPI_SR_AHB2NE_MASK                   BIT(9)
#define QSPI_SR_AHB2NE(v)                     FIELD_PREP(QSPI_SR_AHB2NE_MASK, (v))
#define QSPI_SR_AHB3NE_MASK                   BIT(10)
#define QSPI_SR_AHB3NE(v)                     FIELD_PREP(QSPI_SR_AHB3NE_MASK, (v))
#define QSPI_SR_AHB0FUL_MASK                  BIT(11)
#define QSPI_SR_AHB0FUL(v)                    FIELD_PREP(QSPI_SR_AHB0FUL_MASK, (v))
#define QSPI_SR_AHB1FUL_MASK                  BIT(12)
#define QSPI_SR_AHB1FUL(v)                    FIELD_PREP(QSPI_SR_AHB1FUL_MASK, (v))
#define QSPI_SR_AHB2FUL_MASK                  BIT(13)
#define QSPI_SR_AHB2FUL(v)                    FIELD_PREP(QSPI_SR_AHB2FUL_MASK, (v))
#define QSPI_SR_AHB3FUL_MASK                  BIT(14)
#define QSPI_SR_AHB3FUL(v)                    FIELD_PREP(QSPI_SR_AHB3FUL_MASK, (v))
#define QSPI_SR_RXWE_MASK                     BIT(16)
#define QSPI_SR_RXWE(v)                       FIELD_PREP(QSPI_SR_RXWE_MASK, (v))
#define QSPI_SR_RXFULL_MASK                   BIT(19)
#define QSPI_SR_RXFULL(v)                     FIELD_PREP(QSPI_SR_RXFULL_MASK, (v))
#define QSPI_SR_RXDMA_MASK                    BIT(23)
#define QSPI_SR_RXDMA(v)                      FIELD_PREP(QSPI_SR_RXDMA_MASK, (v))
#define QSPI_SR_TXNE_MASK                     BIT(24)
#define QSPI_SR_TXNE(v)                       FIELD_PREP(QSPI_SR_TXNE_MASK, (v))
#define QSPI_SR_TXWA_MASK                     BIT(25)
#define QSPI_SR_TXWA(v)                       FIELD_PREP(QSPI_SR_TXWA_MASK, (v))
#define QSPI_SR_TXDMA_MASK                    BIT(26)
#define QSPI_SR_TXDMA(v)                      FIELD_PREP(QSPI_SR_TXDMA_MASK, (v))
#define QSPI_SR_TXFULL_MASK                   BIT(27)
#define QSPI_SR_TXFULL(v)                     FIELD_PREP(QSPI_SR_TXFULL_MASK, (v))
/* Flag Register */
#define QSPI_FR                               0x160
#define QSPI_FR_TFF_MASK                      BIT(0)
#define QSPI_FR_TFF(v)                        FIELD_PREP(QSPI_FR_TFF_MASK, (v))
#define QSPI_FR_IPIEF_MASK                    BIT(6)
#define QSPI_FR_IPIEF(v)                      FIELD_PREP(QSPI_FR_IPIEF_MASK, (v))
#define QSPI_FR_IPAEF_MASK                    BIT(7)
#define QSPI_FR_IPAEF(v)                      FIELD_PREP(QSPI_FR_IPAEF_MASK, (v))
#define QSPI_FR_PPWF_MASK                     BIT(8)
#define QSPI_FR_PPWF(v)                       FIELD_PREP(QSPI_FR_PPWF_MASK, (v))
#define QSPI_FR_CRCBEF_MASK                   BIT(9)
#define QSPI_FR_CRCBEF(v)                     FIELD_PREP(QSPI_FR_CRCBEF_MASK, (v))
#define QSPI_FR_CRCAEF_MASK                   BIT(10)
#define QSPI_FR_CRCAEF(v)                     FIELD_PREP(QSPI_FR_CRCAEF_MASK, (v))
#define QSPI_FR_ABOF_MASK                     BIT(12)
#define QSPI_FR_ABOF(v)                       FIELD_PREP(QSPI_FR_ABOF_MASK, (v))
#define QSPI_FR_AIBSEF_MASK                   BIT(13)
#define QSPI_FR_AIBSEF(v)                     FIELD_PREP(QSPI_FR_AIBSEF_MASK, (v))
#define QSPI_FR_AITEF_MASK                    BIT(14)
#define QSPI_FR_AITEF(v)                      FIELD_PREP(QSPI_FR_AITEF_MASK, (v))
#define QSPI_FR_AAEF_MASK                     BIT(15)
#define QSPI_FR_AAEF(v)                       FIELD_PREP(QSPI_FR_AAEF_MASK, (v))
#define QSPI_FR_RBDF_MASK                     BIT(16)
#define QSPI_FR_RBDF(v)                       FIELD_PREP(QSPI_FR_RBDF_MASK, (v))
#define QSPI_FR_RBOF_MASK                     BIT(17)
#define QSPI_FR_RBOF(v)                       FIELD_PREP(QSPI_FR_RBOF_MASK, (v))
#define QSPI_FR_ILLINE_MASK                   BIT(23)
#define QSPI_FR_ILLINE(v)                     FIELD_PREP(QSPI_FR_ILLINE_MASK, (v))
#define QSPI_FR_DLLUNLCK_MASK                 BIT(24)
#define QSPI_FR_DLLUNLCK(v)                   FIELD_PREP(QSPI_FR_DLLUNLCK_MASK, (v))
#define QSPI_FR_TBUF_MASK                     BIT(26)
#define QSPI_FR_TBUF(v)                       FIELD_PREP(QSPI_FR_TBUF_MASK, (v))
#define QSPI_FR_TBFF_MASK                     BIT(27)
#define QSPI_FR_TBFF(v)                       FIELD_PREP(QSPI_FR_TBFF_MASK, (v))
#define QSPI_FR_DLLABRT_MASK                  BIT(28)
#define QSPI_FR_DLLABRT(v)                    FIELD_PREP(QSPI_FR_DLLABRT_MASK, (v))
#define QSPI_FR_DLPFF_MASK                    BIT(31)
#define QSPI_FR_DLPFF(v)                      FIELD_PREP(QSPI_FR_DLPFF_MASK, (v))
/* Interrupt and DMA Request Select and Enable Register */
#define QSPI_RSER                             0x164
#define QSPI_RSER_TFIE_MASK                   BIT(0)
#define QSPI_RSER_TFIE(v)                     FIELD_PREP(QSPI_RSER_TFIE_MASK, (v))
#define QSPI_RSER_IPIEIE_MASK                 BIT(6)
#define QSPI_RSER_IPIEIE(v)                   FIELD_PREP(QSPI_RSER_IPIEIE_MASK, (v))
#define QSPI_RSER_IPAEIE_MASK                 BIT(7)
#define QSPI_RSER_IPAEIE(v)                   FIELD_PREP(QSPI_RSER_IPAEIE_MASK, (v))
#define QSPI_RSER_PPWIE_MASK                  BIT(8)
#define QSPI_RSER_PPWIE(v)                    FIELD_PREP(QSPI_RSER_PPWIE_MASK, (v))
#define QSPI_RSER_CRCBIE_MASK                 BIT(9)
#define QSPI_RSER_CRCBIE(v)                   FIELD_PREP(QSPI_RSER_CRCBIE_MASK, (v))
#define QSPI_RSER_CRCAIE_MASK                 BIT(10)
#define QSPI_RSER_CRCAIE(v)                   FIELD_PREP(QSPI_RSER_CRCAIE_MASK, (v))
#define QSPI_RSER_ABOIE_MASK                  BIT(12)
#define QSPI_RSER_ABOIE(v)                    FIELD_PREP(QSPI_RSER_ABOIE_MASK, (v))
#define QSPI_RSER_AIBSIE_MASK                 BIT(13)
#define QSPI_RSER_AIBSIE(v)                   FIELD_PREP(QSPI_RSER_AIBSIE_MASK, (v))
#define QSPI_RSER_AITIE_MASK                  BIT(14)
#define QSPI_RSER_AITIE(v)                    FIELD_PREP(QSPI_RSER_AITIE_MASK, (v))
#define QSPI_RSER_AAIE_MASK                   BIT(15)
#define QSPI_RSER_AAIE(v)                     FIELD_PREP(QSPI_RSER_AAIE_MASK, (v))
#define QSPI_RSER_RBDIE_MASK                  BIT(16)
#define QSPI_RSER_RBDIE(v)                    FIELD_PREP(QSPI_RSER_RBDIE_MASK, (v))
#define QSPI_RSER_RBOIE_MASK                  BIT(17)
#define QSPI_RSER_RBOIE(v)                    FIELD_PREP(QSPI_RSER_RBOIE_MASK, (v))
#define QSPI_RSER_RBDDE_MASK                  BIT(21)
#define QSPI_RSER_RBDDE(v)                    FIELD_PREP(QSPI_RSER_RBDDE_MASK, (v))
#define QSPI_RSER_ILLINIE_MASK                BIT(23)
#define QSPI_RSER_ILLINIE(v)                  FIELD_PREP(QSPI_RSER_ILLINIE_MASK, (v))
#define QSPI_RSER_DLLULIE_MASK                BIT(24)
#define QSPI_RSER_DLLULIE(v)                  FIELD_PREP(QSPI_RSER_DLLULIE_MASK, (v))
#define QSPI_RSER_TBFDE_MASK                  BIT(25)
#define QSPI_RSER_TBFDE(v)                    FIELD_PREP(QSPI_RSER_TBFDE_MASK, (v))
#define QSPI_RSER_TBUIE_MASK                  BIT(26)
#define QSPI_RSER_TBUIE(v)                    FIELD_PREP(QSPI_RSER_TBUIE_MASK, (v))
#define QSPI_RSER_TBFIE_MASK                  BIT(27)
#define QSPI_RSER_TBFIE(v)                    FIELD_PREP(QSPI_RSER_TBFIE_MASK, (v))
#define QSPI_RSER_DLPFIE_MASK                 BIT(31)
#define QSPI_RSER_DLPFIE(v)                   FIELD_PREP(QSPI_RSER_DLPFIE_MASK, (v))
/* Sequence Pointer Clear Register */
#define QSPI_SPTRCLR                          0x16c
#define QSPI_SPTRCLR_BFPTRC_MASK              BIT(0)
#define QSPI_SPTRCLR_BFPTRC(v)                FIELD_PREP(QSPI_SPTRCLR_BFPTRC_MASK, (v))
#define QSPI_SPTRCLR_IPPTRC_MASK              BIT(8)
#define QSPI_SPTRCLR_IPPTRC(v)                FIELD_PREP(QSPI_SPTRCLR_IPPTRC_MASK, (v))
#define QSPI_SPTRCLR_ABRT_CLR_MASK            BIT(16)
#define QSPI_SPTRCLR_ABRT_CLR(v)              FIELD_PREP(QSPI_SPTRCLR_ABRT_CLR_MASK, (v))
#define QSPI_SPTRCLR_PREFETCH_DIS_MASK        BIT(17)
#define QSPI_SPTRCLR_PREFETCH_DIS(v)          FIELD_PREP(QSPI_SPTRCLR_PREFETCH_DIS_MASK, (v))
#define QSPI_SPTRCLR_OTFAD_BNDRY_MASK         GENMASK(25, 24)
#define QSPI_SPTRCLR_OTFAD_BNDRY(v)           FIELD_PREP(QSPI_SPTRCLR_OTFAD_BNDRY_MASK, (v))
/* Serial Flash Memory n Top Address Register */
#define QSPI_SFAD(n)                          (0x180 + 0x4 * (n))
#define QSPI_SFAD_TPAD_MASK                   GENMASK(31, 10)
#define QSPI_SFAD_TPAD(v)                     FIELD_PREP(QSPI_SFAD_TPAD_MASK, (v))
/* Data Learn Pattern Register */
#define QSPI_DLPR                             0x190
#define QSPI_DLPR_DLPV_MASK                   GENMASK(31, 0)
#define QSPI_DLPR_DLPV(v)                     FIELD_PREP(QSPI_DLPR_DLPV_MASK, (v))
/* Flash Memory n Failing Address Status Register */
#define QSPI_FAIL_ADDR(n)                     (0x194 + 0x4 * (n))
#define QSPI_FAIL_ADDR_ADDR_MASK              GENMASK(31, 0)
#define QSPI_FAIL_ADDR_ADDR(v)                FIELD_PREP(QSPI_FAIL_ADDR_ADDR_MASK, (v))
/* RX Buffer Data Register */
#define QSPI_RBDR(n)                          (0x200 + 0x4 * (n))
#define QSPI_RBDR_RXDATA_MASK                 GENMASK(31, 0)
#define QSPI_RBDR_RXDATA(v)                   FIELD_PREP(QSPI_RBDR_RXDATA_MASK, (v))
/* LUT Key Register */
#define QSPI_LUTKEY                           0x300
#define QSPI_LUTKEY_KEY_MASK                  GENMASK(31, 0)
#define QSPI_LUTKEY_KEY(v)                    FIELD_PREP(QSPI_LUTKEY_KEY_MASK, (v))
/* LUT Lock Configuration Register */
#define QSPI_LCKCR                            0x304
#define QSPI_LCKCR_LOCK_MASK                  BIT(0)
#define QSPI_LCKCR_LOCK(v)                    FIELD_PREP(QSPI_LCKCR_LOCK_MASK, (v))
#define QSPI_LCKCR_UNLOCK_MASK                BIT(1)
#define QSPI_LCKCR_UNLOCK(v)                  FIELD_PREP(QSPI_LCKCR_UNLOCK_MASK, (v))
/* LUT Register */
#define QSPI_LUT(n)                           (0x310 + 0x4 * (n))
/* Flash(n) Region Start Address */
#define QSPI_WORD0(n)                         (0x800 + 0x20 * (n))
#define QSPI_WORD0_STARTADR_MASK              GENMASK(31, 16)
#define QSPI_WORD0_STARTADR(v)                FIELD_PREP(QSPI_WORD0_STARTADR_MASK, (v))
/* Flash Region End Address */
#define QSPI_WORD1(n)                         (0x804 + 0x20 * (n))
#define QSPI_WORD1_ENDADR_MASK                GENMASK(31, 16)
#define QSPI_WORD1_ENDADR(v)                  FIELD_PREP(QSPI_WORD1_ENDADR_MASK, (v))
/* Flash Region Privileges */
#define QSPI_WORD2(n)                         (0x808 + 0x20 * (n))
#define QSPI_WORD2_MD0ACP_MASK                GENMASK(2, 0)
#define QSPI_WORD2_MD0ACP(v)                  FIELD_PREP(QSPI_WORD2_MD0ACP_MASK, (v))
#define QSPI_WORD2_MD1ACP_MASK                GENMASK(5, 3)
#define QSPI_WORD2_MD1ACP(v)                  FIELD_PREP(QSPI_WORD2_MD1ACP_MASK, (v))
#define QSPI_WORD2_EALO_MASK                  GENMASK(29, 24)
#define QSPI_WORD2_EALO(v)                    FIELD_PREP(QSPI_WORD2_EALO_MASK, (v))
/* Flash Region Lock Control */
#define QSPI_WORD3(n)                         (0x80c + 0x20 * (n))
#define QSPI_WORD3_EAL_MASK                   GENMASK(25, 24)
#define QSPI_WORD3_EAL(v)                     FIELD_PREP(QSPI_WORD3_EAL_MASK, (v))
#define QSPI_WORD3_LOCK_MASK                  GENMASK(30, 29)
#define QSPI_WORD3_LOCK(v)                    FIELD_PREP(QSPI_WORD3_LOCK_MASK, (v))
#define QSPI_WORD3_VLD_MASK                   BIT(31)
#define QSPI_WORD3_VLD(v)                     FIELD_PREP(QSPI_WORD3_VLD_MASK, (v))
/* Flash Region Compare Address Status */
#define QSPI_WORD4(n)                         (0x810 + 0x20 * (n))
#define QSPI_WORD4_CMP_ADDR_MASK              GENMASK(31, 0)
#define QSPI_WORD4_CMP_ADDR(v)                FIELD_PREP(QSPI_WORD4_CMP_ADDR_MASK, (v))
/* Flash Region Compare Status Data */
#define QSPI_WORD5(n)                         (0x814 + 0x20 * (n))
#define QSPI_WORD5_CMP_MDID_MASK              GENMASK(5, 0)
#define QSPI_WORD5_CMP_MDID(v)                FIELD_PREP(QSPI_WORD5_CMP_MDID_MASK, (v))
#define QSPI_WORD5_CMP_SA_MASK                BIT(6)
#define QSPI_WORD5_CMP_SA(v)                  FIELD_PREP(QSPI_WORD5_CMP_SA_MASK, (v))
#define QSPI_WORD5_CMP_PA_MASK                BIT(7)
#define QSPI_WORD5_CMP_PA(v)                  FIELD_PREP(QSPI_WORD5_CMP_PA_MASK, (v))
#define QSPI_WORD5_CMP_ERR_MASK               BIT(29)
#define QSPI_WORD5_CMP_ERR(v)                 FIELD_PREP(QSPI_WORD5_CMP_ERR_MASK, (v))
#define QSPI_WORD5_CMPVALID_MASK              BIT(30)
#define QSPI_WORD5_CMPVALID(v)                FIELD_PREP(QSPI_WORD5_CMPVALID_MASK, (v))
/* Target Group n Master Domain Access Descriptor */
#define QSPI_TGMDAD(n)                        (0x900 + 0x10 * (n))
#define QSPI_TGMDAD_MIDMATCH_MASK             GENMASK(5, 0)
#define QSPI_TGMDAD_MIDMATCH(v)               FIELD_PREP(QSPI_TGMDAD_MIDMATCH_MASK, (v))
#define QSPI_TGMDAD_MASK_MASK                 GENMASK(11, 6)
#define QSPI_TGMDAD_MASK(v)                   FIELD_PREP(QSPI_TGMDAD_MASK_MASK, (v))
#define QSPI_TGMDAD_MASKTYPE_MASK             BIT(12)
#define QSPI_TGMDAD_MASKTYPE(v)               FIELD_PREP(QSPI_TGMDAD_MASKTYPE_MASK, (v))
#define QSPI_TGMDAD_SA_MASK                   GENMASK(15, 14)
#define QSPI_TGMDAD_SA(v)                     FIELD_PREP(QSPI_TGMDAD_SA_MASK, (v))
#define QSPI_TGMDAD_LCK_MASK                  BIT(29)
#define QSPI_TGMDAD_LCK(v)                    FIELD_PREP(QSPI_TGMDAD_LCK_MASK, (v))
#define QSPI_TGMDAD_VLD_MASK                  BIT(31)
#define QSPI_TGMDAD_VLD(v)                    FIELD_PREP(QSPI_TGMDAD_VLD_MASK, (v))
/* Target Group n SFAR Address */
#define QSPI_TGSFAR(n)                        (0x904 + 0x10 * (n))
#define QSPI_TGSFAR_SFARADDR_MASK             GENMASK(31, 0)
#define QSPI_TGSFAR_SFARADDR(v)               FIELD_PREP(QSPI_TGSFAR_SFARADDR_MASK, (v))
#define QSPI_TGSFAR_S_TG_MID_MASK             GENMASK(5, 0)
#define QSPI_TGSFAR_S_TG_MID(v)               FIELD_PREP(QSPI_TGSFAR_S_TG_MID_MASK, (v))
#define QSPI_TGSFAR_S_SA_MASK                 BIT(10)
#define QSPI_TGSFAR_S_SA(v)                   FIELD_PREP(QSPI_TGSFAR_S_SA_MASK, (v))
#define QSPI_TGSFAR_S_PA_MASK                 BIT(12)
#define QSPI_TGSFAR_S_PA(v)                   FIELD_PREP(QSPI_TGSFAR_S_PA_MASK, (v))
#define QSPI_TGSFAR_S_CLR_MASK                BIT(29)
#define QSPI_TGSFAR_S_CLR(v)                  FIELD_PREP(QSPI_TGSFAR_S_CLR_MASK, (v))
#define QSPI_TGSFAR_S_ERR_MASK                BIT(30)
#define QSPI_TGSFAR_S_ERR(v)                  FIELD_PREP(QSPI_TGSFAR_S_ERR_MASK, (v))
#define QSPI_TGSFAR_S_VLD_MASK                BIT(31)
#define QSPI_TGSFAR_S_VLD(v)                  FIELD_PREP(QSPI_TGSFAR_S_VLD_MASK, (v))
/* Target Group n SFAR Status */
#define QSPI_TGSFARS(n)                       (0x908 + 0x10 * (n))
#define QSPI_TGSFARS_TG_MID_MASK              GENMASK(5, 0)
#define QSPI_TGSFARS_TG_MID(v)                FIELD_PREP(QSPI_TGSFARS_TG_MID_MASK, (v))
#define QSPI_TGSFARS_SA_MASK                  BIT(10)
#define QSPI_TGSFARS_SA(v)                    FIELD_PREP(QSPI_TGSFARS_SA_MASK, (v))
#define QSPI_TGSFARS_PA_MASK                  BIT(12)
#define QSPI_TGSFARS_PA(v)                    FIELD_PREP(QSPI_TGSFARS_PA_MASK, (v))
#define QSPI_TGSFARS_CLR_MASK                 BIT(29)
#define QSPI_TGSFARS_CLR(v)                   FIELD_PREP(QSPI_TGSFARS_CLR_MASK, (v))
#define QSPI_TGSFARS_ERR_MASK                 BIT(30)
#define QSPI_TGSFARS_ERR(v)                   FIELD_PREP(QSPI_TGSFARS_ERR_MASK, (v))
#define QSPI_TGSFARS_VLD_MASK                 BIT(31)
#define QSPI_TGSFARS_VLD(v)                   FIELD_PREP(QSPI_TGSFARS_VLD_MASK, (v))
/* Target Group n IPCR Status */
#define QSPI_TGIPCRS(n)                       (0x90c + 0x10 * (n))
#define QSPI_TGIPCRS_IDATSZ_MASK              GENMASK(15, 0)
#define QSPI_TGIPCRS_IDATSZ(v)                FIELD_PREP(QSPI_TGIPCRS_IDATSZ_MASK, (v))
#define QSPI_TGIPCRS_SEQID_MASK               GENMASK(19, 16)
#define QSPI_TGIPCRS_SEQID(v)                 FIELD_PREP(QSPI_TGIPCRS_SEQID_MASK, (v))
#define QSPI_TGIPCRS_PAR_MASK                 BIT(20)
#define QSPI_TGIPCRS_PAR(v)                   FIELD_PREP(QSPI_TGIPCRS_PAR_MASK, (v))
#define QSPI_TGIPCRS_CLR_MASK                 BIT(28)
#define QSPI_TGIPCRS_CLR(v)                   FIELD_PREP(QSPI_TGIPCRS_CLR_MASK, (v))
#define QSPI_TGIPCRS_ERR_MASK                 GENMASK(30, 29)
#define QSPI_TGIPCRS_ERR(v)                   FIELD_PREP(QSPI_TGIPCRS_ERR_MASK, (v))
#define QSPI_TGIPCRS_VLD_MASK                 BIT(31)
#define QSPI_TGIPCRS_VLD(v)                   FIELD_PREP(QSPI_TGIPCRS_VLD_MASK, (v))
/* Master Global Configuration */
#define QSPI_MGC                              0x920
#define QSPI_MGC_GCLCKMID_MASK                GENMASK(5, 0)
#define QSPI_MGC_GCLCKMID(v)                  FIELD_PREP(QSPI_MGC_GCLCKMID_MASK, (v))
#define QSPI_MGC_GCLCK_MASK                   GENMASK(11, 10)
#define QSPI_MGC_GCLCK(v)                     FIELD_PREP(QSPI_MGC_GCLCK_MASK, (v))
#define QSPI_MGC_GVLDFRAD_MASK                BIT(27)
#define QSPI_MGC_GVLDFRAD(v)                  FIELD_PREP(QSPI_MGC_GVLDFRAD_MASK, (v))
#define QSPI_MGC_GVLDMDAD_MASK                BIT(29)
#define QSPI_MGC_GVLDMDAD(v)                  FIELD_PREP(QSPI_MGC_GVLDMDAD_MASK, (v))
#define QSPI_MGC_GVLD_MASK                    BIT(31)
#define QSPI_MGC_GVLD(v)                      FIELD_PREP(QSPI_MGC_GVLD_MASK, (v))
/* Master Read Command */
#define QSPI_MRC                              0x924
#define QSPI_MRC_READ_CMD0_MASK               GENMASK(5, 0)
#define QSPI_MRC_READ_CMD0(v)                 FIELD_PREP(QSPI_MRC_READ_CMD0_MASK, (v))
#define QSPI_MRC_READ_CMD1_MASK               GENMASK(13, 8)
#define QSPI_MRC_READ_CMD1(v)                 FIELD_PREP(QSPI_MRC_READ_CMD1_MASK, (v))
#define QSPI_MRC_READ_CMD2_MASK               GENMASK(21, 16)
#define QSPI_MRC_READ_CMD2(v)                 FIELD_PREP(QSPI_MRC_READ_CMD2_MASK, (v))
#define QSPI_MRC_VLDCMD02_MASK                BIT(22)
#define QSPI_MRC_VLDCMD02(v)                  FIELD_PREP(QSPI_MRC_VLDCMD02_MASK, (v))
#define QSPI_MRC_READ_CMD3_MASK               GENMASK(29, 24)
#define QSPI_MRC_READ_CMD3(v)                 FIELD_PREP(QSPI_MRC_READ_CMD3_MASK, (v))
#define QSPI_MRC_VLDCMD03_MASK                BIT(30)
#define QSPI_MRC_VLDCMD03(v)                  FIELD_PREP(QSPI_MRC_VLDCMD03_MASK, (v))
/* Master Timeout */
#define QSPI_MTO                              0x928
#define QSPI_MTO_WRITE_TO_MASK                GENMASK(31, 0)
#define QSPI_MTO_WRITE_TO(v)                  FIELD_PREP(QSPI_MTO_WRITE_TO_MASK, (v))
/* FlashSeq Request */
#define QSPI_FLSEQREQ                         0x92c
#define QSPI_FLSEQREQ_REQ_MID_MASK            GENMASK(5, 0)
#define QSPI_FLSEQREQ_REQ_MID(v)              FIELD_PREP(QSPI_FLSEQREQ_REQ_MID_MASK, (v))
#define QSPI_FLSEQREQ_REQ_TG_MASK             BIT(6)
#define QSPI_FLSEQREQ_REQ_TG(v)               FIELD_PREP(QSPI_FLSEQREQ_REQ_TG_MASK, (v))
#define QSPI_FLSEQREQ_SA_MASK                 BIT(8)
#define QSPI_FLSEQREQ_SA(v)                   FIELD_PREP(QSPI_FLSEQREQ_SA_MASK, (v))
#define QSPI_FLSEQREQ_PA_MASK                 BIT(9)
#define QSPI_FLSEQREQ_PA(v)                   FIELD_PREP(QSPI_FLSEQREQ_PA_MASK, (v))
#define QSPI_FLSEQREQ_FRAD_MASK               GENMASK(14, 12)
#define QSPI_FLSEQREQ_FRAD(v)                 FIELD_PREP(QSPI_FLSEQREQ_FRAD_MASK, (v))
#define QSPI_FLSEQREQ_SEQID_MASK              GENMASK(19, 16)
#define QSPI_FLSEQREQ_SEQID(v)                FIELD_PREP(QSPI_FLSEQREQ_SEQID_MASK, (v))
#define QSPI_FLSEQREQ_CMD_MASK                BIT(22)
#define QSPI_FLSEQREQ_CMD(v)                  FIELD_PREP(QSPI_FLSEQREQ_CMD_MASK, (v))
#define QSPI_FLSEQREQ_TIMEOUT_MASK            BIT(27)
#define QSPI_FLSEQREQ_TIMEOUT(v)              FIELD_PREP(QSPI_FLSEQREQ_TIMEOUT_MASK, (v))
#define QSPI_FLSEQREQ_CLR_MASK                BIT(29)
#define QSPI_FLSEQREQ_CLR(v)                  FIELD_PREP(QSPI_FLSEQREQ_CLR_MASK, (v))
#define QSPI_FLSEQREQ_VLD_MASK                BIT(31)
#define QSPI_FLSEQREQ_VLD(v)                  FIELD_PREP(QSPI_FLSEQREQ_VLD_MASK, (v))
/* FSM Status */
#define QSPI_FSMSTAT                          0x930
#define QSPI_FSMSTAT_STATE_MASK               GENMASK(1, 0)
#define QSPI_FSMSTAT_STATE(v)                 FIELD_PREP(QSPI_FSMSTAT_STATE_MASK, (v))
#define QSPI_FSMSTAT_MID_MASK                 GENMASK(13, 8)
#define QSPI_FSMSTAT_MID(v)                   FIELD_PREP(QSPI_FSMSTAT_MID_MASK, (v))
#define QSPI_FSMSTAT_CMD_MASK                 BIT(16)
#define QSPI_FSMSTAT_CMD(v)                   FIELD_PREP(QSPI_FSMSTAT_CMD_MASK, (v))
#define QSPI_FSMSTAT_VLD_MASK                 BIT(31)
#define QSPI_FSMSTAT_VLD(v)                   FIELD_PREP(QSPI_FSMSTAT_VLD_MASK, (v))
/* IPS Error */
#define QSPI_IPSERROR                         0x934
#define QSPI_IPSERROR_MID_MASK                GENMASK(5, 0)
#define QSPI_IPSERROR_MID(v)                  FIELD_PREP(QSPI_IPSERROR_MID_MASK, (v))
#define QSPI_IPSERROR_TG0LCK_MASK             BIT(8)
#define QSPI_IPSERROR_TG0LCK(v)               FIELD_PREP(QSPI_IPSERROR_TG0LCK_MASK, (v))
#define QSPI_IPSERROR_TG1LCK_MASK             BIT(9)
#define QSPI_IPSERROR_TG1LCK(v)               FIELD_PREP(QSPI_IPSERROR_TG1LCK_MASK, (v))
#define QSPI_IPSERROR_TG0SEC_MASK             BIT(10)
#define QSPI_IPSERROR_TG0SEC(v)               FIELD_PREP(QSPI_IPSERROR_TG0SEC_MASK, (v))
#define QSPI_IPSERROR_TG1SEC_MASK             BIT(11)
#define QSPI_IPSERROR_TG1SEC(v)               FIELD_PREP(QSPI_IPSERROR_TG1SEC_MASK, (v))
#define QSPI_IPSERROR_TG0MID_MASK             BIT(12)
#define QSPI_IPSERROR_TG0MID(v)               FIELD_PREP(QSPI_IPSERROR_TG0MID_MASK, (v))
#define QSPI_IPSERROR_TG1MID_MASK             BIT(13)
#define QSPI_IPSERROR_TG1MID(v)               FIELD_PREP(QSPI_IPSERROR_TG1MID_MASK, (v))
#define QSPI_IPSERROR_MDADPROG_MASK           BIT(14)
#define QSPI_IPSERROR_MDADPROG(v)             FIELD_PREP(QSPI_IPSERROR_MDADPROG_MASK, (v))
#define QSPI_IPSERROR_FRADPROG_MASK           BIT(15)
#define QSPI_IPSERROR_FRADPROG(v)             FIELD_PREP(QSPI_IPSERROR_FRADPROG_MASK, (v))
#define QSPI_IPSERROR_CLR_MASK                BIT(29)
#define QSPI_IPSERROR_CLR(v)                  FIELD_PREP(QSPI_IPSERROR_CLR_MASK, (v))
/* Error Status */
#define QSPI_ERRSTAT                          0x938
#define QSPI_ERRSTAT_FRADMTCH_MASK            BIT(0)
#define QSPI_ERRSTAT_FRADMTCH(v)              FIELD_PREP(QSPI_ERRSTAT_FRADMTCH_MASK, (v))
#define QSPI_ERRSTAT_FRAD0ACC_MASK            BIT(1)
#define QSPI_ERRSTAT_FRAD0ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD0ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD1ACC_MASK            BIT(2)
#define QSPI_ERRSTAT_FRAD1ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD1ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD2ACC_MASK            BIT(3)
#define QSPI_ERRSTAT_FRAD2ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD2ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD3ACC_MASK            BIT(4)
#define QSPI_ERRSTAT_FRAD3ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD3ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD4ACC_MASK            BIT(5)
#define QSPI_ERRSTAT_FRAD4ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD4ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD5ACC_MASK            BIT(6)
#define QSPI_ERRSTAT_FRAD5ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD5ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD6ACC_MASK            BIT(7)
#define QSPI_ERRSTAT_FRAD6ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD6ACC_MASK, (v))
#define QSPI_ERRSTAT_FRAD7ACC_MASK            BIT(8)
#define QSPI_ERRSTAT_FRAD7ACC(v)              FIELD_PREP(QSPI_ERRSTAT_FRAD7ACC_MASK, (v))
#define QSPI_ERRSTAT_IPS_ERR_MASK             BIT(9)
#define QSPI_ERRSTAT_IPS_ERR(v)               FIELD_PREP(QSPI_ERRSTAT_IPS_ERR_MASK, (v))
#define QSPI_ERRSTAT_TG0SFAR_MASK             BIT(10)
#define QSPI_ERRSTAT_TG0SFAR(v)               FIELD_PREP(QSPI_ERRSTAT_TG0SFAR_MASK, (v))
#define QSPI_ERRSTAT_TG1SFAR_MASK             BIT(11)
#define QSPI_ERRSTAT_TG1SFAR(v)               FIELD_PREP(QSPI_ERRSTAT_TG1SFAR_MASK, (v))
#define QSPI_ERRSTAT_TG0IPCR_MASK             BIT(12)
#define QSPI_ERRSTAT_TG0IPCR(v)               FIELD_PREP(QSPI_ERRSTAT_TG0IPCR_MASK, (v))
#define QSPI_ERRSTAT_TG1IPCR_MASK             BIT(13)
#define QSPI_ERRSTAT_TG1IPCR(v)               FIELD_PREP(QSPI_ERRSTAT_TG1IPCR_MASK, (v))
#define QSPI_ERRSTAT_TO_ERR_MASK              BIT(14)
#define QSPI_ERRSTAT_TO_ERR(v)                FIELD_PREP(QSPI_ERRSTAT_TO_ERR_MASK, (v))
/* Interrupt Enable */
#define QSPI_INT_EN                           0x93c
#define QSPI_INT_EN_FRADMTCH_MASK             BIT(0)
#define QSPI_INT_EN_FRADMTCH(v)               FIELD_PREP(QSPI_INT_EN_FRADMTCH_MASK, (v))
#define QSPI_INT_EN_FRAD0ACC_MASK             BIT(1)
#define QSPI_INT_EN_FRAD0ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD0ACC_MASK, (v))
#define QSPI_INT_EN_FRAD1ACC_MASK             BIT(2)
#define QSPI_INT_EN_FRAD1ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD1ACC_MASK, (v))
#define QSPI_INT_EN_FRAD2ACC_MASK             BIT(3)
#define QSPI_INT_EN_FRAD2ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD2ACC_MASK, (v))
#define QSPI_INT_EN_FRAD3ACC_MASK             BIT(4)
#define QSPI_INT_EN_FRAD3ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD3ACC_MASK, (v))
#define QSPI_INT_EN_FRAD4ACC_MASK             BIT(5)
#define QSPI_INT_EN_FRAD4ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD4ACC_MASK, (v))
#define QSPI_INT_EN_FRAD5ACC_MASK             BIT(6)
#define QSPI_INT_EN_FRAD5ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD5ACC_MASK, (v))
#define QSPI_INT_EN_FRAD6ACC_MASK             BIT(7)
#define QSPI_INT_EN_FRAD6ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD6ACC_MASK, (v))
#define QSPI_INT_EN_FRAD7ACC_MASK             BIT(8)
#define QSPI_INT_EN_FRAD7ACC(v)               FIELD_PREP(QSPI_INT_EN_FRAD7ACC_MASK, (v))
#define QSPI_INT_EN_IPS_ERR_MASK              BIT(9)
#define QSPI_INT_EN_IPS_ERR(v)                FIELD_PREP(QSPI_INT_EN_IPS_ERR_MASK, (v))
#define QSPI_INT_EN_TG0SFAR_MASK              BIT(10)
#define QSPI_INT_EN_TG0SFAR(v)                FIELD_PREP(QSPI_INT_EN_TG0SFAR_MASK, (v))
#define QSPI_INT_EN_TG1SFAR_MASK              BIT(11)
#define QSPI_INT_EN_TG1SFAR(v)                FIELD_PREP(QSPI_INT_EN_TG1SFAR_MASK, (v))
#define QSPI_INT_EN_TG0IPCR_MASK              BIT(12)
#define QSPI_INT_EN_TG0IPCR(v)                FIELD_PREP(QSPI_INT_EN_TG0IPCR_MASK, (v))
#define QSPI_INT_EN_TG1IPCR_MASK              BIT(13)
#define QSPI_INT_EN_TG1IPCR(v)                FIELD_PREP(QSPI_INT_EN_TG1IPCR_MASK, (v))
#define QSPI_INT_EN_TO_ERR_MASK               BIT(14)
#define QSPI_INT_EN_TO_ERR(v)                 FIELD_PREP(QSPI_INT_EN_TO_ERR_MASK, (v))

/* Handy accessors */
#define REG_READ(r)     sys_read32(config->base + (r))
#define REG_WRITE(r, v) sys_write32((v), config->base + (r))

#define QSPI_HAS_FEATURE_(idx, feat) DT_INST_PROP(idx, CONCAT(has_, feat))
#define QSPI_HAS_FEATURE(feat)       DT_INST_FOREACH_STATUS_OKAY_VARGS(QSPI_HAS_FEATURE_, feat)

/* Maximum number of CS lines available per port */
#define QSPI_MAX_CS_PER_PORT 2U

/* Timeout for completing QSPI IP commands */
#define QSPI_CMD_COMPLETE_TIMEOUT_US      10000U
/* Timeout for DLL lock sequence */
#define QSPI_DLL_LOCK_TIMEOUT_US          10000U
/* Delay after resetting tx buffers */
#define QSPI_TX_BUFFER_RESET_DELAY_CYCLES 430U
/* Delay after resetting controller */
#define QSPI_RESET_DELAY_CYCLES           276U

/* Pack two LUT entries into a LUT register */
#define QSPI_LUT_PACK_REG(cmd0, cmd1) (((uint32_t)(cmd1) << 16U) | (uint32_t)((cmd0) & 0xffff))
/* Invalid LUT command */
#define QSPI_LUT_CMD_INVALID          ((uint16_t)0xFFFFU)
/* End of LUT sequence */
#define QSPI_LUT_SEQ_END              ((uint16_t)0x0U)
/* LUT sequence ID used for IP operations */
#define QSPI_LUT_SEQID_IP             0U
/* Number of LUT 32-bit entries that make up a sequence */
#define QSPI_LUT_SEQ_SIZE             (NXP_QSPI_LUT_MAX_CMD / 2U)
/* Key to lock or unlock the LUT */
#define QSPI_LUT_KEY                  0x5AF05AF0

struct qspi_data {
	struct k_sem sem;
};

struct qspi_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pincfg;
	const struct qspi_controller *controller;
};

enum qspi_read_mode {
	QSPI_READ_MODE_LOOPBACK = 1U,
	QSPI_READ_MODE_EXTERNAL_DQS = 3U,
#if QSPI_HAS_FEATURE(dqs_clock)
	QSPI_READ_MODE_INTERNAL_DQS = 0U,
	QSPI_READ_MODE_LOOPBACK_DQS = 2U,
#endif /* QSPI_HAS_FEATURE(dqs_clock) */
};

#if QSPI_HAS_FEATURE(dll)
enum qspi_dll_mode {
	QSPI_DLL_BYPASSED = 0U,
	QSPI_DLL_MANUAL_UPDATE = 1U,
	QSPI_DLL_AUTO_UPDATE = 2U,
};

struct qspi_dll {
	enum qspi_dll_mode mode;
	bool freq_enable;
	uint8_t coarse_delay;
	uint8_t fine_delay;
	uint8_t tap_select;
#if QSPI_HAS_FEATURE(dllcr_loop_control)
	uint8_t reference_counter;
	uint8_t resolution;
#endif /* QSPI_HAS_FEATURE(dllcr_loop_control) */
};
#endif /* QSPI_HAS_FEATURE(dll) */

struct qspi_port {
	uint32_t memory_sizes[QSPI_MAX_CS_PER_PORT];
	enum qspi_read_mode read_mode;
#if QSPI_HAS_FEATURE(mcr_isd)
	bool io2_idle: 1;
	bool io3_idle: 1;
#endif /* QSPI_HAS_FEATURE(mcr_isd) */
#if QSPI_HAS_FEATURE(dll)
	struct qspi_dll dll;
#endif /* QSPI_HAS_FEATURE(dll) */
};

#if QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap)
struct qspi_address {
#if QSPI_HAS_FEATURE(sfacr_cas)
	uint8_t column_space;
	bool word_addressable;
#endif /* QSPI_HAS_FEATURE(sfacr_cas) */
#if QSPI_HAS_FEATURE(sfacr_byte_swap)
	bool byte_swap;
#endif /* QSPI_HAS_FEATURE(sfacr_byte_swap) */
};
#endif /* QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap) */

struct qspi_buffer {
	uint16_t master;
	uint16_t size;
} __packed;

struct qspi_rx_buffers {
	struct qspi_buffer *buffers;
	uint8_t buffers_cnt;
	bool all_masters;
};

struct qspi_sampling {
	bool delay_half_cycle;
	bool phase_inverted;
};

struct qspi_cs {
	uint8_t hold_time;
	uint8_t setup_time;
};

struct qspi_timing {
#if QSPI_HAS_FEATURE(ddr)
	bool aligned_2x_refclk;
#endif /* QSPI_HAS_FEATURE(ddr) */
	struct qspi_cs cs;
	struct qspi_sampling sampling;
};

struct qspi_controller {
	mem_addr_t amba_base;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options)
	uint32_t chip_options;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options) */
#if QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap)
	struct qspi_address address;
#endif /* QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap) */
	struct qspi_timing timing;
	struct qspi_rx_buffers rx_buffers;
	struct qspi_port *ports;
	uint8_t ports_cnt;
	uint8_t tx_fifo_size;
	uint8_t rx_fifo_size;
	uint8_t lut_size;
#if QSPI_HAS_FEATURE(ddr)
	bool ddr;
#endif /* QSPI_HAS_FEATURE(ddr) */
};

static ALWAYS_INLINE void qspi_acquire(const struct device *dev)
{
	struct qspi_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static ALWAYS_INLINE void qspi_release(const struct device *dev)
{
	struct qspi_data *data = dev->data;

	k_sem_give(&data->sem);
}

static void qspi_config_ports(const struct qspi_config *config)
{
	const struct qspi_controller *controller = config->controller;
	const struct qspi_port *port;
	uint32_t reg_val;

	reg_val = controller->amba_base;
	for (int i = 0; i < controller->ports_cnt; i++) {
		port = &controller->ports[i];

		/* Configure external flash memory map sizes */
		for (int j = 0; j < QSPI_MAX_CS_PER_PORT; j++) {
			reg_val += port->memory_sizes[j];
			REG_WRITE(QSPI_SFAD((i * 2) + j), reg_val);
		}

#if QSPI_HAS_FEATURE(mcr_isd)
		/* Configure idle line values */
		reg_val = REG_READ(QSPI_MCR);
		reg_val &= ~(QSPI_MCR_ISD2_MASK(i) | QSPI_MCR_ISD3_MASK(i));
		reg_val |= QSPI_MCR_ISD2(i, port->io2_idle) | QSPI_MCR_ISD3(i, port->io3_idle);
		REG_WRITE(QSPI_MCR, reg_val);
#endif /* QSPI_HAS_FEATURE(mcr_isd) */
	}
}

#if QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap)
static void qspi_config_flash_address(const struct qspi_config *config)
{
	const struct qspi_address *address = &config->controller->address;
	uint32_t reg_val;

#if QSPI_HAS_FEATURE(sfacr_cas)
	reg_val = REG_READ(QSPI_SFACR) & ~(QSPI_SFACR_CAS_MASK | QSPI_SFACR_WA_MASK);
	REG_WRITE(QSPI_SFACR, reg_val | (QSPI_SFACR_CAS(address->column_space) |
					 QSPI_SFACR_WA(address->word_addressable ? 1U : 0U)));
#endif /* QSPI_HAS_FEATURE(sfacr_cas) */

#if QSPI_HAS_FEATURE(sfacr_byte_swap)
	reg_val = REG_READ(QSPI_SFACR) & ~QSPI_SFACR_BYTE_SWAP_MASK;
	REG_WRITE(QSPI_SFACR, reg_val | QSPI_SFACR_BYTE_SWAP(address->byte_swap ? 1U : 0U));
#endif /* QSPI_HAS_FEATURE(sfacr_byte_swap */
}
#endif /* QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap) */

static void qspi_config_cs(const struct qspi_config *config)
{
	const struct qspi_cs *cs = &config->controller->timing.cs;
	uint32_t reg_val;

	/* Configure CS holdtime and setup time */
	reg_val = REG_READ(QSPI_FLSHCR) & ~(QSPI_FLSHCR_TCSH_MASK | QSPI_FLSHCR_TCSS_MASK);
	reg_val |= QSPI_FLSHCR_TCSH(cs->hold_time) | QSPI_FLSHCR_TCSS(cs->setup_time);
	REG_WRITE(QSPI_FLSHCR, reg_val);
}

static void qspi_config_buffers(const struct qspi_config *config)
{
	const struct qspi_rx_buffers *rx_buffers = &config->controller->rx_buffers;
	uint32_t reg_val;
	uint8_t i;

	/* Set watermarks */
	reg_val = REG_READ(QSPI_TBCT) & ~QSPI_TBCT_WMRK_MASK;
	REG_WRITE(QSPI_TBCT, reg_val | QSPI_TBCT_WMRK(1U));
	reg_val = REG_READ(QSPI_RBCT) & ~QSPI_RBCT_WMRK_MASK;
	REG_WRITE(QSPI_RBCT, reg_val | QSPI_RBCT_WMRK(0U));

#if QSPI_HAS_FEATURE(rbct_rxbrd)
	/* Read Rx buffer through RBDR registers */
	reg_val = REG_READ(QSPI_RBCT) & ~QSPI_RBCT_RXBRD_MASK;
	REG_WRITE(QSPI_RBCT, reg_val | QSPI_RBCT_RXBRD(1U));
#endif /* QSPI_HAS_FEATURE(rbct_rxbrd) */

	reg_val = 0;
	for (i = 0; i < rx_buffers->buffers_cnt; i++) {
		/* Set AHB transfer sizes to match the buffer sizes */
		REG_WRITE(QSPI_BUFCR(i), (QSPI_BUFCR_ADATSZ(rx_buffers->buffers[i].size >> 3U) |
					  QSPI_BUFCR_MSTRID(rx_buffers->buffers[i].master)));
		/* Set AHB buffer index */
		if (i < (rx_buffers->buffers_cnt - 1)) {
			reg_val += rx_buffers->buffers[i].size;
			REG_WRITE(QSPI_BUFIND(i), QSPI_BUFIND_TPINDX(reg_val));
		}
	}

	reg_val = REG_READ(QSPI_BUFCR(rx_buffers->buffers_cnt - 1)) & ~QSPI_BUFCR_ALLMST_MASK;
	REG_WRITE(QSPI_BUFCR(rx_buffers->buffers_cnt - 1),
		  reg_val | QSPI_BUFCR_ALLMST(rx_buffers->all_masters ? 1U : 0U));
}

static void qspi_config_read_options(const struct qspi_config *config)
{
	const struct qspi_controller *controller = config->controller;
	uint32_t reg_val;

#if QSPI_HAS_FEATURE(dqs_clock)
	/* Enable DQS */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_DQS_EN(1U));
#endif /* QSPI_HAS_FEATURE(dqs_clock) */

#if QSPI_HAS_FEATURE(ddr)
	if (controller->ddr) {
		/* Enable DDR */
		REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_DDR_EN(1U));
		/* Configure data hold time */
		reg_val = REG_READ(QSPI_FLSHCR) & ~QSPI_FLSHCR_TDH_MASK;
		reg_val |= QSPI_FLSHCR_TDH(controller->timing.aligned_2x_refclk ? 1U : 0U);
		REG_WRITE(QSPI_FLSHCR, reg_val);
	} else {
		/* Disable DDR to use SDR */
		REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) & ~QSPI_MCR_DDR_EN_MASK);
		/* Ignore output data align setting in SDR mode */
		REG_WRITE(QSPI_FLSHCR, REG_READ(QSPI_FLSHCR) & ~QSPI_FLSHCR_TDH_MASK);
	}
#endif /* QSPI_HAS_FEATURE(ddr) */

	for (int i = 0; i < controller->ports_cnt; i++) {
		/* Select DQS source*/
		reg_val = REG_READ(QSPI_MCR) & ~QSPI_MCR_DQS_SEL_MASK(i);
		REG_WRITE(QSPI_MCR, reg_val | QSPI_MCR_DQS_SEL(i, controller->ports[i].read_mode));

#if QSPI_HAS_FEATURE(dll)
		/* Select DLL tap in SMPR register */
		reg_val = REG_READ(QSPI_SMPR) & ~QSPI_SMPR_DLLFSMP_MASK(i);
		REG_WRITE(QSPI_SMPR,
			  reg_val | QSPI_SMPR_DLLFSMP(i, controller->ports[i].dll.tap_select));
#endif /* QSPI_HAS_FEATURE(dll) */
	}
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options)
static void qspi_config_chip_options(const struct qspi_config *config)
{
	const struct qspi_controller *controller = config->controller;

	REG_WRITE(QSPI_SOCCR, controller->chip_options);
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options) */

static void qspi_config_sampling(const struct qspi_config *config)
{
	const struct qspi_sampling *sampling = &config->controller->timing.sampling;
	uint32_t reg_val;

	reg_val = REG_READ(QSPI_SMPR) & ~(QSPI_SMPR_FSPHS_MASK | QSPI_SMPR_FSDLY_MASK);
	reg_val |= QSPI_SMPR_FSDLY(sampling->delay_half_cycle ? 1U : 0U) |
		   QSPI_SMPR_FSPHS(sampling->phase_inverted ? 1U : 0U);
	REG_WRITE(QSPI_SMPR, reg_val);
}

static void qspi_sw_reset(const struct qspi_config *config)
{
	volatile uint32_t delay = QSPI_RESET_DELAY_CYCLES;

	/* Software reset AHB domain and Serial Flash domain at the same time */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_SWRSTHD(1U) | QSPI_MCR_SWRSTSD(1U));
	delay = QSPI_RESET_DELAY_CYCLES;
	while (delay--)
		;

	/* Disable QSPI module before de-asserting the reset bits */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_MDIS(1U));

	/* De-asset Software reset AHB domain and Serial Flash domain bits */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) & ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK));

	/* Re-enable QSPI module after reset */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) & ~QSPI_MCR_MDIS_MASK);
	delay = QSPI_RESET_DELAY_CYCLES;
	while (delay--)
		;
}

#if QSPI_HAS_FEATURE(dll)
static inline void qspi_dll_slave_update(const struct qspi_config *config, uint8_t port,
					 bool enable)
{
	uint32_t reg_val;

	reg_val = REG_READ(QSPI_DLLCR(port)) & ~QSPI_DLLCR_SLV_UPD_MASK;
	REG_WRITE(QSPI_DLLCR(port), reg_val | QSPI_DLLCR_SLV_UPD(enable ? 1U : 0U));
}

static int qspi_dll_wait_lock(const struct qspi_config *config, uint8_t port, bool slave)
{
	uint32_t cycle_count = k_us_to_cyc_ceil32(QSPI_DLL_LOCK_TIMEOUT_US);
	uint32_t cycle_start = k_cycle_get_32();
	uint32_t lock_mask =
		slave ? QSPI_DLLSR_SLV_LOCK_MASK(port) : QSPI_DLLSR_DLL_LOCK_MASK(port);
	uint32_t err_mask =
		QSPI_DLLSR_DLL_RANGE_ERR_MASK(port) | QSPI_DLLSR_DLL_FINE_UNDERFLOW_MASK(port);
	bool locked = false;
	bool error = false;

	while (!locked && !error && (cycle_count > (k_cycle_get_32() - cycle_start))) {
		locked = FIELD_GET(lock_mask, REG_READ(QSPI_DLLSR)) != 0U;
		error = FIELD_GET(err_mask, REG_READ(QSPI_DLLSR)) != 0U;
	}

	return error ? -EIO : (locked ? 0 : -ETIMEDOUT);
}

static int qspi_dll_config_bypass(const struct qspi_config *config, uint8_t port)
{
	const struct qspi_controller *controller = config->controller;
	uint32_t reg_val;
	int ret = 0;

	/* Set DLL in bypass mode */
	reg_val = REG_READ(QSPI_DLLCR(port));
	reg_val |= QSPI_DLLCR_SLV_DLL_BYPASS(1U);
#if QSPI_HAS_FEATURE(dllcr_slave_auto_updt)
	reg_val &= ~QSPI_DLLCR_SLAVE_AUTO_UPDT_MASK;
#endif /* QSPI_HAS_FEATURE(dllcr_slave_auto_updt) */
	REG_WRITE(QSPI_DLLCR(port), reg_val);

	/* Program desired DQS delay for sampling */
	reg_val = REG_READ(QSPI_DLLCR(port));
	reg_val &= ~(QSPI_DLLCR_SLV_DLY_COARSE_MASK | QSPI_DLLCR_SLV_FINE_OFFSET_MASK |
		     QSPI_DLLCR_FREQEN_MASK);
	reg_val |= QSPI_DLLCR_SLV_DLY_COARSE(controller->ports[port].dll.coarse_delay);
	reg_val |= QSPI_DLLCR_SLV_FINE_OFFSET(controller->ports[port].dll.fine_delay);
	reg_val |= QSPI_DLLCR_FREQEN(controller->ports[port].dll.freq_enable);
	REG_WRITE(QSPI_DLLCR(port), reg_val);

	/* Trigger update to load these values in the slave delay chain */
	qspi_dll_slave_update(config, port, true);

	/* Wait for slave delay chain update to take effect */
	ret = qspi_dll_wait_lock(config, port, true);

	/* Disable slave chain update */
	qspi_dll_slave_update(config, port, false);

	return ret;
}

static int qspi_dll_config_update(const struct qspi_config *config, uint8_t port)
{
	const struct qspi_controller *controller = config->controller;
	uint32_t reg_val;
	int ret = 0;

	/* Set DLL in auto update mode */
	reg_val = REG_READ(QSPI_DLLCR(port));
	reg_val &= ~QSPI_DLLCR_SLV_DLL_BYPASS_MASK;
#if QSPI_HAS_FEATURE(dllcr_slave_auto_updt)
	if (controller->ports[port].dll.mode == QSPI_DLL_AUTO_UPDATE) {
		reg_val |= QSPI_DLLCR_SLAVE_AUTO_UPDT(1U);
	} else {
		reg_val &= ~QSPI_DLLCR_SLAVE_AUTO_UPDT_MASK;
	}
#endif /* QSPI_HAS_FEATURE(dllcr_slave_auto_updt) */
	REG_WRITE(QSPI_DLLCR(port), reg_val);

	/* Program DLL configuration */
#if QSPI_HAS_FEATURE(dllcr_loop_control)
	reg_val = REG_READ(QSPI_DLLCR(port)) &
		  ~(QSPI_DLLCR_DLL_REFCNTR_MASK | QSPI_DLLCR_DLLRES_MASK);
	reg_val |= QSPI_DLLCR_DLL_REFCNTR(controller->ports[port].dll.reference_counter);
	reg_val |= QSPI_DLLCR_DLLRES(controller->ports[port].dll.resolution);
	REG_WRITE(QSPI_DLLCR(port), reg_val);
#endif /* QSPI_HAS_FEATURE(dllcr_loop_control) */

	/* Program desired DQS delay for sampling */
	reg_val = REG_READ(QSPI_DLLCR(port));
	reg_val &= ~(QSPI_DLLCR_SLV_DLY_OFFSET_MASK | QSPI_DLLCR_SLV_FINE_OFFSET_MASK |
		     QSPI_DLLCR_FREQEN_MASK);
	reg_val |= QSPI_DLLCR_SLV_DLY_OFFSET(controller->ports[port].dll.coarse_delay);
	reg_val |= QSPI_DLLCR_SLV_FINE_OFFSET(controller->ports[port].dll.fine_delay);
	reg_val |= QSPI_DLLCR_FREQEN(controller->ports[port].dll.freq_enable);
	REG_WRITE(QSPI_DLLCR(port), reg_val);

	if (controller->ports[port].dll.mode == QSPI_DLL_AUTO_UPDATE) {
		/* Trigger update to load these values in the slave delay chain */
		qspi_dll_slave_update(config, port, true);
	}

#if QSPI_HAS_FEATURE(dllcr_en)
	REG_WRITE(QSPI_DLLCR(port), REG_READ(QSPI_DLLCR(port)) | QSPI_DLLCR_DLLEN(1U));
#endif /* QSPI_HAS_FEATURE(dllcr_en) */

	if (controller->ports[port].dll.mode == QSPI_DLL_MANUAL_UPDATE) {
		/* For manual update mode, wait for DLL lock before triggering slave chain update */
		ret = qspi_dll_wait_lock(config, port, false);
		if (ret) {
			return ret;
		}
		qspi_dll_slave_update(config, port, true);
	}

	/* Wait for slave delay chain update to take effect */
	ret = qspi_dll_wait_lock(config, port, true);

	/* Disable slave chain update */
	qspi_dll_slave_update(config, port, false);

	return ret;
}

static int qspi_config_dll(const struct qspi_config *config)
{
	const struct qspi_controller *controller = config->controller;
	int ret = 0;
	uint8_t port;

	/* Perform slave delay chain programming sequence */
	for (port = 0; port < controller->ports_cnt; port++) {
		/* Ensure DLL and slave chain update are disabled */
		qspi_dll_slave_update(config, port, false);
#if QSPI_HAS_FEATURE(dllcr_en)
		REG_WRITE(QSPI_DLLCR(port), REG_READ(QSPI_DLLCR(port)) & ~QSPI_DLLCR_DLLEN_MASK);
#endif /* QSPI_HAS_FEATURE(dllcr_en) */

		/* Enable DQS slave delay chain before any settings take place */
		REG_WRITE(QSPI_DLLCR(port), REG_READ(QSPI_DLLCR(port)) | QSPI_DLLCR_SLV_EN(1U));

		if (controller->ports[port].dll.mode == QSPI_DLL_BYPASSED) {
			ret = qspi_dll_config_bypass(config, port);
		} else {
			ret = qspi_dll_config_update(config, port);
		}
		if (ret) {
			break;
		}
	}

	return ret;
}
#endif /* QSPI_HAS_FEATURE(dll) */

static int qspi_init(const struct device *dev)
{
	struct qspi_data *data = dev->data;
	const struct qspi_config *config = dev->config;
	int ret;

	if (pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT)) {
		return -EIO;
	}

	k_sem_init(&data->sem, 1, 1);

	/* Disable controller */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_MDIS(1U));

	qspi_config_ports(config);
#if QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap)
	qspi_config_flash_address(config);
#endif /* QSPI_HAS_FEATURE(sfacr_cas) || QSPI_HAS_FEATURE(sfacr_byte_swap) */
	qspi_config_sampling(config);
	qspi_config_cs(config);
	qspi_config_buffers(config);
	qspi_config_read_options(config);
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options)
	qspi_config_chip_options(config);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options) */

	/* Enable controller */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) & ~QSPI_MCR_MDIS_MASK);

	/* Reset AHB and serial flash domains, configuration registers are not affected */
	qspi_sw_reset(config);

#if QSPI_HAS_FEATURE(dll)
	ret = qspi_config_dll(config);
	if (ret) {
		REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_MDIS(1U));
		LOG_ERR("Fail to initialize QSPI controller (%d)", ret);
		return -EIO;
	}
#endif /* QSPI_HAS_FEATURE(dll) */

	/* Unlock LUTs */
	REG_WRITE(QSPI_LUTKEY, QSPI_LUTKEY_KEY(QSPI_LUT_KEY));
	REG_WRITE(QSPI_LCKCR, QSPI_LCKCR_UNLOCK(1U));

	/* Clear all errors flags */
	REG_WRITE(QSPI_FR, 0xFFFFFFFFUL);

	return ret;
}

static inline uint32_t qspi_get_port_mem_base_addr(const struct qspi_config *config,
						   enum memc_nxp_qspi_port port)
{
	__ASSERT_NO_MSG(port < config->controller->ports_cnt * QSPI_MAX_CS_PER_PORT);
	return (port == NXP_QSPI_PORT_A1) ? config->controller->amba_base
					  : REG_READ(QSPI_SFAD(port - 1));
}

static inline int qspi_get_status(const struct qspi_config *config)
{
	int ret = 0;
	uint32_t err_mask = QSPI_FR_TBUF_MASK | QSPI_FR_ILLINE_MASK | QSPI_FR_RBOF_MASK |
			    QSPI_FR_IPAEF_MASK | QSPI_FR_IPIEF_MASK | QSPI_FR_AIBSEF_MASK;

	if (FIELD_GET(QSPI_SR_BUSY_MASK, REG_READ(QSPI_SR)) != 0U) {
		ret = -EBUSY;
	} else if (FIELD_GET(err_mask, REG_READ(QSPI_FR)) != 0U) {
		REG_WRITE(QSPI_FR, err_mask);
		ret = -EIO;
	}

	return ret;
}

static inline void qspi_config_lut_seq(const struct qspi_config *config,
				       nxp_qspi_lut_seq_t *lut_seq)
{
	uint32_t vlut[QSPI_LUT_SEQ_SIZE] = {QSPI_LUT_SEQ_END};
	uint16_t cmd0 = QSPI_LUT_CMD_INVALID;
	uint16_t cmd1 = QSPI_LUT_CMD_INVALID;
	uint8_t i = 0;
	uint8_t j = 0;

	/* Populate the virtual LUT sequence */
	while ((cmd1 != QSPI_LUT_SEQ_END) && (j < QSPI_LUT_SEQ_SIZE)) {
		cmd0 = (*lut_seq)[i++];
		cmd1 = (cmd0 == QSPI_LUT_SEQ_END) ? QSPI_LUT_SEQ_END : (*lut_seq)[i++];
		vlut[j++] = QSPI_LUT_PACK_REG(cmd0, cmd1);
		__ASSERT_NO_MSG(i < NXP_QSPI_LUT_MAX_CMD);
	}

	/* Commit the changes to the physical LUT */
	for (j = 0; j < QSPI_LUT_SEQ_SIZE; j++) {
		REG_WRITE(QSPI_LUT(QSPI_LUT_SEQID_IP + j), vlut[j]);
	}
}

static inline uint32_t get_word_size(uint32_t remaining_size)
{
	return (remaining_size > 4U) ? 4U : remaining_size;
}

static inline int qspi_read_rx_buf(const struct qspi_config *config, uint8_t *dest, uint32_t size,
				   uint32_t padding)
{
	uint8_t *data = dest;
	uint32_t bytes_remaining = size;
	uint32_t bytes_cnt = 0U;
	uint32_t words_cnt = 0U;
	uint32_t word_size;
	uint32_t buf;

	__ASSERT_NO_MSG(dest != NULL);

	if (padding != 0U) {
		/* Ignore all padding words, jump to the fist data */
		words_cnt = padding >> 2U;
		bytes_remaining -= words_cnt << 2U;
		padding &= 0x3U;

		buf = REG_READ(QSPI_RBDR(words_cnt));
		word_size = get_word_size(bytes_remaining);
		/* Ignore padding bytes and copy the rest to buffer */
		for (bytes_cnt = padding; bytes_cnt < word_size; bytes_cnt++) {
#if defined(CONFIG_BIG_ENDIAN)
			*data = ((uint8_t *)&buf)[3U - bytes_cnt];
#else
			*data = ((uint8_t *)&buf)[bytes_cnt];
#endif /* CONFIG_BIG_ENDIAN */
			data++;
		}
		bytes_remaining -= word_size;
		words_cnt++;
	}

	/* Read aligned addresses, 4 bytes at a time */
	if (((uintptr_t)data & 0x3U) == 0U) {
		while (bytes_remaining >= 4U) {
			*((uint32_t *)data) = REG_READ(QSPI_RBDR(words_cnt));
			data = &data[4U];
			words_cnt++;
			bytes_remaining -= 4U;
		}
	}

	/* Process remaining bytes one by one */
	while (bytes_remaining > 0U) {
		buf = REG_READ(QSPI_RBDR(words_cnt));
		word_size = get_word_size(bytes_remaining);
		for (bytes_cnt = 0U; bytes_cnt < word_size; bytes_cnt++) {
#if defined(CONFIG_BIG_ENDIAN)
			*data = (uint8_t)(buf >> 24U);
			buf <<= 8U;
#else
			*data = (uint8_t)(buf & 0xFFU);
			buf >>= 8U;
#endif /* CONFIG_BIG_ENDIAN */
			data++;
			bytes_remaining--;
		}
		words_cnt++;
	}

	return 0;
}

static void qspi_fill_tx_buf(const struct qspi_config *config, const uint8_t *src, uint32_t size,
			     uint8_t pre_padding, uint8_t post_padding)
{
	uint32_t buf = 0xFFFFFFFFUL;
	uint8_t *pbuf = (uint8_t *)(&buf);
	const uint8_t *psrc = src;
	uint32_t remaining_size = size;
	uint32_t word_size;
	uint32_t bytes_cnt;

	__ASSERT_NO_MSG(src != NULL);

	/* Insert pre_padding words */
	while (pre_padding >= 4U) {
		REG_WRITE(QSPI_TBDR, 0xFFFFFFFFUL);
		pre_padding -= 4U;
	}

	if (pre_padding != 0U) {
		word_size = pre_padding + remaining_size;

		if (word_size > 4U) {
			word_size = 4U;
			remaining_size -= (4U - pre_padding);
		} else {
			/*
			 * When (pre_padding + data + post_padding) fit into a word, decreasing
			 * post_padding is not needed because data have been already consumed
			 */
			remaining_size = 0U;
		}

		/* Fill user data between pre_padding and post_padding */
		for (bytes_cnt = pre_padding; bytes_cnt < word_size; bytes_cnt++) {
#if defined(CONFIG_BIG_ENDIAN)
			pbuf[3U - bytes_cnt] = *psrc;
#else
			pbuf[bytes_cnt] = *psrc;
#endif /* CONFIG_BIG_ENDIAN */
			psrc++;
		}

		REG_WRITE(QSPI_TBDR, buf);
	}

	/* Check user buffer alignment */
	if (((uint32_t)psrc & 0x3U) == 0U) {
		/* Process 4 bytes at a time to speed things up */
		while (remaining_size >= 4U) {
			buf = *(const uint32_t *)((uint32_t)psrc);
			remaining_size -= 4U;
			psrc = &(psrc[4U]);
			REG_WRITE(QSPI_TBDR, buf);
		}
	}

	/* Process remaining bytes one by one */
	while (remaining_size > 0U) {
		/* Processes last few buf bytes (less than 4) */
		buf = 0xFFFFFFFFUL;
		word_size = get_word_size(remaining_size);

		for (bytes_cnt = 0U; bytes_cnt < word_size; bytes_cnt++) {
#if defined(CONFIG_BIG_ENDIAN)
			pbuf[3U - bytes_cnt] = *psrc;
#else
			pbuf[bytes_cnt] = *psrc;
#endif /* CONFIG_BIG_ENDIAN */

			psrc++;
		}
		REG_WRITE(QSPI_TBDR, buf);
		remaining_size -= word_size;
	}

	/* Insert post_padding words */
	while (post_padding >= 4U) {
		REG_WRITE(QSPI_TBDR, 0xFFFFFFFFUL);
		post_padding -= 4U;
	}
}

static inline void qspi_invalidate_tx_buf(const struct qspi_config *config)
{
	volatile uint32_t delay = QSPI_TX_BUFFER_RESET_DELAY_CYCLES;

	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_CLR_TXF_MASK);
	while (delay--)
		;
}

static void qspi_flush_ahb(const struct qspi_config *config)
{
#if QSPI_HAS_FEATURE(sptrclr_abrt_clr)
	/* Use the AHB buffer clear bit to avoid losing the DLL lock */
	REG_WRITE(QSPI_SPTRCLR, QSPI_SPTRCLR_ABRT_CLR_MASK);
	__ASSERT_NO_MSG(
		WAIT_FOR(FIELD_GET(QSPI_SPTRCLR_ABRT_CLR_MASK, REG_READ(QSPI_SPTRCLR) == 0U),
			 QSPI_CMD_COMPLETE_TIMEOUT_US, NULL));
#else
	/* Reset AHB and serial flash domains, configuration registers are not affected */
	qspi_sw_reset(config);
#endif /* QSPI_HAS_FEATURE(sptrclr_abrt_clr) */
}

static int qspi_ip_command(const struct qspi_config *config, uint32_t addr)
{
	int ret = 0;

	/* Reset AHB buffers to force re-read from memory after erase operation */
	qspi_flush_ahb(config);

	/* Trigger IP command */
	REG_WRITE(QSPI_SFAR, addr);
	REG_WRITE(QSPI_IPCR, QSPI_IPCR_SEQID(QSPI_LUT_SEQID_IP));
	if (!WAIT_FOR(qspi_get_status(config) == 0U, QSPI_CMD_COMPLETE_TIMEOUT_US, NULL)) {
		ret = -ETIMEDOUT;
	}

	/* Make sure there is no garbage in rx fifo in case of a dummy read command */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_CLR_RXF_MASK);

	return ret;
}

static int qspi_ip_write(const struct qspi_config *config, uint32_t addr, const uint8_t *src,
			 uint32_t size, uint8_t alignment)
{
	int ret = 0;
	uint16_t total_size = 0U;
	uint32_t alignment_mask = (uint32_t)alignment - 1UL;
	uint32_t pre_padding = addr & alignment_mask;
	uint32_t post_padding = alignment_mask - ((addr + size - 1UL) & alignment_mask);

	__ASSERT_NO_MSG(size <= config->controller->tx_fifo_size);
	__ASSERT_NO_MSG(src != NULL);

	addr -= pre_padding;
	total_size = (uint16_t)(size + pre_padding + post_padding);

	/* Reset AHB buffers to force re-read from memory after write operation */
	qspi_flush_ahb(config);

	/* Ensure there is no garbage in Tx FIFO */
	qspi_invalidate_tx_buf(config);

	/* Set write address */
	REG_WRITE(QSPI_SFAR, addr);

	qspi_fill_tx_buf(config, src, size, pre_padding, post_padding);

	/* Trigger IP command */
	REG_WRITE(QSPI_IPCR, QSPI_IPCR_SEQID(QSPI_LUT_SEQID_IP) | QSPI_IPCR_IDATSZ(total_size));
	if (!WAIT_FOR(qspi_get_status(config) == 0U, QSPI_CMD_COMPLETE_TIMEOUT_US, NULL)) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int qspi_ip_read(const struct qspi_config *config, uint32_t addr, uint8_t *dest,
			uint32_t size, uint32_t padding)
{
	uint16_t data_size = 0U;
	int ret = 0U;

	__ASSERT_NO_MSG(size <= config->controller->rx_fifo_size);

	/* If size is odd, round up to even size; this is needed in octal DDR mode */
	data_size = (uint16_t)((size + 1U) & (~1U));

	/* Make sure there is no garbage in rx fifo */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_CLR_RXF_MASK);

	/* Set read address */
	REG_WRITE(QSPI_SFAR, addr);

	/* Trigger IP command */
	REG_WRITE(QSPI_IPCR, QSPI_IPCR_SEQID(QSPI_LUT_SEQID_IP) | QSPI_IPCR_IDATSZ(data_size));
	if (!WAIT_FOR(qspi_get_status(config) == 0U, QSPI_CMD_COMPLETE_TIMEOUT_US, NULL)) {
		ret = -ETIMEDOUT;
	} else if (dest != NULL) {
		ret = qspi_read_rx_buf(config, dest, size, padding);
	}

	/* Reset rx fifo */
	REG_WRITE(QSPI_MCR, REG_READ(QSPI_MCR) | QSPI_MCR_CLR_RXF_MASK);

	return ret;
}

static int qspi_ip_read_all(const struct qspi_config *config, uint32_t addr, uint8_t *dest,
			    uint32_t size, uint8_t alignment)
{
	uint32_t len = config->controller->rx_fifo_size;
	uint8_t *pbuf = dest;
	uint32_t padding = 0U;
	int ret = 0;

	/* Handle transfer alignment requirements for the first read */
	if (alignment > 0U) {
		padding = addr & ((uint32_t)alignment - 1U);
		if (padding != 0U) {
			addr -= padding;
			size += padding;
		}
	}

	while (!ret && (size > 0U)) {
		len = MIN(size, len);
		ret = qspi_ip_read(config, addr, pbuf, len, padding);
		size -= len;
		addr += len;
		pbuf = &pbuf[len - padding];
		padding = 0;
	}

	return ret;
}

int memc_nxp_qspi_transfer(const struct device *dev, struct memc_nxp_qspi_config *transfer)
{
	const struct qspi_config *config = dev->config;
	uint32_t addr;
	int ret = 0;

	if (transfer->port > (config->controller->ports_cnt * QSPI_MAX_CS_PER_PORT)) {
		LOG_ERR("Invalid port %u", transfer->port);
		return -EINVAL;
	}

	if ((transfer->cmd == NXP_QSPI_WRITE) && (transfer->data == NULL)) {
		LOG_ERR("Invalid data source pointer");
		return -EINVAL;
	}

	addr = qspi_get_port_mem_base_addr(config, transfer->port) + transfer->addr;

	qspi_acquire(dev);

	qspi_config_lut_seq(config, transfer->lut_seq);

	switch (transfer->cmd) {
	case NXP_QSPI_COMMAND:
		ret = qspi_ip_command(config, addr);
		break;
	case NXP_QSPI_READ:
		ret = qspi_ip_read_all(config, addr, transfer->data, transfer->size,
				       transfer->alignment);
		break;
	case NXP_QSPI_WRITE:
		ret = qspi_ip_write(config, addr, transfer->data, transfer->size,
				    transfer->alignment);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	qspi_release(dev);

	return ret;
}

int memc_nxp_qspi_get_status(const struct device *dev)
{
	const struct qspi_config *config = dev->config;
	int ret;

	qspi_acquire(dev);
	ret = qspi_get_status(config);
	qspi_release(dev);

	return ret;
}

#define BITS_TO_BYTES(node_id, prop, idx) (DT_PROP_BY_IDX(node_id, prop, idx) / 8),

#define QSPI_PORT_NODE(n, name) DT_INST_CHILD(n, port_##name)

#define QSPI_FOREACH_PORT(n, fn)                                                                   \
	IF_ENABLED(DT_NODE_EXISTS(QSPI_PORT_NODE(n, a)), (fn(QSPI_PORT_NODE(n, a))))               \
	IF_ENABLED(DT_NODE_EXISTS(QSPI_PORT_NODE(n, b)), (fn(QSPI_PORT_NODE(n, b))))

#define QSPI_PORT_CONFIG_VERIFY(port_node)                                                         \
	BUILD_ASSERT(_CONCAT(QSPI_READ_MODE_, DT_STRING_UPPER_TOKEN(port_node, rx_clock_source)) + \
			     1U,                                                                   \
		     "rx-clock-source source mode selected is not supported");                     \
	BUILD_ASSERT(DT_PROP_LEN(port_node, memory_sizes) == QSPI_MAX_CS_PER_PORT,                 \
		     "memory-sizes array must be of length 2");

/* clang-format off */
#define QSPI_PORT_CONFIG(port_node)                                                                \
	{                                                                                          \
		.memory_sizes = {DT_FOREACH_PROP_ELEM(port_node, memory_sizes, BITS_TO_BYTES)},    \
		.read_mode = _CONCAT(QSPI_READ_MODE_,                                              \
				     DT_STRING_UPPER_TOKEN(port_node, rx_clock_source)),           \
		IF_ENABLED(QSPI_HAS_FEATURE(mcr_isd), (                                            \
		.io2_idle = DT_PROP(port_node, io2_idle_high),                                     \
		.io3_idle = DT_PROP(port_node, io3_idle_high),                                     \
		))                                                                                 \
		IF_ENABLED(QSPI_HAS_FEATURE(dll), (                                                \
		.dll = {                                                                           \
			.mode = _CONCAT(QSPI_DLL_, DT_STRING_UPPER_TOKEN(port_node, dll_mode)),    \
			.freq_enable = DT_PROP(port_node, dll_freq_enable),                        \
			.coarse_delay = DT_PROP_OR(port_node, dll_coarse_delay, 0U),               \
			.fine_delay = DT_PROP_OR(port_node, dll_fine_delay, 0U),                   \
			.tap_select = DT_PROP_OR(port_node, dll_tap_select, 0U),                   \
			IF_ENABLED(QSPI_HAS_FEATURE(dllcr_loop_control), (                         \
			.reference_counter = DT_PROP_OR(port_node, dll_ref_counter, 1U),           \
			.resolution = DT_PROP_OR(port_node, dll_resolution, 2U),                   \
			))                                                                         \
		},))                                                                               \
	},

#define QSPI_CONTROLLER_CONFIG(n)                                                                  \
	static uint16_t memc_nxp_qspi_rx_buffers_cfg_##n[] =                                       \
		DT_INST_PROP_OR(n, ahb_buffers, {0});                                              \
                                                                                                   \
	static const struct qspi_port memc_nxp_qspi_ports_cfg_##n[] = {                            \
		QSPI_FOREACH_PORT(n, QSPI_PORT_CONFIG)                                             \
	};                                                                                         \
                                                                                                   \
	static const struct qspi_controller memc_nxp_qspi_controller_cfg_##n = {                   \
		.amba_base = (mem_addr_t)DT_INST_REG_ADDR_BY_NAME(n, qspi_amba),                   \
		IF_ENABLED(QSPI_HAS_FEATURE(ddr), (                                                \
		.ddr = (bool)DT_INST_ENUM_IDX(n, data_rate),                                       \
		))                                                                                 \
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_options), (                       \
		.chip_options = DT_INST_PROP(n, chip_options),                                     \
		))                                                                                 \
		.timing = {                                                                        \
			IF_ENABLED(QSPI_HAS_FEATURE(ddr), (                                        \
			.aligned_2x_refclk = DT_INST_PROP(n, hold_time_2x),                        \
			))                                                                         \
			.cs = {                                                                    \
				.hold_time = DT_INST_PROP(n, cs_hold_time),                        \
				.setup_time = DT_INST_PROP(n, cs_setup_time),                      \
			},                                                                         \
			.sampling = {                                                              \
				.delay_half_cycle = DT_INST_PROP(n, sample_delay_half_cycle),      \
				.phase_inverted = DT_INST_PROP(n, sample_phase_inverted),          \
			},                                                                         \
		},                                                                                 \
		.rx_buffers = {                                                                    \
			.buffers = (struct qspi_buffer *)memc_nxp_qspi_rx_buffers_cfg_##n,         \
			.buffers_cnt = (sizeof(memc_nxp_qspi_rx_buffers_cfg_##n) /                 \
					sizeof(struct qspi_buffer)),                               \
			.all_masters = (bool)DT_INST_PROP(n, ahb_buffers_all_masters),             \
		},                                                                                 \
		IF_ENABLED(UTIL_OR(QSPI_HAS_FEATURE(sfacr_cas),                                    \
				   QSPI_HAS_FEATURE(sfacr_byte_swap)), (                           \
		.address = {                                                                       \
			IF_ENABLED(QSPI_HAS_FEATURE(sfacr_cas), (                                  \
			.column_space = DT_INST_PROP(n, column_space),                             \
			.word_addressable = DT_INST_PROP(n, word_addressable),                     \
			))                                                                         \
			IF_ENABLED(QSPI_HAS_FEATURE(sfacr_byte_swap), (                            \
			.byte_swap = DT_INST_PROP(n, byte_swapping),                               \
			))                                                                         \
		},))                                                                               \
		.ports = (struct qspi_port *)memc_nxp_qspi_ports_cfg_##n,                          \
		.ports_cnt = ARRAY_SIZE(memc_nxp_qspi_ports_cfg_##n),                              \
		.tx_fifo_size = DT_INST_PROP(n, tx_fifo_size),                                     \
		.rx_fifo_size = DT_INST_PROP(n, rx_fifo_size),                                     \
		.lut_size = DT_INST_PROP(n, lut_size),                                             \
	}
/* clang-format on */

#define QSPI_INIT_DEVICE(n)                                                                        \
	QSPI_FOREACH_PORT(n, QSPI_PORT_CONFIG_VERIFY);                                             \
	QSPI_CONTROLLER_CONFIG(n);                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct qspi_data memc_nxp_s32_qspi_data_##n;                                        \
	static const struct qspi_config memc_nxp_s32_qspi_config_##n = {                           \
		.base = (mem_addr_t)DT_INST_REG_ADDR_BY_NAME(n, qspi),                             \
		.controller = &memc_nxp_qspi_controller_cfg_##n,                                   \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, qspi_init, NULL, &memc_nxp_s32_qspi_data_##n,                     \
			      &memc_nxp_s32_qspi_config_##n, POST_KERNEL,                          \
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(QSPI_INIT_DEVICE)
