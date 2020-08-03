/* @file
 * @brief Object Transfer
 *
 * For use with the Object Transfer Service (OTS)
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_

#include <stdbool.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include "uint48_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque type representing a service instance. */
struct ots_svc_inst_t;

#define BT_OTS_MAX_OLCP_SIZE    7
#define BT_OTS_DIRLISTING_ID    0x000000000000
#define BT_OTS_ID_LEN           (UINT48_LEN)

#define SET_OR_CLEAR_BIT(var, bit_val, set) \
	((var) = (set) ? ((var) | bit_val) : ((var) & ~bit_val))

/** @brief Object Action Control Point Features flags field. */
enum {
	/** Bit 0 OACP Create Op Code Supported */
	BT_OTS_OACP_FEAT_CREATE     = BIT(0),
	/** Bit 1 OACP Delete Op Code Supported  */
	BT_OTS_OACP_FEAT_DELETE     = BIT(1),
	/** Bit 2 OACP Calculate Checksum Op Code Supported */
	BT_OTS_OACP_FEAT_CHECKSUM   = BIT(2),
	/** Bit 3 OACP Execute Op Code Supported */
	BT_OTS_OACP_FEAT_EXECUTE    = BIT(3),
	/** Bit 4 OACP Read Op Code Supported */
	BT_OTS_OACP_FEAT_READ       = BIT(4),
	/** Bit 5 OACP Write Op Code Supported */
	BT_OTS_OACP_FEAT_WRITE      = BIT(5),
	/** Bit 6 Appending Additional Data to Objects Supported  */
	BT_OTS_OACP_FEAT_APPEND     = BIT(6),
	/** Bit 7 Truncation of Objects Supported */
	BT_OTS_OACP_FEAT_TRUNCATE   = BIT(7),
	/** Bit 8 Patching of Objects Supported  */
	BT_OTS_OACP_FEAT_PATCH      = BIT(8),
	/** Bit 9 OACP Abort Op Code Supported */
	BT_OTS_OACP_FEAT_ABORT      = BIT(9),
};

#define BT_OTS_OACP_SET_FEAT_CREATE(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_CREATE, set)
#define BT_OTS_OACP_SET_FEAT_DELETE(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_DELETE, set)
#define BT_OTS_OACP_SET_FEAT_CHECKSUM(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_CHECKSUM, set)
#define BT_OTS_OACP_SET_FEAT_EXECUTE(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_EXECUTE, set)
#define BT_OTS_OACP_SET_FEAT_READ(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_READ, set)
#define BT_OTS_OACP_SET_FEAT_WRITE(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_WRITE, set)
#define BT_OTS_OACP_SET_FEAT_APPEND(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_APPEND, set)
#define BT_OTS_OACP_SET_FEAT_TRUNCATE(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_TRUNCATE, set)
#define BT_OTS_OACP_SET_FEAT_PATCH(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_PATCH, set)
#define BT_OTS_OACP_SET_FEAT_ABORT(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OACP_FEAT_ABORT, set)

#define BT_OTS_OACP_GET_FEAT_CREATE(feat)   ((feat) & BT_OTS_OACP_FEAT_CREATE)
#define BT_OTS_OACP_GET_FEAT_DELETE(feat)   ((feat) & BT_OTS_OACP_FEAT_DELETE)
#define BT_OTS_OACP_GET_FEAT_CHECKSUM(feat) ((feat) & BT_OTS_OACP_FEAT_CHECKSUM)
#define BT_OTS_OACP_GET_FEAT_EXECUTE(feat)  ((feat) & BT_OTS_OACP_FEAT_EXECUTE)
#define BT_OTS_OACP_GET_FEAT_READ(feat)     ((feat) & BT_OTS_OACP_FEAT_READ)
#define BT_OTS_OACP_GET_FEAT_WRITE(feat)    ((feat) & BT_OTS_OACP_FEAT_WRITE)
#define BT_OTS_OACP_GET_FEAT_APPEND(feat)   ((feat) & BT_OTS_OACP_FEAT_APPEND)
#define BT_OTS_OACP_GET_FEAT_TRUNCATE(feat) ((feat) & BT_OTS_OACP_FEAT_TRUNCATE)
#define BT_OTS_OACP_GET_FEAT_PATCH(feat)    ((feat) & BT_OTS_OACP_FEAT_PATCH)
#define BT_OTS_OACP_GET_FEAT_ABORT(feat)    ((feat) & BT_OTS_OACP_FEAT_ABORT)


/** @brief Object List Control Point Features flags field. */
enum {
	/** Bit 0 OLCP Go To Op Code Supported */
	BT_OTS_OLCP_FEAT_GO_TO      = BIT(0),
	/** Bit 1 OLCP Order Op Code Supported */
	BT_OTS_OLCP_FEAT_ORDER      = BIT(1),
	/** Bit 2 OLCP Request Number of Objects Op Code Supported */
	BT_OTS_OLCP_FEAT_NUM_REQ    = BIT(2),
	/** Bit 3 OLCP Clear Marking Op Code Supported*/
	BT_OTS_OLCP_FEAT_CLEAR      = BIT(3),
};


#define BT_OTS_OLCP_SET_FEAT_GO_TO(feat, set) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OLCP_FEAT_GO_TO, set)
#define BT_OTS_OLCP_SET_FEAT_ORDER(feat) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OLCP_FEAT_ORDER, set)
#define BT_OTS_OLCP_SET_FEAT_NUM_REQ(feat) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OLCP_FEAT_NUM_REQ, set)
#define BT_OTS_OLCP_SET_FEAT_CLEAR(feat) \
	SET_OR_CLEAR_BIT(feat, BT_OTS_OLCP_FEAT_CLEAR, set)

#define BT_OTS_OLCP_GET_FEAT_GO_TO(feat)     ((feat) & BT_OTS_OLCP_FEAT_GO_TO)
#define BT_OTS_OLCP_GET_FEAT_ORDER(feat)     ((feat) & BT_OTS_OLCP_FEAT_ORDER)
#define BT_OTS_OLCP_GET_FEAT_NUM_REQ(feat)   ((feat) & BT_OTS_OLCP_FEAT_NUM_REQ)
#define BT_OTS_OLCP_GET_FEAT_CLEAR(feat)     ((feat) & BT_OTS_OLCP_FEAT_CLEAR)

/** @brief Features of the OTS. */
struct bt_ots_feat {
	uint32_t oacp;
	uint32_t olcp;
};

/** @brief Properties of an OTS object. */
enum {
	/** Bit 0 Deletion of this object is permitted */
	BT_OTS_OBJ_PROP_DELETE      = BIT(0),
	/** Bit 1 Execution of this object is permitted */
	BT_OTS_OBJ_PROP_EXECUTE     = BIT(1),
	/** Bit 2 Reading this object is permitted */
	BT_OTS_OBJ_PROP_READ        = BIT(2),
	/** Bit 3 Writing data to this object is permitted */
	BT_OTS_OBJ_PROP_WRITE       = BIT(3),
	/** @brief Bit 4 Appending data to this object is permitted.
	 *
	 * Appending data increases its Allocated Size.
	 */
	BT_OTS_OBJ_PROP_APPEND      = BIT(4),
	/** Bit 5 Truncation of this object is permitted */
	BT_OTS_OBJ_PROP_TRUNCATE    = BIT(5),
	/** @brief Bit 6 Patching this object is permitted
	 *
	 *  Patching this object overwrites some of
	 *  the object's existing contents.
	 */
	BT_OTS_OBJ_PROP_PATCH       = BIT(6),
	/** Bit 7 This object is a marked object */
	BT_OTS_OBJ_PROP_MARKED      = BIT(7),
};

#define BT_OTS_OBJ_SET_PROP_DELETE(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_DELETE, set)
#define BT_OTS_OBJ_SET_PROP_EXECUTE(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_EXECUTE, set)
#define BT_OTS_OBJ_SET_PROP_READ(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_READ, set)
#define BT_OTS_OBJ_SET_PROP_WRITE(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_WRITE, set)
#define BT_OTS_OBJ_SET_PROP_APPEND(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_APPEND, set)
#define BT_OTS_OBJ_SET_PROP_TRUNCATE(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_TRUNCATE, set)
#define BT_OTS_OBJ_SET_PROP_PATCH(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_PATCH, set)
#define BT_OTS_OBJ_SET_PROP_MARKED(prop, set) \
	SET_OR_CLEAR_BIT(prop, BT_OTS_OBJ_PROP_MARKED, set)

#define BT_OTS_OBJ_GET_PROP_DELETE(prop)     ((prop) & BT_OTS_OBJ_PROP_DELETE)
#define BT_OTS_OBJ_GET_PROP_EXECUTE(prop)    ((prop) & BT_OTS_OBJ_PROP_EXECUTE)
#define BT_OTS_OBJ_GET_PROP_READ(prop)       ((prop) & BT_OTS_OBJ_PROP_READ)
#define BT_OTS_OBJ_GET_PROP_WRITE(prop)      ((prop) & BT_OTS_OBJ_PROP_WRITE)
#define BT_OTS_OBJ_GET_PROP_APPEND(prop)     ((prop) & BT_OTS_OBJ_PROP_APPEND)
#define BT_OTS_OBJ_GET_PROP_TRUNCATE(prop)   ((prop) & BT_OTS_OBJ_PROP_TRUNCATE)
#define BT_OTS_OBJ_GET_PROP_PATCH(prop)      ((prop) & BT_OTS_OBJ_PROP_PATCH)
#define BT_OTS_OBJ_GET_PROP_MARKED(prop)     ((prop) & BT_OTS_OBJ_PROP_MARKED)

/** @brief Directory Listing record flag field. */
enum {
	/** Bit 0 Object Type UUID Size 0: 16bit 1: 128bit*/
	BT_OTS_DIRLIST_FLAG_TYPE_128            = BIT(0),
	/** Bit 1 Current Size Present*/
	BT_OTS_DIRLIST_FLAG_CUR_SIZE            = BIT(1),
	/** Bit 2 Allocated Size Present */
	BT_OTS_DIRLIST_FLAG_ALLOC_SIZE          = BIT(2),
	/** Bit 3 Object First-Created Present*/
	BT_OTS_DIRLIST_FLAG_FIRST_CREATED       = BIT(3),
	/** Bit 4 Object Last-Modified Present*/
	BT_OTS_DIRLIST_FLAG_LAST_MODIFIED       = BIT(4),
	/** Bit 5 Object Properties Present*/
	BT_OTS_DIRLIST_FLAG_PROPERTIES          = BIT(5),
	/** Bit 6 RFU*/
	BT_OTS_DIRLIST_FLAG_RFU                 = BIT(6),
	/** Bit 7 Extended Flags Present*/
	BT_OTS_DIRLIST_FLAG_EXTENDED            = BIT(7),
};

#define BT_OTS_DIRLIST_SET_FLAG_TYPE_128(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_TYPE_128, set)
#define BT_OTS_DIRLIST_SET_FLAG_CUR_SIZE(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_CUR_SIZE, set)
#define BT_OTS_DIRLIST_SET_FLAG_ALLOC_SIZE(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_ALLOC_SIZE, set)
#define BT_OTS_DIRLIST_SET_FLAG_FIRST_CREATED(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_FIRST_CREATED, set)
#define BT_OTS_DIRLIST_SET_FLAG_LAST_MODIFIED(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_LAST_MODIFIED, set)
#define BT_OTS_DIRLIST_SET_FLAG_PROPERTIES(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_PROPERTIES, set)
#define BT_OTS_DIRLIST_SET_FLAG_RFU(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_RFU, set)
#define BT_OTS_DIRLIST_SET_FLAG_EXTENDED(flags, set) \
	SET_OR_CLEAR_BIT(flags, BT_OTS_DIRLIST_FLAG_EXTENDED, set)

#define BT_OTS_DIRLIST_GET_FLAG_TYPE_128(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_TYPE_128)
#define BT_OTS_DIRLIST_GET_FLAG_CUR_SIZE(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_CUR_SIZE)
#define BT_OTS_DIRLIST_GET_FLAG_ALLOC_SIZE(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_ALLOC_SIZE)
#define BT_OTS_DIRLIST_GET_FLAG_FIRST_CREATED(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_FIRST_CREATED)
#define BT_OTS_DIRLIST_GET_FLAG_LAST_MODIFIED(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_LAST_MODIFIED)
#define BT_OTS_DIRLIST_GET_FLAG_PROPERTIES(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_PROPERTIES)
#define BT_OTS_DIRLIST_GET_FLAG_RFU(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_RFU)
#define BT_OTS_DIRLIST_GET_FLAG_EXTENDED(flags) \
	((flags) & BT_OTS_DIRLIST_FLAG_EXTENDED)

/** @brief Type of an OTS Object. */
struct bt_ots_obj_type {
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	};
};

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

#define BT_OTS_NAME_MAX_SIZE           120
/* TODO: Add Kconfig to control max number of objects */
#define BT_OTS_MAX_OBJ_CNT             50

/** @brief OTS Attribute Protocol Application Error codes. */
enum bt_ots_write_err_codes {
	/** @brief The write request was rejected
	 *
	 *  An attempt was made to write a value that is invalid or
	 *  not supported by this Server for a reason other than
	 *  the attribute permissions.
	 */
	BT_OTS_WRITE_REQUEST_REJECTED		= 0x80,
	/** @brief No object was selected.
	 *
	 *  An attempt was made to read or write to an Object Metadata
	 *  characteristic while the Current Object was an Invalid Object.
	 */
	BT_OTS_OBJECT_NOT_SELECTED		= 0x81,
	/** @brief Currency limit exceeeded.
	 *
	 *  The Server is unable to service the Read Request or Write Request
	 *  because it exceeds the concurrency limit of the service.
	 */
	BT_OTS_CONCURRENCY_LIMIT_EXCEEDED	= 0x82,
	/** @brief The object name already exists
	 *
	 *  The requested object name was rejected because
	 *  the name was already in use by an existing object on the Server.
	 */
	BT_OTS_OBJECT_NAME_ALREADY_EXISTS	= 0x83,
};

/*----------------------- OTS OCAP -------------------------------------*/

/** @brief  Types of Object Action Control Point Procedures. */
enum bt_ots_oacp_proc_type {
	/** Create object.*/
	BT_OTS_OACP_PROC_CREATE        = 0x01,
	/** Delete object.*/
	BT_OTS_OACP_PROC_DELETE        = 0x02,
	/** Calculate Checksum.*/
	BT_OTS_OACP_PROC_CALC_CHKSUM   = 0x03,
	/** Execute Object.*/
	BT_OTS_OACP_PROC_EXECUTE       = 0x04,
	/** Read object.*/
	BT_OTS_OACP_PROC_READ          = 0x05,
	/** Write object.*/
	BT_OTS_OACP_PROC_WRITE         = 0x06,
	/** Abort object.*/
	BT_OTS_OACP_PROC_ABORT         = 0x07,
	/** Procedure response.*/
	BT_OTS_OACP_PROC_RESP          = 0x60
};

/** @brief Object Action Control Point return codes. */
enum bt_ots_oacp_res_code {
	/** Success.*/
	BT_OTS_OACP_RES_SUCCESS        = 0x01,
	/** Not supported*/
	BT_OTS_OACP_RES_OPCODE_NOT_SUP = 0x02,
	/** Invalid parameter*/
	BT_OTS_OACP_RES_INV_PARAM      = 0x03,
	/** Insufficient resources.*/
	BT_OTS_OACP_RES_INSUFF_RES     = 0x04,
	/** Invalid object.*/
	BT_OTS_OACP_RES_INV_OBJ        = 0x05,
	/** Channel unavailable.*/
	BT_OTS_OACP_RES_CHAN_UNAVAIL   = 0x06,
	/** Unsupported procedure.*/
	BT_OTS_OACP_RES_UNSUP_TYPE     = 0x07,
	/** Procedure not permitted.*/
	BT_OTS_OACP_RES_NOT_PERMITTED  = 0x08,
	/** Object locked.*/
	BT_OTS_OACP_RES_OBJ_LOCKED     = 0x09,
	/** Operation Failed.*/
	BT_OTS_OACP_RES_OPER_FAILED    = 0x0A
};

/* Object Action Control Point procedure definition. */
struct bt_ots_oacp_proc {
	uint8_t type;
	union {
		struct {
			uint32_t size;
			struct bt_ots_obj_type obj_type;
		} create_params;
		struct {
			uint32_t offset;
			uint32_t length;
		} checksum_params;
		struct {
			uint8_t cmd_data_len;
			uint8_t *p_cmd_data;
		} execute_params;
		struct {
			uint32_t offset;
			uint32_t length;
		} read_params;
		struct {
			uint32_t offset;
			uint32_t length;
			uint8_t write_mode;
		} write_params;
	};
};

/*--------------------- OTS OLCP ----------------------------------------*/
/** @brief The types of OLCP procedures. */
enum bt_ots_olcp_proc_type {
	/**Select the first object.*/
	BT_OTS_OLCP_PROC_FIRST         = 0x01,
	/**Select the last object.*/
	BT_OTS_OLCP_PROC_LAST          = 0x02,
	/**Select the previous object.*/
	BT_OTS_OLCP_PROC_PREV          = 0x03,
	/**Select the next object.*/
	BT_OTS_OLCP_PROC_NEXT          = 0x04,
	/**Select the object with the given object ID.*/
	BT_OTS_OLCP_PROC_GOTO          = 0x05,
	/**Order the objects.*/
	BT_OTS_OLCP_PROC_ORDER         = 0x06,
	/**Request the number of objects.*/
	BT_OTS_OLCP_PROC_REQ_NUM_OBJS  = 0x07,
	/**Clear Marking.*/
	BT_OTS_OLCP_PROC_CLEAR_MARKING = 0x08,
	/**Response.*/
	BT_OTS_OLCP_PROC_RESP          = 0x70,
};

/** @brief The types of OLCP sort orders. */
enum bt_ots_olcp_sort_order {
	/** Order the list by object name, ascending */
	BT_OTS_SORT_BY_NAME_ASCEND     = 0x01,
	/** Order the list by object type, ascending*/
	BT_OTS_SORT_BY_TYPE_ASCEND     = 0x02,
	/** Order the list by object current size, ascending*/
	BT_OTS_SORT_BY_SIZE_ASCEND     = 0x03,
	/** Order the list by object first-created timestamp, ascending*/
	BT_OTS_SORT_BY_FC_ASCEND       = 0x04,
	/** Order the list by object last-modified timestamp, ascending */
	BT_OTS_SORT_BY_LM_ASCEND       = 0x05,
	/** Order the list by object name, descending */
	BT_OTS_SORT_BY_NAME_DESCEND    = 0x11,
	/** Order the list by object type, descending*/
	BT_OTS_SORT_BY_TYPE_DESCEND    = 0x12,
	/** Order the list by object current size, descending*/
	BT_OTS_SORT_BY_SIZE_DESCEND    = 0x13,
	/** Order the list by object first-created timestamp, descending*/
	BT_OTS_SORT_BY_FC_DESCEND      = 0x14,
	/** Order the list by object last-modified timestamp, descending */
	BT_OTS_SORT_BY_LM_DESCEND      = 0x15,
};

/** @brief Definition of a OLCP procedure. */
struct bt_ots_olcp_proc {
	uint8_t type;
	union {
		uint64_t goto_id;
		enum bt_ots_olcp_sort_order sort_order;
	};
};

/** @brief The return codes obtained from doing OLCP procedures. */
enum bt_ots_olcp_res_code {
	/** Response for successful operation. */
	BT_OTS_OLCP_RES_SUCCESS			= 0x01,
	/**Response if unsupported Op Code is received.*/
	BT_OTS_OLCP_RES_PROC_NOT_SUP		= 0x02,
	/** @brief Invalid parameters
	 *
	 *  Response if Parameter received does not meet
	 *  the requirements of the service.
	 */
	BT_OTS_OLCP_RES_INVALID_PARAMETER	= 0x03,
	/** @brief Operation failed
	 *
	 *  Response if the requested procedure failed for a reason
	 *  other than those enumerated below.
	 */
	BT_OTS_OLCP_RES_OPERATION_FAILED	= 0x04,
	/** @brief Object out of bounds
	 *
	 *  Response if the requested procedure attempted to select an object
	 *  beyond the first object or
	 *  beyond the last object in the current list.
	 */
	BT_OTS_OLCP_RES_OUT_OF_BONDS		= 0x05,
	/** @brief Too many objects
	 *
	 *  Response if the requested procedure failed due
	 *  to too many objects in the current list.
	 */
	BT_OTS_OLCP_RES_TOO_MANY_OBJECTS	= 0x06,
	/** @brief No object
	 *
	 *  Response if the requested procedure failed due
	 *  to there being zero objects in the current list.
	 */
	BT_OTS_OLCP_RES_NO_OBJECT		= 0x07,
	/** @brief Object ID not found
	 *
	 *  Response if the requested procedure failed due
	 *  to there being no object with the requested Object ID.
	 */
	BT_OTS_OLCP_RES_OBJECT_ID_NOT_FOUND	= 0x08,
};

/** @brief Callback function for content of the selected object.
 *
 *  Called by the OTS server to get the content data to send when the object
 *  content is read by the the OTS client.
 *
 *  @param inst         Pointer to the service instance.
 *  @param id           Object ID being sent.
 *  @param offset       Offset of the data to be sent.
 *  @param len          Length of the data to be sent.
 *  @param data         Pointer to the data to send.
 *
 *  @return             Length of the buffer containing the data to send.
 *
 */
typedef uint32_t (*bt_ots_obj_content_cb_t)(struct ots_svc_inst_t *inst,
					    uint64_t id, uint32_t offset,
					    uint32_t len, uint8_t **data);

/** @brief Callback function for object selected.
 *
 *  @param id           Object ID of the selected object.
 */
typedef void (*bt_ots_obj_on_select_cb_t)(uint64_t id);

/** @brief Object metadata.
 *
 *  Object metadata, kept by the user of OTS.
 *
 *  Note that there are two ways for the OTS server to access the data of the
 *  object from the user when the object is read - it can either use the
 *  "content" pointer, or it can use the "content_cb" callback.  Only one of
 *  these should be set, the other should be NULL.
 *  (Alternatively: The OTS will try to use <whichever> first, if that is NULL,
 *  <the other> will be used.
 */
struct bt_ots_obj_metadata {
	uint32_t                      size;
	struct bt_ots_obj_type        type;
	char                          *name;
	uint32_t                      properties;
};

/** @brief OTS server callback structure. */
struct bt_ots_cb {
	/** Object created callback
	 *
	 *  This callback is called whenever a new object is created.
	 *  Application can reject this request by returning an error
	 *  when it does not have necessary resources to hold this new
	 *  object.
	 *
	 *  @param ots  OTS Server instance
	 *  @param conn The connection that is requesting object creation or
	 *              NULL if object is created by bt_ots_object_add()
	 *  @param id   Object ID
	 *  @param init Object initialization metadata
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  Possible return values:
	 *  -ENOMEM if no available space for new object.
	 */
	int (*obj_created)(struct ots_svc_inst_t *ots, struct bt_conn *conn,
			   uint64_t id,
			   const struct bt_ots_obj_metadata *metadata);

	/** Object deleted callback
	 *
	 *  This callback is called whenever an object is deleted.
	 *
	 *  @param ots  OTS Server instance
	 *  @param conn The connection that deleted the object or NULL if
	 *              this request came from the server
	 *  @param id   Object ID
	 */
	void (*obj_deleted)(struct ots_svc_inst_t *ots, struct bt_conn *conn,
			    uint64_t id);

	/** Object selected callback
	 *
	 *  This callback is called on successful object selection.
	 *
	 *  @param ots  OTS Server instance
	 *  @param conn The connection that selected new object
	 *  @param id   Object ID
	 */
	void (*obj_selected)(struct ots_svc_inst_t *ots, struct bt_conn *conn,
			     uint64_t id);

	/** Object read callback
	 *
	 *  This callback is called multiple times during the Object read
	 *  operation. OTS module will keep requesting successive Object
	 *  fragments from the application until the read operation is
	 *  completed. The end of read operation is indicated by NULL data
	 *  parameter.
	 *
	 *  @param ots    OTS Server instance
	 *  @param conn   The connection that read object
	 *  @param id     Object ID
	 *  @param data   In:  NULL once the read operations is completed.
	 *                Out: Next chunk of data to be sent
	 *  @param len    Remaining length requested by the client
	 *  @param offset Object data offset
	 *
	 *  @return Data length to be sent via data parameter. This value
	 *          should be smaller or equal to the len parameter.
	 */
	uint32_t (*obj_read)(struct ots_svc_inst_t *ots, struct bt_conn *conn,
			     uint64_t id, uint8_t **data, uint32_t len,
			     uint32_t offset);
};

/** @brief Descriptor for OTS initialization. */
struct bt_ots_service_register_t {
	/* OTS features */
	struct bt_ots_feat features;

	/* Callbacks */
	struct bt_ots_cb *cb;
};

/************************ SERVER API ************************/
/** @brief Register a OTS service as a secondary service
 *
 *  @param service_reg   Service register request information.
 *
 *  @return Pointer to the instance if success, else NULL.
 */
struct ots_svc_inst_t *bt_ots_register_service(
	struct bt_ots_service_register_t *service_reg);

/** @brief Unregister a OTS service instance.
 *
 *  This will clear all data in the service.
 *
 * @param inst     Pointer o the instance.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_ots_unregister_service(struct ots_svc_inst_t *inst);

/** @brief Get the service pointer to include as secondary service.
 *
 * @param inst  The OTS instance to get the service pointer of.
 *
 * @return      Pointer that can be used for include.
 */
void *bt_ots_get_incl(struct ots_svc_inst_t *inst);

/** @brief Add an object to OTS.
 *
 *  @param inst         Pointer to the instance.
 *  @param p_metadata   Meta data of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_ots_add_object(struct ots_svc_inst_t *inst,
		      struct bt_ots_obj_metadata *p_metadata);

/** @brief Remove an object from OTS.
 *
 * TODO: Should consider using anonymous object pointers instead of IDs?
 *
 *  @param inst       Pointer to the OTS instance.
 *  @param id         Id of the object to remove (u48).
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_ots_remove_object(struct ots_svc_inst_t *inst, uint64_t id);

/** @brief
 *
 *  @param inst       Pointer to the OTS instance.
 *  @param inst Pointer to the OTS instance.
 *  @param id   ID of the object to get.
 *
 *  @return Pointer to a copy of the object metadata.
 */
struct bt_ots_obj_metadata *bt_ots_get_object(struct ots_svc_inst_t *inst,
					      uint64_t id);

/** @brief Update the metadata of an object in OTS.
 *
 *  @param inst       Pointer to the OTS instance.
 *  @param id         ID of the updated object (u48).
 *  @param metadata   (Pointer to) metadata of the updated object.
 */
int bt_ots_update_object(struct ots_svc_inst_t *inst, uint64_t id,
			 struct bt_ots_obj_metadata *p_metadata);

#if defined(CONFIG_BT_DEBUG_OTS)

/** @brief Dump the content of the directory listing object in Hex (Debug).
 *
 *  @param inst Pointer to the OTS instance.
 */
void bt_ots_dump_directory_listing(struct ots_svc_inst_t *inst);

#endif /* defined(CONFIG_BT_DEBUG_OTS) */


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_ */
