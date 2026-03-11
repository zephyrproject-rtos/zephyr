/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_UHC_DWC2_NRF_USBHS_NRF54L_H
#define ZEPHYR_DRIVERS_UHC_DWC2_NRF_USBHS_NRF54L_H

#include <nrfx.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/regulator.h>

struct nrf_usbhs_nrf54l_config {
	NRF_USBHS_Type *wrapper_base;
	const struct device *vregusb_dev;
};

struct nrf_usbhs_nrf54l_data {
	struct k_event events;
	struct onoff_manager *pclk24m_mgr;
	struct onoff_client pclk24m_cli;
};

/*
 * On USBHS, we cannot access the DWC2 register until VBUS is detected and
 * valid. If the user tries to force usbd_enable() and the corresponding
 * udc_enable() without a "VBUS ready" notification, the event wait will block
 * until a valid VBUS signal is detected or until the
 * CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT timeout expires.
 */
#define USBHS_VBUS_READY	BIT(0)


static void vregusb_event_cb(const struct device *dev,
			     const struct regulator_event *const evt,
			     const void *const user_data)
{
	const struct device *const dwc2_dev = user_data;
	struct nrf_usbhs_nrf54l_data *const data = UHC_DWC2_QUIRK_DATA(dwc2_dev);

	if (evt->type == REGULATOR_VOLTAGE_DETECTED) {
		k_event_post(&data->events, USBHS_VBUS_READY);
	}

	if (evt->type == REGULATOR_VOLTAGE_REMOVED) {
		k_event_set_masked(&data->events, 0, USBHS_VBUS_READY);
	}
}

static inline int usbhs_enable_core(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct nrf_usbhs_nrf54l_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	NRF_USBHS_Type *wrapper = cfg->wrapper_base;
	k_timeout_t timeout = K_FOREVER;
	int err;

	data->pclk24m_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF24M);

	if (CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT) {
		timeout = K_MSEC(CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT);
	}

	if (!k_event_wait(&data->events, USBHS_VBUS_READY, false, K_NO_WAIT)) {
		LOG_INF("VBUS is not ready, block udc_enable()");
		if (!k_event_wait(&data->events, USBHS_VBUS_READY, false, timeout)) {
			LOG_ERR("Timed out while waiting VBUS to be ready");
			return -ETIMEDOUT;
		}
	}

	/* Request PCLK24M using clock control driver */
	sys_notify_init_spinwait(&data->pclk24m_cli.notify);
	err = onoff_request(data->pclk24m_mgr, &data->pclk24m_cli);
	if (err != 0) {
		LOG_ERR("Failed to start PCLK24M %d", err);
		return err;
	}

	/* Power up peripheral */
	wrapper->ENABLE = USBHS_ENABLE_CORE_Msk;

	/* Set ID to Host and force D+ pull-up off for now */
	wrapper->PHY.OVERRIDEVALUES = (1 << 24) | (1 << 23);
	wrapper->PHY.INPUTOVERRIDE = (1 << 31) | (1 << 30) | (1 << 24) | (1 << 23);

	/* Release PHY power-on reset */
	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;

	/* Wait for PHY clock to start */
	k_busy_wait(45);

	/* Release DWC2 reset */
	wrapper->TASKS_START = 1UL;

	/* Wait for clock to start to avoid hang on too early register read */
	k_busy_wait(1);

	/* DWC2 opmode is now guaranteed to be Non-Driving, allow D+ pull-up to
	 * become active once driver clears DCTL SftDiscon bit.
	 */
	wrapper->PHY.INPUTOVERRIDE = (1 << 31);

	return 0;
}

static inline int usbhs_init_vreg_and_clock(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	int err;

	if (!device_is_ready(cfg->vregusb_dev)) {
		LOG_ERR("%s is not ready", cfg->vregusb_dev->name);
		return -ENODEV;
	}

	err = regulator_set_callback(cfg->vregusb_dev, vregusb_event_cb, dev);
	if (err != 0) {
		LOG_ERR("Failed to set regulator callback");
		return err;
	}

	err = regulator_enable(cfg->vregusb_dev);
	if (err != 0) {
		LOG_ERR("Failed to enable %s", cfg->vregusb_dev->name);
		return err;
	}

	err = usbhs_enable_core(dev);
	if (err != 0) {
		LOG_ERR("Failed to enable DWC2 core");
		return err;
	}

	return 0;
}

static inline int usbhs_disable_core(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct nrf_usbhs_nrf54l_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	NRF_USBHS_Type *wrapper = cfg->wrapper_base;
	int err;

	/* Set ID to Host and forcefully disable D+ pull-up */
	wrapper->PHY.OVERRIDEVALUES = (1 << 24) | (1 << 23);
	wrapper->PHY.INPUTOVERRIDE = (1 << 31) | (1 << 30) | (1 << 24) | (1 << 23);

	wrapper->ENABLE = 0UL;

	/* Release PCLK24M using clock control driver */
	err = onoff_cancel_or_release(data->pclk24m_mgr, &data->pclk24m_cli);
	if (err != 0) {
		LOG_ERR("Failed to stop PCLK24M %d", err);
		return err;
	}

	return 0;
}

static inline int usbhs_disable_vreg(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	return regulator_disable(cfg->vregusb_dev);
}

#define QUIRK_NRF_USBHS_DEFINE(n)						\
	struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {		\
		.init = usbhs_init_vreg_and_clock,				\
		.disable = usbhs_disable_core,					\
		.shutdown = usbhs_disable_vreg,					\
	};									\
										\
	static const struct nrf_usbhs_nrf54l_config uhc_dwc2_quirk_config_##n = {\
		.wrapper_base = UINT_TO_POINTER(DT_REG_ADDR(DT_INST_PARENT(n))),\
		.vregusb_dev = DEVICE_DT_GET(DT_PHANDLE(DT_INST_PARENT(n),	\
							regulator)),		\
	};									\
										\
	static struct nrf_usbhs_nrf54l_data uhc_dwc2_quirk_data_##n = {		\
		.events = Z_EVENT_INITIALIZER(uhc_dwc2_quirk_data_##n.events),	\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_NRF_USBHS_DEFINE)

#endif /* ZEPHYR_DRIVERS_UHC_DWC2_NRF_USBHS_NRF54L_H */
