/* Freescale K20 microprocessor UART register definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Contains the UART Registers for the K20 Family of microprocessors.
 */

#ifndef _K20UART_H_
#define _K20UART_H_

#include <stdint.h>
#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

union BDH {
	uint8_t value;
	struct {
		uint8_t sbr : 5 __packed; /* Hi Baud Rate Bits */
		uint8_t res_5 : 1 __packed;
		uint8_t rx_edge_int_en : 1 __packed; /* RxD Active Edge */
		uint8_t lbkd_int_en : 1 __packed; /* LIN Break Detect */
	} field;
}; /* 0x000 BaudRate High */

union C1 {
	uint8_t value;
	struct {
		uint8_t odd_parity : 1 __packed;
		uint8_t parity_enable : 1 __packed;
		uint8_t idle_line_type : 1 __packed;
		uint8_t rx_wakep_method : 1 __packed;
		uint8_t mode9bit : 1 __packed;
		uint8_t remote_loopback : 1 __packed;
		uint8_t uart_stop_wait : 1 __packed;
		uint8_t loopback_en : 1 __packed;
	} field;
}; /* 0x002 Control 1 */

#define RX_EN_MASK 0x04
#define TX_EN_MASK 0x08

union C2 {
	uint8_t value;
	struct {
		uint8_t send_break : 1 __packed;
		uint8_t rx_wakeup_ctrl : 1 __packed;
		uint8_t rx_enable : 1 __packed;
		uint8_t tx_enable : 1 __packed;
		uint8_t idle_line_int_en : 1 __packed;
		uint8_t rx_full_int_dma_tx_en : 1 __packed;
		uint8_t tx_complete_int_en : 1 __packed;
		uint8_t tx_int_dma_tx_en : 1 __packed;
	} field;
}; /* 0x003 Control 2 */

union C3 {
	uint8_t value;
	struct {
		uint8_t parity_err_int_en : 1 __packed;
		uint8_t frame_err_int_en : 1 __packed;
		uint8_t noise_err_int_en : 1 __packed;
		uint8_t overrun_err_int_en : 1 __packed;
		uint8_t tx_data_invert : 1 __packed;
		uint8_t tx_data_pin_outt_dir : 1 __packed;
		uint8_t tx_bit8 : 1 __packed;
		uint8_t rx_bit8 : 1 __packed;
	} field;
}; /* 0x006 Control 3 */

union C4 {
	uint8_t value;
	struct {
		uint8_t brfa : 5 __packed; /* BaudRateFineAdjust*/
		uint8_t mode10bit : 1 __packed;
		uint8_t matech_addr_mode1_en : 1 __packed;
		uint8_t match_addr_mode2_en : 1 __packed;
	} field;
}; /* 0x00A Control 4 */

#define TX_DATA_EMPTY_MASK 0x80
#define RX_DATA_FULL_MASK 0x20

union S1 {
	uint8_t value;
	struct {
		uint8_t parity_err : 1 __packed;
		uint8_t framing_err : 1 __packed;
		uint8_t noice : 1 __packed;
		uint8_t rx_overrun : 1 __packed;
		uint8_t idle_line : 1 __packed;
		uint8_t rx_data_full : 1 __packed;
		uint8_t tx_complete : 1 __packed;
		uint8_t tx_data_empty : 1 __packed;
	} field;
}; /* 0x004 Status 1 */

union S2 {
	uint8_t value;
	struct e {
		uint8_t rx_active : 1 __packed;
		uint8_t lin_bk_detect_en : 1 __packed;
		uint8_t brk_char_len13 : 1 __packed;
		uint8_t rx_wakeup_idle_detect : 1 __packed;
		uint8_t rx_data_inverted : 1 __packed;
		uint8_t msb_first : 1 __packed;
		uint8_t rxedgif : 1 __packed;
		uint8_t lbkdif : 1 __packed;
	} field;
}; /* 0x005 Status 2 */

#define FIFO_SIZE_1 0
#define FIFO_SIZE_4 1
#define FIFO_SIZE_8 2
#define FIFO_SIZE_16 3
#define FIFO_SIZE_32 4
#define FIFO_SIZE_64 5
#define FIFO_SIZE_128 6
#define FIFO_SIZE_RES 6 /* Reserved size */

#define RX_FIFO_EN_MASK 0x08
#define TX_FIFO_EN_MASK 0x80

union PFIFO {
	uint8_t value;
	struct {
		uint8_t rx_fifo_size : 3 __packed; /* read-only */
		uint8_t rx_fifo_en : 1 __packed;
		uint8_t tx_fifo_size : 3 __packed; /* read-only */
		uint8_t tx_fifo_en : 1 __packed;
	} field;
}; /* 0x010 Fifo Parameter 1 */

#define RX_FIFO_FLUSH_MASK 0x40
#define TX_FIFO_FLUSH_MASK 0x80

union CFIFO {
	uint8_t value;
	struct {
		uint8_t rx_fifo_underflow_int_en : 1 __packed;
		uint8_t tx_fifo_overflow_int_en : 1 __packed;
		uint8_t rx_fifo_overflow_int_en : 1 __packed;
		uint8_t res_3 : 3 __packed;
		uint8_t rx_fifo_flush : 1 __packed; /* write-only */
		uint8_t tx_fifo_flush : 1 __packed; /* write-only */
	} field;
}; /* 0x011 Fifo Control */

struct K20_UART {
	union BDH bdh;		 /* 0x000 Baud Rate High */
	uint8_t bdl;			 /* 0x001 Baud Rate Low (04)*/
	union C1 c1;			 /* 0x002 Control 1 */
	union C2 c2;			 /* 0x003 Control 2 */
	union S1 s1;			 /* 0x004 Status  1 (C0) RO*/
	union S2 s2;			 /* 0x005 Status  2 */
	union C3 c3;			 /* 0x006 Control 3 */
	uint8_t d;			 /* 0x007 Data */
	uint8_t ma1;			 /* 0x008 Match Address 1 */
	uint8_t ma2;			 /* 0x009 Match Address 1 */
	union C4 c4;			 /* 0x00A Control 4 */
	uint8_t c5;			 /* 0x00B Control 5 */
	uint8_t ed;			 /* 0x00C Extended Data */
	uint8_t modem;			 /* 0x00D Modem */
	uint8_t ir;			 /* 0x00E Infrared */
	uint8_t z_reserved00f;		 /* 0x00F */
	union PFIFO pfifo;		 /* 0x010 FIFO Param */
	union CFIFO cfifo;		 /* 0x011 FIFO Control */
	uint8_t sfifo;			 /* 0x012 FIFO Status (C0)*/
	uint8_t twfifo;			 /* 0x013 FIFO Tx Watermark */
	uint8_t tcfifo;			 /* 0x014 FIFO Tx Count */
	uint8_t rwfifo;			 /* 0x015 FIFO Rx Watermark (01)*/
	uint8_t rcfifo;			 /* 0x016 FIFO Rx Count */
	uint8_t u_7816[0x20 - 0x17];     /* 0x017-0x1F UART ISO-7816 standard */
	uint8_t u_cea709_1[0x32 - 0x20]; /* 0x020-0x31 UART CEA8709.1 standard*/
	/* 0x032-0xFFF Reserved */
	uint8_t z_reserv_statused038_03c[0x1000 - 0x32];

}; /* K20 Microntroller UART module */

/**
 * @brief Set baud rate for K20 UART port.
 *
 * @param uart_p UART data
 * @param clk_freq Clock frequency
 * @param baud_rate Baud rate to set
 *
 * @return N/A
 */
static ALWAYS_INLINE void _uart_k20_baud_rate_set(volatile struct K20_UART *u,
						  uint32_t clk_freq,
						  uint32_t baud_rate)
{
	/* avoid divide by zero */
	if ((baud_rate == 0) || (clk_freq == 0)) {
		return;
	}

	/*
	 * The baud rate is calculated as:
	 * baud_rate = clk_freq/(16*(SBR[12:0]+BRFA[5:0]/32)), where
	 * - SBR is the combined UART Baud Rate Register settings and
	 * - BRFA is the UART Baud Rate Fine Adjustment setting
	 * This is equivalent to:
	 * 32xSBR + BRFA = 2 * clkFreq/baudRate
	 */
	uint32_t clk_br = 2 * clk_freq / baud_rate;
	uint16_t sbr = clk_br >> 5;
	uint8_t brfa = clk_br - (sbr << 5);

	__ASSERT((sbr && 0x1FFF),
		 "clk_freq is too high or baud_rate is too low");

	/* Note there are other fields (interrupts flag) in BDH register */
	u->bdh.field.sbr = (uint8_t)(sbr >> 8);
	u->bdl = (uint8_t)(sbr & 0xFF);
	u->c4.field.brfa = brfa;
}

/**
 * @brief Enable FIFO for K20 UART port
 *
 * @param uart_p UART data
 *
 * @return N/A
 */
static inline void _uart_k20_fifo_enable(volatile struct K20_UART *uart_p)
{
	uint8_t tx_rx_state = uart_p->c2.value && (TX_EN_MASK | RX_EN_MASK);

	/* disable Rx and Tx */
	uart_p->c2.value &= ~(TX_EN_MASK | RX_EN_MASK);

	uart_p->pfifo.value |= (TX_FIFO_EN_MASK | RX_FIFO_EN_MASK);

	uart_p->cfifo.value |= (TX_FIFO_FLUSH_MASK | RX_FIFO_FLUSH_MASK);

	/* restore Rx and Tx */
	uart_p->c2.value |= tx_rx_state;
}

#ifdef __cplusplus
}
#endif

#endif /* _K20UART_H_ */
