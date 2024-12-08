/*
 * Copyright (c) 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  udc_stm32.c
 * @brief STM32 USB device controller (UDC) driver
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

#include "udc_common.h"

#include "stm32_hsem.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_stm32, CONFIG_UDC_DRIVER_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define DT_DRV_COMPAT st_stm32_otghs
#define UDC_STM32_IRQ_NAME     otghs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
#define DT_DRV_COMPAT st_stm32_otgfs
#define UDC_STM32_IRQ_NAME     otgfs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define DT_DRV_COMPAT st_stm32_usb
#define UDC_STM32_IRQ_NAME     usb
#endif

#define UDC_STM32_IRQ		DT_INST_IRQ_BY_NAME(0, UDC_STM32_IRQ_NAME, irq)
#define UDC_STM32_IRQ_PRI	DT_INST_IRQ_BY_NAME(0, UDC_STM32_IRQ_NAME, priority)

struct udc_stm32_data  {
	PCD_HandleTypeDef pcd;
	const struct device *dev;
	uint32_t irq;
	uint32_t occupied_mem;
	void (*pcd_prepare)(const struct device *dev);
	int (*clk_enable)(void);
	int (*clk_disable)(void);
};

struct udc_stm32_config {
	uint32_t num_endpoints;
	uint32_t pma_offset;
	uint32_t dram_size;
	uint16_t ep0_mps;
	uint16_t ep_mps;
};

static int udc_stm32_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int udc_stm32_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

#define hpcd2data(hpcd) CONTAINER_OF(hpcd, struct udc_stm32_data, pcd);

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	const struct device *dev = priv->dev;
	const struct udc_stm32_config *cfg = dev->config;
	struct udc_ep_config *ep;

	/* Re-Enable control endpoints */
	ep = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	if (ep && ep->stat.enabled) {
		HAL_PCD_EP_Open(&priv->pcd, USB_CONTROL_EP_OUT, cfg->ep0_mps,
				EP_TYPE_CTRL);
	}

	ep = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	if (ep && ep->stat.enabled) {
		HAL_PCD_EP_Open(&priv->pcd, USB_CONTROL_EP_IN, cfg->ep0_mps,
				EP_TYPE_CTRL);
	}

	udc_submit_event(priv->dev, UDC_EVT_RESET, 0);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_event(priv->dev, UDC_EVT_VBUS_READY, 0);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_event(priv->dev, UDC_EVT_VBUS_REMOVED, 0);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_set_suspended(priv->dev, true);
	udc_submit_event(priv->dev, UDC_EVT_SUSPEND, 0);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_set_suspended(priv->dev, false);
	udc_submit_event(priv->dev, UDC_EVT_RESUME, 0);
}

static int usbd_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&cfg->fifo, buf);

	HAL_PCD_EP_Receive(&priv->pcd, cfg->addr, buf->data, buf->size);

	return 0;
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	struct usb_setup_packet *setup = (void *)priv->pcd.Setup;
	const struct device *dev = priv->dev;
	struct net_buf *buf;
	int err;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT,
			     sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return;
	}

	udc_ep_buf_set_setup(buf);
	memcpy(buf->data, setup, 8);
	net_buf_add(buf, 8);

	udc_ctrl_update_stage(dev, buf);

	if (!buf->len) {
		return;
	}

	if ((setup->bmRequestType == 0) &&
	    (setup->bRequest == USB_SREQ_SET_ADDRESS)) {
		/* HAL requires we set the address before submitting status */
		HAL_PCD_SetAddress(&priv->pcd, setup->wValue);
	}

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		err = usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		udc_ctrl_submit_s_in_status(dev);
	} else {
		udc_ctrl_submit_s_status(dev);
	}
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_event(priv->dev, UDC_EVT_SOF, 0);
}

static int udc_stm32_tx(const struct device *dev, uint8_t ep,
			struct net_buf *buf)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	uint8_t *data; uint32_t len;
	HAL_StatusTypeDef status;

	LOG_DBG("TX ep 0x%02x len %u", ep, buf->len);

	if (udc_ep_is_busy(dev, ep)) {
		return 0;
	}

	data = buf->data;
	len = buf->len;

	if (ep == USB_CONTROL_EP_IN) {
		len = MIN(cfg->ep0_mps, buf->len);
	}

	buf->data += len;
	buf->len -= len;

	status = HAL_PCD_EP_Transmit(&priv->pcd, ep, data, len);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Transmit failed(0x%02x), %d", ep, (int)status);
		return -EIO;
	}

	udc_ep_set_busy(dev, ep, true);

	if (ep == USB_CONTROL_EP_IN && len > 0) {
		/* Wait for an empty package from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		usbd_ctrl_feed_dout(dev, 0);
	}

	return 0;
}

static int udc_stm32_rx(const struct device *dev, uint8_t ep,
			struct net_buf *buf)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("RX ep 0x%02x len %u", ep, buf->size);

	if (udc_ep_is_busy(dev, ep)) {
		return 0;
	}

	status = HAL_PCD_EP_Receive(&priv->pcd, ep, buf->data, buf->size);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Receive failed(0x%02x), %d", ep, (int)status);
		return -EIO;
	}

	udc_ep_set_busy(dev, ep, true);

	return 0;
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	uint32_t rx_count = HAL_PCD_EP_GetRxCount(hpcd, epnum);
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	const struct device *dev = priv->dev;
	uint8_t ep = epnum | USB_EP_DIR_OUT;
	struct net_buf *buf;

	LOG_DBG("DataOut ep 0x%02x",  ep);

	udc_ep_set_busy(dev, ep, false);

	buf = udc_buf_get(dev, ep);
	if (unlikely(buf == NULL)) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		return;
	}

	net_buf_add(buf, rx_count);

	if (ep == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			udc_ctrl_update_stage(dev, buf);
			udc_ctrl_submit_status(dev, buf);
		} else {
			udc_ctrl_update_stage(dev, buf);
		}

		if (udc_ctrl_stage_is_status_in(dev)) {
			udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}

	buf = udc_buf_peek(dev, ep);
	if (buf) {
		udc_stm32_rx(dev, ep, buf);
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	const struct device *dev = priv->dev;
	uint8_t ep = epnum | USB_EP_DIR_IN;
	struct net_buf *buf;

	LOG_DBG("DataIn ep 0x%02x",  ep);

	udc_ep_set_busy(dev, ep, false);

	buf = udc_buf_peek(dev, ep);
	if (unlikely(buf == NULL)) {
		return;
	}

	if (ep == USB_CONTROL_EP_IN && buf->len) {
		const struct udc_stm32_config *cfg = dev->config;
		uint32_t len = MIN(cfg->ep0_mps, buf->len);

		HAL_PCD_EP_Transmit(&priv->pcd, ep, buf->data, len);

		buf->len -= len;
		buf->data += len;

		return;
	}

	if (udc_ep_buf_has_zlp(buf) && ep != USB_CONTROL_EP_IN) {
		udc_ep_buf_clear_zlp(buf);
		HAL_PCD_EP_Transmit(&priv->pcd, ep, buf->data, 0);

		return;
	}

	udc_buf_get(dev, ep);

	if (ep == USB_CONTROL_EP_IN) {
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

		return;
	}

	udc_submit_ep_event(dev, buf, 0);

	buf = udc_buf_peek(dev, ep);
	if (buf) {
		udc_stm32_tx(dev, ep, buf);
	}
}

#if DT_INST_NODE_HAS_PROP(0, disconnect_gpios)
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state)
{
	struct gpio_dt_spec usb_disconnect = GPIO_DT_SPEC_INST_GET(0, disconnect_gpios);

	gpio_pin_configure_dt(&usb_disconnect,
			      state ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
}
#endif

static void udc_stm32_irq(const struct device *dev)
{
	const struct udc_stm32_data *priv =  udc_get_private(dev);

	/* HAL irq handler will call the related above callback */
	HAL_PCD_IRQHandler((PCD_HandleTypeDef *)&priv->pcd);
}

int udc_stm32_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	if (priv->clk_enable && priv->clk_enable()) {
		LOG_ERR("Error enabling clock(s)");
		return -EIO;
	}

	priv->pcd_prepare(dev);

	status = HAL_PCD_Init(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	HAL_PCD_Stop(&priv->pcd);

	return 0;
}

#if defined(USB) || defined(USB_DRD_FS)
static inline void udc_stm32_mem_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;

	priv->occupied_mem = cfg->pma_offset;
}

static int udc_stm32_ep_mem_config(const struct device *dev,
				   struct udc_ep_config *ep,
				   bool enable)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	uint32_t size;

	size = MIN(udc_mps_ep_size(ep), cfg->ep_mps);

	if (!enable) {
		priv->occupied_mem -= size;
		return 0;
	}

	if (priv->occupied_mem + size >= cfg->dram_size) {
		LOG_ERR("Unable to allocate FIFO for 0x%02x", ep->addr);
		return -ENOMEM;
	}

	/* Configure PMA offset for the endpoint */
	HAL_PCDEx_PMAConfig(&priv->pcd, ep->addr, PCD_SNG_BUF,
			    priv->occupied_mem);

	priv->occupied_mem += size;

	return 0;
}
#else
static void udc_stm32_mem_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	int words;

	LOG_DBG("DRAM size: %ub", cfg->dram_size);

	if (cfg->ep_mps % 4 || cfg->ep0_mps % 4) {
		LOG_ERR("Not a 32-bit word multiple: ep0(%u)|ep(%u)",
			cfg->ep0_mps, cfg->ep_mps);
		return;
	}

	/* The documentation is not clear at all about RX FiFo size requirement,
	 * Allocate a minimum of 0x40 words, which seems to work reliably.
	 */
	words = MAX(0x40, cfg->ep_mps / 4);
	HAL_PCDEx_SetRxFiFo(&priv->pcd, words);
	priv->occupied_mem = words * 4;

	/* For EP0 TX, reserve only one MPS */
	HAL_PCDEx_SetTxFiFo(&priv->pcd, 0, cfg->ep0_mps / 4);
	priv->occupied_mem += cfg->ep0_mps;

	/* Reset TX allocs */
	for (unsigned int i = 1U; i < cfg->num_endpoints; i++) {
		HAL_PCDEx_SetTxFiFo(&priv->pcd, i, 0);
	}
}

static int udc_stm32_ep_mem_config(const struct device *dev,
				   struct udc_ep_config *ep,
				   bool enable)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	unsigned int words;

	if (!(ep->addr & USB_EP_DIR_IN) || !USB_EP_GET_IDX(ep->addr)) {
		return 0;
	}

	words = MIN(udc_mps_ep_size(ep), cfg->ep_mps) / 4;
	words = (words <= 64) ? words * 2 : words;

	if (!enable) {
		if (priv->occupied_mem >= (words * 4)) {
			priv->occupied_mem -= (words * 4);
		}
		HAL_PCDEx_SetTxFiFo(&priv->pcd, USB_EP_GET_IDX(ep->addr), 0);
		return 0;
	}

	if (cfg->dram_size - priv->occupied_mem < words * 4) {
		LOG_ERR("Unable to allocate FIFO for 0x%02x", ep->addr);
		return -ENOMEM;
	}

	HAL_PCDEx_SetTxFiFo(&priv->pcd, USB_EP_GET_IDX(ep->addr), words);

	priv->occupied_mem += words * 4;

	return 0;
}
#endif

static int udc_stm32_enable(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	HAL_StatusTypeDef status;
	int ret;

	LOG_DBG("Enable UDC");

	udc_stm32_mem_init(dev);

	status = HAL_PCD_Start(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				     USB_EP_TYPE_CONTROL, cfg->ep0_mps, 0);
	if (ret) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_OUT);
		return ret;
	}

	ret |= udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				      USB_EP_TYPE_CONTROL, cfg->ep0_mps, 0);
	if (ret) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_IN);
		return ret;
	}

	irq_enable(priv->irq);

	return 0;
}

static int udc_stm32_disable(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	irq_disable(UDC_STM32_IRQ);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	status = HAL_PCD_Stop(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Stop failed, %d", (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_shutdown(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	status = HAL_PCD_DeInit(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_DeInit failed, %d", (int)status);
		/* continue anyway */
	}

	if (priv->clk_disable && priv->clk_disable()) {
		LOG_ERR("Error disabling clock(s)");
		/* continue anyway */
	}

	if (irq_is_enabled(priv->irq)) {
		irq_disable(priv->irq);
	}

	return 0;
}

static int udc_stm32_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Set Address %u", addr);

	status = HAL_PCD_SetAddress(&priv->pcd, addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_SetAddress failed(0x%02x), %d",
			addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_host_wakeup(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	status = HAL_PCD_ActivateRemoteWakeup(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_ActivateRemoteWakeup, %d", (int)status);
		return -EIO;
	}

	/* Must be active from 1ms to 15ms as per reference manual. */
	k_sleep(K_MSEC(2));

	status = HAL_PCD_DeActivateRemoteWakeup(&priv->pcd);
	if (status != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_enable(const struct device *dev,
			       struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;
	uint8_t ep_type;
	int ret;

	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_type = EP_TYPE_CTRL;
		break;
	case USB_EP_TYPE_BULK:
		ep_type = EP_TYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_type = EP_TYPE_INTR;
		break;
	case USB_EP_TYPE_ISO:
		ep_type = EP_TYPE_ISOC;
		break;
	default:
		return -EINVAL;
	}

	ret = udc_stm32_ep_mem_config(dev, ep_cfg, true);
	if (ret) {
		return ret;
	}

	status = HAL_PCD_EP_Open(&priv->pcd, ep_cfg->addr,
				 udc_mps_ep_size(ep_cfg), ep_type);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Open failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_disable(const struct device *dev,
			      struct udc_ep_config *ep)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Disable ep 0x%02x", ep->addr);

	status = HAL_PCD_EP_Close(&priv->pcd, ep->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Close failed(0x%02x), %d",
			ep->addr, (int)status);
		return -EIO;
	}

	return udc_stm32_ep_mem_config(dev, ep, false);
}

static int udc_stm32_ep_set_halt(const struct device *dev,
				 struct udc_ep_config *cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Halt ep 0x%02x", cfg->addr);

	status = HAL_PCD_EP_SetStall(&priv->pcd, cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_SetStall failed(0x%02x), %d",
			cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_clear_halt(const struct device *dev,
				   struct udc_ep_config *cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Clear halt for ep 0x%02x", cfg->addr);

	status = HAL_PCD_EP_ClrStall(&priv->pcd, cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_ClrStall failed(0x%02x), %d",
			cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_flush(const struct device *dev,
			      struct udc_ep_config *cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Flush ep 0x%02x", cfg->addr);

	status = HAL_PCD_EP_Flush(&priv->pcd, cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Flush failed(0x%02x), %d",
			cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_enqueue(const struct device *dev,
				struct udc_ep_config *epcfg,
				struct net_buf *buf)
{
	unsigned int lock_key;
	int ret;

	udc_buf_put(epcfg, buf);

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(epcfg->addr)) {
		ret = udc_stm32_tx(dev, epcfg->addr, buf);
	} else {
		ret = udc_stm32_rx(dev, epcfg->addr, buf);
	}

	irq_unlock(lock_key);

	return ret;
}

static int udc_stm32_ep_dequeue(const struct device *dev,
				struct udc_ep_config *epcfg)
{
	struct net_buf *buf;

	udc_stm32_ep_flush(dev, epcfg);

	buf = udc_buf_get_all(dev, epcfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, epcfg->addr, false);

	return 0;
}

static enum udc_bus_speed udc_stm32_device_speed(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);

#ifdef USBD_HS_SPEED
	if (priv->pcd.Init.speed == USBD_HS_SPEED) {
		return UDC_BUS_SPEED_HS;
	}
#endif

	if (priv->pcd.Init.speed == USBD_FS_SPEED) {
		return UDC_BUS_SPEED_FS;
	}

	return UDC_BUS_UNKNOWN;
}

static const struct udc_api udc_stm32_api = {
	.lock = udc_stm32_lock,
	.unlock = udc_stm32_unlock,
	.init = udc_stm32_init,
	.enable = udc_stm32_enable,
	.disable = udc_stm32_disable,
	.shutdown = udc_stm32_shutdown,
	.set_address = udc_stm32_set_address,
	.host_wakeup = udc_stm32_host_wakeup,
	.ep_try_config = NULL,
	.ep_enable = udc_stm32_ep_enable,
	.ep_disable = udc_stm32_ep_disable,
	.ep_set_halt = udc_stm32_ep_set_halt,
	.ep_clear_halt = udc_stm32_ep_clear_halt,
	.ep_enqueue = udc_stm32_ep_enqueue,
	.ep_dequeue = udc_stm32_ep_dequeue,
	.device_speed = udc_stm32_device_speed,
};

/* ----------------- Instance/Device specific data ----------------- */

/*
 * USB, USB_OTG_FS and USB_DRD_FS are defined in STM32Cube HAL and allows to
 * distinguish between two kind of USB DC. STM32 F0, F3, L0 and G4 series
 * support USB device controller. STM32 F4 and F7 series support USB_OTG_FS
 * device controller. STM32 F1 and L4 series support either USB or USB_OTG_FS
 * device controller.STM32 G0 series supports USB_DRD_FS device controller.
 *
 * WARNING: Don't mix USB defined in STM32Cube HAL and CONFIG_USB_* from Zephyr
 * Kconfig system.
 */
#define USB_NUM_BIDIR_ENDPOINTS	DT_INST_PROP(0, num_bidir_endpoints)

#if defined(USB) || defined(USB_DRD_FS)
#define EP0_MPS 64U
#define EP_MPS 64U
#define USB_BTABLE_SIZE  (8 * USB_NUM_BIDIR_ENDPOINTS)
#define USB_RAM_SIZE	DT_INST_PROP(0, ram_size)
#else /* USB_OTG_FS */
#define EP0_MPS USB_OTG_MAX_EP0_SIZE
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define EP_MPS USB_OTG_HS_MAX_PACKET_SIZE
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs) || DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define EP_MPS USB_OTG_FS_MAX_PACKET_SIZE
#endif
#define USB_RAM_SIZE	DT_INST_PROP(0, ram_size)
#define USB_BTABLE_SIZE 0
#endif /* USB */

#define USB_OTG_HS_EMB_PHY (DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usbphyc) && \
			    DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

#define USB_OTG_HS_ULPI_PHY (DT_HAS_COMPAT_STATUS_OKAY(usb_ulpi_phy) && \
			    DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs))

static struct udc_stm32_data udc0_priv;

static struct udc_data udc0_data = {
	.mutex = Z_MUTEX_INITIALIZER(udc0_data.mutex),
	.priv = &udc0_priv,
};

static const struct udc_stm32_config udc0_cfg  = {
	.num_endpoints = USB_NUM_BIDIR_ENDPOINTS,
	.dram_size = USB_RAM_SIZE,
	.pma_offset = USB_BTABLE_SIZE,
	.ep0_mps = EP0_MPS,
	.ep_mps = EP_MPS,
};

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
static uint32_t usb_dc_stm32_get_maximum_speed(void)
{
/*
 * STM32L4 series USB LL API doesn't provide HIGH and HIGH_IN_FULL speed
 * defines.
 */
#if defined(CONFIG_SOC_SERIES_STM32L4X)
#define USB_OTG_SPEED_HIGH                     0U
#define USB_OTG_SPEED_HIGH_IN_FULL             1U
#endif /* CONFIG_SOC_SERIES_STM32L4X */
/*
 * If max-speed is not passed via DT, set it to USB controller's
 * maximum hardware capability.
 */
#if USB_OTG_HS_EMB_PHY || USB_OTG_HS_ULPI_PHY
	uint32_t speed = USB_OTG_SPEED_HIGH;
#else
	uint32_t speed = USB_OTG_SPEED_FULL;
#endif

#ifdef USB_MAXIMUM_SPEED

	if (!strncmp(USB_MAXIMUM_SPEED, "high-speed", 10)) {
		speed = USB_OTG_SPEED_HIGH;
	} else if (!strncmp(USB_MAXIMUM_SPEED, "full-speed", 10)) {
#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(USB_OTG_HS_EMB_PHY)
		speed = USB_OTG_SPEED_HIGH_IN_FULL;
#else
		speed = USB_OTG_SPEED_FULL;
#endif
	} else {
		LOG_DBG("Unsupported maximum speed defined in device tree. "
			"USB controller will default to its maximum HW "
			"capability");
	}
#endif

	return speed;
}
#endif /* USB_OTG_FS || USB_OTG_HS */

static void priv_pcd_prepare(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;

	memset(&priv->pcd, 0, sizeof(priv->pcd));

	/* Default values */
	priv->pcd.Init.dev_endpoints = cfg->num_endpoints;
	priv->pcd.Init.ep0_mps = cfg->ep0_mps;
	priv->pcd.Init.speed = PCD_SPEED_FULL;

	/* Per controller/Phy values */
#if defined(USB)
	priv->pcd.Instance = USB;
#elif defined(USB_DRD_FS)
	priv->pcd.Instance = USB_DRD_FS;
#elif defined(USB_OTG_FS) || defined(USB_OTG_HS)
	priv->pcd.Init.speed = usb_dc_stm32_get_maximum_speed();
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
	priv->pcd.Instance = USB_OTG_HS;
#else
	priv->pcd.Instance = USB_OTG_FS;
#endif
#endif /* USB */

#if USB_OTG_HS_EMB_PHY
	priv->pcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
#elif USB_OTG_HS_ULPI_PHY
	priv->pcd.Init.phy_itface = USB_OTG_ULPI_PHY;
#else
	priv->pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
#endif /* USB_OTG_HS_EMB_PHY */
}

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

static int priv_clock_enable(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) && defined(CONFIG_SOC_SERIES_STM32U5X)
	/* Sequence to enable the power of the OTG HS on a stm32U5 serie : Enable VDDUSB */
	bool pwr_clk = LL_AHB3_GRP1_IsEnabledClock(LL_AHB3_GRP1_PERIPH_PWR);

	if (!pwr_clk) {
		LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);
	}

	/* Check that power range is 1 or 2 */
	if (LL_PWR_GetRegulVoltageScaling() < LL_PWR_REGU_VOLTAGE_SCALE2) {
		LOG_ERR("Wrong Power range to use USB OTG HS");
		return -EIO;
	}

	LL_PWR_EnableVddUSB();
	/* Configure VOSR register of USB HSTransceiverSupply(); */
	LL_PWR_EnableUSBPowerSupply();
	LL_PWR_EnableUSBEPODBooster();
	while (LL_PWR_IsActiveFlag_USBBOOST() != 1) {
		/* Wait for USB EPOD BOOST ready */
	}

	/* Leave the PWR clock in its initial position */
	if (!pwr_clk) {
		LL_AHB3_GRP1_DisableClock(LL_AHB3_GRP1_PERIPH_PWR);
	}

	/* Set the OTG PHY reference clock selection (through SYSCFG) block */
	LL_APB3_GRP1_EnableClock(LL_APB3_GRP1_PERIPH_SYSCFG);
	HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);
	/* Configuring the SYSCFG registers OTG_HS PHY : OTG_HS PHY enable*/
	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);
#elif defined(PWR_USBSCR_USB33SV) || defined(PWR_SVMCR_USV)
	/*
	 * VDDUSB independent USB supply (PWR clock is on)
	 * with LL_PWR_EnableVDDUSB function (higher case)
	 */
	LL_PWR_EnableVDDUSB();
#endif

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_PWR_EnableUSBVoltageDetector();

	/* Per AN2606: USBREGEN not supported when running in FS mode. */
	LL_PWR_DisableUSBReg();
	while (!LL_PWR_IsActiveFlag_USB()) {
		LOG_INF("PWR not active yet");
		k_sleep(K_MSEC(100));
	}
#endif

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk, (clock_control_subsys_t *)&pclken[1],
									NULL) != 0) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&pclken[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_USB_DC_STM32_CLOCK_CHECK)) {
		uint32_t usb_clock_rate;

		if (clock_control_get_rate(clk,
					   (clock_control_subsys_t *)&pclken[1],
					   &usb_clock_rate) != 0) {
			LOG_ERR("Failed to get USB domain clock rate");
			return -EIO;
		}

		if (usb_clock_rate != MHZ(48)) {
			LOG_ERR("USB Clock is not 48MHz (%d)", usb_clock_rate);
			return -ENOTSUP;
		}
	}

	/* Previous check won't work in case of F1/F3. Add build time check */
#if defined(RCC_CFGR_OTGFSPRE) || defined(RCC_CFGR_USBPRE)

#if (MHZ(48) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) && !defined(STM32_PLL_USBPRE)
	/* PLL output clock is set to 48MHz, it should not be divided */
#warning USBPRE/OTGFSPRE should be set in rcc node
#endif

#endif /* RCC_CFGR_OTGFSPRE / RCC_CFGR_USBPRE */

#if USB_OTG_HS_ULPI_PHY
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
#else
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
#endif
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) /* USB_OTG_HS_ULPI_PHY */
	/* Disable ULPI interface (for external high-speed PHY) clock in sleep/low-power mode.
	 * It is disabled by default in run power mode, no need to disable it.
	 */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_USBPHY);
	/* Both OTG HS and USBPHY sleep clock MUST be disabled here at the same time */
	LL_AHB2_GRP1_DisableClockStopSleep(LL_AHB2_GRP1_PERIPH_OTG_HS ||
						LL_AHB2_GRP1_PERIPH_USBPHY);
#else
	LL_AHB1_GRP1_DisableClockLowPower(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
#endif /* defined(CONFIG_SOC_SERIES_STM32H7X) */

#if USB_OTG_HS_EMB_PHY
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
#endif
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
	/* The USB2 controller only works in FS mode, but the ULPI clock needs
	 * to be disabled in sleep mode for it to work.
	 */
	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);
#endif /* USB_OTG_HS_ULPI_PHY */

	return 0;
}

static int priv_clock_disable(void)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_off(clk, (clock_control_subsys_t *)&pclken[0]) != 0) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) && defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_USBPHY);
#endif

	return 0;
}

static struct udc_ep_config ep_cfg_in[DT_INST_PROP(0, num_bidir_endpoints)];
static struct udc_ep_config ep_cfg_out[DT_INST_PROP(0, num_bidir_endpoints)];

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *usb_pcfg =
					PINCTRL_DT_INST_DEV_CONFIG_GET(0);

#if USB_OTG_HS_ULPI_PHY
static const struct gpio_dt_spec ulpi_reset =
	GPIO_DT_SPEC_GET_OR(DT_PHANDLE(DT_INST(0, st_stm32_otghs), phys), reset_gpios, {0});
#endif

static int udc_stm32_driver_init0(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	struct udc_data *data = dev->data;
	int err;

	for (unsigned int i = 0; i < ARRAY_SIZE(ep_cfg_out); i++) {
		ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			ep_cfg_out[i].caps.control = 1;
			ep_cfg_out[i].caps.mps = cfg->ep0_mps;
		} else {
			ep_cfg_out[i].caps.bulk = 1;
			ep_cfg_out[i].caps.interrupt = 1;
			ep_cfg_out[i].caps.iso = 1;
			ep_cfg_out[i].caps.mps = cfg->ep_mps;
		}

		ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(ep_cfg_in); i++) {
		ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			ep_cfg_in[i].caps.control = 1;
			ep_cfg_in[i].caps.mps = cfg->ep0_mps;
		} else {
			ep_cfg_in[i].caps.bulk = 1;
			ep_cfg_in[i].caps.interrupt = 1;
			ep_cfg_in[i].caps.iso = 1;
			ep_cfg_in[i].caps.mps = 1023;
		}

		ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.out_ack = false;
	data->caps.mps0 = UDC_MPS0_64;

	priv->dev = dev;
	priv->irq = UDC_STM32_IRQ;
	priv->clk_enable = priv_clock_enable;
	priv->clk_disable = priv_clock_disable;
	priv->pcd_prepare = priv_pcd_prepare;

	IRQ_CONNECT(UDC_STM32_IRQ, UDC_STM32_IRQ_PRI, udc_stm32_irq,
		    DEVICE_DT_INST_GET(0), 0);

	err = pinctrl_apply_state(usb_pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", err);
		return err;
	}

#ifdef SYSCFG_CFGR1_USB_IT_RMP
	/*
	 * STM32F302/F303: USB IRQ collides with CAN_1 IRQ (§14.1.3, RM0316)
	 * Remap IRQ by default to enable use of both IPs simultaneoulsy
	 * This should be done before calling any HAL function
	 */
	if (LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG)) {
		LL_SYSCFG_EnableRemapIT_USB();
	} else {
		LOG_ERR("System Configuration Controller clock is "
			"disabled. Unable to enable IRQ remapping.");
	}
#endif

#if USB_OTG_HS_ULPI_PHY
	if (ulpi_reset.port != NULL) {
		if (!gpio_is_ready_dt(&ulpi_reset)) {
			LOG_ERR("Reset GPIO device not ready");
			return -EINVAL;
		}
		if (gpio_pin_configure_dt(&ulpi_reset, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}
#endif

	/*cd
	 * Required for at least STM32L4 devices as they electrically
	 * isolate USB features from VDDUSB. It must be enabled before
	 * USB can function. Refer to section 5.1.3 in DM00083560 or
	 * DM00310109.
	 */
#ifdef PWR_CR2_USV
#if defined(LL_APB1_GRP1_PERIPH_PWR)
	if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR)) {
		LL_PWR_EnableVddUSB();
	} else {
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_PWR_EnableVddUSB();
		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
	}
	#else
	LL_PWR_EnableVddUSB();
#endif /* defined(LL_APB1_GRP1_PERIPH_PWR) */
#endif /* PWR_CR2_USV */

	return 0;
}

DEVICE_DT_INST_DEFINE(0, udc_stm32_driver_init0, NULL, &udc0_data, &udc0_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &udc_stm32_api);
