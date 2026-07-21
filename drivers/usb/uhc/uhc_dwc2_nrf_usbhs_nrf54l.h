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
 * valid. If the user tries to force usbh_enable() and the corresponding
 * uhc_enable() without a "VBUS ready" notification, the event wait will block
 * until a valid VBUS signal is detected or until the
 * CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT timeout expires.
 */
#define USBHS_VBUS_READY BIT(0)

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

static inline int usbhs_pre_init(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct nrf_usbhs_nrf54l_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	NRF_USBHS_Type *wrapper = cfg->wrapper_base;
	k_timeout_t timeout;
	int err;

	if (!device_is_ready(cfg->vregusb_dev)) {
		LOG_ERR("%s is not ready", cfg->vregusb_dev->name);
		return -ENODEV;
	}

	/* Setup the PCLK24M clock */

	data->pclk24m_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF24M);

	sys_notify_init_spinwait(&data->pclk24m_cli.notify);

	err = onoff_request(data->pclk24m_mgr, &data->pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to start PCLK24M %d", err);
		return err;
	}

	/* Enable the power and wait that it is enabled */

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

	if (CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT > 0) {
		timeout = K_MSEC(CONFIG_UHC_DWC2_USBHS_VBUS_READY_TIMEOUT);
	} else {
		timeout = K_FOREVER;
	}

	if (!k_event_wait(&data->events, USBHS_VBUS_READY, false, timeout)) {
		LOG_ERR("Timed out while waiting VBUS to be ready");
		return -ETIMEDOUT;
	}

	/* Enable the DWC2 core */
	wrapper->ENABLE = USBHS_ENABLE_CORE_Msk;

	/* Set ID to Host */
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_ID_Msk;
	wrapper->PHY.OVERRIDEVALUES = 0;

	/* Release PHY power-on reset and wait for it to start */
	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;
	k_busy_wait(45);

	/* Release DWC2 reset and wait to avoid hanging on too early register reads */
	wrapper->TASKS_START = 1UL;
	k_busy_wait(1);

	return 0;
}

static inline int usbhs_shutdown(const struct device *dev)
{
	const struct nrf_usbhs_nrf54l_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct nrf_usbhs_nrf54l_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	NRF_USBHS_Type *wrapper = cfg->wrapper_base;
	int err;

	/* Disable the DWC2 core */
	wrapper->ENABLE = 0;

	err = onoff_cancel_or_release(data->pclk24m_mgr, &data->pclk24m_cli);
	if (err < 0) {
		LOG_ERR("Failed to stop PCLK24M %d", err);
		return err;
	}

	/* Affect the regulator */
	err = regulator_disable(cfg->vregusb_dev);
	if (err != 0) {
		LOG_ERR("Failed to disable %s: %d", cfg->vregusb_dev->name, err);
		return err;
	}

	k_event_set_masked(&data->events, 0, USBHS_VBUS_READY);

	return 0;
}

#define QUIRK_NRF_USBHS_NRF54L_DEFINE(n)					\
	struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {		\
		.pre_init = usbhs_pre_init,					\
		.shutdown = usbhs_shutdown,					\
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

DT_INST_FOREACH_STATUS_OKAY(QUIRK_NRF_USBHS_NRF54L_DEFINE)

#endif /* ZEPHYR_DRIVERS_UHC_DWC2_NRF_USBHS_NRF54L_H */
