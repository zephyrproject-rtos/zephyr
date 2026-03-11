/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT elan_em32_usbd

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>

#include <../drivers/usb/udc/udc_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_e967, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define USB_NUM_BIDIR_ENDPOINTS 5
#define EP0_MPS                 8
#define EP_MPS                  64

#define USBD_BASE     DT_REG_ADDR(DT_NODELABEL(usbd))
#define SYS_CTRL_BASE DT_REG_ADDR(DT_NODELABEL(sysctrl))
#define CLK_CTRL_BASE DT_REG_ADDR(DT_NODELABEL(clkctrl))


/*
 * Customer specific feature
 *  This feature is applied to specific customer.
 *  Do not enable CUSTOMER_SPECIFIC_FEATURE_ENABLE, 
 *  otherwise the testusb test will not be possible.
 */
#define CUSTOMER_SPECIFIC_FEATURE_ENABLE 0

enum udc_e967_msg_type {
	UDC_E967_MSG_TYPE_SETUP,
	UDC_E967_MSG_TYPE_XFER,
};

struct udc_e967_msg {
	enum udc_e967_msg_type type;
	union {
		uint32_t setup_ref;
		uint8_t xfer_ep;
	};
};

struct udc_e967_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	uint32_t ep_cfg_out_size;
	uint32_t ep_cfg_in_size;
	int speed_idx;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

struct e967_usbd_ep {
	uint8_t idx;
	uint32_t data_size_in;
	uint32_t data_size_out;
	struct epx_int_en *reg_ep_int_en;
	struct udc_epx_int_sta *reg_ep_int_sta;
	volatile uint32_t *reg_data_cnt;
	volatile uint32_t *reg_data_buf;
};

struct e967_ctrl_regs {
	struct usb_ctrl *reg_udc_ctrl;
	struct udc_ctrl1 *reg_udc_ctrl1;
	struct udc_int_en *reg_udc_int_en;
	struct udc_int_sta *reg_udc_int_sta;
	struct e967_phy_ctrl *reg_usb_phy;
};

struct udc_e967_data {
	uint8_t setup_pkg[8];
	const struct device *dev;
	uint8_t addr;
	struct k_msgq *msgq;
	struct k_thread thread_data;
	uint8_t ep_out_num;
	uint8_t ep_out_num_new;
	struct ep0_int_en *reg_ep0_int_en;
	struct udc_ep0_int_sta *reg_ep0_int_sts;
	uint32_t ep0_out_size;
	uint32_t ep0_in_size;
	uint32_t ep0_cur_ref;
	uint32_t ep0_proc_ref;
	uint32_t is_addressed_state;
	uint32_t is_configured_state;
	uint32_t is_proc_remote_wakeup;
	volatile uint32_t *reg_ep0_data_buf;
	struct e967_usbd_ep epx_ctrl[USB_NUM_BIDIR_ENDPOINTS - 1];
	struct e967_ctrl_regs regs;
};

static struct e967_usbd_ep *e967_get_ep(struct udc_e967_data *priv, uint8_t ep_addr)
{
	uint8_t ep_idx;

	ep_idx = USB_EP_GET_IDX(ep_addr);

	if (ep_idx >= USB_NUM_BIDIR_ENDPOINTS || ep_idx == 0) {
		return NULL;
	}

	return &priv->epx_ctrl[ep_idx - 1];
}

static inline void e967_usbd_sw_disconnect(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	priv->regs.reg_usb_phy->usb_phy_rsw = 0;
}

static inline void e967_usbd_sw_connect(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	priv->regs.reg_usb_phy->usb_phy_rsw = 1;
}

static void e967_phy_setup(struct udc_e967_data *priv)
{
	priv->regs.reg_udc_ctrl->bits.udc_en = 1;
	while (priv->regs.reg_udc_ctrl->bits.udc_rst_rdy == 0) {
	}

	priv->regs.reg_usb_phy->usb_phy_rsw = 0;
	e967_usb_configure_ep();
}

static void e967_usb_init(struct udc_e967_data *priv)
{
	struct phy_test *reg_phy_test = (struct phy_test *)(USBD_BASE + 0x6C);

	e967_phy_setup(priv);

	priv->regs.reg_udc_int_en->bits.rst_int_en = 1;
	priv->regs.reg_udc_int_en->bits.suspend_int_en = 1;
	priv->regs.reg_udc_int_en->bits.resume_int_en = 1;

	priv->reg_ep0_int_en->bits.setup_int_en = 1;
	priv->reg_ep0_int_en->bits.in_int_en = 1;
	priv->reg_ep0_int_en->bits.out_int_en = 1;

	priv->regs.reg_udc_ctrl->bits.ep1_en = 0;
	priv->regs.reg_udc_ctrl->bits.ep2_en = 0;
	priv->regs.reg_udc_ctrl->bits.ep3_en = 0;
	priv->regs.reg_udc_ctrl->bits.ep4_en = 0;

	reg_phy_test->bits.usb_wakeup_en = 1;
	atrim_clk_disable();
}

static void e967_epx_init(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct e967_usbd_ep *pepx;

	pepx = e967_get_ep(priv, 1);
	if (pepx == NULL) {
		return;
	}

	pepx->idx = 1;
	pepx->data_size_in = 0;
	pepx->data_size_out = 0;
	pepx->reg_ep_int_en = (struct epx_int_en *)(USBD_BASE + 0x10);
	pepx->reg_ep_int_sta = (struct udc_epx_int_sta *)(USBD_BASE + 0x28);
	pepx->reg_data_cnt = (volatile uint32_t *)(USBD_BASE + 0x50);
	pepx->reg_data_buf = (volatile uint32_t *)(USBD_BASE + 0x3C);

	pepx = e967_get_ep(priv, 2);
	if (pepx == NULL) {
		return;
	}

	pepx->idx = 2;
	pepx->data_size_in = 0;
	pepx->data_size_out = 0;
	pepx->reg_ep_int_en = (struct epx_int_en *)(USBD_BASE + 0x14);
	pepx->reg_ep_int_sta = (struct udc_epx_int_sta *)(USBD_BASE + 0x2C);
	pepx->reg_data_cnt = (volatile uint32_t *)(USBD_BASE + 0x54);
	pepx->reg_data_buf = (volatile uint32_t *)(USBD_BASE + 0x40);

	pepx = e967_get_ep(priv, 3);
	if (pepx == NULL) {
		return;
	}

	pepx->idx = 3;
	pepx->data_size_in = 0;
	pepx->data_size_out = 0;
	pepx->reg_ep_int_en = (struct epx_int_en *)(USBD_BASE + 0x18);
	pepx->reg_ep_int_sta = (struct udc_epx_int_sta *)(USBD_BASE + 0x30);
	pepx->reg_data_cnt = (volatile uint32_t *)(USBD_BASE + 0x58);
	pepx->reg_data_buf = (volatile uint32_t *)(USBD_BASE + 0x44);

	pepx = e967_get_ep(priv, 4);
	if (pepx == NULL) {
		return;
	}

	pepx->idx = 4;
	pepx->data_size_in = 0;
	pepx->data_size_out = 0;
	pepx->reg_ep_int_en = (struct epx_int_en *)(USBD_BASE + 0x1C);
	pepx->reg_ep_int_sta = (struct udc_epx_int_sta *)(USBD_BASE + 0x34);
	pepx->reg_data_cnt = (volatile uint32_t *)(USBD_BASE + 0x5C);
	pepx->reg_data_buf = (volatile uint32_t *)(USBD_BASE + 0x48);
}

static int udc_e967_send_msg(const struct device *dev, const struct udc_e967_msg *msg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	int err;

	err = k_msgq_put(priv->msgq, msg, K_NO_WAIT);
	if (err < 0) {
		k_msgq_purge(priv->msgq);
	}

	return err;
}

#if (CUSTOMER_SPECIFIC_FEATURE_ENABLE)
static void get_out_pipe_num(const struct device *dev, struct net_buf *buf)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	uint8_t *ptr;
	uint8_t *end_ptr;
	uint32_t len;

	ptr = buf->data;
	len = buf->len;

	if (len <= 9) {
		return;
	}

	if (ptr[0] == 0x09 && ptr[1] == 0x02) {
		end_ptr = ptr + len;
		ptr = ptr + ptr[0];
		while (ptr < end_ptr) {
			if (ptr[0] == 7 && ptr[1] == 5 && (ptr[2] & 0x80) == 0) {
				priv->ep_out_num = ptr[2];
				priv->ep_out_num_new = 3;
				ptr[2] = priv->ep_out_num_new;
			}
			ptr = ptr + ptr[0];
		}
	}
}
#endif

static int udc_e967_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			       struct net_buf *buf)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *new_cfg;
	unsigned int lock_key;
	struct udc_e967_msg msg = {0};
	uint32_t is_halted;
	uint8_t ep = cfg->addr;

#if (CUSTOMER_SPECIFIC_FEATURE_ENABLE)
	if (ep == USB_CONTROL_EP_IN) {
		get_out_pipe_num(dev, buf);
	}
#endif

	if ((priv->ep_out_num != 0) && (ep == priv->ep_out_num)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_out_num_new);
		udc_buf_put(new_cfg, buf);
		ep = priv->ep_out_num_new;
	} else {
		udc_buf_put(cfg, buf);
	}

	lock_key = irq_lock();
	is_halted = cfg->stat.halted;
	irq_unlock(lock_key);

	if (!is_halted) {
		msg.type = UDC_E967_MSG_TYPE_XFER;
		msg.xfer_ep = ep;
		udc_e967_send_msg(dev, &msg);
	}

	return 0;
}

static int udc_e967_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
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

static int udc_e967_ep_set_halt_impl(struct udc_e967_data *priv, struct udc_ep_config *const cfg,
				     bool is_halt)
{
	uint8_t ep_idx;

	ep_idx = USB_EP_GET_IDX(cfg->addr);
	cfg->stat.halted = is_halt;

	if (ep_idx > (USB_NUM_BIDIR_ENDPOINTS - 1)) {
		return -EINVAL;
	}

	if (is_halt) {
		if (ep_idx == 0) {
			priv->regs.reg_udc_ctrl1->bits.stall = 1;
		} else if (ep_idx == 1) {
			priv->regs.reg_udc_ctrl1->bits.ep1_stall = 1;
		} else if (ep_idx == 2) {
			priv->regs.reg_udc_ctrl1->bits.ep2_stall = 1;
		} else if (ep_idx == 3) {
			priv->regs.reg_udc_ctrl1->bits.ep3_stall = 1;
		} else {
			priv->regs.reg_udc_ctrl1->bits.ep4_stall = 1;
		}
	} else {
		if (ep_idx == 0) {
			priv->regs.reg_udc_ctrl1->bits.stall = 0;
		} else if (ep_idx == 1) {
			priv->regs.reg_udc_ctrl1->bits.ep1_stall = 0;
		} else if (ep_idx == 2) {
			priv->regs.reg_udc_ctrl1->bits.ep2_stall = 0;
		} else if (ep_idx == 3) {
			priv->regs.reg_udc_ctrl1->bits.ep3_stall = 0;
		} else {
			priv->regs.reg_udc_ctrl1->bits.ep4_stall = 0;
		}
	}

	return 0;
}

static int udc_e967_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	int err;

	err = udc_e967_ep_set_halt_impl(priv, cfg, true);

	return err;
}

static int udc_e967_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	int err;

	err = udc_e967_ep_set_halt_impl(priv, cfg, false);

	return err;
}

static int udc_e967_host_wakeup(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	priv->regs.reg_udc_ctrl1->bits.dev_resume = 1;
	k_busy_wait(10000);
	priv->regs.reg_udc_ctrl1->bits.dev_resume = 0;

	return 0;
}

static int usbd_ctrl_feed_dout(const struct device *dev, struct net_buf *setup_pkg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi;
	struct net_buf *data_buf;
	struct net_buf *st_buf;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len, i, bRead;
	uint32_t length = udc_data_stage_length(setup_pkg);

	data_buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (data_buf == NULL) {
		return -ENOMEM;
	}

	net_buf_frag_add(setup_pkg, data_buf);
	bi = udc_get_buf_info(data_buf);
	bi->data = true;

	st_buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_IN, 0);
	if (data_buf == NULL) {
		return -ENOMEM;
	}
	net_buf_frag_add(data_buf, st_buf);
	bi = udc_get_buf_info(st_buf);
	bi->status = true;

	bRead = 0;
	do {
		if (priv->ep0_proc_ref != priv->ep0_cur_ref) {
			goto _error;
		}

		data_ptr = net_buf_tail(data_buf);
		data_len = net_buf_tailroom(data_buf);
		if (data_len == 0) {
			break;
		}
		if (priv->reg_ep0_int_sts->bits.ep0_out_int_sf) {
			priv->reg_ep0_int_sts->bits.ep0_out_int_sf_clr = 1;
			priv->ep0_out_size = 0;

			len = EP0_MPS;
			if (len > data_len) {
				len = data_len;
			}

			for (i = 0; i < len; i++) {
				*data_ptr = *(priv->reg_ep0_data_buf);
			}

			net_buf_add(data_buf, len);
			bRead = bRead + len;
		}
	} while (1);

	udc_submit_ep_event(dev, setup_pkg, 0);

	return 0;

_error:
	net_buf_unref(setup_pkg);
	net_buf_unref(data_buf);
	net_buf_unref(st_buf);
	return -1;
}

static const uint8_t set_address_cmd[8] = {0x0, 0x5, 0x0f, 0x0, 0x0, 0x0, 0x0, 0x0};

static void update_address_event(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	uint8_t *setup_pkg;
	struct net_buf *buf;

	if (priv->is_addressed_state == 0) {
		if (*((uint32_t *)(priv->setup_pkg)) == 0x01000680 &&
		    *((uint16_t *)(priv->setup_pkg + 6)) > 8) {

			priv->reg_ep0_int_en->bits.setup_int_en = 0;
			buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
			udc_ep_buf_set_setup(buf);
			setup_pkg = net_buf_tail(buf);

			for (int i = 0; i < 8; i++) {
				setup_pkg[i] = set_address_cmd[i];
			}

			net_buf_add(buf, 8);
			udc_ctrl_update_stage(dev, buf);

			if (udc_ctrl_stage_is_data_in(dev)) {
				udc_ctrl_submit_s_in_status(dev);
			} else {
				udc_ctrl_submit_s_status(dev);
			}

			priv->is_addressed_state = 1;
			priv->reg_ep0_int_en->bits.setup_int_en = 1;
		}
	}
}

static const uint8_t set_configuration_cmd[8] = {0x0, 0x9, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};

static void update_configured_event(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	uint8_t *setup_pkg;
	struct net_buf *buf;

	if (priv->is_configured_state == 0) {
		setup_pkg = priv->setup_pkg;
		if (*((uint32_t *)(priv->setup_pkg)) == 0x02000680 &&
		    *((uint16_t *)(priv->setup_pkg + 6)) > 9) {
			priv->is_configured_state = 1;
		}
	} else if (priv->is_configured_state == 1) {
		priv->reg_ep0_int_en->bits.setup_int_en = 0;
		buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
		udc_ep_buf_set_setup(buf);
		setup_pkg = net_buf_tail(buf);

		for (int i = 0; i < 8; i++) {
			setup_pkg[i] = set_configuration_cmd[i];
		}

		net_buf_add(buf, 8);
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_data_in(dev)) {
			udc_ctrl_submit_s_in_status(dev);
		} else {
			udc_ctrl_submit_s_status(dev);
		}

		priv->is_configured_state = 2;
		priv->reg_ep0_int_en->bits.setup_int_en = 1;
	} else {
		return;
	}
}

static const uint8_t set_remote_wakeup_cmd[8] = {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};

static int handle_set_feature_remote_wakeup(const struct device *dev, uint32_t is_set)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_e967_msg msg;

	if (priv->is_configured_state != 3) {
		return 0;
	}

	priv->ep0_in_size = 0;
	priv->ep0_out_size = 0;

	for (int i = 0; i < 8; i++) {
		priv->setup_pkg[i] = set_remote_wakeup_cmd[i];
	}

	if (is_set) {
		priv->is_proc_remote_wakeup = 1;
		priv->setup_pkg[1] = 0x03;
	} else {
		priv->is_proc_remote_wakeup = 2;
		priv->setup_pkg[1] = 0x01;
	}

	priv->ep0_cur_ref++;
	msg.type = UDC_E967_MSG_TYPE_SETUP;
	msg.setup_ref = priv->ep0_cur_ref;

	k_msgq_put(priv->msgq, &msg, K_NO_WAIT);

	return 1;
}

static int udc_e967_msg_handler_setup(const struct device *dev, struct udc_e967_msg *msg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct net_buf *setup_pkg;
	uint8_t *data_ptr;
	struct udc_ep_config *ep_ctrl_in;
	struct udc_ep_config *ep_ctrl_out;
	int i;
	int err = 0;

	update_address_event(dev);
	update_configured_event(dev);

	setup_pkg = NULL;
	priv->ep0_proc_ref = msg->setup_ref;

	ep_ctrl_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	ep_ctrl_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);

	udc_ep_set_busy(ep_ctrl_in, false);
	udc_ep_set_busy(ep_ctrl_out, false);

	udc_e967_ep_set_halt_impl(priv, ep_ctrl_in, false);
	udc_e967_ep_set_halt_impl(priv, ep_ctrl_out, false);

	setup_pkg = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	udc_ep_buf_set_setup(setup_pkg);
	data_ptr = net_buf_tail(setup_pkg);

	for (i = 0; i < 8; i++) {
		*(data_ptr + i) = priv->setup_pkg[i];
	}
	net_buf_add(setup_pkg, 8);

	udc_ctrl_update_stage(dev, setup_pkg);

	if (udc_ctrl_stage_is_data_out(dev)) {
		usbd_ctrl_feed_dout(dev, setup_pkg);
		return 0;
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static int usbd_ctrl_out(const struct device *dev, uint8_t ep)
{
	return 0;
}

static int usbd_ctrl_in(const struct device *dev, uint8_t ep)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	buf = udc_buf_peek(ep_cfg);
	bi = udc_get_buf_info(buf);

	if (bi->status) {
		if (priv->is_addressed_state == 1) {
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			priv->is_addressed_state = 2;
			return 0;
		} else if (priv->is_configured_state == 2) {
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			priv->is_configured_state = 3;
			return 0;
		} else if (priv->is_proc_remote_wakeup) {
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			if (priv->is_proc_remote_wakeup == 1) {
				udc_set_suspended(dev, true);
				udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
			}
			priv->is_proc_remote_wakeup = 0;
		} else {
			return 0;
		}
	}

	if (priv->ep0_in_size) {
		priv->ep0_in_size = 0;
	}

	return 0;
}

static int usbd_ctrl_handler(const struct device *dev, uint8_t ep)
{
	int ret;

	if (USB_EP_DIR_IS_OUT(ep)) {
		ret = usbd_ctrl_out(dev, ep);
	} else {
		ret = usbd_ctrl_in(dev, ep);
	}

	return ret;
}

static int e967_usbd_xfer_out(const struct device *dev, uint8_t ep)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct e967_usbd_ep *ep_ctrl;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len, len, i;
	uint32_t is_data_out;
	uint32_t lock_key;

	lock_key = irq_lock();
	ep_ctrl = e967_get_ep(priv, ep);
	ep_cfg = udc_get_ep_cfg(dev, ep);

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		goto exit;
	}

	data_len = net_buf_tailroom(buf);
	data_ptr = net_buf_tail(buf);

	is_data_out = 0;
	if (ep_ctrl->data_size_out) {
		is_data_out = 1;
	}

	if (!is_data_out) {
		goto exit;
	}

	do {
		priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 1;
	} while (priv->regs.reg_udc_ctrl1->bits.ep_in_prehold != 1);

	data_len = net_buf_tailroom(buf);
	data_ptr = net_buf_tail(buf);

	len = *(ep_ctrl->reg_data_cnt);
	len = len >> 16;
	if (len > EP_MPS) {
		len = EP_MPS;
	}
	if (len > data_len) {
		len = data_len;
	}

	for (i = 0; i < len; i++) {
		*data_ptr = *(ep_ctrl->reg_data_buf);
		data_ptr++;
	}

	priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 0;
	net_buf_add(buf, len);

	data_len = net_buf_tailroom(buf);

	if (data_len < EP_MPS) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

	ep_ctrl->data_size_out = 0;

exit:
	irq_unlock(lock_key);
	return 0;
}

static int e967_usbd_xfer_in(const struct device *dev, uint8_t ep)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct e967_usbd_ep *ep_ctrl;

	ep_ctrl = e967_get_ep(priv, ep);
	ep_cfg = udc_get_ep_cfg(dev, ep);

	if (ep_ctrl->data_size_in) {
		ep_ctrl->reg_ep_int_en->bits.epx_in_int_en = 0;
		ep_ctrl->data_size_in = 0;
		ep_ctrl->reg_ep_int_en->bits.epx_in_int_en = 1;
	}

	return 0;
}

static int e967_usbd_msg_handle_xfer(const struct device *dev, struct udc_e967_msg *msg)
{
	uint8_t ep;

	ep = msg->xfer_ep;

	if (USB_EP_GET_IDX(ep) == 0) {
		usbd_ctrl_handler(dev, ep);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		e967_usbd_xfer_out(dev, ep);
	} else {
		e967_usbd_xfer_in(dev, ep);
	}

	return 0;
}

static void e967_usbd_msg_handler(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_e967_msg msg;

	while (true) {
		if (k_msgq_get(priv->msgq, &msg, K_FOREVER)) {
			continue;
		}

		switch (msg.type) {
		case UDC_E967_MSG_TYPE_SETUP:
			udc_e967_msg_handler_setup(dev, &msg);
			break;
		case UDC_E967_MSG_TYPE_XFER:
			e967_usbd_msg_handle_xfer(dev, &msg);
			break;
		default:
			break;
		}
	}
}

static void e967_usb_suspend_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	if (priv->regs.reg_udc_int_sta->bits.suspend_int_sf == 1) {
		priv->regs.reg_udc_int_sta->bits.suspend_int_sf_clr = 1;
	}

	if (handle_set_feature_remote_wakeup(dev, 1)) {
		return;
	}

	udc_set_suspended(dev, true);
	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
}

static void e967_usb_resume_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	if (priv->regs.reg_udc_int_sta->bits.resume_int_sf == 1) {
		priv->regs.reg_udc_int_sta->bits.resume_int_sf_clr = 1;
	}

	udc_set_suspended(dev, false);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);

	handle_set_feature_remote_wakeup(dev, 0);
}

static void e967_usb_reset_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	if (priv->regs.reg_udc_int_sta->bits.rst_int_sf == 1) {
		priv->regs.reg_udc_int_sta->bits.rst_int_sf_clr = 1;
	}

	priv->addr = 0;
	priv->ep0_cur_ref = 0;
	priv->ep0_proc_ref = 0;
	priv->is_addressed_state = 0;
	priv->is_configured_state = 0;
	priv->ep_out_num = 0;
	priv->ep_out_num_new = 0;

	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void e967_usb_setup_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct udc_e967_msg msg;
	uint8_t *ptr;
	struct net_buf *buf;
	uint32_t index;

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	buf = udc_buf_get(ep_cfg);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	priv->ep0_in_size = 0;
	priv->ep0_out_size = 0;

	for (index = 0; index < 8; index++) {
		priv->setup_pkg[index] = (uint8_t)*(priv->reg_ep0_data_buf);
	}

	priv->ep0_cur_ref++;
	ptr = priv->setup_pkg;

	msg.type = UDC_E967_MSG_TYPE_SETUP;
	msg.setup_ref = priv->ep0_cur_ref;
	k_msgq_put(priv->msgq, &msg, K_NO_WAIT);

	priv->reg_ep0_int_sts->bits.setup_int_sf_clr = 1;
}

static void e967_proc_ep0_h2d(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	if (priv->ep0_out_size == 1) {
		return;
	}

	priv->ep0_out_size = 1;
}

static void e967_proc_ep0_d2h(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *nbuf;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len, i;
	struct udc_buf_info *bi;

	if (priv->ep0_in_size) {
		goto exit;
	}

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	nbuf = udc_buf_peek(ep_cfg);

	if (nbuf == NULL) {
		priv->ep0_in_size = 1;
		goto exit;
	}

	data_ptr = nbuf->data;
	data_len = nbuf->len;

	len = EP0_MPS;
	if (len > data_len) {
		len = data_len;
	}

	for (i = 0; i < len; i++) {
		*(priv->reg_ep0_data_buf) = *data_ptr;
		data_ptr++;
	}
	priv->reg_ep0_int_en->bits.data_ready = 1;

	net_buf_pull(nbuf, len);

	bi = udc_get_buf_info(nbuf);
	if (bi->status) {
		udc_submit_ep_event(dev, nbuf, 0);
		goto exit;
	}

	data_len = nbuf->len;
	if (data_len == 0 && len == 0) {
		nbuf = udc_buf_get(ep_cfg);
		net_buf_unref(nbuf);
	}

exit:
	priv->reg_ep0_int_sts->bits.ep0_in_int_sf_clr = 1;
}

static void e967_proc_epx_d2h(const struct device *dev, uint8_t ep_addr)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct e967_usbd_ep *ep_ctrl;
	struct net_buf *nbuf;
	uint8_t *data_ptr;
	uint32_t data_len, len, i;
	int err;

	ep_ctrl = e967_get_ep(priv, ep_addr);
	ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	nbuf = udc_buf_peek(ep_cfg);

	if (ep_ctrl->data_size_in) {
		goto exit;
	}

	if (nbuf == NULL) {
		ep_ctrl->data_size_in = 1;
		ep_ctrl->reg_ep_int_en->bits.epx_in_int_en = 0;
		goto exit;
	}

	data_ptr = nbuf->data;
	data_len = nbuf->len;

	do {
		priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 1;
	} while (priv->regs.reg_udc_ctrl1->bits.ep_in_prehold != 1);

	len = data_len;
	if (len > EP_MPS) {
		len = EP_MPS;
	}

	*(ep_ctrl->reg_data_cnt) = len;
	if (len > 0) {
		for (i = 0; i < len; i++) {
			*(ep_ctrl->reg_data_buf) = *data_ptr;
			data_ptr++;
		}
	}

	ep_ctrl->reg_ep_int_en->bits.epx_data_ready = 1;
	priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 0;

	net_buf_pull(nbuf, len);
	data_len = nbuf->len;

	if (data_len == 0) {
		nbuf = udc_buf_get(ep_cfg);
		err = udc_submit_ep_event(dev, nbuf, 0);
	}

exit:
	ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf_clr = 1;
}

static void e967_usb_ep_d2h_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct e967_usbd_ep *ep_ctrl;

	if (priv->reg_ep0_int_sts->bits.ep0_in_int_sf == 1) {
		e967_proc_ep0_d2h(dev);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_IN | 1);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf == 1) {
		e967_proc_epx_d2h(dev, USB_EP_DIR_IN | 1);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_IN | 2);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf == 1) {
		e967_proc_epx_d2h(dev, USB_EP_DIR_IN | 2);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_IN | 3);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf == 1) {
		e967_proc_epx_d2h(dev, USB_EP_DIR_IN | 3);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_IN | 4);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf == 1) {
		e967_proc_epx_d2h(dev, USB_EP_DIR_IN | 4);
		return;
	}
}

static void e967_proc_epx_h2d(const struct device *dev, uint8_t ep_addr)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct e967_usbd_ep *ep_ctrl;
	struct net_buf *nbuf;
	uint8_t *data_ptr;
	uint32_t data_len, len, i;

	ep_ctrl = e967_get_ep(priv, ep_addr);
	ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	nbuf = udc_buf_peek(ep_cfg);

	ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf_clr = 1;

	if (ep_ctrl->data_size_out) {
		return;
	}

	data_ptr = NULL;
	data_len = 0;

	if (nbuf == NULL) {
		ep_ctrl->data_size_out = 1;
		return;
	}

	data_ptr = net_buf_tail(nbuf);
	data_len = net_buf_tailroom(nbuf);

	if ((data_ptr == NULL) && (data_len != 0)) {
		return;
	}

	len = 0;

	do {
		priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 1;
	} while (priv->regs.reg_udc_ctrl1->bits.ep_in_prehold != 1);

	len = *(ep_ctrl->reg_data_cnt);
	len = len >> 16;
	if (len > EP_MPS) {
		len = EP_MPS;
	}
	if (len > data_len) {
		len = data_len;
	}

	if (len > 0) {
		for (i = 0; i < len; i++) {
			*data_ptr = *(ep_ctrl->reg_data_buf);
			data_ptr++;
		}
	}

	priv->regs.reg_udc_ctrl1->bits.ep_in_prehold = 0;

	net_buf_add(nbuf, len);

	data_ptr = net_buf_tail(nbuf);
	data_len = net_buf_tailroom(nbuf);

	if (data_len < EP_MPS) {
		nbuf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, nbuf, 0);
	}
}

static void e967_usb_ep_h2d_isr(const struct device *dev)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct e967_usbd_ep *ep_ctrl;

	if (priv->reg_ep0_int_sts->bits.ep0_out_int_sf == 1) {
		e967_proc_ep0_h2d(dev);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_OUT | 1);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf == 1) {
		e967_proc_epx_h2d(dev, USB_EP_DIR_OUT | 1);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_OUT | 2);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf == 1) {
		e967_proc_epx_h2d(dev, USB_EP_DIR_OUT | 2);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_OUT | 3);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf == 1) {
		e967_proc_epx_h2d(dev, USB_EP_DIR_OUT | 3);
		return;
	}

	ep_ctrl = e967_get_ep(priv, USB_EP_DIR_OUT | 4);
	if (ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf == 1) {
		e967_proc_epx_h2d(dev, USB_EP_DIR_OUT | 4);
		return;
	}
}

static int udc_e967_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct e967_usbd_ep *ep_ctrl;
	uint32_t lock_key;
	uint8_t ep_dir;
	uint8_t ep_idx;

	ep_ctrl = e967_get_ep(priv, cfg->addr);
	ep_dir = USB_EP_GET_DIR(cfg->addr);
	ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	if (ep_idx > ((USB_NUM_BIDIR_ENDPOINTS - 1))) {
		return -EINVAL;
	}

	if (ep_dir == USB_EP_DIR_IN) {
		lock_key = irq_lock();
		ep_ctrl->data_size_in = 0;
		irq_unlock(lock_key);
		ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf_clr = 1;
		ep_ctrl->reg_ep_int_en->bits.epx_in_int_en = 1;
	} else {
		lock_key = irq_lock();
		ep_ctrl->data_size_out = 0;
		irq_unlock(lock_key);
		ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf_clr = 1;
		ep_ctrl->reg_ep_int_en->bits.epx_out_int_en = 1;
	}

	if (ep_idx == 1) {
		priv->regs.reg_udc_ctrl1->bits.ep1_stall = 0;
		priv->regs.reg_udc_ctrl->bits.ep1_en = 1;
	} else if (ep_idx == 2) {
		priv->regs.reg_udc_ctrl1->bits.ep2_stall = 0;
		priv->regs.reg_udc_ctrl->bits.ep2_en = 1;
	} else if (ep_idx == 3) {
		priv->regs.reg_udc_ctrl1->bits.ep3_stall = 0;
		priv->regs.reg_udc_ctrl->bits.ep3_en = 1;
	} else {
		priv->regs.reg_udc_ctrl1->bits.ep4_stall = 0;
		priv->regs.reg_udc_ctrl->bits.ep4_en = 1;
	}

	return 0;
}

static int udc_e967_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_e967_data *priv = udc_get_private(dev);
	struct e967_usbd_ep *ep_ctrl;
	uint32_t lock_key;
	uint8_t ep_dir;
	uint8_t ep_idx;

	ep_ctrl = e967_get_ep(priv, cfg->addr);
	ep_dir = USB_EP_GET_DIR(cfg->addr);
	ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	if (ep_idx > 4) {
		return -EINVAL;
	}

	if (ep_idx == 1) {
		priv->regs.reg_udc_ctrl->bits.ep1_en = 0;
	} else if (ep_idx == 2) {
		priv->regs.reg_udc_ctrl->bits.ep2_en = 0;
	} else if (ep_idx == 3) {
		priv->regs.reg_udc_ctrl->bits.ep3_en = 0;
	} else {
		priv->regs.reg_udc_ctrl->bits.ep4_en = 0;
	}

	if (ep_dir == USB_EP_DIR_IN) {
		lock_key = irq_lock();
		ep_ctrl->data_size_in = 0;
		irq_unlock(lock_key);
		ep_ctrl->reg_ep_int_en->bits.epx_in_int_en = 0;
		ep_ctrl->reg_ep_int_sta->bits.epx_in_int_sf_clr = 1;
	} else {
		lock_key = irq_lock();
		ep_ctrl->data_size_out = 0;
		irq_unlock(lock_key);
		ep_ctrl->reg_ep_int_en->bits.epx_out_int_en = 0;
		ep_ctrl->reg_ep_int_sta->bits.epx_out_int_sf_clr = 1;
	}

	return 0;
}

static int udc_e967_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_e967_data *priv = udc_get_private(dev);

	priv->addr = addr;

	return 0;
}

static int udc_e967_enable(const struct device *dev)
{
	e967_usbd_sw_connect(dev);

	return 0;
}

static int udc_e967_disable(const struct device *dev)
{
	e967_usbd_sw_disconnect(dev);

	return 0;
}

static void enable_all_ep(const struct device *dev)
{
	struct udc_ep_config *cfg;

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 1);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 2);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 3);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 4);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 1);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 2);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 3);
	udc_e967_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 4);
	udc_e967_ep_enable(dev, cfg);
}

static int udc_e967_init(const struct device *dev)
{
	const struct udc_e967_config *config = dev->config;
	struct udc_e967_data *priv = udc_get_private(dev);

	e967_usb_clock_set(USB_IRC);
	e967_usb_init(priv);

	e967_usbd_sw_disconnect(dev);

	priv->addr = 0;
	priv->ep_out_num = 0;
	priv->ep_out_num_new = 0;

	e967_epx_init(dev);
	enable_all_ep(dev);
	config->irq_enable_func(dev);

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 8, 0)) {
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 8, 0)) {
		return -EIO;
	}

	return 0;
}

static int udc_e967_shutdown(const struct device *dev)
{
	const struct udc_e967_config *config = dev->config;
	struct udc_e967_data *priv = udc_get_private(dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		return -EIO;
	}

	config->irq_disable_func(dev);
	e967_usbd_sw_disconnect(dev);
	priv->regs.reg_usb_phy->usb_phy_pd_b = 0;
	usb_clk_enable();
	k_msgq_purge(priv->msgq);

	return 0;
}

static int udc_e967_driver_preinit(const struct device *dev)
{
	const struct udc_e967_config *config = dev->config;
	struct udc_data *data = dev->data;
	int err;
	int i;

	data->caps.hs = false;
	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_8;
	data->caps.out_ack = true;
	data->caps.can_detect_vbus = false;

	config->ep_cfg_out[0].caps.out = 1;
	config->ep_cfg_out[0].caps.control = 1;
	config->ep_cfg_out[0].caps.mps = 8;
	config->ep_cfg_out[0].addr = USB_EP_DIR_OUT | 0;
	err = udc_register_ep(dev, &config->ep_cfg_out[0]);
	if (err != 0) {
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = 8;
	config->ep_cfg_in[0].addr = USB_EP_DIR_IN | 0;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err != 0) {
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	for (i = 1; i <= (USB_NUM_BIDIR_ENDPOINTS - 1); i++) {
		config->ep_cfg_out[i].caps.out = 1;
		config->ep_cfg_out[i].caps.interrupt = 1;
		config->ep_cfg_out[i].caps.bulk = 1;
		config->ep_cfg_out[i].caps.iso = 1;
		config->ep_cfg_out[i].caps.mps = 1023;
		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (i = 1; i <= (USB_NUM_BIDIR_ENDPOINTS - 1); i++) {
		config->ep_cfg_in[i].caps.in = 1;
		config->ep_cfg_in[i].caps.interrupt = 1;
		config->ep_cfg_in[i].caps.bulk = 1;
		config->ep_cfg_in[i].caps.iso = 1;
		config->ep_cfg_in[i].caps.mps = 1023;
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

static void udc_e967_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_e967_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static enum udc_bus_speed udc_e967_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	if (data->caps.hs) {
		return UDC_BUS_SPEED_HS;
	}

	return UDC_BUS_SPEED_FS;
}

static const struct udc_api udc_e967_api = {
	.device_speed = udc_e967_device_speed,
	.ep_enqueue = udc_e967_ep_enqueue,
	.ep_dequeue = udc_e967_ep_dequeue,
	.ep_set_halt = udc_e967_ep_set_halt,
	.ep_clear_halt = udc_e967_ep_clear_halt,
	.ep_enable = udc_e967_ep_enable,
	.ep_disable = udc_e967_ep_disable,
	.host_wakeup = udc_e967_host_wakeup,
	.set_address = udc_e967_set_address,
	.enable = udc_e967_enable,
	.disable = udc_e967_disable,
	.init = udc_e967_init,
	.shutdown = udc_e967_shutdown,
	.lock = udc_e967_lock,
	.unlock = udc_e967_unlock,
	.test_mode = NULL,
};

#define UDC_E967_DEVICE_DEFINE(inst)                                                               \
	static void udc_e967_irq_enable_func(const struct device *dev)                             \
	{                                                                                          \
		irq_connect_dynamic(E967_USB_SETUP_IRQN, 0,                                        \
				    (void (*)(const void *))e967_usb_setup_isr,                    \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(E967_USB_SUSPEND_IRQN, 0,                                      \
				    (void (*)(const void *))e967_usb_suspend_isr,                  \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(E967_USB_RESUME_IRQN, 0,                                       \
				    (void (*)(const void *))e967_usb_resume_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(E967_USB_RESET_IRQN, 0,                                        \
				    (void (*)(const void *))e967_usb_reset_isr,                    \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(E967_USB_EPX_IN_EPX_EMPTY_IRQN, 0,                             \
				    (void (*)(const void *))e967_usb_ep_d2h_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(E967_USB_EPX_OUT_IRQN, 0,                                      \
				    (void (*)(const void *))e967_usb_ep_h2d_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_enable(E967_USB_SETUP_IRQN);                                                   \
		irq_enable(E967_USB_SUSPEND_IRQN);                                                 \
		irq_enable(E967_USB_RESUME_IRQN);                                                  \
		irq_enable(E967_USB_RESET_IRQN);                                                   \
		irq_enable(E967_USB_EPX_IN_EPX_EMPTY_IRQN);                                        \
		irq_enable(E967_USB_EPX_OUT_IRQN);                                                 \
	}                                                                                          \
                                                                                                   \
	static void udc_e967_irq_disable_func(const struct device *dev)                            \
	{                                                                                          \
		irq_disable(E967_USB_SETUP_IRQN);                                                  \
		irq_disable(E967_USB_SUSPEND_IRQN);                                                \
		irq_disable(E967_USB_RESUME_IRQN);                                                 \
		irq_disable(E967_USB_RESET_IRQN);                                                  \
		irq_disable(E967_USB_EPX_IN_EPX_EMPTY_IRQN);                                       \
		irq_disable(E967_USB_EPX_OUT_IRQN);                                                \
	}                                                                                          \
                                                                                                   \
	K_THREAD_STACK_DEFINE(udc_e967_stack_##inst, CONFIG_UDC_E967_STACK_SIZE);                  \
                                                                                                   \
	static void udc_e967_thread_##inst(void *dev, void *arg1, void *arg2)                      \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		e967_usbd_msg_handler(dev);                                                        \
	}                                                                                          \
                                                                                                   \
	static void udc_e967_make_thread(const struct device *dev)                                 \
	{                                                                                          \
		struct udc_e967_data *priv = udc_get_private(dev);                                 \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_e967_stack_##inst,                         \
				K_THREAD_STACK_SIZEOF(udc_e967_stack_##inst),                      \
				udc_e967_thread_##inst, (void *)dev, NULL, NULL,                   \
				K_PRIO_COOP(CONFIG_UDC_E967_THREAD_PRIORITY), K_ESSENTIAL,         \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out_##inst[USB_NUM_BIDIR_ENDPOINTS];                    \
	static struct udc_ep_config ep_cfg_in_##inst[USB_NUM_BIDIR_ENDPOINTS];                     \
                                                                                                   \
	static const struct udc_e967_config udc_e967_config_##inst = {                             \
		.num_of_eps = USB_NUM_BIDIR_ENDPOINTS,                                             \
		.ep_cfg_in = ep_cfg_in_##inst,                                                     \
		.ep_cfg_out = ep_cfg_out_##inst,                                                   \
		.ep_cfg_out_size = ARRAY_SIZE(ep_cfg_out_##inst),                                  \
		.ep_cfg_in_size = ARRAY_SIZE(ep_cfg_in_##inst),                                    \
		.make_thread = udc_e967_make_thread,                                               \
		.speed_idx = UDC_BUS_SPEED_FS,                                                     \
		.irq_enable_func = udc_e967_irq_enable_func,                                       \
		.irq_disable_func = udc_e967_irq_disable_func};                                    \
                                                                                                   \
	K_MSGQ_DEFINE(e967_usbd_msgq_##inst, sizeof(struct udc_e967_msg),                          \
		      CONFIG_UDC_E967_MSG_QUEUE_SIZE, 4);                                          \
                                                                                                   \
	static struct udc_e967_data e967_udc_priv_##inst = {                                       \
		.setup_pkg = {0},                                                                  \
		.msgq = &e967_usbd_msgq_##inst,                                                    \
		.reg_ep0_data_buf = (volatile uint32_t *)(USBD_BASE + 0x38),                       \
		.reg_ep0_int_sts = (struct udc_ep0_int_sta *)(USBD_BASE + 0x24),                   \
		.reg_ep0_int_en = (struct ep0_int_en *)(USBD_BASE + 0x0C),                         \
		.ep0_out_size = 0,                                                                 \
		.ep0_in_size = 0,                                                                  \
		.ep0_cur_ref = 0,                                                                  \
		.ep0_proc_ref = 0,                                                                 \
		.is_configured_state = 0,                                                          \
		.is_addressed_state = 0,                                                           \
		.is_proc_remote_wakeup = 0,                                                        \
		.regs = {                                                                          \
			.reg_udc_ctrl = (struct usb_ctrl *)(USBD_BASE + 0x00),                     \
			.reg_udc_ctrl1 = (struct udc_ctrl1 *)(USBD_BASE + 0x74),                   \
			.reg_udc_int_en = (struct udc_int_en *)(USBD_BASE + 0x08),                 \
			.reg_udc_int_sta = (struct udc_int_sta *)(USBD_BASE + 0x20),               \
			.reg_usb_phy = (struct e967_phy_ctrl *)(CLK_CTRL_BASE + 0x0700),           \
		}};                                                                                \
                                                                                                   \
	static struct udc_data e967_udc_data_##inst = {                                            \
		.mutex = Z_MUTEX_INITIALIZER(e967_udc_data_##inst.mutex),                          \
		.priv = &e967_udc_priv_##inst,                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, udc_e967_driver_preinit, NULL, &e967_udc_data_##inst,          \
			      &udc_e967_config_##inst, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_e967_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_E967_DEVICE_DEFINE)
