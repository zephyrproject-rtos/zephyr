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

/**
 *
 *  @brief Large Composition Data Client model composition data entry.
 */
#define BT_MESH_MODEL_LARGE_COMP_DATA_CLI                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LARGE_COMP_DATA_CLI,                 \
			 _bt_mesh_large_comp_data_cli_op, NULL, NULL,          \
			 &_bt_mesh_large_comp_data_cli_cb)

/** @brief Send Large Composition Data Get message.
 *
 * This API is used to read a portion of a page of the Composition Data.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node element address.
 *  @param page        Composition Data page to read.
 *  @param offset      Offset within the Composition Data Page.
 *  @param comp        Output buffer for storing received Composition data.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_large_comp_data_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct net_buf_simple *comp);

/** @brief Send Models Metadata Get message.
 *
 * This API is used to read a portion of a page of the Models Metadata state.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node element address.
 *  @param page        Models Metadata page to read.
 *  @param offset      Offset within the Models Metadata Page.
 *  @param metadata    Output buffer for storing received Models Metadata.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_models_metadata_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct net_buf_simple *metadata);

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
