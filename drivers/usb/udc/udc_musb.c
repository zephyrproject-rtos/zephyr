/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include <string.h>

#include "udc_common.h"
#include "udc_musb.h"

LOG_MODULE_REGISTER(udc_musb, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define MUSB_EP_IN_OFFSET 16U

#define MUSB_RESUME_HOLD_MS 10U

#define MUSB_EP0_FIFO_BYTES USB_CONTROL_EP_MPS

#define MUSB_FIFO_ADDR_UNIT 8U

/* USB 2.0 maximum packet size, not the FIFO size. */
#define MUSB_EP_MPS_MAX 1024U

#define MUSB_FIFO_SIZE_MIN         8U
#define MUSB_FIFO_SIZE_MAX         4096U
#define MUSB_FIFO_SIZE_LOG2_OFFSET 3U

#define MUSB_FADDR           0x00U
#define MUSB_POWER           0x01U
#define MUSB_INTRTX          0x02U
#define MUSB_INTRRX          0x04U
#define MUSB_INTRTXE         0x06U
#define MUSB_INTRRXE         0x08U
#define MUSB_INTRUSB         0x0AU
#define MUSB_INTRUSBE        0x0BU
#define MUSB_INDEX           0x0EU
#define MUSB_TESTMODE        0x0FU
#define MUSB_TXMAXP          0x10U
#define MUSB_CSR0L           0x12U
#define MUSB_CSR0H           0x13U
#define MUSB_TXCSRL          0x12U
#define MUSB_TXCSRH          0x13U
#define MUSB_RXMAXP          0x14U
#define MUSB_RXCSRL          0x16U
#define MUSB_RXCSRH          0x17U
#define MUSB_COUNT0          0x18U
#define MUSB_RXCOUNT         0x18U
#define MUSB_FIFO_OFFSET(ep) (0x20U + ((ep) * 4U))
#define MUSB_TXFIFOSZ        0x62U
#define MUSB_RXFIFOSZ        0x63U
#define MUSB_TXFIFOADD       0x64U
#define MUSB_RXFIFOADD       0x66U
#define MUSB_EPINFO          0x78U
#define MUSB_RAMINFO         0x79U

#define MUSB_POWER_RESUME   BIT(2)
#define MUSB_POWER_HSMODE   BIT(4)
#define MUSB_POWER_HSENAB   BIT(5)
#define MUSB_POWER_SOFTCONN BIT(6)

#define MUSB_INTR_SUSPEND    BIT(0)
#define MUSB_INTR_RESUME     BIT(1)
#define MUSB_INTR_RESET      BIT(2)
#define MUSB_INTR_SOF        BIT(3)
#define MUSB_INTR_DISCONNECT BIT(5)

#define MUSB_TEST_SE0_NAK BIT(0)
#define MUSB_TEST_J       BIT(1)
#define MUSB_TEST_K       BIT(2)
#define MUSB_TEST_PACKET  BIT(3)

#define MUSB_CSR0_RXPKTRDY      BIT(0)
#define MUSB_CSR0_TXPKTRDY      BIT(1)
#define MUSB_CSR0_P_SENTSTALL   BIT(2)
#define MUSB_CSR0_P_DATAEND     BIT(3)
#define MUSB_CSR0_P_SETUPEND    BIT(4)
#define MUSB_CSR0_P_SENDSTALL   BIT(5)
#define MUSB_CSR0_P_SVDRXPKTRDY BIT(6)
#define MUSB_CSR0_P_SVDSETUPEND BIT(7)
#define MUSB_CSR0H_FLUSHFIFO    BIT(0)

#define MUSB_TXCSR_TXPKTRDY    BIT(0)
#define MUSB_TXCSR_P_UNDERRUN  BIT(2)
#define MUSB_TXCSR_FLUSHFIFO   BIT(3)
#define MUSB_TXCSR_P_SENDSTALL BIT(4)
#define MUSB_TXCSR_P_SENTSTALL BIT(5)
#define MUSB_TXCSR_CLRDATATOG  BIT(6)
#define MUSB_TXCSRH_MODE       BIT(5)

#define MUSB_RXCSR_RXPKTRDY    BIT(0)
#define MUSB_RXCSR_P_OVERRUN   BIT(2)
#define MUSB_RXCSR_FLUSHFIFO   BIT(4)
#define MUSB_RXCSR_P_SENDSTALL BIT(5)
#define MUSB_RXCSR_P_SENTSTALL BIT(6)
#define MUSB_RXCSR_CLRDATATOG  BIT(7)

enum musb_event_type {
	MUSB_EVT_SETUP,
	MUSB_EVT_XFER_NEW,
	MUSB_EVT_XFER_FINISHED,
};

static uint8_t musb_ep_to_bnum(const uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep);

	return (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) ? (MUSB_EP_IN_OFFSET + idx) : idx;
}

static uint8_t musb_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit_idx;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit_idx = find_lsb_set(*bitmap) - 1U;
	*bitmap &= ~BIT(bit_idx);

	if (bit_idx >= MUSB_EP_IN_OFFSET) {
		return USB_EP_DIR_IN | (uint8_t)(bit_idx - MUSB_EP_IN_OFFSET);
	}

	return USB_EP_DIR_OUT | (uint8_t)bit_idx;
}

static inline uint8_t musb_readb(mem_addr_t mbase, uint16_t off)
{
	return sys_read8(mbase + off);
}

static inline uint16_t musb_readw(mem_addr_t mbase, uint16_t off)
{
	return sys_read16(mbase + off);
}

static inline uint32_t musb_readl(mem_addr_t mbase, uint16_t off)
{
	return sys_read32(mbase + off);
}

static inline void musb_writeb(mem_addr_t mbase, uint16_t off, uint8_t val)
{
	sys_write8(val, mbase + off);
}

static inline void musb_writew(mem_addr_t mbase, uint16_t off, uint16_t val)
{
	sys_write16(val, mbase + off);
}

static inline void musb_writel(mem_addr_t mbase, uint16_t off, uint32_t val)
{
	sys_write32(val, mbase + off);
}

static inline mem_addr_t musb_mbase(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;

	return cfg->mbase;
}

static void musb_fifo_write(const struct device *dev, uint8_t idx, const uint8_t *data,
			    uint16_t len)
{
	const mem_addr_t mbase = musb_mbase(dev);

	while (len >= sizeof(uint32_t)) {
		uint32_t word;

		memcpy(&word, data, sizeof(word));
		musb_writel(mbase, MUSB_FIFO_OFFSET(idx), word);
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len != 0U) {
		musb_writeb(mbase, MUSB_FIFO_OFFSET(idx), *data++);
		len--;
	}
}

static void musb_fifo_read(const struct device *dev, uint8_t idx, uint8_t *data, uint16_t len)
{
	const mem_addr_t mbase = musb_mbase(dev);

	while (len >= sizeof(uint32_t)) {
		uint32_t word = musb_readl(mbase, MUSB_FIFO_OFFSET(idx));

		memcpy(data, &word, sizeof(word));
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	/* All reads of one packet must have the same width, the tail included. */
	if (len != 0U) {
		uint32_t word = musb_readl(mbase, MUSB_FIFO_OFFSET(idx));

		memcpy(data, &word, len);
	}
}

static void musb_fifo_discard(const struct device *dev, uint8_t idx, uint16_t len, uint16_t count)
{
	const mem_addr_t mbase = musb_mbase(dev);

	for (uint16_t i = DIV_ROUND_UP(len, 4U); i < DIV_ROUND_UP(count, 4U); i++) {
		(void)musb_readl(mbase, MUSB_FIFO_OFFSET(idx));
	}
}

/* FlushFIFO acts only with PktRdy set and must be written with it. */
#define MUSB_FIFO_FLUSH_PASSES 2U

static void musb_ep0_flush(const struct device *dev)
{
	const mem_addr_t mbase = musb_mbase(dev);

	if ((musb_readb(mbase, MUSB_CSR0L) & (MUSB_CSR0_RXPKTRDY | MUSB_CSR0_TXPKTRDY)) == 0U) {
		return;
	}

	musb_writeb(mbase, MUSB_CSR0H, MUSB_CSR0H_FLUSHFIFO);
}

static void musb_ep_tx_flush(const struct device *dev)
{
	const mem_addr_t mbase = musb_mbase(dev);

	for (uint8_t pass = 0U; pass < MUSB_FIFO_FLUSH_PASSES; pass++) {
		if ((musb_readb(mbase, MUSB_TXCSRL) & MUSB_TXCSR_TXPKTRDY) == 0U) {
			return;
		}

		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_TXPKTRDY | MUSB_TXCSR_FLUSHFIFO);
	}
}

static void musb_ep_rx_flush(const struct device *dev)
{
	const mem_addr_t mbase = musb_mbase(dev);

	for (uint8_t pass = 0U; pass < MUSB_FIFO_FLUSH_PASSES; pass++) {
		if ((musb_readb(mbase, MUSB_RXCSRL) & MUSB_RXCSR_RXPKTRDY) == 0U) {
			return;
		}

		musb_writeb(mbase, MUSB_RXCSRL, MUSB_RXCSR_RXPKTRDY | MUSB_RXCSR_FLUSHFIFO);
	}
}

static uint16_t musb_fifo_size(uint16_t mps)
{
	uint16_t size = MUSB_FIFO_SIZE_MIN;

	while (size < mps) {
		size *= 2U;
	}

	return size;
}

static uint8_t musb_fifo_size_encode(uint16_t size)
{
	return (uint8_t)(find_msb_set(size) - 1U - MUSB_FIFO_SIZE_LOG2_OFFSET);
}

static int musb_fifo_alloc(const struct device *dev, uint8_t ep, uint16_t size)
{
	const struct udc_musb_config *cfg = dev->config;
	const size_t slots = UDC_MUSB_FIFO_SLOTS(cfg->num_of_eps);
	uint16_t cand = MUSB_EP0_FIFO_BYTES;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));
	const struct udc_musb_fifo_block prev = cfg->fifo[slot];
	bool moved;

	cfg->fifo[slot].offset = 0U;
	cfg->fifo[slot].size = 0U;

	do {
		moved = false;

		for (size_t i = 0; i < slots; i++) {
			const struct udc_musb_fifo_block *blk = &cfg->fifo[i];

			if (blk->size == 0U) {
				continue;
			}

			if ((cand < (blk->offset + blk->size)) && (blk->offset < (cand + size))) {
				cand = blk->offset + blk->size;
				moved = true;
			}
		}
	} while (moved);

	if ((uint32_t)cand + size > cfg->fifo_ram_bytes) {
		LOG_ERR("No FIFO space for ep 0x%02x (%u bytes)", ep, size);
		cfg->fifo[slot] = prev;
		return -ENOMEM;
	}

	cfg->fifo[slot].offset = cand;
	cfg->fifo[slot].size = size;

	return 0;
}

static void musb_fifo_free(const struct device *dev, uint8_t ep)
{
	const struct udc_musb_config *cfg = dev->config;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));

	cfg->fifo[slot].size = 0U;
	cfg->fifo[slot].offset = 0U;
}

static uint16_t musb_fifo_offset(const struct device *dev, uint8_t ep)
{
	const struct udc_musb_config *cfg = dev->config;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));

	return cfg->fifo[slot].offset;
}

static bool musb_ep0_write(const struct device *dev, struct net_buf *const buf)
{
	const mem_addr_t mbase = musb_mbase(dev);
	uint16_t len = MIN(buf->len, USB_CONTROL_EP_MPS);
	uint8_t csr = MUSB_CSR0_TXPKTRDY;
	bool more;

	musb_fifo_write(dev, 0U, buf->data, len);
	net_buf_pull(buf, len);

	more = buf->len != 0U;

	if (!more && (len == USB_CONTROL_EP_MPS) && udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		more = true;
	}

	if (!more) {
		csr |= MUSB_CSR0_P_DATAEND;
	}

	musb_writeb(mbase, MUSB_CSR0L, csr);

	return !more;
}

static bool musb_ep0_read(const struct device *dev, struct net_buf *const buf)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	uint16_t count = musb_readb(mbase, MUSB_COUNT0);
	uint16_t len = MIN(count, net_buf_tailroom(buf));
	bool done;

	musb_fifo_read(dev, 0U, net_buf_tail(buf), len);
	net_buf_add(buf, len);

	if (count > len) {
		LOG_ERR("Control OUT overflow: %u bytes dropped", count - len);
		musb_fifo_discard(dev, 0U, len, count);
	}

	done = (count < USB_CONTROL_EP_MPS) || (buf->len >= priv->ep0_len) ||
	       (net_buf_tailroom(buf) == 0U);

	/* Hold the last packet's RxPktRdy: the status stage waits for the stack's verdict. */
	if (done) {
		priv->ep0_ack_pending = true;
	} else {
		musb_writeb(mbase, MUSB_CSR0L, MUSB_CSR0_P_SVDRXPKTRDY);
	}

	return done;
}

static void musb_xfer_finished(const struct device *dev, uint8_t ep)
{
	struct udc_musb_data *const priv = udc_get_private(dev);

	atomic_set_bit(&priv->xfer_finished, musb_ep_to_bnum(ep));
	k_event_post(&priv->events, BIT(MUSB_EVT_XFER_FINISHED));
}

static bool musb_ep0_finish_status(const struct device *dev)
{
	static const uint8_t eps[] = {USB_CONTROL_EP_IN, USB_CONTROL_EP_OUT};

	for (size_t i = 0; i < ARRAY_SIZE(eps); i++) {
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, eps[i]);
		struct net_buf *buf = udc_buf_peek(ep_cfg);
		struct udc_buf_info *bi;

		if (buf == NULL) {
			continue;
		}

		bi = udc_get_buf_info(buf);
		if (!bi->status) {
			continue;
		}

		musb_xfer_finished(dev, eps[i]);
		return true;
	}

	return false;
}

static void musb_ep0_setup(const struct device *dev)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const struct usb_setup_packet *setup;

	if (musb_readb(mbase, MUSB_COUNT0) != sizeof(struct usb_setup_packet)) {
		LOG_ERR("SETUP packet is %u bytes", musb_readb(mbase, MUSB_COUNT0));
		musb_writeb(mbase, MUSB_CSR0L, MUSB_CSR0_P_SVDRXPKTRDY | MUSB_CSR0_P_SENDSTALL);
		return;
	}

	musb_fifo_read(dev, 0U, priv->setup, sizeof(priv->setup));

	setup = (const struct usb_setup_packet *)priv->setup;
	priv->ep0_len = sys_le16_to_cpu(setup->wLength);

	priv->ep0_ack_pending = (priv->ep0_len == 0U);

	if (priv->ep0_ack_pending) {
		priv->ep0_stage = MUSB_EP0_STATUS;
	} else if (USB_REQTYPE_GET_DIR(setup->bmRequestType) == USB_REQTYPE_DIR_TO_HOST) {
		priv->ep0_stage = MUSB_EP0_DATA_IN;
	} else {
		priv->ep0_stage = MUSB_EP0_DATA_OUT;
	}

	if (!priv->ep0_ack_pending) {
		musb_writeb(mbase, MUSB_CSR0L, MUSB_CSR0_P_SVDRXPKTRDY);
	}

	priv->ep0_status_done = false;
	k_event_post(&priv->events, BIT(MUSB_EVT_SETUP));
}

static void musb_ep0_poll_setup(const struct device *dev)
{
	const mem_addr_t mbase = musb_mbase(dev);

	if (musb_readb(mbase, MUSB_CSR0L) & MUSB_CSR0_RXPKTRDY) {
		musb_ep0_setup(dev);
	}
}

/* CSR0L mixes write-one and write-zero bits: write computed values, never RMW. */
static void musb_ep0_isr(const struct device *dev)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	struct net_buf *buf;
	uint8_t csr0;

	musb_writeb(mbase, MUSB_INDEX, 0U);
	csr0 = musb_readb(mbase, MUSB_CSR0L);

	/* A stall before DataEnd raises SetupEnd too; the next SETUP cancels the rest. */
	if (csr0 & (MUSB_CSR0_P_SENTSTALL | MUSB_CSR0_P_SETUPEND)) {
		musb_writeb(mbase, MUSB_CSR0L,
			    (csr0 & MUSB_CSR0_P_SETUPEND) ? MUSB_CSR0_P_SVDSETUPEND : 0U);
		priv->ep0_stage = MUSB_EP0_SETUP;
		priv->ep0_status_done = false;
		priv->ep0_ack_pending = false;
		musb_ep0_poll_setup(dev);
		return;
	}

	switch (priv->ep0_stage) {
	case MUSB_EP0_SETUP:
		if ((csr0 & MUSB_CSR0_RXPKTRDY) == 0U) {
			break;
		}

		musb_ep0_setup(dev);
		break;

	case MUSB_EP0_DATA_IN:
		if (csr0 & MUSB_CSR0_TXPKTRDY) {
			break;
		}

		buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
		if (buf == NULL) {
			LOG_ERR("No buffer for the control IN data stage");
			break;
		}

		if (musb_ep0_write(dev, buf)) {
			priv->ep0_stage = MUSB_EP0_STATUS;
			musb_xfer_finished(dev, USB_CONTROL_EP_IN);
		}
		break;

	case MUSB_EP0_DATA_OUT:
		if ((csr0 & MUSB_CSR0_RXPKTRDY) == 0U) {
			break;
		}

		/* Until the stack queues the data buffer, the head is the SETUP buffer. */
		buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
		if ((buf == NULL) || !udc_get_buf_info(buf)->data) {
			LOG_DBG("Control OUT data before a buffer, deferred");
			break;
		}

		if (musb_ep0_read(dev, buf)) {
			priv->ep0_stage = MUSB_EP0_STATUS;
			musb_xfer_finished(dev, USB_CONTROL_EP_OUT);
		}
		break;

	case MUSB_EP0_STATUS:
		if (priv->ep0_ack_pending) {
			break;
		}

		priv->ep0_stage = MUSB_EP0_SETUP;

		if (!musb_ep0_finish_status(dev)) {
			priv->ep0_status_done = true;
		}

		musb_ep0_poll_setup(dev);
		break;
	}
}

static void musb_ep_in_isr(const struct device *dev, const uint8_t idx)
{
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t ep = USB_EP_DIR_IN | idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;
	uint8_t csr = musb_readb(mbase, MUSB_TXCSRL);

	if (csr & MUSB_TXCSR_P_SENTSTALL) {
		musb_writeb(mbase, MUSB_TXCSRL,
			    csr & ~(MUSB_TXCSR_P_SENTSTALL | MUSB_TXCSR_P_UNDERRUN));
		return;
	}

	if (csr & MUSB_TXCSR_P_UNDERRUN) {
		musb_writeb(mbase, MUSB_TXCSRL, csr & ~MUSB_TXCSR_P_UNDERRUN);
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		return;
	}

	if (buf->len != 0U) {
		uint16_t len = MIN(buf->len, udc_mps_ep_size(ep_cfg));

		musb_fifo_write(dev, idx, buf->data, len);
		net_buf_pull(buf, len);
		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_TXPKTRDY);
		return;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_TXPKTRDY);
		return;
	}

	musb_xfer_finished(dev, ep);
}

static void musb_ep_out_isr(const struct device *dev, const uint8_t idx)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t ep = USB_EP_DIR_OUT | idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	uint8_t csr = musb_readb(mbase, MUSB_RXCSRL);
	struct net_buf *buf;
	uint16_t count;
	uint16_t len;

	if (csr & MUSB_RXCSR_P_SENTSTALL) {
		musb_writeb(mbase, MUSB_RXCSRL,
			    csr & ~(MUSB_RXCSR_P_SENTSTALL | MUSB_RXCSR_P_OVERRUN));
		return;
	}

	if ((csr & MUSB_RXCSR_RXPKTRDY) == 0U) {
		return;
	}

	if ((priv->rx_held & BIT(idx)) != 0U) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		/* RxPktRdy stays set so the core NAKs; start_xfer drains it later. */
		return;
	}

	count = musb_readw(mbase, MUSB_RXCOUNT);
	len = MIN(count, net_buf_tailroom(buf));

	musb_fifo_read(dev, idx, net_buf_tail(buf), len);
	net_buf_add(buf, len);

	if (count > len) {
		LOG_ERR("ep 0x%02x overflow: %u bytes dropped", ep, count - len);
		musb_fifo_discard(dev, idx, len, count);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
	}

	if ((count < udc_mps_ep_size(ep_cfg)) || (net_buf_tailroom(buf) == 0U)) {
		/* Keep RxPktRdy set, so the core NAKs, until the thread retires buf. */
		priv->rx_held |= BIT(idx);
		musb_xfer_finished(dev, ep);
		return;
	}

	musb_writeb(mbase, MUSB_RXCSRL, 0U);
}

static void musb_bus_isr(const struct device *dev, uint8_t intrusb)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);

	if (intrusb & MUSB_INTR_RESET) {
		musb_writeb(mbase, MUSB_FADDR, 0U);
		musb_writew(mbase, MUSB_INTRTXE, BIT(0));
		musb_writew(mbase, MUSB_INTRRXE, 0U);
		priv->ep0_stage = MUSB_EP0_SETUP;
		priv->ep0_status_done = false;
		priv->ep0_ack_pending = false;
		priv->rx_held = 0U;
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if ((intrusb & MUSB_INTR_SUSPEND) && !udc_is_suspended(dev)) {
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if ((intrusb & MUSB_INTR_RESUME) && udc_is_suspended(dev)) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (intrusb & MUSB_INTR_DISCONNECT) {
		LOG_DBG("Disconnect on %s", dev->name);
	}

	if (intrusb & MUSB_INTR_SOF) {
		udc_submit_sof_event(dev);
	}
}

/* INTRTX, INTRRX and INTRUSB clear on read: read each once. */
void udc_musb_isr(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	k_spinlock_key_t key;
	uint16_t intrtx;
	uint16_t intrrx;
	uint8_t intrusb;
	uint8_t index;

	key = k_spin_lock(&priv->lock);

	intrtx = musb_readw(mbase, MUSB_INTRTX);
	intrrx = musb_readw(mbase, MUSB_INTRRX);
	intrusb = musb_readb(mbase, MUSB_INTRUSB);

	index = musb_readb(mbase, MUSB_INDEX);

	if (intrtx & BIT(0)) {
		musb_ep0_isr(dev);
	}

	for (uint8_t idx = 1U; idx < cfg->num_of_eps; idx++) {
		if (intrtx & BIT(idx)) {
			musb_writeb(mbase, MUSB_INDEX, idx);
			musb_ep_in_isr(dev, idx);
		}

		if (intrrx & BIT(idx)) {
			musb_writeb(mbase, MUSB_INDEX, idx);
			musb_ep_out_isr(dev, idx);
		}
	}

	musb_writeb(mbase, MUSB_INDEX, index);

	k_spin_unlock(&priv->lock, key);

	if (intrusb != 0U) {
		musb_bus_isr(dev, intrusb);
	}
}

static void musb_start_xfer(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	struct net_buf *buf = udc_buf_peek(ep_cfg);
	struct udc_buf_info *bi;
	k_spinlock_key_t key;
	uint8_t index;

	if (buf == NULL) {
		return;
	}

	bi = udc_get_buf_info(buf);

	if (bi->setup) {
		return;
	}

	udc_ep_set_busy(ep_cfg, true);

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);
	musb_writeb(mbase, MUSB_INDEX, idx);

	if (idx == 0U) {
		if (bi->status) {
			if (priv->ep0_ack_pending) {
				priv->ep0_ack_pending = false;
				musb_writeb(mbase, MUSB_CSR0L,
					    MUSB_CSR0_P_SVDRXPKTRDY | MUSB_CSR0_P_DATAEND);
			} else if (priv->ep0_status_done) {
				priv->ep0_status_done = false;
				musb_xfer_finished(dev, ep_cfg->addr);
			}
		} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			if (priv->ep0_stage == MUSB_EP0_DATA_IN) {
				if (musb_ep0_write(dev, buf)) {
					priv->ep0_stage = MUSB_EP0_STATUS;
					musb_xfer_finished(dev, ep_cfg->addr);
				}
			}
		} else if (priv->ep0_stage == MUSB_EP0_DATA_OUT) {
			if (musb_readb(mbase, MUSB_CSR0L) & MUSB_CSR0_RXPKTRDY) {
				if (musb_ep0_read(dev, buf)) {
					priv->ep0_stage = MUSB_EP0_STATUS;
					musb_xfer_finished(dev, ep_cfg->addr);
				}
			}
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		uint16_t len = MIN(buf->len, udc_mps_ep_size(ep_cfg));

		musb_fifo_write(dev, idx, buf->data, len);
		net_buf_pull(buf, len);
		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_TXPKTRDY);
	} else if ((priv->rx_held & BIT(idx)) != 0U) {
		priv->rx_held &= ~BIT(idx);
		musb_writeb(mbase, MUSB_RXCSRL, 0U);
	} else {
		musb_ep_out_isr(dev, idx);
	}

	musb_writeb(mbase, MUSB_INDEX, index);
	k_spin_unlock(&priv->lock, key);
}

static void musb_handle_xfer_finished(const struct device *dev)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	uint32_t eps = atomic_clear(&priv->xfer_finished);

	while (eps != 0U) {
		uint8_t ep = musb_pull_ep_from_bmsk(&eps);
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
		struct net_buf *buf = udc_buf_get(ep_cfg);

		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			continue;
		}

		udc_ep_set_busy(ep_cfg, false);
		udc_submit_ep_event(dev, buf, 0);

		musb_start_xfer(dev, ep_cfg);
	}
}

static void musb_handle_xfer_new(const struct device *dev)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	uint32_t eps = atomic_clear(&priv->xfer_new);

	while (eps != 0U) {
		uint8_t ep = musb_pull_ep_from_bmsk(&eps);
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);

		if (udc_ep_is_busy(ep_cfg)) {
			continue;
		}

		musb_start_xfer(dev, ep_cfg);
	}
}

static void musb_thread_handler(const struct device *const dev)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	uint32_t evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);

	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(MUSB_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(MUSB_EVT_XFER_FINISHED));

		musb_handle_xfer_finished(dev);
	}

	if (evt & BIT(MUSB_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(MUSB_EVT_XFER_NEW));

		musb_handle_xfer_new(dev);
	}

	if (evt & BIT(MUSB_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(MUSB_EVT_SETUP));

		udc_setup_received(dev, priv->setup);
	}

	udc_unlock_internal(dev);
}

void udc_musb_thread(void *dev, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	while (true) {
		musb_thread_handler(dev);
	}
}

static int udc_musb_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
			       struct net_buf *buf)
{
	struct udc_musb_data *const priv = udc_get_private(dev);

	LOG_DBG("%s enqueue 0x%02x %p", dev->name, ep_cfg->addr, (void *)buf);
	udc_buf_put(ep_cfg, buf);

	if (!ep_cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, musb_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(MUSB_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_musb_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);
	musb_writeb(mbase, MUSB_INDEX, idx);

	if (idx == 0U) {
		/* Retire only this direction: the stage is shared by both halves. */
		const enum udc_musb_ep0_stage stage =
			USB_EP_DIR_IS_IN(ep_cfg->addr) ? MUSB_EP0_DATA_IN : MUSB_EP0_DATA_OUT;

		if (priv->ep0_stage == stage) {
			priv->ep0_stage = MUSB_EP0_SETUP;
			musb_ep0_flush(dev);
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		musb_ep_tx_flush(dev);
	} else {
		priv->rx_held &= ~BIT(idx);
		musb_ep_rx_flush(dev);
	}

	musb_writeb(mbase, MUSB_INDEX, index);

	/* Cancel before unlocking: the flush raises an interrupt for this endpoint. */
	atomic_clear_bit(&priv->xfer_finished, musb_ep_to_bnum(ep_cfg->addr));
	udc_ep_cancel_queued(dev, ep_cfg);
	udc_ep_set_busy(ep_cfg, false);

	k_spin_unlock(&priv->lock, key);

	return 0;
}

static int udc_musb_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	uint16_t size;
	k_spinlock_key_t key;
	uint8_t index;
	int ret = 0;

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);

	if (idx == 0U) {
		musb_writew(mbase, MUSB_INTRTXE, musb_readw(mbase, MUSB_INTRTXE) | BIT(0));
		goto out;
	}

	if (mps > MUSB_FIFO_SIZE_MAX) {
		ret = -EINVAL;
		goto out;
	}

	size = musb_fifo_size(mps);

	ret = musb_fifo_alloc(dev, ep_cfg->addr, size);
	if (ret != 0) {
		goto out;
	}

	musb_writeb(mbase, MUSB_INDEX, idx);

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		musb_writew(mbase, MUSB_TXMAXP, mps);
		musb_writeb(mbase, MUSB_TXFIFOSZ, musb_fifo_size_encode(size));
		musb_writew(mbase, MUSB_TXFIFOADD,
			    musb_fifo_offset(dev, ep_cfg->addr) / MUSB_FIFO_ADDR_UNIT);
		musb_writeb(mbase, MUSB_TXCSRH, MUSB_TXCSRH_MODE);
		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_CLRDATATOG);
		musb_ep_tx_flush(dev);
		musb_writew(mbase, MUSB_INTRTXE, musb_readw(mbase, MUSB_INTRTXE) | BIT(idx));
	} else {
		musb_writew(mbase, MUSB_RXMAXP, mps);
		musb_writeb(mbase, MUSB_RXFIFOSZ, musb_fifo_size_encode(size));
		musb_writew(mbase, MUSB_RXFIFOADD,
			    musb_fifo_offset(dev, ep_cfg->addr) / MUSB_FIFO_ADDR_UNIT);
		musb_writeb(mbase, MUSB_RXCSRH, 0U);
		musb_writeb(mbase, MUSB_RXCSRL, MUSB_RXCSR_CLRDATATOG);
		musb_ep_rx_flush(dev);
		musb_writew(mbase, MUSB_INTRRXE, musb_readw(mbase, MUSB_INTRRXE) | BIT(idx));
	}

	LOG_DBG("Enable ep 0x%02x mps %u fifo %u at %u", ep_cfg->addr, mps, size,
		musb_fifo_offset(dev, ep_cfg->addr));

out:
	musb_writeb(mbase, MUSB_INDEX, index);
	k_spin_unlock(&priv->lock, key);

	return ret;
}

static int udc_musb_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);
	musb_writeb(mbase, MUSB_INDEX, idx);

	if (idx == 0U) {
		musb_writew(mbase, MUSB_INTRTXE, musb_readw(mbase, MUSB_INTRTXE) & ~BIT(0));
		musb_ep0_flush(dev);
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		musb_writew(mbase, MUSB_INTRTXE, musb_readw(mbase, MUSB_INTRTXE) & ~BIT(idx));
		musb_ep_tx_flush(dev);
		musb_fifo_free(dev, ep_cfg->addr);
	} else {
		musb_writew(mbase, MUSB_INTRRXE, musb_readw(mbase, MUSB_INTRRXE) & ~BIT(idx));
		priv->rx_held &= ~BIT(idx);
		musb_ep_rx_flush(dev);
		musb_fifo_free(dev, ep_cfg->addr);
	}

	musb_writeb(mbase, MUSB_INDEX, index);
	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_musb_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);
	musb_writeb(mbase, MUSB_INDEX, idx);

	if (idx == 0U) {
		musb_writeb(mbase, MUSB_CSR0L, MUSB_CSR0_P_SENDSTALL | MUSB_CSR0_P_SVDRXPKTRDY);
		priv->ep0_stage = MUSB_EP0_SETUP;
		priv->ep0_ack_pending = false;
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		musb_writeb(mbase, MUSB_TXCSRL, MUSB_TXCSR_P_SENDSTALL);
		ep_cfg->stat.halted = true;
	} else {
		musb_writeb(mbase, MUSB_RXCSRL, MUSB_RXCSR_P_SENDSTALL);
		priv->rx_held &= ~BIT(idx);
		ep_cfg->stat.halted = true;
	}

	musb_writeb(mbase, MUSB_INDEX, index);
	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_musb_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	if (idx == 0U) {
		return 0;
	}

	key = k_spin_lock(&priv->lock);
	index = musb_readb(mbase, MUSB_INDEX);
	musb_writeb(mbase, MUSB_INDEX, idx);

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		uint8_t csr = musb_readb(mbase, MUSB_TXCSRL);

		csr &= ~(MUSB_TXCSR_P_SENDSTALL | MUSB_TXCSR_P_SENTSTALL | MUSB_TXCSR_P_UNDERRUN);
		csr |= MUSB_TXCSR_CLRDATATOG;

		musb_writeb(mbase, MUSB_TXCSRL, csr);
	} else {
		uint8_t csr = musb_readb(mbase, MUSB_RXCSRL);

		csr &= ~(MUSB_RXCSR_P_SENDSTALL | MUSB_RXCSR_P_SENTSTALL | MUSB_RXCSR_P_OVERRUN);
		csr |= MUSB_RXCSR_CLRDATATOG;

		musb_writeb(mbase, MUSB_RXCSRL, csr);
	}

	musb_writeb(mbase, MUSB_INDEX, index);
	k_spin_unlock(&priv->lock, key);

	ep_cfg->stat.halted = false;

	if (!udc_ep_is_busy(ep_cfg) && (udc_buf_peek(ep_cfg) != NULL)) {
		atomic_set_bit(&priv->xfer_new, musb_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(MUSB_EVT_XFER_NEW));
	}

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_musb_set_address(const struct device *dev, const uint8_t addr)
{
	musb_writeb(musb_mbase(dev), MUSB_FADDR, addr);

	LOG_DBG("Set new address %u for %s", addr, dev->name);

	return 0;
}

static int udc_musb_host_wakeup(const struct device *dev)
{
	const mem_addr_t mbase = musb_mbase(dev);

	LOG_DBG("Remote wakeup from %s", dev->name);

	musb_writeb(mbase, MUSB_POWER, musb_readb(mbase, MUSB_POWER) | MUSB_POWER_RESUME);
	k_msleep(MUSB_RESUME_HOLD_MS);
	musb_writeb(mbase, MUSB_POWER, musb_readb(mbase, MUSB_POWER) & ~MUSB_POWER_RESUME);

	return 0;
}

static enum udc_bus_speed udc_musb_device_speed(const struct device *dev)
{
	if (musb_readb(musb_mbase(dev), MUSB_POWER) & MUSB_POWER_HSMODE) {
		return UDC_BUS_SPEED_HS;
	}

	return UDC_BUS_SPEED_FS;
}

/* USB 2.0 table 7-4 test packet; must be in the EP0 FIFO before entering the mode. */
static const uint8_t musb_test_packet[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF, 0xEF, 0xF7,
	0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E,
};

static int udc_musb_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	k_spinlock_key_t key;
	uint8_t testmode;
	uint8_t index;

	switch (mode) {
	case USB_SFS_TEST_MODE_J:
		testmode = MUSB_TEST_J;
		break;
	case USB_SFS_TEST_MODE_K:
		testmode = MUSB_TEST_K;
		break;
	case USB_SFS_TEST_MODE_SE0_NAK:
		testmode = MUSB_TEST_SE0_NAK;
		break;
	case USB_SFS_TEST_MODE_PACKET:
		testmode = MUSB_TEST_PACKET;
		break;
	default:
		return -EINVAL;
	}

	if ((musb_readb(mbase, MUSB_POWER) & MUSB_POWER_HSMODE) == 0U) {
		LOG_ERR("Test mode %u needs the bus at high speed", mode);
		return -EPERM;
	}

	if (musb_readb(mbase, MUSB_TESTMODE) != 0U) {
		return -EALREADY;
	}

	if (dryrun) {
		LOG_DBG("Test mode %u supported", mode);
		return 0;
	}

	key = k_spin_lock(&priv->lock);

	if (mode == USB_SFS_TEST_MODE_PACKET) {
		index = musb_readb(mbase, MUSB_INDEX);
		musb_writeb(mbase, MUSB_INDEX, 0U);
		musb_fifo_write(dev, 0U, musb_test_packet, sizeof(musb_test_packet));
		musb_writeb(mbase, MUSB_CSR0L, MUSB_CSR0_TXPKTRDY);
		musb_writeb(mbase, MUSB_INDEX, index);
	}

	musb_writeb(mbase, MUSB_TESTMODE, testmode);

	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Enter test mode %u on %s", mode, dev->name);

	return 0;
}

static int udc_musb_init(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	struct udc_musb_data *const priv = udc_get_private(dev);
	const mem_addr_t mbase = musb_mbase(dev);
	int ret;

	priv->ep0_stage = MUSB_EP0_SETUP;
	priv->ep0_status_done = false;
	priv->ep0_ack_pending = false;
	priv->rx_held = 0U;

	ret = cfg->init(dev);
	if (ret != 0) {
		return ret;
	}

	musb_writeb(mbase, MUSB_INDEX, 0U);
	musb_writeb(mbase, MUSB_TXFIFOSZ, musb_fifo_size_encode(MUSB_EP0_FIFO_BYTES));
	musb_writeb(mbase, MUSB_RXFIFOSZ, musb_fifo_size_encode(MUSB_EP0_FIFO_BYTES));
	musb_writew(mbase, MUSB_TXFIFOADD, 0U);
	musb_writew(mbase, MUSB_RXFIFOADD, 0U);

	LOG_DBG("EPINFO 0x%02x RAMINFO 0x%02x", musb_readb(mbase, MUSB_EPINFO),
		musb_readb(mbase, MUSB_RAMINFO));

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, USB_CONTROL_EP_MPS,
				   0) != 0) {
		LOG_ERR("Failed to enable the control OUT endpoint");
		goto shutdown;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, USB_CONTROL_EP_MPS,
				   0) != 0) {
		LOG_ERR("Failed to enable the control IN endpoint");
		(void)udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
		goto shutdown;
	}

	LOG_DBG("Init device %s", dev->name);

	return 0;

shutdown:
	(void)cfg->shutdown(dev);

	return -EIO;
}

static void musb_restore_ep_irq(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	const mem_addr_t mbase = musb_mbase(dev);
	uint16_t intrtxe = BIT(0);
	uint16_t intrrxe = 0U;

	for (uint8_t idx = 1U; idx < cfg->num_of_eps; idx++) {
		if (cfg->ep_cfg_in[idx].stat.enabled) {
			intrtxe |= BIT(idx);
		}

		if (cfg->ep_cfg_out[idx].stat.enabled) {
			intrrxe |= BIT(idx);
		}
	}

	musb_writew(mbase, MUSB_INTRTXE, intrtxe);
	musb_writew(mbase, MUSB_INTRRXE, intrrxe);
}

static int udc_musb_enable(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	const mem_addr_t mbase = musb_mbase(dev);
	uint8_t power = musb_readb(mbase, MUSB_POWER) & ~MUSB_POWER_HSENAB;

	if (cfg->high_speed) {
		power |= MUSB_POWER_HSENAB;
	}

	musb_writeb(mbase, MUSB_POWER, power);

	musb_writeb(mbase, MUSB_INTRUSBE,
		    MUSB_INTR_RESET | MUSB_INTR_SUSPEND | MUSB_INTR_RESUME | MUSB_INTR_DISCONNECT);

	musb_restore_ep_irq(dev);

	cfg->irq_enable_func(dev);

	musb_writeb(mbase, MUSB_POWER, power | MUSB_POWER_SOFTCONN);

	LOG_DBG("Enable device %s (%s)", dev->name, cfg->high_speed ? "high speed" : "full speed");

	return 0;
}

static int udc_musb_disable(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	const mem_addr_t mbase = musb_mbase(dev);

	musb_writeb(mbase, MUSB_POWER, musb_readb(mbase, MUSB_POWER) & ~MUSB_POWER_SOFTCONN);

	musb_writeb(mbase, MUSB_INTRUSBE, 0U);
	musb_writew(mbase, MUSB_INTRTXE, 0U);
	musb_writew(mbase, MUSB_INTRRXE, 0U);

	cfg->irq_disable_func(dev);

	LOG_DBG("Disable device %s", dev->name);

	return 0;
}

static int udc_musb_shutdown(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	const mem_addr_t mbase = musb_mbase(dev);
	int ret;

	cfg->irq_disable_func(dev);

	musb_writeb(mbase, MUSB_POWER, musb_readb(mbase, MUSB_POWER) & ~MUSB_POWER_SOFTCONN);

	ret = cfg->shutdown(dev);

	LOG_DBG("Shutdown device %s", dev->name);

	return ret;
}

static int udc_musb_register_eps(const struct device *dev, struct udc_ep_config *ep_cfg,
				 size_t num_eps, uint8_t dir, uint16_t mps)
{
	int err;

	for (size_t i = 0; i < num_eps; i++) {
		if (dir == USB_EP_DIR_IN) {
			ep_cfg[i].caps.in = 1;
		} else {
			ep_cfg[i].caps.out = 1;
		}

		if (i == 0) {
			ep_cfg[i].caps.control = 1;
			ep_cfg[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			ep_cfg[i].caps.bulk = 1;
			ep_cfg[i].caps.interrupt = 1;
			ep_cfg[i].caps.iso = 0;
			ep_cfg[i].caps.mps = mps;
		}

		ep_cfg[i].addr = dir | i;

		err = udc_register_ep(dev, &ep_cfg[i]);
		if (err) {
			LOG_ERR("Failed to register endpoint 0x%02x", ep_cfg[i].addr);
			return err;
		}
	}

	return 0;
}

int udc_musb_preinit(const struct device *dev)
{
	const struct udc_musb_config *cfg = dev->config;
	struct udc_musb_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	for (size_t i = 0; i < UDC_MUSB_FIFO_SLOTS(cfg->num_of_eps); i++) {
		cfg->fifo[i].offset = 0U;
		cfg->fifo[i].size = 0U;
	}

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.can_detect_vbus = false;
	data->caps.hs = cfg->high_speed;

	err = udc_musb_register_eps(dev, cfg->ep_cfg_out, cfg->num_of_eps, USB_EP_DIR_OUT,
				    MUSB_EP_MPS_MAX);
	if (err) {
		return err;
	}

	err = udc_musb_register_eps(dev, cfg->ep_cfg_in, cfg->num_of_eps, USB_EP_DIR_IN,
				    MUSB_EP_MPS_MAX);
	if (err) {
		return err;
	}

	cfg->make_thread(dev);

	return 0;
}

static void udc_musb_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_musb_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

const struct udc_api udc_musb_api = {
	.lock = udc_musb_lock,
	.unlock = udc_musb_unlock,
	.device_speed = udc_musb_device_speed,
	.init = udc_musb_init,
	.enable = udc_musb_enable,
	.disable = udc_musb_disable,
	.shutdown = udc_musb_shutdown,
	.set_address = udc_musb_set_address,
	.test_mode = udc_musb_test_mode,
	.host_wakeup = udc_musb_host_wakeup,
	.ep_enable = udc_musb_ep_enable,
	.ep_disable = udc_musb_ep_disable,
	.ep_set_halt = udc_musb_ep_set_halt,
	.ep_clear_halt = udc_musb_ep_clear_halt,
	.ep_enqueue = udc_musb_ep_enqueue,
	.ep_dequeue = udc_musb_ep_dequeue,
};
