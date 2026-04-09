/*
 * Copyright (c) 2026 Narek Aydinyan
 * Copyright (c) 2026 Arayik Gharibyan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_lan78xx

#include "eth_lan78xx_priv.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>

#include <usbh_ch9.h>
#include <usbh_class.h>
#include <usbh_device.h>

LOG_MODULE_REGISTER(eth_lan78xx, CONFIG_ETHERNET_LOG_LEVEL);

#define LAN78XX_INTR_LEN      16u
#define LAN78XX_ETH_MIN_NOFCS 60u
#define LAN78XX_ETH_MAX_NOFCS 1514u

#define LAN78XX_HW_RESET_TIMEOUT_MS  1000
#define LAN78XX_PHY_READY_TIMEOUT_MS 1000
#define LAN78XX_MDIO_TIMEOUT_MS      200
#define LAN78XX_MDIO_POLL_DELAY_US   1000u
#define LAN78XX_HW_CFG_POLL_DELAY_US 10000u
#define LAN78XX_HW_RESET_SETTLE_US   5000u
#define LAN78XX_FIFO_FLUSH_DELAY_US  1000u

#define LAN78XX_ATTACH_RETRY_MAX          5u
#define LAN78XX_ATTACH_RETRY_DELAY_MS     50u
#define LAN78XX_ATTACH_HW_INIT_DELAY_MS   250u
#define LAN78XX_ATTACH_POST_BIND_DELAY_MS 200u
#define LAN78XX_HW_INIT_RETRY_COUNT       3
#define LAN78XX_HW_INIT_RETRY_DELAY_MS    200u

#define LAN78XX_BULK_IN_DLY_DEFAULT 0x2000u
#define LAN78XX_BURST_CAP_DEFAULT   32u
#define LAN78XX_USB_VID             DT_INST_PROP(0, vid)
#define LAN78XX_USB_PID             DT_INST_PROP(0, pid)

#define LAN78XX_MGMT_EVT_ATTACH BIT(0)
#define LAN78XX_MGMT_EVT_DETACH BIT(1)
#define LAN78XX_MGMT_EVT_WAKE   BIT(2)

#define LAN78XX_NO_POLL_SCHEDULED          (-1LL)
#define LAN78XX_INITIAL_LINK_POLL_DELAY_MS 2000u
#define LAN78XX_LINK_POLL_ERROR_DELAY_MS   200u
#define LAN78XX_TX_WAIT_MS                 10

#define LAN78XX_RX_SLOT_FREE  0u
#define LAN78XX_RX_SLOT_ARMED 1u
#define LAN78XX_RX_SLOT_DONE  2u

#define LAN78XX_INTR_STATE_IDLE  0u
#define LAN78XX_INTR_STATE_ARMED 1u
#define LAN78XX_INTR_STATE_DONE  2u

#define LAN78XX_TX_SLOT_FREE 0u
#define LAN78XX_TX_SLOT_BUSY 1u

#define LAN78XX_TX_ALIGN 4u
#define LAN78XX_TX_PAD(len)                                                                        \
	((uint32_t)((LAN78XX_TX_ALIGN - ((len) & (LAN78XX_TX_ALIGN - 1u))) &                       \
		    (LAN78XX_TX_ALIGN - 1u)))
#define LAN78XX_TX_USB_LEN(wire_len) (LAN78XX_CMD_HDR_LEN + (wire_len) + LAN78XX_TX_PAD(wire_len))

#define LAN78XX_CFG_RX_BUF_SIZE  CONFIG_ETH_LAN78XX_RX_BUF_SIZE
#define LAN78XX_CFG_RX_XFERS     CONFIG_ETH_LAN78XX_RX_XFERS
#define LAN78XX_CFG_TX_INFLIGHT  CONFIG_ETH_LAN78XX_TX_INFLIGHT
#define LAN78XX_CFG_LINK_POLL_MS CONFIG_ETH_LAN78XX_LINK_POLL_MS
#define LAN78XX_CFG_DESC_MAX_LEN 4096u
#define LAN78XX_PHY_ADDR_COUNT   32u
#define LAN78XX_USB_EP_ATTR_MASK 0x03u
#define LAN78XX_USB_EP_MPS_MASK  0x07FFu
#define LAN78XX_RX_XFERS_MAX     LAN78XX_CFG_RX_XFERS
#define LAN78XX_TX_INFLIGHT_MAX  LAN78XX_CFG_TX_INFLIGHT

#define LAN78XX_ETH_FCS_LEN         4u
#define LAN78XX_REG_ACCESS_LEN      sizeof(uint32_t)
#define LAN78XX_RX_FRAME_PREFIX_LEN (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t))
#define LAN78XX_RX_FRAME_ALIGN      4u

BUILD_ASSERT(LAN78XX_RX_XFERS_MAX >= 1, "ETH_LAN78XX_RX_XFERS must be >= 1");
BUILD_ASSERT(LAN78XX_TX_INFLIGHT_MAX >= 1, "ETH_LAN78XX_TX_INFLIGHT must be >= 1");

struct lan78xx_config {
	uint8_t alt_setting;
	uint8_t led_enable_mask;
	struct net_eth_mac_config mac_cfg;
};

struct lan78xx_ctx;

struct lan78xx_rx_slot {
	struct lan78xx_ctx *ctx;
	struct uhc_transfer *xfer;
	atomic_t state;
};

struct lan78xx_intr_slot {
	struct lan78xx_ctx *ctx;
	struct uhc_transfer *xfer;
	atomic_t state;
};

struct lan78xx_tx_slot {
	struct lan78xx_ctx *ctx;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	struct net_pkt *pkt;
	atomic_t state;
	uint8_t index;
};

struct lan78xx_ep_ids {
	uint8_t ifnum;
	uint8_t bulk_in;
	uint8_t bulk_out;
	uint8_t intr_in;
};

struct lan78xx_ep_mps {
	uint16_t bulk_in;
	uint16_t bulk_out;
	uint16_t intr_in;
};

struct lan78xx_ctx {
	const struct device *dev;
	const struct lan78xx_config *cfg;
	struct net_if *iface;
	struct usb_device *udev;

	struct lan78xx_ep_ids ep_ids;
	struct lan78xx_ep_mps ep_mps;

	bool connected;
	bool link_up;
	bool ctrl_ok;

	struct k_mutex ctrl_mutex;

	uint8_t phy_id;
	bool phy_valid;
	uint8_t attach_attempt;

	uint8_t mac[NET_ETH_ADDR_LEN];

	struct lan78xx_rx_slot rx_slots[LAN78XX_RX_XFERS_MAX];
	uint8_t rx_count;
	atomic_t rx_stopping;

	struct lan78xx_intr_slot intr_slot;
	atomic_t intr_stopping;

	struct lan78xx_tx_slot tx_slots[LAN78XX_TX_INFLIGHT_MAX];
	uint8_t tx_pool_depth;
	struct k_sem tx_free_sem;
	struct k_mutex tx_lock;

	struct k_thread mgmt_thread;
	struct k_sem mgmt_sem;
	atomic_t mgmt_flags;
	int64_t next_link_poll_at;

	struct k_thread io_thread;
	struct k_sem io_sem;
};

static K_KERNEL_STACK_DEFINE(lan78xx_mgmt_stack_0, CONFIG_ETH_LAN78XX_MGMT_STACK_SIZE);
static K_KERNEL_STACK_DEFINE(lan78xx_io_stack_0, CONFIG_ETH_LAN78XX_IO_STACK_SIZE);

static int lan78xx_enable_auto_mac_link_mode(struct lan78xx_ctx *ctx);
static void lan78xx_mgmt_thread_fn(void *p1, void *p2, void *p3);
static void lan78xx_io_thread_fn(void *p1, void *p2, void *p3);

#define LAN78XX_CFG(inst)                                                                          \
	static const struct lan78xx_config lan78xx_cfg_##inst = {                                  \
		.alt_setting = DT_INST_PROP_OR(inst, alt, 0),                                      \
		.led_enable_mask = DT_INST_PROP_OR(inst, led_enable_mask, 0),                      \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                  \
	}

LAN78XX_CFG(0);

static struct lan78xx_ctx lan78xx_ctx_0 = {
	.cfg = &lan78xx_cfg_0,
};

static inline int lan78xx_norm_async_ok(int ret)
{
	if (ret == 0 || ret == -EINPROGRESS || ret == -EALREADY || ret == EINPROGRESS) {
		return 0;
	}

	return ret;
}

static inline void lan78xx_mgmt_signal(struct lan78xx_ctx *ctx, atomic_val_t flags)
{
	atomic_or(&ctx->mgmt_flags, flags);
	k_sem_give(&ctx->mgmt_sem);
}

static inline void lan78xx_io_signal(struct lan78xx_ctx *ctx)
{
	k_sem_give(&ctx->io_sem);
}

static bool lan78xx_match_vidpid(const struct usb_device_descriptor *dd)
{
	const uint16_t vid = sys_le16_to_cpu(dd->idVendor);
	const uint16_t pid = sys_le16_to_cpu(dd->idProduct);

	return (vid == LAN78XX_USB_VID) && (pid == LAN78XX_USB_PID);
}

static bool lan78xx_udev_ready(const struct usb_device *udev)
{
	return (udev != NULL) && (udev->state == USB_STATE_CONFIGURED) && (udev->actual_cfg != 0) &&
	       (udev->cfg_desc != NULL);
}

static bool lan78xx_mac_is_valid(const uint8_t mac[NET_ETH_ADDR_LEN])
{
	if ((mac[0] & 0x01u) != 0u) {
		return false;
	}

	for (size_t i = 0; i < NET_ETH_ADDR_LEN; i++) {
		if (mac[i] != 0x00u) {
			return true;
		}
	}

	return false;
}

static int lan78xx_mac_load(struct lan78xx_ctx *ctx)
{
	int ret = net_eth_mac_load(&ctx->cfg->mac_cfg, ctx->mac);

	if (ret == -ENODATA) {
		LOG_ERR("MAC not configured, set local-mac-address or zephyr,random-mac-address");
		return -ENODEV;
	} else if (ret < 0) {
		LOG_ERR("Could not load MAC from configuration (%d)", ret);
		return ret;
	}

	if (!lan78xx_mac_is_valid(ctx->mac)) {
		LOG_ERR("Configured MAC is invalid");
		return -EINVAL;
	}

	return 0;
}

static void lan78xx_start_workers(struct lan78xx_ctx *ctx)
{
	k_thread_create(&ctx->io_thread, lan78xx_io_stack_0,
			K_KERNEL_STACK_SIZEOF(lan78xx_io_stack_0), lan78xx_io_thread_fn, ctx, NULL,
			NULL, K_PRIO_PREEMPT(CONFIG_ETH_LAN78XX_IO_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&ctx->io_thread, "lan78xx_io");

	k_thread_create(&ctx->mgmt_thread, lan78xx_mgmt_stack_0,
			K_KERNEL_STACK_SIZEOF(lan78xx_mgmt_stack_0), lan78xx_mgmt_thread_fn, ctx,
			NULL, NULL, K_PRIO_PREEMPT(CONFIG_ETH_LAN78XX_MGMT_THREAD_PRIO), 0,
			K_NO_WAIT);
	k_thread_name_set(&ctx->mgmt_thread, "lan78xx_mgmt");

	if (atomic_get(&ctx->mgmt_flags) != 0) {
		k_sem_give(&ctx->mgmt_sem);
	}
	if (atomic_get(&ctx->rx_stopping) != 0 || atomic_get(&ctx->intr_stopping) != 0) {
		k_sem_give(&ctx->io_sem);
	}
}

static bool lan78xx_phy_id_word_valid(uint16_t value)
{
	return (value != 0u) && (value != UINT16_MAX);
}

static int lan78xx_alloc_transfer_with_buffer(struct lan78xx_ctx *ctx, uint8_t ep, uint16_t mps,
					      usbh_udev_cb_t done_cb, void *priv, size_t buf_size,
					      struct uhc_transfer **out_xfer)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (!ctx || !ctx->udev || !out_xfer) {
		return -EINVAL;
	}

	*out_xfer = NULL;

	xfer = usbh_xfer_alloc(ctx->udev, ep, done_cb, NULL);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	xfer->mps = mps;

	buf = usbh_xfer_buf_alloc(ctx->udev, buf_size);
	if (buf == NULL) {
		(void)usbh_xfer_free(ctx->udev, xfer);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(ctx->udev, xfer, buf);
	if (ret != 0) {
		usbh_xfer_buf_free(ctx->udev, buf);
		(void)usbh_xfer_free(ctx->udev, xfer);
		return ret;
	}

	xfer->priv = priv;
	*out_xfer = xfer;

	return 0;
}

static int lan78xx_read32_nolock(struct lan78xx_ctx *ctx, uint16_t reg, uint32_t *out)
{
	struct net_buf *nb;
	int ret;

	if (!ctx || !out || !ctx->udev) {
		return -EINVAL;
	}

	nb = usbh_xfer_buf_alloc(ctx->udev, LAN78XX_REG_ACCESS_LEN);
	if (nb == NULL) {
		return -ENOMEM;
	}

	net_buf_reset(nb);
	nb->len = LAN78XX_REG_ACCESS_LEN;
	memset(nb->data, 0, LAN78XX_REG_ACCESS_LEN);

	ret = usbh_req_setup(ctx->udev, LAN78XX_REQTYPE_VENDOR_IN_DEV, LAN78XX_USB_REQ_READ_REG, 0,
			     reg, LAN78XX_REG_ACCESS_LEN, nb);
	if (ret == 0) {
		if (nb->len != LAN78XX_REG_ACCESS_LEN) {
			ret = -EIO;
		} else {
			*out = sys_get_le32(nb->data);
		}
	}

	usbh_xfer_buf_free(ctx->udev, nb);
	return ret;
}

static int lan78xx_write32_nolock(struct lan78xx_ctx *ctx, uint16_t reg, uint32_t value)
{
	struct net_buf *nb;
	int ret;

	if (!ctx || !ctx->udev) {
		return -EINVAL;
	}

	nb = usbh_xfer_buf_alloc(ctx->udev, LAN78XX_REG_ACCESS_LEN);
	if (nb == NULL) {
		return -ENOMEM;
	}

	net_buf_reset(nb);
	sys_put_le32(value, net_buf_add(nb, LAN78XX_REG_ACCESS_LEN));

	ret = usbh_req_setup(ctx->udev, LAN78XX_REQTYPE_VENDOR_OUT_DEV, LAN78XX_USB_REQ_WRITE_REG,
			     0, reg, LAN78XX_REG_ACCESS_LEN, nb);

	usbh_xfer_buf_free(ctx->udev, nb);
	return ret;
}

static int lan78xx_read32(struct lan78xx_ctx *ctx, uint16_t reg, uint32_t *out)
{
	int ret;

	if (!ctx || !ctx->udev) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->ctrl_mutex, K_FOREVER);
	ret = lan78xx_read32_nolock(ctx, reg, out);
	k_mutex_unlock(&ctx->ctrl_mutex);

	return ret;
}

static int lan78xx_write32(struct lan78xx_ctx *ctx, uint16_t reg, uint32_t value)
{
	int ret;

	if (!ctx || !ctx->udev) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->ctrl_mutex, K_FOREVER);
	ret = lan78xx_write32_nolock(ctx, reg, value);
	k_mutex_unlock(&ctx->ctrl_mutex);

	return ret;
}

static int lan78xx_update32(struct lan78xx_ctx *ctx, uint16_t reg, uint32_t mask, uint32_t val)
{
	uint32_t cur = 0;
	int ret = lan78xx_read32(ctx, reg, &cur);

	if (ret != 0) {
		return ret;
	}

	cur = (cur & ~mask) | (val & mask);
	return lan78xx_write32(ctx, reg, cur);
}

static int lan78xx_mdio_wait_not_busy_nolock(struct lan78xx_ctx *ctx, int timeout_ms)
{
	const int64_t end = k_uptime_get() + timeout_ms;
	uint32_t v = 0;

	while (k_uptime_get() < end) {
		int ret = lan78xx_read32_nolock(ctx, LAN78XX_MII_ACC, &v);

		if (ret != 0) {
			return ret;
		}

		if ((v & LAN78XX_MII_ACC_MII_BUSY) == 0u) {
			return 0;
		}

		k_busy_wait(LAN78XX_MDIO_POLL_DELAY_US);
	}

	return -ETIMEDOUT;
}

static inline uint32_t lan78xx_mii_acc(uint8_t phy_id, uint8_t reg, bool read)
{
	uint32_t v = 0;

	v |= ((uint32_t)phy_id << LAN78XX_MII_ACC_PHY_ADDR_SHIFT) & LAN78XX_MII_ACC_PHY_ADDR_MASK;
	v |= ((uint32_t)reg << LAN78XX_MII_ACC_REG_SHIFT) & LAN78XX_MII_ACC_REG_MASK;

	if (!read) {
		v |= LAN78XX_MII_ACC_MII_WRITE;
	}

	v |= LAN78XX_MII_ACC_MII_BUSY;

	return v;
}

static int lan78xx_mdio_read16(struct lan78xx_ctx *ctx, uint8_t phy_id, uint8_t reg, uint16_t *out)
{
	uint32_t data = 0;
	int ret;

	if (!ctx || !ctx->udev || !out) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->ctrl_mutex, K_FOREVER);

	ret = lan78xx_mdio_wait_not_busy_nolock(ctx, LAN78XX_MDIO_TIMEOUT_MS);
	if (ret != 0) {
		goto out;
	}

	ret = lan78xx_write32_nolock(ctx, LAN78XX_MII_ACC, lan78xx_mii_acc(phy_id, reg, true));
	if (ret != 0) {
		goto out;
	}

	ret = lan78xx_mdio_wait_not_busy_nolock(ctx, LAN78XX_MDIO_TIMEOUT_MS);
	if (ret != 0) {
		goto out;
	}

	ret = lan78xx_read32_nolock(ctx, LAN78XX_MII_DATA, &data);
	if (ret != 0) {
		goto out;
	}

	*out = (uint16_t)(data & 0xFFFFu);

out:
	k_mutex_unlock(&ctx->ctrl_mutex);
	return ret;
}

static int lan78xx_find_phy(struct lan78xx_ctx *ctx, uint8_t *found)
{
	if (!ctx || !ctx->udev || !found) {
		return -EINVAL;
	}

	for (uint8_t phy = 0; phy < LAN78XX_PHY_ADDR_COUNT; phy++) {
		uint16_t id1 = 0;
		uint16_t id2 = 0;

		if (lan78xx_mdio_read16(ctx, phy, LAN78XX_MII_PHYID1, &id1) != 0) {
			continue;
		}
		if (lan78xx_mdio_read16(ctx, phy, LAN78XX_MII_PHYID2, &id2) != 0) {
			continue;
		}

		if (!lan78xx_phy_id_word_valid(id1) || !lan78xx_phy_id_word_valid(id2)) {
			continue;
		}

		*found = phy;
		LOG_INF("Found PHY at %u: ID1=0x%04x ID2=0x%04x", phy, id1, id2);
		return 0;
	}

	return -ENODEV;
}

static int lan78xx_link_is_up(struct lan78xx_ctx *ctx, bool *up)
{
	uint16_t bmcr = 0;
	uint16_t bmsr = 0;
	int ret;

	if (!ctx || !up || !ctx->udev || !ctx->phy_valid) {
		return -EINVAL;
	}

	ret = lan78xx_mdio_read16(ctx, ctx->phy_id, LAN78XX_MII_BMCR, &bmcr);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_mdio_read16(ctx, ctx->phy_id, LAN78XX_MII_BMSR, &bmsr);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_mdio_read16(ctx, ctx->phy_id, LAN78XX_MII_BMSR, &bmsr);
	if (ret != 0) {
		return ret;
	}

	*up = (bmsr & LAN78XX_MII_BMSR_LINK_STATUS) != 0 &&
	      (((bmcr & LAN78XX_MII_BMCR_AUTONEG_EN) == 0) ||
	       ((bmsr & LAN78XX_MII_BMSR_AUTONEG_COMPLETE) != 0));

	return 0;
}

struct lan78xx_ep_scan {
	int cur_if;
	int found_if;
	uint8_t bulk_in;
	uint8_t bulk_out;
	uint8_t intr_in;
	uint16_t bulk_in_mps;
	uint16_t bulk_out_mps;
	uint16_t intr_in_mps;
};

static int lan78xx_parse_desc_header(const uint8_t *desc, const uint8_t *end, uint8_t *len,
				     uint8_t *type)
{
	if ((desc + 2u) > end) {
		return -EINVAL;
	}

	*len = desc[0];
	*type = desc[1];

	if ((*len < 2u) || ((desc + *len) > end)) {
		return -EINVAL;
	}

	return 0;
}

static bool lan78xx_scan_iface_selected(const struct lan78xx_ep_scan *scan)
{
	return (scan->found_if >= 0) && (scan->cur_if != scan->found_if);
}

static bool lan78xx_scan_interface_desc(struct lan78xx_ep_scan *scan, const uint8_t *desc)
{
	const struct usb_if_descriptor *ifd = (const struct usb_if_descriptor *)desc;

	scan->cur_if = ifd->bInterfaceNumber;
	return lan78xx_scan_iface_selected(scan);
}

static void lan78xx_scan_endpoint_desc(struct lan78xx_ep_scan *scan, const uint8_t *desc)
{
	const struct usb_ep_descriptor *epd;
	uint8_t xfertype;
	bool in;
	uint16_t mps;

	if (scan->cur_if < 0 || lan78xx_scan_iface_selected(scan)) {
		return;
	}

	epd = (const struct usb_ep_descriptor *)desc;
	xfertype = epd->bmAttributes & LAN78XX_USB_EP_ATTR_MASK;
	in = (epd->bEndpointAddress & USB_EP_DIR_IN) != 0u;
	mps = sys_le16_to_cpu(epd->wMaxPacketSize) & LAN78XX_USB_EP_MPS_MASK;

	if (mps == 0u) {
		return;
	}

	if (xfertype == USB_EP_TYPE_BULK) {
		if (in) {
			scan->bulk_in = epd->bEndpointAddress;
			scan->bulk_in_mps = mps;
		} else {
			scan->bulk_out = epd->bEndpointAddress;
			scan->bulk_out_mps = mps;
		}
	} else if (xfertype == USB_EP_TYPE_INTERRUPT) {
		if (in) {
			scan->intr_in = epd->bEndpointAddress;
			scan->intr_in_mps = mps;
		}
	} else {
		/* Ignore endpoint types this driver does not use. */
	}

	if (scan->bulk_in != 0u && scan->bulk_out != 0u && scan->found_if < 0) {
		scan->found_if = scan->cur_if;
	}
}

static int lan78xx_store_scanned_endpoints(struct lan78xx_ctx *ctx,
					   const struct lan78xx_ep_scan *scan)
{
	if ((scan->found_if < 0) || (scan->bulk_in == 0u) || (scan->bulk_out == 0u) ||
	    (scan->bulk_in_mps == 0u) || (scan->bulk_out_mps == 0u)) {
		return -ENODEV;
	}

	ctx->ep_ids.ifnum = (uint8_t)scan->found_if;
	ctx->ep_ids.bulk_in = scan->bulk_in;
	ctx->ep_ids.bulk_out = scan->bulk_out;
	ctx->ep_ids.intr_in = scan->intr_in;
	ctx->ep_mps.bulk_in = scan->bulk_in_mps;
	ctx->ep_mps.bulk_out = scan->bulk_out_mps;
	ctx->ep_mps.intr_in = scan->intr_in_mps;

	return 0;
}

static int lan78xx_parse_endpoints(struct lan78xx_ctx *ctx)
{
	const struct usb_cfg_descriptor *cfg;
	const uint8_t *p;
	const uint8_t *end;
	struct lan78xx_ep_scan scan = {
		.cur_if = -1,
		.found_if = -1,
	};

	if (!ctx || !ctx->udev || !ctx->udev->cfg_desc) {
		return -EAGAIN;
	}

	p = (const uint8_t *)ctx->udev->cfg_desc;
	if (p[0] < sizeof(struct usb_cfg_descriptor) || p[1] != USB_DESC_CONFIGURATION) {
		return -EINVAL;
	}

	cfg = (const struct usb_cfg_descriptor *)p;
	if (sys_le16_to_cpu(cfg->wTotalLength) < sizeof(*cfg) ||
	    sys_le16_to_cpu(cfg->wTotalLength) > LAN78XX_CFG_DESC_MAX_LEN) {
		return -EINVAL;
	}

	end = p + sys_le16_to_cpu(cfg->wTotalLength);

	while ((p + 2) <= end) {
		uint8_t len = 0u;
		uint8_t type = 0u;
		bool done = false;
		int ret = lan78xx_parse_desc_header(p, end, &len, &type);

		if (ret != 0) {
			return -EINVAL;
		}

		switch (type) {
		case USB_DESC_INTERFACE:
			done = lan78xx_scan_interface_desc(&scan, p);
			break;
		case USB_DESC_ENDPOINT:
			lan78xx_scan_endpoint_desc(&scan, p);
			break;
		default:
			break;
		}

		if (done) {
			break;
		}

		p += len;
	}

	if (lan78xx_store_scanned_endpoints(ctx, &scan) != 0) {
		return -ENODEV;
	}

	LOG_INF("LAN78xx endpoints: if=%u bulk_in=0x%02x mps=%u bulk_out=0x%02x mps=%u "
		"intr_in=0x%02x mps=%u",
		ctx->ep_ids.ifnum, ctx->ep_ids.bulk_in, ctx->ep_mps.bulk_in, ctx->ep_ids.bulk_out,
		ctx->ep_mps.bulk_out, ctx->ep_ids.intr_in, ctx->ep_mps.intr_in);

	return 0;
}

static int lan78xx_wait_hwcfg_clear(struct lan78xx_ctx *ctx, uint32_t mask)
{
	uint32_t v = 0;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(LAN78XX_HW_RESET_TIMEOUT_MS));

	while (!sys_timepoint_expired(end)) {
		int ret = lan78xx_read32(ctx, LAN78XX_HW_CFG, &v);

		if (ret != 0) {
			return ret;
		}

		if ((v & mask) == 0u) {
			return 0;
		}

		k_sleep(K_USEC(LAN78XX_HW_CFG_POLL_DELAY_US));
	}

	LOG_ERR("timeout waiting HW_CFG clear mask=0x%08x (HW_CFG=0x%08x)", mask, v);
	return -ETIMEDOUT;
}

static int lan78xx_hw_reset(struct lan78xx_ctx *ctx)
{
	uint32_t hw = 0;
	int ret = lan78xx_read32(ctx, LAN78XX_HW_CFG, &hw);

	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_HW_CFG, hw | LAN78XX_HW_CFG_LRST);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_wait_hwcfg_clear(ctx, LAN78XX_HW_CFG_LRST);
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_USEC(LAN78XX_HW_RESET_SETTLE_US));
	return 0;
}

static uint32_t lan78xx_hwcfg_led_bits_from_mask(uint32_t mask)
{
	uint32_t v = 0;

	WRITE_BIT(v, 20, IS_BIT_SET(mask, 0));
	WRITE_BIT(v, 21, IS_BIT_SET(mask, 1));
	WRITE_BIT(v, 22, IS_BIT_SET(mask, 2));
	WRITE_BIT(v, 23, IS_BIT_SET(mask, 3));

	return v;
}

static int lan78xx_enable_led_outputs(struct lan78xx_ctx *ctx)
{
	uint32_t hw_cfg;
	uint32_t led_bits;
	int ret;

	ret = lan78xx_read32(ctx, LAN78XX_HW_CFG, &hw_cfg);
	if (ret != 0) {
		return ret;
	}

	led_bits = lan78xx_hwcfg_led_bits_from_mask(ctx->cfg->led_enable_mask);
	if (led_bits == 0U) {
		return 0;
	}

	hw_cfg |= led_bits;

	ret = lan78xx_write32(ctx, LAN78XX_HW_CFG, hw_cfg);
	if (ret == 0) {
		LOG_INF("Enabled LAN78xx LED outputs mask=0x%x hwcfg_bits=0x%x",
			ctx->cfg->led_enable_mask, led_bits);
	}

	return ret;
}

static int lan78xx_start_data_path(struct lan78xx_ctx *ctx)
{
	uint32_t v = 0;
	int ret;

	ret = lan78xx_read32(ctx, LAN78XX_MAC_TX, &v);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_MAC_TX, v | LAN78XX_MAC_TX_TXEN);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_FCT_TX_CTL, LAN78XX_FCT_TX_CTL_EN);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_FCT_RX_CTL, LAN78XX_FCT_RX_CTL_EN);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_read32(ctx, LAN78XX_MAC_RX, &v);
	if (ret != 0) {
		return ret;
	}

	return lan78xx_write32(ctx, LAN78XX_MAC_RX, v | LAN78XX_MAC_RX_RXEN);
}

static void lan78xx_flush_fifos(struct lan78xx_ctx *ctx)
{
	(void)lan78xx_update32(ctx, LAN78XX_FCT_RX_CTL, LAN78XX_FCT_RX_CTL_RST,
			       LAN78XX_FCT_RX_CTL_RST);
	(void)lan78xx_update32(ctx, LAN78XX_FCT_TX_CTL, LAN78XX_FCT_TX_CTL_RST,
			       LAN78XX_FCT_TX_CTL_RST);
	k_busy_wait(LAN78XX_FIFO_FLUSH_DELAY_US);
}

static int lan78xx_hw_init(struct lan78xx_ctx *ctx)
{
	uint32_t v = 0;
	uint32_t lo;
	uint32_t hi;
	const int64_t t0 = k_uptime_get();
	int ret;

	ret = lan78xx_hw_reset(ctx);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_read32(ctx, LAN78XX_HW_CFG, &v);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_HW_CFG, v | LAN78XX_HW_CFG_MEF);
	if (ret != 0) {
		return ret;
	}

	(void)lan78xx_write32(ctx, LAN78XX_BULK_IN_DLY, LAN78XX_BULK_IN_DLY_DEFAULT);
	(void)lan78xx_write32(ctx, LAN78XX_BURST_CAP, LAN78XX_BURST_CAP_DEFAULT);

	ret = lan78xx_read32(ctx, LAN78XX_USB_CFG0, &v);
	if (ret != 0) {
		return ret;
	}

	v |= LAN78XX_USB_CFG0_BCE | LAN78XX_USB_CFG0_BIR;

	ret = lan78xx_write32(ctx, LAN78XX_USB_CFG0, v);
	if (ret != 0) {
		return ret;
	}

	lo = (uint32_t)ctx->mac[0] | ((uint32_t)ctx->mac[1] << 8) | ((uint32_t)ctx->mac[2] << 16) |
	     ((uint32_t)ctx->mac[3] << 24);
	hi = (uint32_t)ctx->mac[4] | ((uint32_t)ctx->mac[5] << 8);

	ret = lan78xx_write32(ctx, LAN78XX_RX_ADDRL, lo);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_RX_ADDRH, hi);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_read32(ctx, LAN78XX_RFE_CTL, &v);
	if (ret != 0) {
		return ret;
	}

	v |= LAN78XX_RFE_CTL_BCAST_EN | LAN78XX_RFE_CTL_DA_PERFECT | LAN78XX_RFE_CTL_UCAST_EN;

	ret = lan78xx_write32(ctx, LAN78XX_RFE_CTL, v);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_read32(ctx, LAN78XX_PMT_CTL, &v);
	if (ret != 0) {
		return ret;
	}

	ret = lan78xx_write32(ctx, LAN78XX_PMT_CTL, v | LAN78XX_PMT_CTL_PHY_RST);
	if (ret != 0) {
		return ret;
	}

	do {
		ret = lan78xx_read32(ctx, LAN78XX_PMT_CTL, &v);
		if (ret != 0) {
			return ret;
		}

		if ((v & LAN78XX_PMT_CTL_PHY_RST) == 0u && (v & LAN78XX_PMT_CTL_READY) != 0u) {
			break;
		}

		k_busy_wait(LAN78XX_MDIO_POLL_DELAY_US);
	} while ((k_uptime_get() - t0) < LAN78XX_PHY_READY_TIMEOUT_MS);

	if ((v & LAN78XX_PMT_CTL_READY) == 0u) {
		LOG_ERR("PHY not ready (PMT_CTL=0x%08x)", v);
		return -ETIMEDOUT;
	}

	ret = lan78xx_enable_auto_mac_link_mode(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cfg->led_enable_mask != 0u) {
		ret = lan78xx_enable_led_outputs(ctx);
		if (ret != 0) {
			LOG_WRN("Failed to enable LED outputs: %d", ret);
		}
	}

	return lan78xx_start_data_path(ctx);
}

static int lan78xx_hw_init_retry(struct lan78xx_ctx *ctx)
{
	int ret = -EIO;

	for (int attempt = 0; attempt < LAN78XX_HW_INIT_RETRY_COUNT; attempt++) {
		if (attempt != 0) {
			k_msleep(LAN78XX_HW_INIT_RETRY_DELAY_MS);
		}

		ret = lan78xx_hw_init(ctx);
		if (ret == 0) {
			return 0;
		}

		LOG_WRN("LAN78xx hw init retry %d failed: %d", attempt + 1, ret);
	}

	return ret;
}

static void lan78xx_rx_submit_frame(struct lan78xx_ctx *ctx, uint8_t *buf, uint32_t frame_len)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, frame_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		return;
	}

	if (net_pkt_write(pkt, buf, frame_len) != 0) {
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

static void lan78xx_rx_deliver_frames(struct lan78xx_ctx *ctx, uint8_t *buf, size_t len)
{
	if (!ctx || !ctx->iface) {
		return;
	}

	while (len >= LAN78XX_RX_FRAME_PREFIX_LEN) {
		uint32_t rx_cmd_a = sys_get_le32(buf);
		uint32_t size;
		uint32_t align;

		buf += sizeof(uint32_t);
		len -= sizeof(uint32_t);

		(void)sys_get_le32(buf);
		buf += sizeof(uint32_t);
		len -= sizeof(uint32_t);

		(void)sys_get_le16(buf);
		buf += sizeof(uint16_t);
		len -= sizeof(uint16_t);

		size = rx_cmd_a & LAN78XX_RX_CMD_A_LEN_MASK;
		if (size < LAN78XX_ETH_FCS_LEN || size > (uint32_t)LAN78XX_CFG_RX_BUF_SIZE ||
		    size > len) {
			return;
		}

		if ((rx_cmd_a & LAN78XX_RX_CMD_A_RED) == 0u) {
			lan78xx_rx_submit_frame(ctx, buf, size - LAN78XX_ETH_FCS_LEN);
		}

		buf += size;
		len -= size;

		align = (LAN78XX_RX_FRAME_ALIGN -
			 ((size + LAN78XX_RX_PADDING) % LAN78XX_RX_FRAME_ALIGN)) %
			LAN78XX_RX_FRAME_ALIGN;
		if (len < align) {
			return;
		}

		buf += align;
		len -= align;
	}
}

static int lan78xx_rx_done(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct lan78xx_rx_slot *slot = xfer ? xfer->priv : NULL;
	struct lan78xx_ctx *ctx = slot ? slot->ctx : NULL;

	if (!ctx || !udev || !xfer || !xfer->buf) {
		return 0;
	}

	if (atomic_get(&ctx->rx_stopping) || ctx->udev != udev) {
		atomic_set(&slot->state, LAN78XX_RX_SLOT_FREE);
		lan78xx_io_signal(ctx);
		return 0;
	}

	if (xfer->err != 0 && xfer->err != -EAGAIN) {
		LOG_WRN("RX done err=%d len=%u", xfer->err,
			xfer->buf ? (unsigned int)xfer->buf->len : 0u);
	}

	atomic_set(&slot->state, LAN78XX_RX_SLOT_DONE);
	lan78xx_io_signal(ctx);
	return 0;
}

static int lan78xx_intr_done(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct lan78xx_intr_slot *slot = xfer ? xfer->priv : NULL;
	struct lan78xx_ctx *ctx = slot ? slot->ctx : NULL;

	if (!ctx || !udev || !xfer || !xfer->buf) {
		return 0;
	}

	if (atomic_get(&ctx->intr_stopping) || ctx->udev != udev) {
		atomic_set(&slot->state, LAN78XX_INTR_STATE_IDLE);
		lan78xx_io_signal(ctx);
		return 0;
	}

	if (xfer->err != 0 && xfer->err != -EAGAIN) {
		LOG_WRN("INTR done err=%d len=%u", xfer->err,
			xfer->buf ? (unsigned int)xfer->buf->len : 0u);
	}

	atomic_set(&slot->state, LAN78XX_INTR_STATE_DONE);
	lan78xx_io_signal(ctx);
	return 0;
}

static int lan78xx_rx_rearm_slot(struct lan78xx_ctx *ctx, struct lan78xx_rx_slot *slot)
{
	int ret;

	if (!ctx || !slot || !slot->xfer || !slot->xfer->buf || !ctx->udev) {
		return -EINVAL;
	}

	net_buf_reset(slot->xfer->buf);
	slot->xfer->mps = ctx->ep_mps.bulk_in;
	slot->xfer->buf->len = LAN78XX_CFG_RX_BUF_SIZE;

	ret = lan78xx_norm_async_ok(usbh_xfer_enqueue(ctx->udev, slot->xfer));
	if (ret == 0) {
		atomic_set(&slot->state, LAN78XX_RX_SLOT_ARMED);
	} else {
		atomic_set(&slot->state, LAN78XX_RX_SLOT_FREE);
	}

	return ret;
}

static int lan78xx_intr_rearm(struct lan78xx_ctx *ctx)
{
	struct lan78xx_intr_slot *slot = &ctx->intr_slot;
	int ret;

	if (!ctx || !slot->xfer || !slot->xfer->buf || !ctx->udev) {
		return -EINVAL;
	}

	net_buf_reset(slot->xfer->buf);
	slot->xfer->mps = ctx->ep_mps.intr_in;
	slot->xfer->buf->len = LAN78XX_INTR_LEN;

	ret = lan78xx_norm_async_ok(usbh_xfer_enqueue(ctx->udev, slot->xfer));
	if (ret == 0) {
		atomic_set(&slot->state, LAN78XX_INTR_STATE_ARMED);
	} else {
		atomic_set(&slot->state, LAN78XX_INTR_STATE_IDLE);
	}

	return ret;
}

static void lan78xx_rx_slot_free(struct lan78xx_ctx *ctx, struct lan78xx_rx_slot *slot)
{
	if (!ctx || !slot || !slot->xfer || !ctx->udev) {
		return;
	}

	(void)usbh_xfer_free(ctx->udev, slot->xfer);
	slot->xfer = NULL;
	atomic_set(&slot->state, LAN78XX_RX_SLOT_FREE);
}

static void lan78xx_intr_free(struct lan78xx_ctx *ctx)
{
	struct lan78xx_intr_slot *slot = &ctx->intr_slot;

	if (!ctx || !slot->xfer || !ctx->udev) {
		return;
	}

	(void)usbh_xfer_free(ctx->udev, slot->xfer);
	slot->xfer = NULL;
	atomic_set(&slot->state, LAN78XX_INTR_STATE_IDLE);
}

static int lan78xx_start_rx(struct lan78xx_ctx *ctx)
{
	const uint8_t want = LAN78XX_CFG_RX_XFERS;

	if (!ctx || !ctx->udev || ctx->ep_ids.bulk_in == 0u || want == 0u) {
		return -EINVAL;
	}

	atomic_clear(&ctx->rx_stopping);
	ctx->rx_count = 0u;

	for (uint8_t n = want; n >= 1u; n = (uint8_t)(n / 2u)) {
		bool ok = true;
		uint8_t allocated = 0u;

		for (uint8_t i = 0; i < want; i++) {
			ctx->rx_slots[i].ctx = ctx;
			ctx->rx_slots[i].xfer = NULL;
			atomic_set(&ctx->rx_slots[i].state, LAN78XX_RX_SLOT_FREE);
		}

		for (uint8_t i = 0; i < n; i++) {
			struct lan78xx_rx_slot *slot = &ctx->rx_slots[i];
			int ret;

			ret = lan78xx_alloc_transfer_with_buffer(
				ctx, ctx->ep_ids.bulk_in, ctx->ep_mps.bulk_in, lan78xx_rx_done,
				slot, LAN78XX_CFG_RX_BUF_SIZE, &slot->xfer);
			if (ret != 0) {
				ok = false;
				break;
			}

			atomic_set(&slot->state, LAN78XX_RX_SLOT_FREE);
			allocated = (uint8_t)(i + 1u);
		}

		if (!ok) {
			for (uint8_t i = 0; i < allocated; i++) {
				lan78xx_rx_slot_free(ctx, &ctx->rx_slots[i]);
			}
			continue;
		}

		ctx->rx_count = allocated;

		for (uint8_t i = 0; i < ctx->rx_count; i++) {
			int ret = lan78xx_rx_rearm_slot(ctx, &ctx->rx_slots[i]);

			if (ret != 0) {
				return ret;
			}
		}

		if (n != want) {
			LOG_WRN("RX memory tight: using rx_xfers=%u (requested %u)", n, want);
		}

		return 0;
	}

	return -ENOMEM;
}

static void lan78xx_stop_rx(struct lan78xx_ctx *ctx)
{
	if (!ctx || ctx->rx_count == 0u) {
		return;
	}

	atomic_set(&ctx->rx_stopping, 1);

	if (ctx->udev) {
		for (uint8_t i = 0; i < ctx->rx_count; i++) {
			struct lan78xx_rx_slot *slot = &ctx->rx_slots[i];

			if (!slot->xfer) {
				continue;
			}

			if (usbh_xfer_dequeue(ctx->udev, slot->xfer) != 0) {
				lan78xx_rx_slot_free(ctx, slot);
			}
		}
	}

	lan78xx_io_signal(ctx);
}

static int lan78xx_start_intr(struct lan78xx_ctx *ctx)
{
	struct lan78xx_intr_slot *slot;
	int ret;

	if (!ctx || !ctx->udev) {
		return -EINVAL;
	}

	if (ctx->ep_ids.intr_in == 0u) {
		return 0;
	}

	atomic_clear(&ctx->intr_stopping);

	slot = &ctx->intr_slot;
	memset(slot, 0, sizeof(*slot));
	slot->ctx = ctx;

	ret = lan78xx_alloc_transfer_with_buffer(ctx, ctx->ep_ids.intr_in, ctx->ep_mps.intr_in,
						 lan78xx_intr_done, slot, LAN78XX_INTR_LEN,
						 &slot->xfer);
	if (ret != 0) {
		return ret;
	}

	atomic_set(&slot->state, LAN78XX_INTR_STATE_IDLE);

	ret = lan78xx_intr_rearm(ctx);
	if (ret != 0) {
		lan78xx_intr_free(ctx);
		return ret;
	}

	return 0;
}

static void lan78xx_stop_intr(struct lan78xx_ctx *ctx)
{
	struct lan78xx_intr_slot *slot = &ctx->intr_slot;

	if (!ctx || !slot->xfer) {
		return;
	}

	atomic_set(&ctx->intr_stopping, 1);

	if (ctx->udev && usbh_xfer_dequeue(ctx->udev, slot->xfer) != 0) {
		lan78xx_intr_free(ctx);
	}

	lan78xx_io_signal(ctx);
}

static struct lan78xx_tx_slot *lan78xx_tx_acquire_slot(struct lan78xx_ctx *ctx)
{
	struct lan78xx_tx_slot *slot = NULL;

	k_mutex_lock(&ctx->tx_lock, K_FOREVER);
	for (uint8_t i = 0; i < ctx->tx_pool_depth; i++) {
		if (atomic_cas(&ctx->tx_slots[i].state, LAN78XX_TX_SLOT_FREE,
			       LAN78XX_TX_SLOT_BUSY)) {
			slot = &ctx->tx_slots[i];
			break;
		}
	}
	k_mutex_unlock(&ctx->tx_lock);

	return slot;
}

static void lan78xx_tx_release_slot(struct lan78xx_tx_slot *slot)
{
	if (!slot || !slot->ctx) {
		return;
	}

	atomic_set(&slot->state, LAN78XX_TX_SLOT_FREE);
	k_sem_give(&slot->ctx->tx_free_sem);
}

static int lan78xx_tx_done(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct lan78xx_tx_slot *slot = xfer ? xfer->priv : NULL;

	ARG_UNUSED(udev);

	if (!slot) {
		LOG_ERR("TX done with null slot");
		return 0;
	}

	if (xfer->err != 0 && xfer->err != -ECONNRESET) {
		LOG_WRN("TX done slot=%u err=%d usb_len=%u mps=%u", slot->index, xfer->err,
			xfer->buf ? (unsigned int)xfer->buf->len : 0u, (unsigned int)xfer->mps);
	}

	if (slot->pkt != NULL) {
		net_pkt_unref(slot->pkt);
		slot->pkt = NULL;
		xfer->buf = slot->buf;
	}

	if (xfer->buf) {
		net_buf_reset(xfer->buf);
	}

	lan78xx_tx_release_slot(slot);
	return 0;
}

static void lan78xx_tx_pool_deinit(struct lan78xx_ctx *ctx)
{
	if (!ctx) {
		return;
	}

	if (ctx->udev) {
		for (uint8_t i = 0; i < ctx->tx_pool_depth; i++) {
			struct lan78xx_tx_slot *slot = &ctx->tx_slots[i];

			if (slot->xfer) {
				(void)usbh_xfer_free(ctx->udev, slot->xfer);
				slot->xfer = NULL;
				slot->buf = NULL;
			}
		}
	}

	ctx->tx_pool_depth = 0u;
}

static int lan78xx_tx_pool_init(struct lan78xx_ctx *ctx)
{
	uint8_t depth;
	size_t max_wire;
	size_t max_usb;

	if (!ctx || !ctx->udev || ctx->ep_ids.bulk_out == 0u) {
		return -EINVAL;
	}

	if (ctx->tx_pool_depth != 0u) {
		return 0;
	}

	depth = LAN78XX_CFG_TX_INFLIGHT;
	ctx->tx_pool_depth = depth;

	max_wire = MAX((size_t)LAN78XX_ETH_MAX_NOFCS, (size_t)LAN78XX_ETH_MIN_NOFCS);
	max_usb = LAN78XX_TX_USB_LEN(max_wire);

	k_sem_init(&ctx->tx_free_sem, depth, depth);

	for (uint8_t i = 0; i < depth; i++) {
		struct lan78xx_tx_slot *slot = &ctx->tx_slots[i];
		int ret;

		memset(slot, 0, sizeof(*slot));
		slot->ctx = ctx;
		slot->index = i;
		atomic_set(&slot->state, LAN78XX_TX_SLOT_FREE);

		ret = lan78xx_alloc_transfer_with_buffer(ctx, ctx->ep_ids.bulk_out,
							 ctx->ep_mps.bulk_out, lan78xx_tx_done,
							 slot, max_usb, &slot->xfer);
		if (ret != 0) {
			lan78xx_tx_pool_deinit(ctx);
			return ret;
		}

		slot->buf = slot->xfer->buf;
	}

	return 0;
}

static void lan78xx_set_carrier(struct lan78xx_ctx *ctx, bool up)
{
	ctx->link_up = up;
	__ASSERT_NO_MSG(ctx->iface != NULL);

	if (up) {
		net_eth_carrier_on(ctx->iface);
	} else {
		net_eth_carrier_off(ctx->iface);
	}
}

static enum ethernet_hw_caps lan78xx_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static const struct device *lan78xx_get_phy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return NULL;
}

static int lan78xx_get_config(const struct device *dev, enum ethernet_config_type type,
			      struct ethernet_config *config)
{
	ARG_UNUSED(dev);

	if (!config) {
		return -EINVAL;
	}

	if (type == ETHERNET_CONFIG_TYPE_EXTRA_TX_PKT_HEADROOM) {
		config->extra_tx_pkt_headroom = LAN78XX_CMD_HDR_LEN;
		return 0;
	}

	return -ENOTSUP;
}

static int lan78xx_try_zerocopy_tx(struct lan78xx_ctx *ctx, struct lan78xx_tx_slot *slot,
				   struct net_pkt *pkt, uint32_t wire_len, uint32_t tx_pad)
{
	struct net_buf *frag = pkt->frags;
	struct uhc_transfer *xfer = slot->xfer;
	uint8_t *hdr;
	uint32_t tx_cmd_a;
	int ret;

	if (frag == NULL || frag->frags != NULL || frag->len != wire_len) {
		return -ENOTSUP;
	}

	if (net_buf_headroom(frag) < LAN78XX_CMD_HDR_LEN || net_buf_tailroom(frag) < tx_pad) {
		return -ENOTSUP;
	}

	tx_cmd_a = (wire_len & LAN78XX_TX_CMD_A_LEN_MASK) | LAN78XX_TX_CMD_A_FCS;
	hdr = net_buf_push(frag, LAN78XX_CMD_HDR_LEN);
	sys_put_le32(tx_cmd_a, hdr);
	sys_put_le32(0u, hdr + sizeof(uint32_t));

	if (tx_pad != 0u) {
		memset(net_buf_add(frag, tx_pad), 0, tx_pad);
	}

	xfer->buf = frag;
	xfer->mps = ctx->ep_mps.bulk_out;

	ret = lan78xx_norm_async_ok(usbh_xfer_enqueue(ctx->udev, xfer));
	if (ret != 0) {
		if (tx_pad != 0u) {
			(void)net_buf_remove_mem(frag, tx_pad);
		}

		(void)net_buf_pull(frag, LAN78XX_CMD_HDR_LEN);
		xfer->buf = slot->buf;
		return ret;
	}

	slot->pkt = net_pkt_ref(pkt);

	return 0;
}

static int lan78xx_send(const struct device *dev, struct net_pkt *pkt)
{
	struct lan78xx_ctx *ctx = dev->data;
	struct lan78xx_tx_slot *slot;
	struct net_buf *nb;
	struct uhc_transfer *xfer;
	uint8_t *dst;
	size_t plen;
	uint32_t wire_len;
	uint32_t tx_pad;
	uint32_t usb_len;
	uint32_t tx_cmd_a;
	int ret;

	if (!ctx || !pkt) {
		LOG_ERR("TX invalid ctx=%p pkt=%p", ctx, pkt);
		return -EINVAL;
	}

	if (!ctx->connected || !ctx->link_up || !ctx->udev || !ctx->iface ||
	    ctx->ep_ids.bulk_out == 0u) {
		return -ENETDOWN;
	}

	if (ctx->tx_pool_depth == 0u) {
		return -EAGAIN;
	}

	plen = net_pkt_get_len(pkt);
	if (plen == 0u || plen > LAN78XX_ETH_MAX_NOFCS) {
		LOG_ERR("TX invalid plen=%u", (unsigned int)plen);
		return -EINVAL;
	}

	if (k_sem_take(&ctx->tx_free_sem, K_MSEC(LAN78XX_TX_WAIT_MS)) != 0) {
		LOG_WRN("TX busy: no free slot depth=%u", ctx->tx_pool_depth);
		return -EAGAIN;
	}

	slot = lan78xx_tx_acquire_slot(ctx);
	if (!slot || !slot->xfer || !slot->buf) {
		if (slot) {
			lan78xx_tx_release_slot(slot);
		} else {
			k_sem_give(&ctx->tx_free_sem);
		}

		LOG_ERR("TX failed acquiring slot slot=%p", slot);
		return -EIO;
	}

	nb = slot->buf;
	xfer = slot->xfer;
	dst = nb->data;

	net_buf_reset(nb);

	wire_len = (uint32_t)plen;
	tx_pad = LAN78XX_TX_PAD(wire_len);

	ret = lan78xx_try_zerocopy_tx(ctx, slot, pkt, wire_len, tx_pad);
	if (ret == 0) {
		return 0;
	}

	if (ret != -ENOTSUP) {
		LOG_WRN("TX enqueue failed slot=%u ret=%d plen=%u", slot->index, ret,
			(unsigned int)plen);
		lan78xx_tx_release_slot(slot);
		return ret;
	}

	usb_len = LAN78XX_TX_USB_LEN(wire_len);
	if (usb_len > nb->size) {
		LOG_ERR("TX buffer too small usb_len=%u buf_size=%u", (unsigned int)usb_len,
			(unsigned int)nb->size);
		lan78xx_tx_release_slot(slot);
		return -ENOMEM;
	}

	xfer->buf = nb;
	tx_cmd_a = (wire_len & LAN78XX_TX_CMD_A_LEN_MASK) | LAN78XX_TX_CMD_A_FCS;
	sys_put_le32(tx_cmd_a, dst + 0);
	sys_put_le32(0u, dst + 4);

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, dst + LAN78XX_CMD_HDR_LEN, plen) != 0) {
		LOG_ERR("TX net_pkt_read failed");
		lan78xx_tx_release_slot(slot);
		return -EIO;
	}

	if (tx_pad != 0u) {
		memset(dst + LAN78XX_CMD_HDR_LEN + wire_len, 0, tx_pad);
	}

	nb->len = usb_len;
	xfer->mps = ctx->ep_mps.bulk_out;

	ret = lan78xx_norm_async_ok(usbh_xfer_enqueue(ctx->udev, xfer));
	if (ret != 0) {
		LOG_WRN("TX enqueue failed slot=%u plen=%u usb_len=%u pad=%u", slot->index,
			(unsigned int)plen, (unsigned int)usb_len, (unsigned int)tx_pad);
		lan78xx_tx_release_slot(slot);
		return ret;
	}

	return 0;
}

static void lan78xx_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct lan78xx_ctx *ctx = dev->data;

	ctx->iface = iface;
	net_if_set_link_addr(iface, ctx->mac, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
	ethernet_init(iface);
	lan78xx_start_workers(ctx);
	lan78xx_set_carrier(ctx, ctx->link_up);
}

static const struct ethernet_api lan78xx_eth_api = {
	.iface_api.init = lan78xx_iface_init,
	.get_capabilities = lan78xx_get_capabilities,
	.get_config = lan78xx_get_config,
	.get_phy = lan78xx_get_phy,
	.send = lan78xx_send,
};

static int lan78xx_enable_auto_mac_link_mode(struct lan78xx_ctx *ctx)
{
	return lan78xx_update32(ctx, LAN78XX_MAC_CR, LAN78XX_MAC_CR_ASD | LAN78XX_MAC_CR_ADD,
				LAN78XX_MAC_CR_ASD | LAN78XX_MAC_CR_ADD);
}

static void lan78xx_on_link_change(struct lan78xx_ctx *ctx, bool up)
{
	if (up) {
		lan78xx_flush_fifos(ctx);
		(void)lan78xx_start_data_path(ctx);
	}

	lan78xx_set_carrier(ctx, up);
}

static void lan78xx_poll_link_state(struct lan78xx_ctx *ctx)
{
	bool up = false;
	int ret = lan78xx_link_is_up(ctx, &up);

	if (ret == 0) {
		if (up != ctx->link_up) {
			LOG_INF("Link %s", up ? "UP" : "DOWN");
			lan78xx_on_link_change(ctx, up);
		}

		ctx->next_link_poll_at = k_uptime_get() + LAN78XX_CFG_LINK_POLL_MS;
	} else {
		ctx->next_link_poll_at = k_uptime_get() + MAX(LAN78XX_CFG_LINK_POLL_MS,
							      LAN78XX_LINK_POLL_ERROR_DELAY_MS);
	}
}

static int lan78xx_probe_control_path(struct lan78xx_ctx *ctx)
{
	uint32_t v = 0;
	bool up = false;
	int ret = lan78xx_read32(ctx, LAN78XX_ID_REV, &v);

	if (ret != 0) {
		return ret;
	}

	LOG_INF("ID_REV=0x%08x", v);

	if (!ctx->phy_valid) {
		uint8_t phy = 0;

		ret = lan78xx_find_phy(ctx, &phy);
		if (ret != 0) {
			return ret;
		}

		ctx->phy_id = phy;
		ctx->phy_valid = true;
	}

	ret = lan78xx_link_is_up(ctx, &up);
	if (ret == 0) {
		lan78xx_set_carrier(ctx, up);
	} else {
		lan78xx_set_carrier(ctx, false);
	}

	return 0;
}

static void lan78xx_disconnect(struct lan78xx_ctx *ctx)
{
	if (!ctx) {
		return;
	}

	ctx->connected = false;
	ctx->ctrl_ok = false;
	ctx->phy_id = 0;
	ctx->phy_valid = false;
	ctx->next_link_poll_at = LAN78XX_NO_POLL_SCHEDULED;

	lan78xx_set_carrier(ctx, false);

	if (ctx->udev) {
		lan78xx_stop_intr(ctx);
		lan78xx_stop_rx(ctx);
		lan78xx_tx_pool_deinit(ctx);
	}

	ctx->udev = NULL;
	ctx->ep_ids.ifnum = 0;
	ctx->ep_ids.bulk_in = 0;
	ctx->ep_ids.bulk_out = 0;
	ctx->ep_ids.intr_in = 0;
	ctx->ep_mps.bulk_in = 0u;
	ctx->ep_mps.bulk_out = 0u;
	ctx->ep_mps.intr_in = 0u;
}

static void lan78xx_attach_device(struct lan78xx_ctx *ctx)
{
	int ret;

	LOG_INF("LAN78xx attach: enter udev=%p connected=%d", ctx->udev, ctx->connected);

	if (!ctx->udev || ctx->connected) {
		return;
	}

	ctx->ctrl_ok = false;

	if (!lan78xx_udev_ready(ctx->udev)) {
		LOG_INF("LAN78xx attach: udev not ready");
		return;
	}

	ret = lan78xx_parse_endpoints(ctx);
	if (ret == 0 && ctx->cfg->alt_setting != 0u) {
		ret = usbh_req_set_alt(ctx->udev, ctx->ep_ids.ifnum, ctx->cfg->alt_setting);
	}

	if (ret != 0) {
		LOG_ERR("LAN78xx attach: endpoint/alt bind failed: %d", ret);
		lan78xx_disconnect(ctx);
		return;
	}

	k_msleep(LAN78XX_ATTACH_POST_BIND_DELAY_MS);

	ret = lan78xx_hw_init_retry(ctx);
	if (ret != 0) {
		LOG_ERR("LAN78xx attach: hw init failed: %d", ret);
		ctx->attach_attempt++;
		if (ctx->attach_attempt < LAN78XX_ATTACH_RETRY_MAX) {
			k_msleep(LAN78XX_ATTACH_HW_INIT_DELAY_MS);
			lan78xx_mgmt_signal(ctx, LAN78XX_MGMT_EVT_ATTACH);
			return;
		}

		lan78xx_disconnect(ctx);
		return;
	}

	ret = lan78xx_probe_control_path(ctx);
	if (ret != 0) {
		ctx->attach_attempt++;
		LOG_WRN("LAN78xx attach: probe failed ret=%d attempt=%u", ret, ctx->attach_attempt);
		if (ctx->attach_attempt < LAN78XX_ATTACH_RETRY_MAX) {
			k_msleep(LAN78XX_ATTACH_RETRY_DELAY_MS);
			lan78xx_mgmt_signal(ctx, LAN78XX_MGMT_EVT_ATTACH);
			return;
		}

		lan78xx_disconnect(ctx);
		return;
	}

	ctx->ctrl_ok = true;
	ctx->attach_attempt = 0;

	ret = lan78xx_start_intr(ctx);
	if (ret != 0) {
		LOG_ERR("LAN78xx attach: start_intr failed: %d", ret);
		lan78xx_disconnect(ctx);
		return;
	}

	ret = lan78xx_start_rx(ctx);
	if (ret != 0) {
		LOG_ERR("LAN78xx attach: start_rx failed: %d", ret);
		lan78xx_disconnect(ctx);
		return;
	}

	ret = lan78xx_tx_pool_init(ctx);
	if (ret != 0) {
		LOG_ERR("LAN78xx attach: tx pool init failed: %d", ret);
		lan78xx_disconnect(ctx);
		return;
	}

	ctx->connected = true;
	ctx->next_link_poll_at = k_uptime_get() + LAN78XX_INITIAL_LINK_POLL_DELAY_MS;
	lan78xx_mgmt_signal(ctx, LAN78XX_MGMT_EVT_WAKE);

	LOG_INF("LAN78xx attached: addr=%u if=%u ep_in=0x%02x mps=%u ep_out=0x%02x mps=%u "
		"ep_int=0x%02x mps=%u tx_depth=%u rx_count=%u",
		ctx->udev->addr, ctx->ep_ids.ifnum, ctx->ep_ids.bulk_in, ctx->ep_mps.bulk_in,
		ctx->ep_ids.bulk_out, ctx->ep_mps.bulk_out, ctx->ep_ids.intr_in,
		ctx->ep_mps.intr_in, ctx->tx_pool_depth, ctx->rx_count);
}

static void lan78xx_mgmt_thread_fn(void *p1, void *p2, void *p3)
{
	struct lan78xx_ctx *ctx = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		k_timeout_t timeout = K_FOREVER;
		int64_t now = k_uptime_get();
		atomic_val_t flags;

		if (ctx->connected && ctx->ctrl_ok && ctx->udev &&
		    ctx->next_link_poll_at != LAN78XX_NO_POLL_SCHEDULED) {
			if (ctx->next_link_poll_at <= now) {
				timeout = K_NO_WAIT;
			} else {
				timeout = K_MSEC(ctx->next_link_poll_at - now);
			}
		}

		(void)k_sem_take(&ctx->mgmt_sem, timeout);

		flags = atomic_set(&ctx->mgmt_flags, 0);

		if ((flags & LAN78XX_MGMT_EVT_DETACH) != 0) {
			lan78xx_disconnect(ctx);
			continue;
		}

		if ((flags & LAN78XX_MGMT_EVT_ATTACH) != 0) {
			lan78xx_attach_device(ctx);
		}

		if (ctx->connected && ctx->ctrl_ok && ctx->udev &&
		    ctx->next_link_poll_at != LAN78XX_NO_POLL_SCHEDULED &&
		    k_uptime_get() >= ctx->next_link_poll_at) {
			lan78xx_poll_link_state(ctx);
		}
	}
}

static void lan78xx_service_rx_slot(struct lan78xx_ctx *ctx, struct lan78xx_rx_slot *slot,
				    uint8_t slot_idx)
{
	atomic_val_t state = atomic_get(&slot->state);

	if (state == LAN78XX_RX_SLOT_DONE) {
		if (!atomic_get(&ctx->rx_stopping) && ctx->udev && slot->xfer->err == 0 &&
		    slot->xfer->buf && slot->xfer->buf->len > 0 && ctx->connected && ctx->link_up &&
		    ctx->iface) {
			lan78xx_rx_deliver_frames(ctx, slot->xfer->buf->data, slot->xfer->buf->len);
		}

		if (!atomic_get(&ctx->rx_stopping) && ctx->udev) {
			int ret = lan78xx_rx_rearm_slot(ctx, slot);

			if (ret != 0) {
				LOG_WRN("RX rearm failed: slot=%u ret=%d", slot_idx, ret);
			}
		} else {
			lan78xx_rx_slot_free(ctx, slot);
		}
		return;
	}

	if (state == LAN78XX_RX_SLOT_FREE && atomic_get(&ctx->rx_stopping)) {
		lan78xx_rx_slot_free(ctx, slot);
		return;
	}
}

static void lan78xx_service_intr_slot(struct lan78xx_ctx *ctx)
{
	atomic_val_t state = atomic_get(&ctx->intr_slot.state);

	if (state == LAN78XX_INTR_STATE_DONE) {
		if (!atomic_get(&ctx->intr_stopping) && ctx->udev &&
		    (ctx->intr_slot.xfer->err == 0 || ctx->intr_slot.xfer->err == -EAGAIN)) {
			int ret = lan78xx_intr_rearm(ctx);

			if (ret != 0) {
				LOG_WRN("INTR rearm failed: ret=%d", ret);
			}
		} else {
			lan78xx_intr_free(ctx);
		}
		return;
	}

	if (state == LAN78XX_INTR_STATE_IDLE && atomic_get(&ctx->intr_stopping)) {
		lan78xx_intr_free(ctx);
		return;
	}
}

static void lan78xx_io_thread_fn(void *p1, void *p2, void *p3)
{
	struct lan78xx_ctx *ctx = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		(void)k_sem_take(&ctx->io_sem, K_FOREVER);

		for (uint8_t i = 0; i < ctx->rx_count; i++) {
			struct lan78xx_rx_slot *slot = &ctx->rx_slots[i];

			if (!slot->xfer) {
				continue;
			}

			lan78xx_service_rx_slot(ctx, slot, i);
		}

		if (ctx->intr_slot.xfer) {
			lan78xx_service_intr_slot(ctx);
		}

		if (atomic_get(&ctx->rx_stopping) && ctx->rx_count != 0u) {
			bool all_gone = true;

			for (uint8_t i = 0; i < ctx->rx_count; i++) {
				if (ctx->rx_slots[i].xfer != NULL) {
					all_gone = false;
					break;
				}
			}

			if (all_gone) {
				ctx->rx_count = 0u;
			}
		}
	}
}

static int lan78xx_usbh_request(struct usbh_class_data *const c_data,
				struct uhc_transfer *const xfer)
{
	ARG_UNUSED(c_data);
	ARG_UNUSED(xfer);
	return 0;
}

static int lan78xx_usbh_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	LOG_INF("LAN78xx USB host class initialized");
	return 0;
}

static int lan78xx_usbh_connected(struct usbh_class_data *const c_data,
				  struct usb_device *const udev, const uint8_t iface)
{
	struct lan78xx_ctx *ctx = &lan78xx_ctx_0;

	ARG_UNUSED(c_data);
	ARG_UNUSED(iface);

	if (!udev) {
		return -ENOTSUP;
	}

	if (!lan78xx_match_vidpid(&udev->dev_desc)) {
		return -ENOTSUP;
	}

	LOG_INF("LAN78xx USB host probe matched addr=%u", udev->addr);
	ctx->udev = udev;
	lan78xx_mgmt_signal(ctx, LAN78XX_MGMT_EVT_ATTACH);

	return 0;
}

static int lan78xx_usbh_removed(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);
	lan78xx_mgmt_signal(&lan78xx_ctx_0, LAN78XX_MGMT_EVT_DETACH);
	return 0;
}

static struct usbh_class_api lan78xx_usbh_api = {
	.init = lan78xx_usbh_init,
	.completion_cb = lan78xx_usbh_request,
	.probe = lan78xx_usbh_connected,
	.removed = lan78xx_usbh_removed,
	.suspended = NULL,
	.resumed = NULL,
};

static const struct usbh_class_filter lan78xx_usbh_filters[] = {
	{
		.vid = LAN78XX_USB_VID,
		.pid = LAN78XX_USB_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0}};

USBH_DEFINE_CLASS(lan78xx, &lan78xx_usbh_api, NULL, lan78xx_usbh_filters);

static int lan78xx_init(const struct device *dev)
{
	struct lan78xx_ctx *ctx = dev->data;
	int ret;

	ctx->dev = dev;

	LOG_INF("LAN78xx net device init: vid=0x%04x pid=0x%04x", LAN78XX_USB_VID, LAN78XX_USB_PID);

	k_mutex_init(&ctx->ctrl_mutex);
	k_mutex_init(&ctx->tx_lock);

	ctx->udev = NULL;
	ctx->ep_ids.ifnum = 0;
	ctx->ep_ids.bulk_in = 0;
	ctx->ep_ids.bulk_out = 0;
	ctx->ep_ids.intr_in = 0;
	ctx->ep_mps.bulk_in = 0u;
	ctx->ep_mps.bulk_out = 0u;
	ctx->ep_mps.intr_in = 0u;

	ctx->connected = false;
	ctx->link_up = false;
	ctx->ctrl_ok = false;

	ctx->phy_id = 0;
	ctx->phy_valid = false;
	ctx->attach_attempt = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ctx->rx_slots); i++) {
		ctx->rx_slots[i].ctx = ctx;
		ctx->rx_slots[i].xfer = NULL;
		atomic_set(&ctx->rx_slots[i].state, LAN78XX_RX_SLOT_FREE);
	}
	ctx->rx_count = 0u;
	atomic_clear(&ctx->rx_stopping);

	memset(&ctx->intr_slot, 0, sizeof(ctx->intr_slot));
	ctx->intr_slot.ctx = ctx;
	atomic_clear(&ctx->intr_stopping);

	for (size_t i = 0; i < ARRAY_SIZE(ctx->tx_slots); i++) {
		ctx->tx_slots[i].ctx = ctx;
		ctx->tx_slots[i].xfer = NULL;
		ctx->tx_slots[i].buf = NULL;
		ctx->tx_slots[i].index = (uint8_t)i;
		atomic_set(&ctx->tx_slots[i].state, LAN78XX_TX_SLOT_FREE);
	}
	ctx->tx_pool_depth = 0u;

	ctx->next_link_poll_at = LAN78XX_NO_POLL_SCHEDULED;

	atomic_set(&ctx->mgmt_flags, 0);
	k_sem_init(&ctx->mgmt_sem, 0, UINT_MAX);
	k_sem_init(&ctx->io_sem, 0, UINT_MAX);

	ret = lan78xx_mac_load(ctx);
	if (ret != 0) {
		return ret;
	}

	k_sem_init(&ctx->tx_free_sem, 0, UINT_MAX);

	return 0;
}

ETH_NET_DEVICE_DT_INST_DEFINE(0, lan78xx_init, NULL, &lan78xx_ctx_0, &lan78xx_cfg_0,
			      CONFIG_ETH_INIT_PRIORITY, &lan78xx_eth_api, NET_ETH_MTU);
