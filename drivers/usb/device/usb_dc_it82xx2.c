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

/* USB Device Controller Registers Bits & Constants */
#define IT8XXX2_USB_IRQ			DT_INST_IRQ_BY_IDX(0, 0, irq)
#define IT8XXX2_WU90_IRQ		DT_INST_IRQ_BY_IDX(0, 1, irq)

#define MAX_NUM_ENDPOINTS		16
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
#define READY_BITS			0x0F
#define COMPLETED_TRANS			0xF0

/* EP Definitions */
#define EP_VALID_MASK			0x0F
#define EP_INVALID_MASK			~(USB_EP_DIR_MASK | EP_VALID_MASK)

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

/* Bit definitions of the register Port0/Port1 MISC Control: 0XE4/0xE8 */
#define PULL_DOWN_EN			BIT(4)

/* Bit definitions of the register EPN0N1_EXTEND_CONTROL_REG: 0X98 ~ 0X9D */
#define EPN1_OUTDATA_SEQ		BIT(4)
#define EPN0_ISO_ENABLE			BIT(2)
#define EPN0_SEND_STALL			BIT(1)
#define EPN0_OUTDATA_SEQ		BIT(0)

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
#define ENDPOINT_EN			BIT(0)
#define ENDPOINT_RDY			BIT(1)
#define EP_OUTDATA_SEQ			BIT(2)
#define EP_SEND_STALL			BIT(3)
#define EP_ISO_ENABLE			BIT(4)
#define EP_DIRECTION			BIT(5)

enum it82xx2_ep_status {
	EP_INIT,
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

enum it82xx2_extend_ep_ctrl {
	/* EPN0N1_EXTEND_CONTROL_REG */
	EXT_EP_ISO_DISABLE,
	EXT_EP_ISO_ENABLE,
	EXT_EP_SEND_STALL,
	EXT_EP_CLEAR_STALL,
	EXT_EP_CHECK_STALL,
	EXT_EP_DATA_SEQ_1,
	EXT_EP_DATA_SEQ_0,
	EXT_EP_DATA_SEQ_INV,
	/* EPN_EXTEND_CONTROL1_REG */
	EXT_EP_DIR_IN,
	EXT_EP_DIR_OUT,
	EXT_EP_ENABLE,
	EXT_EP_DISABLE,
	/* EPN_EXTEND_CONTROL2_REG */
	EXT_EP_READY,
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

	/* Check the EP interrupt status for (ep > 0) */
	bool ep_ready[3];

	struct k_sem ep_sem[3];
	struct k_sem suspended_sem;
	struct k_work_delayable check_suspended_work;
};

/* Mapped to the bit definitions in the EPN_EXTEND_CONTROL1 Register
 * (D6h to DDh) for configuring the FIFO direction and for enabling/disabling
 * the endpoints.
 */
static uint8_t ext_ep_bit_shift[12] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3};

/* The ep_fifo_res[ep_idx % FIFO_NUM] where the FIFO_NUM is 3 represents the
 * EP mapping because when (ep_idx % FIFO_NUM) is 3, it actually means the EP0.
 */
static const uint8_t ep_fifo_res[3] = {3, 1, 2};

/* Mapping the enum it82xx2_extend_ep_ctrl code to their corresponding bit in
 * the EP45/67/89/1011/1213/1415 Extended Control Registers.
 */
static const uint8_t ext_ctrl_tbl[7] = {
	EPN0_ISO_ENABLE,
	EPN0_ISO_ENABLE,
	EPN0_SEND_STALL,
	EPN0_SEND_STALL,
	EPN0_SEND_STALL,
	EPN0_OUTDATA_SEQ,
	EPN0_OUTDATA_SEQ,
};

/* Indexing of the following control codes:
 * EXT_EP_DIR_IN, EXT_EP_DIR_OUT, EXT_EP_ENABLE, EXT_EP_DISABLE
 */
static const uint8_t epn_ext_ctrl_tbl[4] = {1, 0, 1, 0};

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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

	usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
	if (enable) {
		usb_regs->dc_interrupt_mask |= DC_SOF_RECEIVED;
	} else {
		usb_regs->dc_interrupt_mask &= ~DC_SOF_RECEIVED;
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

/* Function it82xx2_get_ep_fifo_ctrl_reg_idx(uint8_t ep_idx):
 *
 * Calculate the register offset index which determines the corresponding
 * EP FIFO Ctrl Registers which is defined as ep_fifo_ctrl[reg_idx] here
 *
 * The ep_fifo_res[ep_idx % FIFO_NUM] represents the EP mapping because when
 * (ep_idx % FIFO_NUM) is 3, it actually means the EP0.
 */
static uint8_t it82xx2_get_ep_fifo_ctrl_reg_idx(uint8_t ep_idx)
{

	uint8_t reg_idx = (ep_idx < EP8) ?
		((ep_fifo_res[ep_idx % FIFO_NUM] - 1) * 2) :
		((ep_fifo_res[ep_idx % FIFO_NUM] - 1) * 2 + 1);

	return reg_idx;
}

/* Function it82xx2_get_ep_fifo_ctrl_reg_val(uint8_t ep_idx):
 *
 * Calculate the register value written to the ep_fifo_ctrl which is defined as
 * ep_fifo_ctrl[reg_idx] here for selecting the corresponding control bit.
 */
static uint8_t it82xx2_get_ep_fifo_ctrl_reg_val(uint8_t ep_idx)
{
	uint8_t reg_val = (ep_idx < EP8) ?
		(1 << ep_idx) : (1 << (ep_idx - EP8));

	return reg_val;
}

/*
 * Functions it82xx2_epn0n1_ext_ctrl_cfg() and epn0n1_ext_ctrl_cfg_seq_inv()
 * provide the entrance of configuring the EPN0N1 Extended Ctrl Registers.
 *
 * The variable set_clr determines if we set/clear the corresponding bit.
 */
static void it82xx2_epn0n1_ext_ctrl_cfg(uint8_t reg_idx, uint8_t bit_mask,
			bool set_clr)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	volatile uint8_t *epn0n1_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl;

	(set_clr) ? (epn0n1_ext_ctrl[reg_idx] |= bit_mask) :
		(epn0n1_ext_ctrl[reg_idx] &= ~(bit_mask));
}

static void it82xx2_epn0n1_ext_ctrl_cfg_seq_inv(uint8_t reg_idx,
			uint8_t bit_mask, bool set_clr)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	volatile uint8_t *epn0n1_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl;

	bool check = (set_clr) ?
		(epn0n1_ext_ctrl[reg_idx] & EPN0_OUTDATA_SEQ) :
		(epn0n1_ext_ctrl[reg_idx] & EPN1_OUTDATA_SEQ);

	(check) ? (epn0n1_ext_ctrl[reg_idx] &= ~(bit_mask)) :
		(epn0n1_ext_ctrl[reg_idx] |= bit_mask);
}

/* Return the status of STALL bit in the EPN0N1 Extend Control Registers */
static bool it82xx2_epn01n1_check_stall(uint8_t reg_idx, uint8_t bit_mask)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	volatile uint8_t *epn0n1_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl;

	return !!(epn0n1_ext_ctrl[reg_idx] & bit_mask);
}

/* Configuring the EPN Extended Ctrl Registers. */
static void it82xx2_epn_ext_ctrl_cfg1(uint8_t reg_idx, uint8_t bit_mask,
			bool set_clr)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	(set_clr) ? (epn_ext_ctrl[reg_idx].epn_ext_ctrl1 |= bit_mask) :
		(epn_ext_ctrl[reg_idx].epn_ext_ctrl1 &= ~(bit_mask));
}

static void it82xx2_epn_ext_ctrl_cfg2(uint8_t reg_idx, uint8_t bit_mask,
			bool set_clr)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	(set_clr) ? (epn_ext_ctrl[reg_idx].epn_ext_ctrl2 |= bit_mask) :
		(epn_ext_ctrl[reg_idx].epn_ext_ctrl2 &= ~(bit_mask));
}

/* From 98h to 9Dh, the EP45/67/89/1011/1213/1415 Extended Control Registers
 * are defined, and their bits definitions are as follows:
 *
 * Bit    Description
 *  7     Reserved
 *  6     EPPOINT5_ISO_ENABLE
 *  5     EPPOINT5_SEND_STALL
 *  4     EPPOINT5_OUT_DATA_SEQUENCE
 *  3     Reserved
 *  2     EPPOINT4_ISO_ENABLE
 *  1     EPPOINT4_SEND_STALL
 *  0     EPPOINT4_OUT_DATA_SEQUENCE
 *
 * Apparently, we can tell that the EP4 and EP5 share the same register, and
 * the EP6 and EP7 share the same one, and the other EPs are defined in the
 * same way.
 *
 * In the function it82xx2_usb_extend_ep_ctrl() we will obtain the mask/flag
 * according to the bits definitions mentioned above. As for the control code,
 * please refer to the definition of enum it82xx2_extend_ep_ctrl.
 */
static int it82xx2_usb_extend_ep_ctrl(uint8_t ep_idx,
			enum it82xx2_extend_ep_ctrl ctrl)
{
	uint8_t reg_idx, mask;
	bool flag;

	if (ep_idx < EP4) {
		return -EINVAL;
	}

	if ((ctrl >= EXT_EP_DIR_IN) && (ctrl < EXT_EP_READY)) {
		/* From EXT_EP_DIR_IN to EXT_EP_DISABLE */
		reg_idx = ep_fifo_res[ep_idx % FIFO_NUM];
		mask = 1 << (ext_ep_bit_shift[ep_idx - 4] * 2 + 1);
		flag = epn_ext_ctrl_tbl[ctrl - EXT_EP_DIR_IN];
		it82xx2_epn_ext_ctrl_cfg1(reg_idx, mask, flag);

	} else if ((ctrl >= EXT_EP_ISO_DISABLE) && (ctrl < EXT_EP_DIR_IN)) {
		/* From EXT_EP_ISO_DISABLE to EXT_EP_DATA_SEQ_0 */
		reg_idx = (ep_idx - 4) >> 1;
		flag = !!(ep_idx & 1);
		mask = flag ? (ext_ctrl_tbl[ctrl] << 4) : (ext_ctrl_tbl[ctrl]);

		if (ctrl == EXT_EP_CHECK_STALL) {
			return it82xx2_epn01n1_check_stall(reg_idx, mask);
		} else if (ctrl == EXT_EP_DATA_SEQ_INV) {
			it82xx2_epn0n1_ext_ctrl_cfg_seq_inv(
				reg_idx, mask, flag);
		} else {
			it82xx2_epn0n1_ext_ctrl_cfg(reg_idx, mask, flag);
		}
	} else if (ctrl == EXT_EP_READY) {
		reg_idx = (ep_idx - 4) >> 1;
		mask = 1 << (ext_ep_bit_shift[ep_idx - 4]);
		it82xx2_epn_ext_ctrl_cfg2(reg_idx, mask, true);
	} else {
		LOG_ERR("Invalid Control Code of Endpoint");
		return -EINVAL;
	}

	return 0;
}

static int it82xx2_usb_dc_ip_init(uint8_t p_action)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

	if (p_action == 0) {
		/* Reset Device Controller */
		usb_regs->host_device_control = RESET_CORE;
		k_msleep(1);
		usb_regs->port0_misc_control &= ~(PULL_DOWN_EN);
		usb_regs->port1_misc_control &= ~(PULL_DOWN_EN);
		/* clear reset bit */
		usb_regs->host_device_control = 0;
	}

	usb_regs->dc_interrupt_status =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED;

	usb_regs->dc_interrupt_mask = 0x00;
	usb_regs->dc_interrupt_mask =
		DC_TRANS_DONE | DC_RESET_EVENT | DC_SOF_RECEIVED;

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

	return it82xx2_usb_dc_ip_init(0);
}

/* Check the condition that SETUP_TOKEN following OUT_TOKEN and return it */
static bool it82xx2_check_setup_following_out(void)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	return ((ep_regs[EP0].ep_transtype_sts & DC_ALL_TRANS) == 0 ||
		(udata0.last_token == IN_TOKEN &&
		ff_regs[EP0].ep_rx_fifo_dcnt_lsb == SETUP_DATA_CNT));
}

static int it82xx2_setup_done(uint8_t ep_ctrl, uint8_t idx)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	LOG_DBG("SETUP(%d)", idx);
	/* wrong trans*/
	if (ep_ctrl & EP_SEND_STALL) {
		ep_regs[idx].ep_ctrl &= ~EP_SEND_STALL;
		udata0.st_state = STALL_SEND;
		ff_regs[idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
		LOG_DBG("Clear Stall Bit & RX FIFO");
		return -EINVAL;
	}
	/* handle status interrupt */
	/* status out */
	if (udata0.st_state == DIN_ST) {
		LOG_DBG("Status OUT");
		udata0.last_token = udata0.now_token;
		udata0.now_token = OUT_TOKEN;
		udata0.st_state = STATUS_ST;
		udata0.ep_data[idx].cb_out(idx, USB_DC_EP_DATA_OUT);

	} else if (udata0.st_state == DOUT_ST || udata0.st_state == SETUP_ST) {
		/* Status IN*/
		LOG_DBG("Status IN");
		udata0.last_token = udata0.now_token;
		udata0.now_token = IN_TOKEN;
		udata0.st_state = STATUS_ST;
		udata0.ep_data[idx].cb_in(idx | 0x80, USB_DC_EP_DATA_IN);
	}

	udata0.last_token = udata0.now_token;
	udata0.now_token = SETUP_TOKEN;
	udata0.st_state = SETUP_ST;

	ep_regs[idx].ep_ctrl |= EP_OUTDATA_SEQ;
	udata0.ep_data[idx].cb_out(USB_EP_DIR_OUT, USB_DC_EP_SETUP);

	/* Set ready bit to no-data control in */
	if (udata0.no_data_ctrl) {
		ep_regs[EP0].ep_ctrl |= ENDPOINT_RDY;
		LOG_DBG("(%d): Set Ready Bit for no-data control", __LINE__);

		udata0.no_data_ctrl = false;
	}

	return 0;
}

static int it82xx2_in_done(uint8_t ep_ctrl, uint8_t idx)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	/* stall send check */
	if (ep_ctrl & EP_SEND_STALL) {
		ep_regs[EP0].ep_ctrl &= ~EP_SEND_STALL;
		udata0.st_state = STALL_SEND;
		LOG_DBG("Clear Stall Bit");
		return -EINVAL;
	}

	if (udata0.st_state >= STATUS_ST) {
		return -EINVAL;
	}

	LOG_DBG("IN(%d)(%d)", idx, !!(ep_regs[idx].ep_ctrl & EP_OUTDATA_SEQ));

	udata0.last_token = udata0.now_token;
	udata0.now_token = IN_TOKEN;

	if (udata0.addr != DC_ADDR_NULL &&
		udata0.addr != usb_regs->dc_address) {
		usb_regs->dc_address = udata0.addr;
		LOG_DBG("Address Is Set Successfully");
	}

	/* set setup stage */
	if (udata0.st_state == DOUT_ST) {
		udata0.st_state = STATUS_ST;
		/* no data status in */
	} else if (udata0.ep_data[EP0].remaining == 0 &&
	udata0.st_state == SETUP_ST) {
		udata0.st_state = STATUS_ST;
	} else {
		udata0.st_state = DIN_ST;
	}

	if (!!(ep_regs[idx].ep_ctrl & EP_OUTDATA_SEQ)) {
		ep_regs[idx].ep_ctrl &= ~EP_OUTDATA_SEQ;
	} else {
		ep_regs[idx].ep_ctrl |= EP_OUTDATA_SEQ;
	}
	udata0.ep_data[idx].cb_in(idx | 0x80, USB_DC_EP_DATA_IN);

	/* set ready bit for status out */
	LOG_DBG("Remaining Bytes: %d, Stage: %d",
	udata0.ep_data[EP0].remaining, udata0.st_state);

	if (udata0.st_state == DIN_ST && udata0.ep_data[EP0].remaining == 0) {
		ep_regs[EP0].ep_ctrl |= ENDPOINT_RDY;
		LOG_DBG("Set EP%d Ready (%d)", idx, __LINE__);
	}

	return 0;
}

static int it82xx2_out_done(uint8_t idx)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	/* ep0 wrong enter check */
	if (udata0.st_state >= STATUS_ST) {
		return -EINVAL;
	}

	LOG_DBG("OUT(%d)", idx);

	udata0.last_token = udata0.now_token;
	udata0.now_token = OUT_TOKEN;

	if (udata0.st_state == SETUP_ST) {
		udata0.st_state = DOUT_ST;
	} else {
		udata0.st_state = STATUS_ST;
	}

	udata0.ep_data[idx].cb_out(idx, USB_DC_EP_DATA_OUT);

	/* SETUP_TOKEN follow OUT_TOKEN */
	if (it82xx2_check_setup_following_out()) {
		LOG_WRN("[%s] OUT => SETUP", __func__);
		udata0.last_token = udata0.now_token;
		udata0.now_token = SETUP_TOKEN;
		udata0.st_state = SETUP_ST;
		ep_regs[EP0].ep_ctrl |= EP_OUTDATA_SEQ;
		udata0.ep_data[EP0].cb_out(USB_EP_DIR_OUT, USB_DC_EP_SETUP);

		/* NOTE: set ready bit to no-data control in */
		if (udata0.no_data_ctrl) {
			ep_regs[EP0].ep_ctrl |= ENDPOINT_RDY;
			LOG_DBG("Set Ready Bit for no-data control");
			udata0.no_data_ctrl = false;
		}
	}

	return 0;
}

/* Functions it82xx2_ep_in_out_config():
 * Dealing with the ep_ctrl configurations in this subroutine when it's
 * invoked in the it82xx2_usb_dc_trans_done().
 */
static void it82xx2_ep_in_out_config(uint8_t idx)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	uint8_t ep_trans = (epn_ext_ctrl[idx].epn_ext_ctrl2 >> 4) & READY_BITS;

	if (udata0.ep_data[ep_trans].ep_status == EP_CONFIG_IN) {
		if (ep_trans < 4) {
			if (!!(ep_regs[idx].ep_ctrl & EP_OUTDATA_SEQ)) {
				ep_regs[idx].ep_ctrl &= ~EP_OUTDATA_SEQ;
			} else {
				ep_regs[idx].ep_ctrl |= EP_OUTDATA_SEQ;
			}
		} else {
			it82xx2_usb_extend_ep_ctrl(ep_trans,
				EXT_EP_DATA_SEQ_INV);
		}

		if (udata0.ep_data[ep_trans].cb_in) {
			udata0.ep_data[ep_trans].cb_in(ep_trans | 0x80,
				USB_DC_EP_DATA_IN);
		}

		k_sem_give(&udata0.ep_sem[idx - 1]);

	} else {
		if (udata0.ep_data[ep_trans].cb_out) {
			udata0.ep_data[ep_trans].cb_out(ep_trans,
				USB_DC_EP_DATA_OUT);
		}
	}
}

static void it82xx2_usb_dc_trans_done(uint8_t ep_ctrl, uint8_t ep_trans_type)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct epn_ext_ctrl_regs *epn_ext_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl;

	int ret;

	for (uint8_t idx = 0 ; idx < EP4 ; idx++) {
		ep_ctrl = ep_regs[idx].ep_ctrl;

		/* check ready bit ,will be 0 when trans done */
		if ((ep_ctrl & ENDPOINT_EN) && !(ep_ctrl & ENDPOINT_RDY)) {
			if (idx == EP0) {
				ep_trans_type = ep_regs[idx].ep_transtype_sts &
					DC_ALL_TRANS;

				/* set up*/
				if (ep_trans_type == DC_SETUP_TRANS) {
					ret = it82xx2_setup_done(ep_ctrl, idx);

					if (ret != 0) {
						continue;
					}
				} else if (ep_trans_type == DC_IN_TRANS) {
					/* in */
					ret = it82xx2_in_done(ep_ctrl, idx);

					if (ret != 0) {
						continue;
					}
				} else if (ep_trans_type == DC_OUTDATA_TRANS) {
					/* out */
					ret = it82xx2_out_done(idx);

					if (ret != 0) {
						continue;
					}
				}
			} else {
				/* prevent wrong entry */
				if (!udata0.ep_ready[idx - 1]) {
					continue;
				}

				if ((epn_ext_ctrl[idx].epn_ext_ctrl2 &
					COMPLETED_TRANS) == 0) {
					continue;
				}

				udata0.ep_ready[idx - 1] = false;
				it82xx2_ep_in_out_config(idx);
			}
		}
	}
}

static void it82xx2_usb_dc_isr(void)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

	uint8_t status = usb_regs->dc_interrupt_status &
		usb_regs->dc_interrupt_mask; /* mask non enable int */
	uint8_t ep_ctrl, ep_trans_type;

	/* reset */
	if (status & DC_RESET_EVENT) {
		if ((usb_regs->dc_line_status & RX_LINE_STATE_MASK) ==
			RX_LINE_RESET) {
			usb_dc_reset();
			usb_regs->dc_interrupt_status = DC_RESET_EVENT;

			if (udata0.usb_status_cb) {
				(*(udata0.usb_status_cb))(USB_DC_RESET, NULL);
			}

			return;

		} else {
			usb_regs->dc_interrupt_status = DC_RESET_EVENT;
		}
	}
	/* sof received */
	if (status & DC_SOF_RECEIVED) {
		it82xx2_enable_sof_int(false);
		k_work_reschedule(&udata0.check_suspended_work, K_MSEC(5));
	}
	/* transaction done */
	if (status & DC_TRANS_DONE) {
		/* clear interrupt before new transaction */
		usb_regs->dc_interrupt_status = DC_TRANS_DONE;
		it82xx2_usb_dc_trans_done(ep_ctrl, ep_trans_type);
		return;
	}

}

static void suspended_check_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct usb_it82xx2_data *udata =
		CONTAINER_OF(dwork, struct usb_it82xx2_data, check_suspended_work);

	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

	if (usb_regs->dc_interrupt_status & DC_SOF_RECEIVED) {
		usb_regs->dc_interrupt_status = DC_SOF_RECEIVED;
		if (udata->suspended) {
			if (udata->usb_status_cb) {
				(*(udata->usb_status_cb))(USB_DC_RESUME, NULL);
			}
			udata->suspended = false;
			k_sem_give(&udata->suspended_sem);
		}
		k_work_reschedule(&udata->check_suspended_work, K_MSEC(5));
		return;
	}

	it82xx2_enable_sof_int(true);

	if (!udata->suspended) {
		if (udata->usb_status_cb) {
			(*(udata->usb_status_cb))(USB_DC_SUSPEND, NULL);
		}
		udata->suspended = true;
		it82xx2_enable_wu90_irq(udata->dev, true);
		it82xx2_enable_standby_state(true);

		k_sem_reset(&udata->suspended_sem);
	}
}

/*
 * USB Device Controller API
 */
int usb_dc_attach(void)
{
	int ret;
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

	if (udata0.attached) {
		LOG_DBG("Already Attached");
		return 0;
	}

	LOG_DBG("Attached");
	ret = it82xx2_usb_dc_attach_init();

	if (ret) {
		return ret;
	}

	for (int idx = 0 ; idx < MAX_NUM_ENDPOINTS ; idx++) {
		udata0.ep_data[idx].ep_status = EP_INIT;
	}

	udata0.attached = 1U;

	/* init ep ready status */
	udata0.ep_ready[0] = false;
	udata0.ep_ready[1] = false;
	udata0.ep_ready[2] = false;

	k_sem_init(&udata0.ep_sem[0], 1, 1);
	k_sem_init(&udata0.ep_sem[1], 1, 1);
	k_sem_init(&udata0.ep_sem[2], 1, 1);
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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	int idx;

	LOG_DBG("USB Device Reset");

	ff_regs[EP0].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
	ff_regs[EP0].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;

	for (idx = 1 ; idx < EP4 ; idx++) {
		if (udata0.ep_data[idx].ep_status > EP_CHECK) {
			ff_regs[idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;
			ff_regs[idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
		}
	}

	ep_regs[EP0].ep_ctrl = ENDPOINT_EN;
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
	uint8_t ep_idx = cfg->ep_addr & EP_VALID_MASK;
	bool in = !!((cfg->ep_addr) & USB_EP_DIR_MASK);

	if ((cfg->ep_addr & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

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

	if (udata0.ep_data[ep_idx].ep_status > 0) {
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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	volatile uint8_t *ep_fifo_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;

	uint8_t ep_idx = (cfg->ep_addr) & EP_VALID_MASK;
	uint8_t reg_idx = it82xx2_get_ep_fifo_ctrl_reg_idx(ep_idx);
	uint8_t reg_val = it82xx2_get_ep_fifo_ctrl_reg_val(ep_idx);
	bool in = !!((cfg->ep_addr) & USB_EP_DIR_MASK);

	if ((cfg->ep_addr & EP_INVALID_MASK) != 0) {
		LOG_DBG("Invalid Address");
		return -EINVAL;
	}

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

	if (ep_idx < EP4) {
		(in) ? (ep_regs[ep_idx].ep_ctrl |= EP_DIRECTION) :
			(ep_regs[ep_idx].ep_ctrl &= ~EP_DIRECTION);

	} else {

		(in) ? (it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_DIR_IN)) :
			(it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_DIR_OUT));

		if (in) {
			it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_DATA_SEQ_0);
		}

		LOG_DBG("ep_status %d", udata0.ep_data[ep_idx].ep_status);
	}

	(in) ? (udata0.ep_data[ep_idx].ep_status = EP_CONFIG_IN) :
		(udata0.ep_data[ep_idx].ep_status = EP_CONFIG_OUT);

	ep_fifo_ctrl[reg_idx] |= reg_val;

	switch (cfg->ep_type) {

	case USB_DC_EP_CONTROL:
		return -EINVAL;

	case USB_DC_EP_ISOCHRONOUS:
		if (ep_idx < EP4) {
			ep_regs[ep_idx].ep_ctrl |= EP_ISO_ENABLE;
		} else {
			it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_ISO_ENABLE);
		}

		break;

	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
	default:
		if (ep_idx < EP4) {
			ep_regs[ep_idx].ep_ctrl &= ~EP_ISO_ENABLE;
		} else {
			it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_ISO_DISABLE);
		}

		break;

	}

	udata0.ep_data[ep_idx].ep_type = cfg->ep_type;

	LOG_DBG("EP%d Configured: 0x%2X(%d)", ep_idx, !!(in), cfg->ep_type);
	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

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

	(ep & USB_EP_DIR_IN) ?
		(udata0.ep_data[ep_idx].cb_in = cb) :
		(udata0.ep_data[ep_idx].cb_out = cb);

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((ep & EP_INVALID_MASK) != 0) {
		LOG_DBG("Bit[6:4] has something invalid");
		return -EINVAL;
	}

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep_idx);
		return -EINVAL;
	}

	if (ep_idx < EP4) {
		LOG_DBG("ep_idx < 4");
		ep_regs[ep_idx].ep_ctrl |= ENDPOINT_EN;
		LOG_DBG("EP%d Enabled %02x", ep_idx, ep_regs[ep_idx].ep_ctrl);
	} else {
		LOG_DBG("ep_idx >= 4");
		it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_ENABLE);
	}

	return 0;
}

int usb_dc_ep_disable(uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

	if (!udata0.attached || ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep_idx);
		return -EINVAL;
	}

	if (ep_idx < EP4) {
		ep_regs[ep_idx].ep_ctrl &= ~ENDPOINT_EN;
	} else {
		it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_DISABLE);
	}

	return 0;
}


int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;

	if (((ep & EP_INVALID_MASK) != 0) || (ep_idx >= MAX_NUM_ENDPOINTS)) {
		return -EINVAL;
	}

	if (ep_idx < EP4) {
		ep_regs[ep_idx].ep_ctrl |= EP_SEND_STALL;
	} else {
		it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_SEND_STALL);
	}

	if (ep_idx == EP0) {
		ep_regs[EP0].ep_ctrl |= ENDPOINT_RDY;
		uint32_t idx = 0;
		/* polling if stall send for 3ms */
		while (idx < 198 &&
			!(ep_regs[EP0].ep_status & DC_STALL_SENT)) {
			/* wait 15.15us */
			gctrl_regs->GCTRL_WNCKR = 0;
			idx++;
		}

		if (idx < 198) {
			ep_regs[EP0].ep_ctrl &= ~EP_SEND_STALL;
		}

		udata0.no_data_ctrl = false;
		udata0.st_state = STALL_SEND;
	}

	LOG_DBG("EP(%d) ctrl: 0x%02x", ep_idx, ep_regs[ep_idx].ep_ctrl);
	LOG_DBG("EP(%d) Set Stall", ep_idx);

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	ep_regs[ep_idx].ep_ctrl &= ~EP_SEND_STALL;
	LOG_DBG("EP(%d) clear stall", ep_idx);

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *stalled)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((!stalled) || ((ep & EP_INVALID_MASK) != 0) ||
		(ep_idx >= MAX_NUM_ENDPOINTS)) {
		return -EINVAL;
	}

	if (ep_idx < EP4) {
		*stalled =
			(0 != (ep_regs[ep_idx].ep_ctrl & EP_SEND_STALL));
	} else {
		*stalled = it82xx2_usb_extend_ep_ctrl(ep_idx,
			EXT_EP_CHECK_STALL);
	}

	return 0;
}

int usb_dc_ep_halt(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_flush(uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	bool in = !!(ep & USB_EP_DIR_MASK);

	if (((ep & EP_INVALID_MASK) != 0) || (ep_idx >= MAX_NUM_ENDPOINTS)) {
		return -EINVAL;
	}

	if (ep_idx > FIFO_NUM) {
		ep_idx = ep_fifo_res[ep_idx % FIFO_NUM];
	}

	in ? (ff_regs[ep_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY) :
		(ff_regs[ep_idx].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY);

	return 0;
}

int usb_dc_ep_write(uint8_t ep, const uint8_t *buf,
				uint32_t data_len, uint32_t *ret_bytes)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;
	volatile uint8_t *ep_fifo_ctrl =
		usb_regs->fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	uint8_t reg_idx = it82xx2_get_ep_fifo_ctrl_reg_idx(ep_idx);
	uint8_t reg_val = it82xx2_get_ep_fifo_ctrl_reg_val(ep_idx);
	uint8_t ep_fifo = (ep_idx > EP0) ?
		(ep_fifo_res[ep_idx % FIFO_NUM]) : 0;
	uint32_t idx;

	if ((ep & EP_INVALID_MASK) != 0)
		return -EINVAL;

	/* status IN */
	if ((ep_idx == EP0) && (data_len == 0) &&
		(udata0.now_token == SETUP_TOKEN)) {
		return 0;
	}

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		return -EINVAL;
	}

	/* clear fifo before write*/
	if (ep_idx == EP0) {
		ff_regs[ep_idx].ep_tx_fifo_ctrl = FIFO_FORCE_EMPTY;
	}

	if ((ep_idx == EP0) && (udata0.st_state == SETUP_ST)) {
		udata0.st_state = DIN_ST;
	}

	/* select FIFO */
	if (ep_idx > EP0) {

		k_sem_take(&udata0.ep_sem[ep_fifo-1], K_FOREVER);

		/* select FIFO */
		ep_fifo_ctrl[reg_idx] |= reg_val;
	}

	if (data_len > udata0.ep_data[ep_idx].mps) {

		for (idx = 0 ; idx < udata0.ep_data[ep_idx].mps ; idx++) {
			ff_regs[ep_fifo].ep_tx_fifo_data = buf[idx];
		}

		*ret_bytes = udata0.ep_data[ep_idx].mps;
		udata0.ep_data[ep_idx].remaining =
			data_len - udata0.ep_data[ep_idx].mps;

		LOG_DBG("data_len: %d, Write Max Packets to TX FIFO(%d)",
			data_len, ep_idx);
	} else {
		for (idx = 0 ; idx < data_len ; idx++) {
			ff_regs[ep_fifo].ep_tx_fifo_data = buf[idx];
		}

		*ret_bytes = data_len;
		udata0.ep_data[ep_idx].remaining = 0;
		LOG_DBG("Write %d Packets to TX FIFO(%d)", data_len, ep_idx);
	}

	if (ep_idx > FIFO_NUM) {
		it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_READY);
	}

	ep_regs[ep_fifo].ep_ctrl |= ENDPOINT_RDY;

	if (ep_fifo > EP0) {
		udata0.ep_ready[ep_fifo - 1] = true;
	}

	LOG_DBG("Set EP%d Ready(%d)", ep_idx, __LINE__);

	return 0;
}

/* Read data from an OUT endpoint */
int usb_dc_ep_read(uint8_t ep, uint8_t *buf, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	uint8_t ep_fifo = 0;
	uint16_t rx_fifo_len;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

	if (ep_regs[ep_idx].ep_status & EP_STATUS_ERROR) {
		LOG_WRN("EP Error Flag: 0x%02x", ep_regs[ep_idx].ep_status);
	}

	if (max_data_len == 0) {

		*read_bytes = 0;

		if (ep_idx > EP0) {
			ep_regs[ep_idx].ep_ctrl |= ENDPOINT_RDY;
			LOG_DBG("Set EP%d Ready(%d)", ep_idx, __LINE__);
		}

		return 0;
	}

	if (ep_idx > EP0) {
		ep_fifo = ep_fifo_res[ep_idx % FIFO_NUM];
	}

	rx_fifo_len = (uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_lsb +
		(((uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_msb) << 8);

	if (ep_idx == 0) {
	/* if ep0 check trans_type in OUT_TOKEN to
	 * prevent wrong read_bytes cause memory error
	 */
		if (udata0.st_state == STATUS_ST &&
			(ep_regs[EP0].ep_transtype_sts & DC_ALL_TRANS) == 0) {

			*read_bytes = 0;
			return 0;

		} else if (udata0.st_state == STATUS_ST) {
			/* status out but rx_fifo_len not zero */
			if (rx_fifo_len != 0) {
				LOG_ERR("Status OUT length not 0 (%d)",
					rx_fifo_len);
			}
			/* rx_fifo_len = 0; */
			*read_bytes = 0;

			return 0;
		} else if (rx_fifo_len == 0 &&
			udata0.now_token == SETUP_TOKEN) {
			/* RX fifo error workaround */

			/* wrong length(like 7),
			 * may read wrong packet so clear fifo then return -1
			 */
			LOG_ERR("Setup length 0, reset to 8");
			rx_fifo_len = 8;
		} else if (rx_fifo_len != 8 &&
			udata0.now_token == SETUP_TOKEN) {
			LOG_ERR("Setup length: %d", rx_fifo_len);
			/* clear rx fifo */
			ff_regs[EP0].ep_rx_fifo_ctrl = FIFO_FORCE_EMPTY;

			return -EIO;
		}
	}

	if (rx_fifo_len > max_data_len) {
		*read_bytes = max_data_len;
		for (int idx = 0 ; idx < max_data_len ; idx++) {
			buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
		}

		LOG_DBG("Read Max (%d) Packets", max_data_len);
	} else {

		*read_bytes = rx_fifo_len;

		for (int idx = 0 ; idx < rx_fifo_len ; idx++) {
			buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
		}

		if (ep_fifo == 0 &&
			udata0.now_token == SETUP_TOKEN) {
			LOG_DBG("RX buf: (%x)(%x)(%x)(%x)(%x)(%x)(%x)(%x)",
			buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], buf[6], buf[7]);
		}

		if (ep_fifo > EP0) {
			ep_regs[ep_fifo].ep_ctrl |= ENDPOINT_RDY;
			it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_READY);
			LOG_DBG("(%d): Set EP%d Ready", __LINE__, ep_idx);
			udata0.ep_ready[ep_fifo - 1] = true;
		} else if (udata0.now_token == SETUP_TOKEN) {

			if (!(buf[0] & USB_EP_DIR_MASK)) {
			/* Host to device transfer check */
				if (buf[6] != 0 || buf[7] != 0) {
					/* clear tx fifo */
					ff_regs[EP0].ep_tx_fifo_ctrl =
					FIFO_FORCE_EMPTY;
					/* set status IN after data OUT */
					ep_regs[EP0].ep_ctrl |=
					ENDPOINT_RDY | EP_OUTDATA_SEQ;
					LOG_DBG("Set EP%d Ready(%d)",
						ep_idx, __LINE__);
				} else {
					/* no_data_ctrl status */

					/* clear tx fifo */
					ff_regs[EP0].ep_tx_fifo_ctrl =
					FIFO_FORCE_EMPTY;
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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;
	struct it82xx2_usb_ep_fifo_regs *ff_regs = usb_regs->fifo_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	uint8_t ep_fifo = 0;
	uint16_t rx_fifo_len;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d): Wrong Endpoint Index/Address", __LINE__);
		return -EINVAL;
	}
	/* Check if OUT ep */
	if (!!(ep & USB_EP_DIR_MASK)) {
		LOG_ERR("Wrong Endpoint Direction");
		return -EINVAL;
	}

	if (ep_idx > EP0) {
		ep_fifo = ep_fifo_res[ep_idx % FIFO_NUM];
	}

	if (ep_regs[ep_fifo].ep_status & EP_STATUS_ERROR) {
		LOG_WRN("EP error flag(%02x)", ep_regs[ep_fifo].ep_status);
	}

	rx_fifo_len = (uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_lsb +
		(((uint16_t)ff_regs[ep_fifo].ep_rx_fifo_dcnt_msb) << 8);

	LOG_DBG("ep_read_wait (EP: %d), len: %d", ep_idx, rx_fifo_len);

	*read_bytes = (rx_fifo_len > max_data_len) ?
		max_data_len : rx_fifo_len;

	for (int idx = 0 ; idx < *read_bytes ; idx++) {
		buf[idx] = ff_regs[ep_fifo].ep_rx_fifo_data;
	}

	LOG_DBG("Read %d packets", *read_bytes);

	if (ep_idx > EP0) {
		LOG_DBG("RX buf[0]: 0x%02X", buf[0]);
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();
	struct it82xx2_usb_ep_regs *ep_regs = usb_regs->usb_ep_regs;

	uint8_t ep_idx = ep & EP_VALID_MASK;
	uint8_t ep_fifo = 2;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

	if (ep_idx >= MAX_NUM_ENDPOINTS) {
		LOG_ERR("(%d): Wrong Endpoint Index/Address", __LINE__);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (!!(ep & USB_EP_DIR_MASK)) {
		LOG_ERR("Wrong Endpoint Direction");
		return -EINVAL;
	}

	if (ep_idx > EP0) {
		ep_fifo = ep_fifo_res[ep_idx % FIFO_NUM];
	}

	it82xx2_usb_extend_ep_ctrl(ep_idx, EXT_EP_READY);
	ep_regs[ep_fifo].ep_ctrl |= ENDPOINT_RDY;
	udata0.ep_ready[ep_fifo - 1] = true;
	LOG_DBG("EP(%d) Read Continue", ep_idx);
	return 0;
}


int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_idx = ep & EP_VALID_MASK;

	if ((ep & EP_INVALID_MASK) != 0) {
		return -EINVAL;
	}

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
	struct usb_it82xx2_regs *const usb_regs =
		(struct usb_it82xx2_regs *)it82xx2_get_usb_regs();

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
