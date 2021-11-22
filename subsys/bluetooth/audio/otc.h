/* @file
 * @brief Object Transfer client header
 *
 * For use with the Object Transfer Service client (OTC)
 *
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_H_

/* TODO: Temporarily here - clean up, and move alongside the Object Transfer Service */

#include <stdbool.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include "ots.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_OTC_MAX_WRITE_SIZE               23
#define BT_OTC_UNKNOWN_ID                   0xFF

#define BT_OTC_METADATA_REQ_NAME          BIT(0)
#define BT_OTC_METADATA_REQ_TYPE          BIT(1)
#define BT_OTC_METADATA_REQ_SIZE          BIT(2)
#define BT_OTC_METADATA_REQ_CREATED       BIT(3)
#define BT_OTC_METADATA_REQ_MODIFIED      BIT(4)
#define BT_OTC_METADATA_REQ_ID            BIT(5)
#define BT_OTC_METADATA_REQ_PROPS         BIT(6)
#define BT_OTC_METADATA_REQ_ALL           0x7F

#define BT_OTC_STOP                       0
#define BT_OTC_CONTINUE                   1

#define BT_OTC_SET_METADATA_REQ_NAME(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_NAME, set)
#define BT_OTC_SET_METADATA_REQ_TYPE(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_TYPE, set)
#define BT_OTC_SET_METADATA_REQ_SIZE(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_SIZE, set)
#define BT_OTC_SET_METADATA_REQ_CREATED(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_CREATED, set)
#define BT_OTC_SET_METADATA_REQ_MODIFIED(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_MODIFIED, set)
#define BT_OTC_SET_METADATA_REQ_ID(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_ID, set)
#define BT_OTC_SET_METADATA_REQ_PROPS(metadata, set) \
	SET_OR_CLEAR_BIT(metadata, BT_OTC_METADATA_REQ_PROPS, set)

#define BT_OTC_GET_METADATA_REQ_NAME(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_NAME)
#define BT_OTC_GET_METADATA_REQ_TYPE(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_TYPE)
#define BT_OTC_GET_METADATA_REQ_SIZE(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_SIZE)
#define BT_OTC_GET_METADATA_REQ_CREATED(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_CREATED)
#define BT_OTC_GET_METADATA_REQ_MODIFIED(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_MODIFIED)
#define BT_OTC_GET_METADATA_REQ_ID(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_ID)
#define BT_OTC_GET_METADATA_REQ_PROPS(metadata) \
	((metadata) & BT_OTC_METADATA_REQ_PROPS)

/**@brief Metadata of an OTS Object  */
struct bt_otc_obj_metadata {
	char name[BT_OTS_NAME_MAX_SIZE + 1];
	struct bt_ots_obj_type type_uuid;
	uint32_t current_size;
	uint32_t alloc_size;
	struct bt_date_time first_created;
	struct bt_date_time modified;
	uint64_t id;
	uint32_t properties;
};

struct bt_otc_instance_t {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t feature_handle;
	uint16_t obj_name_handle;
	uint16_t obj_type_handle;
	uint16_t obj_size_handle;
	uint16_t obj_properties_handle;
	uint16_t obj_created_handle;
	uint16_t obj_modified_handle;
	uint16_t obj_id_handle;
	uint16_t oacp_handle;
	uint16_t olcp_handle;

	struct bt_gatt_subscribe_params oacp_sub_params;
	struct bt_gatt_discover_params oacp_sub_disc_params;
	struct bt_gatt_subscribe_params olcp_sub_params;
	struct bt_gatt_discover_params olcp_sub_disc_params;

	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_proc;
	struct bt_otc_cb *cb;

	struct bt_ots_feat features;

	struct bt_otc_obj_metadata cur_object;
};

/** @brief Callback used decoding the directory listing.
 *
 *  @param object The object metadata that was decoded.
 *
 *  @return int   BT_OTC_STOP or BT_OTC_CONTINUE. BT_OTC_STOP can be used to
 *                stop the decoding.
 */
typedef int (*bt_otc_dirlisting_cb)(struct bt_otc_obj_metadata *object);

struct bt_otc_cb {
	/** @brief Callback function when a new object is selected.
	 *
	 *  Called when the a new object is selected and the current object
	 *  has changed. The `cur_object` in `ots_inst` will have been reset,
	 *  and metadata should be read again with `bt_otc_obj_metadata_read`.
	 *
	 *  @param conn              The connection to the peer device.
	 *  @param err               Error code (bt_ots_olcp_res_code).
	 *  @param ots_inst          Pointer to the OTC instance.
	 */
	void (*obj_selected)(struct bt_conn *conn, int err,
			     struct bt_otc_instance_t *ots_inst);


	/** @brief Callback function for content of the selected object.
	 *
	 *  Called when the object content is received.
	 *
	 *  @param conn          The connection to the peer device.
	 *  @param offset        Offset of the received data.
	 *  @param len           Length of the received data.
	 *  @param data_p        Pointer to the received data.
	 *  @param is_complete   Indicate if the whole object has been received.
	 *  @param ots_inst      Pointer to the OTC instance.
	 *
	 *  @return int          BT_OTC_STOP or BT_OTC_CONTINUE. BT_OTC_STOP can
	 *                       be used to stop reading.
	 */
	int (*content_cb)(struct bt_conn *conn, uint32_t offset, uint32_t len,
			  uint8_t *data_p, bool is_complete,
			  struct bt_otc_instance_t *ots_inst);

	/** @brief Callback function for metadata of the selected object.
	 *
	 *  Called when metadata of the selected object are read. Not all of
	 *  the metadata may have been initialized.
	 *
	 *  @param conn              The connection to the peer device.
	 *  @param err               Error value. 0 on success,
	 *                           GATT error or ERRNO on fail.
	 *  @param ots_inst          Pointer to the OTC instance.
	 *  @param metadata_read     Bitfield of the metadata that was
	 *                           successfully read.
	 */
	void (*metadata_cb)(struct bt_conn *conn, int err,
			    struct bt_otc_instance_t *ots_inst,
			    uint8_t metadata_read);
};

/** @brief Register an Object Transfer Service Instance.
 *
 *  Register an Object Transfer Service instance discovered on the peer.
 *  Call this function when an OTS instance is discovered
 *  (discovery is to be handled by the higher layer).
 *
 *  @param[in]  ots_inst      Discovered OTS instance.
 *
 *  @return int               0 if success, ERRNO on failure.
 */
int bt_otc_register(struct bt_otc_instance_t *ots_inst);

/** @brief OTS Indicate Handler function.
 *
 *  Set this function as callback for indicate handler when discovering OTS.
 *
 *   @param conn        Connection object. May be NULL, indicating that the
 *                      peer is being unpaired.
 *   @param params      Subscription parameters.
 *   @param data        Attribute value data. If NULL then subscription was
 *                      removed.
 *   @param length      Attribute value length.
 */
uint8_t bt_otc_indicate_handler(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length);

/** @brief Read the OTS feature characteristic.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_read_feature(struct bt_conn *conn,
			struct bt_otc_instance_t *otc_inst);

/** @brief Select an object by its Object ID.
 *
 *  @param conn         Pointer to the connection object.
 *  @param obj_id       Object's ID.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_select_id(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst,
		     uint64_t obj_id);

/** @brief Select the first object.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_select_first(struct bt_conn *conn,
			struct bt_otc_instance_t *otc_inst);

/** @brief Select the last object.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_select_last(struct bt_conn *conn,
		       struct bt_otc_instance_t *otc_inst);

/** @brief Select the next object.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_select_next(struct bt_conn *conn,
		       struct bt_otc_instance_t *otc_inst);

/** @brief Select the previous object.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_select_prev(struct bt_conn *conn,
		       struct bt_otc_instance_t *otc_inst);

/** @brief Read the metadata of the current object.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *  @param metadata     Bitfield (`BT_OTC_METADATA_REQ_*`) of the metadata
 *                      to read.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_obj_metadata_read(struct bt_conn *conn,
			     struct bt_otc_instance_t *otc_inst,
			     uint8_t metadata);

/** @brief Read the currently selected object.
 *
 *  This will trigger an OACP read operation for the current size of the object
 *  with a 0 offset and then expect receiving the content via the L2CAP CoC.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_read(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst);

/** @brief Used to decode the Directory Listing object into object metadata.
 *
 *  If the Directory Listing object contains multiple objects, then the
 *  callback will be called for each of them.
 *
 *  @param data        The data received for the directory listing object.
 *  @param length      Length of the data.
 *  @param cb          The callback that will be called for each object.
 */
int bt_otc_decode_dirlisting(uint8_t *data, uint16_t length,
			     bt_otc_dirlisting_cb cb);

/** @brief Displays one or more object metadata as text with BT_INFO.
 *
 * @param metadata Pointer to the first (or only) metadata in an array.
 * @param count    Number of metadata objects to display information of.
 */
void bt_otc_metadata_display(struct bt_otc_obj_metadata *metadata,
			     uint16_t count);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTC_H_ */
