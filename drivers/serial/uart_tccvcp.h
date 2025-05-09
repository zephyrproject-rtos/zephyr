/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_TCCVCP_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_TCCVCP_H_

#define UART_POLLING_MODE 0U
#define UART_INTR_MODE    1U
#define UART_DMA_MODE     2U

#define UART_CTSRTS_ON  1U
#define UART_CTSRTS_OFF 0U

#define ENABLE_FIFO  1U
#define DISABLE_FIFO 0U

#define TWO_STOP_BIT_ON  1U
#define TWO_STOP_BIT_OFF 0U

/* UART Channels */
#define UART_CH0    0U
#define UART_CH1    1U
#define UART_CH2    2U
#define UART_CH3    3U
#define UART_CH4    4U
#define UART_CH5    5U
#define UART_CH_MAX 6U

#define UART_DEBUG_CLK 48000000UL /* 48MHz */

/* UART Base address */
#define UART_GET_BASE(n) (MCU_BSP_UART_BASE + (0x10000UL * (n)))

/* UART Register (BASE Address + Offset) */
#define UART_REG_DR    0x00U /* Data register */
#define UART_REG_RSR   0x04U /* Receive Status register */
#define UART_REG_ECR   0x04U /* Error Clear register */
#define UART_REG_FR    0x18U /* Flag register */
#define UART_REG_IBRD  0x24U /* Integer Baud rate register */
#define UART_REG_FBRD  0x28U /* Fractional Baud rate register */
#define UART_REG_LCRH  0x2cU /* Line Control register */
#define UART_REG_CR    0x30U /* Control register */
#define UART_REG_IFLS  0x34U /* Interrupt FIFO Level status register */
#define UART_REG_IMSC  0x38U /* Interrupt Mask Set/Clear register */
#define UART_REG_RIS   0x3cU /* Raw Interrupt Status register */
#define UART_REG_MIS   0x40U /* Masked Interrupt Status register */
#define UART_REG_ICR   0x44U /* Interrupt Clear register */
#define UART_REG_DMACR 0x48U /* DMA Control register */

/* UART Flag Register(FR) Fields */
#define UART_FR_TXFE (1UL << 7U) /* Transmit FIFO empty */
#define UART_FR_RXFF (1UL << 6U) /* Receive FIFO full */
#define UART_FR_TXFF (1UL << 5U) /* Transmit FIFO full */
#define UART_FR_RXFE (1UL << 4U) /* Receive FIFO empty */
#define UART_FR_BUSY (1UL << 3U) /* UART busy */
#define UART_FR_CTS  (1UL << 0U) /* Clear to send */

/* UART Line Control Register (LCR_H) Fields */
#define UART_LCRH_SPS     (1UL << 7U) /* Stick parity select */
#define UART_LCRH_WLEN(x) ((x) << 5U) /* Word length */
#define UART_LCRH_FEN     (1UL << 4U) /* Enable FIFOs */
#define UART_LCRH_STP2    (1UL << 3U) /* Two stop bits select */
#define UART_LCRH_EPS     (1UL << 2U) /* Even parity select */
#define UART_LCRH_PEN     (1UL << 1U) /* Parity enable */
#define UART_LCRH_BRK     (1UL << 0U) /* Send break */

/* UART Control Register (CR) Fields */
#define UART_CR_CTSEN (1UL << 15U) /* CTS hardware flow control enable */
#define UART_CR_RTSEN (1UL << 14U) /* RTS hardware flow control enable */
#define UART_CR_RTS   (1UL << 11U) /* Request to send */
#define UART_CR_RXE   (1UL << 9U)  /* Receive enable */
#define UART_CR_TXE   (1UL << 8U)  /* Transmit enable */
#define UART_CR_LBE   (1UL << 7U)  /* Loopback enable */
#define UART_CR_EN    (1UL << 0U)  /* UART enable */

#define UART_TX_FIFO_SIZE 8UL
#define UART_RX_FIFO_SIZE 12UL

#define UART_INT_OEIS (1UL << 10U) /* Overrun error interrupt */
#define UART_INT_BEIS (1UL << 9U)  /* Break error interrupt */
#define UART_INT_PEIS (1UL << 8U)  /* Parity error interrupt */
#define UART_INT_FEIS (1UL << 7U)  /* Framing error interrupt */
#define UART_INT_RTIS (1UL << 6U)  /* Receive timeout interrupt */
#define UART_INT_TXIS (1UL << 5U)  /* Transmit interrupt */
#define UART_INT_RXIS (1UL << 4U)  /* Receive interrupt */

/* UART Settings */
#define UART_BUFF_SIZE 0x100UL /* 256 */

#define UART_MODE_TX 0UL
#define UART_MODE_RX 1UL

#define UART_PORT_CFG_MAX  16U
#define UART_PORT_TBL_SIZE UART_PORT_CFG_MAX

/* DMA Control Register (DMACR) Fields */
#define UART_DMACR_DMAONERR (1UL << 2U) /* DMA on error */
#define UART_DMACR_TXDMAE   (1UL << 1U) /* Transmit DMA enable */
#define UART_DMACR_RXDMAE   (1UL << 0U) /* Receive DMA enable */

#define UART_BASE_ADDR DT_INST_REG_ADDR(0)

#define TCC_GPNONE 0xFFFFUL

enum uart_word_len {
	WORD_LEN_5 = 0,
	WORD_LEN_6,
	WORD_LEN_7,
	WORD_LEN_8
};

enum uart_parity {
	PARITY_SPACE = 0,
	PARITY_EVEN,
	PARITY_ODD,
	PARITY_MARK
};

struct uart_board_port {
	uint32_t bd_port_cfg; /* Config port ID */
	uint32_t bd_port_tx;  /* UT_TXD GPIO */
	uint32_t bd_port_rx;  /* UT_RXD GPIO */
	uint32_t bd_port_rts; /* UT_RTS GPIO */
	uint32_t bd_port_cts; /* UT_CTS GPIO */
	uint32_t bd_port_fs;  /* UART function select */
	uint32_t bd_port_ch;  /* Channel */
};

struct uart_interrupt_data {
	int8_t *irq_data_xmit_buf;
	int32_t irq_data_head;
	int32_t irq_data_tail;
	int32_t irq_data_size;
};

struct uart_param {
	uint8_t channel;
	uint32_t priority;              /* Interrupt priority */
	uint32_t baud_rate;             /* Baudrate */
	uint8_t mode;                   /* polling or interrupt */
	uint8_t cts_rts;                /* on/off */
	uint8_t port_cfg;               /* port selection */
	uint8_t fifo;                   /* on/off */
	uint8_t stop_bit;               /* on/off */
	enum uart_word_len word_length; /* 5~8 bits */
	enum uart_parity parity;        /* space, even, odd, mark */
	tic_isr_func callback_fn;       /* callback function */
};

struct uart_status {
	unsigned char status_is_probed;
	uint32_t status_base;                      /* UART Controller base address */
	uint8_t status_chan;                       /* UART Channel */
	uint8_t status_op_mode;                    /* Operation Mode */
	uint8_t status_cts_rts;                    /* CTS and RTS */
	uint8_t status_2stop_bit;                  /* 1: two stop bits are transmitted */
	uint32_t baudrate;                         /* Baudrate setting in bps */
	enum uart_parity status_parity;            /* 0:disable, 1:enable */
	enum uart_word_len status_word_len;        /* Word Length */
	struct uart_board_port status_port;        /* GPIO Port Information */
	struct uart_interrupt_data status_rx_intr; /* Rx Interrupt */
	struct uart_interrupt_data status_tx_intr; /* Tx Interrupt */
};

/** Device configuration structure */
struct uart_tccvcp_dev_config {
	DEVICE_MMIO_ROM;
	uint8_t channel;
	uint32_t sys_clk_freq;
	struct uart_param uart_pars;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
	uint32_t baud_rate;
};

/** Device data structure */
struct uart_tccvcp_dev_data_t {
	DEVICE_MMIO_RAM;
	uint32_t parity;
	uint32_t stopbits;
	uint32_t databits;
	uint32_t flowctrl;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_TCCVCP_H_ */
