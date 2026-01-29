/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UART_XUANTIE_LL_H_
#define _UART_XUANTIE_LL_H_

#include <stdio.h>
#include <csi_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */

#ifdef CONFIG_XIP
#define ATTRIBUTE_DATA __attribute__((section(".ram.code")))
#else
#define ATTRIBUTE_DATA
#endif

/*! IER, offset: 0x4 */
#define DW_UART_IER_ERBFI_Pos             (0U)
#define DW_UART_IER_ERBFI_Msk             (0x1U << DW_UART_IER_ERBFI_Pos)
#define DW_UART_IER_ERBFI_EN              DW_UART_IER_ERBFI_Msk

#define DW_UART_IER_ETBEI_Pos             (1U)
#define DW_UART_IER_ETBEI_Msk             (0x1U << DW_UART_IER_ETBEI_Pos)
#define DW_UART_IER_ETBEI_EN              DW_UART_IER_ETBEI_Msk

#define DW_UART_IER_ELSI_Pos              (2U)
#define DW_UART_IER_ELSI_Msk              (0x1U << DW_UART_IER_ELSI_Pos)
#define DW_UART_IER_ELSI_EN               DW_UART_IER_ELSI_Msk

#define DW_UART_IER_EDSSI_Pos             (3U)
#define DW_UART_IER_EDSSI_Msk             (0x1U << DW_UART_IER_EDSSI_Pos)
#define DW_UART_IER_EDSSI_EN              DW_UART_IER_EDSSI_Msk

/*! IIR, offset: 0x8 */
#define DW_UART_IIR_IID_Pos               (0U)
#define DW_UART_IIR_IID_Msk               (0xFU << DW_UART_IIR_IID_Pos)
#define DW_UART_IIR_IID_MODEM_STATUS      (0x0U)
#define DW_UART_IIR_IID_NO_INTERRUPT      (0x1U)
#define DW_UART_IIR_IID_THR_EMPTY         (0x2U)
#define DW_UART_IIR_IID_RECV_DATA_AVAIL   (0x4U)
#define DW_UART_IIR_IID_RECV_LINE_STATUS  (0x6U)
#define DW_UART_IIR_IID_BUSY_DETECT       (0x7U)
#define DW_UART_IIR_IID_CHARACTER_TIMEOUT (0xCU)

#define DW_UART_IIR_FIFOSE_Pos            (6U)
#define DW_UART_IIR_FIFOSE_Msk            (0x3U << DW_UART_IIR_FIFOSE_Pos)
#define DW_UART_IIR_FIFOSE_EN             DW_UART_IIR_FIFOSE_Msk

/*! FCR, offset: 0x8 */
#define DW_UART_FCR_FIFOE_Pos             (0U)
#define DW_UART_FCR_FIFOE_Msk             (0x1U << DW_UART_FCR_FIFOE_Pos)
#define DW_UART_FCR_FIFOE_EN              DW_UART_FCR_FIFOE_Msk

#define DW_UART_FCR_RFIFOR_Pos            (1U)
#define DW_UART_FCR_RFIFOR_Msk            (0x1U << DW_UART_FCR_RFIFOR_Pos)
#define DW_UART_FCR_RFIFOR_RESET          DW_UART_FCR_RFIFOR_Msk

#define DW_UART_FCR_XFIFOR_Pos            (2U)
#define DW_UART_FCR_XFIFOR_Msk            (0x1U << DW_UART_FCR_XFIFOR_Pos)
#define DW_UART_FCR_XFIFOR_RESET          DW_UART_FCR_XFIFOR_Msk

#define DW_UART_FCR_TET_Pos               (4U)
#define DW_UART_FCR_TET_Msk               (0x3U << DW_UART_FCR_TET_Pos)
#define DW_UART_FCR_TET_FIFO_EMTPY        (0x0U)
#define DW_UART_FCR_TET_FIFO_2_CHAR       (0x1U << DW_UART_FCR_TET_Pos)
#define DW_UART_FCR_TET_FIFO_1_4_FULL     (0x2U << DW_UART_FCR_TET_Pos)
#define DW_UART_FCR_TET_FIFO_1_2_FULL     (0x3U << DW_UART_FCR_TET_Pos)

#define DW_UART_FCR_RT_Pos                (6U)
#define DW_UART_FCR_RT_Msk                (0x3U << DW_UART_FCR_RT_Pos)
#define DW_UART_FCR_RT_FIFO_1_CHAR        (0x0U)
#define DW_UART_FCR_RT_FIFO_1_4_FULL      (0x1U << DW_UART_FCR_RT_Pos)
#define DW_UART_FCR_RT_FIFO_1_2_FULL      (0x2U << DW_UART_FCR_RT_Pos)
#define DW_UART_FCR_RT_FIFO_2_LESS_FULL   (0x3U << DW_UART_FCR_RT_Pos)

/*! LCR, offset: 0xC */
#define DW_UART_LCR_DLS_Pos               (0U)
#define DW_UART_LCR_DLS_Msk               (0x3U << DW_UART_LCR_DLS_Pos)
#define DW_UART_LCR_DLS_5_BITS            (0x0U)
#define DW_UART_LCR_DLS_6_BITS            (0x1U << DW_UART_LCR_DLS_Pos)
#define DW_UART_LCR_DLS_7_BITS            (0x2U << DW_UART_LCR_DLS_Pos)
#define DW_UART_LCR_DLS_8_BITS            (0x3U << DW_UART_LCR_DLS_Pos)

#define DW_UART_LCR_STOP_Pos              (2U)
#define DW_UART_LCR_STOP_Msk              (0x1U << DW_UART_LCR_STOP_Pos)
#define DW_UART_LCR_STOP_1_BIT            (0x0U)
#define DW_UART_LCR_STOP_2_BIT            (0x1U << DW_UART_LCR_STOP_Pos)

#define DW_UART_LCR_PEN_Pos               (3U)
#define DW_UART_LCR_PEN_Msk               (0x1U << DW_UART_LCR_PEN_Pos)
#define DW_UART_LCR_PEN_EN                DW_UART_LCR_PEN_Msk

#define DW_UART_LCR_EPS_Pos               (4U)
#define DW_UART_LCR_EPS_Msk               (0x1U << DW_UART_LCR_EPS_Pos)
#define DW_UART_LCR_EPS_EN                DW_UART_LCR_EPS_Msk

#define DW_UART_LCR_BC_Pos                (6U)
#define DW_UART_LCR_BC_Msk                (0x1U << DW_UART_LCR_BC_Pos)
#define DW_UART_LCR_BC_EN                 DW_UART_LCR_BC_Msk

#define DW_UART_LCR_DLAB_Pos              (7U)
#define DW_UART_LCR_DLAB_Msk              (0x1U << DW_UART_LCR_DLAB_Pos)
#define DW_UART_LCR_DLAB_EN               DW_UART_LCR_DLAB_Msk

/*! MCR, offset: 0x10 */
#define DW_UART_MCR_RTS_Pos               (1U)
#define DW_UART_MCR_RTS_Msk               (0x1U << DW_UART_MCR_RTS_Pos)
#define DW_UART_MCR_RTS_EN                DW_UART_MCR_RTS_Msk

#define DW_UART_MCR_LB_Pos                (4U)
#define DW_UART_MCR_LB_Msk                (0x1U << DW_UART_MCR_LB_Pos)
#define DW_UART_MCR_LB_EN                 DW_UART_MCR_LB_Msk

#define DW_UART_MCR_AFCE_Pos              (5U)
#define DW_UART_MCR_AFCE_Msk              (0x1U << DW_UART_MCR_AFCE_Pos)
#define DW_UART_MCR_AFCE_EN               DW_UART_MCR_AFCE_Msk

/*! LSR, offset: 0x14 */
#define DW_UART_LSR_DR_Pos                (0U)
#define DW_UART_LSR_DR_Msk                (0x1U << DW_UART_LSR_DR_Pos)
#define DW_UART_LSR_DR_READY              DW_UART_LSR_DR_Msk

#define DW_UART_LSR_OE_Pos                (1U)
#define DW_UART_LSR_OE_Msk                (0x1U << DW_UART_LSR_OE_Pos)
#define DW_UART_LSR_OE_ERROR              DW_UART_LSR_OE_Msk

#define DW_UART_LSR_PE_Pos                (2U)
#define DW_UART_LSR_PE_Msk                (0x1U << DW_UART_LSR_PE_Pos)
#define DW_UART_LSR_PE_ERROR              DW_UART_LSR_PE_Msk

#define DW_UART_LSR_FE_Pos                (3U)
#define DW_UART_LSR_FE_Msk                (0x1U << DW_UART_LSR_FE_Pos)
#define DW_UART_LSR_FE_ERROR              DW_UART_LSR_FE_Msk

#define DW_UART_LSR_BI_Pos                (4U)
#define DW_UART_LSR_BI_Msk                (0x1U << DW_UART_LSR_BI_Pos)
#define DW_UART_LSR_BI_SET                DW_UART_LSR_BI_Msk

#define DW_UART_LSR_THRE_Pos              (5U)
#define DW_UART_LSR_THRE_Msk              (0x1U << DW_UART_LSR_THRE_Pos)
#define DW_UART_LSR_THRE_SET              DW_UART_LSR_THRE_Msk

#define DW_UART_LSR_TEMT_Pos              (6U)
#define DW_UART_LSR_TEMT_Msk              (0x1U << DW_UART_LSR_TEMT_Pos)
#define DW_UART_LSR_TEMT_SET              DW_UART_LSR_TEMT_Msk

#define DW_UART_LSR_RFE_Pos               (7U)
#define DW_UART_LSR_RFE_Msk               (0x1U << DW_UART_LSR_RFE_Pos)
#define DW_UART_LSR_RFE_ERROR             DW_UART_LSR_RFE_Msk

/*! MSR, offset: 0x18 */
#define DW_UART_MSR_DCTS_Pos              (0U)
#define DW_UART_MSR_DCTS_Msk              (0x1U << DW_UART_MSR_DCTS_Pos)
#define DW_UART_MSR_DCTS_CHANGE           DW_UART_MSR_DCTS_Msk

#define DW_UART_MSR_DDSR_Pos              (1U)
#define DW_UART_MSR_DDSR_Msk              (0x1U << DW_UART_MSR_DDSR_Pos)
#define DW_UART_MSR_DDSR_CHANGE           DW_UART_MSR_DDSR_Msk

#define DW_UART_MSR_TERI_Pos              (2U)
#define DW_UART_MSR_TERI_Msk              (0x1U << DW_UART_MSR_TERI_Pos)
#define DW_UART_MSR_TERI_CHANGE           DW_UART_MSR_TERI_Msk

#define DW_UART_MSR_DDCD_Pos              (3U)
#define DW_UART_MSR_DDCD_Msk              (0x1U << DW_UART_MSR_DDCD_Pos)
#define DW_UART_MSR_DDCD_CHANGE           DW_UART_MSR_DDCD_Msk

#define DW_UART_MSR_CTS_Pos               (4U)
#define DW_UART_MSR_CTS_Msk               (0x1U << DW_UART_MSR_CTS_Pos)
#define DW_UART_MSR_CTS_ASSERTED          DW_UART_MSR_CTS_Msk

#define DW_UART_MSR_DSR_Pos               (5U)
#define DW_UART_MSR_DSR_Msk               (0x1U << DW_UART_MSR_DSR_Pos)
#define DW_UART_MSR_DSR_ASSERTED          DW_UART_MSR_DSR_Msk

#define DW_UART_MSR_RI_Pos                (6U)
#define DW_UART_MSR_RI_Msk                (0x1U << DW_UART_MSR_RI_Pos)
#define DW_UART_MSR_RI_ASSERTED           DW_UART_MSR_RI_Msk

#define DW_UART_MSR_DCD_Pos               (7U)
#define DW_UART_MSR_DCD_Msk               (0x1U << DW_UART_MSR_DCD_Pos)
#define DW_UART_MSR_DCD_ASSERTED          DW_UART_MSR_DCD_Msk

/*! SCR, offset: 0x1C */
#define DW_UART_SCR_SCRATCHPAD_Pos        (0U)
#define DW_UART_SCR_SCRATCHPAD_Msk        (0xFFU << DW_UART_SCR_SCRATCHPAD_Pos)

/*! USR, offset: 0x7C */
#define DW_UART_USR_BUSY_Pos              (0U)
#define DW_UART_USR_BUSY_Msk              (0x1U << DW_UART_USR_BUSY_Pos)
#define DW_UART_USR_BUSY_SET              DW_UART_USR_BUSY_Msk

#define DW_UART_USR_TFNF_Pos              (1U)
#define DW_UART_USR_TFNF_Msk              (0x1U << DW_UART_USR_TFNF_Pos)
#define DW_UART_USR_TFNF_SET              DW_UART_USR_TFNF_Msk

#define DW_UART_USR_TFE_Pos               (2U)
#define DW_UART_USR_TFE_Msk               (0x1U << DW_UART_USR_TFE_Pos)
#define DW_UART_USR_TFE_SET               DW_UART_USR_TFE_Msk

#define DW_UART_USR_RFNE_Pos              (3U)
#define DW_UART_USR_RFNE_Msk              (0x1U << DW_UART_USR_RFNE_Pos)
#define DW_UART_USR_RFNE_SET              DW_UART_USR_RFNE_Msk

#define DW_UART_USR_RFF_Pos               (4U)
#define DW_UART_USR_RFF_Msk               (0x1U << DW_UART_USR_RFF_Pos)
#define DW_UART_USR_RFF_SET               DW_UART_USR_RFF_Msk

/*! TFL, offset: 0x80 */
#define DW_UART_TFL_TFIFOL_Pos            (0U)
#define DW_UART_TFL_TFIFOL_Msk            (0x1FU << DW_UART_TFL_TFIFOL_Pos)
#define DW_UART_TFL_TFIFOL(n)             (nU << DW_UART_TFL_TFIFOL_Pos)

/*! RFL, offset: 0x84 */
#define DW_UART_RFL_RFIFOL_Pos            (0U)
#define DW_UART_RFL_RFIFOL_Msk            (0x1FU << DW_UART_RFL_RFIFOL_Pos)
#define DW_UART_RFL_RFIFOL(n)             (nU << DW_UART_TFL_TFIFOL_Pos)

/*! HTX, offset: 0xA4 */
#define DW_UART_HTX_HALTTX_Pos            (0U)
#define DW_UART_HTX_HALTTX_Msk            (0x1U << DW_UART_HTX_HALTTX_Pos)
#define DW_UART_HTX_HALTTX_EN             DW_UART_HTX_HALTTX_Msk

#define DW_UART_HTX_RX_ETB_FUNC_Pos       (6U)
#define DW_UART_HTX_RX_ETB_FUNC_Msk       (0x1U << DW_UART_HTX_RX_ETB_FUNC_Pos)
#define DW_UART_HTX_RX_ETB_FUNC_EN        DW_UART_HTX_RX_ETB_FUNC_Msk

#define DW_UART_HTX_TX_ETB_FUNC_Pos       (7U)
#define DW_UART_HTX_TX_ETB_FUNC_Msk       (0x1U << DW_UART_HTX_TX_ETB_FUNC_Pos)
#define DW_UART_HTX_TX_ETB_FUNC_EN        DW_UART_HTX_TX_ETB_FUNC_Msk

/*! DMASA, offset: 0xA8 */
#define DW_UART_DMASA_DMASACK_Pos         (0U)
#define DW_UART_DMASA_DMASACK_Msk         (0x1U << DW_UART_DMASA_DMASACK_Pos)
#define DW_UART_DMASA_DMASACK_SET         DW_UART_DMASA_DMASACK_Msk

/* FIFO CONFIG */
#define UART_FIFO_INIT_CONFIG				\
	(DW_UART_FCR_FIFOE_EN			|	    \
	DW_UART_FCR_RT_FIFO_1_2_FULL	|		\
	DW_UART_FCR_RFIFOR_RESET		|		\
	DW_UART_FCR_XFIFOR_RESET)

/*! UART_RATE, offset: 0x3FC */
#define DW_UART_SUPPORT_RATE              0x10102U

#define UART_BUSY_TIMEOUT                 0x70000000U

typedef struct {
	union {
		/* Offset: 0x000 (R/ )  Receive buffer register */
		__IM  uint32_t RBR;
		/* Offset: 0x000 ( /W)  Transmission hold register */
		__OM  uint32_t THR;
		/* Offset: 0x000 (R/W)  Clock frequency division low section register */
		__IOM uint32_t DLL;
	};
	union {
		/* Offset: 0x004 (R/W)  Clock frequency division high section register */
		__IOM uint32_t DLH;
		/* Offset: 0x004 (R/W)  Interrupt enable register */
		__IOM uint32_t IER;
	};
	union {
		/* Offset: 0x008 (R/ )  Interrupt identification register */
		__IM  uint32_t IIR;
		/* Offset: 0x008 ( /W)  FIFO control register */
		__OM  uint32_t FCR;
	};
	/* Offset: 0x00C (R/W)  Line control register */
	__IOM uint32_t LCR;
	/* Offset: 0x010 (R/W)  Modem control register */
	__IOM uint32_t MCR;
	/* Offset: 0x014 (R/ )  Line state register */
	__IM  uint32_t LSR;
	/* Offset: 0x018 (R/ )  Modem state register */
	__IM  uint32_t MSR;
	uint32_t RESERVED1[21];
	/* Offset: 0x07c (R/ )  UART state register */
	__IM  uint32_t USR;
} dw_uart_regs_t;

/* clang-format on */

static inline void dw_uart_enable_recv_irq(dw_uart_regs_t *uart_base)
{
	uart_base->IER |= (DW_UART_IER_ERBFI_EN | DW_UART_IER_ELSI_EN);
}

static inline void dw_uart_disable_recv_irq(dw_uart_regs_t *uart_base)
{
	uart_base->IER &= ~(DW_UART_IER_ERBFI_EN | DW_UART_IER_ELSI_EN);
}

static inline void dw_uart_enable_trans_irq(dw_uart_regs_t *uart_base)
{
	uart_base->IER |= DW_UART_IER_ETBEI_EN;
}

static inline void dw_uart_disable_trans_irq(dw_uart_regs_t *uart_base)
{
	uart_base->IER &= ~(DW_UART_IER_ETBEI_EN);
}

static inline void dw_uart_fifo_init(dw_uart_regs_t *uart_base)
{
	/* FIFO enable */
	uart_base->FCR = UART_FIFO_INIT_CONFIG;
}

static inline void dw_uart_fifo_enable(dw_uart_regs_t *uart_base)
{
	uart_base->FCR |= DW_UART_FCR_FIFOE_EN;
}

static inline void dw_uart_fifo_disable(dw_uart_regs_t *uart_base)
{
	uart_base->FCR &= ~(DW_UART_FCR_FIFOE_EN);
}

static inline uint32_t dw_uart_putready(dw_uart_regs_t *uart_base)
{
	uint32_t status = 0U, ret = 0U;

	status = uart_base->LSR & DW_UART_LSR_THRE_SET;

	if (status != 0U) {
		ret = 1U;
	}

	return ret;
}

static inline uint32_t dw_uart_getready(dw_uart_regs_t *uart_base)
{
	uint32_t status = 0U, ret = 0U;

	status = uart_base->LSR & DW_UART_LSR_DR_READY;

	if (status != 0U) {
		ret = 1U;
	}

	return ret;
}

static inline uint32_t dw_uart_get_line_status(dw_uart_regs_t *uart_base)
{
	return uart_base->LSR;
}

static inline void dw_uart_config_stop_bits_1(dw_uart_regs_t *uart_base)
{
	uart_base->LCR &= ~(DW_UART_LCR_STOP_Msk);
}

static inline void dw_uart_config_stop_bits_2(dw_uart_regs_t *uart_base)
{
	uart_base->LCR |= DW_UART_LCR_STOP_2_BIT;
}

static inline void dw_uart_putchar(dw_uart_regs_t *uart_base, uint8_t ch)
{
	uart_base->THR = ch;
}

static inline uint8_t dw_uart_getchar(dw_uart_regs_t *uart_base)
{
	return (uint8_t)(uart_base->RBR);
}

static inline uint32_t dw_uart_get_intr_en_status(dw_uart_regs_t *uart_base)
{
	return uart_base->IER;
}

static inline void dw_uart_set_intr_en_status(dw_uart_regs_t *uart_base, uint32_t status)
{
	uart_base->IER = status;
}

static inline void dw_uart_set_fcr_reg(dw_uart_regs_t *uart_base, uint32_t value)
{
	uart_base->FCR = value;
}

static inline void dw_uart_enable_auto_flow_control(dw_uart_regs_t *uart_base)
{
	uart_base->MCR |= DW_UART_MCR_AFCE_EN;
	uart_base->MCR |= DW_UART_MCR_RTS_EN;
}

static inline void dw_uart_disable_auto_flow_control(dw_uart_regs_t *uart_base)
{
	uart_base->MCR &= ~DW_UART_MCR_AFCE_EN;
	uart_base->MCR &= ~DW_UART_MCR_RTS_EN;
}

int32_t dw_uart_wait_timeout(dw_uart_regs_t *uart_base);

int32_t dw_uart_wait_idle(dw_uart_regs_t *uart_base);

int32_t dw_uart_config_baudrate(dw_uart_regs_t *uart_base, uint32_t baud, uint32_t uart_freq);

int32_t dw_uart_config_stop_bits(dw_uart_regs_t *uart_base, uint32_t stop_bits);

int32_t dw_uart_config_parity_none(dw_uart_regs_t *uart_base);

int32_t dw_uart_config_parity_odd(dw_uart_regs_t *uart_base);

int32_t dw_uart_config_parity_even(dw_uart_regs_t *uart_base);

int32_t dw_uart_config_data_bits(dw_uart_regs_t *uart_base, uint32_t data_bits);

#ifdef __cplusplus
}
#endif

#endif /* _UART_XUANTIE_LL_H_ */
