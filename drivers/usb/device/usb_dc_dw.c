/* usb_dc_dw.c - USB DesignWare device controller driver */

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

#include <string.h>
#include <stdio.h>
#include <misc/byteorder.h>
#include <usb/usb_dc.h>
#include <usb/usb_device.h>
#include "usb_dw_registers.h"
#include <soc.h>
#ifdef CONFIG_QMSI
#include "clk.h"
#endif

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL
#include <logging/sys_log.h>

/* convert from endpoint address to hardware endpoint index */
#define USB_DW_EP_ADDR2IDX(ep)  ((ep) & ~USB_EP_DIR_MASK)
/* get direction from endpoint address */
#define USB_DW_EP_ADDR2DIR(ep)  ((ep) & USB_EP_DIR_MASK)
/* convert from hardware endpoint index and direction to endpoint address */
#define USB_DW_EP_IDX2ADDR(idx, dir)    ((idx) | ((dir) & USB_EP_DIR_MASK))

/* Number of SETUP back-to-back packets */
#define USB_DW_SUP_CNT (1)

/*
 * USB endpoint private structure.
 */
struct usb_ep_ctrl_prv {
	u8_t ep_ena;
	u8_t fifo_num;
	u32_t fifo_size;
	u16_t mps;         /* Max ep pkt size */
	usb_dc_ep_callback cb;/* Endpoint callback function */
	u32_t data_len;
};

/*
 * USB controller private structure.
 */
struct usb_dw_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USB_DW_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USB_DW_OUT_EP_NUM];
	int n_tx_fifos;
	u8_t attached;
};


static struct usb_dw_ctrl_prv usb_dw_ctrl;

static inline void _usb_dw_int_unmask(void)
{
#if defined(CONFIG_SOC_QUARK_SE_C1000)
	QM_INTERRUPT_ROUTER->usb_0_int_mask &= ~BIT(0);
#endif
}

#if (CONFIG_SYS_LOG_USB_DRIVER_LEVEL >= SYS_LOG_LEVEL_DEBUG)
static void usb_dw_reg_dump(void)
{
	u8_t i;

	SYS_LOG_DBG("USB registers:");
	SYS_LOG_DBG("  GOTGCTL : 0x%x", USB_DW->gotgctl);
	SYS_LOG_DBG("  GOTGINT : 0x%x", USB_DW->gotgint);
	SYS_LOG_DBG("  GAHBCFG : 0x%x", USB_DW->gahbcfg);
	SYS_LOG_DBG("  GUSBCFG : 0x%x", USB_DW->gusbcfg);
	SYS_LOG_DBG("  GINTSTS : 0x%x", USB_DW->gintsts);
	SYS_LOG_DBG("  GINTMSK : 0x%x", USB_DW->gintmsk);
	SYS_LOG_DBG("  DCFG    : 0x%x", USB_DW->dcfg);
	SYS_LOG_DBG("  DCTL    : 0x%x", USB_DW->dctl);
	SYS_LOG_DBG("  DSTS    : 0x%x", USB_DW->dsts);
	SYS_LOG_DBG("  DIEPMSK : 0x%x", USB_DW->diepmsk);
	SYS_LOG_DBG("  DOEPMSK : 0x%x", USB_DW->doepmsk);
	SYS_LOG_DBG("  DAINT   : 0x%x", USB_DW->daint);
	SYS_LOG_DBG("  DAINTMSK: 0x%x", USB_DW->daintmsk);
	SYS_LOG_DBG("  GHWCFG1 : 0x%x", USB_DW->ghwcfg1);
	SYS_LOG_DBG("  GHWCFG2 : 0x%x", USB_DW->ghwcfg2);
	SYS_LOG_DBG("  GHWCFG3 : 0x%x", USB_DW->ghwcfg3);
	SYS_LOG_DBG("  GHWCFG4 : 0x%x", USB_DW->ghwcfg4);

	for (i = 0; i < USB_DW_OUT_EP_NUM; i++) {
		SYS_LOG_DBG("\n  EP %d registers:", i);
		SYS_LOG_DBG("    DIEPCTL : 0x%x",
		    USB_DW->in_ep_reg[i].diepctl);
		SYS_LOG_DBG("    DIEPINT : 0x%x",
		    USB_DW->in_ep_reg[i].diepint);
		SYS_LOG_DBG("    DIEPTSIZ: 0x%x",
		    USB_DW->in_ep_reg[i].dieptsiz);
		SYS_LOG_DBG("    DIEPDMA : 0x%x",
		    USB_DW->in_ep_reg[i].diepdma);
		SYS_LOG_DBG("    DOEPCTL : 0x%x",
		    USB_DW->out_ep_reg[i].doepctl);
		SYS_LOG_DBG("    DOEPINT : 0x%x",
		    USB_DW->out_ep_reg[i].doepint);
		SYS_LOG_DBG("    DOEPTSIZ: 0x%x",
		    USB_DW->out_ep_reg[i].doeptsiz);
		SYS_LOG_DBG("    DOEPDMA : 0x%x",
		    USB_DW->out_ep_reg[i].doepdma);
	}
}
#else
#define usb_dw_reg_dump()
#endif

static u8_t usb_dw_ep_is_valid(u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	/* Check if ep enabled */
	if ((USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) &&
	    ep_idx < USB_DW_OUT_EP_NUM) {
		return 1;
	} else if ((USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) &&
	    ep_idx < USB_DW_IN_EP_NUM) {
		return 1;
	}

	return 0;
}

static u8_t usb_dw_ep_is_enabled(u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	/* Check if ep enabled */
	if ((USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) &&
	    usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	} else if ((USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) &&
	    usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	}

	return 0;
}

static inline void usb_dw_udelay(u32_t us)
{
	k_busy_wait(us);
}

static int usb_dw_reset(void)
{
	u32_t cnt = 0;

	/* Wait for AHB master idle state. */
	while (!(USB_DW->grstctl & USB_DW_GRSTCTL_AHB_IDLE)) {
		usb_dw_udelay(1);
		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			SYS_LOG_ERR("USB reset HANG! AHB Idle GRSTCTL=0x%08x",
			    USB_DW->grstctl);
			return -EIO;
		}
	}

	/* Core Soft Reset */
	cnt = 0;
	USB_DW->grstctl |= USB_DW_GRSTCTL_C_SFT_RST;
	do {
		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			SYS_LOG_DBG("USB reset HANG! Soft Reset GRSTCTL=0x%08x",
			    USB_DW->grstctl);
			return -EIO;
		}
		usb_dw_udelay(1);
	} while (USB_DW->grstctl & USB_DW_GRSTCTL_C_SFT_RST);

	/* Wait for 3 PHY Clocks */
	usb_dw_udelay(100);

	return 0;
}

static int usb_dw_clock_enable(void)
{
#if defined(CONFIG_SOC_QUARK_SE_C1000)
	/* 7.2.7 USB Clock Operation */
	clk_sys_set_mode(CLK_SYS_CRYSTAL_OSC, CLK_SYS_DIV_1);

	/* Enable the USB Clock */
	QM_SCSS_CCU->ccu_mlayer_ahb_ctl |= QM_CCU_USB_CLK_EN;

	/* Set up the PLL. */
	QM_USB_PLL_CFG0 = QM_USB_PLL_CFG0_DEFAULT | QM_USB_PLL_PDLD;

	/* Wait for the PLL lock */
	while (0 == (QM_USB_PLL_CFG0 & QM_USB_PLL_LOCK)) {
	}
#endif
	return 0;
}

static void usb_dw_clock_disable(void)
{
#if defined(CONFIG_SOC_QUARK_SE_C1000)
	/* Disable the USB Clock */
	QM_SCSS_CCU->ccu_mlayer_ahb_ctl &= ~QM_CCU_USB_CLK_EN;

	/* Disable the PLL */
	QM_USB_PLL_CFG0 &= ~QM_USB_PLL_PDLD;
#endif
}

static int usb_dw_num_dev_eps(void)
{
	return (USB_DW->ghwcfg2 >> 10) & 0xf;
}

static void usb_dw_flush_tx_fifo(int ep)
{
	int fnum = usb_dw_ctrl.in_ep_ctrl[ep].fifo_num;

	USB_DW->grstctl = (fnum << 6) | (1<<5);
	while (USB_DW->grstctl & (1<<5))
		;
}

static int usb_dw_tx_fifo_avail(int ep)
{
	return USB_DW->in_ep_reg[ep].dtxfsts &
		USB_DW_DTXFSTS_TXF_SPC_AVAIL_MASK;
}

/* Choose a FIFO number for an IN endpoint */
static int usb_dw_set_fifo(u8_t ep)
{
	int ep_idx = USB_DW_EP_ADDR2IDX(ep);
	volatile u32_t *reg = &USB_DW->in_ep_reg[ep_idx].diepctl;
	u32_t val;
	int fifo = 0;
	int ded_fifo = !!(USB_DW->ghwcfg4 & USB_DW_HWCFG4_DEDFIFOMODE);

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

		reg = &USB_DW->in_ep_reg[ep_idx].diepctl;
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

static int usb_dw_ep_set(u8_t ep,
	u32_t ep_mps, enum usb_dc_ep_type ep_type)
{
	volatile u32_t *p_depctl;
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	SYS_LOG_DBG("usb_dw_ep_set ep %x, mps %d, type %d", ep, ep_mps,
		    ep_type);

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		p_depctl = &USB_DW->out_ep_reg[ep_idx].doepctl;
		usb_dw_ctrl.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		p_depctl = &USB_DW->in_ep_reg[ep_idx].diepctl;
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

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) {
		int ret = usb_dw_set_fifo(ep);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void usb_dw_prep_rx(const u8_t ep, u8_t setup)
{
	enum usb_dw_out_ep_idx ep_idx = USB_DW_EP_ADDR2IDX(ep);
	u32_t ep_mps = usb_dw_ctrl.out_ep_ctrl[ep_idx].mps;

	/* Set max RX size to EP mps so we get an interrupt
	 * each time a packet is received
	 */

	USB_DW->out_ep_reg[ep_idx].doeptsiz =
	    (USB_DW_SUP_CNT << USB_DW_DOEPTSIZ_SUP_CNT_OFFSET) |
	    (1 << USB_DW_DEPTSIZ_PKT_CNT_OFFSET) | ep_mps;

    /* Clear NAK and enable ep */
	if (!setup) {
		USB_DW->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_CNAK;
	}
	USB_DW->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_EP_ENA;

	SYS_LOG_DBG("USB OUT EP%d armed", ep_idx);
}

static int usb_dw_tx(u8_t ep, const u8_t *const data,
		u32_t data_len)
{
	enum usb_dw_in_ep_idx ep_idx = USB_DW_EP_ADDR2IDX(ep);
	u32_t max_xfer_size, max_pkt_cnt, pkt_cnt, avail_space;
	u32_t ep_mps = usb_dw_ctrl.in_ep_ctrl[ep_idx].mps;
	unsigned int key;
	u32_t i;

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

	avail_space *= 4;
	if (!avail_space) {
		SYS_LOG_ERR("USB IN EP%d no space available, DTXFSTS %x",
		    ep_idx, USB_DW->in_ep_reg[ep_idx].dtxfsts);
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

	if (data_len != 0) {
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
			SYS_LOG_WRN("USB IN EP%d len too big (%d->%d)", ep_idx,
			    data_len, max_xfer_size);
			data_len = max_xfer_size;
		}

		/*
		 * Program the transfer size and packet count as follows:
		 *
		 * transfer size = N * ep_maxpacket + short_packet
		 * pktcnt = N + (short_packet exist ? 1 : 0)
		 */

		pkt_cnt = (data_len + ep_mps - 1) / ep_mps;

		if (pkt_cnt > max_pkt_cnt) {
			SYS_LOG_WRN("USB IN EP%d pkt count too big (%d->%d)",
			    ep_idx, pkt_cnt, pkt_cnt);
			pkt_cnt = max_pkt_cnt;
			data_len = pkt_cnt * ep_mps;
		}
	} else {
		/* Zero length packet */
		pkt_cnt = 1;
	}

	/* Set number of packets and transfer size */
	USB_DW->in_ep_reg[ep_idx].dieptsiz =
	    (pkt_cnt << USB_DW_DEPTSIZ_PKT_CNT_OFFSET) | data_len;

	/* Clear NAK and enable ep */
	USB_DW->in_ep_reg[ep_idx].diepctl |= (USB_DW_DEPCTL_EP_ENA |
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
	for (i = 0; i < data_len; i += 4) {
		u32_t val = data[i];

		if (i + 1 < data_len) {
			val |= ((u32_t)data[i+1]) << 8;
		}
		if (i + 2 < data_len) {
			val |= ((u32_t)data[i+2]) << 16;
		}
		if (i + 3 < data_len) {
			val |= ((u32_t)data[i+3]) << 24;
		}
		USB_DW_EP_FIFO(ep_idx) = val;
	}
	irq_unlock(key);

	SYS_LOG_DBG("USB IN EP%d write %u bytes", ep_idx, data_len);

	return data_len;
}

static int usb_dw_init(void)
{
	u8_t ep;
	int ret;

	ret = usb_dw_reset();
	if (ret) {
		return ret;
	}

#ifdef CONFIG_USB_DW_USB_2_0
	/* set the PHY interface to be 16-bit UTMI */
	USB_DW->gusbcfg = (USB_DW->gusbcfg & ~USB_DW_GUSBCFG_PHY_IF_MASK) |
		USB_DW_GUSBCFG_PHY_IF_16_BIT;

	/* Set USB2.0 High Speed */
	USB_DW->dcfg |= USB_DW_DCFG_DEV_SPD_USB2_HS;
#else
	/* Set device speed to Full Speed */
	USB_DW->dcfg |= USB_DW_DCFG_DEV_SPD_FS;
#endif

	/* Set NAK for all OUT EPs */
	for (ep = 0; ep < USB_DW_OUT_EP_NUM; ep++) {
		USB_DW->out_ep_reg[ep].doepctl = USB_DW_DEPCTL_SNAK;
	}

	/* Enable global interrupts */
	USB_DW->gintmsk = USB_DW_GINTSTS_OEP_INT |
	    USB_DW_GINTSTS_IEP_INT |
	    USB_DW_GINTSTS_ENUM_DONE |
	    USB_DW_GINTSTS_USB_RST |
	    USB_DW_GINTSTS_WK_UP_INT |
	    USB_DW_GINTSTS_USB_SUSP;

	/* Enable global interrupt */
	USB_DW->gahbcfg |= USB_DW_GAHBCFG_GLB_INTR_MASK;

    /* Disable soft disconnect */
	USB_DW->dctl &= ~USB_DW_DCTL_SFT_DISCON;

	usb_dw_reg_dump();

	return 0;
}

static void usb_dw_handle_reset(void)
{
	SYS_LOG_DBG("USB RESET event");

	/* Inform upper layers */
	if (usb_dw_ctrl.status_cb) {
		usb_dw_ctrl.status_cb(USB_DC_RESET, NULL);
	}

	/* Clear device address during reset. */
	USB_DW->dcfg &= ~USB_DW_DCFG_DEV_ADDR_MASK;

	/* enable global EP interrupts */
	USB_DW->doepmsk = 0;
	USB_DW->gintmsk |= USB_DW_GINTSTS_RX_FLVL;
	USB_DW->diepmsk |= USB_DW_DIEPINT_XFER_COMPL;
}

static void usb_dw_handle_enum_done(void)
{
	u32_t speed;

	speed = (USB_DW->dsts & ~USB_DW_DSTS_ENUM_SPD_MASK) >>
	    USB_DW_DSTS_ENUM_SPD_OFFSET;

	SYS_LOG_DBG("USB ENUM DONE event, %s speed detected",
	    speed == USB_DW_DSTS_ENUM_LS ? "Low" : "Full");

	/* Inform upper layers */
	if (usb_dw_ctrl.status_cb) {
		usb_dw_ctrl.status_cb(USB_DC_CONNECTED, NULL);
	}
}

/* USB ISR handler */
static void usb_dw_isr_handler(void)
{
	u32_t int_status, ep_int_status;
	u8_t ep_idx;
	usb_dc_ep_callback ep_cb;

	/*  Read interrupt status */
	while ((int_status = (USB_DW->gintsts & USB_DW->gintmsk))) {

		SYS_LOG_DBG("USB GINTSTS 0x%x", int_status);

		if (int_status & USB_DW_GINTSTS_USB_RST) {
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_USB_RST;

			/* Reset detected */
			usb_dw_handle_reset();
		}

		if (int_status & USB_DW_GINTSTS_ENUM_DONE) {
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_ENUM_DONE;

			/* Enumeration done detected */
			usb_dw_handle_enum_done();
		}

		if (int_status & USB_DW_GINTSTS_USB_SUSP) {
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_USB_SUSP;

			if (usb_dw_ctrl.status_cb) {
				usb_dw_ctrl.status_cb(USB_DC_SUSPEND, NULL);
			}
		}

		if (int_status & USB_DW_GINTSTS_WK_UP_INT) {
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_WK_UP_INT;

			if (usb_dw_ctrl.status_cb) {
				usb_dw_ctrl.status_cb(USB_DC_RESUME, NULL);
			}
		}

		if (int_status & USB_DW_GINTSTS_RX_FLVL) {
			/* Packet in RX FIFO */
			u32_t status, xfer_size;
			u32_t grxstsp = USB_DW->grxstsp;

			ep_idx = grxstsp & USB_DW_GRXSTSR_EP_NUM_MASK;
			status = (grxstsp & USB_DW_GRXSTSR_PKT_STS_MASK) >>
			    USB_DW_GRXSTSR_PKT_STS_OFFSET;
			xfer_size = (grxstsp & USB_DW_GRXSTSR_PKT_CNT_MASK) >>
			    USB_DW_GRXSTSR_PKT_CNT_OFFSET;

			SYS_LOG_DBG("USB OUT EP%d: RX_FLVL status %d, size %d",
				ep_idx, status, xfer_size);
			usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len = xfer_size;
			ep_cb = usb_dw_ctrl.out_ep_ctrl[ep_idx].cb;
			switch (status) {
			case USB_DW_GRXSTSR_PKT_STS_SETUP:
				/* Call the registered callback if any */
				if (ep_cb) {
					ep_cb(USB_DW_EP_IDX2ADDR(ep_idx,
					    USB_EP_DIR_OUT),
					    USB_DC_EP_SETUP);
					}
				break;
			case USB_DW_GRXSTSR_PKT_STS_OUT_DATA:
				if (ep_cb) {
					ep_cb(USB_DW_EP_IDX2ADDR(ep_idx,
					    USB_EP_DIR_OUT),
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

		if (int_status & USB_DW_GINTSTS_IEP_INT) {
			/* IN EP interrupt */
			for (ep_idx = 0; ep_idx < USB_DW_IN_EP_NUM; ep_idx++) {
				if (USB_DW->daint &
				    USB_DW_DAINT_IN_EP_INT(ep_idx)) {
					/* Read IN EP interrupt status */
					ep_int_status =
					    USB_DW->in_ep_reg[ep_idx].diepint &
					    USB_DW->diepmsk;

					/* Clear IN EP interrupts */
					USB_DW->in_ep_reg[ep_idx].diepint =
					    ep_int_status;

					SYS_LOG_DBG("USB IN EP%d interrupt "
						    "status: 0x%x", ep_idx,
						    ep_int_status);

					ep_cb =
					    usb_dw_ctrl.in_ep_ctrl[ep_idx].cb;

					if ((ep_int_status &
					    USB_DW_DIEPINT_XFER_COMPL) &&
						ep_cb) {

						/* Call the registered
						 * callback
						 */
						ep_cb(USB_DW_EP_IDX2ADDR(ep_idx,
						    USB_EP_DIR_IN),
						    USB_DC_EP_DATA_IN);
					}
				}
			}
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_IEP_INT;
		}

		if (int_status & USB_DW_GINTSTS_OEP_INT) {
			/* No OUT interrupt expected in FIFO mode,
			 * just clear interruot
			 */
			for (ep_idx = 0; ep_idx < USB_DW_OUT_EP_NUM; ep_idx++) {
				if (USB_DW->daint &
				    USB_DW_DAINT_OUT_EP_INT(ep_idx)) {
					/* Read OUT EP interrupt status */
					ep_int_status =
					    USB_DW->out_ep_reg[ep_idx].doepint &
					    USB_DW->doepmsk;

					/* Clear OUT EP interrupts */
					USB_DW->out_ep_reg[ep_idx].doepint =
					    ep_int_status;

					SYS_LOG_DBG("USB OUT EP%d interrupt "
						    "status: 0x%x\n", ep_idx,
						    ep_int_status);
				}
			}
			/* Clear interrupt. */
			USB_DW->gintsts = USB_DW_GINTSTS_OEP_INT;
		}
	}
}

int usb_dc_attach(void)
{
	int ret;

	if (usb_dw_ctrl.attached) {
		return 0;
	}

	ret = usb_dw_clock_enable();
	if (ret) {
		return ret;
	}

	ret = usb_dw_init();
	if (ret) {
		return ret;
	}

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(USB_DW_IRQ, CONFIG_USB_DW_IRQ_PRI,
	    usb_dw_isr_handler, 0, IOAPIC_EDGE | IOAPIC_HIGH);
	irq_enable(USB_DW_IRQ);

	_usb_dw_int_unmask();

	usb_dw_ctrl.attached = 1;

	return 0;
}

int usb_dc_detach(void)
{
	if (!usb_dw_ctrl.attached) {
		return 0;
	}

	usb_dw_clock_disable();

	irq_disable(USB_DW_IRQ);

	/* Enable soft disconnect */
	USB_DW->dctl |= USB_DW_DCTL_SFT_DISCON;

	usb_dw_ctrl.attached = 0;

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

int usb_dc_set_address(const u8_t addr)
{
	if (addr > (USB_DW_DCFG_DEV_ADDR_MASK >> USB_DW_DCFG_DEV_ADDR_OFFSET)) {
		return -EINVAL;
	}

	USB_DW->dcfg &= ~USB_DW_DCFG_DEV_ADDR_MASK;
	USB_DW->dcfg |= addr << USB_DW_DCFG_DEV_ADDR_OFFSET;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(cfg->ep_addr);

	SYS_LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		    cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		SYS_LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (cfg->ep_mps > DW_USB_MAX_PACKET_SIZE) {
		SYS_LOG_WRN("unsupported packet size");
		return -1;
	}

	if ((USB_DW_EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_OUT) &&
	    (ep_idx >= DW_USB_OUT_EP_NUM)) {
		SYS_LOG_WRN("OUT endpoint address out of range");
		return -1;
	}

	if ((USB_DW_EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_IN) &&
	    (ep_idx >= DW_USB_IN_EP_NUM)) {
		SYS_LOG_WRN("IN endpoint address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep_cfg->ep_addr)) {
		return -EINVAL;
	}

	usb_dw_ep_set(ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

	return 0;
}

int usb_dc_ep_set_stall(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_STALL;
	} else {
		USB_DW->in_ep_reg[ep_idx].diepctl |= USB_DW_DEPCTL_STALL;
	}

	return 0;
}

int usb_dc_ep_clear_stall(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Not possible to clear stall for EP0 */
		return -EINVAL;
	}

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->out_ep_reg[ep_idx].doepctl &= ~USB_DW_DEPCTL_STALL;
	} else {
		USB_DW->in_ep_reg[ep_idx].diepctl &= ~USB_DW_DEPCTL_STALL;
	}

	return 0;
}

int usb_dc_ep_halt(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);
	volatile u32_t *p_depctl;


	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Cannot disable EP0, just set stall */
		usb_dc_ep_set_stall(ep);
	} else {
		if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
			p_depctl = &USB_DW->out_ep_reg[ep_idx].doepctl;
		} else {
			p_depctl = &USB_DW->in_ep_reg[ep_idx].diepctl;
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

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = 0;
	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		if (USB_DW->out_ep_reg[ep_idx].doepctl & USB_DW_DEPCTL_STALL) {
			*stalled = 1;
		}
	} else {
		if (USB_DW->in_ep_reg[ep_idx].diepctl & USB_DW_DEPCTL_STALL) {
			*stalled = 1;
		}
	}

	return 0;
}

int usb_dc_ep_enable(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	/* enable EP interrupts */
	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->daintmsk |= USB_DW_DAINT_OUT_EP_INT(ep_idx);
	} else {
		USB_DW->daintmsk |= USB_DW_DAINT_IN_EP_INT(ep_idx);
	}

	/* Activate Ep */
	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->out_ep_reg[ep_idx].doepctl |= USB_DW_DEPCTL_USB_ACT_EP;
		usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1;
	} else {
		USB_DW->in_ep_reg[ep_idx].diepctl |= USB_DW_DEPCTL_USB_ACT_EP;
		usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1;
	}

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT &&
		usb_dw_ctrl.out_ep_ctrl[ep_idx].cb != usb_transfer_ep_callback) {
		/* Start reading now, except for transfer managed eps */
		usb_dw_prep_rx(ep, 0);
	}

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	/* Disable EP interrupts */
	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->daintmsk &= ~USB_DW_DAINT_OUT_EP_INT(ep_idx);
		USB_DW->doepmsk &= ~USB_DW_DOEPINT_SET_UP;
	} else {
		USB_DW->daintmsk &= ~USB_DW_DAINT_IN_EP_INT(ep_idx);
		USB_DW->diepmsk &= ~USB_DW_DIEPINT_XFER_COMPL;
		USB_DW->gintmsk &= ~USB_DW_GINTSTS_RX_FLVL;
	}

	/* De-activate, disable and set NAK for Ep */
	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		USB_DW->out_ep_reg[ep_idx].doepctl &=
		    ~(USB_DW_DEPCTL_USB_ACT_EP |
		    USB_DW_DEPCTL_EP_ENA |
		    USB_DW_DEPCTL_SNAK);
		usb_dw_ctrl.out_ep_ctrl[ep_idx].ep_ena = 0;
	} else {
		USB_DW->in_ep_reg[ep_idx].diepctl &=
		    ~(USB_DW_DEPCTL_USB_ACT_EP |
		    USB_DW_DEPCTL_EP_ENA |
		    USB_DW_DEPCTL_SNAK);
		usb_dw_ctrl.in_ep_ctrl[ep_idx].ep_ena = 0;
	}

	return 0;
}

int usb_dc_ep_flush(const u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);
	u32_t cnt;

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		/* RX FIFO is global and cannot be flushed per EP */
		return -EINVAL;
	}

	/* Each endpoint has dedicated Tx FIFO */
	USB_DW->grstctl |= ep_idx << USB_DW_GRSTCTL_TX_FNUM_OFFSET;
	USB_DW->grstctl |= USB_DW_GRSTCTL_TX_FFLSH;

	cnt = 0;
	do {
		if (++cnt > USB_DW_CORE_RST_TIMEOUT_US) {
			SYS_LOG_ERR("USB TX FIFO flush HANG!");
			return -EIO;
		}
		usb_dw_udelay(1);
	} while (USB_DW->grstctl & USB_DW_GRSTCTL_TX_FFLSH);

	return 0;
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		const u32_t data_len, u32_t * const ret_bytes)
{
	int ret;

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	/* Check if IN ep */
	if (USB_DW_EP_ADDR2DIR(ep) != USB_EP_DIR_IN) {
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

int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *read_bytes)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);
	u32_t i, j, data_len, bytes_to_copy;

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		SYS_LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_DW_EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
		SYS_LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		SYS_LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usb_dw_ep_is_enabled(ep)) {
		SYS_LOG_ERR("Not enabled endpoint");
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
		SYS_LOG_ERR("Not enough room to copy all the rcvd data!");
		bytes_to_copy = max_data_len;
	} else {
		bytes_to_copy = data_len;
	}

	SYS_LOG_DBG("Read EP%d, req %d, read %d bytes",
	    ep, max_data_len, bytes_to_copy);

	/* Data in the FIFOs is always stored per 32-bit words */
	for (i = 0; i < (bytes_to_copy & ~0x3); i += 4) {
		*(u32_t *)(data + i) = USB_DW_EP_FIFO(ep_idx);
	}
	if (bytes_to_copy & 0x3) {
		/* Not multiple of 4 */
		u32_t last_dw = USB_DW_EP_FIFO(ep_idx);

		for (j = 0; j < (bytes_to_copy & 0x3); j++) {
			*(data + i + j) =
				(sys_cpu_to_le32(last_dw) >> (8 * j)) & 0xFF;
			}
	}

	usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	return 0;

}

int usb_dc_ep_read_continue(u8_t ep)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		SYS_LOG_ERR("No valid endpoint");
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_DW_EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
		SYS_LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usb_dw_ctrl.out_ep_ctrl[ep_idx].data_len) {
		usb_dw_prep_rx(ep_idx, 0);
	}

	return 0;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		const u32_t max_data_len, u32_t * const read_bytes)
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

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	u8_t ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (!usb_dw_ctrl.attached && !usb_dw_ep_is_valid(ep)) {
		return -EINVAL;
	}

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) {
		usb_dw_ctrl.in_ep_ctrl[ep_idx].cb = cb;
	} else {
		usb_dw_ctrl.out_ep_ctrl[ep_idx].cb = cb;
	}

	return 0;
}

int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usb_dw_ctrl.status_cb = cb;

	return 0;
}

int usb_dc_ep_mps(const u8_t ep)
{
	enum usb_dw_out_ep_idx ep_idx = USB_DW_EP_ADDR2IDX(ep);

	if (USB_DW_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		return usb_dw_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usb_dw_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}
