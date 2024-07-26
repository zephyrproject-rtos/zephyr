/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_PIC32CXSG_GMAC_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_PIC32CXSG_GMAC_H_

/*
 * Map the PIC32CXSG-family DFP GMAC register names to the Microchip PIC32CXSG-family DFP GMAC
 * register names.
 */
#define GMAC_NCR        NCR.reg
#define GMAC_NCFGR      NCFGR.reg
#define GMAC_NSR        NSR.reg
#define GMAC_UR         UR.reg
#define GMAC_DCFGR      DCFGR.reg
#define GMAC_TSR        TSR.reg
#define GMAC_RBQB       RBQB.reg
#define GMAC_TBQB       TBQB.reg
#define GMAC_RSR        RSR.reg
#define GMAC_ISR        ISR.reg
#define GMAC_IER        IER.reg
#define GMAC_IDR        IDR.reg
#define GMAC_IMR        IMR.reg
#define GMAC_MAN        MAN.reg
#define GMAC_RPQ        RPQ.reg
#define GMAC_TPQ        TPQ.reg
#define GMAC_TPSF       TPSF.reg
#define GMAC_RPSF       RPSF.reg
#define GMAC_RJFML      RJFML.reg
#define GMAC_HRB        HRB.reg
#define GMAC_HRT        HRT.reg
#define GMAC_SA         Sa
#define GMAC_WOL        WOL.reg
#define GMAC_IPGS       IPGS.reg
#define GMAC_SVLAN      SVLAN.reg
#define GMAC_TPFCP      TPFCP.reg
#define GMAC_SAMB1      SAMB1.reg
#define GMAC_SAMT1      SAMT1.reg
#define GMAC_NSC        NSC.reg
#define GMAC_SCL        SCL.reg
#define GMAC_SCH        SCH.reg
#define GMAC_EFTSH      EFTSH.reg
#define GMAC_EFRSH      EFRSH.reg
#define GMAC_PEFTSH     PEFTSH.reg
#define GMAC_PEFRSH     PEFRSH.reg
#define GMAC_OTLO       OTLO.reg
#define GMAC_OTHI       OTHI.reg
#define GMAC_FT         FT.reg
#define GMAC_BCFT       BCFT.reg
#define GMAC_MFT        MFT.reg
#define GMAC_PFT        PFT.reg
#define GMAC_BFT64      BFT64.reg
#define GMAC_TBFT127    TBFT127.reg
#define GMAC_TBFT255    TBFT255.reg
#define GMAC_TBFT511    TBFT511.reg
#define GMAC_TBFT1023   TBFT1023.reg
#define GMAC_TBFT1518   TBFT1518.reg
#define GMAC_GTBFT1518  GTBFT1518.reg
#define GMAC_TUR        TUR.reg
#define GMAC_SCF        SCF.reg
#define GMAC_MCF        MCF.reg
#define GMAC_EC         EC.reg
#define GMAC_LC         LC.reg
#define GMAC_DTF        DTF.reg
#define GMAC_CSE        CSE.reg
#define GMAC_ORLO       ORLO.reg
#define GMAC_ORHI       ORHI.reg
#define GMAC_FR         FR.reg
#define GMAC_BCFR       BCFR.reg
#define GMAC_MFR        MFR.reg
#define GMAC_PFR        PFR.reg
#define GMAC_BFR64      BFR64.reg
#define GMAC_TBFR127    TBFR127.reg
#define GMAC_TBFR255    TBFR255.reg
#define GMAC_TBFR511    TBFR511.reg
#define GMAC_TBFR1023   TBFR1023.reg
#define GMAC_TBFR1518   TBFR1518.reg
#define GMAC_TMXBFR     TMXBFR.reg
#define GMAC_UFR        UFR.reg
#define GMAC_OFR        OFR.reg
#define GMAC_JR         JR.reg
#define GMAC_FCSE       FCSE.reg
#define GMAC_LFFE       LFFE.reg
#define GMAC_RSE        RSE.reg
#define GMAC_AE         AE.reg
#define GMAC_RRE        RRE.reg
#define GMAC_ROE        ROE.reg
#define GMAC_IHCE       IHCE.reg
#define GMAC_TCE        TCE.reg
#define GMAC_UCE        UCE.reg
#define GMAC_TISUBN     TISUBN.reg
#define GMAC_TSH        TSH.reg
#define GMAC_TSSSL      TSSSL.reg
#define GMAC_TSSN       TSSN.reg
#define GMAC_TSL        TSL.reg
#define GMAC_TN         TN.reg
#define GMAC_TA         TA.reg
#define GMAC_TI         TI.reg
#define GMAC_EFTSL      EFTSL.reg
#define GMAC_EFTN       EFTN.reg
#define GMAC_EFRSL      EFRSL.reg
#define GMAC_EFRN       EFRN.reg
#define GMAC_PEFTSL     PEFTSL.reg
#define GMAC_PEFTN      PEFTN.reg
#define GMAC_PEFRSL     PEFRSL.reg
#define GMAC_PEFRN      PEFRN.reg
#define GMAC_RLPITR     RLPITR.reg
#define GMAC_RLPITI     RLPITI.reg
#define GMAC_TLPITR     TLPITR.reg
#define GMAC_TLPITI     TLPITI.reg

#define GMAC_SAB        SAB.reg
#define GMAC_SAT        SAT.reg

/*
 * Define the register field value symbols that are missing in the SAM0-family
 * DFP GMAC headers.
 */
#define GMAC_NCFGR_CLK_MCK_8    GMAC_NCFGR_CLK(0)
#define GMAC_NCFGR_CLK_MCK_16   GMAC_NCFGR_CLK(1)
#define GMAC_NCFGR_CLK_MCK_32   GMAC_NCFGR_CLK(2)
#define GMAC_NCFGR_CLK_MCK_48   GMAC_NCFGR_CLK(3)
#define GMAC_NCFGR_CLK_MCK_64   GMAC_NCFGR_CLK(4)
#define GMAC_NCFGR_CLK_MCK_96   GMAC_NCFGR_CLK(5)

#define GMAC_DCFGR_FBLDO_SINGLE GMAC_DCFGR_FBLDO(1)
#define GMAC_DCFGR_FBLDO_INCR4  GMAC_DCFGR_FBLDO(2)
#define GMAC_DCFGR_FBLDO_INCR8  GMAC_DCFGR_FBLDO(3)
#define GMAC_DCFGR_FBLDO_INCR16 GMAC_DCFGR_FBLDO(4)

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_PIC32CXSG_GMAC_H_ */
