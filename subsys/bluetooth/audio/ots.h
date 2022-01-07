/* @file
 * @brief Object Transfer
 *
 * For use with the Object Transfer Service (OTS)
 *
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_

/* TODO: Temporarily here - clean up, and move alongside the Object Transfer Service */

#include <stdbool.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque type representing a service instance. */
struct ots_svc_inst_t;

/** @brief Size of OTS object ID (in bytes). */
#define BT_OTS_OBJ_ID_SIZE 6

/** @brief Length of OTS object ID string (in bytes). */
#define BT_OTS_OBJ_ID_STR_LEN 15

#define BT_OTS_MAX_OLCP_SIZE    7
#define BT_OTS_DIRLISTING_ID    0x000000000000
#define BT_OTS_ID_LEN           (BT_OTS_OBJ_ID_SIZE)


#define SET_OR_CLEAR_BIT(var, bit_val, set) \
	((var) = (set) ? ((var) | bit_val) : ((var) & ~bit_val))


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


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_H_ */
