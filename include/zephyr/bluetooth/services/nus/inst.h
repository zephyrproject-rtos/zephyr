/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_INST_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_INST_H_

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_nus_inst {
	/** Pointer to the NUS Service Instance  */
	const struct bt_gatt_service_static *svc;

	/** List of subscribers to invoke callbacks on asynchronous events */
	sys_slist_t *cbs;
};

#define BT_UUID_NUS_SERVICE BT_UUID_DECLARE_128(BT_UUID_NUS_SRV_VAL)
#define BT_UUID_NUS_TX_CHAR BT_UUID_DECLARE_128(BT_UUID_NUS_TX_CHAR_VAL)
#define BT_UUID_NUS_RX_CHAR BT_UUID_DECLARE_128(BT_UUID_NUS_RX_CHAR_VAL)

/** Required as the service may be instantiated outside of the module */
ssize_t nus_bt_chr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

#define Z_INTERNAL_BT_NUS_INST_DEFINE(_name)							   \
												   \
BT_GATT_SERVICE_DEFINE(_name##_svc,								   \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),						   \
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX_CHAR,						   \
		BT_GATT_CHRC_NOTIFY,								   \
		BT_GATT_PERM_NONE,								   \
		NULL, NULL, NULL),								   \
	BT_GATT_CCC(nus_ccc_cfg_changed,							   \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),					   \
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX_CHAR,						   \
		BT_GATT_CHRC_WRITE |								   \
		BT_GATT_CHRC_WRITE_WITHOUT_RESP,						   \
		BT_GATT_PERM_WRITE,								   \
		NULL, nus_bt_chr_write, NULL),							   \
);												   \
												   \
sys_slist_t _name##_cbs = SYS_SLIST_STATIC_INIT(&_name##_cbs);					   \
												   \
STRUCT_SECTION_ITERABLE(bt_nus_inst, _name) = {							   \
	.svc = &_name##_svc,									   \
	.cbs = &_name##_cbs,									   \
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_INST_H_ */
