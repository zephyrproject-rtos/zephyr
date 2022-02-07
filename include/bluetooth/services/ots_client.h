/* @file
 * @brief Object Transfer Service client header
 *
 * For use with the Object Transfer Service Client
 *
 * Copyright (c) 2020 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_CLIENT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_CLIENT_H_

/**
 * @brief Object Transfer Service Client (OTC)
 * @defgroup bt_otc Object Transfer Service Client (OTC)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <stdbool.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Date and Time structure.
 *  TODO: Move somewhere else - bluetooth.h?
 */
#define DATE_TIME_FIELD_SIZE 7
struct bt_date_time {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
};

#define BT_OTC_STOP                       0
#define BT_OTC_CONTINUE                   1

/**@brief Metadata of an OTS Object  */
struct bt_otc_obj_metadata {
	char name[CONFIG_BT_OTS_OBJ_MAX_NAME_LEN + 1];
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

/** @brief Directory listing object metadata callback
 *
 * If a directory listing is decoded using bt_otc_decode_dirlisting(),
 * this callback will be called for each object in the directory listing.
 *
 *  @param meta The metadata of the decoded object
 *
 *  @return int   BT_OTC_STOP or BT_OTC_CONTINUE. BT_OTC_STOP can be used to
 *                stop the decoding.
 */
typedef int (*bt_otc_dirlisting_cb)(struct bt_otc_obj_metadata *meta);

/** OTC callback structure */
struct bt_otc_cb {
	/** @brief Callback function when a new object is selected.
	 *
	 *  Called when the a new object is selected and the current object
	 *  has changed. The `cur_object` in `ots_inst` will have been reset,
	 *  and metadata should be read again with bt_otc_read_object_metadata().
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
 *  @param metadata     Bitfield (`BT_OTS_METADATA_REQ_*`) of the metadata
 *                      to read.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_read_object_metadata(struct bt_conn *conn,
				struct bt_otc_instance_t *otc_inst,
				uint8_t metadata);

/** @brief Read the (data of the) current selected object.
 *
 *  This will trigger an OACP read operation for the current size of the object
 *  with a 0 offset and then expect receiving the content via the L2CAP CoC.
 *
 *  @param conn         Pointer to the connection object.
 *  @param otc_inst     Pointer to the OTC instance.
 *
 *  @return int         0 if success, ERRNO on failure.
 */
int bt_otc_read_object_data(struct bt_conn *conn,
		       struct bt_otc_instance_t *otc_inst);

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

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_CLIENT_H_ */
