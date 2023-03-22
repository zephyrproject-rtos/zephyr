/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_large_comp_data_cli Large Composition Data Client model
 * @{
 * @brief API for the Large Composition Data Client model.
 */
#ifndef BT_MESH_LARGE_COMP_DATA_CLI_H__
#define BT_MESH_LARGE_COMP_DATA_CLI_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_large_comp_data_cli;

/** Large Composition Data response. */
struct bt_mesh_large_comp_data_rsp {
	/** Page number. */
	uint8_t page;
	/** Offset within the page. */
	uint16_t offset;
	/** Total size of the page. */
	uint16_t total_size;
	/** Pointer to allocated buffer for storing received data. */
	struct net_buf_simple *data;
};

/** Large Composition Data Status messages callbacks */
struct bt_mesh_large_comp_data_cli_cb {
	/** @brief Optional callback for Large Composition Data Status message.
	 *
	 *  Handles received Large Composition Data Status messages from a Large Composition Data
	 *  Server.
	 *
	 *  If the content of @c rsp is needed after exiting this callback, a user should
	 *  deep copy it.
	 *
	 *  @param cli         Large Composition Data Client context.
	 *  @param addr        Address of the sender.
	 *  @param rsp         Response received from the server.
	 */
	void (*large_comp_data_status)(struct bt_mesh_large_comp_data_cli *cli, uint16_t addr,
				       struct bt_mesh_large_comp_data_rsp *rsp);

	/** @brief Optional callback for Models Metadata Status message.
	 *
	 *  Handles received Models Metadata Status messages from a Large Composition Data
	 *  Server.
	 *
	 *  If the content of @c rsp is needed after exiting this callback, a user should
	 *  deep copy it.
	 *
	 *  @param cli         Large Composition Data Client context.
	 *  @param addr        Address of the sender.
	 *  @param rsp         Response received from the server.
	 */
	void (*models_metadata_status)(struct bt_mesh_large_comp_data_cli *cli, uint16_t addr,
				       struct bt_mesh_large_comp_data_rsp *rsp);
};

/** Large Composition Data Client model context */
struct bt_mesh_large_comp_data_cli {
	/** Model entry pointer. */
	struct bt_mesh_model *model;

	/** Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;

	/** Optional callback for Large Composition Data Status messages. */
	const struct bt_mesh_large_comp_data_cli_cb *cb;
};

/**
 *
 *  @brief Large Composition Data Client model Composition Data entry.
 *
 *  @param cli_data Pointer to a @ref bt_mesh_large_comp_data_cli instance.
 */
#define BT_MESH_MODEL_LARGE_COMP_DATA_CLI(cli_data)                                                \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LARGE_COMP_DATA_CLI,                                     \
			 _bt_mesh_large_comp_data_cli_op, NULL, cli_data,                          \
			 &_bt_mesh_large_comp_data_cli_cb)

/** @brief Send Large Composition Data Get message.
 *
 * This API is used to read a portion of a Composition Data Page.
 *
 * This API can be used asynchronously by setting @p rsp as NULL. This way, the
 * method will not wait for a response and will return immediately after sending
 * the command.
 *
 * When @c rsp is set, the user is responsible for providing a buffer for the
 * Composition Data in @ref bt_mesh_large_comp_data_rsp::data. If a buffer is
 * not provided, the metadata won't be copied.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node element address.
 *  @param page        Composition Data Page to read.
 *  @param offset      Offset within the Composition Data Page.
 *  @param rsp         Pointer to a struct storing the received response from
 *                     the server, or NULL to not wait for a response.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_large_comp_data_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct bt_mesh_large_comp_data_rsp *rsp);

/** @brief Send Models Metadata Get message.
 *
 * This API is used to read a portion of a Models Metadata Page.
 *
 * This API can be used asynchronously by setting @p rsp as NULL. This way, the
 * method will not wait for a response and will return immediately after sending
 * the command.
 *
 * When @c rsp is set, a user is responsible for providing a buffer for
 * metadata in @ref bt_mesh_large_comp_data_rsp::data. If a buffer is not
 * provided, the metadata won't be copied.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node element address.
 *  @param page        Models Metadata Page to read.
 *  @param offset      Offset within the Models Metadata Page.
 *  @param rsp         Pointer to a struct storing the received response from
 *                     the server, or NULL to not wait for a response.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_models_metadata_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct bt_mesh_large_comp_data_rsp *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_large_comp_data_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_large_comp_data_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* BT_MESH_LARGE_COMP_DATA_CLI_H__ */
