/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <nrf_usbhs_wrapper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbhs, LOG_LEVEL_ERR);

struct usbhs_wrapper_config {
	NRF_USBHS_Type *base;
	const struct device *vregusb_dev;
};

struct nrf_usbhs_wrapper_data {
	const struct device *dev;
	struct k_event events;
	regulator_callback_t bc12_cb;
	const void *bc12_cb_data;
	regulator_callback_t phy_cb;
	const void *phy_cb_data;
	atomic_t refcount;
	struct k_spinlock lock;
};

static void vregusb_event_cb(const struct device *vreg_dev,
			     const struct regulator_event *const evt,
			     const void *const user_data)
{
	const struct device *const dev = user_data;
	const struct usbhs_wrapper_config *const config = dev->config;
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	if (evt->type == REGULATOR_VOLTAGE_DETECTED) {
		LOG_DBG("VBUS detected");
		k_event_post(&data->events, NRF_USBHS_VREG_READY);
		if (!IS_ENABLED(CONFIG_USB_BC12_NRF_USBHS)) {
			/*
			 * If the USB BC12 driver is not enabled, post the
			 * event immediately.
			 */
			LOG_INF("USB BC12 driver disabled, post PHY ready");
			k_event_post(&data->events, NRF_USBHS_PHY_READY);
		}
	}

	if (evt->type == REGULATOR_VOLTAGE_REMOVED) {
		LOG_DBG("VBUS removed");
		k_event_set_masked(&data->events, 0, UINT32_MAX);
	}

	if (data->bc12_cb != NULL) {
		data->bc12_cb(config->vregusb_dev, evt, data->bc12_cb_data);
	}

	if (data->phy_cb != NULL) {
		data->phy_cb(config->vregusb_dev, evt, data->phy_cb_data);
	}

}

void nrf_usbhs_wrapper_start(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	NRF_USBHS_Type *wrapper = config->base;

	wrapper->TASKS_START = 1UL;
}

void nrf_usbhs_wrapper_stop(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	NRF_USBHS_Type *wrapper = config->base;

	wrapper->TASKS_STOP = 1UL;
}

static void wrapper_enable_phy_internal(const struct device *dev, const bool suspended)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	NRF_USBHS_Type *wrapper = config->base;
	uint32_t inputoverride;

	/* Power up peripheral */
	wrapper->ENABLE = USBHS_ENABLE_CORE_Msk;

	/* Set role to Device, force D+ pull-up off by overriding VBUS valid signal */
	if (suspended) {
		/* Additionally suspend PHY */
		inputoverride = USBHS_PHY_INPUTOVERRIDE_ID_Msk |
				USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk |
				USBHS_PHY_INPUTOVERRIDE_SUSPENDM0_Msk;
	} else {
		inputoverride = USBHS_PHY_INPUTOVERRIDE_ID_Msk |
				USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk;
	}

	wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_ID_Msk;
	wrapper->PHY.INPUTOVERRIDE = inputoverride;

	/* Release PHY power-on reset */
	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;
}

static void wrapper_enable_and_start_internal(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	NRF_USBHS_Type *wrapper = config->base;

	if (IS_ENABLED(CONFIG_USB_BC12_NRF_USBHS)) {
		/* Clear suspend bit set by the BC12 driver */
		wrapper->PHY.INPUTOVERRIDE &= ~USBHS_PHY_INPUTOVERRIDE_SUSPENDM0_Msk;
	} else {
		wrapper_enable_phy_internal(dev, false);
	}

	/* Wait for PHY clock to start */
	k_busy_wait(45);

	/* Release DWC2 reset */
	nrf_usbhs_wrapper_start(dev);

	/* Wait for clock to start to avoid hang on too early register read */
	k_busy_wait(1);

	/* DWC2 opmode is now guaranteed to be Non-Driving, allow D+ pull-up to
	 * become active once driver clears DCTL SftDiscon bit.
	 */
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_ID_Msk;

	if (IS_ENABLED(CONFIG_USB_BC12_NRF_USBHS)) {
		wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_ID_Msk;
		wrapper->PHY.BATTCHRG &= ~USBHS_PHY_BATTCHRG_VDATSRCENB0_Msk;
	}
}

void nrf_usbhs_wrapper_enable_phy(const struct device *dev, const bool suspended)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	wrapper_enable_phy_internal(dev, suspended);
	atomic_inc(&data->refcount);
	LOG_INF("Enable %s, refcount %lu",
		"USB peripheral", atomic_get(&data->refcount));
}

void nrf_usbhs_wrapper_enable_udc(const struct device *dev)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	wrapper_enable_and_start_internal(dev);
	k_spin_unlock(&data->lock, key);
	atomic_inc(&data->refcount);
	LOG_INF("Enable and start %s, refcount %lu",
		"USB peripheral", atomic_get(&data->refcount));
}

void nrf_usbhs_wrapper_bc12_finished(const struct device *dev)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	LOG_INF("BC12 detection finished, refcount %lu", atomic_get(&data->refcount));

	k_event_post(&data->events, NRF_USBHS_PHY_READY);
	if (atomic_get(&data->refcount) > 1) {
		LOG_INF("Re-enable and start %s", "USB peripheral");
		/*
		 * The UDC controller enables the PHY and controller
		 * unconditionally and wants to keep it enabled. Otherwise, the
		 * PHY will remain suspended, and the controller will be
		 * disabled after reconnecting and finished BC12 driver
		 * detection.
		 */
		wrapper_enable_and_start_internal(dev);
	}
}

void nrf_usbhs_wrapper_disable(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	struct nrf_usbhs_wrapper_data *const data = dev->data;
	NRF_USBHS_Type *wrapper = config->base;
	atomic_val_t prev_refcount;
	k_spinlock_key_t key;

	prev_refcount = atomic_dec(&data->refcount);
	if (prev_refcount != 1) {
		LOG_INF("Keep PHY and %s enabled, previous refcount %lu",
			"USB peripheral", prev_refcount);
		return;
	}

	key = k_spin_lock(&data->lock);
	/* Set ID to Device and forcefully disable D+ pull-up */
	wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_ID_Msk;
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_ID_Msk |
				     USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk;

	wrapper->ENABLE = 0UL;
	k_spin_unlock(&data->lock, key);
	LOG_INF("Disable %s, refcount %lu",
		"USB peripheral", atomic_get(&data->refcount));
}

struct k_event *nrf_usbhs_wrapper_get_events_ptr(const struct device *dev)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	return &data->events;
}

void nrf_usbhs_wrapper_set_bc12_cb(const struct device *dev,
				   regulator_callback_t cb,
				   const void *const user_data)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	data->bc12_cb = cb;
	data->bc12_cb_data = user_data;
}

void nrf_usbhs_wrapper_set_udc_cb(const struct device *dev,
				  regulator_callback_t cb,
				  const void *const user_data)
{
	struct nrf_usbhs_wrapper_data *const data = dev->data;

	data->phy_cb = cb;
	data->phy_cb_data = user_data;
}

int nrf_usbhs_wrapper_vreg_enable(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;

	return regulator_enable(config->vregusb_dev);
}

int nrf_usbhs_wrapper_vreg_disable(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;

	return regulator_disable(config->vregusb_dev);
}

static int usbhs_wrapper_init(const struct device *dev)
{
	const struct usbhs_wrapper_config *const config = dev->config;
	struct nrf_usbhs_wrapper_data *const data = dev->data;
	int err;

	k_event_init(&data->events);
	atomic_set(&data->refcount, 0);

	err = regulator_set_callback(config->vregusb_dev, vregusb_event_cb, dev);
	if (err) {
		LOG_ERR("Failed to set regulator callback");
		return err;
	}

	return 0;
}

#define DT_DRV_COMPAT nordic_nrf_usbhs_wrapper

#define NRF_USBHS_WRAPPER_DEFINE(n)						\
	static const struct usbhs_wrapper_config usbhs_wrapper_config_##n = {	\
		.base = (void *)(DT_INST_REG_ADDR(n)),				\
		.vregusb_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, regulator)),	\
	};									\
										\
	static struct nrf_usbhs_wrapper_data usbhs_wrapper_data_##n;		\
										\
	DEVICE_DT_INST_DEFINE(n, usbhs_wrapper_init, NULL,			\
		      &usbhs_wrapper_data_##n, &usbhs_wrapper_config_##n,	\
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(NRF_USBHS_WRAPPER_DEFINE)
