/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_
#define ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_

#include <zephyr/sys/util.h>

/*
 * The Bosch M_CAN register definitions correspond to those found in the Bosch M_CAN Controller Area
 * Network User's Manual, Revision 3.3.0.
 *
 * The STMicroelectronics STM32 FDCAN definitions correspond to those found in the
 * STMicroelectronics STM32G4 Series Reference manual (RM0440), Rev 7.
 */

/* Core Release register */
#define CAN_MCAN_CREL	      0x000
#define CAN_MCAN_CREL_REL     GENMASK(31, 28)
#define CAN_MCAN_CREL_STEP    GENMASK(27, 24)
#define CAN_MCAN_CREL_SUBSTEP GENMASK(23, 20)
#define CAN_MCAN_CREL_YEAR    GENMASK(19, 16)
#define CAN_MCAN_CREL_MON     GENMASK(15, 8)
#define CAN_MCAN_CREL_DAY     GENMASK(7, 0)

/* Endian register */
#define CAN_MCAN_ENDN	  0x004
#define CAN_MCAN_ENDN_ETV GENMASK(31, 0)

/* Customer register */
#define CAN_MCAN_CUST	   0x008
#define CAN_MCAN_CUST_CUST GENMASK(31, 0)

/* Data Bit Timing & Prescaler register */
#define CAN_MCAN_DBTP	     0x00C
#define CAN_MCAN_DBTP_TDC    BIT(23)
#define CAN_MCAN_DBTP_DBRP   GENMASK(20, 16)
#define CAN_MCAN_DBTP_DTSEG1 GENMASK(12, 8)
#define CAN_MCAN_DBTP_DTSEG2 GENMASK(7, 4)
#define CAN_MCAN_DBTP_DSJW   GENMASK(3, 0)

/* Test register */
#define CAN_MCAN_TEST 0x010
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_TEST_SVAL  BIT(21)
#define CAN_MCAN_TEST_TXBNS GENMASK(20, 16)
#define CAN_MCAN_TEST_PVAL  BIT(13)
#define CAN_MCAN_TEST_TXBNP GENMASK(12, 8)
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_TEST_RX   BIT(7)
#define CAN_MCAN_TEST_TX   GENMASK(6, 5)
#define CAN_MCAN_TEST_LBCK BIT(4)

/* RAM Watchdog register */
#define CAN_MCAN_RWD	 0x014
#define CAN_MCAN_RWD_WDV GENMASK(15, 8)
#define CAN_MCAN_RWD_WDC GENMASK(7, 0)

/* CC Control register */
#define CAN_MCAN_CCCR	   0x018
#define CAN_MCAN_CCCR_NISO BIT(15)
#define CAN_MCAN_CCCR_TXP  BIT(14)
#define CAN_MCAN_CCCR_EFBI BIT(13)
#define CAN_MCAN_CCCR_PXHD BIT(12)
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_CCCR_WMM  BIT(11)
#define CAN_MCAN_CCCR_UTSU BIT(10)
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_CCCR_BRSE BIT(9)
#define CAN_MCAN_CCCR_FDOE BIT(8)
#define CAN_MCAN_CCCR_TEST BIT(7)
#define CAN_MCAN_CCCR_DAR  BIT(6)
#define CAN_MCAN_CCCR_MON  BIT(5)
#define CAN_MCAN_CCCR_CSR  BIT(4)
#define CAN_MCAN_CCCR_CSA  BIT(3)
#define CAN_MCAN_CCCR_ASM  BIT(2)
#define CAN_MCAN_CCCR_CCE  BIT(1)
#define CAN_MCAN_CCCR_INIT BIT(0)

/* Nominal Bit Timing & Prescaler register */
#define CAN_MCAN_NBTP	     0x01C
#define CAN_MCAN_NBTP_NSJW   GENMASK(31, 25)
#define CAN_MCAN_NBTP_NBRP   GENMASK(24, 16)
#define CAN_MCAN_NBTP_NTSEG1 GENMASK(15, 8)
#define CAN_MCAN_NBTP_NTSEG2 GENMASK(6, 0)

/* Timestamp Counter Configuration register */
#define CAN_MCAN_TSCC	  0x020
#define CAN_MCAN_TSCC_TCP GENMASK(19, 16)
#define CAN_MCAN_TSCC_TSS GENMASK(1, 0)

/* Timestamp Counter Value register */
#define CAN_MCAN_TSCV	  0x024
#define CAN_MCAN_TSCV_TSC GENMASK(15, 0)

/* Timeout Counter Configuration register */
#define CAN_MCAN_TOCC	   0x028
#define CAN_MCAN_TOCC_TOP  GENMASK(31, 16)
#define CAN_MCAN_TOCC_TOS  GENMASK(2, 1)
#define CAN_MCAN_TOCC_ETOC BIT(1)

/* Timeout Counter Value register */
#define CAN_MCAN_TOCV	  0x02C
#define CAN_MCAN_TOCV_TOC GENMASK(15, 0)

/* Error Counter register */
#define CAN_MCAN_ECR	 0x040
#define CAN_MCAN_ECR_CEL GENMASK(23, 16)
#define CAN_MCAN_ECR_RP	 BIT(15)
#define CAN_MCAN_ECR_REC GENMASK(14, 8)
#define CAN_MCAN_ECR_TEC GENMASK(7, 0)

/* Protocol Status register */
#define CAN_MCAN_PSR	  0x044
#define CAN_MCAN_PSR_TDCV GENMASK(22, 16)
#define CAN_MCAN_PSR_PXE  BIT(14)
#define CAN_MCAN_PSR_RFDF BIT(13)
#define CAN_MCAN_PSR_RBRS BIT(12)
#define CAN_MCAN_PSR_RESI BIT(11)
#define CAN_MCAN_PSR_DLEC GENMASK(10, 8)
#define CAN_MCAN_PSR_BO	  BIT(7)
#define CAN_MCAN_PSR_EW	  BIT(6)
#define CAN_MCAN_PSR_EP	  BIT(5)
#define CAN_MCAN_PSR_ACT  GENMASK(4, 3)
#define CAN_MCAN_PSR_LEC  GENMASK(2, 0)

/* Transmitter Delay Compensation register */
#define CAN_MCAN_TDCR	   0x048
#define CAN_MCAN_TDCR_TDCO GENMASK(14, 8)
#define CAN_MCAN_TDCR_TDCF GENMASK(6, 0)

/* Interrupt register */
#define CAN_MCAN_IR 0x050
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_IR_ARA	 BIT(23)
#define CAN_MCAN_IR_PED	 BIT(22)
#define CAN_MCAN_IR_PEA	 BIT(21)
#define CAN_MCAN_IR_WDI	 BIT(20)
#define CAN_MCAN_IR_BO	 BIT(19)
#define CAN_MCAN_IR_EW	 BIT(18)
#define CAN_MCAN_IR_EP	 BIT(17)
#define CAN_MCAN_IR_ELO	 BIT(16)
#define CAN_MCAN_IR_TOO	 BIT(15)
#define CAN_MCAN_IR_MRAF BIT(14)
#define CAN_MCAN_IR_TSW	 BIT(13)
#define CAN_MCAN_IR_TEFL BIT(12)
#define CAN_MCAN_IR_TEFF BIT(11)
#define CAN_MCAN_IR_TEFN BIT(10)
#define CAN_MCAN_IR_TFE	 BIT(9)
#define CAN_MCAN_IR_TCF	 BIT(8)
#define CAN_MCAN_IR_TC	 BIT(7)
#define CAN_MCAN_IR_HPM	 BIT(6)
#define CAN_MCAN_IR_RF1L BIT(5)
#define CAN_MCAN_IR_RF1F BIT(4)
#define CAN_MCAN_IR_RF1N BIT(3)
#define CAN_MCAN_IR_RF0L BIT(2)
#define CAN_MCAN_IR_RF0F BIT(1)
#define CAN_MCAN_IR_RF0N BIT(0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_IR_ARA	 BIT(29)
#define CAN_MCAN_IR_PED	 BIT(28)
#define CAN_MCAN_IR_PEA	 BIT(27)
#define CAN_MCAN_IR_WDI	 BIT(26)
#define CAN_MCAN_IR_BO	 BIT(25)
#define CAN_MCAN_IR_EW	 BIT(24)
#define CAN_MCAN_IR_EP	 BIT(23)
#define CAN_MCAN_IR_ELO	 BIT(22)
#define CAN_MCAN_IR_BEU	 BIT(21)
#define CAN_MCAN_IR_BEC	 BIT(20)
#define CAN_MCAN_IR_DRX	 BIT(19)
#define CAN_MCAN_IR_TOO	 BIT(18)
#define CAN_MCAN_IR_MRAF BIT(17)
#define CAN_MCAN_IR_TSW	 BIT(16)
#define CAN_MCAN_IR_TEFL BIT(15)
#define CAN_MCAN_IR_TEFF BIT(14)
#define CAN_MCAN_IR_TEFW BIT(13)
#define CAN_MCAN_IR_TEFN BIT(12)
#define CAN_MCAN_IR_TFE	 BIT(11)
#define CAN_MCAN_IR_TCF	 BIT(10)
#define CAN_MCAN_IR_TC	 BIT(9)
#define CAN_MCAN_IR_HPM	 BIT(8)
#define CAN_MCAN_IR_RF1L BIT(7)
#define CAN_MCAN_IR_RF1F BIT(6)
#define CAN_MCAN_IR_RF1W BIT(5)
#define CAN_MCAN_IR_RF1N BIT(4)
#define CAN_MCAN_IR_RF0L BIT(3)
#define CAN_MCAN_IR_RF0F BIT(2)
#define CAN_MCAN_IR_RF0W BIT(1)
#define CAN_MCAN_IR_RF0N BIT(0)
#endif /* !CONFIG_CAN_STM32FD */

/* Interrupt Enable register */
#define CAN_MCAN_IE 0x054
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_IE_ARAE  BIT(23)
#define CAN_MCAN_IE_PEDE  BIT(22)
#define CAN_MCAN_IE_PEAE  BIT(21)
#define CAN_MCAN_IE_WDIE  BIT(20)
#define CAN_MCAN_IE_BOE	  BIT(19)
#define CAN_MCAN_IE_EWE	  BIT(18)
#define CAN_MCAN_IE_EPE	  BIT(17)
#define CAN_MCAN_IE_ELOE  BIT(16)
#define CAN_MCAN_IE_TOOE  BIT(15)
#define CAN_MCAN_IE_MRAFE BIT(14)
#define CAN_MCAN_IE_TSWE  BIT(13)
#define CAN_MCAN_IE_TEFLE BIT(12)
#define CAN_MCAN_IE_TEFFE BIT(11)
#define CAN_MCAN_IE_TEFNE BIT(10)
#define CAN_MCAN_IE_TFEE  BIT(9)
#define CAN_MCAN_IE_TCFE  BIT(8)
#define CAN_MCAN_IE_TCE	  BIT(7)
#define CAN_MCAN_IE_HPME  BIT(6)
#define CAN_MCAN_IE_RF1LE BIT(5)
#define CAN_MCAN_IE_RF1FE BIT(4)
#define CAN_MCAN_IE_RF1NE BIT(3)
#define CAN_MCAN_IE_RF0LE BIT(2)
#define CAN_MCAN_IE_RF0FE BIT(1)
#define CAN_MCAN_IE_RF0NE BIT(0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_IE_ARAE  BIT(29)
#define CAN_MCAN_IE_PEDE  BIT(28)
#define CAN_MCAN_IE_PEAE  BIT(27)
#define CAN_MCAN_IE_WDIE  BIT(26)
#define CAN_MCAN_IE_BOE	  BIT(25)
#define CAN_MCAN_IE_EWE	  BIT(24)
#define CAN_MCAN_IE_EPE	  BIT(23)
#define CAN_MCAN_IE_ELOE  BIT(22)
#define CAN_MCAN_IE_BEUE  BIT(21)
#define CAN_MCAN_IE_BECE  BIT(20)
#define CAN_MCAN_IE_DRXE  BIT(19)
#define CAN_MCAN_IE_TOOE  BIT(18)
#define CAN_MCAN_IE_MRAFE BIT(17)
#define CAN_MCAN_IE_TSWE  BIT(16)
#define CAN_MCAN_IE_TEFLE BIT(15)
#define CAN_MCAN_IE_TEFFE BIT(14)
#define CAN_MCAN_IE_TEFWE BIT(13)
#define CAN_MCAN_IE_TEFNE BIT(12)
#define CAN_MCAN_IE_TFEE  BIT(11)
#define CAN_MCAN_IE_TCFE  BIT(10)
#define CAN_MCAN_IE_TCE	  BIT(9)
#define CAN_MCAN_IE_HPME  BIT(8)
#define CAN_MCAN_IE_RF1LE BIT(7)
#define CAN_MCAN_IE_RF1FE BIT(6)
#define CAN_MCAN_IE_RF1WE BIT(5)
#define CAN_MCAN_IE_RF1NE BIT(4)
#define CAN_MCAN_IE_RF0LE BIT(3)
#define CAN_MCAN_IE_RF0FE BIT(2)
#define CAN_MCAN_IE_RF0WE BIT(1)
#define CAN_MCAN_IE_RF0NE BIT(0)
#endif /* !CONFIG_CAN_STM32FD */

/* Interrupt Line Select register */
#define CAN_MCAN_ILS 0x058
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_ILS_PERR    BIT(6)
#define CAN_MCAN_ILS_BERR    BIT(5)
#define CAN_MCAN_ILS_MISC    BIT(4)
#define CAN_MCAN_ILS_TFERR   BIT(3)
#define CAN_MCAN_ILS_SMSG    BIT(2)
#define CAN_MCAN_ILS_RXFIFO1 BIT(1)
#define CAN_MCAN_ILS_RXFIFO0 BIT(0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_ILS_ARAL  BIT(29)
#define CAN_MCAN_ILS_PEDL  BIT(28)
#define CAN_MCAN_ILS_PEAL  BIT(27)
#define CAN_MCAN_ILS_WDIL  BIT(26)
#define CAN_MCAN_ILS_BOL   BIT(25)
#define CAN_MCAN_ILS_EWL   BIT(24)
#define CAN_MCAN_ILS_EPL   BIT(23)
#define CAN_MCAN_ILS_ELOL  BIT(22)
#define CAN_MCAN_ILS_BEUL  BIT(21)
#define CAN_MCAN_ILS_BECL  BIT(20)
#define CAN_MCAN_ILS_DRXL  BIT(19)
#define CAN_MCAN_ILS_TOOL  BIT(18)
#define CAN_MCAN_ILS_MRAFL BIT(17)
#define CAN_MCAN_ILS_TSWL  BIT(16)
#define CAN_MCAN_ILS_TEFLL BIT(15)
#define CAN_MCAN_ILS_TEFFL BIT(14)
#define CAN_MCAN_ILS_TEFWL BIT(13)
#define CAN_MCAN_ILS_TEFNL BIT(12)
#define CAN_MCAN_ILS_TFEL  BIT(11)
#define CAN_MCAN_ILS_TCFL  BIT(10)
#define CAN_MCAN_ILS_TCL   BIT(9)
#define CAN_MCAN_ILS_HPML  BIT(8)
#define CAN_MCAN_ILS_RF1LL BIT(7)
#define CAN_MCAN_ILS_RF1FL BIT(6)
#define CAN_MCAN_ILS_RF1WL BIT(5)
#define CAN_MCAN_ILS_RF1NL BIT(4)
#define CAN_MCAN_ILS_RF0LL BIT(3)
#define CAN_MCAN_ILS_RF0FL BIT(2)
#define CAN_MCAN_ILS_RF0WL BIT(1)
#define CAN_MCAN_ILS_RF0NL BIT(0)
#endif /* !CONFIG_CAN_STM32FD */

/* Interrupt Line Enable register */
#define CAN_MCAN_ILE	   0x05C
#define CAN_MCAN_ILE_EINT1 BIT(1)
#define CAN_MCAN_ILE_EINT0 BIT(0)

/* Global filter configuration register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXGFC	    0x080
#define CAN_MCAN_RXGFC_LSE  GENMASK(27, 24)
#define CAN_MCAN_RXGFC_LSS  GENMASK(20, 16)
#define CAN_MCAN_RXGFC_F0OM BIT(9)
#define CAN_MCAN_RXGFC_F1OM BIT(8)
#define CAN_MCAN_RXGFC_ANFS GENMASK(5, 4)
#define CAN_MCAN_RXGFC_ANFE GENMASK(3, 2)
#define CAN_MCAN_RXGFC_RRFS BIT(1)
#define CAN_MCAN_RXGFC_RRFE BIT(0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_GFC	  0x080
#define CAN_MCAN_GFC_ANFS GENMASK(5, 4)
#define CAN_MCAN_GFC_ANFE GENMASK(3, 2)
#define CAN_MCAN_GFC_RRFS BIT(1)
#define CAN_MCAN_GFC_RRFE BIT(0)
#endif /* !CONFIG_CAN_STM32FD */

/* Standard ID Filter Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_SIDFC	     0x084
#define CAN_MCAN_SIDFC_LSS   GENMASK(23, 16)
#define CAN_MCAN_SIDFC_FLSSA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Extended ID Filter Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_XIDFC	     0x088
#define CAN_MCAN_XIDFC_LSS   GENMASK(22, 16)
#define CAN_MCAN_XIDFC_FLESA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Extended ID AND Mask register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_XIDAM 0x084
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_XIDAM 0x090
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_XIDAM_EIDM GENMASK(28, 0)

/* HPMS register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_HPMS 0x088
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_HPMS 0x094
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_HPMS_FLST BIT(15)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_HPMS_FIDX GENMASK(12, 8)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_HPMS_FIDX GENMASK(14, 8)
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_HPMS_MSI GENMASK(7, 6)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_HPMS_BIDX GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_HPMS_BIDX GENMASK(5, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* New Data 1 register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_NDAT1	  0x098
#define CAN_MCAN_NDAT1_ND GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* New Data 2 register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_NDAT2	  0x09C
#define CAN_MCAN_NDAT2_ND GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 0 Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF0C	    0x0A0
#define CAN_MCAN_RXF0C_F0OM BIT(31)
#define CAN_MCAN_RXF0C_F0WM GENMASK(30, 24)
#define CAN_MCAN_RXF0C_F0S  GENMASK(22, 16)
#define CAN_MCAN_RXF0C_F0SA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 0 Status register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF0S 0x090
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF0S 0x0A4
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF0S_RF0L BIT(25)
#define CAN_MCAN_RXF0S_F0F  BIT(24)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF0S_F0PI GENMASK(17, 16)
#define CAN_MCAN_RXF0S_F0GI GENMASK(9, 8)
#define CAN_MCAN_RXF0S_F0FL GENMASK(3, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF0S_F0PI GENMASK(21, 16)
#define CAN_MCAN_RXF0S_F0GI GENMASK(13, 8)
#define CAN_MCAN_RXF0S_F0FL GENMASK(6, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 0 Acknowledge register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF0A	    0x094
#define CAN_MCAN_RXF0A_F0AI GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF0A	    0x0A8
#define CAN_MCAN_RXF0A_F0AI GENMASK(5, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx Buffer Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXBC	   0x0AC
#define CAN_MCAN_RXBC_RBSA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 1 Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF1C	    0x0B0
#define CAN_MCAN_RXF1C_F1OM BIT(31)
#define CAN_MCAN_RXF1C_F1WM GENMASK(30, 24)
#define CAN_MCAN_RXF1C_F1S  GENMASK(22, 16)
#define CAN_MCAN_RXF1C_F1SA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 1 Status register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF1S 0x098
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF1S 0x0B4
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF1S_RF1L BIT(25)
#define CAN_MCAN_RXF1S_F1F  BIT(24)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF1S_F1PI GENMASK(17, 16)
#define CAN_MCAN_RXF1S_F1GI GENMASK(9, 8)
#define CAN_MCAN_RXF1S_F1FL GENMASK(3, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF1S_F1PI GENMASK(21, 16)
#define CAN_MCAN_RXF1S_F1GI GENMASK(13, 8)
#define CAN_MCAN_RXF1S_F1FL GENMASK(6, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx FIFO 1 Acknowledge register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF1A	    0x09C
#define CAN_MCAN_RXF1A_F1AI GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_RXF1A	    0x0B8
#define CAN_MCAN_RXF1A_F1AI GENMASK(5, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Rx Buffer/FIFO Element Size Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXESC	    0x0BC
#define CAN_MCAN_RXESC_RBDS GENMASK(10, 8)
#define CAN_MCAN_RXESC_F1DS GENMASK(6, 4)
#define CAN_MCAN_RXESC_F0DS GENMASK(2, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Configuration register */
#define CAN_MCAN_TXBC 0x0C0
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBC_TFQM BIT(24)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBC_TFQM BIT(30)
#define CAN_MCAN_TXBC_TFQS GENMASK(29, 24)
#define CAN_MCAN_TXBC_NDTB GENMASK(21, 16)
#define CAN_MCAN_TXBC_TBSA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx FIFO/Queue Status register */
#define CAN_MCAN_TXFQS	    0x0C4
#define CAN_MCAN_TXFQS_TFQF BIT(21)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXFQS_TFQPI GENMASK(17, 16)
#define CAN_MCAN_TXFQS_TFGI  GENMASK(9, 8)
#define CAN_MCAN_TXFQS_TFFL  GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXFQS_TFQPI GENMASK(20, 16)
#define CAN_MCAN_TXFQS_TFGI  GENMASK(12, 8)
#define CAN_MCAN_TXFQS_TFFL  GENMASK(5, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Element Size Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXESC	    0x0C8
#define CAN_MCAN_TXESC_TBDS GENMASK(2, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Request Pending register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBRP	   0x0C8
#define CAN_MCAN_TXBRP_TRP GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBRP	   0x0CC
#define CAN_MCAN_TXBRP_TRP GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Add Request register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBAR	  0x0CC
#define CAN_MCAN_TXBAR_AR GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBAR	  0x0D0
#define CAN_MCAN_TXBAR_AR GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Cancellation Request register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBCR	  0x0D0
#define CAN_MCAN_TXBCR_CR GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBCR	  0x0D4
#define CAN_MCAN_TXBCR_CR GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Transmission Occurred register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBTO	  0x0D4
#define CAN_MCAN_TXBTO_TO GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBTO	  0x0D8
#define CAN_MCAN_TXBTO_TO GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Cancellation Finished register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBCF	  0x0D8
#define CAN_MCAN_TXBCF_CF GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD*/
#define CAN_MCAN_TXBCF	  0x0DC
#define CAN_MCAN_TXBCF_CF GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Transmission Interrupt Enable register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBTIE	    0x0DC
#define CAN_MCAN_TXBTIE_TIE GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBTIE	    0x0E0
#define CAN_MCAN_TXBTIE_TIE GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Buffer Cancellation Finished Interrupt Enable register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXBCIE	     0x0E0
#define CAN_MCAN_TXBCIE_CFIE GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXBCIE	     0x0E4
#define CAN_MCAN_TXBCIE_CFIE GENMASK(31, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Event FIFO Configuration register */
#ifndef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXEFC	    0x0F0
#define CAN_MCAN_TXEFC_EFWM GENMASK(29, 24)
#define CAN_MCAN_TXEFC_EFS  GENMASK(21, 16)
#define CAN_MCAN_TXEFC_EFSA GENMASK(15, 2)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Event FIFO Status register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXEFS 0x0E4
#else /* CONFIG_CAN_STM32FD*/
#define CAN_MCAN_TXEFS 0x0F4
#endif /* !CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXEFS_TEFL BIT(25)
#define CAN_MCAN_TXEFS_EFF  BIT(24)
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXEFS_EFPI GENMASK(17, 16)
#define CAN_MCAN_TXEFS_EFGI GENMASK(9, 8)
#define CAN_MCAN_TXEFS_EFFL GENMASK(2, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXEFS_EFPI GENMASK(20, 16)
#define CAN_MCAN_TXEFS_EFGI GENMASK(12, 8)
#define CAN_MCAN_TXEFS_EFFL GENMASK(5, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Tx Event FIFO Acknowledge register */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_TXEFA	    0x0E8
#define CAN_MCAN_TXEFA_EFAI GENMASK(1, 0)
#else /* CONFIG_CAN_STM32FD */
#define CAN_MCAN_TXEFA	    0x0F8
#define CAN_MCAN_TXEFA_EFAI GENMASK(4, 0)
#endif /* !CONFIG_CAN_STM32FD */

/* Message RAM Base Address register */
#ifdef CONFIG_CAN_MCUX_MCAN
#define CAN_MCAN_MRBA	 0x200
#define CAN_MCAN_MRBA_BA GENMASK(31, 16)
#endif /* CONFIG_CAN_MCUX_MCAN */

#endif /* ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_*/
