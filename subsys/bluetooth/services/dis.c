/** @file
 *  @brief GATT Device Infromation Service
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#if CONFIG_BT_GATT_DIS_PNP
struct dis_pnp {
	u8_t pnp_vid_src;
	u16_t pnp_vid;
	u16_t pnp_pid;
	u16_t pnp_ver;
} __packed;

static struct dis_pnp dis_pnp_id = {
	.pnp_vid_src = CONFIG_BT_GATT_DIS_PNP_VID_SRC,
	.pnp_vid = CONFIG_BT_GATT_DIS_PNP_VID,
	.pnp_pid = CONFIG_BT_GATT_DIS_PNP_PID,
	.pnp_ver = CONFIG_BT_GATT_DIS_PNP_VER,
};
#endif

static ssize_t read_str(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 strlen(attr->user_data));
}

#if CONFIG_BT_GATT_DIS_PNP
static ssize_t read_pnp_id(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dis_pnp_id,
				 sizeof(dis_pnp_id));
}
#endif

/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, CONFIG_BT_GATT_DIS_MODEL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_str, NULL, CONFIG_BT_GATT_DIS_MANUF),
#if CONFIG_BT_GATT_DIS_PNP
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_pnp_id, NULL, &dis_pnp_id),
#endif
};

static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);

static int dis_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return bt_gatt_service_register(&dis_svc);
}

SYS_INIT(dis_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
