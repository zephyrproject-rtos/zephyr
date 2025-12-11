/*
 * Copyright 2022 Meta Platforms, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_CDNS_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_CDNS_H_

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

enum csr_parity_val {
	EVEN_PARITY_VAL,
	ODD_PARITY_VAL,
	SPACE_PARITY_VAL,
	MARK_PARITY_VAL,
	NO_PARITY_VAL
};

/* @brief  Control(CTRL) Registers offset 0x00 */
#define CTRL_STPBRK_MASK  (1 << 8)
#define CTRL_STPBRK_SHIFT (8)
#define CTRL_STTBRK_MASK  (1 << 7)
#define CTRL_STTBRK_SHIFT (7)
#define CTRL_RSTTO_MASK	  (1 << 6)
#define CTRL_RSTTO_SHIFT  (6)
#define CTRL_TXDIS_MASK	  (1 << 5)
#define CTRL_TXDIS_SHIFT  (5)
#define CTRL_TXEN_MASK	  (1 << 4)
#define CTRL_TXEN_SHIFT	  (4)
#define CTRL_RXDIS_MASK	  (1 << 3)
#define CTRL_RXDIS_SHIFT  (3)
#define CTRL_RXEN_MASK	  (1 << 2)
#define CTRL_RXEN_SHIFT	  (2)
#define CTRL_TXRES_MASK	  (1 << 1)
#define CTRL_TXRES_SHIFT  (1)
#define CTRL_RXRES_MASK	  (1 << 0)
#define CTRL_RXRES_SHIFT  (0)

/* @brief  Mode Registers offset 0x04 */
#define MODE_WSIZE_MASK	  (0x3 << 12)
#define MODE_WSIZE_SHIFT  (12)
#define MODE_WSIZE_SIZE	  (2)
#define MODE_IRMODE_MASK  (1 << 11)
#define MODE_IRMODE_SHIFT (11)
#define MODE_UCLKEN_MASK  (1 << 10)
#define MODE_UCLKEN_SHIFT (10)
#define MODE_CHMOD_MASK	  (0x3 << 8)
#define MODE_CHMOD_SHIFT  (8)
#define MODE_CHMOD_SIZE	  (2)
#define MODE_NBSTOP_MASK  (0x3 << 6)
#define MODE_NBSTOP_SHIFT (6)
#define MODE_NBSTOP_SIZE  (2)
#define MODE_PAR_MASK	  (0x7 << 3)
#define MODE_PAR_SHIFT	  (3)
#define MODE_PAR_SIZE	  (3)
#define MODE_CHRL_MASK	  (0x3 << 1)
#define MODE_CHRL_SHIFT	  (2)
#define MODE_CHRL_SIZE	  (2)
#define MODE_CLKS_MASK	  (1 << 0)
#define MODE_CLKS_SHIFT	  (0)

/* @brief  IER, IDR, IMR and CSIR Registers offset 0x08, 0xC, 0x10 and 0x14 */
#define CSR_RBRK_MASK	 (1 << 13)
#define CSR_RBRK_SHIFT	 (13)
#define CSR_TOVR_MASK	 (1 << 12)
#define CSR_TOVR_SHIFT	 (12)
#define CSR_TNFUL_MASK	 (1 << 11)
#define CSR_TNFUL_SHIFT	 (11)
#define CSR_TTRIG_MASK	 (1 << 10)
#define CSR_TTRIG_SHIFT	 (10)
#define CSR_DMSI_MASK	 (1 << 9)
#define CSR_DMSI_SHIFT	 (9)
#define CSR_TOUT_MASK	 (1 << 8)
#define CSR_TOUT_SHIFT	 (8)
#define CSR_PARE_MASK	 (1 << 7)
#define CSR_PARE_SHIFT	 (7)
#define CSR_FRAME_MASK	 (1 << 6)
#define CSR_FRAME_SHIFT	 (6)
#define CSR_ROVR_MASK	 (1 << 5)
#define CSR_ROVR_SHIFT	 (5)
#define CSR_TFUL_MASK	 (1 << 4)
#define CSR_TFUL_SHIFT	 (4)
#define CSR_TEMPTY_MASK	 (1 << 3)
#define CSR_TEMPTY_SHIFT (3)
#define CSR_RFUL_MASK	 (1 << 2)
#define CSR_RFUL_SHIFT	 (2)
#define CSR_REMPTY_MASK	 (1 << 1)
#define CSR_REMPTY_SHIFT (1)
#define CSR_RTRIG_MASK	 (1 << 0)
#define CSR_RTRIG_SHIFT	 (0)

#define RXDATA_MASK   0xFF /* Receive Data Mask */
#define MAX_FIFO_SIZE (64)

#define DEFAULT_RTO_PERIODS_FACTOR 8

#define SET_VAL32(name, val) (((uint32_t)(val) << name##_SHIFT) & name##_MASK)

#define CDNS_PARTITY_MAP(parity)								   \
	(parity == UART_CFG_PARITY_NONE)    ? NO_PARITY_VAL					   \
	: (parity == UART_CFG_PARITY_ODD)   ? ODD_PARITY_VAL					   \
	: (parity == UART_CFG_PARITY_MARK)  ? MARK_PARITY_VAL					   \
	: (parity == UART_CFG_PARITY_SPACE) ? SPACE_PARITY_VAL					   \
					    : EVEN_PARITY_VAL
struct uart_cdns_regs {
	volatile uint32_t ctrl;			 /* Control Register */
	volatile uint32_t mode;			 /* Mode Register */
	volatile uint32_t intr_enable;		 /* Interrupt Enable Register */
	volatile uint32_t intr_disable;		 /* Interrupt Disable Register */
	volatile uint32_t intr_mask;		 /* Interrupt Mask Register */
	volatile uint32_t channel_intr_status;	 /* Channel Interrupt Status Register */
	volatile uint32_t baud_rate_gen;	 /* Baud Rate Generator Register */
	volatile uint32_t rx_timeout;		 /* Receiver Timeout Register */
	volatile uint32_t rx_fifo_trigger_level; /* Receiver FIFO Trigger Level Register */
	volatile uint32_t modem_control;	 /* Modem Control Register */
	volatile uint32_t modem_status;		 /* Modem Status Register */
	volatile uint32_t channel_status;	 /* Channel status */
	volatile uint32_t rx_tx_fifo;		 /* RX TX FIFO Register */
	volatile uint32_t baud_rate_div;	 /* Baud Rate Divider Register */
	volatile uint32_t flow_ctrl_delay;	 /* Flow Control Delay Register */
	volatile uint32_t rpwr;			 /* IR Minimum Received Pulse Register */
	volatile uint32_t tpwr;			 /* IR TRansmitted Pulse Width Register */
	volatile uint32_t tx_fifo_trigger_level; /* Transmitter FIFO trigger level */
	volatile uint32_t rbrs;			 /* RX FIFO Byte Status Register */
};

struct uart_cdns_device_config {
	uint32_t port;
	uint32_t bdiv;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
	uint8_t parity;
	void (*cfg_func)(void);
};

struct uart_cdns_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_CDNS_H_ */
