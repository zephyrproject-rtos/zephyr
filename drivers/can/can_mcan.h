/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_MCAN_H_
#define ZEPHYR_DRIVERS_CAN_MCAN_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
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

#ifdef CONFIG_CAN_MCUX_MCAN
#define MCAN_DT_PATH DT_NODELABEL(can0)
#else
#define MCAN_DT_PATH DT_PATH(soc, can)
#endif

#define NUM_STD_FILTER_ELEMENTS DT_PROP(MCAN_DT_PATH, std_filter_elements)
#define NUM_EXT_FILTER_ELEMENTS DT_PROP(MCAN_DT_PATH, ext_filter_elements)
#define NUM_RX_FIFO0_ELEMENTS	DT_PROP(MCAN_DT_PATH, rx_fifo0_elements)
#define NUM_RX_FIFO1_ELEMENTS	DT_PROP(MCAN_DT_PATH, rx_fifo0_elements)
#define NUM_RX_BUF_ELEMENTS	DT_PROP(MCAN_DT_PATH, rx_buffer_elements)
#define NUM_TX_BUF_ELEMENTS	DT_PROP(MCAN_DT_PATH, tx_buffer_elements)

#ifdef CONFIG_CAN_STM32FD
#define NUM_STD_FILTER_DATA CONFIG_CAN_MAX_STD_ID_FILTER
#define NUM_EXT_FILTER_DATA CONFIG_CAN_MAX_EXT_ID_FILTER
#else
#define NUM_STD_FILTER_DATA NUM_STD_FILTER_ELEMENTS
#define NUM_EXT_FILTER_DATA NUM_EXT_FILTER_ELEMENTS
#endif

struct can_mcan_rx_fifo_hdr {
	union {
		struct {
			volatile uint32_t ext_id: 29; /* Extended Identifier */
			volatile uint32_t rtr: 1;     /* Remote Transmission Request*/
			volatile uint32_t xtd: 1;     /* Extended identifier */
			volatile uint32_t esi: 1;     /* Error state indicator */
		};
		struct {
			volatile uint32_t pad1: 18;
			volatile uint32_t std_id: 11; /* Standard Identifier */
			volatile uint32_t pad2: 3;
		};
	};

	volatile uint32_t rxts: 16; /* Rx timestamp */
	volatile uint32_t dlc: 4;   /* Data Length Code */
	volatile uint32_t brs: 1;   /* Bit Rate Switch */
	volatile uint32_t fdf: 1;   /* FD Format */
	volatile uint32_t res: 2;   /* Reserved */
	volatile uint32_t fidx: 7;  /* Filter Index */
	volatile uint32_t anmf: 1;  /* Accepted non-matching frame */
} __packed __aligned(4);

struct can_mcan_rx_fifo {
	struct can_mcan_rx_fifo_hdr hdr;
	union {
		volatile uint8_t data[64];
		volatile uint32_t data_32[16];
	};
} __packed __aligned(4);

struct can_mcan_mm {
	volatile uint8_t idx: 5;
	volatile uint8_t cnt: 3;
} __packed;

struct can_mcan_tx_buffer_hdr {
	union {
		struct {
			volatile uint32_t ext_id: 29; /* Identifier */
			volatile uint32_t rtr: 1;     /* Remote Transmission Request*/
			volatile uint32_t xtd: 1;     /* Extended identifier */
			volatile uint32_t esi: 1;     /* Error state indicator */
		};
		struct {
			volatile uint32_t pad1: 18;
			volatile uint32_t std_id: 11; /* Identifier */
			volatile uint32_t pad2: 3;
		};
	};
	volatile uint16_t res1;	  /* Reserved */
	volatile uint8_t dlc: 4;  /* Data Length Code */
	volatile uint8_t brs: 1;  /* Bit Rate Switch */
	volatile uint8_t fdf: 1;  /* FD Format */
	volatile uint8_t res2: 1; /* Reserved */
	volatile uint8_t efc: 1;  /* Event FIFO control (Store Tx events) */
	struct can_mcan_mm mm;	  /* Message marker */
} __packed __aligned(4);

struct can_mcan_tx_buffer {
	struct can_mcan_tx_buffer_hdr hdr;
	union {
		volatile uint8_t data[64];
		volatile uint32_t data_32[16];
	};
} __packed __aligned(4);

#define CAN_MCAN_TE_TX	0x1 /* TX event */
#define CAN_MCAN_TE_TXC 0x2 /* TX event in spite of cancellation */

struct can_mcan_tx_event_fifo {
	volatile uint32_t id: 29; /* Identifier */
	volatile uint32_t rtr: 1; /* Remote Transmission Request*/
	volatile uint32_t xtd: 1; /* Extended identifier */
	volatile uint32_t esi: 1; /* Error state indicator */

	volatile uint16_t txts;	 /* TX Timestamp */
	volatile uint8_t dlc: 4; /* Data Length Code */
	volatile uint8_t brs: 1; /* Bit Rate Switch */
	volatile uint8_t fdf: 1; /* FD Format */
	volatile uint8_t et: 2;	 /* Event type */
	struct can_mcan_mm mm;	 /* Message marker */
} __packed __aligned(4);

#define CAN_MCAN_FCE_DISABLE	0x0
#define CAN_MCAN_FCE_FIFO0	0x1
#define CAN_MCAN_FCE_FIFO1	0x2
#define CAN_MCAN_FCE_REJECT	0x3
#define CAN_MCAN_FCE_PRIO	0x4
#define CAN_MCAN_FCE_PRIO_FIFO0 0x5
#define CAN_MCAN_FCE_PRIO_FIFO1 0x7

#define CAN_MCAN_SFT_RANGE    0x0
#define CAN_MCAN_SFT_DUAL     0x1
#define CAN_MCAN_SFT_MASKED   0x2
#define CAN_MCAN_SFT_DISABLED 0x3

struct can_mcan_std_filter {
	volatile uint32_t id2: 11; /* ID2 for dual or range, mask otherwise */
	volatile uint32_t res: 5;
	volatile uint32_t id1: 11;
	volatile uint32_t sfce: 3; /* Filter config */
	volatile uint32_t sft: 2;  /* Filter type */
} __packed __aligned(4);

#define CAN_MCAN_EFT_RANGE_XIDAM 0x0
#define CAN_MCAN_EFT_DUAL	 0x1
#define CAN_MCAN_EFT_MASKED	 0x2
#define CAN_MCAN_EFT_RANGE	 0x3

struct can_mcan_ext_filter {
	volatile uint32_t id1: 29;
	volatile uint32_t efce: 3; /* Filter config */
	volatile uint32_t id2: 29; /* ID2 for dual or range, mask otherwise */
	volatile uint32_t res: 1;
	volatile uint32_t eft: 2; /* Filter type */
} __packed __aligned(4);

struct can_mcan_msg_sram {
	volatile struct can_mcan_std_filter std_filt[NUM_STD_FILTER_ELEMENTS];
	volatile struct can_mcan_ext_filter ext_filt[NUM_EXT_FILTER_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo0[NUM_RX_FIFO0_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo1[NUM_RX_FIFO1_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_buffer[NUM_RX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_event_fifo tx_event_fifo[NUM_TX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_buffer tx_buffer[NUM_TX_BUF_ELEMENTS];
} __packed __aligned(4);

struct can_mcan_data {
	struct can_mcan_msg_sram *msg_ram;
	struct k_mutex lock;
	struct k_sem tx_sem;
	struct k_mutex tx_mtx;
	can_tx_callback_t tx_fin_cb[NUM_TX_BUF_ELEMENTS];
	void *tx_fin_cb_arg[NUM_TX_BUF_ELEMENTS];
	can_rx_callback_t rx_cb_std[NUM_STD_FILTER_DATA];
	can_rx_callback_t rx_cb_ext[NUM_EXT_FILTER_DATA];
	void *cb_arg_std[NUM_STD_FILTER_DATA];
	void *cb_arg_ext[NUM_EXT_FILTER_DATA];
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
	uint32_t std_filt_fd_frame;
	uint32_t std_filt_rtr;
	uint32_t std_filt_rtr_mask;
	uint16_t ext_filt_fd_frame;
	uint16_t ext_filt_rtr;
	uint16_t ext_filt_rtr_mask;
	struct can_mcan_mm mm;
	bool started;
#ifdef CONFIG_CAN_FD_MODE
	bool fd;
#endif /* CONFIG_CAN_FD_MODE */
	void *custom;
} __aligned(4);

/**
 * @brief Bosch M_CAN driver front-end callback for reading a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_read_reg_t)(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * @brief Bosch M_CAN driver front-end callback for writing a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_write_reg_t)(const struct device *dev, uint16_t reg, uint32_t val);

struct can_mcan_config {
	can_mcan_read_reg_t read_reg;
	can_mcan_write_reg_t write_reg;
	uint32_t bus_speed;
	uint32_t bus_speed_data;
	uint16_t sjw;
	uint16_t sample_point;
	uint16_t prop_ts1;
	uint16_t ts2;
#ifdef CONFIG_CAN_FD_MODE
	uint16_t sample_point_data;
	uint8_t sjw_data;
	uint8_t prop_ts1_data;
	uint8_t ts2_data;
	uint8_t tx_delay_comp_offset;
#endif
	const struct device *phy;
	uint32_t max_bitrate;
	const void *custom;
};

/**
 * @brief Static initializer for @p can_mcan_config struct
 *
 * @param node_id Devicetree node identifier
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _read_reg Driver frontend Bosch M_CAN register read function
 * @param _write_reg Driver frontend Bosch M_CAN register write function
 */
#ifdef CONFIG_CAN_FD_MODE
#define CAN_MCAN_DT_CONFIG_GET(node_id, _custom, _read_reg, _write_reg)                            \
	{                                                                                          \
		.read_reg = _read_reg, .write_reg = _write_reg,                                    \
		.bus_speed = DT_PROP(node_id, bus_speed), .sjw = DT_PROP(node_id, sjw),            \
		.sample_point = DT_PROP_OR(node_id, sample_point, 0),                              \
		.prop_ts1 = DT_PROP_OR(node_id, prop_seg, 0) + DT_PROP_OR(node_id, phase_seg1, 0), \
		.ts2 = DT_PROP_OR(node_id, phase_seg2, 0),                                         \
		.bus_speed_data = DT_PROP(node_id, bus_speed_data),                                \
		.sjw_data = DT_PROP(node_id, sjw_data),                                            \
		.sample_point_data = DT_PROP_OR(node_id, sample_point_data, 0),                    \
		.prop_ts1_data = DT_PROP_OR(node_id, prop_seg_data, 0) +                           \
				 DT_PROP_OR(node_id, phase_seg1_data, 0),                          \
		.ts2_data = DT_PROP_OR(node_id, phase_seg2_data, 0),                               \
		.tx_delay_comp_offset = DT_PROP(node_id, tx_delay_comp_offset),                    \
		.phy = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node_id, phys)),                           \
		.max_bitrate = DT_CAN_TRANSCEIVER_MAX_BITRATE(node_id, 8000000),                   \
		.custom = _custom,                                                                 \
	}
#else /* CONFIG_CAN_FD_MODE */
#define CAN_MCAN_DT_CONFIG_GET(node_id, _custom, _read_reg, _write_reg)                            \
	{                                                                                          \
		.read_reg = _read_reg, .write_reg = _write_reg,                                    \
		.bus_speed = DT_PROP(node_id, bus_speed), .sjw = DT_PROP(node_id, sjw),            \
		.sample_point = DT_PROP_OR(node_id, sample_point, 0),                              \
		.prop_ts1 = DT_PROP_OR(node_id, prop_seg, 0) + DT_PROP_OR(node_id, phase_seg1, 0), \
		.ts2 = DT_PROP_OR(node_id, phase_seg2, 0),                                         \
		.phy = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node_id, phys)),                           \
		.max_bitrate = DT_CAN_TRANSCEIVER_MAX_BITRATE(node_id, 1000000),                   \
		.custom = _custom,                                                                 \
	}
#endif /* !CONFIG_CAN_FD_MODE */

/**
 * @brief Static initializer for @p can_mcan_config struct from DT_DRV_COMPAT instance
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _read_reg Driver frontend Bosch M_CAN register read function
 * @param _write_reg Driver frontend Bosch M_CAN register write function
 * @see CAN_MCAN_DT_CONFIG_GET()
 */
#define CAN_MCAN_DT_CONFIG_INST_GET(inst, _custom, _read_reg, _write_reg)                          \
	CAN_MCAN_DT_CONFIG_GET(DT_DRV_INST(inst), _custom, _read_reg, _write_reg)

/**
 * @brief Initializer for a @a can_mcan_data struct
 * @param _msg_ram Pointer to message RAM structure
 * @param _custom Pointer to custom driver frontend data structure
 */
#define CAN_MCAN_DATA_INITIALIZER(_msg_ram, _custom)                                               \
	{                                                                                          \
		.msg_ram = _msg_ram, .custom = _custom,                                            \
	}

/**
 * @brief Bosch M_CAN driver front-end callback helper for reading a memory mapped register
 *
 * @param base Register base address
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 Memory mapped register read always succeeds.
 */
static inline int can_mcan_sys_read_reg(mm_reg_t base, uint16_t reg, uint32_t *val)
{
	*val = sys_read32(base + reg);

	return 0;
}

/**
 * @brief Bosch M_CAN driver front-end callback helper for writing a memory mapped register
 *
 * @param base Register base address
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 Memory mapped register write always succeeds.
 */
static inline int can_mcan_sys_write_reg(mm_reg_t base, uint16_t reg, uint32_t val)
{
	sys_write32(val, base + reg);

	return 0;
}

/**
 * @brief Read a Bosch M_CAN register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
int can_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * @brief Write a Bosch M_CAN register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
int can_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val);

int can_mcan_get_capabilities(const struct device *dev, can_mode_t *cap);

int can_mcan_start(const struct device *dev);

int can_mcan_stop(const struct device *dev);

int can_mcan_set_mode(const struct device *dev, can_mode_t mode);

int can_mcan_set_timing(const struct device *dev, const struct can_timing *timing);

int can_mcan_set_timing_data(const struct device *dev, const struct can_timing *timing_data);

int can_mcan_init(const struct device *dev);

void can_mcan_line_0_isr(const struct device *dev);

void can_mcan_line_1_isr(const struct device *dev);

int can_mcan_recover(const struct device *dev, k_timeout_t timeout);

int can_mcan_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		  can_tx_callback_t callback, void *user_data);

int can_mcan_get_max_filters(const struct device *dev, bool ide);

int can_mcan_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			   const struct can_filter *filter);

void can_mcan_remove_rx_filter(const struct device *dev, int filter_id);

int can_mcan_get_state(const struct device *dev, enum can_state *state,
		       struct can_bus_err_cnt *err_cnt);

void can_mcan_set_state_change_callback(const struct device *dev,
					can_state_change_callback_t callback, void *user_data);

int can_mcan_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate);

void can_mcan_enable_configuration_change(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_CAN_MCAN_H_ */
