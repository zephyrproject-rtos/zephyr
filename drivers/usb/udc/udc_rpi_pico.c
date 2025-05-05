/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2021 Pete Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <string.h>
#include <stdio.h>

#include <soc.h>
#include <hardware/structs/usb.h>
#include <hardware/resets.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_rpi_pico, CONFIG_UDC_DRIVER_LOG_LEVEL);

struct rpi_pico_config {
	usb_hw_t *base;
	usb_device_dpram_t *dpram;
	sys_mem_blocks_t *mem_block;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	const struct device *clk_dev;
	struct pinctrl_dev_config *const pcfg;
	clock_control_subsys_t clk_sys;
};

struct rpi_pico_ep_data {
	void *buf;
	uint8_t next_pid;
};

enum rpi_pico_event_type {
	/* Setup packet received */
	RPI_PICO_EVT_SETUP,
	/* Trigger new transfer (except control OUT) */
	RPI_PICO_EVT_XFER_NEW,
	/* Transfer for specific endpoint is finished */
	RPI_PICO_EVT_XFER_FINISHED,
};

struct rpi_pico_data {
	struct k_thread thread_data;
	/*
	 * events are events that the driver thread waits.
	 * xfer_new and xfer_finished contain information on which endpoints
	 * events RPI_PICO_EVT_XFER_NEW or RPI_PICO_EVT_XFER_FINISHED are
	 * triggered. The mapping is bits 31..16 for IN endpoints and bits
	 * 15..0 for OUT endpoints.
	 */
	struct k_event events;
	atomic_t xfer_new;
	atomic_t xfer_finished;
	struct rpi_pico_ep_data out_ep[USB_NUM_ENDPOINTS];
	struct rpi_pico_ep_data in_ep[USB_NUM_ENDPOINTS];
	bool rwu_pending;
	uint8_t setup[8];
};

static inline int udc_ep_to_bnum(const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		return 16UL + USB_EP_GET_IDX(ep);
	}

	return USB_EP_GET_IDX(ep);
}

static inline uint8_t udc_pull_ep_from_bmsk(uint32_t *const bitmap)
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

/* Use Atomic Register Access to set bits */
static void ALWAYS_INLINE rpi_pico_bit_set(const mm_reg_t reg, const uint32_t bit)
{
	sys_write32(bit, REG_ALIAS_SET_BITS | reg);
}

/* Use Atomic Register Access to clear bits */
static void ALWAYS_INLINE rpi_pico_bit_clr(const mm_reg_t reg, const uint32_t bit)
{
	sys_write32(bit, REG_ALIAS_CLR_BITS | reg);
}


static void sie_dp_pullup(const struct device *dev, const bool enable)
{
	const struct rpi_pico_config *config = dev->config;
	usb_hw_t *base = config->base;

	if (enable) {
		rpi_pico_bit_set((mm_reg_t)&base->sie_ctrl, USB_SIE_CTRL_PULLUP_EN_BITS);
	} else {
		rpi_pico_bit_clr((mm_reg_t)&base->sie_ctrl, USB_SIE_CTRL_PULLUP_EN_BITS);
	}
}

static void ALWAYS_INLINE sie_status_clr(const struct device *dev, const uint32_t bit)
{
	const struct rpi_pico_config *config = dev->config;
	usb_hw_t *base = config->base;

	rpi_pico_bit_clr((mm_reg_t)&base->sie_status, bit);
}

static inline uint32_t get_ep_mask(const uint8_t ep)
{
	const int idx = USB_EP_GET_IDX(ep) * 2 + USB_EP_DIR_IS_OUT(ep);

	return BIT(idx);
}

/* Get the address of an endpoint control register */
static mem_addr_t get_ep_ctrl_reg(const struct device *dev, const uint8_t ep)
{
	const struct rpi_pico_config *config = dev->config;
	usb_device_dpram_t *dpram = config->dpram;

	if (USB_EP_GET_IDX(ep) == 0) {
		return 0UL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		return (uintptr_t)&dpram->ep_ctrl[USB_EP_GET_IDX(ep) - 1].out;
	}

	return (uintptr_t)&dpram->ep_ctrl[USB_EP_GET_IDX(ep) - 1].in;
}

/* Get the address of an endpoint buffer control register */
static mem_addr_t get_buf_ctrl_reg(const struct device *dev, const uint8_t ep)
{
	const struct rpi_pico_config *config = dev->config;
	usb_device_dpram_t *dpram = config->dpram;

	if (USB_EP_DIR_IS_OUT(ep)) {
		return (uintptr_t)&dpram->ep_buf_ctrl[USB_EP_GET_IDX(ep)].out;
	}

	return (uintptr_t)&dpram->ep_buf_ctrl[USB_EP_GET_IDX(ep)].in;
}

/* Get the address of an endpoint buffer control register */
static struct rpi_pico_ep_data *get_ep_data(const struct device *dev, const uint8_t ep)
{
	struct rpi_pico_data *priv = udc_get_private(dev);

	if (USB_EP_DIR_IS_OUT(ep)) {
		return &priv->out_ep[USB_EP_GET_IDX(ep)];
	}

	return &priv->in_ep[USB_EP_GET_IDX(ep)];
}

static uint32_t read_buf_ctrl_reg(const struct device *dev, const uint8_t ep)
{
	mm_reg_t buf_ctrl_reg = get_buf_ctrl_reg(dev, ep);

	return sys_read32(buf_ctrl_reg);
}

static void write_buf_ctrl_reg(const struct device *dev, const uint8_t ep,
			       const uint32_t buf_ctrl)
{
	mm_reg_t buf_ctrl_reg = get_buf_ctrl_reg(dev, ep);

	sys_write32(buf_ctrl, buf_ctrl_reg);
}

static void write_ep_ctrl_reg(const struct device *dev, const uint8_t ep,
			      const uint32_t ep_ctrl)
{
	mm_reg_t ep_ctrl_reg =  get_ep_ctrl_reg(dev, ep);

	sys_write32(ep_ctrl, ep_ctrl_reg);
}

static void rpi_pico_ep_cancel(const struct device *dev, const uint8_t ep)
{
	bool abort_handshake_supported = rp2040_chip_version() >= 2;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	const struct rpi_pico_config *config = dev->config;
	usb_hw_t *base = config->base;
	mm_reg_t abort_done_reg = (mm_reg_t)&base->abort_done;
	mm_reg_t abort_reg = (mm_reg_t)&base->abort;
	const uint32_t ep_mask = get_ep_mask(ep);
	uint32_t buf_ctrl;

	buf_ctrl = read_buf_ctrl_reg(dev, ep);
	if (!(buf_ctrl & USB_BUF_CTRL_AVAIL)) {
		/* The buffer is not used by the controller */
		udc_ep_set_busy(ep_cfg, false);
		return;
	}

	if (abort_handshake_supported) {
		rpi_pico_bit_set(abort_reg, ep_mask);
		while ((sys_read32(abort_done_reg) & ep_mask) != ep_mask) {
		}
	}

	buf_ctrl &= ~USB_BUF_CTRL_AVAIL;
	write_buf_ctrl_reg(dev, ep, buf_ctrl);

	if (abort_handshake_supported) {
		rpi_pico_bit_clr(abort_reg, ep_mask);
	}

	udc_ep_set_busy(ep_cfg, false);
	LOG_INF("Canceled ep 0x%02x transaction", ep);
}

static int rpi_pico_prep_rx(const struct device *dev,
			    struct net_buf *const buf, struct udc_ep_config *const cfg)
{
	struct rpi_pico_ep_data *const ep_data = get_ep_data(dev, cfg->addr);
	unsigned int lock_key;
	uint32_t buf_ctrl;

	buf_ctrl = read_buf_ctrl_reg(dev, cfg->addr);
	if (buf_ctrl & USB_BUF_CTRL_AVAIL) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", cfg->addr);
		return -EBUSY;
	}

	LOG_DBG("Prepare RX ep 0x%02x len %u pid: %u",
		cfg->addr, net_buf_tailroom(buf), ep_data->next_pid);

	lock_key = irq_lock();

	buf_ctrl = cfg->mps;
	buf_ctrl |= ep_data->next_pid ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID;
	ep_data->next_pid ^= 1U;

	write_buf_ctrl_reg(dev, cfg->addr, buf_ctrl);
	/*
	 * By default, clk_sys runs at 125MHz, wait 3 nop instructions before
	 * setting the AVAILABLE bit. See 4.1.2.5.1. Concurrent access.
	 */
	arch_nop();
	arch_nop();
	arch_nop();
	write_buf_ctrl_reg(dev, cfg->addr, buf_ctrl | USB_BUF_CTRL_AVAIL);

	irq_unlock(lock_key);

	return 0;
}

static int rpi_pico_prep_tx(const struct device *dev,
			    struct net_buf *const buf, struct udc_ep_config *const cfg)
{
	struct rpi_pico_ep_data *const ep_data = get_ep_data(dev, cfg->addr);
	unsigned int lock_key;
	uint32_t buf_ctrl;
	size_t len;

	buf_ctrl = read_buf_ctrl_reg(dev, cfg->addr);
	if (buf_ctrl & USB_BUF_CTRL_AVAIL) {
		LOG_ERR("ep 0x%02x buffer is used by the controller", cfg->addr);
		return -EBUSY;
	}

	lock_key = irq_lock();

	len = MIN(cfg->mps, buf->len);
	memcpy(ep_data->buf, buf->data, len);

	LOG_DBG("Prepare TX ep 0x%02x len %u pid: %u",
		cfg->addr, len, ep_data->next_pid);

	buf_ctrl = len;
	buf_ctrl |= ep_data->next_pid ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID;
	buf_ctrl |= USB_BUF_CTRL_FULL;
	ep_data->next_pid ^= 1U;

	write_buf_ctrl_reg(dev, cfg->addr, buf_ctrl);
	/*
	 * By default, clk_sys runs at 125MHz, wait 3 nop instructions before
	 * setting the AVAILABLE bit. See 4.1.2.5.1. Concurrent access.
	 */
	arch_nop();
	arch_nop();
	arch_nop();
	write_buf_ctrl_reg(dev, cfg->addr, buf_ctrl | USB_BUF_CTRL_AVAIL);

	irq_unlock(lock_key);

	return 0;
}

static int rpi_pico_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return rpi_pico_prep_rx(dev, buf, ep_cfg);
}

static void drop_control_transfers(const struct device *dev)
{
	struct udc_ep_config *cfg_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_ep_config *cfg_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *buf;

	buf = udc_buf_get_all(cfg_out);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(cfg_in);
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

static int rpi_pico_handle_evt_setup(const struct device *dev)
{
	struct rpi_pico_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	drop_control_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOMEM;
	}

	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "setup");

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);

		err = rpi_pico_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err != 0) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s:%p|feed for -in-status", buf);
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p|no data", buf);
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static inline int rpi_pico_handle_evt_dout(const struct device *dev,
					   struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_get(cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for OUT ep 0x%02x", cfg->addr);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENODATA;
	}

	udc_ep_set_busy(cfg, false);

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			LOG_DBG("dout:%p|status, feed >s", buf);

			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
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

static int rpi_pico_handle_evt_din(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	int err;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", cfg->addr);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}

	buf = udc_buf_get(cfg);
	udc_ep_set_busy(cfg, false);

	if (cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/* IN transfer finished, submit buffer for status stage */
			net_buf_unref(buf);

			err = rpi_pico_ctrl_feed_dout(dev, 0);
			if (err == -ENOMEM) {
				err = udc_submit_ep_event(dev, buf, err);
			}
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static void rpi_pico_handle_xfer_next(const struct device *dev,
				      struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	int err;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		if (cfg->stat.halted) {
			return;
		}

		err = rpi_pico_prep_rx(dev, buf, cfg);
	} else {
		err = rpi_pico_prep_tx(dev, buf, cfg);
	}

	if (err != 0) {
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	} else {
		udc_ep_set_busy(cfg, true);
	}
}

static ALWAYS_INLINE void rpi_pico_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct rpi_pico_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(RPI_PICO_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(RPI_PICO_EVT_XFER_FINISHED));

		eps = atomic_clear(&priv->xfer_finished);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("Finished event ep 0x%02x", ep);

			if (USB_EP_DIR_IS_IN(ep)) {
				rpi_pico_handle_evt_din(dev, ep_cfg);
			} else {
				rpi_pico_handle_evt_dout(dev, ep_cfg);
			}

			if (!udc_ep_is_busy(ep_cfg)) {
				rpi_pico_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint 0x%02x busy", ep);
			}
		}
	}

	if (evt & BIT(RPI_PICO_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(RPI_PICO_EVT_XFER_NEW));

		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("New transfer ep 0x%02x in the queue", ep);

			if (!udc_ep_is_busy(ep_cfg)) {
				rpi_pico_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint 0x%02x busy", ep);
			}
		}
	}

	if (evt & BIT(RPI_PICO_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(RPI_PICO_EVT_SETUP));
		LOG_DBG("SETUP event");
		rpi_pico_handle_evt_setup(dev);
	}

	udc_unlock_internal(dev);
}

static void rpi_pico_handle_setup(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	struct rpi_pico_data *priv = udc_get_private(dev);
	usb_device_dpram_t *dpram = config->dpram;
	/*
	 * Host may issue a new setup packet even if the previous control transfer
	 * did not complete. Cancel any active transaction.
	 */
	rpi_pico_ep_cancel(dev, USB_CONTROL_EP_IN);
	rpi_pico_ep_cancel(dev, USB_CONTROL_EP_OUT);

	sys_put_le32(sys_read32((uintptr_t)&dpram->setup_packet[0]), &priv->setup[0]);
	sys_put_le32(sys_read32((uintptr_t)&dpram->setup_packet[4]), &priv->setup[4]);

	/* Set DATA1 PID for the next (data or status) stage */
	get_ep_data(dev, USB_CONTROL_EP_IN)->next_pid = 1;
	get_ep_data(dev, USB_CONTROL_EP_OUT)->next_pid = 1;

	k_event_post(&priv->events, BIT(RPI_PICO_EVT_SETUP));
}

static void rpi_pico_handle_buff_status_in(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct rpi_pico_data *priv = udc_get_private(dev);
	__maybe_unused int err;
	struct net_buf *buf;
	size_t len;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	len = read_buf_ctrl_reg(dev, ep) & USB_BUF_CTRL_LEN_MASK;
	net_buf_pull(buf, len);

	if (buf->len) {
		err = rpi_pico_prep_tx(dev, buf, ep_cfg);
		__ASSERT(err == 0, "Failed to start new IN transaction");
	} else {
		if (udc_ep_buf_has_zlp(buf)) {
			err = rpi_pico_prep_tx(dev, buf, ep_cfg);
			__ASSERT(err == 0, "Failed to start new IN transaction");
			udc_ep_buf_clear_zlp(buf);
		} else {
			atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
			k_event_post(&priv->events, BIT(RPI_PICO_EVT_XFER_FINISHED));
		}
	}
}

static void rpi_pico_handle_buff_status_out(const struct device *dev, const uint8_t ep)
{
	struct rpi_pico_ep_data *ep_data = get_ep_data(dev, ep);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct rpi_pico_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	size_t len;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	len = read_buf_ctrl_reg(dev, ep) & USB_BUF_CTRL_LEN_MASK;
	net_buf_add_mem(buf, ep_data->buf, MIN(len, net_buf_tailroom(buf)));

	if (net_buf_tailroom(buf) && len == udc_mps_ep_size(ep_cfg)) {
		__unused int err;

		err = rpi_pico_prep_rx(dev, buf, ep_cfg);
		__ASSERT(err == 0, "Failed to start new OUT transaction");
	} else {
		atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
		k_event_post(&priv->events, BIT(RPI_PICO_EVT_XFER_FINISHED));
	}
}

static void rpi_pico_handle_buff_status(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	usb_hw_t *base = config->base;
	uint32_t buf_status;
	uint8_t ep;

	buf_status = sys_read32((mm_reg_t)&base->buf_status);

	for (unsigned int i = 0; buf_status && i < USB_NUM_ENDPOINTS * 2; i++) {
		if (!(buf_status & BIT(i))) {
			continue;
		}

		rpi_pico_bit_clr((mm_reg_t)&base->buf_status, BIT(i));
		/* Odd bits in the register correspond to OUT endpoints */
		if (i & 1U) {
			ep = USB_EP_DIR_OUT | (i >> 1U);
			rpi_pico_handle_buff_status_out(dev, ep);
		} else {
			ep = USB_EP_DIR_IN | (i >> 1U);
			rpi_pico_handle_buff_status_in(dev, ep);
		}

		buf_status &= ~BIT(i);
	}
}

static void rpi_pico_isr_handler(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	struct rpi_pico_data *priv = udc_get_private(dev);
	usb_hw_t *base = config->base;
	uint32_t status = base->ints;
	uint32_t handled = 0;

	if (status & USB_INTS_DEV_SOF_BITS) {
		handled |= USB_INTS_DEV_SOF_BITS;
		sys_read32((mm_reg_t)&base->sof_rd);
	}

	if (status & USB_INTS_DEV_CONN_DIS_BITS) {
		uint32_t sie_status;

		sie_status = sys_read32((mm_reg_t)&base->sie_status);
		LOG_DBG("CONNECTED bit %u, VBUS_DETECTED bit %u",
			(bool)(sie_status & USB_SIE_STATUS_CONNECTED_BITS),
			(bool)(sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS));

		if (pcfg != NULL && !(sie_status & USB_SIE_STATUS_CONNECTED_BITS) &&
		    !(sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS)) {
			sie_dp_pullup(dev, false);
			udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
		}

		handled |= USB_INTS_DEV_CONN_DIS_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_CONNECTED_BITS);
	}

	if (status & USB_INTS_VBUS_DETECT_BITS) {
		uint32_t sie_status;

		sie_status = sys_read32((mm_reg_t)&base->sie_status);
		LOG_DBG("VBUS_DETECTED bit %u",
			(bool)(sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS));

		if (pcfg != NULL && (sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS)) {
			sie_dp_pullup(dev, true);
			udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
		}

		handled |= USB_INTS_VBUS_DETECT_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_VBUS_DETECTED_BITS);
	}

	if ((status & (USB_INTS_BUFF_STATUS_BITS | USB_INTS_SETUP_REQ_BITS)) &&
	    priv->rwu_pending) {
		/* The rpi pico USB device does not appear to be sending
		 * USB_INTR_DEV_RESUME_FROM_HOST interrupts when the resume is
		 * a result of a remote wakeup request sent by us.
		 * This will simulate a resume event if bus activity is observed
		 */

		priv->rwu_pending = false;
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (status & USB_INTR_DEV_RESUME_FROM_HOST_BITS) {
		handled |= USB_INTR_DEV_RESUME_FROM_HOST_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_RESUME_BITS);

		priv->rwu_pending = false;
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (status & USB_INTS_DEV_SUSPEND_BITS) {
		handled |= USB_INTS_DEV_SUSPEND_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_SUSPENDED_BITS);

		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if (status & USB_INTS_BUS_RESET_BITS) {
		handled |= USB_INTS_BUS_RESET_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_BUS_RESET_BITS);

		sys_write32(0, (mm_reg_t)&base->dev_addr_ctrl);
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if (status & USB_INTS_ERROR_DATA_SEQ_BITS) {
		handled |= USB_INTS_ERROR_DATA_SEQ_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_DATA_SEQ_ERROR_BITS);
		/*
		 * This can be triggered before the STALL handshake response
		 * to the OUT DATAx. Handling IRQ_ON_STALL to fix the expected
		 * DATA PID is too much overhead since the endpoint is halted
		 * anyway.
		 */
		LOG_WRN("Data Sequence Error");
	}

	if (status & USB_INTS_ERROR_RX_TIMEOUT_BITS) {
		handled |= USB_INTS_ERROR_RX_TIMEOUT_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_RX_TIMEOUT_BITS);

		LOG_ERR("RX timeout");
		udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
	}

	if (status & USB_INTS_ERROR_RX_OVERFLOW_BITS) {
		sie_status_clr(dev, USB_SIE_STATUS_RX_OVERFLOW_BITS);
		handled |= USB_INTS_ERROR_RX_OVERFLOW_BITS;

		LOG_ERR("RX overflow");
		udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
	}

	if (status & USB_INTS_ERROR_BIT_STUFF_BITS) {
		handled |= USB_INTS_ERROR_BIT_STUFF_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_BIT_STUFF_ERROR_BITS);

		LOG_ERR("Bit Stuff Error");
		udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
	}

	if (status & USB_INTS_ERROR_CRC_BITS) {
		handled |= USB_INTS_ERROR_CRC_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_CRC_ERROR_BITS);

		LOG_ERR("CRC Error");
		udc_submit_event(dev, UDC_EVT_ERROR, -EINVAL);
	}

	/*
	 * Here both interrupt flags BUF_STATUS and SETUP_REQ may be set at
	 * the same time, e.g. because of the interrupt latency. Check
	 * BUF_STATUS interrupt first to get the notifications in the right
	 * order.
	 */
	if (status & USB_INTS_BUFF_STATUS_BITS) {
		handled |= USB_INTS_BUFF_STATUS_BITS;
		rpi_pico_handle_buff_status(dev);
	}

	if (status & USB_INTS_SETUP_REQ_BITS) {
		handled |= USB_INTS_SETUP_REQ_BITS;
		sie_status_clr(dev, USB_SIE_STATUS_SETUP_REC_BITS);

		rpi_pico_handle_setup(dev);
	}

	if (status ^ handled) {
		LOG_ERR("Unhandled IRQ: 0x%x", status ^ handled);
	}
}

static int udc_rpi_pico_ep_enqueue(const struct device *dev,
				   struct udc_ep_config *const cfg,
				   struct net_buf *buf)
{
	struct rpi_pico_data *priv = udc_get_private(dev);

	udc_buf_put(cfg, buf);

	if (!cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(cfg->addr));
		k_event_post(&priv->events, BIT(RPI_PICO_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_rpi_pico_ep_dequeue(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	rpi_pico_ep_cancel(dev, cfg->addr);
	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	irq_unlock(lock_key);

	return 0;
}

static int udc_rpi_pico_ep_enable(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	struct rpi_pico_ep_data *const ep_data = get_ep_data(dev, cfg->addr);
	const struct rpi_pico_config *config = dev->config;
	uint8_t type  = cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;
	usb_device_dpram_t *dpram = config->dpram;
	int err;

	write_buf_ctrl_reg(dev, cfg->addr, USB_BUF_CTRL_DATA0_PID);
	ep_data->next_pid = 0;

	if (USB_EP_GET_IDX(cfg->addr) != 0) {
		uint32_t ep_ctrl = EP_CTRL_ENABLE_BITS |
				   EP_CTRL_INTERRUPT_PER_BUFFER |
				   (type << EP_CTRL_BUFFER_TYPE_LSB);
		size_t blocks = DIV_ROUND_UP(cfg->mps, 64U);

		err = sys_mem_blocks_alloc(config->mem_block, blocks, &ep_data->buf);
		if (err != 0) {
			LOG_ERR("Failed to allocate %zu memory blocks for ep 0x%02x",
				cfg->addr, blocks);
			return err;
		}

		ep_ctrl |= (uintptr_t)ep_data->buf & 0xFFFFUL;
		write_ep_ctrl_reg(dev, cfg->addr, ep_ctrl);
	} else {
		ep_data->buf = dpram->ep0_buf_a;
	}

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_rpi_pico_ep_disable(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	struct rpi_pico_ep_data *const ep_data = get_ep_data(dev, cfg->addr);
	const struct rpi_pico_config *config = dev->config;
	int err;

	rpi_pico_ep_cancel(dev, cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) != 0) {
		size_t blocks = DIV_ROUND_UP(cfg->mps, 64U);

		write_ep_ctrl_reg(dev, cfg->addr, 0UL);
		err = sys_mem_blocks_free(config->mem_block, blocks, &ep_data->buf);
		if (err != 0) {
			LOG_ERR("Failed to free memory blocks");
			return err;
		}
	}

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_rpi_pico_ep_set_halt(const struct device *dev,
				    struct udc_ep_config *const cfg)
{
	const struct rpi_pico_config *config = dev->config;
	mem_addr_t buf_ctrl_reg = get_buf_ctrl_reg(dev, cfg->addr);
	usb_hw_t *base = config->base;
	unsigned int lock_key;
	uint32_t bits;

	lock_key = irq_lock();
	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		bits = USB_EP_DIR_IS_OUT(cfg->addr) ?
		       USB_EP_STALL_ARM_EP0_OUT_BITS : USB_EP_STALL_ARM_EP0_IN_BITS;
		rpi_pico_bit_set((mm_reg_t)&base->ep_stall_arm, bits);
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		/*
		 * Cancel any transfer in progress.  The available bit must be
		 * set for the controller to respond to OUT DATAx with a STALL
		 * handshake.
		 */
		rpi_pico_ep_cancel(dev, cfg->addr);
		bits = USB_BUF_CTRL_STALL | USB_BUF_CTRL_AVAIL;
	} else {
		/* Only STALL bit needs to be set here. */
		bits = USB_BUF_CTRL_STALL;
	}

	rpi_pico_bit_set(buf_ctrl_reg, bits);

	if (USB_EP_GET_IDX(cfg->addr) != 0) {
		cfg->stat.halted = true;
	}

	irq_unlock(lock_key);
	LOG_DBG("Set halt ep 0x%02x buf_ctrl 0x%08x busy %u",
		cfg->addr, sys_read32(buf_ctrl_reg), udc_ep_is_busy(cfg));

	return 0;
}

static int udc_rpi_pico_ep_clear_halt(const struct device *dev,
				      struct udc_ep_config *const cfg)
{
	struct rpi_pico_ep_data *const ep_data = get_ep_data(dev, cfg->addr);
	struct rpi_pico_data *priv = udc_get_private(dev);
	mem_addr_t buf_ctrl_reg = get_buf_ctrl_reg(dev, cfg->addr);
	unsigned int lock_key;

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	lock_key = irq_lock();
	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		/* Cancel responds with a STALL handshake.*/
		rpi_pico_ep_cancel(dev, cfg->addr);
	} else {
		rpi_pico_bit_clr(buf_ctrl_reg, USB_BUF_CTRL_STALL);
	}

	ep_data->next_pid = 0;
	cfg->stat.halted = false;
	irq_unlock(lock_key);

	if (udc_ep_is_busy(cfg)) {
		rpi_pico_handle_xfer_next(dev, cfg);
	} else if (udc_buf_peek(cfg)) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(cfg->addr));
		k_event_post(&priv->events, BIT(RPI_PICO_EVT_XFER_NEW));
	}

	LOG_DBG("Clear halt ep 0x%02x buf_ctrl 0x%08x busy %u",
		cfg->addr, sys_read32(buf_ctrl_reg), udc_ep_is_busy(cfg));

	return 0;
}

static int udc_rpi_pico_set_address(const struct device *dev, const uint8_t addr)
{
	const struct rpi_pico_config *config = dev->config;
	usb_hw_t *base = config->base;

	sys_write32(addr, (mm_reg_t)&base->dev_addr_ctrl);
	LOG_DBG("Set new address %u for %s", addr, dev->name);

	return 0;
}

static int udc_rpi_pico_host_wakeup(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	struct rpi_pico_data *priv = udc_get_private(dev);
	usb_hw_t *base = config->base;

	rpi_pico_bit_set((mm_reg_t)&base->sie_ctrl, USB_SIE_CTRL_RESUME_BITS);
	priv->rwu_pending = true;

	LOG_DBG("Remote wakeup from %s", dev->name);

	return 0;
}

static int udc_rpi_pico_enable(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	usb_device_dpram_t *dpram = config->dpram;
	usb_hw_t *base = config->base;

	/* Reset USB controller */
	reset_block(RESETS_RESET_USBCTRL_BITS);
	unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

	/* Clear registers and DPRAM */
	memset(base, 0, sizeof(usb_hw_t));
	memset(dpram, 0, sizeof(usb_device_dpram_t));

	/* Connect USB controller to the onboard PHY */
	sys_write32(USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS,
		    (mm_reg_t)&base->muxing);

	if (pcfg == NULL) {
		/* Force VBUS detect so the device thinks it is plugged into a host */
		sys_write32(USB_USB_PWR_VBUS_DETECT_BITS |
			    USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS, (mm_reg_t)&base->pwr);
	}

	/* Enable an interrupt per EP0 transaction */
	sys_write32(USB_SIE_CTRL_EP0_INT_1BUF_BITS, (mm_reg_t)&base->sie_ctrl);

	/* Enable interrupts */
	sys_write32(USB_INTE_DEV_SOF_BITS |
		    USB_INTE_SETUP_REQ_BITS |
		    USB_INTE_DEV_RESUME_FROM_HOST_BITS |
		    USB_INTE_DEV_SUSPEND_BITS |
		    USB_INTE_DEV_CONN_DIS_BITS |
		    USB_INTE_BUS_RESET_BITS |
		    USB_INTE_VBUS_DETECT_BITS |
		    USB_INTE_ERROR_CRC_BITS |
		    USB_INTE_ERROR_BIT_STUFF_BITS |
		    USB_INTE_ERROR_RX_OVERFLOW_BITS |
		    USB_INTE_ERROR_RX_TIMEOUT_BITS |
		    USB_INTE_ERROR_DATA_SEQ_BITS |
		    USB_INTE_BUFF_STATUS_BITS,
		    (mm_reg_t)&base->inte);

	if (sys_read32((mm_reg_t)&base->sie_status) & USB_SIE_STATUS_VBUS_DETECTED_BITS) {
		/* Present full speed device by enabling pull up on DP */
		sie_dp_pullup(dev, true);
	}

	/* Enable the USB controller in device mode. */
	sys_write32(USB_MAIN_CTRL_CONTROLLER_EN_BITS, (mm_reg_t)&base->main_ctrl);

	config->irq_enable_func(dev);

	LOG_DBG("Enable device %s %p", dev->name, (void *)base);

	return 0;
}

static int udc_rpi_pico_disable(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;

	config->irq_disable_func(dev);
	LOG_DBG("Disable device %p", dev);

	return 0;
}

static int udc_rpi_pico_init(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	int err;

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

	if (pcfg != NULL) {
		err = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("Failed to apply default pinctrl state (%d)", err);
			return err;
		}
	}

	return clock_control_on(config->clk_dev, config->clk_sys);
}

static int udc_rpi_pico_shutdown(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return clock_control_off(config->clk_dev, config->clk_sys);
}

static int udc_rpi_pico_driver_preinit(const struct device *dev)
{
	const struct rpi_pico_config *config = dev->config;
	struct rpi_pico_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;

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

	return 0;
}

static void udc_rpi_pico_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_rpi_pico_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static const struct udc_api udc_rpi_pico_api = {
	.lock = udc_rpi_pico_lock,
	.unlock = udc_rpi_pico_unlock,
	.init = udc_rpi_pico_init,
	.enable = udc_rpi_pico_enable,
	.disable = udc_rpi_pico_disable,
	.shutdown = udc_rpi_pico_shutdown,
	.set_address = udc_rpi_pico_set_address,
	.host_wakeup = udc_rpi_pico_host_wakeup,
	.ep_enable = udc_rpi_pico_ep_enable,
	.ep_disable = udc_rpi_pico_ep_disable,
	.ep_set_halt = udc_rpi_pico_ep_set_halt,
	.ep_clear_halt = udc_rpi_pico_ep_clear_halt,
	.ep_enqueue = udc_rpi_pico_ep_enqueue,
	.ep_dequeue = udc_rpi_pico_ep_dequeue,
};

#define DT_DRV_COMPAT raspberrypi_pico_usbd

#define UDC_RPI_PICO_PINCTRL_DT_INST_DEFINE(n)						\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),				\
		    (PINCTRL_DT_INST_DEFINE(n)), ())

#define UDC_RPI_PICO_PINCTRL_DT_INST_DEV_CONFIG_GET(n)					\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),				\
		    ((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define UDC_RPI_PICO_DEVICE_DEFINE(n)							\
	UDC_RPI_PICO_PINCTRL_DT_INST_DEFINE(n);						\
	K_THREAD_STACK_DEFINE(udc_rpi_pico_stack_##n, CONFIG_UDC_RPI_PICO_STACK_SIZE);	\
											\
	SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(rpi_pico_mb_##n,			\
						  64U, 58U,				\
						  usb_dpram->epx_data)			\
											\
	static void udc_rpi_pico_thread_##n(void *dev, void *arg1, void *arg2)		\
	{										\
		while (true) {								\
			rpi_pico_thread_handler(dev);					\
		}									\
	}										\
											\
	static void udc_rpi_pico_make_thread_##n(const struct device *dev)		\
	{										\
		struct rpi_pico_data *priv = udc_get_private(dev);			\
											\
		k_thread_create(&priv->thread_data,					\
				udc_rpi_pico_stack_##n,					\
				K_THREAD_STACK_SIZEOF(udc_rpi_pico_stack_##n),		\
				udc_rpi_pico_thread_##n,				\
				(void *)dev, NULL, NULL,				\
				K_PRIO_COOP(CONFIG_UDC_RPI_PICO_THREAD_PRIORITY),	\
				K_ESSENTIAL,						\
				K_NO_WAIT);						\
		k_thread_name_set(&priv->thread_data, dev->name);			\
	}										\
											\
	static void udc_rpi_pico_irq_enable_func_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
			    DT_INST_IRQ(n, priority),					\
			    rpi_pico_isr_handler,					\
			    DEVICE_DT_INST_GET(n),					\
			    0);								\
											\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static void udc_rpi_pico_irq_disable_func_##n(const struct device *dev)		\
	{										\
		irq_disable(DT_INST_IRQN(n));						\
	}										\
											\
	static struct udc_ep_config ep_cfg_out[USB_NUM_ENDPOINTS];			\
	static struct udc_ep_config ep_cfg_in[USB_NUM_ENDPOINTS];			\
											\
	static const struct rpi_pico_config rpi_pico_config_##n = {			\
		.base = (usb_hw_t *)DT_INST_REG_ADDR(n),				\
		.dpram = (usb_device_dpram_t *)USBCTRL_DPRAM_BASE,			\
		.mem_block = &rpi_pico_mb_##n,						\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),			\
		.ep_cfg_in = ep_cfg_out,						\
		.ep_cfg_out = ep_cfg_in,						\
		.make_thread = udc_rpi_pico_make_thread_##n,				\
		.irq_enable_func = udc_rpi_pico_irq_enable_func_##n,			\
		.irq_disable_func = udc_rpi_pico_irq_disable_func_##n,			\
		.pcfg = UDC_RPI_PICO_PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clk_sys = (void *)DT_INST_PHA_BY_IDX(n, clocks, 0, clk_id),		\
	};										\
											\
	static struct rpi_pico_data udc_priv_##n = {					\
	};										\
											\
	static struct udc_data udc_data_##n = {						\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),			\
		.priv = &udc_priv_##n,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, udc_rpi_pico_driver_preinit, NULL,			\
			      &udc_data_##n, &rpi_pico_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &udc_rpi_pico_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_RPI_PICO_DEVICE_DEFINE)
