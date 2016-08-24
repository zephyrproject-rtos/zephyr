/* ieee802154_cc2520.c - IEEE 802.15.4 driver for TI CC2520 */

/*
 * Copyright (c) 2016 Intel Corporation.
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

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_TI_CC2520_LEVEL
#define SYS_LOG_NO_NEWLINE
#include <misc/sys_log.h>

#include <errno.h>

#include <nanokernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>

#include <misc/byteorder.h>
#include <string.h>
#include <rand32.h>

#include <gpio.h>

#include <net/l2_buf.h>
#include <packetbuf.h>

#include <dev/radio.h>
#include <net_driver_15_4.h>
static struct device *cc2520_sglt;

#include "ieee802154_cc2520.h"

/**
 * Content is split as follows:
 * 1 - Debug related functions
 * 2 - Generic helper functions (for any parts)
 * 3 - GPIO related functions
 * 4 - TX related helper functions
 * 5 - RX related helper functions
 * 6 - Radio device API functions
 * 7 - Legacy radio device API functions
 * 8 - Initialization
 */


#if defined(CONFIG_TI_CC2520_AUTO_CRC) &&  defined(CONFIG_TI_CC2520_AUTO_ACK)
#define CC2520_AUTOMATISM		(FRMCTRL0_AUTOCRC | FRMCTRL0_AUTOACK)
#elif defined(CONFIG_TI_CC2520_AUTO_CRC)
#define CC2520_AUTOMATISM		(FRMCTRL0_AUTOCRC)
#else
#define CC2520_AUTOMATISM		(0)
#endif

#define CC2520_TX_THRESHOLD		(0x7F)
#define CC2520_FCS_LENGTH		(2)

/*********
 * DEBUG *
 ********/
#ifndef CONFIG_TI_CC2520_DEBUG
#define _cc2520_print_gpio_config(...)
#define _cc2520_print_exceptions(...)
#define _cc2520_print_errors(...)
#else
static inline void _cc2520_print_gpio_config(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s: GPIOCTRL0/1/2/3/4/5 = 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
	    __func__,
	    read_reg_gpioctrl0(&cc2520->spi),
	    read_reg_gpioctrl1(&cc2520->spi),
	    read_reg_gpioctrl2(&cc2520->spi),
	    read_reg_gpioctrl3(&cc2520->spi),
	    read_reg_gpioctrl4(&cc2520->spi),
	    read_reg_gpioctrl5(&cc2520->spi));
	SYS_LOG_DBG("%s: GPIOPOLARITY: 0x%x\n",
	    __func__, read_reg_gpiopolarity(&cc2520->spi));
	SYS_LOG_DBG("%s: GPIOCTRL: 0x%x\n",
	    __func__, read_reg_gpioctrl(&cc2520->spi));
}

static inline void _cc2520_print_exceptions(struct cc2520_context *cc2520)
{
	uint8_t flag = read_reg_excflag0(&cc2520->spi);

	SYS_LOG_DBG("%s: EXCFLAG0: ", __func__);
	if (flag & EXCFLAG0_RF_IDLE) {
		SYS_LOG_DBG("RF_IDLE ");
	}
	if (flag & EXCFLAG0_TX_FRM_DONE) {
		SYS_LOG_DBG("TX_FRM_DONE ");
	}
	if (flag & EXCFLAG0_TX_ACK_DONE) {
		SYS_LOG_DBG("TX_ACK_DONE ");
	}
	if (flag & EXCFLAG0_TX_UNDERFLOW) {
		SYS_LOG_DBG("TX_UNDERFLOW ");
	}
	if (flag & EXCFLAG0_TX_OVERFLOW) {
		SYS_LOG_DBG("TX_OVERFLOW ");
	}
	if (flag & EXCFLAG0_RX_UNDERFLOW) {
		SYS_LOG_DBG("RX_UNDERFLOW ");
	}
	if (flag & EXCFLAG0_RX_OVERFLOW) {
		SYS_LOG_DBG("RX_OVERFLOW ");
	}
	if (flag & EXCFLAG0_RXENABLE_ZERO) {
		SYS_LOG_DBG("RXENABLE_ZERO");
	}
	SYS_LOG_DBG("\n");

	flag = read_reg_excflag1(&cc2520->spi);

	SYS_LOG_DBG("%s: EXCFLAG1: ", __func__);
	if (flag & EXCFLAG1_RX_FRM_DONE) {
		SYS_LOG_DBG("RX_FRM_DONE ");
	}
	if (flag & EXCFLAG1_RX_FRM_ACCEPTED) {
		SYS_LOG_DBG("RX_FRM_ACCEPTED ");
	}
	if (flag & EXCFLAG1_SRC_MATCH_DONE) {
		SYS_LOG_DBG("SRC_MATCH_DONE ");
	}
	if (flag & EXCFLAG1_SRC_MATCH_FOUND) {
		SYS_LOG_DBG("SRC_MATCH_FOUND ");
	}
	if (flag & EXCFLAG1_FIFOP) {
		SYS_LOG_DBG("FIFOP ");
	}
	if (flag & EXCFLAG1_SFD) {
		SYS_LOG_DBG("SFD ");
	}
	if (flag & EXCFLAG1_DPU_DONE_L) {
		SYS_LOG_DBG("DPU_DONE_L ");
	}
	if (flag & EXCFLAG1_DPU_DONE_H) {
		SYS_LOG_DBG("DPU_DONE_H");
	}
	SYS_LOG_DBG("\n");
}

static inline void _cc2520_print_errors(struct cc2520_context *cc2520)
{
	uint8_t flag = read_reg_excflag2(&cc2520->spi);

	SYS_LOG_DBG("EXCFLAG2: ");
	if (flag & EXCFLAG2_MEMADDR_ERROR) {
		SYS_LOG_DBG("MEMADDR_ERROR ");
	}
	if (flag & EXCFLAG2_USAGE_ERROR) {
		SYS_LOG_DBG("USAGE_ERROR ");
	}
	if (flag & EXCFLAG2_OPERAND_ERROR) {
		SYS_LOG_DBG("OPERAND_ERROR ");
	}
	if (flag & EXCFLAG2_SPI_ERROR) {
		SYS_LOG_DBG("SPI_ERROR ");
	}
	if (flag & EXCFLAG2_RF_NO_LOCK) {
		SYS_LOG_DBG("RF_NO_LOCK ");
	}
	if (flag & EXCFLAG2_RX_FRM_ABORTED) {
		SYS_LOG_DBG("RX_FRM_ABORTED ");
	}
	if (flag & EXCFLAG2_RFBUFMOV_TIMEOUT) {
		SYS_LOG_DBG("RFBUFMOV_TIMEOUT");
	}
	SYS_LOG_DBG("\n");
}
#endif


/*********************
 * Generic functions *
 ********************/
static void _usleep(uint32_t usec)
{
	static void (*func[3])(int32_t timeout_in_ticks) = {
		NULL,
		fiber_sleep,
		task_sleep,
	};

	if (sys_execution_context_type_get() == 0) {
		sys_thread_busy_wait(usec);
		return;
	}

	/* Timeout in ticks: */
	usec = USEC(usec);
	/** Most likely usec will generate 0 ticks,
	 * so setting at least to 1
	 */
	if (!usec) {
		usec = 1;
	}

	func[sys_execution_context_type_get()](usec);
}

uint8_t _cc2520_read_reg(struct cc2520_spi *spi,
			 bool freg, uint8_t addr)
{
	uint8_t len = freg ? 2 : 3;

	spi->cmd_buf[0] = freg ? CC2520_INS_REGRD | addr : CC2520_INS_MEMRD;
	spi->cmd_buf[1] = freg ? 0 : addr;
	spi->cmd_buf[2] = 0;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, len,
			   spi->cmd_buf, len) == 0) {
		return spi->cmd_buf[len - 1];
	}

	return 0;
}

bool _cc2520_write_reg(struct cc2520_spi *spi, bool freg,
		       uint8_t addr, uint8_t value)
{
	uint8_t len = freg ? 2 : 3;

	spi->cmd_buf[0] = freg ? CC2520_INS_REGWR | addr : CC2520_INS_MEMWR;
	spi->cmd_buf[1] = freg ? value : addr;
	spi->cmd_buf[2] = freg ? 0 : value;

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, len) == 0);
}

bool _cc2520_write_ram(struct cc2520_spi *spi, uint16_t addr,
		       uint8_t *data_buf, uint8_t len)
{
	spi->cmd_buf[0] = CC2520_INS_MEMWR | (addr >> 8);
	spi->cmd_buf[1] = addr;

	memcpy(&spi->cmd_buf[2], data_buf, len);

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, len + 2) == 0);
}

static uint8_t _cc2520_status(struct cc2520_spi *spi)
{
	spi->cmd_buf[0] = CC2520_INS_SNOP;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 1,
			   spi->cmd_buf, 1) == 0) {
		return spi->cmd_buf[0];
	}

	return 0;
}

static bool verify_osc_stabilization(struct cc2520_context *cc2520)
{
	uint8_t timeout = 100;
	uint8_t status;

	do {
		status = _cc2520_status(&cc2520->spi);
		_usleep(1);
		timeout--;
	} while (!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING) && timeout);

	return !!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING);
}


/******************
 * GPIO functions *
 *****************/
static inline void set_reset(struct device *dev, uint32_t value)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_pin_write(cc2520->gpios[CC2520_GPIO_IDX_RESET],
		       CONFIG_CC2520_GPIO_RESET, value);
}

static inline void set_vreg_en(struct device *dev, uint32_t value)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_pin_write(cc2520->gpios[CC2520_GPIO_IDX_VREG_EN],
		       CONFIG_CC2520_GPIO_VREG_EN, value);
}

static inline uint32_t get_fifo(struct cc2520_context *cc2520)
{
	uint32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_FIFO],
		      CONFIG_CC2520_GPIO_FIFO, &pin_value);

	return pin_value;
}

static inline uint32_t get_fifop(struct cc2520_context *cc2520)
{
	uint32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_FIFOP],
		      CONFIG_CC2520_GPIO_FIFOP, &pin_value);

	return pin_value;
}

static inline uint32_t get_cca(struct cc2520_context *cc2520)
{
	uint32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_CCA],
		      CONFIG_CC2520_GPIO_CCA, &pin_value);

	return pin_value;
}

static inline void sfd_int_handler(struct device *port,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, sfd_cb);

	if (atomic_get(&cc2520->tx) == 1) {
		atomic_set(&cc2520->tx, 0);
		device_sync_call_complete(&cc2520->tx_sync);
	}
}

static inline void fifop_int_handler(struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, fifop_cb);

	/* Note: Errata document - 1.2 */
	if (!get_fifop(cc2520) && !get_fifop(cc2520)) {
		return;
	}

	if (!get_fifo(cc2520)) {
		cc2520->overflow = true;
	}

	nano_isr_sem_give(&cc2520->rx_lock);
}

static void enable_fifop_interrupt(struct cc2520_context *cc2520,
				   bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(cc2520->gpios[CC2520_GPIO_IDX_FIFOP],
					 CONFIG_CC2520_GPIO_FIFOP);
	} else {
		gpio_pin_disable_callback(cc2520->gpios[CC2520_GPIO_IDX_FIFOP],
					  CONFIG_CC2520_GPIO_FIFOP);
	}
}

static void enable_sfd_interrupt(struct cc2520_context *cc2520,
				 bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(cc2520->gpios[CC2520_GPIO_IDX_SFD],
					 CONFIG_CC2520_GPIO_SFD);
	} else {
		gpio_pin_disable_callback(cc2520->gpios[CC2520_GPIO_IDX_SFD],
					  CONFIG_CC2520_GPIO_SFD);
	}
}

static inline void setup_gpio_callbacks(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_init_callback(&cc2520->sfd_cb,
			   sfd_int_handler, BIT(CONFIG_CC2520_GPIO_SFD));
	gpio_add_callback(cc2520->gpios[CC2520_GPIO_IDX_SFD],
			  &cc2520->sfd_cb);

	gpio_init_callback(&cc2520->fifop_cb,
			   fifop_int_handler, BIT(CONFIG_CC2520_GPIO_FIFOP));
	gpio_add_callback(cc2520->gpios[CC2520_GPIO_IDX_FIFOP],
			  &cc2520->fifop_cb);
}


/****************
 * TX functions *
 ***************/
static inline bool write_txfifo_length(struct cc2520_spi *spi,
				       struct net_buf *buf)
{
	spi->cmd_buf[0] = CC2520_INS_TXBUF;
	spi->cmd_buf[1] = packetbuf_totlen(buf) + CC2520_FCS_LENGTH;

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, 2) == 0);
}

static inline bool write_txfifo_content(struct cc2520_spi *spi,
					struct net_buf *buf)
{
	uint8_t cmd[128 + 1];

	cmd[0] = CC2520_INS_TXBUF;
	memcpy(&cmd[1], packetbuf_hdrptr(buf), packetbuf_totlen(buf));

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, cmd, packetbuf_totlen(buf) + 1) == 0);
}

static inline bool verify_txfifo_status(struct cc2520_context *cc2520,
					struct net_buf *buf)
{
	if (read_reg_txfifocnt(&cc2520->spi) < (packetbuf_totlen(buf) + 1) ||
	    (read_reg_excflag0(&cc2520->spi) & EXCFLAG0_TX_UNDERFLOW)) {
		return false;
	}

	return true;
}

static inline bool verify_tx_done(struct cc2520_context *cc2520)
{
	uint8_t timeout = 10;
	uint8_t status;

	do {
		_usleep(1);
		timeout--;
		status = read_reg_excflag0(&cc2520->spi);
	} while (!(status & EXCFLAG0_TX_FRM_DONE) && timeout);

	return !!(status & EXCFLAG0_TX_FRM_DONE);
}

static inline void enable_reception(struct cc2520_context *cc2520)
{
	/* Note: Errata document - 1.1 */
	enable_fifop_interrupt(cc2520, false);

	instruct_srxon(&cc2520->spi);
	instruct_sflushrx(&cc2520->spi);
	instruct_sflushrx(&cc2520->spi);

	enable_fifop_interrupt(cc2520, true);

	write_reg_excflag0(&cc2520->spi, EXCFLAG0_RESET_RX_FLAGS);
}

/****************
 * RX functions *
 ***************/

static inline void flush_rxfifo(struct cc2520_context *cc2520)
{
	/* Note: Errata document - 1.1 */
	enable_fifop_interrupt(cc2520, false);

	instruct_sflushrx(&cc2520->spi);
	instruct_sflushrx(&cc2520->spi);

	enable_fifop_interrupt(cc2520, true);

	write_reg_excflag0(&cc2520->spi, EXCFLAG0_RESET_RX_FLAGS);
}

#ifdef CONFIG_SPI_QMSI
/** This is a workaround, for SPI QMSI drivers as current QMSI API does not
 * support asymmetric tx/rx buffer lengths.
 * (i.e.: it's up to the user to handle tx dummy bytes in tx buffer)
 */
static inline uint8_t read_rxfifo_length(struct cc2520_spi *spi)
{
	spi->cmd_buf[0] = CC2520_INS_RXBUF;
	spi->cmd_buf[1] = 0;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 2,
			   spi->cmd_buf, 2) == 0) {
		return spi->cmd_buf[1];
	}

	return 0;
}

static inline bool read_rxfifo_content(struct cc2520_spi *spi,
				       struct net_buf *buf, uint8_t len)
{
	uint8_t data[128+1];

	data[0] = CC2520_INS_RXBUF;
	memset(&data[1], 0, len);

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, data, len+1, data, len+1) != 0) {
		return false;
	}

	if (read_reg_excflag0(spi) & EXCFLAG0_RX_UNDERFLOW) {
		return false;
	}

	memcpy(packetbuf_dataptr(buf), &data[1], len);
	packetbuf_set_datalen(buf, len);

	return true;
}

static inline bool read_rxfifo_footer(struct cc2520_spi *spi, uint8_t *buf)
{
	spi->cmd_buf[0] = CC2520_INS_RXBUF;
	memset(&spi->cmd_buf[1], 0, CC2520_FCS_LENGTH);

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, CC2520_FCS_LENGTH+1,
			   spi->cmd_buf, CC2520_FCS_LENGTH+1) != 0) {
		return false;
	}

	memcpy(buf, &spi->cmd_buf[1], CC2520_FCS_LENGTH);

	return true;
}
#else /* CONFIG_SPI_QMSI */
static inline uint8_t read_rxfifo_length(struct cc2520_spi *spi)
{
	spi->cmd_buf[0] = CC2520_INS_RXBUF;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 1,
			   spi->cmd_buf, 2) == 0) {
		return spi->cmd_buf[1];
	}

	return 0;
}

static inline bool read_rxfifo_content(struct cc2520_spi *spi,
				       struct net_buf *buf, uint8_t len)
{
	uint8_t data[128+1];

	spi->cmd_buf[0] = CC2520_INS_RXBUF;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 1, data, len+1) != 0) {
		return false;
	}

	if (read_reg_excflag0(spi) & EXCFLAG0_RX_UNDERFLOW) {
		return false;
	}

	memcpy(packetbuf_dataptr(buf), &data[1], len);
	packetbuf_set_datalen(buf, len);

	return true;
}

static inline bool read_rxfifo_footer(struct cc2520_spi *spi, uint8_t *buf)
{
	spi->cmd_buf[0] = CC2520_INS_RXBUF;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 1,
			   spi->cmd_buf, CC2520_FCS_LENGTH+1) != 0) {
		return false;
	}

	memcpy(buf, &spi->cmd_buf[1], CC2520_FCS_LENGTH);

	return true;
}
#endif /* CONFIG_SPI_QMSI */

static inline bool verify_rxfifo_validity(struct cc2520_spi *spi,
					  uint8_t pkt_len)
{
	if (pkt_len < 2 || read_reg_rxfifocnt(spi) != pkt_len) {
		return false;
	}

	return true;
}

static void cc2520_rx(int arg, int unused2)
{
	struct device *dev = INT_TO_POINTER(arg);
	struct cc2520_context *cc2520 = dev->driver_data;
	struct net_buf *pkt_buf = NULL;
	uint8_t pkt_len;
#ifdef CONFIG_TI_CC2520_AUTO_CRC
	uint8_t buf[CC2520_FCS_LENGTH];
#endif

	ARG_UNUSED(unused2);

	while (1) {
		nano_fiber_sem_take(&cc2520->rx_lock, TICKS_UNLIMITED);

		if (cc2520->overflow) {
			SYS_LOG_DBG("RX overflow!\n");
			cc2520->overflow = false;
			goto flush;
		}

		pkt_len = read_rxfifo_length(&cc2520->spi) & 0x7f;
		if (!verify_rxfifo_validity(&cc2520->spi, pkt_len)) {
			SYS_LOG_DBG("Invalid content\n");
			goto flush;
		}

		pkt_buf = l2_buf_get_reserve(0);
		if (!pkt_buf) {
			SYS_LOG_DBG("No pkt buf available\n");
			goto flush;
		}

		if (!read_rxfifo_content(&cc2520->spi, pkt_buf,
					 pkt_len - CC2520_FCS_LENGTH)) {
			SYS_LOG_DBG("No content read\n");
			goto error;
		}
#ifdef CONFIG_TI_CC2520_AUTO_CRC
		if (!read_rxfifo_footer(&cc2520->spi, buf)) {
			SYS_LOG_DBG("No footer read\n");
			goto error;
		}

		if (!(buf[1] & CC2520_FCS_CRC_OK)) {
			SYS_LOG_DBG("Bad packet CRC\n");
			goto error;
		}
#ifdef CONFIG_TI_CC2520_LINK_DETAILS
		packetbuf_set_attr(pkt_buf, PACKETBUF_ATTR_RSSI,
				   buf[0]);
		packetbuf_set_attr(pkt_buf, PACKETBUF_ATTR_LINK_QUALITY,
				   buf[1] & CC2520_FCS_CORRELATION);
#endif /* CONFIG_TI_CC2520_LINK_DETAILS */
#endif /* CONFIG_TI_CC2520_AUTO_CRC */

		SYS_LOG_DBG("Caught a packet (%u)\n", pkt_len - CC2520_FCS_LENGTH);

		if (net_driver_15_4_recv_from_hw(pkt_buf) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack\n");
			goto error;
		}

		net_analyze_stack("CC2520 Rx Fiber stack",
				  cc2520->cc2520_rx_stack,
				  CONFIG_CC2520_RX_STACK_SIZE);
		goto flush;
error:
		l2_buf_unref(pkt_buf);
flush:
		flush_rxfifo(cc2520);
	}
}


/********************
 * Radio device API *
 *******************/
static inline int cc2520_set_channel(struct device *dev, uint16_t channel)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s: %u\n", __func__, channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	/* See chapter 16 */
	channel = 11 + 5 * (channel - 11);

	if (!write_reg_freqctrl(&cc2520->spi, FREQCTRL_FREQ(channel))) {
		SYS_LOG_DBG("%s: FAILED\n", __func__);
		return -EIO;
	}

	return 0;
}

static inline int cc2520_set_pan_id(struct device *dev, uint16_t pan_id)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s: 0x%x\n", __func__, pan_id);

	pan_id = sys_le16_to_cpu(pan_id);

	if (!write_mem_pan_id(&cc2520->spi, (uint8_t *) &pan_id)) {
		SYS_LOG_DBG("%s: FAILED\n", __func__);
		return -EIO;
	}

	return 0;
}

static inline int cc2520_set_short_addr(struct device *dev, uint16_t short_addr)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s: 0x%x\n", __func__, short_addr);

	short_addr = sys_le16_to_cpu(short_addr);

	if (!write_mem_short_addr(&cc2520->spi, (uint8_t *) &short_addr)) {
		SYS_LOG_DBG("%s: FAILED\n", __func__);
		return -EIO;
	}

	return 0;
}

static inline int cc2520_set_ieee_addr(struct device *dev,
				       const uint8_t *ieee_addr)
{
	struct cc2520_context *cc2520 = dev->driver_data;
	uint8_t ext_addr[8];
	int idx;

	for (idx = 0; idx < 8; idx++) {
		ext_addr[idx] = ieee_addr[7 - idx];
	}

	if (!write_mem_ext_addr(&cc2520->spi, ext_addr)) {
		SYS_LOG_DBG("%s: FAILED\n", __func__);
		return -EIO;
	}

	SYS_LOG_DBG("%s: IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
	    __func__,
	    ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3],
	    ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]);

	return 0;
}

static inline int cc2520_tx(struct device *dev, struct net_buf *buf)
{
	struct cc2520_context *cc2520 = dev->driver_data;
	uint8_t retry = 2;
	bool status;

	SYS_LOG_DBG("%s: %p (%u)\n", __func__, buf, packetbuf_totlen(buf));

	if (!write_reg_excflag0(&cc2520->spi, EXCFLAG0_RESET_TX_FLAGS) ||
	    !write_txfifo_length(&cc2520->spi, buf) ||
	    !write_txfifo_content(&cc2520->spi, buf)) {
		SYS_LOG_DBG("%s: Cannot feed in TX fifo\n", __func__);
		goto error;
	}

	if (!verify_txfifo_status(cc2520, buf)) {
		SYS_LOG_DBG("%s: Did not write properly into TX FIFO\n", __func__);
		goto error;
	}

	/* 1 retry is allowed here */
	do {
		atomic_set(&cc2520->tx, 1);

		if (!instruct_stxoncca(&cc2520->spi)) {
			SYS_LOG_DBG("%s: Cannot start transmission\n", __func__);
			goto error;
		}

		/* _cc2520_print_exceptions(cc2520); */

		device_sync_call_wait(&cc2520->tx_sync);

		retry--;
		status = verify_tx_done(cc2520);
	} while (!status && retry);

	if (!status) {
		SYS_LOG_DBG("%s: No TX_FRM_DONE\n", __func__);
		goto error;
	}

	enable_reception(cc2520);

	return 0;
error:
	atomic_set(&cc2520->tx, 0);
	instruct_sflushtx(&cc2520->spi);
	enable_reception(cc2520);

	return -EIO;
}

static inline uint8_t *cc2520_get_mac(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	if (cc2520->mac_addr[1] == 0x00) {
		/* TI OUI */
		cc2520->mac_addr[0] = 0x00;
		cc2520->mac_addr[1] = 0x12;
		cc2520->mac_addr[2] = 0x4b;

		cc2520->mac_addr[3] = 0x00;
		UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
			      (uint32_t *) ((void *)cc2520->mac_addr+4));

		cc2520->mac_addr[7] = (cc2520->mac_addr[7] & ~0x01) | 0x02;
	}

	return cc2520->mac_addr;
}

static inline int cc2520_start(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s\n", __func__);

	if (!instruct_sxoscon(&cc2520->spi) ||
	    !instruct_srxon(&cc2520->spi) ||
	    !verify_osc_stabilization(cc2520)) {
		return -EIO;
	}

	flush_rxfifo(cc2520);

	enable_fifop_interrupt(cc2520, true);
	enable_sfd_interrupt(cc2520, true);

	return 0;
}

static inline int cc2520_stop(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%s\n", __func__);

	enable_fifop_interrupt(cc2520, false);
	enable_sfd_interrupt(cc2520, false);

	if (!instruct_srfoff(&cc2520->spi) ||
	    !instruct_sxoscoff(&cc2520->spi)) {
		return -EIO;
	}

	flush_rxfifo(cc2520);

	return 0;
}


/***************************
 * Legacy Radio device API *
 **************************/
/**
 * NOTE: This legacy API DOES NOT FIT within Zephyr device driver model
 *       and, as such, will be made obsolete soon (well, hopefully...)
 */

static int cc2520_initialize(void)
{
	const uint8_t *mac = cc2520_get_mac(cc2520_sglt);
	uint16_t short_addr;

	/** That is not great either, basically ieee802154/net stack,
	 * should get the mac, then set what's relevant. It's not up
	 * to the driver to do such thing.
	 */
	net_set_mac((uint8_t *)mac, 8);

	/* Setting short address... */
	short_addr = (mac[0] << 8) + mac[1];
	cc2520_set_short_addr(cc2520_sglt, short_addr);

	/* ... And ieee address */
	cc2520_set_ieee_addr(cc2520_sglt, mac);

	return 1;
}

static int cc2520_prepare(const void *payload, unsigned short payload_len)
{
	return 0;
}

static int cc2520_transmit(struct net_buf *buf, unsigned short transmit_len)
{
	if (cc2520_tx(cc2520_sglt, buf) != 0) {
		return RADIO_TX_ERR;
	}

	return RADIO_TX_OK;
}

static int cc2520_send(struct net_buf *buf,
		       const void *payload, unsigned short payload_len)
{
	return cc2520_transmit(buf, payload_len);
}

static int cc2520_read(void *buf, unsigned short buf_len)
{
	return 0;
}

static int cc2520_channel_clear(void)
{
	struct cc2520_context *cc2520 = cc2520_sglt->driver_data;

	return get_cca(cc2520);
}

static int cc2520_receiving_packet(void)
{
	return 0;
}

static int cc2520_pending_packet(void)
{
	return 0;
}

static int cc2520_on(void)
{
	return (cc2520_start(cc2520_sglt) == 0);
}

static int cc2520_off(void)
{
	return (cc2520_stop(cc2520_sglt) == 0);
}

static radio_result_t cc2520_get_value(radio_param_t param,
				       radio_value_t *value)
{
	switch (param) {
	case RADIO_PARAM_POWER_MODE:
		*value = RADIO_POWER_MODE_ON;
		break;
	case RADIO_PARAM_CHANNEL:
		*value = CONFIG_TI_CC2520_CHANNEL;
		break;
	case RADIO_CONST_CHANNEL_MIN:
		*value = 11;
		break;
	case RADIO_CONST_CHANNEL_MAX:
		*value = 26;
		break;
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}

	return RADIO_RESULT_OK;
}

static radio_result_t cc2520_set_value(radio_param_t param,
				       radio_value_t value)
{
	switch (param) {
	case RADIO_PARAM_POWER_MODE:
		break;
	case RADIO_PARAM_CHANNEL:
		cc2520_set_channel(cc2520_sglt, value);
		break;
	case RADIO_PARAM_PAN_ID:
		cc2520_set_pan_id(cc2520_sglt, value);
		break;
	case RADIO_PARAM_RX_MODE:
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}

	return RADIO_RESULT_OK;
}

static radio_result_t cc2520_get_object(radio_param_t param,
					void *dest, size_t size)
{
	return RADIO_RESULT_NOT_SUPPORTED;
}

static radio_result_t cc2520_set_object(radio_param_t param,
					const void *src, size_t size)
{
	return RADIO_RESULT_NOT_SUPPORTED;
}

struct radio_driver cc2520_15_4_radio_driver = {
	.init = cc2520_initialize,
	.prepare = cc2520_prepare,
	.transmit = cc2520_transmit,
	.send = cc2520_send,
	.read = cc2520_read,
	.channel_clear = cc2520_channel_clear,
	.receiving_packet = cc2520_receiving_packet,
	.pending_packet = cc2520_pending_packet,
	.on = cc2520_on,
	.off = cc2520_off,
	.get_value = cc2520_get_value,
	.set_value = cc2520_set_value,
	.get_object = cc2520_get_object,
	.set_object = cc2520_set_object,
};

/******************
 * Initialization *
 *****************/
static int power_on_and_setup(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	/* Switching to LPM2 mode */
	set_reset(dev, 0);
	_usleep(150);

	set_vreg_en(dev, 0);
	_usleep(250);

	/* Then to ACTIVE mode */
	set_vreg_en(dev, 1);
	_usleep(250);

	set_reset(dev, 1);
	_usleep(150);

	if (!verify_osc_stabilization(cc2520)) {
		return -EIO;
	}

	/* Default settings to always write (see chapter 28 part 1) */
	if (!write_reg_txpower(&cc2520->spi, CC2520_TXPOWER_DEFAULT) ||
	    !write_reg_ccactrl0(&cc2520->spi, CC2520_CCACTRL0_DEFAULT) ||
	    !write_reg_mdmctrl0(&cc2520->spi, CC2520_MDMCTRL0_DEFAULT) ||
	    !write_reg_mdmctrl1(&cc2520->spi, CC2520_MDMCTRL1_DEFAULT) ||
	    !write_reg_rxctrl(&cc2520->spi, CC2520_RXCTRL_DEFAULT) ||
	    !write_reg_fsctrl(&cc2520->spi, CC2520_FSCTRL_DEFAULT) ||
	    !write_reg_fscal1(&cc2520->spi, CC2520_FSCAL1_DEFAULT) ||
	    !write_reg_agcctrl1(&cc2520->spi, CC2520_AGCCTRL1_DEFAULT) ||
	    !write_reg_adctest0(&cc2520->spi, CC2520_ADCTEST0_DEFAULT) ||
	    !write_reg_adctest1(&cc2520->spi, CC2520_ADCTEST1_DEFAULT) ||
	    !write_reg_adctest2(&cc2520->spi, CC2520_ADCTEST2_DEFAULT)) {
		return -EIO;
	}

	/* EXTCLOCK0: Disabling external clock
	 * FRMCTRL0: AUTOACK and AUTOCRC enabled
	 * FRMCTRL1: SET_RXENMASK_ON_TX and IGNORE_TX_UNDERF
	 * FRMFILT0: Frame filtering (setting CC2520_FRAME_FILTERING)
	 * FIFOPCTRL: Set TX threshold (setting CC2520_TX_THRESHOLD)
	 */
	if (!write_reg_extclock(&cc2520->spi, 0) ||
	    !write_reg_frmctrl0(&cc2520->spi, CC2520_AUTOMATISM) ||
	    !write_reg_frmctrl1(&cc2520->spi, FRMCTRL1_IGNORE_TX_UNDERF |
				FRMCTRL1_SET_RXENMASK_ON_TX) ||
	    !write_reg_frmfilt0(&cc2520->spi, FRMFILT0_FRAME_FILTER_EN |
				FRMFILT0_MAX_FRAME_VERSION(3)) ||
	    !write_reg_frmfilt1(&cc2520->spi, FRMFILT1_ACCEPT_ALL) ||
	    !write_reg_srcmatch(&cc2520->spi, SRCMATCH_DEFAULTS) ||
	    !write_reg_fifopctrl(&cc2520->spi,
				 FIFOPCTRL_FIFOP_THR(CC2520_TX_THRESHOLD))) {
		return -EIO;
	}

	/* Cleaning up TX fifo */
	instruct_sflushtx(&cc2520->spi);

	setup_gpio_callbacks(dev);

	_cc2520_print_gpio_config(dev);

	return 0;
}

static inline int configure_spi(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;
	struct spi_config spi_conf = {
		.config = SPI_WORD(8),
		.max_sys_freq = CONFIG_TI_CC2520_SPI_FREQ,
	};

	cc2520->spi.dev = device_get_binding(CONFIG_TI_CC2520_SPI_DRV_NAME);
	if (cc2520->spi.dev) {
		cc2520->spi.slave = CONFIG_TI_CC2520_SPI_SLAVE;

		if (spi_configure(cc2520->spi.dev, &spi_conf) != 0 ||
		    spi_slave_select(cc2520->spi.dev,
				     cc2520->spi.slave) != 0) {
			cc2520->spi.dev = NULL;
			return -EIO;
		}
	}

	return 0;
}

int cc2520_init(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	dev->driver_api = NULL;

	device_sync_call_init(&cc2520->tx_sync);
	atomic_set(&cc2520->tx, 0);
	nano_sem_init(&cc2520->rx_lock);

	cc2520->gpios = cc2520_configure_gpios();
	if (!cc2520->gpios) {
		SYS_LOG_DBG("Configuring GPIOS failed\n");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		SYS_LOG_DBG("Configuring SPI failed\n");
		return -EIO;
	}

	SYS_LOG_DBG("GPIO and SPI configured\n");

	if (power_on_and_setup(dev) != 0) {
		SYS_LOG_DBG("Configuring CC2520 failed\n");
		return -EIO;
	}

	/* That should not be done here... */
	if (cc2520_set_pan_id(dev, 0xFFFF) != 0 ||
	    cc2520_set_short_addr(dev, 0x0000) != 0 ||
	    cc2520_set_channel(dev, CONFIG_TI_CC2520_CHANNEL) != 0) {
		SYS_LOG_DBG("Could not initialize properly cc2520\n");
		return -EIO;
	}

	task_fiber_start(cc2520->cc2520_rx_stack,
			 CONFIG_CC2520_RX_STACK_SIZE,
			 cc2520_rx, POINTER_TO_INT(dev),
			 0, 0, 0);

	cc2520_sglt = dev;

	return 0;
}

struct cc2520_context cc2520_context_data;

DEVICE_INIT(cc2520, CONFIG_TI_CC2520_DRV_NAME,
	    cc2520_init, &cc2520_context_data, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
