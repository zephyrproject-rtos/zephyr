/*
 * Copyright (C) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_usb3

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns3, CONFIG_UDC_DRIVER_LOG_LEVEL);

#include "udc_common.h"
#include "udc_cdns3.h"

K_HEAP_DEFINE(cdns3_pool, CONFIG_UDC_CDNS3_POOL_SIZE);

/* Include vendor quirks system */
#include "udc_cdns3_vendor_quirks.h"

/* MMIO access macros. These should be defined after including quirks */
#define DEV_CFG(dev)  ((const struct udc_cdns3_config *)(dev)->config)
#define DEV_DATA(dev) ((struct udc_cdns3_data *)udc_get_private(dev))

static inline struct udc_cdns3_otg_regs *get_otg_regs(const struct device *dev)
{
	return (struct udc_cdns3_otg_regs *)DEVICE_MMIO_NAMED_GET(dev, otg);
}

static inline struct udc_cdns3_device_regs *get_dev_regs(const struct device *dev)
{
	return (struct udc_cdns3_device_regs *)DEVICE_MMIO_NAMED_GET(dev, device);
}

/* Select current EP */
static inline void cdns3_select_ep(const struct device *dev, uint8_t ep_addr)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	dev_regs->EP_SEL = ep_addr;
}

/* Notify that TRB is ready - alternative to EP_CMD_DRDY */
static inline void cdns3_ring_doorbell(const struct device *dev, uint8_t ep_addr)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	dev_regs->DRBL = BIT(CDNS3_TO_IRQ_IDX(ep_addr));
}

/*
 * TRB manipulation helpers
 */
static inline void cdns3_trb_set_buffer(struct cdns3_trb *trb, void *buf)
{
	trb->buffer = sys_cpu_to_le32((uint32_t)buf);
}

static inline void cdns3_trb_set_control(struct cdns3_trb *trb, uint32_t control)
{
	trb->control = sys_cpu_to_le32(control);
}

static inline uint32_t cdns3_trb_get_control(struct cdns3_trb *trb)
{
	return sys_le32_to_cpu(trb->control);
}

static inline void cdns3_trb_set_length(struct cdns3_trb *trb, uint32_t length)
{
	trb->length = sys_cpu_to_le32((length & TRB_LEN_MASK));
}

static inline uint32_t cdns3_trb_get_length(struct cdns3_trb *trb)
{
	return sys_le32_to_cpu(trb->length) & TRB_LEN_MASK;
}

/*
 * Transfer ring management
 */

static inline void cdns3_update_link_trb_control(struct udc_cdns3_ep_data *ep_data)
{
	struct cdns3_trb *trb = &ep_data->trb_pool[CONFIG_UDC_CDNS3_NUM_TRBS - 1];
	uint32_t control = TRB_TYPE(TRB_LINK) | TRB_CHAIN | TRB_TOGGLE;

	if (ep_data->pcs) {
		control |= TRB_CYCLE;
	}

	cdns3_trb_set_buffer(trb, ep_data->trb_pool);
	cdns3_trb_set_length(trb, 0);
	cdns3_trb_set_control(trb, control);

	sys_cache_data_flush_range(trb, sizeof(struct cdns3_trb));
}

int cdns3_ep_init_ring(const struct device *dev, struct udc_cdns3_ep_data *ep_data)
{
	size_t ring_size = CONFIG_UDC_CDNS3_NUM_TRBS * sizeof(struct cdns3_trb);

	ep_data->trb_pool = k_heap_alloc(&cdns3_pool, ring_size, K_NO_WAIT);
	if (!ep_data->trb_pool) {
		LOG_ERR("Failed to allocate TRB ring for ep %d", ep_data->cfg.addr);
		return -ENOMEM;
	}
	memset(ep_data->trb_pool, 0, ring_size);
	sys_cache_data_flush_range(ep_data->trb_pool, ring_size);

	/* Allocate buffer mapping array */
	ep_data->trb_buffers = k_heap_alloc(
		&cdns3_pool, CONFIG_UDC_CDNS3_NUM_TRBS * sizeof(struct net_buf *), K_NO_WAIT);
	if (!ep_data->trb_buffers) {
		LOG_ERR("Failed to allocate TRB buffer mapping for ep %d", ep_data->cfg.addr);
		k_heap_free(&cdns3_pool, ep_data->trb_pool);
		return -ENOMEM;
	}
	memset(ep_data->trb_buffers, 0, CONFIG_UDC_CDNS3_NUM_TRBS * sizeof(struct net_buf *));

	/* Initialize ring state */
	ep_data->enqueue = 0;
	ep_data->dequeue = 0;
	ep_data->pcs = true;
	ep_data->ccs = true;
	ep_data->free_trbs = CONFIG_UDC_CDNS3_NUM_TRBS - 1;

	cdns3_update_link_trb_control(ep_data);

	LOG_DBG("Initialized TRB ring for ep 0x%02x: %d TRBs at %p", ep_data->cfg.addr,
		CONFIG_UDC_CDNS3_NUM_TRBS, (void *)ep_data->trb_pool);

	return 0;
}

void cdns3_ep_free_ring(const struct device *dev, struct udc_cdns3_ep_data *ep_data)
{
	if (ep_data->trb_buffers) {
		k_heap_free(&cdns3_pool, ep_data->trb_buffers);
		ep_data->trb_buffers = NULL;
	}

	if (ep_data->trb_pool) {
		k_heap_free(&cdns3_pool, ep_data->trb_pool);
		ep_data->trb_pool = NULL;
	}
}

static inline bool cdns3_ring_empty(struct udc_cdns3_ep_data *ep_data)
{
	return ep_data->free_trbs == CONFIG_UDC_CDNS3_NUM_TRBS - 1;
}

static inline bool cdns3_ring_full(struct udc_cdns3_ep_data *ep_data)
{
	return ep_data->free_trbs == 0;
}

static inline void cdns3_ring_inc_enq(struct udc_cdns3_ep_data *ep_data)
{

	ep_data->enqueue++;
	if (ep_data->enqueue == (CONFIG_UDC_CDNS3_NUM_TRBS - 1)) {
		cdns3_update_link_trb_control(ep_data);
		ep_data->enqueue = 0;
		ep_data->pcs ^= true;
	}
	ep_data->free_trbs--;
}

static inline void cdns3_ring_inc_deq(struct udc_cdns3_ep_data *ep_data)
{
	ep_data->dequeue++;
	if (ep_data->dequeue == (CONFIG_UDC_CDNS3_NUM_TRBS - 1)) {
		ep_data->dequeue = 0;
		ep_data->ccs ^= true;
	}
	ep_data->free_trbs++;
}

void cdns3_ep_program_trb(const struct device *dev, struct cdns3_trb *trb, struct net_buf *buf,
			  uint8_t ep_addr, bool pcs)
{
	uint32_t control;
	bool is_in = USB_EP_DIR_IS_IN(ep_addr);
	uint32_t length = is_in ? buf->len : net_buf_tailroom(buf);

	control = TRB_TYPE(TRB_NORMAL) | TRB_IOC | TRB_ISP;
	if (pcs) {
		control |= TRB_CYCLE;
	}

	/* Configure TRB */
	cdns3_trb_set_buffer(trb, buf->data);
	cdns3_trb_set_length(trb, length);
	cdns3_trb_set_control(trb, control);

	/* Flush TRB */
	sys_cache_data_flush_range(trb, sizeof(struct cdns3_trb));

	if (is_in) {
		/* Flush data if IN (Device -> Host) */
		sys_cache_data_flush_range(buf->data, length);
	}
}

int cdns3_ep_retrieve_trb(const struct device *dev, struct cdns3_trb *trb, struct net_buf *buf,
			  uint8_t ep_addr, bool ccs)
{
	uint32_t control;
	bool is_in = USB_EP_DIR_IS_IN(ep_addr);

	/* Invalidate TRB */
	sys_cache_data_invd_range(trb, sizeof(struct cdns3_trb));

	control = cdns3_trb_get_control(trb);
	if ((control & TRB_CYCLE) != ccs) {
		return -EIO;
	}

	if (buf != NULL) {
		/* Invalidate data if OUT (Host -> Device) */
		buf->len = cdns3_trb_get_length(trb);
		if (!is_in) {
			sys_cache_data_invd_range(buf->data, buf->len);
		}
	}

	return 0;
}

int cdns3_ep_enqueue_trb(const struct device *dev, struct udc_cdns3_ep_data *ep_data,
			 struct net_buf *buf)
{
	struct cdns3_trb *trb;

	if (cdns3_ring_full(ep_data)) {
		LOG_WRN("TRB ring full for ep 0x%02x", ep_data->cfg.addr);
		return -ENOSPC;
	}

	trb = &ep_data->trb_pool[ep_data->enqueue];

	cdns3_ep_program_trb(dev, trb, buf, ep_data->cfg.addr, ep_data->pcs);

	ep_data->trb_buffers[ep_data->enqueue] = buf;

	/* Advance ring */
	cdns3_ring_inc_enq(ep_data);

	LOG_DBG("Enqueued TRB for ep 0x%02x: buf=0x%x len=%d", ep_data->cfg.addr,
		(uint32_t)buf->data, trb->length);

	return 0;
}

struct net_buf *cdns3_ep_dequeue_trb(const struct device *dev, struct udc_cdns3_ep_data *ep_data)
{
	struct cdns3_trb *trb;
	struct net_buf *buf;
	int ret;

	if (cdns3_ring_empty(ep_data)) {
		return NULL;
	}

	trb = &ep_data->trb_pool[ep_data->dequeue];

	/* Retrieve stored buffer */
	buf = ep_data->trb_buffers[ep_data->dequeue];

	ret = cdns3_ep_retrieve_trb(dev, trb, buf, ep_data->cfg.addr, ep_data->ccs);
	if (ret != 0) {
		return NULL;
	}

	/* update store buffer to NULL */
	ep_data->trb_buffers[ep_data->dequeue] = NULL;

	/* Advance dequeue pointer */
	cdns3_ring_inc_deq(ep_data);

	LOG_DBG("Dequeued TRB for ep 0x%02x: buf=%p len=%d", ep_data->cfg.addr, buf, buf->len);

	return buf;
}

/*
 * Endpoint operations
 */

static int cdns3_ep_enable(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	uint32_t ep_cfg_val;
	int ret;

	/* Select endpoint for configuration */
	cdns3_select_ep(dev, ep_cfg->addr);

	/* Build endpoint configuration */
	ep_cfg_val = EP_CFG_ENABLE;

	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_cfg_val |= EP_CFG_EPTYPE(EP_CFG_EPTYPE_CTRL);
		break;
	case USB_EP_TYPE_ISO:
		ep_cfg_val |= EP_CFG_EPTYPE(EP_CFG_EPTYPE_ISOC);
		break;
	case USB_EP_TYPE_BULK:
		ep_cfg_val |= EP_CFG_EPTYPE(EP_CFG_EPTYPE_BULK);
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_cfg_val |= EP_CFG_EPTYPE(EP_CFG_EPTYPE_INT);
		break;
	default:
		LOG_ERR("Invalid endpoint type");
		return -EINVAL;
	}

	ep_cfg_val |= EP_CFG_MAXPKTSIZE(ep_cfg->mps);
	ep_cfg_val |= EP_CFG_MAXBURST(0); /* Single packet per microframe */

	/* Configure endpoint */
	dev_regs->EP_CFG = ep_cfg_val;

	if (USB_EP_GET_IDX(ep_cfg->addr) != 0) {
		struct udc_cdns3_ep_data *ep_data =
			CONTAINER_OF(ep_cfg, struct udc_cdns3_ep_data, cfg);

		/* Initialize transfer ring */
		ret = cdns3_ep_init_ring(dev, ep_data);
		if (ret != 0) {
			return ret;
		}

		/* Configure transfer ring address */
		dev_regs->EP_TRADDR = (uint32_t)(uintptr_t)ep_data->trb_pool;
	}

	/* Enable endpoint status events */
	dev_regs->EP_STS_EN = EP_STS_EN_SETUPEN | EP_STS_EN_DESCMISEN | EP_STS_EN_TRBERREN;

	/* Enable endpoint interrupt */
	dev_regs->EP_IEN |= BIT(CDNS3_TO_IRQ_IDX(ep_cfg->addr));

	LOG_INF("Enabled endpoint 0x%02x mps %d type %d", ep_cfg->addr, ep_cfg->mps,
		ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK);

	return 0;
}

static int cdns3_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	/* Disable endpoint interrupt */
	dev_regs->EP_IEN &= ~BIT(CDNS3_TO_IRQ_IDX(ep_cfg->addr));

	/* Select and disable endpoint */
	cdns3_select_ep(dev, ep_cfg->addr);
	dev_regs->EP_CFG = 0;
	dev_regs->EP_STS_EN = 0;

	if (USB_EP_GET_IDX(ep_cfg->addr) != 0) {
		struct udc_cdns3_ep_data *ep_data =
			CONTAINER_OF(ep_cfg, struct udc_cdns3_ep_data, cfg);

		k_work_cancel(&ep_data->work);

		/* Free transfer ring */
		cdns3_ep_free_ring(dev, ep_data);
	}

	LOG_INF("Disabled endpoint 0x%02x", ep_cfg->addr);

	return 0;
}

static int cdns3_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	/* Select endpoint and set stall */
	cdns3_select_ep(dev, cfg->addr);

	dev_regs->EP_CMD = EP_CMD_SSTALL;

	LOG_DBG("Set halt on endpoint 0x%02x", cfg->addr);

	return 0;
}

static int cdns3_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	/* Select endpoint and clear stall */
	cdns3_select_ep(dev, cfg->addr);

	dev_regs->EP_CMD = EP_CMD_CSTALL;

	LOG_DBG("Cleared halt on endpoint 0x%02x", cfg->addr);

	return 0;
}

static void cdns3_ep0_enqueue_trb(const struct device *dev, struct net_buf *buf, uint8_t ep0_addr)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	struct udc_cdns3_data *priv = udc_get_private(dev);
	struct cdns3_trb *trb = &priv->ep0_trb;

	cdns3_ep_program_trb(dev, trb, buf, ep0_addr, true);

	cdns3_select_ep(dev, ep0_addr);
	dev_regs->EP_TRADDR = (uint32_t)(uintptr_t)trb;
}

static struct net_buf *cdns3_ep0_dequeue_trb(const struct device *dev, uint8_t ep0_addr)
{
	struct udc_ep_config *ep0_out_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_cdns3_data *priv = udc_get_private(dev);
	struct cdns3_trb *trb = &priv->ep0_trb;
	struct net_buf *buf = udc_buf_peek(ep0_out_cfg);

	if (buf == NULL) {
		return NULL;
	}

	(void)cdns3_ep_retrieve_trb(dev, trb, buf, ep0_addr, true);

	return buf;
}

static void cdns3_ep0_enqueue_setup(const struct device *dev, struct net_buf *buf)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	cdns3_ep0_enqueue_trb(dev, buf, USB_CONTROL_EP_OUT);
	dev_regs->EP_CMD = EP_CMD_DRDY;
}

static void cdns3_ep0_enqueue_data(const struct device *dev, struct net_buf *buf, uint8_t ep0_addr)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	cdns3_ep0_enqueue_trb(dev, buf, ep0_addr);
	dev_regs->EP_CMD = EP_CMD_DRDY | EP_CMD_ERDY;
}

static void cdns3_ep0_handle_status(const struct device *dev, struct net_buf *buf)
{
	struct udc_ep_config *ep0_out_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	struct net_buf *next;

	udc_buf_get(ep0_out_cfg);

	next = udc_buf_peek(ep0_out_cfg);
	if (next != NULL && udc_get_buf_info(next)->setup) {
		cdns3_ep0_enqueue_setup(dev, next);
	}

	dev_regs->EP_CMD = EP_CMD_REQ_CMPL | EP_CMD_ERDY;
	udc_submit_ep_event(dev, buf, 0);
}

static void cdns3_ep0_process_next(const struct device *dev)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf = udc_buf_peek(ep_cfg);
	struct udc_buf_info *bi;

	if (buf == NULL) {
		return;
	}

	bi = udc_get_buf_info(buf);

	if (bi->status) {
		cdns3_ep0_handle_status(dev, buf);
	} else if (bi->setup) {
		cdns3_ep0_enqueue_setup(dev, buf);
	} else if (bi->data) {
		cdns3_ep0_enqueue_data(dev, buf, bi->ep);
	}
}

static void cdns3_ep_worker(struct k_work *work)
{
	struct udc_cdns3_ep_data *ep_data = CONTAINER_OF(work, struct udc_cdns3_ep_data, work);
	const struct device *dev = ep_data->dev;
	struct net_buf *buf;
	int ret;

	/* Process queued buffers */
	while ((buf = udc_buf_peek(&ep_data->cfg)) != NULL) {
		ret = cdns3_ep_enqueue_trb(dev, ep_data, buf);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue TRB: %d", ret);
			udc_submit_ep_event(dev, buf, ret);
			continue;
		}

		udc_buf_get(&ep_data->cfg);
	}

	/* Ring doorbell if we have active transfers */
	if (ep_data->enqueue != ep_data->dequeue) {
		cdns3_ring_doorbell(dev, ep_data->cfg.addr);
	}
}

static int cdns3_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
			    struct net_buf *buf)
{
	if (USB_EP_GET_IDX(ep_cfg->addr) == 0) {
		struct udc_ep_config *ep0_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		bool is_free = udc_buf_peek(ep0_cfg) == NULL;

		/* queue everything to EP OUT to maintain synchronicity */
		udc_buf_put(ep0_cfg, buf);

		if (is_free) {
			cdns3_ep0_process_next(dev);
		}
	} else {
		struct udc_cdns3_ep_data *ep_data =
			CONTAINER_OF(ep_cfg, struct udc_cdns3_ep_data, cfg);

		/* Schedule work to process the queue */
		udc_buf_put(ep_cfg, buf);
		k_work_submit(&ep_data->work);
	}

	return 0;
}

static int cdns3_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	udc_ep_cancel_queued(dev, ep_cfg);

	return 0;
}

static enum udc_bus_speed cdns3_device_speed(const struct device *dev)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	uint32_t speed;

	speed = USB_STS_USBSSPD(dev_regs->USB_STS);

	switch (speed) {
	case USB_STS_SPD_FS:
		return UDC_BUS_SPEED_FS;
	case USB_STS_SPD_HS:
		return UDC_BUS_SPEED_HS;
	case USB_STS_SPD_SS:
		return UDC_BUS_SPEED_SS;
	default:
		return UDC_BUS_UNKNOWN;
	}
}

static inline int cdns3_set_address(const struct device *dev, uint8_t addr)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);

	dev_regs->USB_CMD = USB_CMD_FADDR(addr) | USB_CMD_SET_ADDR;

	LOG_DBG("Set address to %d", addr);

	return 0;
}

static int cdns3_enable(const struct device *dev)
{
	const struct udc_cdns3_config *config = dev->config;
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	struct udc_cdns3_data *priv = udc_get_private(dev);
	int ret;

	ret = udc_cdns3_quirks_enable(dev);
	if (ret != 0) {
		return ret;
	}

	/* Enable device */
	dev_regs->USB_CONF = USB_CONF_DEVEN;

	/* Reset bookkept variables */
	priv->wait_for_setup = false;

	/* Enable interrupts */
	config->irq_enable_func(dev);

	LOG_INF("CDNS3 controller enabled");

	return 0;
}

static int cdns3_disable(const struct device *dev)
{
	const struct udc_cdns3_config *config = dev->config;
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	int ret;

	/* Disable interrupts */
	config->irq_disable_func(dev);

	/* Disable device */
	dev_regs->USB_CONF = USB_CONF_DEVDS;

	/* Quirks-specific disable sequence */
	ret = udc_cdns3_quirks_disable(dev);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("CDNS3 controller disabled");

	return 0;
}

static int cdns3_init(const struct device *dev)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	struct udc_cdns3_otg_regs *otg_regs = get_otg_regs(dev);
	struct udc_cdns3_data *priv = udc_get_private(dev);
	uint32_t dev_ver;
	int ret;

	/* Wait for OTG/DRD controller ready */
	ret = WAIT_FOR(((otg_regs->OTGSTS & CDNS3_OTGSTS_OTG_NRDY) == 0),
		       CDNS3_OTG_READY_TIMEOUT_US, NULL);
	if (ret == 0) {
		LOG_ERR("Timeout waiting for OTG_NRDY to clear (OTGSTS=0x%08x)", otg_regs->OTGSTS);
		return -ETIMEDOUT;
	}

	ret = WAIT_FOR(((otg_regs->OTGSTS & CDNS3_OTGSTS_DEV_READY)) != 0,
		       CDNS3_DEV_READY_TIMEOUT_US, NULL);
	if (ret == 0) {
		LOG_ERR("Timeout waiting for DEV_READY (OTGSTS=0x%08x)", otg_regs->OTGSTS);
		return -ETIMEDOUT;
	}

	priv->dev = dev;

	/* Read device version */
	dev_ver = dev_regs->USB_CAP6;

	LOG_INF("CDNS3 controller version: 0x%08x", dev_ver);

	if (dev_ver != CDNS3_DEV_VER_V3) {
		LOG_WRN("Version 0x%08x is not supported yet, you may encounter some problems",
			dev_ver);
	}

	/* Software reset */
	dev_regs->USB_CONF = USB_CONF_SWRST;

	/* Clear configuration reset */
	dev_regs->USB_CONF = USB_CONF_CFGRST;

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				     priv->out_ep0_cfg.caps.mps, 0);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable EP0 OUT: %d", ret);
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     priv->in_ep0_cfg.caps.mps, 0);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable EP0 IN: %d", ret);
		return ret;
	}

	/* Enable core interrupts */
	dev_regs->USB_IEN = USB_ISTS_INIT;

	/* Quirks-specific initialization */
	ret = udc_cdns3_quirks_init(dev);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("CDNS3 controller initialized");

	return 0;
}

static int cdns3_shutdown(const struct device *dev)
{
	int ret;

	/* Disable controller */
	cdns3_disable(dev);

	/* Disable EP0 */
	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	if (ret != 0) {
		LOG_ERR("Failed to disable EP0 OUT");
		return ret;
	}

	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);
	if (ret != 0) {
		LOG_ERR("Failed to disable EP0 IN");
		return ret;
	}

	/* Quirks-specific shutdown */
	ret = udc_cdns3_quirks_shutdown(dev);
	if (ret != 0) {
		LOG_WRN("Quirks shutdown failed: %d", ret);
	}

	LOG_INF("CDNS3 controller shutdown");

	return 0;
}

static void cdns3_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void cdns3_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

void cdns3_handle_usb_events(const struct device *dev, uint32_t usb_ists)
{
	struct udc_cdns3_data *priv = udc_get_private(dev);

	if (usb_ists & (USB_ISTS_U2RESI | USB_ISTS_UWRESI | USB_ISTS_UHRESI)) {
		struct udc_ep_config *ep0_out_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
		struct net_buf *ep0_out_next = udc_buf_peek(ep0_out_cfg);

		LOG_DBG("USB Reset event");

		/* Reset bookkept variables */
		priv->wait_for_setup = false;

		/* Configuration reset */
		dev_regs->USB_CONF = USB_CONF_CFGRST;

		/* If setup is prepared at this point, inform the controller  */
		if (ep0_out_next != NULL && udc_get_buf_info(ep0_out_next)->setup) {
			cdns3_ring_doorbell(dev, USB_CONTROL_EP_OUT);
		}

		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if (usb_ists & USB_ISTS_CONI) {
		LOG_DBG("USB Connect event");
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
	}

	if (usb_ists & USB_ISTS_DISI) {
		LOG_DBG("USB Disconnect event");
		udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
	}

	if (usb_ists & (USB_ISTS_U3ENTI | USB_ISTS_L2ENTI)) {
		LOG_DBG("USB Suspend event");
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if (usb_ists & (USB_ISTS_U3EXTI | USB_ISTS_L1EXTI | USB_ISTS_L2EXTI)) {
		LOG_DBG("USB Resume event");
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}
}

void cdns3_handle_ep(const struct device *dev, uint8_t ep_addr, int32_t ep_sts)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	struct udc_cdns3_ep_data *ep_data = CONTAINER_OF(ep_cfg, struct udc_cdns3_ep_data, cfg);
	struct net_buf *buf;

	/* Handle transfer completion first (most common case) */
	if ((ep_sts & EP_STS_IOC) || (ep_sts & EP_STS_ISP)) {
		LOG_DBG("Transfer complete on EP 0x%02x (IOC:%d ISP:%d)", ep_data->cfg.addr,
			!!(ep_sts & EP_STS_IOC), !!(ep_sts & EP_STS_ISP));

		buf = cdns3_ep_dequeue_trb(dev, ep_data);
		if (buf != NULL) {
			udc_submit_ep_event(dev, buf, 0);
		}

		k_work_submit(&ep_data->work);
	} else if (ep_sts & EP_STS_TRBERR) {
		LOG_ERR("TRB error on EP 0x%02x", ep_data->cfg.addr);

		buf = cdns3_ep_dequeue_trb(dev, ep_data);
		if (buf != NULL) {
			udc_submit_ep_event(dev, buf, -EIO);
		}

		k_work_submit(&ep_data->work);
	} else if (ep_sts & EP_STS_DESCMIS) {
		LOG_ERR("Descriptor missing on EP 0x%02x", ep_data->cfg.addr);

		k_work_submit(&ep_data->work);
	}

	if (ep_sts & EP_STS_STALL) {
		LOG_DBG("EP 0x%02x is stalled", ep_data->cfg.addr);
	}
}

void cdns3_handle_ep0(const struct device *dev, uint8_t ep0_addr, uint32_t ep_sts)
{
	struct udc_ep_config *ep0_out_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_cdns3_data *priv = udc_get_private(dev);
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	struct net_buf *buf;

	if (ep_sts & EP_STS_SETUP) {
		priv->wait_for_setup = true;
	}

	if ((ep_sts & EP_STS_IOC) || (ep_sts & EP_STS_ISP)) {
		if (priv->wait_for_setup) {
			buf = cdns3_ep0_dequeue_trb(dev, USB_CONTROL_EP_OUT);
			if (buf != NULL) {
				struct usb_setup_packet *packet =
					(struct usb_setup_packet *)buf->data;

				if (packet->bRequest == USB_SREQ_SET_CONFIGURATION &&
				    packet->wValue != 0) {
					dev_regs->USB_CONF = USB_CONF_CFGSET;
					k_usleep(1000);
				}

				/* Submit setup buffer */
				udc_setup_received(dev, NULL);

				priv->wait_for_setup = false;
			}
		} else {
			/* Data received */
			buf = cdns3_ep0_dequeue_trb(dev, ep0_addr);
			if (buf != NULL) {
				udc_buf_get(ep0_out_cfg);
				udc_submit_ep_event(dev, buf, 0);

				/* process deferred status buffer */
				cdns3_ep0_process_next(dev);
			}
		}
	}
}

void cdns3_handle_ep_events(const struct device *dev, uint32_t ep_ists)
{
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	uint32_t ep_sts;

	while (ep_ists) {
		uint8_t ep_idx = find_lsb_set(ep_ists) - 1;
		uint8_t ep_addr = CDNS3_FROM_IRQ_IDX(ep_idx);

		/* Select endpoint */
		cdns3_select_ep(dev, ep_addr);

		/* Read endpoint status */
		ep_sts = dev_regs->EP_STS;

		/* Clear status bits */
		dev_regs->EP_STS = ep_sts;

		if (USB_EP_GET_IDX(ep_addr) == 0) {
			cdns3_handle_ep0(dev, ep_addr, ep_sts);
		} else {
			cdns3_handle_ep(dev, ep_addr, ep_sts);
		}

		ep_ists &= ~BIT(ep_idx);
	}
}

/*
 * Interrupt handling
 */

static void cdns3_irq_handler(const struct device *dev)
{
	struct udc_cdns3_data *priv = udc_get_private(dev);
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	uint32_t usb_ists;
	bool do_work = false;

	/* Read and clear interrupt status */

	usb_ists = dev_regs->USB_ISTS;
	if (usb_ists != 0) {
		dev_regs->USB_IEN &= ~usb_ists;
		do_work = true;
	}

	if (dev_regs->EP_ISTS != 0) {
		/* Disable all endpoint interrupts to prevent barrage */
		dev_regs->EP_IEN = 0;
		do_work = true;
	}

	if (do_work) {
		k_work_schedule(&priv->irq_work, K_NO_WAIT);
	}
}

static void cdns3_irq_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct udc_cdns3_data *priv = CONTAINER_OF(dwork, struct udc_cdns3_data, irq_work);
	const struct device *dev = priv->dev;
	struct udc_cdns3_device_regs *dev_regs = get_dev_regs(dev);
	uint32_t ep_ists;
	uint32_t usb_ists;

	/* Process USB events */
	usb_ists = dev_regs->USB_ISTS;
	if (usb_ists != 0) {
		dev_regs->USB_ISTS = usb_ists;
		dev_regs->USB_IEN = USB_ISTS_INIT;

		/* Process USB events */
		cdns3_handle_usb_events(dev, usb_ists);
	}

	ep_ists = dev_regs->EP_ISTS;
	if (ep_ists != 0) {
		/* Process per-endpoint events */
		cdns3_handle_ep_events(dev, ep_ists);

		/* Re-enable EP interrupts */
		dev_regs->EP_IEN = ~0;
	}
}

static const struct udc_api udc_cdns3_api = {
	.lock = cdns3_lock,
	.unlock = cdns3_unlock,
	.device_speed = cdns3_device_speed,
	.init = cdns3_init,
	.enable = cdns3_enable,
	.disable = cdns3_disable,
	.shutdown = cdns3_shutdown,
	.set_address = cdns3_set_address,
	.ep_enable = cdns3_ep_enable,
	.ep_disable = cdns3_ep_disable,
	.ep_set_halt = cdns3_ep_set_halt,
	.ep_clear_halt = cdns3_ep_clear_halt,
	.ep_enqueue = cdns3_ep_enqueue,
	.ep_dequeue = cdns3_ep_dequeue,
};

static int udc_cdns3_driver_preinit(const struct device *dev)
{
	const struct udc_cdns3_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_cdns3_data *priv = udc_get_private(dev);
	uint16_t mps = 0;
	uint16_t mps0 = 0;
	int ret;
	int i;

	DEVICE_MMIO_NAMED_MAP(dev, otg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, device, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state (%d)", ret);
		return ret;
	}

	/* Quirks-specific pre-initialization */
	ret = udc_cdns3_quirks_preinit(dev);
	if (ret != 0) {
		return ret;
	}

	k_mutex_init(&data->mutex);
	k_work_init_delayable(&priv->irq_work, cdns3_irq_worker);

	switch (config->max_speed) {
	case UDC_CDNS3_SPEED_IDX_HIGH_SPEED:
		data->caps.hs = true;
		data->caps.mps0 = UDC_MPS0_64;
		mps = 1024;
		mps0 = 64;
		break;
	case UDC_CDNS3_SPEED_IDX_FULL_SPEED:
		data->caps.mps0 = UDC_MPS0_64;
		mps = 64;
		mps0 = 64;
		break;
	default:
		LOG_ERR("Speed not implemented: %u", config->max_speed);
		return -ENOSYS;
	}

	/* Initialize EP0 IN */
	priv->in_ep0_cfg.addr = USB_CONTROL_EP_IN;
	priv->in_ep0_cfg.caps.in = 1;
	priv->in_ep0_cfg.caps.out = 1;
	priv->in_ep0_cfg.caps.mps = mps0;
	priv->in_ep0_cfg.caps.control = 1;

	ret = udc_register_ep(dev, &priv->in_ep0_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to register EP0 IN %d", ret);
		return ret;
	}

	/* Initialize IN endpoints */
	for (i = 0; i < config->num_in_endpoints; i++) {
		priv->in_ep_data[i].dev = dev;
		priv->in_ep_data[i].cfg.addr = USB_EP_DIR_IN | (i + 1);
		priv->in_ep_data[i].cfg.caps.in = 1;
		priv->in_ep_data[i].cfg.caps.mps = mps;
		priv->in_ep_data[i].cfg.caps.bulk = 1;
		priv->in_ep_data[i].cfg.caps.interrupt = 1;
		priv->in_ep_data[i].cfg.caps.iso = 1;

		k_work_init(&priv->in_ep_data[i].work, cdns3_ep_worker);

		ret = udc_register_ep(dev, &priv->in_ep_data[i].cfg);
		if (ret != 0) {
			LOG_ERR("Failed to register EP 0x%02x %d", priv->in_ep_data[i].cfg.addr,
				ret);
			return ret;
		}
	}

	/* Initialize EP0 OUT */
	priv->out_ep0_cfg.addr = USB_CONTROL_EP_OUT;
	priv->out_ep0_cfg.caps.out = 1;
	priv->out_ep0_cfg.caps.in = 1;
	priv->out_ep0_cfg.caps.mps = mps0;
	priv->out_ep0_cfg.caps.control = 1;

	ret = udc_register_ep(dev, &priv->out_ep0_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to register EP0 OUT %d", ret);
		return ret;
	}

	/* Initialize OUT endpoints (including EP0) */
	for (i = 0; i < config->num_out_endpoints; i++) {
		priv->out_ep_data[i].dev = dev;
		priv->out_ep_data[i].cfg.addr = USB_EP_DIR_OUT | (i + 1);
		priv->out_ep_data[i].cfg.caps.out = 1;
		priv->out_ep_data[i].cfg.caps.mps = mps;
		priv->out_ep_data[i].cfg.caps.bulk = 1;
		priv->out_ep_data[i].cfg.caps.interrupt = 1;
		priv->out_ep_data[i].cfg.caps.iso = 1;

		k_work_init(&priv->out_ep_data[i].work, cdns3_ep_worker);

		ret = udc_register_ep(dev, &priv->out_ep_data[i].cfg);
		if (ret != 0) {
			LOG_ERR("Failed to register EP 0x%02x", priv->out_ep_data[i].cfg.addr);
			return ret;
		}
	}

	data->caps.rwup = true;
	data->caps.addr_before_status = true;

	return 0;
}

/*
 * Device instantiation macro
 */
#define UDC_CDNS3_DEVICE_DEFINE(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void udc_cdns3_irq_enable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns3_irq_handler,          \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void udc_cdns3_irq_disable_func_##n(const struct device *dev)                       \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
                                                                                                   \
	static struct udc_cdns3_ep_data                                                            \
		udc_cdns3_ep_data_in_##n[DT_INST_PROP(n, num_in_endpoints)];                       \
	static struct udc_cdns3_ep_data                                                            \
		udc_cdns3_ep_data_out_##n[DT_INST_PROP(n, num_out_endpoints)];                     \
                                                                                                   \
	static const struct udc_cdns3_config udc_cdns3_config_##n = {                              \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(otg, DT_DRV_INST(n)),                           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(device, DT_DRV_INST(n)),                        \
		.irq_enable_func = udc_cdns3_irq_enable_func_##n,                                  \
		.irq_disable_func = udc_cdns3_irq_disable_func_##n,                                \
		.max_speed = DT_INST_ENUM_IDX_OR(n, maximum_speed, UDC_BUS_SPEED_SS),              \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.num_in_endpoints = DT_INST_PROP(n, num_in_endpoints),                             \
		.num_out_endpoints = DT_INST_PROP(n, num_out_endpoints),                           \
	};                                                                                         \
                                                                                                   \
	static struct udc_cdns3_data udc_cdns3_data_##n = {                                        \
		.in_ep_data = udc_cdns3_ep_data_in_##n,                                            \
		.out_ep_data = udc_cdns3_ep_data_out_##n,                                          \
		.quirks = UDC_CDNS3_QUIRKS(n),                                                     \
	};                                                                                         \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_cdns3_data_##n,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_cdns3_driver_preinit, NULL, &udc_data_##n,                    \
			      &udc_cdns3_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_cdns3_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_CDNS3_DEVICE_DEFINE)
