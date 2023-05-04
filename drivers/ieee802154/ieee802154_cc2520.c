/* ieee802154_cc2520.c - TI CC2520 driver */

#define DT_DRV_COMPAT ti_cc2520

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_cc2520
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/rand32.h>

#include <zephyr/drivers/gpio.h>

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO

#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/cipher.h>

#endif /* CONFIG_IEEE802154_CC2520_CRYPTO */

#include <zephyr/net/ieee802154_radio.h>

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


#define CC2520_AUTOMATISM		(FRMCTRL0_AUTOCRC | FRMCTRL0_AUTOACK)
#define CC2520_TX_THRESHOLD		(0x7F)
#define CC2520_FCS_LENGTH		(2)

/*********
 * DEBUG *
 ********/
#if LOG_LEVEL == LOG_LEVEL_DBG
static inline void cc2520_print_gpio_config(const struct device *dev)
{
	struct cc2520_context *cc2520 = dev->data;

	LOG_DBG("GPIOCTRL0/1/2/3/4/5 = 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x",
		read_reg_gpioctrl0(cc2520),
		read_reg_gpioctrl1(cc2520),
		read_reg_gpioctrl2(cc2520),
		read_reg_gpioctrl3(cc2520),
		read_reg_gpioctrl4(cc2520),
		read_reg_gpioctrl5(cc2520));
	LOG_DBG("GPIOPOLARITY: 0x%x",
		read_reg_gpiopolarity(cc2520));
	LOG_DBG("GPIOCTRL: 0x%x",
		read_reg_gpioctrl(cc2520));
}

static inline void cc2520_print_exceptions(struct cc2520_context *cc2520)
{
	uint8_t flag = read_reg_excflag0(cc2520);

	LOG_DBG("EXCFLAG0:");

	if (flag & EXCFLAG0_RF_IDLE) {
		LOG_DBG(" RF_IDLE");
	}

	if (flag & EXCFLAG0_TX_FRM_DONE) {
		LOG_DBG(" TX_FRM_DONE");
	}

	if (flag & EXCFLAG0_TX_ACK_DONE) {
		LOG_DBG(" TX_ACK_DONE");
	}

	if (flag & EXCFLAG0_TX_UNDERFLOW) {
		LOG_DBG(" TX_UNDERFLOW");
	}

	if (flag & EXCFLAG0_TX_OVERFLOW) {
		LOG_DBG(" TX_OVERFLOW");
	}

	if (flag & EXCFLAG0_RX_UNDERFLOW) {
		LOG_DBG(" RX_UNDERFLOW");
	}

	if (flag & EXCFLAG0_RX_OVERFLOW) {
		LOG_DBG(" RX_OVERFLOW");
	}

	if (flag & EXCFLAG0_RXENABLE_ZERO) {
		LOG_DBG(" RXENABLE_ZERO");
	}

	flag = read_reg_excflag1(cc2520);

	LOG_DBG("EXCFLAG1:");

	if (flag & EXCFLAG1_RX_FRM_DONE) {
		LOG_DBG(" RX_FRM_DONE");
	}

	if (flag & EXCFLAG1_RX_FRM_ACCEPTED) {
		LOG_DBG(" RX_FRM_ACCEPTED");
	}

	if (flag & EXCFLAG1_SRC_MATCH_DONE) {
		LOG_DBG(" SRC_MATCH_DONE");
	}

	if (flag & EXCFLAG1_SRC_MATCH_FOUND) {
		LOG_DBG(" SRC_MATCH_FOUND");
	}

	if (flag & EXCFLAG1_FIFOP) {
		LOG_DBG(" FIFOP");
	}

	if (flag & EXCFLAG1_SFD) {
		LOG_DBG(" SFD");
	}

	if (flag & EXCFLAG1_DPU_DONE_L) {
		LOG_DBG(" DPU_DONE_L");
	}

	if (flag & EXCFLAG1_DPU_DONE_H) {
		LOG_DBG(" DPU_DONE_H");
	}
}

static inline void cc2520_print_errors(struct cc2520_context *cc2520)
{
	uint8_t flag = read_reg_excflag2(cc2520);

	LOG_DBG("EXCFLAG2:");

	if (flag & EXCFLAG2_MEMADDR_ERROR) {
		LOG_DBG(" MEMADDR_ERROR");
	}

	if (flag & EXCFLAG2_USAGE_ERROR) {
		LOG_DBG(" USAGE_ERROR");
	}

	if (flag & EXCFLAG2_OPERAND_ERROR) {
		LOG_DBG(" OPERAND_ERROR");
	}

	if (flag & EXCFLAG2_SPI_ERROR) {
		LOG_DBG(" SPI_ERROR");
	}

	if (flag & EXCFLAG2_RF_NO_LOCK) {
		LOG_DBG(" RF_NO_LOCK");
	}

	if (flag & EXCFLAG2_RX_FRM_ABORTED) {
		LOG_DBG(" RX_FRM_ABORTED");
	}

	if (flag & EXCFLAG2_RFBUFMOV_TIMEOUT) {
		LOG_DBG(" RFBUFMOV_TIMEOUT");
	}
}
#else
#define cc2520_print_gpio_config(...)
#define cc2520_print_exceptions(...)
#define cc2520_print_errors(...)
#endif /* LOG_LEVEL == LOG_LEVEL_DBG */


/*********************
 * Generic functions *
 ********************/
#define z_usleep(usec) k_busy_wait(usec)

bool z_cc2520_access(const struct device *dev, bool read, uint8_t ins,
		     uint16_t addr, void *data, size_t length)
{
	const struct cc2520_config *cfg = dev->config;
	uint8_t cmd_buf[2];
	struct spi_buf buf[2] = {
		{
			.buf = cmd_buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length,

		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
	};


	cmd_buf[0] = ins;

	if (ins == CC2520_INS_MEMRD || ins == CC2520_INS_MEMWR) {
		buf[0].len = 2;
		cmd_buf[0] |= (uint8_t)(addr >> 8);
		cmd_buf[1] = (uint8_t)(addr & 0xff);
	} else if (ins == CC2520_INS_REGRD || ins == CC2520_INS_REGWR) {
		cmd_buf[0] |= (uint8_t)(addr & 0xff);
	}

	if (read) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		tx.count = 1;

		return (spi_transceive_dt(&cfg->bus, &tx, &rx) == 0);
	}

	tx.count = data ? 2 : 1;

	return (spi_write_dt(&cfg->bus, &tx) == 0);
}

static inline uint8_t cc2520_status(const struct device *dev)
{
	uint8_t status;

	if (z_cc2520_access(dev, true, CC2520_INS_SNOP, 0, &status, 1)) {
		return status;
	}

	return 0;
}

static bool verify_osc_stabilization(const struct device *dev)
{
	uint8_t timeout = 100U;
	uint8_t status;

	do {
		status = cc2520_status(dev);
		z_usleep(1);
		timeout--;
	} while (!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING) && timeout);

	return !!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING);
}


static inline uint8_t *get_mac(const struct device *dev)
{
	struct cc2520_context *cc2520 = dev->data;

#if defined(CONFIG_IEEE802154_CC2520_RANDOM_MAC)
	uint32_t *ptr = (uint32_t *)(cc2520->mac_addr + 4);

	UNALIGNED_PUT(sys_rand32_get(), ptr);

	cc2520->mac_addr[7] = (cc2520->mac_addr[7] & ~0x01) | 0x02;
#else
	cc2520->mac_addr[4] = CONFIG_IEEE802154_CC2520_MAC4;
	cc2520->mac_addr[5] = CONFIG_IEEE802154_CC2520_MAC5;
	cc2520->mac_addr[6] = CONFIG_IEEE802154_CC2520_MAC6;
	cc2520->mac_addr[7] = CONFIG_IEEE802154_CC2520_MAC7;
#endif

	cc2520->mac_addr[0] = 0x00;
	cc2520->mac_addr[1] = 0x12;
	cc2520->mac_addr[2] = 0x4b;
	cc2520->mac_addr[3] = 0x00;

	return cc2520->mac_addr;
}

static int cc2520_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	LOG_DBG("0x%x", pan_id);

	pan_id = sys_le16_to_cpu(pan_id);

	if (!write_mem_pan_id(dev, (uint8_t *) &pan_id)) {
		LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_set_short_addr(const struct device *dev,
				 uint16_t short_addr)
{
	LOG_DBG("0x%x", short_addr);

	short_addr = sys_le16_to_cpu(short_addr);

	if (!write_mem_short_addr(dev, (uint8_t *) &short_addr)) {
		LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_set_ieee_addr(const struct device *dev,
				const uint8_t *ieee_addr)
{
	if (!write_mem_ext_addr(dev, (void *)ieee_addr)) {
		LOG_ERR("Failed");
		return -EIO;
	}

	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	return 0;
}

/******************
 * GPIO functions *
 *****************/
static inline void set_reset(const struct device *dev, uint32_t value)
{
	const struct cc2520_config *cfg = dev->config;

	gpio_pin_set_raw(cfg->reset.port, cfg->reset.pin, value);
}

static inline void set_vreg_en(const struct device *dev, uint32_t value)
{
	const struct cc2520_config *cfg = dev->config;

	gpio_pin_set_raw(cfg->vreg_en.port, cfg->vreg_en.pin, value);
}

static inline uint32_t get_fifo(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;

	return gpio_pin_get_raw(cfg->fifo.port, cfg->fifo.pin);
}

static inline uint32_t get_fifop(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;

	return gpio_pin_get_raw(cfg->fifop.port, cfg->fifop.pin);
}

static inline uint32_t get_cca(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;

	return gpio_pin_get_raw(cfg->cca.port, cfg->cca.pin);
}

static inline void sfd_int_handler(const struct device *port,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, sfd_cb);

	if (atomic_get(&cc2520->tx) == 1) {
		atomic_set(&cc2520->tx, 0);
		k_sem_give(&cc2520->tx_sync);
	}
}

static inline void fifop_int_handler(const struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, fifop_cb);

	/* Note: Errata document - 1.2 */
	if (!get_fifop(cc2520->dev) && !get_fifop(cc2520->dev)) {
		return;
	}

	if (!get_fifo(cc2520->dev)) {
		cc2520->overflow = true;
	}

	k_sem_give(&cc2520->rx_lock);
}

static void enable_fifop_interrupt(const struct device *dev,
				   bool enable)
{
	const struct cc2520_config *cfg = dev->config;
	gpio_flags_t mode = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->fifop, mode);
}

static void enable_sfd_interrupt(const struct device *dev,
				 bool enable)
{
	const struct cc2520_config *cfg = dev->config;
	gpio_flags_t mode = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->sfd, mode);
}

static inline int setup_gpio_callbacks(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;
	struct cc2520_context *cc2520 = dev->data;

	gpio_init_callback(&cc2520->sfd_cb, sfd_int_handler, BIT(cfg->sfd.pin));
	if (gpio_add_callback(cfg->sfd.port, &cc2520->sfd_cb) != 0) {
		return -EIO;
	}


	gpio_init_callback(&cc2520->fifop_cb, fifop_int_handler, BIT(cfg->fifop.pin));
	if (gpio_add_callback(cfg->fifop.port, &cc2520->fifop_cb) != 0) {
		return -EIO;
	}

	return 0;
}


/****************
 * TX functions *
 ***************/
static inline bool write_txfifo_length(const struct device *dev, uint8_t len)
{
	uint8_t length = len + CC2520_FCS_LENGTH;

	return z_cc2520_access(dev, false, CC2520_INS_TXBUF, 0, &length, 1);
}

static inline bool write_txfifo_content(const struct device *dev,
					uint8_t *frame, uint8_t len)
{
	return z_cc2520_access(dev, false, CC2520_INS_TXBUF, 0, frame, len);
}

static inline bool verify_txfifo_status(const struct device *dev,
					uint8_t len)
{
	if (read_reg_txfifocnt(dev) < len ||
	    (read_reg_excflag0(dev) & EXCFLAG0_TX_UNDERFLOW)) {
		return false;
	}

	return true;
}

static inline bool verify_tx_done(const struct device *dev)
{
	uint8_t timeout = 10U;
	uint8_t status;

	do {
		z_usleep(1);
		timeout--;
		status = read_reg_excflag0(dev);
	} while (!(status & EXCFLAG0_TX_FRM_DONE) && timeout);

	return !!(status & EXCFLAG0_TX_FRM_DONE);
}

/****************
 * RX functions *
 ***************/

static inline void flush_rxfifo(const struct device *dev)
{
	/* Note: Errata document - 1.1 */
	enable_fifop_interrupt(dev, false);

	instruct_sflushrx(dev);
	instruct_sflushrx(dev);

	enable_fifop_interrupt(dev, true);

	write_reg_excflag0(dev, EXCFLAG0_RESET_RX_FLAGS);
}

static inline uint8_t read_rxfifo_length(const struct device *dev)
{
	uint8_t len;


	if (z_cc2520_access(dev, true, CC2520_INS_RXBUF, 0, &len, 1)) {
		return len;
	}

	return 0;
}

static inline bool read_rxfifo_content(const struct device *dev,
				       struct net_buf *buf, uint8_t len)
{
	if (!z_cc2520_access(dev, true, CC2520_INS_RXBUF, 0, buf->data, len)) {
		return false;
	}

	if (read_reg_excflag0(dev) & EXCFLAG0_RX_UNDERFLOW) {
		LOG_ERR("RX underflow!");
		return false;
	}

	net_buf_add(buf, len);

	return true;
}

static inline void insert_radio_noise_details(struct net_pkt *pkt, uint8_t *buf)
{
	uint8_t lqi;

	net_pkt_set_ieee802154_rssi(pkt, buf[0]);

	/**
	 * CC2520 does not provide an LQI but a correlation factor.
	 * See Section 20.6
	 * Such calculation can be loosely used to transform it to lqi:
	 * corr <= 50 ? lqi = 0
	 * or:
	 * corr >= 110 ? lqi = 255
	 * else:
	 * lqi = (lqi - 50) * 4
	 */
	lqi = buf[1] & CC2520_FCS_CORRELATION;
	if (lqi <= 50U) {
		lqi = 0U;
	} else if (lqi >= 110U) {
		lqi = 255U;
	} else {
		lqi = (lqi - 50U) << 2;
	}

	net_pkt_set_ieee802154_lqi(pkt, lqi);
}

static inline bool verify_crc(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t fcs[2];

	if (!z_cc2520_access(dev, true, CC2520_INS_RXBUF, 0, &fcs, 2)) {
		return false;
	}

	if (!(fcs[1] & CC2520_FCS_CRC_OK)) {
		return false;
	}

	insert_radio_noise_details(pkt, fcs);

	return true;
}

static inline bool verify_rxfifo_validity(const struct device *dev,
					  uint8_t pkt_len)
{
	if (pkt_len < 2 || read_reg_rxfifocnt(dev) != pkt_len) {
		return false;
	}

	return true;
}

static void cc2520_rx(void *arg)
{
	const struct device *dev = arg;
	struct cc2520_context *cc2520 = dev->data;
	struct net_pkt *pkt;
	uint8_t pkt_len;

	while (1) {
		pkt = NULL;

		k_sem_take(&cc2520->rx_lock, K_FOREVER);

		if (cc2520->overflow) {
			LOG_ERR("RX overflow!");
			cc2520->overflow = false;

			goto flush;
		}

		pkt_len = read_rxfifo_length(dev) & 0x7f;
		if (!verify_rxfifo_validity(dev, pkt_len)) {
			LOG_ERR("Invalid content");
			goto flush;
		}

		pkt = net_pkt_rx_alloc_with_buffer(cc2520->iface, pkt_len,
						   AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No pkt available");
			goto flush;
		}

		if (!IS_ENABLED(CONFIG_IEEE802154_RAW_MODE)) {
			pkt_len -= 2U;
		}

		if (!read_rxfifo_content(dev, pkt->buffer, pkt_len)) {
			LOG_ERR("No content read");
			goto flush;
		}

		if (!verify_crc(dev, pkt)) {
			LOG_ERR("Bad packet CRC");
			goto out;
		}

		if (ieee802154_radio_handle_ack(cc2520->iface, pkt) == NET_OK) {
			LOG_DBG("ACK packet handled");
			goto out;
		}

		LOG_DBG("Caught a packet (%u)", pkt_len);

		if (net_recv_data(cc2520->iface, pkt) < 0) {
			LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		log_stack_usage(&cc2520->cc2520_rx_thread);
		continue;
flush:
		cc2520_print_exceptions(cc2520);
		cc2520_print_errors(cc2520);
		flush_rxfifo(dev);
out:
		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

/********************
 * Radio device API *
 *******************/
static enum ieee802154_hw_caps cc2520_get_capabilities(const struct device *dev)
{
	/* ToDo: Add support for IEEE802154_HW_PROMISC */
	return IEEE802154_HW_FCS |
		IEEE802154_HW_2_4_GHZ |
		IEEE802154_HW_FILTER;
}

static int cc2520_cca(const struct device *dev)
{
	if (!get_cca(dev)) {
		LOG_WRN("Busy");
		return -EBUSY;
	}

	return 0;
}

static int cc2520_set_channel(const struct device *dev, uint16_t channel)
{
	LOG_DBG("%u", channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	/* See chapter 16 */
	channel = 11 + (channel - 11) * 5U;

	if (!write_reg_freqctrl(dev, FREQCTRL_FREQ(channel))) {
		LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_filter(const struct device *dev,
			 bool set,
			 enum ieee802154_filter_type type,
			 const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return cc2520_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return cc2520_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return cc2520_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int cc2520_set_txpower(const struct device *dev, int16_t dbm)
{
	uint8_t pwr;

	LOG_DBG("%d", dbm);

	/* See chapter 19 part 8 */
	switch (dbm) {
	case 5:
		pwr = 0xF7;
		break;
	case 3:
		pwr = 0xF2;
		break;
	case 2:
		pwr = 0xAB;
		break;
	case 1:
		pwr = 0x13;
		break;
	case 0:
		pwr = 0x32;
		break;
	case -2:
		pwr = 0x81;
		break;
	case -4:
		pwr = 0x88;
		break;
	case -7:
		pwr = 0x2C;
		break;
	case -18:
		pwr = 0x03;
		break;
	default:
		goto error;
	}

	if (!write_reg_txpower(dev, pwr)) {
		goto error;
	}

	return 0;
error:
	LOG_ERR("Failed");
	return -EIO;
}

static int cc2520_tx(const struct device *dev,
		     enum ieee802154_tx_mode mode,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	uint8_t *frame = frag->data;
	uint8_t len = frag->len;
	struct cc2520_context *cc2520 = dev->data;
	uint8_t retry = 2U;
	bool status;

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	LOG_DBG("%p (%u)", frag, len);

	if (!write_reg_excflag0(dev, EXCFLAG0_RESET_TX_FLAGS) ||
	    !write_txfifo_length(dev, len) ||
	    !write_txfifo_content(dev, frame, len)) {
		LOG_ERR("Cannot feed in TX fifo");
		goto error;
	}

	if (!verify_txfifo_status(dev, len)) {
		LOG_ERR("Did not write properly into TX FIFO");
		goto error;
	}

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_take(&cc2520->access_lock, K_FOREVER);
#endif

	/* 1 retry is allowed here */
	do {
		atomic_set(&cc2520->tx, 1);
		k_sem_init(&cc2520->tx_sync, 0, K_SEM_MAX_LIMIT);

		if (!instruct_stxoncca(dev)) {
			LOG_ERR("Cannot start transmission");
			goto error;
		}

		k_sem_take(&cc2520->tx_sync, K_MSEC(10));

		retry--;
		status = verify_tx_done(dev);
	} while (!status && retry);

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_give(&cc2520->access_lock);
#endif

	if (status) {
		return 0;
	}

error:
#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_give(&cc2520->access_lock);
#endif
	LOG_ERR("No TX_FRM_DONE");
	cc2520_print_exceptions(cc2520);
	cc2520_print_errors(cc2520);

	atomic_set(&cc2520->tx, 0);
	instruct_sflushtx(dev);

	return -EIO;
}

static int cc2520_start(const struct device *dev)
{
	if (!instruct_sxoscon(dev) ||
	    !instruct_srxon(dev) ||
	    !verify_osc_stabilization(dev)) {
		LOG_ERR("Error starting CC2520");
		return -EIO;
	}

	flush_rxfifo(dev);

	enable_fifop_interrupt(dev, true);
	enable_sfd_interrupt(dev, true);

	return 0;
}

static int cc2520_stop(const struct device *dev)
{
	flush_rxfifo(dev);

	enable_fifop_interrupt(dev, false);
	enable_sfd_interrupt(dev, false);

	if (!instruct_srfoff(dev) ||
	    !instruct_sxoscoff(dev)) {
		LOG_ERR("Error stopping CC2520");
		return -EIO;
	}

	return 0;
}

/******************
 * Initialization *
 *****************/
static int power_on_and_setup(const struct device *dev)
{
	/* Switching to LPM2 mode */
	set_reset(dev, 0);
	z_usleep(150);

	set_vreg_en(dev, 0);
	z_usleep(250);

	/* Then to ACTIVE mode */
	set_vreg_en(dev, 1);
	z_usleep(250);

	set_reset(dev, 1);
	z_usleep(150);

	if (!verify_osc_stabilization(dev)) {
		return -EIO;
	}

	/* Default settings to always write (see chapter 28 part 1) */
	if (!write_reg_txpower(dev, CC2520_TXPOWER_DEFAULT) ||
	    !write_reg_ccactrl0(dev, CC2520_CCACTRL0_DEFAULT) ||
	    !write_reg_mdmctrl0(dev, CC2520_MDMCTRL0_DEFAULT) ||
	    !write_reg_mdmctrl1(dev, CC2520_MDMCTRL1_DEFAULT) ||
	    !write_reg_rxctrl(dev, CC2520_RXCTRL_DEFAULT) ||
	    !write_reg_fsctrl(dev, CC2520_FSCTRL_DEFAULT) ||
	    !write_reg_fscal1(dev, CC2520_FSCAL1_DEFAULT) ||
	    !write_reg_agcctrl1(dev, CC2520_AGCCTRL1_DEFAULT) ||
	    !write_reg_adctest0(dev, CC2520_ADCTEST0_DEFAULT) ||
	    !write_reg_adctest1(dev, CC2520_ADCTEST1_DEFAULT) ||
	    !write_reg_adctest2(dev, CC2520_ADCTEST2_DEFAULT)) {
		return -EIO;
	}

	/* EXTCLOCK0: Disabling external clock
	 * FRMCTRL0: AUTOACK and AUTOCRC enabled
	 * FRMCTRL1: SET_RXENMASK_ON_TX and IGNORE_TX_UNDERF
	 * FRMFILT0: Frame filtering (setting CC2520_FRAME_FILTERING)
	 * FIFOPCTRL: Set TX threshold (setting CC2520_TX_THRESHOLD)
	 */
	if (!write_reg_extclock(dev, 0) ||
	    !write_reg_frmctrl0(dev, CC2520_AUTOMATISM) ||
	    !write_reg_frmctrl1(dev, FRMCTRL1_IGNORE_TX_UNDERF |
				FRMCTRL1_SET_RXENMASK_ON_TX) ||
	    !write_reg_frmfilt0(dev, FRMFILT0_FRAME_FILTER_EN |
				FRMFILT0_MAX_FRAME_VERSION(3)) ||
	    !write_reg_frmfilt1(dev, FRMFILT1_ACCEPT_ALL) ||
	    !write_reg_srcmatch(dev, SRCMATCH_DEFAULTS) ||
	    !write_reg_fifopctrl(dev,
				 FIFOPCTRL_FIFOP_THR(CC2520_TX_THRESHOLD))) {
		return -EIO;
	}

	/* Cleaning up TX fifo */
	instruct_sflushtx(dev);

	if (setup_gpio_callbacks(dev) != 0) {
		return -EIO;
	}

	cc2520_print_gpio_config(dev);

	return 0;
}

static int configure_gpios(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;

	if (!device_is_ready(cfg->vreg_en.port) ||
	    !device_is_ready(cfg->reset.port) ||
	    !device_is_ready(cfg->fifo.port) ||
	    !device_is_ready(cfg->cca.port) ||
	    !device_is_ready(cfg->sfd.port) ||
	    !device_is_ready(cfg->fifop.port)) {
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->vreg_en, GPIO_OUTPUT_LOW);
	gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_LOW);
	gpio_pin_configure_dt(&cfg->fifo, GPIO_INPUT);
	gpio_pin_configure_dt(&cfg->cca, GPIO_INPUT);
	gpio_pin_configure_dt(&cfg->sfd, GPIO_INPUT);
	gpio_pin_configure_dt(&cfg->fifop, GPIO_INPUT);

	return 0;
}

static int cc2520_init(const struct device *dev)
{
	const struct cc2520_config *cfg = dev->config;
	struct cc2520_context *cc2520 = dev->data;

	cc2520->dev = dev;

	atomic_set(&cc2520->tx, 0);
	k_sem_init(&cc2520->rx_lock, 0, K_SEM_MAX_LIMIT);

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_init(&cc2520->access_lock, 1, 1);
#endif

	if (configure_gpios(dev) != 0) {
		LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -EIO;
	}

	LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		LOG_ERR("Configuring CC2520 failed");
		return -EIO;
	}

	k_thread_create(&cc2520->cc2520_rx_thread, cc2520->cc2520_rx_stack,
			CONFIG_IEEE802154_CC2520_RX_STACK_SIZE,
			(k_thread_entry_t)cc2520_rx,
			(void *)dev, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&cc2520->cc2520_rx_thread, "cc2520_rx");

	LOG_INF("CC2520 initialized");

	return 0;
}

static void cc2520_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cc2520_context *cc2520 = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	cc2520->iface = iface;

	ieee802154_init(iface);
}

static const struct cc2520_config cc2520_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.vreg_en = GPIO_DT_SPEC_INST_GET(0, vreg_en_gpios),
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
	.fifo = GPIO_DT_SPEC_INST_GET(0, fifo_gpios),
	.cca = GPIO_DT_SPEC_INST_GET(0, cca_gpios),
	.sfd = GPIO_DT_SPEC_INST_GET(0, sfd_gpios),
	.fifop = GPIO_DT_SPEC_INST_GET(0, fifop_gpios)
};

static struct cc2520_context cc2520_context_data;

static struct ieee802154_radio_api cc2520_radio_api = {
	.iface_api.init	= cc2520_iface_init,

	.get_capabilities	= cc2520_get_capabilities,
	.cca			= cc2520_cca,
	.set_channel		= cc2520_set_channel,
	.filter			= cc2520_filter,
	.set_txpower		= cc2520_set_txpower,
	.start			= cc2520_start,
	.stop			= cc2520_stop,
	.tx			= cc2520_tx,
};

#if defined(CONFIG_IEEE802154_RAW_MODE)
DEVICE_DT_INST_DEFINE(0, cc2520_init, NULL, &cc2520_context_data, NULL,
		      POST_KERNEL, CONFIG_IEEE802154_CC2520_INIT_PRIO,
		      &cc2520_radio_api);
#else
NET_DEVICE_DT_INST_DEFINE(0, cc2520_init, NULL, &cc2520_context_data,
			  &cc2520_config, CONFIG_IEEE802154_CC2520_INIT_PRIO,
			  &cc2520_radio_api, IEEE802154_L2,
			  NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
#endif


#ifdef CONFIG_IEEE802154_CC2520_CRYPTO

static inline bool cc2520_read_ram(const struct device *dev, uint16_t addr,
				   uint8_t *data_buf, uint8_t len)
{
	return z_cc2520_access(dev, true, CC2520_INS_MEMRD,
			       addr, data_buf, len);
}

static inline bool cc2520_write_ram(const struct device *dev, uint16_t addr,
				    uint8_t *data_buf, uint8_t len)
{
	return z_cc2520_access(dev, false, CC2520_INS_MEMWR,
			       addr, data_buf, len);
}

static inline bool instruct_uccm_ccm(const struct device *dev,
				     bool uccm,
				     uint8_t key_addr,
				     uint8_t auth_crypt,
				     uint8_t nonce_addr,
				     uint16_t input_addr,
				     uint16_t output_addr,
				     uint8_t in_len,
				     uint8_t m)
{
	const struct cc2520_config *cfg = dev->config;
	struct cc2520_context *ctx = dev->data;
	uint8_t cmd[9];
	const struct spi_buf buf[1] = {
		{
			.buf = cmd,
			.len = 9,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 1
	};

	int ret;

	LOG_DBG("%sCCM(P={01} K={%02x} C={%02x} N={%02x}"
		    " A={%03x} E={%03x} F{%02x} M={%02x})",
		    uccm ? "U" : "", key_addr, auth_crypt, nonce_addr,
		    input_addr, output_addr, in_len, m);

	cmd[0] = uccm ? CC2520_INS_UCCM | 1 : CC2520_INS_CCM | 1;
	cmd[1] = key_addr;
	cmd[2] = (auth_crypt & 0x7f);
	cmd[3] = nonce_addr;
	cmd[4] = (uint8_t)(((input_addr & 0x0f00) >> 4) |
			   ((output_addr & 0x0f00) >> 8));
	cmd[5] = (uint8_t)(input_addr & 0x00ff);
	cmd[6] = (uint8_t)(output_addr & 0x00ff);
	cmd[7] = (in_len & 0x7f);
	cmd[8] = (m & 0x03);

	k_sem_take(&ctx->access_lock, K_FOREVER);

	ret = spi_write_dt(&cfg->bus, &tx);

	k_sem_give(&ctx->access_lock);

	if (ret) {
		LOG_ERR("%sCCM Failed", uccm ? "U" : "");
		return false;
	}

	return true;
}

static inline void generate_nonce(uint8_t *ccm_nonce, uint8_t *nonce,
				  struct cipher_aead_pkt *apkt, uint8_t m)
{
	nonce[0] = 0 | (apkt->ad_len ? 0x40 : 0) | (m << 3) | 1;

	memcpy(&nonce[1], ccm_nonce, 13);

	nonce[14] = (uint8_t)(apkt->pkt->in_len >> 8);
	nonce[15] = (uint8_t)(apkt->pkt->in_len);

	/* See section 26.8.1 */
	sys_mem_swap(nonce, 16);
}

static int insert_crypto_parameters(struct cipher_ctx *ctx,
				    struct cipher_aead_pkt *apkt,
				    uint8_t *ccm_nonce, uint8_t *auth_crypt)
{
	const struct device *cc2520 = ctx->device;
	uint8_t data[128];
	uint8_t *in_buf;
	uint8_t in_len;
	uint8_t m = 0U;

	if (!apkt->pkt->out_buf || !apkt->pkt->out_buf_max) {
		LOG_ERR("Out buffer needs to be set");
		return -EINVAL;
	}

	if (!ctx->key.bit_stream || !ctx->keylen) {
		LOG_ERR("No key installed");
		return -EINVAL;
	}

	if (!(ctx->flags & CAP_INPLACE_OPS)) {
		LOG_ERR("It supports only in-place operation");
		return -EINVAL;
	}

	if (!apkt->ad || !apkt->ad_len) {
		LOG_ERR("CCM needs associated data");
		return -EINVAL;
	}

	if (apkt->pkt->in_buf && apkt->pkt->in_buf - apkt->ad_len != apkt->ad) {
		LOG_ERR("In-place needs ad and input in same memory");
		return -EINVAL;
	}

	if (!apkt->pkt->in_buf) {
		if (!ctx->mode_params.ccm_info.tag_len) {
			LOG_ERR("Auth only needs a tag length");
			return -EINVAL;
		}

		in_buf = apkt->ad;
		in_len = apkt->ad_len;

		*auth_crypt = 0U;
	} else {
		in_buf = data;

		memcpy(in_buf, apkt->ad, apkt->ad_len);
		memcpy(in_buf + apkt->ad_len,
		       apkt->pkt->in_buf, apkt->pkt->in_len);
		in_len = apkt->ad_len + apkt->pkt->in_len;

		*auth_crypt = !apkt->tag ? apkt->pkt->in_len :
			apkt->pkt->in_len - ctx->mode_params.ccm_info.tag_len;
	}

	if (ctx->mode_params.ccm_info.tag_len) {
		if ((ctx->mode_params.ccm_info.tag_len >> 2) > 3) {
			m = 3U;
		} else {
			m = ctx->mode_params.ccm_info.tag_len >> 2;
		}
	}

	/* Writing the frame in RAM */
	if (!cc2520_write_ram(cc2520, CC2520_MEM_DATA, in_buf, in_len)) {
		LOG_ERR("Cannot write the frame in RAM");
		return -EIO;
	}

	/* See section 26.8.1 */
	sys_memcpy_swap(data, ctx->key.bit_stream, ctx->keylen);

	/* Writing the key in RAM */
	if (!cc2520_write_ram(cc2520, CC2520_MEM_KEY, data, 16)) {
		LOG_ERR("Cannot write the key in RAM");
		return -EIO;
	}

	generate_nonce(ccm_nonce, data, apkt, m);

	/* Writing the nonce in RAM */
	if (!cc2520_write_ram(cc2520, CC2520_MEM_NONCE, data, 16)) {
		LOG_ERR("Cannot write the nonce in RAM");
		return -EIO;
	}

	return m;
}

static int cc2520_crypto_ccm(struct cipher_ctx *ctx,
			      struct cipher_aead_pkt *apkt,
			      uint8_t *ccm_nonce)
{
	const struct device *cc2520 = ctx->device;
	uint8_t auth_crypt;
	int m;

	if (!apkt || !apkt->pkt) {
		LOG_ERR("Invalid crypto packet to operate with");
		return -EINVAL;
	}

	m = insert_crypto_parameters(ctx, apkt, ccm_nonce, &auth_crypt);
	if (m < 0) {
		LOG_ERR("Inserting crypto parameters failed");
		return m;
	}

	apkt->pkt->out_len = apkt->pkt->in_len + apkt->ad_len +
		(m ? ctx->mode_params.ccm_info.tag_len : 0);

	if (apkt->pkt->out_len > apkt->pkt->out_buf_max) {
		LOG_ERR("Result will not fit into out buffer %u vs %u",
			    apkt->pkt->out_len, apkt->pkt->out_buf_max);
		return -ENOBUFS;
	}

	if (!instruct_uccm_ccm(cc2520, false, CC2520_MEM_KEY >> 4, auth_crypt,
			       CC2520_MEM_NONCE >> 4, CC2520_MEM_DATA,
			       0x000, apkt->ad_len, m) ||
	    !cc2520_read_ram(cc2520, CC2520_MEM_DATA,
			      apkt->pkt->out_buf, apkt->pkt->out_len)) {
		LOG_ERR("CCM or reading result from RAM failed");
		return -EIO;
	}

	if (apkt->tag) {
		memcpy(apkt->tag, apkt->pkt->out_buf + apkt->pkt->in_len,
					ctx->mode_params.ccm_info.tag_len);
	}

	return 0;
}

static int cc2520_crypto_uccm(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *apkt,
			       uint8_t *ccm_nonce)
{
	const struct device *cc2520 = ctx->device;
	uint8_t auth_crypt;
	int m;

	if (!apkt || !apkt->pkt) {
		LOG_ERR("Invalid crypto packet to operate with");
		return -EINVAL;
	}

	if (ctx->mode_params.ccm_info.tag_len && !apkt->tag) {
		LOG_ERR("In case of MIC you need to provide a tag");
		return -EINVAL;
	}

	m = insert_crypto_parameters(ctx, apkt, ccm_nonce, &auth_crypt);
	if (m < 0) {
		return m;
	}

	apkt->pkt->out_len = apkt->pkt->in_len + apkt->ad_len;

	if (!instruct_uccm_ccm(cc2520, true, CC2520_MEM_KEY >> 4, auth_crypt,
			       CC2520_MEM_NONCE >> 4, CC2520_MEM_DATA,
			       0x000, apkt->ad_len, m) ||
	    !cc2520_read_ram(cc2520, CC2520_MEM_DATA,
			      apkt->pkt->out_buf, apkt->pkt->out_len)) {
		LOG_ERR("UCCM or reading result from RAM failed");
		return -EIO;
	}

	if (m && (!(read_reg_dpustat(cc2520) & DPUSTAT_AUTHSTAT_H))) {
		LOG_ERR("Authentication of the frame failed");
		return -EBADMSG;
	}

	return 0;
}

static int cc2520_crypto_hw_caps(const struct device *dev)
{
	return CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SYNC_OPS;
}

static int cc2520_crypto_begin_session(const struct device *dev,
				       struct cipher_ctx *ctx,
				       enum cipher_algo algo,
				       enum cipher_mode mode,
				       enum cipher_op op_type)
{
	if (algo != CRYPTO_CIPHER_ALGO_AES || mode != CRYPTO_CIPHER_MODE_CCM) {
		LOG_ERR("Wrong algo (%u) or mode (%u)", algo, mode);
		return -EINVAL;
	}

	if (ctx->mode_params.ccm_info.nonce_len != 13U) {
		LOG_ERR("Nonce length erroneous (%u)",
			    ctx->mode_params.ccm_info.nonce_len);
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		ctx->ops.ccm_crypt_hndlr = cc2520_crypto_ccm;
	} else {
		ctx->ops.ccm_crypt_hndlr = cc2520_crypto_uccm;
	}

	ctx->ops.cipher_mode = mode;
	ctx->device = dev;

	return 0;
}

static int cc2520_crypto_free_session(const struct device *dev,
				      struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);

	ctx->ops.ccm_crypt_hndlr = NULL;
	ctx->device = NULL;

	return 0;
}

static int cc2520_crypto_init(const struct device *dev)
{
	LOG_INF("CC2520 crypto part initialized");

	return 0;
}

struct crypto_driver_api cc2520_crypto_api = {
	.query_hw_caps			= cc2520_crypto_hw_caps,
	.cipher_begin_session			= cc2520_crypto_begin_session,
	.cipher_free_session			= cc2520_crypto_free_session,
	.cipher_async_callback_set	= NULL
};

DEVICE_DEFINE(cc2520_crypto, "cc2520_crypto",
		cc2520_crypto_init, NULL,
		&cc2520_context_data, NULL, POST_KERNEL,
		CONFIG_IEEE802154_CC2520_CRYPTO_INIT_PRIO, &cc2520_crypto_api);

#endif /* CONFIG_IEEE802154_CC2520_CRYPTO */
