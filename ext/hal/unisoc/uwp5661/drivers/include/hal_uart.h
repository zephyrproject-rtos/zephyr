/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_UART_H
#define __HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#define MODE_UART	0
#define MODE_IRDA	1

#define MAX_TX_COUNT 0x7F
	enum {
		UART_RXF_FULL = 0,
		UART_TXF_EMPTY,
		UART_PARITY_ERR,
		UART_FRAME_ERR,
		UART_RXF_OVERRUN,
		UART_DSR_CHG,
		UART_CTS_CHG,
		UART_BRK_DTCT,
		UART_DSR,
		UART_CTS,
		UART_RTS,
		UART_RXD,
		UART_RXF_REALEMPTY,
		UART_TIME_OUT,
		UART_RXF_REALFULL,
		UART_TRANS_OVER,
	};

	typedef struct uart_txd {
		u32_t txd	:8;
		u32_t rsvd	:24;
	}txd_t;

	typedef struct uart_rxd {
		u32_t rxd	:8;
		u32_t rsvd	:24;
	}rxd_t;

	typedef struct uart_sts1 {
		u32_t rxf_cnt		:7;
		u32_t rsvd0			:1;
		u32_t txf_cnt		:7;
		u32_t rsvd1			:17;
	}sts1_t;

	typedef union uart_ctrl0 {
		u32_t reg;

		struct {
			u32_t odd_parity	:1;
			u32_t parity_en		:1;
			u32_t byte_len		:2;
			u32_t stop_bit_num	:2;
			u32_t rts_reg		:1;
			u32_t send_brk_en	:1;
			u32_t dtr_reg		:1;
			u32_t ir_tx_iv		:1;
			u32_t ir_rx_iv		:1;
			u32_t ir_tx_en		:1;
			u32_t ir_dplx		:1;
			u32_t ir_wctl		:1;
			u32_t rsvd0			:1;
			u32_t mode_sel		:1;
			u32_t rsvd1			:16;
		}bit;
	}ctrl0_t;

	typedef union uart_ctrl1 {
		u32_t reg;

		struct {
			u32_t rcv_hw_flow_thld	:7;
			u32_t rcv_hw_flow_en	:1;
			u32_t tx_hw_flow_en		:1;
			u32_t rx_tout_thld		:5;
			u32_t loop_back			:1;
			u32_t dma_en			:1;
			u32_t rsvd				:16;
		}bit;
	}ctrl1_t;

	typedef union uart_ctrl2 {
		u32_t reg;
		struct {
			u32_t rxf_full_thld		:7;
			u32_t rsvd0				:1;
			u32_t txf_empty_thld	:7;
			u32_t rsvd1				:17;
		}bit;
	}ctrl2_t;

	typedef struct uart_cdk0 {
		u32_t cdk0		:16;
		u32_t rsvd		:16;
	}cdk0_t;

	typedef struct uart_dspwait {
		u32_t dspwait			:4;
		u32_t rx_dma_mod_sel	:1;
		u32_t tx_dma_mod_sel	:1;
	}dspwait_t;

	struct uwp_uart {
		txd_t		txd;
		rxd_t		rxd;
		u32_t		sts0;
		sts1_t		sts1;
		u32_t		ien;
		u32_t		iclr;
		ctrl0_t		ctrl0;
		ctrl1_t		ctrl1;
		ctrl2_t		ctrl2;
		cdk0_t		cdk0;
		u32_t		sts2;
		dspwait_t	dspwait;
	};

static inline u32_t malin_uart_rx_ready(volatile struct uwp_uart *uart)
{
	return uart->sts1.rxf_cnt > 0 ? 1 : 0;
}

static inline u32_t malin_uart_tx_ready(volatile struct uwp_uart *uart)
{
	return uart->sts1.txf_cnt < MAX_TX_COUNT ? 1 : 0;
}

static inline u8_t uwp_uart_read(volatile struct uwp_uart *uart)
{
	return uart->rxd.rxd;
}

static inline void uwp_uart_write(volatile struct uwp_uart *uart,
		u8_t c)
{
	uart->txd.txd = c;
}

static inline u32_t uwp_uart_trans_over(volatile struct uwp_uart *uart)
{
	return uart->sts0 & BIT(UART_TRANS_OVER);
}

static inline void uwp_uart_set_cdk(volatile struct uwp_uart *uart,
		u32_t cdk)
{
	uart->cdk0.cdk0 = cdk;
}

static inline void uwp_uart_set_stop_bit_num(volatile struct uwp_uart *uart,
		u32_t stop_bit_num)
{
	uart->ctrl0.bit.stop_bit_num = stop_bit_num;
}

static inline void uwp_uart_set_byte_len(volatile struct uwp_uart *uart,
		u32_t byte_len)
{
	uart->ctrl0.bit.byte_len = byte_len;
}

static inline void uwp_uart_init(volatile struct uwp_uart *uart)
{
//	uart->cdk0.cdk0 = 0xe1;

	uart->ien = 0;
	uart->ctrl1.bit.rx_tout_thld = 1;
	uart->ctrl2.bit.rxf_full_thld = 1;
	uart->ctrl2.bit.txf_empty_thld = 64;
}

static inline void uwp_uart_int_enable(volatile struct uwp_uart *uart,
		u32_t flags)
{
	uart->ien |= flags;
}

static inline void uwp_uart_int_disable(volatile struct uwp_uart *uart,
		u32_t flags)
{
	uart->ien &= ~flags;
}

static inline void uwp_uart_int_clear(volatile struct uwp_uart *uart,
		u32_t flags)
{
	uart->iclr |= flags;
}

static inline u32_t uwp_uart_status(volatile struct uwp_uart *uart)
{
	return uart->sts0;
}

#ifdef __cplusplus
}
#endif

#endif
