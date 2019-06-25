/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service E
 *
 *  This code is auto-generated from the Excel Workbook 'GATT_Test_Databases.xlsm' Sheet: 'Large Database 3'
 */

#include <misc/byteorder.h>
#include <misc/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_E
 *  @brief UUID for the Service E
 */
#define BT_UUID_SERVICE_E               BT_UUID_DECLARE_16(0xa00e)


#define BT_GATT_CHRC_NONE 0

static struct bt_gatt_attr service_e_3_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SERVICE_E, 0xFFFF)
};

static struct bt_gatt_service service_e_3_svc = BT_GATT_SERVICE(service_e_3_attrs);

/**
 * @brief Register the Service E and all its Characteristics...
 */
void service_e_3_init(void)
{
    bt_gatt_service_register(&service_e_3_svc);
}

/**
 * @brief Un-Register the Service E and all its Characteristics...
 */
void service_e_3_remove(void)
{
    bt_gatt_service_unregister(&service_e_3_svc);
}
