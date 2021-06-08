/** @file
 *  @brief Proxy APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_

/**
 * @brief Proxy
 * @defgroup bt_mesh_proxy Proxy
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Callbacks for the Proxy feature.
 *
 *  Should be instantiated with @ref BT_MESH_PROXY_CB_DEFINE.
 */
struct bt_mesh_proxy_cb {
	/** @brief Started sending Node Identity beacons on the given subnet.
	 *
	 *  @param net_idx Network index the Node Identity beacons are running
	 *                 on.
	 */
	void (*identity_enabled)(uint16_t net_idx);
	/** @brief Stopped sending Node Identity beacons on the given subnet.
	 *
	 *  @param net_idx Network index the Node Identity beacons were running
	 *                 on.
	 */
	void (*identity_disabled)(uint16_t net_idx);
};

/** @def BT_MESH_PROXY_CB_DEFINE
 *
 *  @brief Register a callback structure for Proxy events.
 *
 *  Registers a structure with callback functions that gets called on various
 *  Proxy events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_MESH_PROXY_CB_DEFINE(_name)                                         \
	static const Z_STRUCT_SECTION_ITERABLE(                                \
		bt_mesh_proxy_cb, _CONCAT(bt_mesh_proxy_cb, _name))

/** @brief Enable advertising with Node Identity.
 *
 *  This API requires that GATT Proxy support has been enabled. Once called
 *  each subnet will start advertising using Node Identity for the next
 *  60 seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_identity_enable(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_ */
