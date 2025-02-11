/** @file
 *  @brief GATT Device Information Service
 */

/*
 * Copyright (c) 2019 Demant
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/byteorder.h>

#define LOG_LEVEL CONFIG_BT_SERVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_dis);

#if CONFIG_BT_DIS_PNP
struct dis_pnp {
	uint8_t pnp_vid_src;
	uint16_t pnp_vid;
	uint16_t pnp_pid;
	uint16_t pnp_ver;
} __packed;

static struct dis_pnp dis_pnp_id = {
	.pnp_vid_src = CONFIG_BT_DIS_PNP_VID_SRC,
	.pnp_vid = CONFIG_BT_DIS_PNP_VID,
	.pnp_pid = CONFIG_BT_DIS_PNP_PID,
	.pnp_ver = CONFIG_BT_DIS_PNP_VER,
};
#endif

#if defined(CONFIG_BT_DIS_SYSTEM_ID)
/*
 * Casting to uint64_t since the value is at most 5 bytes, but will appear as a 32-bit literal if it
 * is less, giving a warning when right-shifting by 32.
 */
static uint8_t dis_system_id[8] = {BT_BYTES_LIST_LE40((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER),
				   BT_BYTES_LIST_LE24(CONFIG_BT_DIS_SYSTEM_ID_OUI)};
#endif

#if defined(CONFIG_BT_DIS_SETTINGS)
#if defined(CONFIG_BT_DIS_MODEL_NUMBER)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_MODEL_NUMBER_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_model[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_MODEL_NUMBER_STR;
#elif defined(CONFIG_BT_DIS_MODEL_DEPRECATED_USED)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_MODEL) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_model[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_MODEL;
#endif
#if defined(CONFIG_BT_DIS_MANUF_NAME)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_MANUF_NAME_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_manuf[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_MANUF_NAME_STR;
#elif defined(CONFIG_BT_DIS_MANUF_DEPRECATED_USED)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_MANUF) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_manuf[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_MANUF;
#endif
#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_SERIAL_NUMBER_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_serial_number[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_SERIAL_NUMBER_STR;
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_FW_REV_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_fw_rev[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_FW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_HW_REV_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_hw_rev[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_HW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_SW_REV_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_sw_rev[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_SW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_UDI)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_UDI_LABEL_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_UDI_DI_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_UDI_ISSUER_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_UDI_AUTHORITY_STR) <= CONFIG_BT_DIS_STR_MAX + 1);

static uint8_t dis_udi_label[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_UDI_LABEL_STR;
static uint8_t dis_udi_di[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_UDI_DI_STR;
static uint8_t dis_udi_issuer[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_UDI_ISSUER_STR;
static uint8_t dis_udi_authority[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_UDI_AUTHORITY_STR;
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
BUILD_ASSERT(sizeof(CONFIG_BT_DIS_IEEE_RCDL_STR) <= CONFIG_BT_DIS_STR_MAX + 1);
static uint8_t dis_ieee_rcdl[CONFIG_BT_DIS_STR_MAX + 1] = CONFIG_BT_DIS_IEEE_RCDL_STR;
#endif

#define BT_DIS_MODEL_REF             dis_model
#define BT_DIS_MANUF_REF             dis_manuf
#define BT_DIS_SERIAL_NUMBER_STR_REF dis_serial_number
#define BT_DIS_FW_REV_STR_REF        dis_fw_rev
#define BT_DIS_HW_REV_STR_REF        dis_hw_rev
#define BT_DIS_SW_REV_STR_REF        dis_sw_rev
#define BT_DIS_UDI_LABEL_STR_REF     dis_udi_label
#define BT_DIS_UDI_DI_STR_REF        dis_udi_di
#define BT_DIS_UDI_ISSUER_STR_REF    dis_udi_issuer
#define BT_DIS_UDI_AUTHORITY_STR_REF dis_udi_authority
#define BT_DIS_IEEE_RCDL_STR_REF     dis_ieee_rcdl

/*
 * When assigning too long string literals to the arrays,
 * the literals may be truncated, removing the null terminator.
 * Using strnlen to avoid sending data outside the array.
 */

#else /* CONFIG_BT_DIS_SETTINGS */

#if defined(CONFIG_BT_DIS_MODEL_NUMBER)
#define BT_DIS_MODEL_REF             CONFIG_BT_DIS_MODEL_NUMBER_STR
#elif defined(CONFIG_BT_DIS_MODEL_DEPRECATED_USED)
#define BT_DIS_MODEL_REF             CONFIG_BT_DIS_MODEL
#endif
#if defined(CONFIG_BT_DIS_MANUF_NAME)
#define BT_DIS_MANUF_REF             CONFIG_BT_DIS_MANUF_NAME_STR
#elif defined(CONFIG_BT_DIS_MANUF_DEPRECATED_USED)
#define BT_DIS_MANUF_REF             CONFIG_BT_DIS_MANUF
#endif
#define BT_DIS_SERIAL_NUMBER_STR_REF CONFIG_BT_DIS_SERIAL_NUMBER_STR
#define BT_DIS_FW_REV_STR_REF        CONFIG_BT_DIS_FW_REV_STR
#define BT_DIS_HW_REV_STR_REF        CONFIG_BT_DIS_HW_REV_STR
#define BT_DIS_SW_REV_STR_REF        CONFIG_BT_DIS_SW_REV_STR
#define BT_DIS_UDI_LABEL_STR_REF     CONFIG_BT_DIS_UDI_LABEL_STR
#define BT_DIS_UDI_DI_STR_REF        CONFIG_BT_DIS_UDI_DI_STR
#define BT_DIS_UDI_ISSUER_STR_REF    CONFIG_BT_DIS_UDI_ISSUER_STR
#define BT_DIS_UDI_AUTHORITY_STR_REF CONFIG_BT_DIS_UDI_AUTHORITY_STR
#define BT_DIS_IEEE_RCDL_STR_REF     CONFIG_BT_DIS_IEEE_RCDL_STR

#endif /* CONFIG_BT_DIS_SETTINGS */

#define BT_DIS_READ_STR_USED \
	(CONFIG_BT_DIS_MODEL_NUMBER || CONFIG_BT_DIS_MODEL_DEPRECATED_USED || \
	 CONFIG_BT_DIS_MANUF_NAME || CONFIG_BT_DIS_MANUF_DEPRECATED_USED || \
	 CONFIG_BT_DIS_SERIAL_NUMBER || CONFIG_BT_DIS_FW_REV || CONFIG_BT_DIS_HW_REV || \
	 CONFIG_BT_DIS_SW_REV || CONFIG_BT_DIS_IEEE_RCDL)

#if BT_DIS_READ_STR_USED
static ssize_t read_str(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 strlen(attr->user_data));
}
#endif

#if CONFIG_BT_DIS_PNP
static ssize_t read_pnp_id(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dis_pnp_id, sizeof(dis_pnp_id));
}
#endif

#if CONFIG_BT_DIS_SYSTEM_ID
static ssize_t read_system_id(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_system_id,
				 sizeof(dis_system_id));
}
#endif

#if CONFIG_BT_DIS_UDI
#define DIS_STR_SIZE(x)           ((x[0]) != '\0' ? strlen(x) + sizeof('\0') : 0)
#define BT_DIS_UDI_FLAG_LABEL     (!!BT_DIS_UDI_LABEL_STR_REF[0])
#define BT_DIS_UDI_FLAG_DI        (!!BT_DIS_UDI_DI_STR_REF[0])
#define BT_DIS_UDI_FLAG_ISSUER    (!!BT_DIS_UDI_ISSUER_STR_REF[0])
#define BT_DIS_UDI_FLAG_AUTHORITY (!!BT_DIS_UDI_AUTHORITY_STR_REF[0])
#define BT_DIS_UDI_FLAGS                                                                           \
	(BT_DIS_UDI_FLAG_LABEL | (BT_DIS_UDI_FLAG_DI << 1) | (BT_DIS_UDI_FLAG_ISSUER << 2) |       \
	 (BT_DIS_UDI_FLAG_AUTHORITY << 3))

/*
 * UDI for medical devices contains a flag and 4 different null-terminated strings that may have
 * unknown length. This requires its own encode method.
 */
static void read_udi_subval(const char *str, uint16_t val_len, char *buf, uint16_t *bytes_read,
			    uint16_t *index, uint16_t len, uint16_t offset)
{
	/* String should not be with included null-terminator if empty */
	if (val_len == sizeof('\0')) {
		return;
	}

	if (*bytes_read == len) {
		return;
	}

	if (*index + val_len < offset) {
		*index += val_len;
		return;
	}

	for (uint16_t i = 0; i < val_len; i++) {
		if (*index >= offset && *bytes_read < len) {
			buf[*bytes_read] = str[i];

			(*bytes_read)++;
		}

		(*index)++;
	}
}

static ssize_t read_udi(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	uint16_t bytes_read = 0;

	char *buf_i = (char *)buf;

	/* Flag */
	uint16_t index = sizeof(uint8_t);

	if (offset == 0) {
		buf_i[0] = BT_DIS_UDI_FLAGS;
		bytes_read = 1U;
	}

	read_udi_subval(BT_DIS_UDI_LABEL_STR_REF, DIS_STR_SIZE(BT_DIS_UDI_LABEL_STR_REF), buf_i,
			&bytes_read, &index, len, offset);
	read_udi_subval(BT_DIS_UDI_DI_STR_REF, DIS_STR_SIZE(BT_DIS_UDI_DI_STR_REF), buf_i,
			&bytes_read, &index, len, offset);
	read_udi_subval(BT_DIS_UDI_ISSUER_STR_REF, DIS_STR_SIZE(BT_DIS_UDI_ISSUER_STR_REF), buf_i,
			&bytes_read, &index, len, offset);
	read_udi_subval(BT_DIS_UDI_AUTHORITY_STR_REF, DIS_STR_SIZE(BT_DIS_UDI_AUTHORITY_STR_REF),
			buf_i, &bytes_read, &index, len, offset);

	return bytes_read;
}
#endif

/* Device Information Service Declaration */
BT_GATT_SERVICE_DEFINE(
	dis_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

#if defined(CONFIG_BT_DIS_MODEL_NUMBER) || defined(CONFIG_BT_DIS_MODEL_DEPRECATED_USED)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_MODEL_REF),
#endif

#if defined(CONFIG_BT_DIS_MANUF_NAME) || defined(CONFIG_BT_DIS_MANUF_DEPRECATED_USED)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_MANUF_REF),
#endif

#if CONFIG_BT_DIS_PNP
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_pnp_id, NULL, &dis_pnp_id),
#endif

#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_SERIAL_NUMBER_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_FW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_HW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_SW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_UDI)
	BT_GATT_CHARACTERISTIC(BT_UUID_UDI_FOR_MEDICAL_DEVICES, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_udi, NULL, NULL),
#endif
#if defined(CONFIG_BT_DIS_SYSTEM_ID)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_system_id, NULL, NULL),
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_IEEE_RCDL, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_IEEE_RCDL_STR_REF),
#endif

);

#if defined(CONFIG_BT_DIS_SETTINGS)
#if defined(CONFIG_BT_DIS_UDI)
static void dis_update_udi_value(const char *new, char *old, settings_read_cb read_cb,
				 const char *logkey)
{
	/*
	 * The characteristic contains 1 one-byte flag and 4-null-terminated.
	 * The null-terminators are only present for strings that are in the flags.
	 */
	const size_t merged_size = sizeof(uint8_t) + DIS_STR_SIZE(BT_DIS_UDI_LABEL_STR_REF) +
				   DIS_STR_SIZE(BT_DIS_UDI_DI_STR_REF) +
				   DIS_STR_SIZE(BT_DIS_UDI_ISSUER_STR_REF) +
				   DIS_STR_SIZE(BT_DIS_UDI_AUTHORITY_STR_REF);

	size_t without_old = merged_size - DIS_STR_SIZE(old);

	bool valid = BT_ATT_MAX_ATTRIBUTE_LEN >= without_old + DIS_STR_SIZE(new);

	if (!valid) {
		LOG_ERR("Failed to set UDI %s. Not enough space. The sum of the 4 DIS UDI for "
			"Medical Devices strings may not exceed the maximum attribute length.",
			logkey);
		return;
	}

	int16_t len = read_cb((void *)new, (void *)old, CONFIG_BT_DIS_STR_MAX);

	if (len < 0) {
		LOG_ERR("Failed to read UDI %s from storage (err %zd)", logkey, len);
	} else {
		old[len] = '\0';

		LOG_DBG("UDI %s set to %s", logkey, old);
	}
}
#endif

static int dis_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *store)
{
	ssize_t len;
	int nlen;
	const char *next;

	ARG_UNUSED(len);

	nlen = settings_name_next(name, &next);
#if defined(CONFIG_BT_DIS_MANUF_NAME) || defined(CONFIG_BT_DIS_MANUF_DEPRECATED_USED)
	if (!strncmp(name, "manuf", nlen)) {
		len = read_cb(store, &dis_manuf, sizeof(dis_manuf) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read manufacturer from storage (err %zd)", len);
		} else {
			dis_manuf[len] = '\0';

			LOG_DBG("Manufacturer set to %s", dis_manuf);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_MODEL_NUMBER) || defined(CONFIG_BT_DIS_MODEL_DEPRECATED_USED)
	if (!strncmp(name, "model", nlen)) {
		len = read_cb(store, &dis_model, sizeof(dis_model) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read model from storage (err %zd)", len);
		} else {
			dis_model[len] = '\0';

			LOG_DBG("Model set to %s", dis_model);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
	if (!strncmp(name, "serial", nlen)) {
		len = read_cb(store, &dis_serial_number, sizeof(dis_serial_number) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read serial number from storage (err %zd)", len);
		} else {
			dis_serial_number[len] = '\0';

			LOG_DBG("Serial number set to %s", dis_serial_number);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
	if (!strncmp(name, "fw", nlen)) {
		len = read_cb(store, &dis_fw_rev, sizeof(dis_fw_rev) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read firmware revision from storage (err %zd)", len);
		} else {
			dis_fw_rev[len] = '\0';

			LOG_DBG("Firmware revision set to %s", dis_fw_rev);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
	if (!strncmp(name, "hw", nlen)) {
		len = read_cb(store, &dis_hw_rev, sizeof(dis_hw_rev) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read hardware revision from storage (err %zd)", len);
		} else {
			dis_hw_rev[len] = '\0';

			LOG_DBG("Hardware revision set to %s", dis_hw_rev);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
	if (!strncmp(name, "sw", nlen)) {
		len = read_cb(store, &dis_sw_rev, sizeof(dis_sw_rev) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read software revision from storage (err %zd)", len);
		} else {
			dis_sw_rev[len] = '\0';

			LOG_DBG("Software revision set to %s", dis_sw_rev);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_UDI)
	if (!strncmp(name, "udi_label", nlen)) {
		dis_update_udi_value(store, BT_DIS_UDI_LABEL_STR_REF, read_cb, "label");
		return 0;
	}

	if (!strncmp(name, "udi_di", nlen)) {
		dis_update_udi_value(store, BT_DIS_UDI_DI_STR_REF, read_cb, "device information");
		return 0;
	}

	if (!strncmp(name, "udi_issuer", nlen)) {
		dis_update_udi_value(store, BT_DIS_UDI_ISSUER_STR_REF, read_cb, "issuer");
		return 0;
	}

	if (!strncmp(name, "udi_authority", nlen)) {
		dis_update_udi_value(store, BT_DIS_UDI_AUTHORITY_STR_REF, read_cb, "authority");
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_SYSTEM_ID)
	if (!strncmp(name, "sysid_oui", nlen)) {
		uint32_t oui = 0;

		len = read_cb(store, &oui, sizeof(oui));
		if (len < 0) {
			LOG_ERR("Failed to read System ID OUI from storage (err %zd)", len);
		} else {
			sys_put_le24(oui, &dis_system_id[5]);
			LOG_DBG("System ID OUI set to %06X", oui);
		}
		return 0;
	}
	if (!strncmp(name, "sysid_identifier", nlen)) {
		uint64_t identifier = 0;

		len = read_cb(store, &identifier, sizeof(identifier));
		if (len < 0) {
			LOG_ERR("Failed to read System ID identifier from storage (err %zd)", len);
		} else {
			sys_put_le40(identifier, &dis_system_id[0]);
			LOG_DBG("System ID identifier set to %10llX", identifier);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
	if (!strncmp(name, "ieeercdl", nlen)) {
		len = read_cb(store, &dis_ieee_rcdl, sizeof(dis_ieee_rcdl) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read IEEE 11073-20601 Regulatory Certification Data "
				"List from storage (err %zd)",
				len);
		} else {
			dis_ieee_rcdl[len] = '\0';

			LOG_DBG("IEEE 11073-20601 Regulatory Certification Data List set to %s",
				dis_ieee_rcdl);
		}
		return 0;
	}
#endif
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_dis, "bt/dis", NULL, dis_set, NULL, NULL);

#endif /* CONFIG_BT_DIS_SETTINGS*/
