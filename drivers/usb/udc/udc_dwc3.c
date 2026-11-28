/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Synopsys DesignWare Cores SuperSpeed USB 3.x device controller driver.
 *
 * The controller initialization, the endpoint commands and the control
 * transfer state machine follow the Linux dwc3 driver, function by function
 * and with its names, so that its errata workarounds and fixes can be
 * carried over. Where the Zephyr UDC model differs, the differences are
 * noted where they are handled.
 */

#define DT_DRV_COMPAT snps_dwc3

#include "udc_common.h"
#include "udc_dwc3.h"

#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_dwc3, CONFIG_UDC_DRIVER_LOG_LEVEL);

enum dwc3_drv_event_type {
	/* The interrupt handler masked the event buffer interrupt */
	DWC3_DRV_EVT_IRQ,
	/* New buffers are queued on physical endpoints in xfer_new */
	DWC3_DRV_EVT_XFER,
};

/* Size of the bounce buffer, the largest endpoint packet */
#define DWC3_BOUNCE_SIZE       1024
/* Time to wait for a control transfer to finish before a disconnect, in ms */
#define DWC3_PULL_UP_TIMEOUT   500
/* Polls of an endpoint command, as in Linux */
#define DWC3_EP_CMD_TIMEOUT    5000
/* Polls of the run/stop state, 1 ms apart, as in Linux */
#define DWC3_RUN_STOP_TIMEOUT  2000
#define DWC3_EVENT_BUFFER_SIZE CONFIG_UDC_DWC3_EVENT_BUFFER_SIZE

/* Revision checks of Linux, see dwc3_ver_within() */
#define DWC3_IP_IS(priv, _ip) ((priv)->ip == USB_##_ip##_IP)
#define DWC3_VER_IS(priv, _ip, _ver)                                                               \
	(DWC3_IP_IS(priv, _ip) && (priv)->revision == USB_##_ip##_REVISION_##_ver)
#define DWC3_VER_IS_PRIOR(priv, _ip, _ver)                                                         \
	(DWC3_IP_IS(priv, _ip) && (priv)->revision < USB_##_ip##_REVISION_##_ver)
#define DWC3_VER_IS_WITHIN(priv, _ip, _from, _to)                                                  \
	(DWC3_IP_IS(priv, _ip) && (priv)->revision >= USB_##_ip##_REVISION_##_from &&              \
	 (!(USB_##_ip##_REVISION_##_to) || (priv)->revision <= USB_##_ip##_REVISION_##_to))

static inline mm_reg_t dwc3_base(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, base);
}

static inline uint32_t dwc3_readl(const struct device *dev, uint32_t offset)
{
	return sys_read32(dwc3_base(dev) + offset);
}

static inline void dwc3_writel(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, dwc3_base(dev) + offset);
}

/*
 * TRBs, the event buffer and the SETUP buffer are in non-cacheable memory when
 * the system has it, and are kept coherent by hand otherwise.
 */
#define DWC3_DESC_CACHED (IS_ENABLED(CONFIG_DCACHE) && !IS_ENABLED(CONFIG_NOCACHE_MEMORY))

static inline void dwc3_desc_flush(void *addr, size_t size)
{
	if (DWC3_DESC_CACHED) {
		sys_cache_data_flush_range(addr, size);
	}
}

static inline void dwc3_desc_invd(void *addr, size_t size)
{
	if (DWC3_DESC_CACHED) {
		sys_cache_data_invd_range(addr, size);
	}
}

/* Memory is identity mapped for DMA */
static inline uint64_t dwc3_dma_addr(const void *addr)
{
	return (uint64_t)(uintptr_t)addr;
}

static inline uint8_t dwc3_ep_number(const uint8_t ep)
{
	return (USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1 : 0);
}

static inline struct dwc3_ep *dwc3_get_ep(const struct device *dev, const uint8_t ep)
{
	return &DEV_CFG(dev)->eps[dwc3_ep_number(ep)];
}

static inline struct udc_ep_config *dwc3_ep0_cfg(const struct device *dev, const bool in)
{
	return udc_get_ep_cfg(dev, in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT);
}

static int dwc3_quirk_call(const struct device *dev, int (*fn)(const struct device *dev))
{
	return fn != NULL ? fn(dev) : 0;
}

#define DWC3_QUIRK(dev, fname)                                                                     \
	(DEV_CFG(dev)->quirks != NULL ? dwc3_quirk_call(dev, DEV_CFG(dev)->quirks->fname) : 0)

static bool dwc3_has_imod(const struct udc_dwc3_data *priv)
{
	return DWC3_VER_IS_WITHIN(priv, DWC3, 300A, ANY) ||
	       DWC3_VER_IS_WITHIN(priv, DWC31, 120A, ANY) || DWC3_IP_IS(priv, DWC32);
}

static uint32_t dwc3_mdwidth(const struct udc_dwc3_data *priv)
{
	uint32_t mdwidth = USB_DWC3_GHWPARAMS0_MDWIDTH(priv->hwparams[0]);

	if (DWC3_IP_IS(priv, DWC32)) {
		mdwidth += USB_DWC3_GHWPARAMS6_MDWIDTH(priv->hwparams[6]);
	}

	return mdwidth;
}

/* ------------------------------------------------------------------------- */
/* Device and endpoint commands                                              */
/* ------------------------------------------------------------------------- */

static void dwc3_gadget_dctl_write_safe(const struct device *dev, uint32_t value)
{
	/* A read-modify-write of DCTL must not request a link state change */
	value &= ~USB_DWC3_DCTL_ULSTCHNGREQ_MASK;
	dwc3_writel(dev, USB_DWC3_DCTL, value);
}

static int dwc3_gadget_set_link_state(const struct device *dev, uint8_t state)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	int retries = 10000;
	uint32_t reg;

	/* Wait until the device controller is ready, 1.94a and later */
	if (!DWC3_VER_IS_PRIOR(priv, DWC3, 194A)) {
		while (--retries) {
			reg = dwc3_readl(dev, USB_DWC3_DSTS);
			if (!(reg & USB_DWC3_DSTS_DCNRD)) {
				break;
			}

			k_busy_wait(5);
		}

		if (retries <= 0) {
			return -ETIMEDOUT;
		}
	}

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	reg &= ~USB_DWC3_DCTL_ULSTCHNGREQ_MASK;

	/* No action first, then the requested state */
	dwc3_writel(dev, USB_DWC3_DCTL, reg);
	reg |= USB_DWC3_DCTL_ULSTCHNGREQ(state);
	dwc3_writel(dev, USB_DWC3_DCTL, reg);

	if (!DWC3_VER_IS_PRIOR(priv, DWC3, 194A)) {
		return 0;
	}

	retries = 10000;
	while (--retries) {
		reg = dwc3_readl(dev, USB_DWC3_DSTS);
		if (USB_DWC3_DSTS_USBLNKST(reg) == state) {
			return 0;
		}

		k_busy_wait(5);
	}

	return -ETIMEDOUT;
}

static void dwc3_gadget_enable_linksts_evts(const struct device *dev, bool set)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	/* Before 2.50a link state change events are always enabled */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 250A)) {
		return;
	}

	reg = dwc3_readl(dev, USB_DWC3_DEVTEN);
	if (set) {
		reg |= USB_DWC3_DEVTEN_ULSTCNGEN;
	} else {
		reg &= ~USB_DWC3_DEVTEN_ULSTCNGEN;
	}

	dwc3_writel(dev, USB_DWC3_DEVTEN, reg);
	priv->linksts_evts = set;
}

struct dwc3_ep_cmd_params {
	uint32_t param0;
	uint32_t param1;
	uint32_t param2;
};

static int dwc3_send_gadget_ep_cmd(const struct device *dev, struct dwc3_ep *dep, uint32_t cmd,
				   const struct dwc3_ep_cmd_params *params)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t timeout = DWC3_EP_CMD_TIMEOUT;
	uint32_t saved_config = 0;
	uint8_t epnum = dep->number;
	int cmd_status = 0;
	int ret = -EINVAL;
	uint32_t reg;

	/*
	 * At USB 2.0 speeds, and for End Transfer, GUSB2PHYCFG.SUSPHY and
	 * ENBLSLPM must be cleared while an endpoint command is issued.
	 */
	if (priv->speed <= DWC3_SPEED_HIGH ||
	    USB_DWC3_DEPCMD_CMD(cmd) == USB_DWC3_DEPCMD_ENDTRANSFER) {
		reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));
		if (reg & USB_DWC3_GUSB2PHYCFG_SUSPHY) {
			saved_config |= USB_DWC3_GUSB2PHYCFG_SUSPHY;
			reg &= ~USB_DWC3_GUSB2PHYCFG_SUSPHY;
		}

		if (reg & USB_DWC3_GUSB2PHYCFG_ENBLSLPM) {
			saved_config |= USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
			reg &= ~USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
		}

		if (saved_config) {
			dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
		}
	}

	/* Update Transfer ignores the parameters, writing them only costs time */
	if (USB_DWC3_DEPCMD_CMD(cmd) != USB_DWC3_DEPCMD_UPDATETRANSFER) {
		dwc3_writel(dev, USB_DWC3_DEPCMDPAR0(epnum), params->param0);
		dwc3_writel(dev, USB_DWC3_DEPCMDPAR1(epnum), params->param1);
		dwc3_writel(dev, USB_DWC3_DEPCMDPAR2(epnum), params->param2);
	}

	/*
	 * Update Transfer of a non-isochronous endpoint is issued as a "No
	 * Response Update Transfer", with CmdAct and CmdIOC cleared, so that
	 * there is no completion to wait for. Control endpoints never use it.
	 */
	if (USB_DWC3_DEPCMD_CMD(cmd) == USB_DWC3_DEPCMD_UPDATETRANSFER &&
	    (dep->cfg == NULL || dep->cfg->attributes != USB_EP_TYPE_ISO)) {
		cmd &= ~(USB_DWC3_DEPCMD_CMDIOC | USB_DWC3_DEPCMD_CMDACT);
	} else {
		cmd |= USB_DWC3_DEPCMD_CMDACT;
	}

	/* Commands must see the TRBs written before them */
	barrier_dmem_fence_full();
	dwc3_writel(dev, USB_DWC3_DEPCMD(epnum), cmd);

	if (!(cmd & USB_DWC3_DEPCMD_CMDACT) ||
	    (USB_DWC3_DEPCMD_CMD(cmd) == USB_DWC3_DEPCMD_ENDTRANSFER &&
	     !(cmd & USB_DWC3_DEPCMD_CMDIOC))) {
		ret = 0;
		goto skip_status;
	}

	do {
		reg = dwc3_readl(dev, USB_DWC3_DEPCMD(epnum));
		if (!(reg & USB_DWC3_DEPCMD_CMDACT)) {
			cmd_status = USB_DWC3_DEPCMD_STATUS(reg);

			switch (cmd_status) {
			case 0:
				ret = 0;
				break;
			case USB_DWC3_DEPEVT_TRANSFER_NO_RESOURCE:
				LOG_ERR("No resource for physical endpoint %u", epnum);
				ret = -EINVAL;
				break;
			case USB_DWC3_DEPEVT_TRANSFER_BUS_EXPIRY:
				ret = -EAGAIN;
				break;
			default:
				LOG_ERR("Unknown command status %d", cmd_status);
			}

			break;
		}

		k_busy_wait(1);
	} while (--timeout);

	if (timeout == 0) {
		ret = -ETIMEDOUT;
		cmd_status = -ETIMEDOUT;
	}

skip_status:
	LOG_DBG("ep %u cmd 0x%x params %08x %08x %08x status %d", epnum, cmd, params->param0,
		params->param1, params->param2, cmd_status);

	if (USB_DWC3_DEPCMD_CMD(cmd) == USB_DWC3_DEPCMD_STARTTRANSFER) {
		if (ret == 0) {
			dep->flags |= DWC3_EP_TRANSFER_STARTED;
		}

		if (ret != -ETIMEDOUT) {
			reg = dwc3_readl(dev, USB_DWC3_DEPCMD(epnum));
			dep->resource_index = USB_DWC3_DEPCMD_GET_RSC_IDX(reg);
		}
	}

	if (USB_DWC3_DEPCMD_CMD(cmd) == USB_DWC3_DEPCMD_ENDTRANSFER &&
	    !(cmd & USB_DWC3_DEPCMD_CMDIOC)) {
		k_busy_wait(1000);
	}

	if (saved_config) {
		reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));
		reg |= saved_config;
		dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
	}

	return ret;
}

static int dwc3_send_clear_stall_ep_cmd(const struct device *dev, struct dwc3_ep *dep)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	const struct dwc3_ep_cmd_params params = {0};
	uint32_t cmd = USB_DWC3_DEPCMD_CLEARSTALL;

	/* Since 2.60a, ClearPendIN goes with Clear Stall on SuperSpeed IN */
	if ((dep->number & 1) && !DWC3_VER_IS_PRIOR(priv, DWC3, 260A) &&
	    priv->speed >= DWC3_SPEED_SUPER) {
		cmd |= USB_DWC3_DEPCMD_CLEARPENDIN;
	}

	return dwc3_send_gadget_ep_cmd(dev, dep, cmd, &params);
}

static int dwc3_gadget_start_config(const struct device *dev, unsigned int resource_index)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	const struct dwc3_ep_cmd_params params = {0};
	uint32_t cmd;
	int ret;

	if (resource_index != 0 && resource_index != 2) {
		return -EINVAL;
	}

	cmd = USB_DWC3_DEPCMD_DEPSTARTCFG | USB_DWC3_DEPCMD_PARAM(resource_index);
	ret = dwc3_send_gadget_ep_cmd(dev, &config->eps[0], cmd, &params);
	if (ret != 0) {
		return ret;
	}

	for (unsigned int i = resource_index; i < MIN(priv->num_eps, 2 * config->num_of_eps); i++) {
		config->eps[i].flags &= ~DWC3_EP_RESOURCE_ALLOCATED;
	}

	return 0;
}

static int dwc3_gadget_set_ep_config(const struct device *dev, struct dwc3_ep *dep, uint32_t action)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = dep->cfg;
	struct dwc3_ep_cmd_params params = {0};
	uint8_t type = cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	params.param0 = USB_DWC3_DEPCFG_EP_TYPE(type) |
			USB_DWC3_DEPCFG_MAX_PACKET_SIZE(udc_mps_ep_size(cfg));

	/* Burst size is only needed in SuperSpeed mode */
	if (priv->speed >= DWC3_SPEED_SUPER) {
		/* No bMaxBurst from the stack yet, a single packet burst */
		params.param0 |= USB_DWC3_DEPCFG_BURST_SIZE(0);
	}

	params.param0 |= action;

	if (type == USB_EP_TYPE_CONTROL) {
		params.param1 = USB_DWC3_DEPCFG_XFER_COMPLETE_EN;
	}

	if (dep->number <= 1 || type == USB_EP_TYPE_ISO) {
		params.param1 |= USB_DWC3_DEPCFG_XFER_NOT_READY_EN;
	}

	if (type != USB_EP_TYPE_CONTROL) {
		params.param1 |= USB_DWC3_DEPCFG_XFER_IN_PROGRESS_EN;
	}

	/* Physical endpoint n maps to logical endpoint n */
	params.param1 |= USB_DWC3_DEPCFG_EP_NUMBER(dep->number);

	/* IN endpoints use the TX FIFO of their endpoint number */
	if (dep->number & 1) {
		params.param0 |= USB_DWC3_DEPCFG_FIFO_NUMBER(dep->number >> 1);
	}

	if (cfg->interval) {
		uint8_t binterval_m1 = MIN(cfg->interval - 1, 13);

		params.param1 |= USB_DWC3_DEPCFG_BINTERVAL_M1(binterval_m1);
	}

	return dwc3_send_gadget_ep_cmd(dev, dep, USB_DWC3_DEPCMD_SETEPCONFIG, &params);
}

static int dwc3_gadget_set_xfer_resource(const struct device *dev, struct dwc3_ep *dep)
{
	struct dwc3_ep_cmd_params params = {0};
	int ret;

	if (dep->flags & DWC3_EP_RESOURCE_ALLOCATED) {
		return 0;
	}

	params.param0 = USB_DWC3_DEPXFERCFG_NUM_XFER_RES(1);
	ret = dwc3_send_gadget_ep_cmd(dev, dep, USB_DWC3_DEPCMD_SETTRANSFRESOURCE, &params);
	if (ret != 0) {
		return ret;
	}

	dep->flags |= DWC3_EP_RESOURCE_ALLOCATED;

	return 0;
}

/* ------------------------------------------------------------------------- */
/* TRB ring of a non-control endpoint                                        */
/* ------------------------------------------------------------------------- */

#define DWC3_TRB_NUM CONFIG_UDC_DWC3_TRB_NUM

static inline uint64_t dwc3_trb_dma_offset(struct dwc3_ep *dep, uint16_t index)
{
	return dwc3_dma_addr(&dep->trb_pool[index]);
}

/* The index never points to the link TRB, the last one of the ring */
static inline void dwc3_ep_inc_trb(uint16_t *index)
{
	(*index)++;
	if (*index == (DWC3_TRB_NUM - 1)) {
		*index = 0;
	}
}

static inline uint16_t dwc3_calc_trbs_left(struct dwc3_ep *dep)
{
	return (DWC3_TRB_NUM - 1) - dep->trbs_used;
}

static void dwc3_ep_ring_init(struct dwc3_ep *dep)
{
	struct usb_dwc3_trb *trb_link = &dep->trb_pool[DWC3_TRB_NUM - 1];
	uint64_t first = dwc3_trb_dma_offset(dep, 0);

	memset(dep->trb_pool, 0, sizeof(struct usb_dwc3_trb) * DWC3_TRB_NUM);
	memset(dep->trb_buf, 0, sizeof(struct net_buf *) * DWC3_TRB_NUM);

	/* The link TRB keeps its HWO bit for good */
	trb_link->bpl = (uint32_t)first;
	trb_link->bph = (uint32_t)(first >> 32);
	trb_link->ctrl = USB_DWC3_TRBCTL_LINK_TRB | USB_DWC3_TRB_CTRL_HWO;
	dwc3_desc_flush(dep->trb_pool, sizeof(struct usb_dwc3_trb) * DWC3_TRB_NUM);

	dep->trb_enqueue = 0;
	dep->trb_dequeue = 0;
	dep->trbs_used = 0;
}

/*
 * Hand all buffers of the TRB ring back to the stack and empty the ring. The
 * transfer must have been ended. Linux skips the TRBs of cancelled requests;
 * here the ring restarts from its first TRB.
 */
static void dwc3_ep_giveback_ring(const struct device *dev, struct dwc3_ep *dep, int err)
{
	while (dep->trbs_used != 0) {
		struct net_buf *buf = dep->trb_buf[dep->trb_dequeue];

		dep->trb_buf[dep->trb_dequeue] = NULL;
		dwc3_ep_inc_trb(&dep->trb_dequeue);
		dep->trbs_used--;

		/* The buffer is in the slot of each of its TRBs, report it once */
		if (buf != NULL && (dep->trbs_used == 0 || dep->trb_buf[dep->trb_dequeue] != buf)) {
			udc_submit_ep_event(dev, buf, err);
		}
	}

	dwc3_ep_ring_init(dep);
	dep->flags &= ~DWC3_EP_PENDING_CANCEL;

	if (dep->cfg != NULL) {
		udc_ep_set_busy(dep->cfg, false);
	}
}

static int __dwc3_gadget_ep_enable(const struct device *dev, struct dwc3_ep *dep, uint32_t action)
{
	uint8_t type = dep->cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;
	int ret;

	ret = dwc3_gadget_set_ep_config(dev, dep, action);
	if (ret != 0) {
		return ret;
	}

	ret = dwc3_gadget_set_xfer_resource(dev, dep);
	if (ret != 0) {
		return ret;
	}

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		uint32_t reg;

		dep->flags |= DWC3_EP_ENABLED;

		reg = dwc3_readl(dev, USB_DWC3_DALEPENA);
		reg |= USB_DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dev, USB_DWC3_DALEPENA, reg);

		dep->trb_enqueue = 0;
		dep->trb_dequeue = 0;
		dep->trbs_used = 0;

		if (type == USB_EP_TYPE_CONTROL) {
			return 0;
		}

		dwc3_ep_ring_init(dep);
	}

	/*
	 * Start a transfer on a TRB without HWO, so that new TRBs are always
	 * given to the controller with a No Response Update Transfer.
	 */
	if (type == USB_EP_TYPE_BULK || type == USB_EP_TYPE_INTERRUPT) {
		struct dwc3_ep_cmd_params params = {0};
		uint64_t trb_dma = dwc3_trb_dma_offset(dep, 0);

		params.param0 = (uint32_t)(trb_dma >> 32);
		params.param1 = (uint32_t)trb_dma;

		ret = dwc3_send_gadget_ep_cmd(dev, dep, USB_DWC3_DEPCMD_STARTTRANSFER, &params);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/*
 * End the transfer of an endpoint and wait for it, as the DEPCMD polling of a
 * DWC_usb3 3.10a or later with GUCTL2.RST_ACTBITLATER does. Earlier
 * controllers and DWC_usb31 get the ForceRM delay of Linux instead.
 */
static int __dwc3_stop_active_transfer(const struct device *dev, struct dwc3_ep *dep, bool force,
				       bool interrupt)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	const struct dwc3_ep_cmd_params params = {0};
	uint32_t cmd;
	int ret;

	cmd = USB_DWC3_DEPCMD_ENDTRANSFER;
	cmd |= force ? USB_DWC3_DEPCMD_HIPRI_FORCERM : 0;
	cmd |= interrupt ? USB_DWC3_DEPCMD_CMDIOC : 0;
	cmd |= USB_DWC3_DEPCMD_PARAM(dep->resource_index);
	ret = dwc3_send_gadget_ep_cmd(dev, dep, cmd, &params);

	/*
	 * A SETUP packet that is not moved out yet keeps the controller from
	 * completing End Transfer. Retry once the SETUP TRB is ready again.
	 */
	if (ret == -ETIMEDOUT && priv->ep0state != EP0_SETUP_PHASE) {
		dep->flags |= DWC3_EP_DELAY_STOP;
		return 0;
	}

	if (ret != 0) {
		LOG_ERR("End transfer on physical endpoint %u failed: %d", dep->number, ret);
	}

	dep->resource_index = 0;

	if (!interrupt) {
		dep->flags &= ~DWC3_EP_TRANSFER_STARTED;
	} else if (ret == 0) {
		dep->flags |= DWC3_EP_END_TRANSFER_PENDING;
	}

	dep->flags &= ~DWC3_EP_DELAY_STOP;

	return ret;
}

static void dwc3_stop_active_transfer(const struct device *dev, struct dwc3_ep *dep, bool force,
				      bool interrupt)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	/* On the control endpoint, only a started data stage is ended */
	if (dep->number <= 1 && priv->ep0state != EP0_DATA_PHASE) {
		return;
	}

	if (interrupt && (dep->flags & DWC3_EP_DELAY_STOP)) {
		return;
	}

	if (!(dep->flags & DWC3_EP_TRANSFER_STARTED) ||
	    (dep->flags & DWC3_EP_END_TRANSFER_PENDING)) {
		return;
	}

	/*
	 * While a control transfer is in progress a SETUP packet may be waiting
	 * for its TRB, and End Transfer would not complete. Delay it until the
	 * SETUP TRB is prepared.
	 */
	if (priv->ep0state != EP0_SETUP_PHASE) {
		dep->flags |= DWC3_EP_DELAY_STOP;
		return;
	}

	__dwc3_stop_active_transfer(dev, dep, force, interrupt);
}

/*
 * End the transfer and give the buffers of the TRB ring back with status.
 * Like Linux, this does not wait for the command completion event: without
 * CmdIOC the End Transfer is done once dwc3_send_gadget_ep_cmd() returns.
 */
static void dwc3_remove_requests(const struct device *dev, struct dwc3_ep *dep, int status)
{
	dwc3_stop_active_transfer(dev, dep, true, false);

	/* If the End Transfer is delayed, the buffers stay with the controller */
	if (dep->flags & DWC3_EP_DELAY_STOP) {
		dep->flags |= DWC3_EP_PENDING_CANCEL;
		return;
	}

	dwc3_ep_giveback_ring(dev, dep, status);
}

static void dwc3_stop_active_transfers(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;

	for (unsigned int i = 2; i < 2 * config->num_of_eps; i++) {
		struct dwc3_ep *dep = &config->eps[i];

		if (dep->cfg == NULL || !(dep->flags & DWC3_EP_ENABLED)) {
			continue;
		}

		dwc3_remove_requests(dev, dep, -ECONNABORTED);
	}
}

static void dwc3_clear_stall_all_ep(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;

	for (unsigned int i = 1; i < 2 * config->num_of_eps; i++) {
		struct dwc3_ep *dep = &config->eps[i];
		int ret;

		if (!(dep->flags & DWC3_EP_STALL)) {
			continue;
		}

		dep->flags &= ~DWC3_EP_STALL;
		if (dep->cfg != NULL) {
			dep->cfg->stat.halted = false;
		}

		ret = dwc3_send_clear_stall_ep_cmd(dev, dep);
		if (ret != 0) {
			LOG_ERR("Failed to clear stall on physical endpoint %u", i);
		}
	}
}

static void dwc3_ep_delayed_stop(const struct device *dev, struct dwc3_ep *dep);

/* ------------------------------------------------------------------------- */
/* Control endpoint                                                          */
/* ------------------------------------------------------------------------- */

static void dwc3_ep0_prepare_one_trb(const struct device *dev, struct dwc3_ep *dep,
				     uint64_t buf_dma, uint32_t len, uint32_t type, bool chain)
{
	struct usb_dwc3_trb *trb = &DEV_CFG(dev)->ep0_trb[dep->trb_enqueue];

	if (chain) {
		dep->trb_enqueue++;
	}

	trb->bpl = (uint32_t)buf_dma;
	trb->bph = (uint32_t)(buf_dma >> 32);
	trb->size = len;
	trb->ctrl = type | USB_DWC3_TRB_CTRL_ISP_IMI;

	if (chain) {
		trb->ctrl |= USB_DWC3_TRB_CTRL_CHN;
	} else {
		trb->ctrl |= USB_DWC3_TRB_CTRL_IOC | USB_DWC3_TRB_CTRL_LST;
	}

	/* The controller must see the whole TRB before the HWO bit */
	barrier_dmem_fence_full();
	trb->ctrl |= USB_DWC3_TRB_CTRL_HWO;
	dwc3_desc_flush(trb, sizeof(*trb));
}

static int dwc3_ep0_start_trans(const struct device *dev, struct dwc3_ep *dep)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint64_t trb_dma = dwc3_dma_addr(DEV_CFG(dev)->ep0_trb);
	struct dwc3_ep_cmd_params params = {0};
	int ret;

	if (dep->flags & DWC3_EP_TRANSFER_STARTED) {
		return 0;
	}

	params.param0 = (uint32_t)(trb_dma >> 32);
	params.param1 = (uint32_t)trb_dma;

	ret = dwc3_send_gadget_ep_cmd(dev, dep, USB_DWC3_DEPCMD_STARTTRANSFER, &params);
	if (ret < 0) {
		return ret;
	}

	priv->ep0_next_event = DWC3_EP0_COMPLETE;

	return 0;
}

static void dwc3_ep0_out_start(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct dwc3_ep *dep = &config->eps[0];
	int ret;

	dwc3_desc_invd(config->setup_buf, 8);
	dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(config->setup_buf), 8,
				 USB_DWC3_TRBCTL_CONTROL_SETUP, false);
	ret = dwc3_ep0_start_trans(dev, dep);
	if (ret < 0) {
		LOG_ERR("ep0 out start transfer failed: %d", ret);
	}

	/* Now that a SETUP packet can be moved out, end the delayed transfers */
	for (unsigned int i = 2; i < 2 * config->num_of_eps; i++) {
		struct dwc3_ep *dwc3_ep = &config->eps[i];

		if (dwc3_ep->flags & DWC3_EP_DELAY_STOP) {
			dwc3_ep_delayed_stop(dev, dwc3_ep);
		}
	}
}

/* Hand back the data or status buffer of the current control transfer */
static void dwc3_ep0_giveback(const struct device *dev, struct udc_ep_config *cfg, int err)
{
	struct net_buf *buf = udc_buf_get(cfg);

	if (buf != NULL) {
		udc_submit_ep_event(dev, buf, err);
	}
}

static int __dwc3_gadget_ep_set_halt(const struct device *dev, struct dwc3_ep *dep, bool value,
				     bool protocol);

/*
 * Protocol stall of the control endpoint, then back to the SETUP phase.
 *
 * Unlike Linux, the data and status buffers queued by the stack stay queued:
 * udc_setup_received() cancels them when the next SETUP packet arrives, and
 * the stack has queued a buffer for that SETUP packet already.
 */
static void dwc3_ep0_stall_and_restart(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep;

	/* Reinitialize physical endpoint 1 */
	dep = &config->eps[1];
	dep->flags &= DWC3_EP_RESOURCE_ALLOCATED;
	dep->flags |= DWC3_EP_ENABLED;

	/* The stall is always issued on physical endpoint 0 */
	dep = &config->eps[0];
	__dwc3_gadget_ep_set_halt(dev, dep, true, false);
	dep->flags &= DWC3_EP_RESOURCE_ALLOCATED | DWC3_EP_TRANSFER_STARTED;
	dep->flags |= DWC3_EP_ENABLED;

	priv->ep0_status_pending = false;
	config->eps[0].trb_enqueue = 0;
	config->eps[1].trb_enqueue = 0;
	priv->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dev);
}

static void dwc3_ep0_end_control_data(const struct device *dev, struct dwc3_ep *dep)
{
	const struct dwc3_ep_cmd_params params = {0};
	uint32_t cmd;
	int ret;

	/*
	 * A status or DATA OUT stage runs on physical endpoint 0, whose resource
	 * index is zero, so End Transfer is allowed for it.
	 */
	if (!dep->resource_index && dep->number) {
		return;
	}

	cmd = USB_DWC3_DEPCMD_ENDTRANSFER | USB_DWC3_DEPCMD_CMDIOC |
	      USB_DWC3_DEPCMD_PARAM(dep->resource_index);
	ret = dwc3_send_gadget_ep_cmd(dev, dep, cmd, &params);
	if (ret != 0) {
		LOG_ERR("ep0 data phase end transfer failed: %d", ret);
	}

	dep->resource_index = 0;
}

static void dwc3_ep0_reset_state(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	unsigned int dir;

	if (priv->ep0state != EP0_SETUP_PHASE) {
		dir = priv->ep0_expect_in;
		if (priv->ep0state == EP0_DATA_PHASE) {
			dwc3_ep0_end_control_data(dev, &config->eps[dir]);
		} else {
			dwc3_ep0_end_control_data(dev, &config->eps[!dir]);
		}

		config->eps[0].trb_enqueue = 0;
		config->eps[1].trb_enqueue = 0;

		dwc3_ep0_stall_and_restart(dev);
	}
}

/*
 * Start the data stage once the stack has queued its buffer. As in Linux, the
 * data stage is not delayed until XferNotReady(Data), so that a SETUP packet
 * arriving in its place is noticed.
 */
static void __dwc3_ep0_do_control_data(const struct device *dev, struct dwc3_ep *dep,
				       struct net_buf *buf)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t maxpacket = udc_mps_ep_size(dep->cfg);
	bool in = dep->number & 1;
	bool zlp = in && udc_ep_buf_has_zlp(buf);
	uint32_t length;
	int ret;

	if (in) {
		length = buf->len;
		sys_cache_data_flush_range(buf->data, buf->len);
	} else {
		length = MIN(net_buf_tailroom(buf), priv->ep0_wlength);
		sys_cache_data_invd_range(buf->data, net_buf_tailroom(buf));
	}

	if (length == 0) {
		/* An OUT TRB holds at least one packet, the bounce buffer takes it */
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(config->bounce),
					 in ? 0 : maxpacket, USB_DWC3_TRBCTL_CONTROL_DATA, false);
	} else if (!in && (length % maxpacket) != 0) {
		/* OUT transfer sizes are multiples of the packet size */
		priv->ep0_bounced = true;
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(buf->data), length,
					 USB_DWC3_TRBCTL_CONTROL_DATA, true);
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(config->bounce),
					 maxpacket - (length % maxpacket),
					 USB_DWC3_TRBCTL_CONTROL_DATA, false);
	} else if (zlp && (length % maxpacket) == 0) {
		/* An extra zero-length TRB ends the data stage with a ZLP */
		priv->ep0_bounced = true;
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(buf->data), length,
					 USB_DWC3_TRBCTL_CONTROL_DATA, true);
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(config->bounce), 0,
					 USB_DWC3_TRBCTL_CONTROL_DATA, false);
	} else {
		dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(buf->data), length,
					 USB_DWC3_TRBCTL_CONTROL_DATA, false);
	}

	ret = dwc3_ep0_start_trans(dev, dep);
	if (ret < 0) {
		LOG_ERR("ep0 data phase start transfer failed: %d", ret);
		return;
	}

	udc_ep_set_busy(dep->cfg, true);
}

static int dwc3_ep0_start_control_status(const struct device *dev, struct dwc3_ep *dep)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t type;

	type = priv->three_stage_setup ? USB_DWC3_TRBCTL_CONTROL_STATUS3
				       : USB_DWC3_TRBCTL_CONTROL_STATUS2;

	/* The status stage has no data, its buffer is not used */
	dwc3_ep0_prepare_one_trb(dev, dep, dwc3_dma_addr(DEV_CFG(dev)->bounce), 0, type, false);

	return dwc3_ep0_start_trans(dev, dep);
}

/*
 * Feed the control endpoint with the buffers the stack has queued. The stack
 * queues them ahead of the phase they belong to: Status OUT together with
 * Data IN, and the buffer for the next SETUP packet after the status buffer.
 * Each buffer is therefore only used in its own phase.
 */
static void dwc3_ep0_kick(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg;
	struct net_buf *buf;
	struct dwc3_ep *dep;

	if (priv->ep0state == EP0_DATA_PHASE) {
		dep = &config->eps[priv->ep0_expect_in];
		cfg = dep->cfg;
		buf = udc_buf_peek(cfg);

		if (buf == NULL || udc_ep_is_busy(cfg) || (dep->flags & DWC3_EP_TRANSFER_STARTED)) {
			return;
		}

		if (!udc_get_buf_info(buf)->data) {
			LOG_DBG("ep 0x%02x: no data stage buffer at the head", cfg->addr);
			return;
		}

		__dwc3_ep0_do_control_data(dev, dep, buf);
		return;
	}

	if (priv->ep0state == EP0_STATUS_PHASE && priv->ep0_status_pending) {
		dep = &config->eps[priv->ep0_status_epnum];
		cfg = dep->cfg;
		buf = udc_buf_peek(cfg);

		if (buf == NULL || !udc_get_buf_info(buf)->status) {
			return;
		}

		priv->ep0_status_pending = false;
		if (dwc3_ep0_start_control_status(dev, dep) != 0) {
			LOG_ERR("ep0 status phase start transfer failed");
			return;
		}

		udc_ep_set_busy(cfg, true);
	}
}

static void dwc3_ep0_inspect_setup(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct usb_setup_packet setup;

	dwc3_desc_invd(config->setup_buf, 8);
	memcpy(&setup, config->setup_buf, sizeof(setup));

	if (!priv->connected) {
		dwc3_ep0_stall_and_restart(dev);
		return;
	}

	priv->ep0_wlength = sys_le16_to_cpu(setup.wLength);
	priv->ep0_status_pending = false;
	priv->ep0_bounced = false;

	if (priv->ep0_wlength == 0) {
		priv->three_stage_setup = false;
		priv->ep0_expect_in = false;
		priv->ep0_next_event = DWC3_EP0_NRDY_STATUS;
	} else {
		priv->three_stage_setup = true;
		priv->ep0_expect_in = !!(setup.bmRequestType & USB_EP_DIR_IN);
		priv->ep0_next_event = DWC3_EP0_NRDY_DATA;
		priv->ep0state = EP0_DATA_PHASE;
	}

	LOG_DBG("SETUP %02x %02x %04x %04x %04x", setup.bmRequestType, setup.bRequest,
		sys_le16_to_cpu(setup.wValue), sys_le16_to_cpu(setup.wIndex), priv->ep0_wlength);

	/*
	 * Requests are handled by the stack. It cancels the buffers left from
	 * an aborted transfer, and queues the data and status buffers of this
	 * one, which dwc3_ep0_kick() picks up.
	 */
	udc_setup_received(dev, &setup);
}

static void dwc3_ep0_complete_data(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = &config->eps[priv->ep0_expect_in];
	struct usb_dwc3_trb *trb = &config->ep0_trb[0];
	struct udc_ep_config *cfg = dep->cfg;
	struct net_buf *buf;
	uint32_t transferred;
	uint32_t length;
	uint32_t status;

	priv->ep0_next_event = DWC3_EP0_NRDY_STATUS;
	udc_ep_set_busy(cfg, false);

	buf = udc_buf_peek(cfg);
	if (buf == NULL || !udc_get_buf_info(buf)->data) {
		return;
	}

	dwc3_desc_invd(trb, 2 * sizeof(*trb));
	status = USB_DWC3_TRB_SIZE_TRBSTS(trb->size);
	if (status == USB_DWC3_TRBSTS_SETUP_PENDING) {
		priv->setup_packet_pending = true;
		dwc3_ep0_giveback(dev, cfg, -ECONNRESET);
		return;
	}

	if (priv->ep0_expect_in) {
		length = buf->len;
	} else {
		length = MIN(net_buf_tailroom(buf), priv->ep0_wlength);
	}

	transferred = length - MIN(trb->size & USB_DWC3_TRB_SIZE_MASK, length);

	if (priv->ep0_bounced) {
		config->ep0_trb[1].ctrl &= ~USB_DWC3_TRB_CTRL_HWO;
		dep->trb_enqueue = 0;
		priv->ep0_bounced = false;
	}

	if (priv->ep0_expect_in && transferred < length) {
		dwc3_ep0_stall_and_restart(dev);
		return;
	}

	if (!priv->ep0_expect_in) {
		sys_cache_data_invd_range(buf->data, net_buf_tailroom(buf));
		net_buf_add(buf, transferred);
	}

	dwc3_ep0_giveback(dev, cfg, 0);
}

static void dwc3_ep0_complete_status(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = &config->eps[priv->ep0_status_epnum];
	struct usb_dwc3_trb *trb = &config->ep0_trb[0];
	struct net_buf *buf;

	udc_ep_set_busy(dep->cfg, false);
	buf = udc_buf_peek(dep->cfg);
	if (buf != NULL && udc_get_buf_info(buf)->status) {
		dwc3_ep0_giveback(dev, dep->cfg, 0);
	}

	dwc3_desc_invd(trb, sizeof(*trb));
	if (USB_DWC3_TRB_SIZE_TRBSTS(trb->size) == USB_DWC3_TRBSTS_SETUP_PENDING) {
		priv->setup_packet_pending = true;
	}

	priv->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dev);
}

static void dwc3_ep0_xfer_complete(const struct device *dev, uint8_t epnum)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = &config->eps[epnum];

	dep->flags &= ~DWC3_EP_TRANSFER_STARTED;
	dep->resource_index = 0;
	priv->setup_packet_pending = false;

	switch (priv->ep0state) {
	case EP0_SETUP_PHASE:
		dwc3_ep0_inspect_setup(dev);
		break;
	case EP0_DATA_PHASE:
		dwc3_ep0_complete_data(dev);
		break;
	case EP0_STATUS_PHASE:
		dwc3_ep0_complete_status(dev);
		break;
	default:
		LOG_ERR("Unknown ep0state %d", priv->ep0state);
	}
}

static void dwc3_ep0_xfernotready(const struct device *dev, uint8_t epnum, uint8_t status)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);

	switch (status) {
	case USB_DWC3_DEPEVT_STATUS_CONTROL_DATA:
		if (!priv->connected) {
			return;
		}

		/*
		 * The data stage starts once its buffer is queued. A request for
		 * the other direction ends it and stalls the control endpoint.
		 */
		if (priv->ep0_expect_in != epnum) {
			LOG_ERR("Unexpected direction for the data phase");
			dwc3_ep0_end_control_data(dev, &config->eps[priv->ep0_expect_in]);
			dwc3_ep0_stall_and_restart(dev);
		}

		break;
	case USB_DWC3_DEPEVT_STATUS_CONTROL_STATUS:
		if (priv->ep0_next_event != DWC3_EP0_NRDY_STATUS) {
			return;
		}

		if (priv->setup_packet_pending) {
			dwc3_ep0_stall_and_restart(dev);
			return;
		}

		/* Status stage when the stack has queued its buffer */
		priv->ep0state = EP0_STATUS_PHASE;
		priv->ep0_status_epnum = epnum;
		priv->ep0_status_pending = true;
		dwc3_ep0_kick(dev);
		break;
	default:
		break;
	}
}

static void dwc3_ep0_interrupt(const struct device *dev, uint32_t event)
{
	const struct udc_dwc3_config *config = dev->config;
	uint8_t epnum = USB_DWC3_DEPEVT_ENDPOINT_NUMBER(event);
	struct dwc3_ep *dep = &config->eps[epnum];

	switch (USB_DWC3_DEPEVT_ENDPOINT_EVENT(event)) {
	case USB_DWC3_DEPEVT_XFERCOMPLETE:
		dwc3_ep0_xfer_complete(dev, epnum);
		break;
	case USB_DWC3_DEPEVT_XFERNOTREADY:
		dwc3_ep0_xfernotready(dev, epnum, USB_DWC3_DEPEVT_STATUS(event));
		break;
	case USB_DWC3_DEPEVT_EPCMDCMPLT:
		if (USB_DWC3_DEPEVT_PARAMETER_CMD(USB_DWC3_DEPEVT_PARAMETERS(event)) ==
		    USB_DWC3_DEPCMD_ENDTRANSFER) {
			dep->flags &= ~(DWC3_EP_END_TRANSFER_PENDING | DWC3_EP_TRANSFER_STARTED);
		}
		break;
	default:
		break;
	}
}

/* ------------------------------------------------------------------------- */
/* Non-control endpoints                                                     */
/* ------------------------------------------------------------------------- */

/*
 * Prepare one TRB of a buffer. Each buffer takes one TRB, and a second one
 * pointing to the bounce buffer when an OUT length has to be rounded up to
 * the packet size or an IN transfer ends with a zero-length packet. Only the
 * last TRB of a buffer interrupts, so that every XferInProgress event
 * completes exactly one buffer, the oldest one in the ring.
 */
static void dwc3_prepare_one_trb(const struct device *dev, struct dwc3_ep *dep, struct net_buf *buf,
				 uint64_t dma, uint32_t trb_length, bool chain)
{
	struct usb_dwc3_trb *trb = &dep->trb_pool[dep->trb_enqueue];

	ARG_UNUSED(dev);

	trb->size = USB_DWC3_TRB_SIZE_LENGTH(trb_length);
	trb->bpl = (uint32_t)dma;
	trb->bph = (uint32_t)(dma >> 32);
	trb->ctrl = USB_DWC3_TRBCTL_NORMAL;

	/* Continue on short packet, so that the next OUT buffer takes over */
	if (!(dep->number & 1)) {
		trb->ctrl |= USB_DWC3_TRB_CTRL_CSP;
	}

	if (chain) {
		trb->ctrl |= USB_DWC3_TRB_CTRL_CHN;
	} else {
		trb->ctrl |= USB_DWC3_TRB_CTRL_IOC;
	}

	dep->trb_buf[dep->trb_enqueue] = buf;

	/* The controller checks HWO last, the rest of the TRB must be there */
	barrier_dmem_fence_full();
	trb->ctrl |= USB_DWC3_TRB_CTRL_HWO;
	dwc3_desc_flush(trb, sizeof(*trb));

	dwc3_ep_inc_trb(&dep->trb_enqueue);
	dep->trbs_used++;
}

/* Length the buffer is given to the controller with */
static inline uint32_t dwc3_buf_length(struct dwc3_ep *dep, struct net_buf *buf)
{
	return (dep->number & 1) ? buf->len : net_buf_tailroom(buf);
}

static bool dwc3_needs_extra_trb(struct dwc3_ep *dep, struct net_buf *buf)
{
	uint32_t maxp = udc_mps_ep_size(dep->cfg);
	uint32_t length = dwc3_buf_length(dep, buf);
	uint32_t rem = length % maxp;
	bool in = dep->number & 1;

	return (in && length && !rem && udc_ep_buf_has_zlp(buf)) || (!in && rem);
}

/*
 * Move the queued buffers the ring has room for from the endpoint FIFO into
 * the ring. Like the pending and started lists of Linux, a buffer belongs to
 * one of them only: it leaves the FIFO before its TRBs are given to the
 * controller. Returns the number of TRBs prepared.
 */
static int dwc3_prepare_trbs(const struct device *dev, struct dwc3_ep *dep)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_ep_config *cfg = dep->cfg;
	uint32_t maxp = udc_mps_ep_size(cfg);
	bool in = dep->number & 1;
	struct net_buf *buf;
	int count = 0;

	while ((buf = udc_buf_peek(cfg)) != NULL) {
		uint32_t length = dwc3_buf_length(dep, buf);
		bool extra = dwc3_needs_extra_trb(dep, buf);
		uint32_t num_trbs = extra ? 2 : 1;

		if (dwc3_calc_trbs_left(dep) < num_trbs) {
			break;
		}

		buf = udc_buf_get(cfg);

		if (in) {
			sys_cache_data_flush_range(buf->data, length);
		} else {
			sys_cache_data_invd_range(net_buf_tail(buf), length);
		}

		/* A normal TRB, unless this is an OUT buffer without room */
		if (in || length) {
			dwc3_prepare_one_trb(dev, dep, buf,
					     dwc3_dma_addr(in ? buf->data : net_buf_tail(buf)),
					     length, extra);
		}

		/* The bounce buffer takes the ZLP or the rest of the last packet */
		if ((!in && length == 0) || extra) {
			dwc3_prepare_one_trb(dev, dep, buf, dwc3_dma_addr(config->bounce),
					     in ? 0 : maxp - (length % maxp), false);
		}

		count += num_trbs;
	}

	return count;
}

static int __dwc3_gadget_kick_transfer(const struct device *dev, struct dwc3_ep *dep)
{
	struct dwc3_ep_cmd_params params = {0};
	bool starting;
	uint32_t cmd;
	int ret;

	ret = dwc3_prepare_trbs(dev, dep);
	starting = !(dep->flags & DWC3_EP_TRANSFER_STARTED);

	/* Nothing new, and no transfer to restart */
	if (ret == 0 && !starting) {
		return 0;
	}

	if (dep->trbs_used == 0) {
		return 0;
	}

	udc_ep_set_busy(dep->cfg, true);

	if (starting) {
		uint64_t trb_dma = dwc3_trb_dma_offset(dep, dep->trb_dequeue);

		params.param0 = (uint32_t)(trb_dma >> 32);
		params.param1 = (uint32_t)trb_dma;
		cmd = USB_DWC3_DEPCMD_STARTTRANSFER;
	} else {
		cmd = USB_DWC3_DEPCMD_UPDATETRANSFER | USB_DWC3_DEPCMD_PARAM(dep->resource_index);
	}

	ret = dwc3_send_gadget_ep_cmd(dev, dep, cmd, &params);
	if (ret < 0) {
		if (ret == -EAGAIN) {
			return ret;
		}

		LOG_ERR("ep 0x%02x: transfer command failed: %d", dep->cfg->addr, ret);
		dwc3_remove_requests(dev, dep, -EIO);
		return ret;
	}

	return 0;
}

/* Feed the endpoint with its queued buffers, when it may run */
static void dwc3_gadget_ep_kick(const struct device *dev, struct dwc3_ep *dep)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	if (dep->cfg == NULL || !(dep->flags & DWC3_EP_ENABLED) || !priv->connected) {
		return;
	}

	/* Start only after the End Transfer is done and the stall is cleared */
	if (dep->flags & (DWC3_EP_END_TRANSFER_PENDING | DWC3_EP_DELAY_STOP | DWC3_EP_STALL)) {
		dep->flags |= DWC3_EP_DELAY_START;
		return;
	}

	dep->flags &= ~DWC3_EP_DELAY_START;
	(void)__dwc3_gadget_kick_transfer(dev, dep);
}

/*
 * Reclaim the TRBs of the oldest buffer and give the buffer back. A short
 * packet in a chained TRB makes the controller skip to the last TRB of the
 * buffer and leave HWO set on those it skipped; it is cleared here.
 */
static void dwc3_gadget_ep_cleanup_completed_request(const struct device *dev, struct dwc3_ep *dep,
						     uint32_t event, int status)
{
	const struct udc_dwc3_config *config = dev->config;
	uint32_t ev_status = USB_DWC3_DEPEVT_STATUS(event);
	uint64_t bounce = dwc3_dma_addr(config->bounce);
	struct net_buf *buf = dep->trb_buf[dep->trb_dequeue];
	uint32_t remaining = 0;
	uint32_t length;

	if (dep->trbs_used == 0 || buf == NULL) {
		LOG_WRN("ep 0x%02x: completion without a buffer", dep->cfg->addr);
		return;
	}

	length = dwc3_buf_length(dep, buf);

	while (dep->trbs_used != 0 && dep->trb_buf[dep->trb_dequeue] == buf) {
		struct usb_dwc3_trb *trb = &dep->trb_pool[dep->trb_dequeue];
		bool last;

		dwc3_desc_invd(trb, sizeof(*trb));

		if ((trb->ctrl & USB_DWC3_TRB_CTRL_HWO) && !(trb->ctrl & USB_DWC3_TRB_CTRL_CHN) &&
		    !(ev_status & USB_DWC3_DEPEVT_STATUS_SHORT)) {
			/* Not done by the controller yet, a spurious event */
			LOG_WRN("ep 0x%02x: TRB %u still owned by the controller", dep->cfg->addr,
				dep->trb_dequeue);
			return;
		}

		trb->ctrl &= ~USB_DWC3_TRB_CTRL_HWO;
		last = !(trb->ctrl & USB_DWC3_TRB_CTRL_CHN);

		/* The bounce buffer TRB does not count in the buffer length */
		if (trb->bpl != (uint32_t)bounce || trb->bph != (uint32_t)(bounce >> 32)) {
			remaining += trb->size & USB_DWC3_TRB_SIZE_MASK;
		}

		dep->trb_buf[dep->trb_dequeue] = NULL;
		dwc3_ep_inc_trb(&dep->trb_dequeue);
		dep->trbs_used--;

		if (last) {
			break;
		}
	}

	if (!(dep->number & 1)) {
		uint32_t actual = length - MIN(remaining, length);

		sys_cache_data_invd_range(net_buf_tail(buf), length);
		net_buf_add(buf, actual);
	} else if (remaining != 0 && status == 0) {
		LOG_WRN("ep 0x%02x: %u of %u bytes not sent", dep->cfg->addr, remaining, length);
	}

	udc_submit_ep_event(dev, buf, status);
}

static void dwc3_gadget_endpoint_trbs_complete(const struct device *dev, struct dwc3_ep *dep,
					       uint32_t event, int status)
{
	dwc3_gadget_ep_cleanup_completed_request(dev, dep, event, status);

	if (dep->trbs_used == 0) {
		udc_ep_set_busy(dep->cfg, false);
	}

	if (dep->flags & DWC3_EP_END_TRANSFER_PENDING) {
		return;
	}

	/* Refill the ring from the queue */
	dwc3_gadget_ep_kick(dev, dep);
}

static void dwc3_gadget_endpoint_transfer_in_progress(const struct device *dev, struct dwc3_ep *dep,
						      uint32_t event)
{
	int status = 0;

	if (USB_DWC3_DEPEVT_STATUS(event) & USB_DWC3_DEPEVT_STATUS_BUSERR) {
		status = -ECONNRESET;
	}

	dwc3_gadget_endpoint_trbs_complete(dev, dep, event, status);
}

static void dwc3_gadget_endpoint_transfer_complete(const struct device *dev, struct dwc3_ep *dep,
						   uint32_t event)
{
	int status = 0;

	dep->flags &= ~DWC3_EP_TRANSFER_STARTED;

	if (USB_DWC3_DEPEVT_STATUS(event) & USB_DWC3_DEPEVT_STATUS_BUSERR) {
		status = -ECONNRESET;
	}

	dwc3_gadget_endpoint_trbs_complete(dev, dep, event, status);
}

static int __dwc3_gadget_ep_set_halt(const struct device *dev, struct dwc3_ep *dep, bool value,
				     bool protocol)
{
	const struct dwc3_ep_cmd_params params = {0};
	int ret;

	ARG_UNUSED(protocol);

	if (value) {
		ret = dwc3_send_gadget_ep_cmd(dev, dep, USB_DWC3_DEPCMD_SETSTALL, &params);
		if (ret != 0) {
			LOG_ERR("Failed to set stall on physical endpoint %u", dep->number);
		} else {
			dep->flags |= DWC3_EP_STALL;
		}

		return ret;
	}

	/* The controller clears the control endpoint stall on SETUP */
	if (dep->number <= 1) {
		dep->flags &= ~DWC3_EP_STALL;
		return 0;
	}

	/*
	 * As in Linux, the transfer is ended and its buffers are given back
	 * before the stall is cleared; the queued ones start afterwards.
	 */
	dwc3_remove_requests(dev, dep, -ECONNABORTED);

	if (dep->flags & (DWC3_EP_END_TRANSFER_PENDING | DWC3_EP_DELAY_STOP)) {
		dep->flags |= DWC3_EP_PENDING_CLEAR_STALL;
		return 0;
	}

	ret = dwc3_send_clear_stall_ep_cmd(dev, dep);
	if (ret != 0) {
		LOG_ERR("Failed to clear stall on physical endpoint %u", dep->number);
		return ret;
	}

	dep->flags &= ~DWC3_EP_STALL;

	return 0;
}

/*
 * Finish a stop that waited for the SETUP phase: end the transfer, give the
 * ring back if that was asked for, clear a stall that waited for it and start
 * the buffers queued in the meantime.
 */
static void dwc3_ep_delayed_stop(const struct device *dev, struct dwc3_ep *dep)
{
	dep->flags &= ~DWC3_EP_DELAY_STOP;

	if (dep->flags & DWC3_EP_TRANSFER_STARTED) {
		__dwc3_stop_active_transfer(dev, dep, true, false);
		if (dep->flags & DWC3_EP_DELAY_STOP) {
			return;
		}
	}

	if (dep->flags & DWC3_EP_PENDING_CANCEL) {
		dwc3_ep_giveback_ring(dev, dep, -ECONNABORTED);
	}

	if (dep->flags & DWC3_EP_PENDING_CLEAR_STALL) {
		dep->flags &= ~DWC3_EP_PENDING_CLEAR_STALL;
		if (dwc3_send_clear_stall_ep_cmd(dev, dep) != 0) {
			LOG_ERR("Failed to clear stall on physical endpoint %u", dep->number);
			return;
		}

		dep->flags &= ~DWC3_EP_STALL;
		if (dep->cfg != NULL) {
			dep->cfg->stat.halted = false;
		}
	}

	if (dep->flags & DWC3_EP_DELAY_START) {
		dwc3_gadget_ep_kick(dev, dep);
	}
}

static void dwc3_gadget_endpoint_command_complete(const struct device *dev, struct dwc3_ep *dep,
						  uint32_t event)
{
	uint8_t cmd = USB_DWC3_DEPEVT_PARAMETER_CMD(USB_DWC3_DEPEVT_PARAMETERS(event));

	if (cmd != USB_DWC3_DEPCMD_ENDTRANSFER || !(dep->flags & DWC3_EP_END_TRANSFER_PENDING)) {
		return;
	}

	dep->flags &= ~(DWC3_EP_END_TRANSFER_PENDING | DWC3_EP_TRANSFER_STARTED);

	if (dep->flags & DWC3_EP_PENDING_CANCEL) {
		dwc3_ep_giveback_ring(dev, dep, -ECONNABORTED);
	}

	if (dep->flags & DWC3_EP_PENDING_CLEAR_STALL) {
		dep->flags &= ~DWC3_EP_PENDING_CLEAR_STALL;
		if (dwc3_send_clear_stall_ep_cmd(dev, dep) != 0) {
			LOG_ERR("Failed to clear stall on physical endpoint %u", dep->number);
			return;
		}

		dep->flags &= ~DWC3_EP_STALL;
		if (dep->cfg != NULL) {
			dep->cfg->stat.halted = false;
		}
	}

	if (dep->flags & DWC3_EP_DELAY_START) {
		dwc3_gadget_ep_kick(dev, dep);
	}
}

static void dwc3_endpoint_interrupt(const struct device *dev, uint32_t event)
{
	const struct udc_dwc3_config *config = dev->config;
	uint8_t epnum = USB_DWC3_DEPEVT_ENDPOINT_NUMBER(event);
	struct dwc3_ep *dep;

	if (epnum >= 2 * config->num_of_eps || config->eps[epnum].cfg == NULL) {
		LOG_WRN("Spurious event, physical endpoint %u is not allocated", epnum);
		return;
	}

	dep = &config->eps[epnum];
	if (!(dep->flags & DWC3_EP_ENABLED)) {
		if (epnum > 1 && !(dep->flags & DWC3_EP_TRANSFER_STARTED)) {
			return;
		}

		/* Only command completion matters for a disabled endpoint */
		if (USB_DWC3_DEPEVT_ENDPOINT_EVENT(event) != USB_DWC3_DEPEVT_EPCMDCMPLT &&
		    !(epnum <= 1 &&
		      USB_DWC3_DEPEVT_ENDPOINT_EVENT(event) == USB_DWC3_DEPEVT_XFERCOMPLETE)) {
			return;
		}
	}

	if (epnum <= 1) {
		dwc3_ep0_interrupt(dev, event);
		return;
	}

	switch (USB_DWC3_DEPEVT_ENDPOINT_EVENT(event)) {
	case USB_DWC3_DEPEVT_XFERINPROGRESS:
		dwc3_gadget_endpoint_transfer_in_progress(dev, dep, event);
		break;
	case USB_DWC3_DEPEVT_XFERCOMPLETE:
		dwc3_gadget_endpoint_transfer_complete(dev, dep, event);
		break;
	case USB_DWC3_DEPEVT_EPCMDCMPLT:
		dwc3_gadget_endpoint_command_complete(dev, dep, event);
		break;
	case USB_DWC3_DEPEVT_XFERNOTREADY:
	case USB_DWC3_DEPEVT_STREAMEVT:
	case USB_DWC3_DEPEVT_RXTXFIFOEVT:
		break;
	default:
		LOG_ERR("Unknown endpoint event %u", USB_DWC3_DEPEVT_ENDPOINT_EVENT(event));
		break;
	}
}

/* ------------------------------------------------------------------------- */
/* Device events                                                             */
/* ------------------------------------------------------------------------- */

static void dwc3_gadget_disconnect_interrupt(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	priv->suspended = false;

	dwc3_gadget_set_link_state(dev, USB_DWC3_LINK_STATE_RX_DET);

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	reg &= ~(USB_DWC3_DCTL_INITU1ENA | USB_DWC3_DCTL_INITU2ENA);
	dwc3_gadget_dctl_write_safe(dev, reg);

	priv->connected = false;
	priv->speed = DWC3_SPEED_UNKNOWN;
	priv->setup_packet_pending = false;
	dwc3_gadget_enable_linksts_evts(dev, false);

	dwc3_ep0_reset_state(dev);

	/*
	 * Without VBUS detection the stack is not told: the next bus reset
	 * brings it back to the default state.
	 */
	LOG_INF("Disconnected");
}

static void dwc3_gadget_reset_interrupt(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	if (priv->suspended) {
		priv->suspended = false;
		udc_set_suspended(dev, false);
	}

	/* No new transfers until the transfers in progress are ended */
	priv->connected = false;

	/*
	 * Before 1.88a no Disconnect event comes when a SETUP packet is
	 * pending in the FIFO, handle the disconnect here then.
	 */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 188A) && priv->setup_packet_pending) {
		dwc3_gadget_disconnect_interrupt(dev);
	}

	/* A bus reset finds the control endpoint in the SETUP phase */
	dwc3_ep0_reset_state(dev);

	/* Active transfers end on a bus reset, databook table 4-2 */
	dwc3_stop_active_transfers(dev);
	priv->connected = true;

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	reg &= ~USB_DWC3_DCTL_TSTCTRL_MASK;
	dwc3_gadget_dctl_write_safe(dev, reg);
	dwc3_gadget_enable_linksts_evts(dev, false);
	dwc3_clear_stall_all_ep(dev);

	reg = dwc3_readl(dev, USB_DWC3_DCFG);
	reg &= ~USB_DWC3_DCFG_DEVADDR_MASK;
	dwc3_writel(dev, USB_DWC3_DCFG, reg);

	/* The stack learns about the reset with the speed, on Connect Done */
}

static void dwc3_gadget_conndone_interrupt(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	const struct dwc3_props *props = config->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint16_t mps0 = 64;
	uint32_t reg;
	uint8_t speed;
	int ret;

	reg = dwc3_readl(dev, USB_DWC3_DSTS);
	speed = reg & USB_DWC3_DSTS_CONNECTSPD;

	switch (speed) {
	case USB_DWC3_DSTS_SUPERSPEED_PLUS:
		priv->speed = DWC3_SPEED_SUPER_PLUS;
		mps0 = 512;
		break;
	case USB_DWC3_DSTS_SUPERSPEED:
		/* Before 1.90a the USB3 reset event can be missing */
		if (DWC3_VER_IS_PRIOR(priv, DWC3, 190A)) {
			dwc3_gadget_reset_interrupt(dev);
		}

		priv->speed = DWC3_SPEED_SUPER;
		mps0 = 512;
		break;
	case USB_DWC3_DSTS_HIGHSPEED:
		priv->speed = DWC3_SPEED_HIGH;
		break;
	case USB_DWC3_DSTS_FULLSPEED:
	default:
		priv->speed = DWC3_SPEED_FULL;
		break;
	}

	LOG_INF("Connected at %s speed", priv->speed >= DWC3_SPEED_SUPER  ? "super"
					 : priv->speed == DWC3_SPEED_HIGH ? "high"
									  : "full");

	/* USB 2.0 LPM */
	if (!DWC3_VER_IS_WITHIN(priv, DWC3, ANY, 194A) && !props->usb2_gadget_lpm_disable &&
	    priv->speed < DWC3_SPEED_SUPER) {
		reg = dwc3_readl(dev, USB_DWC3_DCFG);
		reg |= USB_DWC3_DCFG_LPM_CAP;
		dwc3_writel(dev, USB_DWC3_DCFG, reg);

		reg = dwc3_readl(dev, USB_DWC3_DCTL);
		reg &= ~(USB_DWC3_DCTL_HIRD_THRES_MASK | USB_DWC3_DCTL_L1_HIBER_EN);
		reg |= USB_DWC3_DCTL_HIRD_THRES(props->hird_threshold |
						(props->is_utmi_l1_suspend << 4));

		/* With the LPM Erratum, from 2.40a, NYET answers deeper BESL */
		if (props->has_lpm_erratum && !DWC3_VER_IS_PRIOR(priv, DWC3, 240A)) {
			reg &= ~USB_DWC3_DCTL_NYET_THRES_MASK;
			reg |= USB_DWC3_DCTL_NYET_THRES(props->lpm_nyet_threshold);
		}

		dwc3_gadget_dctl_write_safe(dev, reg);
	} else {
		if (props->usb2_gadget_lpm_disable) {
			reg = dwc3_readl(dev, USB_DWC3_DCFG);
			reg &= ~USB_DWC3_DCFG_LPM_CAP;
			dwc3_writel(dev, USB_DWC3_DCFG, reg);
		}

		reg = dwc3_readl(dev, USB_DWC3_DCTL);
		reg &= ~USB_DWC3_DCTL_HIRD_THRES_MASK;
		dwc3_gadget_dctl_write_safe(dev, reg);
	}

	/* Control endpoint packet size of the connection speed */
	config->eps[0].cfg->mps = mps0;
	config->eps[1].cfg->mps = mps0;

	ret = __dwc3_gadget_ep_enable(dev, &config->eps[0], USB_DWC3_DEPCFG_ACTION_MODIFY);
	if (ret == 0) {
		ret = __dwc3_gadget_ep_enable(dev, &config->eps[1], USB_DWC3_DEPCFG_ACTION_MODIFY);
	}

	if (ret != 0) {
		LOG_ERR("Failed to reconfigure the control endpoint: %d", ret);
		udc_submit_event(dev, UDC_EVT_ERROR, ret);
		return;
	}

	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void dwc3_gadget_wakeup_interrupt(const struct device *dev, uint32_t evtinfo)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	priv->link_state = evtinfo & USB_DWC3_LINK_STATE_MASK;

	if (priv->suspended) {
		priv->suspended = false;
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}
}

static void dwc3_gadget_suspend_gadget(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	if (!priv->suspended) {
		priv->suspended = true;
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}
}

static void dwc3_gadget_linksts_change_interrupt(const struct device *dev, uint32_t evtinfo)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint8_t next = evtinfo & USB_DWC3_LINK_STATE_MASK;
	uint32_t pwropt = USB_DWC3_GHWPARAMS1_EN_PWROPT(priv->hwparams[1]);

	/* Before 2.50a without hibernation, U3 to Resume is spurious */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 250A) && pwropt != USB_DWC3_GHWPARAMS1_EN_PWROPT_HIB &&
	    priv->link_state == USB_DWC3_LINK_STATE_U3 && next == USB_DWC3_LINK_STATE_RESUME) {
		return;
	}

	switch (next) {
	case USB_DWC3_LINK_STATE_U0:
		/* Remote wakeup done */
		if (priv->linksts_evts) {
			dwc3_gadget_enable_linksts_evts(dev, false);
		}

		dwc3_gadget_wakeup_interrupt(dev, evtinfo);
		break;
	case USB_DWC3_LINK_STATE_U1:
		if (priv->speed == DWC3_SPEED_SUPER) {
			dwc3_gadget_suspend_gadget(dev);
		}
		break;
	case USB_DWC3_LINK_STATE_U2:
	case USB_DWC3_LINK_STATE_U3:
		dwc3_gadget_suspend_gadget(dev);
		break;
	case USB_DWC3_LINK_STATE_RESUME:
		dwc3_gadget_wakeup_interrupt(dev, evtinfo);
		break;
	default:
		break;
	}

	priv->link_state = next;
}

static void dwc3_gadget_suspend_interrupt(const struct device *dev, uint32_t evtinfo)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint8_t next = evtinfo & USB_DWC3_LINK_STATE_MASK;

	if (next == USB_DWC3_LINK_STATE_U3) {
		dwc3_gadget_suspend_gadget(dev);
	}

	priv->link_state = next;
}

static void dwc3_gadget_interrupt(const struct device *dev, uint32_t event)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t evtinfo = USB_DWC3_DEVT_EVENT_INFO(event);

	switch (USB_DWC3_DEVT_TYPE(event)) {
	case USB_DWC3_DEVICE_EVENT_DISCONNECT:
		dwc3_gadget_disconnect_interrupt(dev);
		break;
	case USB_DWC3_DEVICE_EVENT_RESET:
		LOG_DBG("Bus reset");
		dwc3_gadget_reset_interrupt(dev);
		break;
	case USB_DWC3_DEVICE_EVENT_CONNECT_DONE:
		dwc3_gadget_conndone_interrupt(dev);
		break;
	case USB_DWC3_DEVICE_EVENT_WAKEUP:
		dwc3_gadget_wakeup_interrupt(dev, evtinfo);
		break;
	case USB_DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE:
		dwc3_gadget_linksts_change_interrupt(dev, evtinfo);
		break;
	case USB_DWC3_DEVICE_EVENT_SUSPEND:
		/* An U3/L2-L1 suspend event since 2.30a */
		if (!DWC3_VER_IS_PRIOR(priv, DWC3, 230A)) {
			dwc3_gadget_suspend_interrupt(dev, evtinfo);
		}
		break;
	case USB_DWC3_DEVICE_EVENT_SOF:
		udc_submit_sof_event(dev);
		break;
	case USB_DWC3_DEVICE_EVENT_ERRATIC_ERROR:
		LOG_WRN("Erratic error");
		break;
	case USB_DWC3_DEVICE_EVENT_OVERFLOW:
		LOG_ERR("Event buffer overflow");
		break;
	case USB_DWC3_DEVICE_EVENT_CMD_CMPL:
		break;
	default:
		LOG_WRN("Unknown device event %u", USB_DWC3_DEVT_TYPE(event));
	}
}

static void dwc3_process_event_entry(const struct device *dev, uint32_t event)
{
	if (!(event & USB_DWC3_EVENT_IS_DEVSPEC)) {
		dwc3_endpoint_interrupt(dev, event);
	} else if (USB_DWC3_EVENT_TYPE(event) == USB_DWC3_EVENT_TYPE_DEV) {
		dwc3_gadget_interrupt(dev, event);
	} else {
		LOG_ERR("Unknown event 0x%08x", event);
	}
}

/*
 * Bottom half of the interrupt. It runs with the UDC lock held, from the
 * driver thread, or from a thread that waits for the controller to halt.
 */
static void dwc3_process_event_buf(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t count;

	while (true) {
		count = dwc3_readl(dev, USB_DWC3_GEVNTCOUNT(0)) & USB_DWC3_GEVNTCOUNT_MASK;
		if (count == 0) {
			break;
		}

		if (count > DWC3_EVENT_BUFFER_SIZE) {
			LOG_ERR("Invalid event count %u", count);
			break;
		}

		for (uint32_t done = 0; done < count; done += 4) {
			uint32_t *entry = &config->evt_buf[priv->lpos / 4];

			dwc3_desc_invd(entry, sizeof(*entry));
			dwc3_process_event_entry(dev, *entry);
			priv->lpos = (priv->lpos + 4) % DWC3_EVENT_BUFFER_SIZE;
		}

		/* Give the processed entries back to the controller */
		dwc3_writel(dev, USB_DWC3_GEVNTCOUNT(0), count);
	}

	if (priv->imod_interval) {
		dwc3_writel(dev, USB_DWC3_GEVNTCOUNT(0), USB_DWC3_GEVNTCOUNT_EHB);
		dwc3_writel(dev, USB_DWC3_DEV_IMOD(0), priv->imod_interval);
	}

	/* Unmask the interrupt */
	dwc3_writel(dev, USB_DWC3_GEVNTSIZ(0), USB_DWC3_GEVNTSIZ_SIZE(DWC3_EVENT_BUFFER_SIZE));
}

static void udc_dwc3_isr_handler(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t count;

	count = dwc3_readl(dev, USB_DWC3_GEVNTCOUNT(0)) & USB_DWC3_GEVNTCOUNT_MASK;
	if (count == 0) {
		return;
	}

	/* Mask the interrupt until the driver thread has processed the events */
	dwc3_writel(dev, USB_DWC3_GEVNTSIZ(0),
		    USB_DWC3_GEVNTSIZ_INTMASK | USB_DWC3_GEVNTSIZ_SIZE(DWC3_EVENT_BUFFER_SIZE));
	k_event_post(&priv->drv_evt, BIT(DWC3_DRV_EVT_IRQ));
}

/* ------------------------------------------------------------------------- */
/* Core initialization                                                       */
/* ------------------------------------------------------------------------- */

static void dwc3_enable_susphy(const struct device *dev, bool enable)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GUSB3PIPECTL(0));
	if (enable && !props->dis_u3_susphy_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_SUSPHY;
	} else {
		reg &= ~USB_DWC3_GUSB3PIPECTL_SUSPHY;
	}

	dwc3_writel(dev, USB_DWC3_GUSB3PIPECTL(0), reg);

	reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));
	if (enable && !props->dis_u2_susphy_quirk) {
		reg |= USB_DWC3_GUSB2PHYCFG_SUSPHY;
	} else {
		reg &= ~USB_DWC3_GUSB2PHYCFG_SUSPHY;
	}

	dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
}

static void dwc3_set_prtcap(const struct device *dev, uint32_t mode)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GCTL);

	/* A dual-role controller suspends no PHY while it changes its role */
	if (USB_DWC3_GHWPARAMS0_MODE(priv->hwparams[0]) == USB_DWC3_GHWPARAMS0_MODE_DRD &&
	    USB_DWC3_GCTL_PRTCAP(reg) != mode) {
		dwc3_enable_susphy(dev, false);
	}

	reg &= ~USB_DWC3_GCTL_PRTCAPDIR(USB_DWC3_GCTL_PRTCAP_OTG);
	reg |= USB_DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_writel(dev, USB_DWC3_GCTL, reg);
}

static int dwc3_core_soft_reset(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	bool slow = DWC3_VER_IS_WITHIN(priv, DWC31, 190A, ANY) || DWC3_IP_IS(priv, DWC32);
	int retries = slow ? 10 : 1000;
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	reg |= USB_DWC3_DCTL_CSFTRST;
	reg &= ~USB_DWC3_DCTL_RUN_STOP;
	dwc3_gadget_dctl_write_safe(dev, reg);

	/* From DWC_usb31 1.90a, CSFTRST clears once the clocks are synchronized */
	do {
		reg = dwc3_readl(dev, USB_DWC3_DCTL);
		if (!(reg & USB_DWC3_DCTL_CSFTRST)) {
			/* Until 1.80a the PHY domain needs 50 ms after the reset */
			if (DWC3_VER_IS_WITHIN(priv, DWC31, ANY, 180A)) {
				k_msleep(50);
			}

			return 0;
		}

		if (slow) {
			k_msleep(20);
		} else {
			k_busy_wait(1);
		}
	} while (--retries);

	LOG_ERR("Controller soft reset failed");

	return -ETIMEDOUT;
}

static void dwc3_ss_phy_setup(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GUSB3PIPECTL(0));

	/* UX_EXIT_PX troubles some PHYs and is not for normal operation */
	reg &= ~USB_DWC3_GUSB3PIPECTL_UX_EXIT_PX;

	/* Clear SUSPHY before the PHY initialization */
	reg &= ~USB_DWC3_GUSB3PIPECTL_SUSPHY;

	if (props->u2ss_inp3_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_U2SSINP3OK;
	}

	if (props->dis_rxdet_inp3_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_DISRXDETINP3;
	}

	if (props->req_p1p2p3_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_REQP1P2P3;
	}

	if (props->del_p1p2p3_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_DEP1P2P3_EN;
	}

	if (props->del_phy_power_chg_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_DEPOCHANGE;
	}

	if (props->lfps_filter_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_LFPSFILT;
	}

	if (props->rx_detect_poll_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_RX_DETOPOLL;
	}

	if (props->tx_de_emphasis_quirk) {
		reg |= USB_DWC3_GUSB3PIPECTL_TX_DEEPH(props->tx_de_emphasis);
	}

	if (props->dis_del_phy_power_chg_quirk) {
		reg &= ~USB_DWC3_GUSB3PIPECTL_DEPOCHANGE;
	}

	dwc3_writel(dev, USB_DWC3_GUSB3PIPECTL(0), reg);
}

static void dwc3_hs_phy_setup(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));

	/* With both UTMI+ and ULPI configured, select the one asked for */
	if (USB_DWC3_GHWPARAMS3_HSPHY_IFC(priv->hwparams[3]) ==
	    USB_DWC3_GHWPARAMS3_HSPHY_IFC_UTMI_ULPI) {
		if (props->hsphy_interface == DWC3_HSPHY_INTERFACE_UTMI) {
			reg &= ~USB_DWC3_GUSB2PHYCFG_ULPI_UTMI;
		} else if (props->hsphy_interface == DWC3_HSPHY_INTERFACE_ULPI) {
			reg |= USB_DWC3_GUSB2PHYCFG_ULPI_UTMI;
			dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
		}
	}

	switch (props->hsphy_mode) {
	case DWC3_HSPHY_MODE_UTMI:
		reg &= ~(USB_DWC3_GUSB2PHYCFG_PHYIF_MASK | USB_DWC3_GUSB2PHYCFG_USBTRDTIM_MASK);
		reg |= USB_DWC3_GUSB2PHYCFG_PHYIF(USB_DWC3_UTMI_PHYIF_8_BIT) |
		       USB_DWC3_GUSB2PHYCFG_USBTRDTIM(USB_DWC3_USBTRDTIM_UTMI_8_BIT);
		break;
	case DWC3_HSPHY_MODE_UTMIW:
		reg &= ~(USB_DWC3_GUSB2PHYCFG_PHYIF_MASK | USB_DWC3_GUSB2PHYCFG_USBTRDTIM_MASK);
		reg |= USB_DWC3_GUSB2PHYCFG_PHYIF(USB_DWC3_UTMI_PHYIF_16_BIT) |
		       USB_DWC3_GUSB2PHYCFG_USBTRDTIM(USB_DWC3_USBTRDTIM_UTMI_16_BIT);
		break;
	default:
		break;
	}

	/* Clear SUSPHY before the PHY initialization */
	reg &= ~USB_DWC3_GUSB2PHYCFG_SUSPHY;

	if (props->dis_enblslpm_quirk) {
		reg &= ~USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
	} else {
		reg |= USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
	}

	if (props->dis_u2_freeclk_exists_quirk || props->gfladj_refclk_lpm_sel) {
		reg &= ~USB_DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS;
	}

	if (props->ulpi_ext_vbus_drv) {
		reg |= USB_DWC3_GUSB2PHYCFG_ULPIEXTVBUSDRV;
	}

	dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
}

static void dwc3_phy_setup(const struct device *dev)
{
	/* A device controller has a single USB 2.0 and USB 3.x port */
	dwc3_ss_phy_setup(dev);
	dwc3_hs_phy_setup(dev);
}

static void dwc3_core_setup_global_control(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t power_opt = USB_DWC3_GHWPARAMS1_EN_PWROPT(priv->hwparams[1]);
	uint32_t hw_mode = USB_DWC3_GHWPARAMS0_MODE(priv->hwparams[0]);
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GCTL);
	reg &= ~USB_DWC3_GCTL_SCALEDOWN_MASK;

	switch (power_opt) {
	case USB_DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		/*
		 * The clock gating workaround of STAR#9000588375, DWC3 2.10a
		 * to 2.50a, applies to host and dual-role operation only.
		 */
		reg &= ~USB_DWC3_GCTL_DSBLCLKGTNG;
		break;
	case USB_DWC3_GHWPARAMS1_EN_PWROPT_HIB:
		/* Device mode hibernation is not used */
		reg |= USB_DWC3_GCTL_GBLHIBERNATIONEN;
		break;
	default:
		break;
	}

	/* STAR#4846132 of DWC_usb31 2.00a, see Linux */
	if (power_opt != USB_DWC3_GHWPARAMS1_EN_PWROPT_NO &&
	    hw_mode != USB_DWC3_GHWPARAMS0_MODE_GADGET && DWC3_VER_IS(priv, DWC31, 200A)) {
		reg |= USB_DWC3_GCTL_DSBLCLKGTNG;
	}

	if (props->disable_scramble_quirk && (priv->hwparams[6] & USB_DWC3_GHWPARAMS6_EN_FPGA)) {
		reg |= USB_DWC3_GCTL_DISSCRAMBLE;
	} else {
		reg &= ~USB_DWC3_GCTL_DISSCRAMBLE;
	}

	if (props->u2exit_lfps_quirk) {
		reg |= USB_DWC3_GCTL_U2EXIT_LFPS;
	}

	/* Before 1.90a a SuperSpeed connection can fall back to a loop */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 190A)) {
		reg |= USB_DWC3_GCTL_U2RSTECN;
	}

	dwc3_writel(dev, USB_DWC3_GCTL, reg);
}

static uint32_t dwc3_clk_rate(const struct dwc3_clk *clk)
{
	uint32_t rate = 0;

	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL) || clk->dev == NULL) {
		return 0;
	}

	if (clock_control_get_rate(clk->dev, clk->subsys, &rate) != 0) {
		return 0;
	}

	return rate;
}

static void dwc3_set_power_down_clk_scale(const struct device *dev)
{
	uint32_t rate = dwc3_clk_rate(&DEV_CFG(dev)->susp_clk);
	uint32_t scale;
	uint32_t reg;

	if (rate == 0) {
		return;
	}

	/* Suspend clock periods per 16 kHz period, rounded up */
	scale = DIV_ROUND_UP(rate, 16000);
	reg = dwc3_readl(dev, USB_DWC3_GCTL);
	if ((reg & USB_DWC3_GCTL_PWRDNSCALE_MASK) < USB_DWC3_GCTL_PWRDNSCALE(scale) ||
	    (reg & USB_DWC3_GCTL_PWRDNSCALE_MASK) > USB_DWC3_GCTL_PWRDNSCALE(scale * 3)) {
		reg &= ~USB_DWC3_GCTL_PWRDNSCALE_MASK;
		reg |= USB_DWC3_GCTL_PWRDNSCALE(scale);
		dwc3_writel(dev, USB_DWC3_GCTL, reg);
	}
}

static void dwc3_frame_length_adjustment(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	if (DWC3_VER_IS_PRIOR(priv, DWC3, 250A) || props->fladj == 0) {
		return;
	}

	reg = dwc3_readl(dev, USB_DWC3_GFLADJ);
	if ((reg & USB_DWC3_GFLADJ_30MHZ_MASK) != props->fladj) {
		reg &= ~USB_DWC3_GFLADJ_30MHZ_MASK;
		reg |= USB_DWC3_GFLADJ_30MHZ_SDBND_SEL | props->fladj;
		dwc3_writel(dev, USB_DWC3_GFLADJ, reg);
	}
}

static void dwc3_ref_clk_period(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t rate = dwc3_clk_rate(&DEV_CFG(dev)->ref_clk);
	uint32_t period;
	uint32_t fladj;
	uint32_t decr;
	uint32_t reg;

	if (rate != 0) {
		period = NSEC_PER_SEC / rate;
	} else if (props->ref_clk_per != 0) {
		period = props->ref_clk_per;
		rate = NSEC_PER_SEC / period;
	} else {
		return;
	}

	reg = dwc3_readl(dev, USB_DWC3_GUCTL);
	reg &= ~USB_DWC3_GUCTL_REFCLKPER_MASK;
	reg |= FIELD_PREP(USB_DWC3_GUCTL_REFCLKPER_MASK, period);
	dwc3_writel(dev, USB_DWC3_GUCTL, reg);

	if (DWC3_VER_IS_PRIOR(priv, DWC3, 250A)) {
		return;
	}

	/*
	 * 125000 * (NSEC_PER_SEC / (rate * period) - 1), the error of the
	 * truncated period in units of 8 ppm, and the 240 MHz decrement scaled
	 * by 2 to give PLS1 as well.
	 */
	fladj = (uint32_t)((125000ULL * NSEC_PER_SEC) / ((uint64_t)rate * period));
	fladj -= 125000;
	decr = 480000000 / rate;

	reg = dwc3_readl(dev, USB_DWC3_GFLADJ);
	reg &= ~(USB_DWC3_GFLADJ_REFCLK_FLADJ_MASK | USB_DWC3_GFLADJ_240MHZDECR |
		 USB_DWC3_GFLADJ_240MHZDECR_PLS1);
	reg |= FIELD_PREP(USB_DWC3_GFLADJ_REFCLK_FLADJ_MASK, fladj);
	reg |= FIELD_PREP(USB_DWC3_GFLADJ_240MHZDECR, decr >> 1);
	reg |= FIELD_PREP(USB_DWC3_GFLADJ_240MHZDECR_PLS1, decr & 1);

	if (props->gfladj_refclk_lpm_sel) {
		reg |= USB_DWC3_GFLADJ_REFCLK_LPM_SEL;
	}

	dwc3_writel(dev, USB_DWC3_GFLADJ, reg);
}

static void dwc3_set_incr_burst_type(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	uint32_t incrx_size;
	bool undef_length;
	uint32_t cfg;

	if (props->incr_burst_len == 0) {
		return;
	}

	/* One value selects INCRx bursts, more allow undefined length ones */
	undef_length = props->incr_burst_len > 1;
	incrx_size = props->incr_burst[0];
	for (unsigned int i = 1; i < props->incr_burst_len; i++) {
		incrx_size = MAX(incrx_size, props->incr_burst[i]);
	}

	cfg = dwc3_readl(dev, USB_DWC3_GSBUSCFG0);
	cfg &= ~USB_DWC3_GSBUSCFG0_INCRBRST_MASK;
	if (undef_length) {
		cfg |= USB_DWC3_GSBUSCFG0_INCRBRSTENA;
	}

	switch (incrx_size) {
	case 256:
		cfg |= USB_DWC3_GSBUSCFG0_INCR256BRSTENA;
		break;
	case 128:
		cfg |= USB_DWC3_GSBUSCFG0_INCR128BRSTENA;
		break;
	case 64:
		cfg |= USB_DWC3_GSBUSCFG0_INCR64BRSTENA;
		break;
	case 32:
		cfg |= USB_DWC3_GSBUSCFG0_INCR32BRSTENA;
		break;
	case 16:
		cfg |= USB_DWC3_GSBUSCFG0_INCR16BRSTENA;
		break;
	case 8:
		cfg |= USB_DWC3_GSBUSCFG0_INCR8BRSTENA;
		break;
	case 4:
		cfg |= USB_DWC3_GSBUSCFG0_INCR4BRSTENA;
		break;
	case 1:
		break;
	default:
		LOG_ERR("Invalid snps,incr-burst-type-adjustment %u", incrx_size);
		break;
	}

	dwc3_writel(dev, USB_DWC3_GSBUSCFG0, cfg);
}

static void dwc3_event_buffers_setup(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint64_t dma = dwc3_dma_addr(config->evt_buf);
	uint32_t reg;

	priv->lpos = 0;
	dwc3_desc_invd(config->evt_buf, DWC3_EVENT_BUFFER_SIZE);
	dwc3_writel(dev, USB_DWC3_GEVNTADRLO(0), (uint32_t)dma);
	dwc3_writel(dev, USB_DWC3_GEVNTADRHI(0), (uint32_t)(dma >> 32));
	dwc3_writel(dev, USB_DWC3_GEVNTSIZ(0), USB_DWC3_GEVNTSIZ_SIZE(DWC3_EVENT_BUFFER_SIZE));

	/* Clear stale events */
	reg = dwc3_readl(dev, USB_DWC3_GEVNTCOUNT(0));
	dwc3_writel(dev, USB_DWC3_GEVNTCOUNT(0), reg);
}

static void dwc3_config_threshold(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	/* Both the packet count and the burst size enable a threshold */
	if (props->rx_thr_num_pkt && props->rx_max_burst) {
		reg = dwc3_readl(dev, USB_DWC3_GRXTHRCFG);
		if (DWC3_IP_IS(priv, DWC3)) {
			reg |= USB_DWC3_GRXTHRCFG_PKTCNTSEL;
			reg &= ~USB_DWC3_GRXTHRCFG_RXPKTCNT(~0);
			reg |= USB_DWC3_GRXTHRCFG_RXPKTCNT(props->rx_thr_num_pkt);
			reg &= ~USB_DWC3_GRXTHRCFG_MAXRXBURSTSIZE(~0);
			reg |= USB_DWC3_GRXTHRCFG_MAXRXBURSTSIZE(props->rx_max_burst);
		} else {
			reg |= USB_DWC31_GRXTHRCFG_PKTCNTSEL;
			reg &= ~USB_DWC31_GRXTHRCFG_RXPKTCNT(~0);
			reg |= USB_DWC31_GRXTHRCFG_RXPKTCNT(props->rx_thr_num_pkt);
			reg &= ~USB_DWC31_GRXTHRCFG_MAXRXBURSTSIZE(~0);
			reg |= USB_DWC31_GRXTHRCFG_MAXRXBURSTSIZE(props->rx_max_burst);
		}

		dwc3_writel(dev, USB_DWC3_GRXTHRCFG, reg);
	}

	if (props->tx_thr_num_pkt && props->tx_max_burst) {
		reg = dwc3_readl(dev, USB_DWC3_GTXTHRCFG);
		if (DWC3_IP_IS(priv, DWC3)) {
			reg |= USB_DWC3_GTXTHRCFG_PKTCNTSEL;
			reg &= ~USB_DWC3_GTXTHRCFG_TXPKTCNT(~0);
			reg |= USB_DWC3_GTXTHRCFG_TXPKTCNT(props->tx_thr_num_pkt);
			reg &= ~USB_DWC3_GTXTHRCFG_MAXTXBURSTSIZE(~0);
			reg |= USB_DWC3_GTXTHRCFG_MAXTXBURSTSIZE(props->tx_max_burst);
		} else {
			reg |= USB_DWC31_GTXTHRCFG_PKTCNTSEL;
			reg &= ~USB_DWC31_GTXTHRCFG_TXPKTCNT(~0);
			reg |= USB_DWC31_GTXTHRCFG_TXPKTCNT(props->tx_thr_num_pkt);
			reg &= ~USB_DWC31_GTXTHRCFG_MAXTXBURSTSIZE(~0);
			reg |= USB_DWC31_GTXTHRCFG_MAXTXBURSTSIZE(props->tx_max_burst);
		}

		dwc3_writel(dev, USB_DWC3_GTXTHRCFG, reg);
	}
}

static int dwc3_core_init(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;
	int ret;

	dwc3_phy_setup(dev);

	ret = DWC3_QUIRK(dev, phy_init);
	if (ret != 0) {
		LOG_ERR("PHY initialization failed: %d", ret);
		return ret;
	}

	ret = dwc3_core_soft_reset(dev);
	if (ret != 0) {
		goto err_exit_phy;
	}

	dwc3_core_setup_global_control(dev);
	dwc3_set_power_down_clk_scale(dev);
	dwc3_frame_length_adjustment(dev);
	dwc3_ref_clk_period(dev);
	dwc3_set_incr_burst_type(dev);

	ret = DWC3_QUIRK(dev, phy_power_on);
	if (ret != 0) {
		LOG_ERR("PHY power on failed: %d", ret);
		goto err_exit_phy;
	}

	dwc3_event_buffers_setup(dev);

	/* End Transfer completion can be polled from DWC_usb3 3.10a on */
	if (DWC3_VER_IS_WITHIN(priv, DWC3, 310A, ANY)) {
		reg = dwc3_readl(dev, USB_DWC3_GUCTL2);
		reg |= USB_DWC3_GUCTL2_RST_ACTBITLATER;
		dwc3_writel(dev, USB_DWC3_GUCTL2, reg);
	}

	/* STAR 9001285599 of DWC_usb3 3.20a, no PM timer ECN */
	if (DWC3_VER_IS(priv, DWC3, 320A)) {
		reg = dwc3_readl(dev, USB_DWC3_GUCTL2);
		reg &= ~USB_DWC3_GUCTL2_LC_TIMER;
		dwc3_writel(dev, USB_DWC3_GUCTL2, reg);
	}

	if (!DWC3_VER_IS_PRIOR(priv, DWC3, 250A)) {
		reg = dwc3_readl(dev, USB_DWC3_GUCTL1);

		/* Hardware remote wakeup from L1 */
		if (!DWC3_VER_IS_PRIOR(priv, DWC3, 290A)) {
			reg |= USB_DWC3_GUCTL1_DEV_L1_EXIT_BY_HW;
		}

		/* Only U3/L2 suspend and wakeup events, no L1 ones */
		if (!DWC3_VER_IS_PRIOR(priv, DWC3, 300A)) {
			reg |= USB_DWC3_GUCTL1_DEV_DECOUPLE_L1L2_EVT;
		}

		if (props->dis_tx_ipgap_linecheck_quirk) {
			reg |= USB_DWC3_GUCTL1_TX_IPGAP_LINECHECK_DIS;
		}

		if (props->parkmode_disable_ss_quirk) {
			reg |= USB_DWC3_GUCTL1_PARKMODE_DISABLE_SS;
		}

		if (props->parkmode_disable_hs_quirk) {
			reg |= USB_DWC3_GUCTL1_PARKMODE_DISABLE_HS;
		}

		if (DWC3_VER_IS_WITHIN(priv, DWC3, 290A, ANY)) {
			if (priv->maximum_speed == DWC3_SPEED_FULL ||
			    priv->maximum_speed == DWC3_SPEED_HIGH) {
				reg |= USB_DWC3_GUCTL1_DEV_FORCE_20_CLK_FOR_30_CLK;
			} else {
				reg &= ~USB_DWC3_GUCTL1_DEV_FORCE_20_CLK_FOR_30_CLK;
			}
		}

		dwc3_writel(dev, USB_DWC3_GUCTL1, reg);
	}

	dwc3_config_threshold(dev);

	return 0;

err_exit_phy:
	(void)DWC3_QUIRK(dev, phy_exit);

	return ret;
}

static void dwc3_check_params(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t hwparam_gen = USB_DWC3_GHWPARAMS3_SSPHY_IFC(priv->hwparams[3]);

	/* IMOD works around STAR 9000961433 of DWC_usb3 3.00a, and is harmless */
	if (dwc3_has_imod(priv)) {
		priv->imod_interval = 1;
	}

	priv->maximum_speed = props->maximum_speed;
	if (priv->maximum_speed == DWC3_SPEED_UNKNOWN) {
		switch (hwparam_gen) {
		case USB_DWC3_GHWPARAMS3_SSPHY_IFC_GEN2:
			priv->maximum_speed = DWC3_SPEED_SUPER_PLUS;
			break;
		case USB_DWC3_GHWPARAMS3_SSPHY_IFC_GEN1:
			priv->maximum_speed =
				DWC3_IP_IS(priv, DWC32) ? DWC3_SPEED_SUPER_PLUS : DWC3_SPEED_SUPER;
			break;
		case USB_DWC3_GHWPARAMS3_SSPHY_IFC_DIS:
			priv->maximum_speed = DWC3_SPEED_HIGH;
			break;
		default:
			priv->maximum_speed = DWC3_SPEED_SUPER;
			break;
		}
	}

	/* The device stack has no SuperSpeed support */
	if (priv->maximum_speed > DWC3_SPEED_HIGH) {
		LOG_INF("USB device stack runs at high speed at most");
		priv->maximum_speed = DWC3_SPEED_HIGH;
	}
}

static void __dwc3_gadget_set_speed(const struct device *dev)
{
	const struct dwc3_props *props = DEV_CFG(dev)->props;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_DCFG);
	reg &= ~USB_DWC3_DCFG_SPEED_MASK;

	/*
	 * Before 2.20a, forcing USB 2.0 mode makes Run/Stop metastable,
	 * STAR#9000525659, so the speed stays SuperSpeed.
	 */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 220A) && !props->dis_metastability_quirk) {
		reg |= USB_DWC3_DCFG_SUPERSPEED;
	} else if (priv->maximum_speed == DWC3_SPEED_FULL ||
		   priv->maximum_speed == DWC3_SPEED_LOW) {
		reg |= USB_DWC3_DCFG_FULLSPEED;
	} else {
		reg |= USB_DWC3_DCFG_HIGHSPEED;
	}

	if (DWC3_IP_IS(priv, DWC32)) {
		reg &= ~USB_DWC3_DCFG_NUMLANES(~0);
	}

	dwc3_writel(dev, USB_DWC3_DCFG, reg);
}

static int dwc3_gadget_run_stop(const struct device *dev, bool is_on)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t timeout = DWC3_RUN_STOP_TIMEOUT;
	uint32_t saved_config = 0;
	uint32_t reg;

	/* At USB 2.0 speeds SUSPHY and ENBLSLPM are cleared while it changes */
	reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));
	if (reg & USB_DWC3_GUSB2PHYCFG_SUSPHY) {
		saved_config |= USB_DWC3_GUSB2PHYCFG_SUSPHY;
		reg &= ~USB_DWC3_GUSB2PHYCFG_SUSPHY;
	}

	if (reg & USB_DWC3_GUSB2PHYCFG_ENBLSLPM) {
		saved_config |= USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
		reg &= ~USB_DWC3_GUSB2PHYCFG_ENBLSLPM;
	}

	if (saved_config) {
		dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
	}

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	if (is_on) {
		if (DWC3_VER_IS_WITHIN(priv, DWC3, ANY, 187A)) {
			reg &= ~USB_DWC3_DCTL_TRGTULST_MASK;
			reg |= USB_DWC3_DCTL_TRGTULST_RX_DET;
		}

		if (!DWC3_VER_IS_PRIOR(priv, DWC3, 194A)) {
			reg &= ~USB_DWC3_DCTL_KEEP_CONNECT;
		}

		reg |= USB_DWC3_DCTL_RUN_STOP;
		__dwc3_gadget_set_speed(dev);
		priv->pullups_connected = true;
	} else {
		reg &= ~USB_DWC3_DCTL_RUN_STOP;
		priv->pullups_connected = false;
	}

	dwc3_gadget_dctl_write_safe(dev, reg);

	/*
	 * The controller halts only once its events are acknowledged. This runs
	 * with the UDC lock held, so it processes them itself.
	 */
	do {
		k_msleep(1);
		if (!is_on) {
			dwc3_process_event_buf(dev);
		}

		reg = dwc3_readl(dev, USB_DWC3_DSTS) & USB_DWC3_DSTS_DEVCTRLHLT;
	} while (--timeout && !(!is_on ^ !reg));

	if (saved_config) {
		reg = dwc3_readl(dev, USB_DWC3_GUSB2PHYCFG(0));
		reg |= saved_config;
		dwc3_writel(dev, USB_DWC3_GUSB2PHYCFG(0), reg);
	}

	return timeout ? 0 : -ETIMEDOUT;
}

static void dwc3_gadget_enable_irq(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	/* All but Start and End of Frame events */
	reg = USB_DWC3_DEVTEN_EVNTOVERFLOWEN | USB_DWC3_DEVTEN_CMDCMPLTEN |
	      USB_DWC3_DEVTEN_ERRTICERREN | USB_DWC3_DEVTEN_WKUPEVTEN |
	      USB_DWC3_DEVTEN_CONNECTDONEEN | USB_DWC3_DEVTEN_USBRSTEN |
	      USB_DWC3_DEVTEN_DISCONNEVTEN;

	if (DWC3_VER_IS_PRIOR(priv, DWC3, 250A)) {
		reg |= USB_DWC3_DEVTEN_ULSTCNGEN;
	}

	/* From 2.30a on, the U3/L2-L1 suspend event */
	if (!DWC3_VER_IS_PRIOR(priv, DWC3, 230A)) {
		reg |= USB_DWC3_DEVTEN_U3L2L1SUSPEN;
	}

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		reg |= USB_DWC3_DEVTEN_SOFEN;
	}

	dwc3_writel(dev, USB_DWC3_DEVTEN, reg);
}

static void dwc3_gadget_setup_nump(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t ram2_depth = USB_DWC3_GHWPARAMS7_RAM2_DEPTH(priv->hwparams[7]);
	uint32_t mdwidth = dwc3_mdwidth(priv);
	uint32_t nump;
	uint32_t reg;

	/* RxFIFO size less 3 SETUP packets and 16 bytes of clock crossing */
	nump = ((ram2_depth * mdwidth / 8) - 24 - 16) / 1024;
	nump = MIN(nump, 16);

	reg = dwc3_readl(dev, USB_DWC3_DCFG);
	reg &= ~USB_DWC3_DCFG_NUMP_MASK;
	reg |= nump << USB_DWC3_DCFG_NUMP_SHIFT;
	dwc3_writel(dev, USB_DWC3_DCFG, reg);
}

static int __dwc3_gadget_start(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;
	int ret;

	if (priv->imod_interval) {
		dwc3_writel(dev, USB_DWC3_DEV_IMOD(0), priv->imod_interval);
		dwc3_writel(dev, USB_DWC3_GEVNTCOUNT(0), USB_DWC3_GEVNTCOUNT_EHB);
	} else if (dwc3_has_imod(priv)) {
		dwc3_writel(dev, USB_DWC3_DEV_IMOD(0), 0);
	}

	/* DCFG.NUMP gives the NUMP of ACK TPs, rather than the controller */
	reg = dwc3_readl(dev, USB_DWC3_GRXTHRCFG);
	if (DWC3_IP_IS(priv, DWC3)) {
		reg &= ~USB_DWC3_GRXTHRCFG_PKTCNTSEL;
	} else {
		reg &= ~USB_DWC31_GRXTHRCFG_PKTCNTSEL;
	}

	dwc3_writel(dev, USB_DWC3_GRXTHRCFG, reg);
	dwc3_gadget_setup_nump(dev);

	/* A single stream at a time */
	reg = dwc3_readl(dev, USB_DWC3_DCFG);
	reg |= USB_DWC3_DCFG_IGNSTRMPP;
	dwc3_writel(dev, USB_DWC3_DCFG, reg);

	if (priv->hwparams[9] & USB_DWC3_GHWPARAMS9_DEV_MST) {
		reg = dwc3_readl(dev, USB_DWC3_DCFG1);
		reg &= ~USB_DWC3_DCFG1_DIS_MST_ENH;
		dwc3_writel(dev, USB_DWC3_DCFG1, reg);
	}

	ret = dwc3_gadget_start_config(dev, 0);
	if (ret != 0) {
		LOG_ERR("Failed to configure the endpoints");
		return ret;
	}

	/* SuperSpeed default, Connect Done sets the speed's packet size */
	config->eps[0].cfg->mps = 512;
	config->eps[1].cfg->mps = 512;

	for (unsigned int i = 0; i < 2; i++) {
		config->eps[i].flags = 0;
		ret = __dwc3_gadget_ep_enable(dev, &config->eps[i], USB_DWC3_DEPCFG_ACTION_INIT);
		if (ret != 0) {
			LOG_ERR("Failed to enable physical endpoint %u", i);
			return ret;
		}
	}

	/* Begin to receive SETUP packets */
	priv->ep0state = EP0_SETUP_PHASE;
	priv->ep0_bounced = false;
	priv->ep0_status_pending = false;
	priv->link_state = USB_DWC3_LINK_STATE_SS_DIS;
	priv->num_active_eps = 0;
	dwc3_ep0_out_start(dev);

	dwc3_gadget_enable_irq(dev);
	dwc3_enable_susphy(dev, true);

	return 0;
}

static void __dwc3_gadget_stop(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	uint32_t reg;

	dwc3_writel(dev, USB_DWC3_DEVTEN, 0);

	for (unsigned int i = 0; i < 2; i++) {
		struct dwc3_ep *dep = &config->eps[i];

		reg = dwc3_readl(dev, USB_DWC3_DALEPENA);
		reg &= ~USB_DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dev, USB_DWC3_DALEPENA, reg);
		dep->flags &= DWC3_EP_RESOURCE_ALLOCATED;
	}
}

/* ------------------------------------------------------------------------- */
/* UDC API                                                                   */
/* ------------------------------------------------------------------------- */

static int udc_dwc3_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			       struct net_buf *buf)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	LOG_DBG("%p enqueue %p", dev, buf);
	udc_buf_put(cfg, buf);

	if (!cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, dwc3_ep_number(cfg->addr));
		k_event_post(&priv->drv_evt, BIT(DWC3_DRV_EVT_XFER));
	}

	return 0;
}

static int udc_dwc3_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct dwc3_ep *dep = dwc3_get_ep(dev, cfg->addr);

	/*
	 * The buffers in the TRB ring come back once the transfer is ended,
	 * which waits for the SETUP phase when a control transfer is running.
	 */
	if (dep->number > 1) {
		dwc3_remove_requests(dev, dep, -ECONNABORTED);
	} else {
		udc_ep_set_busy(cfg, false);
	}

	udc_ep_cancel_queued(dev, cfg);

	return 0;
}

static int udc_dwc3_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = dwc3_get_ep(dev, cfg->addr);
	int ret;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	/* The control endpoint is enabled with the controller */
	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	if (!priv->started) {
		return -EPERM;
	}

	/*
	 * Linux resets the transfer resources of the non-control endpoints on
	 * SET_CONFIGURATION, which the stack handles. Resetting them when no
	 * such endpoint is enabled does the same, as that is the state at
	 * SET_CONFIGURATION, and is harmless at SET_INTERFACE.
	 */
	if (priv->num_active_eps == 0) {
		ret = dwc3_gadget_start_config(dev, 2);
		if (ret != 0) {
			return ret;
		}
	}

	/* A stop still waiting for the SETUP phase is done now */
	if (dep->flags & DWC3_EP_DELAY_STOP) {
		dep->flags &= ~DWC3_EP_DELAY_STOP;
		__dwc3_stop_active_transfer(dev, dep, true, false);
		dwc3_ep_giveback_ring(dev, dep, -ECONNABORTED);
	}

	dep->flags &= DWC3_EP_RESOURCE_ALLOCATED;

	ret = __dwc3_gadget_ep_enable(dev, dep, USB_DWC3_DEPCFG_ACTION_INIT);
	if (ret != 0) {
		LOG_ERR("Failed to enable ep 0x%02x: %d", cfg->addr, ret);
		return ret;
	}

	priv->num_active_eps++;

	return 0;
}

static int udc_dwc3_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = dwc3_get_ep(dev, cfg->addr);
	uint32_t reg;
	uint32_t mask;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		return 0;
	}

	if (dep->flags & DWC3_EP_STALL) {
		__dwc3_gadget_ep_set_halt(dev, dep, false, false);
	}

	reg = dwc3_readl(dev, USB_DWC3_DALEPENA);
	reg &= ~USB_DWC3_DALEPENA_EP(dep->number);
	dwc3_writel(dev, USB_DWC3_DALEPENA, reg);

	dwc3_remove_requests(dev, dep, -ECONNABORTED);

	/* A delayed End Transfer is retried in the next SETUP phase */
	mask = DWC3_EP_RESOURCE_ALLOCATED;
	if (dep->flags & DWC3_EP_DELAY_STOP) {
		mask |= DWC3_EP_DELAY_STOP | DWC3_EP_TRANSFER_STARTED | DWC3_EP_PENDING_CANCEL;
	}

	dep->flags &= mask;
	priv->num_active_eps--;

	return 0;
}

static int udc_dwc3_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct dwc3_ep *dep = dwc3_get_ep(dev, cfg->addr);
	int ret;

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		/* Protocol stall, cleared by the next SETUP packet */
		dwc3_ep0_stall_and_restart(dev);
		return 0;
	}

	ret = __dwc3_gadget_ep_set_halt(dev, dep, true, false);
	if (ret == 0) {
		cfg->stat.halted = true;
	}

	return ret;
}

static int udc_dwc3_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct dwc3_ep *dep = dwc3_get_ep(dev, cfg->addr);
	int ret;

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	ret = __dwc3_gadget_ep_set_halt(dev, dep, false, false);
	if (ret != 0) {
		return ret;
	}

	cfg->stat.halted = false;

	/* Start the buffers queued while the endpoint was halted */
	atomic_set_bit(&priv->xfer_new, dep->number);
	k_event_post(&priv->drv_evt, BIT(DWC3_DRV_EVT_XFER));

	return 0;
}

static int udc_dwc3_set_address(const struct device *dev, const uint8_t addr)
{
	uint32_t reg;

	LOG_DBG("Set address %u", addr);

	reg = dwc3_readl(dev, USB_DWC3_DCFG);
	reg &= ~USB_DWC3_DCFG_DEVADDR_MASK;
	reg |= USB_DWC3_DCFG_DEVADDR(addr);
	dwc3_writel(dev, USB_DWC3_DCFG, reg);

	return 0;
}

static int udc_dwc3_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	uint32_t reg;

	switch (mode) {
	case USB_SFS_TEST_MODE_J:
	case USB_SFS_TEST_MODE_K:
	case USB_SFS_TEST_MODE_SE0_NAK:
	case USB_SFS_TEST_MODE_PACKET:
	case USB_SFS_TEST_MODE_FORCE_ENABLE:
		break;
	default:
		return -EINVAL;
	}

	if (dryrun) {
		return 0;
	}

	reg = dwc3_readl(dev, USB_DWC3_DCTL);
	reg &= ~USB_DWC3_DCTL_TSTCTRL_MASK;
	reg |= mode << 1;
	dwc3_gadget_dctl_write_safe(dev, reg);

	return 0;
}

static int udc_dwc3_host_wakeup(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;
	uint8_t link_state;
	int ret;

	/* Remote wakeup is only requested from early suspend or suspend */
	reg = dwc3_readl(dev, USB_DWC3_DSTS);
	link_state = USB_DWC3_DSTS_USBLNKST(reg);

	switch (link_state) {
	case USB_DWC3_LINK_STATE_RESET:
	case USB_DWC3_LINK_STATE_RX_DET:
	case USB_DWC3_LINK_STATE_U3:
	case USB_DWC3_LINK_STATE_U2:
	case USB_DWC3_LINK_STATE_U1:
	case USB_DWC3_LINK_STATE_RESUME:
		break;
	default:
		return -EINVAL;
	}

	/* The link state change to U0 tells when the wakeup is done */
	dwc3_gadget_enable_linksts_evts(dev, true);

	ret = dwc3_gadget_set_link_state(dev, USB_DWC3_LINK_STATE_RECOV);
	if (ret < 0) {
		LOG_ERR("Failed to put the link in recovery");
		dwc3_gadget_enable_linksts_evts(dev, false);
		return ret;
	}

	/* Recent versions write the zeroes to the link change request */
	if (DWC3_VER_IS_PRIOR(priv, DWC3, 194A)) {
		reg = dwc3_readl(dev, USB_DWC3_DCTL);
		reg &= ~USB_DWC3_DCTL_ULSTCHNGREQ_MASK;
		dwc3_writel(dev, USB_DWC3_DCTL, reg);
	}

	return 0;
}

static enum udc_bus_speed udc_dwc3_device_speed(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	switch (priv->speed) {
	case DWC3_SPEED_SUPER:
	case DWC3_SPEED_SUPER_PLUS:
		return UDC_BUS_SPEED_SS;
	case DWC3_SPEED_HIGH:
		return UDC_BUS_SPEED_HS;
	case DWC3_SPEED_FULL:
		return UDC_BUS_SPEED_FS;
	default:
		return UDC_BUS_UNKNOWN;
	}
}

static int udc_dwc3_enable(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	int ret;

	/*
	 * A reconnect after a device initiated disconnect requires a core soft
	 * reset, DWC_usb31 1.90a programming guide 4.1.9.
	 */
	if (priv->started) {
		ret = dwc3_core_soft_reset(dev);
		if (ret != 0) {
			return ret;
		}

		dwc3_event_buffers_setup(dev);
	}

	ret = __dwc3_gadget_start(dev);
	if (ret != 0) {
		return ret;
	}

	priv->started = true;
	config->irq_enable_func(dev);

	ret = dwc3_gadget_run_stop(dev, true);
	if (ret != 0) {
		LOG_ERR("Controller did not start");
		return ret;
	}

	LOG_INF("Controller started");

	return 0;
}

static int udc_dwc3_disable(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	int64_t deadline;
	int ret;

	if (!priv->pullups_connected) {
		return 0;
	}

	priv->connected = false;

	/* Active transfers end before Run/Stop is cleared, databook table 4-7 */
	dwc3_stop_active_transfers(dev);

	/*
	 * Let a control transfer in progress end and the control endpoint
	 * return to the SETUP phase, or force it there.
	 */
	deadline = k_uptime_get() + DWC3_PULL_UP_TIMEOUT;
	while (priv->ep0state != EP0_SETUP_PHASE && k_uptime_get() < deadline) {
		k_msleep(1);
		dwc3_process_event_buf(dev);
	}

	if (priv->ep0state != EP0_SETUP_PHASE) {
		LOG_WRN("Wait for SETUP phase timed out");
		dwc3_ep0_reset_state(dev);
	}

	ret = dwc3_gadget_run_stop(dev, false);

	config->irq_disable_func(dev);
	__dwc3_gadget_stop(dev);
	priv->speed = DWC3_SPEED_UNKNOWN;

	if (ret != 0) {
		LOG_ERR("Controller did not halt");
	}

	return ret;
}

static int dwc3_clk_enable(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	int ret;

	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL)) {
		return 0;
	}

	for (unsigned int i = 0; i < config->num_clks; i++) {
		if (!device_is_ready(config->clks[i].dev)) {
			return -ENODEV;
		}

		ret = clock_control_on(config->clks[i].dev, config->clks[i].subsys);
		if (ret != 0 && ret != -EALREADY) {
			return ret;
		}
	}

	return 0;
}

static int dwc3_reset_deassert(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	int ret;

	if (!IS_ENABLED(CONFIG_RESET)) {
		return 0;
	}

	for (unsigned int i = 0; i < config->num_resets; i++) {
		if (!device_is_ready(config->resets[i].dev)) {
			return -ENODEV;
		}

		ret = reset_line_deassert_dt(&config->resets[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int udc_dwc3_init(const struct device *dev)
{
	int ret;

	ret = dwc3_core_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize the core: %d", ret);
		return ret;
	}

	dwc3_set_prtcap(dev, USB_DWC3_GCTL_PRTCAP_DEVICE);

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0);
	if (ret != 0) {
		LOG_ERR("Failed to enable control endpoint");
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0);
	if (ret != 0) {
		LOG_ERR("Failed to enable control endpoint");
		return ret;
	}

	return 0;
}

static int udc_dwc3_shutdown(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT) ||
	    udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	(void)DWC3_QUIRK(dev, phy_power_off);
	(void)DWC3_QUIRK(dev, phy_exit);
	priv->started = false;

	return 0;
}

static bool dwc3_ep_is_reserved(const struct dwc3_props *props, unsigned int number)
{
	for (unsigned int i = 0; i < props->reserved_eps_len; i++) {
		if (props->reserved_eps[i] == number) {
			return true;
		}
	}

	return false;
}

/* Largest IN packet the TX FIFO of the endpoint holds, as Linux computes it */
static uint16_t dwc3_in_ep_mps(const struct device *dev, unsigned int number)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t mdwidth = dwc3_mdwidth(priv) / 8;
	uint32_t size;
	int maxpacket;

	size = dwc3_readl(dev, USB_DWC3_GTXFIFOSIZ(number >> 1));
	size = DWC3_IP_IS(priv, DWC3) ? USB_DWC3_GTXFIFOSIZ_TXFDEP(size)
				      : USB_DWC31_GTXFIFOSIZ_TXFDEP(size);

	if (DWC3_VER_IS_PRIOR(priv, DWC3, 290A)) {
		maxpacket = mdwidth * (size - 1);
	} else {
		maxpacket = mdwidth * ((size - 1) - 1) - mdwidth;
	}

	return CLAMP(maxpacket, 0, 1024);
}

/* Largest OUT packet, a third of the shared RxFIFO less SETUP packet room */
static uint16_t dwc3_out_ep_mps(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t mdwidth = dwc3_mdwidth(priv) / 8;
	int size;

	size = dwc3_readl(dev, USB_DWC3_GRXFIFOSIZ(0));
	size = DWC3_IP_IS(priv, DWC3) ? USB_DWC3_GRXFIFOSIZ_RXFDEP(size)
				      : USB_DWC31_GRXFIFOSIZ_RXFDEP(size);
	size *= mdwidth;
	size -= (3 * 8) + 16;

	return size < 0 ? 0 : MIN(size / 3, 1024);
}

static int dwc3_core_is_valid(const struct device *dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t reg;

	reg = dwc3_readl(dev, USB_DWC3_GSNPSID);
	priv->ip = USB_DWC3_GSNPS_ID(reg);

	/* DWC_usb4 controllers carry a DWC_usb32 */
	if (priv->ip == USB_DWC4_IP) {
		priv->ip = USB_DWC32_IP;
	}

	if (DWC3_IP_IS(priv, DWC3)) {
		priv->revision = reg;
	} else if (DWC3_IP_IS(priv, DWC31) || DWC3_IP_IS(priv, DWC32)) {
		priv->revision = dwc3_readl(dev, USB_DWC3_VER_NUMBER);
		priv->version_type = dwc3_readl(dev, USB_DWC3_VER_TYPE);
	} else {
		return -ENODEV;
	}

	return 0;
}

static int dwc3_register_eps(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint16_t out_mps = dwc3_out_ep_mps(dev);
	int err;

	for (unsigned int i = 0; i < config->num_of_eps; i++) {
		struct udc_ep_config *out = &config->ep_cfg_out[i];
		struct udc_ep_config *in = &config->ep_cfg_in[i];
		unsigned int out_num = 2 * i;
		unsigned int in_num = 2 * i + 1;

		if (out_num < priv->num_eps && !dwc3_ep_is_reserved(config->props, out_num)) {
			out->caps.out = 1;
			if (i == 0) {
				out->caps.control = 1;
				out->caps.mps = 512;
			} else {
				out->caps.bulk = 1;
				out->caps.interrupt = 1;
				out->caps.mps = out_mps;
			}

			out->addr = USB_EP_DIR_OUT | i;
			err = udc_register_ep(dev, out);
			if (err != 0) {
				return err;
			}

			config->eps[out_num].cfg = out;
		}

		if (in_num < priv->num_eps && !dwc3_ep_is_reserved(config->props, in_num)) {
			in->caps.in = 1;
			if (i == 0) {
				in->caps.control = 1;
				in->caps.mps = 512;
			} else {
				in->caps.bulk = 1;
				in->caps.interrupt = 1;
				in->caps.mps = dwc3_in_ep_mps(dev, in_num);
			}

			in->addr = USB_EP_DIR_IN | i;
			err = udc_register_ep(dev, in);
			if (err != 0) {
				return err;
			}

			config->eps[in_num].cfg = in;
		}

		config->eps[out_num].number = out_num;
		config->eps[in_num].number = in_num;

		if (i != 0) {
			config->eps[out_num].trb_pool = &config->trb_pools[out_num * DWC3_TRB_NUM];
			config->eps[out_num].trb_buf = &config->trb_bufs[out_num * DWC3_TRB_NUM];
			config->eps[in_num].trb_pool = &config->trb_pools[in_num * DWC3_TRB_NUM];
			config->eps[in_num].trb_buf = &config->trb_bufs[in_num * DWC3_TRB_NUM];
		}
	}

	return 0;
}

static int dwc3_driver_preinit(const struct device *dev)
{
	const struct udc_dwc3_config *config = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE);
	k_event_init(&priv->drv_evt);
	atomic_clear(&priv->xfer_new);

	err = dwc3_reset_deassert(dev);
	if (err != 0) {
		LOG_ERR("Failed to release the controller reset: %d", err);
		return err;
	}

	err = dwc3_clk_enable(dev);
	if (err != 0) {
		LOG_ERR("Failed to enable the controller clocks: %d", err);
		return err;
	}

	err = dwc3_core_is_valid(dev);
	if (err != 0) {
		LOG_ERR("Not a DesignWare USB3 DRD core");
		return err;
	}

	for (unsigned int i = 0; i < 9; i++) {
		priv->hwparams[i] =
			dwc3_readl(dev, i == 8 ? USB_DWC3_GHWPARAMS8 : USB_DWC3_GHWPARAMS0 + 4 * i);
	}

	if (DWC3_IP_IS(priv, DWC32)) {
		priv->hwparams[9] = dwc3_readl(dev, USB_DWC3_GHWPARAMS9);
	}

	if (USB_DWC3_GHWPARAMS0_MODE(priv->hwparams[0]) == USB_DWC3_GHWPARAMS0_MODE_HOST) {
		LOG_ERR("Host-only controller");
		return -ENODEV;
	}

	priv->num_eps = USB_DWC3_NUM_EPS(priv->hwparams[3]);
	dwc3_check_params(dev);

	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.hs = priv->maximum_speed >= DWC3_SPEED_HIGH;

	err = dwc3_register_eps(dev);
	if (err != 0) {
		LOG_ERR("Failed to register endpoints");
		return err;
	}

	config->make_thread(dev);

	LOG_INF("DWC3 %04x revision %08x, %u endpoints, %s speed", priv->ip, priv->revision,
		priv->num_eps, priv->maximum_speed >= DWC3_SPEED_HIGH ? "high" : "full");

	return 0;
}

static void udc_dwc3_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_dwc3_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static void dwc3_handle_xfer_next(const struct device *dev, uint8_t number)
{
	const struct udc_dwc3_config *config = dev->config;

	if (number <= 1) {
		dwc3_ep0_kick(dev);
	} else if (number < 2 * config->num_of_eps && !config->eps[number].cfg->stat.halted) {
		dwc3_gadget_ep_kick(dev, &config->eps[number]);
	}
}

static ALWAYS_INLINE void dwc3_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	uint32_t evt;
	uint32_t eps;

	evt = k_event_wait(&priv->drv_evt, UINT32_MAX, false, K_FOREVER);

	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(DWC3_DRV_EVT_IRQ)) {
		k_event_clear(&priv->drv_evt, BIT(DWC3_DRV_EVT_IRQ));
		dwc3_process_event_buf(dev);
	}

	if (evt & BIT(DWC3_DRV_EVT_XFER)) {
		k_event_clear(&priv->drv_evt, BIT(DWC3_DRV_EVT_XFER));
		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			uint8_t number = find_lsb_set(eps) - 1;

			eps &= ~BIT(number);
			if (priv->started) {
				dwc3_handle_xfer_next(dev, number);
			}
		}
	}

	udc_unlock_internal(dev);
}

static const struct udc_api udc_dwc3_api = {
	.lock = udc_dwc3_lock,
	.unlock = udc_dwc3_unlock,
	.device_speed = udc_dwc3_device_speed,
	.init = udc_dwc3_init,
	.enable = udc_dwc3_enable,
	.disable = udc_dwc3_disable,
	.shutdown = udc_dwc3_shutdown,
	.set_address = udc_dwc3_set_address,
	.test_mode = udc_dwc3_test_mode,
	.host_wakeup = udc_dwc3_host_wakeup,
	.ep_enable = udc_dwc3_ep_enable,
	.ep_disable = udc_dwc3_ep_disable,
	.ep_set_halt = udc_dwc3_ep_set_halt,
	.ep_clear_halt = udc_dwc3_ep_clear_halt,
	.ep_enqueue = udc_dwc3_ep_enqueue,
	.ep_dequeue = udc_dwc3_ep_dequeue,
};

/* ------------------------------------------------------------------------- */
/* Devicetree                                                                */
/* ------------------------------------------------------------------------- */

/* An optional uint8-array property read as one byte, as Linux reads a u8 */
#define DWC3_DT_U8(n, prop, def)                                                                   \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (DT_INST_PROP_BY_IDX(n, prop, 0)), (def))

#define DWC3_DT_ARRAY_DEFINE(n, prop, type, name)                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop),                                                \
		    (static const type name##_##n[] = DT_INST_PROP(n, prop);), ())

#define DWC3_DT_ARRAY_GET(n, prop, name)                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (name##_##n), (NULL))

#define DWC3_DT_PROPS_DEFINE(n)                                                                    \
	DWC3_DT_ARRAY_DEFINE(n, snps_incr_burst_type_adjustment, uint32_t, dwc3_incr_burst)        \
	DWC3_DT_ARRAY_DEFINE(n, snps_reserved_endpoints, uint8_t, dwc3_reserved_eps)               \
                                                                                                   \
	static const struct dwc3_props dwc3_props_##n = {                                          \
		.incr_burst =                                                                      \
			DWC3_DT_ARRAY_GET(n, snps_incr_burst_type_adjustment, dwc3_incr_burst),    \
		.incr_burst_len = DT_INST_PROP_LEN_OR(n, snps_incr_burst_type_adjustment, 0),      \
		.reserved_eps = DWC3_DT_ARRAY_GET(n, snps_reserved_endpoints, dwc3_reserved_eps),  \
		.reserved_eps_len = DT_INST_PROP_LEN_OR(n, snps_reserved_endpoints, 0),            \
		.fladj = DT_INST_PROP_OR(n, snps_quirk_frame_length_adjustment, 0),                \
		.ref_clk_per = DT_INST_PROP_OR(n, snps_ref_clock_period_ns, 0),                    \
		.maximum_speed = DT_INST_ENUM_IDX_OR(n, maximum_speed, -1) + 1,                    \
		.hsphy_mode = DT_INST_ENUM_IDX_OR(n, phy_type, -1) + 1,                            \
		.hsphy_interface = DT_INST_ENUM_IDX_OR(n, snps_hsphy_interface, -1) + 1,           \
		.lpm_nyet_threshold = DWC3_DT_U8(n, snps_lpm_nyet_threshold, 0xf),                 \
		.hird_threshold = DWC3_DT_U8(n, snps_hird_threshold, 12),                          \
		.tx_de_emphasis = DWC3_DT_U8(n, snps_tx_de_emphasis, 1),                           \
		.rx_thr_num_pkt = DWC3_DT_U8(n, snps_rx_thr_num_pkt, 0),                           \
		.rx_max_burst = DWC3_DT_U8(n, snps_rx_max_burst, 0),                               \
		.tx_thr_num_pkt = DWC3_DT_U8(n, snps_tx_thr_num_pkt, 0),                           \
		.tx_max_burst = DWC3_DT_U8(n, snps_tx_max_burst, 0),                               \
		.has_lpm_erratum = DT_INST_PROP(n, snps_has_lpm_erratum),                          \
		.is_utmi_l1_suspend = DT_INST_PROP(n, snps_is_utmi_l1_suspend),                    \
		.usb2_gadget_lpm_disable = DT_INST_PROP(n, snps_usb2_gadget_lpm_disable),          \
		.disable_scramble_quirk = DT_INST_PROP(n, snps_disable_scramble_quirk),            \
		.u2exit_lfps_quirk = DT_INST_PROP(n, snps_u2exit_lfps_quirk),                      \
		.u2ss_inp3_quirk = DT_INST_PROP(n, snps_u2ss_inp3_quirk),                          \
		.req_p1p2p3_quirk = DT_INST_PROP(n, snps_req_p1p2p3_quirk),                        \
		.del_p1p2p3_quirk = DT_INST_PROP(n, snps_del_p1p2p3_quirk),                        \
		.del_phy_power_chg_quirk = DT_INST_PROP(n, snps_del_phy_power_chg_quirk),          \
		.lfps_filter_quirk = DT_INST_PROP(n, snps_lfps_filter_quirk),                      \
		.rx_detect_poll_quirk = DT_INST_PROP(n, snps_rx_detect_poll_quirk),                \
		.dis_u3_susphy_quirk = DT_INST_PROP(n, snps_dis_u3_susphy_quirk),                  \
		.dis_u2_susphy_quirk = DT_INST_PROP(n, snps_dis_u2_susphy_quirk),                  \
		.dis_enblslpm_quirk = DT_INST_PROP(n, snps_dis_enblslpm_quirk),                    \
		.dis_u1_entry_quirk = DT_INST_PROP(n, snps_dis_u1_entry_quirk),                    \
		.dis_u2_entry_quirk = DT_INST_PROP(n, snps_dis_u2_entry_quirk),                    \
		.dis_rxdet_inp3_quirk = DT_INST_PROP(n, snps_dis_rxdet_inp3_quirk),                \
		.dis_u2_freeclk_exists_quirk = DT_INST_PROP(n, snps_dis_u2_freeclk_exists_quirk),  \
		.dis_del_phy_power_chg_quirk = DT_INST_PROP(n, snps_dis_del_phy_power_chg_quirk),  \
		.dis_tx_ipgap_linecheck_quirk =                                                    \
			DT_INST_PROP(n, snps_dis_tx_ipgap_linecheck_quirk),                        \
		.resume_hs_terminations = DT_INST_PROP(n, snps_resume_hs_terminations),            \
		.ulpi_ext_vbus_drv = DT_INST_PROP(n, snps_ulpi_ext_vbus_drv),                      \
		.parkmode_disable_ss_quirk = DT_INST_PROP(n, snps_parkmode_disable_ss_quirk),      \
		.parkmode_disable_hs_quirk = DT_INST_PROP(n, snps_parkmode_disable_hs_quirk),      \
		.gfladj_refclk_lpm_sel = DT_INST_PROP(n, snps_gfladj_refclk_lpm_sel_quirk),        \
		.tx_de_emphasis_quirk = DT_INST_PROP(n, snps_tx_de_emphasis_quirk),                \
		.dis_metastability_quirk = DT_INST_PROP(n, snps_dis_metastability_quirk),          \
	};

#define DWC3_CLK_BY_IDX(idx, n)                                                                    \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(n, idx)),                          \
		.subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, idx, clkid),       \
	}

#define DWC3_CLK_BY_NAME(n, name)                                                                  \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, name)),                        \
		.subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, name, clkid),     \
	}

/* A clock of Linux, by its name or by its legacy name */
#define DWC3_CLK_NAMED(n, name, legacy)                                                            \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, name), (DWC3_CLK_BY_NAME(n, name)),                 \
		    (COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, legacy),                               \
				 (DWC3_CLK_BY_NAME(n, legacy)), ({0}))))

#define DWC3_CLKS_DEFINE(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),                                              \
		    (static const struct dwc3_clk dwc3_clks_##n[] = {                             \
			     LISTIFY(DT_INST_NUM_CLOCKS(n), DWC3_CLK_BY_IDX, (,), n)};),           \
		    ())

#define DWC3_RESET_BY_IDX(idx, n) RESET_DT_SPEC_INST_GET_BY_IDX(n, idx)

#define DWC3_RESETS_DEFINE(n)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, resets),                                              \
		    (static const struct reset_dt_spec dwc3_resets_##n[] = {                      \
			     LISTIFY(DT_INST_PROP_LEN(n, resets), DWC3_RESET_BY_IDX, (,), n)};),   \
		    ())

#define DWC3_CLKS_GET(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks), (dwc3_clks_##n), (NULL))

#define DWC3_RESETS_GET(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, resets), (dwc3_resets_##n), (NULL))

#define DWC3_IRQ_FLAGS(n) COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags), (DT_INST_IRQ(n, flags)), (0))

#define UDC_DWC3_DEVICE_DEFINE(n)                                                                  \
	K_THREAD_STACK_DEFINE(udc_dwc3_stack_##n, CONFIG_UDC_DWC3_STACK_SIZE);                     \
                                                                                                   \
	static void udc_dwc3_thread_##n(void *dev, void *arg1, void *arg2)                         \
	{                                                                                          \
		while (true) {                                                                     \
			dwc3_thread_handler(dev);                                                  \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc3_make_thread_##n(const struct device *dev)                             \
	{                                                                                          \
		struct udc_dwc3_data *priv = udc_get_private(dev);                                 \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_dwc3_stack_##n,                            \
				K_THREAD_STACK_SIZEOF(udc_dwc3_stack_##n), udc_dwc3_thread_##n,    \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_DWC3_THREAD_PRIORITY), K_ESSENTIAL,         \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc3_irq_enable_func_##n(const struct device *dev)                         \
	{                                                                                          \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc3_irq_disable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
                                                                                                   \
	static int dwc3_driver_preinit_##n(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), udc_dwc3_isr_handler,       \
			    DEVICE_DT_INST_GET(n), DWC3_IRQ_FLAGS(n));                             \
                                                                                                   \
		return dwc3_driver_preinit(dev);                                                   \
	}                                                                                          \
                                                                                                   \
	DWC3_DT_PROPS_DEFINE(n)                                                                    \
	DWC3_CLKS_DEFINE(n)                                                                        \
	DWC3_RESETS_DEFINE(n)                                                                      \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out_##n[DT_INST_PROP(n, num_bidir_endpoints)];          \
	static struct udc_ep_config ep_cfg_in_##n[DT_INST_PROP(n, num_bidir_endpoints)];           \
	static struct dwc3_ep dwc3_eps_##n[2 * DT_INST_PROP(n, num_bidir_endpoints)];              \
                                                                                                   \
	static __nocache uint32_t dwc3_evt_buf_##n[DWC3_EVENT_BUFFER_SIZE / 4] __aligned(64);      \
	static __nocache struct usb_dwc3_trb dwc3_ep0_trb_##n[2] __aligned(64);                    \
	static __nocache struct usb_dwc3_trb                                                       \
		dwc3_trbs_##n[2 * DT_INST_PROP(n, num_bidir_endpoints) * DWC3_TRB_NUM]             \
		__aligned(64);                                                                     \
	static struct net_buf                                                                      \
		*dwc3_trb_bufs_##n[2 * DT_INST_PROP(n, num_bidir_endpoints) * DWC3_TRB_NUM];       \
	static __nocache uint8_t dwc3_setup_buf_##n[8] __aligned(64);                              \
	static __nocache uint8_t dwc3_bounce_##n[DWC3_BOUNCE_SIZE] __aligned(64);                  \
                                                                                                   \
	static const struct udc_dwc3_config udc_dwc3_config_##n = {                                \
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(n)),                                  \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                                \
		.ep_cfg_in = ep_cfg_in_##n,                                                        \
		.ep_cfg_out = ep_cfg_out_##n,                                                      \
		.eps = dwc3_eps_##n,                                                               \
		.trb_pools = dwc3_trbs_##n,                                                        \
		.trb_bufs = dwc3_trb_bufs_##n,                                                     \
		.evt_buf = dwc3_evt_buf_##n,                                                       \
		.ep0_trb = dwc3_ep0_trb_##n,                                                       \
		.setup_buf = dwc3_setup_buf_##n,                                                   \
		.bounce = dwc3_bounce_##n,                                                         \
		.props = &dwc3_props_##n,                                                          \
		.clks = DWC3_CLKS_GET(n),                                                          \
		.num_clks = DT_INST_PROP_LEN_OR(n, clocks, 0),                                     \
		.ref_clk = DWC3_CLK_NAMED(n, ref, ref_clk),                                        \
		.susp_clk = DWC3_CLK_NAMED(n, suspend, suspend_clk),                               \
		.resets = DWC3_RESETS_GET(n),                                                      \
		.num_resets = DT_INST_PROP_LEN_OR(n, resets, 0),                                   \
		.quirks = UDC_DWC3_VENDOR_QUIRK_GET(n),                                            \
		.quirk_data = UDC_DWC3_VENDOR_QUIRK_DATA_GET(n),                                   \
		.make_thread = udc_dwc3_make_thread_##n,                                           \
		.irq_enable_func = udc_dwc3_irq_enable_func_##n,                                   \
		.irq_disable_func = udc_dwc3_irq_disable_func_##n,                                 \
	};                                                                                         \
                                                                                                   \
	static struct udc_dwc3_data udc_priv_##n = {};                                             \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dwc3_driver_preinit_##n, NULL, &udc_data_##n,                     \
			      &udc_dwc3_config_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_dwc3_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_DWC3_DEVICE_DEFINE)
