/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"
#include "udc_dwc2.h"

#include <string.h>
#include <stdio.h>

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usb_ch9.h>
#include <usb_dwc2_hw.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
#include "udc_dwc2_vendor_quirks.h"

enum dwc2_drv_event_type {
	/* Trigger next transfer, must not be used for control OUT */
	DWC2_DRV_EVT_XFER,
	/* Setup packet received */
	DWC2_DRV_EVT_SETUP,
	/* Transaction on endpoint is finished */
	DWC2_DRV_EVT_EP_FINISHED,
	/* Core should exit hibernation due to bus reset */
	DWC2_DRV_EVT_HIBERNATION_EXIT_BUS_RESET,
	/* Core should exit hibernation due to host resume */
	DWC2_DRV_EVT_HIBERNATION_EXIT_HOST_RESUME,
};

/* Minimum RX FIFO size in 32-bit words considering the largest used OUT packet
 * of 512 bytes. The value must be adjusted according to the number of OUT
 * endpoints.
 */
#define UDC_DWC2_GRXFSIZ_FS_DEFAULT	(15U + 512U/4U)
/* Default Rx FIFO size in 32-bit words calculated to support High-Speed with:
 *   * 1 control endpoint in Completer/Buffer DMA mode: 13 locations
 *   * Global OUT NAK: 1 location
 *   * Space for 3 * 1024 packets: ((1024/4) + 1) * 3 = 774 locations
 * Driver adds 2 locations for each OUT endpoint to this value.
 */
#define UDC_DWC2_GRXFSIZ_HS_DEFAULT	(13 + 1 + 774)

/* TX FIFO0 depth in 32-bit words (used by control IN endpoint) */
#define UDC_DWC2_FIFO0_DEPTH		16U

/* Get Data FIFO access register */
#define UDC_DWC2_EP_FIFO(base, idx)	((mem_addr_t)base + 0x1000 * (idx + 1))

enum dwc2_suspend_type {
	DWC2_SUSPEND_NO_POWER_SAVING,
	DWC2_SUSPEND_HIBERNATION,
};

/* Registers that have to be stored before Partial Power Down or Hibernation */
struct dwc2_reg_backup {
	uint32_t gotgctl;
	uint32_t gahbcfg;
	uint32_t gusbcfg;
	uint32_t gintmsk;
	uint32_t grxfsiz;
	uint32_t gnptxfsiz;
	uint32_t gi2cctl;
	uint32_t glpmcfg;
	uint32_t gdfifocfg;
	union {
		uint32_t dptxfsiz[15];
		uint32_t dieptxf[15];
	};
	uint32_t dcfg;
	uint32_t dctl;
	uint32_t diepmsk;
	uint32_t doepmsk;
	uint32_t daintmsk;
	uint32_t diepctl[16];
	uint32_t dieptsiz[16];
	uint32_t diepdma[16];
	uint32_t doepctl[16];
	uint32_t doeptsiz[16];
	uint32_t doepdma[16];
	uint32_t pcgcctl;
};

/* Driver private data per instance */
struct udc_dwc2_data {
	struct k_thread thread_data;
	/* Main events the driver thread waits for */
	struct k_event drv_evt;
	/* Transfer triggers (OUT on bits 0-15, IN on bits 16-31) */
	struct k_event xfer_new;
	/* Finished transactions (OUT on bits 0-15, IN on bits 16-31) */
	struct k_event xfer_finished;
	struct dwc2_reg_backup backup;
	uint32_t ghwcfg1;
	uint32_t txf_set;
	uint32_t max_xfersize;
	uint32_t max_pktcnt;
	uint32_t tx_len[16];
	uint32_t rx_siz[16];
	uint16_t dfifodepth;
	uint16_t rxfifo_depth;
	uint16_t max_txfifo_depth[16];
	uint16_t sof_num;
	/* Configuration flags */
	unsigned int dynfifosizing : 1;
	unsigned int bufferdma : 1;
	/* Runtime state flags */
	unsigned int hibernated : 1;
	unsigned int enumdone : 1;
	unsigned int enumspd : 2;
	enum dwc2_suspend_type suspend_type;
	/* Number of endpoints including control endpoint */
	uint8_t numdeveps;
	/* Number of IN endpoints including control endpoint */
	uint8_t ineps;
	/* Number of OUT endpoints including control endpoint */
	uint8_t outeps;
	uint8_t setup[8];
};

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>

static int dwc2_init_pinctrl(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	int ret = 0;

	if (pcfg == NULL) {
		LOG_INF("Skip pinctrl configuration");
		return 0;
	}

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply default pinctrl state (%d)", ret);
	}

	LOG_DBG("Apply pinctrl");

	return ret;
}
#else
static int dwc2_init_pinctrl(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}
#endif

static inline struct usb_dwc2_reg *dwc2_get_base(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;

	return config->base;
}

static void dwc2_wait_for_bit(const struct device *dev,
			      mem_addr_t addr, uint32_t bit)
{
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(100));

	/* This could potentially be converted to use proper synchronization
	 * primitives instead of busy looping, but the number of interrupt bits
	 * this function can be waiting for is rather high.
	 *
	 * Busy looping is most likely fine unless profiling shows otherwise.
	 */
	while (!(sys_read32(addr) & bit)) {
		if (dwc2_quirk_is_phy_clk_off(dev)) {
			/* No point in waiting, because the bit can only be set
			 * when the PHY is actively clocked.
			 */
			return;
		}

		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("Timeout waiting for bit 0x%08X at 0x%08X",
				bit, (uint32_t)addr);
			return;
		}
	}
}

/* Get DOEPCTLn or DIEPCTLn register address */
static mem_addr_t dwc2_get_dxepctl_reg(const struct device *dev, const uint8_t ep)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep)) {
		return (mem_addr_t)&base->out_ep[ep_idx].doepctl;
	} else {
		return (mem_addr_t)&base->in_ep[ep_idx].diepctl;
	}
}

/* Get available FIFO space in bytes */
static uint32_t dwc2_ftx_avail(const struct device *dev, const uint32_t idx)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t reg = (mem_addr_t)&base->in_ep[idx].dtxfsts;
	uint32_t dtxfsts;

	dtxfsts = sys_read32(reg);

	return usb_dwc2_get_dtxfsts_ineptxfspcavail(dtxfsts) * 4;
}

static uint32_t dwc2_get_iept_pktctn(const struct device *dev, const uint32_t idx)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);

	if (idx == 0) {
		return usb_dwc2_get_dieptsiz0_pktcnt(UINT32_MAX);
	} else {
		return priv->max_pktcnt;
	}
}

static uint32_t dwc2_get_iept_xfersize(const struct device *dev, const uint32_t idx)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);

	if (idx == 0) {
		return usb_dwc2_get_dieptsiz0_xfersize(UINT32_MAX);
	} else {
		return priv->max_xfersize;
	}
}

static void dwc2_flush_rx_fifo(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;

	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, grstctl_reg);
	while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_RXFFLSH) {
	}
}

static void dwc2_flush_tx_fifo(const struct device *dev, const uint8_t fnum)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;
	uint32_t grstctl;

	grstctl = usb_dwc2_set_grstctl_txfnum(fnum) | USB_DWC2_GRSTCTL_TXFFLSH;

	sys_write32(grstctl, grstctl_reg);
	while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_TXFFLSH) {
	}
}

/* Return TX FIFOi depth in 32-bit words (i = f_idx + 1) */
static uint32_t dwc2_get_txfdep(const struct device *dev, const uint32_t f_idx)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint32_t dieptxf;

	dieptxf = sys_read32((mem_addr_t)&base->dieptxf[f_idx]);

	return usb_dwc2_get_dieptxf_inepntxfdep(dieptxf);
}

/* Return TX FIFOi address (i = f_idx + 1) */
static uint32_t dwc2_get_txfaddr(const struct device *dev, const uint32_t f_idx)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint32_t dieptxf;

	dieptxf = sys_read32((mem_addr_t)&base->dieptxf[f_idx]);

	return  usb_dwc2_get_dieptxf_inepntxfstaddr(dieptxf);
}

/* Set TX FIFOi address and depth (i = f_idx + 1) */
static void dwc2_set_txf(const struct device *dev, const uint32_t f_idx,
			 const uint32_t dep, const uint32_t addr)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint32_t dieptxf;

	dieptxf = usb_dwc2_set_dieptxf_inepntxfdep(dep) |
		  usb_dwc2_set_dieptxf_inepntxfstaddr(addr);

	sys_write32(dieptxf, (mem_addr_t)&base->dieptxf[f_idx]);
}

/* Enable/disable endpoint interrupt */
static void dwc2_set_epint(const struct device *dev,
			   struct udc_ep_config *const cfg, const bool enabled)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t reg = (mem_addr_t)&base->daintmsk;
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	uint32_t epmsk;

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		epmsk = USB_DWC2_DAINT_INEPINT(ep_idx);
	} else {
		epmsk = USB_DWC2_DAINT_OUTEPINT(ep_idx);
	}

	if (enabled) {
		sys_set_bits(reg, epmsk);
	} else {
		sys_clear_bits(reg, epmsk);
	}
}

static bool dwc2_ep_is_periodic(struct udc_ep_config *const cfg)
{
	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_INTERRUPT:
		__fallthrough;
	case USB_EP_TYPE_ISO:
		return true;
	default:
		return false;
	}
}

static bool dwc2_ep_is_iso(struct udc_ep_config *const cfg)
{
	return (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_ISO;
}

static bool dwc2_dma_buffer_ok_to_use(const struct device *dev, void *buf,
				      uint32_t xfersize, uint16_t mps)
{
	ARG_UNUSED(dev);

	if (!IS_ALIGNED(buf, 4)) {
		LOG_ERR("Buffer not aligned");
		return false;
	}

	/* We can only do 1 packet if Max Packet Size is not multiple of 4 */
	if (unlikely(mps % 4) && (xfersize > USB_MPS_EP_SIZE(mps))) {
		LOG_ERR("Padding not supported");
		return false;
	}

	return true;
}

/* Can be called from ISR context */
static int dwc2_tx_fifo_write(const struct device *dev,
			      struct udc_ep_config *const cfg, struct net_buf *const buf)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	mem_addr_t dieptsiz_reg = (mem_addr_t)&base->in_ep[ep_idx].dieptsiz;
	/* TODO: use dwc2_get_dxepctl_reg() */
	mem_addr_t diepctl_reg = (mem_addr_t)&base->in_ep[ep_idx].diepctl;
	mem_addr_t diepint_reg = (mem_addr_t)&base->in_ep[ep_idx].diepint;

	uint32_t diepctl;
	uint32_t max_xfersize, max_pktcnt, pktcnt;
	const uint32_t addnl = USB_MPS_ADDITIONAL_TRANSACTIONS(cfg->mps);
	const size_t d = sizeof(uint32_t);
	unsigned int key;
	uint32_t len;
	const bool is_periodic = dwc2_ep_is_periodic(cfg);
	const bool is_iso = dwc2_ep_is_iso(cfg);

	if (is_iso) {
		/* Isochronous transfers can only be programmed one
		 * (micro-)frame at a time.
		 */
		len = MIN(buf->len, USB_MPS_TO_TPL(cfg->mps));
	} else {
		/* DMA automatically handles packet split. In completer mode,
		 * the value is sanitized below.
		 */
		len = buf->len;
	}

	if (!priv->bufferdma) {
		uint32_t spcavail = dwc2_ftx_avail(dev, ep_idx);
		uint32_t spcperpkt = ROUND_UP(udc_mps_ep_size(cfg), 4);
		uint32_t max_pkts, max_transfer;

		/* Maximum number of packets that can fit in TxFIFO */
		max_pkts = spcavail / spcperpkt;

		/* We can transfer up to max_pkts MPS packets and a short one */
		max_transfer = (max_pkts * udc_mps_ep_size(cfg)) +
			       (spcavail % spcperpkt);

		/* If there is enough space for the transfer, there's no need
		 * to check any additional conditions. If the transfer is larger
		 * than TxFIFO then TxFIFO must be able to hold at least one
		 * packet (for periodic transfers at least the number of packets
		 * per microframe).
		 */
		if ((len > max_transfer) && ((1 + addnl) > max_pkts)) {
			LOG_ERR("ep 0x%02x FIFO space is too low, %u (%u)",
				cfg->addr, spcavail, len);
			return -EAGAIN;
		}

		len = MIN(len, max_transfer);
	}

	if (len != 0U) {
		max_pktcnt = dwc2_get_iept_pktctn(dev, ep_idx);
		max_xfersize = dwc2_get_iept_xfersize(dev, ep_idx);

		if (len > max_xfersize) {
			/*
			 * Avoid short packets if the transfer size cannot be
			 * handled in one set.
			 */
			len = ROUND_DOWN(max_xfersize, USB_MPS_TO_TPL(cfg->mps));
		}

		/*
		 * Determine the number of packets for the current transfer;
		 * if the pktcnt is too large, truncate the actual transfer length.
		 */
		pktcnt = DIV_ROUND_UP(len, udc_mps_ep_size(cfg));
		if (pktcnt > max_pktcnt) {
			pktcnt = ROUND_DOWN(max_pktcnt, (1 + addnl));
			len = pktcnt * udc_mps_ep_size(cfg);
		}
	} else {
		/* ZLP */
		pktcnt = 1U;
	}

	LOG_DBG("Prepare ep 0x%02x xfer len %u pktcnt %u addnl %u",
		cfg->addr, len, pktcnt, addnl);
	priv->tx_len[ep_idx] = len;

	/* Lock and write to endpoint FIFO */
	key = irq_lock();

	/* Set number of packets and transfer size */
	sys_write32((is_periodic ? usb_dwc2_set_dieptsizn_mc(1 + addnl) : 0) |
		    usb_dwc2_set_dieptsizn_pktcnt(pktcnt) |
		    usb_dwc2_set_dieptsizn_xfersize(len), dieptsiz_reg);

	if (priv->bufferdma) {
		if (!dwc2_dma_buffer_ok_to_use(dev, buf->data, len, cfg->mps)) {
			/* Cannot continue unless buffer is bounced. Device will
			 * cease to function. Is fatal error appropriate here?
			 */
			irq_unlock(key);
			return -ENOTSUP;
		}

		sys_write32((uint32_t)buf->data,
			    (mem_addr_t)&base->in_ep[ep_idx].diepdma);

		sys_cache_data_flush_range(buf->data, len);
	}

	diepctl = sys_read32(diepctl_reg);
	if (!(diepctl & USB_DWC2_DEPCTL_USBACTEP)) {
		/* Do not attempt to write data on inactive endpoint, because
		 * no fifo is assigned to inactive endpoint and therefore it is
		 * possible that the write will corrupt other endpoint fifo.
		 */
		irq_unlock(key);
		return -ENOENT;
	}

	if (is_iso) {
		/* Queue transfer on next SOF. TODO: allow stack to explicitly
		 * specify on which (micro-)frame the data should be sent.
		 */
		if (priv->sof_num & 1) {
			diepctl |= USB_DWC2_DEPCTL_SETEVENFR;
		} else {
			diepctl |= USB_DWC2_DEPCTL_SETODDFR;
		}
	}

	/* Clear NAK and set endpoint enable */
	diepctl |= USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_CNAK;
	sys_write32(diepctl, diepctl_reg);

	/* Clear IN Endpoint NAK Effective interrupt in case it was set */
	sys_write32(USB_DWC2_DIEPINT_INEPNAKEFF, diepint_reg);

	if (!priv->bufferdma) {
		const uint8_t *src = buf->data;

		while (pktcnt > 0) {
			uint32_t pktlen = MIN(len, udc_mps_ep_size(cfg));

			for (uint32_t i = 0UL; i < pktlen; i += d) {
				uint32_t val = src[i];

				if (i + 1 < pktlen) {
					val |= ((uint32_t)src[i + 1UL]) << 8;
				}
				if (i + 2 < pktlen) {
					val |= ((uint32_t)src[i + 2UL]) << 16;
				}
				if (i + 3 < pktlen) {
					val |= ((uint32_t)src[i + 3UL]) << 24;
				}

				sys_write32(val, UDC_DWC2_EP_FIFO(base, ep_idx));
			}

			pktcnt--;
			src += pktlen;
			len -= pktlen;
		}
	}

	irq_unlock(key);

	return 0;
}

static inline int dwc2_read_fifo(const struct device *dev, const uint8_t ep,
				 struct net_buf *const buf, const size_t size)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	size_t len = buf ? MIN(size, net_buf_tailroom(buf)) : 0;
	const size_t d = sizeof(uint32_t);

	/* FIFO access is always in 32-bit words */

	for (uint32_t n = 0; n < (len / d); n++) {
		net_buf_add_le32(buf, sys_read32(UDC_DWC2_EP_FIFO(base, ep)));
	}

	if (len % d) {
		uint8_t r[4];

		/* Get the remaining */
		sys_put_le32(sys_read32(UDC_DWC2_EP_FIFO(base, ep)), r);
		for (uint32_t i = 0U; i < (len % d); i++) {
			net_buf_add_u8(buf, r[i]);
		}
	}

	if (unlikely(size > len)) {
		for (uint32_t n = 0; n < DIV_ROUND_UP(size - len, d); n++) {
			(void)sys_read32(UDC_DWC2_EP_FIFO(base, ep));
		}
	}

	return 0;
}

/* Can be called from ISR and we call it only when there is a buffer in the queue */
static void dwc2_prep_rx(const struct device *dev, struct net_buf *buf,
			 struct udc_ep_config *const cfg)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	mem_addr_t doeptsiz_reg = (mem_addr_t)&base->out_ep[ep_idx].doeptsiz;
	mem_addr_t doepctl_reg = dwc2_get_dxepctl_reg(dev, ep_idx);
	uint32_t pktcnt;
	uint32_t doeptsiz;
	uint32_t doepctl;
	uint32_t xfersize;

	/* Clear NAK and set endpoint enable */
	doepctl = sys_read32(doepctl_reg);
	doepctl |= USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_CNAK;

	if (dwc2_ep_is_iso(cfg)) {
		xfersize = USB_MPS_TO_TPL(cfg->mps);
		pktcnt = 1 + USB_MPS_ADDITIONAL_TRANSACTIONS(cfg->mps);

		if (xfersize > net_buf_tailroom(buf)) {
			LOG_ERR("ISO RX buffer too small");
			return;
		}

		/* Set the Even/Odd (micro-)frame appropriately */
		if (priv->sof_num & 1) {
			doepctl |= USB_DWC2_DEPCTL_SETEVENFR;
		} else {
			doepctl |= USB_DWC2_DEPCTL_SETODDFR;
		}
	} else {
		xfersize = net_buf_tailroom(buf);

		/* Do as many packets in a single transfer as possible */
		if (xfersize > priv->max_xfersize) {
			xfersize = ROUND_DOWN(priv->max_xfersize, USB_MPS_TO_TPL(cfg->mps));
		}

		pktcnt = DIV_ROUND_UP(xfersize, USB_MPS_EP_SIZE(cfg->mps));
	}

	pktcnt = DIV_ROUND_UP(xfersize, udc_mps_ep_size(cfg));
	doeptsiz = usb_dwc2_set_doeptsizn_pktcnt(pktcnt) |
		   usb_dwc2_set_doeptsizn_xfersize(xfersize);
	if (cfg->addr == USB_CONTROL_EP_OUT) {
		/* Use 1 to allow 8 byte long buffers for SETUP data */
		doeptsiz |= (1 << USB_DWC2_DOEPTSIZ0_SUPCNT_POS);
	}

	priv->rx_siz[ep_idx] = doeptsiz;
	sys_write32(doeptsiz, doeptsiz_reg);

	if (priv->bufferdma) {
		if (!dwc2_dma_buffer_ok_to_use(dev, buf->data, xfersize, cfg->mps)) {
			/* Cannot continue unless buffer is bounced. Device will
			 * cease to function. Is fatal error appropriate here?
			 */
			return;
		}

		sys_write32((uint32_t)buf->data,
			    (mem_addr_t)&base->out_ep[ep_idx].doepdma);

		sys_cache_data_invd_range(buf->data, xfersize);
	}

	sys_write32(doepctl, doepctl_reg);

	LOG_INF("Prepare RX 0x%02x doeptsiz 0x%x", cfg->addr, doeptsiz);
}

static void dwc2_handle_xfer_next(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, cfg->addr);
	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		dwc2_prep_rx(dev, buf, cfg);
	} else {
		int err = dwc2_tx_fifo_write(dev, cfg, buf);

		if (err) {
			LOG_ERR("Failed to start write to TX FIFO, ep 0x%02x (err: %d)",
				cfg->addr, err);

			buf = udc_buf_get(dev, cfg->addr);
			if (udc_submit_ep_event(dev, buf, -ECONNREFUSED)) {
				LOG_ERR("Failed to submit endpoint event");
			};

			return;
		}
	}

	udc_ep_set_busy(dev, cfg->addr, true);
}

static int dwc2_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);
	dwc2_prep_rx(dev, buf, ep_cfg);
	LOG_DBG("feed buf %p", buf);

	return 0;
}

static int dwc2_handle_evt_setup(const struct device *dev)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
	if (buf == NULL) {
		LOG_ERR("No buffer queued for control ep");
		return -ENODATA;
	}

	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "setup");

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	/* We always allocate and feed buffer large enough for a setup packet. */

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);

		/* Allocate at least 8 bytes in case the host decides to send
		 * SETUP DATA instead of OUT DATA packet.
		 */
		err = dwc2_ctrl_feed_dout(dev, MAX(udc_data_stage_length(buf), 8));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s:%p|feed for -in-status", buf);

		err = dwc2_ctrl_feed_dout(dev, 8);
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}

		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p|feed >setup", buf);

		err = dwc2_ctrl_feed_dout(dev, 8);
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}

		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static inline int dwc2_handle_evt_dout(const struct device *dev,
				       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_get(dev, cfg->addr);
	if (buf == NULL) {
		LOG_ERR("No buffer queued for ep 0x%02x", cfg->addr);
		return -ENODATA;
	}

	udc_ep_set_busy(dev, cfg->addr, false);

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			/* s-in-status finished */
			LOG_DBG("dout:%p| status, feed >s", buf);

			/* Feed a buffer for the next setup packet */
			err = dwc2_ctrl_feed_dout(dev, 8);
			if (err == -ENOMEM) {
				err = udc_submit_ep_event(dev, buf, err);
			}

			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		} else {
			/*
			 * For all other cases we feed with a buffer
			 * large enough for setup packet.
			 */
			LOG_DBG("dout:%p| data, feed >s", buf);

			err = dwc2_ctrl_feed_dout(dev, 8);
			if (err == -ENOMEM) {
				err = udc_submit_ep_event(dev, buf, err);
			}
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			err = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
	}

	return err;
}

static int dwc2_handle_evt_din(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, cfg->addr);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", cfg->addr);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}

	if (buf->len) {
		/* Looks like we failed to continue in ISR, retry */
		return dwc2_tx_fifo_write(dev, cfg, buf);
	}

	if (cfg->addr == USB_CONTROL_EP_IN && udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		return dwc2_tx_fifo_write(dev, cfg, buf);
	}

	buf = udc_buf_get(dev, cfg->addr);
	udc_ep_set_busy(dev, cfg->addr, false);

	if (cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/*
			 * IN transfer finished, release buffer,
			 * control OUT buffer should be already fed.
			 */
			net_buf_unref(buf);
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static void dwc2_backup_registers(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct dwc2_reg_backup *backup = &priv->backup;

	backup->gotgctl = sys_read32((mem_addr_t)&base->gotgctl);
	backup->gahbcfg = sys_read32((mem_addr_t)&base->gahbcfg);
	backup->gusbcfg = sys_read32((mem_addr_t)&base->gusbcfg);
	backup->gintmsk = sys_read32((mem_addr_t)&base->gintmsk);
	backup->grxfsiz = sys_read32((mem_addr_t)&base->grxfsiz);
	backup->gnptxfsiz = sys_read32((mem_addr_t)&base->gnptxfsiz);
	backup->gi2cctl = sys_read32((mem_addr_t)&base->gi2cctl);
	backup->glpmcfg = sys_read32((mem_addr_t)&base->glpmcfg);
	backup->gdfifocfg = sys_read32((mem_addr_t)&base->gdfifocfg);

	for (uint8_t i = 1U; i < priv->ineps; i++) {
		backup->dieptxf[i - 1] = sys_read32((mem_addr_t)&base->dieptxf[i - 1]);
	}

	backup->dcfg = sys_read32((mem_addr_t)&base->dcfg);
	backup->dctl = sys_read32((mem_addr_t)&base->dctl);
	backup->diepmsk = sys_read32((mem_addr_t)&base->diepmsk);
	backup->doepmsk = sys_read32((mem_addr_t)&base->doepmsk);
	backup->daintmsk = sys_read32((mem_addr_t)&base->daintmsk);

	for (uint8_t i = 0U; i < 16; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_IN || epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			backup->diepctl[i] = sys_read32((mem_addr_t)&base->in_ep[i].diepctl);
			if (backup->diepctl[i] & USB_DWC2_DEPCTL_DPID) {
				backup->diepctl[i] |= USB_DWC2_DEPCTL_SETD1PID;
			} else {
				backup->diepctl[i] |= USB_DWC2_DEPCTL_SETD0PID;
			}
			backup->dieptsiz[i] = sys_read32((mem_addr_t)&base->in_ep[i].dieptsiz);
			backup->diepdma[i] = sys_read32((mem_addr_t)&base->in_ep[i].diepdma);
		}

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_OUT || epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			backup->doepctl[i] = sys_read32((mem_addr_t)&base->out_ep[i].doepctl);
			if (backup->doepctl[i] & USB_DWC2_DEPCTL_DPID) {
				backup->doepctl[i] |= USB_DWC2_DEPCTL_SETD1PID;
			} else {
				backup->doepctl[i] |= USB_DWC2_DEPCTL_SETD0PID;
			}
			backup->doeptsiz[i] = sys_read32((mem_addr_t)&base->out_ep[i].doeptsiz);
			backup->doepdma[i] = sys_read32((mem_addr_t)&base->out_ep[i].doepdma);
		}
	}

	backup->pcgcctl = sys_read32((mem_addr_t)&base->pcgcctl);
}

static void dwc2_restore_essential_registers(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct dwc2_reg_backup *backup = &priv->backup;
	uint32_t pcgcctl = backup->pcgcctl & USB_DWC2_PCGCCTL_RESTOREVALUE_MASK;

	sys_write32(backup->glpmcfg, (mem_addr_t)&base->glpmcfg);
	sys_write32(backup->gi2cctl, (mem_addr_t)&base->gi2cctl);
	sys_write32(pcgcctl, (mem_addr_t)&base->pcgcctl);

	sys_write32(backup->gahbcfg | USB_DWC2_GAHBCFG_GLBINTRMASK,
		    (mem_addr_t)&base->gahbcfg);

	sys_write32(0xFFFFFFFFUL, (mem_addr_t)&base->gintsts);
	sys_write32(USB_DWC2_GINTSTS_RSTRDONEINT, (mem_addr_t)&base->gintmsk);

	sys_write32(backup->gusbcfg, (mem_addr_t)&base->gusbcfg);
	sys_write32(backup->dcfg, (mem_addr_t)&base->dcfg);

	pcgcctl |= USB_DWC2_PCGCCTL_RESTOREMODE | USB_DWC2_PCGCCTL_RSTPDWNMODULE;
	sys_write32(pcgcctl, (mem_addr_t)&base->pcgcctl);
	k_busy_wait(1);

	pcgcctl |= USB_DWC2_PCGCCTL_ESSREGRESTORED;
	sys_write32(pcgcctl, (mem_addr_t)&base->pcgcctl);
}

static void dwc2_restore_device_registers(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct dwc2_reg_backup *backup = &priv->backup;

	sys_write32(backup->gotgctl, (mem_addr_t)&base->gotgctl);
	sys_write32(backup->gahbcfg, (mem_addr_t)&base->gahbcfg);
	sys_write32(backup->gusbcfg, (mem_addr_t)&base->gusbcfg);
	sys_write32(backup->gintmsk, (mem_addr_t)&base->gintmsk);
	sys_write32(backup->grxfsiz, (mem_addr_t)&base->grxfsiz);
	sys_write32(backup->gnptxfsiz, (mem_addr_t)&base->gnptxfsiz);
	sys_write32(backup->gdfifocfg, (mem_addr_t)&base->gdfifocfg);

	for (uint8_t i = 1U; i < priv->ineps; i++) {
		sys_write32(backup->dieptxf[i - 1], (mem_addr_t)&base->dieptxf[i - 1]);
	}

	sys_write32(backup->dctl, (mem_addr_t)&base->dctl);
	sys_write32(backup->diepmsk, (mem_addr_t)&base->diepmsk);
	sys_write32(backup->doepmsk, (mem_addr_t)&base->doepmsk);
	sys_write32(backup->daintmsk, (mem_addr_t)&base->daintmsk);

	for (uint8_t i = 0U; i < 16; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_IN || epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			sys_write32(backup->dieptsiz[i], (mem_addr_t)&base->in_ep[i].dieptsiz);
			sys_write32(backup->diepdma[i], (mem_addr_t)&base->in_ep[i].diepdma);
			sys_write32(backup->diepctl[i], (mem_addr_t)&base->in_ep[i].diepctl);
		}

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_OUT || epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			sys_write32(backup->doeptsiz[i], (mem_addr_t)&base->out_ep[i].doeptsiz);
			sys_write32(backup->doepdma[i], (mem_addr_t)&base->out_ep[i].doepdma);
			sys_write32(backup->doepctl[i], (mem_addr_t)&base->out_ep[i].doepctl);
		}
	}
}

static void dwc2_enter_hibernation(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t gpwrdn_reg = (mem_addr_t)&base->gpwrdn;
	mem_addr_t pcgcctl_reg = (mem_addr_t)&base->pcgcctl;

	dwc2_backup_registers(dev);

	/* This code currently only supports UTMI+. UTMI+ runs at either 30 or
	 * 60 MHz and therefore 1 us busy waits have sufficiently large margin.
	 */

	/* Enable PMU Logic */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PMUACTV);
	k_busy_wait(1);

	/* Stop PHY clock */
	sys_set_bits(pcgcctl_reg, USB_DWC2_PCGCCTL_STOPPCLK);
	k_busy_wait(1);

	/* Enable PMU interrupt */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PMUINTSEL);
	k_busy_wait(1);

	/* Unmask PMU interrupt bits */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_LINESTAGECHANGEMSK |
				 USB_DWC2_GPWRDN_RESETDETMSK |
				 USB_DWC2_GPWRDN_DISCONNECTDETECTMSK |
				 USB_DWC2_GPWRDN_STSCHNGINTMSK);
	k_busy_wait(1);

	/* Enable power clamps */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNCLMP);
	k_busy_wait(1);

	/* Switch off power to the controller */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNSWTCH);

	/* Mark that the core is hibernated */
	priv->hibernated = 1;
	LOG_DBG("Hibernated");
}

static void dwc2_exit_hibernation(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t gpwrdn_reg = (mem_addr_t)&base->gpwrdn;
	mem_addr_t pcgcctl_reg = (mem_addr_t)&base->pcgcctl;

	/* Switch on power to the controller */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNSWTCH);
	k_busy_wait(1);

	/* Reset the controller */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNRST_N);
	k_busy_wait(1);

	/* Enable restore from PMU */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_RESTORE);
	k_busy_wait(1);

	/* Disable power clamps */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNCLMP);

	/* Remove reset to the controller */
	sys_set_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PWRDNRST_N);
	k_busy_wait(1);

	/* Disable PMU interrupt */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PMUINTSEL);

	dwc2_restore_essential_registers(dev);

	/* Wait for Restore Done Interrupt */
	dwc2_wait_for_bit(dev, (mem_addr_t)&base->gintsts, USB_DWC2_GINTSTS_RSTRDONEINT);
	sys_write32(0xFFFFFFFFUL, (mem_addr_t)&base->gintsts);

	/* Disable restore from PMU */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_RESTORE);
	k_busy_wait(1);

	/* Clear reset to power down module */
	sys_clear_bits(pcgcctl_reg, USB_DWC2_PCGCCTL_RSTPDWNMODULE);

	/* Restore GUSBCFG, DCFG and DCTL */
	sys_write32(priv->backup.gusbcfg, (mem_addr_t)&base->gusbcfg);
	sys_write32(priv->backup.dcfg, (mem_addr_t)&base->dcfg);
	sys_write32(priv->backup.dctl, (mem_addr_t)&base->dctl);

	/* Disable PMU */
	sys_clear_bits(gpwrdn_reg, USB_DWC2_GPWRDN_PMUACTV);
	k_busy_wait(5);

	sys_set_bits((mem_addr_t)&base->dctl, USB_DWC2_DCTL_PWRONPRGDONE);
	k_msleep(1);
	sys_write32(0xFFFFFFFFUL, (mem_addr_t)&base->gintsts);

	dwc2_restore_device_registers(dev);

	priv->hibernated = 0;
	LOG_DBG("Hibernation exit complete");
}

static void dwc2_unset_unused_fifo(const struct device *dev)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct udc_ep_config *tmp;

	for (uint8_t i = priv->ineps - 1U; i > 0; i--) {
		tmp = udc_get_ep_cfg(dev, i | USB_EP_DIR_IN);

		if (tmp->stat.enabled && (priv->txf_set & BIT(i))) {
			return;
		}

		if (!tmp->stat.enabled && (priv->txf_set & BIT(i))) {
			priv->txf_set &= ~BIT(i);
		}
	}
}

/*
 * In dedicated FIFO mode there are i (i = 1 ... ineps - 1) FIFO size registers,
 * e.g. DIEPTXF1, DIEPTXF2, ... DIEPTXF4. When dynfifosizing is enabled,
 * the size register is mutable. The offset of DIEPTXF1 registers is 0.
 */
static int dwc2_set_dedicated_fifo(const struct device *dev,
				   struct udc_ep_config *const cfg,
				   uint32_t *const diepctl)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	const uint32_t addnl = USB_MPS_ADDITIONAL_TRANSACTIONS(cfg->mps);
	uint32_t reqdep;
	uint32_t txfaddr;
	uint32_t txfdep;
	uint32_t tmp;

	/* Keep everything but FIFO number */
	tmp = *diepctl & ~USB_DWC2_DEPCTL_TXFNUM_MASK;

	reqdep = DIV_ROUND_UP(udc_mps_ep_size(cfg), 4U);
	if (priv->bufferdma) {
		/* In DMA mode, TxFIFO capable of holding 2 packets is enough */
		reqdep *= MIN(2, (1 + addnl));
	} else {
		reqdep *= (1 + addnl);
	}

	if (priv->dynfifosizing) {
		if (priv->txf_set & ~BIT_MASK(ep_idx)) {
			dwc2_unset_unused_fifo(dev);
		}

		if (priv->txf_set & ~BIT_MASK(ep_idx)) {
			LOG_WRN("Some of the FIFOs higher than %u are set, %lx",
				ep_idx, priv->txf_set & ~BIT_MASK(ep_idx));
			return -EIO;
		}

		if ((ep_idx - 1) != 0U) {
			txfaddr = dwc2_get_txfdep(dev, ep_idx - 2) +
				  dwc2_get_txfaddr(dev, ep_idx - 2);
		} else {
			txfaddr = priv->rxfifo_depth +
				MAX(UDC_DWC2_FIFO0_DEPTH, priv->max_txfifo_depth[0]);
		}

		/* Make sure to not set TxFIFO greater than hardware allows */
		txfdep = reqdep;
		if (txfdep > priv->max_txfifo_depth[ep_idx]) {
			return -ENOMEM;
		}

		/* Do not allocate TxFIFO outside the SPRAM */
		if (txfaddr + txfdep > priv->dfifodepth) {
			return -ENOMEM;
		}

		/* Set FIFO depth (32-bit words) and address */
		dwc2_set_txf(dev, ep_idx - 1, txfdep, txfaddr);
	} else {
		txfdep = dwc2_get_txfdep(dev, ep_idx - 1);
		txfaddr = dwc2_get_txfaddr(dev, ep_idx - 1);

		if (reqdep > txfdep) {
			return -ENOMEM;
		}

		LOG_DBG("Reuse FIFO%u addr 0x%08x depth %u", ep_idx, txfaddr, txfdep);
	}

	/* Assign FIFO to the IN endpoint */
	*diepctl = tmp | usb_dwc2_set_depctl_txfnum(ep_idx);
	priv->txf_set |= BIT(ep_idx);
	dwc2_flush_tx_fifo(dev, ep_idx);

	LOG_INF("Set FIFO%u (ep 0x%02x) addr 0x%04x depth %u size %u",
		ep_idx, cfg->addr, txfaddr, txfdep, dwc2_ftx_avail(dev, ep_idx));

	return 0;
}

static int dwc2_ep_control_enable(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	mem_addr_t dxepctl0_reg;
	uint32_t dxepctl0;

	dxepctl0_reg = dwc2_get_dxepctl_reg(dev, cfg->addr);
	dxepctl0 = sys_read32(dxepctl0_reg);

	dxepctl0 &= ~USB_DWC2_DEPCTL0_MPS_MASK;
	switch (cfg->mps) {
	case 8:
		dxepctl0 |= USB_DWC2_DEPCTL0_MPS_8 << USB_DWC2_DEPCTL_MPS_POS;
		break;
	case 16:
		dxepctl0 |= USB_DWC2_DEPCTL0_MPS_16 << USB_DWC2_DEPCTL_MPS_POS;
		break;
	case 32:
		dxepctl0 |= USB_DWC2_DEPCTL0_MPS_32 << USB_DWC2_DEPCTL_MPS_POS;
		break;
	case 64:
		dxepctl0 |= USB_DWC2_DEPCTL0_MPS_64 << USB_DWC2_DEPCTL_MPS_POS;
		break;
	default:
		return -EINVAL;
	}

	dxepctl0 |= USB_DWC2_DEPCTL_USBACTEP;

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		int ret;

		dwc2_flush_rx_fifo(dev);
		ret = dwc2_ctrl_feed_dout(dev, 8);
		if (ret) {
			return ret;
		}
	} else {
		dwc2_flush_tx_fifo(dev, 0);
	}

	sys_write32(dxepctl0, dxepctl0_reg);
	dwc2_set_epint(dev, cfg, true);

	return 0;
}

static int udc_dwc2_ep_activate(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	mem_addr_t dxepctl_reg;
	uint32_t dxepctl;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	if (ep_idx == 0U) {
		return dwc2_ep_control_enable(dev, cfg);
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		/* TODO: use dwc2_get_dxepctl_reg() */
		dxepctl_reg = (mem_addr_t)&base->out_ep[ep_idx].doepctl;
	} else {
		if (priv->ineps > 0U && ep_idx > (priv->ineps - 1U)) {
			LOG_ERR("No resources available for ep 0x%02x", cfg->addr);
			return -EINVAL;
		}

		dxepctl_reg = (mem_addr_t)&base->in_ep[ep_idx].diepctl;
	}

	dxepctl = sys_read32(dxepctl_reg);
	/* Set max packet size */
	dxepctl &= ~USB_DWC2_DEPCTL_MPS_MASK;
	dxepctl |= usb_dwc2_set_depctl_mps(udc_mps_ep_size(cfg));

	/* Set endpoint type */
	dxepctl &= ~USB_DWC2_DEPCTL_EPTYPE_MASK;

	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_BULK:
		dxepctl |= USB_DWC2_DEPCTL_EPTYPE_BULK <<
			   USB_DWC2_DEPCTL_EPTYPE_POS;
		dxepctl |= USB_DWC2_DEPCTL_SETD0PID;
		break;
	case USB_EP_TYPE_INTERRUPT:
		dxepctl |= USB_DWC2_DEPCTL_EPTYPE_INTERRUPT <<
			   USB_DWC2_DEPCTL_EPTYPE_POS;
		dxepctl |= USB_DWC2_DEPCTL_SETD0PID;
		break;
	case USB_EP_TYPE_ISO:
		dxepctl |= USB_DWC2_DEPCTL_EPTYPE_ISO <<
			   USB_DWC2_DEPCTL_EPTYPE_POS;
		break;
	default:
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(cfg->addr) && udc_mps_ep_size(cfg) != 0U) {
		int ret = dwc2_set_dedicated_fifo(dev, cfg, &dxepctl);

		if (ret) {
			return ret;
		}
	}

	dxepctl |= USB_DWC2_DEPCTL_USBACTEP;

	/* Enable endpoint interrupts */
	dwc2_set_epint(dev, cfg, true);
	sys_write32(dxepctl, dxepctl_reg);

	for (uint8_t i = 1U; i < priv->ineps; i++) {
		LOG_DBG("DIEPTXF%u %08x DIEPCTL%u %08x",
			i, sys_read32((mem_addr_t)&base->dieptxf[i - 1U]), i, dxepctl);
	}

	return 0;
}

static int dwc2_unset_dedicated_fifo(const struct device *dev,
				     struct udc_ep_config *const cfg,
				     uint32_t *const diepctl)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	/* Clear FIFO number field */
	*diepctl &= ~USB_DWC2_DEPCTL_TXFNUM_MASK;

	if (priv->dynfifosizing) {
		if (priv->txf_set & ~BIT_MASK(ep_idx)) {
			LOG_WRN("Some of the FIFOs higher than %u are set, %lx",
				ep_idx, priv->txf_set & ~BIT_MASK(ep_idx));
			return 0;
		}

		dwc2_set_txf(dev, ep_idx - 1, 0, 0);
	}

	priv->txf_set &= ~BIT(ep_idx);

	return 0;
}

/* Disabled IN endpoint means that device will send NAK (isochronous: ZLP) after
 * receiving IN token from host even if there is packet available in TxFIFO.
 * Disabled OUT endpoint means that device will NAK (isochronous: discard data)
 * incoming OUT data (or HS PING) even if there is space available in RxFIFO.
 *
 * Set stall parameter to true if caller wants to send STALL instead of NAK.
 */
static void udc_dwc2_ep_disable(const struct device *dev,
				struct udc_ep_config *const cfg, bool stall)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	mem_addr_t dxepctl_reg;
	uint32_t dxepctl;

	dxepctl_reg = dwc2_get_dxepctl_reg(dev, cfg->addr);
	dxepctl = sys_read32(dxepctl_reg);

	if (dxepctl & USB_DWC2_DEPCTL_NAKSTS) {
		/* Endpoint already sends forced NAKs. STALL if necessary. */
		if (stall) {
			dxepctl |= USB_DWC2_DEPCTL_STALL;
			sys_write32(dxepctl, dxepctl_reg);
		}

		return;
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		mem_addr_t dctl_reg, gintsts_reg, doepint_reg;
		uint32_t dctl;

		dctl_reg = (mem_addr_t)&base->dctl;
		gintsts_reg = (mem_addr_t)&base->gintsts;
		doepint_reg = (mem_addr_t)&base->out_ep[ep_idx].doepint;

		dctl = sys_read32(dctl_reg);

		if (sys_read32(gintsts_reg) & USB_DWC2_GINTSTS_GOUTNAKEFF) {
			LOG_ERR("GOUTNAKEFF already active");
		} else {
			dctl |= USB_DWC2_DCTL_SGOUTNAK;
			sys_write32(dctl, dctl_reg);
			dctl &= ~USB_DWC2_DCTL_SGOUTNAK;
		}

		dwc2_wait_for_bit(dev, gintsts_reg, USB_DWC2_GINTSTS_GOUTNAKEFF);

		/* The application cannot disable control OUT endpoint 0. */
		if (ep_idx != 0) {
			dxepctl |= USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_EPDIS;
		}

		if (stall) {
			/* For OUT endpoints STALL is set instead of SNAK */
			dxepctl |= USB_DWC2_DEPCTL_STALL;
		} else {
			dxepctl |= USB_DWC2_DEPCTL_SNAK;
		}
		sys_write32(dxepctl, dxepctl_reg);

		if (ep_idx != 0) {
			dwc2_wait_for_bit(dev, doepint_reg, USB_DWC2_DOEPINT_EPDISBLD);
		}

		/* Clear Endpoint Disabled interrupt */
		sys_write32(USB_DWC2_DIEPINT_EPDISBLD, doepint_reg);

		dctl |= USB_DWC2_DCTL_CGOUTNAK;
		sys_write32(dctl, dctl_reg);
	} else {
		mem_addr_t diepint_reg;

		diepint_reg = (mem_addr_t)&base->in_ep[ep_idx].diepint;

		dxepctl |= USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_SNAK;
		if (stall) {
			/* For IN endpoints STALL is set in addition to SNAK */
			dxepctl |= USB_DWC2_DEPCTL_STALL;
		}
		sys_write32(dxepctl, dxepctl_reg);

		dwc2_wait_for_bit(dev, diepint_reg, USB_DWC2_DIEPINT_INEPNAKEFF);

		dxepctl |= USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_EPDIS;
		sys_write32(dxepctl, dxepctl_reg);

		dwc2_wait_for_bit(dev, diepint_reg, USB_DWC2_DIEPINT_EPDISBLD);

		/* Clear Endpoint Disabled interrupt */
		sys_write32(USB_DWC2_DIEPINT_EPDISBLD, diepint_reg);

		/* TODO: Read DIEPTSIZn here? Programming Guide suggest it to
		 * let application know how many bytes of interrupted transfer
		 * were transferred to the host.
		 */

		dwc2_flush_tx_fifo(dev, usb_dwc2_get_depctl_txfnum(dxepctl));
	}

	udc_ep_set_busy(dev, cfg->addr, false);
}

/* Deactivated endpoint means that there will be a bus timeout when the host
 * tries to access the endpoint.
 */
static int udc_dwc2_ep_deactivate(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	mem_addr_t dxepctl_reg;
	uint32_t dxepctl;

	dxepctl_reg = dwc2_get_dxepctl_reg(dev, cfg->addr);
	dxepctl = sys_read32(dxepctl_reg);

	if (dxepctl & USB_DWC2_DEPCTL_USBACTEP) {
		LOG_DBG("Disable ep 0x%02x DxEPCTL%u %x",
			cfg->addr, ep_idx, dxepctl);

		udc_dwc2_ep_disable(dev, cfg, false);

		dxepctl = sys_read32(dxepctl_reg);
		dxepctl &= ~USB_DWC2_DEPCTL_USBACTEP;
	} else {
		LOG_WRN("ep 0x%02x is not active DxEPCTL%u %x",
			cfg->addr, ep_idx, dxepctl);
	}

	if (USB_EP_DIR_IS_IN(cfg->addr) && udc_mps_ep_size(cfg) != 0U &&
	    ep_idx != 0U) {
		dwc2_unset_dedicated_fifo(dev, cfg, &dxepctl);
	}

	sys_write32(dxepctl, dxepctl_reg);
	dwc2_set_epint(dev, cfg, false);

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		struct net_buf *buf = udc_buf_get_all(dev, cfg->addr);

		/* Release the buffer allocated in dwc2_ctrl_feed_dout() */
		if (buf) {
			net_buf_unref(buf);
		}
	}

	return 0;
}

static int udc_dwc2_ep_set_halt(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	udc_dwc2_ep_disable(dev, cfg, true);

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);
	if (ep_idx != 0) {
		cfg->stat.halted = true;
	}

	return 0;
}

static int udc_dwc2_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t dxepctl_reg = dwc2_get_dxepctl_reg(dev, cfg->addr);
	uint32_t dxepctl;

	dxepctl = sys_read32(dxepctl_reg);
	dxepctl &= ~USB_DWC2_DEPCTL_STALL;
	dxepctl |= USB_DWC2_DEPCTL_SETD0PID;
	sys_write32(dxepctl, dxepctl_reg);

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);
	cfg->stat.halted = false;

	/* Resume queued transfers if any */
	if (udc_buf_peek(dev, cfg->addr)) {
		uint32_t ep_bit;

		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			ep_bit = BIT(16 + USB_EP_GET_IDX(cfg->addr));
		} else {
			ep_bit = BIT(USB_EP_GET_IDX(cfg->addr));
		}

		k_event_post(&priv->xfer_new, ep_bit);
		k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_XFER));
	}

	return 0;
}

static int udc_dwc2_ep_enqueue(const struct device *dev,
			       struct udc_ep_config *const cfg,
			       struct net_buf *const buf)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);

	LOG_DBG("%p enqueue %x %p", dev, cfg->addr, buf);
	udc_buf_put(cfg, buf);

	if (!cfg->stat.halted) {
		uint32_t ep_bit;

		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			ep_bit = BIT(16 + USB_EP_GET_IDX(cfg->addr));
		} else {
			ep_bit = BIT(USB_EP_GET_IDX(cfg->addr));
		}

		k_event_post(&priv->xfer_new, ep_bit);
		k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_XFER));
	}

	return 0;
}

static int udc_dwc2_ep_dequeue(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	udc_dwc2_ep_disable(dev, cfg, false);

	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, cfg->addr, false);

	LOG_DBG("dequeue ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_dwc2_set_address(const struct device *dev, const uint8_t addr)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t dcfg_reg = (mem_addr_t)&base->dcfg;
	uint32_t dcfg;

	if (addr > (USB_DWC2_DCFG_DEVADDR_MASK >> USB_DWC2_DCFG_DEVADDR_POS)) {
		return -EINVAL;
	}

	dcfg = sys_read32(dcfg_reg);
	dcfg &= ~USB_DWC2_DCFG_DEVADDR_MASK;
	dcfg |= usb_dwc2_set_dcfg_devaddr(addr);
	sys_write32(dcfg, dcfg_reg);
	LOG_DBG("Set new address %u for %p", addr, dev);

	return 0;
}

static int udc_dwc2_test_mode(const struct device *dev,
			      const uint8_t mode, const bool dryrun)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t dctl_reg = (mem_addr_t)&base->dctl;
	uint32_t dctl;

	if (mode == 0U || mode > USB_DWC2_DCTL_TSTCTL_TESTFE) {
		return -EINVAL;
	}

	dctl = sys_read32(dctl_reg);
	if (usb_dwc2_get_dctl_tstctl(dctl) != USB_DWC2_DCTL_TSTCTL_DISABLED) {
		return -EALREADY;
	}

	if (dryrun) {
		LOG_DBG("Test Mode %u supported", mode);
		return 0;
	}

	dctl |= usb_dwc2_set_dctl_tstctl(mode);
	sys_write32(dctl, dctl_reg);
	LOG_DBG("Enable Test Mode %u", mode);

	return 0;
}

static int udc_dwc2_host_wakeup(const struct device *dev)
{
	LOG_DBG("Remote wakeup from %p", dev);

	return -ENOTSUP;
}

/* Return actual USB device speed */
static enum udc_bus_speed udc_dwc2_device_speed(const struct device *dev)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);

	switch (priv->enumspd) {
	case USB_DWC2_DSTS_ENUMSPD_HS3060:
		return UDC_BUS_SPEED_HS;
	case USB_DWC2_DSTS_ENUMSPD_LS6:
		__ASSERT(false, "Low speed mode not supported");
		__fallthrough;
	case USB_DWC2_DSTS_ENUMSPD_FS48:
		__fallthrough;
	case USB_DWC2_DSTS_ENUMSPD_FS3060:
		__fallthrough;
	default:
		return UDC_BUS_SPEED_FS;
	}
}

static int dwc2_core_soft_reset(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;
	const unsigned int csr_timeout_us = 10000UL;
	uint32_t cnt = 0UL;

	/* Check AHB master idle state */
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -EIO;
		}
	}

	/* Apply Core Soft Reset */
	sys_write32(USB_DWC2_GRSTCTL_CSFTRST, grstctl_reg);

	cnt = 0UL;
	do {
		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -EIO;
		}

		k_busy_wait(1);
	} while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRST &&
		 !(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRSTDONE));

	sys_clear_bits(grstctl_reg, USB_DWC2_GRSTCTL_CSFTRST | USB_DWC2_GRSTCTL_CSFTRSTDONE);

	return 0;
}

static int udc_dwc2_init_controller(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	mem_addr_t grxfsiz_reg = (mem_addr_t)&base->grxfsiz;
	mem_addr_t gahbcfg_reg = (mem_addr_t)&base->gahbcfg;
	mem_addr_t gusbcfg_reg = (mem_addr_t)&base->gusbcfg;
	mem_addr_t dcfg_reg = (mem_addr_t)&base->dcfg;
	uint32_t dcfg;
	uint32_t gusbcfg;
	uint32_t gahbcfg;
	uint32_t ghwcfg2;
	uint32_t ghwcfg3;
	uint32_t ghwcfg4;
	uint32_t val;
	int ret;
	bool hs_phy;

	ret = dwc2_core_soft_reset(dev);
	if (ret) {
		return ret;
	}

	priv->ghwcfg1 = sys_read32((mem_addr_t)&base->ghwcfg1);
	ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	ghwcfg3 = sys_read32((mem_addr_t)&base->ghwcfg3);
	ghwcfg4 = sys_read32((mem_addr_t)&base->ghwcfg4);

	if (!(ghwcfg4 & USB_DWC2_GHWCFG4_DEDFIFOMODE)) {
		LOG_ERR("Only dedicated TX FIFO mode is supported");
		return -ENOTSUP;
	}

	/*
	 * Force device mode as we do no support role changes.
	 * Wait 25ms for the change to take effect.
	 */
	gusbcfg = USB_DWC2_GUSBCFG_FORCEDEVMODE;
	sys_write32(gusbcfg, gusbcfg_reg);
	k_msleep(25);

	/* Buffer DMA is always supported in Internal DMA mode.
	 * TODO: check and support descriptor DMA if available
	 */
	priv->bufferdma = (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) ==
			   USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA);

	if (!IS_ENABLED(CONFIG_UDC_DWC2_DMA)) {
		priv->bufferdma = 0;
	} else if (priv->bufferdma) {
		LOG_WRN("Experimental DMA enabled");
	}

	if (ghwcfg2 & USB_DWC2_GHWCFG2_DYNFIFOSIZING) {
		LOG_DBG("Dynamic FIFO Sizing is enabled");
		priv->dynfifosizing = true;
	}

	if (IS_ENABLED(CONFIG_UDC_DWC2_HIBERNATION) &&
	    ghwcfg4 & USB_DWC2_GHWCFG4_HIBERNATION) {
		LOG_INF("Hibernation enabled");
		priv->suspend_type = DWC2_SUSPEND_HIBERNATION;
	} else {
		priv->suspend_type = DWC2_SUSPEND_NO_POWER_SAVING;
	}

	/* Get the number or endpoints and IN endpoints we can use later */
	priv->numdeveps = usb_dwc2_get_ghwcfg2_numdeveps(ghwcfg2) + 1U;
	priv->ineps = usb_dwc2_get_ghwcfg4_ineps(ghwcfg4) + 1U;
	LOG_DBG("Number of endpoints (NUMDEVEPS + 1) %u", priv->numdeveps);
	LOG_DBG("Number of IN endpoints (INEPS + 1) %u", priv->ineps);

	LOG_DBG("Number of periodic IN endpoints (NUMDEVPERIOEPS) %u",
		usb_dwc2_get_ghwcfg4_numdevperioeps(ghwcfg4));
	LOG_DBG("Number of additional control endpoints (NUMCTLEPS) %u",
		usb_dwc2_get_ghwcfg4_numctleps(ghwcfg4));

	LOG_DBG("OTG architecture (OTGARCH) %u, mode (OTGMODE) %u",
		usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2),
		usb_dwc2_get_ghwcfg2_otgmode(ghwcfg2));

	priv->dfifodepth = usb_dwc2_get_ghwcfg3_dfifodepth(ghwcfg3);
	LOG_DBG("DFIFO depth (DFIFODEPTH) %u bytes", priv->dfifodepth * 4);

	priv->max_pktcnt = GHWCFG3_PKTCOUNT(usb_dwc2_get_ghwcfg3_pktsizewidth(ghwcfg3));
	priv->max_xfersize = GHWCFG3_XFERSIZE(usb_dwc2_get_ghwcfg3_xfersizewidth(ghwcfg3));
	LOG_DBG("Max packet count %u, Max transfer size %u",
		priv->max_pktcnt, priv->max_xfersize);

	LOG_DBG("Vendor Control interface support enabled: %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_VNDCTLSUPT) ? "true" : "false");

	LOG_DBG("PHY interface type: FSPHYTYPE %u, HSPHYTYPE %u, DATAWIDTH %u",
		usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4));

	LOG_DBG("LPM mode is %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_LPMMODE) ? "enabled" : "disabled");

	/* Configure AHB, select Completer or DMA mode */
	gahbcfg = sys_read32(gahbcfg_reg);

	if (priv->bufferdma) {
		gahbcfg |= USB_DWC2_GAHBCFG_DMAEN;
	} else {
		gahbcfg &= ~USB_DWC2_GAHBCFG_DMAEN;
	}

	sys_write32(gahbcfg, gahbcfg_reg);

	dcfg = sys_read32(dcfg_reg);

	dcfg &= ~USB_DWC2_DCFG_DESCDMA;

	/* Configure PHY and device speed */
	dcfg &= ~USB_DWC2_DCFG_DEVSPD_MASK;
	switch (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2)) {
	case USB_DWC2_GHWCFG2_HSPHYTYPE_UTMIPLUSULPI:
		__fallthrough;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI:
		gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB20 |
			   USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
		dcfg |= USB_DWC2_DCFG_DEVSPD_USBHS20
			<< USB_DWC2_DCFG_DEVSPD_POS;
		hs_phy = true;
		break;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_UTMIPLUS:
		gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB20 |
			   USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_UTMI;
		dcfg |= USB_DWC2_DCFG_DEVSPD_USBHS20
			<< USB_DWC2_DCFG_DEVSPD_POS;
		hs_phy = true;
		break;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS:
		__fallthrough;
	default:
		if (usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2) !=
		    USB_DWC2_GHWCFG2_FSPHYTYPE_NO_FS) {
			gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB11;
		}

		dcfg |= USB_DWC2_DCFG_DEVSPD_USBFS1148
			<< USB_DWC2_DCFG_DEVSPD_POS;
		hs_phy = false;
	}

	if (usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4)) {
		gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
	}

	/* Update PHY configuration */
	sys_write32(gusbcfg, gusbcfg_reg);
	sys_write32(dcfg, dcfg_reg);

	priv->outeps = 0U;
	for (uint8_t i = 0U; i < priv->numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_OUT ||
		    epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			mem_addr_t doepctl_reg = dwc2_get_dxepctl_reg(dev, i);

			sys_write32(USB_DWC2_DEPCTL_SNAK, doepctl_reg);
			priv->outeps++;
		}
	}

	LOG_DBG("Number of OUT endpoints %u", priv->outeps);

	/* Read and store all TxFIFO depths because Programmed FIFO Depths must
	 * not exceed the power-on values.
	 */
	val = sys_read32((mem_addr_t)&base->gnptxfsiz);
	priv->max_txfifo_depth[0] = usb_dwc2_get_gnptxfsiz_nptxfdep(val);
	for (uint8_t i = 1; i < priv->ineps; i++) {
		priv->max_txfifo_depth[i] = dwc2_get_txfdep(dev, i - 1);
	}

	priv->rxfifo_depth = usb_dwc2_get_grxfsiz(sys_read32(grxfsiz_reg));

	if (priv->dynfifosizing) {
		uint32_t gnptxfsiz;
		uint32_t default_depth;

		/* TODO: For proper runtime FIFO sizing UDC driver would have to
		 * have prior knowledge of the USB configurations. Only with the
		 * prior knowledge, the driver will be able to fairly distribute
		 * available resources. For the time being just use different
		 * defaults based on maximum configured PHY speed, but this has
		 * to be revised if e.g. thresholding support would be necessary
		 * on some target.
		 */
		if (hs_phy) {
			default_depth = UDC_DWC2_GRXFSIZ_HS_DEFAULT;
		} else {
			default_depth = UDC_DWC2_GRXFSIZ_FS_DEFAULT;
		}
		default_depth += priv->outeps * 2U;

		/* Driver does not dynamically resize RxFIFO so there is no need
		 * to store reset value. Read the reset value and make sure that
		 * the programmed value is not greater than what driver sets.
		 */
		priv->rxfifo_depth = MIN(priv->rxfifo_depth, default_depth);
		sys_write32(usb_dwc2_set_grxfsiz(priv->rxfifo_depth), grxfsiz_reg);

		/* Set TxFIFO 0 depth */
		val = MAX(UDC_DWC2_FIFO0_DEPTH, priv->max_txfifo_depth[0]);
		gnptxfsiz = usb_dwc2_set_gnptxfsiz_nptxfdep(val) |
			    usb_dwc2_set_gnptxfsiz_nptxfstaddr(priv->rxfifo_depth);

		sys_write32(gnptxfsiz, (mem_addr_t)&base->gnptxfsiz);
	}

	LOG_DBG("RX FIFO size %u bytes", priv->rxfifo_depth * 4);
	for (uint8_t i = 1U; i < priv->ineps; i++) {
		LOG_DBG("TX FIFO%u depth %u addr %u",
			i, priv->max_txfifo_depth[i], dwc2_get_txfaddr(dev, i));
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	/* Unmask interrupts */
	sys_write32(USB_DWC2_GINTSTS_OEPINT | USB_DWC2_GINTSTS_IEPINT |
		    USB_DWC2_GINTSTS_ENUMDONE | USB_DWC2_GINTSTS_USBRST |
		    USB_DWC2_GINTSTS_WKUPINT | USB_DWC2_GINTSTS_USBSUSP |
		    USB_DWC2_GINTSTS_INCOMPISOOUT | USB_DWC2_GINTSTS_INCOMPISOIN |
		    USB_DWC2_GINTSTS_SOF,
		    (mem_addr_t)&base->gintmsk);

	return 0;
}

static int udc_dwc2_enable(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	int err;

	err = dwc2_quirk_pre_enable(dev);
	if (err) {
		LOG_ERR("Quirk pre enable failed %d", err);
		return err;
	}

	err = udc_dwc2_init_controller(dev);
	if (err) {
		return err;
	}

	err = dwc2_quirk_post_enable(dev);
	if (err) {
		LOG_ERR("Quirk post enable failed %d", err);
		return err;
	}

	/* Enable global interrupt */
	sys_set_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
	config->irq_enable_func(dev);

	/* Disable soft disconnect */
	sys_clear_bits((mem_addr_t)&base->dctl, USB_DWC2_DCTL_SFTDISCON);
	LOG_DBG("Enable device %p", base);

	return 0;
}

static int udc_dwc2_disable(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t dctl_reg = (mem_addr_t)&base->dctl;
	int err;

	/* Enable soft disconnect */
	sys_set_bits(dctl_reg, USB_DWC2_DCTL_SFTDISCON);
	LOG_DBG("Disable device %p", dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_DBG("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_DBG("Failed to disable control endpoint");
		return -EIO;
	}

	config->irq_disable_func(dev);
	sys_clear_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	err = dwc2_quirk_disable(dev);
	if (err) {
		LOG_ERR("Quirk disable failed %d", err);
		return err;
	}

	return 0;
}

static int udc_dwc2_init(const struct device *dev)
{
	int ret;

	ret = dwc2_quirk_init(dev);
	if (ret) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	return dwc2_init_pinctrl(dev);
}

static int udc_dwc2_shutdown(const struct device *dev)
{
	int ret;

	ret = dwc2_quirk_shutdown(dev);
	if (ret) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

	return 0;
}

static int dwc2_driver_preinit(const struct device *dev)
{
	const struct udc_dwc2_config *config = dev->config;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	uint32_t numdeveps;
	uint32_t ineps;
	int err;

	k_mutex_init(&data->mutex);

	k_event_init(&priv->drv_evt);
	k_event_init(&priv->xfer_new);
	k_event_init(&priv->xfer_finished);

	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_64;

	(void)dwc2_quirk_caps(dev);
	if (data->caps.hs) {
		mps = 1024;
	}

	/*
	 * At this point, we cannot or do not want to access the hardware
	 * registers to get GHWCFGn values. For now, we will use devicetree to
	 * get GHWCFGn values and use them to determine the number and type of
	 * configured endpoints in the hardware. This can be considered a
	 * workaround, and we may change the upper layer internals to avoid it
	 * in the future.
	 */
	ineps = usb_dwc2_get_ghwcfg4_ineps(config->ghwcfg4) + 1U;
	numdeveps = usb_dwc2_get_ghwcfg2_numdeveps(config->ghwcfg2) + 1U;
	LOG_DBG("Number of endpoints (NUMDEVEPS + 1) %u", numdeveps);
	LOG_DBG("Number of IN endpoints (INEPS + 1) %u", ineps);

	for (uint32_t i = 0, n = 0; i < numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(config->ghwcfg1, i);

		if (epdir != USB_DWC2_GHWCFG1_EPDIR_OUT &&
		    epdir != USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			continue;
		}

		if (i == 0) {
			config->ep_cfg_out[n].caps.control = 1;
			config->ep_cfg_out[n].caps.mps = 64;
		} else {
			config->ep_cfg_out[n].caps.bulk = 1;
			config->ep_cfg_out[n].caps.interrupt = 1;
			config->ep_cfg_out[n].caps.iso = 1;
			config->ep_cfg_out[n].caps.high_bandwidth = data->caps.hs;
			config->ep_cfg_out[n].caps.mps = mps;
		}

		config->ep_cfg_out[n].caps.out = 1;
		config->ep_cfg_out[n].addr = USB_EP_DIR_OUT | i;

		LOG_DBG("Register ep 0x%02x (%u)", i, n);
		err = udc_register_ep(dev, &config->ep_cfg_out[n]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}

		n++;
		/* Also check the number of desired OUT endpoints in devicetree. */
		if (n >= config->num_out_eps) {
			break;
		}
	}

	for (uint32_t i = 0, n = 0; i < numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(config->ghwcfg1, i);

		if (epdir != USB_DWC2_GHWCFG1_EPDIR_IN &&
		    epdir != USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			continue;
		}

		if (i == 0) {
			config->ep_cfg_in[n].caps.control = 1;
			config->ep_cfg_in[n].caps.mps = 64;
		} else {
			config->ep_cfg_in[n].caps.bulk = 1;
			config->ep_cfg_in[n].caps.interrupt = 1;
			config->ep_cfg_in[n].caps.iso = 1;
			config->ep_cfg_in[n].caps.high_bandwidth = data->caps.hs;
			config->ep_cfg_in[n].caps.mps = mps;
		}

		config->ep_cfg_in[n].caps.in = 1;
		config->ep_cfg_in[n].addr = USB_EP_DIR_IN | i;

		LOG_DBG("Register ep 0x%02x (%u)", USB_EP_DIR_IN | i, n);
		err = udc_register_ep(dev, &config->ep_cfg_in[n]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}

		n++;
		/* Also check the number of desired IN endpoints in devicetree. */
		if (n >= MIN(ineps, config->num_in_eps)) {
			break;
		}
	}

	config->make_thread(dev);

	return 0;
}

static int udc_dwc2_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int udc_dwc2_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

static void dwc2_on_bus_reset(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint32_t doepmsk;

	/* Set the NAK bit for all OUT endpoints */
	for (uint8_t i = 0U; i < priv->numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);
		mem_addr_t doepctl_reg;

		LOG_DBG("ep 0x%02x EPDIR %u", i, epdir);
		if (epdir == USB_DWC2_GHWCFG1_EPDIR_OUT ||
		    epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			doepctl_reg = dwc2_get_dxepctl_reg(dev, i);
			sys_write32(USB_DWC2_DEPCTL_SNAK, doepctl_reg);
		}
	}

	doepmsk = USB_DWC2_DOEPINT_SETUP | USB_DWC2_DOEPINT_XFERCOMPL;
	if (priv->bufferdma) {
		doepmsk |= USB_DWC2_DOEPINT_STSPHSERCVD;
	}

	sys_write32(doepmsk, (mem_addr_t)&base->doepmsk);
	sys_set_bits((mem_addr_t)&base->diepmsk, USB_DWC2_DIEPINT_XFERCOMPL);

	/* Software has to handle RxFLvl interrupt only in Completer mode */
	if (!priv->bufferdma) {
		sys_set_bits((mem_addr_t)&base->gintmsk,
			     USB_DWC2_GINTSTS_RXFLVL);
	}

	/* Clear device address during reset. */
	sys_clear_bits((mem_addr_t)&base->dcfg, USB_DWC2_DCFG_DEVADDR_MASK);

	/* Speed enumeration must happen after reset. */
	priv->enumdone = 0;
}

static void dwc2_handle_enumdone(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	uint32_t dsts;

	dsts = sys_read32((mem_addr_t)&base->dsts);
	priv->enumspd = usb_dwc2_get_dsts_enumspd(dsts);
	priv->enumdone = 1;
}

static inline int dwc2_read_fifo_setup(const struct device *dev, uint8_t ep,
				       const size_t size)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	size_t offset;

	/* FIFO access is always in 32-bit words */

	if (size != 8) {
		LOG_ERR("%d bytes SETUP", size);
	}

	/*
	 * We store the setup packet temporarily in the driver's private data
	 * because there is always a race risk after the status stage OUT
	 * packet from the host and the new setup packet. This is fine in
	 * bottom-half processing because the events arrive in a queue and
	 * there will be a next net_buf for the setup packet.
	 */
	for (offset = 0; offset < MIN(size, 8); offset += 4) {
		sys_put_le32(sys_read32(UDC_DWC2_EP_FIFO(base, ep)),
			     &priv->setup[offset]);
	}

	/* On protocol error simply discard extra data */
	while (offset < size) {
		sys_read32(UDC_DWC2_EP_FIFO(base, ep));
		offset += 4;
	}

	return 0;
}

static inline void dwc2_handle_rxflvl(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint32_t grxstsp;
	uint32_t pktsts;
	uint32_t bcnt;
	uint8_t ep;

	grxstsp = sys_read32((mem_addr_t)&base->grxstsp);
	ep = usb_dwc2_get_grxstsp_epnum(grxstsp);
	bcnt = usb_dwc2_get_grxstsp_bcnt(grxstsp);
	pktsts = usb_dwc2_get_grxstsp_pktsts(grxstsp);

	LOG_DBG("ep 0x%02x: pktsts %u, bcnt %u", ep, pktsts, bcnt);

	switch (pktsts) {
	case USB_DWC2_GRXSTSR_PKTSTS_SETUP:
		dwc2_read_fifo_setup(dev, ep, bcnt);
		break;
	case USB_DWC2_GRXSTSR_PKTSTS_OUT_DATA:
		ep_cfg = udc_get_ep_cfg(dev, ep);

		buf = udc_buf_peek(dev, ep_cfg->addr);

		/* RxFIFO data must be retrieved even when buf is NULL */
		dwc2_read_fifo(dev, ep, buf, bcnt);
		break;
	case USB_DWC2_GRXSTSR_PKTSTS_OUT_DATA_DONE:
		LOG_DBG("RX pktsts DONE");
		break;
	case USB_DWC2_GRXSTSR_PKTSTS_SETUP_DONE:
		LOG_DBG("SETUP pktsts DONE");
	case USB_DWC2_GRXSTSR_PKTSTS_GLOBAL_OUT_NAK:
		LOG_DBG("Global OUT NAK");
		break;
	default:
		break;
	}
}

static inline void dwc2_handle_in_xfercompl(const struct device *dev,
					    const uint8_t ep_idx)
{
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	ep_cfg = udc_get_ep_cfg(dev, ep_idx | USB_EP_DIR_IN);
	buf = udc_buf_peek(dev, ep_cfg->addr);
	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	net_buf_pull(buf, priv->tx_len[ep_idx]);
	if (buf->len  && dwc2_tx_fifo_write(dev, ep_cfg, buf) == 0) {
		return;
	}

	k_event_post(&priv->xfer_finished, BIT(16 + ep_idx));
	k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_EP_FINISHED));
}

static inline void dwc2_handle_iepint(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	const uint8_t n_max = 16;
	uint32_t diepmsk;
	uint32_t daint;

	diepmsk = sys_read32((mem_addr_t)&base->diepmsk);
	daint = sys_read32((mem_addr_t)&base->daint);

	for (uint8_t n = 0U; n < n_max; n++) {
		mem_addr_t diepint_reg = (mem_addr_t)&base->in_ep[n].diepint;
		uint32_t diepint;
		uint32_t status;

		if (daint & USB_DWC2_DAINT_INEPINT(n)) {
			/* Read and clear interrupt status */
			diepint = sys_read32(diepint_reg);
			status = diepint & diepmsk;
			sys_write32(status, diepint_reg);

			LOG_DBG("ep 0x%02x interrupt status: 0x%x",
				n | USB_EP_DIR_IN, status);

			if (status & USB_DWC2_DIEPINT_XFERCOMPL) {
				dwc2_handle_in_xfercompl(dev, n);
			}

		}
	}

	/* Clear IEPINT interrupt */
	sys_write32(USB_DWC2_GINTSTS_IEPINT, (mem_addr_t)&base->gintsts);
}

static inline void dwc2_handle_out_xfercompl(const struct device *dev,
					     const uint8_t ep_idx)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_idx);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	uint32_t bcnt;
	struct net_buf *buf;
	uint32_t doeptsiz;
	const bool is_iso = dwc2_ep_is_iso(ep_cfg);

	doeptsiz = sys_read32((mem_addr_t)&base->out_ep[ep_idx].doeptsiz);

	buf = udc_buf_peek(dev, ep_cfg->addr);
	if (!buf) {
		LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	/* The original transfer size value is necessary here because controller
	 * decreases the value for every byte stored.
	 */
	bcnt = usb_dwc2_get_doeptsizn_xfersize(priv->rx_siz[ep_idx]) -
	       usb_dwc2_get_doeptsizn_xfersize(doeptsiz);

	if (is_iso) {
		uint32_t pkts;
		bool valid;

		pkts = usb_dwc2_get_doeptsizn_pktcnt(priv->rx_siz[ep_idx]) -
			usb_dwc2_get_doeptsizn_pktcnt(doeptsiz);
		switch (usb_dwc2_get_doeptsizn_rxdpid(doeptsiz)) {
		case USB_DWC2_DOEPTSIZN_RXDPID_DATA0:
			valid = (pkts == 1);
			break;
		case USB_DWC2_DOEPTSIZN_RXDPID_DATA1:
			valid = (pkts == 2);
			break;
		case USB_DWC2_DOEPTSIZN_RXDPID_DATA2:
			valid = (pkts == 3);
			break;
		case USB_DWC2_DOEPTSIZN_RXDPID_MDATA:
		default:
			valid = false;
			break;
		}

		if (!valid) {
			if (!priv->bufferdma) {
				/* RxFlvl added data to net buf, rollback */
				net_buf_remove_mem(buf, bcnt);
			}
			/* Data is not valid, discard it */
			bcnt = 0;
		}
	}

	if (priv->bufferdma && bcnt) {
		sys_cache_data_invd_range(buf->data, bcnt);
		net_buf_add(buf, bcnt);
	}

	if (!is_iso && (bcnt % udc_mps_ep_size(ep_cfg)) == 0 &&
	    net_buf_tailroom(buf)) {
		dwc2_prep_rx(dev, buf, ep_cfg);
	} else {
		k_event_post(&priv->xfer_finished, BIT(ep_idx));
		k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_EP_FINISHED));
	}
}

static inline void dwc2_handle_oepint(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	const uint8_t n_max = 16;
	uint32_t doepmsk;
	uint32_t daint;

	doepmsk = sys_read32((mem_addr_t)&base->doepmsk);
	daint = sys_read32((mem_addr_t)&base->daint);

	for (uint8_t n = 0U; n < n_max; n++) {
		mem_addr_t doepint_reg = (mem_addr_t)&base->out_ep[n].doepint;
		uint32_t doepint;
		uint32_t status;

		if (!(daint & USB_DWC2_DAINT_OUTEPINT(n))) {
			continue;
		}

		/* Read and clear interrupt status */
		doepint = sys_read32(doepint_reg);
		status = doepint & doepmsk;
		sys_write32(status, doepint_reg);

		LOG_DBG("ep 0x%02x interrupt status: 0x%x", n, status);

		/* StupPktRcvd is not enabled for interrupt, but must be checked
		 * when XferComp hits to determine if SETUP token was received.
		 */
		if (priv->bufferdma && (status & USB_DWC2_DOEPINT_XFERCOMPL) &&
		    (doepint & USB_DWC2_DOEPINT_STUPPKTRCVD)) {
			uint32_t addr;

			sys_write32(USB_DWC2_DOEPINT_STUPPKTRCVD, doepint_reg);
			status &= ~USB_DWC2_DOEPINT_XFERCOMPL;

			/* DMAAddr points past the memory location where the
			 * SETUP data was stored. Copy the received SETUP data
			 * to temporary location used also in Completer mode
			 * which allows common SETUP interrupt handling.
			 */
			addr = sys_read32((mem_addr_t)&base->out_ep[0].doepdma);
			sys_cache_data_invd_range((void *)(addr - 8), 8);
			memcpy(priv->setup, (void *)(addr - 8), sizeof(priv->setup));
		}

		if (status & USB_DWC2_DOEPINT_SETUP) {
			k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_SETUP));
		}

		if (status & USB_DWC2_DOEPINT_STSPHSERCVD) {
			/* Driver doesn't need any special handling, but it is
			 * mandatory that the bit is cleared in Buffer DMA mode.
			 * If the bit is not cleared (i.e. when this interrupt
			 * bit is masked), then SETUP interrupts will cease
			 * after first control transfer with data stage from
			 * device to host.
			 */
		}

		if (status & USB_DWC2_DOEPINT_XFERCOMPL) {
			dwc2_handle_out_xfercompl(dev, n);
		}
	}

	/* Clear OEPINT interrupt */
	sys_write32(USB_DWC2_GINTSTS_OEPINT, (mem_addr_t)&base->gintsts);
}

/* In DWC2 otg context incomplete isochronous IN transfer means that the host
 * did not issue IN token to at least one isochronous endpoint and software has
 * find on which endpoints the data is no longer valid and discard it.
 */
static void dwc2_handle_incompisoin(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t gintsts_reg = (mem_addr_t)&base->gintsts;
	const uint32_t mask =
		USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_EPTYPE_MASK |
		USB_DWC2_DEPCTL_USBACTEP;
	const uint32_t val =
		USB_DWC2_DEPCTL_EPENA |
		usb_dwc2_set_depctl_eptype(USB_DWC2_DEPCTL_EPTYPE_ISO) |
		USB_DWC2_DEPCTL_USBACTEP;

	for (uint8_t i = 1U; i < priv->numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_IN ||
		    epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			mem_addr_t diepctl_reg = dwc2_get_dxepctl_reg(dev, i | USB_EP_DIR_IN);
			uint32_t diepctl;

			diepctl = sys_read32(diepctl_reg);

			/* Check if endpoint didn't receive ISO OUT data */
			if ((diepctl & mask) == val) {
				struct udc_ep_config *cfg;
				struct net_buf *buf;

				cfg = udc_get_ep_cfg(dev, i | USB_EP_DIR_IN);
				__ASSERT_NO_MSG(cfg && cfg->stat.enabled &&
						dwc2_ep_is_iso(cfg));

				udc_dwc2_ep_disable(dev, cfg, false);

				buf = udc_buf_get(dev, cfg->addr);
				if (buf) {
					udc_submit_ep_event(dev, buf, 0);
				}
			}
		}
	}

	sys_write32(USB_DWC2_GINTSTS_INCOMPISOIN, gintsts_reg);
}

/* In DWC2 otg context incomplete isochronous OUT transfer means that the host
 * did not issue OUT token to at least one isochronous endpoint and software has
 * to find on which endpoint it didn't receive any data and let the stack know.
 */
static void dwc2_handle_incompisoout(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t gintsts_reg = (mem_addr_t)&base->gintsts;
	const uint32_t mask =
		USB_DWC2_DEPCTL_EPENA | USB_DWC2_DEPCTL_EPTYPE_MASK |
		USB_DWC2_DEPCTL_DPID | USB_DWC2_DEPCTL_USBACTEP;
	const uint32_t val =
		USB_DWC2_DEPCTL_EPENA |
		usb_dwc2_set_depctl_eptype(USB_DWC2_DEPCTL_EPTYPE_ISO) |
		((priv->sof_num & 1) ? USB_DWC2_DEPCTL_DPID : 0) |
		USB_DWC2_DEPCTL_USBACTEP;

	for (uint8_t i = 1U; i < priv->numdeveps; i++) {
		uint32_t epdir = usb_dwc2_get_ghwcfg1_epdir(priv->ghwcfg1, i);

		if (epdir == USB_DWC2_GHWCFG1_EPDIR_OUT ||
		    epdir == USB_DWC2_GHWCFG1_EPDIR_BDIR) {
			mem_addr_t doepctl_reg = dwc2_get_dxepctl_reg(dev, i);
			uint32_t doepctl;

			doepctl = sys_read32(doepctl_reg);

			/* Check if endpoint didn't receive ISO OUT data */
			if ((doepctl & mask) == val) {
				struct udc_ep_config *cfg;
				struct net_buf *buf;

				cfg = udc_get_ep_cfg(dev, i);
				__ASSERT_NO_MSG(cfg && cfg->stat.enabled &&
						dwc2_ep_is_iso(cfg));

				udc_dwc2_ep_disable(dev, cfg, false);

				buf = udc_buf_get(dev, cfg->addr);
				if (buf) {
					udc_submit_ep_event(dev, buf, 0);
				}
			}
		}
	}

	sys_write32(USB_DWC2_GINTSTS_INCOMPISOOUT, gintsts_reg);
}

static void udc_dwc2_isr_handler(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	mem_addr_t gintsts_reg = (mem_addr_t)&base->gintsts;
	uint32_t int_status;
	uint32_t gintmsk;

	if (priv->hibernated) {
		uint32_t gpwrdn = sys_read32((mem_addr_t)&base->gpwrdn);
		bool reset, resume = false;

		/* Clear interrupts */
		sys_write32(gpwrdn, (mem_addr_t)&base->gpwrdn);

		if (gpwrdn & USB_DWC2_GPWRDN_LNSTSCHNG) {
			resume = usb_dwc2_get_gpwrdn_linestate(gpwrdn) ==
				 USB_DWC2_GPWRDN_LINESTATE_DM1DP0;
		}

		reset = gpwrdn & USB_DWC2_GPWRDN_RESETDETECTED;

		if (resume) {
			k_event_post(&priv->drv_evt,
				     BIT(DWC2_DRV_EVT_HIBERNATION_EXIT_HOST_RESUME));
		}

		if (reset) {
			k_event_post(&priv->drv_evt, BIT(DWC2_DRV_EVT_HIBERNATION_EXIT_BUS_RESET));
		}

		(void)dwc2_quirk_irq_clear(dev);
		return;
	}

	gintmsk = sys_read32((mem_addr_t)&base->gintmsk);

	/*  Read and handle interrupt status register */
	while ((int_status = sys_read32(gintsts_reg) & gintmsk)) {

		LOG_DBG("GINTSTS 0x%x", int_status);

		if (int_status & USB_DWC2_GINTSTS_SOF) {
			uint32_t dsts;

			/* Clear USB SOF interrupt. */
			sys_write32(USB_DWC2_GINTSTS_SOF, gintsts_reg);

			dsts = sys_read32((mem_addr_t)&base->dsts);
			priv->sof_num = usb_dwc2_get_dsts_soffn(dsts);
			udc_submit_event(dev, UDC_EVT_SOF, 0);
		}

		if (int_status & USB_DWC2_GINTSTS_USBRST) {
			/* Clear and handle USB Reset interrupt. */
			sys_write32(USB_DWC2_GINTSTS_USBRST, gintsts_reg);
			dwc2_on_bus_reset(dev);
			LOG_DBG("USB Reset interrupt");
		}

		if (int_status & USB_DWC2_GINTSTS_ENUMDONE) {
			/* Clear and handle Enumeration Done interrupt. */
			sys_write32(USB_DWC2_GINTSTS_ENUMDONE, gintsts_reg);
			dwc2_handle_enumdone(dev);
			udc_submit_event(dev, UDC_EVT_RESET, 0);
		}

		if (int_status & USB_DWC2_GINTSTS_WKUPINT) {
			/* Clear Resume/Remote Wakeup Detected interrupt. */
			sys_write32(USB_DWC2_GINTSTS_WKUPINT, gintsts_reg);
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}

		if (int_status & USB_DWC2_GINTSTS_IEPINT) {
			/* Handle IN Endpoints interrupt */
			dwc2_handle_iepint(dev);
		}

		if (int_status & USB_DWC2_GINTSTS_RXFLVL) {
			/* Handle RxFIFO Non-Empty interrupt */
			dwc2_handle_rxflvl(dev);
		}

		if (int_status & USB_DWC2_GINTSTS_OEPINT) {
			/* Handle OUT Endpoints interrupt */
			dwc2_handle_oepint(dev);
		}

		if (int_status & USB_DWC2_GINTSTS_INCOMPISOIN) {
			dwc2_handle_incompisoin(dev);
		}

		if (int_status & USB_DWC2_GINTSTS_INCOMPISOOUT) {
			dwc2_handle_incompisoout(dev);
		}

		if (int_status & USB_DWC2_GINTSTS_USBSUSP) {
			if (!priv->enumdone) {
				/* Clear stale suspend interrupt */
				sys_write32(USB_DWC2_GINTSTS_USBSUSP, gintsts_reg);
				continue;
			}

			/* Notify the stack */
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);

			if (priv->suspend_type == DWC2_SUSPEND_HIBERNATION) {
				dwc2_enter_hibernation(dev);
				/* Next interrupt will be from PMU */
				break;
			}

			/* Clear USB Suspend interrupt. */
			sys_write32(USB_DWC2_GINTSTS_USBSUSP, gintsts_reg);
		}
	}

	(void)dwc2_quirk_irq_clear(dev);
}

static uint8_t pull_next_ep_from_bitmap(uint32_t *bitmap)
{
	unsigned int bit;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16) {
		return USB_EP_DIR_IN | (bit - 16);
	} else {
		return USB_EP_DIR_OUT | bit;
	}
}

static ALWAYS_INLINE void dwc2_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct udc_dwc2_data *const priv = udc_get_private(dev);
	const struct udc_dwc2_config *const config = dev->config;
	struct udc_ep_config *ep_cfg;
	const uint32_t hibernation_exit_events = (BIT(DWC2_DRV_EVT_HIBERNATION_EXIT_BUS_RESET) |
						  BIT(DWC2_DRV_EVT_HIBERNATION_EXIT_HOST_RESUME));
	uint32_t prev;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;

	/* This is the bottom-half of the ISR handler and the place where
	 * a new transfer can be fed.
	 */
	evt = k_event_wait(&priv->drv_evt, UINT32_MAX, false, K_FOREVER);

	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(DWC2_DRV_EVT_XFER)) {
		k_event_clear(&priv->drv_evt, BIT(DWC2_DRV_EVT_XFER));

		LOG_DBG("New transfer(s) in the queue");
		eps = k_event_test(&priv->xfer_new, UINT32_MAX);
		k_event_clear(&priv->xfer_new, eps);

		while (eps) {
			ep = pull_next_ep_from_bitmap(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);

			if (!udc_ep_is_busy(dev, ep_cfg->addr)) {
				dwc2_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_DBG("ep 0x%02x busy", ep_cfg->addr);
			}
		}
	}

	if (evt & BIT(DWC2_DRV_EVT_EP_FINISHED)) {
		k_event_clear(&priv->drv_evt, BIT(DWC2_DRV_EVT_EP_FINISHED));

		eps = k_event_test(&priv->xfer_finished, UINT32_MAX);
		k_event_clear(&priv->xfer_finished, eps);

		while (eps) {
			ep = pull_next_ep_from_bitmap(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);

			if (USB_EP_DIR_IS_IN(ep)) {
				LOG_DBG("DIN event ep 0x%02x", ep);
				dwc2_handle_evt_din(dev, ep_cfg);
			} else {
				LOG_DBG("DOUT event ep 0x%02x", ep_cfg->addr);
				dwc2_handle_evt_dout(dev, ep_cfg);
			}

			if (!udc_ep_is_busy(dev, ep_cfg->addr)) {
				dwc2_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_DBG("ep 0x%02x busy", ep_cfg->addr);
			}
		}
	}

	if (evt & BIT(DWC2_DRV_EVT_SETUP)) {
		k_event_clear(&priv->drv_evt, BIT(DWC2_DRV_EVT_SETUP));

		LOG_DBG("SETUP event");
		dwc2_handle_evt_setup(dev);
	}

	if (evt & hibernation_exit_events) {
		LOG_DBG("Hibernation exit event");
		config->irq_disable_func(dev);

		prev = k_event_clear(&priv->drv_evt, hibernation_exit_events);

		if (priv->hibernated) {
			dwc2_exit_hibernation(dev);

			/* Let stack know we are no longer suspended */
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);

			if (prev & BIT(DWC2_DRV_EVT_HIBERNATION_EXIT_BUS_RESET)) {
				dwc2_on_bus_reset(dev);
			}
		}

		config->irq_enable_func(dev);
	}

	udc_unlock_internal(dev);
}

static const struct udc_api udc_dwc2_api = {
	.lock = udc_dwc2_lock,
	.unlock = udc_dwc2_unlock,
	.device_speed = udc_dwc2_device_speed,
	.init = udc_dwc2_init,
	.enable = udc_dwc2_enable,
	.disable = udc_dwc2_disable,
	.shutdown = udc_dwc2_shutdown,
	.set_address = udc_dwc2_set_address,
	.test_mode = udc_dwc2_test_mode,
	.host_wakeup = udc_dwc2_host_wakeup,
	.ep_enable = udc_dwc2_ep_activate,
	.ep_disable = udc_dwc2_ep_deactivate,
	.ep_set_halt = udc_dwc2_ep_set_halt,
	.ep_clear_halt = udc_dwc2_ep_clear_halt,
	.ep_enqueue = udc_dwc2_ep_enqueue,
	.ep_dequeue = udc_dwc2_ep_dequeue,
};

#define DT_DRV_COMPAT snps_dwc2

#define UDC_DWC2_VENDOR_QUIRK_GET(n)						\
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),			\
		    (&dwc2_vendor_quirks_##n),					\
		    (NULL))

#define UDC_DWC2_DT_INST_REG_ADDR(n)						\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)), (DT_INST_REG_ADDR(n)),		\
		    (DT_INST_REG_ADDR_BY_NAME(n, core)))

#define UDC_DWC2_PINCTRL_DT_INST_DEFINE(n)					\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
		    (PINCTRL_DT_INST_DEFINE(n)), ())

#define UDC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
		    ((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define UDC_DWC2_IRQ_FLAGS_TYPE0(n)	0
#define UDC_DWC2_IRQ_FLAGS_TYPE1(n)	DT_INST_IRQ(n, type)
#define DW_IRQ_FLAGS(n) \
	_CONCAT(UDC_DWC2_IRQ_FLAGS_TYPE, DT_INST_IRQ_HAS_CELL(n, type))(n)

/*
 * A UDC driver should always be implemented as a multi-instance
 * driver, even if your platform does not require it.
 */
#define UDC_DWC2_DEVICE_DEFINE(n)						\
	UDC_DWC2_PINCTRL_DT_INST_DEFINE(n);					\
										\
	K_THREAD_STACK_DEFINE(udc_dwc2_stack_##n, CONFIG_UDC_DWC2_STACK_SIZE);	\
										\
	static void udc_dwc2_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		while (true) {							\
			dwc2_thread_handler(dev);				\
		}								\
	}									\
										\
	static void udc_dwc2_make_thread_##n(const struct device *dev)		\
	{									\
		struct udc_dwc2_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
				udc_dwc2_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_dwc2_stack_##n),	\
				udc_dwc2_thread_##n,				\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UDC_DWC2_THREAD_PRIORITY),	\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static void udc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    udc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    DW_IRQ_FLAGS(n));					\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void udc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_out_eps)];	\
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_in_eps)];	\
										\
	static const struct udc_dwc2_config udc_dwc2_config_##n = {		\
		.num_out_eps = DT_INST_PROP(n, num_out_eps),			\
		.num_in_eps = DT_INST_PROP(n, num_in_eps),			\
		.ep_cfg_in = ep_cfg_in,						\
		.ep_cfg_out = ep_cfg_out,					\
		.make_thread = udc_dwc2_make_thread_##n,			\
		.base = (struct usb_dwc2_reg *)UDC_DWC2_DT_INST_REG_ADDR(n),	\
		.pcfg = UDC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.irq_enable_func = udc_dwc2_irq_enable_func_##n,		\
		.irq_disable_func = udc_dwc2_irq_disable_func_##n,		\
		.quirks = UDC_DWC2_VENDOR_QUIRK_GET(n),				\
		.ghwcfg1 = DT_INST_PROP(n, ghwcfg1),				\
		.ghwcfg2 = DT_INST_PROP(n, ghwcfg2),				\
		.ghwcfg4 = DT_INST_PROP(n, ghwcfg4),				\
	};									\
										\
	static struct udc_dwc2_data udc_priv_##n = {				\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, dwc2_driver_preinit, NULL,			\
			      &udc_data_##n, &udc_dwc2_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &udc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_DWC2_DEVICE_DEFINE)
