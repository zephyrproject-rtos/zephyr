/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB host controller driver for the Mentor Graphics MUSB core.
 *
 * The MUSB core is a dual-role controller. This driver implements the host
 * (UHC) role; the peripheral role is provided by drivers/usb/udc/udc_musb.c.
 * Vendor-specific wrapper handling (controller reset, role selection, PHY
 * setup and wrapper interrupts) is provided through struct
 * uhc_musb_vendor_quirks, see uhc_musb.h.
 *
 * Supported: root-port connect / disconnect detection, bus reset with
 * high-speed detection, EP0 control transfers and EP1-15 bulk, interrupt and
 * isochronous transfers. Hubs and low-speed devices are not supported.
 */

#define DT_DRV_COMPAT mentor_musb_host

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/uhc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_musb, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* Included after the log module so that the vendor glue can log errors. */
#include "uhc_common.h"
#include "uhc_musb.h"

/*
 * FIFO RAM layout. EP0 uses the fixed 64-byte region at offset 0; the
 * allocator hands out space for EP1-15 above that. FIFO start addresses are
 * programmed in 8-byte units (TXFIFOADD/RXFIFOADD), sizes as a log2 code.
 */
#define MUSB_HOST_FIFO_UNIT     8U
#define MUSB_HOST_FIFO_EP0_SIZE 64U
#define MUSB_HOST_FIFO_RAM_SIZE 4096U /* RAM reserved for EP1-15 FIFOs */
#define MUSB_HOST_FIFO_SIZE_MIN 8U

/* Timeout waiting for VBUS to reach the valid level after enabling the switch */
#define MUSB_HOST_VBUS_VALID_TIMEOUT_MS 100

/* USB bus timing for host operations */
#define MUSB_HOST_ATTACH_DEBOUNCE_MS 100 /* connect debounce before reset */
#define MUSB_HOST_RESET_HOLD_MS      50  /* SE0 reset signaling duration */
#define MUSB_HOST_RESET_RECOVERY_MS  10  /* device recovery after reset */
#define MUSB_HOST_RESUME_MS          20  /* resume signaling duration */
#define MUSB_HOST_VBUSERR_RESET_MS   100 /* NRSTX soft reset on VBUS error */

/*
 * EP0 NAK limit: 0 disables the NAK timeout so the core retries NAKs for the
 * duration of a control transaction (normal during enumeration). A control
 * request that never completes is cancelled by the host stack timeout.
 */
#define MUSB_HOST_EP0_NAKLIMIT 0U

/* Low level helpers */

/* Select the EP0 register window. */
static ALWAYS_INLINE void musb_ep0_select(struct usb_musb_reg *const base)
{
	musb_write8(&base->index, MUSB_INDEX_SELEP(MUSB_HOST_EP0_IDX));
}

/* Program the target function address and speed used for EP0 transactions. */
static void musb_ep0_set_target(struct usb_musb_reg *const base,
				const struct usb_device *udev)
{
	uint8_t type0;

	musb_ep0_select(base);

	musb_write8(&base->target[MUSB_HOST_EP0_IDX].txfuncaddr, MUSB_FUNCADDR(udev->addr));
	musb_write8(&base->target[MUSB_HOST_EP0_IDX].rxfuncaddr, MUSB_FUNCADDR(udev->addr));
	/* Direct connection: no hub between host and device. */
	musb_write8(&base->target[MUSB_HOST_EP0_IDX].txhubaddr, 0U);
	musb_write8(&base->target[MUSB_HOST_EP0_IDX].rxhubaddr, 0U);

	switch (udev->speed) {
	case USB_SPEED_SPEED_HS:
		type0 = MUSB_TYPE_SPEED(MUSB_TYPE_SPEED_HIGH);
		break;
	case USB_SPEED_SPEED_LS:
		type0 = MUSB_TYPE_SPEED(MUSB_TYPE_SPEED_LOW);
		break;
	case USB_SPEED_SPEED_FS:
	default:
		type0 = MUSB_TYPE_SPEED(MUSB_TYPE_SPEED_FULL);
		break;
	}

	musb_write8(&base->type0, type0);
	musb_write8(&base->naklimit0, MUSB_HOST_EP0_NAKLIMIT);
}

/* Byte-wise FIFO write (writes are byte-addressable on the FIFO port). */
static void musb_fifo_write(struct usb_musb_reg *const base, uint8_t ep,
			    const uint8_t *src, size_t len)
{
	const mem_addr_t fifo = musb_fifo_addr(base, ep);

	for (size_t i = 0; i < len; i++) {
		sys_write8(src[i], fifo);
	}
}

/*
 * Word-wise FIFO read. The FIFO port pops a full 32-bit word on every read
 * access, so a byte-wide read would discard three bytes per access.
 */
static void musb_fifo_read(struct usb_musb_reg *const base, uint8_t ep,
			   uint8_t *dst, size_t len)
{
	const mem_addr_t fifo = musb_fifo_addr(base, ep);
	size_t words = len / MUSB_HOST_FIFO_WORD_SIZE;
	size_t rem = len % MUSB_HOST_FIFO_WORD_SIZE;
	uint32_t word;

	for (size_t i = 0; i < words; i++) {
		word = sys_read32(fifo);
		*dst++ = (uint8_t)(word);
		*dst++ = (uint8_t)(word >> 8);
		*dst++ = (uint8_t)(word >> 16);
		*dst++ = (uint8_t)(word >> 24);
	}

	if (rem > 0) {
		word = sys_read32(fifo);
		for (size_t j = 0; j < rem; j++) {
			*dst++ = (uint8_t)(word >> (j * 8));
		}
	}
}

/* EP0 control transfer state machine */

static void musb_ep0_prime_setup(const struct device *dev,
				 struct uhc_transfer *const xfer)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	musb_ep0_set_target(base, xfer->udev);
	musb_fifo_write(base, MUSB_HOST_EP0_IDX, xfer->setup_pkt, MUSB_HOST_SETUP_PACKET_SIZE);
	musb_write8(&base->csr0l, MUSB_CSR0L_TXPKTRDY_Msk | MUSB_CSR0L_HOST_SETUPPKT_Msk);

	xfer->stage = UHC_CONTROL_STAGE_SETUP;
}

static void musb_ep0_prime_data_in(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	musb_ep0_select(base);
	musb_write8(&base->csr0l, MUSB_CSR0L_HOST_REQPKT_Msk);
}

static void musb_ep0_prime_data_out(const struct device *dev,
				    struct uhc_transfer *const xfer)
{
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct net_buf *buf = xfer->buf;
	size_t len = MIN(xfer->mps, buf->len);

	musb_ep0_select(base);
	musb_fifo_write(base, MUSB_HOST_EP0_IDX, buf->data, len);
	net_buf_pull(buf, len);
	musb_write8(&base->csr0l, MUSB_CSR0L_TXPKTRDY_Msk);
}

static void musb_ep0_prime_status(const struct device *dev,
				  struct uhc_transfer *const xfer)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	musb_ep0_select(base);
	xfer->stage = UHC_CONTROL_STAGE_STATUS;

	/*
	 * Control read (data stage IN): the status stage is an OUT (zero-length
	 * DATA1). Control write and no-data control: the status stage is an IN.
	 */
	if (xfer->buf != NULL && USB_EP_DIR_IS_IN(xfer->ep)) {
		musb_write8(&base->csr0l,
			    MUSB_CSR0L_TXPKTRDY_Msk | MUSB_CSR0L_HOST_STATUSPKT_Msk);
	} else {
		musb_write8(&base->csr0l,
			    MUSB_CSR0L_HOST_REQPKT_Msk | MUSB_CSR0L_HOST_STATUSPKT_Msk);
	}
}

/* Stop the current EP0 transaction and leave EP0 idle. */
static void musb_ep0_abort(struct usb_musb_reg *const base)
{
	musb_ep0_select(base);

	if ((musb_read8(&base->csr0l) &
	     (MUSB_CSR0L_RXPKTRDY_Msk | MUSB_CSR0L_TXPKTRDY_Msk)) != 0U) {
		musb_write8(&base->csr0h, MUSB_CSR0H_FLUSHFIFO_Msk);
	}

	musb_write8(&base->csr0l, 0U);
}

/*
 * Transfer scheduler: dispatches queued control (EP0) and bulk, interrupt and
 * isochronous (EP1-15) transfers to idle hardware channels.
 */
static void musb_schedule(const struct device *dev);

/*
 * A transfer cancelled by uhc_musb_ep_dequeue() while it completes is still
 * reported as cancelled, the owner relies on -ECONNRESET to release it.
 */
static ALWAYS_INLINE int musb_xfer_status(const struct uhc_transfer *const xfer, const int err)
{
	return (xfer->err == -ECONNRESET) ? -ECONNRESET : err;
}

/* Complete the active control transfer and start the next transfer. */
static void musb_ep0_finish(const struct device *dev, int err)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->active_xfer;

	if (xfer != NULL) {
		priv->active_xfer = NULL;
		uhc_xfer_return(dev, xfer, musb_xfer_status(xfer, err));
	}

	musb_schedule(dev);
}

/* Advance the EP0 state machine on a completed EP0 transaction. */
static void musb_ep0_advance(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct uhc_transfer *xfer = priv->active_xfer;
	uint8_t csr0;

	if (xfer == NULL) {
		return;
	}

	musb_ep0_select(base);
	csr0 = musb_read8(&base->csr0l);

	/* Error conditions take priority over stage handling. */
	if (csr0 & MUSB_CSR0L_HOST_RXSTALL_Msk) {
		musb_write8(&base->csr0l, (uint8_t)(csr0 & ~MUSB_CSR0L_HOST_RXSTALL_Msk));
		LOG_DBG("EP0 stalled");
		musb_ep0_finish(dev, -EPIPE);
		return;
	}

	if (csr0 & MUSB_CSR0L_HOST_ERROR_Msk) {
		musb_write8(&base->csr0l, (uint8_t)(csr0 & ~MUSB_CSR0L_HOST_ERROR_Msk));
		LOG_DBG("EP0 transaction error");
		musb_ep0_finish(dev, -EIO);
		return;
	}

	if (csr0 & MUSB_CSR0L_HOST_NAKTIMEOUT_Msk) {
		/* NAK limit reached: clear and re-issue the current phase. */
		musb_write8(&base->csr0l, (uint8_t)(csr0 & ~MUSB_CSR0L_HOST_NAKTIMEOUT_Msk));
		LOG_DBG("EP0 NAK timeout, retrying");
		if (xfer->stage == UHC_CONTROL_STAGE_DATA && USB_EP_DIR_IS_IN(xfer->ep)) {
			musb_ep0_prime_data_in(dev);
		}
		return;
	}

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		if (xfer->buf != NULL) {
			xfer->stage = UHC_CONTROL_STAGE_DATA;
			if (USB_EP_DIR_IS_IN(xfer->ep)) {
				musb_ep0_prime_data_in(dev);
			} else {
				musb_ep0_prime_data_out(dev, xfer);
			}
		} else {
			musb_ep0_prime_status(dev, xfer);
		}
		break;

	case UHC_CONTROL_STAGE_DATA:
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			struct net_buf *buf = xfer->buf;
			uint16_t wlength = sys_get_le16(&xfer->setup_pkt[6]);
			uint8_t count;
			size_t len;

			if (!(csr0 & MUSB_CSR0L_RXPKTRDY_Msk)) {
				/* No data yet; wait for the next interrupt. */
				break;
			}

			count = musb_read8(&base->count0);
			len = MIN(count, net_buf_tailroom(buf));
			musb_fifo_read(base, MUSB_HOST_EP0_IDX, net_buf_add(buf, len), len);

			/* Acknowledge the packet (clear RxPktRdy). */
			musb_write8(&base->csr0l, (uint8_t)(csr0 & ~MUSB_CSR0L_RXPKTRDY_Msk));

			/*
			 * The data stage ends on a short packet, once wLength
			 * bytes have been received, or when the buffer is full.
			 * wLength is used rather than the buffer capacity because
			 * the host stack may allocate a larger buffer.
			 */
			if (count < xfer->mps || buf->len >= wlength ||
			    net_buf_tailroom(buf) == 0) {
				musb_ep0_prime_status(dev, xfer);
			} else {
				musb_ep0_prime_data_in(dev);
			}
		} else {
			/* OUT data packet acknowledged by the device. */
			if (xfer->buf != NULL && xfer->buf->len > 0) {
				musb_ep0_prime_data_out(dev, xfer);
			} else {
				musb_ep0_prime_status(dev, xfer);
			}
		}
		break;

	case UHC_CONTROL_STAGE_STATUS:
		/*
		 * The status transaction has completed. Clear every residual
		 * EP0 command and status bit so that the next control transfer
		 * starts from a clean CSR0; a leftover StatusPkt or RxPktRdy
		 * corrupts the following SETUP transaction.
		 */
		musb_write8(&base->csr0l, 0U);
		musb_ep0_finish(dev, 0);
		break;

	default:
		break;
	}
}

/* EP1-15 transfer engine (bulk, interrupt and isochronous) */

/* Map a USB device speed to the TXTYPE/RXTYPE SPEED field value. */
static uint8_t musb_type_speed(enum usb_device_speed speed)
{
	switch (speed) {
	case USB_SPEED_SPEED_HS:
		return MUSB_TYPE_SPEED_HIGH;
	case USB_SPEED_SPEED_LS:
		return MUSB_TYPE_SPEED_LOW;
	case USB_SPEED_SPEED_FS:
	default:
		return MUSB_TYPE_SPEED_FULL;
	}
}

/* Map a UHC endpoint type to the TXTYPE/RXTYPE PROTOCOL field value. */
static uint8_t musb_type_proto(uint8_t ep_type)
{
	switch (ep_type & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_ISO:
		return MUSB_TYPE_PROTOCOL_ISO;
	case USB_EP_TYPE_INTERRUPT:
		return MUSB_TYPE_PROTOCOL_INTERRUPT;
	case USB_EP_TYPE_BULK:
	default:
		return MUSB_TYPE_PROTOCOL_BULK;
	}
}

/* Round a FIFO size up to the next power of two (at least 8 bytes). */
static uint16_t musb_fifo_round(uint16_t size)
{
	uint16_t s = MUSB_HOST_FIFO_SIZE_MIN;

	while (s < size) {
		s <<= 1;
	}

	return s;
}

/* FIFO size field code: 8 << code == size. */
static uint8_t musb_fifo_size_code(uint16_t size)
{
	uint8_t code = 0;
	uint16_t s = MUSB_HOST_FIFO_SIZE_MIN;

	while (s < size) {
		s <<= 1;
		code++;
	}

	return code;
}

/* Allocate a FIFO region for an EP1-15 channel. Returns byte offset or 0. */
static uint16_t musb_fifo_alloc(struct uhc_musb_data *priv, uint16_t size)
{
	uint16_t addr = priv->fifo_next;

	if ((uint32_t)addr + size > MUSB_HOST_FIFO_EP0_SIZE + MUSB_HOST_FIFO_RAM_SIZE) {
		return 0U;
	}

	priv->fifo_next += size;

	return addr;
}

/* Configure a hardware channel for a device endpoint on its first use. */
static int musb_epx_configure(const struct device *dev, uint8_t idx,
			      bool dir_in, struct uhc_transfer *const xfer)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct uhc_musb_hep *hep = dir_in ? &priv->hep_in[idx] : &priv->hep_out[idx];
	uint16_t payload = xfer->mps & 0x7FFU;
	uint8_t mult = (uint8_t)(((xfer->mps >> 11) & 0x3U) + 1U);
	uint8_t proto = musb_type_proto(xfer->type);
	uint8_t speed = musb_type_speed(xfer->udev->speed);
	uint8_t type = MUSB_TYPE_SPEED(speed) | MUSB_TYPE_PROTOCOL(proto) |
		       MUSB_TYPE_TARGET_EP(idx);
	uint16_t fifo_bytes;
	uint16_t addr;

	if (hep->configured) {
		return 0;
	}

	fifo_bytes = musb_fifo_round((uint16_t)(payload * mult));
	addr = musb_fifo_alloc(priv, fifo_bytes);
	if (addr == 0U) {
		LOG_ERR("EP%u: out of FIFO RAM (%u bytes)", idx, fifo_bytes);
		return -ENOMEM;
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(idx));

	/*
	 * TXMAXP/RXMAXP take the raw wMaxPacketSize: bits 12:11 (additional
	 * transactions per microframe) map onto the high-bandwidth multiplier
	 * field, so the value must not be masked to the payload bits.
	 */
	if (dir_in) {
		uint8_t rxcsrh = 0U;

		musb_write8(&base->target[idx].rxfuncaddr, MUSB_FUNCADDR(xfer->udev->addr));
		musb_write8(&base->target[idx].rxhubaddr, 0U);
		musb_write16(&base->rxmaxp, xfer->mps);
		musb_write8(&base->rxtype, type);
		musb_write8(&base->rxinterval, (uint8_t)xfer->interval);
		musb_write8(&base->rxfifosz, MUSB_RXFIFOSZ_SZ(musb_fifo_size_code(fifo_bytes)));
		musb_write16(&base->rxfifoadd,
			     MUSB_RXFIFOADD_ADDR(addr / MUSB_HOST_FIFO_UNIT));

		/*
		 * Isochronous IN: with AUTOREQ the core requests the next
		 * packet as soon as RxPktRdy is cleared, which is needed for
		 * continuous streaming.
		 */
		if (proto == MUSB_TYPE_PROTOCOL_ISO) {
			rxcsrh = MUSB_RXCSRH_HOST_AUTOREQ_Msk;
		}

		musb_write8(&base->rxcsrh, rxcsrh);
		musb_write8(&base->rxcsrl, 0U);
		musb_set16(&base->intrrxe, (uint16_t)BIT(idx));
	} else {
		musb_write8(&base->target[idx].txfuncaddr, MUSB_FUNCADDR(xfer->udev->addr));
		musb_write8(&base->target[idx].txhubaddr, 0U);
		musb_write16(&base->txmaxp, xfer->mps);
		musb_write8(&base->txtype, type);
		musb_write8(&base->txinterval, (uint8_t)xfer->interval);
		musb_write8(&base->txfifosz, MUSB_TXFIFOSZ_SZ(musb_fifo_size_code(fifo_bytes)));
		musb_write16(&base->txfifoadd,
			     MUSB_TXFIFOADD_ADDR(addr / MUSB_HOST_FIFO_UNIT));
		musb_write8(&base->txcsrh, 0U);
		musb_write8(&base->txcsrl, 0U);
		musb_set16(&base->intrtxe, (uint16_t)BIT(idx));
	}

	hep->proto = proto;
	hep->configured = true;
	hep->halted = false;

	LOG_DBG("EP%u %s: type 0x%02x mps %u fifo %u@%u", idx, dir_in ? "IN" : "OUT",
		type, xfer->mps, fifo_bytes, addr);

	return 0;
}

/* Start the current transfer on a hardware channel. */
static void musb_epx_start(const struct device *dev, struct uhc_musb_hep *hep,
			   uint8_t idx, bool dir_in, struct uhc_transfer *const xfer)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	musb_write8(&base->index, MUSB_INDEX_SELEP(idx));

	if (dir_in) {
		if (hep->halted && hep->proto == MUSB_TYPE_PROTOCOL_ISO) {
			/* Re-enable the automatic IN requests stopped by musb_epx_halt(). */
			musb_write8(&base->rxcsrh, MUSB_RXCSRH_HOST_AUTOREQ_Msk);
		}

		/* Request an IN transaction. */
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_HOST_REQPKT_Msk);
	} else {
		struct net_buf *buf = xfer->buf;
		size_t len = (buf != NULL) ? MIN((size_t)xfer->mps, buf->len) : 0;

		if (len > 0) {
			musb_fifo_write(base, idx, buf->data, len);
			net_buf_pull(buf, len);
		}
		musb_write8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
	}

	hep->halted = false;
}

/*
 * Stop a hardware channel: no further IN requests or OUT packets, FIFO
 * flushed. The FIFO is flushed twice to cover double-buffered packets.
 */
static void musb_epx_halt(struct usb_musb_reg *const base, uint8_t idx, bool dir_in)
{
	musb_write8(&base->index, MUSB_INDEX_SELEP(idx));

	if (dir_in) {
		musb_write8(&base->rxcsrh, 0U);
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->rxcsrl, 0U);
	} else {
		musb_write8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->txcsrl, 0U);
	}
}

/* Complete the active transfer on a hardware channel and schedule the next. */
static void musb_epx_complete(const struct device *dev, uint8_t idx,
			      bool dir_in, int err)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_musb_hep *hep = dir_in ? &priv->hep_in[idx] : &priv->hep_out[idx];
	struct uhc_transfer *xfer = hep->xfer;

	if (xfer == NULL) {
		return;
	}

	hep->xfer = NULL;
	uhc_xfer_return(dev, xfer, musb_xfer_status(xfer, err));
	musb_schedule(dev);
}

/* Handle an IN completion (INTRRX) on EP1-15. */
static void musb_epx_in_done(const struct device *dev, uint8_t idx)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct uhc_musb_hep *hep = &priv->hep_in[idx];
	struct uhc_transfer *xfer = hep->xfer;
	struct net_buf *buf;
	uint16_t count;
	size_t len;
	uint8_t csr;

	if (xfer == NULL) {
		return;
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(idx));
	csr = musb_read8(&base->rxcsrl);

	if (csr & MUSB_RXCSRL_HOST_RXSTALL_Msk) {
		musb_write8(&base->rxcsrl, (uint8_t)(csr & ~MUSB_RXCSRL_HOST_RXSTALL_Msk));
		musb_epx_complete(dev, idx, true, -EPIPE);
		return;
	}

	if (csr & MUSB_RXCSRL_HOST_ERROR_Msk) {
		musb_write8(&base->rxcsrl, (uint8_t)(csr & ~MUSB_RXCSRL_HOST_ERROR_Msk));
		musb_epx_complete(dev, idx, true, -EIO);
		return;
	}

	if (!(csr & MUSB_RXCSRL_RXPKTRDY_Msk)) {
		return;
	}

	buf = xfer->buf;
	count = musb_read16(&base->rxcount);
	len = MIN((size_t)count, net_buf_tailroom(buf));
	musb_fifo_read(base, idx, net_buf_add(buf, len), len);

	/* Acknowledge the packet (clear RxPktRdy). */
	musb_write8(&base->rxcsrl, (uint8_t)(csr & ~MUSB_RXCSRL_RXPKTRDY_Msk));

	/*
	 * Isochronous and interrupt transfers complete after one service
	 * interval. Bulk transfers continue until a short packet or until the
	 * buffer is full.
	 */
	if (hep->proto == MUSB_TYPE_PROTOCOL_BULK &&
	    count == (xfer->mps & 0x7FFU) && net_buf_tailroom(buf) > 0) {
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_HOST_REQPKT_Msk);
		return;
	}

	musb_epx_complete(dev, idx, true, 0);
}

/* Handle an OUT completion (INTRTX) on EP1-15. */
static void musb_epx_out_done(const struct device *dev, uint8_t idx)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct uhc_musb_hep *hep = &priv->hep_out[idx];
	struct uhc_transfer *xfer = hep->xfer;
	uint8_t csr;

	if (xfer == NULL) {
		return;
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(idx));
	csr = musb_read8(&base->txcsrl);

	if (csr & MUSB_TXCSRL_HOST_RXSTALL_Msk) {
		musb_write8(&base->txcsrl, (uint8_t)(csr & ~MUSB_TXCSRL_HOST_RXSTALL_Msk));
		musb_epx_complete(dev, idx, false, -EPIPE);
		return;
	}

	if (csr & MUSB_TXCSRL_HOST_ERROR_Msk) {
		musb_write8(&base->txcsrl, (uint8_t)(csr & ~MUSB_TXCSRL_HOST_ERROR_Msk));
		musb_epx_complete(dev, idx, false, -EIO);
		return;
	}

	/* Packet accepted by the device: send the next chunk or finish. */
	if (xfer->buf != NULL && xfer->buf->len > 0) {
		size_t len = MIN((size_t)xfer->mps, xfer->buf->len);

		musb_fifo_write(base, idx, xfer->buf->data, len);
		net_buf_pull(xfer->buf, len);
		musb_write8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
	} else {
		musb_epx_complete(dev, idx, false, 0);
	}
}

/*
 * Reset all EP1-15 channel state and hardware on connect and disconnect, so
 * that the next device starts with unconfigured channels and an empty FIFO
 * allocator.
 */
static void musb_eps_reset(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);

	memset(priv->hep_in, 0, sizeof(priv->hep_in));
	memset(priv->hep_out, 0, sizeof(priv->hep_out));
	priv->fifo_next = MUSB_HOST_FIFO_EP0_SIZE;

	atomic_clear(&priv->intrtx_pending);
	atomic_clear(&priv->intrrx_pending);

	/* Disable EP1-15 interrupts, keep EP0 for control transfers. */
	musb_write16(&base->intrtxe, MUSB_INTRTXE_EP0TXEN_Msk);
	musb_write16(&base->intrrxe, 0U);

	/* Discard pending interrupt status (read-to-clear). */
	(void)musb_read16(&base->intrtx);
	(void)musb_read16(&base->intrrx);

	/* Flush FIFOs (twice, for double-buffered packets) and clear the CSRs. */
	for (uint8_t idx = 1; idx < config->num_of_eps; idx++) {
		musb_write8(&base->index, MUSB_INDEX_SELEP(idx));

		musb_write8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);
		musb_write8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);

		musb_write8(&base->txcsrl, 0U);
		musb_write8(&base->txcsrh, 0U);
		musb_write8(&base->rxcsrl, 0U);
		musb_write8(&base->rxcsrh, 0U);
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(0));
}

/* Return every transfer owned by the driver, active or still queued. */
static void musb_xfers_abort(const struct device *dev, const int err)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer;
	struct uhc_transfer *tmp;

	/* Active transfers stay on the queue until they are returned. */
	priv->active_xfer = NULL;
	for (uint8_t idx = 1; idx < config->num_of_eps; idx++) {
		priv->hep_in[idx].xfer = NULL;
		priv->hep_out[idx].xfer = NULL;
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		uhc_xfer_return(dev, xfer, err);
	}
}

/* Stop and return the transfers cancelled by uhc_musb_ep_dequeue(). */
static void musb_xfers_drop_cancelled(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer;
	struct uhc_transfer *tmp;

	if (priv->active_xfer != NULL && priv->active_xfer->err == -ECONNRESET) {
		musb_ep0_abort(base);
		priv->active_xfer = NULL;
	}

	for (uint8_t idx = 1; idx < config->num_of_eps; idx++) {
		struct uhc_musb_hep *hep_in = &priv->hep_in[idx];
		struct uhc_musb_hep *hep_out = &priv->hep_out[idx];

		if (hep_in->xfer != NULL && hep_in->xfer->err == -ECONNRESET) {
			musb_epx_halt(base, idx, true);
			hep_in->halted = true;
			hep_in->xfer = NULL;
		}

		if (hep_out->xfer != NULL && hep_out->xfer->err == -ECONNRESET) {
			musb_epx_halt(base, idx, false);
			hep_out->halted = true;
			hep_out->xfer = NULL;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		if (xfer->err == -ECONNRESET) {
			uhc_xfer_return(dev, xfer, -ECONNRESET);
		}
	}
}

static void musb_schedule(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer;
	struct uhc_transfer *tmp;

	musb_xfers_drop_cancelled(dev);

	if (!atomic_test_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED)) {
		return;
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		uint8_t idx = USB_EP_GET_IDX(xfer->ep);
		struct uhc_musb_hep *hep;
		bool dir_in;

		if (idx == MUSB_HOST_EP0_IDX) {
			/* One control transfer at a time. */
			if (priv->active_xfer == NULL) {
				priv->active_xfer = xfer;
				musb_ep0_prime_setup(dev, xfer);
			}
			continue;
		}

		if (idx >= config->num_of_eps) {
			LOG_ERR("Endpoint 0x%02x not supported", xfer->ep);
			uhc_xfer_return(dev, xfer, -EINVAL);
			continue;
		}

		dir_in = USB_EP_DIR_IS_IN(xfer->ep);
		hep = dir_in ? &priv->hep_in[idx] : &priv->hep_out[idx];

		/* Channel already servicing a transfer. */
		if (hep->xfer != NULL) {
			continue;
		}

		if (musb_epx_configure(dev, idx, dir_in, xfer) != 0) {
			uhc_xfer_return(dev, xfer, -ENOMEM);
			continue;
		}

		hep->xfer = xfer;
		musb_epx_start(dev, hep, idx, dir_in, xfer);
	}
}

/* Root port handling */

/* Assert and release USB reset signaling on the root port. */
static void musb_do_bus_reset(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t power;

	power = musb_read8(&base->power);
	musb_write8(&base->power, power | MUSB_POWER_RESET_Msk);
	k_msleep(MUSB_HOST_RESET_HOLD_MS);

	power = musb_read8(&base->power);
	musb_write8(&base->power, (uint8_t)(power & ~MUSB_POWER_RESET_Msk));
	k_msleep(MUSB_HOST_RESET_RECOVERY_MS);
}

/* Determine the negotiated speed after a bus reset. */
static enum usb_device_speed musb_reset_speed(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	if (musb_read8(&base->power) & MUSB_POWER_HSMODE_Msk) {
		return USB_SPEED_SPEED_HS;
	}

	return USB_SPEED_SPEED_FS;
}

static void musb_handle_connect(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	enum uhc_event_type type;

	/*
	 * Debounce, then reset the port so that the device speed can be
	 * detected (high speed is only known after the reset chirp handshake).
	 */
	k_msleep(MUSB_HOST_ATTACH_DEBOUNCE_MS);
	musb_do_bus_reset(dev);
	priv->speed = musb_reset_speed(dev);
	musb_eps_reset(dev);

	/* Transfers are returned on disconnect, none should be left. */
	if (!sys_dlist_is_empty(&data->ctrl_xfers)) {
		LOG_WRN("Stale transfers on connect");
		musb_xfers_abort(dev, -ESHUTDOWN);
	}

	atomic_set_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED);

	if (priv->speed == USB_SPEED_SPEED_HS) {
		type = UHC_EVT_DEV_CONNECTED_HS;
	} else {
		type = UHC_EVT_DEV_CONNECTED_FS;
	}

	LOG_DBG("Device connected (%s speed)",
		priv->speed == USB_SPEED_SPEED_HS ? "high" : "full");
	uhc_submit_event(dev, type, 0);
}

static void musb_handle_disconnect(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);

	atomic_clear_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED);
	priv->speed = USB_SPEED_UNKNOWN;

	musb_xfers_abort(dev, -ESHUTDOWN);
	musb_eps_reset(dev);

	LOG_DBG("Device disconnected");
	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

static void musb_handle_usb_events(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	uint8_t intrusb = (uint8_t)atomic_clear(&priv->intrusb_pending);

	if (intrusb & MUSB_INTRUSB_VBUSERR_Msk) {
		struct usb_musb_reg *const base = musb_get_base(dev);

		LOG_WRN("VBUS error, restarting session");

		/* A VBUS error ends the session: treat it like a disconnect. */
		if (atomic_test_and_clear_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED)) {
			musb_xfers_abort(dev, -ESHUTDOWN);
			musb_eps_reset(dev);
			uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
		}

		musb_write8(&base->softrst, MUSB_SOFTRST_NRSTX_Msk);
		k_msleep(MUSB_HOST_VBUSERR_RESET_MS);
		musb_set8(&base->devctl, MUSB_DEVCTL_SESSION_Msk);
	}

	if (intrusb & MUSB_INTRUSB_DISCON_Msk) {
		musb_handle_disconnect(dev);
	}

	if (intrusb & MUSB_INTRUSB_CONN_Msk) {
		musb_handle_connect(dev);
	}
}

/* Interrupt handling */

static void uhc_musb_isr_handler(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t intrusb;
	uint16_t intrtx;
	uint16_t intrrx;

	/* Reading INTRUSB, INTRTX and INTRRX clears the active bits. */
	intrusb = musb_read8(&base->intrusb);
	intrtx = musb_read16(&base->intrtx);
	intrrx = musb_read16(&base->intrrx);

	/* Acknowledge the vendor wrapper once the core status has been latched. */
	(void)uhc_musb_quirk_irq_clear(dev);

	if (intrusb & (MUSB_INTRUSB_CONN_Msk | MUSB_INTRUSB_DISCON_Msk |
		       MUSB_INTRUSB_VBUSERR_Msk)) {
		atomic_or(&priv->intrusb_pending, intrusb);
		k_event_post(&priv->events, MUSB_HOST_EVT_USB);
	}

	/* EP0 activity is reported through INTRTX bit 0 in host mode. */
	if (intrtx & MUSB_INTRTX_EP0TX_Msk) {
		k_event_post(&priv->events, MUSB_HOST_EVT_EP0);
	}

	/* EP1-15 TX (bit N) and RX (bit N) completions. */
	intrtx &= (uint16_t)~MUSB_INTRTX_EP0TX_Msk;
	if (intrtx != 0U) {
		atomic_or(&priv->intrtx_pending, intrtx);
		k_event_post(&priv->events, MUSB_HOST_EVT_EPX);
	}

	if (intrrx != 0U) {
		atomic_or(&priv->intrrx_pending, intrrx);
		k_event_post(&priv->events, MUSB_HOST_EVT_EPX);
	}
}

/* Worker thread */

static void uhc_musb_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct uhc_musb_config *config = dev->config;
	struct uhc_musb_data *priv = uhc_get_private(dev);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		uint32_t evt = k_event_wait(&priv->events,
					    MUSB_HOST_EVT_USB | MUSB_HOST_EVT_EP0 |
					    MUSB_HOST_EVT_EPX | MUSB_HOST_EVT_XFR,
					    false, K_FOREVER);

		k_event_clear(&priv->events, evt);

		uhc_lock_internal(dev, K_FOREVER);

		if (evt & MUSB_HOST_EVT_USB) {
			musb_handle_usb_events(dev);
		}

		if (evt & MUSB_HOST_EVT_EP0) {
			musb_ep0_advance(dev);
		}

		if (evt & MUSB_HOST_EVT_EPX) {
			uint16_t tx = (uint16_t)atomic_clear(&priv->intrtx_pending);
			uint16_t rx = (uint16_t)atomic_clear(&priv->intrrx_pending);

			for (uint8_t idx = 1; idx < config->num_of_eps; idx++) {
				if (rx & BIT(idx)) {
					musb_epx_in_done(dev, idx);
				}
				if (tx & BIT(idx)) {
					musb_epx_out_done(dev, idx);
				}
			}
		}

		if (evt & MUSB_HOST_EVT_XFR) {
			musb_schedule(dev);
		}

		uhc_unlock_internal(dev);
	}
}

/* UHC API */

static int uhc_musb_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_musb_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_musb_init(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_data *data = dev->data;
	int ret;

	/* The core registers are accessible once the vendor init completes. */
	ret = uhc_musb_quirk_init(dev);
	if (ret != 0) {
		LOG_ERR("Vendor init failed: %d", ret);
		return ret;
	}

	data->caps.hs = (config->speed_idx == MUSB_HOST_SPEED_IDX_HS) ? 1 : 0;

	return 0;
}

static int uhc_musb_enable(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t power;
	uint8_t devctl;
	int ret;

	/* Power VBUS on the downstream port if the board switches it. */
	if (config->vbus_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->vbus_gpio, 1);
		if (ret != 0) {
			LOG_ERR("Failed to enable VBUS: %d", ret);
			return ret;
		}
	}

	/*
	 * Write POWER without read-modify-write: SOFTCONN, SUSPENDMODE,
	 * ENABLESUSPENDM and ISOUPDATE must be cleared in host mode (SOFTCONN
	 * would connect the core as a peripheral). HSENABLE lets the core
	 * negotiate high speed during the bus reset.
	 */
	power = (config->speed_idx == MUSB_HOST_SPEED_IDX_HS) ? MUSB_POWER_HSENABLE_Msk : 0U;
	musb_write8(&base->power, power);

	/* Enable connect, disconnect and VBUS error interrupts, and EP0. */
	musb_write8(&base->intrusbe, MUSB_INTRUSBE_CONNEN_Msk | MUSB_INTRUSBE_DISCONEN_Msk |
				     MUSB_INTRUSBE_VBUSERREN_Msk);
	musb_write16(&base->intrtxe, MUSB_INTRTXE_EP0TXEN_Msk);

	ret = uhc_musb_quirk_post_enable(dev);
	if (ret != 0) {
		LOG_ERR("Vendor post enable failed: %d", ret);
		return ret;
	}

	/* The core only stays in host mode while VBUS is valid. */
	for (int i = 0; i < MUSB_HOST_VBUS_VALID_TIMEOUT_MS; i++) {
		if ((musb_read8(&base->devctl) & MUSB_DEVCTL_VBUS_Msk) ==
		    MUSB_DEVCTL_VBUS_AVBUSVALID) {
			break;
		}
		k_msleep(1);
	}

	devctl = musb_read8(&base->devctl);
	if ((devctl & MUSB_DEVCTL_VBUS_Msk) != MUSB_DEVCTL_VBUS_AVBUSVALID) {
		LOG_WRN("VBUS not valid (DEVCTL 0x%02x)", devctl);
	}

	/* Start a host session (enables the port and host framing). */
	musb_set8(&base->devctl, MUSB_DEVCTL_SESSION_Msk);

	config->irq_enable_func(dev);

	LOG_DBG("Host enabled: DEVCTL 0x%02x POWER 0x%02x",
		musb_read8(&base->devctl), musb_read8(&base->power));

	return 0;
}

static int uhc_musb_disable(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct usb_musb_reg *const base = musb_get_base(dev);
	int ret;

	config->irq_disable_func(dev);

	ret = uhc_musb_quirk_disable(dev);
	if (ret != 0) {
		LOG_ERR("Vendor disable failed: %d", ret);
		return ret;
	}

	musb_write8(&base->intrusbe, 0U);
	musb_write16(&base->intrtxe, 0U);

	/* End the host session and power VBUS down. */
	musb_clear8(&base->devctl, MUSB_DEVCTL_SESSION_Msk);

	if (config->vbus_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->vbus_gpio, 0);
		if (ret != 0) {
			LOG_ERR("Failed to disable VBUS: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int uhc_musb_shutdown(const struct device *dev)
{
	return uhc_musb_quirk_shutdown(dev);
}

static int uhc_musb_bus_reset(const struct device *dev)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);

	if (!atomic_test_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED)) {
		return -ENODEV;
	}

	musb_do_bus_reset(dev);
	priv->speed = musb_reset_speed(dev);

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);

	return 0;
}

static int uhc_musb_sof_enable(const struct device *dev)
{
	/*
	 * The core generates SOF automatically while a device is connected and
	 * the bus is not suspended.
	 */
	ARG_UNUSED(dev);

	return 0;
}

static int uhc_musb_bus_suspend(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	musb_set8(&base->power, MUSB_POWER_SUSPENDMODE_Msk);
	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	return 0;
}

static int uhc_musb_bus_resume(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t power;

	power = musb_read8(&base->power);
	power &= ~MUSB_POWER_SUSPENDMODE_Msk;
	power |= MUSB_POWER_RESUME_Msk;
	musb_write8(&base->power, power);
	k_msleep(MUSB_HOST_RESUME_MS);

	power = musb_read8(&base->power);
	musb_write8(&base->power, (uint8_t)(power & ~MUSB_POWER_RESUME_Msk));

	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);

	return 0;
}

static int uhc_musb_ep_enqueue(const struct device *dev,
			       struct uhc_transfer *const xfer)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	int ret;

	if (!atomic_test_bit(&priv->port_state, MUSB_HOST_PORT_CONNECTED)) {
		return -ENODEV;
	}

	/* A resubmitted transfer still carries the status of its last completion. */
	xfer->err = 0;

	ret = uhc_xfer_append(dev, xfer);
	if (ret) {
		return ret;
	}

	k_event_post(&priv->events, MUSB_HOST_EVT_XFR);

	return 0;
}

/*
 * Called with the driver lock held. The transfer is marked here and stopped
 * and returned with -ECONNRESET by the worker thread.
 */
static int uhc_musb_ep_dequeue(const struct device *dev,
			       struct uhc_transfer *const xfer)
{
	struct uhc_musb_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (xfer == tmp) {
			tmp->err = -ECONNRESET;
			k_event_post(&priv->events, MUSB_HOST_EVT_XFR);
			break;
		}
	}

	return 0;
}

/* Initialization */

static int uhc_musb_driver_preinit(const struct device *dev)
{
	const struct uhc_musb_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_musb_data *priv = data->priv;
	int ret;

	priv->speed = USB_SPEED_UNKNOWN;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	if (config->vbus_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->vbus_gpio)) {
			LOG_ERR("VBUS GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->vbus_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure VBUS GPIO: %d", ret);
			return ret;
		}
	}

	k_thread_create(&priv->thread_data, config->thread_stk,
			config->thread_stk_sz, uhc_musb_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(config->thread_priority), 0,
			K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	return 0;
}

static DEVICE_API(uhc, uhc_musb_api) = {
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
	.ep_enqueue = uhc_musb_ep_enqueue,
	.ep_dequeue = uhc_musb_ep_dequeue,
};

/*
 * A node with a single register region describes the MUSB core only. A vendor
 * wrapper in front of the core is described by the named regions "wrapper"
 * and "core".
 */
#define UHC_MUSB_DT_INST_CORE_ADDR(n)						\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),				\
		    (DT_INST_REG_ADDR(n)),					\
		    (DT_INST_REG_ADDR_BY_NAME(n, core)))

#define UHC_MUSB_DT_INST_WRAPPER_ADDR(n)					\
	COND_CODE_1(DT_INST_REG_HAS_NAME(n, wrapper),				\
		    (DT_INST_REG_ADDR_BY_NAME(n, wrapper)),			\
		    (0))

#define UHC_MUSB_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_musb_irq_enable_func_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_musb_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void uhc_musb_irq_disable_func_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		irq_disable(DT_INST_IRQN(n));					\
	}

#define UHC_MUSB_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(uhc_musb_stack_##n, CONFIG_UHC_MUSB_STACK_SIZE);	\
										\
	UHC_MUSB_IRQ_DT_INST_DEFINE(n)						\
										\
	BUILD_ASSERT(DT_INST_PROP(n, num_bidir_endpoints) <= MUSB_HOST_EP_MAX,	\
		     "MUSB supports at most 16 endpoints");			\
										\
	static const struct uhc_musb_config uhc_musb_config_##n = {		\
		.base = UHC_MUSB_DT_INST_CORE_ADDR(n),				\
		.wrapper = UHC_MUSB_DT_INST_WRAPPER_ADDR(n),			\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		\
		.thread_stk = uhc_musb_stack_##n,				\
		.thread_stk_sz = K_THREAD_STACK_SIZEOF(uhc_musb_stack_##n),	\
		.thread_priority = CONFIG_UHC_MUSB_THREAD_PRIORITY,		\
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),	\
		.vbus_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vbus_gpios, {0}),	\
		.irq_enable_func = uhc_musb_irq_enable_func_##n,		\
		.irq_disable_func = uhc_musb_irq_disable_func_##n,		\
		.quirks = UHC_MUSB_VENDOR_QUIRK_GET(n),				\
	};									\
										\
	static struct uhc_musb_data uhc_priv_##n;				\
										\
	static struct uhc_data uhc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),		\
		.priv = &uhc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_musb_driver_preinit, NULL,			\
			      &uhc_data_##n, &uhc_musb_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &uhc_musb_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_MUSB_DEVICE_DEFINE)
