/**
 * sci_ra_piv.h
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * @brief RA SCI registers definitions. Common for all (UART/I2C/SPI) SCI modes
 *
 * Based on Renesas CMSIS header
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __SCI_RA_PRIV_H__
#define __SCI_RA_PRIV_H__

/* ===========================  RDR  ================================ */
#define R_SCI_RDR             0x05          /*!< offset: 0x05	      */
#define R_SCI_RDR_RDR_Pos     (0UL)         /*!< RDR (Bit 0)          */
#define R_SCI_RDR_RDR_Msk     GENMASK(7, 0) /*!< RDR (bmask 0xff)     */

/* =========================  RDRHL  ================================ */
#define R_SCI_RDRHL          0x10          /*!< offset: 0x010         */
#define R_SCI_RDRHL_RDAT_Pos (0UL)         /*!< RDRHL (Bit 0)         */
#define R_SCI_RDRHL_RDAT_Msk GENMASK(8, 0) /*!< RDRHL (bmask: 0x1ff)  */

/* =========================  FRDRHL  =============================== */
#define R_SCI_FRDRHL         0x10          /*!< offset: 0x010         */
#define R_SCI_FRDRHL_RDF_Pos       (14UL)     /*!< RDF (Bit 14)       */
#define R_SCI_FRDRHL_RDF_Msk       (0x4000UL) /*!< RDF (bmask 0x01)   */
#define R_SCI_FRDRHL_ORER_Pos      (13UL)     /*!< ORER (Bit 13)      */
#define R_SCI_FRDRHL_ORER_Msk      (0x2000UL) /*!< ORER (bmask 0x01)  */
#define R_SCI_FRDRHL_FER_Pos       (12UL)     /*!< FER (Bit 12)       */
#define R_SCI_FRDRHL_FER_Msk       (0x1000UL) /*!< FER (bmask 0x01)   */
#define R_SCI_FRDRHL_PER_Pos       (11UL)    /*!< PER (Bit 11)        */
#define R_SCI_FRDRHL_PER_Msk       (0x800UL) /*!< PER (bmask 0x01)    */
#define R_SCI_FRDRHL_DR_Pos        (10UL)    /*!< DR (Bit 10)         */
#define R_SCI_FRDRHL_DR_Msk        (0x400UL) /*!< DR (bmask 0x01)     */
#define R_SCI_FRDRHL_MPB_Pos       (9UL)     /*!< MPB (Bit 9)         */
#define R_SCI_FRDRHL_MPB_Msk       (0x200UL) /*!< MPB (bmask 0x01)    */
#define R_SCI_FRDRHL_RDAT_Pos      (0UL)     /*!< RDAT (Bit 0)        */
#define R_SCI_FRDRHL_RDAT_Msk  GENMASK(8, 0) /*!< RDAT (bmask 0x1ff)  */

/* =========================  FRDRH  ================================ */
#define R_SCI_FRDRH               0x10          /*!< offset: 0x010    */
#define R_SCI_FRDRH_RDF_Pos       (6UL)      /*!< RDF (Bit 6)         */
#define R_SCI_FRDRH_RDF_Msk       (0x40UL)   /*!< RDF (bmask 0x01)    */
#define R_SCI_FRDRH_ORER_Pos      (5UL)      /*!< ORER (Bit 5)        */
#define R_SCI_FRDRH_ORER_Msk      (0x20UL)   /*!< ORER (bmask 0x01)   */
#define R_SCI_FRDRH_FER_Pos       (4UL)      /*!< FER (Bit 4)         */
#define R_SCI_FRDRH_FER_Msk       (0x10UL)   /*!< FER (bmask 0x01)    */
#define R_SCI_FRDRH_PER_Pos       (3UL)      /*!< PER (Bit 3)         */
#define R_SCI_FRDRH_PER_Msk       (0x8UL)    /*!< PER (bmask 0x01)    */
#define R_SCI_FRDRH_DR_Pos        (2UL)      /*!< DR (Bit 2)          */
#define R_SCI_FRDRH_DR_Msk        (0x4UL)    /*!< DR (bmask 0x01)     */
#define R_SCI_FRDRH_MPB_Pos       (1UL)      /*!< MPB (Bit 1)         */
#define R_SCI_FRDRH_MPB_Msk       (0x2UL)    /*!< MPB (bmask 0x01)    */
#define R_SCI_FRDRH_RDATH_Pos     (0UL)      /*!< RDATH (Bit 0)       */
#define R_SCI_FRDRH_RDATH_Msk     (0x1UL)    /*!< RDATH (bmask 0x01)  */

/* =========================  FRDRL  ================================ */
#define R_SCI_FRDRL               0x11       /*!< offset: 0x010       */
#define R_SCI_FRDRL_RDATL_Pos     (0UL)      /*!< RDATL (Bit 0)       */
#define R_SCI_FRDRL_RDATL_Msk     (0xffUL)   /*!< RDATL (bmask 0xff)  */

/* =========================  TDR  ================================== */
#define R_SCI_TDR                 0x03       /*!< offset: 0x03        */
#define R_SCI_TDR_TDR_Pos         (0UL)      /*!< TDR (Bit 0)         */
#define R_SCI_TDR_TDR_Msk    GENMASK(7, 0)   /*!< TDR (bmask 0xff)    */

/* =========================  TDRHL  ================================ */
#define R_SCI_TDRHL             0x0e       /*!< offset: 0x0e          */
#define R_SCI_TDRHL_TDAT_Pos  (0UL)       /*!< TDAT (Bit 0)           */
#define R_SCI_TDRHL_TDAT_Msk GENMASK(8, 0) /*!< TDAT (bmask 0x1ff)     */
#define R_SCI_TDRHL_TDAT(x)  \
	(((x) & R_SCI_TDRHL_TDAT_Msk) | GENMASK(15, 9))

/* =========================  FTDRHL  =============================== */
#define R_SCI_FTDRHL            0x0e        /*!< offset: 0x0e         */
#define R_SCI_FTDRHL_MPBT_Pos      (9UL)     /*!< MPBT (Bit 9)        */
#define R_SCI_FTDRHL_MPBT_Msk      (0x200UL) /*!< MPBT (bmask 0x01)   */
#define R_SCI_FTDRHL_TDAT_Pos      (0UL)     /*!< TDAT (Bit 0)        */
#define R_SCI_FTDRHL_TDAT_Msk  GENMASK(8, 0) /*!< TDAT (bmask 0x1ff)  */

/* ==========================  FTDRH  =============================== */
#define R_SCI_FTDRH             0x0e        /*!< offset: 0x0e         */
#define R_SCI_FTDRH_MPBT_Pos      (1UL)     /*!< MPBT (Bit 1)         */
#define R_SCI_FTDRH_MPBT_Msk      (0x2UL)   /*!< MPBT (bmask 0x01)    */
#define R_SCI_FTDRH_TDATH_Pos     (0UL)     /*!< TDATH (Bit 0)        */
#define R_SCI_FTDRH_TDATH_Msk     (0x1UL)   /*!< TDATH (bmask 0x01)   */

/* ==========================  FTDRL  =============================== */
#define R_SCI_FTDRL             0x0f         /*!< offset: 0x0f        */
#define R_SCI_FTDRL_TDATL_Pos   (0UL)        /*!< TDATL (Bit 0)       */
#define R_SCI_FTDRL_TDATL_Msk   GENMASK(7, 0) /*!< TDATL (bmask 0xff)  */

/* ==========================  SMR  ================================= */
#define R_SCI_SMR                  0x00      /*!< offset: 0x00        */
#define R_SCI_SMR_CM_Pos           (7UL)     /*!< CM (Bit 7 )         */
#define R_SCI_SMR_CM_Msk           (0x80UL)  /*!< CM (bmask 0x01)     */
#define R_SCI_SMR_CHR_Pos          (6UL)     /*!< CHR (Bit 6)         */
#define R_SCI_SMR_CHR_Msk          (0x40UL)  /*!< CHR (bmask 0x01)    */
#define R_SCI_SMR_PE_Pos           (5UL)     /*!< PE (Bit 5)          */
#define R_SCI_SMR_PE_Msk           (0x20UL)  /*!< PE (bmask 0x01)     */
#define R_SCI_SMR_PM_Pos           (4UL)     /*!< PM (Bit 4)          */
#define R_SCI_SMR_PM_Msk           (0x10UL)  /*!< PM (bmask 0x01)     */
#define R_SCI_SMR_STOP_Pos         (3UL)     /*!< STOP (Bit 3)        */
#define R_SCI_SMR_STOP_Msk         (0x8UL)   /*!< STOP (bmask 0x01)   */
#define R_SCI_SMR_MP_Pos           (2UL)     /*!< MP (Bit 2)          */
#define R_SCI_SMR_MP_Msk           (0x4UL)   /*!< MP (bmask 0x01)     */
#define R_SCI_SMR_CKS_Pos          (0UL)     /*!< CKS (Bit 0)         */
#define R_SCI_SMR_CKS_Msk      GENMASK(1, 0) /*!< CKS (bmask 0x03)    */
#define R_SCI_SMR_CKS(x)	\
	(((x)<<R_SCI_SMR_CKS_Pos) & R_SCI_SMR_CKS_Msk)

/* ========================  SMR_SMCI  ============================== */
#define R_SCI_SMR_SMCI              0x00      /*!< offset: 0x00       */
#define R_SCI_SMR_SMCI_GM_Pos       (7UL)    /*!< GM (Bit 7)          */
#define R_SCI_SMR_SMCI_GM_Msk       (0x80UL) /*!< GM (bmask 0x01)     */
#define R_SCI_SMR_SMCI_BLK_Pos      (6UL)    /*!< BLK (Bit 6)         */
#define R_SCI_SMR_SMCI_BLK_Msk      (0x40UL) /*!< BLK (bmask 0x01)    */
#define R_SCI_SMR_SMCI_PE_Pos       (5UL)    /*!< PE (Bit 5)          */
#define R_SCI_SMR_SMCI_PE_Msk       (0x20UL) /*!< PE (bmask 0x01)     */
#define R_SCI_SMR_SMCI_PM_Pos       (4UL)    /*!< PM (Bit 4)          */
#define R_SCI_SMR_SMCI_PM_Msk       (0x10UL) /*!< PM (bmask 0x01)     */
#define R_SCI_SMR_SMCI_BCP_Pos      (2UL)    /*!< BCP (Bit 2)         */
#define R_SCI_SMR_SMCI_BCP_Msk      (0xcUL)  /*!< BCP (bmask 0x03)    */
#define R_SCI_SMR_SMCI_CKS_Pos      (0UL)    /*!< CKS (Bit 0)         */
#define R_SCI_SMR_SMCI_CKS_Msk     GENMASK(1, 0) /*!< CKS (bmask 0x03)*/

/* ==========================  SCR  ================================= */
#define R_SCI_SCR                    0x02      /*!< offset: 0x02      */
#define R_SCI_SCR_TIE_Pos            (7UL)      /*!< TIE (Bit 7)      */
#define R_SCI_SCR_TIE_Msk	\
	BIT(R_SCI_SCR_TIE_Pos)       /*!< TIE (bmask 0x01) */
#define R_SCI_SCR_RIE_Pos            (6UL)      /*!< RIE (Bit 6)      */
#define R_SCI_SCR_RIE_Msk	\
	BIT(R_SCI_SCR_RIE_Pos)       /*!< RIE (bmask 0x01) */
#define R_SCI_SCR_TE_Pos             (5UL)      /*!< TE (Bit 5)       */
#define R_SCI_SCR_TE_Msk	\
	BIT(R_SCI_SCR_TE_Pos)       /*!< TE (bmask 0x01)  */
#define R_SCI_SCR_RE_Pos             (4UL)      /*!< RE (Bit 4)       */
#define R_SCI_SCR_RE_Msk	\
	BIT(R_SCI_SCR_RE_Pos)       /*!< RE (bmask 0x01)  */
#define R_SCI_SCR_MPIE_Pos           (3UL)      /*!< MPIE (Bit 3)     */
#define R_SCI_SCR_MPIE_Msk	\
	BIT(R_SCI_SCR_MPIE_Pos)       /*!< MPIE (bmask 0x01)*/
#define R_SCI_SCR_TEIE_Pos           (2UL)      /*!< TEIE (Bit 2)     */
#define R_SCI_SCR_TEIE_Msk	\
	BIT(R_SCI_SCR_TEIE_Pos)       /*!< TEIE (bmask 0x01)*/
#define R_SCI_SCR_CKE_Pos            (0UL)      /*!< CKE (Bit 0)      */
#define R_SCI_SCR_CKE_Msk           GENMASK(1, 0) /*!< CKE(bmask 0x03)*/

/* ============================  SCR_SMCI  ========================== */
#define R_SCI_SCR_SMCI              0x02     /*!< offset: 0x02        */
#define R_SCI_SCR_SMCI_TIE_Pos      (7UL)    /*!< TIE (Bit 7)         */
#define R_SCI_SCR_SMCI_TIE_Msk      (0x80UL) /*!< TIE (bmask 0x01)    */
#define R_SCI_SCR_SMCI_RIE_Pos      (6UL)    /*!< RIE (Bit 6)         */
#define R_SCI_SCR_SMCI_RIE_Msk      (0x40UL) /*!< RIE (bmask 0x01)    */
#define R_SCI_SCR_SMCI_TE_Pos       (5UL)    /*!< TE (Bit 5)          */
#define R_SCI_SCR_SMCI_TE_Msk       (0x20UL) /*!< TE (bmask 0x01)     */
#define R_SCI_SCR_SMCI_RE_Pos       (4UL)    /*!< RE (Bit 4)          */
#define R_SCI_SCR_SMCI_RE_Msk       (0x10UL) /*!< RE (bmask 0x01)     */
#define R_SCI_SCR_SMCI_MPIE_Pos     (3UL)    /*!< MPIE (Bit 3)        */
#define R_SCI_SCR_SMCI_MPIE_Msk     (0x8UL)  /*!< MPIE (bmask 0x01)   */
#define R_SCI_SCR_SMCI_TEIE_Pos     (2UL)    /*!< TEIE (Bit 2)        */
#define R_SCI_SCR_SMCI_TEIE_Msk     (0x4UL)  /*!< TEIE (bmask 0x01)   */
#define R_SCI_SCR_SMCI_CKE_Pos      (0UL)    /*!< CKE (Bit 0)         */
#define R_SCI_SCR_SMCI_CKE_Msk      (0x3UL)  /*!< CKE (bmask 0x03)    */

/* ===========================  SSR  ================================ */
#define R_SCI_SSR                    0x04      /*!< offset: 0x04      */
#define R_SCI_SSR_TDRE_Pos           (7UL)      /*!< TDRE (Bit 7)     */
#define R_SCI_SSR_TDRE_Msk           (0x80UL)   /*!< TDRE (bmask 0x01)*/
#define R_SCI_SSR_RDRF_Pos           (6UL)      /*!< RDRF (Bit 6)     */
#define R_SCI_SSR_RDRF_Msk           (0x40UL)   /*!< RDRF (bmask 0x01)*/
#define R_SCI_SSR_ORER_Pos           (5UL)      /*!< ORER (Bit 5)     */
#define R_SCI_SSR_ORER_Msk           (0x20UL)   /*!< ORER (bmask 0x01)*/
#define R_SCI_SSR_FER_Pos            (4UL)      /*!< FER (Bit 4)      */
#define R_SCI_SSR_FER_Msk            (0x10UL)   /*!< FER (bmask 0x01) */
#define R_SCI_SSR_PER_Pos            (3UL)      /*!< PER (Bit 3)      */
#define R_SCI_SSR_PER_Msk            (0x8UL)    /*!< PER (bmask 0x01) */
#define R_SCI_SSR_TEND_Pos           (2UL)      /*!< TEND (Bit 2)     */
#define R_SCI_SSR_TEND_Msk           (0x4UL)    /*!< TEND (bmask 0x01)*/
#define R_SCI_SSR_MPB_Pos            (1UL)      /*!< MPB (Bit 1)      */
#define R_SCI_SSR_MPB_Msk            (0x2UL)    /*!< MPB (bmask 0x01) */
#define R_SCI_SSR_MPBT_Pos           (0UL)      /*!< MPBT (Bit 0)     */
#define R_SCI_SSR_MPBT_Msk           (0x1UL)    /*!< MPBT (bmask 0x01)*/

/* ==========================  SSR_FIFO  ============================ */
#define R_SCI_SSR_FIFO              0x04      /*!< offset: 0x04       */
#define R_SCI_SSR_FIFO_TDFE_Pos    (7UL)      /*!< TDFE (Bit 7)       */
#define R_SCI_SSR_FIFO_TDFE_Msk    (0x80UL)   /*!< TDFE (bmask 0x01)  */
#define R_SCI_SSR_FIFO_RDF_Pos     (6UL)      /*!< RDF (Bit 6)        */
#define R_SCI_SSR_FIFO_RDF_Msk     (0x40UL)   /*!< RDF (bmask 0x01)   */
#define R_SCI_SSR_FIFO_ORER_Pos    (5UL)      /*!< ORER (Bit 5)       */
#define R_SCI_SSR_FIFO_ORER_Msk    (0x20UL)   /*!< ORER (bmask 0x01)  */
#define R_SCI_SSR_FIFO_FER_Pos     (4UL)      /*!< FER (Bit 4)        */
#define R_SCI_SSR_FIFO_FER_Msk     (0x10UL)   /*!< FER (bmask 0x01)   */
#define R_SCI_SSR_FIFO_PER_Pos     (3UL)      /*!< PER (Bit 3)        */
#define R_SCI_SSR_FIFO_PER_Msk     (0x8UL)    /*!< PER (bmask 0x01)   */
#define R_SCI_SSR_FIFO_TEND_Pos    (2UL)      /*!< TEND (Bit 2)       */
#define R_SCI_SSR_FIFO_TEND_Msk    (0x4UL)    /*!< TEND (bmask 0x01)  */
#define R_SCI_SSR_FIFO_DR_Pos      (0UL)      /*!< DR (Bit 0)         */
#define R_SCI_SSR_FIFO_DR_Msk      (0x1UL)    /*!< DR (bmask 0x01)    */

/* ==========================  SSR_SMCI  ============================ */
#define R_SCI_SSR_SMCI               0x04       /*!< offset: 0x04     */
#define R_SCI_SSR_SMCI_TDRE_Pos      (7UL)      /*!< TDRE (Bit 7)     */
#define R_SCI_SSR_SMCI_TDRE_Msk      (0x80UL)   /*!< TDRE (bmask 0x01)*/
#define R_SCI_SSR_SMCI_RDRF_Pos      (6UL)      /*!< RDRF (Bit 6)     */
#define R_SCI_SSR_SMCI_RDRF_Msk      (0x40UL)   /*!< RDRF (bmask 0x01)*/
#define R_SCI_SSR_SMCI_ORER_Pos      (5UL)      /*!< ORER (Bit 5)     */
#define R_SCI_SSR_SMCI_ORER_Msk      (0x20UL)   /*!< ORER (bmask 0x01)*/
#define R_SCI_SSR_SMCI_ERS_Pos       (4UL)      /*!< ERS (Bit 4)      */
#define R_SCI_SSR_SMCI_ERS_Msk       (0x10UL)   /*!< ERS (bmask 0x01) */
#define R_SCI_SSR_SMCI_PER_Pos       (3UL)      /*!< PER (Bit 3)      */
#define R_SCI_SSR_SMCI_PER_Msk       (0x8UL)    /*!< PER (bmask 0x01) */
#define R_SCI_SSR_SMCI_TEND_Pos      (2UL)      /*!< TEND (Bit 2)     */
#define R_SCI_SSR_SMCI_TEND_Msk      (0x4UL)    /*!< TEND (bmask 0x01)*/
#define R_SCI_SSR_SMCI_MPB_Pos       (1UL)      /*!< MPB (Bit 1)      */
#define R_SCI_SSR_SMCI_MPB_Msk       (0x2UL)    /*!< MPB (bmask 0x01) */
#define R_SCI_SSR_SMCI_MPBT_Pos      (0UL)      /*!< MPBT (Bit 0)     */
#define R_SCI_SSR_SMCI_MPBT_Msk      (0x1UL)    /*!< MPBT (bmask 0x01)*/

/* ============================  SCMR  ============================== */
#define R_SCI_SCMR                  0x06      /*!< offset: 0x06       */
#define R_SCI_SCMR_BCP2_Pos         (7UL)      /*!< BCP2 (Bit 7)      */
#define R_SCI_SCMR_BCP2_Msk	\
		BIT(R_SCI_SCMR_BCP2_Pos)       /*!< BCP2 (bmask 0x01) */
#define R_SCI_SCMR_CHR1_Pos         (4UL)      /*!< CHR1 (Bit 4)      */
#define R_SCI_SCMR_CHR1_Msk	\
		BIT(R_SCI_SCMR_CHR1_Pos)       /*!< CHR1 (bmask 0x01) */
#define R_SCI_SCMR_SDIR_Pos         (3UL)      /*!< SDIR (Bit 3)      */
#define R_SCI_SCMR_SDIR_Msk	\
		BIT(R_SCI_SCMR_SDIR_Pos)       /*!< SDIR (bmask 0x01) */
#define R_SCI_SCMR_SINV_Pos         (2UL)      /*!< SINV (Bit 2)      */
#define R_SCI_SCMR_SINV_Msk	\
		BIT(R_SCI_SCMR_SINV_Pos)       /*!< SINV (bmask 0x01) */
#define R_SCI_SCMR_SMIF_Pos         (0UL)      /*!< SMIF (Bit 0)      */
#define R_SCI_SCMR_SMIF_Msk	\
		BIT(R_SCI_SCMR_SMIF_Pos)       /*!< SMIF (bmask 0x01) */

/* =============================  BRR  ============================== */
#define R_SCI_BRR                   0x01     /*!< offset: 0x01        */
#define R_SCI_BRR_BRR_Pos           (0UL)    /*!< BRR (Bit 0)         */
#define R_SCI_BRR_BRR_Msk      GENMASK(7, 0) /*!< BRR (bmask 0xff)    */
#define R_SCI_BRR_BRR(x)	\
	(((x)<<R_SCI_BRR_BRR_Pos) & R_SCI_BRR_BRR_Msk)

/* =============================  MDDR  ============================= */
#define R_SCI_MDDR                   0x12      /*!< offset: 0x12      */
#define R_SCI_MDDR_MDDR_Pos          (0UL)     /*!< MDDR (Bit 0)      */
#define R_SCI_MDDR_MDDR_Msk    GENMASK(7, 0)   /*!< MDDR (bmask 0xff) */

/* ============================  SEMR  ============================== */
#define R_SCI_SEMR                0x07      /*!< offset: 0x07         */
#define R_SCI_SEMR_RXDESEL_Pos    (7UL)     /*!< RXDESEL (Bit 7)      */
#define R_SCI_SEMR_RXDESEL_Msk    \
		BIT(R_SCI_SEMR_RXDESEL_Pos) /*!< RXDESEL (bmask 0x01) */
#define R_SCI_SEMR_BGDM_Pos       (6UL)     /*!< BGDM (Bit 6)         */
#define R_SCI_SEMR_BGDM_Msk       \
		BIT(R_SCI_SEMR_BGDM_Pos)    /*!< BGDM (bmask 0x01)    */
#define R_SCI_SEMR_NFEN_Pos       (5UL)     /*!< NFEN (Bit 5)         */
#define R_SCI_SEMR_NFEN_Msk       \
		BIT(R_SCI_SEMR_NFEN_Pos)    /*!< NFEN (bmask 0x01)    */
#define R_SCI_SEMR_ABCS_Pos       (4UL)     /*!< ABCS (Bit 4)         */
#define R_SCI_SEMR_ABCS_Msk       \
		BIT(R_SCI_SEMR_ABCS_Pos)    /*!< ABCS (bmask 0x01)    */
#define R_SCI_SEMR_ABCSE_Pos      (3UL)     /*!< ABCSE (Bit 3)        */
#define R_SCI_SEMR_ABCSE_Msk      \
		BIT(R_SCI_SEMR_ABCSE_Pos)   /*!< ABCSE (bmask 0x01)   */
#define R_SCI_SEMR_BRME_Pos       (2UL)     /*!< BRME (Bit 2)         */
#define R_SCI_SEMR_BRME_Msk       \
		BIT(R_SCI_SEMR_BRME_Pos)    /*!< BRME (bmask 0x01)    */
#define R_SCI_SEMR_PADIS_Pos      (1UL)     /*!< PADIS (Bit 1)        */
#define R_SCI_SEMR_PADIS_Msk      \
		BIT(R_SCI_SEMR_PADIS_Pos)   /*!< PADIS (bmask 0x01)   */
#define R_SCI_SEMR_ACS0_Pos       (0UL)     /*!< ACS0 (Bit 0)         */
#define R_SCI_SEMR_ACS0_Msk       \
		BIT(R_SCI_SEMR_ACS0_Pos)    /*!< ACS0 (bmask 0x01)    */

/* =============================  SNFR  ============================= */
#define R_SCI_SNFR                   0x08     /*!< offset: 0x08       */
#define R_SCI_SNFR_NFCS_Pos          (0UL)    /*!< NFCS (Bit 0)       */
#define R_SCI_SNFR_NFCS_Msk    GENMASK(7, 0)  /*!< NFCS (bmask 0x07)  */

/* ============================== SIMR1 ============================= */
#define R_SCI_SIMR1               0x09       /*!< offset: 0x09        */
#define R_SCI_SIMR1_IICDL_Pos     (3UL)      /*!< IICDL (Bit 3)       */
#define R_SCI_SIMR1_IICDL_Msk    GENMASK(7, 3) /*!< IICDL (bmask 0x1f)  */
#define R_SCI_SIMR1_IICDL(x)	\
	(((x)<<R_SCI_SIMR1_IICDL_Pos) & R_SCI_SIMR1_IICDL_Msk)
#define R_SCI_SIMR1_IICM_Pos      (0UL)      /*!< IICM (Bit 0)        */
#define R_SCI_SIMR1_IICM_Msk	\
		BIT(R_SCI_SIMR1_IICM_Pos)    /*!< IICM (bmask 0x01)   */
/* =============================  SIMR2  ============================ */
#define R_SCI_SIMR2               0x0a     /*!< offset: 0x0a          */
#define R_SCI_SIMR2_IICACKT_Pos   (5UL)    /*!< IICACKT (Bit 5)       */
#define R_SCI_SIMR2_IICACKT_Msk	\
		BIT(R_SCI_SIMR2_IICACKT_Pos)  /*!< IICACKT (bmask 0x01)  */
#define R_SCI_SIMR2_IICCSC_Pos    (1UL)    /*!< IICCSC (Bit 1)        */
#define R_SCI_SIMR2_IICCSC_Msk	\
		BIT(R_SCI_SIMR2_IICCSC_Pos)  /*!< IICCSC (bmask 0x01)   */
#define R_SCI_SIMR2_IICINTM_Pos   (0UL)    /*!< IICINTM (Bit 0)       */
#define R_SCI_SIMR2_IICINTM_Msk	\
		BIT(R_SCI_SIMR2_IICINTM_Pos)  /*!< IICINTM (bmask 0x01)  */

/* =============================  SIMR3  ============================ */
#define R_SCI_SIMR3               0x0b     /*!< offset: 0x0b          */
#define R_SCI_SIMR3_IICSCLS_Pos   (6UL)    /*!< IICSCLS (Bit 6)         */
#define R_SCI_SIMR3_IICSCLS_Msk  GENMASK(7, 6) /*!< IICSCLS (bmask 0x03) */
#define R_SCI_SIMR3_IICSCLS(x)	\
	(((x)<<R_SCI_SIMR3_IICSCLS_Pos) & R_SCI_SIMR3_IICSCLS_Msk)
#define R_SCI_SIMR3_IICSDAS_Pos   (4UL)    /*!< IICSDAS (Bit 4)         */
#define R_SCI_SIMR3_IICSDAS_Msk  GENMASK(5, 4) /*!< IICSDAS (bmask 0x03) */
#define R_SCI_SIMR3_IICSDAS(x)	\
	(((x)<<R_SCI_SIMR3_IICSDAS_Pos) & R_SCI_SIMR3_IICSDAS_Msk)
#define R_SCI_SIMR3_IICSTIF_Pos   (3UL)    /*!< IICSTIF (Bit 3)         */
#define R_SCI_SIMR3_IICSTIF_Msk   (0x8UL)  /*!< IICSTIF (bmask 0x01) */
#define R_SCI_SIMR3_IICSTPREQ_Pos  (2UL)   /*!< IICSTPREQ (Bit 2)       */
#define R_SCI_SIMR3_IICSTPREQ_Msk  (0x4UL) /*!< IICSTPREQ (bmask 0x01) */
#define R_SCI_SIMR3_IICRSTAREQ_Pos (1UL)   /*!< IICRSTAREQ (Bit 1)      */
#define R_SCI_SIMR3_IICRSTAREQ_Msk (0x2UL) /*!< IICRSTAREQ (bmask 0x01) */
#define R_SCI_SIMR3_IICSTAREQ_Pos  (0UL)   /*!< IICSTAREQ (Bit 0)       */
#define R_SCI_SIMR3_IICSTAREQ_Msk  (0x1UL) /*!< IICSTAREQ (bmask 0x01) */

#define R_SCI_SIMR3_NORMAL_OUT	(0UL)
#define R_SCI_SIMR3_GEN_COND	(1UL)
#define R_SCI_SIMR3_LOW		(2UL)
#define R_SCI_SIMR3_HIGH_Z	(3UL)

/* ==============================  SISR  ============================ */
#define R_SCI_SISR                0x0c     /*!< offset: 0x0c          */
#define R_SCI_SISR_IICACKR_Pos    (0UL)    /*!< IICACKR (Bit 0)         */
#define R_SCI_SISR_IICACKR_Msk    (0x1UL)  /*!< IICACKR (bmask 0x01) */

/* ===============================  SPMR  =========================== */
#define R_SCI_SPMR                 0x0d     /*!< offset: 0x0d         */
#define R_SCI_SPMR_CKPH_Pos        (7UL)      /*!< CKPH (Bit 7)            */
#define R_SCI_SPMR_CKPH_Msk        (0x80UL)   /*!< CKPH (bmask 0x01)*/
#define R_SCI_SPMR_CKPOL_Pos       (6UL)      /*!< CKPOL (Bit 6)           */
#define R_SCI_SPMR_CKPOL_Msk       (0x40UL)   /*!< CKPOL (bmask 0x01) */
#define R_SCI_SPMR_MFF_Pos         (4UL)      /*!< MFF (Bit 4)             */
#define R_SCI_SPMR_MFF_Msk         (0x10UL)   /*!< MFF (bmask 0x01) */
#define R_SCI_SPMR_CSTPEN_Pos      (3UL)      /*!< CSTPEN (Bit 3)          */
#define R_SCI_SPMR_CSTPEN_Msk      (0x8UL)    /*!< CSTPEN (bmask 0x01)*/
#define R_SCI_SPMR_MSS_Pos         (2UL)      /*!< MSS (Bit 2)             */
#define R_SCI_SPMR_MSS_Msk         (0x4UL)    /*!< MSS (bmask 0x01) */
#define R_SCI_SPMR_CTSE_Pos        (1UL)      /*!< CTSE (Bit 1)            */
#define R_SCI_SPMR_CTSE_Msk        (0x2UL)    /*!< CTSE (bmask 0x01)*/
#define R_SCI_SPMR_SSE_Pos         (0UL)      /*!< SSE (Bit 0)             */
#define R_SCI_SPMR_SSE_Msk         (0x1UL)    /*!< SSE (bmask 0x01) */

/* ================================  FCR  =========================== */
#define R_SCI_FCR                0x14      /*!< offset: 0x14         */
#define R_SCI_FCR_RSTRG_Pos      (12UL)     /*!< RSTRG (Bit 12)          */
#define R_SCI_FCR_RSTRG_Msk      (0xf000UL) /*!< RSTRG (bmask 0x0f) */
#define R_SCI_FCR_RTRG_Pos       (8UL)      /*!< RTRG (Bit 8)            */
#define R_SCI_FCR_RTRG_Msk       (0xf00UL)  /*!< RTRG (bmask 0x0f)*/
#define R_SCI_FCR_TTRG_Pos       (4UL)      /*!< TTRG (Bit 4)            */
#define R_SCI_FCR_TTRG_Msk       (0xf0UL)   /*!< TTRG (bmask 0x0f)*/
#define R_SCI_FCR_DRES_Pos       (3UL)      /*!< DRES (Bit 3)            */
#define R_SCI_FCR_DRES_Msk       (0x8UL)    /*!< DRES (bmask 0x01)*/
#define R_SCI_FCR_TFRST_Pos      (2UL)      /*!< TFRST (Bit 2)           */
#define R_SCI_FCR_TFRST_Msk      (0x4UL)    /*!< TFRST (bmask 0x01) */
#define R_SCI_FCR_RFRST_Pos      (1UL)      /*!< RFRST (Bit 1)           */
#define R_SCI_FCR_RFRST_Msk      (0x2UL)    /*!< RFRST (bmask 0x01) */
#define R_SCI_FCR_FM_Pos         (0UL)      /*!< FM (Bit 0)              */
#define R_SCI_FCR_FM_Msk         (0x1UL)    /*!< FM (bmask 0x01)*/

/* =================================  FDR  ========================== */
#define R_SCI_FDR                 0x16      /*!< offset: 0x14         */
#define R_SCI_FDR_T_Pos              (8UL)      /*!< T (Bit 8)                */
#define R_SCI_FDR_T_Msk              (0x1f00UL) /*!< T (bmask 0x1f)  */
#define R_SCI_FDR_R_Pos              (0UL)      /*!< R (Bit 0)                */
#define R_SCI_FDR_R_Msk              (0x1fUL)   /*!< R (bmask 0x1f)  */

/* =================================  LSR  ========================= */
#define R_SCI_LSR                 0x18      /*!< offset: 0x18        */
#define R_SCI_LSR_PNUM_Pos          (8UL)      /*!< PNUM (Bit 8)     */
#define R_SCI_LSR_PNUM_Msk          (0x1f00UL) /*!< PNUM (bmask 0x1f)*/
#define R_SCI_LSR_FNUM_Pos          (2UL)      /*!< FNUM (Bit 2)     */
#define R_SCI_LSR_FNUM_Msk          (0x7cUL)   /*!< FNUM (bmask 0x1f)*/
#define R_SCI_LSR_ORER_Pos          (0UL)      /*!< ORER (Bit 0)     */
#define R_SCI_LSR_ORER_Msk          (0x1UL)    /*!< ORER (bmask 0x01)*/

/* ==================================  CDR  ========================= */
#define R_SCI_CDR                 0x1a      /*!< offset: 0x1a         */
#define R_SCI_CDR_CMPD_Pos           (0UL)      /*!< CMPD (Bit 0)     */
#define R_SCI_CDR_CMPD_Msk   GENMASK(8, 0)  /*!< CMPD (bmask 0x1ff)   */

/* ==================================  DCCR  ======================== */
#define R_SCI_DCCR                 0x13      /*!< offset: 0x13        */
#define R_SCI_DCCR_DCME_Pos        (7UL)     /*!< DCME (Bit 7)        */
#define R_SCI_DCCR_DCME_Msk        (0x80UL)  /*!< DCME (bmask 0x01)   */
#define R_SCI_DCCR_IDSEL_Pos       (6UL)     /*!< IDSEL (Bit 6)       */
#define R_SCI_DCCR_IDSEL_Msk       (0x40UL)  /*!< IDSEL (bmask 0x01)  */
#define R_SCI_DCCR_DFER_Pos        (4UL)     /*!< DFER (Bit 4)        */
#define R_SCI_DCCR_DFER_Msk        (0x10UL)  /*!< DFER (bmask 0x01)   */
#define R_SCI_DCCR_DPER_Pos        (3UL)     /*!< DPER (Bit 3)        */
#define R_SCI_DCCR_DPER_Msk        (0x8UL)   /*!< DPER (bmask 0x01)   */
#define R_SCI_DCCR_DCMF_Pos        (0UL)     /*!< DCMF (Bit 0)        */
#define R_SCI_DCCR_DCMF_Msk        (0x1UL)   /*!< DCMF (bmask 0x01)   */

/* ===================================  SPTR  ======================= */
#define R_SCI_SPTR                 0x1c      /*!< offset: 0x1c        */
#define R_SCI_SPTR_SPB2IO_Pos     (2UL)      /*!< SPB2IO (Bit 2)      */
#define R_SCI_SPTR_SPB2IO_Msk     (0x4UL)    /*!< SPB2IO (bmask 0x01) */
#define R_SCI_SPTR_SPB2DT_Pos     (1UL)      /*!< SPB2DT (Bit 1)      */
#define R_SCI_SPTR_SPB2DT_Msk     (0x2UL)    /*!< SPB2DT (bmask: 0x01)*/
#define R_SCI_SPTR_RXDMON_Pos     (0UL)      /*!< RXDMON (Bit 0)      */
#define R_SCI_SPTR_RXDMON_Msk     (0x1UL)    /*!< RXDMON (bmask: 0x01)*/
#define R_SCI_SPTR_RINV_Pos       (4UL)      /*!< RINV (Bit 4)        */
#define R_SCI_SPTR_RINV_Msk       (0x10UL)   /*!< RINV (bmask 0x01)   */
#define R_SCI_SPTR_TINV_Pos       (5UL)      /*!< TINV (Bit 5)        */
#define R_SCI_SPTR_TINV_Msk       (0x20UL)   /*!< TINV (bmask 0x01)   */
#define R_SCI_SPTR_ASEN_Pos       (6UL)      /*!< ASEN (Bit 6)        */
#define R_SCI_SPTR_ASEN_Msk       (0x40UL)   /*!< ASEN (bmask 0x01)   */
#define R_SCI_SPTR_ATEN_Pos       (7UL)      /*!< ATEN (Bit 7)        */
#define R_SCI_SPTR_ATEN_Msk       (0x80UL)   /*!< ATEN (bmask 0x01)   */

#endif /* __SCI_RA_PRIV_H__ */
