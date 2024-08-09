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
static uint8_t dis_system_id[8] = { (CONFIG_BT_DIS_SYSTEM_ID_OUI >> 16) & 0xFF, 
									(CONFIG_BT_DIS_SYSTEM_ID_OUI >> 8) & 0xFF, 
									(CONFIG_BT_DIS_SYSTEM_ID_OUI >> 0) & 0xFF,
									((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER >> 32) & 0xFF,
									((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER >> 24) & 0xFF,
									((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER >> 16) & 0xFF,
									((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER >> 8) & 0xFF,
									((uint64_t)CONFIG_BT_DIS_SYSTEM_ID_IDENTIFIER >> 0) & 0xFF };
#endif

#if defined(CONFIG_BT_DIS_SETTINGS)
static uint8_t dis_model[CONFIG_BT_DIS_STR_MAX] = CONFIG_BT_DIS_MODEL;
static uint8_t dis_manuf[CONFIG_BT_DIS_STR_MAX] = CONFIG_BT_DIS_MANUF;
#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
static uint8_t dis_serial_number[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_SERIAL_NUMBER_STR;
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
static uint8_t dis_fw_rev[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_FW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
static uint8_t dis_hw_rev[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_HW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
static uint8_t dis_sw_rev[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_SW_REV_STR;
#endif
#if defined(CONFIG_BT_DIS_UDI)
static uint8_t dis_udi_label[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_UDI_LABEL_STR;
static uint8_t dis_udi_di[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_UDI_DI_STR;
static uint8_t dis_udi_issuer[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_UDI_ISSUER_STR;
static uint8_t dis_udi_authority[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_UDI_AUTHORITY_STR;
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
static uint8_t dis_ieee_rcdl[CONFIG_BT_DIS_STR_MAX] =
	CONFIG_BT_DIS_IEEE_RCDL_STR;
#endif

#define BT_DIS_MODEL_REF		dis_model
#define BT_DIS_MANUF_REF		dis_manuf
#define BT_DIS_SERIAL_NUMBER_STR_REF	dis_serial_number
#define BT_DIS_FW_REV_STR_REF		dis_fw_rev
#define BT_DIS_HW_REV_STR_REF		dis_hw_rev
#define BT_DIS_SW_REV_STR_REF		dis_sw_rev
#define BT_DIS_UDI_LABEL_STR_REF	dis_udi_label
#define BT_DIS_UDI_DI_STR_REF		dis_udi_di
#define BT_DIS_UDI_ISSUER_STR_REF	dis_udi_issuer
#define BT_DIS_UDI_AUTHORITY_STR_REF	dis_udi_authority
#define BT_DIS_IEEE_RCDL_STR_REF	dis_ieee_rcdl

#else /* CONFIG_BT_DIS_SETTINGS */

#define BT_DIS_MODEL_REF		CONFIG_BT_DIS_MODEL
#define BT_DIS_MANUF_REF		CONFIG_BT_DIS_MANUF
#define BT_DIS_SERIAL_NUMBER_STR_REF	CONFIG_BT_DIS_SERIAL_NUMBER_STR
#define BT_DIS_FW_REV_STR_REF		CONFIG_BT_DIS_FW_REV_STR
#define BT_DIS_HW_REV_STR_REF		CONFIG_BT_DIS_HW_REV_STR
#define BT_DIS_SW_REV_STR_REF		CONFIG_BT_DIS_SW_REV_STR
#define BT_DIS_UDI_LABEL_STR_REF	CONFIG_BT_DIS_UDI_LABEL_STR
#define BT_DIS_UDI_DI_STR_REF		CONFIG_BT_DIS_UDI_DI_STR
#define BT_DIS_UDI_ISSUER_STR_REF	CONFIG_BT_DIS_UDI_ISSUER_STR
#define BT_DIS_UDI_AUTHORITY_STR_REF	CONFIG_BT_DIS_UDI_AUTHORITY_STR
#define BT_DIS_IEEE_RCDL_STR_REF	CONFIG_BT_DIS_IEEE_RCDL_STR

#endif /* CONFIG_BT_DIS_SETTINGS */

static ssize_t read_str(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 strlen(attr->user_data));
}

#if CONFIG_BT_DIS_PNP
static ssize_t read_pnp_id(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dis_pnp_id,
				 sizeof(dis_pnp_id));
}
#endif

#if CONFIG_BT_DIS_SYSTEM_ID
static ssize_t read_system_id(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_system_id,
				 sizeof(dis_system_id));
}
#endif

#if CONFIG_BT_DIS_UDI
#define BT_DIS_UDI_FLAG_LABEL		(!!BT_DIS_UDI_LABEL_STR_REF[0])
#define BT_DIS_UDI_FLAG_DI			(!!BT_DIS_UDI_DI_STR_REF[0])
#define BT_DIS_UDI_FLAG_ISSUER		(!!BT_DIS_UDI_ISSUER_STR_REF[0])
#define BT_DIS_UDI_FLAG_AUTHORITY	(!!BT_DIS_UDI_AUTHORITY_STR_REF[0])
#define BT_DIS_UDI_FLAGS			(BT_DIS_UDI_FLAG_LABEL | (BT_DIS_UDI_FLAG_DI << 1) | (BT_DIS_UDI_FLAG_ISSUER << 2) | (BT_DIS_UDI_FLAG_AUTHORITY << 3))

#if defined(CONFIG_BT_SETTINGS)
static uint8_t bt_dis_udi_merged[MIN(CONFIG_BT_DIS_STR_MAX * 4 + 1, MIN(BT_ATT_MAX_ATTRIBUTE_LEN, CONFIG_BT_L2CAP_TX_MTU))];
#else
static uint8_t bt_dis_udi_merged[MIN(BT_ATT_MAX_ATTRIBUTE_LEN, CONFIG_BT_L2CAP_TX_MTU)];
#endif

static size_t bt_dis_udi_merged_size = 0;

static ssize_t read_udi(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	if (!bt_dis_udi_merged_size) 
	{
		bt_dis_udi_merged[bt_dis_udi_merged_size++] = BT_DIS_UDI_FLAGS;
		
		if (BT_DIS_UDI_FLAG_LABEL) 
		{
			size_t l = MIN(strlen(BT_DIS_UDI_LABEL_STR_REF) + 1, sizeof(bt_dis_udi_merged) - bt_dis_udi_merged_size);
			memcpy(bt_dis_udi_merged + bt_dis_udi_merged_size, BT_DIS_UDI_LABEL_STR_REF, l);
			bt_dis_udi_merged_size += l;
		}

		if (BT_DIS_UDI_FLAG_DI) 
		{
			size_t l = MIN(strlen(BT_DIS_UDI_DI_STR_REF) + 1, sizeof(bt_dis_udi_merged) - bt_dis_udi_merged_size);
			memcpy(bt_dis_udi_merged + bt_dis_udi_merged_size, BT_DIS_UDI_DI_STR_REF, l);
			bt_dis_udi_merged_size += l;
		}

		if (BT_DIS_UDI_FLAG_ISSUER) 
		{
			size_t l = MIN(strlen(BT_DIS_UDI_AUTHORITY_STR_REF) + 1, sizeof(bt_dis_udi_merged) - bt_dis_udi_merged_size);
			memcpy(bt_dis_udi_merged + bt_dis_udi_merged_size, BT_DIS_UDI_ISSUER_STR_REF, l);
			bt_dis_udi_merged_size += l;
		}

		if (BT_DIS_UDI_FLAG_AUTHORITY) 
		{
			size_t l = MIN(strlen(BT_DIS_UDI_AUTHORITY_STR_REF) + 1, sizeof(bt_dis_udi_merged) - bt_dis_udi_merged_size);
			memcpy(bt_dis_udi_merged + bt_dis_udi_merged_size, BT_DIS_UDI_AUTHORITY_STR_REF, l);
			bt_dis_udi_merged_size += l;
		}
	}
	

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &bt_dis_udi_merged,
				 bt_dis_udi_merged_size);
}
#endif

/* Device Information Service Declaration */
BT_GATT_SERVICE_DEFINE(dis_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_MODEL_REF),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_MANUF_REF),
#if CONFIG_BT_DIS_PNP
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_pnp_id, NULL, &dis_pnp_id),
#endif

#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL,
			       BT_DIS_SERIAL_NUMBER_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_FW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_HW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_SW_REV_STR_REF),
#endif
#if defined(CONFIG_BT_DIS_UDI)
	BT_GATT_CHARACTERISTIC(BT_UUID_UDI_FOR_MEDICAL_DEVICES,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_udi, NULL, NULL),
#endif
#if defined(CONFIG_BT_DIS_SYSTEM_ID)
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_system_id, NULL, NULL),
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_IEEE_RCDL,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, BT_DIS_IEEE_RCDL_STR_REF),
#endif

);

#if defined(CONFIG_BT_DIS_SETTINGS)
static int dis_set(const char *name, size_t len_rd,
		   settings_read_cb read_cb, void *store)
{
	ssize_t len;
	int nlen;
	const char *next;

	nlen = settings_name_next(name, &next);
	if (!strncmp(name, "manuf", nlen)) {
		len = read_cb(store, &dis_manuf, sizeof(dis_manuf) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read manufacturer from storage"
				       " (err %zd)", len);
		} else {
			dis_manuf[len] = '\0';

			LOG_DBG("Manufacturer set to %s", dis_manuf);
		}
		return 0;
	}
	if (!strncmp(name, "model", nlen)) {
		len = read_cb(store, &dis_model, sizeof(dis_model) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read model from storage"
				       " (err %zd)", len);
		} else {
			dis_model[len] = '\0';

			LOG_DBG("Model set to %s", dis_model);
		}
		return 0;
	}
#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
	if (!strncmp(name, "serial", nlen)) {
		len = read_cb(store, &dis_serial_number,
			   sizeof(dis_serial_number) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read serial number from storage"
				       " (err %zd)", len);
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
			LOG_ERR("Failed to read firmware revision from storage"
				       " (err %zd)", len);
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
			LOG_ERR("Failed to read hardware revision from storage"
				       " (err %zd)", len);
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
			LOG_ERR("Failed to read software revision from storage"
				       " (err %zd)", len);
		} else {
			dis_sw_rev[len] = '\0';

			LOG_DBG("Software revision set to %s", dis_sw_rev);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_UDI)
	const size_t currently_used_udi_merged_space = 1 + 
		strlen(BT_DIS_UDI_LABEL_STR_REF) + 
		strlen(BT_DIS_UDI_DI_STR_REF) + 
		strlen(BT_DIS_UDI_ISSUER_STR_REF) + 
		strlen(BT_DIS_UDI_AUTHORITY_STR_REF) + 
		BT_DIS_UDI_FLAG_LABEL + 
		BT_DIS_UDI_FLAG_DI + 
		BT_DIS_UDI_FLAG_ISSUER + 
		BT_DIS_UDI_FLAG_AUTHORITY;

	if (!strncmp(name, "udi_label", nlen)) {
		size_t in_use_without_this = currently_used_udi_merged_space - strlen(BT_DIS_UDI_LABEL_STR_REF) - BT_DIS_UDI_FLAG_LABEL;
		if (sizeof(bt_dis_udi_merged) < in_use_without_this + strlen(store) + 1) 
		{
			LOG_ERR("Failed to set UDI Label. Not enough space. The sum of the 4 DIS UDI for Medical Devices strings may not exceed the maximum attribute length or maximum transmit MTU.");
			return 0;
		}

		len = read_cb(store, &dis_udi_label, sizeof(dis_udi_label) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read UDI Label from storage"
				       " (err %zd)", len);
		} else {
			dis_udi_label[len] = '\0';
			bt_dis_udi_merged_size = 0;

			LOG_DBG("UDI Label set to %s", dis_udi_label);
		}
		return 0;
	}

	if (!strncmp(name, "udi_di", nlen)) {
		size_t in_use_without_this = currently_used_udi_merged_space - strlen(BT_DIS_UDI_DI_STR_REF) - BT_DIS_UDI_FLAG_DI;
		if (sizeof(bt_dis_udi_merged) < in_use_without_this + strlen(store) + 1) 
		{
			LOG_ERR("Failed to set UDI Device Identifier. Not enough space. The sum of the 4 DIS UDI for Medical Devices strings may not exceed the maximum attribute length or maximum transmit MTU.");
			return 0;
		} 

		len = read_cb(store, &dis_udi_di, sizeof(dis_udi_di) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read UDI Device Identifier from storage"
				       " (err %zd)", len);
		} else {
			dis_udi_di[len] = '\0';
			bt_dis_udi_merged_size = 0;

			LOG_DBG("UDI Device Identifier set to %s", dis_udi_di);
		}
		return 0;
	}

	if (!strncmp(name, "udi_issuer", nlen)) {
		size_t in_use_without_this = currently_used_udi_merged_space - strlen(BT_DIS_UDI_ISSUER_STR_REF) - BT_DIS_UDI_FLAG_ISSUER;
		if (sizeof(bt_dis_udi_merged) < in_use_without_this + strlen(store) + 1) 
		{
			LOG_ERR("Failed to set UDI Issuer. Not enough space. The sum of the 4 DIS UDI for Medical Devices strings may not exceed the maximum attribute length or maximum transmit MTU.");
			return 0;
		} 

		len = read_cb(store, &dis_udi_issuer, sizeof(dis_udi_issuer) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read UDI Issuer from storage"
				       " (err %zd)", len);
		} else {
			dis_udi_issuer[len] = '\0';
			bt_dis_udi_merged_size = 0;

			LOG_DBG("UDI Issuer set to %s", dis_udi_issuer);
		}
		return 0;
	}

	if (!strncmp(name, "udi_authority", nlen)) {
		size_t in_use_without_this = currently_used_udi_merged_space - strlen(BT_DIS_UDI_AUTHORITY_STR_REF) - BT_DIS_UDI_FLAG_AUTHORITY;
		if (sizeof(bt_dis_udi_merged) < in_use_without_this + strlen(store) + 1) 
		{
			LOG_ERR("Failed to set UDI Authority. Not enough space. The sum of the 4 DIS UDI for Medical Devices strings may not exceed the maximum attribute length or maximum transmit MTU.");
			return 0;
		} 

		len = read_cb(store, &dis_udi_authority, sizeof(dis_udi_authority) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read UDI Authority from storage"
				       " (err %zd)", len);
		} else {
			dis_udi_authority[len] = '\0';
			bt_dis_udi_merged_size = 0;

			LOG_DBG("UDI Authority set to %s", dis_udi_authority);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_SYSTEM_ID)
	if (!strncmp(name, "sysid_oui", nlen)) {
		uint32_t oui = 0;
		len = read_cb(store, &oui, sizeof(oui));
		if (len < 0) {
			LOG_ERR("Failed to read System ID OUI from storage"
				       " (err %zd)", len);
		} else {
			dis_system_id[0] = (oui >> 16) & 0xFF;
			dis_system_id[1] = (oui >> 8) & 0xFF;
			dis_system_id[2] = (oui >> 0) & 0xFF;
			LOG_DBG("System ID OUI set to %06X", oui);
		}
		return 0;
	}
	if (!strncmp(name, "sysid_identifier", nlen)) {
		uint64_t identifier = 0;
		len = read_cb(store, &identifier, sizeof(identifier));
		if (len < 0) {
			LOG_ERR("Failed to read System ID identifier from storage"
				       " (err %zd)", len);
		} else {
			dis_system_id[3] = (identifier >> 32) & 0xFF;
			dis_system_id[4] = (identifier >> 24) & 0xFF;
			dis_system_id[5] = (identifier >> 16) & 0xFF;
			dis_system_id[6] = (identifier >> 8) & 0xFF;
			dis_system_id[7] = (identifier >> 0) & 0xFF;
			LOG_DBG("System ID identifier set to %10llX", identifier);
		}
		return 0;
	}
#endif
#if defined(CONFIG_BT_DIS_IEEE_RCDL)
	if (!strncmp(name, "ieeercdl", nlen)) {
		len = read_cb(store, &dis_ieee_rcdl, sizeof(dis_ieee_rcdl) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read IEEE 11073-20601 Regulatory Certification Data List from storage"
				       " (err %zd)", len);
		} else {
			dis_ieee_rcdl[len] = '\0';

			LOG_DBG("IEEE 11073-20601 Regulatory Certification Data List set to %s", dis_ieee_rcdl);
		}
		return 0;
	}
#endif
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_dis, "bt/dis", NULL, dis_set, NULL, NULL);

#endif /* CONFIG_BT_DIS_SETTINGS*/
