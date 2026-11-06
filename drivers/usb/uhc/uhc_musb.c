/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Host controller driver for the Mentor Graphics Inventra MUSB core in
 * multipoint mode with dynamic FIFO sizing, in PIO mode. Control transfers use
 * endpoint 0. Each bulk or interrupt transfer claims a free host endpoint,
 * programs it and releases it on completion, so several transfers run in
 * parallel.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/usb/usb_ch9.h>

#include "uhc_common.h"
#include "uhc_musb.h"

LOG_MODULE_REGISTER(uhc_musb, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* Common registers */
#define MUSB_REG_FADDR     0x00U
#define MUSB_REG_POWER     0x01U
#define MUSB_REG_INTRTX    0x02U
#define MUSB_REG_INTRRX    0x04U
#define MUSB_REG_INTRTXE   0x06U
#define MUSB_REG_INTRRXE   0x08U
#define MUSB_REG_INTRUSB   0x0AU
#define MUSB_REG_INTRUSBE  0x0BU
#define MUSB_REG_FRAME     0x0CU
#define MUSB_REG_INDEX     0x0EU
#define MUSB_REG_TESTMODE  0x0FU
#define MUSB_REG_FIFO(ep)  (0x20U + 4U * (ep))
#define MUSB_REG_DEVCTL    0x60U
#define MUSB_REG_TXFIFOSZ  0x62U
#define MUSB_REG_RXFIFOSZ  0x63U
#define MUSB_REG_TXFIFOADD 0x64U
#define MUSB_REG_RXFIFOADD 0x66U

/* Multipoint bus control registers */
#define MUSB_REG_TXFUNCADDR(ep) (0x80U + 8U * (ep))
#define MUSB_REG_TXHUBADDR(ep)  (0x82U + 8U * (ep))
#define MUSB_REG_TXHUBPORT(ep)  (0x83U + 8U * (ep))
#define MUSB_REG_RXFUNCADDR(ep) (0x84U + 8U * (ep))
#define MUSB_REG_RXHUBADDR(ep)  (0x86U + 8U * (ep))
#define MUSB_REG_RXHUBPORT(ep)  (0x87U + 8U * (ep))

/* Indexed endpoint registers, selected by MUSB_REG_INDEX */
#define MUSB_REG_TXMAXP     0x10U
#define MUSB_REG_CSR0       0x12U
#define MUSB_REG_TXCSR      0x12U
#define MUSB_REG_RXMAXP     0x14U
#define MUSB_REG_RXCSR      0x16U
#define MUSB_REG_COUNT0     0x18U
#define MUSB_REG_RXCOUNT    0x18U
#define MUSB_REG_TYPE0      0x1AU
#define MUSB_REG_TXTYPE     0x1AU
#define MUSB_REG_NAKLIMIT0  0x1BU
#define MUSB_REG_TXINTERVAL 0x1BU
#define MUSB_REG_RXTYPE     0x1CU
#define MUSB_REG_RXINTERVAL 0x1DU

#define MUSB_POWER_ISOUPDATE BIT(7)
#define MUSB_POWER_SOFTCONN  BIT(6)
#define MUSB_POWER_HSENAB    BIT(5)
#define MUSB_POWER_HSMODE    BIT(4)
#define MUSB_POWER_RESET     BIT(3)
#define MUSB_POWER_RESUME    BIT(2)
#define MUSB_POWER_SUSPENDM  BIT(1)
#define MUSB_POWER_ENSUSPEND BIT(0)

#define MUSB_INTRUSB_SUSPEND    BIT(0)
#define MUSB_INTRUSB_RESUME     BIT(1)
#define MUSB_INTRUSB_BABBLE     BIT(2)
#define MUSB_INTRUSB_SOF        BIT(3)
#define MUSB_INTRUSB_CONNECT    BIT(4)
#define MUSB_INTRUSB_DISCONNECT BIT(5)
#define MUSB_INTRUSB_SESSREQ    BIT(6)
#define MUSB_INTRUSB_VBUSERROR  BIT(7)

#define MUSB_DEVCTL_BDEVICE  BIT(7)
#define MUSB_DEVCTL_FSDEV    BIT(6)
#define MUSB_DEVCTL_LSDEV    BIT(5)
#define MUSB_DEVCTL_VBUS     GENMASK(4, 3)
#define MUSB_DEVCTL_HOSTMODE BIT(2)
#define MUSB_DEVCTL_HOSTREQ  BIT(1)
#define MUSB_DEVCTL_SESSION  BIT(0)

#define MUSB_CSR0_H_DIS_PING      BIT(11)
#define MUSB_CSR0_H_WR_DATATOGGLE BIT(10)
#define MUSB_CSR0_H_DATATOGGLE    BIT(9)
#define MUSB_CSR0_FLUSHFIFO       BIT(8)
#define MUSB_CSR0_H_NAKTIMEOUT    BIT(7)
#define MUSB_CSR0_H_STATUSPKT     BIT(6)
#define MUSB_CSR0_H_REQPKT        BIT(5)
#define MUSB_CSR0_H_ERROR         BIT(4)
#define MUSB_CSR0_H_SETUPPKT      BIT(3)
#define MUSB_CSR0_H_RXSTALL       BIT(2)
#define MUSB_CSR0_TXPKTRDY        BIT(1)
#define MUSB_CSR0_RXPKTRDY        BIT(0)

#define MUSB_TXCSR_MODE            BIT(13)
#define MUSB_TXCSR_H_WR_DATATOGGLE BIT(9)
#define MUSB_TXCSR_H_DATATOGGLE    BIT(8)
#define MUSB_TXCSR_H_NAKTIMEOUT    BIT(7)
#define MUSB_TXCSR_CLRDATATOG      BIT(6)
#define MUSB_TXCSR_H_RXSTALL       BIT(5)
#define MUSB_TXCSR_FLUSHFIFO       BIT(3)
#define MUSB_TXCSR_H_ERROR         BIT(2)
#define MUSB_TXCSR_FIFONOTEMPTY    BIT(1)
#define MUSB_TXCSR_TXPKTRDY        BIT(0)
/* Bits that are cleared by writing zero */
#define MUSB_TXCSR_H_WZC_BITS                                                                      \
	(MUSB_TXCSR_H_NAKTIMEOUT | MUSB_TXCSR_H_RXSTALL | MUSB_TXCSR_H_ERROR |                     \
	 MUSB_TXCSR_FIFONOTEMPTY)

#define MUSB_RXCSR_H_WR_DATATOGGLE BIT(10)
#define MUSB_RXCSR_H_DATATOGGLE    BIT(9)
#define MUSB_RXCSR_CLRDATATOG      BIT(7)
#define MUSB_RXCSR_H_RXSTALL       BIT(6)
#define MUSB_RXCSR_H_REQPKT        BIT(5)
#define MUSB_RXCSR_FLUSHFIFO       BIT(4)
#define MUSB_RXCSR_DATAERROR       BIT(3)
#define MUSB_RXCSR_H_ERROR         BIT(2)
#define MUSB_RXCSR_FIFOFULL        BIT(1)
#define MUSB_RXCSR_RXPKTRDY        BIT(0)
/* Bits that are cleared by writing zero */
#define MUSB_RXCSR_H_WZC_BITS                                                                      \
	(MUSB_RXCSR_H_RXSTALL | MUSB_RXCSR_H_ERROR | MUSB_RXCSR_DATAERROR | MUSB_RXCSR_RXPKTRDY)

#define MUSB_TYPE_SPEED_HS   1U
#define MUSB_TYPE_SPEED_FS   2U
#define MUSB_TYPE_SPEED_LS   3U
#define MUSB_TYPE_SPEED_POS  6U
#define MUSB_TYPE_PROTO_BULK 2U
#define MUSB_TYPE_PROTO_INT  3U
#define MUSB_TYPE_PROTO_POS  4U

/* FIFO size code 6 selects 2^(6 + 3) = 512 bytes */
#define MUSB_FIFOSZ_512     6U
#define MUSB_FIFO_BYTES     512U
#define MUSB_FIFO_ADDR_UNIT 8U
#define MUSB_EP0_FIFO_BYTES 64U

/*
 * NAK limit of bulk transfers, 2^(m - 1) frames or microframes. When it
 * expires, the transfer is put back at the end of the queue so that other
 * transfers are not blocked by an endpoint that keeps NAKing.
 */
#define MUSB_BULK_NAK_LIMIT 4U

#define MUSB_CONNECT_DEBOUNCE_MS 100
#define MUSB_RESET_MS            50
#define MUSB_RESET_RECOVERY_MS   20
#define MUSB_RESUME_MS           20
#define MUSB_VBUS_RETRIES        3
#define MUSB_VBUS_OFF_MS         100
#define MUSB_FLUSH_RETRIES       4

#define DEV_CFG(dev)  ((const struct uhc_musb_config *)(dev)->config)
#define DEV_PRIV(dev) ((struct uhc_musb_data *)uhc_get_private(dev))

/*
 * Each host endpoint, except endpoint 0, gets a TX and an RX FIFO of
 * MUSB_FIFO_BYTES and is claimed by one transfer at a time.
 */
static uint8_t musb_num_eps(const struct uhc_musb_config *cfg)
{
	uint8_t eps = 1U + (cfg->fifo_size - MUSB_EP0_FIFO_BYTES) / (2U * MUSB_FIFO_BYTES);

	return MIN(MIN(cfg->num_eps, eps), UHC_MUSB_MAX_EPS);
}

static inline mem_addr_t musb_reg(const struct device *dev, uint32_t offset)
{
	return DEV_CFG(dev)->mbase + offset;
}

static inline uint8_t musb_read8(const struct device *dev, uint32_t offset)
{
	return sys_read8(musb_reg(dev, offset));
}

static inline void musb_write8(const struct device *dev, uint32_t offset, uint8_t val)
{
	sys_write8(val, musb_reg(dev, offset));
}

static inline uint16_t musb_read16(const struct device *dev, uint32_t offset)
{
	return sys_read16(musb_reg(dev, offset));
}

static inline void musb_write16(const struct device *dev, uint32_t offset, uint16_t val)
{
	sys_write16(val, musb_reg(dev, offset));
}

static inline void musb_index(const struct device *dev, uint8_t ep)
{
	musb_write8(dev, MUSB_REG_INDEX, ep);
}

static void musb_fifo_write(const struct device *dev, uint8_t ep, const uint8_t *data, size_t len)
{
	mem_addr_t fifo = musb_reg(dev, MUSB_REG_FIFO(ep));

	while (len >= sizeof(uint32_t)) {
		uint32_t word;

		memcpy(&word, data, sizeof(word));
		sys_write32(word, fifo);
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len != 0U) {
		sys_write8(*data++, fifo);
		len--;
	}
}

static void musb_fifo_read(const struct device *dev, uint8_t ep, uint8_t *data, size_t len)
{
	mem_addr_t fifo = musb_reg(dev, MUSB_REG_FIFO(ep));

	while (len >= sizeof(uint32_t)) {
		uint32_t word = sys_read32(fifo);

		memcpy(data, &word, sizeof(word));
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	/* All reads of one packet must have the same width, the tail included */
	if (len != 0U) {
		uint32_t word = sys_read32(fifo);

		memcpy(data, &word, len);
	}
}

static uint8_t musb_speed_type(const struct usb_device *udev)
{
	switch (udev->speed) {
	case USB_SPEED_SPEED_HS:
		return MUSB_TYPE_SPEED_HS << MUSB_TYPE_SPEED_POS;
	case USB_SPEED_SPEED_LS:
		return MUSB_TYPE_SPEED_LS << MUSB_TYPE_SPEED_POS;
	case USB_SPEED_SPEED_FS:
	default:
		return MUSB_TYPE_SPEED_FS << MUSB_TYPE_SPEED_POS;
	}
}

/* Program the function and the transaction translator addresses of an endpoint */
static void musb_set_target(const struct device *dev, uint8_t ep, const struct usb_device *udev)
{
	uint8_t hub_addr = 0U;
	uint8_t hub_port = 0U;

	if (udev->hub != NULL && udev->hub->speed == USB_SPEED_SPEED_HS &&
	    udev->speed != USB_SPEED_SPEED_HS) {
		hub_addr = udev->hub->addr;
		hub_port = udev->hub_port;
	}

	musb_write8(dev, MUSB_REG_TXFUNCADDR(ep), udev->addr);
	musb_write8(dev, MUSB_REG_TXHUBADDR(ep), hub_addr);
	musb_write8(dev, MUSB_REG_TXHUBPORT(ep), hub_port);
	musb_write8(dev, MUSB_REG_RXFUNCADDR(ep), udev->addr);
	musb_write8(dev, MUSB_REG_RXHUBADDR(ep), hub_addr);
	musb_write8(dev, MUSB_REG_RXHUBPORT(ep), hub_port);
}

static struct uhc_musb_toggle *musb_toggle_get(struct uhc_musb_data *priv,
					       const struct uhc_transfer *xfer)
{
	struct uhc_musb_toggle *empty = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(priv->toggles); i++) {
		struct uhc_musb_toggle *toggle = &priv->toggles[i];

		if (toggle->udev == xfer->udev && toggle->ep == xfer->ep) {
			return toggle;
		}

		if (toggle->udev == NULL && empty == NULL) {
			empty = toggle;
		}
	}

	if (empty != NULL) {
		empty->udev = xfer->udev;
		empty->ep = xfer->ep;
		empty->next = 0U;
	}

	return empty;
}

/* Forget the toggles of one endpoint, or of all endpoints if ep is 0 */
static void musb_toggle_reset(struct uhc_musb_data *priv, const struct usb_device *udev, uint8_t ep)
{
	for (size_t i = 0; i < ARRAY_SIZE(priv->toggles); i++) {
		struct uhc_musb_toggle *toggle = &priv->toggles[i];

		if (toggle->udev == udev && (ep == 0U || toggle->ep == ep)) {
			*toggle = (struct uhc_musb_toggle){0};
		}
	}
}

/* Forget the toggles of all endpoints of an interface, in any alternate setting */
static void musb_toggle_reset_iface(struct uhc_musb_data *priv, const struct usb_device *udev,
				    uint8_t iface)
{
	const struct usb_cfg_descriptor *cfg_desc = udev->cfg_desc;
	const uint8_t *p = udev->cfg_desc;
	const uint8_t *end;
	bool match = false;

	if (cfg_desc == NULL) {
		musb_toggle_reset(priv, udev, 0U);
		return;
	}

	end = p + sys_le16_to_cpu(cfg_desc->wTotalLength);
	while (p + sizeof(struct usb_desc_header) <= end) {
		const struct usb_desc_header *header = (const void *)p;

		if (header->bLength < sizeof(struct usb_desc_header) || p + header->bLength > end) {
			break;
		}

		if (header->bDescriptorType == USB_DESC_INTERFACE &&
		    header->bLength >= sizeof(struct usb_if_descriptor)) {
			const struct usb_if_descriptor *if_desc = (const void *)p;

			match = if_desc->bInterfaceNumber == iface;
		} else if (match && header->bDescriptorType == USB_DESC_ENDPOINT &&
			   header->bLength >= sizeof(struct usb_ep_descriptor)) {
			const struct usb_ep_descriptor *ep_desc = (const void *)p;

			musb_toggle_reset(priv, udev, ep_desc->bEndpointAddress);
		}

		p += header->bLength;
	}
}

static void musb_ctrl_update_toggles(struct uhc_musb_data *priv, const struct uhc_transfer *xfer)
{
	const struct usb_setup_packet *setup = (const void *)xfer->setup_pkt;

	if (setup->RequestType.type != USB_REQTYPE_TYPE_STANDARD) {
		return;
	}

	if (setup->bRequest == USB_SREQ_SET_CONFIGURATION) {
		musb_toggle_reset(priv, xfer->udev, 0U);
	}

	if (setup->bRequest == USB_SREQ_SET_INTERFACE &&
	    setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_INTERFACE) {
		musb_toggle_reset_iface(priv, xfer->udev, sys_le16_to_cpu(setup->wIndex) & 0xFFU);
	}

	if (setup->bRequest == USB_SREQ_CLEAR_FEATURE &&
	    setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_ENDPOINT &&
	    sys_le16_to_cpu(setup->wValue) == USB_SFS_ENDPOINT_HALT) {
		uint8_t ep = sys_le16_to_cpu(setup->wIndex) & 0xFFU;

		if (USB_EP_GET_IDX(ep) != 0U) {
			musb_toggle_reset(priv, xfer->udev, ep);
		}
	}
}

static void musb_ep0_flush(const struct device *dev)
{
	musb_index(dev, 0U);

	for (int i = 0; i < MUSB_FLUSH_RETRIES; i++) {
		uint16_t csr = musb_read16(dev, MUSB_REG_CSR0);

		if ((csr & (MUSB_CSR0_TXPKTRDY | MUSB_CSR0_RXPKTRDY)) == 0U) {
			break;
		}

		musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_FLUSHFIFO);
	}

	/* Also clears REQPKT, STATUSPKT and the error bits */
	musb_write16(dev, MUSB_REG_CSR0, 0U);
}

static void musb_tx_flush(const struct device *dev, uint8_t ep)
{
	uint16_t csr;

	musb_index(dev, ep);

	/* Clear NAKTIMEOUT while keeping the other write-zero-to-clear bits */
	csr = musb_read16(dev, MUSB_REG_TXCSR) | MUSB_TXCSR_H_WZC_BITS;
	musb_write16(dev, MUSB_REG_TXCSR, csr & ~MUSB_TXCSR_H_NAKTIMEOUT);

	for (int i = 0; i < MUSB_FLUSH_RETRIES; i++) {
		csr = musb_read16(dev, MUSB_REG_TXCSR);

		if ((csr & MUSB_TXCSR_FIFONOTEMPTY) == 0U) {
			break;
		}

		musb_write16(dev, MUSB_REG_TXCSR,
			     MUSB_TXCSR_MODE | MUSB_TXCSR_FLUSHFIFO | MUSB_TXCSR_TXPKTRDY);
	}

	musb_write16(dev, MUSB_REG_TXCSR, MUSB_TXCSR_MODE);
}

static void musb_rx_flush(const struct device *dev, uint8_t ep)
{
	uint16_t csr;

	musb_index(dev, ep);

	/*
	 * Stop the transaction by clearing REQPKT first, then DATAERROR (NAK
	 * timeout), keeping the other write-zero-to-clear bits.
	 */
	csr = musb_read16(dev, MUSB_REG_RXCSR) | MUSB_RXCSR_H_WZC_BITS;
	csr &= ~MUSB_RXCSR_H_REQPKT;
	musb_write16(dev, MUSB_REG_RXCSR, csr);
	csr &= ~MUSB_RXCSR_DATAERROR;
	musb_write16(dev, MUSB_REG_RXCSR, csr);

	if ((musb_read16(dev, MUSB_REG_RXCSR) & MUSB_RXCSR_RXPKTRDY) != 0U) {
		/* Write twice in case the FIFO is double buffered */
		musb_write16(dev, MUSB_REG_RXCSR, MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_RXPKTRDY);
		musb_write16(dev, MUSB_REG_RXCSR, MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_RXPKTRDY);
	}

	musb_write16(dev, MUSB_REG_RXCSR, 0U);
}

static void musb_xfer_abort(const struct device *dev, uint8_t ep, const struct uhc_transfer *xfer)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_musb_toggle *toggle;
	uint16_t csr;

	if (xfer->type == USB_EP_TYPE_CONTROL) {
		musb_ep0_flush(dev);
		atomic_and(&priv->intrtx, ~BIT(0));
		return;
	}

	/* The hardware may have completed a packet that was not processed yet */
	musb_index(dev, ep);
	toggle = musb_toggle_get(priv, xfer);

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		csr = musb_read16(dev, MUSB_REG_RXCSR);
		if (toggle != NULL) {
			toggle->next = (csr & MUSB_RXCSR_H_DATATOGGLE) != 0U ? 1U : 0U;
		}

		musb_rx_flush(dev, ep);
		atomic_and(&priv->intrrx, ~BIT(ep));
	} else {
		csr = musb_read16(dev, MUSB_REG_TXCSR);
		if (toggle != NULL) {
			toggle->next = (csr & MUSB_TXCSR_H_DATATOGGLE) != 0U ? 1U : 0U;
		}

		musb_tx_flush(dev, ep);
		atomic_and(&priv->intrtx, ~BIT(ep));
	}
}

/* Give the endpoint back and return the transfer */
static void musb_xfer_finish(const struct device *dev, uint8_t ep, int err)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[ep];

	if (xfer == NULL) {
		return;
	}

	if (err != 0) {
		musb_xfer_abort(dev, ep, xfer);
	} else if (xfer->type == USB_EP_TYPE_CONTROL) {
		musb_ctrl_update_toggles(priv, xfer);
	}

	priv->xfers[ep] = NULL;
	uhc_xfer_return(dev, xfer, err);
}

/* Give the endpoint back and move the transfer to the end of the queue */
static void musb_xfer_requeue(const struct device *dev, uint8_t ep)
{
	struct uhc_data *data = dev->data;
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[ep];

	sys_dnode_t *node = sys_dlist_peek_head(&data->ctrl_xfers);
	size_t count = 0;

	musb_xfer_abort(dev, ep, xfer);
	priv->xfers[ep] = NULL;

	for (sys_dnode_t *n = node; n != NULL; n = sys_dlist_peek_next(&data->ctrl_xfers, n)) {
		count++;
	}

	/* Move all transfers of this endpoint to the end, keeping their order */
	for (size_t i = 0; i < count && node != NULL; i++) {
		sys_dnode_t *next = sys_dlist_peek_next(&data->ctrl_xfers, node);
		struct uhc_transfer *tmp = SYS_DLIST_CONTAINER(node, tmp, node);

		if (tmp->udev == xfer->udev && tmp->ep == xfer->ep) {
			sys_dlist_remove(node);
			sys_dlist_append(&data->ctrl_xfers, node);
		}

		node = next;
	}
}

/* Return the transfers owning an endpoint and all queued transfers */
static void musb_xfers_drain(const struct device *dev, int err)
{
	struct uhc_transfer *xfer;

	for (uint8_t ep = 0U; ep < UHC_MUSB_MAX_EPS; ep++) {
		musb_xfer_finish(dev, ep, err);
	}

	while ((xfer = uhc_xfer_get_next(dev)) != NULL) {
		uhc_xfer_return(dev, xfer, err);
	}
}

static int musb_fifo_configure(const struct device *dev)
{
	const struct uhc_musb_config *cfg = DEV_CFG(dev);
	uint16_t addr = MUSB_EP0_FIFO_BYTES;

	for (uint8_t ep = 1U; ep < musb_num_eps(cfg); ep++) {
		musb_index(dev, ep);
		musb_write8(dev, MUSB_REG_TXFIFOSZ, MUSB_FIFOSZ_512);
		musb_write16(dev, MUSB_REG_TXFIFOADD, addr / MUSB_FIFO_ADDR_UNIT);
		addr += MUSB_FIFO_BYTES;
		musb_write8(dev, MUSB_REG_RXFIFOSZ, MUSB_FIFOSZ_512);
		musb_write16(dev, MUSB_REG_RXFIFOADD, addr / MUSB_FIFO_ADDR_UNIT);
		addr += MUSB_FIFO_BYTES;
	}

	return 0;
}

static uint16_t musb_ctrl_wlength(const struct uhc_transfer *xfer)
{
	const struct usb_setup_packet *setup = (const void *)xfer->setup_pkt;

	return sys_le16_to_cpu(setup->wLength);
}

static bool musb_ctrl_data_in(const struct uhc_transfer *xfer)
{
	return USB_REQTYPE_GET_DIR(xfer->setup_pkt[0]) == USB_REQTYPE_DIR_TO_HOST;
}

static void musb_ctrl_send_out(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[0];
	uint16_t len = MIN(xfer->mps, xfer->buf->len);

	musb_index(dev, 0U);
	musb_fifo_write(dev, 0U, xfer->buf->data, len);
	priv->packet_len[0] = len;
	musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_TXPKTRDY);
}

static void musb_ctrl_status_start(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[0];

	if (xfer->no_status) {
		musb_xfer_finish(dev, 0U, 0);
		return;
	}

	xfer->stage = UHC_CONTROL_STAGE_STATUS;
	musb_index(dev, 0U);

	/* The status stage goes in the opposite direction of the data stage */
	if (musb_ctrl_data_in(xfer) && musb_ctrl_wlength(xfer) != 0U) {
		musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY);
	} else {
		musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_H_REQPKT);
	}
}

static void musb_ctrl_data_start(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[0];

	if (priv->ctrl_remaining == 0U) {
		musb_ctrl_status_start(dev);
		return;
	}

	xfer->stage = UHC_CONTROL_STAGE_DATA;
	if (musb_ctrl_data_in(xfer)) {
		musb_index(dev, 0U);
		musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_H_REQPKT);
	} else {
		musb_ctrl_send_out(dev);
	}
}

static int musb_ctrl_start(const struct device *dev, struct uhc_transfer *xfer)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	uint16_t wlength = musb_ctrl_wlength(xfer);

	if (wlength != 0U) {
		if (xfer->buf == NULL) {
			return -EINVAL;
		}

		if (musb_ctrl_data_in(xfer) ? net_buf_tailroom(xfer->buf) < wlength
					    : xfer->buf->len != wlength) {
			return -EINVAL;
		}
	}

	priv->ctrl_remaining = wlength;
	xfer->stage = UHC_CONTROL_STAGE_SETUP;

	musb_index(dev, 0U);
	musb_set_target(dev, 0U, xfer->udev);
	musb_write8(dev, MUSB_REG_TYPE0, musb_speed_type(xfer->udev));
	musb_write8(dev, MUSB_REG_NAKLIMIT0, 0U);
	musb_fifo_write(dev, 0U, xfer->setup_pkt, sizeof(struct usb_setup_packet));
	musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

	return 0;
}

static void musb_ctrl_handle(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[0];
	struct net_buf *buf = xfer->buf;
	uint16_t csr;

	musb_index(dev, 0U);
	csr = musb_read16(dev, MUSB_REG_CSR0);

	if ((csr & MUSB_CSR0_H_RXSTALL) != 0U) {
		LOG_DBG("Control transfer stalled");
		musb_xfer_finish(dev, 0U, -EPIPE);
		return;
	}

	if ((csr & (MUSB_CSR0_H_ERROR | MUSB_CSR0_H_NAKTIMEOUT)) != 0U) {
		LOG_WRN("Control transfer failed, CSR0 0x%04x", csr);
		musb_xfer_finish(dev, 0U, -EIO);
		return;
	}

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		if ((csr & MUSB_CSR0_TXPKTRDY) == 0U) {
			musb_ctrl_data_start(dev);
		}
		break;
	case UHC_CONTROL_STAGE_DATA:
		if (musb_ctrl_data_in(xfer)) {
			uint16_t count;

			if ((csr & MUSB_CSR0_RXPKTRDY) == 0U) {
				break;
			}

			count = musb_read8(dev, MUSB_REG_COUNT0);
			if (count > priv->ctrl_remaining || count > net_buf_tailroom(buf)) {
				LOG_ERR("Control IN packet of %u bytes overflows", count);
				musb_xfer_finish(dev, 0U, -EOVERFLOW);
				break;
			}

			musb_fifo_read(dev, 0U, net_buf_add(buf, count), count);
			musb_write16(dev, MUSB_REG_CSR0, 0U);
			priv->ctrl_remaining -= count;

			if (count < xfer->mps || priv->ctrl_remaining == 0U) {
				musb_ctrl_status_start(dev);
			} else {
				musb_write16(dev, MUSB_REG_CSR0, MUSB_CSR0_H_REQPKT);
			}
		} else {
			if ((csr & MUSB_CSR0_TXPKTRDY) != 0U) {
				break;
			}

			net_buf_pull(buf, priv->packet_len[0]);
			priv->ctrl_remaining -= priv->packet_len[0];
			if (priv->ctrl_remaining == 0U) {
				musb_ctrl_status_start(dev);
			} else {
				musb_ctrl_send_out(dev);
			}
		}
		break;
	case UHC_CONTROL_STAGE_STATUS:
		if (musb_ctrl_data_in(xfer) && musb_ctrl_wlength(xfer) != 0U) {
			if ((csr & MUSB_CSR0_TXPKTRDY) != 0U) {
				break;
			}
		} else if ((csr & MUSB_CSR0_RXPKTRDY) == 0U) {
			break;
		}

		musb_write16(dev, MUSB_REG_CSR0, 0U);
		musb_xfer_finish(dev, 0U, 0);
		break;
	default:
		break;
	}
}

static void musb_bulk_send(const struct device *dev, uint8_t ep)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[ep];
	uint16_t len = MIN(xfer->mps, xfer->buf->len);

	musb_index(dev, ep);
	musb_fifo_write(dev, ep, xfer->buf->data, len);
	priv->packet_len[ep] = len;
	musb_write16(dev, MUSB_REG_TXCSR, MUSB_TXCSR_MODE | MUSB_TXCSR_TXPKTRDY);
}

/* Program the endpoint for a bulk or interrupt transfer and start it */
static int musb_ep_start(const struct device *dev, uint8_t ep, struct uhc_transfer *xfer)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_musb_toggle *toggle;
	uint8_t proto;
	uint8_t interval;
	uint8_t type;

	if (xfer->buf == NULL) {
		return -EINVAL;
	}

	toggle = musb_toggle_get(priv, xfer);
	if (toggle == NULL) {
		LOG_ERR("No free data toggle slot");
		return -ENOMEM;
	}

	if (xfer->type == USB_EP_TYPE_INTERRUPT) {
		proto = MUSB_TYPE_PROTO_INT;
		interval = xfer->interval;
	} else {
		proto = MUSB_TYPE_PROTO_BULK;
		interval = MUSB_BULK_NAK_LIMIT;
	}

	type = musb_speed_type(xfer->udev) | (proto << MUSB_TYPE_PROTO_POS) |
	       USB_EP_GET_IDX(xfer->ep);

	musb_index(dev, ep);
	musb_set_target(dev, ep, xfer->udev);

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		musb_write16(dev, MUSB_REG_RXMAXP, xfer->mps);
		musb_write8(dev, MUSB_REG_RXTYPE, type);
		musb_write8(dev, MUSB_REG_RXINTERVAL, interval);
		musb_write16(dev, MUSB_REG_RXCSR,
			     MUSB_RXCSR_H_WR_DATATOGGLE |
				     (toggle->next != 0U ? MUSB_RXCSR_H_DATATOGGLE : 0U));
		musb_write16(dev, MUSB_REG_RXCSR, MUSB_RXCSR_H_REQPKT);
	} else {
		musb_write16(dev, MUSB_REG_TXMAXP, xfer->mps);
		musb_write8(dev, MUSB_REG_TXTYPE, type);
		musb_write8(dev, MUSB_REG_TXINTERVAL, interval);
		musb_write16(dev, MUSB_REG_TXCSR,
			     MUSB_TXCSR_MODE | MUSB_TXCSR_H_WR_DATATOGGLE |
				     (toggle->next != 0U ? MUSB_TXCSR_H_DATATOGGLE : 0U));
		musb_bulk_send(dev, ep);
	}

	return 0;
}

static void musb_ep_in_handle(const struct device *dev, uint8_t ep)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[ep];
	struct net_buf *buf = xfer->buf;
	struct uhc_musb_toggle *toggle;
	uint16_t count;
	uint16_t csr;

	musb_index(dev, ep);
	csr = musb_read16(dev, MUSB_REG_RXCSR);

	if ((csr & MUSB_RXCSR_H_RXSTALL) != 0U) {
		musb_xfer_finish(dev, ep, -EPIPE);
		return;
	}

	if ((csr & MUSB_RXCSR_H_ERROR) != 0U) {
		LOG_WRN("IN transfer failed, RXCSR 0x%04x", csr);
		musb_xfer_finish(dev, ep, -EIO);
		return;
	}

	if ((csr & MUSB_RXCSR_DATAERROR) != 0U) {
		/* NAK limit expired */
		musb_xfer_requeue(dev, ep);
		return;
	}

	if ((csr & MUSB_RXCSR_RXPKTRDY) == 0U) {
		return;
	}

	count = musb_read16(dev, MUSB_REG_RXCOUNT);
	if (count > net_buf_tailroom(buf)) {
		LOG_ERR("IN packet of %u bytes overflows", count);
		musb_xfer_finish(dev, ep, -EOVERFLOW);
		return;
	}

	musb_fifo_read(dev, ep, net_buf_add(buf, count), count);
	toggle = musb_toggle_get(priv, xfer);
	toggle->next = (csr & MUSB_RXCSR_H_DATATOGGLE) != 0U ? 1U : 0U;
	musb_write16(dev, MUSB_REG_RXCSR, 0U);

	if (count < xfer->mps || net_buf_tailroom(buf) == 0U) {
		musb_xfer_finish(dev, ep, 0);
	} else {
		musb_write16(dev, MUSB_REG_RXCSR, MUSB_RXCSR_H_REQPKT);
	}
}

static void musb_ep_out_handle(const struct device *dev, uint8_t ep)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *xfer = priv->xfers[ep];
	struct uhc_musb_toggle *toggle;
	uint16_t csr;

	musb_index(dev, ep);
	csr = musb_read16(dev, MUSB_REG_TXCSR);

	if ((csr & MUSB_TXCSR_H_RXSTALL) != 0U) {
		musb_xfer_finish(dev, ep, -EPIPE);
		return;
	}

	if ((csr & MUSB_TXCSR_H_ERROR) != 0U) {
		LOG_WRN("OUT transfer failed, TXCSR 0x%04x", csr);
		musb_xfer_finish(dev, ep, -EIO);
		return;
	}

	if ((csr & MUSB_TXCSR_H_NAKTIMEOUT) != 0U) {
		musb_xfer_requeue(dev, ep);
		return;
	}

	if ((csr & MUSB_TXCSR_TXPKTRDY) != 0U) {
		return;
	}

	toggle = musb_toggle_get(priv, xfer);
	toggle->next = (csr & MUSB_TXCSR_H_DATATOGGLE) != 0U ? 1U : 0U;
	net_buf_pull(xfer->buf, priv->packet_len[ep]);

	if (xfer->buf->len == 0U) {
		musb_xfer_finish(dev, ep, 0);
	} else {
		musb_bulk_send(dev, ep);
	}
}

static bool musb_xfer_active(const struct uhc_musb_data *priv, const struct uhc_transfer *xfer)
{
	for (uint8_t ep = 0U; ep < UHC_MUSB_MAX_EPS; ep++) {
		if (priv->xfers[ep] == xfer) {
			return true;
		}
	}

	return false;
}

/* Another transfer to the same device endpoint is in flight */
static bool musb_xfer_ep_busy(const struct uhc_musb_data *priv, const struct uhc_transfer *xfer)
{
	for (uint8_t ep = 0U; ep < UHC_MUSB_MAX_EPS; ep++) {
		const struct uhc_transfer *tmp = priv->xfers[ep];

		if (tmp != NULL && tmp->udev == xfer->udev && tmp->ep == xfer->ep) {
			return true;
		}
	}

	return false;
}

/* Claim a free host endpoint, endpoint 0 is reserved for control transfers */
static int musb_ep_claim(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);

	for (uint8_t ep = 1U; ep < musb_num_eps(DEV_CFG(dev)); ep++) {
		if (priv->xfers[ep] == NULL) {
			return ep;
		}
	}

	return -EBUSY;
}

/*
 * Start queued transfers on free endpoints. Transfers to one device endpoint
 * run in queue order, so a transfer waits for the one in front of it.
 */
static void musb_schedule(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	bool no_ep = false;
	sys_dnode_t *node;
	sys_dnode_t *next;

	if (!priv->connected) {
		return;
	}

	for (node = sys_dlist_peek_head(&data->ctrl_xfers); node != NULL; node = next) {
		struct uhc_transfer *xfer = SYS_DLIST_CONTAINER(node, xfer, node);
		int ep = 0;
		int ret;

		next = sys_dlist_peek_next(&data->ctrl_xfers, node);

		if (musb_xfer_active(priv, xfer) || musb_xfer_ep_busy(priv, xfer)) {
			continue;
		}

		if (xfer->type == USB_EP_TYPE_CONTROL) {
			if (priv->xfers[0] != NULL) {
				continue;
			}
		} else {
			if (no_ep) {
				continue;
			}

			ep = musb_ep_claim(dev);
			if (ep < 0) {
				no_ep = true;
				continue;
			}
		}

		priv->xfers[ep] = xfer;
		if (xfer->type == USB_EP_TYPE_CONTROL) {
			ret = musb_ctrl_start(dev, xfer);
		} else {
			ret = musb_ep_start(dev, ep, xfer);
		}

		if (ret != 0) {
			priv->xfers[ep] = NULL;
			uhc_xfer_return(dev, xfer, ret);
		}
	}
}

static void musb_handle_connect(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	enum uhc_event_type type;
	uint8_t devctl;
	uint8_t power;

	k_msleep(MUSB_CONNECT_DEBOUNCE_MS);

	devctl = musb_read8(dev, MUSB_REG_DEVCTL);
	if ((devctl & MUSB_DEVCTL_HOSTMODE) == 0U ||
	    (devctl & (MUSB_DEVCTL_FSDEV | MUSB_DEVCTL_LSDEV)) == 0U) {
		LOG_DBG("Spurious connect, DEVCTL 0x%02x", devctl);
		return;
	}

	/* Reset the port to detect the device speed, POWER.HSMODE is valid after reset */
	power = musb_read8(dev, MUSB_REG_POWER);
	musb_write8(dev, MUSB_REG_POWER, power | MUSB_POWER_RESET);
	k_msleep(MUSB_RESET_MS);
	musb_write8(dev, MUSB_REG_POWER, power & ~MUSB_POWER_RESET);
	k_msleep(MUSB_RESET_RECOVERY_MS);

	if ((musb_read8(dev, MUSB_REG_POWER) & MUSB_POWER_HSMODE) != 0U) {
		LOG_INF("High-speed device connected");
		type = UHC_EVT_DEV_CONNECTED_HS;
	} else if ((devctl & MUSB_DEVCTL_LSDEV) != 0U) {
		LOG_INF("Low-speed device connected");
		type = UHC_EVT_DEV_CONNECTED_LS;
	} else {
		LOG_INF("Full-speed device connected");
		type = UHC_EVT_DEV_CONNECTED_FS;
	}

	priv->connected = true;
	priv->vbus_retries = 0U;
	uhc_submit_event(dev, type, 0);
}

static void musb_handle_disconnect(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);

	LOG_INF("Device disconnected");
	priv->connected = false;
	musb_xfers_drain(dev, -ENODEV);
	memset(priv->toggles, 0, sizeof(priv->toggles));
	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

/* The core drops the session on a VBUS error, power cycle VBUS and restart it */
static void musb_handle_vbus_error(const struct device *dev)
{
	const struct uhc_musb_config *cfg = DEV_CFG(dev);
	struct uhc_musb_data *priv = DEV_PRIV(dev);

	LOG_ERR("VBUS error, DEVCTL 0x%02x", musb_read8(dev, MUSB_REG_DEVCTL));

	if (priv->vbus_retries >= MUSB_VBUS_RETRIES) {
		uhc_submit_event(dev, UHC_EVT_ERROR, -EIO);
		return;
	}

	priv->vbus_retries++;
	musb_xfers_drain(dev, -ENODEV);
	if (priv->connected) {
		priv->connected = false;
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}

	(void)cfg->vbus_off(dev);
	k_msleep(MUSB_VBUS_OFF_MS);
	(void)cfg->vbus_on(dev);
	musb_write8(dev, MUSB_REG_DEVCTL, MUSB_DEVCTL_SESSION);
}

static void musb_handle_events(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	uint8_t intrusb = atomic_clear(&priv->intrusb);
	uint16_t intrtx = atomic_clear(&priv->intrtx);
	uint16_t intrrx = atomic_clear(&priv->intrrx);

	if ((intrusb & MUSB_INTRUSB_VBUSERROR) != 0U) {
		musb_handle_vbus_error(dev);
	}

	if ((intrusb & MUSB_INTRUSB_BABBLE) != 0U) {
		LOG_ERR("Babble");
		uhc_submit_event(dev, UHC_EVT_ERROR, -EIO);
	}

	if ((intrusb & MUSB_INTRUSB_DISCONNECT) != 0U && priv->connected) {
		musb_handle_disconnect(dev);
	}

	if ((intrusb & MUSB_INTRUSB_CONNECT) != 0U && !priv->connected) {
		musb_handle_connect(dev);
	}

	if ((intrusb & MUSB_INTRUSB_RESUME) != 0U) {
		uhc_submit_event(dev, UHC_EVT_RWUP, 0);
	}

	if ((intrtx & BIT(0)) != 0U && priv->xfers[0] != NULL) {
		musb_ctrl_handle(dev);
	}

	for (uint8_t ep = 1U; ep < musb_num_eps(DEV_CFG(dev)); ep++) {
		const struct uhc_transfer *xfer = priv->xfers[ep];

		if (xfer == NULL) {
			continue;
		}

		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			if ((intrrx & BIT(ep)) != 0U) {
				musb_ep_in_handle(dev, ep);
			}
		} else if ((intrtx & BIT(ep)) != 0U) {
			musb_ep_out_handle(dev, ep);
		}
	}
}

/* Latch the clear-on-read interrupt status registers */
static void musb_latch_irqs(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	uint8_t intrusb = musb_read8(dev, MUSB_REG_INTRUSB);
	uint16_t intrtx = musb_read16(dev, MUSB_REG_INTRTX);
	uint16_t intrrx = musb_read16(dev, MUSB_REG_INTRRX);

	if ((intrusb | intrtx | intrrx) == 0U) {
		return;
	}

	atomic_or(&priv->intrusb, intrusb);
	atomic_or(&priv->intrtx, intrtx);
	atomic_or(&priv->intrrx, intrrx);
	k_sem_give(&priv->irq_sem);
}

void uhc_musb_isr(const struct device *dev)
{
	musb_latch_irqs(dev);
}

void uhc_musb_thread(void *arg0, void *arg1, void *arg2)
{
	const struct device *dev = arg0;
	struct uhc_musb_data *priv = DEV_PRIV(dev);

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	while (true) {
		k_sem_take(&priv->irq_sem, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);
		if (uhc_is_enabled(dev)) {
			musb_handle_events(dev);
			musb_schedule(dev);
		}
		uhc_unlock_internal(dev);
	}
}

static int uhc_musb_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_musb_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static void musb_irqs_disable(const struct device *dev)
{
	musb_write16(dev, MUSB_REG_INTRTXE, 0U);
	musb_write16(dev, MUSB_REG_INTRRXE, 0U);
	musb_write8(dev, MUSB_REG_INTRUSBE, 0U);
	(void)musb_read8(dev, MUSB_REG_INTRUSB);
	(void)musb_read16(dev, MUSB_REG_INTRTX);
	(void)musb_read16(dev, MUSB_REG_INTRRX);
}

static int uhc_musb_init(const struct device *dev)
{
	const struct uhc_musb_config *cfg = DEV_CFG(dev);
	struct uhc_data *data = dev->data;
	int ret;

	if (musb_num_eps(cfg) < 2U) {
		return -EINVAL;
	}

	cfg->irq_disable_func(dev);
	ret = cfg->init(dev);
	if (ret != 0) {
		return ret;
	}

	ret = musb_fifo_configure(dev);
	if (ret != 0) {
		(void)cfg->shutdown(dev);
		return ret;
	}

	musb_irqs_disable(dev);
	data->caps.hs = cfg->high_speed;

	return 0;
}

static int uhc_musb_enable(const struct device *dev)
{
	const struct uhc_musb_config *cfg = DEV_CFG(dev);
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	int ret;

	ret = cfg->vbus_on(dev);
	if (ret != 0) {
		return ret;
	}

	musb_write8(dev, MUSB_REG_POWER, cfg->high_speed ? MUSB_POWER_HSENAB : 0U);
	/*
	 * Start a session. With the ID pin forced to A-device, the core enters
	 * host mode once a device attaches and signals it with CONNECT.
	 */
	musb_write8(dev, MUSB_REG_DEVCTL, MUSB_DEVCTL_SESSION);

	priv->connected = false;
	priv->vbus_retries = 0U;
	atomic_clear(&priv->intrusb);
	atomic_clear(&priv->intrtx);
	atomic_clear(&priv->intrrx);

	musb_write16(dev, MUSB_REG_INTRTXE, BIT(musb_num_eps(cfg)) - 1U);
	musb_write16(dev, MUSB_REG_INTRRXE, (BIT(musb_num_eps(cfg)) - 1U) & ~BIT(0));
	musb_write8(dev, MUSB_REG_INTRUSBE,
		    MUSB_INTRUSB_CONNECT | MUSB_INTRUSB_DISCONNECT | MUSB_INTRUSB_RESUME |
			    MUSB_INTRUSB_BABBLE | MUSB_INTRUSB_VBUSERROR);
	cfg->irq_enable_func(dev);

	/* Catch a device connected before the interrupts were enabled */
	musb_latch_irqs(dev);

	return 0;
}

static int uhc_musb_disable(const struct device *dev)
{
	const struct uhc_musb_config *cfg = DEV_CFG(dev);
	struct uhc_musb_data *priv = DEV_PRIV(dev);

	cfg->irq_disable_func(dev);
	musb_irqs_disable(dev);

	musb_xfers_drain(dev, -ESHUTDOWN);
	musb_write8(dev, MUSB_REG_DEVCTL, 0U);
	memset(priv->toggles, 0, sizeof(priv->toggles));

	if (priv->connected) {
		priv->connected = false;
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}

	return cfg->vbus_off(dev);
}

static int uhc_musb_shutdown(const struct device *dev)
{
	return DEV_CFG(dev)->shutdown(dev);
}

static int uhc_musb_bus_reset(const struct device *dev)
{
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	uint8_t power;

	power = musb_read8(dev, MUSB_REG_POWER);
	musb_write8(dev, MUSB_REG_POWER, power | MUSB_POWER_RESET);
	k_msleep(MUSB_RESET_MS);
	musb_write8(dev, MUSB_REG_POWER, power & ~MUSB_POWER_RESET);
	k_msleep(MUSB_RESET_RECOVERY_MS);

	memset(priv->toggles, 0, sizeof(priv->toggles));
	uhc_submit_event(dev, UHC_EVT_RESETED, 0);

	return 0;
}

static int uhc_musb_sof_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* The core sends SOFs whenever the port is not suspended */
	return 0;
}

static int uhc_musb_bus_suspend(const struct device *dev)
{
	uint8_t power = musb_read8(dev, MUSB_REG_POWER);

	if ((power & MUSB_POWER_SUSPENDM) != 0U) {
		return -EALREADY;
	}

	musb_write8(dev, MUSB_REG_POWER, power | MUSB_POWER_SUSPENDM);
	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	return 0;
}

static int uhc_musb_bus_resume(const struct device *dev)
{
	uint8_t power = musb_read8(dev, MUSB_REG_POWER);

	if ((power & MUSB_POWER_SUSPENDM) == 0U) {
		return -EALREADY;
	}

	power &= ~MUSB_POWER_SUSPENDM;
	musb_write8(dev, MUSB_REG_POWER, power | MUSB_POWER_RESUME);
	k_msleep(MUSB_RESUME_MS);
	musb_write8(dev, MUSB_REG_POWER, power);
	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);

	return 0;
}

static int uhc_musb_enqueue(const struct device *dev, struct uhc_transfer *xfer)
{
	uint16_t max_mps;
	int ret;

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		max_mps = MUSB_EP0_FIFO_BYTES;
		break;
	case USB_EP_TYPE_BULK:
	case USB_EP_TYPE_INTERRUPT:
		max_mps = MUSB_FIFO_BYTES;
		break;
	default:
		LOG_WRN("Transfer type %u not supported", xfer->type);
		return -ENOTSUP;
	}

	if (xfer->mps == 0U || xfer->mps > max_mps) {
		return -EINVAL;
	}

	ret = uhc_xfer_append(dev, xfer);
	if (ret == 0) {
		k_sem_give(&DEV_PRIV(dev)->irq_sem);
	}

	return ret;
}

static int uhc_musb_dequeue(const struct device *dev, struct uhc_transfer *xfer)
{
	struct uhc_data *data = dev->data;
	struct uhc_musb_data *priv = DEV_PRIV(dev);
	struct uhc_transfer *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (tmp != xfer) {
			continue;
		}

		for (uint8_t ep = 0U; ep < UHC_MUSB_MAX_EPS; ep++) {
			if (priv->xfers[ep] == xfer) {
				musb_xfer_abort(dev, ep, xfer);
				priv->xfers[ep] = NULL;
			}
		}

		uhc_xfer_return(dev, xfer, -ECONNRESET);
		/* Let the thread start the next transfer */
		k_sem_give(&priv->irq_sem);
		break;
	}

	return 0;
}

DEVICE_API(uhc, uhc_musb_api) = {
	.lock = uhc_musb_lock,
	.unlock = uhc_musb_unlock,
	.init = uhc_musb_init,
	.enable = uhc_musb_enable,
	.disable = uhc_musb_disable,
	.shutdown = uhc_musb_shutdown,
	.bus_reset = uhc_musb_bus_reset,
	.sof_enable = uhc_musb_sof_enable,
	.bus_suspend = uhc_musb_bus_suspend,
	.bus_resume = uhc_musb_bus_resume,
	.ep_enqueue = uhc_musb_enqueue,
	.ep_dequeue = uhc_musb_dequeue,
};
