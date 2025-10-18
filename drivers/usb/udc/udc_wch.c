/*
 * Copyright (c) 2025 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <hal_ch32fun.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_wch, CONFIG_UDC_DRIVER_LOG_LEVEL);

struct udc_wch_config {
	USBOTG_FS_TypeDef *regs;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	int speed_idx;
	const struct device *clock_dev;
	uint8_t clock_id;
	void (*irq_enable_func)(const struct device *dev);
};

struct udc_wch_data {
	struct k_thread thread_data;
	uint32_t setup[2];
};

enum udc_wch_event_type {
	/* Shim driver event to trigger next transfer */
	UDC_WCH_EVT_XFER,
};

struct udc_wch_evt {
	enum udc_wch_event_type type;
	uint8_t ep;
};

K_MSGQ_DEFINE(drv_msgq, sizeof(struct udc_wch_evt), CONFIG_UDC_WCH_MAX_QMESSAGES, sizeof(uint32_t));

static int usbd_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&cfg->fifo, buf);

	regs->UEP0_DMA = (uint32_t)&buf->data;

	regs->UEP0_RX_CTRL = USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;

	return 0;
}

static void udc_wch_handle_setup(const struct device *dev)
{
	struct udc_wch_data *priv = udc_get_private(dev);
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	struct usb_setup_packet *setup;
	struct net_buf *buf;
	int err;

	regs->UEP0_TX_CTRL = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_NAK;
	regs->UEP0_RX_CTRL = USBFS_UEP_R_TOG | USBFS_UEP_R_RES_NAK;

	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	cfg->stat.halted = false;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	cfg->stat.halted = false;

	setup = (struct usb_setup_packet *)&priv->setup;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return;
	}

	udc_ep_buf_set_setup(buf);
	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		udc_ctrl_submit_s_in_status(dev);
	} else {
		udc_ctrl_submit_s_status(dev);
	}
}

static void udc_wch_xfer_next(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	struct net_buf *buf;
	volatile uint32_t *dma_reg;
	volatile uint16_t *tx_len;
	volatile uint8_t *tx_ctrl;
	volatile uint8_t *rx_ctrl;
	int len;

	buf = udc_buf_peek(ep_cfg);

	if (buf != NULL) {
		if (ep == USB_CONTROL_EP_IN) {
			ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

			len = MIN(ep_cfg->mps, buf->len);

			regs->UEP0_DMA = (uint32_t)buf->data;
			regs->UEP0_TX_LEN = len;
			regs->UEP0_TX_CTRL = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;

			buf->data += len;
			buf->len -= len;
		} else {

			if (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) {
				dma_reg = &regs->UEP0_DMA + USB_EP_GET_IDX(ep);
				tx_len = &regs->UEP0_TX_LEN + 2 * USB_EP_GET_IDX(ep);
				tx_ctrl = &regs->UEP0_TX_CTRL + 4 * USB_EP_GET_IDX(ep);

				struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
				len = MIN(cfg->mps, buf->len);

				*dma_reg = (uint32_t)buf->data;
				*tx_len = len;
				*tx_ctrl =
					(*tx_ctrl & ~USBOTG_UEP_T_RES_MASK) | USBOTG_UEP_T_RES_ACK;
				buf->data += len;
				buf->len -= len;
			} else {
				dma_reg = &regs->UEP0_DMA + USB_EP_GET_IDX(ep);
				rx_ctrl = &regs->UEP0_RX_CTRL + 4 * USB_EP_GET_IDX(ep);

				*dma_reg = (uint32_t)buf->data;
				*rx_ctrl =
					(*rx_ctrl & ~USBOTG_UEP_R_RES_MASK) | USBOTG_UEP_R_RES_ACK;
			}
		}
	}

	/* FIXME */
	k_busy_wait(1000);
}

static ALWAYS_INLINE void wch_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;

	LOG_DBG("Driver %p thread started", dev);
	while (true) {
		struct udc_wch_evt evt;

		int ret = k_msgq_get(&drv_msgq, &evt, K_FOREVER);

		if (ret != 0) {
			printk("k_msgq_get failed, %d\n", ret);
			continue;
		}

		switch (evt.type) {
		case UDC_WCH_EVT_XFER:
			udc_wch_xfer_next(dev, evt.ep);
			break;
		}
	}
}

static void udc_wch_set_status_buffer(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	struct udc_wch_data *priv = udc_get_private(dev);
	USBOTG_FS_TypeDef *regs = config->regs;

	regs->UEP0_DMA = (uint32_t)&priv->setup;
}

static int udc_wch_xfer_in(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	uint8_t int_status = regs->INT_ST;
	uint8_t ep_idx = int_status & USBFS_UIS_ENDP_MASK;
	uint8_t ep = ep_idx | USB_EP_DIR_IN;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;
	volatile uint8_t *tx_ctrl;
	volatile uint32_t *dma_reg;
	volatile uint16_t *tx_len;
	int len;

	buf = udc_buf_peek(ep_cfg);

	if (ep == USB_CONTROL_EP_IN) {

		regs->UEP0_TX_CTRL ^= USBFS_UEP_T_TOG;

		if (unlikely(buf == NULL)) {
			LOG_DBG("ep 0x%02x queue is empty", USB_CONTROL_EP_IN);
			return 0; /* FIXME : the function return an int just due to the sleep hack
				   */
		}

		if (buf->len != 0) {
			struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

			len = MIN(cfg->mps, buf->len);

			regs->UEP0_DMA = (uint32_t)buf->data;
			regs->UEP0_TX_LEN = len;

			buf->data += len;
			buf->len -= len;

			return 1;
		}

		if (udc_ep_buf_has_zlp(buf)) {
			udc_ep_buf_clear_zlp(buf);
			regs->UEP0_TX_LEN = 0;
			return 1;
		}

		buf = udc_buf_get(ep_cfg);

		regs->UEP0_TX_CTRL = USBFS_UEP_R_RES_NAK;

		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			regs->UEP0_RX_CTRL = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
			udc_wch_set_status_buffer(dev);
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/* IN transfer finished, release buffer */
			usbd_ctrl_feed_dout(dev, 0);
			net_buf_unref(buf);
		}
	} else {
		tx_ctrl = &regs->UEP0_TX_CTRL + 4 * ep_idx;

		*tx_ctrl ^= USBFS_UEP_T_TOG;

		if (buf->len != 0) {
			struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);

			len = MIN(cfg->mps, buf->len);

			dma_reg = &regs->UEP0_DMA + USB_EP_GET_IDX(ep);
			tx_len = &regs->UEP0_TX_LEN + 2 * USB_EP_GET_IDX(ep);

			*dma_reg = (uint32_t)buf->data;
			*tx_len = len;

			buf->data += len;
			buf->len -= len;

			return 1;
		}

		if (udc_ep_buf_has_zlp(buf)) {
			printf("ZLP !!!\n");
			udc_ep_buf_clear_zlp(buf);
			*tx_len = 0;
			return 1;
		}

		/* Remove buffer from queue */
		buf = udc_buf_get(ep_cfg);

		*tx_ctrl = (*tx_ctrl & ~USBOTG_UEP_T_RES_MASK) | USBOTG_UEP_T_RES_NAK;

		udc_submit_ep_event(dev, buf, 0);
	}

	return 0;
}

static void udc_wch_xfer_out(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	uint8_t int_status = regs->INT_ST;
	uint8_t ep = int_status & USBFS_UIS_ENDP_MASK;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;
	volatile uint8_t *rx_ctrl;
	int len;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	if (ep == USB_CONTROL_EP_OUT) {

		if (udc_ctrl_stage_is_status_out(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
			udc_wch_set_status_buffer(dev);
		}
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			udc_ctrl_submit_s_out_status(dev, buf);
		}

	} else {
		rx_ctrl = &regs->UEP0_RX_CTRL + 4 * USB_EP_GET_IDX(ep);
		*rx_ctrl ^= USBOTG_UEP_R_TOG;

		len = regs->RX_LEN;
		net_buf_add(buf, len);
		udc_submit_ep_event(dev, buf, 0);
	}
}

static void udc_wch_isr_handler(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	uint8_t int_flag, int_status;
	int suspend_status, reset_status;
	int ret;

	int_flag = regs->INT_FG;
	if (int_flag & USBOTG_UIE_TRANSFER) {

		int_status = regs->INT_ST;
		if ((int_status & USBFS_UIS_TOKEN_MASK) == USBFS_UIS_TOKEN_OUT) {
			udc_wch_xfer_out(dev);

		} else if ((int_status & USBFS_UIS_TOKEN_MASK) == USBFS_UIS_TOKEN_IN) {
			ret = udc_wch_xfer_in(dev);
			if (ret) { /* FIXME */
				regs->INT_FG = USBOTG_UIE_TRANSFER;
				k_busy_wait(100);
				return;
			}

		} else if ((int_status & USBFS_UIS_TOKEN_MASK) == USBFS_UIS_TOKEN_SETUP) {
			udc_wch_handle_setup(dev);
		}
		regs->INT_FG = USBOTG_UIE_TRANSFER;
	}

	if (int_flag & USBOTG_UIE_BUS_RST) {

		reset_status = regs->MIS_ST & USBOTG_UMS_BUS_RESET;
		if (reset_status) {
			udc_submit_event(dev, UDC_EVT_RESET, 0);

			udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
			udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);

			udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0);
			udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0);
		}

		regs->INT_FG = USBOTG_UIE_BUS_RST;
	}

	if (int_flag & USBOTG_UIE_SUSPEND) {

		suspend_status = regs->MIS_ST & USBOTG_UMS_SUSPEND;
		if (suspend_status) {
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		} else {
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}

		regs->INT_FG = USBOTG_UIE_SUSPEND;
	}
}

static int udc_wch_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			      struct net_buf *buf)
{
	struct udc_wch_evt evt;

	struct udc_buf_info *bi;
	bi = udc_get_buf_info(buf);

	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	evt.ep = cfg->addr;
	evt.type = UDC_WCH_EVT_XFER;
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);

	return 0;
}

static int udc_wch_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	irq_unlock(lock_key);

	return 0;
}

static int udc_wch_get_mode_ctrl_reg(const struct device *dev, uint8_t ep,
				     volatile uint8_t **mod_ctrl_reg, int *flag_tx, int *flag_rx)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;

	switch (USB_EP_GET_IDX(ep)) {
	case 1:
		*mod_ctrl_reg = &regs->UEP4_1_MOD;
		*flag_tx = USBOTG_UEP1_TX_EN;
		*flag_rx = USBOTG_UEP1_RX_EN;
		break;
	case 2:
		*mod_ctrl_reg = &regs->UEP2_3_MOD;
		*flag_tx = USBOTG_UEP2_TX_EN;
		*flag_rx = USBOTG_UEP2_RX_EN;
		break;
	case 3:
		*mod_ctrl_reg = &regs->UEP2_3_MOD;
		*flag_tx = USBOTG_UEP3_TX_EN;
		*flag_rx = USBOTG_UEP3_RX_EN;
		break;
	case 4:
		*mod_ctrl_reg = &regs->UEP4_1_MOD;
		*flag_tx = USBOTG_UEP4_TX_EN;
		*flag_rx = USBOTG_UEP4_RX_EN;
		break;
	case 5:
		*mod_ctrl_reg = &regs->UEP5_6_MOD;
		*flag_tx = USBOTG_UEP5_TX_EN;
		*flag_rx = USBOTG_UEP5_RX_EN;
		break;
	case 6:
		*mod_ctrl_reg = &regs->UEP5_6_MOD;
		*flag_tx = USBOTG_UEP6_TX_EN;
		*flag_rx = USBOTG_UEP6_RX_EN;
		break;
	case 7:
		*mod_ctrl_reg = &regs->UEP7_MOD;
		*flag_tx = USBOTG_UEP7_TX_EN;
		*flag_rx = USBOTG_UEP7_RX_EN;
		break;
	default:
		LOG_ERR("ep 0x%02x doesn't exist\n", ep);
		return -ENOTSUP;
	}

	return 0;
}

static int udc_wch_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	const uint8_t ep = cfg->addr;
	volatile uint8_t *tx_ctrl, *rx_ctrl;
	volatile uint8_t *ctrl_reg;
	int flag_tx, flag_rx;
	int ret;

	LOG_DBG("Enable ep 0x%02x", ep);

	if (ep == USB_CONTROL_EP_IN) {
		regs->UEP0_TX_CTRL = USBOTG_UEP_T_RES_NAK;
		udc_wch_set_status_buffer(dev);
	} else if (ep == USB_CONTROL_EP_OUT) {
		regs->UEP0_RX_CTRL = USBOTG_UEP_R_RES_ACK;
	} else {

		ret = udc_wch_get_mode_ctrl_reg(dev, ep, &ctrl_reg, &flag_tx, &flag_rx);
		if (ret) {
			return ret;
		}

		if (USB_EP_DIR_IS_IN(ep)) {
			*ctrl_reg |= flag_tx;
			tx_ctrl = &regs->UEP0_TX_CTRL +
				  (4 * USB_EP_GET_IDX(ep)); /* Get UEPn_TX_CTRL */
			*tx_ctrl = USBOTG_UEP_T_RES_NAK;
		} else {
			*ctrl_reg |= flag_rx;
			rx_ctrl = &regs->UEP0_RX_CTRL +
				  (4 * USB_EP_GET_IDX(ep)); /* Get UEPn_RX_CTRL */
			*rx_ctrl = USBOTG_UEP_R_RES_NAK;
		}
	}

	return 0;
}

static int udc_wch_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	volatile uint8_t *tx_ctrl, *rx_ctrl;
	volatile uint8_t *ctrl_reg;
	int flag_tx, flag_rx;
	int ret;

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	ret = udc_wch_get_mode_ctrl_reg(dev, cfg->addr, &ctrl_reg, &flag_tx, &flag_rx);
	if (ret) {
		return ret;
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		*ctrl_reg &= ~flag_tx;
		tx_ctrl = &regs->UEP0_TX_CTRL +
			  (4 * USB_EP_GET_IDX(cfg->addr)); /* Get UEPn_TX_CTRL */
		*tx_ctrl = USBOTG_UEP_T_RES_NAK;

	} else {
		*ctrl_reg &= ~flag_rx;
		rx_ctrl = &regs->UEP0_RX_CTRL +
			  (4 * USB_EP_GET_IDX(cfg->addr)); /* Get UEPn_RX_CTRL */
		*rx_ctrl = USBOTG_UEP_R_RES_NAK;
	}

	return 0;
}

static int udc_wch_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;
	LOG_DBG("Set halt ep 0x%02x", cfg->addr);
	volatile uint8_t *tx_ctrl, *rx_ctrl;

	if (cfg->addr & USB_EP_DIR_IN) {
		tx_ctrl = &regs->UEP0_TX_CTRL + 4 * USB_EP_GET_IDX(cfg->addr);
		*tx_ctrl = USBOTG_UEP_T_RES_STALL;
	} else {
		rx_ctrl = &regs->UEP0_RX_CTRL + 4 * USB_EP_GET_IDX(cfg->addr);
		*rx_ctrl = USBOTG_UEP_R_RES_STALL;
	}

	cfg->stat.halted = true;

	return 0;
}

static int udc_wch_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);
	cfg->stat.halted = false;

	/* If there is a request for this endpoint, enqueue the request. */
	if (udc_buf_peek(cfg) != NULL) {
		struct udc_wch_evt evt = {
			.type = UDC_WCH_EVT_XFER,
			.ep = cfg->addr,
		};

		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	return 0;
}

static int udc_wch_set_address(const struct device *dev, const uint8_t addr)
{
	LOG_DBG("Set new address %u for %p", addr, dev);

	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;

	regs->DEV_ADDR = (regs->DEV_ADDR & USBFS_UDA_GP_BIT) | addr;
	return 0;
}

static int udc_wch_host_wakeup(const struct device *dev)
{
	LOG_DBG("Remote wakeup from %p", dev);

	return 0;
}

/* Return actual USB device speed */
static enum udc_bus_speed udc_wch_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_wch_enable(const struct device *dev)
{

	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;

	regs->BASE_CTRL = USBOTG_UC_DEV_PU_EN | USBOTG_UC_INT_BUSY | USBOTG_UC_DMA_EN;
	regs->UDEV_CTRL = USBOTG_UD_PD_DIS | USBOTG_UD_PORT_EN;

	LOG_DBG("Enable device %p", dev);

	return 0;
}

static int udc_wch_disable(const struct device *dev)
{
	LOG_DBG("Disable device %p", dev);
	printf("disable\n");
	return 0;
}

/*
 * Prepare and configure most of the parts, if the controller has a way
 * of detecting VBUS activity it should be enabled here.
 * Only udc_skeleton_enable() makes device visible to the host.
 */
static int udc_wch_init(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	USBOTG_FS_TypeDef *regs = config->regs;

	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clock_id;
	clock_control_on(config->clock_dev, clock_sys);

	regs->BASE_CTRL = USBOTG_UC_RESET_SIE | USBOTG_UC_CLR_ALL;
	k_usleep(10);
	regs->BASE_CTRL = 0;

	regs->INT_EN = USBOTG_UIE_SUSPEND | USBOTG_UIE_BUS_RST | USBOTG_UIE_TRANSFER;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	config->irq_enable_func(dev);

	return 0;
}

/* Shut down the controller completely */
static int udc_wch_shutdown(const struct device *dev)
{
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return 0;
}

/*
 * This is called once to initialize the controller and endpoints
 * capabilities, and register endpoint structures.
 */
static int udc_wch_driver_preinit(const struct device *dev)
{
	const struct udc_wch_config *config = dev->config;
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	int err;

	/*
	 * You do not need to initialize it if your driver does not use
	 * udc_lock_internal() / udc_unlock_internal(), but implements its
	 * own mechanism.
	 */
	k_mutex_init(&data->mutex);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->speed_idx == 2) {
		data->caps.hs = true;
		mps = 1024;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->make_thread(dev);
	LOG_INF("Device %p (max. speed %d)", dev, config->speed_idx);

	return 0;
}

static void udc_wch_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_wch_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

/*
 * UDC API structure.
 * Note, you do not need to implement basic checks, these are done by
 * the UDC common layer udc_common.c
 */
static const struct udc_api udc_wch_api = {
	.lock = udc_wch_lock,
	.unlock = udc_wch_unlock,
	.device_speed = udc_wch_device_speed,
	.init = udc_wch_init,
	.enable = udc_wch_enable,
	.disable = udc_wch_disable,
	.shutdown = udc_wch_shutdown,
	.set_address = udc_wch_set_address,
	.host_wakeup = udc_wch_host_wakeup,
	.ep_enable = udc_wch_ep_enable,
	.ep_disable = udc_wch_ep_disable,
	.ep_set_halt = udc_wch_ep_set_halt,
	.ep_clear_halt = udc_wch_ep_clear_halt,
	.ep_enqueue = udc_wch_ep_enqueue,
	.ep_dequeue = udc_wch_ep_dequeue,
};

#define DT_DRV_COMPAT wch_usbfs

#define UDC_WCH_DEVICE_DEFINE(n)                                                                   \
	K_THREAD_STACK_DEFINE(udc_wch_stack_##n, CONFIG_UDC_WCH_STACK_SIZE);                       \
                                                                                                   \
	static void udc_wch_thread_##n(void *dev, void *arg1, void *arg2)                          \
	{                                                                                          \
		wch_thread_handler(dev);                                                           \
	}                                                                                          \
                                                                                                   \
	static void udc_wch_make_thread_##n(const struct device *dev)                              \
	{                                                                                          \
		struct udc_wch_data *priv = udc_get_private(dev);                                  \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_wch_stack_##n,                             \
				K_THREAD_STACK_SIZEOF(udc_wch_stack_##n), udc_wch_thread_##n,      \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_WCH_THREAD_PRIORITY), K_ESSENTIAL,          \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];              \
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];               \
                                                                                                   \
	static void udc_wch_irq_enable_func_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), udc_wch_isr_handler,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct udc_wch_config udc_wch_config_##n = {                                  \
		.regs = (USBOTG_FS_TypeDef *)DT_INST_REG_ADDR(n),                                  \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                                \
		.ep_cfg_in = ep_cfg_out,                                                           \
		.ep_cfg_out = ep_cfg_in,                                                           \
		.make_thread = udc_wch_make_thread_##n,                                            \
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_id = DT_INST_CLOCKS_CELL(n, id),                                            \
		.irq_enable_func = udc_wch_irq_enable_func_##n,                                    \
	};                                                                                         \
                                                                                                   \
	static struct udc_wch_data udc_priv_##n = {};                                              \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_wch_driver_preinit, NULL, &udc_data_##n, &udc_wch_config_##n, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_wch_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_WCH_DEVICE_DEFINE)
