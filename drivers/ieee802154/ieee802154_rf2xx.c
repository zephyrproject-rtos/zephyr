/* ieee802154_rf2xx.c - ATMEL RF2XX IEEE 802.15.4 Driver */

#define DT_DRV_COMPAT atmel_rf2xx

/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_rf2xx
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include <kernel.h>
#include <arch/cpu.h>
#include <debug/stack.h>

#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <sys/byteorder.h>
#include <string.h>
#include <random/rand32.h>
#include <linker/sections.h>
#include <sys/atomic.h>

#include <drivers/spi.h>
#include <drivers/gpio.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_rf2xx.h"
#include "ieee802154_rf2xx_regs.h"
#include "ieee802154_rf2xx_iface.h"

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <net/openthread.h>

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
	.ieee802154_lqi = 80,
	.ieee802154_rssi = -40,
};
#endif /* CONFIG_NET_L2_OPENTHREAD */

/**
 * RF output power for RF2xx
 *
 * The table below is exact for RF233. For RF231/2 the TX power might
 * be a bit off, but good enough.
 *
 * RF233: http://ww1.microchip.com/downloads/en/devicedoc/atmel-8351-mcu_wireless-at86rf233_datasheet.pdf
 * 9.2.5 Register Description Register 0x05 (PHY_TX_PWR)
 * 0x0 = 4dBm .. 0xF = -17dBm
 *
 * RF232: http://ww1.microchip.com/downloads/en/DeviceDoc/doc8321.pdf
 * 9.2.5 Register Description Register 0x05 (PHY_TX_PWR)
 * 0x0 = 3dBm .. 0xF = -17dBm
 *
 * RF231: http://ww1.microchip.com/downloads/en/DeviceDoc/doc8111.pdf
 * 9.2.5 Register Description Register 0x05 (PHY_TX_PWR)
 * 0x0 = 3dBm .. 0xF = -17dBm
 */

#define RF2XX_OUTPUT_POWER_MAX		4
#define RF2XX_OUTPUT_POWER_MIN		(-17)

/* Lookup table for PHY_TX_PWR register for RF233 */
static const uint8_t phy_tx_pwr_lt[] = {
	0xf,                     /* -17  dBm: -17 */
	0xe, 0xe, 0xe, 0xe, 0xe, /* -12  dBm: -16, -15, -14, -13, -12 */
	0xd, 0xd, 0xd, 0xd,      /* -8   dBm: -11, -10, -9, -8 */
	0xc, 0xc,                /* -6   dBm: -7, -6 */
	0xb, 0xb,                /* -4   dBm: -5, -4 */
	0xa,                     /* -3   dBm: -3 */
	0x9,                     /* -2   dBm: -2 */
	0x8,                     /* -1   dBm: -1 */
	0x7,                     /*  0.0 dBm:  0 */
	0x6,                     /*  1   dBm:  1 */
	0x5,                     /*  2   dBm:  2 */
	/* 0x4, */               /*  2.5 dBm */
	0x3,                     /*  3   dBm:  3 */
	/* 0x2, */               /*  3.4 dBm */
	/* 0x1, */               /*  3.7 dBm */
	0x0                      /*  4   dBm: 4 */
};

/* Radio Transceiver ISR */
static inline void trx_isr_handler(struct device *port,
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

static void rf2xx_trx_set_state(struct device *dev,
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

static void rf2xx_trx_set_tx_state(struct device *dev)
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

static void rf2xx_trx_set_rx_state(struct device *dev)
{
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);
	/**
	 * Set extended RX mode
	 * Datasheet: chapter 7.2 Extended Operating Mode
	 */
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_RX_AACK_ON);
}

static void rf2xx_trx_rx(struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	struct net_pkt *pkt = NULL;
	uint8_t rx_buf[RX2XX_MAX_FRAME_SIZE];
	uint8_t pkt_len;
	uint8_t frame_len;
	uint8_t trac;

	/*
	 * The rf2xx frame buffer can have length > 128 bytes. The
	 * net_pkt_alloc_with_buffer allocates max value of 128 bytes.
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

	if (pkt_len < RX2XX_FRAME_MIN_PHR_SIZE) {
		LOG_ERR("invalid RX frame length");
		return;
	}

	frame_len = RX2XX_FRAME_HEADER_SIZE + pkt_len +
		    RX2XX_FRAME_FOOTER_SIZE;

	rf2xx_iface_frame_read(dev, rx_buf, frame_len);

	trac = rx_buf[pkt_len + RX2XX_FRAME_TRAC_INDEX];
	trac = (trac >> RF2XX_RX_TRAC_STATUS) & RF2XX_RX_TRAC_BIT_MASK;

	if (trac == RF2XX_TRX_PHY_STATE_TRAC_INVALID) {
		LOG_ERR("invalid RX frame");

		return;
	}

	ctx->pkt_lqi = rx_buf[pkt_len + RX2XX_FRAME_LQI_INDEX];
	ctx->pkt_ed = rx_buf[pkt_len + RX2XX_FRAME_ED_INDEX];

	if (!IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) &&
	    !IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
		pkt_len -= RX2XX_FRAME_FCS_LENGTH;
	}

	pkt = net_pkt_alloc_with_buffer(ctx->iface, pkt_len,
					AF_UNSPEC, 0, K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("No buf available");
		return;
	}

	memcpy(pkt->buffer->data, rx_buf + RX2XX_FRAME_HEADER_SIZE, pkt_len);
	net_buf_add(pkt->buffer, pkt_len);
	net_pkt_set_ieee802154_lqi(pkt, ctx->pkt_lqi);
	net_pkt_set_ieee802154_rssi(pkt, ctx->pkt_ed + ctx->trx_rssi_base);

	LOG_DBG("Caught a packet (%02X) (LQI: %02X, RSSI: %d, ED: %02X)",
		pkt_len, ctx->pkt_lqi, ctx->trx_rssi_base + ctx->pkt_ed,
		ctx->pkt_ed);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		LOG_DBG("Packet dropped by NET stack");
		net_pkt_unref(pkt);
		return;
	}

	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		log_stack_usage(&ctx->trx_thread);
	}
}

static void rf2xx_process_rx_frame(struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;

	if (ctx->trx_model != RF2XX_TRX_MODEL_231) {
		rf2xx_trx_rx(dev);
	} else {
		/* Ensures that automatically ACK will be sent
		 * when requested
		 */
		while (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) ==
		       RF2XX_TRX_PHY_STATUS_BUSY_RX_AACK) {
			;
		};

		/* Set PLL_ON to avoid transceiver receive
		 * new data until finish reading process
		 */
		rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_PLL_ON);
		rf2xx_trx_rx(dev);
		rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_RX_AACK_ON);
	}
}

static void rf2xx_process_tx_frame(struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;

	ctx->trx_trac = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATE_REG) >>
			 RF2XX_TRAC_STATUS) & 7;
	k_sem_give(&ctx->trx_tx_sync);
	rf2xx_trx_set_rx_state(dev);
}

static void rf2xx_process_trx_end(struct device *dev)
{
	uint8_t trx_status = (rf2xx_iface_reg_read(dev, RF2XX_TRX_STATUS_REG) &
			   RF2XX_TRX_PHY_STATUS_MASK);

	if (trx_status == RF2XX_TRX_PHY_STATUS_TX_ARET_ON) {
		rf2xx_process_tx_frame(dev);
	} else {
		rf2xx_process_rx_frame(dev);
	}
}

static void rf2xx_thread_main(void *arg)
{
	struct device *dev = INT_TO_POINTER(arg);
	struct rf2xx_context *ctx = dev->data;
	uint8_t isr_status;

	while (true) {
		k_sem_take(&ctx->trx_isr_lock, K_FOREVER);

		isr_status = rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);

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
				rf2xx_iface_sram_read(dev, 0, &ctx->rx_phr, 1);
			}
		} else if (isr_status & (1 << RF2XX_TRX_END)) {
			rf2xx_process_trx_end(dev);
		}
	}
}

static inline uint8_t *get_mac(struct device *dev)
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

static enum ieee802154_hw_caps rf2xx_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return IEEE802154_HW_FCS |
	       IEEE802154_HW_PROMISC |
	       IEEE802154_HW_FILTER |
	       IEEE802154_HW_CSMA |
	       IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_2_4_GHZ;
}

static int rf2xx_cca(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int rf2xx_set_channel(struct device *dev, uint16_t channel)
{
	uint8_t reg;

	if (channel < 11 || channel > 26) {
		LOG_ERR("Unsupported channel %u", channel);
		return -EINVAL;
	}

	reg = rf2xx_iface_reg_read(dev, RF2XX_PHY_CC_CCA_REG) & ~0x1f;
	rf2xx_iface_reg_write(dev, RF2XX_PHY_CC_CCA_REG, reg | channel);

	return 0;
}

static int rf2xx_set_txpower(struct device *dev, int16_t dbm)
{
	if (dbm < RF2XX_OUTPUT_POWER_MIN) {
		LOG_INF("TX-power %d dBm below min of %d dBm, using %d dBm",
			dbm,
			RF2XX_OUTPUT_POWER_MIN,
			RF2XX_OUTPUT_POWER_MAX);
		dbm = RF2XX_OUTPUT_POWER_MIN;
	} else if (dbm > RF2XX_OUTPUT_POWER_MAX) {
		LOG_INF("TX-power %d dBm above max of %d dBm, using %d dBm",
			dbm,
			RF2XX_OUTPUT_POWER_MIN,
			RF2XX_OUTPUT_POWER_MAX);
		dbm = RF2XX_OUTPUT_POWER_MAX;
	}

	rf2xx_iface_reg_write(dev, RF2XX_PHY_TX_PWR_REG,
		phy_tx_pwr_lt[dbm - RF2XX_OUTPUT_POWER_MIN]);

	return 0;
}

static int rf2xx_set_ieee_addr(struct device *dev, bool set,
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

static int rf2xx_set_short_addr(struct device *dev, bool set,
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

static int rf2xx_set_pan_id(struct device *dev, bool set, uint16_t pan_id)
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

static int rf2xx_filter(struct device *dev,
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

	if (ieee802154_radio_handle_ack(ctx->iface, &rf2xx_ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled.");
	}
}
#else
	#define rf2xx_handle_ack(...)
#endif

static int rf2xx_tx(struct device *dev,
		    enum ieee802154_tx_mode mode,
		    struct net_pkt *pkt,
		    struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	struct rf2xx_context *ctx = dev->data;
	int response = 0;

	if (mode != IEEE802154_TX_MODE_CSMA_CA) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
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

static int rf2xx_start(struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;

	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);
	gpio_pin_interrupt_configure(ctx->irq_gpio, conf->irq.pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
	rf2xx_trx_set_rx_state(dev);

	return 0;
}

static int rf2xx_stop(struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;

	gpio_pin_interrupt_configure(ctx->irq_gpio, conf->irq.pin,
				     GPIO_INT_DISABLE);
	rf2xx_trx_set_state(dev, RF2XX_TRX_PHY_STATE_CMD_TRX_OFF);
	rf2xx_iface_reg_read(dev, RF2XX_IRQ_STATUS_REG);

	return 0;
}

int rf2xx_configure(struct device *dev, enum ieee802154_config_type type,
		    const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return 0;
}

static int power_on_and_setup(struct device *dev)
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
	if (ctx->trx_model != RF2XX_TRX_MODEL_231 &&
	    ctx->trx_model != RF2XX_TRX_MODEL_232 &&
	    ctx->trx_model != RF2XX_TRX_MODEL_233) {
		LOG_DBG("Invalid or not supported transceiver");
		return -ENODEV;
	}

	if (ctx->trx_version < 0x02) {
		LOG_DBG("Transceiver is old and unstable release");
	}

	/* Set RSSI base */
	if (ctx->trx_model == RF2XX_TRX_MODEL_233) {
		ctx->trx_rssi_base = -94;
	} else if (ctx->trx_model == RF2XX_TRX_MODEL_231) {
		ctx->trx_rssi_base = -91;
	} else {
		ctx->trx_rssi_base = -90;
	}

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

	/* Configure INT behaviour */
	config = (1 << RF2XX_RX_START) |
		 (1 << RF2XX_TRX_END);
	rf2xx_iface_reg_write(dev, RF2XX_IRQ_MASK_REG, config);

	gpio_init_callback(&ctx->irq_cb, trx_isr_handler, BIT(conf->irq.pin));
	gpio_add_callback(ctx->irq_gpio, &ctx->irq_cb);

	return 0;
}

static inline int configure_gpios(struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	struct rf2xx_context *ctx = dev->data;

	/* Chip IRQ line */
	ctx->irq_gpio = device_get_binding(conf->irq.devname);
	if (ctx->irq_gpio == NULL) {
		LOG_ERR("Failed to get instance of %s device",
			conf->irq.devname);
		return -EINVAL;
	}
	gpio_pin_configure(ctx->irq_gpio, conf->irq.pin, conf->irq.flags |
			   GPIO_INPUT);
	gpio_pin_interrupt_configure(ctx->irq_gpio, conf->irq.pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	/* Chip RESET line */
	ctx->reset_gpio = device_get_binding(conf->reset.devname);
	if (ctx->reset_gpio == NULL) {
		LOG_ERR("Failed to get instance of %s device",
			conf->reset.devname);
		return -EINVAL;
	}
	gpio_pin_configure(ctx->reset_gpio, conf->reset.pin, conf->reset.flags |
			   GPIO_OUTPUT_INACTIVE);

	/* Chip SLPTR line */
	ctx->slptr_gpio = device_get_binding(conf->slptr.devname);
	if (ctx->slptr_gpio == NULL) {
		LOG_ERR("Failed to get instance of %s device",
			conf->slptr.devname);
		return -EINVAL;
	}
	gpio_pin_configure(ctx->slptr_gpio, conf->slptr.pin, conf->slptr.flags |
			   GPIO_OUTPUT_INACTIVE);

	/* Chip DIG2 line (Optional feature) */
	ctx->dig2_gpio = device_get_binding(conf->dig2.devname);
	if (ctx->dig2_gpio != NULL) {
		LOG_INF("Optional instance of %s device activated",
			conf->dig2.devname);
		gpio_pin_configure(ctx->dig2_gpio, conf->dig2.pin,
				   conf->dig2.flags | GPIO_INPUT);
		gpio_pin_interrupt_configure(ctx->dig2_gpio, conf->dig2.pin,
					     GPIO_INT_EDGE_TO_ACTIVE);
	}

	/* Chip CLKM line (Optional feature) */
	ctx->clkm_gpio = device_get_binding(conf->clkm.devname);
	if (ctx->clkm_gpio != NULL) {
		LOG_INF("Optional instance of %s device activated",
			conf->clkm.devname);
		gpio_pin_configure(ctx->clkm_gpio, conf->clkm.pin,
				   conf->clkm.flags | GPIO_INPUT);
	}

	return 0;
}

static inline int configure_spi(struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	const struct rf2xx_config *conf = dev->config;

	/* Get SPI Driver Instance*/
	ctx->spi = device_get_binding(conf->spi.devname);
	if (!ctx->spi) {
		LOG_ERR("Failed to get instance of %s device",
			conf->spi.devname);

		return -ENODEV;
	}

	/* Apply SPI Config: 8-bit, MSB First, MODE-0 */
	ctx->spi_cfg.operation = SPI_WORD_SET(8) |
				 SPI_TRANSFER_MSB;
	ctx->spi_cfg.slave = conf->spi.addr;
	ctx->spi_cfg.frequency = conf->spi.freq;
	ctx->spi_cfg.cs = NULL;

	/*
	 * Get SPI Chip Select Instance
	 *
	 * This is an optinal feature configured on DTS. Some SPI controllers
	 * automatically set CS line by device slave address. Check your SPI
	 * device driver to understand if you need this option enabled.
	 */
	ctx->spi_cs.gpio_dev = device_get_binding(conf->spi.cs.devname);
	if (ctx->spi_cs.gpio_dev) {
		ctx->spi_cs.gpio_pin = conf->spi.cs.pin;
		ctx->spi_cs.gpio_dt_flags = conf->spi.cs.flags;
		ctx->spi_cs.delay = 0U;

		ctx->spi_cfg.cs = &ctx->spi_cs;

		LOG_DBG("SPI GPIO CS configured on %s:%u",
			conf->spi.cs.devname, conf->spi.cs.pin);
	}

	return 0;
}

static int rf2xx_init(struct device *dev)
{
	struct rf2xx_context *ctx = dev->data;
	const struct rf2xx_config *conf = dev->config;
	char thread_name[20];

	LOG_DBG("\nInitialize RF2XX Transceiver\n");

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

	k_thread_create(&ctx->trx_thread,
			ctx->trx_stack,
			CONFIG_IEEE802154_RF2XX_RX_STACK_SIZE,
			(k_thread_entry_t) rf2xx_thread_main,
			dev, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	snprintk(thread_name, sizeof(thread_name),
		 "rf2xx_trx [%d]", conf->inst);
	k_thread_name_set(&ctx->trx_thread, thread_name);

	return 0;
}

static void rf2xx_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct rf2xx_context *ctx = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	ctx->iface = iface;

	ieee802154_init(iface);
}

static struct ieee802154_radio_api rf2xx_radio_api = {
	.iface_api.init   = rf2xx_iface_init,

	.get_capabilities = rf2xx_get_capabilities,
	.cca              = rf2xx_cca,
	.set_channel      = rf2xx_set_channel,
	.filter           = rf2xx_filter,
	.set_txpower      = rf2xx_set_txpower,
	.tx               = rf2xx_tx,
	.start            = rf2xx_start,
	.stop             = rf2xx_stop,
	.configure        = rf2xx_configure,
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

/*
 * Optional features place holders, get a 0 if the "gpio" doesn't exist
 */

#define DRV_INST_GPIO_LABEL(n, gpio_pha)				\
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, gpio_pha),			\
		 DT_INST_GPIO_LABEL(n, gpio_pha))

#define DRV_INST_GPIO_PIN(n, gpio_pha)					\
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, gpio_pha),			\
		 DT_INST_GPIO_PIN(n, gpio_pha))

#define DRV_INST_GPIO_FLAGS(n, gpio_pha)				\
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, gpio_pha),			\
		 DT_INST_GPIO_FLAGS(n, gpio_pha))

#define DRV_INST_SPI_DEV_CS_GPIOS_LABEL(n)				\
	UTIL_AND(DT_INST_SPI_DEV_HAS_CS_GPIOS(n),			\
		 DT_INST_SPI_DEV_CS_GPIOS_LABEL(n))

#define DRV_INST_SPI_DEV_CS_GPIOS_PIN(n)				\
	UTIL_AND(DT_INST_SPI_DEV_HAS_CS_GPIOS(n),			\
		 DT_INST_SPI_DEV_CS_GPIOS_PIN(n))

#define DRV_INST_SPI_DEV_CS_GPIOS_FLAGS(n)				\
	UTIL_AND(DT_INST_SPI_DEV_HAS_CS_GPIOS(n),			\
		 DT_INST_SPI_DEV_CS_GPIOS_FLAGS(n))

#define DRV_INST_LOCAL_MAC_ADDRESS(n)					\
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, local_mac_address),		\
		 UTIL_AND(DT_INST_PROP_LEN(n, local_mac_address) == 8,	\
			  DT_INST_PROP(n, local_mac_address)))

#define IEEE802154_RF2XX_DEVICE_CONFIG(n)				\
	static const struct rf2xx_config rf2xx_ctx_config_##n = {	\
		.inst = n,						\
		.has_mac = DT_INST_NODE_HAS_PROP(n, local_mac_address), \
									\
		.irq.devname = DRV_INST_GPIO_LABEL(n, irq_gpios),	\
		.irq.pin = DRV_INST_GPIO_PIN(n, irq_gpios),		\
		.irq.flags = DRV_INST_GPIO_FLAGS(n, irq_gpios),		\
									\
		.reset.devname = DRV_INST_GPIO_LABEL(n, reset_gpios),	\
		.reset.pin = DRV_INST_GPIO_PIN(n, reset_gpios),		\
		.reset.flags = DRV_INST_GPIO_FLAGS(n, reset_gpios),	\
									\
		.slptr.devname = DRV_INST_GPIO_LABEL(n, slptr_gpios),	\
		.slptr.pin = DRV_INST_GPIO_PIN(n, slptr_gpios),		\
		.slptr.flags = DRV_INST_GPIO_FLAGS(n, slptr_gpios),	\
									\
		.dig2.devname = DRV_INST_GPIO_LABEL(n, dig2_gpios),	\
		.dig2.pin = DRV_INST_GPIO_PIN(n, dig2_gpios),		\
		.dig2.flags = DRV_INST_GPIO_FLAGS(n, dig2_gpios),	\
									\
		.clkm.devname = DRV_INST_GPIO_LABEL(n, clkm_gpios),	\
		.clkm.pin = DRV_INST_GPIO_PIN(n, clkm_gpios),		\
		.clkm.flags = DRV_INST_GPIO_FLAGS(n, clkm_gpios),	\
									\
		.spi.devname = DT_INST_BUS_LABEL(n),			\
		.spi.addr = DT_INST_REG_ADDR(n),			\
		.spi.freq = DT_INST_PROP(n, spi_max_frequency),		\
		.spi.cs.devname = DRV_INST_SPI_DEV_CS_GPIOS_LABEL(n),	\
		.spi.cs.pin = DRV_INST_SPI_DEV_CS_GPIOS_PIN(n),		\
		.spi.cs.flags = DRV_INST_SPI_DEV_CS_GPIOS_FLAGS(n),	\
	}

#define IEEE802154_RF2XX_DEVICE_DATA(n)                                \
	static struct rf2xx_context rf2xx_ctx_data_##n = {              \
		.mac_addr = DRV_INST_LOCAL_MAC_ADDRESS(n)               \
	}

#define IEEE802154_RF2XX_RAW_DEVICE_INIT(n)	   \
	DEVICE_AND_API_INIT(			   \
		rf2xx_##n,			   \
		DT_INST_LABEL(n),		   \
		&rf2xx_init,			   \
		&rf2xx_ctx_data_##n,		   \
		&rf2xx_ctx_config_##n,		   \
		POST_KERNEL,			   \
		CONFIG_IEEE802154_RF2XX_INIT_PRIO, \
		&rf2xx_radio_api)

#define IEEE802154_RF2XX_NET_DEVICE_INIT(n)	   \
	NET_DEVICE_INIT(			   \
		rf2xx_##n,			   \
		DT_INST_LABEL(n),		   \
		&rf2xx_init,			   \
		device_pm_control_nop,		   \
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
