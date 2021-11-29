/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2021 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/audio/hap.h>
#include <bluetooth/audio/has.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_hap {
    uint8_t features;
};

/** @brief Hearing Access Profile server internal representation */
struct bt_hap_server {
    /** Common profile reference object */
    struct bt_hap hap;

    /** Registered application callbacks */
    struct bt_has_cb *cb;
};

/** @def BT_HAP_SERVER(_hap)
 *  @brief Helper macro getting container object of type bt_hap_server
 *  address having the same container hap member address as object in question.
 *
 *  @param _hap Address of object of bt_hap type
 *
 *  @return Address of in memory bt_hap_server object type containing
 *          the address of in question object.
 */
#define BT_HAP_SERVER(_hap) CONTAINER_OF(_hap, struct bt_hap_server, hap)

/** @brief Hearing Access Profile client internal representation */
struct bt_hap_client {
    /** Common profile reference object */
    struct bt_hap hap;

    /** Profile connection reference */
	struct bt_conn *conn;
};

/** @def BT_HAP_CLIENT(_hap)
 *  @brief Helper macro getting container object of type bt_hap_client
 *  address having the same container hap member address as object in question.
 *
 *  @param _hap Address of object of bt_hap type
 *
 *  @return Address of in memory bt_hap_client object type containing
 *          the address of in question object.
 */
#define BT_HAP_CLIENT(_hap) CONTAINER_OF(_hap, struct bt_hap_client, hap)

#ifdef __cplusplus
}
#endif
