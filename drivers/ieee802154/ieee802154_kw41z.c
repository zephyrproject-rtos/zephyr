/* ieee802154_kw41z.c - NXP KW41Z driver */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kw41z_ieee802154

#define LOG_MODULE_NAME ieee802154_kw41z
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include "fsl_xcvr.h"

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif


/*
 * For non-invasive tracing of IRQ events. Sometimes the print logs
 * will shift the timings around so this trace buffer can be used to
 * post inspect conditions to see what sequence of events occurred.
 */

#define KW41_DBG_TRACE_WTRM	0
#define KW41_DBG_TRACE_RX	1
#define KW41_DBG_TRACE_TX	2
#define KW41_DBG_TRACE_CCA	3
#define KW41_DBG_TRACE_TMR3	0xFF

#if defined(CONFIG_KW41_DBG_TRACE)

#define KW41_DBG_TRACE_SIZE 30

struct kw41_dbg_trace {
	uint8_t	type;
	uint32_t	time;
	uint32_t	irqsts;
	uint32_t	phy_ctrl;
	uint32_t	seq_state;
};

struct kw41_dbg_trace kw41_dbg[KW41_DBG_TRACE_SIZE];
int kw41_dbg_idx;

#define KW_DBG_TRACE(_type, _irqsts, _phy_ctrl, _seq_state) \
	do { \
		kw41_dbg[kw41_dbg_idx].type = (_type); \
		kw41_dbg[kw41_dbg_idx].time = \
			ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT; \
		kw41_dbg[kw41_dbg_idx].irqsts = (_irqsts); \
		kw41_dbg[kw41_dbg_idx].phy_ctrl = (_phy_ctrl); \
		kw41_dbg[kw41_dbg_idx].seq_state = (_seq_state); \
		if (++kw41_dbg_idx == KW41_DBG_TRACE_SIZE) { \
			kw41_dbg_idx = 0; \
		} \
	} while (false)

#else

#define KW_DBG_TRACE(_type, _irqsts, _phy_ctrl, _seq_state)

#endif

#define KW41Z_DEFAULT_CHANNEL		26
#define KW41Z_CCA_TIME			8
#define KW41Z_SHR_PHY_TIME		12
#define KW41Z_PER_BYTE_TIME		2
#define KW41Z_ACK_WAIT_TIME		54
#define KW41Z_PRE_RX_WAIT_TIME		1
#define KW40Z_POST_SEQ_WAIT_TIME	1

#define RADIO_0_IRQ_PRIO		0x0
#define KW41Z_FCS_LENGTH		2
#define KW41Z_PSDU_LENGTH		125
#define KW41Z_OUTPUT_POWER_MAX		4
#define KW41Z_OUTPUT_POWER_MIN		(-31)

#define IEEE802154_ACK_LENGTH		5

#define BM_ZLL_IRQSTS_TMRxMSK (ZLL_IRQSTS_TMR1MSK_MASK | \
				ZLL_IRQSTS_TMR2MSK_MASK | \
				ZLL_IRQSTS_TMR3MSK_MASK | \
				ZLL_IRQSTS_TMR4MSK_MASK)

/*
 * Clear channel assessment types. Note that there is an extra one when
 * bit 26 is included for "No CCA before transmit" if we are handling
 * ACK frames but we will let the hardware handle that automatically.
 */
enum {
	KW41Z_CCA_ED,       /* Energy detect */
	KW41Z_CCA_MODE1,    /* Energy above threshold   */
	KW41Z_CCA_MODE2,    /* Carrier sense only       */
	KW41Z_CCA_MODE3     /* Mode 1 + Mode 2          */
};

/*
 * KW41Z has a sequencer that can run in any of the following states.
 */
enum {
	KW41Z_STATE_IDLE,
	KW41Z_STATE_RX,
	KW41Z_STATE_TX,
	KW41Z_STATE_CCA,
	KW41Z_STATE_TXRX,
	KW41Z_STATE_CCCA
};

/* Lookup table for PA_PWR register */
static const uint8_t pa_pwr_lt[] = {
	1,                   /* -31.1 dBm: -31 */
	2, 2, 2, 2, 2, 2, 2, /* -25.0 dBm: -30, -29, -28, -27, -26, -25 */
	4, 4, 4, 4, 4,       /* -19.0 dBm: -24, -23, -22, -21, -20, -19 */
	6, 6, 6,             /* -15.6 dBm: -18, -17, -16 */
	8, 8,                /* -13.1 dBm: -15, -14 */
	10, 10,              /* -11.2 dBm: -13, -12 */
	12, 12,              /* - 9.6 dBm: -11, -10 */
	14,                  /* - 8.3 dBm: -9 */
	16,                  /* - 7.2 dBm: -8 */
	18,                  /* - 6.2 dBm: -7 */
	20,                  /* - 5.3 dBm: -6 */
	22,                  /* - 4.5 dBm: -5 */
	24,                  /* - 3.8 dBm: -4 */
	28,                  /* - 2.5 dBm: -3 */
	30,                  /* - 1.9 dBm: -2 */
	34,                  /* - 1.0 dBm: -1 */
	40,                  /* + 0.3 dBm:  0 */
	44,                  /* + 1.1 dBm: +1 */
	50,                  /* + 2.1 dBm: +2 */
	58,                  /* + 3.1 dBm: +3 */
	62                   /* + 3.5 dBm: +4 */
};

struct kw41z_context {
	struct net_if *iface;
	uint8_t mac_addr[8];

	struct k_sem seq_sync;
	atomic_t seq_retval;

	uint32_t rx_warmup_time;
	uint32_t tx_warmup_time;

	bool frame_pending; /* FP bit state from the most recent ACK frame. */
};

static struct kw41z_context kw41z_context_data;

static inline uint8_t kw41z_get_instant_state(void)
{
	return (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK) >>
	       ZLL_SEQ_STATE_SEQ_STATE_SHIFT;
}

static inline uint8_t kw41z_get_seq_state(void)
{
	return (ZLL->PHY_CTRL & ZLL_PHY_CTRL_XCVSEQ_MASK) >>
	       ZLL_PHY_CTRL_XCVSEQ_SHIFT;
}

static inline void kw41z_set_seq_state(uint8_t state)
{
#if CONFIG_SOC_MKW40Z4
	/*
	 * KW40Z seems to require a small delay when switching to IDLE state
	 * after a programmed sequence is complete.
	 */
	if (state == KW41Z_STATE_IDLE) {
		k_busy_wait(KW40Z_POST_SEQ_WAIT_TIME);
	}
#endif

	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_XCVSEQ_MASK) |
			ZLL_PHY_CTRL_XCVSEQ(state);
}

static inline void kw41z_wait_for_idle(void)
{
	uint8_t state = kw41z_get_instant_state();

	while (state != KW41Z_STATE_IDLE) {
		state = kw41z_get_instant_state();
	}

	if (state != KW41Z_STATE_IDLE) {
		LOG_ERR("Error waiting for idle state");
	}
}

static void kw41z_phy_abort(void)
{
	unsigned int key;

	key = irq_lock();

	/* Mask SEQ interrupt */
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_SEQMSK_MASK;
	/* Disable timer trigger (for scheduled XCVSEQ) */
	if (ZLL->PHY_CTRL & ZLL_PHY_CTRL_TMRTRIGEN_MASK) {
		ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMRTRIGEN_MASK;
		/* give the FSM enough time to start if it was triggered */
		while ((XCVR_MISC->XCVR_CTRL &
			XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK) == 0) {
		}
	}

	/* If XCVR is not idle, abort current SEQ */
	if (ZLL->PHY_CTRL & ZLL_PHY_CTRL_XCVSEQ_MASK) {
		ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_XCVSEQ_MASK;
		/* wait for Sequence Idle (if not already) */

		while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK) {
		}
	}

	/* Stop timers */
	ZLL->PHY_CTRL &= ~(ZLL_PHY_CTRL_TMR1CMP_EN_MASK |
			ZLL_PHY_CTRL_TMR2CMP_EN_MASK |
			ZLL_PHY_CTRL_TMR3CMP_EN_MASK |
			ZLL_PHY_CTRL_TC3TMOUT_MASK);

	/*
	 * Clear all IRQ bits to avoid unexpected interrupts.
	 *
	 * For Coverity, this is a pointer to a register bank and the IRQSTS
	 * register bits get cleared when a 1 is written to them so doing a
	 * reg=reg may generate a warning but it is needed to clear the bits.
	 */
	ZLL->IRQSTS = ZLL->IRQSTS;

	irq_unlock(key);
}

static void kw41z_isr_timeout_cleanup(void)
{
	uint32_t irqsts;

	/*
	 * Set the PHY sequencer back to IDLE and disable TMR3 comparator
	 * and timeout
	 */
	ZLL->PHY_CTRL &= ~(ZLL_PHY_CTRL_TMR3CMP_EN_MASK |
			ZLL_PHY_CTRL_TC3TMOUT_MASK   |
			ZLL_PHY_CTRL_XCVSEQ_MASK);

	/* Mask SEQ, RX, TX and CCA interrupts */
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_CCAMSK_MASK |
			ZLL_PHY_CTRL_RXMSK_MASK  |
			ZLL_PHY_CTRL_TXMSK_MASK  |
			ZLL_PHY_CTRL_SEQMSK_MASK;

	while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK) {
	}

	irqsts = ZLL->IRQSTS;
	/* Mask TMR3 interrupt */
	irqsts |= ZLL_IRQSTS_TMR3MSK_MASK;

	ZLL->IRQSTS = irqsts;
}

static void kw41z_isr_seq_cleanup(void)
{
	uint32_t irqsts;

	/* Set the PHY sequencer back to IDLE */
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_XCVSEQ_MASK;
	/* Mask SEQ, RX, TX and CCA interrupts */
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_CCAMSK_MASK |
			ZLL_PHY_CTRL_RXMSK_MASK  |
			ZLL_PHY_CTRL_TXMSK_MASK  |
			ZLL_PHY_CTRL_SEQMSK_MASK;

	while (ZLL->SEQ_STATE & ZLL_SEQ_STATE_SEQ_STATE_MASK) {
	}

	irqsts = ZLL->IRQSTS;
	/* Mask TMR3 interrupt */
	irqsts |= ZLL_IRQSTS_TMR3MSK_MASK;

	/* Clear transceiver interrupts except TMRxIRQ */
	irqsts &= ~(ZLL_IRQSTS_TMR1IRQ_MASK |
		ZLL_IRQSTS_TMR2IRQ_MASK |
		ZLL_IRQSTS_TMR3IRQ_MASK |
		ZLL_IRQSTS_TMR4IRQ_MASK);
	ZLL->IRQSTS = irqsts;
}

static inline void kw41z_enable_seq_irq(void)
{
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_SEQMSK_MASK;
}

static inline void kw41z_disable_seq_irq(void)
{
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_SEQMSK_MASK;
}

/*
 * Set the T3CMP timer comparator. The 'timeout' value is an offset from
 * now.
 */
static void kw41z_tmr3_set_timeout(uint32_t timeout)
{
	uint32_t irqsts;

	/* Add in the current time so that we can get the comparator to
	 * match appropriately to our offset time.
	 */
	timeout += ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT;

	/* disable TMR3 compare */
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TMR3CMP_EN_MASK;
	ZLL->T3CMP = timeout & ZLL_T3CMP_T3CMP_MASK;

	/* acknowledge TMR3 IRQ */
	irqsts  = ZLL->IRQSTS & BM_ZLL_IRQSTS_TMRxMSK;
	irqsts |= ZLL_IRQSTS_TMR3IRQ_MASK;
	ZLL->IRQSTS = irqsts;
	/* enable TMR3 compare and autosequence stop by TC3 match */
	ZLL->PHY_CTRL |=
		(ZLL_PHY_CTRL_TMR3CMP_EN_MASK | ZLL_PHY_CTRL_TC3TMOUT_MASK);
}

static void kw41z_tmr3_disable(void)
{
	uint32_t irqsts;

	/*
	 * disable TMR3 compare and disable autosequence stop by TC3
	 * match
	 */
	ZLL->PHY_CTRL &= ~(ZLL_PHY_CTRL_TMR3CMP_EN_MASK |
			ZLL_PHY_CTRL_TC3TMOUT_MASK);
	/* mask TMR3 interrupt (do not change other IRQ status) */
	irqsts  = ZLL->IRQSTS & BM_ZLL_IRQSTS_TMRxMSK;
	irqsts |= ZLL_IRQSTS_TMR3MSK_MASK;
	/* acknowledge TMR3 IRQ */
	irqsts |= ZLL_IRQSTS_TMR3IRQ_MASK;

	ZLL->IRQSTS = irqsts;
}

static enum ieee802154_hw_caps kw41z_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_RX_TX_ACK;
}

static int kw41z_cca(const struct device *dev)
{
	struct kw41z_context *kw41z = dev->data;

	kw41z_phy_abort();

	k_sem_init(&kw41z->seq_sync, 0, 1);

	kw41z_enable_seq_irq();
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_CCATYPE_MASK) |
			ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1);

	kw41z_set_seq_state(KW41Z_STATE_CCA);

	k_sem_take(&kw41z->seq_sync, K_FOREVER);

	return kw41z->seq_retval;
}

static int kw41z_set_channel(const struct device *dev, uint16_t channel)
{
	if (channel < 11 || channel > 26) {
		return channel < 11 ? -ENOTSUP : -EINVAL;
	}

	ZLL->CHANNEL_NUM0 = channel;
	return 0;
}

static int kw41z_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	ZLL->MACSHORTADDRS0 = (ZLL->MACSHORTADDRS0 &
			       ~ZLL_MACSHORTADDRS0_MACPANID0_MASK) |
			      ZLL_MACSHORTADDRS0_MACPANID0(pan_id);
	return 0;
}

static int kw41z_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	ZLL->MACSHORTADDRS0 = (ZLL->MACSHORTADDRS0 &
			       ~ZLL_MACSHORTADDRS0_MACSHORTADDRS0_MASK) |
			      ZLL_MACSHORTADDRS0_MACSHORTADDRS0(short_addr);
	return 0;
}

static int kw41z_set_ieee_addr(const struct device *dev,
			       const uint8_t *ieee_addr)
{
	uint32_t val;

	memcpy(&val, ieee_addr, sizeof(val));
	ZLL->MACLONGADDRS0_LSB = val;

	memcpy(&val, ieee_addr + sizeof(val), sizeof(val));
	ZLL->MACLONGADDRS0_MSB = val;

	return 0;
}

static int kw41z_filter(const struct device *dev,
			bool set,
			enum ieee802154_filter_type type,
			const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return kw41z_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return kw41z_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return kw41z_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int kw41z_set_txpower(const struct device *dev, int16_t dbm)
{
	if (dbm < KW41Z_OUTPUT_POWER_MIN) {
		LOG_INF("TX-power %d dBm below min of %d dBm, using %d dBm",
			    dbm,
			    KW41Z_OUTPUT_POWER_MIN,
			    KW41Z_OUTPUT_POWER_MIN);
		dbm = KW41Z_OUTPUT_POWER_MIN;
	} else if (dbm > KW41Z_OUTPUT_POWER_MAX) {
		LOG_INF("TX-power %d dBm above max of %d dBm, using %d dBm",
			    dbm,
			    KW41Z_OUTPUT_POWER_MAX,
			    KW41Z_OUTPUT_POWER_MAX);
		dbm = KW41Z_OUTPUT_POWER_MAX;
	}

	ZLL->PA_PWR = pa_pwr_lt[dbm - KW41Z_OUTPUT_POWER_MIN];

	return 0;
}

static int kw41z_start(const struct device *dev)
{
	irq_enable(Radio_1_IRQn);

	kw41z_set_seq_state(KW41Z_STATE_RX);
	kw41z_enable_seq_irq();

	return 0;
}

static int kw41z_stop(const struct device *dev)
{
	irq_disable(Radio_1_IRQn);

	kw41z_disable_seq_irq();
	kw41z_set_seq_state(KW41Z_STATE_IDLE);

	return 0;
}

static uint8_t kw41z_convert_lqi(uint8_t hw_lqi)
{
	if (hw_lqi >= 220U) {
		return 255;
	} else {
		return (hw_lqi * 51U) / 44;
	}
}

static inline void kw41z_rx(struct kw41z_context *kw41z, uint8_t len)
{
	struct net_pkt *pkt = NULL;
	struct net_buf *buf = NULL;
	uint8_t pkt_len, hw_lqi;
	int rslt;

	LOG_DBG("ENTRY: len: %d", len);

#if defined(CONFIG_NET_L2_OPENTHREAD)
	/*
	 * OpenThread stack expects a receive frame to include the FCS
	 */
	pkt_len = len;
#else
	pkt_len = len - KW41Z_FCS_LENGTH;
#endif

	pkt = net_pkt_rx_alloc_with_buffer(kw41z->iface, pkt_len,
					   NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No buf available");
		goto out;
	}

	buf = pkt->buffer;

#if CONFIG_SOC_MKW41Z4
	/* PKT_BUFFER_RX needs to be accessed aligned to 16 bits */
	for (uint16_t reg_val = 0, i = 0; i < pkt_len; i++) {
		if (i % 2 == 0U) {
			reg_val = ZLL->PKT_BUFFER_RX[i/2U];
			buf->data[i] = reg_val & 0xFF;
		} else {
			buf->data[i] = reg_val >> 8;
		}
	}
#else /* CONFIG_SOC_MKW40Z4 */
	/* PKT_BUFFER needs to be accessed aligned to 32 bits */
	for (uint32_t reg_val = 0, i = 0; i < pkt_len; i++) {
		switch (i % 4) {
		case 0:
			reg_val = ZLL->PKT_BUFFER[i/4U];
			buf->data[i] = reg_val & 0xFF;
			break;
		case 1:
			buf->data[i] = (reg_val >> 8) & 0xFF;
			break;
		case 2:
			buf->data[i] = (reg_val >> 16) & 0xFF;
			break;
		default:
			buf->data[i] = reg_val >> 24;
		}
	}
#endif

	net_buf_add(buf, pkt_len);

	hw_lqi = (ZLL->LQI_AND_RSSI & ZLL_LQI_AND_RSSI_LQI_VALUE_MASK) >>
		 ZLL_LQI_AND_RSSI_LQI_VALUE_SHIFT;
	net_pkt_set_ieee802154_lqi(pkt, kw41z_convert_lqi(hw_lqi));
	/* ToDo: get the rssi as well and use net_pkt_set_ieee802154_rssi() */

	rslt = net_recv_data(kw41z->iface, pkt);
	if (rslt < 0) {
		LOG_ERR("RCV Packet dropped by NET stack: %d", rslt);
		goto out;
	}

	return;
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

#define ACK_FRAME_LEN 3
#define ACK_FRAME_TYPE (2 << 0)
#define ACK_FRAME_PENDING_BIT (1 << 4)

static void handle_ack(struct kw41z_context *kw41z, uint8_t seq_number)
{
	struct net_pkt *ack_pkt;
	uint8_t ack_psdu[ACK_FRAME_LEN];

	ack_pkt = net_pkt_rx_alloc_with_buffer(kw41z->iface, ACK_FRAME_LEN,
					       NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		return;
	}

	/* Re-create ACK frame. */
	ack_psdu[0] = kw41z_context_data.frame_pending ?
		      ACK_FRAME_TYPE | ACK_FRAME_PENDING_BIT : ACK_FRAME_TYPE;
	ack_psdu[1] = 0;
	ack_psdu[2] = seq_number;

	if (net_pkt_write(ack_pkt, ack_psdu, sizeof(ack_psdu)) < 0) {
		LOG_ERR("Failed to write to a packet.");
		goto out;
	}

	/* Use some fake values for LQI and RSSI. */
	(void)net_pkt_set_ieee802154_lqi(ack_pkt, 80);
	(void)net_pkt_set_ieee802154_rssi_dbm(ack_pkt, -40);

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_handle_ack(kw41z->iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

out:
	net_pkt_unref(ack_pkt);
}

static int kw41z_tx(const struct device *dev, enum ieee802154_tx_mode mode,
		    struct net_pkt *pkt, struct net_buf *frag)
{
	struct kw41z_context *kw41z = dev->data;
	uint8_t payload_len = frag->len;
	uint32_t tx_timeout;
	uint8_t xcvseq;
	unsigned int key;

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	/*
	 * The transmit requests are preceded by the CCA request. On
	 * completion of the CCA the sequencer should be in the IDLE
	 * state.
	 */
	if (kw41z_get_seq_state() != KW41Z_STATE_IDLE) {
		LOG_WRN("Can't initiate new SEQ state");
		return -EBUSY;
	}

	if (payload_len > KW41Z_PSDU_LENGTH) {
		LOG_ERR("Payload too long");
		return 0;
	}

	key = irq_lock();

	/* Disable the 802.15.4 radio IRQ */
	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_TRCV_MSK_MASK;
	kw41z_disable_seq_irq();

#if CONFIG_SOC_MKW41Z4
	((uint8_t *)ZLL->PKT_BUFFER_TX)[0] = payload_len + KW41Z_FCS_LENGTH;
	memcpy(((uint8_t *)ZLL->PKT_BUFFER_TX) + 1,
		(void *)frag->data, payload_len);
#else /* CONFIG_SOC_MKW40Z4 */
	((uint8_t *)ZLL->PKT_BUFFER)[0] = payload_len + KW41Z_FCS_LENGTH;
	memcpy(((uint8_t *)ZLL->PKT_BUFFER) + 1,
		(void *)frag->data, payload_len);
#endif

	/* Set CCA mode */
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_CCATYPE_MASK) |
			ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1);

	/* Clear all IRQ flags */
	ZLL->IRQSTS = ZLL->IRQSTS;

	/* Perform automatic reception of ACK frame, if required */
	if (ieee802154_is_ar_flag_set(frag)) {
		tx_timeout = kw41z->tx_warmup_time + KW41Z_SHR_PHY_TIME +
				 payload_len * KW41Z_PER_BYTE_TIME + 10 +
				 KW41Z_ACK_WAIT_TIME;

		LOG_DBG("AUTOACK ENABLED: len: %d, timeout: %d, seq: %d",
			payload_len, tx_timeout, frag->data[2]);

		kw41z_tmr3_set_timeout(tx_timeout);
		ZLL->PHY_CTRL |= ZLL_PHY_CTRL_RXACKRQD_MASK;
		xcvseq = KW41Z_STATE_TXRX;
	} else {
		LOG_DBG("AUTOACK DISABLED: len: %d, seq: %d",
			payload_len, frag->data[2]);

		ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_RXACKRQD_MASK;
		xcvseq = KW41Z_STATE_TX;
	}

	kw41z_enable_seq_irq();
	/*
	 * PHY_CTRL is sensitive to multiple writes that can kick off
	 * the sequencer engine causing TX with AR request to send the
	 * TX frame multiple times.
	 *
	 * To minimize, ensure there is only one write to PHY_CTRL with
	 * TXRX sequence enable and the 802.15.4 radio IRQ.
	 */
	ZLL->PHY_CTRL = (ZLL->PHY_CTRL & ~ZLL_PHY_CTRL_TRCV_MSK_MASK) | xcvseq;
	irq_unlock(key);
	k_sem_take(&kw41z->seq_sync, K_FOREVER);

	if ((kw41z->seq_retval == 0) && ieee802154_is_ar_flag_set(frag)) {
		handle_ack(kw41z, frag->data[2]);
	}

	LOG_DBG("seq_retval: %ld", kw41z->seq_retval);
	return kw41z->seq_retval;
}

static void kw41z_isr(int unused)
{
	uint32_t irqsts = ZLL->IRQSTS;
	uint8_t state = kw41z_get_seq_state();
	uint8_t restart_rx = 1U;
	uint32_t rx_len;

	/*
	 * Variable is used in debug output to capture the state of the
	 * sequencer at interrupt.
	 */
	uint32_t seq_state = ZLL->SEQ_STATE;

	LOG_DBG("ENTRY: irqsts: 0x%08X, PHY_CTRL: 0x%08X, "
		"SEQ_STATE: 0x%08X, SEQ_CTRL: 0x%08X, TMR: %d, state: %d",
		irqsts, (unsigned int)ZLL->PHY_CTRL,
		(unsigned int)seq_state,
		(unsigned int)ZLL->SEQ_CTRL_STS,
		(unsigned int)(ZLL->EVENT_TMR >> ZLL_EVENT_TMR_EVENT_TMR_SHIFT),
		state);

	/* Clear interrupts */
	ZLL->IRQSTS = irqsts;

	if (irqsts & ZLL_IRQSTS_FILTERFAIL_IRQ_MASK) {
		LOG_DBG("Incoming RX failed packet filtering rules: "
			"CODE: 0x%08X, irqsts: 0x%08X, PHY_CTRL: 0x%08X, "
			"SEQ_STATE: 0x%08X, state: %d",
			(unsigned int)ZLL->FILTERFAIL_CODE,
			irqsts,
			(unsigned int)ZLL->PHY_CTRL,
			(unsigned int)seq_state, state);

		restart_rx = 0U;

	} else if ((!(ZLL->PHY_CTRL & ZLL_PHY_CTRL_RX_WMRK_MSK_MASK)) &&
	    (irqsts & ZLL_IRQSTS_RXWTRMRKIRQ_MASK)) {
		/*
		 * There is a bug in the KW41Z where in noisy environments
		 * the RX sequence can get lost. The watermark mask IRQ can
		 * start TMR3 to complete the rest of the read or to assert
		 * IRQ if the sequencer gets lost so we can reset things.
		 * Note that a TX from the upper layers will also reset
		 * things so the problem is contained a bit in normal
		 * operation.
		 */
		rx_len = (irqsts & ZLL_IRQSTS_RX_FRAME_LENGTH_MASK)
			>> ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT;

		KW_DBG_TRACE(KW41_DBG_TRACE_WTRM, irqsts,
			(unsigned int)ZLL->PHY_CTRL, seq_state);

		if (rx_len > IEEE802154_ACK_LENGTH) {

			LOG_DBG("WMRK irq: seq_state: 0x%08x, rx_len: %d",
				seq_state, rx_len);
			/*
			 * Assume the RX includes an auto-ACK so set the
			 * timer to include the RX frame size, crc, IFS,
			 * and ACK length and convert to symbols.
			 *
			 * IFS is 12 symbols
			 *
			 * ACK frame is 11 bytes: 4 preamble, 1 start of
			 * frame, 1 frame length, 2 frame control,
			 * 1 sequence, 2 FCS. Times two to convert to symbols.
			 */
			rx_len = rx_len * 2U + 12 + 22 + 2;
			kw41z_tmr3_set_timeout(rx_len);
		}
		restart_rx = 0U;
	}

	/* Sequence done IRQ */
	if ((state != KW41Z_STATE_IDLE) && (irqsts & ZLL_IRQSTS_SEQIRQ_MASK)) {
		/*
		 * PLL unlock, the autosequence has been aborted due to
		 * PLL unlock
		 */
		if (irqsts & ZLL_IRQSTS_PLL_UNLOCK_IRQ_MASK) {
			LOG_ERR("PLL unlock error");
			kw41z_isr_seq_cleanup();
			restart_rx = 1U;
		}
		/*
		 * TMR3 timeout, the autosequence has been aborted due to
		 * TMR3 timeout
		 */
		else if ((irqsts & ZLL_IRQSTS_TMR3IRQ_MASK) &&
			(!(irqsts & ZLL_IRQSTS_RXIRQ_MASK)) &&
			(state != KW41Z_STATE_TX)) {

			LOG_DBG("a) TMR3 timeout: irqsts: 0x%08X, "
				"seq_state: 0x%08X, PHY_CTRL: 0x%08X, "
				"state: %d",
				irqsts, seq_state,
				(unsigned int)ZLL->PHY_CTRL, state);

			KW_DBG_TRACE(KW41_DBG_TRACE_TMR3, irqsts,
				(unsigned int)ZLL->PHY_CTRL, seq_state);

			kw41z_isr_timeout_cleanup();
			restart_rx = 1U;

			if (state == KW41Z_STATE_TXRX) {
				/* TODO: What is the right error for no ACK? */
				atomic_set(&kw41z_context_data.seq_retval,
					   -EBUSY);
				k_sem_give(&kw41z_context_data.seq_sync);
			}
		} else {
			kw41z_isr_seq_cleanup();

			switch (state) {
			case KW41Z_STATE_RX:
				LOG_DBG("RX seq done: SEQ_STATE: 0x%08X",
					(unsigned int)seq_state);

				KW_DBG_TRACE(KW41_DBG_TRACE_RX, irqsts,
					(unsigned int)ZLL->PHY_CTRL, seq_state);

				kw41z_tmr3_disable();

				rx_len = (ZLL->IRQSTS &
					  ZLL_IRQSTS_RX_FRAME_LENGTH_MASK) >>
					  ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT;

				if (irqsts & ZLL_IRQSTS_RXIRQ_MASK) {
					if (rx_len != 0U) {
						kw41z_rx(&kw41z_context_data,
							rx_len);
					}
				}
				restart_rx = 1U;
				break;
			case KW41Z_STATE_TXRX:
				LOG_DBG("TXRX seq done");
				kw41z_tmr3_disable();
				/* Store the frame pending bit status. */
				kw41z_context_data.frame_pending =
					irqsts & ZLL_IRQSTS_RX_FRM_PEND_MASK;
			case KW41Z_STATE_TX:
				LOG_DBG("TX seq done");
				KW_DBG_TRACE(KW41_DBG_TRACE_TX, irqsts,
					(unsigned int)ZLL->PHY_CTRL, seq_state);
				if (irqsts & ZLL_IRQSTS_CCA_MASK) {
					atomic_set(
						&kw41z_context_data.seq_retval,
						-EBUSY);
				} else {
					atomic_set(
						&kw41z_context_data.seq_retval,
						0);
				}

				k_sem_give(&kw41z_context_data.seq_sync);
				restart_rx = 1U;

				break;
			case KW41Z_STATE_CCA:
				LOG_DBG("CCA seq done");
				KW_DBG_TRACE(KW41_DBG_TRACE_CCA, irqsts,
					(unsigned int)ZLL->PHY_CTRL, seq_state);
				if (irqsts & ZLL_IRQSTS_CCA_MASK) {
					atomic_set(
						&kw41z_context_data.seq_retval,
						-EBUSY);
					restart_rx = 1U;
				} else {
					atomic_set(
						&kw41z_context_data.seq_retval,
						0);
					restart_rx = 0U;
				}

				k_sem_give(&kw41z_context_data.seq_sync);
				break;
			default:
				LOG_DBG("Unhandled state: %d", state);
				restart_rx = 1U;
				break;
			}
		}
	} else {
		/* Timer 3 Compare Match */
		if ((irqsts & ZLL_IRQSTS_TMR3IRQ_MASK) &&
			(!(irqsts & ZLL_IRQSTS_TMR3MSK_MASK))) {

			LOG_DBG("b) TMR3 timeout: irqsts: 0x%08X, "
				"seq_state: 0x%08X, state: %d",
				irqsts, seq_state, state);

			kw41z_tmr3_disable();
			restart_rx = 0U;
			if (state != KW41Z_STATE_IDLE) {
				kw41z_isr_timeout_cleanup();
				restart_rx = 1U;
				/* If we are not running an automated
				 * sequence then handle event. TMR3 can expire
				 * during Recv/Ack sequence where the transmit
				 * of the ACK is not being interrupted.
				 */
			}
		}
	}

	/* Restart RX */
	if (restart_rx) {
		LOG_DBG("RESET RX");
		kw41z_phy_abort();
		kw41z_set_seq_state(KW41Z_STATE_RX);
		kw41z_enable_seq_irq();
	}
}

static inline uint8_t *get_mac(const struct device *dev)
{
	struct kw41z_context *kw41z = dev->data;

	/*
	 * The KW40Z has two 32-bit registers for the MAC address where
	 * 40 bits of the registers are factory programmed to be unique
	 * and the rest are to be assigned as the "company-specific" value.
	 * 802.15.4 defines a EUI-64 64-bit address with company specific
	 * being 24 or 36 bits with the unique value being 24 or 40 bits.
	 *
	 * TODO: Grab from RSIM->MAC_LSB/MAC_MSB for the unique 40 bits
	 *       and how to allow for a OUI portion?
	 */

	sys_rand_get(kw41z->mac_addr, sizeof(kw41z->mac_addr));

	/*
	 * Clear bit 0 to ensure it isn't a multicast address and set
	 * bit 1 to indicate address is locally administered and may
	 * not be globally unique.
	 */
	kw41z->mac_addr[0] = (kw41z->mac_addr[0] & ~0x01) | 0x02;

	return kw41z->mac_addr;
}

static int kw41z_init(const struct device *dev)
{
	struct kw41z_context *kw41z = dev->data;
	xcvrStatus_t xcvrStatus;

	xcvrStatus = XCVR_Init(ZIGBEE_MODE, DR_500KBPS);
	if (xcvrStatus != gXcvrSuccess_c) {
		return -EIO;
	}

	/* Disable all timers, enable AUTOACK, mask all interrupts */
	ZLL->PHY_CTRL = ZLL_PHY_CTRL_CCATYPE(KW41Z_CCA_MODE1)	|
			ZLL_PHY_CTRL_CRC_MSK_MASK		|
			ZLL_PHY_CTRL_PLL_UNLOCK_MSK_MASK	|
			/*ZLL_PHY_CTRL_FILTERFAIL_MSK_MASK	|*/
			ZLL_PHY_CTRL_RX_WMRK_MSK_MASK	|
			ZLL_PHY_CTRL_CCAMSK_MASK		|
			ZLL_PHY_CTRL_RXMSK_MASK			|
			ZLL_PHY_CTRL_TXMSK_MASK			|
			ZLL_PHY_CTRL_CCABFRTX_MASK		|
			ZLL_PHY_CTRL_SEQMSK_MASK;

#if CONFIG_SOC_MKW41Z4
	ZLL->PHY_CTRL |= ZLL_IRQSTS_WAKE_IRQ_MASK;
#endif

	ZLL->PHY_CTRL |= ZLL_PHY_CTRL_AUTOACK_MASK;

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
			       ZLL_RX_FRAME_FILTER_ACK_FT_MASK		|
			       ZLL_RX_FRAME_FILTER_BEACON_FT_MASK;

	/* Set prescaler to obtain 1 symbol (16us) timebase */
	ZLL->TMR_PRESCALE = 0x05;

	kw41z_tmr3_disable();

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

	/* Enable the RxWatermark IRQ  */
	ZLL->PHY_CTRL &= ~(ZLL_PHY_CTRL_RX_WMRK_MSK_MASK);
	/* Set Rx watermark level */
	ZLL->RX_WTR_MARK = 0;


	/* Set default channel to 2405 MHZ */
	kw41z_set_channel(dev, KW41Z_DEFAULT_CHANNEL);

	/* Unmask Transceiver Global Interrupts */
	ZLL->PHY_CTRL &= ~ZLL_PHY_CTRL_TRCV_MSK_MASK;

	/* Configure Radio IRQ */
	NVIC_ClearPendingIRQ(Radio_1_IRQn);
	IRQ_CONNECT(Radio_1_IRQn, RADIO_0_IRQ_PRIO, kw41z_isr, 0, 0);

	return 0;
}

static void kw41z_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct kw41z_context *kw41z = dev->data;
	uint8_t *mac = get_mac(dev);

#if defined(CONFIG_KW41_DBG_TRACE)
	kw41_dbg_idx = 0;
#endif

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
	kw41z->iface = iface;
	ieee802154_init(iface);
}

static int kw41z_configure(const struct device *dev,
			   enum ieee802154_config_type type,
			   const struct ieee802154_config *config)
{
	return 0;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int kw41z_attr_get(const struct device *dev, enum ieee802154_attr attr,
			  struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static const struct ieee802154_radio_api kw41z_radio_api = {
	.iface_api.init	= kw41z_iface_init,

	.get_capabilities	= kw41z_get_capabilities,
	.cca			= kw41z_cca,
	.set_channel		= kw41z_set_channel,
	.filter			= kw41z_filter,
	.set_txpower		= kw41z_set_txpower,
	.start			= kw41z_start,
	.stop			= kw41z_stop,
	.tx			= kw41z_tx,
	.configure		= kw41z_configure,
	.attr_get		= kw41z_attr_get,
};

#if defined(CONFIG_NET_L2_IEEE802154)

#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU KW41Z_PSDU_LENGTH

#elif defined(CONFIG_NET_L2_OPENTHREAD)

#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280

#endif

NET_DEVICE_DT_INST_DEFINE(
	0,
	kw41z_init,                         /* Initialization Function */
	NULL,              /* No PM API support */
	&kw41z_context_data,                /* Context data */
	NULL,                               /* Configuration info */
	CONFIG_IEEE802154_KW41Z_INIT_PRIO,  /* Initial priority */
	&kw41z_radio_api,                   /* API interface functions */
	L2,                                 /* L2 */
	L2_CTX_TYPE,                        /* L2 context type */
	MTU);                               /* MTU size */
