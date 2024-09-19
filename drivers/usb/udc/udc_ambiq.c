/*
 * Copyright 2024 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include "am_mcu_apollo.h"
#include <string.h>
#include <zephyr/drivers/clock_control/clock_control_ambiq.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "udc_common.h"
#include <usb_dwc2_hw.h>

LOG_MODULE_REGISTER(udc_ambiq, CONFIG_UDC_DRIVER_LOG_LEVEL);

enum udc_ambiq_event_type {
	/* SETUP packet received at Control Endpoint */
	UDC_AMBIQ_EVT_HAL_SETUP,
	/* OUT transaction completed */
	UDC_AMBIQ_EVT_HAL_OUT_CMP,
	/* IN transaction completed */
	UDC_AMBIQ_EVT_HAL_IN_CMP,
	/* Xfer request received via udc_ambiq_ep_enqueue API */
	UDC_AMBIQ_EVT_XFER,
};

struct udc_ambiq_event {
	const struct device *dev;
	enum udc_ambiq_event_type type;
	uint8_t ep;
};

K_MSGQ_DEFINE(drv_msgq, sizeof(struct udc_ambiq_event), CONFIG_UDC_AMBIQ_MAX_QMESSAGES,
	      sizeof(void *));

/* USB device controller access from devicetree */
#define DT_DRV_COMPAT ambiq_usb

#define EP0_MPS   64U
#define EP_FS_MPS 64U
#define EP_HS_MPS 512U

struct udc_ambiq_data {
	struct k_thread thread_data;
	void *usb_handle;
	am_hal_usb_dev_speed_e usb_speed;
	uint8_t setup[8];
	uint8_t ctrl_pending_setup_buffer[8];
	bool ctrl_pending_in_ack;
	bool ctrl_pending_setup;
	bool ctrl_setup_recv_at_status_in;
};

struct udc_ambiq_config {
	uint32_t num_endpoints;
	int speed_idx;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	struct gpio_dt_spec vddusb33_gpio;
	struct gpio_dt_spec vddusb0p9_gpio;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*callback_register_func)(const struct device *dev);
};

static int udc_ambiq_rx(const struct device *dev, uint8_t ep, struct net_buf *buf);

static int usbd_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&cfg->fifo, buf);
	if (length) {
		udc_ambiq_rx(dev, cfg->addr, buf);
	}

	return 0;
}

static int udc_ambiq_tx(const struct device *dev, uint8_t ep, struct net_buf *buf)
{
	const struct udc_ambiq_data *priv = udc_get_private(dev);
	uint32_t status;

	if (udc_ep_is_busy(dev, ep)) {
		LOG_WRN("ep 0x%02x is busy!", ep);
		return 0;
	}
	udc_ep_set_busy(dev, ep, true);

	/* buf equals NULL is used as indication of ZLP request */
	if (buf == NULL) {
		status = am_hal_usb_ep_xfer(priv->usb_handle, ep, NULL, 0);
	} else {
		status = am_hal_usb_ep_xfer(priv->usb_handle, ep, buf->data, buf->len);
	}

	if (status != AM_HAL_STATUS_SUCCESS) {
		udc_ep_set_busy(dev, ep, false);
		LOG_ERR("am_hal_usb_ep_xfer write failed(0x%02x), %d", ep, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_ambiq_rx(const struct device *dev, uint8_t ep, struct net_buf *buf)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	uint32_t status;
	uint16_t rx_size = buf->size;

	if (udc_ep_is_busy(dev, ep)) {
		LOG_WRN("ep 0x%02x is busy!", ep);
		return 0;
	}
	udc_ep_set_busy(dev, ep, true);

	/* Make sure that OUT transaction size triggered doesn't exceed EP's MPS */
	if ((ep != USB_CONTROL_EP_OUT) && (cfg->mps < rx_size)) {
		rx_size = cfg->mps;
	}

	status = am_hal_usb_ep_xfer(priv->usb_handle, ep, buf->data, rx_size);
	if (status != AM_HAL_STATUS_SUCCESS) {
		udc_ep_set_busy(dev, ep, false);
		LOG_ERR("am_hal_usb_ep_xfer read(rx) failed(0x%02x), %d", ep, (int)status);
		return -EIO;
	}

	return 0;
}

static void udc_ambiq_evt_callback(const struct device *dev, am_hal_usb_dev_event_e dev_state)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	switch (dev_state) {
	case AM_HAL_USB_DEV_EVT_BUS_RESET:
		/* enable usb bus interrupts */
		am_hal_usb_intr_usb_enable(priv->usb_handle,
					   USB_CFG2_SOFE_Msk | USB_CFG2_ResumeE_Msk |
						   USB_CFG2_SuspendE_Msk | USB_CFG2_ResetE_Msk);
		/* init the endpoint */
		am_hal_usb_ep_init(priv->usb_handle, 0, 0, EP0_MPS);
		/* Set USB device speed to HAL */
		am_hal_usb_set_dev_speed(priv->usb_handle, priv->usb_speed);
		LOG_INF("USB Reset event");
		/* Submit USB reset event to UDC */
		udc_submit_event(dev, UDC_EVT_RESET, 0);
		break;
	case AM_HAL_USB_DEV_EVT_RESUME:
		/* Handle USB Resume event, then set device state to active */
		am_hal_usb_set_dev_state(priv->usb_handle, AM_HAL_USB_DEV_STATE_ACTIVE);
		LOG_INF("RESUMING from suspend");
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
		break;
	case AM_HAL_USB_DEV_EVT_SOF:
		udc_submit_event(dev, UDC_EVT_SOF, 0);
		break;
	case AM_HAL_USB_DEV_EVT_SUSPEND:
		/* Handle USB Suspend event, then set device state to suspended */
		am_hal_usb_set_dev_state(priv->usb_handle, AM_HAL_USB_DEV_STATE_SUSPENDED);
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		break;
	default:
		/* Unreachable case */
		break;
	}
}

static void udc_ambiq_ep0_setup_callback(const struct device *dev, uint8_t *usb_setup)
{
	struct udc_ambiq_event evt = {.type = UDC_AMBIQ_EVT_HAL_SETUP};
	struct udc_ambiq_data *priv = udc_get_private(dev);

	/* Defer Setup Packet that arrives when we are waiting for
	 * status stage for OUT data control transfer to be completed
	 */
	if (priv->ctrl_pending_in_ack) {
		priv->ctrl_pending_setup = true;
		memcpy(priv->ctrl_pending_setup_buffer, usb_setup, 8);
		return;
	}

	/* Check whether we received SETUP packet during OUT_ACK (a.k.a STATUS_IN)
	 * state. If so, it might be inversion caused by register reading sequence.
	 * Raise flag accordingly and handle later.
	 */
	priv->ctrl_setup_recv_at_status_in = udc_ctrl_stage_is_status_in(dev);
	memcpy(priv->setup, usb_setup, sizeof(struct usb_setup_packet));
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void udc_ambiq_ep_xfer_complete_callback(const struct device *dev, uint8_t ep_addr,
						uint16_t xfer_len, am_hal_usb_xfer_code_e code,
						void *param)
{
	struct net_buf *buf;
	struct udc_ambiq_event evt;

	/* Extract EP information and queue event */
	evt.ep = ep_addr;
	if (USB_EP_DIR_IS_IN(ep_addr)) {
		evt.type = UDC_AMBIQ_EVT_HAL_IN_CMP;
	} else {
		buf = udc_buf_peek(dev, ep_addr);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep_addr);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			return;
		}

		net_buf_add(buf, xfer_len);
		evt.type = UDC_AMBIQ_EVT_HAL_OUT_CMP;
	}

	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static enum udc_bus_speed udc_ambiq_device_speed(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	am_hal_usb_dev_speed_e e_speed = am_hal_get_usb_dev_speed(priv->usb_handle);

	if (e_speed == AM_HAL_USB_SPEED_HIGH) {
		return UDC_BUS_SPEED_HS;
	} else {
		return UDC_BUS_SPEED_FS;
	}
}

static int udc_ambiq_ep_enqueue(const struct device *dev, struct udc_ep_config *ep_cfg,
				struct net_buf *buf)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	struct udc_ambiq_event evt = {
		.ep = ep_cfg->addr,
		.type = UDC_AMBIQ_EVT_XFER,
	};

	LOG_DBG("%p enqueue %x %p", dev, ep_cfg->addr, buf);
	udc_buf_put(ep_cfg, buf);
	if (ep_cfg->addr == USB_CONTROL_EP_IN && buf->len == 0 && priv->ctrl_pending_in_ack) {
		priv->ctrl_pending_in_ack = false;
		udc_ambiq_ep_xfer_complete_callback(dev, USB_CONTROL_EP_IN, 0, 0, NULL);
		return 0;
	}
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);

	return 0;
}

static int udc_ambiq_ep_dequeue(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	unsigned int lock_key;
	struct udc_ambiq_data *priv = udc_get_private(dev);
	struct net_buf *buf;

	lock_key = irq_lock();

	buf = udc_buf_get_all(dev, ep_cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, ep_cfg->addr, false);
	am_hal_usb_ep_state_reset(priv->usb_handle, ep_cfg->addr);
	irq_unlock(lock_key);

	LOG_DBG("dequeue ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_ambiq_ep_set_halt(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	LOG_DBG("Halt ep 0x%02x", ep_cfg->addr);

	am_hal_usb_ep_stall(priv->usb_handle, ep_cfg->addr);

	return 0;
}

static int udc_ambiq_ep_clear_halt(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	am_hal_usb_ep_clear_stall(priv->usb_handle, ep_cfg->addr);

	return 0;
}

static int udc_ambiq_ep_enable(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	uint8_t endpoint_type;
	uint32_t status;

	__ASSERT_NO_MSG(ep_cfg);

	switch (ep_cfg->attributes) {
	case USB_EP_TYPE_CONTROL:
		endpoint_type = 0; /* AM_HAL_USB_EP_XFER_CONTROL */
		break;
	case USB_EP_TYPE_ISO:
		endpoint_type = 1; /* AM_HAL_USB_EP_XFER_ISOCHRONOUS */
		break;
	case USB_EP_TYPE_BULK:
		endpoint_type = 2; /* AM_HAL_USB_EP_XFER_BULK */
		break;
	case USB_EP_TYPE_INTERRUPT:
		endpoint_type = 3; /* AM_HAL_USB_EP_XFER_INTERRUPT */
		break;
	default:
		return -EINVAL;
	}

	status = am_hal_usb_ep_init(priv->usb_handle, ep_cfg->addr, endpoint_type, ep_cfg->mps);
	if (status != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_usb_ep_init failed(0x%02x), %d", ep_cfg->addr, (int)status);
		return -EIO;
	}

	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_ambiq_ep_disable(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	__ASSERT_NO_MSG(ep_cfg);
	am_hal_usb_ep_state_reset(priv->usb_handle, ep_cfg->addr);
	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	return 0;
}

static int udc_ambiq_host_wakeup(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	am_hal_usb_start_remote_wakeup(priv->usb_handle);

	return 0;
}

static int udc_ambiq_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	LOG_DBG("addr %u (0x%02x)", addr, addr);
	am_hal_usb_set_addr(priv->usb_handle, addr);
	am_hal_usb_set_dev_state(priv->usb_handle, AM_HAL_USB_DEV_STATE_ADDRESSED);

	return 0;
}

static int udc_ambiq_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	am_hal_usb_test_mode_e am_usb_test_mode;
	struct udc_ambiq_data *priv = udc_get_private(dev);

	switch (mode) {
	case USB_DWC2_DCTL_TSTCTL_TESTJ:
		am_usb_test_mode = AM_HAL_USB_TEST_J;
		break;
	case USB_DWC2_DCTL_TSTCTL_TESTK:
		am_usb_test_mode = AM_HAL_USB_TEST_K;
		break;
	case USB_DWC2_DCTL_TSTCTL_TESTSN:
		am_usb_test_mode = AM_HAL_USB_TEST_SE0_NAK;
		break;
	case USB_DWC2_DCTL_TSTCTL_TESTPM:
		am_usb_test_mode = AM_HAL_USB_TEST_PACKET;
		break;
	default:
		return -EINVAL;
	}

	if (dryrun) {
		LOG_DBG("Test Mode %u supported", mode);
		return 0;
	}

	am_hal_usb_enter_test_mode(priv->usb_handle);
	am_hal_usb_test_mode(priv->usb_handle, am_usb_test_mode);

	return 0;
}

static int udc_ambiq_enable(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);

	/* USB soft connect */
	am_hal_usb_attach(priv->usb_handle);
	LOG_DBG("Enable UDC");

	return 0;
}

static int udc_ambiq_disable(const struct device *dev)
{
	unsigned int lock_key;
	struct udc_ambiq_data *priv = udc_get_private(dev);
	const struct udc_ambiq_config *cfg = dev->config;

	/* Disable USB interrupt */
	lock_key = irq_lock();
	cfg->irq_disable_func(dev);
	irq_unlock(lock_key);

	/* Disable soft disconnect */
	am_hal_usb_detach(priv->usb_handle);
	am_hal_usb_intr_usb_disable(priv->usb_handle, USB_CFG2_SOFE_Msk | USB_CFG2_ResumeE_Msk |
							      USB_CFG2_SuspendE_Msk |
							      USB_CFG2_ResetE_Msk);
	am_hal_usb_intr_usb_clear(priv->usb_handle);
	for (unsigned int i = 0; i < cfg->num_endpoints; i++) {
		am_hal_usb_ep_state_reset(priv->usb_handle, i);
		am_hal_usb_ep_state_reset(priv->usb_handle, BIT(7) | i);
	}
	LOG_DBG("Disable UDC");

	return 0;
}

static void udc_ambiq_usb_isr(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	uint32_t int_status[3];

	am_hal_usb_intr_status_get(priv->usb_handle, &int_status[0], &int_status[1],
				   &int_status[2]);
	am_hal_usb_interrupt_service(priv->usb_handle, int_status[0], int_status[1], int_status[2]);
}

static int usb_power_rails_set(const struct device *dev, bool on)
{
	int ret = 0;
	const struct udc_ambiq_config *cfg = dev->config;

	/* Check that both power control GPIO is defined */
	if ((cfg->vddusb33_gpio.port == NULL) || (cfg->vddusb0p9_gpio.port == NULL)) {
		LOG_WRN("vddusb control gpio not defined");
		return -EINVAL;
	}

	/* Enable USB IO */
	ret = gpio_pin_configure_dt(&cfg->vddusb33_gpio, GPIO_OUTPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->vddusb0p9_gpio, GPIO_OUTPUT);
	if (ret) {
		return ret;
	}

	/* power rails set */
	ret = gpio_pin_set_dt(&cfg->vddusb33_gpio, on);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->vddusb0p9_gpio, on);
	if (ret) {
		return ret;
	}
	am_hal_delay_us(50000);

	return 0;
}

static int udc_ambiq_init(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	const struct udc_ambiq_config *cfg = dev->config;
	uint32_t ret = 0;

	/* Create USB */
	if (am_hal_usb_initialize(0, (void *)&priv->usb_handle) != AM_HAL_STATUS_SUCCESS) {
		return -EIO;
	}

	/* Register callback functions */
	cfg->callback_register_func(dev);
	/* enable internal power rail */
	am_hal_usb_power_control(priv->usb_handle, AM_HAL_SYSCTRL_WAKE, false);
	/* Assert USB PHY reset in MCU control registers */
	am_hal_usb_enable_phy_reset_override();
	/* Enable the USB power rails */
	ret = usb_power_rails_set(dev, true);
	if (ret) {
		return ret;
	}
	/* Disable BC detection voltage source */
	am_hal_usb_hardware_unreset();
	/* Release USB PHY reset */
	am_hal_usb_disable_phy_reset_override();
	/* Set USB Speed */
	am_hal_usb_set_dev_speed(priv->usb_handle, priv->usb_speed);
	/* Enable USB interrupt */
	am_hal_usb_intr_usb_enable(priv->usb_handle, USB_INTRUSB_Reset_Msk);
	/* Enable Control Endpoints */
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, EP0_MPS, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, EP0_MPS, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}
	/* Connect and enable USB interrupt */
	cfg->irq_enable_func(dev);

	return 0;
}

static int udc_ambiq_shutdown(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	const struct udc_ambiq_config *cfg = dev->config;
	int ret = 0;

	LOG_INF("shutdown");

	/* Disable Control Endpoints */
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}
	/* Disable USB interrupt */
	cfg->irq_disable_func(dev);
	/* Assert USB PHY reset */
	am_hal_usb_enable_phy_reset_override();
	/* Disable the USB power rails */
	ret = usb_power_rails_set(dev, false);
	if (ret) {
		return ret;
	}
	/* Power down USB HAL */
	am_hal_usb_power_control(priv->usb_handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
	/* Deinitialize USB instance */
	am_hal_usb_deinitialize(priv->usb_handle);
	priv->usb_handle = NULL;

	return 0;
}

static int udc_ambiq_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int udc_ambiq_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

static void ambiq_handle_evt_setup(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	/* Create network buffer for SETUP packet and pass into UDC framework */
	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return;
	}
	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "setup");

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		priv->ctrl_pending_in_ack = true;
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		/* Submit event for data IN stage */
		LOG_DBG("s:%p|feed for -in-status", buf);
		udc_ctrl_submit_s_in_status(dev);
	} else {
		/* Submit event for no-data stage */
		LOG_DBG("s:%p|feed >setup", buf);
		udc_ctrl_submit_s_status(dev);
	}
}

static inline void ambiq_handle_evt_dout(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	/* retrieve endpoint buffer */
	buf = udc_buf_get(dev, cfg->addr);
	if (buf == NULL) {
		LOG_ERR("No buffer queued for control ep");
		return;
	}

	/* Clear endpoint busy status */
	udc_ep_set_busy(dev, cfg->addr, false);

	/* Handle transfer complete event */
	if (cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			udc_ctrl_update_stage(dev, buf);
			udc_ctrl_submit_status(dev, buf);
		} else {
			udc_ctrl_update_stage(dev, buf);
		}

		if (udc_ctrl_stage_is_status_in(dev)) {
			udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

static void ambiq_handle_zlp_tx(const struct device *dev, struct udc_ep_config *const cfg)
{
	udc_ambiq_tx(dev, cfg->addr, NULL);
}

static void ambiq_handle_evt_din(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	struct net_buf *buf;
	bool udc_ambiq_rx_status_in_completed = false;

	/* Clear endpoint busy status */
	udc_ep_set_busy(dev, cfg->addr, false);
	/* Check and Handle ZLP flag */
	buf = udc_buf_peek(dev, cfg->addr);
	if (cfg->addr != USB_CONTROL_EP_IN) {
		if (udc_ep_buf_has_zlp(buf)) {
			udc_ep_buf_clear_zlp(buf);
			udc_ambiq_tx(dev, cfg->addr, NULL);
			ambiq_handle_zlp_tx(dev, cfg);
			return;
		}
	}

	/* retrieve endpoint buffer */
	buf = udc_buf_get(dev, cfg->addr);
	if (buf == NULL) {
		LOG_ERR("No buffer queued for control ep");
		return;
	}
	LOG_DBG("DataIn ep 0x%02x len %u", cfg->addr, buf->size);

	/* Handle transfer complete event */
	if (cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			if (data->caps.out_ack == 0) {
				/* Status stage finished, notify upper layer */
				udc_ctrl_submit_status(dev, buf);
			}

			if (udc_ctrl_stage_is_status_in(dev)) {
				udc_ambiq_rx_status_in_completed = true;
			}
		}

		if (priv->ctrl_setup_recv_at_status_in && (buf->len == 0)) {
			priv->ctrl_setup_recv_at_status_in = false;
			net_buf_unref(buf);
			return;
		}
		priv->ctrl_setup_recv_at_status_in = false;
		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (((data->caps.out_ack == false) && udc_ctrl_stage_is_status_out(dev)) ||
		    ((data->caps.out_ack == true) && (data->stage == CTRL_PIPE_STAGE_SETUP))) {
			/*
			 * IN transfer finished, release buffer,
			 * control OUT buffer should be already fed.
			 */
			net_buf_unref(buf);
		}

		/*
		 * Trigger deferred SETUP that was hold back if we are
		 * waiting for DATA_OUT status stage to be completed
		 */
		if (udc_ambiq_rx_status_in_completed && priv->ctrl_pending_setup) {
			priv->ctrl_pending_setup = false;
			udc_ambiq_ep0_setup_callback(dev, priv->ctrl_pending_setup_buffer);
		}
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

static void udc_event_xfer(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, cfg->addr);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", cfg->addr);
		return;
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		udc_ambiq_tx(dev, cfg->addr, buf);
	} else {
		udc_ambiq_rx(dev, cfg->addr, buf);
	}
}

static ALWAYS_INLINE void ambiq_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct udc_ep_config *ep_cfg;
	struct udc_ambiq_event evt;

	while (true) {
		k_msgq_get(&drv_msgq, &evt, K_FOREVER);
		ep_cfg = udc_get_ep_cfg(dev, evt.ep);

		switch (evt.type) {
		case UDC_AMBIQ_EVT_XFER:
			udc_event_xfer(dev, ep_cfg);
			break;
		case UDC_AMBIQ_EVT_HAL_SETUP:
			LOG_DBG("SETUP event");
			ambiq_handle_evt_setup(dev);
			break;
		case UDC_AMBIQ_EVT_HAL_OUT_CMP:
			LOG_DBG("DOUT event ep 0x%02x", ep_cfg->addr);
			ambiq_handle_evt_dout(dev, ep_cfg);
			break;
		case UDC_AMBIQ_EVT_HAL_IN_CMP:
			LOG_DBG("DIN event");
			ambiq_handle_evt_din(dev, ep_cfg);
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	}
}

/*
 * This is called once to initialize the controller and endpoints
 * capabilities, and register endpoint structures.
 */
static int udc_ambiq_driver_init(const struct device *dev)
{
	struct udc_ambiq_data *priv = udc_get_private(dev);
	const struct udc_ambiq_config *cfg = dev->config;
	struct udc_data *data = dev->data;
	int ep_mps = 0;
	int err;

	if (cfg->speed_idx == 1) {
		data->caps.hs = false;
		priv->usb_speed = AM_HAL_USB_SPEED_FULL;
		ep_mps = EP_FS_MPS;
	} else if ((cfg->speed_idx == 2)) {
		data->caps.hs = true;
		priv->usb_speed = AM_HAL_USB_SPEED_HIGH;
		ep_mps = EP_HS_MPS;
	}

	for (unsigned int i = 0; i < cfg->num_endpoints; i++) {
		cfg->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			cfg->ep_cfg_out[i].caps.control = 1;
			cfg->ep_cfg_out[i].caps.mps = EP0_MPS;
		} else {
			cfg->ep_cfg_out[i].caps.bulk = 1;
			cfg->ep_cfg_out[i].caps.interrupt = 1;
			cfg->ep_cfg_out[i].caps.iso = 1;
			cfg->ep_cfg_out[i].caps.mps = ep_mps;
		}

		cfg->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &cfg->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (unsigned int i = 0; i < cfg->num_endpoints; i++) {
		cfg->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			cfg->ep_cfg_in[i].caps.control = 1;
			cfg->ep_cfg_in[i].caps.mps = EP0_MPS;
		} else {
			cfg->ep_cfg_in[i].caps.bulk = 1;
			cfg->ep_cfg_in[i].caps.interrupt = 1;
			cfg->ep_cfg_in[i].caps.iso = 1;
			cfg->ep_cfg_in[i].caps.mps = ep_mps;
		}

		cfg->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &cfg->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}
	data->caps.addr_before_status = true;
	data->caps.rwup = true;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_MPS0_64;

	cfg->make_thread(dev);

	return 0;
}

static const struct udc_api udc_ambiq_api = {
	.device_speed = udc_ambiq_device_speed,
	.ep_enqueue = udc_ambiq_ep_enqueue,
	.ep_dequeue = udc_ambiq_ep_dequeue,
	.ep_set_halt = udc_ambiq_ep_set_halt,
	.ep_clear_halt = udc_ambiq_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = udc_ambiq_ep_enable,
	.ep_disable = udc_ambiq_ep_disable,
	.host_wakeup = udc_ambiq_host_wakeup,
	.set_address = udc_ambiq_set_address,
	.test_mode = udc_ambiq_test_mode,
	.enable = udc_ambiq_enable,
	.disable = udc_ambiq_disable,
	.init = udc_ambiq_init,
	.shutdown = udc_ambiq_shutdown,
	.lock = udc_ambiq_lock,
	.unlock = udc_ambiq_unlock,
};

/*
 * A UDC driver should always be implemented as a multi-instance
 * driver, even if your platform does not require it.
 */
#define UDC_AMBIQ_DEVICE_DEFINE(n)                                                                 \
	K_THREAD_STACK_DEFINE(udc_ambiq_stack_##n, CONFIG_UDC_AMBIQ_STACK_SIZE);                   \
                                                                                                   \
	static void udc_ambiq_evt_callback_##n(am_hal_usb_dev_event_e dev_state)                   \
	{                                                                                          \
		udc_ambiq_evt_callback(DEVICE_DT_INST_GET(n), dev_state);                          \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_ep0_setup_callback_##n(uint8_t *usb_setup)                           \
	{                                                                                          \
		udc_ambiq_ep0_setup_callback(DEVICE_DT_INST_GET(n), usb_setup);                    \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_ep_xfer_complete_callback_##n(                                       \
		uint8_t ep_addr, uint16_t xfer_len, am_hal_usb_xfer_code_e code, void *param)      \
	{                                                                                          \
		udc_ambiq_ep_xfer_complete_callback(DEVICE_DT_INST_GET(n), ep_addr, xfer_len,      \
						    code, param);                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_register_callback_##n(const struct device *dev)                      \
	{                                                                                          \
		struct udc_ambiq_data *priv = udc_get_private(dev);                                \
                                                                                                   \
		am_hal_usb_register_dev_evt_callback(priv->usb_handle,                             \
						     udc_ambiq_evt_callback_##n);                  \
		am_hal_usb_register_ep0_setup_received_callback(priv->usb_handle,                  \
								udc_ambiq_ep0_setup_callback_##n); \
		am_hal_usb_register_ep_xfer_complete_callback(                                     \
			priv->usb_handle, udc_ambiq_ep_xfer_complete_callback_##n);                \
	}                                                                                          \
	static void udc_ambiq_thread_##n(void *dev, void *arg1, void *arg2)                        \
	{                                                                                          \
		ambiq_thread_handler(dev);                                                         \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_make_thread_##n(const struct device *dev)                            \
	{                                                                                          \
		struct udc_ambiq_data *priv = udc_get_private(dev);                                \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_ambiq_stack_##n,                           \
				K_THREAD_STACK_SIZEOF(udc_ambiq_stack_##n), udc_ambiq_thread_##n,  \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_AMBIQ_THREAD_PRIORITY), K_ESSENTIAL,        \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_irq_enable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), udc_ambiq_usb_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void udc_ambiq_irq_disable_func_##n(const struct device *dev)                       \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];              \
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];               \
                                                                                                   \
	static const struct udc_ambiq_config udc_ambiq_config_##n = {                              \
		.num_endpoints = DT_INST_PROP(n, num_bidir_endpoints),                             \
		.ep_cfg_in = ep_cfg_out,                                                           \
		.ep_cfg_out = ep_cfg_in,                                                           \
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),                           \
		.vddusb33_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(n), vddusb33_gpios, {0}),         \
		.vddusb0p9_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(n), vddusb0p9_gpios, {0}),       \
		.irq_enable_func = udc_ambiq_irq_enable_func_##n,                                  \
		.irq_disable_func = udc_ambiq_irq_disable_func_##n,                                \
		.make_thread = udc_ambiq_make_thread_##n,                                          \
		.callback_register_func = udc_ambiq_register_callback_##n,                         \
	};                                                                                         \
                                                                                                   \
	static struct udc_ambiq_data udc_priv_##n = {};                                            \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_ambiq_driver_init, NULL, &udc_data_##n,                       \
			      &udc_ambiq_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_ambiq_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_AMBIQ_DEVICE_DEFINE)
