/*
 * Copyright (c) 2023 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it82xx2_usb

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <soc_dt.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/dt-bindings/interrupt-controller/it8xxx2-wuc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_it82xx2, CONFIG_USB_DRIVER_LOG_LEVEL);

#define IT8XXX2_IS_EXTEND_ENDPOINT(n) (USB_EP_GET_IDX(n) >= 4)

/* USB Device Controller Registers Bits & Constants */
#define IT8XXX2_USB_IRQ			DT_INST_IRQ_BY_IDX(0, 0, irq)
#define IT8XXX2_WU90_IRQ		DT_INST_IRQ_BY_IDX(0, 1, irq)

#define FIFO_NUM			3
#define SETUP_DATA_CNT			8
#define DC_ADDR_NULL			0x00
#define DC_ADDR_MASK			0x7F

/* The related definitions of the register EP STATUS:
 * 0x41/0x45/0x49/0x4D
 */
#define EP_STATUS_ERROR			0x0F

/* The related definitions of the register dc_line_status: 0x51 */
#define RX_LINE_STATE_MASK		(RX_LINE_FULL_SPD | RX_LINE_LOW_SPD)
#define RX_LINE_LOW_SPD			0x02
#define RX_LINE_FULL_SPD		0x01
#define RX_LINE_RESET			0x00

/* EPN Extend Control 2 Register Mask Definition */
#define COMPLETED_TRANS			0xF0

/* Bit [1:0] represents the TRANSACTION_TYPE as follows: */
enum it82xx2_transaction_types {
	DC_SETUP_TRANS,
	DC_IN_TRANS,
	DC_OUTDATA_TRANS,
	DC_ALL_TRANS
};

/* The bit definitions of the register EP RX/TX FIFO Control:
 * EP_RX_FIFO_CONTROL: 0X64/0x84/0xA4/0xC4
 * EP_TX_FIFO_CONTROL: 0X74/0x94/0xB4/0xD4
 */
#define FIFO_FORCE_EMPTY		BIT(0)

/* The bit definitions of the register Host/Device Control: 0XE0 */
#define RESET_CORE			BIT(1)

/* ENDPOINT[3..0]_STATUS_REG */
#define DC_STALL_SENT			BIT(5)

/* DC_INTERRUPT_STATUS_REG */
#define DC_TRANS_DONE			BIT(0)
#define DC_RESUME_INT			BIT(1)
#define DC_RESET_EVENT			BIT(2)
#define DC_SOF_RECEIVED			BIT(3)
#define DC_NAK_SENT_INT			BIT(4)

/* DC_CONTROL_REG */
#define DC_GLOBAL_ENABLE		BIT(0)
#define DC_TX_LINE_STATE_DM		BIT(1)
#define DC_DIRECT_CONTROL		BIT(3)
#define DC_FULL_SPEED_LINE_POLARITY	BIT(4)
#define DC_FULL_SPEED_LINE_RATE		BIT(5)
#define DC_CONNECT_TO_HOST		BIT(6) /* internal pull-up */

/* ENDPOINT[3..0]_CONTROL_REG */
#define ENDPOINT_ENABLE_BIT      BIT(0)
#define ENDPOINT_READY_BIT       BIT(1)
#define ENDPOINT_OUTDATA_SEQ_BIT BIT(2)
#define ENDPOINT_SEND_STALL_BIT  BIT(3)
#define ENDPOINT_ISO_ENABLE_BIT  BIT(4)
#define ENDPOINT_DIRECTION_BIT   BIT(5)

enum it82xx2_ep_status {
	EP_INIT = 0,
	EP_CHECK,
	EP_CONFIG,
	EP_CONFIG_IN,
	EP_CONFIG_OUT,
};

enum it82xx2_trans_type {
	SETUP_TOKEN,
	IN_TOKEN,
	OUT_TOKEN,
};

enum it82xx2_setup_stage {
	INIT_ST,
	SETUP_ST,
	DIN_ST,
	DOUT_ST,
	STATUS_ST,
	STALL_SEND,
};

enum it82xx2_ep_ctrl {
	EP_IN_DIRECTION_SET,
	EP_STALL_SEND,
	EP_STALL_CHECK,
	EP_IOS_ENABLE,
	EP_ENABLE,
	EP_DATA_SEQ_1,
	EP_DATA_SEQ_TOGGLE,
	EP_READY_ENABLE,
};

struct usb_it8xxx2_wuc {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct usb_it82xx2_config {
	struct usb_it82xx2_regs *const base;
	const struct pinctrl_dev_config *pcfg;
	const struct usb_it8xxx2_wuc *wuc_list;
};

static const struct usb_it8xxx2_wuc usb_wuc0[IT8XXX2_DT_INST_WUCCTRL_LEN(0)] =
		IT8XXX2_DT_WUC_ITEMS_LIST(0);

PINCTRL_DT_INST_DEFINE(0);

static const struct usb_it82xx2_config ucfg0 = {
	.base = (struct usb_it82xx2_regs *)DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.wuc_list = usb_wuc0
};

struct it82xx2_endpoint_data {
	usb_dc_ep_callback cb_in;
	usb_dc_ep_callback cb_out;
	enum it82xx2_ep_status ep_status;
	enum usb_dc_ep_transfer_type ep_type;
	uint16_t remaining; /* remaining bytes */
	uint16_t mps;
};

struct usb_it82xx2_data {
	const struct device *dev;
	struct it82xx2_endpoint_data ep_data[MAX_NUM_ENDPOINTS];
	enum it82xx2_setup_stage st_state; /* Setup State */

	/* EP0 status */
	enum it82xx2_trans_type last_token;

	/* EP0 status */
	enum it82xx2_trans_type now_token;

	uint8_t attached;
	uint8_t addr;
	bool no_data_ctrl;
	bool suspended;
	usb_dc_status_callback usb_status_cb;

	/* FIFO_1/2/3 ready status */
	bool fifo_ready[3];

	struct k_sem fifo_sem[3];
	struct k_sem suspended_sem;
	struct k_work_delayable check_suspended_work;
};

/* The ep_fifo_res[ep_idx % FIFO_NUM] where the FIFO_NUM is 3 represents the
 * EP mapping because when (ep_idx % FIFO_NUM) is 3, it actually means the EP0.
 */
static const uint8_t ep_fifo_res[3] = {3, 1, 2};

static struct usb_it82xx2_data udata0;

static struct usb_it82xx2_regs *it82xx2_get_usb_regs(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(usb0));
	const struct usb_it82xx2_config *cfg = dev->config;
	struct usb_it82xx2_regs *const usb_regs = cfg->base;

	return usb_regs;
}

static void it82xx2_enable_sof_int(bool enable)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
	if (enable) {
		usb_regs->dc_interrupt_mask |= DC_SOF_RECEIVED;
	} else {
		usb_regs->dc_interrupt_mask &= ~DC_SOF_RECEIVED;
	}
}

static void it82xx2_enable_resume_int(bool enable)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	usb_regs->dc_interrupt_status = DC_RESUME_INT;
	if (enable) {
		usb_regs->dc_interrupt_mask |= DC_RESUME_INT;
	} else {
		usb_regs->dc_interrupt_mask &= ~DC_RESUME_INT;
	}
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

/* WU90 (USB D+) Enable/Disable */
static void it82xx2_enable_wu90_irq(const struct device *dev, bool enable)
{
	const struct usb_it82xx2_config *cfg = dev->config;

	/* Clear pending interrupt */
	it8xxx2_wuc_clear_status(cfg->wuc_list[0].wucs, cfg->wuc_list[0].mask);

	if (enable) {
		irq_enable(IT8XXX2_WU90_IRQ);
	} else {
		irq_disable(IT8XXX2_WU90_IRQ);
	}
}

static void it82xx2_wu90_isr(const struct device *dev)
{
	it82xx2_enable_wu90_irq(dev, false);
	it82xx2_enable_standby_state(false);
	LOG_DBG("USB D+ (WU90) Triggered");
}

/* WU90 (USB D+) Initializations */
static void it8xxx2_usb_dc_wuc_init(const struct device *dev)
{
	const struct usb_it82xx2_config *cfg = dev->config;

	/* Initializing the WUI */
	it8xxx2_wuc_set_polarity(cfg->wuc_list[0].wucs,
				cfg->wuc_list[0].mask,
				WUC_TYPE_EDGE_FALLING);
	it8xxx2_wuc_clear_status(cfg->wuc_list[0].wucs,
				cfg->wuc_list[0].mask);

	/* Enabling the WUI */
	it8xxx2_wuc_enable(cfg->wuc_list[0].wucs, cfg->wuc_list[0].mask);

	/* Connect WU90 (USB D+) interrupt but make it disabled initially */
	IRQ_CONNECT(IT8XXX2_WU90_IRQ, 0, it82xx2_wu90_isr, 0, 0);
}

static int it82xx2_usb_fifo_ctrl(const uint8_t ep, const bool clear)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	volatile uint8_t *ep_fifo_ctrl = usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t fifon_ctrl = (ep_fifo_res[ep_idx % FIFO_NUM] - 1) * 2;
	unsigned int key;
	int ret = 0;

	if (ep_idx == 0) {
		LOG_ERR("Invalid endpoint 0x%x", ep);
		return -EINVAL;
	}

	key = irq_lock();
	if (clear) {
		ep_fifo_ctrl[fifon_ctrl] = 0x0;
		ep_fifo_ctrl[fifon_ctrl + 1] = 0x0;
		goto out;
	}

	if (USB_EP_DIR_IS_IN(ep) && udata0.ep_data[ep_idx].ep_status == EP_CONFIG_IN) {
		if (ep_idx < 8) {
			ep_fifo_ctrl[fifon_ctrl] = BIT(ep_idx);
			ep_fifo_ctrl[fifon_ctrl + 1] = 0x0;
		} else {
			ep_fifo_ctrl[fifon_ctrl] = 0x0;
			ep_fifo_ctrl[fifon_ctrl + 1] = BIT(ep_idx - 8);
		}
	} else if (USB_EP_DIR_IS_OUT(ep) &&
		   udata0.ep_data[ep_idx].ep_status == EP_CONFIG_OUT) {
		if (ep_idx < 8) {
			ep_fifo_ctrl[fifon_ctrl] |= BIT(ep_idx);
		} else {
			ep_fifo_ctrl[fifon_ctrl + 1] |= BIT(ep_idx - 8);
		}
	} else {
		LOG_ERR("Failed to set fifo control register for ep 0x%x", ep);
		ret = -EINVAL;
	}

out:
	irq_unlock(key);
	return ret;
}

static volatile void *it82xx2_get_ext_ctrl(int ep_idx, enum it82xx2_ep_ctrl ctrl)
{
	uint8_t idx;
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	union epn0n1_extend_ctrl_reg *epn0n1_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl;
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	if ((ctrl == EP_IN_DIRECTION_SET) || (ctrl == EP_ENABLE)) {
		idx = ((ep_idx - 4) % 3) + 1;
		return &ext_ctrl[idx].epn_ext_ctrl1;
	}

	idx = (ep_idx - 4) / 2;
	return &epn0n1_ext_ctrl[idx];
}

static int it82xx2_usb_extend_ep_ctrl(uint8_t ep, enum it82xx2_ep_ctrl ctrl, bool enable)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;
	union epn_extend_ctrl1_reg *epn_ext_ctrl1 = NULL;
	union epn0n1_extend_ctrl_reg *epn0n1_ext_ctrl = NULL;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;

	if (!IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		return -EINVAL;
	}

	if ((ctrl == EP_IN_DIRECTION_SET) || (ctrl == EP_ENABLE)) {
		epn_ext_ctrl1 = (union epn_extend_ctrl1_reg *)it82xx2_get_ext_ctrl(ep_idx, ctrl);
	} else {
		epn0n1_ext_ctrl =
			(union epn0n1_extend_ctrl_reg *)it82xx2_get_ext_ctrl(ep_idx, ctrl);
	}

	switch (ctrl) {
	case EP_STALL_SEND:
		if (ep_idx % 2) {
			epn0n1_ext_ctrl->fields.epn1_send_stall_bit = enable;
		} else {
			epn0n1_ext_ctrl->fields.epn0_send_stall_bit = enable;
		}
		break;
	case EP_STALL_CHECK:
		if (ep_idx % 2) {
			return epn0n1_ext_ctrl->fields.epn1_send_stall_bit;
		} else {
			return epn0n1_ext_ctrl->fields.epn0_send_stall_bit;
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
		ep_regs[ep_fifo].ep_ctrl.fields.ready_bit = enable;
		break;
	default:
		LOG_ERR("Unknown control type 0x%x", ctrl);
		return -EINVAL;
	}

	return 0;
}

static int it82xx2_usb_ep_ctrl(uint8_t ep, enum it82xx2_ep_ctrl ctrl, bool enable)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	uint8_t ep_ctrl_value;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		return -EINVAL;
	}

	ep_ctrl_value = ep_regs[ep_idx].ep_ctrl.value & ~ENDPOINT_READY_BIT;

	switch (ctrl) {
	case EP_IN_DIRECTION_SET:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_DIRECTION_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_DIRECTION_BIT;
		}
		break;
	case EP_STALL_SEND:
		if (enable) {
			ep_ctrl_value |= ENDPOINT_SEND_STALL_BIT;
		} else {
			ep_ctrl_value &= ~ENDPOINT_SEND_STALL_BIT;
		}
		break;
	case EP_STALL_CHECK:
		return ep_regs[ep_idx].ep_ctrl.fields.send_stall_bit;
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

static int it82xx2_usb_set_ep_ctrl(uint8_t ep, enum it82xx2_ep_ctrl ctrl, bool enable)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int ret = 0;
	unsigned int key;

	key = irq_lock();
	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		ret = it82xx2_usb_extend_ep_ctrl(ep, ctrl, enable);
	} else {
		ret = it82xx2_usb_ep_ctrl(ep, ctrl, enable);
	}
	irq_unlock(key);
	return ret;
}

static int it82xx2_usb_dc_ip_init(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	/* Reset Device Controller */
	usb_regs->host_device_control = RESET_CORE;
	k_msleep(1);
	usb_regs->port0_misc_control &= ~(PULL_DOWN_EN);
	usb_regs->port1_misc_control &= ~(PULL_DOWN_EN);
	/* clear reset bit */
	usb_regs->host_device_control = 0;

	usb_regs->dc_interrupt_status =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED | DC_RESUME_INT;

	usb_regs->dc_interrupt_mask = 0x00;
	usb_regs->dc_interrupt_mask =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED | DC_RESUME_INT;

	usb_regs->dc_address = DC_ADDR_NULL;

	return 0;
}

static int it82xx2_usb_dc_attach_init(void)
{
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;
	/*
	 * Disable USB debug path , prevent CPU enter
	 * JTAG mode and then reset by USB command.
	 */
	gctrl_regs->GCTRL_MCCR &= ~(IT8XXX2_GCTRL_MCCR_USB_EN);
	gctrl_regs->gctrl_pmer2 |= IT8XXX2_GCTRL_PMER2_USB_PAD_EN;

	return it82xx2_usb_dc_ip_init();
}

/* Check the condition that SETUP_TOKEN following OUT_TOKEN and return it */
static bool it82xx2_check_setup_following_out(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	return ((ep_regs[EP0].ep_transtype_sts & DC_ALL_TRANS) == 0 ||
		(udata0.last_token == IN_TOKEN &&
		ff_regs[EP0].ep_rx_fifo_dcnt_lsb == SETUP_DATA_CNT));
}

static inline void it82xx2_handler_setup(uint8_t fifo_idx)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	uint8_t ep_idx = fifo_idx;

	/* wrong trans */
	if (ep_regs[ep_idx].ep_ctrl.fields.send_stall_bit) {
		it82xx2_usb_set_ep_ctrl(fifo_idx, EP_STALL_SEND, false);
		udata0.st_state = STALL_SEND;
		ff_regs[fifo_idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
		LOG_DBG("Clear Stall Bit & RX FIFO");
		return;
	}

	if (udata0.st_state == DIN_ST) {
		/* setup -> in(data) -> out(status) */
		udata0.last_token = udata0.now_token;
		udata0.now_token = OUT_TOKEN;
		udata0.st_state = STATUS_ST;
		udata0.ep_data[ep_idx].cb_out(ep_idx | USB_EP_DIR_OUT, USB_DC_EP_DATA_OUT);

	} else if (udata0.st_state == DOUT_ST || udata0.st_state == SETUP_ST) {
		/* setup -> out(data) -> in(status)
		 * or
		 * setup -> in(status)
		 */
		udata0.last_token = udata0.now_token;
		udata0.now_token = IN_TOKEN;
		udata0.st_state = STATUS_ST;
		udata0.ep_data[ep_idx].cb_in(ep_idx | USB_EP_DIR_IN, USB_DC_EP_DATA_IN);
	}

	udata0.last_token = udata0.now_token;
	udata0.now_token = SETUP_TOKEN;
	udata0.st_state = SETUP_ST;

	ep_regs[fifo_idx].ep_ctrl.fields.outdata_sequence_bit = 1;
	udata0.ep_data[ep_idx].cb_out(ep_idx | USB_EP_DIR_OUT, USB_DC_EP_SETUP);

	/* Set ready bit to no-data control in */
	if (udata0.no_data_ctrl) {
		it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
		udata0.no_data_ctrl = false;
	}
}

static inline void it82xx2_handler_in(const uint8_t ep_idx)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	if (ep_idx == 0) {
		if (ep_regs[ep_idx].ep_ctrl.fields.send_stall_bit) {
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_STALL_SEND, false);
			udata0.st_state = STALL_SEND;
			LOG_DBG("Clear Stall Bit");
			return;
		}

		if (udata0.st_state >= STATUS_ST) {
			return;
		}

		udata0.last_token = udata0.now_token;
		udata0.now_token = IN_TOKEN;

		if (udata0.addr != DC_ADDR_NULL &&
			udata0.addr != usb_regs->dc_address) {
			usb_regs->dc_address = udata0.addr;
			LOG_DBG("Address Is Set Successfully");
		}

		if (udata0.st_state == DOUT_ST) {
			/* setup -> out(data) -> in(status) */
			udata0.st_state = STATUS_ST;

		} else if (udata0.ep_data[ep_idx].remaining == 0 &&
		udata0.st_state == SETUP_ST) {
			/* setup -> in(status) */
			udata0.st_state = STATUS_ST;
		} else {
			/* setup -> in(data) */
			udata0.st_state = DIN_ST;
		}
	}

	it82xx2_usb_set_ep_ctrl(ep_idx, EP_DATA_SEQ_TOGGLE, true);

	if (udata0.ep_data[ep_idx].cb_in) {
		udata0.ep_data[ep_idx].cb_in(ep_idx | USB_EP_DIR_IN, USB_DC_EP_DATA_IN);
	}

	if (ep_idx != 0) {
		uint8_t ep_fifo = ep_fifo_res[ep_idx % FIFO_NUM];

		/* clear fifo ctrl registers when IN transaction is completed */
		it82xx2_usb_fifo_ctrl(ep_idx, true);
		k_sem_give(&udata0.fifo_sem[ep_fifo - 1]);
	} else {
		if (udata0.st_state == DIN_ST && udata0.ep_data[ep_idx].remaining == 0) {
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
		}
	}
}

static inline void it82xx2_handler_out(const uint8_t ep_idx)
{
	if (ep_idx == 0) {
		/* ep0 wrong enter check */
		if (udata0.st_state >= STATUS_ST) {
			return;
		}

		udata0.last_token = udata0.now_token;
		udata0.now_token = OUT_TOKEN;

		if (udata0.st_state == SETUP_ST) {
			/* setup -> out(data) */
			udata0.st_state = DOUT_ST;
		} else {
			/* setup -> in(data) -> out(status) */
			udata0.st_state = STATUS_ST;
		}
	}

	if (udata0.ep_data[ep_idx].cb_out) {
		udata0.ep_data[ep_idx].cb_out(ep_idx, USB_DC_EP_DATA_OUT);
	}

	if (ep_idx == 0) {
		/* SETUP_TOKEN follow OUT_TOKEN */
		if (it82xx2_check_setup_following_out()) {
			udata0.last_token = udata0.now_token;
			udata0.now_token = SETUP_TOKEN;
			udata0.st_state = SETUP_ST;
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_DATA_SEQ_1, true);
			udata0.ep_data[ep_idx].cb_out(ep_idx | USB_EP_DIR_OUT, USB_DC_EP_SETUP);

			if (udata0.no_data_ctrl) {
				it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
				udata0.no_data_ctrl = false;
			}
		}
	}
}

static bool get_extend_enable_bit(const uint8_t ep_idx)
{
	union epn_extend_ctrl1_reg *epn_ext_ctrl1 = NULL;
	bool enable;

	epn_ext_ctrl1 = (union epn_extend_ctrl1_reg *)it82xx2_get_ext_ctrl(ep_idx, EP_ENABLE);
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

static bool get_extend_ready_bit(const uint8_t ep_idx)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct epn_ext_ctrl_regs *ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;
	int idx = ((ep_idx - 4) % 3) + 1;

	return ((ext_ctrl[idx].epn_ext_ctrl2 & BIT((ep_idx - 4) / 3)) != 0);
}

static uint16_t get_fifo_ctrl(const uint8_t fifo_idx)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	volatile uint8_t *ep_fifo_ctrl = usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;
	uint8_t fifon_ctrl = (fifo_idx - 1) * 2;

	if (fifo_idx == 0) {
		LOG_ERR("Invalid fifo_idx 0x%x", fifo_idx);
		return 0;
	}

	return (ep_fifo_ctrl[fifon_ctrl + 1] << 8 | ep_fifo_ctrl[fifon_ctrl]);
}

static bool it82xx2_usb_fake_token(const uint8_t ep_idx, uint8_t *token_type)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	bool is_fake = false;
	bool enable_bit, ready_bit;
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;

	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		enable_bit = get_extend_enable_bit(ep_idx);
		ready_bit = get_extend_ready_bit(ep_idx);
	} else {
		enable_bit = (ep_regs[ep_idx].ep_ctrl.fields.enable_bit != 0);
		ready_bit = (ep_regs[ep_idx].ep_ctrl.fields.ready_bit != 0);
	}

	/* The enable bit is set and the ready bit is cleared if the
	 * transaction is completed.
	 */
	if (!enable_bit || ready_bit) {
		return true;
	}

	*token_type = ep_regs[ep_fifo].ep_transtype_sts & DC_ALL_TRANS;

	if (ep_idx == 0) {
		return false;
	}

	switch (*token_type) {
	case DC_IN_TRANS:
		if (get_fifo_ctrl(ep_fifo) != BIT(ep_idx) ||
		    udata0.ep_data[ep_idx].ep_status != EP_CONFIG_IN) {
			is_fake = true;
		}
		break;
	case DC_OUTDATA_TRANS:
		if (!udata0.fifo_ready[ep_fifo - 1] ||
		    udata0.ep_data[ep_idx].ep_status != EP_CONFIG_OUT) {
			is_fake = true;
		} else {
			udata0.fifo_ready[ep_fifo - 1] = false;
		}
		break;
	case DC_SETUP_TRANS:
		__fallthrough;
	default:
		is_fake = true;
		break;
	}

	return is_fake;
}

static void it82xx2_usb_dc_trans_done(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	for (uint8_t fifo_idx = 0; fifo_idx < 4; fifo_idx++) {
		uint8_t ep_idx, token_type;

		if (fifo_idx == 0) {
			ep_idx = 0;
		} else {
			ep_idx = (epn_ext_ctrl[fifo_idx].epn_ext_ctrl2 & COMPLETED_TRANS) >> 4;
			if (ep_idx == 0) {
				continue;
			}
		}

		if (!it82xx2_usb_fake_token(ep_idx, &token_type)) {
			switch (token_type) {
			case DC_SETUP_TRANS:
				it82xx2_handler_setup(fifo_idx);
				break;
			case DC_IN_TRANS:
				it82xx2_handler_in(ep_idx);
				break;
			case DC_OUTDATA_TRANS:
				it82xx2_handler_out(ep_idx);
				break;
			default:
				break;
			}
		}
	}
}

static inline void emit_resume_event(void)
{
	if (udata0.suspended) {
		udata0.suspended = false;
		k_sem_give(&udata0.suspended_sem);
		if (udata0.usb_status_cb) {
			(*(udata0.usb_status_cb))(USB_DC_RESUME, NULL);
		}
	}
}

static void it82xx2_usb_dc_isr(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	uint8_t status = usb_regs->dc_interrupt_status &
		usb_regs->dc_interrupt_mask; /* mask non enable int */

	/* reset */
	if (status & DC_RESET_EVENT) {
		if ((usb_regs->dc_line_status & RX_LINE_STATE_MASK) ==
			RX_LINE_RESET) {
			usb_dc_reset();
			usb_regs->dc_interrupt_status = DC_RESET_EVENT;
			return;
		} else {
			usb_regs->dc_interrupt_status = DC_RESET_EVENT;
		}
	}
	/* sof received */
	if (status & DC_SOF_RECEIVED) {
		it82xx2_enable_sof_int(false);
		it82xx2_enable_resume_int(false);
		emit_resume_event();
		k_work_reschedule(&udata0.check_suspended_work, K_MSEC(5));
	}
	/* resume received */
	if (status & DC_RESUME_INT) {
		it82xx2_enable_resume_int(false);
		emit_resume_event();
	}
	/* transaction done */
	if (status & DC_TRANS_DONE) {
		/* clear interrupt before new transaction */
		usb_regs->dc_interrupt_status = DC_TRANS_DONE;
		it82xx2_usb_dc_trans_done();
		return;
	}

}

static void suspended_check_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct usb_it82xx2_data *udata =
		CONTAINER_OF(dwork, struct usb_it82xx2_data, check_suspended_work);

	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	unsigned int key;

	if (usb_regs->dc_interrupt_status & DC_SOF_RECEIVED) {
		usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
		k_work_reschedule(&udata->check_suspended_work, K_MSEC(5));
		return;
	}

	key = irq_lock();
	if (!udata->suspended) {
		if (udata->usb_status_cb) {
			(*(udata->usb_status_cb))(USB_DC_SUSPEND, NULL);
		}
		udata->suspended = true;
		it82xx2_enable_wu90_irq(udata->dev, true);
		it82xx2_enable_standby_state(true);

		k_sem_reset(&udata->suspended_sem);
	}

	it82xx2_enable_resume_int(true);
	it82xx2_enable_sof_int(true);

	irq_unlock(key);
}

/*
 * USB Device Controller API
 */
int usb_dc_attach(void)
{
	int ret;
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	if (udata0.attached) {
		LOG_DBG("Already Attached");
		return 0;
	}

	LOG_DBG("Attached");
	ret = it82xx2_usb_dc_attach_init();

	if (ret) {
		return ret;
	}

	for (uint8_t idx = 0; idx < MAX_NUM_ENDPOINTS; idx++) {
		udata0.ep_data[idx].ep_status = EP_INIT;
	}

	udata0.attached = 1U;

	/* init fifo ready status */
	udata0.fifo_ready[0] = false;
	udata0.fifo_ready[1] = false;
	udata0.fifo_ready[2] = false;

	k_sem_init(&udata0.fifo_sem[0], 1, 1);
	k_sem_init(&udata0.fifo_sem[1], 1, 1);
	k_sem_init(&udata0.fifo_sem[2], 1, 1);
	k_sem_init(&udata0.suspended_sem, 0, 1);

	k_work_init_delayable(&udata0.check_suspended_work, suspended_check_handler);

	/* Connect USB interrupt */
	IRQ_CONNECT(IT8XXX2_USB_IRQ, 0, it82xx2_usb_dc_isr, 0, 0);

	usb_regs->dc_control =
		DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
		DC_FULL_SPEED_LINE_RATE | DC_CONNECT_TO_HOST;

	/* Enable USB D+ and USB interrupts */
	it82xx2_enable_wu90_irq(udata0.dev, true);
	irq_enable(IT8XXX2_USB_IRQ);

	return 0;
}

int usb_dc_detach(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	if (!udata0.attached) {
		LOG_DBG("Already Detached");
		return 0;
	}

	LOG_DBG("Detached");
	irq_disable(IT8XXX2_USB_IRQ);

	/* stop pull-up D+ D-*/
	usb_regs->dc_control &= ~DC_CONNECT_TO_HOST;
	udata0.attached = 0U;

	return 0;
}

int usb_dc_reset(void)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	LOG_DBG("USB Device Reset");

	ff_regs[EP0].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
	ff_regs[EP0].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;

	for (uint8_t idx = 1; idx < 4; idx++) {
		if (udata0.ep_data[idx].ep_status > EP_CHECK) {
			ff_regs[idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
			ff_regs[idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
		}
	}

	ep_regs[0].ep_ctrl.value = ENDPOINT_ENABLE_BIT;
	usb_regs->dc_address = DC_ADDR_NULL;
	udata0.addr = DC_ADDR_NULL;
	usb_regs->dc_interrupt_status = DC_NAK_SENT_INT | DC_SOF_RECEIVED;

	if (udata0.usb_status_cb) {
		(*(udata0.usb_status_cb))(USB_DC_RESET, NULL);
	}

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("Set Address(0x%02x) to Data", addr);
	udata0.addr = addr & DC_ADDR_MASK;
	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	udata0.usb_status_cb = cb;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	bool in = USB_EP_DIR_IS_IN(cfg->ep_addr);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx > EP0) {
		LOG_ERR("Invalid Endpoint Configuration");
		return -EINVAL;
	}

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_WRN("Invalid Endpoint Number 0x%02x", cfg->ep_addr);
		return -EINVAL;
	}

	if ((ep_idx != 0) && (!in && ep_idx % FIFO_NUM != 2)) {
		LOG_WRN("Invalid Endpoint Number 0x%02x", cfg->ep_addr);
		return -EINVAL;
	}

	if ((ep_idx != 0) && (in && ep_idx % FIFO_NUM == 2)) {
		LOG_WRN("Invalid Endpoint Number 0x%02x", cfg->ep_addr);
		return -EINVAL;
	}

	if (udata0.ep_data[ep_idx].ep_status > EP_INIT) {
		LOG_WRN("EP%d have been used", ep_idx);
		return -EINVAL;
	}

	if (ep_idx > EP0) {
		udata0.ep_data[ep_idx].mps = cfg->ep_mps;
	}

	udata0.ep_data[ep_idx].ep_status = EP_CHECK;
	LOG_DBG("Check cap(%02x)", cfg->ep_addr);

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);
	bool in = USB_EP_DIR_IS_IN(cfg->ep_addr);

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_DBG("Not attached / Invalid Endpoint: 0x%X", cfg->ep_addr);
		return -EINVAL;
	}

	if (!cfg->ep_mps) {
		LOG_DBG("Wrong EP or Descriptor");
		return -EINVAL;
	}

	udata0.ep_data[ep_idx].ep_status = EP_CONFIG;
	udata0.ep_data[ep_idx].mps = cfg->ep_mps;

	LOG_DBG("ep_status: %d, mps: %d",
		udata0.ep_data[ep_idx].ep_status, udata0.ep_data[ep_idx].mps);

	if (!(ep_idx > EP0)) {
		return 0;
	}

	it82xx2_usb_set_ep_ctrl(ep_idx, EP_IN_DIRECTION_SET, in);

	if (in) {
		it82xx2_usb_set_ep_ctrl(ep_idx, EP_DATA_SEQ_1, false);
		udata0.ep_data[ep_idx].ep_status = EP_CONFIG_IN;
	} else {
		udata0.ep_data[ep_idx].ep_status = EP_CONFIG_OUT;
		it82xx2_usb_fifo_ctrl(cfg->ep_addr, false);
	}

	switch (cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		return -EINVAL;
	case USB_DC_EP_ISOCHRONOUS:
		it82xx2_usb_set_ep_ctrl(ep_idx, EP_IOS_ENABLE, true);
		break;
	case USB_DC_EP_BULK:
		__fallthrough;
	case USB_DC_EP_INTERRUPT:
		__fallthrough;
	default:
		it82xx2_usb_set_ep_ctrl(ep_idx, EP_IOS_ENABLE, false);
		break;
	}

	udata0.ep_data[ep_idx].ep_type = cfg->ep_type;

	LOG_DBG("EP%d Configured: 0x%2X(%d)", ep_idx, !!(in), cfg->ep_type);
	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d)Not attached / Invalid endpoint: EP 0x%x",
			__LINE__, ep);
		return -EINVAL;
	}

	if (cb == NULL) {
		LOG_ERR("(%d): NO callback function", __LINE__);
		return -EINVAL;
	}

	LOG_DBG("EP%d set callback: %d", ep_idx, !!(ep & USB_EP_DIR_IN));

	if (USB_EP_DIR_IS_IN(ep)) {
		udata0.ep_data[ep_idx].cb_in = cb;
	} else {
		udata0.ep_data[ep_idx].cb_out = cb;
	}

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int ret = 0;

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep_idx);
		return -EINVAL;
	}

	if (IT8XXX2_IS_EXTEND_ENDPOINT(ep_idx)) {
		uint8_t ep_fifo = ep_fifo_res[ep_idx % FIFO_NUM];

		it82xx2_usb_set_ep_ctrl(ep_fifo, EP_ENABLE, true);
	}

	ret = it82xx2_usb_set_ep_ctrl(ep_idx, EP_ENABLE, true);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Endpoint 0x%02x is enabled", ep);

	return 0;
}

int usb_dc_ep_disable(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep_idx);
		return -EINVAL;
	}

	return it82xx2_usb_set_ep_ctrl(ep_idx, EP_ENABLE, false);
}


int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	it82xx2_usb_set_ep_ctrl(ep_idx, EP_STALL_SEND, true);

	if (ep_idx == 0) {
		uint32_t idx = 0;

		it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
		/* polling if stall send for 3ms */
		while (idx < 198 &&
			!(ep_regs[ep_idx].ep_status & DC_STALL_SENT)) {
			/* wait 15.15us */
			gctrl_regs->GCTRL_WNCKR = 0;
			idx++;
		}

		if (idx < 198) {
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_STALL_SEND, false);
		}

		udata0.no_data_ctrl = false;
		udata0.st_state = STALL_SEND;
	}

	LOG_DBG("EP(%d) ctrl: 0x%02x", ep_idx, ep_regs[ep_idx].ep_ctrl.value);
	LOG_DBG("EP(%d) Set Stall", ep_idx);

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	it82xx2_usb_set_ep_ctrl(ep_idx, EP_STALL_SEND, false);
	LOG_DBG("EP(%d) clear stall", ep_idx);

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *stalled)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if ((!stalled) || (ep_idx >= MAX_NUM_ENDPOINTS)) {
		return -EINVAL;
	}

	*stalled = it82xx2_usb_set_ep_ctrl(ep_idx, EP_STALL_CHECK, true);

	return 0;
}

int usb_dc_ep_halt(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_flush(uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		ff_regs[ep_fifo].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
	} else {
		ff_regs[ep_fifo].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
	}

	return 0;
}

int usb_dc_ep_write(uint8_t ep, const uint8_t *buf,
				uint32_t data_len, uint32_t *ret_bytes)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	unsigned int key;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	if (ep_idx == EP0) {
		if ((udata0.now_token == SETUP_TOKEN) && (data_len == 0)) {
			return 0;
		}
		/* clear fifo before write*/
		ff_regs[ep_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;

		if (udata0.st_state == SETUP_ST) {
			udata0.st_state = DIN_ST;
		}
	} else {
		k_sem_take(&udata0.fifo_sem[ep_fifo - 1], K_FOREVER);
		key = irq_lock();
		it82xx2_usb_fifo_ctrl(ep, false);
	}

	if (data_len > udata0.ep_data[ep_idx].mps) {

		for (uint32_t idx = 0; idx < udata0.ep_data[ep_idx].mps; idx++) {
			ff_regs[ep_fifo].ep_tx_fifo_data = buf[idx];
		}

		*ret_bytes = udata0.ep_data[ep_idx].mps;
		udata0.ep_data[ep_idx].remaining =
			data_len - udata0.ep_data[ep_idx].mps;

		LOG_DBG("data_len: %d, Write Max Packets to TX FIFO(%d)",
			data_len, ep_idx);
	} else {
		for (uint32_t idx = 0; idx < data_len; idx++) {
			ff_regs[ep_fifo].ep_tx_fifo_data = buf[idx];
		}

		*ret_bytes = data_len;
		udata0.ep_data[ep_idx].remaining = 0;
		LOG_DBG("Write %d Packets to TX FIFO(%d)", data_len, ep_idx);
	}

	it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
	if (ep_idx != 0) {
		irq_unlock(key);
	}

	LOG_DBG("Set EP%d Ready(%d)", ep_idx, __LINE__);

	return 0;
}

/* Read data from an OUT endpoint */
int usb_dc_ep_read(uint8_t ep, uint8_t *buf, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;
	uint16_t rx_fifo_len;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	if (ep_regs[ep_fifo].ep_status & EP_STATUS_ERROR) {
		LOG_WRN("fifo_%d error status: 0x%02x", ep_fifo, ep_regs[ep_fifo].ep_status);
	}

	rx_fifo_len = (uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_lsb +
		      (((uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_msb) << 8);

	if (!buf && !max_data_len) {
		/*
		 * When both buffer and max data to read are zero return
		 * the available data length in buffer.
		 */
		if (read_bytes) {
			*read_bytes = rx_fifo_len;
		}

		if (ep_idx > 0 && !rx_fifo_len) {
			udata0.fifo_ready[ep_fifo - 1] = true;
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
		}
		return 0;
	}

	if (ep_idx == 0) {
		/* Prevent wrong read_bytes cause memory error
		 * if EP0 is in OUT status stage
		 */
		if (udata0.st_state == STATUS_ST) {
			*read_bytes = 0;
			return 0;
		} else if (udata0.now_token == SETUP_TOKEN) {
			if (rx_fifo_len == 0) {
				LOG_ERR("Setup length 0, reset to 8");
				rx_fifo_len = 8;
			}
			if (rx_fifo_len != 8) {
				LOG_ERR("Setup length: %d", rx_fifo_len);
				ff_regs[0].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
				return -EIO;
			}
		}
	}

	if (rx_fifo_len > max_data_len) {
		*read_bytes = max_data_len;
		for (uint32_t idx = 0; idx < max_data_len; idx++) {
			buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
		}

		LOG_DBG("Read Max (%d) Packets", max_data_len);
	} else {

		*read_bytes = rx_fifo_len;

		for (uint32_t idx = 0; idx < rx_fifo_len; idx++) {
			buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
		}

		if (ep_fifo == 0 &&
			udata0.now_token == SETUP_TOKEN) {
			LOG_DBG("RX buf: (%x)(%x)(%x)(%x)(%x)(%x)(%x)(%x)",
			buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], buf[6], buf[7]);
		}

		if (ep_fifo > EP0) {
			udata0.fifo_ready[ep_fifo - 1] = true;
			it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
		} else if (udata0.now_token == SETUP_TOKEN) {
			if (!(buf[0] & USB_EP_DIR_MASK)) {
				/* request type: host-to-device transfer direction */
				ff_regs[0].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
				if (buf[6] != 0 || buf[7] != 0) {
					/* set status IN after data OUT */
					it82xx2_usb_set_ep_ctrl(ep_idx, EP_DATA_SEQ_1, true);
					it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
				} else {
					/* no_data_ctrl status */
					udata0.no_data_ctrl = true;
				}
			}
		}
	}

	return 0;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *buf, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;
	uint16_t rx_fifo_len;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d): Wrong Endpoint Index/Address", __LINE__);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Wrong Endpoint Direction");
		return -EINVAL;
	}

	if (ep_regs[ep_fifo].ep_status & EP_STATUS_ERROR) {
		LOG_WRN("fifo_%d error status(%02x)", ep_fifo, ep_regs[ep_fifo].ep_status);
	}

	rx_fifo_len = (uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_lsb +
		(((uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_msb) << 8);

	LOG_DBG("ep_read_wait (EP: %d), len: %d", ep_idx, rx_fifo_len);

	*read_bytes = (rx_fifo_len > max_data_len) ?
		max_data_len : rx_fifo_len;

	for (uint32_t idx = 0; idx < *read_bytes; idx++) {
		buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
	}

	LOG_DBG("Read %d packets", *read_bytes);

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_fifo = (ep_idx > 0) ? (ep_fifo_res[ep_idx % FIFO_NUM]) : 0;

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d): Wrong Endpoint Index/Address", __LINE__);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Wrong Endpoint Direction");
		return -EINVAL;
	}

	udata0.fifo_ready[ep_fifo - 1] = true;
	it82xx2_usb_set_ep_ctrl(ep_idx, EP_READY_ENABLE, true);
	LOG_DBG("EP(%d) Read Continue", ep_idx);
	return 0;
}


int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d): Wrong Endpoint Index/Address", __LINE__);
		return -EINVAL;
	}
	/* Not configured, return length 0 */
	if (udata0.ep_data[ep_idx].ep_status < EP_CONFIG) {
		LOG_WRN("(%d)EP not set", __LINE__);
		return 0;
	}

	return udata0.ep_data[ep_idx].mps;
}

int usb_dc_wakeup_request(void)
{
	int ret;
	struct usb_it82xx2_regs *const usb_regs = it82xx2_get_usb_regs();

	if (udata0.suspended) {

		usb_regs->dc_control =
			DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
			DC_FULL_SPEED_LINE_RATE | DC_DIRECT_CONTROL |
			DC_TX_LINE_STATE_DM | DC_CONNECT_TO_HOST;

		/* The remote wakeup device must hold the resume signal for */
		/* at least 1 ms but for no more than 15 ms                 */
		k_msleep(2);

		usb_regs->dc_control =
			DC_GLOBAL_ENABLE | DC_FULL_SPEED_LINE_POLARITY |
			DC_FULL_SPEED_LINE_RATE | DC_CONNECT_TO_HOST;

		ret = k_sem_take(&udata0.suspended_sem, K_MSEC(500));
		if (ret < 0) {
			LOG_ERR("failed to wake up host");
		}
	}
	return 0;
}

static int it82xx2_usb_dc_init(const struct device *dev)
{
	const struct usb_it82xx2_config *cfg = dev->config;

	int status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (status < 0) {
		LOG_ERR("Failed to configure USB pins");
		return status;
	}

	/* Initializing WU90 (USB D+) */
	it8xxx2_usb_dc_wuc_init(dev);

	udata0.dev = dev;

	return 0;
}

DEVICE_DT_INST_DEFINE(0,
	&it82xx2_usb_dc_init,
	NULL,
	&udata0,
	&ucfg0,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	NULL);
