/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net_buf.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/usb/class/usbd_mtp.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
#include "usbd_mtp_class.h"

LOG_MODULE_REGISTER(usb_mtp_class, CONFIG_USBD_MTP_LOG_LEVEL);

/* MTP Operation Codes */
#define MTP_OP_GET_DEVICE_INFO    0x1001
#define MTP_OP_OPEN_SESSION       0x1002
#define MTP_OP_CLOSE_SESSION      0x1003
#define MTP_OP_GET_STORAGE_IDS    0x1004
#define MTP_OP_GET_STORAGE_INFO   0x1005
#define MTP_OP_GET_OBJECT_HANDLES 0x1007
#define MTP_OP_GET_OBJECT_INFO    0x1008
#define MTP_OP_GET_OBJECT         0x1009
#define MTP_OP_DELETE_OBJECT      0x100B
#define MTP_OP_SEND_OBJECT_INFO   0x100C
#define MTP_OP_SEND_OBJECT        0x100D

/* MTP Response Codes */
#define MTP_RESP_UNDEFINED                                0x2000
#define MTP_RESP_OK                                       0x2001
#define MTP_RESP_GENERAL_ERROR                            0x2002
#define MTP_RESP_SESSION_NOT_OPEN                         0x2003
#define MTP_RESP_INVALID_TRANSACTION_ID                   0x2004
#define MTP_RESP_OPERATION_NOT_SUPPORTED                  0x2005
#define MTP_RESP_PARAMETER_NOT_SUPPORTED                  0x2006
#define MTP_RESP_INCOMPLETE_TRANSFER                      0x2007
#define MTP_RESP_INVALID_STORAGE_ID                       0x2008
#define MTP_RESP_INVALID_OBJECT_HANDLE                    0x2009
#define MTP_RESP_DEVICE_PROP_NOT_SUPPORTED                0x200A
#define MTP_RESP_INVALID_OBJECT_FORMAT_CODE               0x200B
#define MTP_RESP_STORAGE_FULL                             0x200C
#define MTP_RESP_OBJECT_WRITE_PROTECTED                   0x200D
#define MTP_RESP_STORE_READ_ONLY                          0x200E
#define MTP_RESP_ACCESS_DENIED                            0x200F
#define MTP_RESP_NO_THUMBNAIL_PRESENT                     0x2010
#define MTP_RESP_SELF_TEST_FAILED                         0x2011
#define MTP_RESP_PARTIAL_DELETION                         0x2012
#define MTP_RESP_STORE_NOT_AVAILABLE                      0x2013
#define MTP_RESP_SPECIFICATION_BY_FORMAT_UNSUPPORTED      0x2014
#define MTP_RESP_NO_VALID_OBJECT_INFO                     0x2015
#define MTP_RESP_INVALID_CODE_FORMAT                      0x2016
#define MTP_RESP_UNKNOWN_VENDOR_CODE                      0x2017
#define MTP_RESP_CAPTURE_ALREADY_TERMINATED               0x2018
#define MTP_RESP_DEVICE_BUSY                              0x2019
#define MTP_RESP_INVALID_PARENT_OBJECT                    0x201A
#define MTP_RESP_INVALID_DEVICE_PROP_FORMAT               0x201B
#define MTP_RESP_INVALID_DEVICE_PROP_VALUE                0x201C
#define MTP_RESP_INVALID_PARAMETER                        0x201D
#define MTP_RESP_SESSION_ALREADY_OPEN                     0x201E
#define MTP_RESP_TRANSACTION_CANCELLED                    0x201F
#define MTP_RESP_SPECIFICATION_OF_DESTINATION_UNSUPPORTED 0x2020
#define MTP_RESP_INVALID_OBJECT_PROP_CODE                 0xA801
#define MTP_RESP_INVALID_OBJECT_PROP_FORMAT               0xA802
#define MTP_RESP_INVALID_OBJECT_PROP_VALUE                0xA803
#define MTP_RESP_INVALID_OBJECT_REFERENCE                 0xA804
#define MTP_RESP_GROUP_NOT_SUPPORTED                      0xA805
#define MTP_RESP_INVALID_DATASET                          0xA806
#define MTP_RESP_SPECIFICATION_BY_GROUP_UNSUPPORTED       0xA807
#define MTP_RESP_SPECIFICATION_BY_DEPTH_UNSUPPORTED       0xA808
#define MTP_RESP_OBJECT_TOO_LARGE                         0xA809
#define MTP_RESP_OBJECT_PROP_NOT_SUPPORTED                0xA80A

/* MTP Image Formats */
#define MTP_FORMAT_UNDEFINED   0x3000
#define MTP_FORMAT_ASSOCIATION 0x3001
#define MTP_FORMAT_TEXT        0x3004

/* MTP Association Types */
#define MTP_ASSOCIATION_TYPE_UNDEFINED 0x0000
#define MTP_ASSOCIATION_TYPE_GENERIC   0x0001

/* MTP Event Codes */
#define MTP_EVENT_OBJECT_ADDED        0x4002
#define MTP_EVENT_OBJECT_REMOVED      0x4003
#define MTP_EVENT_STORE_ADDED         0x4004
#define MTP_EVENT_STORE_REMOVED       0x4005
#define MTP_EVENT_OBJECT_INFO_CHANGED 0x4007

/* MTP Device properties */
#define MTP_DEVICE_PROPERTY_BATTERY_LEVEL 0x5001

/* Storage Types */
#define STORAGE_TYPE_FIXED_ROM     0x0001
#define STORAGE_TYPE_REMOVABLE_ROM 0x0002
#define STORAGE_TYPE_FIXED_RAM     0x0003
#define STORAGE_TYPE_REMOVABLE_RAM 0x0004

/* MTP File system types */
#define FS_TYPE_GENERIC_HIERARCHICAL 0x0002

/* Object Protection */
#define OBJECT_PROTECTION_NO                0x0000
#define OBJECT_PROTECTION_READ_ONLY         0x0001
#define OBJECT_PROTECTION_READ_ONLY_DATA    0x8002
#define OBJECT_PROTECTION_NON_TRANSFERRABLE 0x8003

/* MACROS */
#define MTP_SERIAL_NUMBER_LEN     32U
#define MTP_ROOT_OBJ_HANDLE       0x00U
#define MTP_ALLROOTOBJECTS        0xFFFFFFFFU
#define MTP_ASSOCIATION_SIZE      0x00000000U
#define MTP_FREE_SPACE_OBJ_UNUSED 0xFFFFFFFFU
#define MTP_STORE_ROOT            0xFFFFFFFFU
#define MTP_ALLSTORAGES           0xFFFFFFFFU

#define STORAGE_TYPE_INTERNAL   0x00010000U
#define GENERATE_STORAGE_ID(id) (STORAGE_TYPE_INTERNAL + (uint32_t)(id))
#define MTP_STR_LEN(str)        (strlen(str) + 1)
/* MTP string length is one byte and includes the NULL UTF-16 code unit. */
#define MTP_MAX_FILE_NAME_LEN   MIN(MAX_FILE_NAME + 1U, UINT8_MAX)
#define MTP_HEADER_SIZE         sizeof(struct mtp_header)

/*
 * MTP ObjectInfo dataset fixed-size fields (excluding MTP header but including
 * the FileNameLength byte):
 *   StorageID(4) + ObjectFormat(2) + ProtectionStatus(2) + ObjectCompressedSize(4)
 *   + ThumbFormat(2) + ThumbCompressedSize(4) + ThumbPixWidth(4) + ThumbPixHeight(4)
 *   + ImagePixWidth(4) + ImagePixHeight(4) + ImageBitDepth(4) + ParentObject(4)
 *   + AssociationType(2) + AssociationDesc(4) + SequenceNumber(4) + FileNameLength(1)
 *   = 53 bytes
 */
#define MTP_OBJ_INFO_FIXED_FIELDS_SIZE                                                             \
	(4U + 2U + 2U + 4U + 2U + 4U + 4U + 4U + 4U + 4U + 4U + 4U + 2U + 4U + 4U + 1U)

#define MTP_CMD(opcode) mtp_##opcode(ctx, mtp_command, payload, buf_resp)

#define MTP_CMD_HANDLER(opcode)                                                                    \
	static void mtp_##opcode(struct mtp_context *ctx, struct mtp_container *mtp_command,       \
				 struct net_buf *payload, struct net_buf *buf)

/* MTP Objects in root should have ParentHandle=0x00 */
#define GET_PARENT_HANDLE(ctx, obj)                                                                \
	(obj->handle.parent_id == MTP_ROOT_OBJ_HANDLE ? MTP_ROOT_OBJ_HANDLE                        \
						      : ctx->partitions[obj->handle.partition_id]  \
								.objlist[obj->handle.parent_id]    \
								.handle.value)

#define MTP_PARAMS_DEBUG(mtp_command, count, ...)                                                  \
	LOG_DBG("Params: %s",                                                                      \
		mtp_params_debug_string(mtp_command, count, (const char *[]){__VA_ARGS__}))

/* Types */
enum mtp_container_type {
	MTP_CONTAINER_UNDEFINED = 0x00,
	MTP_CONTAINER_COMMAND,
	MTP_CONTAINER_DATA,
	MTP_CONTAINER_RESPONSE,
	MTP_CONTAINER_EVENT,
};

struct mtp_header {
	uint32_t length;         /* Total length of the response block */
	uint16_t type;           /* a value from \ref:mtp_container_type */
	uint16_t code;           /* MTP response code (e.g., MTP_RESP_OK) */
	uint32_t transaction_id; /* Transaction ID of the command being responded to */
} __packed;

struct mtp_container {
	struct mtp_header hdr;
	uint32_t param[5]; /* Optional Parameters */
} __packed;

union mtp_storage_id {
	struct {
		uint16_t id;
		uint16_t type;
	};
	uint32_t value;
};

/* Constants */
static const uint16_t mtp_operations[] = {
	MTP_OP_GET_DEVICE_INFO, MTP_OP_OPEN_SESSION,     MTP_OP_CLOSE_SESSION,
	MTP_OP_GET_STORAGE_IDS, MTP_OP_GET_STORAGE_INFO, MTP_OP_GET_OBJECT_HANDLES,
	MTP_OP_GET_OBJECT_INFO, MTP_OP_GET_OBJECT,
#if !defined(CONFIG_USBD_MTP_DISABLE_WRITE_ACCESS)
	MTP_OP_DELETE_OBJECT,   MTP_OP_SEND_OBJECT_INFO, MTP_OP_SEND_OBJECT,
#endif
};

static const uint16_t playback_formats[] = {
	MTP_FORMAT_UNDEFINED,
#if !defined(CONFIG_USBD_MTP_DISABLE_DIRECTORIES)
	MTP_FORMAT_ASSOCIATION,
#endif
};

/* Build Asserts */
/*
 * Maximum number of 32-bit array entries (object handles or storage IDs) that fit
 * in a single MTP data response buffer. The buffer is USBD_MAX_BULK_MPS bytes;
 * MTP_HEADER_SIZE is reserved as headroom and sizeof(uint32_t) is used for the
 * array element count field, leaving the rest for the array entries themselves.
 */
#define MTP_MAX_ARRAY_ENTRIES_PER_PACKET                                                           \
	((USBD_MAX_BULK_MPS - MTP_HEADER_SIZE - sizeof(uint32_t)) / sizeof(uint32_t))

BUILD_ASSERT(CONFIG_USBD_MTP_STORAGES_PER_INSTANCE <= MTP_MAX_ARRAY_ENTRIES_PER_PACKET,
	     "CONFIG_USBD_MTP_STORAGES_PER_INSTANCE too large to fit in a single response packet; "
	     "reduce CONFIG_USBD_MTP_STORAGES_PER_INSTANCE");
BUILD_ASSERT(CONFIG_USBD_MTP_MAX_HANDLES <= MTP_MAX_ARRAY_ENTRIES_PER_PACKET,
	     "CONFIG_USBD_MTP_MAX_HANDLES too large to fit in a single response packet; "
	     "reduce CONFIG_USBD_MTP_MAX_HANDLES");

/* Local functions declarations */
static int send_response_code(struct mtp_context *ctx, struct net_buf *buf, uint16_t err_code);
static int send_response_with_params(struct mtp_context *ctx, struct net_buf *buf,
				     uint16_t err_code, uint32_t *params, uint8_t params_count);
static inline void set_mtp_phase(struct mtp_context *ctx, enum mtp_phase phase);

static bool mtp_serial_number_valid(const char *serial_number)
{
	if (serial_number == NULL || strlen(serial_number) != MTP_SERIAL_NUMBER_LEN) {
		return false;
	}

	for (size_t i = 0; i < MTP_SERIAL_NUMBER_LEN; i++) {
		if (!isxdigit((unsigned char)serial_number[i])) {
			return false;
		}
	}

	return true;
}

static inline union mtp_storage_id decode_storage_id(uint32_t mtp_param_val)
{
	return (union mtp_storage_id){.value = mtp_param_val};
}

static inline union mtp_object_handle decode_object_handle(uint32_t mtp_param_val)
{
	return (union mtp_object_handle){.value = mtp_param_val};
}

static inline uint64_t fs_size_bytes(unsigned long blocks, unsigned long frsize)
{
	return (uint64_t)blocks * (uint64_t)frsize;
}

static inline const char *mtp_params_debug_string(struct mtp_container *mtp_command, size_t count,
						  const char *names[])
{
	static char debug_str[128];
	int offset = 0;
	uint32_t value;

	for (size_t i = 0; i < count; i++) {
		value = mtp_command->param[i];
		if (strcmp(names[i], "ObjHandle") == 0) {
			union mtp_object_handle handle = decode_object_handle(value);

			offset += snprintf(debug_str + offset, sizeof(debug_str) - offset,
					   "%s%s=0x%x(PartID=%u, ParentID=%u, ObjID=%u)",
					   (i > 0) ? ", " : "", names[i], value,
					   handle.partition_id, handle.parent_id, handle.object_id);
			continue;
		}
		offset += snprintf(debug_str + offset, sizeof(debug_str) - offset, "%s%s=0x%x",
				   (i > 0) ? ", " : "", names[i], value);
	}

	return debug_str;
}

const char *mtp_code_to_string(uint16_t code)
{
	static const char *const mtp_operation_names[] = {
		[0x01] = "GetDeviceInfo",      [0x02] = "OpenSession",
		[0x03] = "CloseSession",       [0x04] = "GetStorageIDs",
		[0x05] = "GetStorageInfo",     [0x06] = "GetNumObjects",
		[0x07] = "GetObjectHandles",   [0x08] = "GetObjectInfo",
		[0x09] = "GetObject",          [0x0A] = "GetThumb",
		[0x0B] = "DeleteObject",       [0x0C] = "SendObjectInfo",
		[0x0D] = "SendObject",         [0x10] = "ResetDevice",
		[0x14] = "GetDevicePropDesc",  [0x15] = "GetDevicePropValue",
		[0x16] = "SetDevicePropValue", [0x17] = "ResetDevicePropValue",
		[0x19] = "MoveObject",         [0x1A] = "CopyObject",
		[0x1B] = "GetPartialObject",
	};

	static const char *const mtp_response_names[] = {
		[0x01] = "OK",
		[0x02] = "GeneralError",
		[0x03] = "SessionNotOpen",
		[0x08] = "InvalidStorageID",
		[0x09] = "InvalidObjectHandle",
		[0x0C] = "StorageFull",
		[0x1E] = "SessionAlreadyOpen",
	};

	static const char *const mtp_format_names[] = {
		[0x01] = "Association",
		[0x04] = "Text",
	};

	static const char *const mtp_event_names[] = {
		[0x02] = "ObjectAdded",  [0x03] = "ObjectRemoved",     [0x04] = "StoreAdded",
		[0x05] = "StoreRemoved", [0x06] = "DevicePropChanged", [0x07] = "ObjectInfoChanged",
	};

	static const char *const mtp_device_prop_names[] = {
		[0x01] = "BatteryLevel",
	};

	static const char *const mtp_object_prop_names[] = {
		[0x01] = "StorageID",    [0x02] = "ObjectFormat",   [0x03] = "ProtectionStatus",
		[0x04] = "ObjectSize",   [0x07] = "ObjectFileName", [0x09] = "DateModified",
		[0x0B] = "ParentObject", [0x41] = "PersistentUID",  [0x44] = "Name",
		[0xE0] = "DisplayName",
	};

	static const char *const mtp_storage_type_names[] = {
		[0x01] = "FixedROM",
		[0x02] = "RemovableROM",
		[0x03] = "FixedRAM",
		[0x04] = "RemovableRAM",
	};

	if ((code & 0xFF00) == 0x1000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_operation_names) && mtp_operation_names[idx] != NULL) {
			return mtp_operation_names[idx];
		}
	} else if ((code & 0xFF00) == 0x2000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_response_names) && mtp_response_names[idx] != NULL) {
			return mtp_response_names[idx];
		}
	} else if ((code & 0xFF00) == 0x3000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_format_names) && mtp_format_names[idx] != NULL) {
			return mtp_format_names[idx];
		}
	} else if ((code & 0xFF00) == 0x4000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_event_names) && mtp_event_names[idx] != NULL) {
			return mtp_event_names[idx];
		}
	} else if ((code & 0xFF00) == 0x5000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_device_prop_names) && mtp_device_prop_names[idx] != NULL) {
			return mtp_device_prop_names[idx];
		}
	} else if ((code & 0xFF00) == 0xDC00) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_object_prop_names) && mtp_object_prop_names[idx] != NULL) {
			return mtp_object_prop_names[idx];
		}
	} else if ((code & 0xFF00) == 0x0000) {
		uint8_t idx = code & 0xFF;

		if (idx < ARRAY_SIZE(mtp_storage_type_names) &&
		    mtp_storage_type_names[idx] != NULL) {
			return mtp_storage_type_names[idx];
		}
	}

	switch (code) {
	case 0x9801:
		return "GetObjectPropsSupported";
	case 0x9802:
		return "GetObjectPropDesc";
	case 0x9803:
		return "GetObjectPropValue";
	case 0x9804:
		return "SetObjectPropValue";
	case 0x9810:
		return "GetObjectReferences";
	case 0x9811:
		return "SetObjectReferences";
	case 0x9820:
		return "Skip";
	}

	return "Unknown Code";
}

static void usb_buf_add_utf16le(struct net_buf *buf, const char *str, uint8_t len)
{
	if (!str) {
		LOG_ERR("string is NULL");
		return;
	}

	for (uint8_t i = 0; i < len; i++) {
		net_buf_add_le16(buf, str[i]);
	}
}

static void mtp_buf_add_string(struct net_buf *buf, const char *str)
{
	size_t len;

	if (!str) {
		net_buf_add_u8(buf, 0);
		return;
	}

	len = MTP_STR_LEN(str);
	if (len > UINT8_MAX) {
		LOG_ERR("MTP string is too long %zu", len);
		net_buf_add_u8(buf, 0);
		return;
	}

	net_buf_add_u8(buf, (uint8_t)len);
	usb_buf_add_utf16le(buf, str, (uint8_t)len);
}

static void usb_buf_pull_utf16le(struct net_buf *buf, char *strbuf, uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i) {
		strbuf[i] = net_buf_pull_u8(buf);
		net_buf_pull_u8(buf);
	}
}

static void mtp_buf_add_u16_array(struct net_buf *buf, const uint16_t *values, size_t count)
{
	net_buf_add_le32(buf, count);

	for (size_t i = 0; i < count; i++) {
		net_buf_add_le16(buf, values[i]);
	}
}

static void mtp_buf_push_data_header(struct mtp_context *ctx, struct net_buf *buf,
				     uint32_t data_len)
{
	struct mtp_header hdr = {
		.length = sys_cpu_to_le32((uint32_t)MTP_HEADER_SIZE + data_len),
		.type = sys_cpu_to_le16(MTP_CONTAINER_DATA),
		.code = sys_cpu_to_le16(ctx->op_state.code),
		.transaction_id = sys_cpu_to_le32(ctx->transaction_id),
	};

	/* Ensure we have enough headroom for the header */
	__ASSERT_NO_MSG(net_buf_headroom(buf) == MTP_HEADER_SIZE);

	net_buf_push_mem(buf, &hdr, MTP_HEADER_SIZE);
}

static inline void set_mtp_phase(struct mtp_context *ctx, enum mtp_phase phase)
{
	LOG_DBG("Transition: %d --> %d", ctx->phase, phase);
	ctx->phase = phase;
}

static int send_response_code(struct mtp_context *ctx, struct net_buf *buf, uint16_t err_code)
{
	ctx->op_state.err = err_code;

	/* Check if response code should be sent in
	 * the response phase (Host Ack'd last data packet)
	 */
	if (ctx->phase == MTP_PHASE_DATA) {
		set_mtp_phase(ctx, MTP_PHASE_RESPONSE);
		return 0;
	}

	return send_response_with_params(ctx, buf, ctx->op_state.err, NULL, 0);
}

static int send_response_with_params(struct mtp_context *ctx, struct net_buf *buf,
				     uint16_t err_code, uint32_t *params, uint8_t params_count)
{
	struct mtp_header mtp_response;

	if (buf == NULL) {
		LOG_ERR("%s: Null Buffer!", __func__);
		return -EINVAL;
	}

	if (params_count > 5) {
		LOG_ERR("%s: params_count exceeds maximum of 5!", __func__);
		return -EINVAL;
	}

	set_mtp_phase(ctx, MTP_PHASE_RESPONSE);
	ctx->op_state.err = err_code;

	mtp_response = (struct mtp_header){
		.length = sys_cpu_to_le32((uint32_t)MTP_HEADER_SIZE +
					  (uint32_t)(params_count * sizeof(uint32_t))),
		.type = sys_cpu_to_le16(MTP_CONTAINER_RESPONSE),
		.code = sys_cpu_to_le16(ctx->op_state.err),
		.transaction_id = sys_cpu_to_le32(ctx->transaction_id),
	};

	/* Make sure response is always in the beginning of the buffer */
	net_buf_reset(buf);
	net_buf_add_mem(buf, &mtp_response, MTP_HEADER_SIZE);

	for (uint8_t i = 0; i < params_count; i++) {
		net_buf_add_le32(buf, params[i]);
	}

	LOG_DBG("Sending Response (ErrCode: 0x%x [%s]), TID: %u", ctx->op_state.err,
		mtp_code_to_string(ctx->op_state.err), ctx->transaction_id);

	set_mtp_phase(ctx, MTP_PHASE_REQUEST);
	return 0;
}

static uint16_t validate_object_handle(struct mtp_context *ctx, union mtp_object_handle handle)
{
	if (handle.partition_id == 0 || handle.partition_id >= ctx->partitions_count) {
		return MTP_RESP_INVALID_STORAGE_ID;
	}
	if (handle.object_id >= ctx->partitions[handle.partition_id].files_count) {
		return MTP_RESP_INVALID_OBJECT_HANDLE;
	}
	if (ctx->partitions[handle.partition_id].objlist[handle.object_id].handle.value !=
	    handle.value) {
		return MTP_RESP_INVALID_OBJECT_HANDLE;
	}
	return MTP_RESP_OK;
}

static int construct_path(char *buf, size_t max_len, const char *path, const char *name)
{
	return snprintf(buf, max_len, "%s/%s", path, name);
}

static int traverse_path(struct mtp_context *ctx, struct mtp_object *obj, char *buf)
{
	int off = 0;
	int ret;

	if (obj->handle.parent_id != MTP_ROOT_OBJ_HANDLE) {
		off = traverse_path(
			ctx,
			&ctx->partitions[obj->handle.partition_id].objlist[obj->handle.parent_id],
			buf);

		if (off < 0) {
			return off;
		}

		ret = snprintf(&buf[off], MAX_OBJPATH_LEN - off, "/%s", obj->name);
	} else {
		ret = construct_path(buf, MAX_OBJPATH_LEN,
				     ctx->partitions[obj->handle.partition_id].mountpoint,
				     obj->name);
	}

	if (ret < 0) {
		buf[0] = '\0';
		return ret;
	}

	if (ret >= ((int)MAX_OBJPATH_LEN - off)) {
		buf[0] = '\0';
		return -ENAMETOOLONG;
	}

	return off + ret;
}

static struct mtp_object *add_object(struct mtp_context *ctx, uint8_t partition_id,
				     uint8_t parent_id, enum fs_dir_entry_type type,
				     const char *name, size_t size)
{
	struct mtp_partition *partition = &ctx->partitions[partition_id];

	if (partition->files_count >= MTP_MAX_FILES) {
		LOG_ERR("No object handle available [file count: %u]", partition->files_count);
		return NULL;
	}

	uint8_t obj_id = partition->files_count; /* files_count always stop at the new empty slot */
	struct mtp_object *obj = &partition->objlist[obj_id];

	memset(obj, 0x00, sizeof(struct mtp_object));
	obj->handle.partition_id = partition_id;
	obj->handle.parent_id = parent_id;
	obj->handle.object_id = obj_id;
	obj->handle.type = type;
	obj->size = (uint32_t)size;
	strncpy(obj->name, name, MAX_FILE_NAME);
	obj->name[MAX_FILE_NAME] = '\0';

	partition->files_count++;

	return obj;
}

static void remove_object(struct mtp_context *ctx, uint8_t partition_id, uint8_t object_id)
{
	struct mtp_partition *partition = &ctx->partitions[partition_id];

	memset(&partition->objlist[object_id], 0x00, sizeof(struct mtp_object));

	/* Reclaim the slot only when rolling back the tail entry */
	if (object_id == partition->files_count - 1) {
		partition->files_count--;
	}
}

static int dir_traverse(struct mtp_context *ctx, uint8_t partition_id, const char *root_path,
			uint8_t parent)
{
	char path[MAX_OBJPATH_LEN];
	struct fs_dir_t dir;
	int err;

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, root_path);
	if (err) {
		LOG_ERR("Unable to open %s (err %d)", root_path, err);
		return -ENOEXEC;
	}

	while (1) {
		struct fs_dirent entry;
		int path_len;

		err = fs_readdir(&dir, &entry);
		if (err) {
			LOG_ERR("Unable to read directory");
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

#if defined(CONFIG_USBD_MTP_DISABLE_DIRECTORIES)
		if (entry.type == FS_DIR_ENTRY_DIR) {
			/* Skip directories when directory support is disabled */
			continue;
		}
#endif

		size_t name_len = strnlen(entry.name, sizeof(entry.name));

		if (name_len >= MTP_MAX_FILE_NAME_LEN) {
			LOG_WRN("Skipping %s, File name is too long", entry.name);
			continue;
		}

		/* Build the full path of the file or directory */
		path_len = construct_path(path, sizeof(path), root_path, entry.name);
		if (path_len < 0 || path_len >= (int)sizeof(path)) {
			LOG_WRN("Skipping %s, path is too long", entry.name);
			continue;
		}

		struct mtp_object *obj =
			add_object(ctx, partition_id, parent, entry.type, entry.name, entry.size);

		if (obj == NULL) {
			LOG_ERR("Max file count reached, cannot store more paths.");
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			/* Recursive call to traverse subdirectory */
			err = dir_traverse(ctx, partition_id, path, obj->handle.object_id);
			if (err) {
				LOG_ERR("Failed to traverse %s", path);
				break;
			}
		}
	}

	fs_closedir(&dir);

	return err;
}

#if !defined(CONFIG_USBD_MTP_DISABLE_DIRECTORIES)
static int dir_delete(char *dirpath)
{
	struct fs_dir_t dir;
	int err;
	int ret;
	char objpath[MAX_OBJPATH_LEN];
	bool partial = false;

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, dirpath);
	if (err) {
		LOG_ERR("Unable to open %s (err %d)", dirpath, err);
		return err;
	}

	while (1) {
		struct fs_dirent entry;
		int path_len;

		ret = fs_readdir(&dir, &entry);
		if (ret) {
			LOG_ERR("Unable to read directory %s (err %d)", dirpath, ret);
			partial = true;
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		/* Build the full path of the file or directory */
		path_len = construct_path(objpath, sizeof(objpath), dirpath, entry.name);
		if (path_len < 0 || path_len >= (int)sizeof(objpath)) {
			LOG_ERR("Path too long for %s/%s", dirpath, entry.name);
			partial = true;
			continue;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			ret = dir_delete(objpath);
		} else {
			ret = fs_unlink(objpath);
		}

		if (ret) {
			LOG_ERR("Failed to delete %s (err %d)", objpath, ret);
			partial = true;
		}
	}

	fs_closedir(&dir);

	if (partial) {
		return -ECANCELED;
	}

	ret = fs_unlink(dirpath);
	if (ret) {
		LOG_ERR("Failed to remove directory %s (err %d)", dirpath, ret);
		return ret;
	}

	return 0;
}
#endif /* !CONFIG_USBD_MTP_DISABLE_DIRECTORIES */

static uint32_t get_child_objects_handles(struct mtp_context *ctx, uint8_t partition_id,
					  uint8_t parent_id, struct net_buf *buf)
{
	struct mtp_partition *partition = &ctx->partitions[partition_id];
	uint32_t found_files = 0;

	/* Skip the ROOT object */
	for (uint8_t i = 1; i < partition->files_count; ++i) {
		if (partition->objlist[i].name[0] == '\0') {
			/* to skip a deleted object */
			continue;
		}
		if (partition->objlist[i].handle.parent_id == parent_id) {
			__ASSERT_NO_MSG(net_buf_tailroom(buf) >= sizeof(uint32_t));
			LOG_DBG("Found %s in partition %d", partition->objlist[i].name,
				partition_id);
			net_buf_add_le32(buf, partition->objlist[i].handle.value);
			found_files++;
		}
	}

	return found_files;
}

static void transfer_state_reset(struct mtp_context *ctx)
{
	if (ctx->transfer_state.filepath[0] != '\0') {
		fs_close(&ctx->transfer_state.file);
	}
	memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
}

static void discard_incomplete_transfer(struct mtp_context *ctx)
{
	if (strnlen(ctx->transfer_state.filepath, MAX_OBJPATH_LEN) == 0) {
		return;
	}

	if (ctx->op_state.code == MTP_OP_SEND_OBJECT) {
		uint8_t part_id = ctx->transfer_state.partition_id;

		fs_close(&ctx->transfer_state.file);
		fs_unlink(ctx->transfer_state.filepath);

		if (part_id > 0 && part_id < ctx->partitions_count) {
			uint8_t obj_id = (ctx->partitions[part_id].files_count - 1U);

			remove_object(ctx, part_id, obj_id);
		}

		memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		return;
	}

	transfer_state_reset(ctx);
}

static int transfer_state_init(struct mtp_context *ctx, struct mtp_object *obj,
			       fs_mode_t open_flags, uint32_t total_size)
{
	int err;

	err = traverse_path(ctx, obj, ctx->transfer_state.filepath);
	if (err < 0) {
		return err;
	}

	fs_file_t_init(&ctx->transfer_state.file);
	err = fs_open(&ctx->transfer_state.file, ctx->transfer_state.filepath, open_flags);
	if (err) {
		return err;
	}
	ctx->transfer_state.partition_id = obj->handle.partition_id;
	ctx->transfer_state.total_size = total_size;
	ctx->transfer_state.transferred = 0;
	ctx->transfer_state.chunks_sent = 0;

	memset(ctx->filebuf, 0, sizeof(ctx->filebuf));
	return 0;
}

MTP_CMD_HANDLER(MTP_OP_GET_DEVICE_INFO)
{
	struct mtp_device_info *dev_info = &ctx->dev_info;

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, MTP_HEADER_SIZE);

	/* Device Info */
	net_buf_add_le16(buf, 100); /* standard_version = MTP version 1.00 */
	net_buf_add_le32(buf, 6); /* vendor_extension_id = MTP standard extension ID (Microsoft) */
	net_buf_add_le16(buf, 100); /* vendor_extension_version */

	/* No Vendor extension is supported */
	net_buf_add_u8(buf, 0);

	/* functional_mode */
	net_buf_add_le16(buf, 0);

	/* operations supported */
	mtp_buf_add_u16_array(buf, mtp_operations, ARRAY_SIZE(mtp_operations));

	/* events supported */
	net_buf_add_le32(buf, 0);

	/* Device properties supported */
	net_buf_add_le32(buf, 0); /* no props are used */

	/* Capture formats count */
	net_buf_add_le32(buf, 0);

	/* Playback formats supported */
	mtp_buf_add_u16_array(buf, playback_formats, ARRAY_SIZE(playback_formats));

	mtp_buf_add_string(buf, dev_info->manufacturer); /* manufacturer[] */

	mtp_buf_add_string(buf, dev_info->model); /* model[] */

	mtp_buf_add_string(buf, dev_info->device_version); /* device_version[] */

	mtp_buf_add_string(buf, dev_info->serial_number); /* serial_number[] */

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_OPEN_SESSION)
{
	uint16_t err_code = MTP_RESP_OK;

	if (ctx->session_opened) {
		LOG_ERR("Session already opened");
		uint32_t params[1] = {ctx->session_id};

		send_response_with_params(ctx, buf, MTP_RESP_SESSION_ALREADY_OPEN, params, 1);
		return;
	}

	if (mtp_command->param[0] == 0) {
		LOG_ERR("Invalid session ID 0");
		send_response_code(ctx, buf, MTP_RESP_INVALID_PARAMETER);
		return;
	}

	ctx->session_id = mtp_command->param[0];
	for (int i = 1; i < ctx->partitions_count; i++) {
		if (dir_traverse(ctx, i, ctx->partitions[i].mountpoint,
				 MTP_ROOT_OBJ_HANDLE)) {
			LOG_ERR("Failed to traverse %s", ctx->partitions[i].mountpoint);
			err_code = MTP_RESP_GENERAL_ERROR;
			break;
		}
	}

	if (err_code == MTP_RESP_OK) {
		LOG_DBG("Session opened successfully (SessionID: %u)", ctx->session_id);
		ctx->session_opened = true;
	}

	send_response_code(ctx, buf, err_code);
}

MTP_CMD_HANDLER(MTP_OP_CLOSE_SESSION)
{
	mtp_reset(ctx);
	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_IDS)
{
	set_mtp_phase(ctx, MTP_PHASE_DATA);
	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, MTP_HEADER_SIZE);

	net_buf_add_le32(buf, (ctx->partitions_count - 1U)); /* Number of stores */
	for (int i = 1; i < ctx->partitions_count; i++) {
		__ASSERT_NO_MSG(net_buf_tailroom(buf) >= sizeof(uint32_t));
		/* Use array index as Storage ID, 0x00 can't be used */
		net_buf_add_le32(buf, GENERATE_STORAGE_ID(i));
	}

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_INFO)
{
	union mtp_storage_id storage_id = decode_storage_id(mtp_command->param[0]);
	uint32_t partition_id = storage_id.id;

	MTP_PARAMS_DEBUG(mtp_command, 1, "StorageID");

	if (partition_id == 0 || partition_id >= ctx->partitions_count) {
		LOG_ERR("Unknown partition ID %x", partition_id);
		send_response_code(ctx, buf, MTP_RESP_INVALID_STORAGE_ID);
		return;
	}

	struct fs_statvfs stat;
	int err = fs_statvfs(ctx->partitions[partition_id].mountpoint, &stat);

	if (err < 0) {
		LOG_ERR("Failed to statvfs %s (%d)", ctx->partitions[partition_id].mountpoint, err);
		send_response_code(ctx, buf, MTP_RESP_GENERAL_ERROR);
		return;
	}

	const char *storage_name = ctx->partitions[partition_id].mountpoint;

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	if (storage_name[0] == '/') {
		/* skip the slash */
		storage_name++;
	}

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, MTP_HEADER_SIZE);

	net_buf_add_le16(buf, STORAGE_TYPE_FIXED_RAM);       /* type */
	net_buf_add_le16(buf, FS_TYPE_GENERIC_HIERARCHICAL); /* fs_type */
	if (ctx->partitions[partition_id].read_only) {
		net_buf_add_le16(buf, OBJECT_PROTECTION_READ_ONLY); /* access_caps */
	} else {
		net_buf_add_le16(buf, OBJECT_PROTECTION_NO);
	}
	net_buf_add_le64(buf, fs_size_bytes(stat.f_blocks, stat.f_frsize)); /* max_capacity */
	net_buf_add_le64(buf, fs_size_bytes(stat.f_bfree, stat.f_frsize));  /* free_space */
	net_buf_add_le32(buf, MTP_FREE_SPACE_OBJ_UNUSED);                   /* free_space_obj */
	mtp_buf_add_string(buf, storage_name);                              /* storage_desc[] */
	net_buf_add_u8(buf, 0); /* volume_id_len, Unused */

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_HANDLES)
{
	union mtp_storage_id storage_id = decode_storage_id(mtp_command->param[0]);
	uint32_t partition_id = storage_id.id;
	uint32_t obj_format_code = mtp_command->param[1];
	union mtp_object_handle obj_handle = decode_object_handle(mtp_command->param[2]);

	/* ObjHandle: Parent object handle for which child objects are requested */
	MTP_PARAMS_DEBUG(mtp_command, 3, "StorageID", "ObjFormatCode", "ObjHandle");

	uint32_t found_files = 0;
	uint8_t parent_id = (obj_handle.value == MTP_ALLROOTOBJECTS ? MTP_ROOT_OBJ_HANDLE
								    : obj_handle.object_id);

	if (obj_format_code) {
		send_response_code(ctx, buf, MTP_RESP_SPECIFICATION_BY_FORMAT_UNSUPPORTED);
		return;
	}

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, MTP_HEADER_SIZE + sizeof(uint32_t));

	/* Host wants all root objects on all partitions */
	if (storage_id.value == MTP_ALLSTORAGES) {
		for (int part_idx = 1; part_idx < ctx->partitions_count; ++part_idx) {
			found_files += get_child_objects_handles(ctx, part_idx, parent_id, buf);
		}
	} else {
		if (partition_id >= ctx->partitions_count || partition_id == 0) {
			LOG_ERR("Unknown partition ID %x", partition_id);
			send_response_code(ctx, buf, MTP_RESP_INVALID_STORAGE_ID);
			return;
		}

		found_files = get_child_objects_handles(ctx, partition_id, parent_id, buf);
	}

	net_buf_push_le32(buf, found_files);
	mtp_buf_push_data_header(ctx, buf, buf->len);

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_INFO)
{
	union mtp_object_handle obj_handle = decode_object_handle(mtp_command->param[0]);
	uint8_t partition_id = obj_handle.partition_id;
	uint8_t object_id = obj_handle.object_id;
	uint16_t err_code = MTP_RESP_OK;

	MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");

	err_code = validate_object_handle(ctx, obj_handle);
	if (err_code != MTP_RESP_OK) {
		send_response_code(ctx, buf, err_code);
		return;
	}

	if (ctx->partitions[partition_id].objlist[object_id].handle.value != obj_handle.value) {
		LOG_ERR("Unknown ID %08x", obj_handle.value);
		send_response_code(ctx, buf, MTP_RESP_INVALID_OBJECT_HANDLE);
		return;
	}

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, MTP_HEADER_SIZE);

	char *filename = ctx->partitions[partition_id].objlist[object_id].name;

	net_buf_add_le32(buf, GENERATE_STORAGE_ID(partition_id)); /* StorageID */

	/* ObjectFormat */
	if (ctx->partitions[partition_id].objlist[object_id].handle.type ==
	    FS_DIR_ENTRY_DIR) {
		net_buf_add_le16(buf, MTP_FORMAT_ASSOCIATION);
	} else {
		net_buf_add_le16(buf, MTP_FORMAT_UNDEFINED);
	}

	/* ProtectionStatus */
	if (ctx->partitions[partition_id].read_only) {
		net_buf_add_le16(buf, OBJECT_PROTECTION_READ_ONLY);
	} else {
		net_buf_add_le16(buf, OBJECT_PROTECTION_NO);
	}

	/* Object Size */
	if (ctx->partitions[partition_id].objlist[object_id].handle.type ==
	    FS_DIR_ENTRY_DIR) {
		net_buf_add_le32(buf, MTP_ASSOCIATION_SIZE);
	} else {
		net_buf_add_le32(buf,
				 ctx->partitions[partition_id].objlist[object_id].size);
	}
	net_buf_add_le16(buf, 0); /* ThumbFormat */
	net_buf_add_le32(buf, 0); /* ThumbCompressedSize */
	net_buf_add_le32(buf, 0); /* ThumbPixWidth */
	net_buf_add_le32(buf, 0); /* ThumbPixHeight */
	net_buf_add_le32(buf, 0); /* ImagePixWidth */
	net_buf_add_le32(buf, 0); /* ImagePixHeight */
	net_buf_add_le32(buf, 0); /* ImageBitDepth */

	uint8_t parent_id =
		ctx->partitions[partition_id].objlist[object_id].handle.parent_id;
	union mtp_object_handle parent_handle =
		ctx->partitions[partition_id].objlist[parent_id].handle;

	net_buf_add_le32(buf, parent_handle.value); /* ParentObject */

	LOG_DBG("%s in %s %x", ctx->partitions[partition_id].objlist[object_id].name,
		parent_handle.value ? "parent" : "Root", parent_handle.value);

	/* AssociationType */
	if (ctx->partitions[partition_id].objlist[object_id].handle.type ==
	    FS_DIR_ENTRY_DIR) {
		net_buf_add_le16(buf, MTP_ASSOCIATION_TYPE_GENERIC);
	} else {
		net_buf_add_le16(buf, MTP_ASSOCIATION_TYPE_UNDEFINED);
	}

	net_buf_add_le32(buf, 0); /* AssociationDesc */
	net_buf_add_le32(buf, 0); /* SequenceNumber */

	mtp_buf_add_string(buf, filename); /* FileName */
	mtp_buf_add_string(buf, NULL);     /* DateCreated */
	mtp_buf_add_string(buf, NULL);     /* DateModified */
	net_buf_add_u8(buf, 0);            /* KeywordsLength, always 0 unused */

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	LOG_DBG("Object Info: %s, Size: %u",
		ctx->partitions[partition_id].objlist[object_id].name,
		ctx->partitions[partition_id].objlist[object_id].size);

	set_mtp_phase(ctx, MTP_PHASE_DATA);
	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT)
{
	union mtp_object_handle obj_handle;
	uint8_t partition_id;
	uint8_t object_id;
	uint32_t available_buf_len = ctx->max_packet_size;
	uint32_t data_len = 0;
	int err = 0;

	if (ctx->phase == MTP_PHASE_REQUEST) {
		/* Reserve space for MTP header at the beginning of the buffer */
		net_buf_reserve(buf, MTP_HEADER_SIZE);

		MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");
		obj_handle = decode_object_handle(mtp_command->param[0]);
		partition_id = obj_handle.partition_id;
		object_id = obj_handle.object_id;

		err = validate_object_handle(ctx, obj_handle);
		if (err != MTP_RESP_OK) {
			send_response_code(ctx, buf, err);
			return;
		}

		err = transfer_state_init(ctx, &ctx->partitions[partition_id].objlist[object_id],
					  FS_O_READ,
					  ctx->partitions[partition_id].objlist[object_id].size);
		if (err) {
			LOG_ERR("Failed to open %s (%d)", ctx->transfer_state.filepath, err);
			send_response_code(ctx, buf, MTP_RESP_ACCESS_DENIED);
			return;
		}

		available_buf_len = ctx->max_packet_size - MTP_HEADER_SIZE;

		mtp_buf_push_data_header(ctx, buf, ctx->transfer_state.total_size);

		set_mtp_phase(ctx, MTP_PHASE_DATA);
		LOG_DBG("Traversed Path: %s (Size: %u)", ctx->transfer_state.filepath,
			ctx->transfer_state.total_size);
	}

	if (ctx->phase != MTP_PHASE_DATA) {
		LOG_ERR("Invalid phase %d, expected %d", ctx->phase, MTP_PHASE_DATA);
		send_response_code(ctx, buf, MTP_RESP_OPERATION_NOT_SUPPORTED);
		return;
	}

	LOG_DBG("Sending file: %s size: %u [max: %u]", ctx->transfer_state.filepath,
		ctx->transfer_state.total_size, available_buf_len);

	data_len = MIN(available_buf_len,
		       (ctx->transfer_state.total_size - ctx->transfer_state.transferred));

	/* An empty object carries no data phase payload; only the header is sent */
	if (data_len > 0) {
		ssize_t read = fs_read(&ctx->transfer_state.file, ctx->filebuf, data_len);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %zd", read);
			transfer_state_reset(ctx);
			send_response_code(ctx, buf, MTP_RESP_GENERAL_ERROR);
			return;
		}
		net_buf_add_mem(buf, ctx->filebuf, (size_t)read);

		ctx->transfer_state.transferred += read;
		ctx->transfer_state.chunks_sent++;

		LOG_DBG("Sent chunk: %u [%u of %u], %u bytes remaining",
			ctx->transfer_state.chunks_sent, ctx->transfer_state.transferred,
			ctx->transfer_state.total_size,
			(ctx->transfer_state.total_size - ctx->transfer_state.transferred));
	}

	if (ctx->transfer_state.transferred >= ctx->transfer_state.total_size) {
		LOG_DBG("Done, Sending Response");
		transfer_state_reset(ctx);
		send_response_code(ctx, buf, MTP_RESP_OK);
	}
}

#if !defined(CONFIG_USBD_MTP_DISABLE_WRITE_ACCESS)
MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT_INFO)
{
	uint32_t dest_partition_id = 0;
	uint32_t dest_parent_id = 0;
	uint16_t object_format = 0;
	uint8_t filename_len = 0;
	int ret = 0;
	uint16_t err_code = MTP_RESP_OK;
	struct mtp_object *new_obj = NULL;
	struct fs_statvfs fs_stat;
	char obj_name[MAX_FILE_NAME + 1];
	uint32_t obj_size = 0;
	enum fs_dir_entry_type obj_type = FS_DIR_ENTRY_FILE;

	/* Process the request only after receiving the 2 packets.
	 * First packet: contains only destination storageID, ParentID.
	 * Second packet: contains the rest of the info.
	 * Reply to host only after the second packet
	 */
	if (ctx->phase == MTP_PHASE_REQUEST) {
		MTP_PARAMS_DEBUG(mtp_command, 2, "StorageID", "ParentObjHandle");

		/* Store the arguments for DATA_PHASE use */
		ctx->op_state.args[0] = mtp_command->param[0];
		ctx->op_state.args[1] = mtp_command->param[1];

		/* Clear transfer state */
		transfer_state_reset(ctx);
		set_mtp_phase(ctx, MTP_PHASE_DATA);
		return;
	}

	dest_partition_id = decode_storage_id(ctx->op_state.args[0]).id;
	dest_parent_id = decode_object_handle(ctx->op_state.args[1]).value;

	/* ParentID should be 0x00 when asked to store the new object at Root of the store */
	if (dest_parent_id == MTP_STORE_ROOT) {
		dest_parent_id = MTP_ROOT_OBJ_HANDLE;
	} else {
		dest_parent_id = decode_object_handle(ctx->op_state.args[1]).object_id;
	}

	/* Initial checks */
	if (ctx->phase != MTP_PHASE_DATA) {
		LOG_ERR("Invalid phase %d, expected %d", ctx->phase, MTP_PHASE_DATA);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	if (dest_partition_id == 0 || dest_partition_id >= ctx->partitions_count) {
		LOG_ERR("Unknown partition id %x", dest_partition_id);
		err_code = MTP_RESP_INVALID_STORAGE_ID;
		goto exit;
	}

	if (ctx->partitions[dest_partition_id].read_only) {
		LOG_ERR("Storage %u is read-only", dest_partition_id);
		err_code = MTP_RESP_STORE_READ_ONLY;
		goto exit;
	}

	if (dest_parent_id != MTP_ROOT_OBJ_HANDLE) {
		if (dest_parent_id >= ctx->partitions[dest_partition_id].files_count ||
		    ctx->partitions[dest_partition_id].objlist[dest_parent_id].name[0] == '\0' ||
		    ctx->partitions[dest_partition_id].objlist[dest_parent_id].handle.type !=
			    FS_DIR_ENTRY_DIR) {
			LOG_ERR("Invalid parent handle 0x%08x", ctx->op_state.args[1]);
			err_code = MTP_RESP_INVALID_PARENT_OBJECT;
			goto exit;
		}
	}

	/* Parse the ObjectInfo dataset from payload into locals first, then allocate */
	memset(obj_name, 0x00, sizeof(obj_name));

	if (payload->len < MTP_HEADER_SIZE + MTP_OBJ_INFO_FIXED_FIELDS_SIZE) {
		LOG_ERR("ObjectInfo payload too short: %u", payload->len);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	net_buf_pull(payload, MTP_HEADER_SIZE);     /* SKIP the header */
	net_buf_pull_le32(payload);                 /* StorageID, always 0 ignore */
	object_format = net_buf_pull_le16(payload); /* ObjectFormat */

	obj_type = (object_format == MTP_FORMAT_ASSOCIATION) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;

#if defined(CONFIG_USBD_MTP_DISABLE_DIRECTORIES)
	if (obj_type == FS_DIR_ENTRY_DIR) {
		LOG_WRN("Directory creation rejected: directory support is disabled");
		err_code = MTP_RESP_OPERATION_NOT_SUPPORTED;
		goto exit;
	}
#endif

	net_buf_pull_le16(payload);            /* ProtectionStatus */
	obj_size = net_buf_pull_le32(payload); /* ObjectCompressedSize */
	net_buf_pull_le16(payload);            /* ThumbFormat */
	net_buf_pull_le32(payload);            /* ThumbCompressedSize */
	net_buf_pull_le32(payload);            /* ThumbPixWidth */
	net_buf_pull_le32(payload);            /* ThumbPixHeight */
	net_buf_pull_le32(payload);            /* ImagePixWidth */
	net_buf_pull_le32(payload);            /* ImagePixHeight */
	net_buf_pull_le32(payload);            /* ImageBitDepth */
	net_buf_pull_le32(payload);            /* ParentObject (always 0, Ignore) */
	net_buf_pull_le16(payload);            /* AssociationType */
	net_buf_pull_le32(payload);            /* AssociationDesc */
	net_buf_pull_le32(payload);            /* SequenceNumber */

	filename_len = net_buf_pull_u8(payload); /* FileNameLength */
	if (filename_len <= 1 || filename_len > MTP_MAX_FILE_NAME_LEN) {
		LOG_ERR("invalid filename length %u", filename_len);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	if (payload->len < (filename_len * sizeof(uint16_t))) {
		LOG_ERR("ObjectInfo FileName field is truncated");
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	usb_buf_pull_utf16le(payload, obj_name, filename_len); /* FileName */
	if (obj_name[filename_len - 1] != '\0') {
		LOG_ERR("filename is not NULL terminated");
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}
	/* Rest of props are ignored */

	ret = fs_statvfs(ctx->partitions[dest_partition_id].mountpoint, &fs_stat);
	if (ret < 0) {
		LOG_ERR("Failed to statvfs %s (%d)", ctx->partitions[dest_partition_id].mountpoint,
			ret);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	uint64_t free_space = fs_size_bytes(fs_stat.f_bfree, fs_stat.f_frsize);

	if (obj_size > free_space) {
		LOG_ERR("Not enough space to store file %u > %llu", obj_size,
			(unsigned long long)free_space);
		err_code = MTP_RESP_STORAGE_FULL;
		goto exit;
	}

	new_obj = add_object(ctx, dest_partition_id, dest_parent_id, obj_type, obj_name, obj_size);
	if (new_obj == NULL) {
		err_code = MTP_RESP_STORAGE_FULL;
		goto exit;
	}

	LOG_DBG("New ObjHandle: 0x%08x (PartID=%u, ParentID=%u, ObjID=%u)", new_obj->handle.value,
		new_obj->handle.partition_id, new_obj->handle.parent_id, new_obj->handle.object_id);

	if (new_obj->handle.type == FS_DIR_ENTRY_DIR) {
		ret = traverse_path(ctx, new_obj, ctx->transfer_state.filepath);
		if (ret < 0) {
			LOG_ERR("Failed to construct directory path (%d)", ret);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto exit;
		}

		ret = fs_mkdir(ctx->transfer_state.filepath);
		if (ret) {
			LOG_ERR("Failed to create directory %s (%d)", ctx->transfer_state.filepath,
				ret);
			err_code = MTP_RESP_GENERAL_ERROR;
		}
	} else {
		ret = transfer_state_init(ctx, new_obj, FS_O_CREATE | FS_O_WRITE, new_obj->size);
		if (ret) {
			LOG_ERR("Open file failed, %d", ret);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto exit;
		}
	}

	LOG_DBG("\n ObjFormat: %x, size: %u, parent: %x\n mnt: %s\n fname: %s\n path: %s "
		"Handle:%x\n parentID: %u",
		object_format, new_obj->size, dest_parent_id,
		ctx->partitions[new_obj->handle.partition_id].mountpoint, new_obj->name,
		ctx->transfer_state.filepath, new_obj->handle.value, new_obj->handle.parent_id);

exit:
	set_mtp_phase(ctx, MTP_PHASE_RESPONSE);

	if (new_obj && err_code == MTP_RESP_OK) {
		uint32_t params[3] = {GENERATE_STORAGE_ID(new_obj->handle.partition_id),
				      GET_PARENT_HANDLE(ctx, new_obj), new_obj->handle.value};

		send_response_with_params(ctx, buf, err_code, params, 3);

		LOG_DBG("Sent info:\n\tSID: %x\n\tPID: %x\n\tOID: %x", params[0], params[1],
			params[2]);
	} else {
		if (new_obj) {
			transfer_state_reset(ctx);
			remove_object(ctx, dest_partition_id, new_obj->handle.object_id);
		}
		send_response_code(ctx, buf, err_code);
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT)
{
	ssize_t ret = 0;

	if (ctx->phase == MTP_PHASE_REQUEST) {

		/* Per MTP spec §11.2: SendObject MUST follow a successful SendObjectInfo */
		if (ctx->transfer_state.filepath[0] == '\0') {
			LOG_ERR("SendObject without prior SendObjectInfo");
			send_response_code(ctx, buf, MTP_RESP_NO_VALID_OBJECT_INFO);
			return;
		}

		if (mtp_command->hdr.type == MTP_CONTAINER_COMMAND) {
			LOG_DBG("COMMAND RECEIVED len: %u", payload->len);
			set_mtp_phase(ctx, MTP_PHASE_DATA);
			return;
		}
	}

	if (ctx->transfer_state.transferred == 0) {
		/* First data packet contains the header */
		LOG_DBG("DATA RECEIVED len: %u", payload->len);
		if (payload->len < MTP_HEADER_SIZE) {
			LOG_ERR("SEND_OBJECT: first data packet too short (%u)", payload->len);
			send_response_code(ctx, buf, MTP_RESP_GENERAL_ERROR);
			return;
		}
		net_buf_pull_mem(payload, MTP_HEADER_SIZE);
	}

	/* Cap write to the remaining declared object size to prevent over-writes */
	uint32_t remaining = ctx->transfer_state.total_size - ctx->transfer_state.transferred;
	uint32_t write_len = MIN((uint32_t)payload->len, remaining);

	ret = fs_write(&ctx->transfer_state.file, payload->data, write_len);
	if (ret < 0) {
		LOG_ERR("Failed to write data to file %s (%zd)", ctx->transfer_state.filepath, ret);
		send_response_code(ctx, buf, MTP_RESP_STORE_NOT_AVAILABLE);
		return;
	}

	if ((uint32_t)ret < write_len) {
		LOG_ERR("Short write on %s (%zd of %u bytes)", ctx->transfer_state.filepath, ret,
			write_len);
		send_response_code(ctx, buf, MTP_RESP_STORAGE_FULL);
		return;
	}

	ctx->transfer_state.chunks_sent++;
	ctx->transfer_state.transferred += (uint32_t)ret;
	LOG_DBG("SEND_OBJECT: Data len: %u out of %u", ctx->transfer_state.transferred,
		ctx->transfer_state.total_size);

	if (ctx->transfer_state.transferred >= ctx->transfer_state.total_size) {
		LOG_DBG("SEND_OBJECT: All data received (%u bytes), Sending Response",
			ctx->transfer_state.transferred);
		transfer_state_reset(ctx);
		set_mtp_phase(ctx, MTP_PHASE_RESPONSE);
		send_response_code(ctx, buf, MTP_RESP_OK);
	} else {
		set_mtp_phase(ctx, MTP_PHASE_DATA);
	}
}

MTP_CMD_HANDLER(MTP_OP_DELETE_OBJECT)
{
	union mtp_object_handle obj_handle = decode_object_handle(mtp_command->param[0]);
	uint8_t partition_id = obj_handle.partition_id;
	uint8_t object_id = obj_handle.object_id;
	uint16_t err_code = MTP_RESP_OK;
	int ret = 0;

	char path[MAX_OBJPATH_LEN];

	MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");

	err_code = validate_object_handle(ctx, obj_handle);
	if (err_code != MTP_RESP_OK) {
		send_response_code(ctx, buf, err_code);
		return;
	}

	if (obj_handle.value == MTP_ALLROOTOBJECTS) {
		LOG_ERR("Invalid object handle 0x%08x", obj_handle.value);
		send_response_code(ctx, buf, MTP_RESP_INVALID_OBJECT_HANDLE);
		return;
	}

	memset(path, 0x00, MAX_OBJPATH_LEN);
	ret = traverse_path(ctx, &ctx->partitions[partition_id].objlist[object_id], path);
	if (ret < 0) {
		LOG_ERR("Failed to construct object path (%d)", ret);
		send_response_code(ctx, buf, MTP_RESP_GENERAL_ERROR);
		return;
	}

	LOG_DBG("Traversed Path: %s", path);

	if (!ctx->partitions[partition_id].read_only) {
#if defined(CONFIG_USBD_MTP_DISABLE_DIRECTORIES)
		if (obj_handle.type == FS_DIR_ENTRY_DIR) {
			LOG_WRN("Directory deletion rejected: directory support is "
				"disabled");
			send_response_code(ctx, buf, MTP_RESP_OPERATION_NOT_SUPPORTED);
			return;
		}
#else
		if (obj_handle.type == FS_DIR_ENTRY_DIR) {
			LOG_DBG("Deleting directory %s", path);
			ret = dir_delete(path);
			if (ret == -ECANCELED) {
				/* do not fall through to generic error path */
				send_response_code(ctx, buf, MTP_RESP_PARTIAL_DELETION);
				return;
			}
		}
#endif
		else {
			LOG_DBG("Deleting file %s", path);
			ret = fs_unlink(path);
		}

		if (ret) {
			LOG_ERR("Failed to delete %s (%d)", path, ret);
			err_code = MTP_RESP_ACCESS_DENIED;
		} else {
			remove_object(ctx, partition_id, object_id);
		}
	} else {
		LOG_WRN("Read only partition %u", partition_id);
		err_code = MTP_RESP_STORE_READ_ONLY;
	}

	send_response_code(ctx, buf, err_code);
}
#endif /* !CONFIG_USBD_MTP_DISABLE_WRITE_ACCESS */

/* Check if the MTP context is in a state where it awaits more data or have a pending packet to be
 * sent to host.
 */
bool mtp_packet_pending(struct mtp_context *ctx)
{
	return (ctx->phase != MTP_PHASE_REQUEST);
}

/*
 * returns the length of the buffer to be sent on success
 *	   or -EINVAL on error
 *	   or 0 on no data to send
 */
int mtp_commands_handler(struct mtp_context *ctx, struct net_buf *buf_in, struct net_buf *buf_resp)
{
	struct mtp_container *mtp_command = NULL;
	struct net_buf *payload = buf_in;

	if (buf_resp == NULL) {
		LOG_ERR("%s: NULL Buffer", __func__);
		return -EINVAL;
	}

	switch (ctx->phase) {
	case MTP_PHASE_REQUEST:
		if (buf_in == NULL) {
			LOG_ERR("%s: NULL Buffer", __func__);
			return -EINVAL;
		}

		mtp_command = (struct mtp_container *)buf_in->data;

		if (ctx->session_opened) {
			if (mtp_command->hdr.transaction_id == 0 ||
			    mtp_command->hdr.transaction_id != ctx->transaction_id + 1) {
				LOG_ERR("Invalid TID: expected %u got %u", ctx->transaction_id + 1,
					mtp_command->hdr.transaction_id);
				send_response_code(ctx, buf_resp, MTP_RESP_INVALID_TRANSACTION_ID);
				return buf_resp->len;
			}
		}

		ctx->op_state.code = mtp_command->hdr.code;
		ctx->transaction_id = mtp_command->hdr.transaction_id;

		if (!ctx->session_opened) {
			if (mtp_command->hdr.code != MTP_OP_OPEN_SESSION &&
			    mtp_command->hdr.code != MTP_OP_GET_DEVICE_INFO) {
				LOG_ERR("MTP Session is not opened!, command rejected");
				send_response_code(ctx, buf_resp, MTP_RESP_SESSION_NOT_OPEN);
				return buf_resp->len;
			}
		}

		LOG_DBG("Phase [REQUEST], New Op request code: 0x%x", mtp_command->hdr.code);
		break;
	case MTP_PHASE_DATA:
		LOG_DBG("Phase [DATA]: Continue Op code: 0x%x", ctx->op_state.code);
		if (ctx->op_state.code != MTP_OP_SEND_OBJECT &&
		    ctx->op_state.code != MTP_OP_SEND_OBJECT_INFO &&
		    ctx->op_state.code != MTP_OP_GET_OBJECT) {
			LOG_ERR("Invalid phase %d for op_code %d", ctx->phase, ctx->op_state.code);
			send_response_code(ctx, buf_resp, MTP_RESP_INCOMPLETE_TRANSFER);
			return buf_resp->len;
		}
		break;
	case MTP_PHASE_RESPONSE:
		send_response_code(ctx, buf_resp, ctx->op_state.err);
		LOG_DBG("Phase [RESPONSE]: Response sent for Op code 0x%x", ctx->op_state.code);
		return buf_resp->len;
	case MTP_PHASE_CANCELED:
		LOG_ERR("Unexpected cmd while an operation cancelling is in progress");
		return -EBUSY;
	default:
		__ASSERT(0, "Invalid phase %d for op_code %d", ctx->phase, ctx->op_state.code);
	}

	LOG_DBG("%u: [%s]", ctx->transaction_id, mtp_code_to_string(ctx->op_state.code));

	switch (ctx->op_state.code) {
	case MTP_OP_GET_DEVICE_INFO:
		MTP_CMD(MTP_OP_GET_DEVICE_INFO);
		break;
	case MTP_OP_OPEN_SESSION:
		MTP_CMD(MTP_OP_OPEN_SESSION);
		break;
	case MTP_OP_CLOSE_SESSION:
		MTP_CMD(MTP_OP_CLOSE_SESSION);
		break;
	case MTP_OP_GET_STORAGE_IDS:
		MTP_CMD(MTP_OP_GET_STORAGE_IDS);
		break;
	case MTP_OP_GET_STORAGE_INFO:
		MTP_CMD(MTP_OP_GET_STORAGE_INFO);
		break;
	case MTP_OP_GET_OBJECT_HANDLES:
		MTP_CMD(MTP_OP_GET_OBJECT_HANDLES);
		break;
	case MTP_OP_GET_OBJECT_INFO:
		MTP_CMD(MTP_OP_GET_OBJECT_INFO);
		break;
	case MTP_OP_GET_OBJECT:
		MTP_CMD(MTP_OP_GET_OBJECT);
		break;
#if !defined(CONFIG_USBD_MTP_DISABLE_WRITE_ACCESS)
	case MTP_OP_DELETE_OBJECT:
		MTP_CMD(MTP_OP_DELETE_OBJECT);
		break;
	case MTP_OP_SEND_OBJECT_INFO:
		MTP_CMD(MTP_OP_SEND_OBJECT_INFO);
		break;
	case MTP_OP_SEND_OBJECT:
		MTP_CMD(MTP_OP_SEND_OBJECT);
		break;
#endif /* !CONFIG_USBD_MTP_DISABLE_WRITE_ACCESS */
	default:
		LOG_ERR("Not supported cmd 0x%x!", ctx->op_state.code);
		send_response_code(ctx, buf_resp, MTP_RESP_OPERATION_NOT_SUPPORTED);
		break;
	}

	return buf_resp->len;
}

int mtp_get_dev_status(struct mtp_context *ctx, struct net_buf *const buf)
{
	struct mtp_device_status_t dev_status;

	if (ctx->phase == MTP_PHASE_CANCELED) {
		LOG_DBG("Operation cancelled by Host, Sending Response");
		set_mtp_phase(ctx, MTP_PHASE_REQUEST);
		dev_status.len = sys_cpu_to_le16(sizeof(struct mtp_device_status_t));
		dev_status.code = sys_cpu_to_le16(MTP_RESP_TRANSACTION_CANCELLED);
		dev_status.ep_in = ctx->dev_status.ep_in;
		dev_status.ep_out = ctx->dev_status.ep_out;
	} else {
		LOG_DBG("Device status OK");
		dev_status.len = sys_cpu_to_le16(sizeof(uint16_t) * 2U);
		dev_status.code = sys_cpu_to_le16(MTP_RESP_OK);
	}

	net_buf_add_mem(buf, &dev_status, sys_le16_to_cpu(dev_status.len));

	return 0;
}

int mtp_cancel(struct mtp_context *ctx, const struct net_buf *const buf)
{
	ARG_UNUSED(buf);

	set_mtp_phase(ctx, MTP_PHASE_CANCELED);
	LOG_DBG("Operation cancelled by Host, Closing incomplete file %s",
		ctx->transfer_state.filepath);

	discard_incomplete_transfer(ctx);

	return 0;
}

int mtp_device_reset(struct mtp_context *ctx)
{
	LOG_INF("MTP device reset requested");
	mtp_reset(ctx);

	return 0;
}

void mtp_reset(struct mtp_context *ctx)
{
	LOG_DBG("%s", __func__);
	discard_incomplete_transfer(ctx);

	/* Clear objects list */
	for (size_t i = 1; i < ARRAY_SIZE(ctx->partitions); i++) {
		memset(ctx->partitions[i].objlist, 0, sizeof(ctx->partitions[i].objlist));
		ctx->partitions[i].files_count = 1; /* Reserve 0th obj for ROOT */
	}

	ctx->transaction_id = 0;
	ctx->session_id = 0;
	ctx->session_opened = false;
	ctx->phase = MTP_PHASE_REQUEST;
	memset(&ctx->op_state, 0, sizeof(ctx->op_state));
}

static int mtp_storages_init(struct mtp_context *ctx, const struct usbd_mtp_instance *config)
{
	if (ctx->initialized) {
		return 0;
	}

	/* Reserve slot 0 as the internal root partition; real storages start at index 1. */
	ctx->partitions[0].mountpoint = "NULL";
	ctx->partitions_count = 1;

	if (config == NULL) {
		LOG_ERR("No MTP instance configuration provided");
		return -ENODEV;
	}

	if (!mtp_serial_number_valid(config->serial_number)) {
		LOG_ERR("MTP serial number must be 32 hexadecimal characters");
		return -EINVAL;
	}

	if (config->storage_count == 0) {
		LOG_ERR("No MTP storage registered");
		return -ENODEV;
	}

	if (config->storage_count > CONFIG_USBD_MTP_STORAGES_PER_INSTANCE) {
		LOG_ERR("Too many MTP storages registered");
		return -ENOMEM;
	}

	ctx->dev_info = (struct mtp_device_info){
		.manufacturer = config->manufacturer,
		.model = config->product,
		.device_version = config->device_version,
		.serial_number = config->serial_number,
	};

	for (size_t i = 0; i < config->storage_count; i++) {
		const struct usbd_mtp_storage *storage = &config->storages[i];

		ctx->partitions[ctx->partitions_count] = (struct mtp_partition){
			.mountpoint = storage->mountpoint,
			.files_count = 1,
			.read_only = storage->read_only,
		};
		ctx->partitions_count++;
	}

	ctx->initialized = true;

	return 0;
}

int mtp_init(struct mtp_context *ctx, const struct usbd_mtp_instance *config)
{
	int ret;

	ret = mtp_storages_init(ctx, config);
	if (ret) {
		return ret;
	}

	mtp_reset(ctx);

	return 0;
}
