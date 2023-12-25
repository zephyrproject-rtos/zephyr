/* ieee802154_rf2xx.c - ATMEL RF2XX IEEE 802.15.4 Driver */

#define DT_DRV_COMPAT atmel_rf2xx

/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_rf2xx
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/random.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/ieee802154_radio.h>

#include "ieee802154_rf2xx.h"
#include "ieee802154_rf2xx_regs.h"
#include "ieee802154_rf2xx_iface.h"

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>

#define RF2XX_OT_PSDU_LENGTH              1280

#define RF2XX_ACK_FRAME_LEN               3
#define RF2XX_ACK_FRAME_TYPE              (2 << 0)
#define RF2XX_ACK_FRAME_PENDING_BIT       (1 << 4)
#define RF2XX_FRAME_CTRL_ACK_REQUEST_BIT  (1 << 5)

static uint8_t rf2xx_ack_psdu[RF2XX_ACK_FRAME_LEN] = { 0 };
static struct net_buf rf2xx_ack_frame = {
	.data  = rf2xx_ack_psdu,
	.size  = RF2XX_ACK_FRAME_LEN,
	.len   = RF2XX_ACK_FRAME_LEN,
	.__buf = rf2xx_ack_psdu,
	.frags = NULL,
};
static struct net_pkt rf2xx_ack_pkt = {
	.buffer = &rf2xx_ack_frame,
	.cb = {
		.lqi = 80,
		.rssi = -40,
	}
};
#endif /* CONFIG_NET_L2_OPENTHREAD */

/* Radio Transceiver ISR */
static inline void trx_isr_handler(const struct device *port,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct rf2xx_context *ctx = CONTAINER_OF(cb,
						 struct rf2xx_context,
						 irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&ctx->trx_isr_lock);
}

static void rf2xx_trx_set_state(const struct device *dev,
				enum rf2xx_trx_state_cmd_t state)
{
	do {
		rf2xx_iface_reg_write(dev, RF2XX_TRX_STATE_REG,
				      RF2XX_TRX_PHY_STATE_CMD_FORCE_TRX_OFF);
	} while (RF2XX_TRX_PHY_STATUS_TRX_OFF !=
		 (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
		  RF2XX_TRX_PHY_STATUS_MASK));

	do {
		rf2xx_iface_reg_write(dev, RF2XX_TRX_STATE_REG, state);
	} while (state !=
		 (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
		  RF2XX_TRX_PHY_STATUS_MASK));
}

static void rf2xx_trx_set_tx_state(const struct device *dev)
{
	uint8_t status;

	/**
	 * Ensures that RX automatically ACK will be sent when requested.
	 * Datasheet: Chapter 7.2.3 RX_AACK_ON – Receive with Automatic ACK
	 * Datasheet: Figure 7-13. Timing Example of an RX_AACK Transaction
	 * for Slotted Operation.
	 *
	 * This will create a spin lock that wait transceiver be free from
	 * current receive frame process
	 */
	do {
		status = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
			  RF2XX_TRX_PHY_STATUS_MASK);
	} while (status == RF2XX_TRX_PHY_STATUS_BUSY_RX_AACK ||
		 status == RF2XX_TRX_PHY_STATUS_STATE_TRANSITION);

	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TX_ARET_ON);
}

static void rf2xx_trx_set_rx_state(const struct device *dev)
{
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);
	/**
	 * Set extended RX mode
	 * Datasheet: chapter 7.2 Extended Operating Mode
	 */
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_RX_AACK_ON);
}

static void rf2xx_set_rssi_base(const struct device *dev, uint16_t channel)
{
	struct rf2xx_context *ctx = dev->data;
	int8_t base;

	if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915) {
		base = channel == 0
				? RF2XX_RSSI_BPSK_20
				: RF2XX_RSSI_BPSK_40;
	} else if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915) {
		base = channel == 0
				? RF2XX_RSSI_OQPSK_SIN_RC_100
				: RF2XX_RSSI_OQPSK_SIN_250;
	} else {
		base = RF2XX_RSSI_OQPSK_RC_250;
	}

	ctx->trx_rssi_base = base;
}

static void rf2xx_trx_rx(const struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	struct net_pkt *pkt = NULL;
	uint8_t rx_buf[RX2XX_MAX_FRAME_SIZE];
	uint8_t pkt_len;
	uint8_t frame_len;
	uint8_t trac;

	/*
	 * The rf2xx frame buffer can have length > 128 bytes. The
	 * net_pkt_rx_alloc_with_buffer allocates max value of 128 bytes.
	 *
	 * This obligate the driver to have rx_buf statically allocated with
	 * RX2XX_MAX_FRAME_SIZE.
	 */
	if (ctx->trx_model != RF2XX_TRX_MODEL_231) {
		pkt_len = ctx->rx_phr;
	} else {
		rf2xx_iface_frame_read(dev, rx_buf, RX2XX_FRAME_HEADER_SIZE);
		pkt_len = rx_buf[RX2XX_FRAME_PHR_INDEX];
	}

	if (!ctx->promiscuous && pkt_len < RX2XX_FRAME_MIN_PHR_SIZE) {
		LOG_ERR("Invalid RX frame length");
		return;
	}

	frame_len = RX2XX_FRAME_HEADER_SIZE + pkt_len +
		    RX2XX_FRAME_FOOTER_SIZE;

	rf2xx_iface_frame_read(dev, rx_buf, frame_len);

	if (ctx->trx_model != RF2XX_TRX_MODEL_231) {
		trac = rx_buf[pkt_len + RX2XX_FRAME_TRAC_INDEX];
		trac = (trac >> RF2XX_RX_TRAC_STATUS) & RF2XX_RX_TRAC_BIT_MASK;

		ctx->pkt_ed = rx_buf[pkt_len + RX2XX_FRAME_ED_INDEX];
	} else {
		trac = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATE_REG)
			>> RF2XX_TRAC_STATUS) & RF2XX_TRAC_BIT_MASK;

		ctx->pkt_ed = (rf2xx_iface_reg_read(dev, RF2XX_PHY_RSSI_REG)
			       >> RF2XX_RSSI) & RF2XX_RSSI_MASK;
	}
	ctx->pkt_lqi = rx_buf[pkt_len + RX2XX_FRAME_LQI_INDEX];

	if (!ctx->promiscuous && trac == RF2XX_TRX_PHY_STATE_TRAC_INVALID) {
		LOG_ERR("Invalid RX frame");
		return;
	}

	if (!IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) &&
	    !IS_ENABLED(CONFIG_NET_L2_OPENTHREAD) &&
	    pkt_len >= RX2XX_FRAME_FCS_LENGTH) {
		pkt_len -= RX2XX_FRAME_FCS_LENGTH;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, pkt_len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No RX buffer available");
		return;
	}

	memcpy(pkt->buffer->data, rx_buf + RX2XX_FRAME_HEADER_SIZE, pkt_len);
	net_buf_add(pkt->buffer, pkt_len);
	net_pkt_set_ieee802154_lqi(pkt, ctx->pkt_lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, ctx->pkt_ed + ctx->trx_rssi_base);

	LOG_DBG("Caught a packet (%02X) (LQI: %02X, RSSI: %d, ED: %02X)",
		pkt_len, ctx->pkt_lqi, ctx->trx_rssi_base + ctx->pkt_ed,
		ctx->pkt_ed);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		LOG_DBG("RX Packet dropped by NET stack");
		net_pkt_unref(pkt);
		return;
	}

	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		log_stack_usage(&ctx->trx_thread);
	}
}

static void rf2xx_process_rx_frame(const struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;

	/*
	 * NOTE: In promiscuous mode invalid frames will be processed.
	 */

	if (ctx->trx_model != RF2XX_TRX_MODEL_231) {
		rf2xx_trx_rx(dev);
	} else {
		/* Ensures that automatically ACK will be sent
		 * when requested
		 */
		while (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) ==
		       RF2XX_TRX_PHY_STATUS_BUSY_RX_AACK) {
			;
		}

		/* Set PLL_ON to avoid transceiver receive
		 * new data until finish reading process
		 */
		rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_PLL_ON);
		rf2xx_trx_rx(dev);
		rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_RX_AACK_ON);
	}
}

static void rf2xx_process_tx_frame(const struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;

	ctx->trx_trac = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATE_REG) >>
			 RF2XX_TRAC_STATUS) & RF2XX_TRAC_BIT_MASK;
	k_sem_give(&ctx->trx_tx_sync);
	rf2xx_trx_set_rx_state(dev);
}

static void rf2xx_process_trx_end(const struct device *dev)
{
	uint8_t trx_status = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
			   RF2XX_TRX_PHY_STATUS_MASK);

	if (trx_status == RF2XX_TRX_PHY_STATUS_TX_ARET_ON) {
		rf2xx_process_tx_frame(dev);
	} else {
		rf2xx_process_rx_frame(dev);
	}
}

static void rf2xx_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct rf2xx_context *ctx = p1;
	uint8_t isr_status;

	while (true) {
		k_sem_take(&ctx->trx_isr_lock, K_FOREVER);

		isr_status = rf2xx_iface_reg_read(ctx->dev,
						  RF2XX_IRQ_STATUS_REG);

		/*
		 *  IRQ_7 (BAT_LOW) Indicates a supply voltage below the
		 *    programmed threshold. 9.5.4
		 *  IRQ_6 (TRX_UR) Indicates a Frame Buffer access
		 *    violation. 9.3.3
		 *  IRQ_5 (AMI) Indicates address matching. 8.2
		 *  IRQ_4 (CCA_ED_DONE) Multi-functional interrupt:
		 *   1. AWAKE_END: 7.1.2.5
		 *      • Indicates finished transition to TRX_OFF state
		 *        from P_ON, SLEEP, DEEP_SLEEP, or RESET state.
		 *   2. CCA_ED_DONE: 8.5.4
		 *      • Indicates the end of a CCA or ED
		 *        measurement. 8.6.4
		 *  IRQ_3 (TRX_END)
		 *    RX: Indicates the completion of a frame
		 *      reception. 7.1.3
		 *    TX: Indicates the completion of a frame
		 *      transmission. 7.1.3
		 *  IRQ_2 (RX_START) Indicates the start of a PSDU
		 *    reception; the AT86RF233 state changed to BUSY_RX;
		 *    the PHR can be read from Frame Buffer. 7.1.3
		 *  IRQ_1 (PLL_UNLOCK) Indicates PLL unlock. If the radio
		 *    transceiver is in BUSY_TX / BUSY_TX_ARET state, the
		 *    PA is turned off immediately. 9.7.5
		 *  IRQ_0 (PLL_LOCK) Indicates PLL lock.
		 */
		if (isr_status & (1 << RF2XX_RX_START)) {
			if (ctx->trx_model != RF2XX_TRX_MODEL_231) {
				rf2xx_iface_sram_read(ctx->dev, 0,
						      &ctx->rx_phr, 1);
			}
		}
		if (isr_status & (1 << RF2XX_TRX_END)) {
			rf2xx_process_trx_end(ctx->dev);
		}
	}
}

static inline uint8_t *get_mac(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;
	uint32_t *ptr = (uint32_t *)(ctx->mac_addr);

	if (!conf->has_mac) {
		UNALIGNED_PUT(sys_rand32_get(), ptr);
		ptr = (uint32_t *)(ctx->mac_addr + 4);
		UNALIGNED_PUT(sys_rand32_get(), ptr);
	}

	/*
	 * Clear bit 0 to ensure it isn't a multicast address and set
	 * bit 1 to indicate address is locally administered and may
	 * not be globally unique.
	 */
	ctx->mac_addr[0] = (ctx->mac_addr[0] & ~0x01) | 0x02;

	return ctx->mac_addr;
}

static enum ieee802154_hw_caps rf2xx_get_capabilities(const struct device *dev)
{
	LOG_DBG("HW Caps");

	return IEEE802154_HW_FCS |
	       IEEE802154_HW_PROMISC |
	       IEEE802154_HW_FILTER |
	       IEEE802154_HW_CSMA |
	       IEEE802154_HW_RETRANSMISSION |
	       IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_RX_TX_ACK;
}

static int rf2xx_configure_sub_channel(const struct device *dev, uint16_t channel)
{
	struct rf2xx_context *ctx = dev->data;
	uint8_t reg;
	uint8_t cc_mask;

	if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915) {
		cc_mask = channel == 0
				   ? RF2XX_CC_BPSK_20
				   : RF2XX_CC_BPSK_40;
	} else if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915) {
		cc_mask = channel == 0
				   ? RF2XX_CC_OQPSK_SIN_RC_100
				   : RF2XX_CC_OQPSK_SIN_250;
	} else {
		cc_mask = RF2XX_CC_OQPSK_RC_250;
	}

	reg = rf2xx_iface_reg_read(dev, RF2XX_TRX_CTRL_2_REG)
	    & ~RF2XX_SUB_CHANNEL_MASK;
	rf2xx_iface_reg_write(dev, RF2XX_TRX_CTRL_2_REG, reg | cc_mask);

	return 0;
}

static int rf2xx_configure_trx_path(const struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	uint8_t reg;
	uint8_t gc_tx_offset;

	if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915) {
		gc_tx_offset = 0x03;
	} else {
		gc_tx_offset = 0x02;
	}

	reg = rf2xx_iface_reg_read(dev, RF2XX_RF_CTRL_0_REG)
	    & ~RF2XX_GC_TX_OFFS_MASK;
	rf2xx_iface_reg_write(dev, RF2XX_RF_CTRL_0_REG, reg | gc_tx_offset);

	return 0;
}

static int rf2xx_cca(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("CCA");

	return 0;
}

static int rf2xx_set_channel(const struct device *dev, uint16_t channel)
{
	struct rf2xx_context *ctx = dev->data;
	uint8_t reg;

	LOG_DBG("Set Channel %d", channel);

	if (ctx->trx_model == RF2XX_TRX_MODEL_212) {
		if ((ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915
		     || ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915)
		    && channel > 10) {
			LOG_ERR("Unsupported channel %u", channel);
			return channel > 26 ? -EINVAL : -ENOTSUP;
		}
		if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_FIVE_OQPSK_780 &&
		    channel > 3) {
			LOG_ERR("Unsupported channel %u", channel);
			return channel > 7 ? -EINVAL : -ENOTSUP;
		}

		rf2xx_configure_sub_channel(dev, channel);
		rf2xx_configure_trx_path(dev);
		rf2xx_set_rssi_base(dev, channel);
	} else {
		/* 2.4G O-QPSK, channel page zero */
		if (channel < 11 || channel > 26) {
			LOG_ERR("Unsupported channel %u", channel);
			return channel < 11 ? -ENOTSUP : -EINVAL;
		}
	}

	reg = rf2xx_iface_reg_read(dev, RF2XX_PHY_CC_CCA_REG) & ~0x1f;
	rf2xx_iface_reg_write(dev, RF2XX_PHY_CC_CCA_REG, reg | channel);

	return 0;
}

static int rf2xx_set_txpower(const struct device *dev, int16_t dbm)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;
	float min, max, step;
	uint8_t reg;
	uint8_t idx;
	uint8_t val;

	LOG_DBG("Try set Power to %d", dbm);

	/**
	 * if table size is equal 1 the code assumes a table was not defined. In
	 * this case the transceiver PHY_TX_PWR register will be set with value
	 * zero. This is a safe value for all variants and represents an output
	 * power above 0 dBm.
	 *
	 * Note: This is a special case too which avoid division by zero when
	 * computing the step variable.
	 */
	if (conf->tx_pwr_table_size == 1) {
		rf2xx_iface_reg_write(dev, RF2XX_PHY_TX_PWR_REG, 0);

		return 0;
	}

	min = conf->tx_pwr_min[1];
	if (conf->tx_pwr_min[0] == 0x01) {
		min *= -1.0f;
	}

	max = conf->tx_pwr_max[1];
	if (conf->tx_pwr_max[0] == 0x01) {
		min *= -1.0f;
	}

	step = (max - min) / ((float)conf->tx_pwr_table_size - 1.0f);

	if (step == 0.0f) {
		step = 1.0f;
	}

	LOG_DBG("Tx-power values: min %f, max %f, step %f, entries %d",
		(double)min, (double)max, (double)step, conf->tx_pwr_table_size);

	if (dbm < min) {
		LOG_INF("TX-power %d dBm below min of %f dBm, using %f dBm",
			dbm, (double)min, (double)max);
		dbm = min;
	} else if (dbm > max) {
		LOG_INF("TX-power %d dBm above max of %f dBm, using %f dBm",
			dbm, (double)min, (double)max);
		dbm = max;
	}

	idx = abs((int) (((float)(dbm - max) / step)));
	LOG_DBG("Tx-power idx: %d", idx);

	if (idx >= conf->tx_pwr_table_size) {
		idx = conf->tx_pwr_table_size - 1;
	}

	val = conf->tx_pwr_table[idx];

	if (ctx->trx_model != RF2XX_TRX_MODEL_212) {
		reg = rf2xx_iface_reg_read(dev, RF2XX_PHY_TX_PWR_REG) & 0xf0;
		val = reg + (val & 0x0f);
	}

	LOG_DBG("Tx-power normalized: %d dBm, PHY_TX_PWR 0x%02x, idx %d",
		dbm, val, idx);

	rf2xx_iface_reg_write(dev, RF2XX_PHY_TX_PWR_REG, val);

	return 0;
}

static int rf2xx_set_ieee_addr(const struct device *dev, bool set,
			       const uint8_t *ieee_addr)
{
	const uint8_t *ptr_to_reg = ieee_addr;

	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	if (set) {
		for (uint8_t i = 0; i < 8; i++, ptr_to_reg++) {
			rf2xx_iface_reg_write(dev, (RF2XX_IEEE_ADDR_0_REG + i),
					      *ptr_to_reg);
		}
	} else {
		for (uint8_t i = 0; i < 8; i++) {
			rf2xx_iface_reg_write(dev, (RF2XX_IEEE_ADDR_0_REG + i),
					      0);
		}
	}

	return 0;
}

static int rf2xx_set_short_addr(const struct device *dev, bool set,
				uint16_t short_addr)
{
	uint8_t short_addr_le[2] = { 0xFF, 0xFF };

	if (set) {
		sys_put_le16(short_addr, short_addr_le);
	}

	rf2xx_iface_reg_write(dev, RF2XX_SHORT_ADDR_0_REG, short_addr_le[0]);
	rf2xx_iface_reg_write(dev, RF2XX_SHORT_ADDR_1_REG, short_addr_le[1]);
	rf2xx_iface_reg_write(dev, RF2XX_CSMA_SEED_0_REG,
			      short_addr_le[0] + short_addr_le[1]);

	LOG_DBG("Short Address: 0x%02X%02X", short_addr_le[1],
		short_addr_le[0]);

	return 0;
}

static int rf2xx_set_pan_id(const struct device *dev, bool set,
			    uint16_t pan_id)
{
	uint8_t pan_id_le[2] = { 0xFF, 0xFF };

	if (set) {
		sys_put_le16(pan_id, pan_id_le);
	}

	rf2xx_iface_reg_write(dev, RF2XX_PAN_ID_0_REG, pan_id_le[0]);
	rf2xx_iface_reg_write(dev, RF2XX_PAN_ID_1_REG, pan_id_le[1]);

	LOG_DBG("Pan Id: 0x%02X%02X", pan_id_le[1], pan_id_le[0]);

	return 0;
}

static int rf2xx_filter(const struct device *dev,
			bool set, enum ieee802154_filter_type type,
			const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return rf2xx_set_ieee_addr(dev, set, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return rf2xx_set_short_addr(dev, set, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return rf2xx_set_pan_id(dev, set, filter->pan_id);
	}

	return -ENOTSUP;
}

#if defined(CONFIG_NET_L2_OPENTHREAD)
static void rf2xx_handle_ack(struct rf2xx_context *ctx, struct net_buf *frag)
{
	if ((frag->data[0] & RF2XX_FRAME_CTRL_ACK_REQUEST_BIT) == 0) {
		return;
	}

	rf2xx_ack_psdu[0] = RF2XX_ACK_FRAME_TYPE;
	rf2xx_ack_psdu[2] = frag->data[2];

	if (ctx->trx_trac == RF2XX_TRX_PHY_STATE_TRAC_SUCCESS_DATA_PENDING) {
		rf2xx_ack_psdu[0] |= RF2XX_ACK_FRAME_PENDING_BIT;
	}

	net_pkt_cursor_init(&rf2xx_ack_pkt);

	if (ieee802154_handle_ack(ctx->iface, &rf2xx_ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled.");
	}
}
#else
	#define rf2xx_handle_ack(...)
#endif

static int rf2xx_tx(const struct device *dev,
		    enum ieee802154_tx_mode mode,
		    struct net_pkt *pkt,
		    struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	struct rf2xx_context *ctx = dev->data;
	int response = 0;

	LOG_DBG("TX");

	if (ctx->tx_mode != mode) {
		switch (mode) {
		case IEEE802154_TX_MODE_DIRECT:
			/* skip retries & csma/ca algorithm */
			rf2xx_iface_reg_write(dev, RF2XX_XAH_CTRL_0_REG, 0x0E);
			break;
		case IEEE802154_TX_MODE_CSMA_CA:
			/* backoff maxBE = 5, minBE = 3 */
			rf2xx_iface_reg_write(dev, RF2XX_CSMA_BE_REG, 0x53);
			/* max frame retries = 3, csma/ca retries = 4 */
			rf2xx_iface_reg_write(dev, RF2XX_XAH_CTRL_0_REG, 0x38);
			break;
		case IEEE802154_TX_MODE_CCA:
			/* backoff period = 0 */
			rf2xx_iface_reg_write(dev, RF2XX_CSMA_BE_REG, 0x00);
			/* no frame retries & no csma/ca retries */
			rf2xx_iface_reg_write(dev, RF2XX_XAH_CTRL_0_REG, 0x00);
			break;
		case IEEE802154_TX_MODE_TXTIME:
		case IEEE802154_TX_MODE_TXTIME_CCA:
		default:
			NET_ERR("TX mode %d not supported", mode);
			return -ENOTSUP;
		}

		ctx->tx_mode = mode;
	}

	rf2xx_trx_set_tx_state(dev);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);

	k_sem_reset(&ctx->trx_tx_sync);
	rf2xx_iface_frame_write(dev, frag->data, frag->len);
	rf2xx_iface_phy_tx_start(dev);
	k_sem_take(&ctx->trx_tx_sync, K_FOREVER);

	switch (ctx->trx_trac) {
	/* Channel is still busy after attempting MAX_CSMA_RETRIES of
	 * CSMA-CA
	 */
	case RF2XX_TRX_PHY_STATE_TRAC_CHANNEL_ACCESS_FAILED:
		response = -EBUSY;
		break;
	/* No acknowledgment frames were received during all retry
	 * attempts
	 */
	case RF2XX_TRX_PHY_STATE_TRAC_NO_ACK:
		response = -EAGAIN;
		break;
	/* Transaction not yet finished */
	case RF2XX_TRX_PHY_STATE_TRAC_INVALID:
		response = -EINTR;
		break;
	/* RF2XX_TRX_PHY_STATE_TRAC_SUCCESS:
	 *  The transaction was responded to by a valid ACK, or, if no
	 *  ACK is requested, after a successful frame transmission.
	 *
	 * RF2XX_TRX_PHY_STATE_TRAC_SUCCESS_DATA_PENDING:
	 * Equivalent to SUCCESS and indicating that the “Frame
	 * Pending” bit (see Section 8.1.2.2) of the received
	 * acknowledgment frame was set.
	 */
	default:
		rf2xx_handle_ack(ctx, frag);
		break;
	}

	return response;
}

static int rf2xx_start(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;

	LOG_DBG("Start");

	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);
	gpio_pin_interrupt_configure_dt(&conf->irq_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
	rf2xx_trx_set_rx_state(dev);

	return 0;
}

static int rf2xx_stop(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;

	LOG_DBG("Stop");

	gpio_pin_interrupt_configure_dt(&conf->irq_gpio, GPIO_INT_DISABLE);
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);

	return 0;
}

static int rf2xx_pan_coord_set(const struct device *dev, bool pan_coordinator)
{
	uint8_t reg;

	if (pan_coordinator) {
		reg = rf2xx_iface_reg_read(dev, RF2XX_CSMA_SEED_1_REG);
		reg |= (1 << RF2XX_AACK_I_AM_COORD);
		rf2xx_iface_reg_write(dev, RF2XX_CSMA_SEED_1_REG, reg);
	} else {
		reg = rf2xx_iface_reg_read(dev, RF2XX_CSMA_SEED_1_REG);
		reg &= ~(1 << RF2XX_AACK_I_AM_COORD);
		rf2xx_iface_reg_write(dev, RF2XX_CSMA_SEED_1_REG, reg);
	}

	return 0;
}

static int rf2xx_promiscuous_set(const struct device *dev, bool promiscuous)
{
	struct rf2xx_context *ctx = dev->data;
	uint8_t reg;

	ctx->promiscuous = promiscuous;

	if (promiscuous) {
		reg = rf2xx_iface_reg_read(dev, RF2XX_XAH_CTRL_1_REG);
		reg |= (1 << RF2XX_AACK_PROM_MODE);
		rf2xx_iface_reg_write(dev, RF2XX_XAH_CTRL_1_REG, reg);

		reg = rf2xx_iface_reg_read(dev, RF2XX_CSMA_SEED_1_REG);
		reg |= (1 << RF2XX_AACK_DIS_ACK);
		rf2xx_iface_reg_write(dev, RF2XX_CSMA_SEED_1_REG, reg);
	} else {
		reg = rf2xx_iface_reg_read(dev, RF2XX_XAH_CTRL_1_REG);
		reg &= ~(1 << RF2XX_AACK_PROM_MODE);
		rf2xx_iface_reg_write(dev, RF2XX_XAH_CTRL_1_REG, reg);

		reg = rf2xx_iface_reg_read(dev, RF2XX_CSMA_SEED_1_REG);
		reg &= ~(1 << RF2XX_AACK_DIS_ACK);
		rf2xx_iface_reg_write(dev, RF2XX_CSMA_SEED_1_REG, reg);
	}

	return 0;
}

int rf2xx_configure(const struct device *dev,
		    enum ieee802154_config_type type,
		    const struct ieee802154_config *config)
{
	int ret = -EINVAL;

	LOG_DBG("Configure %d", type);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
	case IEEE802154_CONFIG_ACK_FPB:
		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		ret = rf2xx_pan_coord_set(dev, config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		ret = rf2xx_promiscuous_set(dev, config->promiscuous);
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
	default:
		break;
	}

	return ret;
}

static int rf2xx_attr_get(const struct device *dev, enum ieee802154_attr attr,
			   struct ieee802154_attr_value *value)
{
	struct rf2xx_context *ctx = dev->data;

	switch (attr) {
	case IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES:
		value->phy_supported_channel_pages = ctx->cc_page;
		return 0;

	case IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES:
		value->phy_supported_channels = &ctx->cc_channels;
		return 0;

	default:
		return -ENOENT;
	}
}

static int power_on_and_setup(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;
	uint8_t config;

	rf2xx_iface_phy_rst(dev);

	/* Sync transceiver state */
	do {
		rf2xx_iface_reg_write(dev, RF2XX_TRX_STATE_REG,
				      RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	} while (RF2XX_TRX_PHY_STATUS_TRX_OFF !=
		 (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
		  RF2XX_TRX_PHY_STATUS_MASK));

	/* get device identification */
	ctx->trx_model = rf2xx_iface_reg_read(dev, RF2XX_PART_NUM_REG);
	ctx->trx_version = rf2xx_iface_reg_read(dev, RF2XX_VERSION_NUM_REG);

	/**
	 * Valid transceiver are:
	 *  231-Rev-A (Version 0x02)
	 *  232-Rev-A (Version 0x02)
	 *  233-Rev-A (Version 0x01) (Warning)
	 *  233-Rev-B (Version 0x02)
	 */
	if (ctx->trx_model <= RF2XX_TRX_MODEL_230) {
		LOG_DBG("Invalid or not supported transceiver");
		return -ENODEV;
	}

	if (ctx->trx_model == RF2XX_TRX_MODEL_233 && ctx->trx_version == 0x01) {
		LOG_DBG("Transceiver is old and unstable release");
	}

	/* Set RSSI base */
	if (ctx->trx_model == RF2XX_TRX_MODEL_212) {
		ctx->trx_rssi_base = -100;
	} else if (ctx->trx_model == RF2XX_TRX_MODEL_233) {
		ctx->trx_rssi_base = -94;
	} else if (ctx->trx_model == RF2XX_TRX_MODEL_231) {
		ctx->trx_rssi_base = -91;
	} else {
		ctx->trx_rssi_base = -90;
	}

	/* Disable All Features of TRX_CTRL_0 */
	config = 0;
	rf2xx_iface_reg_write(dev, RF2XX_TRX_CTRL_0_REG, config);

	/* Configure PHY behaviour */
	config = (1 << RF2XX_TX_AUTO_CRC_ON) |
		 (3 << RF2XX_SPI_CMD_MODE) |
		 (1 << RF2XX_IRQ_MASK_MODE);
	rf2xx_iface_reg_write(dev, RF2XX_TRX_CTRL_1_REG, config);

	config = (1 << RF2XX_RX_SAFE_MODE);
	if (ctx->trx_model != RF2XX_TRX_MODEL_232) {
		config |= (1 << RF2XX_OQPSK_SCRAM_EN);
	}
	rf2xx_iface_reg_write(dev, RF2XX_TRX_CTRL_2_REG, config);

	if (ctx->trx_model == RF2XX_TRX_MODEL_212) {
		rf2xx_configure_trx_path(dev);
		rf2xx_iface_reg_write(dev, RF2XX_CC_CTRL_1_REG, 0);
	}

	ctx->tx_mode = IEEE802154_TX_MODE_CSMA_CA;
	ctx->promiscuous = false;

	/* Configure INT behaviour */
	config = (1 << RF2XX_RX_START) |
		 (1 << RF2XX_TRX_END);
	rf2xx_iface_reg_write(dev, RF2XX_IRQ_MASK_REG, config);

	gpio_init_callback(&ctx->irq_cb, trx_isr_handler,
			   BIT(conf->irq_gpio.pin));

	if (gpio_add_callback(conf->irq_gpio.port, &ctx->irq_cb) < 0) {
		LOG_ERR("Could not set IRQ callback.");
		return -ENXIO;
	}

	return 0;
}

static inline int configure_gpios(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;

	/* Chip IRQ line */
	if (!gpio_is_ready_dt(&conf->irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&conf->irq_gpio, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&conf->irq_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	/* Chip RESET line */
	if (!gpio_is_ready_dt(&conf->reset_gpio)) {
		LOG_ERR("RESET GPIO device not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&conf->reset_gpio, GPIO_OUTPUT_INACTIVE);

	/* Chip SLPTR line */
	if (!gpio_is_ready_dt(&conf->slptr_gpio)) {
		LOG_ERR("SLPTR GPIO device not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&conf->slptr_gpio, GPIO_OUTPUT_INACTIVE);

	/* Chip DIG2 line (Optional feature) */
	if (conf->dig2_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&conf->dig2_gpio)) {
			LOG_ERR("DIG2 GPIO device not ready");
			return -ENODEV;
		}
		LOG_INF("Optional instance of %s device activated",
			conf->dig2_gpio.port->name);
		gpio_pin_configure_dt(&conf->dig2_gpio, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&conf->dig2_gpio,
						GPIO_INT_EDGE_TO_ACTIVE);
	}

	/* Chip CLKM line (Optional feature) */
	if (conf->clkm_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&conf->clkm_gpio)) {
			LOG_ERR("CLKM GPIO device not ready");
			return -ENODEV;
		}
		LOG_INF("Optional instance of %s device activated",
			conf->clkm_gpio.port->name);
		gpio_pin_configure_dt(&conf->clkm_gpio, GPIO_INPUT);
	}

	return 0;
}

static inline int configure_spi(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;

	if (!spi_is_ready_dt(&conf->spi)) {
		LOG_ERR("SPI bus %s is not ready",
			conf->spi.bus->name);
		return -ENODEV;
	}

	return 0;
}

static int rf2xx_init(const struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	const struct rf2xx_config *conf = dev->config;
	char thread_name[20];

	LOG_DBG("\nInitialize RF2XX Transceiver\n");

	ctx->dev = dev;

	k_sem_init(&ctx->trx_tx_sync, 0, 1);
	k_sem_init(&ctx->trx_isr_lock, 0, 1);

	if (configure_gpios(dev) != 0) {
		LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		LOG_ERR("Configuring RF2XX failed");
		return -EIO;
	}

	LOG_DBG("RADIO configured");

	k_thread_create(&ctx->trx_thread,
			ctx->trx_stack,
			CONFIG_IEEE802154_RF2XX_RX_STACK_SIZE,
			rf2xx_thread_main,
			ctx, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	snprintk(thread_name, sizeof(thread_name),
		 "rf2xx_trx [%d]", conf->inst);
	k_thread_name_set(&ctx->trx_thread, thread_name);

	LOG_DBG("Thread OK");

	return 0;
}

static void rf2xx_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct rf2xx_context *ctx = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	ctx->iface = iface;

	if (ctx->trx_model == RF2XX_TRX_MODEL_212) {
		if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915 ||
		    ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915) {
			ctx->cc_range.from_channel = 0U;
			ctx->cc_range.to_channel = 10U;
		} else if (ctx->cc_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_FIVE_OQPSK_780) {
			ctx->cc_range.from_channel = 0U;
			ctx->cc_range.to_channel = 3U;
		} else {
			__ASSERT(false, "Unsupported channel page %u.", ctx->cc_page);
		}
	} else {
		__ASSERT(ctx->cc_page ==
				 IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
			 "Unsupported channel page %u.", ctx->cc_page);
		ctx->cc_range.from_channel = 11U;
		ctx->cc_range.to_channel = 26U;
	}

	ieee802154_init(iface);
}

static const struct ieee802154_radio_api rf2xx_radio_api = {
	.iface_api.init		= rf2xx_iface_init,

	.get_capabilities	= rf2xx_get_capabilities,
	.cca			= rf2xx_cca,
	.set_channel		= rf2xx_set_channel,
	.filter			= rf2xx_filter,
	.set_txpower		= rf2xx_set_txpower,
	.tx			= rf2xx_tx,
	.start			= rf2xx_start,
	.stop			= rf2xx_stop,
	.configure		= rf2xx_configure,
	.attr_get		= rf2xx_attr_get,
};

#if !defined(CONFIG_IEEE802154_RAW_MODE)
    #if defined(CONFIG_NET_L2_IEEE802154)
	#define L2 IEEE802154_L2
	#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
	#define MTU RF2XX_MAX_PSDU_LENGTH
    #elif defined(CONFIG_NET_L2_OPENTHREAD)
	#define L2 OPENTHREAD_L2
	#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
	#define MTU RF2XX_OT_PSDU_LENGTH
    #endif
#endif /* CONFIG_IEEE802154_RAW_MODE */

#define DRV_INST_LOCAL_MAC_ADDRESS(n)					\
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, local_mac_address),		\
		 UTIL_AND(DT_INST_PROP_LEN(n, local_mac_address) == 8,	\
			  DT_INST_PROP(n, local_mac_address)))

#define IEEE802154_RF2XX_DEVICE_CONFIG(n)				  \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, tx_pwr_min) == 2,		  \
	"rf2xx: Error TX-PWR-MIN len is different of two");		  \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, tx_pwr_max) == 2,		  \
	"rf2xx: Error TX-PWR-MAX len is different of two");		  \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, tx_pwr_table) != 0,		  \
	"rf2xx: Error TX-PWR-TABLE len must be greater than zero");	  \
	static const uint8_t rf2xx_pwr_table_##n[] =			  \
		DT_INST_PROP_OR(n, tx_pwr_table, 0);			  \
	static const struct rf2xx_config rf2xx_ctx_config_##n = {	  \
		.inst = n,						  \
		.has_mac = DT_INST_NODE_HAS_PROP(n, local_mac_address),   \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios),	  \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),	  \
		.slptr_gpio = GPIO_DT_SPEC_INST_GET(n, slptr_gpios),	  \
		.dig2_gpio = GPIO_DT_SPEC_INST_GET_OR(n, dig2_gpios, {}), \
		.clkm_gpio = GPIO_DT_SPEC_INST_GET_OR(n, clkm_gpios, {}), \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8) |	  \
				 SPI_TRANSFER_MSB, 0),			  \
									  \
		.tx_pwr_min = DT_INST_PROP_OR(n, tx_pwr_min, 0),	  \
		.tx_pwr_max = DT_INST_PROP_OR(n, tx_pwr_max, 0),	  \
		.tx_pwr_table = rf2xx_pwr_table_##n,			  \
		.tx_pwr_table_size = DT_INST_PROP_LEN(n, tx_pwr_table),	  \
	}

#define IEEE802154_RF2XX_DEVICE_DATA(n)                                 \
	static struct rf2xx_context rf2xx_ctx_data_##n = {              \
		.mac_addr = { DRV_INST_LOCAL_MAC_ADDRESS(n) },          \
		.cc_page = BIT(DT_INST_ENUM_IDX_OR(n, channel_page, 0)),\
		.cc_channels = {                                        \
			.ranges = &rf2xx_ctx_data_##n.cc_range,         \
			.num_ranges = 1U,                               \
		}                                                       \
	}

#define IEEE802154_RF2XX_RAW_DEVICE_INIT(n)	   \
	DEVICE_DT_INST_DEFINE(			   \
		n,				   \
		&rf2xx_init,			   \
		NULL,				   \
		&rf2xx_ctx_data_##n,		   \
		&rf2xx_ctx_config_##n,		   \
		POST_KERNEL,			   \
		CONFIG_IEEE802154_RF2XX_INIT_PRIO, \
		&rf2xx_radio_api)

#define IEEE802154_RF2XX_NET_DEVICE_INIT(n)	   \
	NET_DEVICE_DT_INST_DEFINE(		   \
		n,				   \
		&rf2xx_init,			   \
		NULL,				   \
		&rf2xx_ctx_data_##n,		   \
		&rf2xx_ctx_config_##n,		   \
		CONFIG_IEEE802154_RF2XX_INIT_PRIO, \
		&rf2xx_radio_api,		   \
		L2,				   \
		L2_CTX_TYPE,			   \
		MTU)

#define IEEE802154_RF2XX_INIT(inst)				\
	IEEE802154_RF2XX_DEVICE_CONFIG(inst);			\
	IEEE802154_RF2XX_DEVICE_DATA(inst);			\
								\
	COND_CODE_1(CONFIG_IEEE802154_RAW_MODE,			\
		    (IEEE802154_RF2XX_RAW_DEVICE_INIT(inst);),	\
		    (IEEE802154_RF2XX_NET_DEVICE_INIT(inst);))

DT_INST_FOREACH_STATUS_OKAY(IEEE802154_RF2XX_INIT)
