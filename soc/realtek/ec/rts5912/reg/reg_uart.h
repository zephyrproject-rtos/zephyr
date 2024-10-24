/*
 * Copyright (c) 2023 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _REALTEK_RTS5912_REG_UART_H
#define _REALTEK_RTS5912_REG_UART_H

/*
 * @brief UART Controller (UART)
 */

typedef struct {
	union {
		const volatile uint32_t RBR;
		volatile uint32_t THR;
		volatile uint32_t DLL;
	};
	union {
		volatile uint32_t DLH;
		volatile uint32_t IER;
	};
	union {
		const volatile uint32_t IIR;
		volatile uint32_t FCR;
	};
	volatile uint32_t LCR;
	const volatile uint32_t RESERVED;
	const volatile uint32_t LSR;
	const volatile uint32_t RESERVED1[25];
	const volatile uint32_t USR;
	const volatile uint32_t TFL;
	const volatile uint32_t RFL;
	volatile uint32_t SRR;
} UART_Type;

/* RBR */
#define UART_RBR_DATA_Pos     (0UL)
#define UART_RBR_DATA_Msk     GENMASK(7, 0)
/* THR */
#define UART_THR_DATA_Pos     (0UL)
#define UART_THR_DATA_Msk     GENMASK(7, 0)
/* DLL */
#define UART_DLL_DIVL_Pos     (0UL)
#define UART_DLL_DIVL_Msk     GENMASK(7, 0)
/* DLH */
#define UART_DLH_DIVH_Pos     (0UL)
#define UART_DLH_DIVH_Msk     GENMASK(7, 0)
/* IER */
#define UART_IER_ERBFI_Pos    (0UL)
#define UART_IER_ERBFI_Msk    BIT(UART_IER_ERBFI_Pos)
#define UART_IER_ETBEI_Pos    (1UL)
#define UART_IER_ETBEI_Msk    BIT(UART_IER_ETBEI_Pos)
#define UART_IER_ELSI_Pos     (2UL)
#define UART_IER_ELSI_Msk     BIT(UART_IER_ELSI_Pos)
#define UART_IER_PTIME_Pos    (7UL)
#define UART_IER_PTIME_Msk    BIT(UART_IER_PTIME_Pos)
/* IIR */
#define UART_IIR_IID_Pos      (0UL)
#define UART_IIR_IID_Msk      GENMASK(3, 0)
#define UART_IIR_FIFOSE_Pos   (6UL)
#define UART_IIR_FIFOSE_Msk   GENMASK(7, 6)
/* FCR */
#define UART_FCR_FIFOE_Pos    (0UL)
#define UART_FCR_FIFOE_Msk    BIT(UART_FCR_FIFOE_Pos)
#define UART_FCR_RFIFOR_Pos   (1UL)
#define UART_FCR_RFIFOR_Msk   BIT(UART_FCR_RFIFOR_Pos)
#define UART_FCR_XFIFOR_Pos   (2UL)
#define UART_FCR_XFIFOR_Msk   BIT(UART_FCR_XFIFOR_Pos)
#define UART_FCR_TXTRILEV_Pos (4UL)
#define UART_FCR_TXTRILEV_Msk GENMASK(5, 4)
#define UART_FCR_RXTRILEV_Pos (6UL)
#define UART_FCR_RXTRILEV_Msk GENMASK(7, 6)
/* LCR */
#define UART_LCR_DLS_Pos      (0UL)
#define UART_LCR_DLS_Msk      GENMASK(1, 0)
#define UART_LCR_STOP_Pos     (2UL)
#define UART_LCR_STOP_Msk     BIT(UART_LCR_STOP_Pos)
#define UART_LCR_PEN_Pos      (3UL)
#define UART_LCR_PEN_Msk      BIT(UART_LCR_PEN_Pos)
#define UART_LCR_EPS_Pos      (4UL)
#define UART_LCR_EPS_Msk      BIT(UART_LCR_EPS_Pos)
#define UART_LCR_STP_Pos      (5UL)
#define UART_LCR_STP_Msk      BIT(UART_LCR_STP_Pos)
#define UART_LCR_BC_Pos       (6UL)
#define UART_LCR_BC_Msk       BIT(UART_LCR_BC_Pos)
#define UART_LCR_DLAB_Pos     (7UL)
#define UART_LCR_DLAB_Msk     BIT(UART_LCR_DLAB_Pos)
/* LSR */
#define UART_LSR_DR_Pos       (0UL)
#define UART_LSR_DR_Msk       BIT(UART_LSR_DR_Pos)
#define UART_LSR_OE_Pos       (1UL)
#define UART_LSR_OE_Msk       BIT(UART_LSR_OE_Pos)
#define UART_LSR_PE_Pos       (2UL)
#define UART_LSR_PE_Msk       BIT(UART_LSR_PE_Pos)
#define UART_LSR_FE_Pos       (3UL)
#define UART_LSR_FE_Msk       BIT(UART_LSR_FE_Pos)
#define UART_LSR_BI_Pos       (4UL)
#define UART_LSR_BI_Msk       BIT(UART_LSR_BI_Pos)
#define UART_LSR_THRE_Pos     (5UL)
#define UART_LSR_THRE_Msk     BIT(UART_LSR_THRE_Pos)
#define UART_LSR_TEMT_Pos     (6UL)
#define UART_LSR_TEMT_Msk     BIT(UART_LSR_TEMT_Pos)
#define UART_LSR_RFE_Pos      (7UL)
#define UART_LSR_RFE_Msk      BIT(UART_LSR_RFE_Pos)
/* USR */
#define UART_USR_BUSY_Pos     (0UL)
#define UART_USR_BUSY_Msk     BIT(UART_USR_BUSY_Pos)
#define UART_USR_TFNF_Pos     (1UL)
#define UART_USR_TFNF_Msk     BIT(UART_USR_TFNF_Pos)
#define UART_USR_TFE_Pos      (2UL)
#define UART_USR_TFE_Msk      BIT(UART_USR_TFE_Pos)
#define UART_USR_RFNE_Pos     (3UL)
#define UART_USR_RFNE_Msk     BIT(UART_USR_RFNE_Pos)
#define UART_USR_RFF_Pos      (4UL)
#define UART_USR_RFF_Msk      BIT(UART_USR_RFF_Pos)
/* SRR */
#define UART_SRR_UR_Pos       (0UL)
#define UART_SRR_UR_Msk       BIT(UART_SRR_UR_Pos)
#define UART_SRR_RFR_Pos      (1UL)
#define UART_SRR_RFR_Msk      BIT(UART_SRR_RFR_Pos)
#define UART_SRR_XFR_Pos      (2UL)
#define UART_SRR_XFR_Msk      BIT(UART_SRR_XFR_Pos)

#endif /* _REALTEK_RTS5912_REG_UART_H */
