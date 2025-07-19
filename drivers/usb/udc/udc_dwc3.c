/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2023-2024 tinyVision.ai Inc.
 */

#define DT_DRV_COMPAT snps_dwc3

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include "udc_common.h"
#include "udc_dwc3.h"

LOG_MODULE_REGISTER(dwc3, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * Shut down the controller completely
 */
static int dwc3_api_shutdown(const struct device *dev)
{
	LOG_INF("api: shutdown");
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

static int dwc3_api_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int dwc3_api_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

/*
 * Ring buffer
 *
 * Helpers to operate the TRB and event ring buffers, shared with the hardware.
 */

void dwc3_ring_inc(uint32_t *nump, uint32_t size)
{
	uint32_t num = *nump + 1;

	*nump = (num >= size) ? 0 : num;
}

static void dwc3_push_trb(const struct device *dev, struct dwc3_ep_data *ep_data,
			   struct net_buf *buf, uint32_t ctrl)
{
	volatile struct dwc3_trb *trb = &ep_data->trb_buf[ep_data->head];

	/* If the next TRB in the chain is still owned by the hardware, need
	 * to retry later when more resources become available. */
	__ASSERT_NO_MSG(!ep_data->full);

	/* Associate an active buffer and a TRB together */
	ep_data->net_buf[ep_data->head] = buf;

	/* TRB# with one more chunk of data */
	trb->ctrl = ctrl;
	trb->addr_lo = LO32((uintptr_t)buf->data);
	trb->addr_hi = HI32((uintptr_t)buf->data);
	trb->status = ep_data->cfg.caps.in ? buf->len : buf->size;
	LOG_DBG("PUSH %u buf %p, data %p, size %u", ep_data->head, buf, buf->data, buf->size);

	/* Shift the head */
	dwc3_ring_inc(&ep_data->head, CONFIG_UDC_DWC3_TRB_NUM - 1);

	/* If the head touches the tail after we add something, we are full */
	ep_data->full = (ep_data->head == ep_data->tail);
}

static struct net_buf *dwc3_pop_trb(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	struct net_buf *buf = ep_data->net_buf[ep_data->tail];

	/* Clear the last TRB */
	ep_data->net_buf[ep_data->tail] = NULL;

	/* Move to the next position in the ring buffer */
	dwc3_ring_inc(&ep_data->tail, CONFIG_UDC_DWC3_TRB_NUM - 1);

	if (buf == NULL) {
		LOG_ERR("pop: the next TRB is emtpy");
		return NULL;
	}

	LOG_DBG("POP %u EP 0x%02x, buf %p, data %p",
		ep_data->tail, ep_data->cfg.addr, buf, buf->data);

	/* If we just pulled a TRB, we know we made one hole and we are not full anymore */
	ep_data->full = false;

	return buf;
}

/*
 * Commands
 *
 * The DEPCMD register acts as a command interface, where a command number
 * is written along with parameters, an action is performed and a CMDACT bit
 * is reset whenever the command completes.
 */

static uint32_t dwc3_depcmd(const struct device *dev, uint32_t addr, uint32_t cmd)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	sys_write32(cmd | DWC3_DEPCMD_CMDACT, cfg->base + addr);
	do {
		reg = sys_read32(cfg->base + addr);
	} while ((reg & DWC3_DEPCMD_CMDACT) != 0);

	switch (reg & DWC3_DEPCMD_STATUS_MASK) {
	case DWC3_DEPCMD_STATUS_OK:
		break;
	case DWC3_DEPCMD_STATUS_CMDERR:
		LOG_ERR("cmd: endpoint command failed");
		break;
	default:
		LOG_ERR("cmd: command failed with unknown status: 0x%08x", reg);
	}

	return FIELD_GET(DWC3_DEPCMD_XFERRSCIDX_MASK, reg);
}

static void dwc3_depcmd_ep_config(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t param0 = 0, param1 = 0;

	LOG_INF("cmd: configuring endpoint 0x%02x with wMaxPacketSize=%u", ep_data->cfg.addr,
		ep_data->cfg.mps);

	if (ep_data->cfg.stat.enabled) {
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_ACTION_MODIFY;
	} else {
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_ACTION_INIT;
	}

	switch (ep_data->cfg.attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_CTRL;
		break;
	case USB_EP_TYPE_BULK:
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_INT;
		break;
	case USB_EP_TYPE_ISO:
		param0 |= DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_ISOC;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Max Packet Size according to the USB descriptor configuration */
	param0 |= FIELD_PREP(DWC3_DEPCMDPAR0_DEPCFG_MPS_MASK, ep_data->cfg.mps);

	/* Burst Size of a single packet per burst (encoded as '0'): no burst */
	param0 |= FIELD_PREP(DWC3_DEPCMDPAR0_DEPCFG_BRSTSIZ_MASK, 15);

	/* Set the FIFO number, must be 0 for all OUT EPs */
	if (ep_data->cfg.caps.in) {
		param0 |= FIELD_PREP(DWC3_DEPCMDPAR0_DEPCFG_FIFONUM_MASK, ep_data->cfg.addr & 0x7f);
	}

	/* Per-endpoint events */
	param1 |= DWC3_DEPCMDPAR1_DEPCFG_XFERINPROGEN;
	param1 |= DWC3_DEPCMDPAR1_DEPCFG_XFERCMPLEN;
	/* param1 |= DWC3_DEPCMDPAR1_DEPCFG_XFERNRDYEN; useful for debugging */

	/* This is the usb protocol endpoint number, but the data encoding
	 * we chose for physical endpoint number is the same as this
	 * register */
	param1 |= FIELD_PREP(DWC3_DEPCMDPAR1_DEPCFG_EPNUMBER_MASK, ep_data->epn);

	sys_write32(param0, cfg->base + DWC3_DEPCMDPAR0(ep_data->epn));
	sys_write32(param1, cfg->base + DWC3_DEPCMDPAR1(ep_data->epn));
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPCFG);
}

static void dwc3_depcmd_ep_xfer_config(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	LOG_DBG("cmd: DepXferConfig: ep=0x%02x", ep_data->cfg.addr);

	reg = FIELD_PREP(DWC3_DEPCMDPAR0_DEPXFERCFG_NUMXFERRES_MASK, 1);
	sys_write32(reg, cfg->base + DWC3_DEPCMDPAR0(ep_data->epn));
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPXFERCFG);
}

static void dwc3_depcmd_set_stall(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	LOG_WRN("cmd: DepSetStall: ep=0x%02x", ep_data->cfg.addr);
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPSETSTALL);
}

static void dwc3_depcmd_clear_stall(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	LOG_INF("cmd: DepClearStall ep=0x%02x", ep_data->cfg.addr);
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPCSTALL);
}

static void dwc3_depcmd_start_xfer(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	/* Make sure the device is in U0 state, assuming TX FIFO is empty */
	reg = sys_read32(cfg->base + DWC3_DCTL);
	reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
	reg |= DWC3_DCTL_ULSTCHNGREQ_REMOTEWAKEUP;
	sys_write32(reg, cfg->base + DWC3_DCTL);

	sys_write32(HI32((uintptr_t)ep_data->trb_buf), cfg->base + DWC3_DEPCMDPAR0(ep_data->epn));
	sys_write32(LO32((uintptr_t)ep_data->trb_buf), cfg->base + DWC3_DEPCMDPAR1(ep_data->epn));

	ep_data->xferrscidx =
		dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPSTRTXFER);
	LOG_DBG("cmd: DepStartXfer done ep=0x%02x xferrscidx=0x%x", ep_data->cfg.addr,
		ep_data->xferrscidx);
}

static void dwc3_depcmd_update_xfer(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	uint32_t flags;

	flags = FIELD_PREP(DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->xferrscidx);
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPUPDXFER | flags);
	LOG_DBG("cmd: DepUpdateXfer done ep=0x%02x addr=0x%08x data=0x%08x", ep_data->cfg.addr,
		DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPUPDXFER | flags);
}

static void dwc3_depcmd_end_xfer(const struct device *dev, struct dwc3_ep_data *ep_data,
				  uint32_t flags)
{
	flags |= FIELD_PREP(DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->xferrscidx);
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPENDXFER | flags);
	LOG_DBG("cmd: DepEndXfer done ep=0x%02x", ep_data->cfg.addr);

	ep_data->head = ep_data->tail = 0;
	ep_data->head = ep_data->tail = 0;
}

static void dwc3_depcmd_start_config(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	uint32_t flags;

	flags = FIELD_PREP(DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->cfg.caps.control ? 0 : 2);
	dwc3_depcmd(dev, DWC3_DEPCMD(ep_data->epn), DWC3_DEPCMD_DEPSTARTCFG | flags);
	LOG_DBG("cmd: DepStartConfig done ep=0x%02x", ep_data->cfg.addr);
}

static void dwc3_dgcmd(const struct device *dev, uint32_t cmd)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	sys_write32(cmd, cfg->base + DWC3_DGCMD);
	do {
		reg = sys_read32(cfg->base + DWC3_DGCMD);
	} while ((reg & DWC3_DEPCMD_CMDACT) != 0);
	LOG_DBG("cmd: done: status=0x%08x", reg);

	if ((reg & DWC3_DGCMD_STATUS_MASK) != DWC3_DGCMD_STATUS_OK) {
		LOG_ERR("cmd: failed: status returned is not 'ok'");
	}
}

static void dwc3_dgcmd_exit_latency(const struct device *dev,
				    const struct usb_system_exit_latency *sel)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	reg = sys_read32(cfg->base + DWC3_DCTL);
	reg = (reg & DWC3_DCTL_INITU2ENA) ? sel->u2pel : sel->u1pel;
	reg = (reg > 125) ? 0 : reg;
	sys_write32(reg, cfg->base + DWC3_DGCMDPAR);
	dwc3_dgcmd(dev, DWC3_DGCMD_EXITLATENCY);
}

static int dwc3_set_address(const struct device *dev, const uint8_t addr)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	LOG_INF("addr: setting to %u", addr);

	/* Configure the new address */
	reg = sys_read32(cfg->base + DWC3_DCFG);
	reg &= ~DWC3_DCFG_DEVADDR_MASK;
	reg |= FIELD_PREP(DWC3_DCFG_DEVADDR_MASK, addr);
	sys_write32(reg, cfg->base + DWC3_DCFG);

	/* Re-apply the same endpoint configuration */
	dwc3_depcmd_ep_config(dev, &cfg->ep_data_in[0]);
	dwc3_depcmd_ep_config(dev, &cfg->ep_data_out[0]);

	return 0;
}

/*
 * Transfer Requests (TRB)
 *
 * DWC3 receives transfer requests from this driver through a shared memory
 * buffer, resubmitted upon every new transfer (through either Start or
 * Update command).
 */

static void dwc3_trb_norm_init(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	volatile struct dwc3_trb *trb = ep_data->trb_buf;
	const size_t i = CONFIG_UDC_DWC3_TRB_NUM - 1;

	LOG_DBG("trb: normal: init");

	/* TRB0 that blocks the transfer from going further */
	trb[0].ctrl = 0;

	/* TRB LINK that loops the ring buffer back to the beginning */
	trb[i].ctrl = DWC3_TRB_CTRL_TRBCTL_LINK_TRB | DWC3_TRB_CTRL_HWO;
	trb[i].addr_lo = LO32((uintptr_t)ep_data->trb_buf);
	trb[i].addr_hi = HI32((uintptr_t)ep_data->trb_buf);

	/* Start the transfer now, update it later */
	dwc3_depcmd_start_xfer(dev, ep_data);
}

static void dwc3_trb_ctrl_in(const struct device *dev, uint32_t ctrl)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = &cfg->ep_data_in[0];
	struct net_buf *buf = ep_data->net_buf[0];
	volatile struct dwc3_trb *trb = ep_data->trb_buf;

	__ASSERT_NO_MSG(buf != NULL);
	LOG_DBG("TRB_CONTROL_IN len=%u", buf->len);

	/* TRB0 sending the data */
	trb[0].addr_lo = LO32((uintptr_t)buf->data);
	trb[0].addr_hi = HI32((uintptr_t)buf->data);
	trb[0].status = buf->len;
	trb[0].ctrl = ctrl | DWC3_TRB_CTRL_LST | DWC3_TRB_CTRL_HWO;

	/* Start a new transfer every time: no ring buffer */
	dwc3_depcmd_start_xfer(dev, ep_data);
}

static void dwc3_trb_ctrl_out(const struct device *dev, struct net_buf *buf, uint32_t ctrl)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = &cfg->ep_data_out[0];
	volatile struct dwc3_trb *trb = ep_data->trb_buf;

	__ASSERT_NO_MSG(buf != NULL);
	LOG_DBG("TRB_CONTROL_OUT size=%u", buf->size);

	/* Associate the buffer with the TRB for picking it up later */
	__ASSERT_NO_MSG(ep_data->net_buf[0] == NULL);
	ep_data->net_buf[0] = buf;

	/* TRB0 for recieinving the data */
	trb[0].addr_lo = LO32((uintptr_t)ep_data->net_buf[0]->data);
	trb[0].addr_hi = HI32((uintptr_t)ep_data->net_buf[0]->data);
	trb[0].status = buf->size;
	trb[0].ctrl = ctrl | DWC3_TRB_CTRL_LST | DWC3_TRB_CTRL_HWO;

	/* Start a new transfer every time: no ring buffer */
	dwc3_depcmd_start_xfer(dev, ep_data);
}

static void dwc3_trb_ctrl_setup_out(const struct device *dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);

	LOG_DBG("TRB_CONTROL_SETUP ep=0x%02x", USB_CONTROL_EP_OUT);
	dwc3_trb_ctrl_out(dev, buf, DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP);
}

static void dwc3_trb_ctrl_data_out(const struct device *dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 512);

	LOG_DBG("TRB_CONTROL_DATA_OUT ep=0x%02x", USB_CONTROL_EP_OUT);
	dwc3_trb_ctrl_out(dev, buf, DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA);
}

static void dwc3_trb_ctrl_data_in(const struct device *dev)
{
	LOG_DBG("TRB_CONTROL_DATA_IN ep=0x%02x", USB_CONTROL_EP_IN);
	dwc3_trb_ctrl_in(dev, DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA);
}

static void dwc3_trb_ctrl_status_2_in(const struct device *dev)
{
	LOG_DBG("TRB_CONTROL_STATUS_2_IN ep=0x%02x", USB_CONTROL_EP_IN);
	dwc3_trb_ctrl_in(dev, DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2);
}

static void dwc3_trb_ctrl_status_3_in(const struct device *dev)
{
	LOG_DBG("TRB_CONTROL_STATUS_3_IN ep=0x%02x", USB_CONTROL_EP_IN);
	dwc3_trb_ctrl_in(dev, DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3);
}

static void dwc3_trb_ctrl_status_3_out(const struct device *dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 0);

	LOG_DBG("TRB_CONTROL_STATUS_3_OUT ep=0x%02x", USB_CONTROL_EP_OUT);
	dwc3_trb_ctrl_out(dev, buf, DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3);
}

static int dwc3_trb_bulk(const struct device *dev, struct dwc3_ep_data *ep_data,
			  struct net_buf *buf)
{
	uint32_t ctrl = DWC3_TRB_CTRL_IOC | DWC3_TRB_CTRL_HWO | DWC3_TRB_CTRL_CSP;

	LOG_INF("TRB_BULK_EP_0x%02x, buf %p, data %p, size %u, len %u", ep_data->cfg.addr, buf,
		buf->data, buf->size, buf->len);

	if (ep_data->full) {
		return -EBUSY;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		LOG_DBG("Buffer has a ZLP flag, terminating the transfer");
		ctrl |= DWC3_TRB_CTRL_TRBCTL_NORMAL_ZLP;
		ep_data->total = 0;
	} else {
		ctrl |= DWC3_TRB_CTRL_TRBCTL_NORMAL;
		ep_data->total += buf->len;

		if (ep_data->cfg.caps.in && ep_data->total % ep_data->cfg.mps == 0) {
			LOG_DBG("Buffer is a multiple of %d, continuing this transfer of %u bytes",
				ep_data->cfg.mps, ep_data->total);
			ctrl |= DWC3_TRB_CTRL_CHN;
		} else {
			LOG_DBG("End of USB transfer, %u bytes transferred", ep_data->total);
			ep_data->total = 0;
		}
	}

	dwc3_push_trb(dev, ep_data, buf, ctrl);
	dwc3_depcmd_update_xfer(dev, ep_data);

	return 0;
}

/*
 * Events
 *
 * Process the events from the event ring buffer. Interrupts gives us a
 * hint that an event is available, which we fetch from a ring buffer shared
 * with the hardware.
 */

static void dwc3_on_soft_reset(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	/* Configure and reset the Device Controller */
	/* TODO confirm that DWC_USB3_EN_LPM_ERRATA == 1 */
	reg = DWC3_DCTL_CSFTRST;
	reg |= FIELD_PREP(DWC3_DCTL_LPM_NYET_THRES_MASK, 15);
	sys_write32(reg, cfg->base + DWC3_DCTL);
	while (sys_read32(cfg->base + DWC3_DCTL) & DWC3_DCTL_CSFTRST) {
		continue;
	}

	/* Enable AXI64 bursts for various sizes expected */
	reg = DWC3_GSBUSCFG0_INCR256BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR128BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR64BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR32BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR16BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR8BRSTENA;
	reg |= DWC3_GSBUSCFG0_INCR4BRSTENA;
	sys_set_bits(cfg->base + DWC3_GSBUSCFG0, reg);

	/* Letting GTXTHRCFG and GRXTHRCFG unchanged */
	reg = DWC3_GTXTHRCFG_USBTXPKTCNTSEL;
	reg |= FIELD_PREP(DWC3_GTXTHRCFG_USBTXPKTCNT_MASK, 1);
	reg |= FIELD_PREP(DWC3_GTXTHRCFG_USBMAXTXBURSTSIZE_MASK, 2);
	sys_write32(reg, cfg->base + DWC3_GTXTHRCFG);

	/* Read the chip identification */
	reg = sys_read32(cfg->base + DWC3_GCOREID);
	LOG_INF("evt: coreid=0x%04lx rel=0x%04lx", FIELD_GET(DWC3_GCOREID_CORE_MASK, reg),
		FIELD_GET(DWC3_GCOREID_REL_MASK, reg));
	__ASSERT_NO_MSG(FIELD_GET(DWC3_GCOREID_CORE_MASK, reg) == 0x5533);

	/* Letting GUID unchanged */
	/* Letting GUSB2PHYCFG and GUSB3PIPECTL unchanged */

	/* Setting fifo size for both TX and RX, experimental values
	 * GRXFIFOSIZ too far below or above  512 * 3 leads to errors */
	reg = 512 * 3;
	sys_write32(reg, cfg->base + DWC3_GTXFIFOSIZ(0));
	sys_write32(reg, cfg->base + DWC3_GRXFIFOSIZ(0));

	/* Setup the event buffer address, size and start event reception */
	memset((void *)cfg->evt_buf, 0, CONFIG_UDC_DWC3_EVENTS_NUM * sizeof(uint32_t));
	sys_write32(HI32((uintptr_t)cfg->evt_buf), cfg->base + DWC3_GEVNTADR_HI(0));
	sys_write32(LO32((uintptr_t)cfg->evt_buf), cfg->base + DWC3_GEVNTADR_LO(0));
	sys_write32(CONFIG_UDC_DWC3_EVENTS_NUM * sizeof(uint32_t), cfg->base + DWC3_GEVNTSIZ(0));
	LOG_INF("Event buffer size is %u bytes", sys_read32(cfg->base + DWC3_GEVNTSIZ(0)));
	sys_write32(0, cfg->base + DWC3_GEVNTCOUNT(0));

	/* Letting GCTL unchanged */

	/* Set the USB device configuration, including max supported speed */
	sys_write32(DWC3_DCFG_PERFRINT_90, cfg->base + DWC3_DCFG);
	switch (cfg->maximum_speed_idx) {
	case DWC3_SPEED_IDX_SUPER_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_SUPER_SPEED");
		sys_set_bits(cfg->base + DWC3_DCFG, DWC3_DCFG_DEVSPD_SUPER_SPEED);
		break;
	case DWC3_SPEED_IDX_HIGH_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_HIGH_SPEED");
		sys_set_bits(cfg->base + DWC3_DCFG, DWC3_DCFG_DEVSPD_HIGH_SPEED);
		break;
	case DWC3_SPEED_IDX_FULL_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_FULL_SPEED");
		sys_set_bits(cfg->base + DWC3_DCFG, DWC3_DCFG_DEVSPD_FULL_SPEED);
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Set the number of USB3 packets the device can receive at once */
	reg = sys_read32(cfg->base + DWC3_DCFG);
	reg &= ~DWC3_DCFG_NUMP_MASK;
	reg |= FIELD_PREP(DWC3_DCFG_NUMP_MASK, 15);
	sys_write32(reg, cfg->base + DWC3_DCFG);

	/* Enable reception of all USB events except DWC3_DEVTEN_ULSTCNGEN */
	reg = DWC3_DEVTEN_INACTTIMEOUTRCVEDEN;
	reg |= DWC3_DEVTEN_VNDRDEVTSTRCVEDEN;
	reg |= DWC3_DEVTEN_EVNTOVERFLOWEN;
	reg |= DWC3_DEVTEN_CMDCMPLTEN;
	reg |= DWC3_DEVTEN_ERRTICERREN;
	reg |= DWC3_DEVTEN_HIBERNATIONREQEVTEN;
	reg |= DWC3_DEVTEN_WKUPEVTEN;
	reg |= DWC3_DEVTEN_CONNECTDONEEN;
	reg |= DWC3_DEVTEN_USBRSTEN;
	reg |= DWC3_DEVTEN_DISCONNEVTEN;
	sys_write32(reg, cfg->base + DWC3_DEVTEN);

	/* Configure endpoint 0x00 and 0x80 only for now */
	dwc3_depcmd_start_config(dev, &cfg->ep_data_in[0]);
	dwc3_depcmd_start_config(dev, &cfg->ep_data_out[0]);
}

static void dwc3_on_usb_reset(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;

	LOG_DBG("Going through DWC3 reset logic");

	/* Reset all ongoing transfers on non-control IN endpoints */
	for (int epn = 1; epn < cfg->num_in_eps; epn++) {
		struct dwc3_ep_data *ep_data = &cfg->ep_data_in[epn];

		continue; /* TODO */
		dwc3_depcmd_end_xfer(dev, ep_data, 0);
		dwc3_depcmd_clear_stall(dev, ep_data);
	}

	/* Reset all ongoing transfers on non-control OUT endpoints */
	for (int epn = 1; epn < cfg->num_out_eps; epn++) {
		struct dwc3_ep_data *ep_data = &cfg->ep_data_out[epn];

		continue; /* TODO */
		dwc3_depcmd_end_xfer(dev, ep_data, 0);
		dwc3_depcmd_clear_stall(dev, ep_data);
	}

	/* Perform the USB reset operations manually to improve latency */
	dwc3_set_address(dev, 0);

	/* Let Zephyr set the device address 0 */
	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void dwc3_on_connect_done(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	int mps = 0;

	/* Adjust parameters against the connection speed */
	switch (sys_read32(cfg->base + DWC3_DSTS) & DWC3_DSTS_CONNECTSPD_MASK) {
	case DWC3_DSTS_CONNECTSPD_FS:
	case DWC3_DSTS_CONNECTSPD_HS:
		mps = 64;
		/* TODO this is not suspending USB3, it enable suspend feature */
		/* sys_set_bits(cfg->base + DWC3_GUSB3PIPECTL, DWC3_GUSB3PIPECTL_SUSPENDENABLE); */
		break;
	case DWC3_DSTS_CONNECTSPD_SS:
		mps = 512;
		/* sys_set_bits(cfg->base + DWC3_GUSB2PHYCFG, DWC3_GUSB2PHYCFG_SUSPHY); */
		break;
	}
	__ASSERT_NO_MSG(mps != 0);

	/* Reconfigure control endpoints connection speed */
	udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT)->mps = mps;
	udc_get_ep_cfg(dev, USB_CONTROL_EP_IN)->mps = mps;
	dwc3_depcmd_ep_config(dev, &cfg->ep_data_in[0]);
	dwc3_depcmd_ep_config(dev, &cfg->ep_data_out[0]);

	/* Letting GTXFIFOSIZn unchanged */
}

static void dwc3_on_link_state_event(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	reg = sys_read32(cfg->base + DWC3_DSTS);

	switch (reg & DWC3_DSTS_CONNECTSPD_MASK) {
	case DWC3_DSTS_CONNECTSPD_SS:
		switch (reg & DWC3_DSTS_USBLNKST_MASK) {
		case DWC3_DSTS_USBLNKST_USB3_U0:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U0 ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_U1:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U1 ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_U2:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U2 ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_U3:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U3 ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_SS_DIS:
			LOG_DBG("--- DSTS_USBLNKST_USB3_SS_DIS ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_RX_DET:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RX_DET ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_SS_INACT:
			LOG_DBG("--- DSTS_USBLNKST_USB3_SS_INACT ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_POLL:
			LOG_DBG("--- DSTS_USBLNKST_USB3_POLL ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_RECOV:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RECOV ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_HRESET:
			LOG_DBG("--- DSTS_USBLNKST_USB3_HRESET ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_CMPLY:
			LOG_DBG("--- DSTS_USBLNKST_USB3_CMPLY ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_LPBK:
			LOG_DBG("--- DSTS_USBLNKST_USB3_LPBK ---");
			break;
		case DWC3_DSTS_USBLNKST_USB3_RESET_RESUME:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RESET_RESUME ---");
			break;
		default:
			LOG_ERR("evt: unknown USB3 link state");
		}
		break;
	case DWC3_DSTS_CONNECTSPD_HS:
	case DWC3_DSTS_CONNECTSPD_FS:
		switch (reg & DWC3_DSTS_USBLNKST_MASK) {
		case DWC3_DSTS_USBLNKST_USB2_ON_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_ON_STATE ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_SLEEP_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_SLEEP_STATE ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_SUSPEND_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_SUSPEND_STATE ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_DISCONNECTED:
			LOG_DBG("--- DSTS_USBLNKST_USB2_DISCONNECTED ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_EARLY_SUSPEND:
			LOG_DBG("--- DSTS_USBLNKST_USB2_EARLY_SUSPEND ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_RESET:
			LOG_DBG("--- DSTS_USBLNKST_USB2_RESET ---");
			break;
		case DWC3_DSTS_USBLNKST_USB2_RESUME:
			LOG_DBG("--- DSTS_USBLNKST_USB2_RESUME ---");
			break;
		default:
			LOG_ERR("evt: unknown USB2 link state");
		}
		break;
	default:
		LOG_ERR("evt: unknown connection speed");
	}
}

/* Control Write */

/* OUT */
static void dwc3_on_ctrl_write_setup(const struct device *dev, struct net_buf *buf)
{
	LOG_DBG("evt: CTRL_WRITE_SETUP (out), data %p", buf->data);
	dwc3_trb_ctrl_data_out(dev);
}

/* OUT */
static void dwc3_on_ctrl_write_data(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_WRITE_DATA (out), data %p", buf->data);
	udc_ctrl_update_stage(dev, buf);
	ret = udc_ctrl_submit_s_out_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
	k_sleep(K_MSEC(1));
}

/* IN */
static void dwc3_on_ctrl_write_status(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_WRITE_STATUS (in), data %p", buf->data);
	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
	udc_ctrl_update_stage(dev, buf);
}

/* Control Read */

/* OUT */
static void dwc3_on_ctrl_read_setup(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_READ_SETUP (out), data %p", buf->data);
	ret = udc_ctrl_submit_s_in_status(dev);
	__ASSERT_NO_MSG(ret == 0);
}

/* IN */
static void dwc3_on_ctrl_read_data(const struct device *dev, struct net_buf *buf)
{
	LOG_DBG("evt: CTRL_READ_DATA (in), data %p", buf->data);
	dwc3_trb_ctrl_status_3_out(dev);
	udc_ctrl_update_stage(dev, buf);
	net_buf_unref(buf);
}

/* OUT */
static void dwc3_on_ctrl_read_status(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_READ_STATUS (out), data %p", buf->data);
	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
	udc_ctrl_update_stage(dev, buf);
}

/* No-Data Control */

/* OUT */
static void dwc3_on_ctrl_nodata_setup(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_NODATA_SETUP (out), data %p", buf->data);
	ret = udc_ctrl_submit_s_status(dev);
	__ASSERT_NO_MSG(ret == 0);
}

/* IN */
static void dwc3_on_ctrl_nodata_status(const struct device *dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_NODATA_STATUS (in), data %p", buf->data);
	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
	udc_ctrl_update_stage(dev, buf);
}

/*
 * We received a packet, we need to let the USB stack parse it to know what direction the next
 * transaction is expected to have.
 */
static void dwc3_on_ctrl_setup_out(const struct device *dev, struct net_buf *buf)
{
	struct dwc3_data *priv = udc_get_private(dev);

	/* Only moment where this information is accessible */
	priv->data_stage_length = udc_data_stage_length(buf);

	/* To be able to differentiate the next stage*/
	udc_ep_buf_set_setup(buf);
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		dwc3_on_ctrl_write_setup(dev, buf);
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		dwc3_on_ctrl_read_setup(dev, buf);
	} else if (udc_ctrl_stage_is_no_data(dev)) {
		dwc3_on_ctrl_nodata_setup(dev, buf);
	} else {
		LOG_ERR("evt: unknown setup stage");
	}
}

/*
 * @brief Handle completion of a CONTROL IN packet (device -> host).
 *
 * Further characterize which type of CONTROL IN packet that is.
 * Handle actions common to all CONTROL IN packets.
 */
static void dwc3_on_ctrl_in(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = &cfg->ep_data_in[0];
	struct dwc3_trb *trb = &ep_data->trb_buf[0];
	struct net_buf *buf = ep_data->net_buf[0];

	/* We are not expected to touch that buffer anymore */
	ep_data->net_buf[0] = NULL;
	__ASSERT_NO_MSG(buf != NULL);

	/* Continue to the next step */
	switch (trb->ctrl & DWC3_TRB_CTRL_TRBCTL_MASK) {
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA:
		dwc3_on_ctrl_read_data(dev, buf);
		break;
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2:
		dwc3_on_ctrl_nodata_status(dev, buf);
		dwc3_trb_ctrl_setup_out(dev);
		break;
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3:
		dwc3_on_ctrl_write_status(dev, buf);
		dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

/*
 * @brief Handle completion of a CONTROL OUT packet (host -> device).
 *
 * Further characterize which type of CONTROL OUT packet that is.
 * Handle actions common to all CONTROL OUT packets.
 */
static void dwc3_on_ctrl_out(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = &cfg->ep_data_out[0];
	struct dwc3_trb *trb = &ep_data->trb_buf[0];
	struct net_buf *buf = ep_data->net_buf[0];

	__ASSERT_NO_MSG(buf != NULL);

	/* For buffers coming from the host, update the size actually received */
	buf->len = buf->size - FIELD_GET(DWC3_TRB_STATUS_BUFSIZ_MASK, trb->status);

	/* Latency optimization: set the address immediately to be able to be able to ACK/NAK the
	 * first packets from the host with the new address, otherwise the host issue a reset */
	if (buf->len > 2 && buf->data[0] == 0x00 && buf->data[1] == 0x05) {
		dwc3_set_address(dev, buf->data[2]);
	}

	/* We are not expected to touch that buffer anymore */
	ep_data->net_buf[0] = NULL;
	__ASSERT_NO_MSG(buf != NULL);

	/* Continue to the next step */
	switch (trb->ctrl & DWC3_TRB_CTRL_TRBCTL_MASK) {
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP:
		dwc3_on_ctrl_setup_out(dev, buf);
		break;
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA:
		dwc3_on_ctrl_write_data(dev, buf);
		break;
	case DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3:
		dwc3_on_ctrl_read_status(dev, buf);
		dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void dwc3_on_xfer_not_ready(const struct device *dev, uint32_t evt)
{
	switch (evt & DWC3_DEPEVT_STATUS_B3_MASK) {
	case DWC3_DEPEVT_STATUS_B3_CONTROL_SETUP:
		LOG_DBG("--- DWC3_DEPEVT_XFERNOTREADY_CONTROL_SETUP ---");
		break;
	case DWC3_DEPEVT_STATUS_B3_CONTROL_DATA:
		LOG_DBG("--- DWC3_DEPEVT_XFERNOTREADY_CONTROL_DATA ---");
		break;
	case DWC3_DEPEVT_STATUS_B3_CONTROL_STATUS:
		LOG_DBG("--- DWC3_DEPEVT_XFERNOTREADY_CONTROL_STATUS ---");
		break;
	}
}

static void dwc3_on_xfer_done(const struct device *dev, struct dwc3_ep_data *ep_data)
{
	struct dwc3_trb *trb = &ep_data->trb_buf[ep_data->tail];

	switch (trb->status & DWC3_TRB_STATUS_TRBSTS_MASK) {
	case DWC3_TRB_STATUS_TRBSTS_OK:
		break;
	case DWC3_TRB_STATUS_TRBSTS_MISSEDISOC:
		LOG_ERR("DWC3_TRB_STATUS_TRBSTS_MISSEDISOC");
		break;
	case DWC3_TRB_STATUS_TRBSTS_SETUPPENDING:
		LOG_ERR("DWC3_TRB_STATUS_TRBSTS_SETUPPENDING");
		break;
	case DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS:
		LOG_ERR("DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS");
		break;
	case DWC3_TRB_STATUS_TRBSTS_ZLPPENDING:
		LOG_ERR("DWC3_TRB_STATUS_TRBSTS_ZLPPENDING");
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void dwc3_on_xfer_done_norm(const struct device *dev, uint32_t evt)
{
	const struct dwc3_config *cfg = dev->config;
	int epn = FIELD_GET(DWC3_DEPEVT_EPN_MASK, evt);
	struct dwc3_ep_data *ep_data =
		(epn & 1) ? &cfg->ep_data_in[epn >> 1] : &cfg->ep_data_out[epn >> 1];
	struct dwc3_trb *trb = &ep_data->trb_buf[ep_data->tail];
	struct net_buf *buf;
	int ret;

	/* Clear the TRB that triggered the event */
	buf = dwc3_pop_trb(dev, ep_data);
	LOG_DBG("evt: XFER_DONE_NORM: EP 0x%02x, data %p", ep_data->cfg.addr, buf->data);
	__ASSERT_NO_MSG(buf != NULL);
	dwc3_on_xfer_done(dev, ep_data);

	/* For buffers coming from the host, update the size actually received */
	if (ep_data->cfg.caps.out) {
		buf->len = buf->size - FIELD_GET(DWC3_TRB_STATUS_BUFSIZ_MASK, trb->status);
	}

	ret = udc_submit_ep_event(dev, buf, 0);
	__ASSERT_NO_MSG(ret == 0);

	/* We just made some room for a new buffer, check if something more to enqueue */
	k_work_submit(&ep_data->work);
}

void dwc3_irq_handler(void *ptr)
{
	const struct device *dev = ptr;
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_data *priv = udc_get_private(dev);

	cfg->irq_clear_func();
	k_work_reschedule(&priv->dwork, K_NO_WAIT);
}

/*
 * UDC API
 *
 * Interface called by Zehpyr from the upper levels of abstractions.
 */

int dwc3_api_ep_enqueue(const struct device *dev, struct udc_ep_config *ep_cfg,
			 struct net_buf *buf)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = (void *)ep_cfg;
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	LOG_DBG("Enqueued buffer to EP 0x%02x", ep_data->cfg.addr);

	switch (ep_data->cfg.addr) {
	case USB_CONTROL_EP_IN:
		/* Save the buffer to fetch it back later */
		__ASSERT(ep_data->net_buf[0] == NULL, "concurrenn requests not allowed for EP0");
		ep_data->net_buf[0] = buf;

		/* Control buffers are managed directly without a queue */
		if (bi->data) {
			dwc3_trb_ctrl_data_in(dev);
		} else if (bi->status && udc_ctrl_stage_is_no_data(dev)) {
			dwc3_trb_ctrl_status_2_in(dev);
		} else if (bi->status) {
			dwc3_trb_ctrl_status_3_in(dev);
		} else {
			CODE_UNREACHABLE;
		}
		break;
	case USB_CONTROL_EP_OUT:
		/* expected to be handled by the driver directly */
		CODE_UNREACHABLE;
		break;
	default:
		/* Submit the buffer to the queue */
		udc_buf_put(ep_cfg, buf);

		/* Process this buffer along with other waiting */
		if (sys_read32(cfg->base + DWC3_DCTL) & DWC3_DCTL_RUNSTOP) {
			LOG_DBG("submitting to EP 0x%02x", ep_data->cfg.addr);
			k_work_submit(&ep_data->work);
		}
	}

	return 0;
}

int dwc3_api_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();
	buf = udc_buf_get_all(dev, ep_cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}
	irq_unlock(lock_key);
	return 0;
}

int dwc3_api_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = (void *)ep_cfg;

	sys_clear_bit(cfg->base + DWC3_DALEPENA, ep_data->epn);
	return 0;
}

/*
 * Halt endpoint. Halted endpoint should respond with a STALL handshake.
 */
int dwc3_api_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_ep_data *ep_data = (void *)ep_cfg;

	LOG_WRN("api: stall ep=0x%02x", ep_data->cfg.addr);

	/* TODO: empty the buffers from the queue */

	switch (ep_data->cfg.addr) {
	case USB_CONTROL_EP_IN:
		/* Remove the TRBs transfer for the cancelled sequence */
		dwc3_depcmd_end_xfer(dev, ep_data, DWC3_DEPCMD_HIPRI_FORCERM);

		/* The datasheet says to only set stall the OUT direction */
		ep_data = &cfg->ep_data_out[0];

		__fallthrough;
	case USB_CONTROL_EP_OUT:
		dwc3_depcmd_end_xfer(dev, ep_data, 0);
		dwc3_depcmd_set_stall(dev, ep_data);

		/* The hardware will automatically clear the halt state upon
		 * the next setup packet received.
		 */
		dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		dwc3_depcmd_set_stall(dev, ep_data);
		ep_data->cfg.stat.halted = true;
	}

	return 0;
}

int dwc3_api_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct dwc3_ep_data *ep_data = (void *)ep_cfg;

	LOG_DBG("api: unstall ep=0x%02x", ep_data->cfg.addr);
	__ASSERT_NO_MSG(ep_data->cfg.addr != USB_CONTROL_EP_OUT);
	__ASSERT_NO_MSG(ep_data->cfg.addr != USB_CONTROL_EP_IN);

	dwc3_depcmd_clear_stall(dev, ep_data);
	ep_data->cfg.stat.halted = false;

	return 0;
}

int dwc3_api_set_address(const struct device *dev, const uint8_t addr)
{
	const struct dwc3_config *cfg = dev->config;
	uint32_t reg;

	/* The address is set in the code earlier to improve latency, only
	 * checking that it is still the value done for consistency. */
	reg = sys_read32(cfg->base + DWC3_DCFG);
	if (FIELD_GET(DWC3_DCFG_DEVADDR_MASK, reg) != addr) {
		return -EPROTO;
	}

	return 0;
}

int dwc3_api_set_system_exit_latency(const struct device *dev,
				     const struct usb_system_exit_latency *sel)
{
	LOG_DBG("api: u1sel=%u u1pel=%u u2sel=%u u2pel=%u", sel->u1sel, sel->u1pel, sel->u2sel,
		sel->u2pel);
	dwc3_dgcmd_exit_latency(dev, sel);
	return 0;
}

enum udc_bus_speed dwc3_api_device_speed(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;

	switch (sys_read32(cfg->base + DWC3_DSTS) & DWC3_DSTS_CONNECTSPD_MASK) {
	case DWC3_DSTS_CONNECTSPD_HS:
		return UDC_BUS_SPEED_HS;
	case DWC3_DSTS_CONNECTSPD_FS:
		return UDC_BUS_SPEED_FS;
	case DWC3_DSTS_CONNECTSPD_SS:
		return UDC_BUS_SPEED_SS;
	}

	__ASSERT(0, "unknown device speed");
	return 0;
}

int dwc3_api_enable(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_data *priv = udc_get_private(dev);

	LOG_DBG("Enabling DWC3 driver");

	__ASSERT_NO_MSG(atomic_test_bit(&priv->flags, DWC3_FLAG_INITIALIZED));

	/* Bootstrap: prepare reception of the initial Setup packet */
	dwc3_trb_ctrl_setup_out(dev);

	/* Enable the DWC3 events */
	sys_set_bits(cfg->base + DWC3_DCTL, DWC3_DCTL_RUNSTOP);

	/* Enable the IRQ (for now, just schedule a first work queue job) */
	//cfg->irq_enable_func();
	k_work_schedule(&priv->dwork, K_NO_WAIT);

	return 0;
}

int dwc3_api_disable(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;

	LOG_DBG("Disabling DWC3 driver");

	sys_clear_bits(cfg->base + DWC3_DCTL, DWC3_DCTL_RUNSTOP);

	return 0;
}

/*
 * Hardware Init
 *
 * Prepare the driver and the hardware to being used.
 * This goes through register configuration and register commands.
 */

int dwc3_api_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct dwc3_ep_data *ep_data = (struct dwc3_ep_data *)ep_cfg;
	const struct dwc3_config *cfg = dev->config;

	LOG_DBG("%s 0x%02x", __func__, ep_data->cfg.addr);

	memset(ep_data->trb_buf, 0, sizeof(*ep_data->trb_buf) * CONFIG_UDC_DWC3_TRB_NUM);
	dwc3_depcmd_ep_config(dev, ep_data);
	dwc3_depcmd_ep_xfer_config(dev, ep_data);

	if (!ep_data->cfg.caps.control) {
		dwc3_trb_norm_init(dev, ep_data);
	}

	/* Starting from here, the endpoint can be used */
	sys_set_bits(cfg->base + DWC3_DALEPENA, DWC3_DALEPENA_USBACTEP(ep_data->epn));

	/* Walk through the list of buffer to enqueue we might have blocked */
	k_work_submit(&ep_data->work);

	return 0;
}

/*
 * Prepare and configure most of the parts, if the controller has a way
 * of detecting VBUS activity it should be enabled here.
 * Only dwc3_enable() makes device visible to the host.
 */
int dwc3_api_init(const struct device *dev)
{
	struct dwc3_data *priv = udc_get_private(dev);
	const struct dwc3_config *cfg = dev->config;
	int ret;

	LOG_DBG("Initializing the DWC3 core");

	/* Issue a soft reset to the core and USB2 and USB3 PHY */
	sys_set_bits(cfg->base + DWC3_GCTL, DWC3_GCTL_CORESOFTRESET);
	sys_set_bits(cfg->base + DWC3_GUSB3PIPECTL, DWC3_GUSB3PIPECTL_PHYSOFTRST);
	sys_set_bits(cfg->base + DWC3_GUSB2PHYCFG, DWC3_GUSB2PHYCFG_PHYSOFTRST);
	k_sleep(K_USEC(1000)); /* TODO: reduce amount of wait time */

	/* Teriminate the reset of the USB2 and USB3 PHY first */
	sys_clear_bits(cfg->base + DWC3_GUSB3PIPECTL, DWC3_GUSB3PIPECTL_PHYSOFTRST);
	sys_clear_bits(cfg->base + DWC3_GUSB2PHYCFG, DWC3_GUSB2PHYCFG_PHYSOFTRST);

	/* Teriminate the reset of the DWC3 core after it */
	sys_clear_bits(cfg->base + DWC3_GCTL, DWC3_GCTL_CORESOFTRESET);

	/* Initialize USB2 PHY vendor-specific wrappers */
	sys_set_bits(cfg->base + DWC3_U2PHYCTRL1, DWC3_U2PHYCTRL1_SEL_INTERNALCLK);
	sys_set_bits(cfg->base + DWC3_U2PHYCTRL2, DWC3_U2PHYCTRL2_REFCLK_SEL);

	/* Initialize USB3 PHY vendor-specific wrappers */
	sys_set_bits(cfg->base + DWC3_U3PHYCTRL1, BIT(22));
	sys_clear_bits(cfg->base + DWC3_U3PHYCTRL4, DWC3_U3PHYCTRL4_INT_CLOCK);

	/* The USB core was reset, configure it as documented */
	dwc3_on_soft_reset(dev);

	/* Configure the control OUT endpoint */
	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 512, 0);
	if (ret < 0) {
		LOG_ERR("init: could not enable control OUT ep");
		return ret;
	}

	/* Configure the control IN endpoint */
	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 512, 0);
	if (ret < 0) {
		LOG_ERR("init: could not enable control IN ep");
		return ret;
	}

	LOG_INF("Event buffer size is %u bytes", sys_read32(cfg->base + DWC3_GEVNTSIZ(0)));

	atomic_set_bit(&priv->flags, DWC3_FLAG_INITIALIZED);
	return 0;
}

static const struct udc_api dwc3_api = {
	.lock = dwc3_api_lock,
	.unlock = dwc3_api_unlock,
	.device_speed = dwc3_api_device_speed,
	.init = dwc3_api_init,
	.enable = dwc3_api_enable,
	.disable = dwc3_api_disable,
	.shutdown = dwc3_api_shutdown,
	.set_address = dwc3_api_set_address,
	.set_system_exit_latency = dwc3_api_set_system_exit_latency,
	.ep_enable = dwc3_api_ep_enable,
	.ep_disable = dwc3_api_ep_disable,
	.ep_set_halt = dwc3_api_ep_set_halt,
	.ep_clear_halt = dwc3_api_ep_clear_halt,
	.ep_enqueue = dwc3_api_ep_enqueue,
	.ep_dequeue = dwc3_api_ep_dequeue,
};

static void dwc3_ep_worker(struct k_work *work)
{
	struct dwc3_ep_data *ep_data = CONTAINER_OF(work, struct dwc3_ep_data, work);
	const struct device *dev = ep_data->dev;
	struct net_buf *buf;
	int ret;

	LOG_DBG("queue: checking for pending transfers for EP 0x%02x", ep_data->cfg.addr);

	if (ep_data->cfg.stat.halted) {
		LOG_DBG("queue: endpoint is halted, not processing buffers");
		return;
	}

	while ((buf = udc_buf_peek(dev, ep_data->cfg.addr)) != NULL) {
		LOG_INF("Processing buffer %p from queue", buf);

		ret = dwc3_trb_bulk(dev, ep_data, buf);
		if (ret < 0) {
			LOG_DBG("queue: abort: No more room for buffer");
			break;
		}

		LOG_DBG("queue: success: Buffer enqueued");
		udc_buf_get(dev, ep_data->cfg.addr);
	}
	LOG_DBG("queue: Done");
}

#define NORMAL_EP(n, fn) fn(n + 2)

static void dwc3_event_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dwc3_data *priv = CONTAINER_OF(dwork, struct dwc3_data, dwork);
	const struct device *dev = priv->dev;
	const struct dwc3_config *cfg = dev->config;
	uint32_t evt;

	if (sys_read32(cfg->base + DWC3_GEVNTCOUNT(0)) == 0) {
		/* In the meantime that IRQs are enabled, schedule the event handler again */
		k_work_schedule(&priv->dwork, K_MSEC(CONFIG_UDC_DWC3_EVENTS_POLL_MS));
		return;
	}

	/* Cache the current event and release the resource */
	evt = cfg->evt_buf[priv->evt_next];

	switch (evt & DWC3_EVT_MASK) {
	case DWC3_DEPEVT_XFERCOMPLETE(0):
		LOG_DBG("--- DEPEVT_XFERCOMPLETE(0) ---");
		dwc3_on_ctrl_out(dev);
		break;
	case DWC3_DEPEVT_XFERCOMPLETE(1):
		LOG_DBG("--- DEPEVT_XFERCOMPLETE(1) ---");
		dwc3_on_ctrl_in(dev);
		break;
	case LISTIFY(30, NORMAL_EP, (: case), DWC3_DEPEVT_XFERCOMPLETE):
	case LISTIFY(30, NORMAL_EP, (: case), DWC3_DEPEVT_XFERINPROGRESS):
		LOG_DBG("--- DEPEVT_XFERINPROGRESS ---");
		dwc3_on_xfer_done_norm(dev, evt);
		break;
	case DWC3_DEPEVT_XFERNOTREADY(0):
	case DWC3_DEPEVT_XFERNOTREADY(1):
		dwc3_on_xfer_not_ready(dev, evt);
		break;
	case DWC3_DEVT_DISCONNEVT:
		LOG_DBG("--- DEVT_DISCONNEVT ---");
		break;
	case DWC3_DEVT_USBRST:
		LOG_DBG("--- DEVT_USBRST ---");
		dwc3_on_usb_reset(dev);
		break;
	case DWC3_DEVT_CONNECTDONE:
		LOG_DBG("--- DEVT_CONNECTDONE ---");
		dwc3_on_connect_done(dev);
		break;
	case DWC3_DEVT_ULSTCHNG:
		LOG_DBG("--- DEVT_ULSTCHNG ---");
		dwc3_on_link_state_event(dev);
		break;
	case DWC3_DEVT_WKUPEVT:
		LOG_DBG("--- DEVT_WKUPEVT ---");
		break;
	case DWC3_DEVT_SUSPEND:
		LOG_DBG("--- DEVT_SUSPEND ---");
		break;
	case DWC3_DEVT_SOF:
		LOG_DBG("--- DEVT_SOF ---");
		break;
	case DWC3_DEVT_CMDCMPLT:
		LOG_DBG("--- DEVT_CMDCMPLT ---");
		break;
	case DWC3_DEVT_VNDRDEVTSTRCVED:
		LOG_DBG("--- DEVT_VNDRDEVTSTRCVED ---");
		break;
	case DWC3_DEVT_ERRTICERR:
		CODE_UNREACHABLE;
		break;
	case DWC3_DEVT_EVNTOVERFLOW:
		CODE_UNREACHABLE;
		break;
	default:
		LOG_ERR("unhandled event: 0x%x", evt);
		CODE_UNREACHABLE;
	}

	sys_write32(sizeof(evt), cfg->base + DWC3_GEVNTCOUNT(0));
	dwc3_ring_inc(&priv->evt_next, CONFIG_UDC_DWC3_EVENTS_NUM);

	LOG_DBG("--- * ---");
	k_work_reschedule(&priv->dwork, K_NO_WAIT);
}

/*
 * Initialize the controller and endpoints capabilities,
 * register endpoint structures, no hardware I/O yet.
 */
int dwc3_driver_preinit(const struct device *dev)
{
	const struct dwc3_config *cfg = dev->config;
	struct dwc3_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	struct dwc3_ep_data *ep_data;
	uint16_t mps = 0;
	int ret;

	k_mutex_init(&data->mutex);
	k_work_init_delayable(&priv->dwork, &dwc3_event_worker);

	data->caps.rwup = true;
	switch (cfg->maximum_speed_idx) {
	case DWC3_SPEED_IDX_SUPER_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_SUPER_SPEED");
		data->caps.mps0 = UDC_MPS0_512;
		data->caps.ss = true;
		data->caps.hs = true;
		mps = 1024;
		break;
	case DWC3_SPEED_IDX_HIGH_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_HIGH_SPEED");
		data->caps.mps0 = UDC_MPS0_64;
		data->caps.hs = true;
		mps = 1024;
		break;
	case DWC3_SPEED_IDX_FULL_SPEED:
		LOG_DBG("DWC3_SPEED_IDX_FULL_SPEED");
		data->caps.mps0 = UDC_MPS0_64;
		mps = 64;
		break;
	default:
		LOG_ERR("Not implemented");
	}

	/* Control IN endpoint */
	ep_data = &cfg->ep_data_in[0];
	k_work_init(&ep_data->work, dwc3_ep_worker);
	ep_data->dev = dev;
	ep_data->cfg.addr = USB_CONTROL_EP_IN;
	ep_data->cfg.caps.in = 1;
	ep_data->cfg.caps.control = 1;
	ep_data->cfg.caps.mps = mps;
	ep_data->trb_buf = cfg->trb_buf_in[0];
	ep_data->epn = 1;
	ret = udc_register_ep(dev, &ep_data->cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register endpoint");
		return ret;
	}

	/* Control OUT endpoint */
	ep_data = &cfg->ep_data_out[0];
	k_work_init(&ep_data->work, dwc3_ep_worker);
	ep_data->dev = dev;
	ep_data->cfg.addr = USB_CONTROL_EP_OUT;
	ep_data->cfg.caps.out = 1;
	ep_data->cfg.caps.control = 1;
	ep_data->cfg.caps.mps = mps;
	ep_data->trb_buf = cfg->trb_buf_out[0];
	ep_data->epn = 0;
	ret = udc_register_ep(dev, &ep_data->cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register endpoint");
		return ret;
	}

	/* Normal IN endpoints */
	for (int i = 1; i < cfg->num_in_eps; i++) {
		LOG_DBG("Preinit endpoint 0x%02x", USB_EP_DIR_IN | i);
		ep_data = &cfg->ep_data_in[i];
		k_work_init(&ep_data->work, dwc3_ep_worker);
		ep_data->dev = dev;
		ep_data->cfg.addr = USB_EP_DIR_IN | i;
		ep_data->cfg.caps.in = true;
		ep_data->cfg.caps.bulk = true;
		ep_data->cfg.caps.interrupt = true;
		ep_data->cfg.caps.iso = true;
		ep_data->cfg.caps.mps = mps;
		ep_data->trb_buf = cfg->trb_buf_in[i];
		ep_data->epn = (i << 1) | 1;
		ret = udc_register_ep(dev, &ep_data->cfg);
		if (ret < 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	/* Normal OUT endpoints */
	for (int i = 1; i < cfg->num_out_eps; i++) {
		LOG_DBG("Preinit endpoint 0x%02x", USB_EP_DIR_OUT | i);
		ep_data = &cfg->ep_data_out[i];
		k_work_init(&ep_data->work, dwc3_ep_worker);
		ep_data->dev = dev;
		ep_data->cfg.addr = USB_EP_DIR_OUT | i;
		ep_data->cfg.caps.out = true;
		ep_data->cfg.caps.bulk = true;
		ep_data->cfg.caps.interrupt = true;
		ep_data->cfg.caps.iso = true;
		ep_data->cfg.caps.mps = mps;
		ep_data->trb_buf = cfg->trb_buf_out[i];
		ep_data->epn = (i << 1) | 0;
		ret = udc_register_ep(dev, &ep_data->cfg);
		if (ret < 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	LOG_DBG("done");

	return 0;
}

#define DWC3_DEVICE_DEFINE(n)							\
										\
	static void dwc3_irq_enable_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    dwc3_irq_handler, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
		sys_write32(0x00000001, DT_INST_REG_ADDR_BY_NAME(n, ev_enable));\
	}									\
										\
	static void dwc3_irq_clear_func_##n(void)				\
	{									\
		sys_write32(0x00000001,						\
			DT_INST_REG_ADDR_BY_NAME(n, ev_pending));		\
	}									\
										\
	static __nocache uint32_t dwc3_dma_evt_buf_##n				\
		[CONFIG_UDC_DWC3_EVENTS_NUM] __aligned(16);			\
										\
	static __nocache struct dwc3_trb dwc3_dma_trb_i##n			\
		[DT_INST_PROP(n, num_in_endpoints)][CONFIG_UDC_DWC3_TRB_NUM];	\
										\
	static __nocache struct dwc3_trb dwc3_dma_trb_o##n			\
		[DT_INST_PROP(n, num_out_endpoints)][CONFIG_UDC_DWC3_TRB_NUM];	\
										\
	static struct dwc3_ep_data dwc3_ep_data_i##n				\
		[DT_INST_PROP(n, num_in_endpoints)];				\
										\
	static struct dwc3_ep_data dwc3_ep_data_o##n				\
		[DT_INST_PROP(n, num_out_endpoints)];				\
										\
	static const struct dwc3_config dwc3_config_##n = {			\
		.base = DT_INST_REG_ADDR_BY_NAME(n, base),			\
		.num_in_eps = DT_INST_PROP(n, num_in_endpoints),		\
		.num_out_eps = DT_INST_PROP(n, num_out_endpoints),		\
		.ep_data_in  = dwc3_ep_data_i##n,				\
		.ep_data_out = dwc3_ep_data_o##n,				\
		.trb_buf_in = dwc3_dma_trb_i##n,				\
		.trb_buf_out = dwc3_dma_trb_o##n,				\
		.evt_buf = dwc3_dma_evt_buf_##n,				\
		.maximum_speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),\
		.irq_enable_func = dwc3_irq_enable_func_##n,			\
		.irq_clear_func = dwc3_irq_clear_func_##n,			\
	};									\
										\
	static struct dwc3_data udc_priv_##n = {				\
		.dev = DEVICE_DT_INST_GET(n),					\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, dwc3_driver_preinit, NULL,			\
			      &udc_data_##n, &dwc3_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &dwc3_api);

DT_INST_FOREACH_STATUS_OKAY(DWC3_DEVICE_DEFINE)
