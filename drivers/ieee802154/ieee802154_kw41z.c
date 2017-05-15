/* ieee802154_kw41z.c - NXP KW41Z driver */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "IEEE802154_KW41Z"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <kernel.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <irq.h>
#include <net/ieee802154_radio.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <misc/byteorder.h>
#include <rand32.h>

#include "fsl_xcvr.h"

#define KW41Z_DEFAULT_CHANNEL		26
#define KW41Z_CCA_TIME			8
#define KW41Z_SHR_PHY_TIME		12
#define KW41Z_PER_BYTE_TIME		2
#define KW41Z_ACK_WAIT_TIME		54
#define KW41Z_IDLE_WAIT_RETRIES		5
#define RADIO_0_IRQ_PRIO		0x80
#define KW41Z_FCS_LENGTH		2
#define KW41Z_PSDU_LENGTH		125
#define KW41Z_OUTPUT_POWER_MAX		2
#define KW41Z_OUTPUT_POWER_MIN		(-19)
#define KW41Z_AUTOACK_ENABLED		1

enum {
	KW41Z_CCA_ED,
	KW41Z_CCA_MODE1,
	KW41Z_CCA_MODE2,
	KW41Z_CCA_MODE3
};

enum {
	KW41Z_STATE_IDLE,
	KW41Z_STATE_RX,
	KW41Z_STATE_TX,
	KW41Z_STATE_CCA,
	KW41Z_STATE_TXRX,
	KW41Z_STATE_CCCA
};

/* Lookup table for PA_PWR register */
static const u8_t pa_pwr_lt[22] = {
	2, 2, 2, 2, 2, 2,	/* -19:-14 dBm */
	4, 4, 4,		/* -13:-11 dBm */
	6, 6, 6,		/* -10:-8 dBm */
	8, 8,			/* -7:-6 dBm */
	10, 10,			/* -5:-4 dBm */
	12,			/* -3 dBm */
	14, 14,			/* -2:-1 dBm */
	18, 18,			/* 0:1 dBm */
	24			/* 2 dBm */
};

struct kw41z_context {
	struct net_if *iface;
	u8_t mac_addr[8];

	struct k_sem seq_sync;
	atomic_t seq_retval;

	u32_t rx_warmup_time;
	u32_t tx_warmup_time;

	u8_t lqi;
};

static struct kw41z_context kw41z_context_data;

static inline u8_t kw41z_get_instant_state(void)
{
	return (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK) >>
	       ZLL_SEQ_STATE_SEQ_STATE_SHIFT;
}

static inline u8_t kw41z_get_seq_state(void)
{
	return (ZLL->PHY_CTRL & ZLL_PHY_CTRL_XCVSEQ_MASK) >>
	       ZLL_PHY_CTRL_XCVSEQ_SHIFT;
}

static inline void kw41z_set_seq_state(u8_t state)
{
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_XCVSEQ_MASK) |
			ZLL_PHY_CTRL_XCVSEQ(state);
}

static inline void kw41z_wait_for_idle(void)
{
	u8_t retries = KW41Z_IDLE_WAIT_RETRIES;
	u8_t state = kw41z_get_instant_state();


	while (state != KW41Z_STATE_IDLE && retries) {
		retries--;
		state = kw41z_get_instant_state();
	}

	if (state != KW41Z_STATE_IDLE) {
		SYS_LOG_ERR("Error waiting for idle state");
	}
}

static int kw41z_prepare_for_new_state(void)
{
	if (kw41z_get_seq_state() == KW41Z_STATE_RX) {
		kw41z_set_seq_state(KW41Z_STATE_IDLE);
	}

	if (kw41z_get_seq_state() != KW41Z_STATE_IDLE) {
		return -1;
	}

	kw41z_wait_for_idle();

	return 0;
}

static inline void kw41z_enable_seq_irq(void)
{
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
}

static inline void kw41z_disable_seq_irq(void)
{
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_SEQMSK_MASK;
}

static void kw41z_tmr1_set_timeout(u32_t timeout)
{
	timeout += ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT;

	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR1CMP_EN_MASK;

	ZLL->T1CMP = timeout;
	ZLL->IRQSTS = (ZLL->IRQSTS & ~ZLL_IRQSTS_TMR1MSK_MASK) |
		      ZLL_IRQSTS_TMR1IRQ_MASK;

	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_TMR1CMP_EN_MASK;
}

static inline void kw41z_tmr1_disable(void)
{
	ZLL->IRQSTS |= ZLL_IRQSTS_TMR1MSK_MASK;
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR1CMP_EN_MASK;
}

static void kw41z_tmr2_set_timeout(u32_t timeout)
{
	timeout += ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT;

	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR2CMP_EN_MASK;

	ZLL->T2CMP = timeout;
	ZLL->IRQSTS = (ZLL->IRQSTS & ~ZLL_IRQSTS_TMR2MSK_MASK) |
		      ZLL_IRQSTS_TMR2IRQ_MASK;

	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_TMR2CMP_EN_MASK;
}

static inline void kw41z_tmr2_disable(void)
{
	ZLL->IRQSTS |= ZLL_IRQSTS_TMR2MSK_MASK;
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR2CMP_EN_MASK;
}

static int kw41z_cca(struct device *dev)
{
	struct kw41z_context *kw41z = dev->driver_data;

	if (kw41z_prepare_for_new_state()) {
		SYS_LOG_DBG("Can't initiate new SEQ state");
		return -EBUSY;
	}

	k_sem_init(&kw41z->seq_sync, 0, 1);

	kw41z_enable_seq_irq();
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_CCATYPE_MASK) |
			ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1);
	kw41z_set_seq_state(KW41Z_STATE_CCA);

	kw41z_tmr1_set_timeout(kw41z->rx_warmup_time + KW41Z_CCA_TIME);
	k_sem_take(&kw41z->seq_sync, K_FOREVER);

	return kw41z->seq_retval;
}

static int kw41z_set_channel(struct device *dev, u16_t channel)
{
	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	ZLL->CHANNEL_NUM0 = channel;
	return 0;
}

static int kw41z_set_pan_id(struct device *dev, u16_t pan_id)
{
	ZLL->MACSHORTADDRS0 = (ZLL->MACSHORTADDRS0 &
			       ~ZLL_MACSHORTADDRS0_MACPANID0_MASK) |
			      ZLL_MACSHORTADDRS0_MACPANID0(pan_id);
	return 0;
}

static int kw41z_set_short_addr(struct device *dev, u16_t short_addr)
{
	ZLL->MACSHORTADDRS0 = (ZLL->MACSHORTADDRS0 &
			       ~ZLL_MACSHORTADDRS0_MACSHORTADDRS0_MASK) |
			      ZLL_MACSHORTADDRS0_MACSHORTADDRS0(short_addr);
	return 0;
}

static int kw41z_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	u32_t val;

	memcpy(&val, ieee_addr, sizeof(val));
	ZLL->MACLONGADDRS0_LSB = val;

	memcpy(&val, ieee_addr + sizeof(val), sizeof(val));
	ZLL->MACLONGADDRS0_MSB = val;

	return 0;
}

static int kw41z_set_txpower(struct device *dev, s16_t dbm)
{
	if (dbm < KW41Z_OUTPUT_POWER_MIN) {
		ZLL->PA_PWR = 0;
	} else if (dbm > KW41Z_OUTPUT_POWER_MAX) {
		ZLL->PA_PWR = 30;
	} else {
		ZLL->PA_PWR = pa_pwr_lt[dbm - KW41Z_OUTPUT_POWER_MIN];
	}

	return 0;
}

static int kw41z_start(struct device *dev)
{
	irq_enable(Radio_1_IRQn);

	kw41z_set_seq_state(KW41Z_STATE_RX);
	kw41z_enable_seq_irq();

	return 0;
}

static int kw41z_stop(struct device *dev)
{
	irq_disable(Radio_1_IRQn);

	kw41z_disable_seq_irq();
	kw41z_set_seq_state(KW41Z_STATE_IDLE);

	return 0;
}

static u8_t kw41z_convert_lqi(u8_t hw_lqi)
{
	if (hw_lqi >= 220) {
		return 255;
	} else {
		return (51 * hw_lqi) / 44;
	}
}

static u8_t kw41z_get_lqi(struct device *dev)
{
	struct kw41z_context *kw41z = dev->driver_data;

	return kw41z->lqi;
}

static inline void kw41z_rx(struct kw41z_context *kw41z, u8_t len)
{
	struct net_pkt *pkt = NULL;
	struct net_buf *frag = NULL;
	u16_t reg_val = 0;
	u8_t pkt_len, hw_lqi, i;

	pkt_len = len - KW41Z_FCS_LENGTH;

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		SYS_LOG_ERR("No buf available");
		goto out;
	}

	frag = net_pkt_get_frag(pkt, K_NO_WAIT);
	if (!frag) {
		SYS_LOG_ERR("No frag available");
		goto out;
	}

	net_pkt_frag_insert(pkt, frag);

	/* PKT_BUFFER_RX needs to be accessed alligned to 16 bits */
	for (i = 0; i < pkt_len; i++) {
		if (i % 2 == 0) {
			reg_val = *(((u16_t *)ZLL->PKT_BUFFER_RX) + i/2);
			frag->data[i] = reg_val & 0xFF;
		} else {
			frag->data[i] = reg_val >> 8;
		}
	}

	net_buf_add(frag, pkt_len);

	if (ieee802154_radio_handle_ack(kw41z->iface, pkt) == NET_OK) {
		SYS_LOG_DBG("ACK packet handled");
		goto out;
	}

	hw_lqi = (ZLL->LQI_AND_RSSI & ZLL_LQI_AND_RSSI_LQI_VALUE_MASK) >>
		 ZLL_LQI_AND_RSSI_LQI_VALUE_SHIFT;
	kw41z->lqi = kw41z_convert_lqi(hw_lqi);

	if (net_recv_data(kw41z->iface, pkt) < 0) {
		SYS_LOG_DBG("Packet dropped by NET stack");
		goto out;
	}

	return;
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static int kw41z_tx(struct device *dev, struct net_pkt *pkt,
		    struct net_buf *frag)
{
	struct kw41z_context *kw41z = dev->driver_data;
	u8_t payload_len = net_pkt_ll_reserve(pkt) + frag->len;
	u8_t *payload = frag->data - net_pkt_ll_reserve(pkt);
	u32_t tx_timeout;

	if (kw41z_prepare_for_new_state()) {
		SYS_LOG_DBG("Can't initiate new SEQ state");
		return -EBUSY;
	}

	if (payload_len > KW41Z_PSDU_LENGTH) {
		SYS_LOG_ERR("Payload too long");
		return 0;
	}

	((u8_t *)ZLL->PKT_BUFFER_TX)[0] = payload_len + KW41Z_FCS_LENGTH;
	memcpy(((u8_t *)ZLL->PKT_BUFFER_TX) + 1, payload, payload_len);

	/* Set CCA mode */
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_CCATYPE_MASK) |
			ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1);

	/* Clear all IRQ flags */
	ZLL->IRQSTS = ZLL->IRQSTS;

	tx_timeout = kw41z->tx_warmup_time + KW41Z_SHR_PHY_TIME +
		     payload_len * KW41Z_PER_BYTE_TIME;

	/* Perform automatic reception of ACK frame, if required */
	if (KW41Z_AUTOACK_ENABLED) {
		tx_timeout += KW41Z_ACK_WAIT_TIME;
		ZLL->PHY_CTRL |= ZLL_PHY_CTRL_RXACKRQD_MASK;
		kw41z_set_seq_state(KW41Z_STATE_TXRX);
		kw41z_tmr2_set_timeout(tx_timeout);
	} else {
		ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_RXACKRQD_MASK;
		kw41z_set_seq_state(KW41Z_STATE_TX);
		kw41z_tmr1_set_timeout(tx_timeout);
	}

	kw41z_enable_seq_irq();

	k_sem_take(&kw41z->seq_sync, K_FOREVER);

	return kw41z->seq_retval;
}

static void kw41z_isr(int unused)
{
	u32_t irqsts = ZLL->IRQSTS;
	u8_t state = kw41z_get_seq_state();
	u8_t restart_rx = 1;
	u8_t rx_len;

	/* TMR1 IRQ - time-out */
	if ((irqsts & ZLL_IRQSTS_TMR1IRQ_MASK) &&
	    !(irqsts & ZLL_IRQSTS_TMR1MSK_MASK)) {
		SYS_LOG_DBG("TMR1 timeout");
		kw41z_tmr1_disable();
		kw41z_disable_seq_irq();

		if (state == KW41Z_STATE_CCA &&
		    !(irqsts & ZLL_IRQSTS_CCA_MASK)) {
			kw41z_set_seq_state(KW41Z_STATE_IDLE);
			atomic_set(&kw41z_context_data.seq_retval, 0);
			restart_rx = 0;
		} else {
			atomic_set(&kw41z_context_data.seq_retval, -EBUSY);
		}

		k_sem_give(&kw41z_context_data.seq_sync);
	}

	/* TMR2 IRQ - time-out */
	if ((irqsts & ZLL_IRQSTS_TMR2IRQ_MASK) &&
	    !(irqsts & ZLL_IRQSTS_TMR2MSK_MASK)) {
		SYS_LOG_DBG("TMR2 timeout");
		kw41z_tmr2_disable();
		atomic_set(&kw41z_context_data.seq_retval, -EBUSY);
		k_sem_give(&kw41z_context_data.seq_sync);
	}

	/* Sequence done IRQ */
	if ((state != KW41Z_STATE_IDLE) && (irqsts & ZLL_IRQSTS_SEQIRQ_MASK)) {
		kw41z_disable_seq_irq();
		kw41z_tmr1_disable();
		kw41z_set_seq_state(KW41Z_STATE_IDLE);

		switch (state) {
		case KW41Z_STATE_RX:
			SYS_LOG_DBG("RX seq done");
			ZLL->IRQSTS = irqsts;
			rx_len = (ZLL->IRQSTS &
				  ZLL_IRQSTS_RX_FRAME_LENGTH_MASK) >>
				 ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT;
			if (rx_len != 0) {
				kw41z_rx(&kw41z_context_data, rx_len);
			}

			break;
		case KW41Z_STATE_TXRX:
			SYS_LOG_DBG("TXRX seq done");
			kw41z_tmr2_disable();
		case KW41Z_STATE_TX:
			SYS_LOG_DBG("TX seq done");
			if (irqsts & ZLL_IRQSTS_CCA_MASK) {
				atomic_set(&kw41z_context_data.seq_retval,
					   -EBUSY);
			} else {
				atomic_set(&kw41z_context_data.seq_retval, 0);
			}

			k_sem_give(&kw41z_context_data.seq_sync);
			break;
		case KW41Z_STATE_CCA:
			SYS_LOG_DBG("CCA seq done");
			if (irqsts & ZLL_IRQSTS_CCA_MASK) {
				atomic_set(&kw41z_context_data.seq_retval,
					   -EBUSY);
			} else {
				atomic_set(&kw41z_context_data.seq_retval, 0);
				restart_rx = 0;
			}

			k_sem_give(&kw41z_context_data.seq_sync);
			break;
		default:
			break;
		}
	}

	/* Clear interrupts */
	ZLL->IRQSTS = irqsts;

	/* Restart RX */
	if (restart_rx && state != KW41Z_STATE_IDLE) {
		kw41z_set_seq_state(KW41Z_STATE_IDLE);
		kw41z_wait_for_idle();

		kw41z_enable_seq_irq();
		kw41z_set_seq_state(KW41Z_STATE_RX);
	}
}

static inline u8_t *get_mac(struct device *dev)
{
	struct kw41z_context *kw41z = dev->driver_data;
	u32_t *ptr = (u32_t *)(kw41z->mac_addr);

	UNALIGNED_PUT(sys_rand32_get(), ptr);
	ptr = (u32_t *)(kw41z->mac_addr + 4);
	UNALIGNED_PUT(sys_rand32_get(), ptr);

	kw41z->mac_addr[0] = (kw41z->mac_addr[0] & ~0x01) | 0x02;

	return kw41z->mac_addr;
}

static int kw41z_init(struct device *dev)
{
	struct kw41z_context *kw41z = dev->driver_data;
	xcvrStatus_t xcvrStatus;

	xcvrStatus = XCVR_Init(ZIGBEE_MODE, DR_500KBPS);
	if (xcvrStatus != gXcvrSuccess_c) {
		return -EIO;
	}

	/* Disable all timers, enable AUTOACK, mask all interrupts */
	ZLL->PHY_CTRL = ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1)	|
			ZLL_IRQSTS_WAKE_IRQ_MASK		|
			ZLL_PHY_CTRL_CRC_MSK_MASK		|
			ZLL_PHY_CTRL_PLL_UNLOCK_MSK_MASK	|
			ZLL_PHY_CTRL_FILTERFAIL_MSK_MASK	|
			ZLL_PHY_CTRL_CCAMSK_MASK		|
			ZLL_PHY_CTRL_RXMSK_MASK			|
			ZLL_PHY_CTRL_TXMSK_MASK			|
			ZLL_PHY_CTRL_SEQMSK_MASK;
#if KW41Z_AUTOACK_ENABLED
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_AUTOACK_MASK;
#endif

	/*
	 * Clear all PP IRQ bits to avoid unexpected interrupts immediately
	 * after init disable all timer interrupts
	 */
	ZLL->IRQSTS = ZLL->IRQSTS;

	/* Clear HW indirect queue */
	ZLL->SAM_TABLE |= ZLL_SAM_TABLE_INVALIDATE_ALL_MASK;

	/* Accept FrameVersion 0 and 1 packets, reject all others */
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_PROMISCUOUS_MASK;
	ZLL->RX_FRAME_FILTER &= ~ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_MASK;
	ZLL->RX_FRAME_FILTER = ZLL_RX_FRAME_FILTER_FRM_VER_FILTER(3)	|
			       ZLL_RX_FRAME_FILTER_CMD_FT_MASK		|
			       ZLL_RX_FRAME_FILTER_DATA_FT_MASK		|
			       ZLL_RX_FRAME_FILTER_BEACON_FT_MASK;

	/* Set prescaller to obtain 1 symbol (16us) timebase */
	ZLL->TMR_PRESCALE = 0x05;

	kw41z_tmr2_disable();
	kw41z_tmr1_disable();

	/* Compute warmup times (scaled to 16us) */
	kw41z->rx_warmup_time = (XCVR_TSM->END_OF_SEQ &
				 XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >>
				XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
	kw41z->tx_warmup_time = (XCVR_TSM->END_OF_SEQ &
				 XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK) >>
				XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT;

	if (kw41z->rx_warmup_time & 0x0F) {
		kw41z->rx_warmup_time = 1 + (kw41z->rx_warmup_time >> 4);
	} else {
		kw41z->rx_warmup_time = kw41z->rx_warmup_time >> 4;
	}

	if (kw41z->tx_warmup_time & 0x0F) {
		kw41z->tx_warmup_time = 1 + (kw41z->tx_warmup_time >> 4);
	} else {
		kw41z->tx_warmup_time = kw41z->tx_warmup_time >> 4;
	}

	/* Set CCA threshold to -75 dBm */
	ZLL->CCA_LQI_CTRL &= ~ZLL_CCA_LQI_CTRL_CCA1_THRESH_MASK;
	ZLL->CCA_LQI_CTRL |= ZLL_CCA_LQI_CTRL_CCA1_THRESH(0xB5);

	/* Set the default power level */
	kw41z_set_txpower(dev, 0);

	/* Adjust ACK delay to fulfill the 802.15.4 turnaround requirements */
	ZLL->ACKDELAY &= ~ZLL_ACKDELAY_ACKDELAY_MASK;
	ZLL->ACKDELAY |= ZLL_ACKDELAY_ACKDELAY(-8);

	/* Adjust LQI compensation */
	ZLL->CCA_LQI_CTRL &= ~ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_MASK;
	ZLL->CCA_LQI_CTRL |= ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP(96);

	/* Set default channel to 2405 MHZ */
	kw41z_set_channel(dev, KW41Z_DEFAULT_CHANNEL);

	/* Unmask Transceiver Global Interrupts */
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TRCV_MSK_MASK;

	/* Configre Radio IRQ */
	NVIC_ClearPendingIRQ(Radio_1_IRQn);
	IRQ_CONNECT(Radio_1_IRQn, RADIO_0_IRQ_PRIO, kw41z_isr, 0, 0);

	return 0;
}

static void kw41z_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct kw41z_context *kw41z = dev->driver_data;
	u8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
	kw41z->iface = iface;
	ieee802154_init(iface);
}

static struct ieee802154_radio_api kw41z_radio_api = {
	.iface_api.init	= kw41z_iface_init,
	.iface_api.send	= ieee802154_radio_send,

	.cca		= kw41z_cca,
	.set_channel	= kw41z_set_channel,
	.set_pan_id	= kw41z_set_pan_id,
	.set_short_addr	= kw41z_set_short_addr,
	.set_ieee_addr	= kw41z_set_ieee_addr,
	.set_txpower	= kw41z_set_txpower,
	.start		= kw41z_start,
	.stop		= kw41z_stop,
	.tx		= kw41z_tx,
	.get_lqi	= kw41z_get_lqi,
};

NET_DEVICE_INIT(kw41z, CONFIG_IEEE802154_KW41Z_DRV_NAME,
		kw41z_init, &kw41z_context_data, NULL,
		CONFIG_IEEE802154_KW41Z_INIT_PRIO,
		&kw41z_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2),
		KW41Z_PSDU_LENGTH);
