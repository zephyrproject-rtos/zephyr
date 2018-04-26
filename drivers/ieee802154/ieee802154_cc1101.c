/* ieee802154_cc1101.c - TI CC1101 driver */

/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2018 Matthias Boesl.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "dev/cc1101"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <misc/byteorder.h>
#include <string.h>
#include <random/rand32.h>

#include <spi.h>
#include <gpio.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_cc1101.h"
#include "ieee802154_cc1101_rf.h"

#if defined(CONFIG_IEEE802154_CC1101_GPIO_SPI_CS)
static struct spi_cs_control cs_ctrl;
#endif

/***********************
 * Debugging functions *
 **********************/
#if CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL == 4
static void _cc1101_print_status(u8_t status)
{
	switch (status) {
	case CC1101_STATUS_SLEEP:
		SYS_LOG_DBG("Sleep");
		break;
	case CC1101_STATUS_IDLE:
		SYS_LOG_DBG("Idling");
		break;
	case CC1101_STATUS_XOFF:
		SYS_LOG_DBG("XOFF");
		break;
	case CC1101_STATUS_VCOON_MC:
		SYS_LOG_DBG("VCOON_MC");
		break;
	case CC1101_STATUS_REGON_MC:
		SYS_LOG_DBG("REGON_MC");
		break;
	case CC1101_STATUS_MANCAL:
		SYS_LOG_DBG("MANCAL");
		break;
	case CC1101_STATUS_VCOON:
		SYS_LOG_DBG("VCOON");
		break;
	case CC1101_STATUS_REGON:
		SYS_LOG_DBG("REGON");
		break;
	case CC1101_STATUS_STARTCAL:
		SYS_LOG_DBG("STARTCAL");
		break;
	case CC1101_STATUS_BWBOOST:
		SYS_LOG_DBG("BWBOOST");
		break;
	case CC1101_STATUS_FS_LOCK:
		SYS_LOG_DBG("FS LOCK");
		break;
	case CC1101_STATUS_IFADCON:
		SYS_LOG_DBG("IFADCON");
		break;
	case CC1101_STATUS_ENDCAL:
		SYS_LOG_DBG("ENDCAL");
		break;
	case CC1101_STATUS_RX:
		SYS_LOG_DBG("RX");
		break;
	case CC1101_STATUS_RX_END:
		SYS_LOG_DBG("RX END");
		break;
	case CC1101_STATUS_RX_RST:
		SYS_LOG_DBG("RX RST");
		break;
	case CC1101_STATUS_TXRX_SWITCH:
		SYS_LOG_DBG("TXRX SW");
		break;
	case CC1101_STATUS_RXFIFO_OVERFLOW:
		SYS_LOG_DBG("RX FIFO OF");
		break;
	case CC1101_STATUS_FSTXON:
		SYS_LOG_DBG("FSTXON");
		break;
	case CC1101_STATUS_TX:
		SYS_LOG_DBG("TX");
		break;
	case CC1101_STATUS_TX_END:
		SYS_LOG_DBG("TX END");
		break;
	case CC1101_STATUS_RXTX_SWITCH:
		SYS_LOG_DBG("RXTX SW");
		break;
	case CC1101_STATUS_TXFIFO_UNDERFLOW:
		SYS_LOG_DBG("TX UF");
		break;
	default:
		SYS_LOG_DBG("UNKNOWN %d", status);
		break;
	}
}
#else
#define _cc1101_print_status(...)
#endif /* CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL */

/*********************
 * Generic functions *
 ********************/

bool _cc1101_access_reg(struct cc1101_context *ctx, bool read, u8_t addr,
			void *data, size_t length, bool burst)
{
	u8_t cmd_buf[2];
	const struct spi_buf buf[2] = {
		{
			.buf = cmd_buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length,

		}
	};
	struct spi_buf_set tx = { .buffers = buf };

	/*
	   SYS_LOG_DBG("%s: addr 0x%02x - Data %p Length %u - %s, %s",
	            read ? "Read" : "Write", addr, data, length,
	            extended ? "extended" : "normal",
	            burst ? "burst" : "single");
	 */

	cmd_buf[0] = 0;

	if (burst) {
		cmd_buf[0] |= CC1101_ACCESS_BURST;
	}

	cmd_buf[0] |= addr;

	if (read) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		cmd_buf[0] |= CC1101_ACCESS_RD;

		tx.count = 1;

		return (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) == 0);
	}

	/* CC1101_ACCESS_WR is 0 so no need to play with it */
	tx.count =  data ? 2 : 1;

	return (spi_write(ctx->spi, &ctx->spi_cfg, &tx) == 0);
}

static inline u8_t *get_mac(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

#if defined(CONFIG_IEEE802154_CC1101_RANDOM_MAC)
	u32_t *ptr = (u32_t *)(cc1101->mac_addr + 4);

	UNALIGNED_PUT(sys_rand32_get(), ptr);

	cc1101->mac_addr[7] = (cc1101->mac_addr[7] & ~0x01) | 0x02;
#else
	cc1101->mac_addr[4] = CONFIG_IEEE802154_CC1101_MAC4;
	cc1101->mac_addr[5] = CONFIG_IEEE802154_CC1101_MAC5;
	cc1101->mac_addr[6] = CONFIG_IEEE802154_CC1101_MAC6;
	cc1101->mac_addr[7] = CONFIG_IEEE802154_CC1101_MAC7;
#endif

	cc1101->mac_addr[0] = 0x00;
	cc1101->mac_addr[1] = 0x12;
	cc1101->mac_addr[2] = 0x4b;
	cc1101->mac_addr[3] = 0x00;

	return cc1101->mac_addr;
}

static u8_t get_status(struct cc1101_context *ctx)
{
	u8_t status;

	if (_cc1101_access_reg(ctx, true, CC1101_REG_MARCSTATE,
			       &status, 1, true)) {
		return status & CC1101_STATUS_MASK;
	}

	/* We cannot get the status, so let's assume about readyness */
	return CC1101_STATUS_CHIP_NOT_READY;
}

/******************
 * GPIO functions *
 *****************/

static inline void gdo_int_handler(struct device *port,
				   struct gpio_callback *cb, u32_t pins)
{

	struct cc1101_context *cc1101 =
		CONTAINER_OF(cb, struct cc1101_context, rx_tx_cb);

	if (atomic_get(&cc1101->tx) == 1) {
		if (atomic_get(&cc1101->tx_start) == 0) {
			atomic_set(&cc1101->tx_start, 1);
		} else {
			atomic_set(&cc1101->tx, 0);
		}

		k_sem_give(&cc1101->tx_sync);
	} else {
		if (atomic_get(&cc1101->rx) == 1) {
			k_sem_give(&cc1101->rx_lock);
			atomic_set(&cc1101->rx, 0);
		} else {
			atomic_set(&cc1101->rx, 1);
		}
	}
}

static void enable_gpio_interrupt(struct cc1101_context *cc1101, u8_t nr, bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(
			cc1101->gpios[nr].dev,
			cc1101->gpios[nr].pin);
	} else {
		gpio_pin_disable_callback(
			cc1101->gpios[nr].dev,
			cc1101->gpios[nr].pin);
	}
}

static void setup_gpio_callback(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	gpio_init_callback(&cc1101->rx_tx_cb, gdo_int_handler,
			   BIT(cc1101->gpios[CC1101_GPIO_IDX_GPIO0].pin));
	gpio_add_callback(cc1101->gpios[CC1101_GPIO_IDX_GPIO0].dev,
			  &cc1101->rx_tx_cb);

	gpio_init_callback(&cc1101->rx_tx_cb, gdo_int_handler,
			   BIT(cc1101->gpios[CC1101_GPIO_IDX_GPIO1].pin));
	gpio_add_callback(cc1101->gpios[CC1101_GPIO_IDX_GPIO1].dev,
			  &cc1101->rx_tx_cb);
}

/****************
 * RF functions *
 ***************/
static bool
rf_install_settings(struct device *dev,
		    const struct cc1101_rf_registers_set *rf_settings)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	if (!_cc1101_access_reg(cc1101, false, CC1101_REG_FIFOTHR,
				(void *)rf_settings->registers,
				CC1101_RF_REGS, true)) {
		SYS_LOG_ERR("Could not install RF settings");
		return false;
	}

	cc1101->rf_settings = rf_settings;

	return true;
}

static int rf_calibrate(struct cc1101_context *ctx)
{
	SYS_LOG_INF("CC1101 calibrate");

	if (!instruct_scal(ctx)) {
		SYS_LOG_ERR("Could not calibrate RF");
		return -EIO;
	}

	k_busy_wait(5 * USEC_PER_MSEC);

	/* We need to re-enable RX as SCAL shuts off the freq synth */
	if (!instruct_sidle(ctx) ||
	    !instruct_sfrx(ctx) ||
	    !instruct_srx(ctx)) {
		SYS_LOG_ERR("Could not switch to RX");
		return -EIO;
	}

	k_busy_wait(10 * USEC_PER_MSEC);

	_cc1101_print_status(get_status(ctx));

	return 0;
}

/****************
 * TX functions *
 ***************/

static inline bool write_txfifo(struct cc1101_context *ctx,
				void *data, size_t length)
{
	return _cc1101_access_reg(ctx, false,
				  CC1101_REG_TXFIFO,
				  data, length, true);
}

/****************
 * RX functions *
 ***************/

static inline bool read_rxfifo(struct cc1101_context *ctx,
			       void *data, size_t length)
{
	return _cc1101_access_reg(ctx, true,
				  CC1101_REG_RXFIFO,
				  data, length, true);
}

static inline u8_t get_packet_length(struct cc1101_context *ctx)
{
	u8_t len;

	if (_cc1101_access_reg(ctx, true, CC1101_REG_RXFIFO,
			       &len, 1, true)) {
		return len + CC1101_FCS_LEN;
	}

	return 0;
}

static inline u8_t get_rx_bytes(struct cc1101_context *ctx)
{
	u8_t rx_bytes;

	if (!_cc1101_access_reg(ctx, true, CC1101_REG_RXBYTES,
				&rx_bytes, 1, true)) {
		return 0;
	}

	if (rx_bytes & 0x80) {
		SYS_LOG_DBG("overflow\n");
	}

	return rx_bytes & 0x7F;
}

static inline bool verify_rxfifo_validity(struct cc1101_context *ctx,
					  u8_t pkt_len)
{
	u8_t rx_bytes;

	if (!_cc1101_access_reg(ctx, true, CC1101_REG_RXBYTES,
				&rx_bytes, 1, true)) {
		return false;
	}

	/* packet should be at least 3 bytes as a ACK */
	if (pkt_len < 5 || rx_bytes > (pkt_len)) {
		return false;
	}

	return true;
}

static inline bool read_rxfifo_content(struct cc1101_context *ctx,
				       struct net_buf *frag, u8_t len)
{

	if (!read_rxfifo(ctx, frag->data, len) ||
	    (get_status(ctx) == CC1101_STATUS_RXFIFO_OVERFLOW)) {
		return false;
	}

	net_buf_add(frag, len);

	return true;
}

static inline bool verify_crc(struct cc1101_context *ctx,
			      struct net_pkt *pkt, u8_t len)
{
	u8_t *fcs = pkt->frags->data + len - CC1101_FCS_LEN;

	if (!read_rxfifo(ctx, fcs, 2)) {
		return false;
	}

	if (!(fcs[1] & CC1101_FCS_CRC_OK)) {
		return false;
	}

	net_pkt_set_ieee802154_rssi(pkt, fcs[0]);
	net_pkt_set_ieee802154_lqi(pkt, fcs[1] & CC1101_FCS_LQI_MASK);

	return true;
}

static void cc1101_rx(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;
	struct net_buf *pkt_frag;
	struct net_pkt *pkt;
	u8_t pkt_len;
	u8_t status;

	while (1) {
		pkt = NULL;

		k_sem_take(&cc1101->rx_lock, K_FOREVER);


		status = get_status(cc1101);
		_cc1101_print_status(status);
		pkt_len = get_packet_length(cc1101);


		SYS_LOG_DBG("red len: %d datalen %d\n", get_rx_bytes(cc1101), pkt_len);

		pkt_len = get_rx_bytes(cc1101);

		if (status == CC1101_STATUS_STARTCAL) {
			SYS_LOG_ERR("start CAL error");
			goto flush;
		}

		if (status == CC1101_STATUS_RXFIFO_OVERFLOW) {
			SYS_LOG_ERR("RX FIFO OF error");
			goto flush;
		}

		if (status == CC1101_STATUS_TXFIFO_UNDERFLOW) {
			SYS_LOG_ERR("TX FIFO UF error");
			goto flush;
		}

		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			SYS_LOG_ERR("No free pkt available");
			goto flush;
		}

		pkt_frag = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!pkt_frag) {
			SYS_LOG_ERR("No free frag available");
			goto flush;
		}

		net_pkt_frag_insert(pkt, pkt_frag);



		if (!verify_rxfifo_validity(cc1101, pkt_len)) {
			SYS_LOG_ERR("Invalid frame");
			goto flush;
		}

		if (!read_rxfifo_content(cc1101, pkt_frag, pkt_len)) {
			SYS_LOG_ERR("No content read");
			goto flush;
		}

		if (!verify_crc(cc1101, pkt, pkt_len)) {
			SYS_LOG_ERR("Bad packet CRC");
			goto flush;
		}

		if (ieee802154_radio_handle_ack(cc1101->iface, pkt) == NET_OK) {
			SYS_LOG_DBG("ACK packet handled");
			goto flush;
		}

		SYS_LOG_DBG("Caught a packet (%u)", pkt_len);

		if (net_recv_data(cc1101->iface, pkt) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto flush;
		}

		net_analyze_stack("CC1101 Rx Fiber stack",
				  K_THREAD_STACK_BUFFER(cc1101->rx_stack),
				  K_THREAD_STACK_SIZEOF(cc1101->rx_stack));
		continue;
flush:
		SYS_LOG_DBG("Flushing RX");
		instruct_sidle(cc1101);
		instruct_sfrx(cc1101);
		instruct_srx(cc1101);

		if (pkt) {
			net_pkt_unref(pkt);
		}

	}
}


/********************
 * Radio device API *
 *******************/
static enum ieee802154_hw_caps cc1101_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_SUB_GHZ;
}

static int cc1101_cca(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;
	u8_t status;

	if (atomic_get(&cc1101->rx) == 0) {
		if (_cc1101_access_reg(cc1101, true, CC1101_REG_PKTSTATUS,
				       &status, 1, true)) {
			return status & CHANNEL_IS_CLEAR;
		}
	}

	SYS_LOG_WRN("Busy");

	return -EBUSY;
}

static int cc1101_set_channel(struct device *dev, u16_t channel)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	if (atomic_get(&cc1101->rx) == 0) {
		if (!write_reg_channel(cc1101, channel) ||
		    rf_calibrate(cc1101)) {
			SYS_LOG_ERR("Could not set channel %u", channel);
			return -EIO;
		}
	} else {
		SYS_LOG_WRN("Busy");
	}

	return 0;
}

static int cc1101_set_txpower(struct device *dev, s16_t dbm)
{
	struct cc1101_context *cc1101 = dev->driver_data;
	u8_t pa_value;

	SYS_LOG_DBG("%d dbm", dbm);

	switch (dbm) {
	case -30:
		pa_value = CC1101_PA_MINUS_30;
		break;
	case -20:
		pa_value = CC1101_PA_MINUS_20;
		break;
	case -15:
		pa_value = CC1101_PA_MINUS_15;
		break;
	case -10:
		pa_value = CC1101_PA_MINUS_10;
		break;
	case -6:
		pa_value = CC1101_PA_MINUS_6;
		break;
	case 0:
		pa_value = CC1101_PA_0;
		break;
	case 5:
		pa_value = CC1101_PA_5;
		break;
	case 7:
		pa_value = CC1101_PA_7;
		break;
	case 10:
		pa_value = CC1101_PA_10;
		break;
	case 11:
		pa_value = CC1101_PA_11;
		break;
	default:
		SYS_LOG_ERR("Unhandled value");
		return -EINVAL;
	}

	if (atomic_get(&cc1101->rx) == 0) {
		if (!_cc1101_access_reg(cc1101, false, CC1101_REG_PATABLE,
					&pa_value, 1, true)) {
			SYS_LOG_ERR("Could not set PA");
			return -EIO;
		}
	}

	return 0;
}

static int cc1101_tx(struct device *dev,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	struct cc1101_context *cc1101 = dev->driver_data;
	u8_t *frame = frag->data - net_pkt_ll_reserve(pkt);
	u8_t len = net_pkt_ll_reserve(pkt) + frag->len;
	bool status = false;

	SYS_LOG_DBG("%p (%u)", frag, len);

	/* ToDo:
	 * Supporting 802.15.4g will require to loop in pkt's frags
	 * depending on len value, this will also take more time.
	 */

	if (!instruct_sidle(cc1101) ||
	    !instruct_sfrx(cc1101) ||
	    !instruct_sftx(cc1101) ||
	    !instruct_sfstxon(cc1101)) {
		SYS_LOG_ERR("Cannot switch to TX mode");
		goto out;
	}

	if (!write_txfifo(cc1101, &len, CC1101_PHY_HDR_LEN) ||
	    !write_txfifo(cc1101, frame, len)) {
		SYS_LOG_ERR("Cannot fill-in TX fifo");
		goto out;
	}

	atomic_set(&cc1101->tx, 1);
	atomic_set(&cc1101->tx_start, 0);

	if (!instruct_stx(cc1101)) {
		SYS_LOG_ERR("Cannot start transmission");
		goto out;
	}

	/* Wait for SYNC to be sent */
	k_sem_take(&cc1101->tx_sync, 100);
	if (atomic_get(&cc1101->tx_start) == 1) {
		/* Now wait for the packet to be fully sent */
		k_sem_take(&cc1101->tx_sync, 100);
	}

out:
	_cc1101_print_status(get_status(cc1101));

	if (atomic_get(&cc1101->tx) == 1 &&
	    get_rx_bytes(cc1101) != 0) {
		SYS_LOG_ERR("TX Failed");

		atomic_set(&cc1101->tx_start, 0);
		instruct_sftx(cc1101);
		status = false;
	} else {
		status = true;
	}

	atomic_set(&cc1101->tx, 0);

	/* Get back to RX */
	instruct_srx(cc1101);

	return status ? 0 : -EIO;
}

static int cc1101_start(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	if (!instruct_sidle(cc1101) ||
	    !instruct_sftx(cc1101) ||
	    !instruct_sfrx(cc1101) ||
	    rf_calibrate(cc1101)) {
		SYS_LOG_ERR("Could not proceed");
		return -EIO;
	}

	enable_gpio_interrupt(cc1101, CC1101_GPIO_IDX_GPIO0, true);
	enable_gpio_interrupt(cc1101, CC1101_GPIO_IDX_GPIO1, true);

	_cc1101_print_status(get_status(cc1101));

	return 0;
}

static int cc1101_stop(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	enable_gpio_interrupt(cc1101, CC1101_GPIO_IDX_GPIO0, false);
	enable_gpio_interrupt(cc1101, CC1101_GPIO_IDX_GPIO1, false);

	if (!instruct_spwd(cc1101)) {
		SYS_LOG_ERR("Could not proceed");
		return -EIO;
	}

	return 0;
}

static u16_t cc1101_get_channel_count(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	return cc1101->rf_settings->channel_limit;
}

/******************
 * Initialization *
 *****************/

static int power_on_and_setup(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	if (!instruct_sres(cc1101)) {
		SYS_LOG_ERR("Cannot reset");
		return -EIO;
	}

	if (!rf_install_settings(dev, &cc1101_rf_settings)) {
		SYS_LOG_ERR("Cannot write settings");
		return -EIO;
	}

	if (!write_reg_iocfg2(cc1101, CC1101_SETTING_IOCFG2) ||
	    !write_reg_iocfg1(cc1101, CC1101_SETTING_IOCFG1) ||
	    !write_reg_iocfg0(cc1101, CC1101_SETTING_IOCFG0)) {
		SYS_LOG_ERR("Cannot configure GPIOs");
		return -EIO;
	}

	setup_gpio_callback(dev);

	return rf_calibrate(cc1101);
}

static int configure_spi(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	cc1101->spi = device_get_binding(
		CONFIG_IEEE802154_CC1101_SPI_DRV_NAME);
	if (!cc1101->spi) {
		SYS_LOG_ERR("Unable to get SPI device");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_IEEE802154_CC1101_GPIO_SPI_CS)) {
		cs_ctrl.gpio_dev = device_get_binding(
			CONFIG_IEEE802154_CC1101_GPIO_SPI_CS_DRV_NAME);
		if (!cs_ctrl.gpio_dev) {
			SYS_LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}

		cs_ctrl.gpio_pin = CONFIG_IEEE802154_CC1101_GPIO_SPI_CS_PIN;
		cs_ctrl.delay = 0;

		cc1101->spi_cfg.cs = &cs_ctrl;

		SYS_LOG_DBG("SPI GPIO CS configured on %s:%u",
			    CONFIG_IEEE802154_CC1101_GPIO_SPI_CS_DRV_NAME,
			    CONFIG_IEEE802154_CC1101_GPIO_SPI_CS_PIN);
	}

	cc1101->spi_cfg.operation = SPI_WORD_SET(8);
	cc1101->spi_cfg.frequency = CONFIG_IEEE802154_CC1101_SPI_FREQ;
	cc1101->spi_cfg.slave = CONFIG_IEEE802154_CC1101_SPI_SLAVE;

	return 0;
}

static int cc1101_init(struct device *dev)
{
	struct cc1101_context *cc1101 = dev->driver_data;

	atomic_set(&cc1101->tx, 0);
	atomic_set(&cc1101->tx_start, 0);
	atomic_set(&cc1101->rx, 0);
	k_sem_init(&cc1101->rx_lock, 0, 1);
	k_sem_init(&cc1101->tx_sync, 0, 1);

	cc1101->gpios = cc1101_configure_gpios();
	if (!cc1101->gpios) {
		SYS_LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		SYS_LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	SYS_LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		SYS_LOG_ERR("Configuring CC1101 failed");
		return -EIO;
	}

	k_thread_create(&cc1101->rx_thread, cc1101->rx_stack,
			CONFIG_IEEE802154_CC1101_RX_STACK_SIZE,
			(k_thread_entry_t)cc1101_rx,
			dev, NULL, NULL, K_PRIO_COOP(2), 0, 0);

	SYS_LOG_INF("CC1101 initialized");

	return 0;
}

static void cc1101_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct cc1101_context *cc1101 = dev->driver_data;
	u8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	cc1101->iface = iface;

	ieee802154_init(iface);
}

static struct cc1101_context cc1101_context_data;

static struct ieee802154_radio_api cc1101_radio_api = {
	.iface_api.init = cc1101_iface_init,
	.iface_api.send = ieee802154_radio_send,
	.get_capabilities = cc1101_get_capabilities,
	.cca = cc1101_cca,
	.set_channel = cc1101_set_channel,
	.set_txpower = cc1101_set_txpower,
	.tx = cc1101_tx,
	.start = cc1101_start,
	.stop = cc1101_stop,
	.get_subg_channel_count = cc1101_get_channel_count,
};

NET_DEVICE_INIT(cc1101, CONFIG_IEEE802154_CC1101_DRV_NAME,
		cc1101_init, &cc1101_context_data, NULL,
		CONFIG_IEEE802154_CC1101_INIT_PRIO,
		&cc1101_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
