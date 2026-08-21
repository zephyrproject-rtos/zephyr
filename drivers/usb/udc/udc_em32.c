/*
 * SPDX-FileCopyrightText: 2026 ELAN Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Access USB device controller from devicetree */
#define DT_DRV_COMPAT elan_em32_usbd

#include "udc_common.h"

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(udc_em32, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define IS_IO_ROUTE 1

/* In 32-bit system, width of register is 4 bytes. */
#define REG_WIDTH               4
#define USB_NUM_BIDIR_ENDPOINTS 5
#define EP0_MPS                 8
#define EP_MPS                  64

#define USB_EM32_DEV_ADDR 0x0F

enum udc_em32_msg_type {
	/* Setup packet received */
	UDC_EM32_MSG_TYPE_SETUP,
	/* Xfer requests which are enqueued */
	UDC_EM32_MSG_TYPE_XFER,
	/* Issue PWR msg to process set/clear feature patch for remote wakeup */
	UDC_EM32_MSG_TYPE_PWR,
};

/* Because the hardware layer automatically handles the set address request
 * and does not notify the firmware, a set address patch must be executed on the
 * upper layer to complete the set address action.
 */
enum udc_em32_address_state {
	/* device is not addressed */
	USB_EM32_NOT_ADDRESSED = 0,
	/* The patch for the set-address request has been marked. */
	USB_EM32_SET_ADDRESS_START = 1,
	/* start to do set-address patch, send the set-address request to upper layer
	 */
	USB_EM32_SET_ADDRESS_PROCESS = 2,
	/* Received the status package from the upper level
	 * and completed the set-address request.
	 */
	USB_EM32_SET_ADDRESS_DONE = 3,
};

/* Similar to the set address patch,
 * the set configuration patch must be executed
 * to perform a set configuration request on the upper layer.
 */
enum udc_em32_configuration_state {
	/* device is not configured */
	USB_EM32_NOT_CONFIGURED = 0,
	/* The patch for the set-configuration request has been marked. */
	USB_EM32_SET_CONFIGURATION_START = 1,
	/* start to do set-configuration patch, send the set-configuration request to
	   upper layer */
	USB_EM32_SET_CONFIGURATION_PROCESS = 2,
	/* Received the status package from the upper level
	 * and completed the set-configuration request.
	 */
	USB_EM32_SET_CONFIGURATION_DONE = 3,
};

/* Because the hardware layer automatically handles set/clear feature requests,
 * the firmware must perform set/clear feature path processing at the following
 * points: Set feature for remote wakeup:
 *   1. Receive a suspend signal
 *   2. The USB device has been configured.
 * Clear feature for remote wakeup:
 *   1. Receive a resume signal
 *   2. The USB device has been configured.
 */
enum udc_em32_remote_wakeup_state {
	/* No set/clear feature patch was sent. */
	USB_REMOTE_WAKEUP_REQ_NOT_ISSUE = 0,
	/* This is a set feature patch */
	USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND = 1,
	/* This is a clear feature patch */
	USB_REMOTE_WAKEUP_REQ_SRC_RESUME = 2,
};

struct udc_em32_msg {
	enum udc_em32_msg_type type;
	union {
		struct {
			enum udc_event_type type;
		} udc_bus_event;
		struct {
			uint8_t ep;
		} setup;
		struct {
			uint8_t ep;
		} xfer;
		struct {
			uint8_t sus;
		} pwr;
	};
};

struct udc_em32_clock {
	const struct device *dev;
	clock_control_subsys_t subsys;
};

struct udc_em32_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	uint32_t ep_cfg_out_size;
	uint32_t ep_cfg_in_size;
	int speed_idx;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
	struct k_msgq *msgq;
	struct udc_em32_clock aip_clock;
	struct udc_em32_clock udc_clock;
	struct udc_em32_clock atrim_clock;
};

struct udc_em32_usbd_ep {
	uint8_t idx;
	uint32_t is_in_pkg;
	uint32_t is_out_pkg;
	/* ep control registers */
	uint32_t reg_ep_int_en;
	uint32_t reg_ep_int_sta;
	uint32_t reg_data_cnt;
	uint32_t reg_data_buf;
};

struct udc_em32_data {
	/* Setup package being processed */
	uint8_t setup_pkg[8];
	/* When patch processing begins, the UDC driver places the newly arrived setup
	 * package in the pending_setup_pkg and sets the is_pending_pkg flag. The UDC
	 * driver then places the patch command in the setup_pkg and begins
	 * processing.
	 */
	uint8_t pending_setup_pkg[8];
	uint8_t address;
	struct k_msgq *msgq;
	struct k_thread thread_data;

#if (IS_IO_ROUTE)
	uint8_t ep_addr_org;
	uint8_t ep_addr_new;
#endif
	/* If it is 1, pending_setup_pkg waits to be processed. */
	uint32_t is_pending_pkg;
	/* If is_ep0_out_pkg is 1, it means that the ep buf does not exist
	 * when the out interrupt occurs.
	 * The out request will be processed in the normal code.
	 */
	uint32_t is_ep0_out_pkg;
	/* When a new setup isr receives a new setup package,
	 * it clears is_ep0_in_en and is_ep0_out_en flags.
	 * is_ep0_in_en and is_ep0_out_en flags are only set
	 * after the sent setup message has been processed, allowing IN and OUT I/O
	 * requests.
	 */
	uint32_t is_ep0_in_en;
	uint32_t is_ep0_out_en;
	/* Size of transfer of package processed currently */
	uint32_t ep0_xfer_size;
	/* addressed_state:
	 * This is used to control the state of the standard set-address command.
	 * The UDC driver sends this command to the upper layer driver
	 *   to complete the standard set-address action.
	 */
	uint32_t addressed_state;
	/* configured_state:
	 * This is used to control the state of the standard set-configuration
	 * command. The UDC driver sends this command to the upper layer driver to
	 * complete the standard set-configuration action.
	 */
	uint32_t configured_state;
	/* The source of the remote wakeup request */
	uint32_t proc_remote_wakeup_state;
	/* Control from ep1 to ep4 */
	struct udc_em32_usbd_ep epx_ctrl[USB_NUM_BIDIR_ENDPOINTS - 1];

#if defined(CONFIG_PM)
	uint8_t _pm_policy_locked;
#endif
};

/* Enable host resume wake-up function */
static inline void em32_enable_usb_wakeup(void)
{
	sys_set_bit(REG_PHY_TEST, PHYTEST_USB_WAKEUP_EN_Pos);
}

/* Disable host resume wake-up function */
static inline void em32_disable_usb_wakeup(void)
{
	sys_clear_bit(REG_PHY_TEST, PHYTEST_USB_WAKEUP_EN_Pos);
}

static inline void em32_set_usb_pll_src_irc(void)
{
	sys_clear_bit(REG_EM32_SYS_CTRL, SYSCTRL_USB_CLK_SEL_Pos);
}

static inline void em32_set_clk_src_irc(void)
{
	sys_set_bit(REG_EM32_SYS_CTRL, SYSCTRL_XTAL_LJIRC_SEL_Pos);
}

static inline void em32_usb_clk_en(void)
{
	sys_clear_bit(REG_USB_PLL_CTRL, AIP_USB_PLL_CTRL_PD_Pos);

	while (!sys_test_bit(REG_USB_PLL_CTRL, AIP_USB_PLL_CTRL_STABLE_Pos)) {
		/* wait until usb pll is stable */
	}
}

/* power on CLOCK */
static inline void em32_clk_pwr_on(void)
{
	sys_clear_bit(REG_LJIRC_CTRL, AIP_LJIRC_CTRL_LJIRC_PD_Pos);
}

/* It is necessary to configure the characteristics of the clock
 * to maintain stable operation.
 */
static inline void usb_em32_set_clk_prop(const struct udc_em32_data *priv)
{
	uint32_t code;
	uint32_t data;

	/* Adjust the IRC parameters to obtain an accurate clock. */
	code = sys_read32(USB_IRC_PROP_CODE_1);
	data = sys_read32(REG_LJIRC_CTRL);

	data = data & (~AIP_LJIRC_CTRL_LJIRC_CODE_Msk);
	code = code << AIP_LJIRC_CTRL_LJIRC_CODE_Pos;
	code = code & AIP_LJIRC_CTRL_LJIRC_CODE_Msk;
	data = data + code;
	sys_write32(data, REG_LJIRC_CTRL);

	code = sys_read32(USB_IRC_PROP_CODE_2);
	data = sys_read32(REG_AIP_USB_PHY);

	data = data & (~AIP_USB_PHY_CTRL_RTRIM_Msk);
	code = code << AIP_USB_PHY_CTRL_RTRIM_Pos;
	code = code & AIP_USB_PHY_CTRL_RTRIM_Msk;
	data = data + code;
	sys_write32(data, REG_AIP_USB_PHY);

	em32_set_clk_src_irc();
	em32_clk_pwr_on();

	k_busy_wait(2000);
}

/* Hardcoded em32 usb ep settings */
static const unsigned char usb_ep_conf_data[6] = {0x43, 0x43, 0x43, 0x43, 0xFA, 0x00};
static const uint32_t ep1_max_pkg_size = 64;
static const uint32_t ep2_max_pkg_size = 64;
static const uint32_t ep3_max_pkg_size = 64;
static const uint32_t ep4_max_pkg_size = 64;

/* Before use, the characteristics of ep need to be configured. */
static void em32_usb_ep_setup(void)
{
	uint32_t data;
	int index;

	/* setup usb ep properties */
	for (index = 0; index < 4; index++) {
		data = sys_read32(REG_USB_CF_DATA);
		data = data & (~USBD_CF_DATA_CONFIG_DATA_Msk);
		data = data + usb_ep_conf_data[index];
		sys_write32(data, REG_USB_CF_DATA);

		while (!sys_test_bit(REG_USB_CF_DATA, USBD_CF_DATA_EP_CONFIG_RDY_Pos)) {
			/* wait until device is ready to process next data */
		}
	}

	data = sys_read32(REG_USB_CF_DATA);
	data = data & (~USBD_CF_DATA_CONFIG_DATA_Msk);
	data = data + usb_ep_conf_data[4];
	sys_write32(data, REG_USB_CF_DATA);

	while (!sys_test_bit(REG_USB_CF_DATA, USBD_CF_DATA_EP_CONFIG_DONE_Pos)) {
		/* wait until eps of usb phy are configured */
	}

	/* Setup ep fifo size */
	data = (ep2_max_pkg_size << 16) + ep1_max_pkg_size;
	sys_write32(data, REG_USB_EP_BUF_SET_0);
	data = (ep4_max_pkg_size << 16) + ep3_max_pkg_size;
	sys_write32(data, REG_USB_EP_BUF_SET_1);
}

/* Obtain the corresponding EP control unit */
static struct udc_em32_usbd_ep *udc_em32_get_ep(struct udc_em32_data *priv, uint8_t ep_addr)
{
	uint8_t ep_idx;

	ep_idx = USB_EP_GET_IDX(ep_addr);

	if (ep_idx >= USB_NUM_BIDIR_ENDPOINTS || ep_idx == 0) {
		return NULL;
	}

	return &priv->epx_ctrl[ep_idx - 1];
}

/* Disconnect from host */
static inline void usb_em32_sw_disconnect(void)
{
	sys_clear_bit(REG_AIP_USB_PHY, AIP_USB_PHY_CTRL_RSW_Pos);
}

/* Connect to host */
static inline void usb_em32_sw_connect()
{
	sys_set_bit(REG_AIP_USB_PHY, AIP_USB_PHY_CTRL_RSW_Pos);
}

#if defined(CONFIG_PM)

static void lock_pm_policy(const struct device *dev, uint8_t lock)
{
	struct udc_em32_data *dev_inst = udc_get_private(dev);

	unsigned int irq_lock_key = arch_irq_lock();
	if (dev_inst->_pm_policy_locked != lock) {
		const struct pm_state_info *cpu_states;
		uint32_t num_cpu_states = pm_state_cpu_get_all(0, &cpu_states);

		if (lock) {
			/* Prevent the CPU from entering any low power states */
			for (uint32_t i = 0; i < num_cpu_states; i++) {
				pm_policy_state_lock_get(cpu_states[i].state, PM_ALL_SUBSTATES);
			}
		} else {
			/* Allow the CPU to enter available low power states */
			for (uint32_t i = 0; i < num_cpu_states; i++) {
				pm_policy_state_lock_put(cpu_states[i].state, PM_ALL_SUBSTATES);
			}
		}
		dev_inst->_pm_policy_locked = lock;
	}
	arch_irq_unlock(irq_lock_key);
}
#else
/* Stub: Do nothing if PM not enabled */
#define lock_pm_policy(dev, lock)
#endif

static int udc_em32_clock_on(const struct udc_em32_clock *clock)
{
	if (!device_is_ready(clock->dev)) {
		return -ENODEV;
	}

	return clock_control_on(clock->dev, clock->subsys);
}

/* Start USB PHY and get it working. */
static int em32_usb_boot(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	int err;

	err = udc_em32_clock_on(&config->aip_clock);
	if (err != 0) {
		return err;
	}

	usb_em32_set_clk_prop(priv);

	/* select usb clock source, then power on usb pll */
	em32_set_usb_pll_src_irc();
	err = udc_em32_clock_on(&config->udc_clock);
	if (err != 0) {
		return err;
	}
	em32_usb_clk_en();

	/* power on usb phy */
	sys_set_bit(REG_AIP_USB_PHY, AIP_USB_PHY_CTRL_PD_Pos);

	return 0;
}

/* 1. Enable usb phy
 * 2. Disconnect from host
 * 3. Configure ep properties
 */
static void em32_usb_phy_setup(void)
{
	/* enable usb phy, wait until it is ready */
	sys_set_bit(REG_USB_CTRL, REG_USB_CTRL_UDC_EN_Pos);
	while (!sys_test_bit(REG_USB_CTRL, REG_USB_CTRL_UDC_RST_RDY_Pos)) {
		/* wait until usb phy is ready for use */
	}

	usb_em32_sw_disconnect();
	em32_usb_ep_setup();
}

static int em32_usb_init(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	int err;

	em32_usb_phy_setup();

	/* turn on RST/SUSPEND/RESUME interrupts */
	sys_set_bit(REG_USB_INT_EN, REG_USB_INT_EN_RST_INT_EN_Pos);
	sys_set_bit(REG_USB_INT_EN, REG_USB_INT_EN_SUS_INT_EN_Pos);
	sys_set_bit(REG_USB_INT_EN, REG_USB_INT_EN_RESUME_INT_EN_Pos);

	/* In the initial stage, turn on ep0. */
	sys_set_bit(REG_EP0_INT_EN, REG_EP0_INT_EN_SETUP_INT_EN_Pos);
	sys_set_bit(REG_EP0_INT_EN, REG_EP0_INT_EN_IN_INT_EN_Pos);
	sys_set_bit(REG_EP0_INT_EN, REG_EP0_INT_EN_OUT_INT_EN_Pos);

	/* In the initial stage, turn off all ep except ep0. */
	sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP1_EN_Pos);
	sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP2_EN_Pos);
	sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP3_EN_Pos);
	sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP4_EN_Pos);

	err = udc_em32_clock_on(&config->atrim_clock);
	if (err != 0) {
		return err;
	}

	return 0;
}

/* Configure control units from ep1 to ep4. */
static void udc_em32_epx_init(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *epx_ctrl;

	for (int i = 1; i <= 4; i++) {
		epx_ctrl = udc_em32_get_ep(priv, i);
		if (epx_ctrl != NULL) {
			memset((void *)epx_ctrl, 0, sizeof(struct udc_em32_usbd_ep));
			epx_ctrl->idx = i;
			epx_ctrl->reg_ep_int_en = REG_EP1_INT_EN + REG_WIDTH * (i - 1);
			epx_ctrl->reg_ep_int_sta = REG_EP1_INT_STA + REG_WIDTH * (i - 1);
			epx_ctrl->reg_data_cnt = REG_EP1_DATA_CNT + REG_WIDTH * (i - 1);
			epx_ctrl->reg_data_buf = REG_EP1_DATA_BUF + REG_WIDTH * (i - 1);
		}
	}
}

#if (IS_IO_ROUTE)
static struct usb_ep_descriptor *get_next_ep_desc(uint8_t *ptr_start, const uint8_t *buf_end,
						  uint8_t addr_org)
{
	struct usb_desc_header *desc_header;
	struct usb_ep_descriptor *ep_desc;

	desc_header = (struct usb_desc_header *)ptr_start;
	do {
		if (desc_header->bLength == 0) {
			/* avoid deadlock caused by invalid header */
			break;
		}

		if ((((uint8_t *)desc_header) + desc_header->bLength) > buf_end) {
			break;
		}

		if (desc_header->bDescriptorType == USB_DESC_ENDPOINT) {
			ep_desc = (struct usb_ep_descriptor *)desc_header;
			if (addr_org == 0 || (ep_desc->bEndpointAddress == addr_org)) {
				return ep_desc;
			}
		}

		desc_header =
			(struct usb_desc_header *)(((uint8_t *)desc_header) + desc_header->bLength);
	} while (((uint8_t *)desc_header) < buf_end);

	return NULL;
}

static int ep_re_direct(const struct device *dev, struct net_buf *buf)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct usb_cfg_descriptor *conf_desc;
	struct usb_ep_descriptor *ep_desc;
	struct usb_desc_header *desc_header;
	uint32_t pipe_using;
	uint32_t pipe_conflict;
	uint32_t pipe_available;
	uint8_t *buf_start;
	uint8_t *buf_end;
	uint32_t len;
	uint8_t ep_idx;
	bool is_done;

	buf_start = buf->data;
	len = buf->len;

	if (buf_start == NULL) {
		return -EINVAL;
	}

	if (len <= sizeof(struct usb_cfg_descriptor)) {
		return 0;
	}

	conf_desc = (struct usb_cfg_descriptor *)buf_start;
	if ((conf_desc->bLength != sizeof(struct usb_cfg_descriptor)) ||
	    (conf_desc->bDescriptorType != USB_DESC_CONFIGURATION) ||
	    (len < conf_desc->wTotalLength)) {
		/* Only parse full configuration descriptor */
		return 0;
	}

	buf_end = buf_start + len;

	/* parse first time to find overlapped pies */
	pipe_using = 0;
	pipe_available = 0x0F;
	desc_header = (struct usb_desc_header *)buf_start;

	do {
		/* Move to nex descriptor */
		ep_desc = get_next_ep_desc((uint8_t *)desc_header, buf_end, 0);
		if (ep_desc != NULL) {
			ep_idx = USB_EP_GET_IDX(ep_desc->bEndpointAddress);
			if (USB_EP_GET_DIR(ep_desc->bEndpointAddress)) {
				/* bit[7:4] for IN pipe */
				pipe_using |= (1 << (ep_idx + 4 - 1));
			} else {
				/* bit[3:0] for OUT pipe */
				pipe_using |= (1 << (ep_idx - 1));
			}
			/* if pipe is in use, it can't be available while re-direct pipe IO */
			pipe_available &= ~(1 << (ep_idx - 1));
		} else {
			break;
		}

		desc_header = (struct usb_desc_header *)(((uint8_t *)ep_desc) + ep_desc->bLength);
	} while (((uint8_t *)desc_header) < buf_end);

	/* find ep which uses both IN and OUT direction */
	pipe_conflict = 0;
	for (int i = 0; i < 4; i++) {
		if ((pipe_using & (1 << i)) && (pipe_using & (1 << (i + 4)))) {
			pipe_conflict |= (1 << i);
		}
	}

	if (!pipe_conflict) {
		/* return with no conflict */
		return 0;
	}

	if (!pipe_available) {
		return -ENOTSUP;
	}

	/* only move IN pipe if conflict with free pipes */
	is_done = false;
	ep_idx = 0;
	for (int i = 0; i < 4; i++) {
		ep_idx++;
		if (pipe_conflict & (1 << i)) {
			ep_desc = get_next_ep_desc(buf_start, buf_end, USB_EP_DIR_IN | ep_idx);
			priv->ep_addr_new = 0;
			for (int j = 0; j < 4; j++) {
				priv->ep_addr_new++;
				if (pipe_available & (1 << j)) {
					priv->ep_addr_org = ep_desc->bEndpointAddress;
					priv->ep_addr_new = USB_EP_DIR_IN | priv->ep_addr_new;
					ep_desc->bEndpointAddress = priv->ep_addr_new;
					is_done = true;
					pipe_available &= ~(1 << j);
					break;
				}
			}
			pipe_conflict &= ~(1 << i);
		}
		if (is_done) {
			break;
		}
	}

	if (pipe_conflict) {
		return -ENOSPC;
	}

	return 0;
}
#endif

static int udc_em32_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			       struct net_buf *buf)
{
#if (IS_IO_ROUTE)
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *new_cfg;
	int err;
#endif
	struct udc_em32_msg msg = {0};
	uint8_t ep = cfg->addr;

#if (IS_IO_ROUTE)
	if (ep == USB_CONTROL_EP_IN) {
		err = ep_re_direct(dev, buf);
		if (err < 0) {
			return err;
		}
	}

	if ((priv->ep_addr_org != 0) && (ep == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		udc_buf_put(new_cfg, buf);
		ep = priv->ep_addr_new;
	} else {
		udc_buf_put(cfg, buf);
	}
#else
	udc_buf_put(cfg, buf);
#endif

	msg.type = UDC_EM32_MSG_TYPE_XFER;
	msg.xfer.ep = ep;
	k_msgq_put(priv->msgq, &msg, K_NO_WAIT);

	return 0;
}

static int udc_em32_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);

#if (IS_IO_ROUTE)
	struct udc_ep_config *new_cfg;

	if ((priv->ep_addr_org != 0) && (cfg->addr == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		udc_ep_cancel_queued(dev, new_cfg);
	} else {
		udc_ep_cancel_queued(dev, cfg);
	}
#else
	udc_ep_cancel_queued(dev, cfg);
#endif

	return 0;
}

static void usb_em32_ep_set_halt(struct udc_em32_data *priv, struct udc_ep_config *const cfg,
				 bool isHalt)
{
	uint8_t ep_idx;

	ep_idx = USB_EP_GET_IDX(cfg->addr);
	cfg->stat.halted = isHalt;

	if (isHalt) {
		/* set corresponding ep to be stalled */
		if (ep_idx == 0) {
			sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP0_STALL_Pos);
		} else if (ep_idx == 1) {
			sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP1_STALL_Pos);
		} else if (ep_idx == 2) {
			sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP2_STALL_Pos);
		} else if (ep_idx == 3) {
			sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP3_STALL_Pos);
		} else if (ep_idx == 4) {
			sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP4_STALL_Pos);
		}
	} else {
		/* reset corresponding ep to be unstalled */
		if (ep_idx == 0) {
			sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP0_STALL_Pos);
		} else if (ep_idx == 1) {
			sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP1_STALL_Pos);
		} else if (ep_idx == 2) {
			sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP2_STALL_Pos);
		} else if (ep_idx == 3) {
			sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP3_STALL_Pos);
		} else if (ep_idx == 4) {
			sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP4_STALL_Pos);
		}
	}
}

/* Halt endpoint. Halted endpoint should respond with a STALL handshake. */
static int udc_em32_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);

#if (IS_IO_ROUTE)
	struct udc_ep_config *new_cfg;

	if ((priv->ep_addr_org != 0) && (cfg->addr == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		usb_em32_ep_set_halt(priv, new_cfg, true);
	} else {
		usb_em32_ep_set_halt(priv, cfg, true);
	}
#else
	usb_em32_ep_set_halt(priv, cfg, true);
#endif

	return 0;
}

/*
 * Opposite to halt endpoint. If there are requests in the endpoint queue,
 * the next transfer should be prepared.
 */
static int udc_em32_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);

#if (IS_IO_ROUTE)
	struct udc_ep_config *new_cfg;

	if ((priv->ep_addr_org != 0) && (cfg->addr == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		usb_em32_ep_set_halt(priv, new_cfg, false);
	} else {
		usb_em32_ep_set_halt(priv, cfg, false);
	}
#else
	usb_em32_ep_set_halt(priv, cfg, false);
#endif

	return 0;
}

static int udc_em32_host_wakeup(const struct device *dev)
{
	sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_DEV_RESUME_Pos);
	k_busy_wait(10000);
	sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_DEV_RESUME_Pos);

	return 0;
}

/* If there are pending requests, `re_issue_pending_pkg` will be called
 *  at the appropriate time to handle them.
 * This approach is typically used when handling patches
 *  for set-address and set-configuration.
 */
static void re_issue_pending_pkg(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_msg msg;

	if (priv->is_pending_pkg) {
		/* Copy the request from pending_setup_pkg to setup_pkg,
		 *  then clear the is_pending_pkg flag.
		 * Send the UDC_EM32_MSG_TYPE_SETUP message to
		 *  process the request from setup_pkg.
		 */
		for (int i = 0; i < 8; i++) {
			priv->setup_pkg[i] = priv->pending_setup_pkg[i];
		}
		atomic_set((void *)&priv->is_pending_pkg, 0);

		msg.type = UDC_EM32_MSG_TYPE_SETUP;
		k_msgq_put(priv->msgq, &msg, K_NO_WAIT);

		return;
	}

	return;
}

/* The EM32 USB PHY automatically handles set-address and set-configuration
 * requests. On Zephyr systems, the UDC driver must provide patches to send
 * these requests to the upper layer. The `do_patch_proc` function is
 * responsible for handling these patches.
 */
static int do_patch_proc(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct usb_setup_packet *setup;
	uint16_t min_req_len;

	min_req_len = 0;
	setup = (struct usb_setup_packet *)priv->setup_pkg;
	if (priv->addressed_state == USB_EM32_NOT_ADDRESSED) {
		min_req_len = sizeof(struct usb_device_descriptor);
		if ((setup->bmRequestType == 0x80) &&
		    (setup->bRequest == USB_SREQ_GET_DESCRIPTOR) &&
		    ((setup->wValue >> 8) == USB_DESC_DEVICE) && (setup->wLength >= min_req_len)) {
			/* When a get-device-descriptor with a length >= 18 bytes is received,
			 * the set-address patching action begins.
			 */
			priv->addressed_state = USB_EM32_SET_ADDRESS_START;
			return 0;
		}
	} else if (priv->addressed_state == USB_EM32_SET_ADDRESS_START) {
		/* When the set-address patch begins execution:
		 * 1. The current request is placed in the pending_setup_pkg for delayed
		 * processing.
		 * 2. The set-address request is set as the currently processing request and
		 *     sent out for processing.
		 */
		priv->addressed_state = USB_EM32_SET_ADDRESS_PROCESS;

		for (int i = 0; i < 8; i++) {
			priv->pending_setup_pkg[i] = priv->setup_pkg[i];
		}
		atomic_set((void *)&priv->is_pending_pkg, 1);

		setup->RequestType.direction = USB_REQTYPE_DIR_TO_DEVICE;
		setup->RequestType.type = USB_REQTYPE_TYPE_STANDARD;
		setup->RequestType.recipient = USB_REQTYPE_RECIPIENT_DEVICE;
		setup->bRequest = USB_SREQ_SET_ADDRESS;
		setup->wValue = USB_EM32_DEV_ADDR;
		setup->wIndex = 0;
		setup->wLength = 0;

		udc_setup_received(dev, priv->setup_pkg);
		return 1;
	}

	/* The operation method for `set-configuration patch` is the same
	 *  as that for `set-address patch`.
	 */
	if (priv->configured_state == USB_EM32_NOT_CONFIGURED) {
		min_req_len = sizeof(struct usb_cfg_descriptor) + sizeof(struct usb_if_descriptor);
		if ((setup->bmRequestType == 0x80) &&
		    (setup->bRequest == USB_SREQ_GET_DESCRIPTOR) &&
		    ((setup->wValue >> 8) == USB_DESC_CONFIGURATION) &&
		    (setup->wLength >= min_req_len)) {
			/* When a get-configuration-descriptor with a length > 18 bytes is
			 * received, the set-configuration patching action begins.
			 */
			priv->configured_state = USB_EM32_SET_CONFIGURATION_START;
			return 0;
		}
	} else if (priv->configured_state == USB_EM32_SET_CONFIGURATION_START) {
		priv->configured_state = USB_EM32_SET_CONFIGURATION_PROCESS;

		for (int i = 0; i < 8; i++) {
			priv->pending_setup_pkg[i] = priv->setup_pkg[i];
		}
		atomic_set((void *)&priv->is_pending_pkg, 1);

		setup->RequestType.direction = USB_REQTYPE_DIR_TO_DEVICE;
		setup->RequestType.type = USB_REQTYPE_TYPE_STANDARD;
		setup->RequestType.recipient = USB_REQTYPE_RECIPIENT_DEVICE;
		setup->bRequest = USB_SREQ_SET_CONFIGURATION;
		setup->wValue = 1;
		setup->wIndex = 0;
		setup->wLength = 0;

		udc_setup_received(dev, priv->setup_pkg);
		return 1;
	}

	return 0;
}

/* The EM32 USB PHY automatically handles the remote wakeup settings sent by the
 * host. The UDC driver must send the remote wakeup settings patch to the upper
 * layer at the appropriate time. The UDC driver will process this patch when
 * the following conditions are met:
 *  1. The device has been configured.
 *  2. A suspend/resume signal has been received.
 */
int em32_set_remote_wakeup_handler(const struct device *dev, uint32_t isSet)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_msg msg = {0};

	atomic_set((void *)&priv->is_pending_pkg, 0);
	for (int i = 0; i < 8; i++) {
		priv->pending_setup_pkg[i] = 0x0;
		priv->setup_pkg[i] = 0x0;
	}

	/* Only do remote-wakeup patch when device is configured */
	if (priv->configured_state < USB_EM32_SET_CONFIGURATION_DONE) {
		return 0;
	}

	/* Send a PWR message to notify the udc driver that
	 *  it has received a suspend or resume signal.
	 */
	msg.type = UDC_EM32_MSG_TYPE_PWR;
	if (isSet) {
		msg.pwr.sus = 1;
	} else {
		msg.pwr.sus = 0;
	}

	k_msgq_put(priv->msgq, &msg, K_NO_WAIT);

	return 1;
}

/* Message handler for setup package */
static int udc_em32_setup_msg_handler(const struct device *dev, const struct udc_em32_msg *msg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint8_t *setup_pkg;
	uint16_t xfer_size;
	struct udc_ep_config *ep_ctrl_in;
	struct udc_ep_config *ep_ctrl_out;

	setup_pkg = priv->setup_pkg;
	xfer_size = *((uint16_t *)(setup_pkg + 6));

	/* If a patch must be executed, this setup request will be pending.
	 * The setup request will be completed only after the patch has finished
	 * executing.
	 */
	if (do_patch_proc(dev)) {
		return 0;
	}

	ep_ctrl_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	ep_ctrl_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);

	udc_ep_set_busy(ep_ctrl_in, false);
	udc_ep_set_busy(ep_ctrl_out, false);

	usb_em32_ep_set_halt(priv, ep_ctrl_in, false);
	usb_em32_ep_set_halt(priv, ep_ctrl_out, false);

	udc_setup_received(dev, setup_pkg);
	priv->ep0_xfer_size = xfer_size;
	atomic_set((void *)&priv->is_ep0_in_en, 1);
	atomic_set((void *)&priv->is_ep0_out_en, 1);

	return 0;
}

/* control pipe OUT transaction handler */
static int udc_em32_ctrl_out(const struct device *dev, uint8_t ep)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len;
	uint32_t is_empty;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	buf = udc_buf_peek(ep_cfg);

	if (buf == NULL) {
		return 0;
	}

	bi = udc_get_buf_info(buf);
	if (bi->setup) {
		/* setup package is processed in UDC_EM32_MSG_TYPE_SETUP msg handler */
		return 0;
	}

	if (bi->status) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
		return 0;
	}

	if (!bi->data) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, -EINVAL);
		return 0;
	}

	/* process data package */
	if (!atomic_test_bit((void *)&priv->is_ep0_out_pkg, 0)) {
		return 0;
	}

	data_len = net_buf_tailroom(buf);
	data_ptr = net_buf_tail(buf);
	len = 0;
	is_empty = 0;

	do {
		if (sys_test_bit(REG_EP_BUF_STA, REG_EP_BUF_STA_EP0_OUTBUF_EMPTY_Pos)) {
			is_empty = 1;
			break;
		}
		if (len >= data_len) {
			break;
		}
		if (len >= EP0_MPS) {
			break;
		}

		*data_ptr = (uint8_t)sys_read32(REG_EP0_DATA_BUF);

		len += 1;
		data_ptr++;

	} while (1);

	net_buf_add(buf, len);
	data_len = net_buf_tailroom(buf);

	if (len < EP0_MPS || data_len == 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

	atomic_set((void *)&priv->is_ep0_out_pkg, 0);

	return 0;
}

/* control pipe IN transaction handler */
static int udc_em32_ctrl_in(const struct device *dev, uint8_t ep)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	buf = udc_buf_peek(ep_cfg);

	if (buf == NULL) {
		return 0;
	}

	bi = udc_get_buf_info(buf);

	if (bi->status) {
		if (priv->addressed_state == USB_EM32_SET_ADDRESS_PROCESS) {

			/* This status transaction is for set-address patch */
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			priv->addressed_state = USB_EM32_SET_ADDRESS_DONE;

			/* while set-address patch is done,
			 *  continue processing pending setup package.
			 */
			re_issue_pending_pkg(dev);

			return 0;
		} else if (priv->configured_state == USB_EM32_SET_CONFIGURATION_PROCESS) {

			/* This status transaction is for set-configuration patch */
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			priv->configured_state = USB_EM32_SET_CONFIGURATION_DONE;

			/* while set-configuration patch is done,
			 *  continue processing pending setup package.
			 */
			re_issue_pending_pkg(dev);

			return 0;
		} else if (priv->proc_remote_wakeup_state) {
			/* This status transaction is for remote-wakeup patch */
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);

			if (priv->proc_remote_wakeup_state == USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND) {
				/* After completing the remote-wakeup patch,
				 *  a suspend event must be sent to notify the upper layer.
				 */
				udc_set_suspended(dev, true);
				udc_submit_event(dev, UDC_EVT_SUSPEND, 0);

				lock_pm_policy(dev, false);
			}

			priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_NOT_ISSUE;
		} else {
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
		}
	}

	return 0;
}

/* control pipe transaction handler */
static int udc_em32_ctrl_handler(const struct device *dev, uint8_t ep)
{
	int ret;
	if (USB_EP_DIR_IS_OUT(ep)) {
		ret = udc_em32_ctrl_out(dev, ep);
	} else {
		ret = udc_em32_ctrl_in(dev, ep);
	}
	return ret;
}

/* Handler for queued OUT transfer */
static int udc_em32_xfer_out(const struct device *dev, uint8_t ep)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct udc_em32_usbd_ep *ep_ctrl;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len, len, i;
	uint32_t is_out_pkg_in;
	uint32_t lock_key;

	lock_key = irq_lock();

	ep_ctrl = udc_em32_get_ep(priv, ep);
	ep_cfg = udc_get_ep_cfg(dev, ep);

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		goto exit;
	}

	data_len = net_buf_tailroom(buf);
	data_ptr = net_buf_tail(buf);

	is_out_pkg_in = 0;
	if (ep_ctrl->is_out_pkg) {
		is_out_pkg_in = 1;
	}

	if (!is_out_pkg_in) {
		goto exit;
	}

	do {
		sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);
	} while (!sys_test_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos));

	data_len = net_buf_tailroom(buf);
	data_ptr = net_buf_tail(buf);

	len = sys_read32(ep_ctrl->reg_data_cnt);
	len = len >> 16;
	if (len > EP_MPS) {
		len = EP_MPS;
	}
	if (len > data_len) {
		len = data_len;
	}

	for (i = 0; i < len; i++) {
		*data_ptr = (uint8_t)sys_read32(ep_ctrl->reg_data_buf);
		data_ptr++;
	}

	sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);

	net_buf_add(buf, len);

	data_len = net_buf_tailroom(buf);
	if (data_len == 0 || len < EP_MPS) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

	ep_ctrl->is_out_pkg = 0;

exit:
	irq_unlock(lock_key);
	return 0;
}

/* Handler for queued IN transfer */
static int udc_em32_xfer_in(const struct device *dev, uint8_t ep)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *ep_ctrl;

	ep_ctrl = udc_em32_get_ep(priv, ep);

	/* Process IN transaction in IN isr. */
	if (ep_ctrl->is_in_pkg) {

		sys_clear_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
		ep_ctrl->is_in_pkg = 0;
		sys_set_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
	}

	return 0;
}

/* Message handler for queued transfer */
static int udc_em32_xfer_msg_handler(const struct device *dev, struct udc_em32_msg *msg)
{
	uint8_t ep;

	ep = msg->xfer.ep;

	if (USB_EP_GET_IDX(ep) == 0) {
		udc_em32_ctrl_handler(dev, ep);
		return 0;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		udc_em32_xfer_out(dev, ep);
	} else {
		udc_em32_xfer_in(dev, ep);
	}

	return 0;
}

/* The EM32 USB PHY automatically handles the remote wakeup settings sent by the
 * host. This handler does remote-wakeup patch while suspend or resume signal is
 * happened.
 */
static int udc_em32_pwr_msg_handler(const struct device *dev, struct udc_em32_msg *msg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct usb_setup_packet *setup;
	struct udc_em32_msg setup_msg;
	uint8_t sus;

	setup = (struct usb_setup_packet *)priv->setup_pkg;
	memset((void *)setup, 0, sizeof(struct usb_setup_packet));
	sus = msg->pwr.sus;

	setup->RequestType.direction = USB_REQTYPE_DIR_TO_DEVICE;
	setup->RequestType.recipient = USB_REQTYPE_RECIPIENT_DEVICE;
	if (sus) {
		priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND;
		setup->bRequest = USB_SREQ_SET_FEATURE;
	} else {
		priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_SRC_RESUME;
		setup->bRequest = USB_SREQ_CLEAR_FEATURE;
	}
	setup->wValue = USB_SFS_REMOTE_WAKEUP;

	setup_msg.type = UDC_EM32_MSG_TYPE_SETUP;
	k_msgq_put(priv->msgq, &setup_msg, K_NO_WAIT);

	return 0;
}

static void em32_usbd_msg_handler(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_msg msg;
	int err;

	while (true) {
		if (k_msgq_get(priv->msgq, &msg, K_FOREVER)) {
			continue;
		}

		err = 0;

		udc_lock_internal(dev, K_FOREVER);

		switch (msg.type) {
		case UDC_EM32_MSG_TYPE_SETUP:
			err = udc_em32_setup_msg_handler(dev, &msg);
			break;
		case UDC_EM32_MSG_TYPE_XFER:
			err = udc_em32_xfer_msg_handler(dev, &msg);
			break;
		case UDC_EM32_MSG_TYPE_PWR:
			err = udc_em32_pwr_msg_handler(dev, &msg);
			break;
		default:
			__ASSERT_NO_MSG(false);
		}

		udc_unlock_internal(dev);

		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}
}

static void usb_em32_suspend_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	/* Activate the host wake-up device function */
	em32_enable_usb_wakeup();

	/* if device is not configured, bypass suspend/resume signal */
	if (priv->configured_state < USB_EM32_SET_CONFIGURATION_DONE) {
		sys_set_bit(REG_USB_INT_STA, REG_USB_INT_STA_SUS_INT_SF_CLR_Pos);
		return;
	}

	if (sys_test_bit(REG_USB_INT_STA, REG_USB_INT_STA_SUS_INT_SF_Pos)) {
		sys_set_bit(REG_USB_INT_STA, REG_USB_INT_STA_SUS_INT_SF_CLR_Pos);
	}

	if (em32_set_remote_wakeup_handler(dev, 1)) {
		return;
	}

	udc_set_suspended(dev, true);
	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);

	lock_pm_policy(dev, false);

	return;
}
static void usb_em32_resume_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	/* Disable the host wake-up device function */
	em32_disable_usb_wakeup();

	/* if device is not configured, bypass suspend/resume signal */
	if (priv->configured_state < USB_EM32_SET_CONFIGURATION_DONE) {
		sys_set_bit(REG_USB_INT_STA, REG_USB_INT_STA_RESUME_INT_SF_CLR_Pos);
		return;
	}

	if (sys_test_bit(REG_USB_INT_STA, REG_USB_INT_STA_RESUME_INT_SF_Pos)) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);

		lock_pm_policy(dev, true);

		em32_set_remote_wakeup_handler(dev, 0);
		sys_set_bit(REG_USB_INT_STA, REG_USB_INT_STA_RESUME_INT_SF_CLR_Pos);
	}

	return;
}

static void usb_em32_clean_ep_buf(struct udc_em32_data *priv, uint8_t ep)
{
	struct udc_em32_usbd_ep *epx_ctrl;
	int len;
	uint8_t tmp;

	if ((ep < 1) || (ep > 4)) {
		return;
	}

	epx_ctrl = udc_em32_get_ep(priv, ep);

	sys_set_bit(epx_ctrl->reg_ep_int_en, REG_EPX_INT_EN_BUF_CLR_Pos);
	sys_clear_bit(epx_ctrl->reg_ep_int_en, REG_EPX_INT_EN_BUF_CLR_Pos);
	len = sys_read32(epx_ctrl->reg_data_cnt);
	len = len >> 16;
	for (int i = 0; i < len; i++) {
		tmp = (uint8_t)sys_read32(epx_ctrl->reg_data_buf);
	}
}

static void usb_em32_reset_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	if (sys_test_bit(REG_USB_INT_STA, REG_USB_INT_STA_RST_INT_SF_Pos)) {
		sys_set_bit(REG_USB_INT_STA, REG_USB_INT_STA_RST_INT_SF_CLR_Pos);
	}

	/* After reset, it is necessary to cleanup ep buffer. */
	usb_em32_clean_ep_buf(priv, 1);
	usb_em32_clean_ep_buf(priv, 2);
	usb_em32_clean_ep_buf(priv, 3);
	usb_em32_clean_ep_buf(priv, 4);

	priv->address = 0;
	priv->addressed_state = USB_EM32_NOT_ADDRESSED;
	priv->configured_state = USB_EM32_NOT_CONFIGURED;

#if (IS_IO_ROUTE)
	priv->ep_addr_org = 0;
	priv->ep_addr_new = 0;
#endif

	udc_submit_event(dev, UDC_EVT_RESET, 0);

	return;
}

static void usb_em32_setup_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_msg msg = {0};
	uint32_t index;

	atomic_set((void *)&priv->is_pending_pkg, 0);
	atomic_set((void *)&priv->is_ep0_in_en, 0);
	atomic_set((void *)&priv->is_ep0_out_en, 0);
	atomic_set((void *)&priv->is_ep0_out_pkg, 0);
	atomic_set((void *)&priv->ep0_xfer_size, 0);

	for (index = 0; index < 8; index++) {
		priv->setup_pkg[index] = (uint8_t)sys_read32(REG_EP0_DATA_BUF);
	}

	msg.type = UDC_EM32_MSG_TYPE_SETUP;
	k_msgq_put(priv->msgq, &msg, K_NO_WAIT);
	sys_set_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_SETUP_INT_SF_CLR_Pos);
}

void usb_em32_proc_ep0_h2d(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len;

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	buf = udc_buf_peek(ep_cfg);

	if (!atomic_test_bit((void *)&priv->is_ep0_out_en, 0)) {
		goto exit;
	}

	if (buf == NULL) {
		atomic_set((void *)&priv->is_ep0_out_pkg, 1);
		goto exit;
	}

	bi = udc_get_buf_info(buf);
	if (bi->setup) {
		/* process setup package in setup message handler */
		goto exit;
	}
	if (bi->status) {
		/* complete status package automatically */
		goto exit;
	}

	if (atomic_test_bit((void *)&priv->is_ep0_out_pkg, 0)) {
		goto exit;
	}

	data_ptr = NULL;
	data_len = 0;

	data_ptr = net_buf_tail(buf);
	data_len = net_buf_tailroom(buf);
	len = 0;

	do {
		if (len >= data_len) {
			break;
		}
		if (len >= EP0_MPS) {
			break;
		}
		if (sys_test_bit(REG_EP_BUF_STA, REG_EP_BUF_STA_EP0_OUTBUF_EMPTY_Pos)) {
			break;
		}

		*data_ptr = (uint8_t)sys_read32(REG_EP0_DATA_BUF);

		len += 1;
		data_ptr++;
	} while (1);

	net_buf_add(buf, len);

	if (!sys_test_bit(REG_EP_BUF_STA, REG_EP_BUF_STA_EP0_OUTBUF_EMPTY_Pos)) {
		/* if buffer is not empty, clear it. */
		sys_set_bit(REG_EP0_INT_EN, REG_EP0_INT_EN_BUF_CLR_Pos);
	}

	data_len = net_buf_tailroom(buf);

	if ((len < EP0_MPS) || (data_len == 0)) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

exit:
	sys_set_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_OUT_INT_SF_CLR_Pos);
}

void usb_em32_proc_ep0_d2h(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len, i;
	struct udc_buf_info *bi;

	if (!atomic_test_bit((void *)&priv->is_ep0_in_en, 0)) {
		goto exit;
	}

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	buf = udc_buf_peek(ep_cfg);

	if (buf == NULL) {
		goto exit;
	}

	bi = udc_get_buf_info(buf);
	if (!bi->data) {
		goto exit;
	}

	data_ptr = buf->data;
	data_len = buf->len;

	len = EP0_MPS;
	if (len > data_len) {
		len = data_len;
	}

	for (i = 0; i < len; i++) {
		sys_write32(*data_ptr, REG_EP0_DATA_BUF);
		data_ptr++;
	}

	sys_set_bit(REG_EP0_INT_EN, REG_EP0_INT_EN_DATA_READY_Pos);

	net_buf_pull(buf, len);

	if (priv->ep0_xfer_size > len) {
		priv->ep0_xfer_size = priv->ep0_xfer_size - len;
	} else {
		priv->ep0_xfer_size = 0;
	}

	data_len = buf->len;
	if (data_len != 0) {
		goto exit;
	}

	if (priv->ep0_xfer_size != 0) {
		if (len == EP0_MPS) {
			goto exit;
		}
	}

	buf = udc_buf_get(ep_cfg);
	udc_submit_ep_event(dev, buf, 0);

exit:
	sys_set_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_IN_INT_SF_CLR_Pos);
}

static void usb_em32_proc_epx_d2h(const struct device *dev, uint8_t ep_addr)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct udc_em32_usbd_ep *ep_ctrl;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len, len;

	ep_ctrl = udc_em32_get_ep(priv, ep_addr);
	ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	buf = udc_buf_peek(ep_cfg);

	if (ep_ctrl->is_in_pkg) {
		goto exit;
	}

	if (buf == NULL) {
		ep_ctrl->is_in_pkg = 1;
		sys_clear_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
		goto exit;
	}

	data_ptr = buf->data;
	data_len = buf->len;

	do {
		sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);
	} while (!sys_test_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos));

	len = data_len;
	if (len > EP_MPS) {
		len = EP_MPS;
	}

	sys_write32(len, ep_ctrl->reg_data_cnt);
	if (len > 0) {
		for (int i = 0; i < len; i++) {
			sys_write32(*data_ptr, ep_ctrl->reg_data_buf);
			data_ptr++;
		}
	}
	sys_set_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_DATA_READY_Pos);
	sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);

	net_buf_pull(buf, len);

	data_len = buf->len;
	if (data_len == 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

exit:
	sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_CLR_Pos);
}

static void usb_em32_ep_d2h_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *ep_ctrl;

	if (sys_test_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_IN_INT_SF_Pos)) {
		usb_em32_proc_ep0_d2h(dev);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_IN | 1);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, USB_EP_DIR_IN | 1);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_IN | 2);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, USB_EP_DIR_IN | 2);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_IN | 3);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, USB_EP_DIR_IN | 3);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_IN | 4);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, USB_EP_DIR_IN | 4);
		return;
	}

	return;
}

static void usb_em32_proc_epx_h2d(const struct device *dev, uint8_t ep_addr)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct udc_em32_usbd_ep *ep_ctrl;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len, len, i;

	ep_ctrl = udc_em32_get_ep(priv, ep_addr);
	ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	buf = udc_buf_peek(ep_cfg);

	sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_CLR_Pos);

	if (ep_ctrl->is_out_pkg) {
		return;
	}

	data_ptr = NULL;
	data_len = 0;

	if (buf == NULL) {
		ep_ctrl->is_out_pkg = 1;
		return;
	}

	len = 0;
	data_ptr = net_buf_tail(buf);
	data_len = net_buf_tailroom(buf);

	do {
		sys_set_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);
	} while (!sys_test_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos));

	len = sys_read32(ep_ctrl->reg_data_cnt);
	len = len >> 16;
	if (len > EP_MPS) {
		len = EP_MPS;
	}
	if (len > data_len) {
		len = data_len;
	}

	if (len > 0) {
		for (i = 0; i < len; i++) {
			*data_ptr = (uint8_t)sys_read32(ep_ctrl->reg_data_buf);
			data_ptr++;
		}
	}

	sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos);

	net_buf_add(buf, len);

	data_len = net_buf_tailroom(buf);
	if ((data_len == 0) || (len < EP_MPS)) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}

	return;
}

static void usb_em32_ep_h2d_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *ep_ctrl;

	if (sys_test_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_OUT_INT_SF_Pos)) {
		usb_em32_proc_ep0_h2d(dev);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_OUT | 1);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, USB_EP_DIR_OUT | 1);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_OUT | 2);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, USB_EP_DIR_OUT | 2);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_OUT | 3);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, USB_EP_DIR_OUT | 3);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, USB_EP_DIR_OUT | 4);
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, USB_EP_DIR_OUT | 4);
		return;
	}
}

static int usb_em32_ep_en(struct udc_em32_data *priv, struct udc_ep_config *const cfg)
{
	struct udc_em32_usbd_ep *ep_ctrl;
	uint8_t ep_dir;
	uint8_t ep_idx;

	ep_ctrl = udc_em32_get_ep(priv, cfg->addr);
	ep_dir = USB_EP_GET_DIR(cfg->addr);
	ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	if (ep_idx > 4) {
		return -EINVAL;
	}

	if (ep_dir == USB_EP_DIR_IN) {
		ep_ctrl->is_in_pkg = 0;
		sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_CLR_Pos);
		sys_set_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
	} else {
		ep_ctrl->is_out_pkg = 0;
		sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_CLR_Pos);
		sys_set_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_OUT_INT_EN_Pos);
	}

	if (ep_idx == 1) {
		sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP1_STALL_Pos);
		sys_set_bit(REG_USB_CTRL, REG_USB_CTRL_EP1_EN_Pos);
	} else if (ep_idx == 2) {
		sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP2_STALL_Pos);
		sys_set_bit(REG_USB_CTRL, REG_USB_CTRL_EP2_EN_Pos);
	} else if (ep_idx == 3) {
		sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP3_STALL_Pos);
		sys_set_bit(REG_USB_CTRL, REG_USB_CTRL_EP3_EN_Pos);
	} else if (ep_idx == 4) {
		sys_clear_bit(REG_USB_CTRL_EXT, REG_USB_CTRL_EXT_EP4_STALL_Pos);
		sys_set_bit(REG_USB_CTRL, REG_USB_CTRL_EP4_EN_Pos);
	}

	return 0;
}

static int udc_em32_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	int err = 0;

#if (IS_IO_ROUTE)
	struct udc_ep_config *new_cfg;

	if ((priv->ep_addr_org != 0) && (cfg->addr == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		err = usb_em32_ep_en(priv, new_cfg);
	} else {
		err = usb_em32_ep_en(priv, cfg);
	}
#else
	err = usb_em32_ep_en(priv, cfg);
#endif

	return err;
}

static int usb_em32_ep_off(struct udc_em32_data *priv, struct udc_ep_config *const cfg)
{
	struct udc_em32_usbd_ep *ep_ctrl;
	uint8_t ep_dir;
	uint8_t ep_idx;

	ep_ctrl = udc_em32_get_ep(priv, cfg->addr);
	ep_dir = USB_EP_GET_DIR(cfg->addr);
	ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	if (ep_idx > 4) {
		return -EINVAL;
	}

	if (ep_idx == 1) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP1_EN_Pos);
	} else if (ep_idx == 2) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP2_EN_Pos);
	} else if (ep_idx == 3) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP3_EN_Pos);
	} else if (ep_idx == 4) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP4_EN_Pos);
	}

	if (ep_dir == USB_EP_DIR_IN) {
		ep_ctrl->is_in_pkg = 0;
		sys_clear_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
		sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_CLR_Pos);
	} else {
		ep_ctrl->is_out_pkg = 0;
		sys_clear_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_OUT_INT_EN_Pos);
		sys_set_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_CLR_Pos);
	}

	return 0;
}

static int udc_em32_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	int err = 0;

#if (IS_IO_ROUTE)
	struct udc_ep_config *new_cfg;

	if ((priv->ep_addr_org != 0) && (cfg->addr == priv->ep_addr_org)) {
		new_cfg = udc_get_ep_cfg(dev, priv->ep_addr_new);
		err = usb_em32_ep_off(priv, new_cfg);
	} else {
		err = usb_em32_ep_off(priv, cfg);
	}
#else
	err = usb_em32_ep_off(priv, cfg);
#endif

	return err;
}

static int udc_em32_set_address(const struct device *dev, const uint8_t address)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	priv->address = address;

	return 0;
}

static int udc_em32_enable(const struct device *dev)
{
	usb_em32_sw_connect();

	lock_pm_policy(dev, true);

	return 0;
}

static int udc_em32_disable(const struct device *dev)
{
	usb_em32_sw_disconnect();

	lock_pm_policy(dev, false);

	return 0;
}

static int udc_em32_init(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	int err;

	/* Initialize USBD H/W */
	err = em32_usb_boot(dev);
	if (err != 0) {
		return err;
	}

	err = em32_usb_init(dev);
	if (err != 0) {
		return err;
	}
	usb_em32_sw_disconnect();

	priv->address = 0;
	priv->addressed_state = USB_EM32_NOT_ADDRESSED;
	priv->configured_state = USB_EM32_NOT_CONFIGURED;

#if (IS_IO_ROUTE)
	priv->ep_addr_org = 0;
	priv->ep_addr_new = 0;
#endif

	/* configure and enable ep1 to ep4 */
	udc_em32_epx_init(dev);

	config->irq_enable_func(dev);
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 8, 0)) {
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 8, 0)) {
		return -EIO;
	}

	return 0;
}

static int udc_em32_shutdown(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	int err;

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		return -EIO;
	}

	/* Uninitialize IRQ */
	config->irq_disable_func(dev);

	/* Set SE0 for S/W disconnect */
	usb_em32_sw_disconnect();

	/* Disable USB PHY */
	sys_clear_bit(REG_AIP_USB_PHY, AIP_USB_PHY_CTRL_PD_Pos);

	/* Gating USB clock */
	err = clock_control_off(config->udc_clock.dev, config->udc_clock.subsys);
	if (err != 0) {
		return err;
	}

	/* Purge message queue */
	k_msgq_purge(priv->msgq);

	return 0;
}

static int udc_em32_driver_preinit(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;
	int i;

	/* EM32 Usbd only supports full speed */
	data->caps.hs = false;
	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_8;
	data->caps.out_ack = true;
	data->caps.can_detect_vbus = false;

	config->ep_cfg_out[0].caps.out = 1;
	config->ep_cfg_out[0].caps.control = 1;
	config->ep_cfg_out[0].caps.mps = EP0_MPS;
	config->ep_cfg_out[0].addr = USB_EP_DIR_OUT | 0;
	err = udc_register_ep(dev, &config->ep_cfg_out[0]);
	if (err != 0) {
		return err;
	}

	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = EP0_MPS;
	config->ep_cfg_in[0].addr = USB_EP_DIR_IN | 0;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err != 0) {
		return err;
	}

	/* The em32 usbd supports four endpoints (excluding ep0). Initialize and
	 * register it. */
	for (i = 1; i <= 4; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		config->ep_cfg_out[i].caps.interrupt = 1;
		config->ep_cfg_out[i].caps.bulk = 1;
		config->ep_cfg_out[i].caps.iso = 1;
		config->ep_cfg_out[i].caps.mps = 1023;
		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			return err;
		}
	}

	for (i = 1; i <= 4; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		config->ep_cfg_in[i].caps.interrupt = 1;
		config->ep_cfg_in[i].caps.bulk = 1;
		config->ep_cfg_in[i].caps.iso = 1;
		config->ep_cfg_in[i].caps.mps = 1023;
		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			return err;
		}
	}

	/* init priv data */
	priv->msgq = config->msgq;
	priv->is_ep0_out_pkg = 0;
	priv->is_pending_pkg = 0;
	priv->is_ep0_in_en = 0;
	priv->is_ep0_out_en = 0;
	priv->ep0_xfer_size = 0;
	priv->configured_state = USB_EM32_NOT_CONFIGURED;
	priv->addressed_state = USB_EM32_NOT_ADDRESSED;
	priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_NOT_ISSUE;

	config->make_thread(dev);

	LOG_INF("UDC: Device %p (max. speed %d)", dev, config->speed_idx);

	return 0;
}

static void udc_em32_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_em32_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static enum udc_bus_speed udc_em32_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;
	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static const struct udc_api udc_em32_api = {
	.device_speed = udc_em32_device_speed,
	.ep_enqueue = udc_em32_ep_enqueue,
	.ep_dequeue = udc_em32_ep_dequeue,
	.ep_set_halt = udc_em32_ep_set_halt,
	.ep_clear_halt = udc_em32_ep_clear_halt,
	.ep_enable = udc_em32_ep_enable,
	.ep_disable = udc_em32_ep_disable,
	.host_wakeup = udc_em32_host_wakeup,
	.set_address = udc_em32_set_address,
	.enable = udc_em32_enable,
	.disable = udc_em32_disable,
	.init = udc_em32_init,
	.shutdown = udc_em32_shutdown,
	.lock = udc_em32_lock,
	.unlock = udc_em32_unlock,
	.test_mode = NULL,
};

#define UDC_EM32_CLOCK_DT_INIT(inst, name)                                                         \
	{                                                                                            \
		.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, name)),                        \
		.subsys = UINT_TO_POINTER(DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clk_id)),           \
	}

#define UDC_EM32_DEVICE_DEFINE(inst)                                                               \
	K_THREAD_STACK_DEFINE(udc_em32_stack_##inst, CONFIG_UDC_EM32_STACK_SIZE);                  \
                                                                                                   \
	static void udc_em32_irq_enable_func(const struct device *dev)                             \
	{                                                                                          \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, setup, irq),                         \
				    DT_INST_IRQ_BY_NAME(inst, setup, priority),                        \
				    (void (*)(const void *))usb_em32_setup_isr,                    \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, suspend, irq),                       \
				    DT_INST_IRQ_BY_NAME(inst, suspend, priority),                      \
				    (void (*)(const void *))usb_em32_suspend_isr,                  \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, resume, irq),                        \
				    DT_INST_IRQ_BY_NAME(inst, resume, priority),                       \
				    (void (*)(const void *))usb_em32_resume_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, reset, irq),                         \
				    DT_INST_IRQ_BY_NAME(inst, reset, priority),                        \
				    (void (*)(const void *))usb_em32_reset_isr,                    \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, ep_in, irq),                         \
				    DT_INST_IRQ_BY_NAME(inst, ep_in, priority),                        \
				    (void (*)(const void *))usb_em32_ep_d2h_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_connect_dynamic(DT_INST_IRQ_BY_NAME(inst, ep_out, irq),                        \
				    DT_INST_IRQ_BY_NAME(inst, ep_out, priority),                       \
				    (void (*)(const void *))usb_em32_ep_h2d_isr,                   \
				    DEVICE_DT_INST_GET(inst), 0);                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, setup, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, suspend, irq));                                \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, resume, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, reset, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ep_in, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ep_out, irq));                                 \
	}                                                                                          \
                                                                                                   \
	static void udc_em32_irq_disable_func(const struct device *dev)                            \
	{                                                                                          \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, setup, irq));                                 \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, suspend, irq));                               \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, resume, irq));                                \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, reset, irq));                                 \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, ep_in, irq));                                 \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, ep_out, irq));                                \
	}                                                                                          \
                                                                                                   \
	static void udc_em32_thread_##inst(void *dev, void *arg1, void *arg2)                      \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		em32_usbd_msg_handler(dev);                                                        \
	}                                                                                          \
                                                                                                   \
	static void udc_em32_make_thread(const struct device *dev)                                 \
	{                                                                                          \
		struct udc_em32_data *priv = udc_get_private(dev);                                 \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_em32_stack_##inst,                         \
				K_THREAD_STACK_SIZEOF(udc_em32_stack_##inst),                      \
				udc_em32_thread_##inst, (void *)dev, NULL, NULL,                   \
				K_PRIO_COOP(CONFIG_UDC_EM32_THREAD_PRIORITY), K_ESSENTIAL,         \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out_##inst[USB_NUM_BIDIR_ENDPOINTS];                    \
	static struct udc_ep_config ep_cfg_in_##inst[USB_NUM_BIDIR_ENDPOINTS];                     \
                                                                                                   \
	K_MSGQ_DEFINE(em32_usbd_msgq_##inst, sizeof(struct udc_em32_msg),                          \
		      CONFIG_UDC_EM32_MSG_QUEUE_SIZE, 4);                                          \
                                                                                                   \
	static const struct udc_em32_config udc_em32_config_##inst = {                             \
		.num_of_eps = USB_NUM_BIDIR_ENDPOINTS,                                             \
		.ep_cfg_in = ep_cfg_in_##inst,                                                     \
		.ep_cfg_out = ep_cfg_out_##inst,                                                   \
		.ep_cfg_out_size = ARRAY_SIZE(ep_cfg_out_##inst),                                  \
		.ep_cfg_in_size = ARRAY_SIZE(ep_cfg_in_##inst),                                    \
		.make_thread = udc_em32_make_thread,                                               \
		.speed_idx = UDC_BUS_SPEED_FS,                                                     \
		.irq_enable_func = udc_em32_irq_enable_func,                                       \
		.irq_disable_func = udc_em32_irq_disable_func,                                     \
		.msgq = &em32_usbd_msgq_##inst,                                                    \
		.aip_clock = UDC_EM32_CLOCK_DT_INIT(inst, aip),                                    \
		.udc_clock = UDC_EM32_CLOCK_DT_INIT(inst, udc),                                    \
		.atrim_clock = UDC_EM32_CLOCK_DT_INIT(inst, atrim),                                \
	};                                                                                         \
                                                                                                   \
	static struct udc_em32_data em32_udc_priv_##inst;                                          \
                                                                                                   \
	static struct udc_data em32_udc_data_##inst = {                                            \
		.mutex = Z_MUTEX_INITIALIZER(em32_udc_data_##inst.mutex),                          \
		.priv = &em32_udc_priv_##inst};                                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, udc_em32_driver_preinit, NULL, &em32_udc_data_##inst,          \
			      &udc_em32_config_##inst, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_em32_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_EM32_DEVICE_DEFINE)
