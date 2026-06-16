/*
 * SPDXFileCopyrightText: Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_NRF_USBHS_NRF54L_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_NRF_USBHS_NRF54L_H

#include <nrfx.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/regulator.h>
#include <nrf_usbhs_wrapper.h>

/*
 * On USBHS, we cannot access the DWC2 register until VBUS is detected and
 * valid. If the user tries to force usbd_enable() and the corresponding
 * udc_enable() without a "VBUS ready" notification, the event wait will block
 * until a valid VBUS signal is detected or until the
 * CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT timeout expires.
 */

static struct onoff_manager *pclk24m_mgr;
static struct onoff_client pclk24m_cli;

static void vregusb_event_cb(const struct device *dev,
			     const struct regulator_event *const evt,
			     const void *const user_data)
{
	ARG_UNUSED(dev);
	const struct device *const udc_dev = user_data;

	if (evt->type == REGULATOR_VOLTAGE_DETECTED) {
		udc_submit_event(udc_dev, UDC_EVT_VBUS_READY, 0);
	}

	if (evt->type == REGULATOR_VOLTAGE_REMOVED) {
		udc_submit_event(udc_dev, UDC_EVT_VBUS_REMOVED, 0);
	}
}

static inline int usbhs_init_vreg_and_clock(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	int err;

	nrf_usbhs_wrapper_set_udc_cb(parent, vregusb_event_cb, dev);

	err = nrf_usbhs_wrapper_vreg_enable(parent);
	if (err) {
		LOG_ERR("Failed to enable regulator");
		return err;
	}

	pclk24m_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF24M);

	return 0;
}

static int usbhs_request_pclk24m(void)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	int err;

	/* Request PCLK24M using clock control driver */
	sys_notify_init_spinwait(&pclk24m_cli.notify);
	err = onoff_request(pclk24m_mgr, &pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to start PCLK24M %d", err);
		return err;
	}

	return 0;
}

static int usbhs_release_pclk24m(void)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	int err;

	/* Release PCLK24M using clock control driver */
	err = onoff_cancel_or_release(pclk24m_mgr, &pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to stop PCLK24M %d", err);
		return err;
	}

	return 0;
}

static inline int usbhs_enable_core(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	struct k_event *events = nrf_usbhs_wrapper_get_events_ptr(parent);
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);
	k_timeout_t timeout = K_FOREVER;
	bool warning_printed = false;
	int err = -EAGAIN;

	if (CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT) {
		timeout = K_MSEC(CONFIG_UDC_DWC2_USBHS_VBUS_READY_TIMEOUT);
	}

	while (err == -EAGAIN) {
		if (!k_event_wait(events, NRF_USBHS_VBUS_READY, false, K_NO_WAIT)) {
			if (!warning_printed) {
				LOG_WRN("VBUS is not ready, block udc_enable()");
				warning_printed = true;
			}

			if (!k_event_wait(events, NRF_USBHS_VBUS_READY, false, timeout)) {
				return -ETIMEDOUT;
			}
		}

		err = usbhs_request_pclk24m();
		if (err < 0) {
			return err;
		}

		err = nrf_usbhs_wrapper_udc_pre_enable(parent);
		if (err != 0) {
			usbhs_release_pclk24m();
		}
	}

	return err;
}

static inline int usbhs_enable_pullup(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));

	nrf_usbhs_wrapper_udc_post_enable(parent);

	return 0;
}

static inline int usbhs_disable_core(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);

	nrf_usbhs_wrapper_udc_disable(parent);
	usbhs_release_pclk24m();

	return 0;
}

static inline int usbhs_disable_vreg(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	ARG_UNUSED(dev);

	return nrf_usbhs_wrapper_vreg_disable(parent);
}

static inline int usbhs_init_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.can_detect_vbus = true;
	data->caps.hs = true;

	return 0;
}

static inline int usbhs_is_phy_clk_off(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	struct k_event *events = nrf_usbhs_wrapper_get_events_ptr(parent);
	bool clk_on;

	clk_on = k_event_test(events, NRF_USBHS_VBUS_READY);

	return !clk_on;
}

static inline int usbhs_post_hibernation_entry(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);

	sys_set_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	nrf_usbhs_wrapper_stop(parent);

	return 0;
}

static inline int usbhs_pre_hibernation_exit(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);

	sys_clear_bits((mem_addr_t)&base->pcgcctl, USB_DWC2_PCGCCTL_GATEHCLK);

	nrf_usbhs_wrapper_start(parent);

	return 0;
}

#define QUIRK_NRF_USBHS_DEFINE(n)						\
	struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.init = usbhs_init_vreg_and_clock,				\
		.pre_enable = usbhs_enable_core,				\
		.post_enable = usbhs_enable_pullup,				\
		.disable = usbhs_disable_core,					\
		.shutdown = usbhs_disable_vreg,					\
		.caps = usbhs_init_caps,					\
		.is_phy_clk_off = usbhs_is_phy_clk_off,				\
		.post_hibernation_entry = usbhs_post_hibernation_entry,		\
		.pre_hibernation_exit = usbhs_pre_hibernation_exit,		\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_NRF_USBHS_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_NRF_USBHS_NRF54L_H */
