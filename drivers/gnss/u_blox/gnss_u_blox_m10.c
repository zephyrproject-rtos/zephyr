/*
 * Copyright (c) 2026 CampOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_m10

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/drivers/gnss/u_blox_m10.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/ubx/protocol.h>
#include <zephyr/modem/ubx/keys.h>
#include <zephyr/modem/backend/uart.h>

#include "gnss_u_blox_iface.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_m10, CONFIG_GNSS_LOG_LEVEL);

/* EXTINT pulse that wakes the receiver from backup mode */
#define UBX_M10_EXTINT_PULSE_MS 10
/* Time for the receiver to start up after being woken from backup mode */
#define UBX_M10_WAKEUP_TIME_MS 500
/* Maximum number of configuration items sent in one UBX-CFG-VALSET */
#define UBX_M10_CFG_ITEMS_MAX 8

struct ubx_m10_config {
	/* First member: the u-blox interface accesses dev->config as its own configuration */
	struct u_blox_iface_config iface;
	struct gpio_dt_spec extint_gpio;
	/* UBX-CFG-VALSET layers holding the driver's receiver configuration */
	uint8_t cfg_layers;
	struct {
		uint8_t operate_mode;
		uint32_t update_period_s;
		uint32_t acq_period_s;
		uint16_t on_time_s;
		uint8_t max_acq_time_s;
		bool extint_wake;
		bool extint_backup;
		uint32_t extint_inactivity_ms;
	} pm;
};

BUILD_ASSERT(offsetof(struct ubx_m10_config, iface) == 0);

static inline uint8_t ubx_m10_cfg_layers(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;

	return cfg->cfg_layers;
}

struct ubx_m10_cfg_item {
	uint32_t key;
	uint32_t value;
};

MODEM_UBX_MATCH_ARRAY_DEFINE(u_blox_m10_unsol_messages,
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_PVT,
			       gnss_ubx_common_pvt_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_SAT,
			       gnss_ubx_common_satellite_callback),
#endif
);

/* Size in bytes of a configuration item's value, given by bits 30..28 of its key ID */
static size_t ubx_m10_cfg_value_size(uint32_t key)
{
	switch ((key >> 28) & 0x7U) {
	case 1U: /* One bit, stored in one byte */
	case 2U:
		return 1U;
	case 3U:
		return 2U;
	case 4U:
		return 4U;
	default:
		return 0U;
	}
}

static int ubx_m10_cfg_set(const struct device *dev, uint8_t layers,
			   const struct ubx_m10_cfg_item *items, size_t count)
{
	uint8_t payload[sizeof(struct ubx_cfg_val_hdr) +
			UBX_M10_CFG_ITEMS_MAX * (sizeof(uint32_t) + sizeof(uint32_t))] = {
		UBX_CFG_VAL_VER_SIMPLE,
		layers,
	};
	size_t len = sizeof(struct ubx_cfg_val_hdr);

	if (count > UBX_M10_CFG_ITEMS_MAX) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		size_t value_size = ubx_m10_cfg_value_size(items[i].key);

		if (value_size == 0U) {
			return -EINVAL;
		}

		sys_put_le32(items[i].key, &payload[len]);
		len += sizeof(uint32_t);

		for (size_t j = 0; j < value_size; j++) {
			payload[len++] = (uint8_t)(items[i].value >> (8U * j));
		}
	}

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					     payload, len, true);
}

/*
 * Power save and EXTINT settings. They are always written to RAM and BBR, so that the BBR layer
 * never holds a stale power mode. OPERATEMODE is set last: in PSMOO the receiver may enter its
 * off state as soon as it is set.
 */
static int ubx_m10_send_pm_config(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	const uint8_t layers = UBX_CFG_VAL_LAYER_RAM | UBX_CFG_VAL_LAYER_BBR;
	const struct ubx_m10_cfg_item pm[] = {
		{UBX_KEY_PM_POSUPDATEPERIOD, cfg->pm.update_period_s},
		{UBX_KEY_PM_ACQPERIOD, cfg->pm.acq_period_s},
		{UBX_KEY_PM_ONTIME, cfg->pm.on_time_s},
		{UBX_KEY_PM_MAXACQTIME, cfg->pm.max_acq_time_s},
		{UBX_KEY_PM_EXTINTWAKE, cfg->pm.extint_wake ? 1U : 0U},
		{UBX_KEY_PM_EXTINTBACKUP, cfg->pm.extint_backup ? 1U : 0U},
		{UBX_KEY_PM_EXTINTINACTIVE, (cfg->pm.extint_inactivity_ms > 0U) ? 1U : 0U},
		{UBX_KEY_PM_EXTINTINACTIVITY, cfg->pm.extint_inactivity_ms},
	};
	const struct ubx_m10_cfg_item operate_mode = {
		UBX_KEY_PM_OPERATEMODE, cfg->pm.operate_mode,
	};
	int err;

	err = ubx_m10_cfg_set(dev, layers, pm, ARRAY_SIZE(pm));
	if (err == 0) {
		err = ubx_m10_cfg_set(dev, layers, &operate_mode, 1);
	}

	if (err != 0) {
		LOG_ERR("Failed to set power management configuration: %d", err);
	}

	return err;
}

static int ubx_m10_send_init_config(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	static const struct ubx_m10_cfg_item init_cfg[] = {
		{UBX_KEY_UART1_PROTO_OUT_NMEA, 0U},
		{UBX_KEY_MSG_OUT_UBX_NAV_PVT_UART1, 1U},
		{UBX_KEY_NAV_CFG_FIX_MODE, UBX_FIX_MODE_AUTO},
		{UBX_KEY_UART1_PROTO_IN_UBX, 1U},
		{UBX_KEY_UART1_PROTO_OUT_UBX, 1U},
#if CONFIG_GNSS_SATELLITES
		{UBX_KEY_MSG_OUT_UBX_NAV_SAT_UART1, 1U},
#endif
	};
	int err;

	err = gnss_set_fix_rate(dev, cfg->iface.fix_rate_ms);
	if (err != 0) {
		LOG_ERR("Failed to set fix-rate: %d", err);
		return err;
	}

	err = ubx_m10_cfg_set(dev, cfg->cfg_layers, init_cfg, ARRAY_SIZE(init_cfg));
	if (err != 0) {
		LOG_ERR("Failed to send init configuration: %d", err);
		return err;
	}

	if ((cfg->cfg_layers & UBX_CFG_VAL_LAYER_BBR) != 0U) {
		/* The receiver restarts from BBR after backup: keep the baudrate there too */
		const struct ubx_m10_cfg_item baudrate = {
			UBX_KEY_CFG_UART1_BAUDRATE, cfg->iface.baudrate.desired,
		};

		err = ubx_m10_cfg_set(dev, UBX_CFG_VAL_LAYER_BBR, &baudrate, 1);
		if (err != 0) {
			LOG_ERR("Failed to store baudrate: %d", err);
			return err;
		}
	}

	return ubx_m10_send_pm_config(dev);
}

static int u_blox_m10_init(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	int err;

	const static struct ubx_frame version_get = UBX_FRAME_GET_INITIALIZER(
						UBX_CLASS_ID_MON, UBX_MSG_ID_MON_VER);
	struct ubx_mon_ver ver;

	if (cfg->extint_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->extint_gpio)) {
			LOG_ERR("EXTINT GPIO not ready");
			return -ENODEV;
		}

		/* Active (high) keeps the receiver awake while the driver configures it */
		err = gpio_pin_configure_dt(&cfg->extint_gpio, GPIO_OUTPUT_ACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure EXTINT GPIO: %d", err);
			return err;
		}
	}

	err = u_blox_iface_init(dev, u_blox_m10_unsol_messages,
			 ARRAY_SIZE(u_blox_m10_unsol_messages), true);
	if (err < 0) {
		return err;
	}

	err = u_blox_iface_msg_get(dev, &version_get,
				   UBX_FRAME_SZ(version_get.payload_size),
				   (void *)&ver, sizeof(ver));
	if (err != 0) {
		LOG_ERR("Failed to get Modem version info: %d", err);
		return err;
	}
	LOG_INF("SW Version %s, HW Version: %s", ver.sw_ver, ver.hw_ver);

	return ubx_m10_send_init_config(dev);
}

static int ubx_m10_start(const struct device *dev, enum gnss_start_mode mode)
{
	struct ubx_cfg_rst rst = { .reserved = 0 };

	switch (mode) {
	case GNSS_HOT_START:
		rst.nav_bbr_mask = (uint16_t)UBX_CFG_RST_HOT_START;
		rst.reset_mode = (uint8_t)UBX_CFG_RST_MODE_GNSS_START;
		break;

	case GNSS_WARM_START:
		rst.nav_bbr_mask = (uint16_t)UBX_CFG_RST_WARM_START;
		rst.reset_mode = (uint8_t)UBX_CFG_RST_MODE_GNSS_START;
		break;

	case GNSS_COLD_START:
		rst.nav_bbr_mask = (uint16_t)UBX_CFG_RST_COLD_START;
		rst.reset_mode = (uint8_t)UBX_CFG_RST_MODE_GNSS_START;
		break;

	default:
		return -EINVAL;
	}

	/* CFG-RST never ACKs — send fire-and-forget, then let the engine settle. */
	u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RST,
				       (const uint8_t *)&rst, sizeof(rst), false);

	k_sleep(K_MSEC(100));

	if (mode == GNSS_HOT_START) {
		/* GNSS-only restart: RAM config is retained, nothing to reapply. */
		return 0;
	}

	/* SW reset (warm/cold): RAM config is lost and must be reapplied. */
	return ubx_m10_send_init_config(dev);
}

static int ubx_m10_stop(const struct device *dev)
{
	struct ubx_cfg_rst rst = {
		.nav_bbr_mask = 0,
		.reset_mode = UBX_CFG_RST_MODE_GNSS_STOP,
		.reserved = 0,
	};

	/* CFG-RST never ACKs — send fire-and-forget, then let the engine settle. */
	u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RST,
				       (const uint8_t *)&rst, sizeof(rst), false);

	k_sleep(K_MSEC(100));

	return 0;
}

static int ubx_m10_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	if (fix_interval_ms < 50 || fix_interval_ms > 65535) {
		return -EINVAL;
	}

	struct ubx_cfg_val_u16 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer = ubx_m10_cfg_layers(dev),
		},
		.key = UBX_KEY_RATE_MEAS,
		.value = fix_interval_ms,
	};

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG,
					     UBX_MSG_ID_CFG_VAL_SET,
					     (const uint8_t *)&rate, sizeof(rate),
					     true);
}

static int ubx_m10_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct ubx_cfg_val_u16 rate;
	int err;

	const static struct ubx_frame get_fix_rate =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_RATE_MEAS);

	err = u_blox_iface_msg_get(dev, &get_fix_rate,
				    UBX_FRAME_SZ(get_fix_rate.payload_size),
				    (void *)&rate, sizeof(rate));

	if (err != 0) {
		LOG_ERR("Failed to retrieve fix rate: %d", err);
		return err;
	}

	*fix_interval_ms = rate.value;

	return 0;
}

/** As this GNSS modem may be used for many applications, the definition of
 * High Dynamics Navigation mode is configurable through Kconfig, in order to
 * maintain a balance between API re-usability and flexibility.
 */
#define GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN							\
	COND_CASE_1(CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_1G,			\
		    (UBX_DYN_MODEL_AIRBORNE_1G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_2G,			\
		    (UBX_DYN_MODEL_AIRBORNE_2G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_4G,			\
		    (UBX_DYN_MODEL_AIRBORNE_4G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AUTOMOTIVE,			\
		    (UBX_DYN_MODEL_AUTOMOTIVE),							\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_SEA,				\
		    (UBX_DYN_MODEL_SEA),							\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_BIKE,				\
		    (UBX_DYN_MODEL_BIKE),							\
		    (UBX_DYN_MODEL_AIRBORNE_2G))

static int ubx_m10_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
{
	enum ubx_dyn_model nav_model;

	struct ubx_cfg_val_u8 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer = ubx_m10_cfg_layers(dev),
		},
		.key = UBX_KEY_NAV_CFG_DYN_MODEL,
	};

	switch (mode) {
	case GNSS_NAVIGATION_MODE_ZERO_DYNAMICS:
		nav_model = UBX_DYN_MODEL_STATIONARY;
		break;
	case GNSS_NAVIGATION_MODE_LOW_DYNAMICS:
		nav_model = UBX_DYN_MODEL_PEDESTRIAN;
		break;
	case GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS:
		nav_model = UBX_DYN_MODEL_PORTABLE;
		break;
	case GNSS_NAVIGATION_MODE_HIGH_DYNAMICS:
		nav_model = GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN;
		break;
	default:
		return -EINVAL;
	}

	rate.value = nav_model;

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					     (const uint8_t *)&rate, sizeof(rate),
					     true);
}

static int ubx_m10_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	int err;
	struct ubx_cfg_val_u8 nav_mode;

	const static struct ubx_frame get_nav_mode =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_NAV_CFG_DYN_MODEL);

	err = u_blox_iface_msg_get(dev, &get_nav_mode,
				   UBX_FRAME_SZ(get_nav_mode.payload_size),
				   (void *)&nav_mode, sizeof(nav_mode));
	if (err != 0) {
		LOG_ERR("Failed to get navigation mode: %d", err);
		return err;
	}

	switch (nav_mode.value) {
	case UBX_DYN_MODEL_STATIONARY:
		*mode = GNSS_NAVIGATION_MODE_ZERO_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PEDESTRIAN:
		*mode = GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PORTABLE:
		*mode = GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
		break;
	case UBX_DYN_MODEL_AIRBORNE_1G:
	case UBX_DYN_MODEL_AIRBORNE_2G:
	case UBX_DYN_MODEL_AIRBORNE_4G:
	case UBX_DYN_MODEL_AUTOMOTIVE:
	case UBX_DYN_MODEL_SEA:
	case UBX_DYN_MODEL_BIKE:
		LOG_WRN("Returning generic HIGH_DYNAMICS nav mode. "
			"UBX DYN Model: %d was set", nav_mode.value);
		*mode = GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
		break;
	default:
		return -EIO;
	}

	return err;
}

static int ubx_m10_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	return -ENOTSUP;
}

static void transform_ubx_selection_to_gnss_systems(uint8_t selection, gnss_systems_t *systems)
{
	*systems = 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GPS) ?
			GNSS_SYSTEM_GPS : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GLONASS) ?
			GNSS_SYSTEM_GLONASS : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_BEIDOU) ?
			GNSS_SYSTEM_BEIDOU : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GALILEO) ?
			GNSS_SYSTEM_GALILEO : 0;
}

static int ubx_m10_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_enabled_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = u_blox_iface_msg_get(dev, &get_enabled_systems,
				   UBX_FRAME_SZ(get_enabled_systems.payload_size),
				   (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	transform_ubx_selection_to_gnss_systems(gnss_selection.selection.enabled, systems);
	return 0;
}

static int ubx_m10_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_support_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = u_blox_iface_msg_get(dev, &get_support_systems,
				   UBX_FRAME_SZ(get_support_systems.payload_size),
				   (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	transform_ubx_selection_to_gnss_systems(gnss_selection.selection.supported, systems);
	return 0;
}

int gnss_u_blox_m10_set_extint(const struct device *dev, bool active)
{
	const struct ubx_m10_config *cfg = dev->config;

	if (cfg->extint_gpio.port == NULL) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&cfg->extint_gpio, active ? 1 : 0);
}

#ifdef CONFIG_PM_DEVICE
static int ubx_m10_pm_suspend(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	struct ubx_rxm_pmreq pmreq = {
		.version = 0,
		.duration_ms = 0,
		.flags = UBX_RXM_PMREQ_FLAGS_BACKUP | UBX_RXM_PMREQ_FLAGS_FORCE,
		.wakeup_sources = (cfg->extint_gpio.port != NULL) ?
				  UBX_RXM_PMREQ_WAKEUP_EXTINT0 : UBX_RXM_PMREQ_WAKEUP_UARTRX,
	};
	int err;

	/* The receiver enters backup mode without acknowledging the request */
	err = u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_RXM, UBX_MSG_ID_RXM_PMREQ,
					    (const uint8_t *)&pmreq, sizeof(pmreq), false);
	if (err < 0) {
		LOG_ERR("Failed to request backup mode: %d", err);
		return err;
	}

	return 0;
}

static int ubx_m10_pm_resume(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	int err;

	if (cfg->extint_gpio.port != NULL) {
		/* Two edges, leaving EXTINT at the level the application last set */
		err = gpio_pin_toggle_dt(&cfg->extint_gpio);
		if (err == 0) {
			k_sleep(K_MSEC(UBX_M10_EXTINT_PULSE_MS));
			err = gpio_pin_toggle_dt(&cfg->extint_gpio);
		}
	} else {
		/* Any activity on the receiver's UART RX line wakes it up; the frame is lost */
		err = u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_MON, UBX_MSG_ID_MON_VER,
						    NULL, 0, false);
	}

	if (err < 0) {
		LOG_ERR("Failed to wake up receiver: %d", err);
		return err;
	}

	k_sleep(K_MSEC(UBX_M10_WAKEUP_TIME_MS));

	err = u_blox_iface_resync(dev, true);
	if (err < 0) {
		return err;
	}

	/* The receiver restarts when leaving backup mode: reapply the configuration */
	return ubx_m10_send_init_config(dev);
}

static int ubx_m10_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return ubx_m10_pm_suspend(dev);
	case PM_DEVICE_ACTION_RESUME:
		return ubx_m10_pm_resume(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(gnss, ublox_m10_driver_api) = {
	.start = ubx_m10_start,
	.stop = ubx_m10_stop,
	.set_fix_rate = ubx_m10_set_fix_rate,
	.get_fix_rate = ubx_m10_get_fix_rate,
	.set_navigation_mode = ubx_m10_set_navigation_mode,
	.get_navigation_mode = ubx_m10_get_navigation_mode,
	.set_enabled_systems = ubx_m10_set_enabled_systems,
	.get_enabled_systems = ubx_m10_get_enabled_systems,
	.get_supported_systems = ubx_m10_get_supported_systems,
};

/* The receiver can enter backup on its own, losing its RAM configuration */
#define UBX_M10_SELF_BACKUP(inst)								\
	((DT_INST_ENUM_IDX(inst, power_save_mode) != UBX_PM_OPERATEMODE_FULL) ||		\
	 DT_INST_PROP(inst, extint_backup) ||							\
	 (DT_INST_PROP(inst, extint_inactivity_timeout_ms) > 0))

#define UBX_M10(inst)										\
												\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, extint_gpios) ||				\
		     !(DT_INST_PROP(inst, extint_wake) || DT_INST_PROP(inst, extint_backup) ||	\
		       (DT_INST_PROP(inst, extint_inactivity_timeout_ms) > 0)),		\
		     "EXTINT control requires extint-gpios");					\
	BUILD_ASSERT((DT_INST_PROP(inst, psm_update_period_seconds) == 0) ||			\
		     (DT_INST_PROP(inst, psm_update_period_seconds) >= 5),			\
		     "psm-update-period-seconds must be 0 or at least 5");			\
	BUILD_ASSERT(DT_INST_PROP(inst, psm_on_time_seconds) <= UINT16_MAX,			\
		     "psm-on-time-seconds out of range");					\
	BUILD_ASSERT(DT_INST_PROP(inst, psm_max_acquisition_time_seconds) <= UINT8_MAX,		\
		     "psm-max-acquisition-time-seconds out of range");				\
												\
	static const struct ubx_m10_config u_blox_m10_cfg_##inst = {				\
		.iface = {									\
			.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
			.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),		\
			.fix_rate_ms = DT_INST_PROP(inst, fix_rate),				\
			.baudrate = {								\
				.initial = DT_INST_PROP(inst, initial_baudrate),		\
				.desired = DT_PROP(DT_INST_BUS(inst), current_speed),		\
			},									\
		},										\
		.extint_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, extint_gpios, {}),		\
		.cfg_layers = UBX_CFG_VAL_LAYER_RAM |						\
			      (UBX_M10_SELF_BACKUP(inst) ? UBX_CFG_VAL_LAYER_BBR : 0),	\
		.pm = {										\
			.operate_mode = DT_INST_ENUM_IDX(inst, power_save_mode),		\
			.update_period_s = DT_INST_PROP(inst, psm_update_period_seconds),	\
			.acq_period_s = DT_INST_PROP(inst, psm_acquisition_period_seconds),	\
			.on_time_s = DT_INST_PROP(inst, psm_on_time_seconds),			\
			.max_acq_time_s = DT_INST_PROP(inst, psm_max_acquisition_time_seconds),	\
			.extint_wake = DT_INST_PROP(inst, extint_wake),				\
			.extint_backup = DT_INST_PROP(inst, extint_backup),			\
			.extint_inactivity_ms =							\
				DT_INST_PROP(inst, extint_inactivity_timeout_ms),		\
		},										\
	};											\
												\
	static struct u_blox_iface_data u_blox_m10_data_##inst;					\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, ubx_m10_pm_action);					\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      u_blox_m10_init,							\
			      PM_DEVICE_DT_INST_GET(inst),					\
			      &u_blox_m10_data_##inst,						\
			      &u_blox_m10_cfg_##inst,						\
			      POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY,				\
			      &ublox_m10_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_M10)
