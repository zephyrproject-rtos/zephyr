/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_H_

/**
 * @brief Object Transfer Service (OTS)
 * @defgroup bt_ots Object Transfer Service (OTS)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>

/** @brief Size of OTS object ID (in bytes). */
#define BT_OTS_OBJ_ID_SIZE 6

/** @brief Length of OTS object ID string (in bytes). */
#define BT_OTS_OBJ_ID_STR_LEN 15

/** @brief Type of an OTS object. */
struct bt_ots_obj_type {
	union {
		/* Used to indicate UUID type */
		struct bt_uuid uuid;

		/* 16-bit UUID value */
		struct bt_uuid_16 uuid_16;

		/* 128-bit UUID value */
		struct bt_uuid_128 uuid_128;
	};
};

/** @brief Properties of an OTS object. */
enum {
	/** Bit 0 Deletion of this object is permitted */
	BT_OTS_OBJ_PROP_DELETE    = 0,

	/** Bit 1 Execution of this object is permitted */
	BT_OTS_OBJ_PROP_EXECUTE   = 1,

	/** Bit 2 Reading this object is permitted */
	BT_OTS_OBJ_PROP_READ      = 2,

	/** Bit 3 Writing data to this object is permitted */
	BT_OTS_OBJ_PROP_WRITE     = 3,

	/** @brief Bit 4 Appending data to this object is permitted.
	 *
	 * Appending data increases its Allocated Size.
	 */
	BT_OTS_OBJ_PROP_APPEND    = 4,

	/** Bit 5 Truncation of this object is permitted */
	BT_OTS_OBJ_PROP_TRUNCATE  = 5,

	/** @brief Bit 6 Patching this object is permitted
	 *
	 *  Patching this object overwrites some of
	 *  the object's existing contents.
	 */
	BT_OTS_OBJ_PROP_PATCH     = 6,

	/** Bit 7 This object is a marked object */
	BT_OTS_OBJ_PROP_MARKED    = 7,
};

/** @brief Set @ref BT_OTS_OBJ_PROP_DELETE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_DELETE(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_DELETE, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_EXECUTE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_EXECUTE(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_EXECUTE, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_READ property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_READ(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_READ, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_WRITE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_WRITE(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_WRITE, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_APPEND property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_APPEND(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_APPEND, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_TRUNCATE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_TRUNCATE(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_TRUNCATE, 1)

/** @brief Set @ref BT_OTS_OBJ_PROP_PATCH property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_PATCH(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_PATCH, 1)

/** @brief Set @ref BT_OTS_OBJ_SET_PROP_MARKED property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_SET_PROP_MARKED(prop) \
	WRITE_BIT(prop, BT_OTS_OBJ_PROP_MARKED, 1)

/** @brief Get @ref BT_OTS_OBJ_PROP_DELETE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_DELETE(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_DELETE))

/** @brief Get @ref BT_OTS_OBJ_PROP_EXECUTE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_EXECUTE(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_EXECUTE))

/** @brief Get @ref BT_OTS_OBJ_PROP_READ property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_READ(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_READ))

/** @brief Get @ref BT_OTS_OBJ_PROP_WRITE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_WRITE(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_WRITE))

/** @brief Get @ref BT_OTS_OBJ_PROP_APPEND property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_APPEND(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_APPEND))

/** @brief Get @ref BT_OTS_OBJ_PROP_TRUNCATE property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_TRUNCATE(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_TRUNCATE))

/** @brief Get @ref BT_OTS_OBJ_PROP_PATCH property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_PATCH(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_PATCH))

/** @brief Get @ref BT_OTS_OBJ_PROP_MARKED property.
 *
 *  @param prop Object properties.
 */
#define BT_OTS_OBJ_GET_PROP_MARKED(prop) \
	((prop) & BIT(BT_OTS_OBJ_PROP_MARKED))

/** @brief Descriptor for OTS Object Size parameter. */
struct bt_ots_obj_size {
	/* Current Size */
	uint32_t cur;

	/* Allocated Size */
	uint32_t alloc;
} __packed;

/** @brief Descriptor for OTS object initialization. */
struct bt_ots_obj_metadata {
	/* Object Name */
	char                   *name;

	/* Object Type */
	struct bt_ots_obj_type type;

	/* Object Size */
	struct bt_ots_obj_size size;

	/* Object Properties */
	uint32_t               props;
};

/** @brief Object Action Control Point Feature bits. */
enum {
	/** Bit 0 OACP Create Op Code Supported */
	BT_OTS_OACP_FEAT_CREATE     = 0,

	/** Bit 1 OACP Delete Op Code Supported  */
	BT_OTS_OACP_FEAT_DELETE     = 1,

	/** Bit 2 OACP Calculate Checksum Op Code Supported */
	BT_OTS_OACP_FEAT_CHECKSUM   = 2,

	/** Bit 3 OACP Execute Op Code Supported */
	BT_OTS_OACP_FEAT_EXECUTE    = 3,

	/** Bit 4 OACP Read Op Code Supported */
	BT_OTS_OACP_FEAT_READ       = 4,

	/** Bit 5 OACP Write Op Code Supported */
	BT_OTS_OACP_FEAT_WRITE      = 5,

	/** Bit 6 Appending Additional Data to Objects Supported  */
	BT_OTS_OACP_FEAT_APPEND     = 6,

	/** Bit 7 Truncation of Objects Supported */
	BT_OTS_OACP_FEAT_TRUNCATE   = 7,

	/** Bit 8 Patching of Objects Supported  */
	BT_OTS_OACP_FEAT_PATCH      = 8,

	/** Bit 9 OACP Abort Op Code Supported */
	BT_OTS_OACP_FEAT_ABORT      = 9,
};

/** @brief Set @ref BT_OTS_OACP_SET_FEAT_CREATE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_CREATE(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_CREATE, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_DELETE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_DELETE(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_DELETE, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_CHECKSUM feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_CHECKSUM(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_CHECKSUM, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_EXECUTE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_EXECUTE(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_EXECUTE, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_READ feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_READ(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_READ, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_WRITE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_WRITE(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_WRITE, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_APPEND feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_APPEND(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_APPEND, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_TRUNCATE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_TRUNCATE(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_TRUNCATE, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_PATCH feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_PATCH(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_PATCH, 1)

/** @brief Set @ref BT_OTS_OACP_FEAT_ABORT feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_SET_FEAT_ABORT(feat) \
	WRITE_BIT(feat, BT_OTS_OACP_FEAT_ABORT, 1)

/** @brief Get @ref BT_OTS_OACP_FEAT_CREATE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_CREATE(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_CREATE))

/** @brief Get @ref BT_OTS_OACP_FEAT_DELETE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_DELETE(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_DELETE))

/** @brief Get @ref BT_OTS_OACP_FEAT_CHECKSUM feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_CHECKSUM(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_CHECKSUM))

/** @brief Get @ref BT_OTS_OACP_FEAT_EXECUTE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_EXECUTE(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_EXECUTE))

/** @brief Get @ref BT_OTS_OACP_FEAT_READ feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_READ(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_READ))

/** @brief Get @ref BT_OTS_OACP_FEAT_WRITE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_WRITE(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_WRITE))

/** @brief Get @ref BT_OTS_OACP_FEAT_APPEND feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_APPEND(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_APPEND))

/** @brief Get @ref BT_OTS_OACP_FEAT_TRUNCATE feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_TRUNCATE(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_TRUNCATE))

/** @brief Get @ref BT_OTS_OACP_FEAT_PATCH feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_PATCH(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_PATCH))

/** @brief Get @ref BT_OTS_OACP_FEAT_ABORT feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OACP_GET_FEAT_ABORT(feat) \
	((feat) & BIT(BT_OTS_OACP_FEAT_ABORT))

/** @brief Object List Control Point Feature bits. */
enum {
	/** Bit 0 OLCP Go To Op Code Supported */
	BT_OTS_OLCP_FEAT_GO_TO      = 0,

	/** Bit 1 OLCP Order Op Code Supported */
	BT_OTS_OLCP_FEAT_ORDER      = 1,

	/** Bit 2 OLCP Request Number of Objects Op Code Supported */
	BT_OTS_OLCP_FEAT_NUM_REQ    = 2,

	/** Bit 3 OLCP Clear Marking Op Code Supported*/
	BT_OTS_OLCP_FEAT_CLEAR      = 3,
};

/** @brief Set @ref BT_OTS_OLCP_FEAT_GO_TO feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_SET_FEAT_GO_TO(feat) \
	WRITE_BIT(feat, BT_OTS_OLCP_FEAT_GO_TO, 1)

/** @brief Set @ref BT_OTS_OLCP_FEAT_ORDER feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_SET_FEAT_ORDER(feat) \
	WRITE_BIT(feat, BT_OTS_OLCP_FEAT_ORDER, 1)

/** @brief Set @ref BT_OTS_OLCP_FEAT_NUM_REQ feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_SET_FEAT_NUM_REQ(feat) \
	WRITE_BIT(feat, BT_OTS_OLCP_FEAT_NUM_REQ, 1)

/** @brief Set @ref BT_OTS_OLCP_FEAT_CLEAR feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_SET_FEAT_CLEAR(feat) \
	WRITE_BIT(feat, BT_OTS_OLCP_FEAT_CLEAR, 1)

/** @brief Get @ref BT_OTS_OLCP_GET_FEAT_GO_TO feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_GET_FEAT_GO_TO(feat) \
	((feat) & BIT(BT_OTS_OLCP_FEAT_GO_TO))

/** @brief Get @ref BT_OTS_OLCP_GET_FEAT_ORDER feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_GET_FEAT_ORDER(feat) \
	((feat) & BIT(BT_OTS_OLCP_FEAT_ORDER))

/** @brief Get @ref BT_OTS_OLCP_GET_FEAT_NUM_REQ feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_GET_FEAT_NUM_REQ(feat) \
	((feat) & BIT(BT_OTS_OLCP_FEAT_NUM_REQ))

/** @brief Get @ref BT_OTS_OLCP_GET_FEAT_CLEAR feature.
 *
 *  @param feat OTS features.
 */
#define BT_OTS_OLCP_GET_FEAT_CLEAR(feat) \
	((feat) & BIT(BT_OTS_OLCP_FEAT_CLEAR))

/**@brief Features of the OTS. */
struct bt_ots_feat {
	/* OACP Features */
	uint32_t oacp;

	/* OLCP Features */
	uint32_t olcp;
} __packed;

/** @brief Opaque OTS instance. */
struct bt_ots;

/** @brief OTS callback structure. */
struct bt_ots_cb {
	/** @brief Object created callback
	 *
	 *  This callback is called whenever a new object is created.
	 *  Application can reject this request by returning an error
	 *  when it does not have necessary resources to hold this new
	 *  object. This callback is also triggered when the server
	 *  creates a new object with bt_ots_obj_add() API.
	 *
	 *  @param ots  OTS instance.
	 *  @param conn The connection that is requesting object creation or
	 *              NULL if object is created by the following function:
	 *              bt_ots_obj_add().
	 *  @param id   Object ID.
	 *  @param init Object initialization metadata.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  Possible return values:
	 *  -ENOMEM if no available space for new object.
	 */
	int (*obj_created)(struct bt_ots *ots, struct bt_conn *conn,
			   uint64_t id,
			   const struct bt_ots_obj_metadata *init);

	/** @brief Object deleted callback
	 *
	 *  This callback is called whenever an object is deleted. It is
	 *  also triggered when the server deletes an object with
	 *  bt_ots_obj_delete() API.
	 *
	 *  @param ots  OTS instance.
	 *  @param conn The connection that deleted the object or NULL if
	 *              this request came from the server.
	 *  @param id   Object ID.
	 */
	void (*obj_deleted)(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id);

	/** @brief Object selected callback
	 *
	 *  This callback is called on successful object selection.
	 *
	 *  @param ots  OTS instance.
	 *  @param conn The connection that selected new object.
	 *  @param id   Object ID.
	 */
	void (*obj_selected)(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id);

	/** @brief Object read callback
	 *
	 *  This callback is called multiple times during the Object read
	 *  operation. OTS module will keep requesting successive Object
	 *  fragments from the application until the read operation is
	 *  completed. The end of read operation is indicated by NULL data
	 *  parameter.
	 *
	 *  @param ots    OTS instance.
	 *  @param conn   The connection that read object.
	 *  @param id     Object ID.
	 *  @param data   In:  NULL once the read operations is completed.
	 *                Out: Next chunk of data to be sent.
	 *  @param len    Remaining length requested by the client.
	 *  @param offset Object data offset.
	 *
	 *  @return Data length to be sent via data parameter. This value
	 *          shall be smaller or equal to the len parameter.
	 */
	uint32_t (*obj_read)(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id, uint8_t **data, uint32_t len,
			     uint32_t offset);
};

/** @brief Descriptor for OTS initialization. */
struct bt_ots_init {
	/* OTS features */
	struct bt_ots_feat features;

	/* Callbacks */
	struct bt_ots_cb *cb;
};

/** @brief Add an object to the OTS instance.
 *
 *  This function adds an object to the OTS database. When the
 *  object is being added, a callback obj_created() is called
 *  to notify the user about a new object ID.
 *
 *  @param ots      OTS instance.
 *  @param obj_init Meta data of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_ots_obj_add(struct bt_ots *ots, struct bt_ots_obj_metadata *obj_init);

/** @brief Delete an object from the OTS instance.
 *
 *  This function deletes an object from the OTS database. When the
 *  object is deleted a callback obj_deleted() is called
 *  to notify the user about this event. At this point, it is possible
 *  to free allocated buffer for object data.
 *
 *  @param ots OTS instance.
 *  @param id  ID of the object to be deleted (uint48).
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_ots_obj_delete(struct bt_ots *ots, uint64_t id);

/** @brief Get the service declaration attribute.
 *
 *  This function is enabled for CONFIG_BT_OTS_SECONDARY_SVC configuration.
 *  The first service attribute can be included in any other GATT service.
 *
 *  @param ots OTS instance.
 *
 *  @return The first OTS attribute instance.
 */
void *bt_ots_svc_decl_get(struct bt_ots *ots);

/** @brief Initialize the OTS instance.
 *
 *  @param ots      OTS instance.
 *  @param ots_init OTS initialization descriptor.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_ots_init(struct bt_ots *ots, struct bt_ots_init *ots_init);

/** @brief Get a free instance of OTS from the pool.
 *
 *  @return OTS instance in case of success or NULL in case of error.
 */
struct bt_ots *bt_ots_free_instance_get(void);

/** @brief Converts binary OTS Object ID to string.
 *
 *  @param obj_id Object ID.
 *  @param str    Address of user buffer with enough room to store
 *                formatted string containing binary Object ID.
 *  @param len    Length of data to be copied to user string buffer.
 *                Refer to BT_OTS_OBJ_ID_STR_LEN about
 *                recommended value.
 *
 *  @return Number of successfully formatted bytes from binary ID.
 */
static inline int bt_ots_obj_id_to_str(uint64_t obj_id, char *str, size_t len)
{
	uint8_t id[6];

	sys_put_le48(obj_id, id);

	return snprintk(str, len, "0x%02X%02X%02X%02X%02X%02X",
			id[5], id[4], id[3], id[2], id[1], id[0]);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_OTS_H_ */
