/* Freescale K20 microprocessor UART register definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module defines the UART Registers for the K20 Family of microprocessors
*/

#ifndef _K20UART_H_
#define _K20UART_H_

#include <stdint.h>
#include <misc/__assert.h>

typedef union {
	uint8_t value;
	struct {
		uint8_t sbr : 5 __packed; /* Hi Baud Rate Bits */
		uint8_t res_5 : 1 __packed;
		uint8_t rxEdgeIntEn : 1 __packed; /* RxD Active Edge */
		uint8_t lbkdIntEn : 1 __packed; /* LIN Break Detect */
	} field;
} BDH_t; /* 0x000 BaudRate High */

typedef union {
	uint8_t value;
	struct {
		uint8_t oddParity : 1 __packed;
		uint8_t parityEnable : 1 __packed;
		uint8_t idleLineType : 1 __packed;
		uint8_t rxWakepMethod : 1 __packed;
		uint8_t mode9Bit : 1 __packed;
		uint8_t remoteLoopback : 1 __packed;
		uint8_t uartStopWait : 1 __packed;
		uint8_t loopbackEn : 1 __packed;
	} field;
} C1_t; /* 0x002 Control 1 */

#define RX_EN_MASK 0x04
#define TX_EN_MASK 0x08

typedef union {
	uint8_t value;
	struct {
		uint8_t sendBreak : 1 __packed;
		uint8_t rxWakeupCtrl : 1 __packed;
		uint8_t rxEnable : 1 __packed;
		uint8_t txEnable : 1 __packed;
		uint8_t idleLineIntEn : 1 __packed;
		uint8_t rxFullInt_dmaTx_en : 1 __packed;
		uint8_t txCompleteIntEn : 1 __packed;
		uint8_t txInt_DmaTx_en : 1 __packed;
	} field;
} C2_t; /* 0x003 Control 2 */

typedef union {
	uint8_t value;
	struct {
		uint8_t parityErrIntEn : 1 __packed;
		uint8_t frameErrIntEn : 1 __packed;
		uint8_t noiseErrIntEn : 1 __packed;
		uint8_t overrunErrIntEn : 1 __packed;
		uint8_t txDataInvert : 1 __packed;
		uint8_t txDataPinOuttDir : 1 __packed;
		uint8_t txBit8 : 1 __packed;
		uint8_t rxBit8 : 1 __packed;
	} field;
} C3_t; /* 0x006 Control 3 */

typedef union {
	uint8_t value;
	struct {
		uint8_t brfa : 5 __packed; /* BaudRateFineAdjust*/
		uint8_t mode10Bit : 1 __packed;
		uint8_t matechAddrMode1En : 1 __packed;
		uint8_t matchAddrMode2En : 1 __packed;
	} field;
} C4_t; /* 0x00A Control 4 */

#define TX_DATA_EMPTY_MASK 0x80
#define RX_DATA_FULL_MASK 0x20

typedef union {
	uint8_t value;
	struct {
		uint8_t parityErr : 1 __packed;
		uint8_t framingErr : 1 __packed;
		uint8_t noice : 1 __packed;
		uint8_t rxOverrun : 1 __packed;
		uint8_t idleLine : 1 __packed;
		uint8_t rxDataFull : 1 __packed;
		uint8_t txComplete : 1 __packed;
		uint8_t txDataEmpty : 1 __packed;
	} field;
} S1_t; /* 0x004 Status 1 */

typedef union {
	uint8_t value;
	struct e {
		uint8_t rxActive : 1 __packed;
		uint8_t linBkDetectEn : 1 __packed;
		uint8_t brkCharLen13 : 1 __packed;
		uint8_t rxWakeupIdleDetect : 1 __packed;
		uint8_t rxDataInverted : 1 __packed;
		uint8_t msbFirst : 1 __packed;
		uint8_t rxedgif : 1 __packed;
		uint8_t lbkdif : 1 __packed;
	} field;
} S2_t; /* 0x005 Status 2 */

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

typedef union {
	uint8_t value;
	struct {
		uint8_t rxFifoSize : 3 __packed; /* read-only */
		uint8_t rxFifoEn : 1 __packed;
		uint8_t txFifoSize : 3 __packed; /* read-only */
		uint8_t txFifoEn : 1 __packed;
	} field;
} PFIFO_t; /* 0x010 Fifo Parameter 1 */

#define RX_FIFO_FLUSH_MASK 0x40
#define TX_FIFO_FLUSH_MASK 0x80

typedef union {
	uint8_t value;
	struct {
		uint8_t rxFifoUnderflowIntEn : 1 __packed;
		uint8_t txFifoOverflowIntEn : 1 __packed;
		uint8_t rxFifoOverflowIntEn : 1 __packed;
		uint8_t res_3 : 3 __packed;
		uint8_t rxFifoFlush : 1 __packed; /* write-only */
		uint8_t txFifoFlush : 1 __packed; /* write-only */
	} field;
} CFIFO_t; /* 0x011 Fifo Control */

typedef volatile struct {
	BDH_t bdh;			 /* 0x000 Baud Rate High */
	uint8_t bdl;			 /* 0x001 Baud Rate Low (04)*/
	C1_t c1;			 /* 0x002 Control 1 */
	C2_t c2;			 /* 0x003 Control 2 */
	S1_t s1;			 /* 0x004 Status  1 (C0) RO*/
	S2_t s2;			 /* 0x005 Status  2 */
	C3_t c3;			 /* 0x006 Control 3 */
	uint8_t d;			 /* 0x007 Data */
	uint8_t ma1;			 /* 0x008 Match Address 1 */
	uint8_t ma2;			 /* 0x009 Match Address 1 */
	C4_t c4;			 /* 0x00A Control 4 */
	uint8_t c5;			 /* 0x00B Control 5 */
	uint8_t ed;			 /* 0x00C Extended Data */
	uint8_t modem;			 /* 0x00D Modem */
	uint8_t ir;			 /* 0x00E Infrared */
	uint8_t zReserved00f;		 /* 0x00F */
	PFIFO_t pfifo;			 /* 0x010 FIFO Param */
	CFIFO_t cfifo;			 /* 0x011 FIFO Control */
	uint8_t sfifo;			 /* 0x012 FIFO Status (C0)*/
	uint8_t twfifo;			 /* 0x013 FIFO Tx Watermark */
	uint8_t tcfifo;			 /* 0x014 FIFO Tx Count */
	uint8_t rwfifo;			 /* 0x015 FIFO Rx Watermark (01)*/
	uint8_t rcfifo;			 /* 0x016 FIFO Rx Count */
	uint8_t u_7816[0x20 - 0x17];     /* 0x017-0x1F UART ISO-7816 standard */
	uint8_t u_cea709_1[0x32 - 0x20]; /* 0x020-0x31 UART CEA8709.1 standard
					    */
	uint8_t zReservStatused038_03c[0x1000 - 0x32]; /* 0x032-0xFFF Reserved
							  */
} K20_UART_t; /* K20 Microntroller UART module */

static ALWAYS_INLINE void _k20UartBaudRateSet(K20_UART_t *uart_p,
					      uint32_t clkFreq,
					      uint32_t baudRate)
{
	/*
	 * The baud rate is calculated as:
	 * baudRate = clkFreq/(16*(SBR[12:0]+BRFA[5:0]/32)), where
	 * - SBR is the combined UART Baud Rate Register settings and
	 * - BRFA is the UART Baud Rate Fine Adjustment setting
	 * This is equivalent to:
	 * 32xSBR + BRFA = 2 * clkFreq/baudRate
	 */
	uint32_t clkBr = 2 * clkFreq / baudRate;
	uint16_t sbr = clkBr >> 5;
	uint8_t brfa = clkBr - (sbr << 5);

	__ASSERT((sbr && 0x1FFF), "clkFreq is too high or baudRate is too low");

	/* Note there are other fields (interrupts flag) in BDH register */
	uart_p->bdh.field.sbr = (uint8_t)(sbr >> 8);
	uart_p->bdl = (uint8_t)(sbr & 0xFF);
	uart_p->c4.field.brfa = brfa;
}

static inline void _k20UartFifoEnable(K20_UART_t *uart_p)
{
	uint8_t tx_rx_state = uart_p->c2.value && (TX_EN_MASK | RX_EN_MASK);

	/* disable Rx and Tx */
	uart_p->c2.value &= !(TX_EN_MASK | RX_EN_MASK);

	uart_p->pfifo.value |= (TX_FIFO_EN_MASK | RX_FIFO_EN_MASK);

	uart_p->cfifo.value |= (TX_FIFO_FLUSH_MASK | RX_FIFO_FLUSH_MASK);

	/* restore Rx and Tx */
	uart_p->c2.value |= tx_rx_state;
}

#endif /* _K20UART_H_ */
