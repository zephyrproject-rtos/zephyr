/** @file
 *  @brief Health Server Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_SRV_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_SRV_H_

/**
 * @brief Health Server Model
 * @defgroup bt_mesh_health_srv Health Server Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Callback function for the Health Server model */
struct bt_mesh_health_srv_cb {
	/** @brief Callback for fetching current faults.
	 *
	 *  Fault values may either be defined by the specification, or by a
	 *  vendor. Vendor specific faults should be interpreted in the context
	 *  of the accompanying Company ID. Specification defined faults may be
	 *  reported for any Company ID, and the same fault may be presented
	 *  for multiple Company IDs.
	 *
	 *  All faults shall be associated with at least one Company ID,
	 *  representing the device vendor or some other vendor whose vendor
	 *  specific fault values are used.
	 *
	 *  If there are multiple Company IDs that have active faults,
	 *  return only the faults associated with one of them at the time.
	 *  To report faults for multiple Company IDs, interleave which Company
	 *  ID is reported for each call.
	 *
	 *  @param model       Health Server model instance to get faults of.
	 *  @param test_id     Test ID response buffer.
	 *  @param company_id  Company ID response buffer.
	 *  @param faults      Array to fill with current faults.
	 *  @param fault_count The number of faults the fault array can fit.
	 *                     Should be updated to reflect the number of faults
	 *                     copied into the array.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*fault_get_cur)(struct bt_mesh_model *model, uint8_t *test_id,
			     uint16_t *company_id, uint8_t *faults,
			     uint8_t *fault_count);

	/** @brief Callback for fetching all registered faults.
	 *
	 *  Registered faults are all past and current faults since the last
	 *  call to @c fault_clear. Only faults associated with the given
	 *  Company ID should be reported.
	 *
	 *  Fault values may either be defined by the specification, or by a
	 *  vendor. Vendor specific faults should be interpreted in the context
	 *  of the accompanying Company ID. Specification defined faults may be
	 *  reported for any Company ID, and the same fault may be presented
	 *  for multiple Company IDs.
	 *
	 *  @param model       Health Server model instance to get faults of.
	 *  @param company_id  Company ID to get faults for.
	 *  @param test_id     Test ID response buffer.
	 *  @param faults      Array to fill with registered faults.
	 *  @param fault_count The number of faults the fault array can fit.
	 *                     Should be updated to reflect the number of faults
	 *                     copied into the array.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*fault_get_reg)(struct bt_mesh_model *model, uint16_t company_id,
			     uint8_t *test_id, uint8_t *faults,
			     uint8_t *fault_count);

	/** @brief Clear all registered faults associated with the given Company
	 * ID.
	 *
	 *  @param model      Health Server model instance to clear faults of.
	 *  @param company_id Company ID to clear faults for.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*fault_clear)(struct bt_mesh_model *model, uint16_t company_id);

	/** @brief Run a self-test.
	 *
	 *  The Health server may support up to 256 self-tests for each Company
	 *  ID. The behavior for all test IDs are vendor specific, and should be
	 *  interpreted based on the accompanying Company ID. Test failures
	 *  should result in changes to the fault array.
	 *
	 *  @param model      Health Server model instance to run test for.
	 *  @param test_id    Test ID to run.
	 *  @param company_id Company ID to run test for.
	 *
	 *  @return 0 if the test execution was started successfully, or
	 * (negative) error code otherwise. Note that the fault array will not
	 * be reported back to the client if the test execution didn't start.
	 */
	int (*fault_test)(struct bt_mesh_model *model, uint8_t test_id,
			  uint16_t company_id);

	/** @brief Start calling attention to the device.
	 *
	 *  The attention state is used to map an element address to a
	 *  physical device. When this callback is called, the device should
	 *  start some physical procedure meant to call attention to itself,
	 *  like blinking, buzzing, vibrating or moving. If there are multiple
	 *  Health server instances on the device, the attention state should
	 *  also help identify the specific element the server is in.
	 *
	 *  The attention calling behavior should continue until the @c attn_off
	 *  callback is called.
	 *
	 *  @param model Health Server model to start the attention state of.
	 */
	void (*attn_on)(struct bt_mesh_model *model);

	/** @brief Stop the attention state.
	 *
	 *  Any physical activity started to call attention to the device should
	 *  be stopped.
	 *
	 *  @param model
	 */
	void (*attn_off)(struct bt_mesh_model *model);
};

/**
 *  A helper to define a health publication context
 *
 *  @param _name       Name given to the publication context variable.
 *  @param _max_faults Maximum number of faults the element can have.
 */
#define BT_MESH_HEALTH_PUB_DEFINE(_name, _max_faults) \
	BT_MESH_MODEL_PUB_DEFINE(_name, NULL, (1 + 3 + (_max_faults)))

/** Mesh Health Server Model Context */
struct bt_mesh_health_srv {
	/** Composition data model entry pointer. */
	struct bt_mesh_model *model;

	/** Optional callback struct */
	const struct bt_mesh_health_srv_cb *cb;

	/** Attention Timer state */
	struct k_work_delayable attn_timer;
};

/**
 *  Define a new health server model. Note that this API needs to be
 *  repeated for each element that the application wants to have a
 *  health server model on. Each instance also needs a unique
 *  bt_mesh_health_srv and bt_mesh_model_pub context.
 *
 *  @param srv Pointer to a unique struct bt_mesh_health_srv.
 *  @param pub Pointer to a unique struct bt_mesh_model_pub.
 *
 *  @return New mesh model instance.
 */
#define BT_MESH_MODEL_HEALTH_SRV(srv, pub)                                     \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_HEALTH_SRV, bt_mesh_health_srv_op,   \
			 pub, srv, &bt_mesh_health_srv_cb)

/** @brief Notify the stack that the fault array state of the given element has
 *  changed.
 *
 *  This prompts the Health server on this element to publish the current fault
 *  array if periodic publishing is disabled.
 *
 *  @param elem Element to update the fault state of.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
__deprecated int bt_mesh_fault_update(struct bt_mesh_elem *elem);

/** @brief Notify the stack that the fault array state of the given element has
 *  changed.
 *
 *  This prompts the Health server on this element to publish the current fault
 *  array if periodic publishing is disabled.
 *
 *  @param elem Element to update the fault state of.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_health_srv_fault_update(struct bt_mesh_elem *elem);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_health_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_health_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_SRV_H_ */
