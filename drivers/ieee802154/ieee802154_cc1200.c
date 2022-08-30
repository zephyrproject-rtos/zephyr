/* ieee802154_cc1200.c - TI CC1200 driver */

#define DT_DRV_COMPAT ti_cc1200

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_cc1200
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

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/ieee802154_radio.h>

#include "ieee802154_cc1200.h"
#include "ieee802154_cc1200_rf.h"

/* ToDo: supporting 802.15.4g will require GPIO2
 * used as CC1200_GPIO_SIG_RXFIFO_THR
 *
 * Note: GPIO3 is unused.
 */
#define CC1200_IOCFG3	CC1200_GPIO_SIG_MARC_2PIN_STATUS_0
#define CC1200_IOCFG2	CC1200_GPIO_SIG_MARC_2PIN_STATUS_1
#define CC1200_IOCFG0	CC1200_GPIO_SIG_PKT_SYNC_RXTX

/***********************
 * Debugging functions *
 **********************/
static void cc1200_print_status(uint8_t status)
{
	if (status == CC1200_STATUS_IDLE) {
		LOG_DBG("Idling");
	} else if (status == CC1200_STATUS_RX) {
		LOG_DBG("Receiving");
	} else if (status == CC1200_STATUS_TX) {
		LOG_DBG("Transmitting");
	} else if (status == CC1200_STATUS_FSTXON) {
		LOG_DBG("FS TX on");
	} else if (status == CC1200_STATUS_CALIBRATE) {
		LOG_DBG("Calibrating");
	} else if (status == CC1200_STATUS_SETTLING) {
		LOG_DBG("Settling");
	} else if (status == CC1200_STATUS_RX_FIFO_ERROR) {
		LOG_DBG("RX FIFO error!");
	} else if (status == CC1200_STATUS_TX_FIFO_ERROR) {
		LOG_DBG("TX FIFO error!");
	}
}

/*********************
 * Generic functions *
 ********************/

bool z_cc1200_access_reg(const struct device *dev, bool read, uint8_t addr,
			 void *data, size_t length, bool extended, bool burst)
{
	const struct cc1200_config *config = dev->config;
	uint8_t cmd_buf[2];
	const struct spi_buf buf[2] = {
		{
			.buf = cmd_buf,
			.len = extended ? 2 : 1,
		},
		{
			.buf = data,
			.len = length,

		}
	};
	struct spi_buf_set tx = { .buffers = buf };

	cmd_buf[0] = 0U;

	if (burst) {
		cmd_buf[0] |= CC1200_ACCESS_BURST;
	}

	if (extended) {
		cmd_buf[0] |= CC1200_REG_EXTENDED_ADDRESS;
		cmd_buf[1] = addr;
	} else {
		cmd_buf[0] |= addr;
	}

	if (read) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		cmd_buf[0] |= CC1200_ACCESS_RD;

		tx.count = 1;

		return (spi_transceive_dt(&config->bus, &tx, &rx) == 0);
	}

	/* CC1200_ACCESS_WR is 0 so no need to play with it */
	tx.count =  data ? 2 : 1;

	return (spi_write_dt(&config->bus, &tx) == 0);
}

static inline uint8_t *get_mac(const struct device *dev)
{
	struct cc1200_context *cc1200 = dev->data;

#if defined(CONFIG_IEEE802154_CC1200_RANDOM_MAC)
	uint32_t *ptr = (uint32_t *)(cc1200->mac_addr + 4);

	UNALIGNED_PUT(sys_rand32_get(), ptr);

	cc1200->mac_addr[7] = (cc1200->mac_addr[7] & ~0x01) | 0x02;
#else
	cc1200->mac_addr[4] = CONFIG_IEEE802154_CC1200_MAC4;
	cc1200->mac_addr[5] = CONFIG_IEEE802154_CC1200_MAC5;
	cc1200->mac_addr[6] = CONFIG_IEEE802154_CC1200_MAC6;
	cc1200->mac_addr[7] = CONFIG_IEEE802154_CC1200_MAC7;
#endif

	cc1200->mac_addr[0] = 0x00;
	cc1200->mac_addr[1] = 0x12;
	cc1200->mac_addr[2] = 0x4b;
	cc1200->mac_addr[3] = 0x00;

	return cc1200->mac_addr;
}

static uint8_t get_status(const struct device *dev)
{
	uint8_t val;

	if (z_cc1200_access_reg(dev, true, CC1200_INS_SNOP,
				&val, 1, false, false)) {
		/* See Section 3.1.2 */
		return val & CC1200_STATUS_MASK;
	}

	/* We cannot get the status, so let's assume about readiness */
	return CC1200_STATUS_CHIP_NOT_READY;
}

/******************
 * GPIO functions *
 *****************/

static inline void gpio0_int_handler(const struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct cc1200_context *cc1200 =
		CONTAINER_OF(cb, struct cc1200_context, rx_tx_cb);

	if (atomic_get(&cc1200->tx) == 1) {
		if (atomic_get(&cc1200->tx_start) == 0) {
			atomic_set(&cc1200->tx_start, 1);
		} else {
			atomic_set(&cc1200->tx, 0);
		}

		k_sem_give(&cc1200->tx_sync);
	} else {
		if (atomic_get(&cc1200->rx) == 1) {
			k_sem_give(&cc1200->rx_lock);
			atomic_set(&cc1200->rx, 0);
		} else {
			atomic_set(&cc1200->rx, 1);
		}
	}
}

static void enable_gpio0_interrupt(const struct device *dev, bool enable)
{
	const struct cc1200_config *cfg = dev->config;
	gpio_flags_t mode = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, mode);
}

static int setup_gpio_callback(const struct device *dev)
{
	const struct cc1200_config *cfg = dev->config;
	struct cc1200_context *cc1200 = dev->data;

	gpio_init_callback(&cc1200->rx_tx_cb, gpio0_int_handler, BIT(cfg->interrupt.pin));

	if (gpio_add_callback(cfg->interrupt.port, &cc1200->rx_tx_cb) != 0) {
		return -EIO;
	}

	return 0;
}

/****************
 * RF functions *
 ***************/

static uint8_t get_lo_divider(const struct device *dev)
{
	/* See Table 34  */
	return FSD_BANDSELECT(read_reg_fs_cfg(dev)) << 1;
}

static bool write_reg_freq(const struct device *dev, uint32_t freq)
{
	uint8_t freq_data[3];

	freq_data[0] = (uint8_t)((freq & 0x00FF0000) >> 16);
	freq_data[1] = (uint8_t)((freq & 0x0000FF00) >> 8);
	freq_data[2] = (uint8_t)(freq & 0x000000FF);

	return z_cc1200_access_reg(dev, false, CC1200_REG_FREQ2,
				   freq_data, 3, true, true);
}


/* See Section 9.12 - RF programming
 *
 * The given formula in datasheet cannot be simply applied here, where CPU
 * limits us to unsigned integers of 32 bits. Instead, "slicing" it to
 * parts that fits in such limit is a solution which is applied below.
 *
 * The original formula being (freqoff is neglected):
 * Freq = ( RF * Lo_Div * 2^16 ) / Xtal
 *
 * RF and Xtal are, from here, expressed in KHz.
 *
 * It first calculates the targeted RF with given ChanCenterFreq0, channel
 * spacing and the channel number.
 *
 * The calculation will slice the targeted RF by multiple of 10:
 * 10^n where n is in [5, 3]. The rest, below 1000, is taken at once.
 * Let's take the 434000 KHz RF for instance:
 * it will be "sliced" in 3 parts: 400000, 30000, 4000.
 * Or the 169406 KHz RF, 4 parts: 100000, 60000, 9000, 406.
 *
 * This permits also to play with Xtal to keep the result big enough to avoid
 * losing precision. A factor - growing as much as Xtal decrease -  is then
 * applied to get to the proper result. Which one is rounded to the nearest
 * integer, again to get a bit better precision.
 *
 * In the end, this algorithm below works for all the supported bands by CC1200.
 * User does not need to pass anything extra besides the nominal settings: no
 * pre-computed part or else.
 */
static uint32_t rf_evaluate_freq_setting(const struct device *dev, uint32_t chan)
{
	struct cc1200_context *ctx = dev->data;
	uint32_t xtal = CONFIG_IEEE802154_CC1200_XOSC;
	uint32_t mult_10 = 100000U;
	uint32_t factor = 1U;
	uint32_t freq = 0U;
	uint32_t rf, lo_div;

	rf = ctx->rf_settings->chan_center_freq0 +
	     ((chan * (uint32_t)ctx->rf_settings->channel_spacing) / 10U);
	lo_div = get_lo_divider(dev);

	LOG_DBG("Calculating freq for %u KHz RF (%u)", rf, lo_div);

	while (rf > 0) {
		uint32_t hz, freq_tmp, rst;

		if (rf < 1000) {
			hz = rf;
		} else {
			hz = rf / mult_10;
			hz *= mult_10;
		}

		if (hz < 1000) {
			freq_tmp = (hz * lo_div * 65536U) / xtal;
		} else {
			freq_tmp = ((hz * lo_div) / xtal) * 65536U;
		}

		rst = freq_tmp % factor;
		freq_tmp /= factor;

		if (factor > 1 && (rst/(factor/10U)) > 5) {
			freq_tmp++;
		}

		freq += freq_tmp;

		factor *= 10U;
		mult_10 /= 10U;
		xtal /= 10U;
		rf -= hz;
	}

	LOG_DBG("FREQ is 0x%06X", freq);

	return freq;
}

static bool
rf_install_settings(const struct device *dev,
		    const struct cc1200_rf_registers_set *rf_settings)
{
	struct cc1200_context *cc1200 = dev->data;

	if (!z_cc1200_access_reg(dev, false, CC1200_REG_SYNC3,
				 (void *)rf_settings->registers,
				 CC1200_RF_NON_EXT_SPACE_REGS, false, true) ||
	    !z_cc1200_access_reg(dev, false, CC1200_REG_IF_MIX_CFG,
				 (uint8_t *)rf_settings->registers
				 + CC1200_RF_NON_EXT_SPACE_REGS,
				 CC1200_RF_EXT_SPACE_REGS, true, true) ||
	    !write_reg_pkt_len(dev, 0xFF)) {
		LOG_ERR("Could not install RF settings");
		return false;
	}

	cc1200->rf_settings = rf_settings;

	return true;
}

static int rf_calibrate(const struct device *dev)
{
	if (!instruct_scal(dev)) {
		LOG_ERR("Could not calibrate RF");
		return -EIO;
	}

	k_busy_wait(USEC_PER_MSEC * 5U);

	/* We need to re-enable RX as SCAL shuts off the freq synth */
	if (!instruct_sidle(dev) ||
	    !instruct_sfrx(dev) ||
	    !instruct_srx(dev)) {
		LOG_ERR("Could not switch to RX");
		return -EIO;
	}

	k_busy_wait(USEC_PER_MSEC * 10U);

	cc1200_print_status(get_status(dev));

	return 0;
}

/****************
 * TX functions *
 ***************/

static inline bool write_txfifo(const struct device *dev,
				void *data, size_t length)
{
	return z_cc1200_access_reg(dev, false,
				   CC1200_REG_TXFIFO,
				   data, length, false, true);
}

/****************
 * RX functions *
 ***************/

static inline bool read_rxfifo(const struct device *dev,
			       void *data, size_t length)
{
	return z_cc1200_access_reg(dev, true,
				   CC1200_REG_RXFIFO,
				   data, length, false, true);
}

static inline uint8_t get_packet_length(const struct device *dev)
{
	uint8_t len;

	if (z_cc1200_access_reg(dev, true, CC1200_REG_RXFIFO,
				&len, 1, false, true)) {
		return len;
	}

	return 0;
}

static inline bool verify_rxfifo_validity(const struct device *dev,
					  uint8_t pkt_len)
{
	/* packet should be at least 3 bytes as a ACK */
	if (pkt_len < 3 ||
	    read_reg_num_rxbytes(dev) > (pkt_len + CC1200_FCS_LEN)) {
		return false;
	}

	return true;
}

static inline bool read_rxfifo_content(const struct device *dev,
				       struct net_buf *buf, uint8_t len)
{

	if (!read_rxfifo(dev, buf->data, len) ||
	    (get_status(dev) == CC1200_STATUS_RX_FIFO_ERROR)) {
		return false;
	}

	net_buf_add(buf, len);

	return true;
}

static inline bool verify_crc(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t fcs[2];

	if (!read_rxfifo(dev, fcs, 2)) {
		return false;
	}

	if (!(fcs[1] & CC1200_FCS_CRC_OK)) {
		return false;
	}

	net_pkt_set_ieee802154_rssi(pkt, fcs[0]);
	net_pkt_set_ieee802154_lqi(pkt, fcs[1] & CC1200_FCS_LQI_MASK);

	return true;
}

static void cc1200_rx(void *arg)
{
	const struct device *dev = arg;
	struct cc1200_context *cc1200 = dev->data;
	struct net_pkt *pkt;
	uint8_t pkt_len;

	while (1) {
		pkt = NULL;

		k_sem_take(&cc1200->rx_lock, K_FOREVER);

		if (get_status(dev) == CC1200_STATUS_RX_FIFO_ERROR) {
			LOG_ERR("Fifo error");
			goto flush;
		}

		pkt_len = get_packet_length(dev);
		if (!verify_rxfifo_validity(dev, pkt_len)) {
			LOG_ERR("Invalid frame");
			goto flush;
		}

		pkt = net_pkt_alloc_with_buffer(cc1200->iface, pkt_len,
						AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No free pkt available");
			goto flush;
		}

		if (!read_rxfifo_content(dev, pkt->buffer, pkt_len)) {
			LOG_ERR("No content read");
			goto flush;
		}

		if (!verify_crc(dev, pkt)) {
			LOG_ERR("Bad packet CRC");
			goto out;
		}

		if (ieee802154_radio_handle_ack(cc1200->iface, pkt) == NET_OK) {
			LOG_DBG("ACK packet handled");
			goto out;
		}

		LOG_DBG("Caught a packet (%u)", pkt_len);

		if (net_recv_data(cc1200->iface, pkt) < 0) {
			LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		log_stack_usage(&cc1200->rx_thread);
		continue;
flush:
		LOG_DBG("Flushing RX");
		instruct_sidle(dev);
		instruct_sfrx(dev);
		instruct_srx(dev);
out:
		if (pkt) {
			net_pkt_unref(pkt);
		}

	}
}


/********************
 * Radio device API *
 *******************/
static enum ieee802154_hw_caps cc1200_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_SUB_GHZ;
}

static int cc1200_cca(const struct device *dev)
{
	struct cc1200_context *cc1200 = dev->data;

	if (atomic_get(&cc1200->rx) == 0) {
		uint8_t status = read_reg_rssi0(dev);

		if (!(status & CARRIER_SENSE) &&
		    (status & CARRIER_SENSE_VALID)) {
			return 0;
		}
	}

	LOG_WRN("Busy");

	return -EBUSY;
}

static int cc1200_set_channel(const struct device *dev, uint16_t channel)
{
	struct cc1200_context *cc1200 = dev->data;

	/* Unlike usual 15.4 chips, cc1200 is closer to a bare metal radio modem
	 * and thus does not provide any means to select a channel directly, but
	 * requires instead that one calculates and configures the actual
	 * targeted frequency for the requested channel.
	 *
	 * See rf_evaluate_freq_setting() above.
	 */

	if (atomic_get(&cc1200->rx) == 0) {
		uint32_t freq = rf_evaluate_freq_setting(dev, channel);

		if (!write_reg_freq(dev, freq) ||
		    rf_calibrate(dev)) {
			LOG_ERR("Could not set channel %u", channel);
			return -EIO;
		}
	}

	return 0;
}

static int cc1200_set_txpower(const struct device *dev, int16_t dbm)
{
	uint8_t pa_power_ramp;

	LOG_DBG("%d dbm", dbm);

	/* See Section 7.1 */
	dbm = ((dbm + 18) * 2) - 1;
	if ((dbm <= 3) || (dbm >= 64)) {
		LOG_ERR("Unhandled value");
		return -EINVAL;
	}

	pa_power_ramp = read_reg_pa_cfg1(dev) & ~PA_POWER_RAMP_MASK;
	pa_power_ramp |= ((uint8_t) dbm) & PA_POWER_RAMP_MASK;

	if (!write_reg_pa_cfg1(dev, pa_power_ramp)) {
		LOG_ERR("Could not proceed");
		return -EIO;
	}

	return 0;
}

static int cc1200_tx(const struct device *dev,
		     enum ieee802154_tx_mode mode,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	struct cc1200_context *cc1200 = dev->data;
	uint8_t *frame = frag->data;
	uint8_t len = frag->len;
	bool status = false;

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	LOG_DBG("%p (%u)", frag, len);

	/* ToDo:
	 * Supporting 802.15.4g will require to loop in pkt's frags
	 * depending on len value, this will also take more time.
	 */

	if (!instruct_sidle(dev) ||
	    !instruct_sfrx(dev) ||
	    !instruct_sftx(dev) ||
	    !instruct_sfstxon(dev)) {
		LOG_ERR("Cannot switch to TX mode");
		goto out;
	}

	if (!write_txfifo(dev, &len, CC1200_PHY_HDR_LEN) ||
	    !write_txfifo(dev, frame, len) ||
	    read_reg_num_txbytes(dev) != (len + CC1200_PHY_HDR_LEN)) {
		LOG_ERR("Cannot fill-in TX fifo");
		goto out;
	}

	atomic_set(&cc1200->tx, 1);
	atomic_set(&cc1200->tx_start, 0);

	if (!instruct_stx(dev)) {
		LOG_ERR("Cannot start transmission");
		goto out;
	}

	/* Wait for SYNC to be sent */
	k_sem_take(&cc1200->tx_sync, K_MSEC(100));
	if (atomic_get(&cc1200->tx_start) == 1) {
		/* Now wait for the packet to be fully sent */
		k_sem_take(&cc1200->tx_sync, K_MSEC(100));
	}

out:
	cc1200_print_status(get_status(dev));

	if (atomic_get(&cc1200->tx) == 1 &&
	    read_reg_num_txbytes(dev) != 0) {
		LOG_ERR("TX Failed");

		atomic_set(&cc1200->tx_start, 0);
		instruct_sftx(dev);
		status = false;
	} else {
		status = true;
	}

	atomic_set(&cc1200->tx, 0);

	/* Get back to RX */
	instruct_srx(dev);

	return status ? 0 : -EIO;
}

static int cc1200_start(const struct device *dev)
{
	if (!instruct_sidle(dev) ||
	    !instruct_sftx(dev) ||
	    !instruct_sfrx(dev) ||
	    rf_calibrate(dev)) {
		LOG_ERR("Could not proceed");
		return -EIO;
	}

	enable_gpio0_interrupt(dev, true);

	cc1200_print_status(get_status(dev));

	return 0;
}

static int cc1200_stop(const struct device *dev)
{
	enable_gpio0_interrupt(dev, false);

	if (!instruct_spwd(dev)) {
		LOG_ERR("Could not proceed");
		return -EIO;
	}

	return 0;
}

static uint16_t cc1200_get_channel_count(const struct device *dev)
{
	struct cc1200_context *cc1200 = dev->data;

	return cc1200->rf_settings->channel_limit;
}

/******************
 * Initialization *
 *****************/

static int power_on_and_setup(const struct device *dev)
{
	if (!instruct_sres(dev)) {
		LOG_ERR("Cannot reset");
		return -EIO;
	}

	if (!rf_install_settings(dev, &cc1200_rf_settings)) {
		return -EIO;
	}

	if (!write_reg_iocfg3(dev, CC1200_IOCFG3) ||
	    !write_reg_iocfg2(dev, CC1200_IOCFG2) ||
	    !write_reg_iocfg0(dev, CC1200_IOCFG0)) {
		LOG_ERR("Cannot configure GPIOs");
		return -EIO;
	}

	if (setup_gpio_callback(dev) != 0) {
		return -EIO;
	}

	return rf_calibrate(dev);
}

static int cc1200_init(const struct device *dev)
{
	const struct cc1200_config *config = dev->config;
	struct cc1200_context *cc1200 = dev->data;

	atomic_set(&cc1200->tx, 0);
	atomic_set(&cc1200->tx_start, 0);
	atomic_set(&cc1200->rx, 0);
	k_sem_init(&cc1200->rx_lock, 0, 1);
	k_sem_init(&cc1200->tx_sync, 0, 1);

	/* Configure GPIOs */
	if (!device_is_ready(config->interrupt.port)) {
		LOG_ERR("GPIO port %s is not ready",
			config->interrupt.port->name);
		return -ENODEV;
	}
	gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	LOG_DBG("GPIO and SPI configured");
	if (power_on_and_setup(dev) != 0) {
		LOG_ERR("Configuring CC1200 failed");
		return -EIO;
	}

	k_thread_create(&cc1200->rx_thread, cc1200->rx_stack,
			CONFIG_IEEE802154_CC1200_RX_STACK_SIZE,
			(k_thread_entry_t)cc1200_rx,
			(void *)dev, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&cc1200->rx_thread, "cc1200_rx");

	LOG_INF("CC1200 initialized");

	return 0;
}

static void cc1200_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cc1200_context *cc1200 = dev->data;
	uint8_t *mac = get_mac(dev);

	LOG_DBG("");

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	cc1200->iface = iface;

	ieee802154_init(iface);
}

static const struct cc1200_config cc1200_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.interrupt = GPIO_DT_SPEC_INST_GET(0, int_gpios)
};

static struct cc1200_context cc1200_context_data;

static struct ieee802154_radio_api cc1200_radio_api = {
	.iface_api.init	= cc1200_iface_init,

	.get_capabilities	= cc1200_get_capabilities,
	.cca			= cc1200_cca,
	.set_channel		= cc1200_set_channel,
	.set_txpower		= cc1200_set_txpower,
	.tx			= cc1200_tx,
	.start			= cc1200_start,
	.stop			= cc1200_stop,
	.get_subg_channel_count = cc1200_get_channel_count,
};

NET_DEVICE_DT_INST_DEFINE(0, cc1200_init, NULL, &cc1200_context_data,
			  &cc1200_config, CONFIG_IEEE802154_CC1200_INIT_PRIO,
			  &cc1200_radio_api, IEEE802154_L2,
			  NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
