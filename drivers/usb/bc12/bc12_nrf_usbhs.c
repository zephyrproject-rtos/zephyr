/*
 * Copyright Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>
#include <assert.h>
#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/usb/usb_bc12.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bc12_usbhs, CONFIG_USB_BC12_LOG_LEVEL);

struct usbhs_bc_config {
	NRF_USBHS_Type *base;
	const struct device *vregusb_dev;
};

enum usbhs_bc_state {
	USBHS_BC_WAIT_FOR_VBUS,
	USBHS_BC_DCD,
	USBHS_BC_PRIMARY_DETECTION,
	USBHS_BC_START_SECONDARY_DETECTION,
	USBHS_BC_SECONDARY_DETECTION,
	USBHS_BC_FINISHED,
	USBHS_BC_DISCONNECTED,
};

struct usbhs_bc_data {
	struct bc12_partner_state partner_state;
	struct k_timer timer;
	bc12_callback_t result_cb;
	void *result_cb_data;
	bool vbus_present;
	enum usbhs_bc_state state;
};

#define TDCD_TIMEOUT		300
#define TVDPSRC_ON		40
#define TVDMSRC_ON		40
#define TCP_VDM_EN		200
#define TVDPSRC_DIS		20
#define TVDMSRC_DIS		20

static void usbhs_bc_set_detection_result(const struct device *dev,
					  const enum bc12_type type)
{
	struct usbhs_bc_data *const data = dev->data;

	switch (type) {
	case BC12_TYPE_DCP:
		LOG_DBG("DCP");
		break;
	case BC12_TYPE_CDP:
		LOG_DBG("CDP");
		break;
	case BC12_TYPE_SDP:
		LOG_DBG("SDP");
		break;
	case BC12_TYPE_UNKNOWN:
		LOG_DBG("UNKNOWN");
		break;
	case BC12_TYPE_PROPRIETARY:
		LOG_DBG("PROPRIETARY");
		break;
	case BC12_TYPE_NONE:
		LOG_DBG("NONE");
		break;
	default:
		LOG_WRN("Unhandled BC12 type");
	}

	data->partner_state.type = type;
	if (type == BC12_TYPE_DCP || type == BC12_TYPE_CDP) {
		data->partner_state.current_ua = BC12_CHARGER_MAX_CURR_UA;
		data->partner_state.voltage_uv = BC12_CHARGER_VOLTAGE_UV;
	} else if (type == BC12_TYPE_NONE) {
		data->partner_state.current_ua = 0;
		data->partner_state.voltage_uv = 0;
	} else {
		data->partner_state.current_ua = BC12_CHARGER_MIN_CURR_UA;
		data->partner_state.voltage_uv = BC12_CHARGER_VOLTAGE_UV;
	}

	if (data->result_cb && data->state == USBHS_BC_FINISHED) {
		data->result_cb(dev, &data->partner_state, data->result_cb_data);
	}
}

static void usbhs_bc_change_state(const struct device *dev,
				  const enum usbhs_bc_state state, const int ms)
{
	struct usbhs_bc_data *const data = dev->data;

	data->state = state;
	k_timer_start(&data->timer, K_MSEC(ms), K_NO_WAIT);
}

static void usbhs_bc_finish_state(const struct device *dev)
{
	struct usbhs_bc_data *const data = dev->data;

	data->state = USBHS_BC_FINISHED;
}

static void usbhs_bc_start_dcd(const struct device *dev)
{
	usbhs_bc_change_state(dev, USBHS_BC_DCD, TDCD_TIMEOUT);
}

static void usbhs_bc_start_primary_detection(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	NRF_USBHS_Type *const wrapper = config->base;

	wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_OPMODE0_Msk |
				      USBHS_PHY_OVERRIDEVALUES_XCVRSEL0_Msk;
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_OPMODE0_Msk |
				     USBHS_PHY_INPUTOVERRIDE_XCVRSEL0_Msk |
				     USBHS_PHY_INPUTOVERRIDE_DPPULLDOWN_Msk |
				     USBHS_PHY_INPUTOVERRIDE_DMPULLDOWN_Msk |
				     USBHS_PHY_INPUTOVERRIDE_SUSPENDM0_Msk;

	/*
	 * Set BATTCHRG.
	 * Enable Attach/Connect detection and data source voltage.
	 * The voltage is sourced onto DP and sunk from DM.
	 */
	wrapper->PHY.BATTCHRG = USBHS_PHY_BATTCHRG_VDATENB0_Msk |
				USBHS_PHY_BATTCHRG_VDATSRCENB0_Msk;

	/* Add 1 ms extra to account for V_DP_SRC startup */
	usbhs_bc_change_state(dev, USBHS_BC_PRIMARY_DETECTION, TVDPSRC_ON + 1);
}

static void usbhs_bc_start_secondary_detection(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	NRF_USBHS_Type *const wrapper = config->base;

	/*
	 * Set BATTCHRG.
	 * The voltage is sourced onto DM and sunk from DP.
	 * Enable Attach/Connect detection and data source voltage.
	 */
	wrapper->PHY.BATTCHRG = USBHS_PHY_BATTCHRG_CHRGSEL0_Msk;
	wrapper->PHY.BATTCHRG = USBHS_PHY_BATTCHRG_CHRGSEL0_Msk |
				USBHS_PHY_BATTCHRG_VDATENB0_Msk |
				USBHS_PHY_BATTCHRG_VDATSRCENB0_Msk;

	/* Add 1 ms extra to account for V_DM_SRC startup */
	usbhs_bc_change_state(dev, USBHS_BC_SECONDARY_DETECTION, TVDMSRC_ON + 1);
}

static void usbhs_bc_check_secondary_detection(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	NRF_USBHS_Type *const wrapper = config->base;
	enum bc12_type type;
	uint32_t status;

	status = wrapper->PHY.BATTCHRGSTATUS;
	/* Disable VDM_SRC, IDP_SINK and VDAT_REF comparator */
	wrapper->PHY.BATTCHRG = 0;

	LOG_DBG("Status secondary detection 0x%08x", status);
	if (status & USBHS_PHY_BATTCHRGSTATUS_CHGDET_Msk) {
		/*
		 * Enable VDP_SRC onto DP and keep it enabled until DWC2 driver
		 * is ready and enables D+ pull-up.
		 */
		wrapper->PHY.BATTCHRG = USBHS_PHY_BATTCHRG_VDATSRCENB0_Msk;
		type = BC12_TYPE_DCP;
	} else {
		type = BC12_TYPE_CDP;
	}

	usbhs_bc_finish_state(dev);
	usbhs_bc_set_detection_result(dev, type);
}

static void usbhs_bc_check_primary_detection(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	NRF_USBHS_Type *const wrapper = config->base;
	bool need_secondary_detection = false;
	enum bc12_type type;
	uint32_t status;

	status = wrapper->PHY.BATTCHRGSTATUS;
	LOG_DBG("Status primary detection 0x%08x", status);
	status &= USBHS_PHY_BATTCHRGSTATUS_CHGDET_Msk | USBHS_PHY_BATTCHRGSTATUS_FSVMINUS_Msk;

	switch (status) {
	case 0:
		type = BC12_TYPE_SDP;
		break;
	case USBHS_PHY_BATTCHRGSTATUS_CHGDET_Msk:
		/* Either CDP or DCP, to be determined in Secondary Detection */
		need_secondary_detection = true;
		break;
	case USBHS_PHY_BATTCHRGSTATUS_CHGDET_Msk | USBHS_PHY_BATTCHRGSTATUS_FSVMINUS_Msk:
		/*
		 * Either prioprietary charger or PS/2 port. BC1.2 does not have
		 * any method to differentiate between the two.
		 */
		type = BC12_TYPE_PROPRIETARY;
		break;
	default:
		type = BC12_TYPE_UNKNOWN;
		break;
	}

	/* Disable VDP_SRC */
	wrapper->PHY.BATTCHRG = 0;

	if (need_secondary_detection) {
		usbhs_bc_change_state(dev, USBHS_BC_START_SECONDARY_DETECTION, TVDPSRC_DIS);
	} else {
		/* Battery Charging finished */
		usbhs_bc_finish_state(dev);
		usbhs_bc_set_detection_result(dev, type);
	}
}

static void usbhs_bc_timer_expiry(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	struct usbhs_bc_data *const data = dev->data;

	switch (data->state) {
	case USBHS_BC_DCD:
		usbhs_bc_start_primary_detection(dev);
		break;
	case USBHS_BC_PRIMARY_DETECTION:
		usbhs_bc_check_primary_detection(dev);
		break;
	case USBHS_BC_START_SECONDARY_DETECTION:
		usbhs_bc_start_secondary_detection(dev);
		break;
	case USBHS_BC_SECONDARY_DETECTION:
		usbhs_bc_check_secondary_detection(dev);
		break;
	case USBHS_BC_DISCONNECTED:
		/* TCP_VDM_EN elapsed, restart charger detection */
		if (data->vbus_present) {
			/* VBUS present - proceed to DCD */
			usbhs_bc_start_dcd(dev);
		} else {
			/* Allow DCD as soon as VBUS gets detected */
			data->state = USBHS_BC_WAIT_FOR_VBUS;
		}
		break;
	default:
		LOG_ERR("Unhandled state %u", data->state);
		break;
	}
}

static void usbhs_bc_vbus_detected(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	struct usbhs_bc_data *const data = dev->data;
	NRF_USBHS_Type *const wrapper = config->base;

	/* Power up peripheral */
	wrapper->ENABLE = USBHS_ENABLE_CORE_Msk;
	/*
	 * Set role to Device, force D+ pull-up off by overriding VBUS
	 * valid signal, and suspend PHY.
	 */
	wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_ID_Msk;
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_ID_Msk |
				     USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk |
				     USBHS_PHY_INPUTOVERRIDE_SUSPENDM0_Msk;

	/* Release PHY power-on reset */
	wrapper->ENABLE = USBHS_ENABLE_PHY_Msk | USBHS_ENABLE_CORE_Msk;

	if (data->state == USBHS_BC_WAIT_FOR_VBUS) {
		usbhs_bc_start_dcd(dev);
	} else {
		__ASSERT_NO_MSG(data->state == USBHS_BC_DISCONNECTED);
		/* Detection process will start after TCP_VDM_EN elapses */
	}
}

static void usbhs_bc_vbus_removed(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	NRF_USBHS_Type *const wrapper = config->base;

	/*
	 * Set role to Device, force D+ pull-up off by overriding VBUS
	 * valid signal.
	 */
	wrapper->PHY.OVERRIDEVALUES = USBHS_PHY_OVERRIDEVALUES_ID_Msk;
	wrapper->PHY.INPUTOVERRIDE = USBHS_PHY_INPUTOVERRIDE_ID_Msk |
				     USBHS_PHY_INPUTOVERRIDE_VBUSVALID_Msk;

	usbhs_bc_set_detection_result(dev, BC12_TYPE_NONE);
	/*
	 * The PD is required to wait for at least TCP_VDM_EN between
	 * disconnect event and restarting the detection process.
	 */
	usbhs_bc_change_state(dev, USBHS_BC_DISCONNECTED, TCP_VDM_EN);
}

static void vregusb_event_cb(const struct device *dev,
			     const struct regulator_event *const evt,
			     const void *const user_data)
{
	const struct device *const bc_dev = user_data;
	struct usbhs_bc_data *const data = bc_dev->data;

	if (evt->type == REGULATOR_VOLTAGE_DETECTED) {
		LOG_DBG("VBUS detected");
		data->vbus_present = true;
		usbhs_bc_vbus_detected(bc_dev);
	}

	if (evt->type == REGULATOR_VOLTAGE_REMOVED) {
		LOG_DBG("VBUS removed");
		data->vbus_present = false;
		usbhs_bc_vbus_removed(bc_dev);
	}
}

static int usbhs_bc_set_role(const struct device *dev, const enum bc12_role role)
{
	const struct usbhs_bc_config *const config = dev->config;
	struct usbhs_bc_data *const data = dev->data;
	int err;

	if (role == BC12_DISCONNECTED) {
		data->partner_state.type = BC12_TYPE_NONE;
		data->partner_state.bc12_role = role;
		err = regulator_disable(config->vregusb_dev);
		if (err) {
			LOG_ERR("Failed to disable regulator");
		}

		return err;
	}

	if (role == BC12_PORTABLE_DEVICE) {
		if (data->result_cb == NULL) {
			LOG_ERR("Detection result callback is not set");
			return -EIO;
		}

		data->partner_state.type = BC12_TYPE_NONE;
		data->partner_state.bc12_role = role;
		err = regulator_enable(config->vregusb_dev);
		if (err) {
			LOG_ERR("Failed to enable regulator %s error %d",
				config->vregusb_dev->name, err);
		}

		return err;
	}

	return -ENOTSUP;
}

static int usbhs_bc_set_result_cb(const struct device *dev,
				  const bc12_callback_t cb, void *const user_data)
{
	struct usbhs_bc_data *const data = dev->data;

	data->result_cb = cb;
	data->result_cb_data = user_data;

	return 0;
}

static int usbhs_bc_init(const struct device *dev)
{
	const struct usbhs_bc_config *const config = dev->config;
	struct usbhs_bc_data *const data = dev->data;
	int err;

	usbhs_bc_set_role(dev, BC12_DISCONNECTED);
	k_timer_init(&data->timer, usbhs_bc_timer_expiry, NULL);
	k_timer_user_data_set(&data->timer, (void *)dev);

	err = regulator_set_callback(config->vregusb_dev, vregusb_event_cb, dev);
	if (err) {
		LOG_ERR("Failed to set regulator callback");
		return err;
	}

	return 0;
}

static DEVICE_API(bc12, usbhs_bc_driver_api) = {
	.set_role = usbhs_bc_set_role,
	.set_result_cb = usbhs_bc_set_result_cb,
};

#define DT_DRV_COMPAT nordic_nrf_usbhs_bc12

#define BC12_NRF54L_DEFINE(n)								\
	static const struct usbhs_bc_config bc_config_##n = {				\
		.base = (void *)(DT_REG_ADDR(DT_INST_PARENT(n))),			\
		.vregusb_dev = DEVICE_DT_GET(DT_PHANDLE(DT_INST_PARENT(n), regulator)),	\
	};										\
											\
	static struct usbhs_bc_data usbhs_bc_data_##n;					\
											\
	DEVICE_DT_INST_DEFINE(n, usbhs_bc_init, NULL,					\
		      &usbhs_bc_data_##n, &bc_config_##n,				\
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		      &usbhs_bc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BC12_NRF54L_DEFINE)
