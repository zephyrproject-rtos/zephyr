/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_ieee802154

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ieee802154_bflb, CONFIG_IEEE802154_DRIVER_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/random/random.h>
#include <zephyr/irq.h>

#include "ieee802154_bflb.h"
#include <lmac154_frame.h>
#include <lmac154.h>
#include <lmac154_fpt.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#if defined(CONFIG_SOC_SERIES_BL61X)
#include <wl_api.h>
#endif

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif

static struct bflb_ieee802154_data bflb_ieee802154_data_inst;

extern int bflb_rf_init(void);

#if defined(CONFIG_SOC_SERIES_BL61X)
/*
 * Registered RX callback for BL616 (v2.3.24 API).
 * Called once per received frame with status indicating event type.
 */
static void bflb_rx_cb(lmac154_rx_status_t status, lmac154_receiveInfo_t *info, uint32_t *frame)
{
	if (status == LMAC154_RX_STATUS_ACK_ERR) {
		return;
	}

	if (status == LMAC154_RX_STATUS_RX_DONE) {
		if (info->rx_error != 0U) {
			LOG_DBG("rx_cb: rx_error=0x%04x len=%u", info->rx_error, info->rx_length);
		}
		lmac154_rxDoneEvent((uint8_t *)frame, info->rx_length, 0U);
	}
}

/*
 * TX completion callback for BL616 lmac154_triggerParamTx.
 * Delivers all statuses including ACKED/NO_ACK in a single call.
 */
static void bflb_tx_done_cb(lmac154_tx_status_t tx_status, uint32_t *ack_frame,
			    uint32_t ack_frame_len)
{
	ARG_UNUSED(ack_frame);
	ARG_UNUSED(ack_frame_len);

	bflb_ieee802154_data_inst.tx_status = (uint8_t)tx_status;

	/* Map ACKED/NO_ACK to the ack semaphore for compatibility */
	if (tx_status == LMAC154_TX_STATUS_ACKED) {
		bflb_ieee802154_data_inst.ack_received = 1U;
		k_sem_give(&bflb_ieee802154_data_inst.ack_sem);
	} else if (tx_status == LMAC154_TX_STATUS_NO_ACK) {
		bflb_ieee802154_data_inst.ack_received = 0U;
		k_sem_give(&bflb_ieee802154_data_inst.ack_sem);
	}

	k_sem_give(&bflb_ieee802154_data_inst.tx_sem);
}

#elif defined(CONFIG_SOC_SERIES_BL70X)

/* BL70x: RF PHY functions from libbl702_rf.a */
extern void rf_tx_pwr_init(void);
extern bool bz_phy_set_tx_power(int power_dbm);
/* Registers the MSOFT (IRQ 3) handler used by the blobs' FreeRTOS port. */
extern void bflb_ble_irq_setup(void);

#elif defined(CONFIG_SOC_SERIES_BL70XL)

/* BL70xL: RF PHY functions from libbl702l_rf.a */
extern bool bz_phy_set_tx_power(int power_dbm);
extern bool bz_phy_optimize_tx_channel(uint32_t channel_mhz);
extern void rf_tx_pwr_init(void);

#endif

/* TX/ACK timeout constants.
 * BL616 lmac154 may deliver ackEvent later than BL702L due to
 * coexistence arbiter overhead -- use longer ACK timeout.
 */
#if defined(CONFIG_SOC_SERIES_BL61X)
#define IEEE802154_BFLB_TX_TIMEOUT  K_MSEC(200)
#define IEEE802154_BFLB_ACK_TIMEOUT K_MSEC(100)
#else
#define IEEE802154_BFLB_TX_TIMEOUT  K_MSEC(100)
#define IEEE802154_BFLB_ACK_TIMEOUT K_MSEC(50)
#endif

/* TX status sentinel for "not yet completed" */
#define IEEE802154_BFLB_TX_STATUS_PENDING 0xFFU

/* TX power range in dBm, from the SoC's own lmac154_tx_power_t. */
#define IEEE802154_BFLB_TX_POWER_MIN LMAC154_TX_POWER_0dBm
#define IEEE802154_BFLB_TX_POWER_MAX LMAC154_TX_POWER_14dBm

/* Radio bring-up values only; the stack reconfigures both through the
 * radio API (e.g. CONFIG_OPENTHREAD_CHANNEL / OPENTHREAD_DEFAULT_TX_POWER).
 */
#define IEEE802154_BFLB_DEFAULT_CHANNEL  11U
#define IEEE802154_BFLB_DEFAULT_TX_POWER 10

/* IRQ numbers from device tree */
#define M154_IRQ      DT_INST_IRQ_BY_NAME(0, m154, irq)
#define M154_IRQ_PRIO DT_INST_IRQ_BY_NAME(0, m154, priority)

/* lmac154 ISR wrapper */

static lmac154_isr_t m154_isr_handler;

static void m154_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);

	if (m154_isr_handler != NULL) {
		m154_isr_handler();
	}
}

void lmac154_txDoneEvent(lmac154_tx_status_t tx_status)
{
	bflb_ieee802154_data_inst.tx_status = (uint8_t)tx_status;
	k_sem_give(&bflb_ieee802154_data_inst.tx_sem);
}

void lmac154_ackEvent(uint8_t ack_received, uint8_t frame_pending, uint8_t seq_num)
{
	bflb_ieee802154_data_inst.ack_received = ack_received;
	bflb_ieee802154_data_inst.ack_frame_pending = frame_pending;
	bflb_ieee802154_data_inst.ack_seq_num = seq_num;
	k_sem_give(&bflb_ieee802154_data_inst.ack_sem);
}

void lmac154_ackFrameEvent(uint8_t ack_received, uint8_t *rx_buf, uint8_t len)
{
	if ((ack_received != 0U) && (rx_buf != NULL) && (len > 0U)) {
		uint8_t copy_len = MIN(len, sizeof(bflb_ieee802154_data_inst.ack_frame_buf));

		(void)memcpy(bflb_ieee802154_data_inst.ack_frame_buf, rx_buf, copy_len);
		bflb_ieee802154_data_inst.ack_frame_len = copy_len;
	}
}

void lmac154_rxDoneEvent(uint8_t *rx_buf, uint8_t rx_len, uint32_t crc_fail)
{
	struct net_pkt *pkt;
	uint8_t pkt_len;

	if (bflb_ieee802154_data_inst.iface == NULL || !bflb_ieee802154_data_inst.started) {
		return;
	}

	if (crc_fail != 0U) {
		LOG_DBG("RX CRC fail, dropping");
		return;
	}

	/* rx_len includes 2 CRC bytes but rx_buf does NOT contain them.
	 * Actual MPDU data length = rx_len - FCS_LEN
	 */
	if (rx_len <= IEEE802154_BFLB_FCS_LEN) {
		return;
	}

	pkt_len = rx_len - IEEE802154_BFLB_FCS_LEN;

	pkt = net_pkt_rx_alloc_with_buffer(bflb_ieee802154_data_inst.iface, rx_len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("RX: no pkt buf");
		return;
	}

	/* Copy MPDU (without CRC) */
	if (net_pkt_write(pkt, rx_buf, pkt_len) != 0) {
		LOG_ERR("RX: write fail");
		net_pkt_unref(pkt);
		return;
	}

	/* For OpenThread (PKT_INCL_FCS), append the 2-byte FCS from hardware */
	if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		uint8_t fcs[IEEE802154_BFLB_FCS_LEN];

		lmac154_readRxCrc(fcs);
		if (net_pkt_write(pkt, fcs, sizeof(fcs)) != 0) {
			LOG_ERR("RX: FCS write fail");
			net_pkt_unref(pkt);
			return;
		}
	}

	/* Set RSSI and LQI in the packet control block */
	net_pkt_set_ieee802154_rssi_dbm(pkt, lmac154_getRSSI());
	net_pkt_set_ieee802154_lqi(pkt, (uint8_t)lmac154_getLQI());

	if (net_recv_data(bflb_ieee802154_data_inst.iface, pkt) < 0) {
		LOG_ERR("RX: net_recv_data fail");
		net_pkt_unref(pkt);
	}
}

void lmac154_rxStartEvent(void)
{
	/* No-op: required by lmac154 callback ABI */
}

void lmac154_hwAutoTxAckDoneEvent(void)
{
	/* No-op: required by lmac154 callback ABI */
}

void lmac154_reqEnhAckEvent(void)
{
	/* No-op: enhanced ACK header IEs (Thread 1.2) are not supported */
}

void lmac154_rxMhrEvent(uint8_t *rx_buf, uint8_t rx_len, uint8_t pkt_len)
{
	ARG_UNUSED(rx_buf);
	ARG_UNUSED(rx_len);
	ARG_UNUSED(pkt_len);
}

void lmac154_rxSecMhrEvent(uint8_t *rx_buf, uint8_t rx_len, uint8_t pkt_len)
{
	ARG_UNUSED(rx_buf);
	ARG_UNUSED(rx_len);
	ARG_UNUSED(pkt_len);
}

/* Zephyr IEEE 802.15.4 radio API */

static enum ieee802154_hw_caps bflb_ieee802154_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER | IEEE802154_HW_PROMISC |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_RX_TX_ACK | IEEE802154_HW_ENERGY_SCAN;
}

static int bflb_ieee802154_cca(const struct device *dev)
{
	int rssi;
	uint8_t busy;

	ARG_UNUSED(dev);

	busy = lmac154_runCCA(&rssi);

	return (busy != 0U) ? -EBUSY : 0;
}

static int bflb_ieee802154_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	if ((channel < IEEE802154_BFLB_CHANNEL_MIN) || (channel > IEEE802154_BFLB_CHANNEL_MAX)) {
		return -EINVAL;
	}

	/* Optimize RF front-end for the target channel (BL702L only; BL616 and
	 * BL702 lmac154 handle RF tuning internally).
	 */
#if defined(CONFIG_SOC_SERIES_BL70XL)
	(void)bz_phy_optimize_tx_channel(2405U + (5U * (channel - IEEE802154_BFLB_CHANNEL_MIN)));
#endif

	/* lmac154 channel enum: 0 = channel 11, 1 = channel 12, etc. */
	lmac154_setChannel((lmac154_channel_t)(channel - IEEE802154_BFLB_CHANNEL_MIN));
	bflb_ieee802154_data_inst.channel = channel;

	return 0;
}

static int bflb_ieee802154_filter(const struct device *dev, bool set,
				  enum ieee802154_filter_type type,
				  const struct ieee802154_filter *filter)
{
	ARG_UNUSED(dev);

	if (!set) {
		return -ENOTSUP;
	}

	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		/* lmac154 API takes non-const uint8_t*; cast required */
		lmac154_setLongAddr((uint8_t *)filter->ieee_addr);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		lmac154_setShortAddr(filter->short_addr);
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		lmac154_setPanId(filter->pan_id);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bflb_ieee802154_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	/* Clamp to hardware range */
	if (dbm < IEEE802154_BFLB_TX_POWER_MIN) {
		dbm = IEEE802154_BFLB_TX_POWER_MIN;
	} else if (dbm > IEEE802154_BFLB_TX_POWER_MAX) {
		dbm = IEEE802154_BFLB_TX_POWER_MAX;
	}

	lmac154_setTxPower((lmac154_tx_power_t)dbm);
#if defined(CONFIG_SOC_SERIES_BL61X)
	wl_rf_set_154_tx_power((uint32_t)dbm);
#else
	(void)bz_phy_set_tx_power((int)dbm);
#endif
	bflb_ieee802154_data_inst.tx_power = dbm;

	return 0;
}

static int bflb_ieee802154_tx(const struct device *dev, enum ieee802154_tx_mode mode,
			      struct net_pkt *pkt, struct net_buf *frag)
{
	uint8_t *data = frag->data;
	uint8_t len = frag->len;
	bool csma = false;
#if defined(CONFIG_SOC_SERIES_BL61X)
	static lmac154_txParam_t tp;
	static uint8_t __aligned(16) tx_buf[IEEE802154_BFLB_MAX_PKT_LEN];
	unsigned int key;
	int iret;
#endif
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	if (len > IEEE802154_BFLB_MAX_PKT_LEN) {
		return -EINVAL;
	}

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
		csma = false;
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
	case IEEE802154_TX_MODE_CCA:
		csma = true;
		break;
	default:
		return -ENOTSUP;
	}

	/* Reset semaphores */
	k_sem_reset(&bflb_ieee802154_data_inst.tx_sem);
	k_sem_reset(&bflb_ieee802154_data_inst.ack_sem);

	bflb_ieee802154_data_inst.tx_status = IEEE802154_BFLB_TX_STATUS_PENDING;
	bflb_ieee802154_data_inst.ack_received = 0U;

#if defined(CONFIG_SOC_SERIES_BL61X)
	/* BL616: use triggerParamTx which delivers ACKED/NO_ACK through
	 * the registered tx_done_cb -- the weak ackEvent does not fire
	 * when using registerEventCallback for the ISR path.
	 */
	memcpy(tx_buf, data, len);
	memset(&tp, 0, sizeof(tp));
	tp.pkt = (uint32_t *)tx_buf;
	tp.pkt_length = len;
	tp.tx_channel = (uint8_t)(bflb_ieee802154_data_inst.channel - IEEE802154_BFLB_CHANNEL_MIN);
	tp.resume_channel = tp.tx_channel;
	tp.csma_ca_max_backoff = 0U;
	tp.is_cca = 0U;
	tp.tx_done_cb = bflb_tx_done_cb;
	tp.next = NULL;

	key = irq_lock();
	iret = lmac154_triggerParamTx(&tp);
	irq_unlock(key);

	if (iret != 0) {
		lmac154_resetTx();
		lmac154_enableRx();
		LOG_DBG("triggerParamTx busy: %d", iret);
		return -EBUSY;
	}
#else
	lmac154_triggerTx(data, len, csma ? 1U : 0U);
#endif

	/* Wait for TX done */
	ret = k_sem_take(&bflb_ieee802154_data_inst.tx_sem, IEEE802154_BFLB_TX_TIMEOUT);
	if (ret != 0) {
#if defined(CONFIG_SOC_SERIES_BL61X)
		lmac154_resetTx();
		lmac154_enableRx();
#endif
		LOG_ERR("TX timeout");
		return -EIO;
	}

	switch (bflb_ieee802154_data_inst.tx_status) {
#if defined(CONFIG_SOC_SERIES_BL61X)
	case (uint8_t)LMAC154_TX_STATUS_ACKED:
		return 0;
	case (uint8_t)LMAC154_TX_STATUS_NO_ACK:
		return -ENOMSG;
	case (uint8_t)LMAC154_TX_STATUS_CCA_FAILED:
		return -EBUSY;
#endif
	case (uint8_t)LMAC154_TX_STATUS_CSMA_FAILED:
		return -EBUSY;
	case (uint8_t)LMAC154_TX_STATUS_TX_FINISHED:
		break;
	default:
		LOG_WRN("TX failed: status=%d", bflb_ieee802154_data_inst.tx_status);
		return -EIO;
	}

	/* TX_FINISHED: if ACK was requested, wait for separate ACK event
	 * (BL702L weak callback path -- BL616 never reaches here).
	 */
	if (LMAC154_FRAME_IS_ACK_REQ(data[0])) {
		ret = k_sem_take(&bflb_ieee802154_data_inst.ack_sem, IEEE802154_BFLB_ACK_TIMEOUT);
		if (ret != 0) {
			LOG_DBG("ACK timeout");
			return -ENOMSG;
		}
		if (bflb_ieee802154_data_inst.ack_received == 0U) {
			LOG_DBG("No ACK received");
			return -ENOMSG;
		}
	}

	return 0;
}

static int bflb_ieee802154_start(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_SOC_SERIES_BL61X)
	lmac154_enable1stStack();
	lmac154_registerEventCallback(LMAC154_STACK_1, bflb_rx_cb);
	m154_isr_handler = lmac154_getInterruptCallback();
#else
	lmac154_setRxStateWhenIdle(true);
#endif
	lmac154_enableRx();
	bflb_ieee802154_data_inst.started = true;

	LOG_DBG("802.15.4 radio started");
	return 0;
}

static int bflb_ieee802154_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	lmac154_disableRx();
	/* RxStateWhenIdle stays set; start() re-enables RX. */
	bflb_ieee802154_data_inst.started = false;

	LOG_DBG("802.15.4 radio stopped");
	return 0;
}

static int bflb_ieee802154_configure(const struct device *dev, enum ieee802154_config_type type,
				     const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_PROMISCUOUS:
		if (config->promiscuous) {
			lmac154_enableRxPromiscuousMode(0, 0);
			bflb_ieee802154_data_inst.promiscuous = true;
		} else {
			lmac154_disableRxPromiscuousMode();
			bflb_ieee802154_data_inst.promiscuous = false;
		}
		break;
	case IEEE802154_CONFIG_EVENT_HANDLER:
		/* Not supported */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static energy_scan_done_cb_t ed_scan_done_cb;

static void ed_scan_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (ed_scan_done_cb != NULL) {
		int rssi;

		(void)lmac154_runCCA(&rssi);
		ed_scan_done_cb(DEVICE_DT_INST_GET(0), (int16_t)rssi);
	}
}

static K_WORK_DELAYABLE_DEFINE(ed_scan_work, ed_scan_work_handler);

static int bflb_ieee802154_ed_scan(const struct device *dev, uint16_t duration,
				   energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);

	/* Async callback via delayed work queue. The delay provides dwell time
	 * for the radio to receive beacon responses during active scan before
	 * moving to the next channel. Running CCA at callback time (after the
	 * dwell) gives a more accurate energy reading for the channel.
	 */
	ed_scan_done_cb = done_cb;
	k_work_schedule(&ed_scan_work, K_MSEC(duration > 0U ? duration : 100U));

	return 0;
}

static void bflb_ieee802154_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct bflb_ieee802154_data *data = dev->data;

	data->iface = iface;

	/* Set the EUI-64 address as the link layer address */
	if (net_if_set_link_addr(iface, data->mac, sizeof(data->mac), NET_LINK_IEEE802154) != 0) {
		LOG_WRN("failed to set interface link address");
	}

	/* Initialize IEEE 802.15.4 L2 */
	ieee802154_init(iface);
}

static const struct ieee802154_radio_api bflb_ieee802154_radio_api = {
	.iface_api.init = bflb_ieee802154_iface_init,

	.get_capabilities = bflb_ieee802154_get_capabilities,
	.cca = bflb_ieee802154_cca,
	.set_channel = bflb_ieee802154_set_channel,
	.filter = bflb_ieee802154_filter,
	.set_txpower = bflb_ieee802154_set_txpower,
	.tx = bflb_ieee802154_tx,
	.start = bflb_ieee802154_start,
	.stop = bflb_ieee802154_stop,
	.configure = bflb_ieee802154_configure,
	.ed_scan = bflb_ieee802154_ed_scan,
};

/* Device init */

#if defined(CONFIG_SOC_SERIES_BL61X)
extern void zb_timer_cfg(uint32_t init_time);
#endif

static int bflb_ieee802154_init(const struct device *dev)
{
	struct bflb_ieee802154_data *data = dev->data;
#if defined(CONFIG_SOC_SERIES_BL61X)
	uint32_t cgen2;
	uint64_t now_us;
#else
	uint32_t tmp;
#endif
	int ret;

	k_sem_init(&data->tx_sem, 0, 1);
	k_sem_init(&data->ack_sem, 0, 1);

	/* Random locally-administered EUI-64 */
	sys_rand_get(data->mac, sizeof(data->mac));
	/* Set the locally-administered bit, clear multicast bit */
	data->mac[0] = (data->mac[0] | 0x02U) & 0xFEU;

#if defined(CONFIG_SOC_SERIES_BL61X)
	/* BL616: enable M154 + BT/BLE peripheral clock gates */
	cgen2 = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	cgen2 |= GLB_CGEN_S3_M1542_MSK | GLB_CGEN_S3_BT_BLE2_MSK;
	sys_write32(cgen2, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

#elif defined(CONFIG_SOC_SERIES_BL70X)
	/* Enable BL70x MAC154/Zigbee peripheral clock and pulse its soft reset */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG1_OFFSET);
	tmp |= (1U << GLB_M154_ZBEN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG1_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	tmp |= (1U << GLB_CGEN_S3_POS);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp |= (1U << GLB_SWRST_S30_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp &= ~(1U << GLB_SWRST_S30_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG0_OFFSET);

#elif defined(CONFIG_SOC_SERIES_BL70XL)
	/* Enable BL70xL MAC154/Zigbee peripheral clock and pulse its soft reset */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG1_OFFSET);
	tmp |= (1U << GLB_M154_ZBEN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG1_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	tmp |= (1U << GLB_CGEN_S300_POS);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp |= (1U << GLB_SWRST_S300_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG0_OFFSET);
	tmp &= ~(1U << GLB_SWRST_S300_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG0_OFFSET);
#endif

#if defined(CONFIG_SOC_SERIES_BL70X)
	/* The BL702 lmac154/RF blobs use the FreeRTOS port's machine software
	 * interrupt (MSOFT, IRQ 3) for deferred work. BL702 bl_irq_enable()
	 * deliberately skips MSOFT, so register its handler here before the
	 * blobs can raise it (the BLE HCI driver does the same).
	 */
	bflb_ble_irq_setup();
#endif

	/* Initialize shared PHY/RF hardware */
	ret = bflb_rf_init();
	if (ret != 0) {
		LOG_ERR("RF init failed: %d", ret);
		return ret;
	}

	/* Initialize lmac154 MAC layer */
	lmac154_init();

#if defined(CONFIG_SOC_SERIES_BL61X)
	lmac154_enableCoex();
	lmac154_setStd2015Extra(true);
	lmac154_setTxRetry(0);
	lmac154_fptClear();
	lmac154_setEnhAckWaitTime((LMAC154_AIFS + (6 + 39) * 2 + 20) << LMAC154_US_PER_SYMBOL_BITS);
	lmac154_setRxStateWhenIdle(true);
	lmac154_setFramePendingMode(LMAC154_FPT_ANY);

	now_us = k_ticks_to_us_floor64(k_uptime_ticks());
	zb_timer_cfg((uint32_t)(now_us >> LMAC154_US_PER_SYMBOL_BITS));

	lmac154_disableRx();

	m154_isr_handler = lmac154_getInterruptCallback();
#else
	m154_isr_handler = lmac154_getInterruptHandler();
#endif

	IRQ_CONNECT(M154_IRQ, M154_IRQ_PRIO, m154_irq_handler, NULL, 0);
	irq_enable(M154_IRQ);
#if !defined(CONFIG_SOC_SERIES_BL61X)
#if defined(CONFIG_SOC_SERIES_BL70XL)
	rf_tx_pwr_init();
#endif
	lmac154_setStd2015(true);
	lmac154_enableHwAutoTxAck();
#if defined(CONFIG_SOC_SERIES_BL70XL)
	(void)bz_phy_optimize_tx_channel(2405U);
#endif
	(void)bz_phy_set_tx_power(IEEE802154_BFLB_DEFAULT_TX_POWER);

	/* BL702L hardware filters frames by default; accept dest PAN/addr
	 * mismatches so OpenThread can receive unicast attach responses
	 * before the device has been assigned an address.
	 */
	lmac154_disableFrameTypeFiltering();
	lmac154_setRxAcceptPolicy(LMAC154_RX_ACCEPT_DST_PANID_MISMATCH |
				  LMAC154_RX_ACCEPT_DST_ADDR_MISMATCH);
#endif

	lmac154_setChannel(LMAC154_CHANNEL_11);
	data->channel = IEEE802154_BFLB_DEFAULT_CHANNEL;
	lmac154_setTxPower(LMAC154_TX_POWER_10dBm);
	data->tx_power = IEEE802154_BFLB_DEFAULT_TX_POWER;

	lmac154_setLongAddr(data->mac);

	lmac154_disableRx();

	LOG_INF("BFLB IEEE 802.15.4 initialized (lmac154 v%s)", lmac154_getVersionString());

	return 0;
}

#if defined(CONFIG_NET_L2_IEEE802154)
#define IEEE802154_BFLB_L2          IEEE802154_L2
#define IEEE802154_BFLB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define IEEE802154_BFLB_MTU         IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define IEEE802154_BFLB_L2          OPENTHREAD_L2
#define IEEE802154_BFLB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define IEEE802154_BFLB_MTU         1280
#endif

NET_DEVICE_DT_INST_DEFINE(0, bflb_ieee802154_init, NULL, &bflb_ieee802154_data_inst, NULL,
			  CONFIG_IEEE802154_BFLB_INIT_PRIO, &bflb_ieee802154_radio_api,
			  IEEE802154_BFLB_L2, IEEE802154_BFLB_L2_CTX_TYPE, IEEE802154_BFLB_MTU);
