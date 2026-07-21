/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Access USB device controller from devicetree */
#define DT_DRV_COMPAT elan_em32_usbd

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/usb/udc.h>
#include <../drivers/usb/udc/udc_common.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_em32, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define USB_NUM_BIDIR_ENDPOINTS 5
#define EP0_MPS                 8
#define EP_MPS                  64

struct udc_em32_irq_mapping {
	uint32_t irq_num;
	void (*isr_handler)(const struct device *dev);
};

/* Obtain the IRQ number used by the USB from the device tree and
 * establish an association with the ISR.
 */
static const struct udc_em32_irq_mapping em32_irq_table[6];

enum em32_event_type {
	/* Received setup request from host */
	EM32_EVT_SETUP,
	/* Trigger transmission event */
	EM32_EVT_XFER,
	/* Issue PWR event to process set/clear feature patch for remote wakeup */
	EM32_EVT_PWR,
};

/* The transmission type is set in the xfer_type variable,
 * which indicates which pipe the event occurred on.
 */
enum em32_xfer_type {
	XFER_T_EP0_IN = 0,
	XFER_T_EP1_IN,
	XFER_T_EP2_IN,
	XFER_T_EP3_IN,
	XFER_T_EP4_IN,
	XFER_T_EP0_OUT,
	XFER_T_EP1_OUT,
	XFER_T_EP2_OUT,
	XFER_T_EP3_OUT,
	XFER_T_EP4_OUT,
};

/* Because the hardware layer automatically handles the set address request and
 * does not notify the firmware;
 * A set address patch must be executed on the upper layer
 * to complete the set address action.
 */
enum em32_address_state {
	/* device is not addressed */
	USB_EM32_NOT_ADDRESSED = 0,
	/* The patch for the set-address request has been marked. */
	USB_EM32_SET_ADDRESS_START = 1,
	/* start to do set-address patch, send the set-address request to upper layer */
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
enum em32_configuration_state {
	/* device is not configured */
	USB_EM32_NOT_CONFIGURED = 0,
	/* The patch for the set-configuration request has been marked. */
	USB_EM32_SET_CONFIGURATION_START = 1,
	/* start to do set-conf patch, send the set-configuration request to upper layer */
	USB_EM32_SET_CONFIGURATION_PROCESS = 2,
	/* Received the status package from the upper level
	 * and completed the set-configuration request.
	 */
	USB_EM32_SET_CONFIGURATION_DONE = 3,
};

/* Because the hardware layer automatically handles set/clear feature requests,
 * the firmware must perform set/clear feature path processing at the following points:
 * Set feature for remote wakeup:
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

struct udc_em32_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	uint32_t ep_cfg_out_size;
	uint32_t ep_cfg_in_size;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
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
	/**
	 * @brief Pointer to the APB clock control device.
	 *
	 * FIXME: Set to NULL temporarily because the SoC clock control driver
	 * is missing in the upstream tree. Restore to DEVICE_DT_GET(DT_NODELABEL(clk_apb))
	 * once the SoC clock infrastructure is ready.
	 */
	const struct device *clock_dev;
	/* Retrieve the clock IDs from the DTS settings. */
	const uint32_t clk_id_udc;
	const uint32_t clk_id_atrim;
	const uint32_t clk_id_aip;
	/* Events are the way to communicate with the driver thread. */
	struct k_event events;
	/* xfer_type specifies which endpoints are used for the transfer. */
	atomic_t xfer_type;
	/* Setup package being processed */
	struct usb_setup_packet setup_pkt;
	/* When patch processing begins, the UDC driver places the newly arrived setup package
	 * in the pending_setup_pkt and sets the is_pending_pkg flag.
	 * The UDC driver then places the patch command in the setup_pkt and begins processing.
	 */
	struct usb_setup_packet pending_setup_pkt;
	/* Device address */
	uint8_t address;
	struct k_thread thread_data;
#if defined(CONFIG_IO_ROUTE)
	/* If CONFIG_IO_ROUTE is defined, the original out ep will be moved to a new out ep.
	 * This is a workaround for a specific project.
	 */
	uint8_t ep_out_num;
	uint8_t ep_out_num_new;
#endif
	/* If it is 1, pending_setup_pkt waits to be processed. */
	uint32_t is_pending_pkg: 1;
	/* If is_ep0_out_pkg is 1, it means that the ep buf does not exist
	 * when the out interrupt occurs.
	 * The out request will be processed in the normal code.
	 */
	uint32_t is_ep0_out_pkg: 1;
	/* When a new setup isr receives a new setup package,
	 * it clears is_ep0_in_en and is_ep0_out_en flags.
	 * is_ep0_in_en and is_ep0_out_en flags are only set
	 * after the sent setup message has been processed, allowing IN and OUT I/O requests.
	 */
	uint32_t is_ep0_in_en: 1;
	uint32_t is_ep0_out_en: 1;
	/* The source of the remote wakeup request */
	enum udc_em32_remote_wakeup_state proc_remote_wakeup_state;
	/* Size of transfer of package processed currently */
	uint32_t ep0_xfer_size;
	/* addressed_state:
	 * This is used to control the state of the standard set-address command.
	 * The UDC driver sends this command to the upper layer driver
	 * to complete the standard set-address action.
	 */
	enum em32_address_state addressed_state;
	/* configured_state:
	 * This is used to control the state of the standard set-configuration command.
	 * The UDC driver sends this command to the upper layer driver
	 * to complete the standard set-configuration action.
	 */
	enum em32_configuration_state configured_state;
	/* Control units from ep1 to ep4 */
	struct udc_em32_usbd_ep epx_ctrl[USB_NUM_BIDIR_ENDPOINTS - 1];
#if defined(CONFIG_PM)
	uint8_t pm_policy_locked;
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
static inline void em32_set_usb_clk_prop(void)
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
static const unsigned char usb_ep_conf_data[6] = {0x43, 0x43, 0x42, 0x42, 0xFA, 0x00};
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
	if ((ep_idx >= USB_NUM_BIDIR_ENDPOINTS) || (ep_idx == 0)) {
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
static inline void usb_em32_sw_connect(void)
{
	sys_set_bit(REG_AIP_USB_PHY, AIP_USB_PHY_CTRL_RSW_Pos);
}

#if defined(CONFIG_PM)
static void lock_pm_policy(const struct device *dev, uint8_t lock)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	unsigned int irq_lock_key = arch_irq_lock();

	if (priv->pm_policy_locked != lock) {
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
		priv->pm_policy_locked = lock;
	}
	arch_irq_unlock(irq_lock_key);
}
#else
/* Stub: Do nothing if PM not enabled */
#define lock_pm_policy(dev, lock)
#endif

/* Start USB PHY and get it working. */
static int em32_usb_boot(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	int ret;

	ret = clock_control_on(priv->clock_dev, UINT_TO_POINTER(priv->clk_id_aip));
	if (ret < 0) {
		LOG_ERR("Failed to enable aip clock: %d", ret);
		return ret;
	}

	em32_set_usb_clk_prop();

	/* select usb clock source, then power on usb pll */
	em32_set_usb_pll_src_irc();
	ret = clock_control_on(priv->clock_dev, UINT_TO_POINTER(priv->clk_id_udc));
	if (ret < 0) {
		LOG_ERR("Failed to enable udc clock: %d", ret);
		return ret;
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
	struct udc_em32_data *priv = udc_get_private(dev);
	int ret;

	em32_usb_phy_setup();

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

	ret = clock_control_on(priv->clock_dev, UINT_TO_POINTER(priv->clk_id_atrim));
	if (ret < 0) {
		LOG_ERR("Failed to enable atrim clock: %d", ret);
		return ret;
	}

	return 0;
}

/* Configure control units from ep1 to ep4. */
static void em32_usb_epx_init(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *epx_ctrl;

	epx_ctrl = udc_em32_get_ep(priv, 1);
	if (epx_ctrl == NULL) {
		return;
	}

	epx_ctrl->idx = 1;
	epx_ctrl->is_in_pkg = 0;
	epx_ctrl->is_out_pkg = 0;
	epx_ctrl->reg_ep_int_en = REG_EP1_INT_EN;
	epx_ctrl->reg_ep_int_sta = REG_EP1_INT_STA;
	epx_ctrl->reg_data_cnt = REG_EP1_DATA_CNT;
	epx_ctrl->reg_data_buf = REG_EP1_DATA_BUF;

	epx_ctrl = udc_em32_get_ep(priv, 2);
	if (epx_ctrl == NULL) {
		return;
	}

	epx_ctrl->idx = 2;
	epx_ctrl->is_in_pkg = 0;
	epx_ctrl->is_out_pkg = 0;
	epx_ctrl->reg_ep_int_en = REG_EP2_INT_EN;
	epx_ctrl->reg_ep_int_sta = REG_EP2_INT_STA;
	epx_ctrl->reg_data_cnt = REG_EP2_DATA_CNT;
	epx_ctrl->reg_data_buf = REG_EP2_DATA_BUF;

	epx_ctrl = udc_em32_get_ep(priv, 3);
	if (epx_ctrl == NULL) {
		return;
	}

	epx_ctrl->idx = 3;
	epx_ctrl->is_in_pkg = 0;
	epx_ctrl->is_out_pkg = 0;
	epx_ctrl->reg_ep_int_en = REG_EP3_INT_EN;
	epx_ctrl->reg_ep_int_sta = REG_EP3_INT_STA;
	epx_ctrl->reg_data_cnt = REG_EP3_DATA_CNT;
	epx_ctrl->reg_data_buf = REG_EP3_DATA_BUF;

	epx_ctrl = udc_em32_get_ep(priv, 4);
	if (epx_ctrl == NULL) {
		return;
	}

	epx_ctrl->idx = 4;
	epx_ctrl->is_in_pkg = 0;
	epx_ctrl->is_out_pkg = 0;
	epx_ctrl->reg_ep_int_en = REG_EP4_INT_EN;
	epx_ctrl->reg_ep_int_sta = REG_EP4_INT_STA;
	epx_ctrl->reg_data_cnt = REG_EP4_DATA_CNT;
	epx_ctrl->reg_data_buf = REG_EP4_DATA_BUF;
}

#if defined(CONFIG_IO_ROUTE)
static void get_out_pipe_num(const struct device *dev, struct net_buf *buf)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint8_t *ptr;
	uint8_t *buf_end;
	uint32_t len;

	ptr = buf->data;
	len = buf->len;

	if (len <= 9) {
		return;
	}

	/* Move the `out ep` in the configuration descriptor from `ep 1` to `ep 3`. */
	if ((ptr[0] == sizeof(struct usb_cfg_descriptor)) && (ptr[1] == USB_DESC_CONFIGURATION)) {
		buf_end = ptr + len;
		/* Move pointer to first interface descriptor in this configuration. */
		ptr = ptr + ptr[0];
		while (ptr < buf_end) {
			/* Find the first OUT ep descriptor,
			 * and then change it to the actual operating ep index.
			 */
			if ((ptr[0] == sizeof(struct usb_ep_descriptor)) &&
			    (ptr[1] == USB_DESC_ENDPOINT) && USB_EP_DIR_IS_OUT(ptr[2])) {
				priv->ep_out_num = ptr[2];
				priv->ep_out_num_new = 3;
				ptr[2] = priv->ep_out_num_new;
			}
			/* Move pointer to next descriptor */
			ptr = ptr + ptr[0];
		}
	}
}
#endif

static inline uint32_t em32_get_xfer_type(uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		if (USB_EP_GET_IDX(ep) == 0) {
			return XFER_T_EP0_IN;
		} else if (USB_EP_GET_IDX(ep) == 1) {
			return XFER_T_EP1_IN;
		} else if (USB_EP_GET_IDX(ep) == 2) {
			return XFER_T_EP2_IN;
		} else if (USB_EP_GET_IDX(ep) == 3) {
			return XFER_T_EP3_IN;
		} else if (USB_EP_GET_IDX(ep) == 4) {
			return XFER_T_EP4_IN;
		}
	} else {
		if (USB_EP_GET_IDX(ep) == 0) {
			return XFER_T_EP0_OUT;
		} else if (USB_EP_GET_IDX(ep) == 1) {
			return XFER_T_EP1_OUT;
		} else if (USB_EP_GET_IDX(ep) == 2) {
			return XFER_T_EP2_OUT;
		} else if (USB_EP_GET_IDX(ep) == 3) {
			return XFER_T_EP3_OUT;
		} else if (USB_EP_GET_IDX(ep) == 4) {
			return XFER_T_EP4_OUT;
		}
	}

	__ASSERT(false, "Invalid endpoint number for IN/OUT transaction.");

	return -EINVAL;
}

static int udc_em32_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			       struct net_buf *buf)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint8_t ep = cfg->addr;
#if defined(CONFIG_IO_ROUTE)
	struct udc_ep_config *new_cfg;
#endif

#if defined(CONFIG_IO_ROUTE)
	if (ep == USB_CONTROL_EP_IN) {
		get_out_pipe_num(dev, buf);
	}

	if ((priv->ep_out_num != 0) && (ep == priv->ep_out_num)) {
		/* Move the packet from the original pipe to new pipe. */
		new_cfg = udc_get_ep_cfg(dev, priv->ep_out_num_new);
		udc_buf_put(new_cfg, buf);
		ep = priv->ep_out_num_new;
	} else {
		udc_buf_put(cfg, buf);
	}
#else
	udc_buf_put(cfg, buf);
#endif

	if (cfg->stat.halted) {
		return 0;
	}

	atomic_set_bit(&priv->xfer_type, em32_get_xfer_type(ep));
	k_event_post(&priv->events, BIT(EM32_EVT_XFER));

	return 0;
}

static int udc_em32_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	udc_ep_cancel_queued(dev, ep_cfg);
	return 0;
}

static void em32_usb_ep_set_halt(struct udc_ep_config *const cfg, bool isHalt)
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
		} else {
			return;
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
		} else {
			return;
		}
	}
}

/* Halt endpoint. Halted endpoint should respond with a STALL handshake. */
static int udc_em32_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	em32_usb_ep_set_halt(cfg, true);
	return 0;
}

/*
 * Opposite to halt endpoint. If there are requests in the endpoint queue,
 * the next transfer should be prepared.
 */
static int udc_em32_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	em32_usb_ep_set_halt(cfg, false);
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

	if (priv->is_pending_pkg) {
		/* Copy the request from pending_setup_pkt to setup_pkt,
		 *  then clear the is_pending_pkg flag.
		 * Send the EM32_EVT_SETUP event to
		 *  process the request from setup_pkt.
		 */
		memcpy(&priv->setup_pkt, &priv->pending_setup_pkt, sizeof(struct usb_setup_packet));
		priv->is_pending_pkg = 0;

		k_event_post(&priv->events, BIT(EM32_EVT_SETUP));
	}
}

/* The EM32 USB PHY automatically handles set-address and set-configuration requests.
 * On Zephyr systems, the UDC driver must provide patches to
 * send these requests to the upper layer.
 * The `do_patch_proc` function is responsible for handling these patches.
 */
static int do_patch_proc(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint16_t required_length;
	uint16_t type;

	required_length = sizeof(struct usb_device_descriptor);
	type = (priv->setup_pkt.wValue >> 8) & 0x00FF;

	if (priv->addressed_state == USB_EM32_NOT_ADDRESSED) {
		if ((priv->setup_pkt.RequestType.direction == USB_REQTYPE_DIR_TO_HOST) &&
		    (priv->setup_pkt.RequestType.type == USB_REQTYPE_TYPE_STANDARD) &&
		    (priv->setup_pkt.RequestType.recipient == USB_REQTYPE_RECIPIENT_DEVICE) &&
		    (priv->setup_pkt.bRequest == USB_SREQ_GET_DESCRIPTOR) &&
		    (type == USB_DESC_DEVICE) && (priv->setup_pkt.wLength >= required_length)) {
			/* When a get device descriptor request is received,
			 * and its length is at least the length of usb_device_descriptor,
			 * the set-address patching action begins.
			 */
			priv->addressed_state = USB_EM32_SET_ADDRESS_START;
			return 0;
		}
	} else if (priv->addressed_state == USB_EM32_SET_ADDRESS_START) {
		/* When the set-address patch begins execution:
		 * 1. The current request is placed in the pending_setup_pkt for delayed processing.
		 * 2. The set-address request is set as the currently processing request and
		 *    sent out for processing.
		 */
		priv->addressed_state = USB_EM32_SET_ADDRESS_PROCESS;

		memcpy(&priv->pending_setup_pkt, &priv->setup_pkt, sizeof(struct usb_setup_packet));
		priv->is_pending_pkg = 1;

		/* Set address request, the address is always set to 0xf */
		memset(&priv->setup_pkt, 0, sizeof(struct usb_setup_packet));
		priv->setup_pkt.bRequest = USB_SREQ_SET_ADDRESS;
		priv->setup_pkt.wValue = 0x0f;

		udc_setup_received(dev, &priv->setup_pkt);
		return 1;
	} else {
		return 0;
	}

	/* The operation method for `set-configuration patch` is the same
	 * as that for `set-address patch`.
	 */
	required_length = sizeof(struct usb_cfg_descriptor);
	if (priv->configured_state == USB_EM32_NOT_CONFIGURED) {
		if ((priv->setup_pkt.RequestType.direction == USB_REQTYPE_DIR_TO_HOST) &&
		    (priv->setup_pkt.RequestType.type == USB_REQTYPE_TYPE_STANDARD) &&
		    (priv->setup_pkt.RequestType.recipient == USB_REQTYPE_RECIPIENT_DEVICE) &&
		    (priv->setup_pkt.bRequest == USB_SREQ_GET_DESCRIPTOR) &&
		    (type == USB_DESC_CONFIGURATION) &&
		    (priv->setup_pkt.wLength > required_length)) {
			/* When a get configuration descriptor request is received,
			 * and its length is greater than the length of usb_cfg_descriptor,
			 * the set-configuration patching action begins.
			 */
			priv->configured_state = USB_EM32_SET_CONFIGURATION_START;
			return 0;
		}
	} else if (priv->configured_state == USB_EM32_SET_CONFIGURATION_START) {
		priv->configured_state = USB_EM32_SET_CONFIGURATION_PROCESS;

		memcpy(&priv->pending_setup_pkt, &priv->setup_pkt, sizeof(struct usb_setup_packet));
		priv->is_pending_pkg = 1;

		/* Standard set configuration request */
		memset(&priv->setup_pkt, 0, sizeof(struct usb_setup_packet));
		priv->setup_pkt.bRequest = USB_SREQ_SET_CONFIGURATION;
		priv->setup_pkt.wValue = 0x01;

		udc_setup_received(dev, &priv->setup_pkt);
		return 1;
	} else {
		return 0;
	}

	return 0;
}

/* The EM32 USB PHY automatically handles the remote wakeup settings sent by the host.
 * The UDC driver must send the remote wakeup settings patch
 *  to the upper layer at the appropriate time.
 * The UDC driver will process this patch when the following conditions are met:
 *  1. The device has been configured.
 *  2. A suspend/resume signal has been received.
 */
static int em32_set_remote_wakeup_handler(const struct device *dev, uint32_t isSet)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	priv->is_pending_pkg = 0;
	memset(&priv->pending_setup_pkt, 0, sizeof(struct usb_setup_packet));
	memset(&priv->setup_pkt, 0, sizeof(struct usb_setup_packet));

	/* Only do remote-wakeup patch when device is configured */
	if (priv->configured_state < USB_EM32_SET_CONFIGURATION_DONE) {
		return 0;
	}

	/* Send a PWR message to notify the udc driver that
	 *  it has received a suspend or resume signal.
	 */
	if (isSet) {
		priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND;
	} else {
		priv->proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_SRC_RESUME;
	}
	k_event_post(&priv->events, BIT(EM32_EVT_PWR));

	return 1;
}

/* Event handler for setup package */
static void em32_setup_evt_handler(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint16_t xfer_size;
	struct udc_ep_config *ep_ctrl_in;
	struct udc_ep_config *ep_ctrl_out;

	xfer_size = priv->setup_pkt.wLength;

	/* If a patch must be executed, this setup request will be pending.
	 * The setup request will be completed only after the patch has finished executing.
	 */
	if (do_patch_proc(dev)) {
		return;
	}

	ep_ctrl_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	ep_ctrl_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);

	udc_ep_set_busy(ep_ctrl_in, false);
	udc_ep_set_busy(ep_ctrl_out, false);

	em32_usb_ep_set_halt(ep_ctrl_in, false);
	em32_usb_ep_set_halt(ep_ctrl_out, false);

	udc_setup_received(dev, &priv->setup_pkt);
	priv->ep0_xfer_size = xfer_size;
	priv->is_ep0_in_en = 1;
	priv->is_ep0_out_en = 1;
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
	uint32_t is_out_pkg_in;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	buf = udc_buf_peek(ep_cfg);

	if (buf == NULL) {
		return 0;
	}

	bi = udc_get_buf_info(buf);
	if (bi->setup) {
		/* setup package is processed in EM32_EVT_SETUP event handler */
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
	is_out_pkg_in = 0;
	if (priv->is_ep0_out_pkg) {
		is_out_pkg_in = 1;
	}

	if (!is_out_pkg_in) {
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
	} while (true);

	net_buf_add(buf, len);
	data_len = net_buf_tailroom(buf);

	if ((len < EP0_MPS) || (data_len == 0)) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, 0);
	}
	priv->is_ep0_out_pkg = 0;

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
			 * continue processing pending setup package.
			 */
			re_issue_pending_pkg(dev);
			return 0;
		} else if (priv->configured_state == USB_EM32_SET_CONFIGURATION_PROCESS) {
			/* This status transaction is for set-configuration patch */
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);
			priv->configured_state = USB_EM32_SET_CONFIGURATION_DONE;
			/* while set-configuration patch is done,
			 * continue processing pending setup package.
			 */
			re_issue_pending_pkg(dev);
			return 0;
		} else if (priv->proc_remote_wakeup_state) {
			/* This status transaction is for remote-wakeup patch */
			buf = udc_buf_get(ep_cfg);
			udc_submit_ep_event(dev, buf, 0);

			if (priv->proc_remote_wakeup_state == USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND) {
				/* After completing the remote-wakeup patch,
				 * a suspend event must be sent to notify the upper layer.
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
	if ((data_len == 0) || (len < EP_MPS)) {
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

	/* Process IN transaction in isr. */
	if (ep_ctrl->is_in_pkg) {
		sys_clear_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
		ep_ctrl->is_in_pkg = 0;
		sys_set_bit(ep_ctrl->reg_ep_int_en, REG_EPX_INT_EN_IN_INT_EN_Pos);
	}

	return 0;
}

/* Event handler for queued transfer */
static void em32_xfer_evt_handler(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	atomic_t eps;

	eps = atomic_clear(&priv->xfer_type);

	while (eps) {
		/* Process ep0 ctrl request */
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP0_IN)) {
			udc_em32_ctrl_handler(dev, USB_CONTROL_EP_IN);
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP0_OUT)) {
			udc_em32_ctrl_handler(dev, USB_CONTROL_EP_OUT);
		}

		/* Process IN transaction */
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP1_IN)) {
			udc_em32_xfer_in(dev, (USB_EP_DIR_IN | 1));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP2_IN)) {
			udc_em32_xfer_in(dev, (USB_EP_DIR_IN | 2));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP3_IN)) {
			udc_em32_xfer_in(dev, (USB_EP_DIR_IN | 3));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP4_IN)) {
			udc_em32_xfer_in(dev, (USB_EP_DIR_IN | 4));
		}

		/* Process OUT transaction */
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP1_OUT)) {
			udc_em32_xfer_out(dev, (USB_EP_DIR_OUT | 1));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP2_OUT)) {
			udc_em32_xfer_out(dev, (USB_EP_DIR_OUT | 2));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP3_OUT)) {
			udc_em32_xfer_out(dev, (USB_EP_DIR_OUT | 3));
		}
		if (atomic_test_and_clear_bit(&eps, XFER_T_EP4_OUT)) {
			udc_em32_xfer_out(dev, (USB_EP_DIR_OUT | 4));
		}
	}
}

/* The EM32 USB PHY automatically handles the remote wakeup settings sent by the host.
 * This handler does remote-wakeup patch while suspend or resume signal is happened.
 */
static void em32_pwr_evt_handler(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);

	/* set/clear feature request for remote wakeup */
	memset(&priv->setup_pkt, 0, sizeof(struct usb_setup_packet));
	if (priv->proc_remote_wakeup_state == USB_REMOTE_WAKEUP_REQ_SRC_SUSPEND) {
		priv->setup_pkt.bRequest = USB_SREQ_SET_FEATURE;
	} else if (priv->proc_remote_wakeup_state == USB_REMOTE_WAKEUP_REQ_SRC_RESUME) {
		priv->setup_pkt.bRequest = USB_SREQ_CLEAR_FEATURE;
	} else {
		/* This should not happened */
		__ASSERT(false, "Unexpected unknown power event sources.");
	}

	k_event_post(&priv->events, BIT(EM32_EVT_SETUP));
}

static void em32_usbd_event_handler(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint32_t evt;

	while (true) {
		evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
		udc_lock_internal(dev, K_FOREVER);

		if (evt & BIT(EM32_EVT_SETUP)) {
			k_event_clear(&priv->events, BIT(EM32_EVT_SETUP));
			em32_setup_evt_handler(dev);
		}

		if (evt & BIT(EM32_EVT_XFER)) {
			k_event_clear(&priv->events, BIT(EM32_EVT_XFER));
			em32_xfer_evt_handler(dev);
		}

		if (evt & BIT(EM32_EVT_PWR)) {
			k_event_clear(&priv->events, BIT(EM32_EVT_PWR));
			em32_pwr_evt_handler(dev);
		}

		udc_unlock_internal(dev);
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
}

static void usb_em32_clean_ep_buf(struct udc_em32_data *priv, uint8_t ep)
{
	struct udc_em32_usbd_ep *epx_ctrl;
	int len, i;
	uint8_t tmp;

	if ((ep < 1) || (ep > 4)) {
		return;
	}

	epx_ctrl = udc_em32_get_ep(priv, ep);

	sys_set_bit(epx_ctrl->reg_ep_int_en, REG_EPX_INT_EN_BUF_CLR_Pos);
	sys_clear_bit(epx_ctrl->reg_ep_int_en, REG_EPX_INT_EN_BUF_CLR_Pos);
	len = sys_read32(epx_ctrl->reg_data_cnt);
	len = len >> 16;
	for (i = 0; i < len; i++) {
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

#if defined(CONFIG_IO_ROUTE)
	priv->ep_out_num = 0;
	priv->ep_out_num_new = 0;
#endif

	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void usb_em32_setup_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	uint8_t *setup_pkt;
	uint32_t index;

	priv->is_pending_pkg = 0;
	priv->is_ep0_in_en = 0;
	priv->is_ep0_out_en = 0;
	priv->is_ep0_out_pkg = 0;
	priv->ep0_xfer_size = 0;

	setup_pkt = (uint8_t *)&priv->setup_pkt;
	for (index = 0; index < sizeof(struct usb_setup_packet); index++) {
		setup_pkt[index] = (uint8_t)sys_read32(REG_EP0_DATA_BUF);
	}

	k_event_post(&priv->events, BIT(EM32_EVT_SETUP));
	sys_set_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_SETUP_INT_SF_CLR_Pos);
}

static void usb_em32_proc_ep0_h2d(const struct device *dev)
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

	if (!(priv->is_ep0_out_en)) {
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

	if (priv->is_ep0_out_pkg) {
		goto exit;
	}

	data_ptr = NULL;
	data_len = 0;

	if (buf == NULL) {
		priv->is_ep0_out_pkg = 1;
		goto exit;
	}

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
	} while (true);

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

/* Process ep0 IN transaction in ISR */
void usb_em32_proc_ep0_d2h(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t len, i;
	struct udc_buf_info *bi;

	if (!(priv->is_ep0_in_en)) {
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

	if ((priv->ep0_xfer_size != 0) && (len == EP0_MPS)) {
		goto exit;
	}

	buf = udc_buf_get(ep_cfg);
	udc_submit_ep_event(dev, buf, 0);

exit:
	sys_set_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_IN_INT_SF_CLR_Pos);
}

/* Process EPx(EP1 to EP4) IN transactions in ISR */
static void usb_em32_proc_epx_d2h(const struct device *dev, uint8_t ep_addr)
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
		for (i = 0; i < len; i++) {
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

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_IN | 1));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, (USB_EP_DIR_IN | 1));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_IN | 2));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, (USB_EP_DIR_IN | 2));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_IN | 3));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, (USB_EP_DIR_IN | 3));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_IN | 4));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_IN_INT_SF_Pos)) {
		usb_em32_proc_epx_d2h(dev, (USB_EP_DIR_IN | 4));
		return;
	}
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
}

static void usb_em32_ep_h2d_isr(const struct device *dev)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *ep_ctrl;

	if (sys_test_bit(REG_EP0_INT_STA, REG_EP0_INT_STA_EP0_OUT_INT_SF_Pos)) {
		usb_em32_proc_ep0_h2d(dev);
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_OUT | 1));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, (USB_EP_DIR_OUT | 1));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_OUT | 2));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, (USB_EP_DIR_OUT | 2));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_OUT | 3));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, (USB_EP_DIR_OUT | 3));
		return;
	}

	ep_ctrl = udc_em32_get_ep(priv, (USB_EP_DIR_OUT | 4));
	if (sys_test_bit(ep_ctrl->reg_ep_int_sta, REG_EPX_INT_STA_OUT_INT_SF_Pos)) {
		usb_em32_proc_epx_h2d(dev, (USB_EP_DIR_OUT | 4));
		return;
	}
}

static int udc_em32_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
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
	} else {
		return -EINVAL;
	}

	return 0;
}

static int udc_em32_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_em32_usbd_ep *ep_ctrl;
	uint8_t ep_dir;
	uint8_t ep_idx;

	ep_ctrl = udc_em32_get_ep(priv, cfg->addr);
	ep_dir = USB_EP_GET_DIR(cfg->addr);
	ep_idx = USB_EP_GET_IDX(cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	if (ep_idx == 1) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP1_EN_Pos);
	} else if (ep_idx == 2) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP2_EN_Pos);
	} else if (ep_idx == 3) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP3_EN_Pos);
	} else if (ep_idx == 4) {
		sys_clear_bit(REG_USB_CTRL, REG_USB_CTRL_EP4_EN_Pos);
	} else {
		return -EINVAL;
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

static void em32_usb_enable_all_ep(const struct device *dev)
{
	struct udc_ep_config *cfg;

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 1);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 2);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 3);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | 4);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 1);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 2);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 3);
	udc_em32_ep_enable(dev, cfg);

	cfg = udc_get_ep_cfg(dev, USB_EP_DIR_OUT | 4);
	udc_em32_ep_enable(dev, cfg);
}

static int udc_em32_init(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	int ret;

	/* Initialize USBD H/W */
	ret = em32_usb_boot(dev);
	if (ret < 0) {
		LOG_ERR("Failed to boot usb, code:%d", ret);
	}

	ret = em32_usb_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init usb, code:%d", ret);
	}

	usb_em32_sw_disconnect();

	priv->address = 0;
	priv->addressed_state = USB_EM32_NOT_ADDRESSED;
	priv->configured_state = USB_EM32_NOT_CONFIGURED;

#if defined(CONFIG_IO_ROUTE)
	priv->ep_out_num = 0;
	priv->ep_out_num_new = 0;
#endif

	/* configure and enable ep1 to ep4 */
	em32_usb_epx_init(dev);
	em32_usb_enable_all_ep(dev);

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
	int ret;

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

	/* Gating usb clock */
	ret = clock_control_off(priv->clock_dev, UINT_TO_POINTER(priv->clk_id_udc));
	if (ret < 0) {
		LOG_ERR("Failed to enable udc clock: %d", ret);
	}

	return 0;
}

static int udc_em32_driver_preinit(const struct device *dev)
{
	const struct udc_em32_config *config = dev->config;
	struct udc_em32_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;
	int i;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_type);

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
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = EP0_MPS;
	config->ep_cfg_in[0].addr = USB_EP_DIR_IN | 0;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err != 0) {
		LOG_ERR("Failed to register endpoint");
		return err;
	}

	/* The em32 usbd supports four endpoints (excluding ep0). Initialize and register it. */
	for (i = 1; i <= 4; i++) {
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

	for (i = 1; i <= 4; i++) {
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

static void udc_em32_irq_enable_func(const struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(em32_irq_table); i++) {
		irq_connect_dynamic(em32_irq_table[i].irq_num, 0,
				    (void (*)(const void *))em32_irq_table[i].isr_handler, dev, 0);
		irq_enable(em32_irq_table[i].irq_num);
	}
}

static void udc_em32_irq_disable_func(const struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(em32_irq_table); i++) {
		irq_disable(em32_irq_table[i].irq_num);
	}
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

#define UDC_EM32_DEVICE_DEFINE(inst)                                                               \
	K_THREAD_STACK_DEFINE(udc_em32_stack_##inst, CONFIG_UDC_EM32_STACK_SIZE);                  \
                                                                                                   \
	static const struct udc_em32_irq_mapping em32_irq_table[6] = {                             \
		{DT_INST_IRQN_BY_NAME(inst, setup), usb_em32_setup_isr},                           \
		{DT_INST_IRQN_BY_NAME(inst, suspend), usb_em32_suspend_isr},                       \
		{DT_INST_IRQN_BY_NAME(inst, resume), usb_em32_resume_isr},                         \
		{DT_INST_IRQN_BY_NAME(inst, reset), usb_em32_reset_isr},                           \
		{DT_INST_IRQN_BY_NAME(inst, epx_in), usb_em32_ep_d2h_isr},                         \
		{DT_INST_IRQN_BY_NAME(inst, epx_out), usb_em32_ep_h2d_isr},                        \
	};                                                                                         \
                                                                                                   \
	static void udc_em32_thread_##inst(void *dev, void *arg1, void *arg2)                      \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		em32_usbd_event_handler(dev);                                                      \
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
	static const struct udc_em32_config udc_em32_config_##inst = {                             \
		.num_of_eps = USB_NUM_BIDIR_ENDPOINTS,                                             \
		.ep_cfg_in = ep_cfg_in_##inst,                                                     \
		.ep_cfg_out = ep_cfg_out_##inst,                                                   \
		.ep_cfg_out_size = ARRAY_SIZE(ep_cfg_out_##inst),                                  \
		.ep_cfg_in_size = ARRAY_SIZE(ep_cfg_in_##inst),                                    \
		.make_thread = udc_em32_make_thread,                                               \
		.irq_enable_func = udc_em32_irq_enable_func,                                       \
		.irq_disable_func = udc_em32_irq_disable_func,                                     \
	};                                                                                         \
                                                                                                   \
	static struct udc_em32_data em32_udc_priv_##inst = {                                       \
		.clock_dev = NULL,                                                                 \
		.clk_id_udc = EM32_GATE_PCLKG_UDC,                                                 \
		.clk_id_atrim = EM32_GATE_PCLKG_ATRIM,                                             \
		.clk_id_aip = EM32_GATE_PCLKG_AIP,                                                 \
		.xfer_type = 0,                                                                    \
		.is_ep0_out_pkg = 0,                                                               \
		.is_pending_pkg = 0,                                                               \
		.is_ep0_in_en = 0,                                                                 \
		.is_ep0_out_en = 0,                                                                \
		.ep0_xfer_size = 0,                                                                \
		.configured_state = USB_EM32_NOT_CONFIGURED,                                       \
		.addressed_state = USB_EM32_NOT_ADDRESSED,                                         \
		.proc_remote_wakeup_state = USB_REMOTE_WAKEUP_REQ_NOT_ISSUE,                       \
	};                                                                                         \
                                                                                                   \
	static struct udc_data em32_udc_data_##inst = {                                            \
		.mutex = Z_MUTEX_INITIALIZER(em32_udc_data_##inst.mutex),                          \
		.priv = &em32_udc_priv_##inst,                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, udc_em32_driver_preinit, NULL, &em32_udc_data_##inst,          \
			      &udc_em32_config_##inst, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_em32_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_EM32_DEVICE_DEFINE)
