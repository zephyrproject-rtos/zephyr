/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB DesignWare device controller driver
 *
 * USB DesignWare device controller driver. The driver implements the low
 * level control routines to deal directly with the hardware.
 */

#define DT_DRV_COMPAT snps_designware_usb

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>

#include "usb_dw_registers.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_dw, CONFIG_USB_DRIVER_LOG_LEVEL);

/* FIXME: The actual number of endpoints should be obtained from GHWCFG4. */
enum usb_dw_in_ep_idx {
	USB_DW_IN_EP_0 = 0,
	USB_DW_IN_EP_1,
	USB_DW_IN_EP_2,
	USB_DW_IN_EP_3,
	USB_DW_IN_EP_4,
	USB_DW_IN_EP_5,
	USB_DW_IN_EP_NUM
};

/* FIXME: The actual number of endpoints should be obtained from GHWCFG2. */
enum usb_dw_out_ep_idx {
	USB_DW_OUT_EP_0 = 0,
	USB_DW_OUT_EP_1,
	USB_DW_OUT_EP_2,
	USB_DW_OUT_EP_3,
	USB_DW_OUT_EP_NUM
};

#define USB_DW_CORE_RST_TIMEOUT_US	10000

/* FIXME: The actual MPS depends on endpoint type and bus speed. */
#define DW_USB_MAX_PACKET_SIZE		64

/* Number of SETUP back-to-back packets */
#define USB_DW_SUP_CNT			1

/* Get Data FIFO access register */
#define USB_DW_EP_FIFO(base, idx)	\
	(*(uint32_t *)(POINTER_TO_UINT(base) + 0x1000 * (idx + 1)))

struct usb_dw_config {
	struct usb_dw_reg *const base;
};

/*
 * USB endpoint private structure.
 */
struct usb_ep_ctrl_prv {
	uint8_t ep_ena;
	uint8_t fifo_num;
	uint32_t fifo_size;
	uint16_t mps;         /* Max ep pkt size */
	usb_dc_ep_callback cb;/* Endpoint callback function */
	uint32_t data_len;
};

/*
 * USB controller private structure.
 */
struct usb_dw_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USB_DW_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USB_DW_OUT_EP_NUM];
	int n_tx_fifos;
	uint8_t attached;
};


#define USB_DW_DEVICE_DEFINE(n)							\
	static const struct usb_dw_config usb_dw_cfg_##n = {			\
		.base = (struct usb_dw_reg *)DT_INST_REG_ADDR(n),		\
	};									\
										\
	static struct usb_dw_ctrl_prv usb_dw_ctrl_##n;

USB_DW_DEVICE_DEFINE(0)

#define usb_dw_ctrl usb_dw_ctrl_0
#define usb_dw_cfg usb_dw_cfg_0

static void usb_dw_reg_dump(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t i;

	LOG_DBG("USB registers:  GOTGCTL : 0x%x  GOTGINT : 0x%x  GAHBCFG : "
		"0x%x", base->gotgctl, base->gotgint, base->gahbcfg);
	LOG_DBG("  GUSBCFG : 0x%x  GINTSTS : 0x%x  GINTMSK : 0x%x",
		base->gusbcfg, base->gintsts, base->gintmsk);
	LOG_DBG("  DCFG    : 0x%x  DCTL    : 0x%x  DSTS    : 0x%x",
		base->dcfg, base->dctl, base->dsts);
	LOG_DBG("  DIEPMSK : 0x%x  DOEPMSK : 0x%x  DAINT   : 0x%x",
		base->diepmsk, base->doepmsk, base->daint);
	LOG_DBG("  DAINTMSK: 0x%x  GHWCFG1 : 0x%x  GHWCFG2 : 0x%x",
		base->daintmsk, base->ghwcfg1, base->ghwcfg2);
	LOG_DBG("  GHWCFG3 : 0x%x  GHWCFG4 : 0x%x",
		base->ghwcfg3, base->ghwcfg4);

	for (i = 0U; i < USB_DW_OUT_EP_NUM; i++) {
		LOG_DBG("\n  EP %d registers:    DIEPCTL : 0x%x    DIEPINT : "
			"0x%x", i, base->in_ep_reg[i].diepctl,
			base->in_ep_reg[i].diepint);
		LOG_DBG("    DIEPTSIZ: 0x%x    DIEPDMA : 0x%x    DOEPCTL : "
			"0x%x", base->in_ep_reg[i].dieptsiz,
			base->in_ep_reg[i].diepdma,
			base->out_ep_reg[i].doepctl);
		LOG_DBG("    DOEPINT : 0x%x    DOEPTSIZ: 0x%x    DOEPDMA : "
			"0x%x", base->out_ep_reg[i].doepint,
			base->out_ep_reg[i].doeptsiz,
			base->out_ep_reg[i].doepdma);
	}
}

static uint8_t usb_dw_ep_is_valid(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	/* Check if ep enabled */
	if ((USB_EP_DIR_IS_OUT(ep)) && ep_idx < USB_DW_OUT_EP_NUM) {
		return 1;
	} else if ((USB_EP_DIR_IS_IN(ep)) && ep_idx < USB_DW_IN_EP_NUM) {
		return 1;
	}

	return 0;
}

static uint8_t usb_dw_ep_is_enabled(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	/* Check if ep enabled */
	if ((USB_EP_DIR_IS_OUT(ep)) &&
	    usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	} else if ((USB_EP_DIR_IS_IN(ep)) &&
		   usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	}

	return 0;
}

static inline void usb_dw_udelay(uint32_t us)
{
	k_busy_wait(us);
}

static int usb_dw_reset(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t cnt = 0U;

	/* Wait for AHB master idle state. */
	while (!(base->grstctl & USB_DW_GRSTCTL_AHB_IDLE)) {
		usb_dw_udelay(1);

		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			LOG_ERR("USB reset HANG! AHB Idle GRSTCTL=0x%08x",
				base->grstctl);
			return -EIO;
		}
	}

	/* Core Soft Reset */
	cnt = 0U;
	base->grstctl |= USB_DW_GRSTCTL_C_SFT_RST;

	do {
		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			LOG_DBG("USB reset HANG! Soft Reset GRSTCTL=0x%08x",
				base->grstctl);
			return -EIO;
		}
		usb_dw_udelay(1);
	} while (base->grstctl & USB_DW_GRSTCTL_C_SFT_RST);

	/* Wait for 3 PHY Clocks */
	usb_dw_udelay(100);

	return 0;
}

static int usb_dw_num_dev_eps(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;

	return (base->ghwcfg2 >> 10) & 0xf;
}

static void usb_dw_flush_tx_fifo(int ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	int fnum = usb_dw_ctrl.in_ep_ctrl[ep].fifo_num;

	base->grstctl = (fnum << 6) | (1<<5);
	while (base->grstctl & (1<<5)) {
	}
}

static int usb_dw_tx_fifo_avail(int ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;

	return base->in_ep_reg[ep].dtxfsts & USB_DW_DTXFSTS_TXF_SPC_AVAIL_MASK;
}

/* Choose a FIFO number for an IN endpoint */
static int usb_dw_set_fifo(uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	int ep_idx = USB_EP_GET_IDX(ep);
	volatile uint32_t *reg = &base->in_ep_reg[ep_idx].diepctl;
	uint32_t val;
	int fifo = 0;
	int ded_fifo = !!(base->ghwcfg4 & USB_DW_HWCFG4_DEDFIFOMODE);

	if (!ded_fifo) {
		/* No support for shared-FIFO mode yet, existing
		 * Zephyr hardware doesn't use it
		 */
		return -ENOTSUP;
	}

	/* In dedicated-FIFO mode, all IN endpoints must have a unique
	 * FIFO number associated with them in the TXFNUM field of
	 * DIEPCTLx, with EP0 always being assigned to FIFO zero (the
	 * reset default, so we don't touch it).
	 *
	 * FIXME: would be better (c.f. the dwc2 driver in Linux) to
	 * choose a FIFO based on the hardware depth: we want the
	 * smallest one that fits our configured maximum packet size
	 * for the endpoint.  This just picks the next available one.
	 */
	if (ep_idx != 0) {
		fifo = ++usb_dw_ctrl.n_tx_fifos;
		if (fifo >= usb_dw_num_dev_eps()) {
			return -EINVAL;
		}

		reg = &base->in_ep_reg[ep_idx].diepctl;
		val = *reg & ~USB_DW_DEPCTL_TXFNUM_MASK;
		val |= fifo << USB_DW_DEPCTL_TXFNUM_OFFSET;
		*reg = val;
	}

	usb_dw_ctrl.in_ep_ctrl[ep_idx].fifo_num = fifo;

	usb_dw_flush_tx_fifo(ep_idx);

	val = usb_dw_tx_fifo_avail(ep_idx);
	usb_dw_ctrl.in_ep_ctrl[ep_idx].fifo_size = val;

	return 0;
}

static int usb_dw_ep_set(uint8_t ep,
			 uint32_t ep_mps, enum usb_dc_ep_transfer_type ep_type)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	volatile uint32_t *p_depctl;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("%s ep %x, mps %d, type %d", __func__, ep, ep_mps, ep_type);

	if (USB_EP_DIR_IS_OUT(ep)) {
		p_depctl = &base->out_ep_reg[ep_idx].doepctl;
		usb_dw_ctrl.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		p_depctl = &base->in_ep_reg[ep_idx].diepctl;
		usb_dw_ctrl.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	if (!ep_idx) {
		/* Set max packet size for EP0 */
		*p_depctl &= ~USB_DW_DEPCTL0_MSP_MASK;

		switch (ep_mps) {
		case 8:
			*p_depctl |= USB_DW_DEPCTL0_MSP_8 <<
				USB_DW_DEPCTL_MSP_OFFSET;
			break;
		case 16:
			*p_depctl |= USB_DW_DEPCTL0_MSP_16 <<
				USB_DW_DEPCTL_MSP_OFFSET;
			break;
		case 32:
			*p_depctl |= USB_DW_DEPCTL0_MSP_32 <<
				USB_DW_DEPCTL_MSP_OFFSET;
			break;
		case 64:
			*p_depctl |= USB_DW_DEPCTL0_MSP_64 <<
				USB_DW_DEPCTL_MSP_OFFSET;
			break;
		default:
			return -EINVAL;
		}
		/* No need to set EP0 type */
	} else {
		/* Set max packet size for EP */
		if (ep_mps > (USB_DW_DEPCTLn_MSP_MASK >>
		    USB_DW_DEPCTL_MSP_OFFSET)) {
			return -EINVAL;
		}

		*p_depctl &= ~USB_DW_DEPCTLn_MSP_MASK;
		*p_depctl |= ep_mps << USB_DW_DEPCTL_MSP_OFFSET;

		/* Set endpoint type */
		*p_depctl &= ~USB_DW_DEPCTL_EP_TYPE_MASK;

		switch (ep_type) {
		case USB_DC_EP_CONTROL:
			*p_depctl |= USB_DW_DEPCTL_EP_TYPE_CONTROL <<
				USB_DW_DEPCTL_EP_TYPE_OFFSET;
			break;
		case USB_DC_EP_BULK:
			*p_depctl |= USB_DW_DEPCTL_EP_TYPE_BULK <<
				USB_DW_DEPCTL_EP_TYPE_OFFSET;
			break;
		case USB_DC_EP_INTERRUPT:
			*p_depctl |= USB_DW_DEPCTL_EP_TYPE_INTERRUPT <<
				USB_DW_DEPCTL_EP_TYPE_OFFSET;
			break;
		default:
			return -EINVAL;
		}

		/* sets the Endpoint Data PID to DATA0 */
		*p_depctl |= USB_DW_DEPCTL_SETDOPID;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		int ret = usb_dw_set_fifo(ep);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void usb_dw_prep_rx(const uint8_t ep, uint8_t setup)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	enum usb_dw_out_ep_idx ep_idx = USB_EP_GET_IDX(ep);
	uint32_t ep_mps = usb_dw_ctrl.out_ep_ctrl[ep_idx].mps;

	/* Set max RX size to EP mps so we get an interrupt
	 * each time a packet is received
	 */

	base->out_ep_reg[ep_idx].doeptsiz =
		(USB_DW_SUP_CNT << USB_DW_DOEPTSIZ_SUP_CNT_OFFSET) |
		(1 << USB_DW_DEPTSIZ_PKT_CNT_OFFSET) | ep_mps;

	/* Clear NAK and enable ep */
	if (!setup) {
		base->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_CNAK;
	}

	base->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_EP_ENA;

	LOG_DBG("USB OUT EP%d armed", ep_idx);
}

static int usb_dw_tx(uint8_t ep, const uint8_t *const data,
		uint32_t data_len)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	enum usb_dw_in_ep_idx ep_idx = USB_EP_GET_IDX(ep);
	uint32_t max_xfer_size, max_pkt_cnt, pkt_cnt, avail_space;
	uint32_t ep_mps = usb_dw_ctrl.in_ep_ctrl[ep_idx].mps;
	unsigned int key;
	uint32_t i;

	/* Wait for FIFO space available */
	do {
		avail_space = usb_dw_tx_fifo_avail(ep_idx);
		if (avail_space == usb_dw_ctrl.in_ep_ctrl[ep_idx].fifo_size) {
			break;
		}
		/* Make sure we don't hog the CPU */
		k_yield();
	} while (1);

	key = irq_lock();

	avail_space *= 4U;
	if (!avail_space) {
		LOG_ERR("USB IN EP%d no space available, DTXFSTS %x", ep_idx,
			base->in_ep_reg[ep_idx].dtxfsts);
		irq_unlock(key);
		return -EAGAIN;
	}

	/* For now tx-fifo sizes are not configured (cf usb_dw_set_fifo). Here
	 * we force available fifo size to be a multiple of ep mps in order to
	 * prevent splitting data incorrectly.
	 */
	avail_space -= avail_space % ep_mps;
	if (data_len > avail_space) {
		data_len = avail_space;
	}

	if (data_len != 0U) {
		/* Get max packet size and packet count for ep */
		if (ep_idx == USB_DW_IN_EP_0) {
			max_pkt_cnt =
			    USB_DW_DIEPTSIZ0_PKT_CNT_MASK >>
			    USB_DW_DEPTSIZ_PKT_CNT_OFFSET;
			max_xfer_size =
			    USB_DW_DEPTSIZ0_XFER_SIZE_MASK >>
			    USB_DW_DEPTSIZ_XFER_SIZE_OFFSET;
		} else {
			max_pkt_cnt =
			    USB_DW_DIEPTSIZn_PKT_CNT_MASK >>
			    USB_DW_DEPTSIZ_PKT_CNT_OFFSET;
			max_xfer_size =
			    USB_DW_DEPTSIZn_XFER_SIZE_MASK >>
			    USB_DW_DEPTSIZ_XFER_SIZE_OFFSET;
		}

		/* Check if transfer len is too big */
		if (data_len > max_xfer_size) {
			LOG_WRN("USB IN EP%d len too big (%d->%d)", ep_idx,
				data_len, max_xfer_size);
			data_len = max_xfer_size;
		}

		/*
		 * Program the transfer size and packet count as follows:
		 *
		 * transfer size = N * ep_maxpacket + short_packet
		 * pktcnt = N + (short_packet exist ? 1 : 0)
		 */

		pkt_cnt = DIV_ROUND_UP(data_len, ep_mps);
		if (pkt_cnt > max_pkt_cnt) {
			LOG_WRN("USB IN EP%d pkt count too big (%d->%d)",
				ep_idx, pkt_cnt, pkt_cnt);
			pkt_cnt = max_pkt_cnt;
			data_len = pkt_cnt * ep_mps;
		}
	} else {
		/* Zero length packet */
		pkt_cnt = 1U;
	}

	/* Set number of packets and transfer size */
	base->in_ep_reg[ep_idx].dieptsiz =
		(pkt_cnt << USB_DW_DEPTSIZ_PKT_CNT_OFFSET) | data_len;

	/* Clear NAK and enable ep */
	base->in_ep_reg[ep_idx].diepctl |= (USB_DW_DEPCTL_EP_ENA |
					      USB_DW_DEPCTL_CNAK);

	/*
	 * Write data to FIFO, make sure that we are protected against
	 * other USB register accesses.  According to "DesignWare Cores
	 * USB 1.1/2.0 Device Subsystem-AHB/VCI Databook": "During FIFO
	 * access, the application must not access the UDC/Subsystem
	 * registers or vendor registers (for ULPI mode). After starting
	 * to access a FIFO, the application must complete the transaction
	 * before accessing the register."
	 */
	for (i = 0U; i < data_len; i += 4U) {
		uint32_t val = data[i];

		if (i + 1 < data_len) {
			val |= ((uint32_t)data[i+1]) << 8;
		}
		if (i + 2 < data_len) {
			val |= ((uint32_t)data[i+2]) << 16;
		}
		if (i + 3 < data_len) {
			val |= ((uint32_t)data[i+3]) << 24;
		}

		USB_DW_EP_FIFO(base, ep_idx) = val;
	}

	irq_unlock(key);

	LOG_DBG("USB IN EP%d write %u bytes", ep_idx, data_len);

	return data_len;
}

static int usb_dw_init(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep;
	int ret;

	ret = usb_dw_reset();
	if (ret) {
		return ret;
	}

#ifdef CONFIG_USB_DW_USB_2_0
	/* set the PHY interface to be 16-bit UTMI */
	base->gusbcfg = (base->gusbcfg & ~USB_DW_GUSBCFG_PHY_IF_MASK) |
		USB_DW_GUSBCFG_PHY_IF_16_BIT;

	/* Set USB2.0 High Speed */
	base->dcfg |= USB_DW_DCFG_DEV_SPD_USB2_HS;
#else
	/* Set device speed to Full Speed */
	base->dcfg |= USB_DW_DCFG_DEV_SPD_FS;
#endif

	/* Set NAK for all OUT EPs */
	for (ep = 0U; ep < USB_DW_OUT_EP_NUM; ep++) {
		base->out_ep_reg[ep].doepctl = USB_DW_DEPCTL_SNAK;
	}

	/* Enable global interrupts */
	base->gintmsk = USB_DW_GINTSTS_OEP_INT |
		USB_DW_GINTSTS_IEP_INT |
		USB_DW_GINTSTS_ENUM_DONE |
		USB_DW_GINTSTS_USB_RST |
		USB_DW_GINTSTS_WK_UP_INT |
		USB_DW_GINTSTS_USB_SUSP;

	/* Enable global interrupt */
	base->gahbcfg |= USB_DW_GAHBCFG_GLB_INTR_MASK;

	/* Disable soft disconnect */
	base->dctl &= ~USB_DW_DCTL_SFT_DISCON;

	usb_dw_reg_dump();

	return 0;
}

static void usb_dw_handle_reset(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;

	LOG_DBG("USB RESET event");

	/* Inform upper layers */
	if (usb_dw_ctrl.status_cb) {
		usb_dw_ctrl.status_cb(USB_DC_RESET, NULL);
	}

	/* Clear device address during reset. */
	base->dcfg &= ~USB_DW_DCFG_DEV_ADDR_MASK;

	/* enable global EP interrupts */
	base->doepmsk = 0U;
	base->gintmsk |= USB_DW_GINTSTS_RX_FLVL;
	base->diepmsk |= USB_DW_DIEPINT_XFER_COMPL;
}

static void usb_dw_handle_enum_done(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t speed;

	speed = (base->dsts & ~USB_DW_DSTS_ENUM_SPD_MASK) >>
	    USB_DW_DSTS_ENUM_SPD_OFFSET;

	LOG_DBG("USB ENUM DONE event, %s speed detected",
		speed == USB_DW_DSTS_ENUM_LS ? "Low" : "Full");

	/* Inform upper layers */
	if (usb_dw_ctrl.status_cb) {
		usb_dw_ctrl.status_cb(USB_DC_CONNECTED, NULL);
	}
}

/* USB ISR handler */
static inline void usb_dw_int_rx_flvl_handler(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t grxstsp = base->grxstsp;
	uint32_t status, xfer_size;
	uint8_t ep_idx;
	usb_dc_ep_callback ep_cb;

	/* Packet in RX FIFO */

	ep_idx = grxstsp & USB_DW_GRXSTSR_EP_NUM_MASK;
	status = (grxstsp & USB_DW_GRXSTSR_PKT_STS_MASK) >>
		USB_DW_GRXSTSR_PKT_STS_OFFSET;
	xfer_size = (grxstsp & USB_DW_GRXSTSR_PKT_CNT_MASK) >>
		USB_DW_GRXSTSR_PKT_CNT_OFFSET;

	LOG_DBG("USB OUT EP%u: RX_FLVL status %u, size %u",
		ep_idx, status, xfer_size);

	usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len = xfer_size;
	ep_cb = usb_dw_ctrl.out_ep_ctrl[ep_idx].cb;

	switch (status) {
	case USB_DW_GRXSTSR_PKT_STS_SETUP:
		/* Call the registered callback if any */
		if (ep_cb) {
			ep_cb(USB_EP_GET_ADDR(ep_idx, USB_EP_DIR_OUT),
			      USB_DC_EP_SETUP);
		}

		break;
	case USB_DW_GRXSTSR_PKT_STS_OUT_DATA:
		if (ep_cb) {
			ep_cb(USB_EP_GET_ADDR(ep_idx, USB_EP_DIR_OUT),
			      USB_DC_EP_DATA_OUT);
		}

		break;
	case USB_DW_GRXSTSR_PKT_STS_OUT_DATA_DONE:
	case USB_DW_GRXSTSR_PKT_STS_SETUP_DONE:
		break;
	default:
		break;
	}
}

static inline void usb_dw_int_iep_handler(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t ep_int_status;
	uint8_t ep_idx;
	usb_dc_ep_callback ep_cb;

	for (ep_idx = 0U; ep_idx < USB_DW_IN_EP_NUM; ep_idx++) {
		if (base->daint & USB_DW_DAINT_IN_EP_INT(ep_idx)) {
			/* Read IN EP interrupt status */
			ep_int_status = base->in_ep_reg[ep_idx].diepint &
				base->diepmsk;

			/* Clear IN EP interrupts */
			base->in_ep_reg[ep_idx].diepint = ep_int_status;

			LOG_DBG("USB IN EP%u interrupt status: 0x%x",
				ep_idx, ep_int_status);

			ep_cb = usb_dw_ctrl.in_ep_ctrl[ep_idx].cb;
			if (ep_cb &&
			    (ep_int_status & USB_DW_DIEPINT_XFER_COMPL)) {

				/* Call the registered callback */
				ep_cb(USB_EP_GET_ADDR(ep_idx, USB_EP_DIR_IN),
				      USB_DC_EP_DATA_IN);
			}
		}
	}

	/* Clear interrupt. */
	base->gintsts = USB_DW_GINTSTS_IEP_INT;
}

static inline void usb_dw_int_oep_handler(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t ep_int_status;
	uint8_t ep_idx;

	for (ep_idx = 0U; ep_idx < USB_DW_OUT_EP_NUM; ep_idx++) {
		if (base->daint & USB_DW_DAINT_OUT_EP_INT(ep_idx)) {
			/* Read OUT EP interrupt status */
			ep_int_status = base->out_ep_reg[ep_idx].doepint &
				base->doepmsk;

			/* Clear OUT EP interrupts */
			base->out_ep_reg[ep_idx].doepint = ep_int_status;

			LOG_DBG("USB OUT EP%u interrupt status: 0x%x\n",
				ep_idx, ep_int_status);
		}
	}

	/* Clear interrupt. */
	base->gintsts = USB_DW_GINTSTS_OEP_INT;
}

static void usb_dw_isr_handler(const void *unused)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint32_t int_status;

	ARG_UNUSED(unused);

	/*  Read interrupt status */
	while ((int_status = (base->gintsts & base->gintmsk))) {

		LOG_DBG("USB GINTSTS 0x%x", int_status);

		if (int_status & USB_DW_GINTSTS_USB_RST) {
			/* Clear interrupt. */
			base->gintsts = USB_DW_GINTSTS_USB_RST;

			/* Reset detected */
			usb_dw_handle_reset();
		}

		if (int_status & USB_DW_GINTSTS_ENUM_DONE) {
			/* Clear interrupt. */
			base->gintsts = USB_DW_GINTSTS_ENUM_DONE;

			/* Enumeration done detected */
			usb_dw_handle_enum_done();
		}

		if (int_status & USB_DW_GINTSTS_USB_SUSP) {
			/* Clear interrupt. */
			base->gintsts = USB_DW_GINTSTS_USB_SUSP;

			if (usb_dw_ctrl.status_cb) {
				usb_dw_ctrl.status_cb(USB_DC_SUSPEND, NULL);
			}
		}

		if (int_status & USB_DW_GINTSTS_WK_UP_INT) {
			/* Clear interrupt. */
			base->gintsts = USB_DW_GINTSTS_WK_UP_INT;

			if (usb_dw_ctrl.status_cb) {
				usb_dw_ctrl.status_cb(USB_DC_RESUME, NULL);
			}
		}

		if (int_status & USB_DW_GINTSTS_RX_FLVL) {
			/* Packet in RX FIFO */
			usb_dw_int_rx_flvl_handler();
		}

		if (int_status & USB_DW_GINTSTS_IEP_INT) {
			/* IN EP interrupt */
			usb_dw_int_iep_handler();
		}

		if (int_status & USB_DW_GINTSTS_OEP_INT) {
			/* No OUT interrupt expected in FIFO mode,
			 * just clear interrupt
			 */
			usb_dw_int_oep_handler();
		}
	}
}

int usb_dc_attach(void)
{
	int ret;

	if (usb_dw_ctrl.attached) {
		return 0;
	}

	ret = usb_dw_init();
	if (ret) {
		return ret;
	}

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    usb_dw_isr_handler, 0,
#ifdef CONFIG_GIC_V1
		    DT_INST_IRQ(0, type));
#else
		    DT_INST_IRQ(0, sense));
#endif

	irq_enable(DT_INST_IRQN(0));

	usb_dw_ctrl.attached = 1U;

	return 0;
}

int usb_dc_detach(void)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;

	if (!usb_dw_ctrl.attached) {
		return 0;
	}

	irq_disable(DT_INST_IRQN(0));

	/* Enable soft disconnect */
	base->dctl |= USB_DW_DCTL_SFT_DISCON;

	usb_dw_ctrl.attached = 0U;

	return 0;
}

int usb_dc_reset(void)
{
	int ret;

	ret = usb_dw_reset();

	/* Clear private data */
	(void)memset(&usb_dw_ctrl, 0, sizeof(usb_dw_ctrl));

	return ret;
}

int usb_dc_set_address(const uint8_t addr)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;

	if (addr > (USB_DW_DCFG_DEV_ADDR_MASK >> USB_DW_DCFG_DEV_ADDR_OFFSET)) {
		return -EINVAL;
	}

	base->dcfg &= ~USB_DW_DCFG_DEV_ADDR_MASK;
	base->dcfg |= addr << USB_DW_DCFG_DEV_ADDR_OFFSET;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (cfg->ep_mps > DW_USB_MAX_PACKET_SIZE) {
		LOG_WRN("unsupported packet size");
		return -1;
	}

	if (USB_EP_DIR_IS_OUT(cfg->ep_addr) && ep_idx >= USB_DW_OUT_EP_NUM) {
		LOG_WRN("OUT endpoint address out of range");
		return -1;
	}

	if (USB_EP_DIR_IS_IN(cfg->ep_addr) && ep_idx >= USB_DW_IN_EP_NUM) {
		LOG_WRN("IN endpoint address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	uint8_t ep;

	if (!ep_cfg) {
		return -EINVAL;
	}

	ep = ep_cfg->ep_addr;

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	usb_dw_ep_set(ep, ep_cfg->ep_mps, ep_cfg->ep_type);

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		base->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_STALL;
	} else {
		base->in_ep_reg[ep_idx].diepctl |= USB_DW_DEPCTL_STALL;
	}

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Not possible to clear stall for EP0 */
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		base->out_ep_reg[ep_idx].doepctl &= ~USB_DW_DEPCTL_STALL;
	} else {
		base->in_ep_reg[ep_idx].diepctl &= ~USB_DW_DEPCTL_STALL;
	}

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	volatile uint32_t *p_depctl;

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Cannot disable EP0, just set stall */
		usb_dc_ep_set_stall(ep);
	} else {
		if (USB_EP_DIR_IS_OUT(ep)) {
			p_depctl = &base->out_ep_reg[ep_idx].doepctl;
		} else {
			p_depctl = &base->in_ep_reg[ep_idx].diepctl;
		}

		/* Set STALL and disable endpoint if enabled */
		if (*p_depctl & USB_DW_DEPCTL_EP_ENA) {
			*p_depctl |= USB_DW_DEPCTL_EP_DIS | USB_DW_DEPCTL_STALL;
		} else {
			*p_depctl |= USB_DW_DEPCTL_STALL;
		}
	}

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = 0U;
	if (USB_EP_DIR_IS_OUT(ep)) {
		if (base->out_ep_reg[ep_idx].doepctl & USB_DW_DEPCTL_STALL) {
			*stalled = 1U;
		}
	} else {
		if (base->in_ep_reg[ep_idx].diepctl & USB_DW_DEPCTL_STALL) {
			*stalled = 1U;
		}
	}

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* enable EP interrupts */
	if (USB_EP_DIR_IS_OUT(ep)) {
		base->daintmsk |= USB_DW_DAINT_OUT_EP_INT(ep_idx);
	} else {
		base->daintmsk |= USB_DW_DAINT_IN_EP_INT(ep_idx);
	}

	/* Activate Ep */
	if (USB_EP_DIR_IS_OUT(ep)) {
		base->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_USB_ACT_EP;
		usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1U;
	} else {
		base->in_ep_reg[ep_idx].diepctl |= USB_DW_DEPCTL_USB_ACT_EP;
		usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1U;
	}

	if (USB_EP_DIR_IS_OUT(ep) &&
	    usb_dw_ctrl.out_ep_ctrl[ep_idx].cb != usb_transfer_ep_callback) {
		/* Start reading now, except for transfer managed eps */
		usb_dw_prep_rx(ep, 0);
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Disable EP interrupts */
	if (USB_EP_DIR_IS_OUT(ep)) {
		base->daintmsk &= ~USB_DW_DAINT_OUT_EP_INT(ep_idx);
		base->doepmsk &= ~USB_DW_DOEPINT_SET_UP;
	} else {
		base->daintmsk &= ~USB_DW_DAINT_IN_EP_INT(ep_idx);
		base->diepmsk &= ~USB_DW_DIEPINT_XFER_COMPL;
		base->gintmsk &= ~USB_DW_GINTSTS_RX_FLVL;
	}

	/* De-activate, disable and set NAK for Ep */
	if (USB_EP_DIR_IS_OUT(ep)) {
		base->out_ep_reg[ep_idx].doepctl &=
		    ~(USB_DW_DEPCTL_USB_ACT_EP |
		    USB_DW_DEPCTL_EP_ENA |
		    USB_DW_DEPCTL_SNAK);
		usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena = 0U;
	} else {
		base->in_ep_reg[ep_idx].diepctl &=
		    ~(USB_DW_DEPCTL_USB_ACT_EP |
		    USB_DW_DEPCTL_EP_ENA |
		    USB_DW_DEPCTL_SNAK);
		usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena = 0U;
	}

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t cnt;

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		/* RX FIFO is global and cannot be flushed per EP */
		return -EINVAL;
	}

	/* Each endpoint has dedicated Tx FIFO */
	base->grstctl |= ep_idx << USB_DW_GRSTCTL_TX_FNUM_OFFSET;
	base->grstctl |= USB_DW_GRSTCTL_TX_FFLSH;

	cnt = 0U;

	do {
		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			LOG_ERR("USB TX FIFO flush HANG!");
			return -EIO;
		}
		usb_dw_udelay(1);
	} while (base->grstctl & USB_DW_GRSTCTL_TX_FFLSH);

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t * const ret_bytes)
{
	int ret;

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if IN ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usb_dw_ep_is_enabled(ep)) {
		return -EINVAL;
	}

	ret = usb_dw_tx(ep, data, data_len);
	if (ret < 0) {
		return ret;
	}

	if (ret_bytes) {
		*ret_bytes = ret;
	}

	return 0;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct usb_dw_reg *const base = usb_dw_cfg.base;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t i, j, data_len, bytes_to_copy;

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usb_dw_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	data_len = usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len;

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero return
		 * the available data in buffer
		 */
		if (read_bytes) {
			*read_bytes = data_len;
		}
		return 0;
	}


	if (data_len > max_data_len) {
		LOG_ERR("Not enough room to copy all the rcvd data!");
		bytes_to_copy = max_data_len;
	} else {
		bytes_to_copy = data_len;
	}

	LOG_DBG("Read EP%d, req %d, read %d bytes", ep, max_data_len,
		bytes_to_copy);

	/* Data in the FIFOs is always stored per 32-bit words */
	for (i = 0U; i < (bytes_to_copy & ~0x3); i += 4U) {
		*(uint32_t *)(data + i) = USB_DW_EP_FIFO(base, ep_idx);
	}
	if (bytes_to_copy & 0x3) {
		/* Not multiple of 4 */
		uint32_t last_dw = USB_DW_EP_FIFO(base, ep_idx);

		for (j = 0U; j < (bytes_to_copy & 0x3); j++) {
			*(data + i + j) =
				(sys_cpu_to_le32(last_dw) >> (j * 8U)) & 0xFF;
			}
	}

	usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	return 0;

}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len) {
		usb_dw_prep_rx(ep_idx, 0);
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		const uint32_t max_data_len, uint32_t * const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		usb_dw_ctrl.in_ep_ctrl[ep_idx].cb = cb;
	} else {
		usb_dw_ctrl.out_ep_ctrl[ep_idx].cb = cb;
	}

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usb_dw_ctrl.status_cb = cb;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	enum usb_dw_out_ep_idx ep_idx = USB_EP_GET_IDX(ep);

	if (!usb_dw_ctrl.attached || !usb_dw_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		return usb_dw_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usb_dw_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}
