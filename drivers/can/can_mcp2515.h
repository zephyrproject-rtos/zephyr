/*
 * Copyright (c) 2018 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _MCP2515_H_
#define _MCP2515_H_

#include <drivers/can.h>

#define MCP2515_RX_CNT                   2
#define MCP2515_TX_CNT                   3
#define MCP2515_FRAME_LEN               13

#define DEV_CFG(dev) \
	((const struct mcp2515_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct mcp2515_data *const)(dev)->data)

struct mcp2515_tx_cb {
	struct k_sem sem;
	can_tx_callback_t cb;
	void *cb_arg;
};

struct mcp2515_data {
	/* spi device data */
	const struct device *spi;
	struct spi_config spi_cfg;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control spi_cs_ctrl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	/* interrupt data */
	const struct device *int_gpio;
	struct gpio_callback int_gpio_cb;
	struct k_thread int_thread;
	k_thread_stack_t *int_thread_stack;
	struct k_sem int_sem;

	/* tx data */
	struct k_sem tx_sem;
	struct mcp2515_tx_cb tx_cb[MCP2515_TX_CNT];
	uint8_t tx_busy_map;

	/* filter data */
	uint32_t filter_usage;
	can_rx_callback_t rx_cb[CONFIG_CAN_MCP2515_MAX_FILTER];
	void *cb_arg[CONFIG_CAN_MCP2515_MAX_FILTER];
	struct zcan_filter filter[CONFIG_CAN_MCP2515_MAX_FILTER];
	can_state_change_isr_t state_change_isr;

	/* general data */
	struct k_mutex mutex;
	enum can_state old_state;
};

struct mcp2515_config {
	/* spi configuration */
	const char *spi_port;
	uint8_t spi_cs_pin;
	uint8_t spi_cs_flags;
	const char *spi_cs_port;
	uint32_t spi_freq;
	uint8_t spi_slave;

	/* interrupt configuration */
	uint8_t int_pin;
	const char *int_port;
	size_t int_thread_stack_size;
	int int_thread_priority;

	/* CAN timing */
	uint8_t tq_sjw;
	uint8_t tq_prop;
	uint8_t tq_bs1;
	uint8_t tq_bs2;
	uint32_t bus_speed;
	uint32_t osc_freq;
};

/* MCP2515 Opcodes */
#define MCP2515_OPCODE_WRITE            0x02
#define MCP2515_OPCODE_READ             0x03
#define MCP2515_OPCODE_BIT_MODIFY       0x05
#define MCP2515_OPCODE_LOAD_TX_BUFFER   0x40
#define MCP2515_OPCODE_RTS              0x80
#define MCP2515_OPCODE_READ_RX_BUFFER   0x90
#define MCP2515_OPCODE_READ_STATUS      0xA0
#define MCP2515_OPCODE_RESET            0xC0

/* MCP2515 Registers */
#define MCP2515_ADDR_CANSTAT            0x0E
#define MCP2515_ADDR_CANCTRL            0x0F
#define MCP2515_ADDR_TEC                0x1C
#define MCP2515_ADDR_REC                0x1D
#define MCP2515_ADDR_CNF3               0x28
#define MCP2515_ADDR_CNF2               0x29
#define MCP2515_ADDR_CNF1               0x2A
#define MCP2515_ADDR_CANINTE            0x2B
#define MCP2515_ADDR_CANINTF            0x2C
#define MCP2515_ADDR_EFLG               0x2D
#define MCP2515_ADDR_TXB0CTRL           0x30
#define MCP2515_ADDR_TXB1CTRL           0x40
#define MCP2515_ADDR_TXB2CTRL           0x50
#define MCP2515_ADDR_RXB0CTRL           0x60
#define MCP2515_ADDR_RXB1CTRL           0x70

#define MCP2515_ADDR_OFFSET_FRAME2FRAME	0x10
#define MCP2515_ADDR_OFFSET_CTRL2FRAME	0x01

/* MCP2515 Operation Modes */
#define MCP2515_MODE_NORMAL             0x00
#define MCP2515_MODE_LOOPBACK           0x02
#define MCP2515_MODE_SILENT             0x03
#define MCP2515_MODE_CONFIGURATION      0x04

/* MCP2515_FRAME_OFFSET */
#define MCP2515_FRAME_OFFSET_SIDH       0
#define MCP2515_FRAME_OFFSET_SIDL       1
#define MCP2515_FRAME_OFFSET_EID8       2
#define MCP2515_FRAME_OFFSET_EID0       3
#define MCP2515_FRAME_OFFSET_DLC        4
#define MCP2515_FRAME_OFFSET_D0         5

/* MCP2515_CANINTF */
#define MCP2515_CANINTF_RX0IF           BIT(0)
#define MCP2515_CANINTF_RX1IF           BIT(1)
#define MCP2515_CANINTF_TX0IF           BIT(2)
#define MCP2515_CANINTF_TX1IF           BIT(3)
#define MCP2515_CANINTF_TX2IF           BIT(4)
#define MCP2515_CANINTF_ERRIF           BIT(5)
#define MCP2515_CANINTF_WAKIF           BIT(6)
#define MCP2515_CANINTF_MERRF           BIT(7)

#define MCP2515_INTE_RX0IE              BIT(0)
#define MCP2515_INTE_RX1IE              BIT(1)
#define MCP2515_INTE_TX0IE              BIT(2)
#define MCP2515_INTE_TX1IE              BIT(3)
#define MCP2515_INTE_TX2IE              BIT(4)
#define MCP2515_INTE_ERRIE              BIT(5)
#define MCP2515_INTE_WAKIE              BIT(6)
#define MCP2515_INTE_MERRE              BIT(7)

#define MCP2515_EFLG_EWARN              BIT(0)
#define MCP2515_EFLG_RXWAR              BIT(1)
#define MCP2515_EFLG_TXWAR              BIT(2)
#define MCP2515_EFLG_RXEP               BIT(3)
#define MCP2515_EFLG_TXEP               BIT(4)
#define MCP2515_EFLG_TXBO               BIT(5)
#define MCP2515_EFLG_RX0OVR             BIT(6)
#define MCP2515_EFLG_RX1OVR             BIT(7)

#define MCP2515_TXCTRL_TXREQ			BIT(3)

#define MCP2515_CANSTAT_MODE_POS		5
#define MCP2515_CANSTAT_MODE_MASK		(0x07 << MCP2515_CANSTAT_MODE_POS)
#define MCP2515_CANCTRL_MODE_POS		5
#define MCP2515_CANCTRL_MODE_MASK		(0x07 << MCP2515_CANCTRL_MODE_POS)

#endif /*_MCP2515_H_*/
