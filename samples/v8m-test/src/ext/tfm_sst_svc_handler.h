/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_SST_SVC_HANDLER_H__
#define __TFM_SST_SVC_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tfm_sst_defs.h"

/**
 * \brief SVC funtion to get handler for the given asset UUID. If an asset is
 *        deleted, the linked asset handle reference is no longer valid and
 *        will give TFM_SST_ERR_ASSET_REF_INVALID if used.
 *
 * \param[in]  asset_uuid  Asset UUID
 * \param[out] hdl         Pointer to store the asset's handler
 *
 * \return Returns TFM_SST_ERR_SUCCESS if asset is found. Otherwise, error code
 *         as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_get_handle(uint16_t asset_uuid,
                                          uint32_t* hdl);

/**
 * \brief SVC funtion to allocate space for the asset, referenced by asset
 *        handler, without setting any data in the asset.
 *
 * \param[in] asset_uuid  Asset UUID
 *
 * \return Returns an TFM_SST_ERR_SUCCESS if asset is created correctly.
 *         Otherwise, error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_create(uint16_t asset_uuid);

/**
 * \brief SVC funtion to get asset's attributes referenced by asset handler.
 *        Uses cached metadata to return the size and write offset of an asset.
 *
 * \param[in]  asset_handle   Asset handler
 * \param[out] attrib_struct  Pointer to store asset's attribute
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_get_attributes(uint32_t asset_handle,
                                       struct tfm_sst_attribs_t* attrib_struct);

/**
 * \brief SVC funtion to read asset's data from asset referenced by asset
 *        handler.
 *
 * \param[in]  asset_handle  Asset handler
 * \param[out] data          Pointer to data vector \ref tfm_sst_buf_t to store
 *                           data, size and offset
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_read(uint32_t asset_handle,
                                    struct tfm_sst_buf_t* data);

/**
 * \brief SVC funtion to write data into an asset referenced by asset handler.
 *
 * \param[in] asset_handle   Asset handler
 * \param[in] data           Pointer to data vector \ref tfm_sst_buf_t which
 *                           contains the data to write
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_write(uint32_t asset_handle,
                                     struct tfm_sst_buf_t* data);

/**
 * \brief SVC funtion to delete the asset referenced by the asset handler.
 *
 * \param[in] asset_handle  Asset handler
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t tfm_sst_svc_delete(uint32_t asset_handle);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_SST_SVC_HANDLER_H__ */
