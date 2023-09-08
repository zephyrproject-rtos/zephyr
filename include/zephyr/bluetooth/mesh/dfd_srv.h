/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_dfd_srv Firmware Distribution Server model
 * @ingroup bt_mesh_dfd
 * @{
 * @brief API for the Firmware Distribution Server model
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_DFD_SRV_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_DFD_SRV_H__

#include <zephyr/bluetooth/mesh/access.h>
#include <zephyr/bluetooth/mesh/dfd.h>
#include <zephyr/bluetooth/mesh/blob_srv.h>
#include <zephyr/bluetooth/mesh/blob_cli.h>
#include <zephyr/bluetooth/mesh/dfu_cli.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX
#define CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX 0
#endif

struct bt_mesh_dfd_srv;

/**
 *
 *  @brief Initialization parameters for the @ref bt_mesh_dfd_srv.
 */
#define BT_MESH_DFD_SRV_INIT(_cb)                                              \
	{                                                                      \
		.cb = _cb,                                                     \
		.dfu = BT_MESH_DFU_CLI_INIT(&_bt_mesh_dfd_srv_dfu_cb),         \
		.upload = {                                                    \
			.blob = { .cb = &_bt_mesh_dfd_srv_blob_cb },           \
		},                                                             \
	}

/**
 *
 *  @brief Firmware Distribution Server model Composition Data entry.
 *
 *  @param _srv Pointer to a @ref bt_mesh_dfd_srv instance.
 */
#define BT_MESH_MODEL_DFD_SRV(_srv)                                            \
	BT_MESH_MODEL_DFU_CLI(&(_srv)->dfu),                                   \
	BT_MESH_MODEL_BLOB_SRV(&(_srv)->upload.blob),                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_DFD_SRV, _bt_mesh_dfd_srv_op, NULL,  \
			 _srv, &_bt_mesh_dfd_srv_cb)

/** Firmware Distribution Server callbacks: */
struct bt_mesh_dfd_srv_cb {

	/** @brief Slot receive callback.
	 *
	 *  Called at the start of an upload procedure. The callback must fill
	 *  @c io with a pointer to a writable BLOB stream for the Firmware Distribution
	 *  Server to write the firmware image to.
	 *
	 *  @param srv  Firmware Distribution Server model instance.
	 *  @param slot DFU image slot being received.
	 *  @param io   BLOB stream response pointer.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*recv)(struct bt_mesh_dfd_srv *srv,
		    const struct bt_mesh_dfu_slot *slot,
		    const struct bt_mesh_blob_io **io);

	/** @brief Slot delete callback.
	 *
	 *  Called when the Firmware Distribution Server is about to delete a DFU image slot.
	 *  All allocated data associated with the firmware slot should be
	 *  deleted.
	 *
	 *  @param srv  Firmware Update Server instance.
	 *  @param slot DFU image slot being deleted.
	 */
	void (*del)(struct bt_mesh_dfd_srv *srv,
		    const struct bt_mesh_dfu_slot *slot);

	/** @brief Slot send callback.
	 *
	 *  Called at the start of a distribution procedure. The callback must
	 *  fill @c io with a pointer to a readable BLOB stream for the Firmware
	 *  Distribution Server to read the firmware image from.
	 *
	 *  @param srv  Firmware Distribution Server model instance.
	 *  @param slot DFU image slot being sent.
	 *  @param io   BLOB stream response pointer.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*send)(struct bt_mesh_dfd_srv *srv,
		    const struct bt_mesh_dfu_slot *slot,
		    const struct bt_mesh_blob_io **io);

	/** @brief Phase change callback (Optional).
	 *
	 *  Called whenever the phase of the Firmware Distribution Server changes.
	 *
	 *  @param srv  Firmware Distribution Server model instance.
	 *  @param phase  New Firmware Distribution phase.
	 */
	void (*phase)(struct bt_mesh_dfd_srv *srv, enum bt_mesh_dfd_phase phase);
};

/** Firmware Distribution Server instance. */
struct bt_mesh_dfd_srv {
	const struct bt_mesh_dfd_srv_cb *cb;
	const struct bt_mesh_model *mod;
	struct bt_mesh_dfu_cli dfu;
	struct bt_mesh_dfu_target targets[CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX];
	struct bt_mesh_blob_target_pull pull_ctxs[CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX];
	const struct bt_mesh_blob_io *io;
	uint16_t target_cnt;
	uint16_t slot_idx;
	bool apply;
	enum bt_mesh_dfd_phase phase;
	struct bt_mesh_blob_cli_inputs inputs;

	struct {
		enum bt_mesh_dfd_upload_phase phase;
		const struct bt_mesh_dfu_slot *slot;
		const struct flash_area *area;
		struct bt_mesh_blob_srv blob;
	} upload;
};

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_dfd_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_dfd_srv_cb;
extern const struct bt_mesh_dfu_cli_cb _bt_mesh_dfd_srv_dfu_cb;
extern const struct bt_mesh_blob_srv_cb _bt_mesh_dfd_srv_blob_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_DFD_SRV_H__ */

/** @} */
