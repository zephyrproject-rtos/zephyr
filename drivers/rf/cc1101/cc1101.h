/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RF_CC1101_H_
#define ZEPHYR_DRIVERS_RF_CC1101_H_

#include "cc1101_registers.h"

/* R/W Bits */
#define CC1101_SPI_WRITE	0x00
#define CC1101_SPI_BURST_WRITE	0x40
#define CC1101_SPI_READ		0x80
#define CC1101_SPI_BURST_READ	0xC0

#define CC1101_FIFO_SIZE	64U
#define CC1101_FIFO_HWM		32U
#define CC1101_IOCFG_CHIP_RDY	0x29
#define CC1101_IOCFG_RX		0x00
#define CC1101_IOCFG_TX		0x02
#define CC1101_IOCFG_XFER	0x06
#define CC1101_IOCFG_HIZ	0x2E

#define CC1101_NUM_CFG_REG	47U
#define CC1101_NUM_PATABLE	8U

#define CC1101_ALLOWED_PKT_LEN_MODE	0x01
#define CC1101_ALLOWED_PKT_FORMAT	0x00

union cc1101_data_item {
        uint8_t buffer[CONFIG_CC1101_MAX_PACKET_SIZE];
        struct {
                uint8_t length;
                uint8_t payload[CONFIG_CC1101_MAX_PACKET_SIZE - 1];
        } __packed;
};

char __aligned(4) rxq_buffer[CONFIG_CC1101_RX_MSG_QUEUE_DEPTH * sizeof(union cc1101_data_item)];

union CC1101_CHIP_STATUS {
	uint8_t reg;
	struct {
		uint8_t FIFO_BYTES_AVAILABLE: 4;
		uint8_t STATE: 3;
		uint8_t CHIP_RDY: 1;
	} __packed;
};

union CC1101_CONFIG_REGISTERS {
	char all[CC1101_NUM_CFG_REG];
	struct {
		union CC1101_IOCFG2 iocfg2;
		union CC1101_IOCFG1 iocfg1;
		union CC1101_IOCFG0 iocfg0;
		union CC1101_FIFOTHR fifothr;
		uint8_t sync1;
		uint8_t sync0;
		uint8_t pktlen;
		union CC1101_PKTCTRL1 pktctrl1;
		union CC1101_PKTCTRL0 pktctrl0;
		uint8_t addr;
		uint8_t channr;
		union CC1101_FSCTRL1 fsctrl1;
		uint8_t fsctrl0;
		union CC1101_FREQ2 freq2;
		uint8_t freq1;
		uint8_t freq0;
		union CC1101_MDMCFG4 mdmcfg4;
		uint8_t mdmcfg3;
		union CC1101_MDMCFG2 mdmcfg2;
		union CC1101_MDMCFG1 mdmcfg1;
		uint8_t mdmcfg0;
		union CC1101_DEVIATN deviatn;
		union CC1101_MCSM2 mcsm2;
		union CC1101_MCSM1 mcsm1;
		union CC1101_MCSM0 mcsm0;
		union CC1101_FOCCFG foccfg;
		union CC1101_BSCFG bscfg;
		union CC1101_AGCCTRL2 agcctrl2;
		union CC1101_AGCCTRL1 agcctrl1;
		union CC1101_AGCCTRL0 agcctrl0;
		uint8_t worevt1;
		uint8_t worevt0;
		union CC1101_WORCTRL worctrl;
		union CC1101_FREND1 frend1;
		union CC1101_FREND0 frend0;
		union CC1101_FSCAL3 fscal3;
		union CC1101_FSCAL2 fscal2;
		union CC1101_FSCAL1 fscal1;
		union CC1101_FSCAL0 fscal0;
		union CC1101_RCCTRL1 rcctrl1;
		union CC1101_RCCTRL0 rcctrl0;
		uint8_t fstest;
		uint8_t ptest;
		uint8_t agctest;
		uint8_t test2;
		uint8_t test1;
		uint8_t test0;
	} __packed;
};

struct cc1101_config {
	const struct spi_dt_spec spi;
	const uint8_t address_filter;
	const bool crc_autoflush;
	const bool append_status;
};

struct cc1101_data {
	/* Device parameters */
	rf_event_cb_t event_cb;
	enum rf_op_mode initial_mode;
	union CC1101_CONFIG_REGISTERS config;
	char patable[CC1101_NUM_PATABLE];
	/* GPIOs */
	const struct gpio_dt_spec gdo0;
	const struct gpio_dt_spec gdo2;
	struct gpio_callback gdo0_cb;
	struct gpio_callback gdo2_cb;
	/* Flags/Locks */
	struct k_mutex xfer_lock;
	struct k_sem rx_data_available;
	struct k_sem tx_done;
	struct k_sem fifo_cont;
	/* Device state */
	union CC1101_RXBYTES rxbytes;
	union CC1101_CHIP_STATUS status;
	/* RX */
	struct k_msgq rxq;
	K_KERNEL_STACK_MEMBER(rx_stack,
		(CONFIG_CC1101_RX_MSG_QUEUE_DEPTH * sizeof(union cc1101_data_item)) +
		CONFIG_CC1101_ADDITIONAL_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

enum cc1101_state {
	CC1101_STATE_IDLE,
	CC1101_STATE_RX,
	CC1101_STATE_TX,
	CC1101_STATE_FSTXON,
	CC1101_STATE_CALIBRATE,
	CC1101_STATE_SETTLING,
	CC1101_STATE_RXFIFO_OVERFLOW,
	CC1101_STATE_TXFIFO_UNDERFLOW,
};

static const char *const status2str[] = {
	[CC1101_STATE_IDLE]		= "Idle mode",
	[CC1101_STATE_RX]		= "Receive mode",
	[CC1101_STATE_TX]		= "Transmit mode",
	[CC1101_STATE_FSTXON]		= "Fast TX ready",
	[CC1101_STATE_CALIBRATE]	= "Calibrating",
	[CC1101_STATE_SETTLING]		= "Settling",
	[CC1101_STATE_RXFIFO_OVERFLOW]	= "RX FIFO overflow",
	[CC1101_STATE_TXFIFO_UNDERFLOW]	= "TX FIFO overflow",
};

static const inline char *cc1101_status2str(unsigned int status)
{
	if (status < ARRAY_SIZE(status2str)) {
		return status2str[status];
	} else {
		return "Unknown status";
	}
}

#endif /* ZEPHYR_DRIVERS_RF_CC1101_H_ */
