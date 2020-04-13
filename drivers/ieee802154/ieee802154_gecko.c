/*
 * Copyright (c) 2020 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ieee802154_gecko, CONFIG_IEEE802154_DRIVER_LOG_LEVEL);

#include <errno.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <debug/stack.h>
#include <soc.h>
#include <device.h>
#include <init.h>
#include <debug/stack.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <sys/byteorder.h>
#include <string.h>
#include <random/rand32.h>
#include <net/ieee802154_radio.h>

#include <em_system.h>
#include <rail.h>
#include <rail_ieee802154.h>

/* Defines maximum frame size used by the radio */
#define RADIO_MAX_FRAME_SIZE                  128
/* Maximum time to wait for RAIL to send the packet */
#define TX_PACKET_SENT_TIMEOUT         K_MSEC(100)
#define IEEE802154_FCS_LENGTH		        2

struct ieee802154_gecko_context {
	/* RAIL internal Tx FIFO */
	u8_t rail_tx_fifo[RADIO_MAX_FRAME_SIZE] __aligned(RAIL_FIFO_ALIGNMENT);

	RAIL_Handle_t rail_handle;

	/* Pointer to the network interface. */
	struct net_if *iface;

	/* Device 802.15.4 long address. */
	u8_t mac[8];

	/* TX synchronization semaphore. Unlocked when frame has been sent or
	 * send procedure failed.
	 */
	struct k_sem tx_wait;

	/* TX result, updated in radio transmit callbacks. */
	int tx_status;

	/* Transmit / receive channel */
	u16_t channel;
};

struct ieee802154_gecko_dev_cfg {
	/* Initial TX power level in RAIL raw units */
	RAIL_TxPowerLevel_t init_tx_power_level_raw;
};

#define DEV_NAME(dev) ((dev)->config->name)

#define DEV_CFG(dev) \
	((const struct ieee802154_gecko_dev_cfg *const)(dev)->config->config_info)

#define DEV_DATA(dev) \
	((struct ieee802154_gecko_context *const)(dev)->driver_data)

static const RAIL_CsmaConfig_t rail_csma_config =
	RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

static const RAIL_LbtConfig_t rail_lbt_config = {
	.lbtMinBoRand = 0,
	.lbtMaxBoRand = 10,
	.lbtTries = 5,
	.lbtThreshold = -75,
	.lbtBackoff = 320,   /* 20 symbols at 16 us/symbol */
	.lbtDuration = 128,  /* 8 symbols at 16 us/symbol */
	.lbtTimeout = 0,     /* No timeout */
};

static const RAIL_IEEE802154_Config_t rail_ieee802154_config = {
	.addresses = NULL,
	.ackConfig = {
		.enable = false,
		.ackTimeout = 672,
		.rxTransitions = {
			RAIL_RF_STATE_RX,
			RAIL_RF_STATE_RX,
		},
		.txTransitions = {
			RAIL_RF_STATE_RX,
			RAIL_RF_STATE_RX,
		},
	},
	.timings = {
		.idleToRx = 100,
		.idleToTx = 100,
		.rxToTx = 192,
		.txToRx = 192,
		.rxSearchTimeout = 0,
		.txToRxSearchTimeout = 0,
	},
	.framesMask = RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
	.promiscuousMode = false,
	.isPanCoordinator = false,
};

static void ieee802154_gecko_rail_cb(RAIL_Handle_t rail_handle,
				     RAIL_Events_t event);

static RAIL_Config_t rail_config = {
	.eventsCallback = &ieee802154_gecko_rail_cb,
};

static void ieee802154_gecko_get_eui64(u8_t *mac)
{
	u64_t uniqueID = SYSTEM_GetUnique();

	memcpy(mac, &uniqueID, sizeof(uniqueID));
}

static void ieee802154_gecko_rx(struct device *dev,
				RAIL_RxPacketHandle_t packet_handle,
				RAIL_RxPacketInfo_t *packet_info)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	struct net_pkt *pkt;

	LOG_DBG("Rx packet received");

	pkt = net_pkt_alloc_with_buffer(dev_data->iface,
					packet_info->packetBytes,
					AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("No net_pkt available");
		return;
	}

	/* Skip Frame Length field, 1 byte at index 0 */
	if (net_pkt_write(pkt, packet_info->firstPortionData + 1,
			  packet_info->firstPortionBytes - 1)) {
		goto drop;
	}
	if (net_pkt_write(pkt, packet_info->lastPortionData,
			  packet_info->packetBytes - packet_info->firstPortionBytes)) {
		goto drop;
	}

	/* Fill packet information */
	RAIL_RxPacketDetails_t packet_details = {
		.timeReceived = {
			.timePosition = RAIL_PACKET_TIME_AT_SYNC_END,
		},
	};
	RAIL_GetRxPacketDetails(dev_data->rail_handle, packet_handle, &packet_details);

	net_pkt_set_ieee802154_lqi(pkt, packet_details.lqi);
	net_pkt_set_ieee802154_rssi(pkt, packet_details.rssi);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	struct net_ptp_time timestamp = {
		.second = packet_details.timeReceived.packetTime / USEC_PER_SEC,
		.nanosecond =
			(packet_details.timeReceived.packetTime % USEC_PER_SEC)
			* NSEC_PER_USEC
	};

	net_pkt_set_timestamp(pkt, &timestamp);
#endif

	if (net_recv_data(dev_data->iface, pkt) < 0) {
		LOG_ERR("Packet dropped by NET stack");
		goto drop;
	}

	return;

drop:
	net_pkt_unref(pkt);
}

/* Radio device API */

static void ieee802154_gecko_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);

	ieee802154_gecko_get_eui64(dev_data->mac);
	net_if_set_link_addr(iface, dev_data->mac, sizeof(dev_data->mac),
			     NET_LINK_IEEE802154);

	dev_data->iface = iface;

	ieee802154_init(iface);
}

static enum ieee802154_hw_caps ieee802154_gecko_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS |
	       IEEE802154_HW_FILTER |
	       IEEE802154_HW_CSMA |
	       IEEE802154_HW_2_4_GHZ;
}

static int ieee802154_gecko_cca(struct device *dev)
{
	return 0;
}

static int ieee802154_gecko_set_channel(struct device *dev, u16_t channel)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_PrepareChannel(dev_data->rail_handle, channel);
	if (status != RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	dev_data->channel = channel;

	return 0;
}

static int ieee802154_gecko_set_pan_id(struct device *dev, u16_t pan_id)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_IEEE802154_SetPanId(dev_data->rail_handle, pan_id, 0);
	if (status != RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	return 0;
}

static int ieee802154_gecko_set_short_addr(struct device *dev, u16_t short_addr)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_IEEE802154_SetShortAddress(dev_data->rail_handle, short_addr, 0);
	if (status != RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	return 0;
}

static int ieee802154_gecko_set_ieee_addr(struct device *dev, const u8_t *addr)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_IEEE802154_SetLongAddress(dev_data->rail_handle, addr, 0);
	if (status != RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	return 0;
}

static int ieee802154_gecko_filter(struct device *dev, bool set,
				   enum ieee802154_filter_type type,
				   const struct ieee802154_filter *filter)
{
	int ret;

	if (!set) {
		return -ENOTSUP;
	}

	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		ret = ieee802154_gecko_set_ieee_addr(dev, filter->ieee_addr);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		ret = ieee802154_gecko_set_short_addr(dev, filter->short_addr);
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		ret = ieee802154_gecko_set_pan_id(dev, filter->pan_id);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int ieee802154_gecko_set_txpower(struct device *dev, s16_t dbm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dbm);

	return -ENOTSUP;
}

static int ieee802154_gecko_start(struct device *dev)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_StartRx(dev_data->rail_handle, dev_data->channel, NULL);
	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_DBG("RAIL_StartRx returned an error %d", status);
		return -EIO;
	}

	return 0;
}

static int ieee802154_gecko_stop(struct device *dev)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);

	RAIL_Idle(dev_data->rail_handle, RAIL_IDLE, true);

	return 0;
}

static int ieee802154_gecko_tx(struct device *dev, enum ieee802154_tx_mode mode,
			       struct net_pkt *pkt, struct net_buf *frag)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	u8_t frame_len = frag->len + IEEE802154_FCS_LENGTH;
	RAIL_Status_t status;

	/* Write packet length at rail_tx_fifo[0] */
	if (RAIL_WriteTxFifo(dev_data->rail_handle, &frame_len, 1, true) != 1) {
		LOG_DBG("Writing packet length to TxFifo failed");
		return -EIO;
	}
	/* Add packet payload */
	if (RAIL_WriteTxFifo(dev_data->rail_handle, frag->data, frag->len, false)
	    != frag->len) {
		LOG_DBG("Writing packet payload to TxFifo failed");
		return -EIO;
	}

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
		status = RAIL_StartTx(dev_data->rail_handle, dev_data->channel,
				RAIL_TX_OPTIONS_DEFAULT, NULL);
		break;
	case IEEE802154_TX_MODE_CCA:
		status = RAIL_StartCcaLbtTx(dev_data->rail_handle,
				dev_data->channel,
				RAIL_TX_OPTIONS_DEFAULT,
				&rail_lbt_config,
				NULL);
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
		status = RAIL_StartCcaCsmaTx(dev_data->rail_handle,
				dev_data->channel,
				RAIL_TX_OPTIONS_DEFAULT,
				&rail_csma_config, NULL);
		break;
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
	default:
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_ERR("Failed to start Tx");
		return -EIO;
	}

	/* Wait for the callback from the radio driver. */
	if (0 != k_sem_take(&dev_data->tx_wait, TX_PACKET_SENT_TIMEOUT)) {
		LOG_DBG("Failed to take tx_wait semaphore");
		return -EIO;
	}

	return dev_data->tx_status;
}

#define RAIL_IRQ_PRIO  0

void ieee802154_gecko_irq_config(void)
{
	IRQ_DIRECT_CONNECT(RFSENSE_IRQn, RAIL_IRQ_PRIO, RFSENSE_IRQHandler, 0);
	irq_enable(RFSENSE_IRQn);
	IRQ_DIRECT_CONNECT(AGC_IRQn, RAIL_IRQ_PRIO, AGC_IRQHandler, 0);
	irq_enable(AGC_IRQn);
	IRQ_DIRECT_CONNECT(BUFC_IRQn, RAIL_IRQ_PRIO, BUFC_IRQHandler, 0);
	irq_enable(BUFC_IRQn);
	IRQ_DIRECT_CONNECT(FRC_IRQn, RAIL_IRQ_PRIO, FRC_IRQHandler, 0);
	irq_enable(FRC_IRQn);
	IRQ_DIRECT_CONNECT(FRC_IRQn, RAIL_IRQ_PRIO, FRC_IRQHandler, 0);
	irq_enable(FRC_IRQn);
	IRQ_DIRECT_CONNECT(FRC_PRI_IRQn, RAIL_IRQ_PRIO, FRC_PRI_IRQHandler, 0);
	irq_enable(FRC_PRI_IRQn);
	IRQ_DIRECT_CONNECT(MODEM_IRQn, RAIL_IRQ_PRIO, MODEM_IRQHandler, 0);
	irq_enable(MODEM_IRQn);
	IRQ_DIRECT_CONNECT(PROTIMER_IRQn, RAIL_IRQ_PRIO, PROTIMER_IRQHandler, 0);
	irq_enable(PROTIMER_IRQn);
	IRQ_DIRECT_CONNECT(RAC_RSM_IRQn, RAIL_IRQ_PRIO, RAC_RSM_IRQHandler, 0);
	irq_enable(RAC_RSM_IRQn);
	IRQ_DIRECT_CONNECT(RAC_SEQ_IRQn, RAIL_IRQ_PRIO, RAC_SEQ_IRQHandler, 0);
	irq_enable(RAC_SEQ_IRQn);
	IRQ_DIRECT_CONNECT(SYNTH_IRQn, RAIL_IRQ_PRIO, SYNTH_IRQHandler, 0);
	irq_enable(SYNTH_IRQn);
}

static int ieee802154_gecko_init_rail(struct device *dev)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	const struct ieee802154_gecko_dev_cfg *const dev_cfg = DEV_CFG(dev);
	RAIL_Status_t status;

	dev_data->rail_handle = RAIL_Init(&rail_config, NULL);
	if (dev_data->rail_handle == NULL) {
		LOG_DBG("Failed to get RAIL handle");
		return -EIO;
	}
	RAIL_Idle(dev_data->rail_handle, RAIL_IDLE, true);

	RAIL_EnablePaCal(true);
	RAIL_ConfigCal(dev_data->rail_handle, RAIL_CAL_ALL);

	/* Configure RAIL callbacks */
	RAIL_ConfigEvents(dev_data->rail_handle, RAIL_EVENTS_ALL,
			  RAIL_EVENT_RX_PACKET_RECEIVED |
			  RAIL_EVENT_TX_PACKET_SENT |
			  RAIL_EVENT_TX_ABORTED |
			  RAIL_EVENT_TX_BLOCKED |
			  RAIL_EVENT_TX_UNDERFLOW |
			  RAIL_EVENT_TX_CHANNEL_BUSY |
			  RAIL_EVENT_CAL_NEEDED);

	/* Initialize the PA */
	RAIL_TxPowerConfig_t txPowerConfig = {
		.mode = RAIL_TX_POWER_MODE_2P4_HP,
		.voltage = 1800U,
		.rampTime = 10U,
	};
	status = RAIL_ConfigTxPower(dev_data->rail_handle, &txPowerConfig);
	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_DBG("RAIL_ConfigTxPower returned an error %d", status);
		return -EIO;
	}

	status = RAIL_SetTxPower(dev_data->rail_handle,
				 dev_cfg->init_tx_power_level_raw);
	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_DBG("RAIL_SetTxPower returned an error %d", status);
		return -EIO;
	}

	RAIL_SetTxFifo(dev_data->rail_handle, dev_data->rail_tx_fifo, 0,
		       sizeof(dev_data->rail_tx_fifo));

	return 0;
}

static int ieee802154_gecko_init_ieee802154(struct device *dev)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	RAIL_Status_t status;

	status = RAIL_IEEE802154_Config2p4GHzRadio(dev_data->rail_handle);
	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_DBG("RAIL_IEEE802154_Config2p4GHzRadio returned an error %d",
			status);
		return -EIO;
	}

	status = RAIL_IEEE802154_Init(dev_data->rail_handle, &rail_ieee802154_config);
	if (status != RAIL_STATUS_NO_ERROR) {
		LOG_DBG("RAIL_IEEE802154_Init returned an error %d", status);
		return -EIO;
	}

	return 0;
}

static int ieee802154_gecko_init(struct device *dev)
{
	struct ieee802154_gecko_context *dev_data = DEV_DATA(dev);
	int ret;

	k_sem_init(&dev_data->tx_wait, 0, 1);

	ret = ieee802154_gecko_init_rail(dev);
	if (ret != 0) {
		LOG_ERR("%d: Failed to initialize RAIL", ret);
		return ret;
	}

	ieee802154_gecko_irq_config();

	ret = ieee802154_gecko_init_ieee802154(dev);
	if (ret != 0) {
		LOG_ERR("%d: Failed to initialize IEEE 802.15.4 Radio", ret);
		return ret;
	}

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static int ieee802154_gecko_configure(struct device *dev,
				      enum ieee802154_config_type type,
				      const struct ieee802154_config *config)
{
	return 0;
}

static struct ieee802154_radio_api ieee802154_gecko_radio_api = {
	.iface_api.init = ieee802154_gecko_iface_init,

	.get_capabilities = ieee802154_gecko_get_capabilities,
	.cca = ieee802154_gecko_cca,
	.set_channel = ieee802154_gecko_set_channel,
	.filter = ieee802154_gecko_filter,
	.set_txpower = ieee802154_gecko_set_txpower,
	.start = ieee802154_gecko_start,
	.stop = ieee802154_gecko_stop,
	.tx = ieee802154_gecko_tx,
	.configure = ieee802154_gecko_configure,
};

DEVICE_DECLARE(ieee802154_gecko_dev0);

static struct ieee802154_gecko_context ieee802154_gecko_dev0_data;
static const struct ieee802154_gecko_dev_cfg ieee802154_gecko_dev0_config = {
	.init_tx_power_level_raw = CONFIG_IEEE802154_GECKO_TXPOWER_RAW,
};

static void ieee802154_gecko_rail_cb(RAIL_Handle_t railHandle,
				     RAIL_Events_t event)
{
	struct device *dev = DEVICE_GET(ieee802154_gecko_dev0);

	if (event & RAIL_EVENT_CAL_NEEDED) {
		RAIL_Calibrate(railHandle, NULL, RAIL_CAL_ALL_PENDING);
	}

	if (event & (RAIL_EVENT_TX_ABORTED |
		     RAIL_EVENT_TX_BLOCKED |
		     RAIL_EVENT_TX_UNDERFLOW |
		     RAIL_EVENT_TX_CHANNEL_BUSY)) {
		LOG_DBG("RAIL_Events_t 0x%llx", event);
		ieee802154_gecko_dev0_data.tx_status = -EIO;
		k_sem_give(&ieee802154_gecko_dev0_data.tx_wait);
	}

	if (event & RAIL_EVENT_TX_PACKET_SENT) {
		LOG_DBG("RAIL_Events_t: TX_PACKET_SENT");
		ieee802154_gecko_dev0_data.tx_status = 0;
		k_sem_give(&ieee802154_gecko_dev0_data.tx_wait);
	}

	if (event & RAIL_EVENT_RX_PACKET_RECEIVED) {
		RAIL_RxPacketInfo_t packet_info;
		RAIL_RxPacketHandle_t rx_packet_handle;

		LOG_DBG("RAIL_Events_t: RX_PACKET_RECEIVED");

		rx_packet_handle = RAIL_GetRxPacketInfo(
				ieee802154_gecko_dev0_data.rail_handle,
				RAIL_RX_PACKET_HANDLE_NEWEST,
				&packet_info);
		if ((rx_packet_handle != RAIL_RX_PACKET_HANDLE_INVALID) &&
		    (packet_info.packetStatus == RAIL_RX_PACKET_READY_SUCCESS)) {
			ieee802154_gecko_rx(dev, rx_packet_handle, &packet_info);
		}
	}
}

#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU 125

NET_DEVICE_INIT(ieee802154_gecko_dev0, CONFIG_IEEE802154_GECKO_DRV_NAME,
		ieee802154_gecko_init, device_pm_control_nop,
		&ieee802154_gecko_dev0_data, &ieee802154_gecko_dev0_config,
		CONFIG_IEEE802154_GECKO_INIT_PRIORITY,
		&ieee802154_gecko_radio_api, L2, L2_CTX_TYPE, MTU);
