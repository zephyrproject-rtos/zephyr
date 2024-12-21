/*
 * Copyright (c) 2023 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <soc.h>
#include <soc_dt.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/dt-bindings/interrupt-controller/it8xxx2-wuc.h>
LOG_MODULE_REGISTER(udc_it82xx2, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT ite_it82xx2_usb

/* TODO: Replace this definition by Kconfig option */
#define USB_DEVICE_CONFIG_SOF_NOTIFICATIONS (0U)

#define IT8XXX2_IS_EXTEND_ENDPOINT(n) (USB_EP_GET_IDX(n) >= 4)

#define IT82xx2_STATE_OUT_SHARED_FIFO_BUSY 0

/* Shared FIFO number including FIFO_1/2/3 */
#define SHARED_FIFO_NUM 3

/* The related definitions of the register dc_line_status: 0x51 */
#define RX_LINE_STATE_MASK (RX_LINE_FULL_SPD | RX_LINE_LOW_SPD)
#define RX_LINE_LOW_SPD    0x02
#define RX_LINE_FULL_SPD   0x01
#define RX_LINE_RESET      0x00

#define DC_ADDR_NULL 0x00
#define DC_ADDR_MASK 0x7F

/* EPN Extend Control 2 Register Mask Definition */
#define COMPLETED_TRANS 0xF0

/* The related definitions of the register EP STATUS:
 * 0x41/0x45/0x49/0x4D
 */
#define EP_STATUS_ERROR 0x0F

/* ENDPOINT[3..0]_CONTROL_REG */
#define ENDPOINT_EN  BIT(0)
#define ENDPOINT_RDY BIT(1)

/* The bit definitions of the register EP RX/TX FIFO Control:
 * EP_RX_FIFO_CONTROL: 0X64/0x84/0xA4/0xC4
 * EP_TX_FIFO_CONTROL: 0X74/0x94/0xB4/0xD4
 */
#define FIFO_FORCE_EMPTY BIT(0)

/* The bit definitions of the register Host/Device Control: 0XE0 */
#define RESET_CORE BIT(1)

/* ENDPOINT[3..0]_STATUS_REG */
#define DC_STALL_SENT BIT(5)

/* DC_INTERRUPT_STATUS_REG */
#define DC_TRANS_DONE   BIT(0)
#define DC_RESUME_EVENT BIT(1)
#define DC_RESET_EVENT  BIT(2)
#define DC_SOF_RECEIVED BIT(3)
#define DC_NAK_SENT_INT BIT(4)

/* DC_CONTROL_REG */
#define DC_GLOBAL_ENABLE            BIT(0)
#define DC_TX_LINE_STATE_DM         BIT(1)
#define DC_DIRECT_CONTROL           BIT(3)
#define DC_FULL_SPEED_LINE_POLARITY BIT(4)
#define DC_FULL_SPEED_LINE_RATE     BIT(5)
#define DC_CONNECT_TO_HOST          BIT(6) /* internal pull-up */

/* ENDPOINT[3..0]_CONTROL_REG */
#define ENDPOINT_ENABLE_BIT      BIT(0)
#define ENDPOINT_READY_BIT       BIT(1)
#define ENDPOINT_OUTDATA_SEQ_BIT BIT(2)
#define ENDPOINT_SEND_STALL_BIT  BIT(3)
#define ENDPOINT_ISO_ENABLE_BIT  BIT(4)
#define ENDPOINT_DIRECTION_BIT   BIT(5)

/* Bit [1:0] represents the TRANSACTION_TYPE as follows: */
enum it82xx2_transaction_types {
	DC_SETUP_TRANS = 0,
	DC_IN_TRANS,
	DC_OUTDATA_TRANS,
	DC_ALL_TRANS
};

enum it82xx2_event_type {
	IT82xx2_EVT_XFER,
	IT82xx2_EVT_SETUP_TOKEN,
	IT82xx2_EVT_OUT_TOKEN,
	IT82xx2_EVT_IN_TOKEN,
};

struct it82xx2_ep_event {
	sys_snode_t node;
	const struct device *dev;
	uint8_t ep;
	enum it82xx2_event_type event;
};

K_MSGQ_DEFINE(evt_msgq, sizeof(struct it82xx2_ep_event),
	      CONFIG_UDC_IT82xx2_EVENT_COUNT, sizeof(uint32_t));

struct usb_it8xxx2_wuc {
	/* WUC control device structure */
	const struct device *dev;
	/* WUC pin mask */
	uint8_t mask;
};

struct it82xx2_data {
	const struct device *dev;

	struct k_fifo fifo;
	struct k_work_delayable suspended_work;

	struct k_thread thread_data;
	struct k_sem suspended_sem;

	/* shared OUT FIFO state */
	atomic_t out_fifo_state;

	/* FIFO_1/2/3 semaphore */
	struct k_sem fifo_sem[SHARED_FIFO_NUM];

	/* Record if the previous transaction of endpoint0 is STALL */
	bool stall_is_sent;
};

struct usb_it82xx2_config {
	struct usb_it82xx2_regs *const base;
	const struct pinctrl_dev_config *pcfg;
	const struct usb_it8xxx2_wuc wuc;
	uint8_t usb_irq;
	uint8_t wu_irq;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
};

enum it82xx2_ep_ctrl {
	EP_IN_DIRECTION_SET,
	EP_STALL_SEND,
	EP_IOS_ENABLE,
	EP_ENABLE,
	EP_DATA_SEQ_1,
	EP_DATA_SEQ_TOGGLE,
	EP_READY_ENABLE,
};

/* The ep_fifo_res[ep_idx % SHARED_FIFO_NUM] where the SHARED_FIFO_NUM is 3 represents the
 * EP mapping because when (ep_idx % SHARED_FIFO_NUM) is 3, it actually means the EP0.
 */
static const uint8_t ep_fifo_res[SHARED_FIFO_NUM] = {3, 1, 2};

static volatile void *it82xx2_get_ext_ctrl(const struct device *dev, const uint8_t ep_idx,
					   const enum it82xx2_ep_ctrl ctrl)
{
	uint8_t idx;
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	union epn0n1_extend_ctrl_reg *epn0n1_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl;
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	if (ctrl == EP_IN_DIRECTION_SET || ctrl == EP_ENABLE) {
		idx = ((ep_idx - 4) % 3) + 1;
		return &ext_ctrl[idx].epn_ext_ctrl1;
	}

	idx = (ep_idx - 4) / 2;
	return &epn0n1_ext_ctrl[idx];
}

static int it82xx2_usb_extend_ep_ctrl(const struct device *dev, const uint8_t ep,
				      const enum it82xx2_ep_ctrl ctrl, const bool enable)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;
	volatile union epn_extend_ctrl1_reg *epn_ext_ctrl1 = NULL;
	volatile union epn0n1_extend_ctrl_reg *epn0n1_ext_ctrl = NULL;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t fifo_idx = (ep_idx > 0) ? (ep_fifo_res[ep_idx % SHARED_FIFO_NUM]) : 0;

	if (ctrl == EP_IN_DIRECTION_SET || ctrl == EP_ENABLE) {
		epn_ext_ctrl1 = it82xx2_get_ext_ctrl(dev, ep_idx, ctrl);
	} else {
		epn0n1_ext_ctrl = it82xx2_get_ext_ctrl(dev, ep_idx, ctrl);
	}

	switch (ctrl) {
	case EP_STALL_SEND:
		if (ep_idx % 2) {
			epn0n1_ext_ctrl->fields.epn1_send_stall_bit = enable;
		} else {
			epn0n1_ext_ctrl->fields.epn0_send_stall_bit = enable;
		}
		break;
	case EP_IOS_ENABLE:
		if (ep_idx % 2) {
			epn0n1_ext_ctrl->fields.epn1_iso_enable_bit = enable;
		} else {
			epn0n1_ext_ctrl->fields.epn0_iso_enable_bit = enable;
		}
		break;
	case EP_DATA_SEQ_1:
		if (ep_idx % 2) {
			epn0n1_ext_ctrl->fields.epn1_outdata_sequence_bit = enable;
		} else {
			epn0n1_ext_ctrl->fields.epn0_outdata_sequence_bit = enable;
		}
		break;
	case EP_DATA_SEQ_TOGGLE:
		if (!enable) {
			break;
		}
		if (ep_idx % 2) {
			if (epn0n1_ext_ctrl->fields.epn1_outdata_sequence_bit) {
				epn0n1_ext_ctrl->fields.epn1_outdata_sequence_bit = 0;
			} else {
				epn0n1_ext_ctrl->fields.epn1_outdata_sequence_bit = 1;
			}
		} else {
			if (epn0n1_ext_ctrl->fields.epn0_outdata_sequence_bit) {
				epn0n1_ext_ctrl->fields.epn0_outdata_sequence_bit = 0;
			} else {
				epn0n1_ext_ctrl->fields.epn0_outdata_sequence_bit = 1;
			}
		}
		break;
	case EP_IN_DIRECTION_SET:
		if (((ep_idx - 4) / 3 == 0)) {
			epn_ext_ctrl1->fields.epn0_direction_bit = enable;
		} else if (((ep_idx - 4) / 3 == 1)) {
			epn_ext_ctrl1->fields.epn3_direction_bit = enable;
		} else if (((ep_idx - 4) / 3 == 2)) {
			epn_ext_ctrl1->fields.epn6_direction_bit = enable;
		} else if (((ep_idx - 4) / 3 == 3)) {
			epn_ext_ctrl1->fields.epn9_direction_bit = enable;
		} else {
			LOG_ERR("Invalid endpoint 0x%x for control type 0x%x", ep, ctrl);
			return -EINVAL;
		}
		break;
	case EP_ENABLE:
		if (((ep_idx - 4) / 3 == 0)) {
			epn_ext_ctrl1->fields.epn0_enable_bit = enable;
		} else if (((ep_idx - 4) / 3 == 1)) {
			epn_ext_ctrl1->fields.epn3_enable_bit = enable;
		} else if (((ep_idx - 4) / 3 == 2)) {
			epn_ext_ctrl1->fields.epn6_enable_bit = enable;
		} else if (((ep_idx - 4) / 3 == 3)) {
			epn_ext_ctrl1->fields.epn9_enable_bit = enable;
		} else {
			LOG_ERR("Invalid endpoint 0x%x for control type 0x%x", ep, ctrl);
			return -EINVAL;
		}
		break;
	case EP_READY_ENABLE:
		int idx = ((ep_idx - 4) % 3) + 1;

		(enable) ? (ext_ctrl[idx].epn_ext_ctrl2 |= BIT((ep_idx - 4) / 3))
			 : (ext_ctrl[idx].epn_ext_ctrl2 &= ~BIT((ep_idx - 4) / 3));
		ep_regs[fifo_idx].ep_ctrl.fields.ready_bit = enable;
		break;
	default:
		LOG_ERR("Unknown control type 0x%x", ctrl);
		return -EINVAL;
	}

	return 0;
}

static int it82xx2_usb_ep_ctrl(const struct device *dev, uint8_t ep, enum it82xx2_ep_ctrl ctrl,
			       bool enable)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_ctrl_value;

	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		return -EINVAL;
	}

	ep_ctrl_value = ep_regs[ep_idx].ep_ctrl.value & ~ENDPOINT_READY_BIT;

	switch (ctrl) {
	case EP_STALL_SEND:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_SEND_STALL_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_SEND_STALL_BIT;
		}
		break;
	case EP_IN_DIRECTION_SET:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_DIRECTION_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_DIRECTION_BIT;
		}
		break;
	case EP_IOS_ENABLE:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_ISO_ENABLE_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_ISO_ENABLE_BIT;
		}
		break;
	case EP_ENABLE:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_ENABLE_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_ENABLE_BIT;
		}
		break;
	case EP_READY_ENABLE:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_READY_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_READY_BIT;
		}
		break;
	case EP_DATA_SEQ_1:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_OUTDATA_SEQ_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_OUTDATA_SEQ_BIT;
		}
		break;
	case EP_DATA_SEQ_TOGGLE:
		if (!enable) {
			break;
		}
		ep_ctrl_value ^= ENDPOINT_OUTDATA_SEQ_BIT;
		break;
	default:
		LOG_ERR("Unknown control type 0x%x", ctrl);
		return -EINVAL;
	}

	ep_regs[ep_idx].ep_ctrl.value = ep_ctrl_value;
	return 0;
}

static int it82xx2_usb_set_ep_ctrl(const struct device *dev, uint8_t ep, enum it82xx2_ep_ctrl ctrl,
				   bool enable)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int ret = 0;
	unsigned int key;

	key = irq_lock();
	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		ret = it82xx2_usb_extend_ep_ctrl(dev, ep, ctrl, enable);
	} else {
		ret = it82xx2_usb_ep_ctrl(dev, ep, ctrl, enable);
	}
	irq_unlock(key);
	return ret;
}

/* Standby(deep doze) mode enable/disable */
static void it82xx2_enable_standby_state(bool enable)
{
	if (enable) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	} else {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
}

/* Wake-up interrupt (USB D+) Enable/Disable */
static void it82xx2_enable_wu_irq(const struct device *dev, bool enable)
{
	const struct usb_it82xx2_config *config = dev->config;

	/* Clear pending interrupt */
	it8xxx2_wuc_clear_status(config->wuc.dev, config->wuc.mask);

	if (enable) {
		irq_enable(config->wu_irq);
	} else {
		irq_disable(config->wu_irq);
	}
}

static void it82xx2_wu_isr(const void *arg)
{
	const struct device *dev = arg;

	it82xx2_enable_wu_irq(dev, false);
	it82xx2_enable_standby_state(false);
	LOG_DBG("USB D+ (WU) Triggered");
}

static void it8xxx2_usb_dc_wuc_init(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;

	/* Initializing the WUI */
	it8xxx2_wuc_set_polarity(config->wuc.dev, config->wuc.mask, WUC_TYPE_EDGE_FALLING);
	it8xxx2_wuc_clear_status(config->wuc.dev, config->wuc.mask);

	/* Enabling the WUI */
	it8xxx2_wuc_enable(config->wuc.dev, config->wuc.mask);

	/* Connect WU (USB D+) interrupt but make it disabled initially */
	irq_connect_dynamic(config->wu_irq, 0, it82xx2_wu_isr, dev, 0);
}

static int it82xx2_usb_fifo_ctrl(const struct device *dev, const uint8_t ep, const bool reset)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	volatile uint8_t *ep_fifo_ctrl = usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;
	uint8_t fifon_ctrl = (ep_fifo_res[ep_idx % SHARED_FIFO_NUM] - 1) * 2;
	unsigned int key;
	int ret = 0;

	if (ep_idx == 0) {
		LOG_ERR("Invalid endpoint 0x%x", ep);
		return -EINVAL;
	}

	key = irq_lock();
	if (reset) {
		ep_fifo_ctrl[fifon_ctrl] = 0x0;
		ep_fifo_ctrl[fifon_ctrl + 1] = 0x0;
		irq_unlock(key);
		return 0;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		if (ep_idx < 8) {
			ep_fifo_ctrl[fifon_ctrl] = BIT(ep_idx);
			ep_fifo_ctrl[fifon_ctrl + 1] = 0x0;
		} else {
			ep_fifo_ctrl[fifon_ctrl] = 0x0;
			ep_fifo_ctrl[fifon_ctrl + 1] = BIT(ep_idx - 8);
		}
	} else if (USB_EP_DIR_IS_OUT(ep)) {
		if (ep_idx < 8) {
			ep_fifo_ctrl[fifon_ctrl] |= BIT(ep_idx);
		} else {
			ep_fifo_ctrl[fifon_ctrl + 1] |= BIT(ep_idx - 8);
		}
	} else {
		LOG_ERR("Failed to set fifo control register for ep 0x%x", ep);
		ret = -EINVAL;
	}
	irq_unlock(key);

	return ret;
}

static void it82xx2_event_submit(const struct device *dev, const uint8_t ep,
				 const enum it82xx2_event_type event)
{
	struct it82xx2_ep_event evt;

	evt.dev = dev;
	evt.ep = ep;
	evt.event = event;
	k_msgq_put(&evt_msgq, &evt, K_NO_WAIT);
}

static int it82xx2_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			      struct net_buf *const buf)
{
	udc_buf_put(cfg, buf);

	it82xx2_event_submit(dev, cfg->addr, IT82xx2_EVT_XFER);
	return 0;
}

static int it82xx2_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	struct net_buf *buf;
	unsigned int lock_key;
	uint8_t fifo_idx;

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;
	lock_key = irq_lock();
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		ff_regs[fifo_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
	} else {
		ff_regs[fifo_idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
	}
	irq_unlock(lock_key);

	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, cfg->addr, false);

	return 0;
}

static inline void ctrl_ep_stall_workaround(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;
	struct it82xx2_data *priv = udc_get_private(dev);
	unsigned int lock_key;
	uint32_t idx = 0;

	priv->stall_is_sent = true;
	lock_key = irq_lock();
	it82xx2_usb_set_ep_ctrl(dev, 0, EP_STALL_SEND, true);
	it82xx2_usb_set_ep_ctrl(dev, 0, EP_READY_ENABLE, true);

	/* It82xx2 does not support clearing the STALL bit by hardware; instead, the STALL bit need
	 * to be cleared by firmware. The SETUP token will be STALLed, which isn't compliant to
	 * USB specification, if firmware clears the STALL bit too late. Due to this hardware
	 * limitations, device controller polls to check if the stall bit has been transmitted for
	 * 3ms and then disables it after responsing STALLed.
	 */
	while (idx < 198 && !(ep_regs[0].ep_status & DC_STALL_SENT)) {
		/* wait 15.15us */
		gctrl_regs->GCTRL_WNCKR = 0;
		idx++;
	}

	if (idx < 198) {
		it82xx2_usb_set_ep_ctrl(dev, 0, EP_STALL_SEND, false);
	}
	irq_unlock(lock_key);
}

static int it82xx2_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		ctrl_ep_stall_workaround(dev);
	} else {
		it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_STALL_SEND, true);
		it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_READY_ENABLE, true);
	}

	LOG_DBG("Endpoint 0x%x is halted", cfg->addr);

	return 0;
}

static int it82xx2_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_STALL_SEND, false);

	LOG_DBG("Endpoint 0x%x clear halted", cfg->addr);

	return 0;
}

static int it82xx2_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	/* Configure endpoint */
	if (ep_idx != 0) {
		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_DATA_SEQ_1, false);
			it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_IN_DIRECTION_SET, true);
			/* clear fifo control registers */
			it82xx2_usb_fifo_ctrl(dev, cfg->addr, true);
		} else {
			it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_IN_DIRECTION_SET, false);
			it82xx2_usb_fifo_ctrl(dev, cfg->addr, false);
		}

		switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
		case USB_EP_TYPE_BULK:
			__fallthrough;
		case USB_EP_TYPE_INTERRUPT:
			it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_IOS_ENABLE, false);
			break;
		case USB_EP_TYPE_ISO:
			it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_IOS_ENABLE, true);
			break;
		case USB_EP_TYPE_CONTROL:
			__fallthrough;
		default:
			return -ENOTSUP;
		}
	}

	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		uint8_t fifo_idx;

		fifo_idx = ep_fifo_res[ep_idx % SHARED_FIFO_NUM];
		it82xx2_usb_set_ep_ctrl(dev, fifo_idx, EP_ENABLE, true);
	}

	it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_ENABLE, true);

	LOG_DBG("Endpoint 0x%02x is enabled", cfg->addr);

	return 0;
}

static int it82xx2_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_ENABLE, false);

	LOG_DBG("Endpoint 0x%02x is disabled", cfg->addr);

	return 0;
}

static int it82xx2_host_wakeup(const struct device *dev)
{
	struct it82xx2_data *priv = udc_get_private(dev);
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	int ret = -EACCES;

	if (udc_is_suspended(dev)) {
		usb_regs->dc_control = DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
				       DC_FULL_SPEED_LINE_RATE | DC_DIRECT_CONTROL |
				       DC_TX_LINE_STATE_DM | DC_CONNECT_TO_HOST;

		/* The remote wakeup device must hold the resume signal for */
		/* at least 1 ms but for no more than 15 ms                 */
		k_msleep(2);

		usb_regs->dc_control = DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
				       DC_FULL_SPEED_LINE_RATE | DC_CONNECT_TO_HOST;

		ret = k_sem_take(&priv->suspended_sem, K_MSEC(500));
		if (ret < 0) {
			LOG_ERR("Failed to wake up host");
		}
	}

	return ret;
}

static int it82xx2_set_address(const struct device *dev, const uint8_t addr)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;

	usb_regs->dc_address = addr & DC_ADDR_MASK;

	LOG_DBG("Set usb address 0x%02x", addr);

	return 0;
}

static int it82xx2_usb_dc_ip_init(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;

	/* reset usb controller */
	usb_regs->host_device_control = RESET_CORE;
	k_msleep(1);
	usb_regs->port0_misc_control &= ~(PULL_DOWN_EN);
	usb_regs->port1_misc_control &= ~(PULL_DOWN_EN);

	/* clear reset bit */
	usb_regs->host_device_control = 0;

	usb_regs->dc_interrupt_status =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED | DC_RESUME_EVENT;

	usb_regs->dc_interrupt_mask = 0x00;
	usb_regs->dc_interrupt_mask =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED | DC_RESUME_EVENT;

	usb_regs->dc_address = DC_ADDR_NULL;

	return 0;
}

static void it82xx2_enable_resume_int(const struct device *dev, bool enable)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;

	usb_regs->dc_interrupt_status = DC_RESUME_EVENT;
	if (enable) {
		usb_regs->dc_interrupt_mask |= DC_RESUME_EVENT;
	} else {
		usb_regs->dc_interrupt_mask &= ~DC_RESUME_EVENT;
	}
}

static void it82xx2_enable_sof_int(const struct device *dev, bool enable)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;

	usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
	if (enable) {
		usb_regs->dc_interrupt_mask |= DC_SOF_RECEIVED;
	} else {
		usb_regs->dc_interrupt_mask &= ~DC_SOF_RECEIVED;
	}
}

void it82xx2_dc_reset(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	struct it82xx2_data *priv = udc_get_private(dev);

	for (uint8_t ep_idx = 0; ep_idx < 4; ep_idx++) {
		ff_regs[ep_idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
		ff_regs[ep_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
	}

	ep_regs[0].ep_ctrl.value = ENDPOINT_EN;
	usb_regs->dc_address = DC_ADDR_NULL;
	usb_regs->dc_interrupt_status = DC_NAK_SENT_INT | DC_SOF_RECEIVED;

	atomic_clear_bit(&priv->out_fifo_state, IT82xx2_STATE_OUT_SHARED_FIFO_BUSY);

	k_sem_give(&priv->fifo_sem[0]);
	k_sem_give(&priv->fifo_sem[1]);
	k_sem_give(&priv->fifo_sem[2]);
}

static int it82xx2_xfer_in_data(const struct device *dev, uint8_t ep, struct net_buf *buf)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	struct it82xx2_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	unsigned int key;
	uint8_t fifo_idx;
	size_t len;

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;
	if (ep_idx == 0) {
		ff_regs[ep_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
	} else {
		k_sem_take(&priv->fifo_sem[fifo_idx - 1], K_FOREVER);
		key = irq_lock();
		it82xx2_usb_fifo_ctrl(dev, ep, false);
	}

	len = MIN(buf->len, udc_mps_ep_size(ep_cfg));

	for (size_t i = 0; i < len; i++) {
		ff_regs[fifo_idx].ep_tx_fifo_data = buf->data[i];
	}

	it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_READY_ENABLE, true);
	if (ep_idx != 0) {
		irq_unlock(key);
	}

	LOG_DBG("Writed %d packets to endpoint%d tx fifo", buf->len, ep_idx);

	return 0;
}

static int it82xx2_xfer_out_data(const struct device *dev, uint8_t ep, struct net_buf *buf)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t fifo_idx;
	size_t len;

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;
	if (ep_regs[fifo_idx].ep_status & EP_STATUS_ERROR) {
		LOG_WRN("endpoint%d error status 0x%02x", ep_idx, ep_regs[fifo_idx].ep_status);
		return -EINVAL;
	}

	len = (uint16_t)ff_regs[fifo_idx].ep_rx_fifo_dcnt_lsb +
	      (((uint16_t)ff_regs[fifo_idx].ep_rx_fifo_dcnt_msb) << 8);

	len = MIN(net_buf_tailroom(buf), len);
	uint8_t *data_ptr = net_buf_tail(buf);

	for (size_t idx = 0; idx < len; idx++) {
		data_ptr[idx] = ff_regs[fifo_idx].ep_rx_fifo_data;
	}

	net_buf_add(buf, len);

	return 0;
}

static uint16_t get_fifo_ctrl(const struct device *dev, const uint8_t fifo_idx)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	volatile uint8_t *ep_fifo_ctrl = usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;
	uint8_t fifon_ctrl;

	if (fifo_idx == 0) {
		LOG_ERR("Invalid fifo_idx 0x%x", fifo_idx);
		return 0;
	}

	fifon_ctrl = (fifo_idx - 1) * 2;

	return (ep_fifo_ctrl[fifon_ctrl + 1] << 8 | ep_fifo_ctrl[fifon_ctrl]);
}

static int work_handler_xfer_continue(const struct device *dev, uint8_t ep, struct net_buf *buf)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int ret = 0;
	uint8_t fifo_idx;

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;
	if (USB_EP_DIR_IS_OUT(ep)) {
		unsigned int key;

		if (ep_idx != 0) {
			struct it82xx2_data *priv = udc_get_private(dev);

			key = irq_lock();
			atomic_set_bit(&priv->out_fifo_state, IT82xx2_STATE_OUT_SHARED_FIFO_BUSY);
		}
		it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_READY_ENABLE, true);
		if (ep_idx != 0) {
			irq_unlock(key);
		}
	} else {
		ret = it82xx2_xfer_in_data(dev, ep, buf);
	}

	return ret;
}

static int work_handler_xfer_next(const struct device *dev, uint8_t ep)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	return work_handler_xfer_continue(dev, ep, buf);
}

/*
 * Allocate buffer and initiate a new control OUT transfer,
 * use successive buffer descriptor when next is true.
 */
static int it82xx2_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}
	udc_buf_put(cfg, buf);

	it82xx2_usb_set_ep_ctrl(dev, 0, EP_READY_ENABLE, true);

	return 0;
}

static bool get_extend_enable_bit(const struct device *dev, const uint8_t ep_idx)
{
	union epn_extend_ctrl1_reg *epn_ext_ctrl1 = NULL;
	bool enable;

	epn_ext_ctrl1 = (union epn_extend_ctrl1_reg *)it82xx2_get_ext_ctrl(dev, ep_idx, EP_ENABLE);
	if (((ep_idx - 4) / 3 == 0)) {
		enable = (epn_ext_ctrl1->fields.epn0_enable_bit != 0);
	} else if (((ep_idx - 4) / 3 == 1)) {
		enable = (epn_ext_ctrl1->fields.epn3_enable_bit != 0);
	} else if (((ep_idx - 4) / 3 == 2)) {
		enable = (epn_ext_ctrl1->fields.epn6_enable_bit != 0);
	} else {
		enable = (epn_ext_ctrl1->fields.epn9_enable_bit != 0);
	}
	return enable;
}

static bool get_extend_ready_bit(const struct device *dev, const uint8_t ep_idx)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;
	int idx = ((ep_idx - 4) % 3) + 1;

	return ((ext_ctrl[idx].epn_ext_ctrl2 & BIT((ep_idx - 4) / 3)) != 0);
}

static bool it82xx2_fake_token(const struct device *dev, const uint8_t ep, const uint8_t token_type)
{
	struct it82xx2_data *priv = udc_get_private(dev);
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t fifo_idx;
	bool is_fake = false;

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;

	switch (token_type) {
	case DC_IN_TRANS:
		if (ep_idx == 0) {
			if (priv->stall_is_sent) {
				return true;
			}
			is_fake = !udc_ctrl_stage_is_data_in(dev) &&
				  !udc_ctrl_stage_is_status_in(dev) &&
				  !udc_ctrl_stage_is_no_data(dev);
		} else {
			if (get_fifo_ctrl(dev, fifo_idx) != BIT(ep_idx)) {
				is_fake = true;
			}
		}
		break;
	case DC_OUTDATA_TRANS:
		if (ep_idx == 0) {
			is_fake = !udc_ctrl_stage_is_data_out(dev) &&
				  !udc_ctrl_stage_is_status_out(dev);
		} else {
			if (!atomic_test_bit(&priv->out_fifo_state,
					     IT82xx2_STATE_OUT_SHARED_FIFO_BUSY)) {
				is_fake = true;
			}
		}
		break;
	default:
		LOG_ERR("Invalid token type(%d)", token_type);
		is_fake = true;
		break;
	}

	return is_fake;
}

static inline int work_handler_in(const struct device *dev, uint8_t ep)
{
	struct it82xx2_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t fifo_idx;
	int err = 0;

	if (it82xx2_fake_token(dev, ep, DC_IN_TRANS)) {
		return 0;
	}

	if (ep != USB_CONTROL_EP_IN) {
		fifo_idx = ep_fifo_res[USB_EP_GET_IDX(ep) % SHARED_FIFO_NUM];
		it82xx2_usb_fifo_ctrl(dev, ep, true);
		k_sem_give(&priv->fifo_sem[fifo_idx - 1]);
	}

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}
	ep_cfg = udc_get_ep_cfg(dev, ep);

	net_buf_pull(buf, MIN(buf->len, udc_mps_ep_size(ep_cfg)));

	it82xx2_usb_set_ep_ctrl(dev, ep, EP_DATA_SEQ_TOGGLE, true);

	if (buf->len) {
		work_handler_xfer_continue(dev, ep, buf);
		return 0;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		work_handler_xfer_continue(dev, ep, buf);
		udc_ep_buf_clear_zlp(buf);
		return 0;
	}

	buf = udc_buf_get(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	udc_ep_set_busy(dev, ep, false);

	if (ep == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/*
			 * IN transfer finished, release buffer,
			 * Feed control OUT buffer for status stage.
			 */
			net_buf_unref(buf);
			err = it82xx2_ctrl_feed_dout(dev, 0U);
		}
		return err;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static inline int work_handler_setup(const struct device *dev, uint8_t ep)
{
	struct it82xx2_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	int err = 0;

	if (udc_ctrl_stage_is_status_out(dev)) {
		/* out -> setup */
		buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
		if (buf) {
			udc_ep_set_busy(dev, USB_CONTROL_EP_OUT, false);
			net_buf_unref(buf);
		}
	}

	if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
		/* in -> setup */
		work_handler_in(dev, USB_CONTROL_EP_IN);
	}

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	udc_ep_buf_set_setup(buf);
	it82xx2_xfer_out_data(dev, ep, buf);
	if (buf->len != sizeof(struct usb_setup_packet)) {
		LOG_DBG("buffer length %d read from chip", buf->len);
		net_buf_unref(buf);
		return 0;
	}

	priv->stall_is_sent = false;
	LOG_HEXDUMP_DBG(buf->data, buf->len, "setup:");

	udc_ctrl_update_stage(dev, buf);

	it82xx2_usb_set_ep_ctrl(dev, ep, EP_DATA_SEQ_1, true);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/* Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = it82xx2_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		udc_ctrl_submit_s_in_status(dev);
	} else {
		udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static inline int work_handler_out(const struct device *dev, uint8_t ep)
{
	struct net_buf *buf;
	int err = 0;
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	const struct usb_it82xx2_config *config = dev->config;
	struct it82xx2_data *priv = udc_get_private(dev);
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	struct udc_ep_config *ep_cfg;
	uint8_t fifo_idx;
	size_t len;

	if (it82xx2_fake_token(dev, ep, DC_OUTDATA_TRANS)) {
		return 0;
	}

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	fifo_idx = ep_idx > 0 ? ep_fifo_res[ep_idx % SHARED_FIFO_NUM] : 0;
	len = (uint16_t)ff_regs[fifo_idx].ep_rx_fifo_dcnt_lsb +
	      (((uint16_t)ff_regs[fifo_idx].ep_rx_fifo_dcnt_msb) << 8);

	if (ep == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev) && len != 0) {
			LOG_DBG("Handle early setup token");
			buf = udc_buf_get(dev, ep);
			/* Notify upper layer */
			udc_ctrl_submit_status(dev, buf);
			/* Update to next stage of control transfer */
			udc_ctrl_update_stage(dev, buf);
			return 0;
		}
	}

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (len > udc_mps_ep_size(ep_cfg)) {
		LOG_ERR("Failed to handle this packet due to the packet size");
		return -ENOBUFS;
	}

	it82xx2_xfer_out_data(dev, ep, buf);

	LOG_DBG("Handle data OUT, %zu | %zu", len, net_buf_tailroom(buf));

	if (net_buf_tailroom(buf) && len == udc_mps_ep_size(ep_cfg)) {
		work_handler_xfer_continue(dev, ep, buf);
		if (ep != USB_CONTROL_EP_OUT) {
			err = udc_submit_ep_event(dev, buf, 0);
		}
		return err;
	}

	buf = udc_buf_get(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	udc_ep_set_busy(dev, ep, false);

	if (ep == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			it82xx2_usb_set_ep_ctrl(dev, ep, EP_DATA_SEQ_1, true);
			err = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		atomic_clear_bit(&priv->out_fifo_state, IT82xx2_STATE_OUT_SHARED_FIFO_BUSY);
		err = udc_submit_ep_event(dev, buf, 0);
	}

	return err;
}

static void xfer_work_handler(const struct device *dev)
{
	while (true) {
		struct it82xx2_ep_event evt;
		int err = 0;

		k_msgq_get(&evt_msgq, &evt, K_FOREVER);

		switch (evt.event) {
		case IT82xx2_EVT_SETUP_TOKEN:
			err = work_handler_setup(evt.dev, evt.ep);
			break;
		case IT82xx2_EVT_IN_TOKEN:
			err = work_handler_in(evt.dev, evt.ep);
			break;
		case IT82xx2_EVT_OUT_TOKEN:
			err = work_handler_out(evt.dev, evt.ep);
			break;
		case IT82xx2_EVT_XFER:
			break;
		default:
			LOG_ERR("Unknown event type 0x%x", evt.event);
			err = -EINVAL;
			break;
		}

		if (err) {
			udc_submit_event(evt.dev, UDC_EVT_ERROR, err);
		}

		if (evt.ep != USB_CONTROL_EP_OUT && !udc_ep_is_busy(evt.dev, evt.ep)) {
			if (work_handler_xfer_next(evt.dev, evt.ep) == 0) {
				udc_ep_set_busy(evt.dev, evt.ep, true);
			}
		}
	}
}

static inline bool it82xx2_check_ep0_stall(const struct device *dev, const uint8_t ep_idx,
					   const uint8_t transtype)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	if (ep_idx != 0) {
		return false;
	}

	/* Check if the stall bit is set */
	if (ep_regs[ep_idx].ep_ctrl.fields.send_stall_bit) {
		it82xx2_usb_set_ep_ctrl(dev, ep_idx, EP_STALL_SEND, false);
		if (transtype == DC_SETUP_TRANS) {
			ff_regs[ep_idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
		}
		LOG_ERR("Cleared stall bit");
		return true;
	}

	/* Check if the IN transaction is STALL */
	if ((transtype == DC_IN_TRANS) && (ep_regs[ep_idx].ep_status & DC_STALL_SENT)) {
		return true;
	}

	return false;
}

static void it82xx2_usb_xfer_done(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	for (uint8_t fifo_idx = 0; fifo_idx < 4; fifo_idx++) {
		bool enable_bit, ready_bit;
		uint8_t ep, ep_idx, ep_ctrl, transtype;

		ep_ctrl = ep_regs[fifo_idx].ep_ctrl.value;
		transtype = ep_regs[fifo_idx].ep_transtype_sts & DC_ALL_TRANS;

		if (fifo_idx == 0) {
			ep_idx = 0;
			if (it82xx2_check_ep0_stall(dev, ep_idx, transtype)) {
				continue;
			}
		} else {
			ep_idx = (epn_ext_ctrl[fifo_idx].epn_ext_ctrl2 & COMPLETED_TRANS) >> 4;
			if (ep_idx == 0) {
				continue;
			}
		}

		if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
			enable_bit = get_extend_enable_bit(dev, ep_idx);
			ready_bit = get_extend_ready_bit(dev, ep_idx);
		} else {
			enable_bit = (ep_regs[ep_idx].ep_ctrl.fields.enable_bit != 0);
			ready_bit = (ep_regs[ep_idx].ep_ctrl.fields.ready_bit != 0);
		}

		/* The enable bit is set and the ready bit is cleared if the
		 * transaction is completed.
		 */
		if (!enable_bit || ready_bit) {
			continue;
		}

		if (ep_idx != 0) {
			if (it82xx2_fake_token(dev, ep_idx, transtype)) {
				continue;
			}
		}

		switch (transtype) {
		case DC_SETUP_TRANS:
			/* SETUP transaction done */
			it82xx2_event_submit(dev, ep_idx, IT82xx2_EVT_SETUP_TOKEN);
			break;
		case DC_IN_TRANS:
			/* IN transaction done */
			ep = USB_EP_DIR_IN | ep_idx;
			it82xx2_event_submit(dev, ep, IT82xx2_EVT_IN_TOKEN);
			break;
		case DC_OUTDATA_TRANS:
			/* OUT transaction done */
			ep = USB_EP_DIR_OUT | ep_idx;
			it82xx2_event_submit(dev, ep, IT82xx2_EVT_OUT_TOKEN);
			break;
		default:
			LOG_ERR("Unknown transaction type");
			break;
		}
	}
}

static inline void emit_resume_event(const struct device *dev)
{
	struct it82xx2_data *priv = udc_get_private(dev);

	if (udc_is_suspended(dev) && udc_is_enabled(dev)) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
		k_sem_give(&priv->suspended_sem);
	}
}

static void it82xx2_usb_dc_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_data *priv = udc_get_private(dev);

	uint8_t status = usb_regs->dc_interrupt_status &
			 usb_regs->dc_interrupt_mask; /* mask non enable int */

	/* reset event */
	if (status & DC_RESET_EVENT) {
		if ((usb_regs->dc_line_status & RX_LINE_STATE_MASK) == RX_LINE_RESET) {
			it82xx2_dc_reset(dev);
			usb_regs->dc_interrupt_status = DC_RESET_EVENT;

			udc_submit_event(dev, UDC_EVT_RESET, 0);
			return;
		}
		usb_regs->dc_interrupt_status = DC_RESET_EVENT;
	}

	/* sof received */
	if (status & DC_SOF_RECEIVED) {
		if (!USB_DEVICE_CONFIG_SOF_NOTIFICATIONS) {
			it82xx2_enable_sof_int(dev, false);
		} else {
			usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
			udc_submit_event(dev, UDC_EVT_SOF, 0);
		}
		it82xx2_enable_resume_int(dev, false);
		emit_resume_event(dev);
		k_work_cancel_delayable(&priv->suspended_work);
		k_work_reschedule(&priv->suspended_work, K_MSEC(5));
	}

	/* resume event */
	if (status & DC_RESUME_EVENT) {
		it82xx2_enable_resume_int(dev, false);
		emit_resume_event(dev);
	}

	/* transaction done */
	if (status & DC_TRANS_DONE) {
		/* clear interrupt before new transaction */
		usb_regs->dc_interrupt_status = DC_TRANS_DONE;
		it82xx2_usb_xfer_done(dev);
		return;
	}
}

static void suspended_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct it82xx2_data *priv = CONTAINER_OF(dwork, struct it82xx2_data, suspended_work);
	const struct device *dev = priv->dev;
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	unsigned int key;

	if (usb_regs->dc_interrupt_status & DC_SOF_RECEIVED) {
		usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
		k_work_reschedule(&priv->suspended_work, K_MSEC(5));
		return;
	}

	key = irq_lock();
	if (!udc_is_suspended(dev) && udc_is_enabled(dev)) {
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		it82xx2_enable_wu_irq(dev, true);
		it82xx2_enable_standby_state(true);

		k_sem_reset(&priv->suspended_sem);
	}

	it82xx2_enable_resume_int(dev, true);

	if (!USB_DEVICE_CONFIG_SOF_NOTIFICATIONS) {
		it82xx2_enable_sof_int(dev, true);
	}

	irq_unlock(key);
}

static int it82xx2_enable(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;
	struct it82xx2_data *priv = udc_get_private(dev);

	k_sem_init(&priv->suspended_sem, 0, 1);
	k_work_init_delayable(&priv->suspended_work, suspended_handler);

	atomic_clear_bit(&priv->out_fifo_state, IT82xx2_STATE_OUT_SHARED_FIFO_BUSY);

	/* Initialize FIFO semaphore */
	k_sem_init(&priv->fifo_sem[0], 1, 1);
	k_sem_init(&priv->fifo_sem[1], 1, 1);
	k_sem_init(&priv->fifo_sem[2], 1, 1);

	usb_regs->dc_control = DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
			       DC_FULL_SPEED_LINE_RATE | DC_CONNECT_TO_HOST;

	/* Enable USB D+ and USB interrupts */
	it82xx2_enable_wu_irq(dev, true);
	irq_enable(config->usb_irq);

	return 0;
}

static int it82xx2_disable(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct usb_it82xx2_regs *const usb_regs = config->base;

	irq_disable(config->usb_irq);

	/* stop pull-up D+ D-*/
	usb_regs->dc_control &= ~DC_CONNECT_TO_HOST;

	return 0;
}

static int it82xx2_init(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;
	int ret;

	/*
	 * Disable USB debug path , prevent CPU enter
	 * JTAG mode and then reset by USB command.
	 */
	gctrl_regs->GCTRL_MCCR &= ~(IT8XXX2_GCTRL_MCCR_USB_EN);
	gctrl_regs->gctrl_pmer2 |= IT8XXX2_GCTRL_PMER2_USB_PAD_EN;

	it82xx2_usb_dc_ip_init(dev);

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				     config->ep_cfg_out[0].caps.mps, 0);
	if (ret) {
		LOG_ERR("Failed to enable ep 0x%02x", USB_CONTROL_EP_OUT);
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     config->ep_cfg_in[0].caps.mps, 0);
	if (ret) {
		LOG_ERR("Failed to enable ep 0x%02x", USB_CONTROL_EP_IN);
		return ret;
	}
	return 0;
}

static int it82xx2_shutdown(const struct device *dev)
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

static int it82xx2_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int it82xx2_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

static const struct udc_api it82xx2_api = {
	.ep_enqueue = it82xx2_ep_enqueue,
	.ep_dequeue = it82xx2_ep_dequeue,
	.ep_set_halt = it82xx2_ep_set_halt,
	.ep_clear_halt = it82xx2_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = it82xx2_ep_enable,
	.ep_disable = it82xx2_ep_disable,
	.host_wakeup = it82xx2_host_wakeup,
	.set_address = it82xx2_set_address,
	.enable = it82xx2_enable,
	.disable = it82xx2_disable,
	.init = it82xx2_init,
	.shutdown = it82xx2_shutdown,
	.lock = it82xx2_lock,
	.unlock = it82xx2_unlock,
};

static int it82xx2_usb_driver_preinit(const struct device *dev)
{
	const struct usb_it82xx2_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct it82xx2_data *priv = udc_get_private(dev);
	int err;

	k_mutex_init(&data->mutex);
	k_fifo_init(&priv->fifo);

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to configure usb pins");
		return err;
	}

	for (int i = 0; i < MAX_NUM_ENDPOINTS; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = USB_CONTROL_EP_MPS;
		} else if ((i % 3) == 2) {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < MAX_NUM_ENDPOINTS; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = USB_CONTROL_EP_MPS;
		} else if ((i % 3) != 2) {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;

	priv->dev = dev;

	config->make_thread(dev);

	/* Initializing WU (USB D+) */
	it8xxx2_usb_dc_wuc_init(dev);

	/* Connect USB interrupt */
	irq_connect_dynamic(config->usb_irq, 0, it82xx2_usb_dc_isr, dev, 0);

	return 0;
}

#define IT82xx2_USB_DEVICE_DEFINE(n)                                                               \
	K_KERNEL_STACK_DEFINE(udc_it82xx2_stack_##n, CONFIG_UDC_IT82xx2_STACK_SIZE);               \
                                                                                                   \
	static void udc_it82xx2_thread_##n(void *dev, void *arg1, void *arg2)                      \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		xfer_work_handler(dev);                                                            \
	}                                                                                          \
                                                                                                   \
	static void udc_it82xx2_make_thread_##n(const struct device *dev)                          \
	{                                                                                          \
		struct it82xx2_data *priv = udc_get_private(dev);                                  \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_it82xx2_stack_##n,                         \
				K_THREAD_STACK_SIZEOF(udc_it82xx2_stack_##n),                      \
				udc_it82xx2_thread_##n, (void *)dev, NULL, NULL, K_PRIO_COOP(8),   \
				0, K_NO_WAIT);                                                     \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out[MAX_NUM_ENDPOINTS];                                 \
	static struct udc_ep_config ep_cfg_in[MAX_NUM_ENDPOINTS];                                  \
                                                                                                   \
	static struct usb_it82xx2_config udc_cfg_##n = {                                           \
		.base = (struct usb_it82xx2_regs *)DT_INST_REG_ADDR(n),                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.wuc = {.dev = IT8XXX2_DEV_WUC(0, n), .mask = IT8XXX2_DEV_WUC_MASK(0, n)},         \
		.usb_irq = DT_INST_IRQ_BY_IDX(n, 0, irq),                                          \
		.wu_irq = DT_INST_IRQ_BY_IDX(n, 1, irq),                                           \
		.ep_cfg_in = ep_cfg_out,                                                           \
		.ep_cfg_out = ep_cfg_in,                                                           \
		.make_thread = udc_it82xx2_make_thread_##n,                                        \
	};                                                                                         \
                                                                                                   \
	static struct it82xx2_data priv_data_##n = {};                                             \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &priv_data_##n,                                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, it82xx2_usb_driver_preinit, NULL, &udc_data_##n, &udc_cfg_##n,    \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &it82xx2_api);

DT_INST_FOREACH_STATUS_OKAY(IT82xx2_USB_DEVICE_DEFINE)
