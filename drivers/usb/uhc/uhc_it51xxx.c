/*
 * Copyright (c) 2026 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uhc_common.h"

#include <soc.h>
#include <soc_dt.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_it51xxx, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT ite_it51xxx_uhc

#define IT51XXX_UHC_EXTEND_CTRL_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(ite_extend_ctrl)

#define UHC00_TX_CTRL_REGISTER 0x0
#define START_XFER             BIT(0)

#define UHC01_TX_TRANS_TYPE 0x1
#define TRANSACTION_TYPE(x) FIELD_PREP(GENMASK(1, 0), x)

#define UHC02_TX_LINE_CTRL       0x2
#define FULL_SPEED_LINE_RATE     BIT(4)
#define FULL_SPEED_LINE_POLARITY BIT(3)
#define DIRECT_CTRL              BIT(2)
#define TX_LINE_STATE_DP         BIT(1)
#define TX_LINE_STATE_DM         BIT(0)

#define UHC03_TX_SOF_EN 0x3
#define SOF_ENABLE      BIT(0)

#define UHC04_TX_ADDRESS_REG 0x4
#define DEVICE_ADDRESS(x)    FIELD_PREP(GENMASK(6, 0), x)

#define UHC05_TX_ENDPOINT_NUMBER 0x5
#define ENDPOINT_NUM(x)          FIELD_PREP(GENMASK(3, 0), x)

#define UHC08_INT_STATUS 0x8
#define UHC09_INT_MASK   0x9
#define SOF_SENT         BIT(3)
#define CONNECTION_EVENT BIT(2)
#define RESUME_EVENT     BIT(1)
#define XFER_DONE        BIT(0)

#define UHC0A_RX_STATUS   0xA
#define BIT_DATA_SEQUENCE 7
#define BIT_ACK_RXED      6
#define BIT_STALL_RXED    5
#define BIT_NACK_RXED     4
#define BIT_RX_TIMEOUT    3
#define BIT_RX_OVERFLOW   2
#define BIT_STUFF_ERROR   1
#define BIT_CRC_ERROR     0
#define STS_RX_ERROR_MASK GENMASK(BIT_RX_TIMEOUT, BIT_CRC_ERROR)

#define UHC0C_MISC_CTRL     0xC
#define PORT1_FULL_SPEED_EN BIT(4)

#define UHC0E_RX_CONNECT_STATE 0xE
#define RX_LINE_STATE_MASK     GENMASK(1, 0)
#define LINE_STATE(x)          FIELD_GET(RX_LINE_STATE_MASK, x)

#define UHC20_RX_FIFO_DATA      0x20
#define UHC22_RX_FIFO_COUNT_MSB 0x22
#define UHC23_RX_FIFO_COUNT_LSB 0x23
#define UHC30_TX_FIFO_DATA      0x30

#define UHCE0_HOST_DEVICE_CTRL 0xE0
#define RESET_CORE             BIT(1)

#define UHCE4_PORT1_MISC_CTRL 0xE4
#define PULL_DOWN             BIT(4)

#define IT51XXX_STATE_BUS_RESUME 0

#define IT51XXX_RESET_USB_CORE_COMPLETE_DELAY_MS 10

enum it51xxx_bus_state {
	SE0_STATE = 0,
	IDLE_STATE,
};

enum it51xxx_line_state {
	DISCONNECT = 0,
	LOW_SPEED,
	FULL_SPEED,
};

enum it51xxx_transaction_type {
	IT51XXX_XFER_TYPE_SETUP = 0,
	IT51XXX_XFER_TYPE_IN,
	IT51XXX_XFER_TYPE_OUT_DATA0,
	IT51XXX_XFER_TYPE_OUT_DATA1,
};

enum it51xxx_event_type {
	IT51XXX_EVT_SUBMIT_NEW_XFER = 0,
	IT51XXX_EVT_XFER_COMPLETED,
	IT51XXX_EVT_SOF,
	IT51XXX_EVT_DEQUEUE,
};

struct it51xxx_data {
	struct k_thread thread_data;
	struct k_event events;

	sys_dlist_t int_xfers;

	struct uhc_transfer *xfer;

	atomic_t state;

	uint16_t toggle_out;
	uint16_t frame_number;
	uint8_t error_count;
};

struct it51xxx_config {
	mm_reg_t base;

#if IT51XXX_UHC_EXTEND_CTRL_ENABLED
	struct ite_extend_control *extend_ctrl;
	size_t extend_ctrl_count;
#endif /* IT51XXX_UHC_EXTEND_CTRL_ENABLED */

	const struct pinctrl_dev_config *pcfg;

	k_thread_stack_t *thread_stk;
	size_t thread_stk_sz;

	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

static int it51xxx_drive_bus_state(const struct device *dev, const uint8_t state)
{
	const struct it51xxx_config *config = dev->config;
	uint8_t speed = LINE_STATE(sys_read8(config->base + UHC0E_RX_CONNECT_STATE));
	uint8_t line_ctrl = sys_read8(config->base + UHC02_TX_LINE_CTRL);

	if (state == IDLE_STATE) {
		if (speed == DISCONNECT) {
			LOG_ERR("usb is disconnected");
			return -EIO;
		}

		if (speed != LOW_SPEED && speed != FULL_SPEED) {
			LOG_ERR("only supported low-speed and full-speed");
			return -ENOTSUP;
		}
	}

	switch (state) {
	case SE0_STATE:
		line_ctrl &= ~(TX_LINE_STATE_DM | TX_LINE_STATE_DP);
		break;
	case IDLE_STATE:
		if (speed == LOW_SPEED) {
			line_ctrl |= TX_LINE_STATE_DM;
			line_ctrl &= ~TX_LINE_STATE_DP;
		} else {
			line_ctrl |= TX_LINE_STATE_DP;
			line_ctrl &= ~TX_LINE_STATE_DM;
		}
		break;
	default:
		LOG_ERR("unknown bus state %d", state);
		return -EINVAL;
	}

	line_ctrl |= DIRECT_CTRL;
	sys_write8(line_ctrl, config->base + UHC02_TX_LINE_CTRL);
	return 0;
}

static int it51xxx_lock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int it51xxx_unlock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

static int it51xxx_sof_enable(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	uint8_t reg_val = sys_read8(config->base + UHC03_TX_SOF_EN);

	if (!(reg_val & SOF_ENABLE)) {
		/* enable frame signaling:
		 * sof packets every 1 ms for full-speed and
		 * keep-alive eop every 1 ms for low-speed
		 */
		sys_write8(reg_val | SOF_ENABLE, config->base + UHC03_TX_SOF_EN);
	}

	LOG_DBG("%s: enable sof generator", dev->name);

	return 0;
}

static int it51xxx_bus_suspend(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	int ret;

	/* disable sof generator */
	sys_write8(sys_read8(config->base + UHC03_TX_SOF_EN) & ~SOF_ENABLE,
		   config->base + UHC03_TX_SOF_EN);

	/* set bus as idle state at least 3 ms */
	ret = it51xxx_drive_bus_state(dev, IDLE_STATE);
	if (ret) {
		return ret;
	}
	k_msleep(3);

	sys_write8(sys_read8(config->base + UHC02_TX_LINE_CTRL) & ~DIRECT_CTRL,
		   config->base + UHC02_TX_LINE_CTRL);

	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	LOG_DBG("%s: suspend USB bus", dev->name);

	return 0;
}

static int it51xxx_bus_reset(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	int ret;

	/* set bus as SE0 state at least 50 ms (root port) */
	ret = it51xxx_drive_bus_state(dev, SE0_STATE);
	if (ret) {
		return ret;
	}

	k_msleep(CONFIG_UHC_IT51XXX_ROOT_RESET_MS);

	sys_write8(sys_read8(config->base + UHC02_TX_LINE_CTRL) & ~DIRECT_CTRL,
		   config->base + UHC02_TX_LINE_CTRL);

	k_msleep(CONFIG_UHC_IT51XXX_RESET_RECOVERY_MS);

	/* reset data toggle */
	priv->toggle_out = 0;

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	LOG_DBG("%s: reset USB bus", dev->name);

	return 0;
}

static int it51xxx_bus_resume(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);

	/* it51xxx hardware will drive resume pattern before sending sof */
	(void)it51xxx_sof_enable(dev);

	atomic_set_bit(&priv->state, IT51XXX_STATE_BUS_RESUME);

	LOG_DBG("%s: resume USB bus", dev->name);

	return 0;
}

static inline uint16_t it51xxx_rx_len(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;

	return ((uint16_t)sys_read8(config->base + UHC22_RX_FIFO_COUNT_MSB) << 8) |
	       sys_read8(config->base + UHC23_RX_FIFO_COUNT_LSB);
}

static void it51xxx_start_in_xfer(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->xfer;
	uint16_t remaining = it51xxx_rx_len(dev);

	sys_write8(DEVICE_ADDRESS(xfer->udev->addr), config->base + UHC04_TX_ADDRESS_REG);
	sys_write8(TRANSACTION_TYPE(IT51XXX_XFER_TYPE_IN), config->base + UHC01_TX_TRANS_TYPE);
	sys_write8(ENDPOINT_NUM(xfer->ep), config->base + UHC05_TX_ENDPOINT_NUMBER);

	if (remaining) {
		LOG_WRN("dropped remaining %d bytes in rx fifo", remaining);
		for (uint16_t i = 0; i < remaining; i++) {
			(void)sys_read8(config->base + UHC20_RX_FIFO_DATA);
		}
	}
	sys_write8(sys_read8(config->base + UHC00_TX_CTRL_REGISTER) | START_XFER,
		   config->base + UHC00_TX_CTRL_REGISTER);
}

static void it51xxx_start_out_xfer(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->xfer;

	sys_write8(DEVICE_ADDRESS(xfer->udev->addr), config->base + UHC04_TX_ADDRESS_REG);
	sys_write8(ENDPOINT_NUM(xfer->ep), config->base + UHC05_TX_ENDPOINT_NUMBER);
	if (priv->toggle_out & BIT(USB_EP_GET_IDX(xfer->ep))) {
		sys_write8(TRANSACTION_TYPE(IT51XXX_XFER_TYPE_OUT_DATA1),
			   config->base + UHC01_TX_TRANS_TYPE);
	} else {
		sys_write8(TRANSACTION_TYPE(IT51XXX_XFER_TYPE_OUT_DATA0),
			   config->base + UHC01_TX_TRANS_TYPE);
	}

	if (xfer->stage != UHC_CONTROL_STAGE_STATUS) {
		for (int i = 0; i < MIN(xfer->buf->len, xfer->mps); i++) {
			sys_write8(xfer->buf->data[i], config->base + UHC30_TX_FIFO_DATA);
		}
	}

	sys_write8(sys_read8(config->base + UHC00_TX_CTRL_REGISTER) | START_XFER,
		   config->base + UHC00_TX_CTRL_REGISTER);
}

static void it51xxx_start_ctrl_xfer(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->xfer;
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		LOG_HEXDUMP_DBG(xfer->setup_pkt, sizeof(xfer->setup_pkt), "setup:");
		sys_write8(DEVICE_ADDRESS(xfer->udev->addr), config->base + UHC04_TX_ADDRESS_REG);
		sys_write8(TRANSACTION_TYPE(IT51XXX_XFER_TYPE_SETUP),
			   config->base + UHC01_TX_TRANS_TYPE);
		sys_write8(ENDPOINT_NUM(USB_EP_GET_IDX(xfer->ep)),
			   config->base + UHC05_TX_ENDPOINT_NUMBER);
		for (uint8_t i = 0; i < sizeof(xfer->setup_pkt); i++) {
			sys_write8(xfer->setup_pkt[i], config->base + UHC30_TX_FIFO_DATA);
		}
		sys_write8(sys_read8(config->base + UHC00_TX_CTRL_REGISTER) | START_XFER,
			   config->base + UHC00_TX_CTRL_REGISTER);
		break;
	case UHC_CONTROL_STAGE_DATA:
		if (usb_reqtype_is_to_host(setup)) {
			/* setup->[in data] */
			it51xxx_start_in_xfer(dev);
		} else {
			/* setup->[out data] */
			it51xxx_start_out_xfer(dev);
		}
		break;
	case UHC_CONTROL_STAGE_STATUS:
		if (setup->wLength == 0 || !usb_reqtype_is_to_host(setup)) {
			/* setup->out data->[in sts] or setup->[in sts] */
			it51xxx_start_in_xfer(dev);
		} else {
			/* setup->in data->[out sts] */
			it51xxx_start_out_xfer(dev);
		}
		break;
	default:
		break;
	};
}

static int it51xxx_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;

	LOG_DBG("%s: enqueue %p addr %#x, ep %#x, stage %d, type %d, intvl. %u", dev->name, xfer,
		xfer->udev->addr, xfer->ep, xfer->stage, xfer->type, xfer->interval);

	if (xfer->interval) {
		xfer->start_frame = priv->frame_number + xfer->interval;
	}

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		sys_dlist_append(&data->ctrl_xfers, &xfer->node);
		break;
	case USB_EP_TYPE_BULK:
		sys_dlist_append(&data->bulk_xfers, &xfer->node);
		break;
	case USB_EP_TYPE_INTERRUPT:
		sys_dlist_append(&priv->int_xfers, &xfer->node);

		/* don't need to post event for interrupt xfers */
		return 0;
	case USB_EP_TYPE_ISO:
		/* TODO: unsupported iso xfer yet */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	/* wake-up thread */
	k_event_post(&priv->events, BIT(IT51XXX_EVT_SUBMIT_NEW_XFER));

	return 0;
}

static int it51xxx_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *tmp;

	if (!xfer) {
		return -EINVAL;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (xfer == tmp) {
			tmp->err = -ECONNRESET;
			k_event_post(&priv->events, BIT(IT51XXX_EVT_DEQUEUE));
			return 0;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&data->bulk_xfers, tmp, node) {
		if (xfer == tmp) {
			tmp->err = -ECONNRESET;
			k_event_post(&priv->events, BIT(IT51XXX_EVT_DEQUEUE));
			return 0;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&priv->int_xfers, tmp, node) {
		if (xfer == tmp) {
			tmp->err = -ECONNRESET;
			k_event_post(&priv->events, BIT(IT51XXX_EVT_DEQUEUE));
			return 0;
		}
	}

	LOG_WRN("failed to find xfer %p in lists", xfer);

	return -EINVAL;
}

static int uhc_it51xxx_init(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	uint8_t reg_val;

	/* reset USB core */
	sys_write8(sys_read8(config->base + UHCE0_HOST_DEVICE_CTRL) | RESET_CORE,
		   config->base + UHCE0_HOST_DEVICE_CTRL);
	k_msleep(IT51XXX_RESET_USB_CORE_COMPLETE_DELAY_MS);

	/* enable pull down */
	sys_write8(sys_read8(config->base + UHCE4_PORT1_MISC_CTRL) | PULL_DOWN,
		   config->base + UHCE4_PORT1_MISC_CTRL);

	reg_val = SOF_SENT | CONNECTION_EVENT | RESUME_EVENT | XFER_DONE;
	sys_write8(reg_val, config->base + UHC09_INT_MASK);

	LOG_DBG("%s: initialize uhc", dev->name);

	return 0;
}

static int uhc_it51xxx_enable(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;

	(void)it51xxx_sof_enable(dev);

	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	config->irq_enable_func(dev);

	LOG_DBG("%s: enable uhc", dev->name);

	return 0;
}

static int uhc_it51xxx_disable(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	uint8_t reg_val = sys_read8(config->base + UHC03_TX_SOF_EN);

	if (reg_val & SOF_ENABLE) {
		sys_write8(reg_val & ~SOF_ENABLE, config->base + UHC03_TX_SOF_EN);
	}

	config->irq_disable_func(dev);

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	LOG_DBG("%s: disable uhc", dev->name);

	return 0;
}

static int uhc_it51xxx_shutdown(const struct device *dev)
{
	LOG_DBG("%s: power-off uhc", dev->name);

	return 0;
}

static int it51xxx_submit_xfer(const struct device *const dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->xfer;

	LOG_DBG("%s: addr %#x, ep %#x, stage %d, type %d", dev->name, xfer->udev->addr, xfer->ep,
		xfer->stage, xfer->type);

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		it51xxx_start_ctrl_xfer(dev);
		break;
	case USB_EP_TYPE_BULK:
		__fallthrough;
	case USB_EP_TYPE_INTERRUPT:
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			it51xxx_start_in_xfer(dev);
		} else {
			it51xxx_start_out_xfer(dev);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void it51xxx_uhc_xfer_cancelled(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *next, *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, tmp, next, node) {
		if (tmp != priv->xfer && tmp->err == -ECONNRESET) {
			uhc_xfer_return(dev, tmp, -ECONNRESET);
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->bulk_xfers, tmp, next, node) {
		if (tmp != priv->xfer && tmp->err == -ECONNRESET) {
			uhc_xfer_return(dev, tmp, -ECONNRESET);
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&priv->int_xfers, tmp, next, node) {
		if (tmp != priv->xfer && tmp->err == -ECONNRESET) {
			uhc_xfer_return(dev, tmp, -ECONNRESET);
		}
	}
}

static struct uhc_transfer *it51xxx_get_due_int_xfer(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer, *next;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&priv->int_xfers, xfer, next, node) {
		if ((int16_t)(priv->frame_number - xfer->start_frame) < 0) {
			continue;
		}

		/* move the selected xfer to the end of the queue */
		sys_dlist_remove(&xfer->node);
		sys_dlist_append(&priv->int_xfers, &xfer->node);

		xfer->start_frame = priv->frame_number + xfer->interval;

		return xfer;
	}

	return NULL;
}

static int it51xxx_schedule_int_xfer(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer;
	int ret;

	if (priv->xfer != NULL) {
		return -EBUSY;
	}

	xfer = it51xxx_get_due_int_xfer(dev);
	if (xfer == NULL) {
		return -ENOENT;
	}

	priv->xfer = xfer;

	ret = it51xxx_submit_xfer(dev);
	if (ret < 0) {
		LOG_WRN("failed to submit int xfer, %d", ret);
		uhc_xfer_return(dev, priv->xfer, ret);
		priv->xfer = NULL;
	}

	return ret;
}

static int it51xxx_error_check(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	uint8_t rx_status = sys_read8(config->base + UHC0A_RX_STATUS);

	LOG_DBG("isr: rx status: %#x", rx_status);

	if (rx_status & STS_RX_ERROR_MASK) {
		if (IS_BIT_SET(rx_status, BIT_RX_TIMEOUT)) {
			LOG_ERR("isr: rx timeout");
		}
		if (IS_BIT_SET(rx_status, BIT_RX_OVERFLOW)) {
			LOG_ERR("isr: rx overflow");
		}
		if (IS_BIT_SET(rx_status, BIT_STUFF_ERROR)) {
			LOG_ERR("isr: stuff error");
		}
		if (IS_BIT_SET(rx_status, BIT_CRC_ERROR)) {
			LOG_ERR("isr: crc error");
		}
		return -EIO;
	}

	if (IS_BIT_SET(rx_status, BIT_NACK_RXED)) {
		LOG_DBG("isr: nack is received");
		return -EAGAIN;
	}

	if (IS_BIT_SET(rx_status, BIT_STALL_RXED)) {
		LOG_WRN("isr: stall is received");
		return -EPIPE;
	}

	if (IS_BIT_SET(rx_status, BIT_DATA_SEQUENCE)) {
		LOG_DBG("isr: in transaction: data1");
	}

	if (IS_BIT_SET(rx_status, BIT_ACK_RXED)) {
		LOG_DBG("isr: ack is received");
	}

	return 0;
}

static int it51xxx_uhc_finish_in(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct it51xxx_config *config = dev->config;
	uint16_t len = it51xxx_rx_len(dev);

	if (len > net_buf_tailroom(xfer->buf)) {
		LOG_ERR("insufficient buffer size, %zu/%zu bytes", len,
			net_buf_tailroom(xfer->buf));
		return -ENOBUFS;
	}

	for (uint16_t i = 0; i < len; i++) {
		net_buf_add_u8(xfer->buf, sys_read8(config->base + UHC20_RX_FIFO_DATA));
	}

	if (net_buf_tailroom(xfer->buf) && len == xfer->mps) {
		LOG_DBG("ep %#x: next in data(%d)", xfer->ep, xfer->buf->len);
		return (int)net_buf_tailroom(xfer->buf);
	}

	LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len, "in data:");

	return 0;
}

static int it51xxx_uhc_finish_out(const struct device *dev, struct uhc_transfer *const xfer)
{
	size_t len = MIN(xfer->buf->len, xfer->mps);

	LOG_DBG("ep %#x: out data: sent %zu bytes", xfer->ep, len);
	net_buf_pull(xfer->buf, len);

	if (xfer->buf->len) {
		LOG_DBG("ep %#x: next out data(%d)", xfer->ep, xfer->buf->len);
		return (int)xfer->buf->len;
	}

	LOG_DBG("ep %#x: finished out data", xfer->ep);

	return 0;
}

static void it51xxx_update_ctrl_stage(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->xfer;
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	int ret = it51xxx_error_check(dev);

	if (ret == -EAGAIN) {
		/* nack is received */
		return;
	}

	if (ret == -EIO) {
		/* error is received */
		if (++priv->error_count >= CONFIG_UHC_IT51XXX_MAX_ERROR_RETRIES) {
			goto xfer_return;
		}
		return;
	}

	if (ret == -EPIPE) {
		/* stall is received */
		goto xfer_return;
	}

	priv->error_count = 0;

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		xfer->stage = (setup->wLength) ? UHC_CONTROL_STAGE_DATA : UHC_CONTROL_STAGE_STATUS;
		if (xfer->stage == UHC_CONTROL_STAGE_DATA && !usb_reqtype_is_to_host(setup)) {
			priv->toggle_out |= BIT(USB_EP_GET_IDX(xfer->ep));
		}
		return;
	case UHC_CONTROL_STAGE_DATA:
		if (usb_reqtype_is_to_host(setup)) {
			/* setup->in data */
			ret = it51xxx_uhc_finish_in(dev, priv->xfer);
			if (ret < 0) {
				uhc_xfer_return(dev, priv->xfer, ret);
				priv->xfer = NULL;
				return;
			}

			if (ret > 0) {
				xfer->stage = UHC_CONTROL_STAGE_DATA;
			} else {
				LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len, "in data:");
				priv->toggle_out |= BIT(USB_EP_GET_IDX(xfer->ep));
				xfer->stage = UHC_CONTROL_STAGE_STATUS;
			}
			return;
		}

		/* setup->out data */
		ret = it51xxx_uhc_finish_out(dev, priv->xfer);
		if (ret > 0) {
			xfer->stage = UHC_CONTROL_STAGE_DATA;
			/* toggle data pid */
			priv->toggle_out ^= BIT(USB_EP_GET_IDX(xfer->ep));
		} else {
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
		}
		return;
	case UHC_CONTROL_STAGE_STATUS:
		if (setup->wLength == 0 || !usb_reqtype_is_to_host(setup)) {
			LOG_DBG("setup->out data->in sts or setup->in sts");
		} else {
			/* the data stage is IN, and the status stage is OUT */
			LOG_DBG("setup->in data->out sts");
		}
		ret = 0;
		break;
	default:
		LOG_ERR("unknown control stage %d", xfer->stage);
		ret = -EINVAL;
		break;
	};

xfer_return:
	uhc_xfer_return(dev, xfer, ret);
	priv->xfer = NULL;
}

static void it51xxx_bulk_xfer_done(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	int ret = it51xxx_error_check(dev);

	if (ret == -EAGAIN) {
		/* nack is received and the bulk node was moved to the tail.
		 * clear it and retry later to prevent occupy the resource here
		 */
		priv->xfer = NULL;
		return;
	}

	if (ret == -EIO) {
		/* error is received */
		if (++priv->error_count >= CONFIG_UHC_IT51XXX_MAX_ERROR_RETRIES) {
			goto xfer_return;
		}
		return;
	}

	if (ret == -EPIPE) {
		/* stall is received */
		goto xfer_return;
	}

	priv->error_count = 0;

	if (USB_EP_DIR_IS_IN(priv->xfer->ep)) {
		ret = it51xxx_uhc_finish_in(dev, priv->xfer);
	} else {
		ret = it51xxx_uhc_finish_out(dev, priv->xfer);
		/* toggle data pid */
		priv->toggle_out ^= BIT(USB_EP_GET_IDX(priv->xfer->ep));
	}

	if (ret > 0) {
		/* still remaining data to xfer */
		return;
	}

xfer_return:
	uhc_xfer_return(dev, priv->xfer, ret);
	priv->xfer = NULL;
}

static void it51xxx_int_xfer_done(const struct device *dev)
{
	struct it51xxx_data *priv = uhc_get_private(dev);
	int ret;
	int status = it51xxx_error_check(dev);

	switch (status) {
	case 0:
		/* ack is received */
		if (USB_EP_DIR_IS_IN(priv->xfer->ep)) {
			ret = it51xxx_uhc_finish_in(dev, priv->xfer);
		} else {
			ret = it51xxx_uhc_finish_out(dev, priv->xfer);
			/* toggle data pid */
			priv->toggle_out ^= BIT(USB_EP_GET_IDX(priv->xfer->ep));
		}

		if (ret == 0 || ret < 0) {
			/* finished int xfer */
			uhc_xfer_return(dev, priv->xfer, ret);
		}
		break;
	case -EAGAIN:
		/* nak is received */
		break;
	case -EPIPE:
		/* stall is received */
		__fallthrough;
	case -EIO:
		/* error is received */
		__fallthrough;
	default:
		uhc_xfer_return(dev, priv->xfer, status);
		break;
	}

	priv->xfer = NULL;
	(void)it51xxx_schedule_int_xfer(dev);
}

static void it51xxx_uhc_handler(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *dev = arg1;
	struct it51xxx_data *priv = uhc_get_private(dev);
	bool submit;
	int ret;
	uint32_t evt;

	while (true) {
		evt = k_event_wait_safe(&priv->events, UINT32_MAX, false, K_FOREVER);

		it51xxx_lock(dev);
		submit = false;

		if (evt & BIT(IT51XXX_EVT_DEQUEUE)) {
			it51xxx_uhc_xfer_cancelled(dev);
		}

		if (evt & BIT(IT51XXX_EVT_SOF)) {
			(void)it51xxx_schedule_int_xfer(dev);
		}

		if (evt & BIT(IT51XXX_EVT_XFER_COMPLETED)) {
			if (priv->xfer == NULL) {
				LOG_WRN("completed without active xfer");
			} else if (priv->xfer->err == -ECONNRESET) {
				uhc_xfer_return(dev, priv->xfer, -ECONNRESET);
				priv->xfer = NULL;
			} else {
				if (priv->xfer->type == USB_EP_TYPE_CONTROL) {
					it51xxx_update_ctrl_stage(dev);
					submit = priv->xfer != NULL;
				} else if (priv->xfer->type == USB_EP_TYPE_BULK) {
					it51xxx_bulk_xfer_done(dev);
					submit = priv->xfer != NULL;
				} else if (priv->xfer->type == USB_EP_TYPE_INTERRUPT) {
					it51xxx_int_xfer_done(dev);
				} else {
					LOG_WRN("isr: xfer type %d done", priv->xfer->type);
				}
			}
		}

		if (priv->xfer == NULL) {
			struct uhc_data *data = dev->data;
			sys_dnode_t *node;
			struct uhc_transfer *xfer;

			node = sys_dlist_peek_head(&data->ctrl_xfers);
			if (node == NULL) {
				node = sys_dlist_peek_head(&data->bulk_xfers);
			}

			if (node != NULL) {
				priv->xfer = SYS_DLIST_CONTAINER(node, xfer, node);
				if (priv->xfer && priv->xfer->type == USB_EP_TYPE_BULK) {
					/* move bulk node to the tail */
					sys_dlist_remove(&priv->xfer->node);
					sys_dlist_append(&data->bulk_xfers, &priv->xfer->node);
				}
			}
			priv->error_count = 0;
			submit = priv->xfer != NULL;
		}

		if (submit) {
			ret = it51xxx_submit_xfer(dev);
			if (ret) {
				LOG_WRN("failed to submit xfer, %d", ret);
				uhc_xfer_return(dev, priv->xfer, ret);
				priv->xfer = NULL;
			}
		}

		it51xxx_unlock(dev);
	}
}

static void it51xxx_uhc_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	uint8_t int_sts = sys_read8(config->base + UHC08_INT_STATUS);

	LOG_DBG("isr: int status %#x", int_sts);

	if (int_sts & SOF_SENT) {
		LOG_DBG("isr: sof sent");
		priv->frame_number++;
		sys_write8(SOF_SENT, config->base + UHC08_INT_STATUS);
		k_event_post(&priv->events, BIT(IT51XXX_EVT_SOF));
	}

	if (int_sts & CONNECTION_EVENT) {
		uint8_t line_state = LINE_STATE(sys_read8(config->base + UHC0E_RX_CONNECT_STATE));

		LOG_DBG("isr: connection event, line state");

		switch (line_state) {
		case LOW_SPEED:
			sys_write8(sys_read8(config->base + UHC0C_MISC_CTRL) & ~PORT1_FULL_SPEED_EN,
				   config->base + UHC0C_MISC_CTRL);
			sys_write8(sys_read8(config->base + UHC02_TX_LINE_CTRL) &
					   ~(FULL_SPEED_LINE_RATE | FULL_SPEED_LINE_POLARITY),
				   config->base + UHC02_TX_LINE_CTRL);

			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
			LOG_INF("isr: low speed device is connected");
			break;
		case FULL_SPEED:
			sys_write8(sys_read8(config->base + UHC0C_MISC_CTRL) | PORT1_FULL_SPEED_EN,
				   config->base + UHC0C_MISC_CTRL);
			sys_write8(sys_read8(config->base + UHC02_TX_LINE_CTRL) |
					   FULL_SPEED_LINE_RATE | FULL_SPEED_LINE_POLARITY,
				   config->base + UHC02_TX_LINE_CTRL);

			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
			LOG_INF("isr: full speed device is connected");
			break;
		case DISCONNECT:
			uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
			LOG_INF("isr: device is disconnected");
			break;
		default:
			LOG_DBG("unknown line stats %#x", line_state);
			break;
		}

		sys_write8(CONNECTION_EVENT, config->base + UHC08_INT_STATUS);
	}

	if (int_sts & RESUME_EVENT) {
		LOG_DBG("isr: resume event");

		if (atomic_test_and_clear_bit(&priv->state, IT51XXX_STATE_BUS_RESUME)) {
			uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
		} else {
			/* the resume event intc is triggered after enabling
			 * sof or receiving remote wake-up
			 */
			(void)it51xxx_sof_enable(dev);
		}

		sys_write8(RESUME_EVENT, config->base + UHC08_INT_STATUS);
	}

	if (int_sts & XFER_DONE) {
		LOG_DBG("isr: xfer done");
		sys_write8(XFER_DONE, config->base + UHC08_INT_STATUS);
		k_event_post(&priv->events, BIT(IT51XXX_EVT_XFER_COMPLETED));
	}
}

static int it51xxx_uhc_preinit(const struct device *dev)
{
	const struct it51xxx_config *config = dev->config;
	struct it51xxx_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to apply pinctrl, ret %d", ret);
		return ret;
	}

#if IT51XXX_UHC_EXTEND_CTRL_ENABLED
	ite_apply_extend_control(config->extend_ctrl, config->extend_ctrl_count);
#endif /* IT51XXX_UHC_EXTEND_CTRL_ENABLED */

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	sys_dlist_init(&priv->int_xfers);

	k_thread_create(&priv->thread_data, config->thread_stk, config->thread_stk_sz,
			it51xxx_uhc_handler, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_IT51XXX_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	return 0;
}

static DEVICE_API(uhc, it51xxx_uhc_api) = {
	.lock = it51xxx_lock,
	.unlock = it51xxx_unlock,
	.init = uhc_it51xxx_init,
	.enable = uhc_it51xxx_enable,
	.disable = uhc_it51xxx_disable,
	.shutdown = uhc_it51xxx_shutdown,

	.bus_reset = it51xxx_bus_reset,
	.sof_enable = it51xxx_sof_enable,
	.bus_suspend = it51xxx_bus_suspend,
	.bus_resume = it51xxx_bus_resume,

	.ep_enqueue = it51xxx_enqueue,
	.ep_dequeue = it51xxx_dequeue,
};

/* clang-format off */
#define IT51XXX_USB_HOST_DEFINE(n)                                                                 \
                                                                                                   \
	IF_ENABLED(IT51XXX_UHC_EXTEND_CTRL_ENABLED, (                                              \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, ite_extend_ctrl), (                            \
			static struct ite_extend_control extctrl_##n[] =                           \
				ITE_DT_EXTEND_CTRL_ITEMS_LIST(n);                                  \
		))                                                                                 \
	))                                                                                         \
                                                                                                   \
	K_KERNEL_STACK_DEFINE(uhc_it51xxx_stack_##n, CONFIG_UHC_IT51XXX_STACK_SIZE);               \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void uhc_it51xxx_irq_enable_func_##n(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, it51xxx_uhc_isr, DEVICE_DT_INST_GET(n), 0);        \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void uhc_it51xxx_irq_disable_func_##n(const struct device *dev)                     \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
                                                                                                   \
	static struct it51xxx_config uhc_cfg_##n = {                                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.thread_stk = uhc_it51xxx_stack_##n,                                               \
		.thread_stk_sz = K_THREAD_STACK_SIZEOF(uhc_it51xxx_stack_##n),                     \
		.irq_enable_func = uhc_it51xxx_irq_enable_func_##n,                                \
		.irq_disable_func = uhc_it51xxx_irq_disable_func_##n,                              \
		IF_ENABLED(IT51XXX_UHC_EXTEND_CTRL_ENABLED, (                                      \
			COND_CODE_1(                                                               \
				DT_INST_NODE_HAS_PROP(n, ite_extend_ctrl),                         \
				(                                                                  \
					.extend_ctrl = extctrl_##n,                                \
					.extend_ctrl_count = ARRAY_SIZE(extctrl_##n),              \
				),                                                                 \
				(                                                                  \
					.extend_ctrl = NULL,                                       \
					.extend_ctrl_count = 0,                                    \
				)                                                                  \
			)                                                                          \
		))                                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct it51xxx_data priv_data_##n;                                                  \
                                                                                                   \
	static struct uhc_data uhc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),                                  \
		.priv = &priv_data_##n,                                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, it51xxx_uhc_preinit, NULL, &uhc_data_##n, &uhc_cfg_##n,           \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &it51xxx_uhc_api);

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_USB_HOST_DEFINE)
/* clang-format on */
