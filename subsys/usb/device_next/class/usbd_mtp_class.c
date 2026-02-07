/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <stdio.h>
#include <string.h>

#include <zephyr/net_buf.h>
#include <zephyr/devicetree.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
#include "usbd_mtp_class.h"

#define DT_DRV_COMPAT zephyr_mtp

LOG_MODULE_REGISTER(usb_mtp_class, CONFIG_USBD_MTP_LOG_LEVEL);

/* MTP Control Requests Codes */
#define MTP_REQUEST_CANCEL            0x64U
#define MTP_REQUEST_GET_DEVICE_STATUS 0x67U
#define MTP_REQUEST_DEVICE_RESET      0x66U

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
#define MAX_FILES                 CONFIG_USBD_MTP_MAX_HANDLES
#define MTP_ROOT_OBJ_HANDLE       0x00U
#define MTP_ALLROOTOBJECTS        0xFFFFFFFFU
#define MTP_ASSOSCIATION_SIZE     0xFFFFFFFFU
#define MTP_FREE_SPACE_OBJ_UNUSED 0xFFFFFFFFU
#define MTP_STORE_ROOT            0xFFFFFFFFU
#define MTP_ALLSTORAGES           0xFFFFFFFFU

#define STORAGE_TYPE_INTERNAL   0x00010000
#define GENERATE_STORAGE_ID(id) (STORAGE_TYPE_INTERNAL + id)
#define MTP_STR_LEN(str)        (strlen(str) + 1)

#define MTP_CMD(opcode) mtp_##opcode(ctx, mtp_command, payload, buf_resp)

#define MTP_CMD_HANDLER(opcode)                                                                    \
	static void mtp_##opcode(struct mtp_context *ctx, struct mtp_container *mtp_command,       \
				 struct net_buf *payload, struct net_buf *buf)
#define PROCESS_MTP_ENTRY(inst)                                                                    \
	{                                                                                          \
		.mountpoint = DT_PROP(DT_INST_PHANDLE(inst, fstab_entry), mount_point),            \
		.files_count = 1,                                                                  \
		.read_only = DT_PROP(DT_INST_PHANDLE(inst, fstab_entry), read_only),               \
	},

/* MTP Objects in root should have ParentHandle=0x00 */
#define GET_PARENT_HANDLE(obj)                                                                     \
	(obj->handle.parent_id == MTP_ROOT_OBJ_HANDLE ? MTP_ROOT_OBJ_HANDLE                        \
						      : partitions[obj->handle.partition_id]       \
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

typedef union {
	struct {
		uint16_t id;
		uint16_t type;
	};
	uint32_t value;
} mtp_storage_id_t;

typedef union {
	struct {
		uint8_t partition_id; /* ID of the partition containing the object */
		uint8_t parent_id;    /* ID of the parent object (0xFF for root) */
		uint8_t object_id;    /* Unique ID of the object within the storage */
		uint8_t type;         /* Type of the object (e.g., file, directory) */
	};
	uint32_t value;
} mtp_object_handle_t;

struct mtp_object_t {
	mtp_object_handle_t handle;
	uint32_t size;
	char name[MAX_FILE_NAME + 1];
};

struct partition_t {
	const char mountpoint[10];
	struct mtp_object_t objlist[CONFIG_USBD_MTP_MAX_HANDLES];
	uint16_t files_count;
	bool read_only;
};

struct dev_info_t {
	const char *manufacturer;
	const char *model;
	const char *device_version;
	const char *serial_number;
};

/* Constants */
static const uint16_t mtp_operations[] = {
	MTP_OP_GET_DEVICE_INFO,  MTP_OP_OPEN_SESSION,     MTP_OP_CLOSE_SESSION,
	MTP_OP_GET_STORAGE_IDS,  MTP_OP_GET_STORAGE_INFO, MTP_OP_GET_OBJECT_HANDLES,
	MTP_OP_GET_OBJECT_INFO,  MTP_OP_GET_OBJECT,       MTP_OP_DELETE_OBJECT,
	MTP_OP_SEND_OBJECT_INFO, MTP_OP_SEND_OBJECT};

static const uint16_t playback_formats[] = {
	MTP_FORMAT_UNDEFINED,
	MTP_FORMAT_ASSOCIATION,
};

/* Local functions declarations */
static int send_response_code(struct mtp_context *ctx, struct net_buf *buf, uint16_t err_code);
static int send_response_with_params(struct mtp_context *ctx, struct net_buf *buf,
				     uint16_t err_code, uint32_t *params, size_t params_count);
static inline void set_mtp_phase(struct mtp_context *ctx, enum mtp_phase phase);

/* Variables */
static struct partition_t partitions[] = {{.mountpoint = "NULL"},
					  DT_INST_FOREACH_STATUS_OKAY(PROCESS_MTP_ENTRY)};

BUILD_ASSERT(ARRAY_SIZE(partitions) > 1, "At least one MTP partition must be configured");

static struct dev_info_t dev_info;

#define RESET "\033[0m"
#define GREEN "\033[32m" /* Green */

static inline const char *mtp_params_debug_string(struct mtp_container *mtp_command, int count,
						  const char *names[])
{
	static char debug_str[128];
	int offset = 0;
	uint32_t value;

	for (int i = 0; i < count; i++) {
		value = mtp_command->param[i];
		if (strcmp(names[i], "ObjHandle") == 0) {
			mtp_object_handle_t handle = (mtp_object_handle_t)value;

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
	char *str = NULL;

	switch (code) {
	/* MTP Operation Codes */
	case 0x1001:
		str = "GetDeviceInfo";
		break;
	case 0x1002:
		str = "OpenSession";
		break;
	case 0x1003:
		str = "CloseSession";
		break;
	case 0x1004:
		str = "GetStorageIDs";
		break;
	case 0x1005:
		str = "GetStorageInfo";
		break;
	case 0x1006:
		str = "GetNumObjects";
		break;
	case 0x1007:
		str = "GetObjectHandles";
		break;
	case 0x1008:
		str = "GetObjectInfo";
		break;
	case 0x1009:
		str = "GetObject";
		break;
	case 0x100A:
		str = "GetThumb";
		break;
	case 0x100B:
		str = "DeleteObject";
		break;
	case 0x100C:
		str = "SendObjectInfo";
		break;
	case 0x100D:
		str = "SendObject";
		break;
	case 0x1010:
		str = "ResetDevice";
		break;
	case 0x1014:
		str = "GetDevicePropDesc";
		break;
	case 0x1015:
		str = "GetDevicePropValue";
		break;
	case 0x1016:
		str = "SetDevicePropValue";
		break;
	case 0x1017:
		str = "ResetDevicePropValue";
		break;
	case 0x1019:
		str = "MoveObject";
		break;
	case 0x101A:
		str = "CopyObject";
		break;
	case 0x101B:
		str = "GetPartialObject";
		break;
	case 0x9801:
		str = "GetObjectPropsSupported";
		break;
	case 0x9802:
		str = "GetObjectPropDesc";
		break;
	case 0x9803:
		str = "GetObjectPropValue";
		break;
	case 0x9804:
		str = "SetObjectPropValue";
		break;
	case 0x9810:
		str = "GetObjectReferences";
		break;
	case 0x9811:
		str = "SetObjectReferences";
		break;
	case 0x9820:
		str = "Skip";
		break;

	/* MTP Response Codes */
	case 0x2001:
		str = "OK";
		break;
	case 0x2002:
		str = "GeneralError";
		break;
	case 0x2003:
		str = "SessionNotOpen";
		break;
	case 0x2008:
		str = "InvalidStorageID";
		break;
	case 0x2009:
		str = "InvalidObjectHandle";
		break;
	case 0x200C:
		str = "StorageFull";
		break;
	case 0x201E:
		str = "SessionAlreadyOpen";
		break;

	/* MTP Image Formats */
	case 0x3001:
		str = "Association";
		break;
	case 0x3004:
		str = "Text";
		break;

	/* MTP Event Codes */
	case 0x4002:
		str = "ObjectAdded";
		break;
	case 0x4003:
		str = "ObjectRemoved";
		break;
	case 0x4004:
		str = "StoreAdded";
		break;
	case 0x4005:
		str = "StoreRemoved";
		break;
	case 0x4006:
		str = "DevicePropChanged";
		break;
	case 0x4007:
		str = "ObjectInfoChanged";
		break;

	/* MTP Device Properties */
	case 0x5001:
		str = "BatteryLevel";
		break;

	/* Object Properties */
	case 0xDC01:
		str = "StorageID";
		break;
	case 0xDC02:
		str = "ObjectFormat";
		break;
	case 0xDC03:
		str = "ProtectionStatus";
		break;
	case 0xDC04:
		str = "ObjectSize";
		break;
	case 0xDC07:
		str = "ObjectFileName";
		break;
	case 0xDC09:
		str = "DateModified";
		break;
	case 0xDC0B:
		str = "ParentObject";
		break;
	case 0xDC41:
		str = "PersistentUID";
		break;
	case 0xDC44:
		str = "Name";
		break;
	case 0xDCE0:
		str = "DisplayName";
		break;
	case 0xDD16:
		str = "FaxNumberBusiness";
		break;

	/* Storage Types */
	case 0x0001:
		str = "FixedROM";
		break;
	case 0x0002:
		str = "RemovableROM";
		break;
	case 0x0003:
		str = "FixedRAM";
		break;
	case 0x0004:
		str = "RemovableRAM";
		break;

	/* Default case if code is not recognized */
	default:
		str = "Unknown Code";
		LOG_WRN("Unknown Code 0x%x", code);
		break;
	}

	return str;
}

static void usb_buf_add_utf16le(struct net_buf *buf, const char *str)
{
	if (!str) {
		LOG_ERR("string is NULL");
		return;
	}

	uint16_t len = MTP_STR_LEN(str); /* we need the null terminator */

	for (int i = 0; i < len; i++) {
		net_buf_add_le16(buf, str[i]);
	}
}

static void mtp_buf_add_string(struct net_buf *buf, const char *str)
{
	if (!str) {
		net_buf_add_u8(buf, 0);
		return;
	}

	net_buf_add_u8(buf, MTP_STR_LEN(str));
	usb_buf_add_utf16le(buf, str);
}

static void usb_buf_pull_utf16le(struct net_buf *buf, char *strbuf, size_t len)
{
	for (int i = 0; i < len; ++i) {
		strbuf[i] = net_buf_pull_u8(buf);
		net_buf_pull_u8(buf);
	}
}

static void mtp_buf_push_data_header(struct mtp_context *ctx, struct net_buf *buf,
				     uint32_t data_len)
{
	/* DATA Block Header */
	struct mtp_header hdr;
	size_t header_size = sizeof(struct mtp_header);

	hdr.type = MTP_CONTAINER_DATA;
	hdr.code = ctx->op_state.code;
	hdr.transaction_id = ctx->transaction_id;
	hdr.length = (header_size + data_len);

	/* Ensure we have enough headroom for the header */
	__ASSERT_NO_MSG(net_buf_headroom(buf) == header_size);

	net_buf_push_mem(buf, &hdr, header_size);
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
				     uint16_t err_code, uint32_t *params, size_t params_count)
{

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

	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header) +
						    (params_count * sizeof(uint32_t)),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = ctx->op_state.err,
					  .transaction_id = ctx->transaction_id};

	/* Make sure response is always in the beginning of the buffer */
	net_buf_reset(buf);
	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
	for (size_t i = 0; i < params_count; i++) {
		net_buf_add_le32(buf, params[i]);
	}

	LOG_DBG("Sending Response (ErrCode: 0x%x [%s]), TID: %u", ctx->op_state.err,
		mtp_code_to_string(ctx->op_state.err), ctx->transaction_id);

	set_mtp_phase(ctx, MTP_PHASE_REQUEST);
	return 0;
}

static uint16_t validate_object_handle(mtp_object_handle_t handle)
{
	if (handle.partition_id == 0 || handle.partition_id >= ARRAY_SIZE(partitions)) {
		return MTP_RESP_INVALID_STORAGE_ID;
	}
	if (handle.object_id >= partitions[handle.partition_id].files_count) {
		return MTP_RESP_INVALID_OBJECT_HANDLE;
	}
	if (partitions[handle.partition_id].objlist[handle.object_id].handle.value !=
	    handle.value) {
		return MTP_RESP_INVALID_OBJECT_HANDLE;
	}
	return MTP_RESP_OK;
}

static int construct_path(char *buf, uint32_t max_len, const char *path, const char *name,
			  enum fs_dir_entry_type type)
{
	return snprintf(buf, max_len, "%s/%s%s", path, name, (type == FS_DIR_ENTRY_DIR) ? "/" : "");
}

static int traverse_path(struct mtp_object_t *obj, char *buf)
{
	if (obj->handle.parent_id != MTP_ROOT_OBJ_HANDLE) {
		int off = traverse_path(
			&partitions[obj->handle.partition_id].objlist[obj->handle.parent_id], buf);
		return snprintf(&buf[off], MAX_OBJPATH_LEN - off, "%s", obj->name);
	} else {
		return construct_path(buf, MAX_OBJPATH_LEN,
				      partitions[obj->handle.partition_id].mountpoint, obj->name,
				      obj->handle.type);
	}
}

static int dir_traverse(uint8_t partition_id, const char *root_path, uint32_t parent)
{
	char path[MAX_OBJPATH_LEN];
	struct fs_dir_t dir;
	int err;
	struct partition_t *cur_partition = &partitions[partition_id];

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, root_path);
	if (err) {
		LOG_ERR("Unable to open %s (err %d)", root_path, err);
		return -ENOEXEC;
	}

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err) {
			LOG_ERR("Unable to read directory");
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		/* Build the full path of the file or directory */
		construct_path(path, sizeof(path), root_path, entry.name, entry.type);

		/* Store the path in the array */
		if (cur_partition->files_count < MAX_FILES) {
			if (strnlen(entry.name, MAX_FILE_NAME) >= MAX_FILE_NAME) {
				LOG_WRN("Skipping %s, File name is too long", entry.name);
				continue;
			}

			strncpy(cur_partition->objlist[cur_partition->files_count].name, entry.name,
				MAX_FILE_NAME - 1);
			cur_partition->objlist[cur_partition->files_count].size = entry.size;
			cur_partition->objlist[cur_partition->files_count].handle.type = entry.type;
			cur_partition->objlist[cur_partition->files_count].handle.parent_id =
				parent;
			cur_partition->objlist[cur_partition->files_count].handle.object_id =
				cur_partition->files_count;
			cur_partition->objlist[cur_partition->files_count].handle.partition_id =
				partition_id;
			cur_partition->files_count++;

			if (entry.type == FS_DIR_ENTRY_DIR) {
				/* Recursive call to traverse subdirectory */
				err = dir_traverse(
					partition_id, path,
					cur_partition->objlist[cur_partition->files_count - 1]
						.handle.object_id);
				if (err) {
					LOG_ERR("Failed to traverse %s", path);
					break;
				}
			}
		} else {
			LOG_ERR("Max file count reached, cannot store more paths.");
			break;
		}
	}

	fs_closedir(&dir);

	return err;
}

static int dir_delete(char *dirpath)
{
	struct fs_dir_t dir;
	int err;
	char objpath[MAX_OBJPATH_LEN];

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, dirpath);
	if (err) {
		LOG_ERR("Unable to open %s (err %d)", dirpath, err);
		return -ENOEXEC;
	}

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err) {
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		/* Build the full path of the file or directory */
		construct_path(objpath, sizeof(objpath), dirpath, entry.name, entry.type);
		if (entry.type == FS_DIR_ENTRY_DIR) {
			dir_delete(objpath);
		} else {
			fs_unlink(objpath);
		}
	}

	if (!err) {
		fs_closedir(&dir);
		fs_unlink(dirpath);
	}

	return 0;
}

static int get_child_objects_handles(uint8_t partition_id, uint32_t parent_id, struct net_buf *buf)
{
	int found_files = 0;

	/* Skip the ROOT object */
	for (int i = 1; i < partitions[partition_id].files_count; ++i) {
		if (partitions[partition_id].objlist[i].name[0] == '\0') {
			/* to skip a deleted object */
			continue;
		}
		if (partitions[partition_id].objlist[i].handle.parent_id == parent_id) {
			LOG_DBG("Found %s in partition %d",
				partitions[partition_id].objlist[i].name, partition_id);
			net_buf_add_le32(buf, partitions[partition_id].objlist[i].handle.value);
			found_files++;
		}
	}

	return found_files;
}

MTP_CMD_HANDLER(MTP_OP_GET_DEVICE_INFO)
{
	if (ctx->session_opened == false && ctx->transaction_id != 0) {
		LOG_DBG("transaction_id is not 0");
		send_response_code(ctx, buf, MTP_RESP_PARAMETER_NOT_SUPPORTED);
		return;
	}

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, sizeof(struct mtp_header));

	/* Device Info */
	net_buf_add_le16(buf, 100); /* standard_version = MTP version 1.00 */
	net_buf_add_le32(buf, 6); /* vendor_extension_id = MTP standard extension ID (Microsoft) */
	net_buf_add_le16(buf, 100); /* vendor_extension_version */

	/* No Vendor extension is supported */
	net_buf_add_u8(buf, 0);

	/* functional_mode */
	net_buf_add_le16(buf, 0);

	/* operations supported */
	net_buf_add_le32(buf, ARRAY_SIZE(mtp_operations));            /* count */
	net_buf_add_mem(buf, mtp_operations, sizeof(mtp_operations)); /* operations_supported[] */

	/* events supported */
	net_buf_add_le32(buf, 0);

	/* Device properties supported */
	net_buf_add_le32(buf, 0); /* no props are used */

	/* Capture formats count */
	net_buf_add_le32(buf, 0);

	/* Playback formats supported */
	net_buf_add_le32(buf, ARRAY_SIZE(playback_formats));              /* count */
	net_buf_add_mem(buf, playback_formats, sizeof(playback_formats)); /* playback_formats[] */

	mtp_buf_add_string(buf, dev_info.manufacturer); /* manufacturer[] */

	mtp_buf_add_string(buf, dev_info.model); /* model[] */

	mtp_buf_add_string(buf, dev_info.device_version); /* device_version[] */

	mtp_buf_add_string(buf, dev_info.serial_number); /* serial_number[] */

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_OPEN_SESSION)
{
	uint16_t err_code = MTP_RESP_OK;

	if (ctx->session_opened == false) {
		for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
			if (dir_traverse(i, partitions[i].mountpoint, MTP_ROOT_OBJ_HANDLE)) {
				LOG_ERR("Failed to traverse %s", partitions[i].mountpoint);
				err_code = MTP_RESP_GENERAL_ERROR;
				break;
			}
		}
	} else {
		LOG_ERR("Session already opened");
		err_code = MTP_RESP_SESSION_ALREADY_OPEN;
	}

	if (err_code == MTP_RESP_OK) {
		LOG_DBG("Session opened successfully");
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
	net_buf_reserve(buf, sizeof(struct mtp_header));

	net_buf_add_le32(buf, ARRAY_SIZE(partitions) - 1); /* Number of stores */
	for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
		net_buf_add_le32(
			buf, GENERATE_STORAGE_ID(
				     i)); /* Use array index as Storage ID, 0x00 can't be used */
	}

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_INFO)
{
	uint32_t partition_id = ((mtp_storage_id_t)(mtp_command->param[0])).id;

	MTP_PARAMS_DEBUG(mtp_command, 1, "StorageID");

	if (partition_id == 0) {
		LOG_ERR("Unknown partition ID %x", partition_id);
	}

	struct fs_statvfs stat;
	int err = fs_statvfs(partitions[partition_id].mountpoint, &stat);

	if (err < 0) {
		LOG_ERR("Failed to statvfs %s (%d)", partitions[partition_id].mountpoint, err);
		return;
	}

	const char *storage_name = partitions[partition_id].mountpoint;

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	if (storage_name[0] == '/') {
		/* skip the slash */
		storage_name++;
	}

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, sizeof(struct mtp_header));

	net_buf_add_le16(buf, STORAGE_TYPE_FIXED_RAM);       /* type */
	net_buf_add_le16(buf, FS_TYPE_GENERIC_HIERARCHICAL); /* fs_type */
	if (partitions[partition_id].read_only) {
		net_buf_add_le16(buf, OBJECT_PROTECTION_READ_ONLY); /* access_caps */
	} else {
		net_buf_add_le16(buf, OBJECT_PROTECTION_NO);
	}
	net_buf_add_le64(buf, (stat.f_blocks * stat.f_frsize)); /* max_capacity */
	net_buf_add_le64(buf, (stat.f_bfree * stat.f_frsize));  /* free_space */
	net_buf_add_le32(buf, MTP_FREE_SPACE_OBJ_UNUSED);       /* free_space_obj */
	mtp_buf_add_string(buf, storage_name);                  /* storage_desc[] */
	net_buf_add_u8(buf, 0);                                 /* volume_id_len, Unused */

	/* Add the Packet Header */
	mtp_buf_push_data_header(ctx, buf, buf->len);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_HANDLES)
{
	mtp_storage_id_t storage_id = (mtp_storage_id_t)(mtp_command->param[0]);
	uint32_t partition_id = storage_id.id;
	uint32_t obj_format_code = mtp_command->param[1];
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[2];

	/* ObjHandle: Parent object handle for which child objects are requested */
	MTP_PARAMS_DEBUG(mtp_command, 3, "StorageID", "ObjFormatCode", "ObjHandle");

	uint32_t found_files = 0;
	uint32_t parent_id = (obj_handle.value == MTP_ALLROOTOBJECTS ? MTP_ROOT_OBJ_HANDLE
								     : obj_handle.object_id);

	if (obj_format_code) {
		send_response_code(ctx, buf, MTP_RESP_SPECIFICATION_BY_FORMAT_UNSUPPORTED);
		return;
	}

	/* Reserve space for MTP header at the beginning of the buffer */
	net_buf_reserve(buf, sizeof(struct mtp_header) + sizeof(uint32_t));

	/* Host wants all root objects on all partitions */
	if (storage_id.value == MTP_ALLSTORAGES) {
		for (int part_idx = 1; part_idx < ARRAY_SIZE(partitions); ++part_idx) {
			found_files = get_child_objects_handles(part_idx, parent_id, buf);
		}
	} else {
		if (partition_id >= ARRAY_SIZE(partitions) || partition_id == 0) {
			LOG_ERR("Unknown partition ID %x", partition_id);
			send_response_code(ctx, buf, MTP_RESP_INVALID_STORAGE_ID);
			return;
		}

		found_files = get_child_objects_handles(partition_id, parent_id, buf);
	}

	net_buf_push_mem(buf, &found_files, sizeof(uint32_t));
	mtp_buf_push_data_header(ctx, buf, buf->len);

	set_mtp_phase(ctx, MTP_PHASE_DATA);

	send_response_code(ctx, buf, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_INFO)
{
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[0];
	uint8_t partition_id = obj_handle.partition_id;
	uint8_t object_id = obj_handle.object_id;
	uint16_t err_code = MTP_RESP_OK;

	MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");

	err_code = validate_object_handle(obj_handle);
	if (err_code != MTP_RESP_OK) {
		send_response_code(ctx, buf, err_code);
		return;
	}

	if (partitions[partition_id].objlist[object_id].handle.value == obj_handle.value) {
		/* Reserve space for MTP header at the beginning of the buffer */
		net_buf_reserve(buf, sizeof(struct mtp_header));

		char *filename = partitions[partition_id].objlist[object_id].name;

		net_buf_add_le32(buf, GENERATE_STORAGE_ID(partition_id)); /* StorageID */

		if (partitions[partition_id].objlist[object_id].handle.type == FS_DIR_ENTRY_DIR) {
			net_buf_add_le16(buf, MTP_FORMAT_ASSOCIATION); /* ObjectFormat */
		} else {
			net_buf_add_le16(buf, MTP_FORMAT_UNDEFINED); /* ObjectFormat */
		}

		net_buf_add_le16(buf, OBJECT_PROTECTION_NO); /* ProtectionStatus */

		if (partitions[partition_id].objlist[object_id].handle.type == FS_DIR_ENTRY_DIR) {
			net_buf_add_le32(buf, MTP_ASSOSCIATION_SIZE); /* ObjectCompressedSize */
		} else {
			net_buf_add_le32(buf, partitions[partition_id].objlist[object_id].size);
		}
		net_buf_add_le16(buf, 0); /* ThumbFormat */
		net_buf_add_le32(buf, 0); /* ThumbCompressedSize */
		net_buf_add_le32(buf, 0); /* ThumbPixWidth */
		net_buf_add_le32(buf, 0); /* ThumbPixHeight */
		net_buf_add_le32(buf, 0); /* ImagePixWidth */
		net_buf_add_le32(buf, 0); /* ImagePixHeight */
		net_buf_add_le32(buf, 0); /* ImageBitDepth */

		mtp_object_handle_t parent_handle = partitions[partition_id]
							    .objlist[partitions[partition_id]
									     .objlist[object_id]
									     .handle.parent_id]
							    .handle;

		net_buf_add_le32(buf, parent_handle.value); /* ParentObject */

		LOG_DBG("%s in %s %x", partitions[partition_id].objlist[object_id].name,
			parent_handle.value ? "parent" : "Root", parent_handle.value);

		if (partitions[partition_id].objlist[object_id].handle.type == FS_DIR_ENTRY_DIR) {
			net_buf_add_le16(buf, MTP_ASSOCIATION_TYPE_GENERIC); /* AssociationType */
		} else {
			net_buf_add_le16(buf, MTP_ASSOCIATION_TYPE_UNDEFINED); /* AssociationType */
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
			partitions[partition_id].objlist[object_id].name,
			partitions[partition_id].objlist[object_id].size);

		set_mtp_phase(ctx, MTP_PHASE_DATA);
	} else {
		LOG_ERR("Unknown ID %08x", obj_handle.value);
		err_code = MTP_RESP_INVALID_OBJECT_HANDLE;
	}

	send_response_code(ctx, buf, err_code);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT)
{
	mtp_object_handle_t obj_handle;
	uint8_t partition_id;
	uint8_t object_id;
	uint32_t available_buf_len = ctx->max_packet_size;
	uint32_t data_len = 0;
	int err = 0;

	if (ctx->phase == MTP_PHASE_REQUEST) {
		/* Reserve space for MTP header at the beginning of the buffer */
		net_buf_reserve(buf, sizeof(struct mtp_header));

		MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");
		obj_handle = (mtp_object_handle_t)mtp_command->param[0];
		partition_id = obj_handle.partition_id;
		object_id = obj_handle.object_id;

		err = validate_object_handle(obj_handle);
		if (err != MTP_RESP_OK) {
			send_response_code(ctx, buf, err);
			return;
		}

		memset(ctx->transfer_state.filepath, 0x00, MAX_OBJPATH_LEN);
		traverse_path(&partitions[partition_id].objlist[object_id],
			      ctx->transfer_state.filepath);

		fs_file_t_init(&ctx->transfer_state.file);
		err = fs_open(&ctx->transfer_state.file, ctx->transfer_state.filepath, FS_O_READ);
		if (err) {
			LOG_ERR("Failed to open %s (%d)", ctx->transfer_state.filepath, err);
			send_response_code(ctx, buf, MTP_RESP_ACCESS_DENIED);
			return;
		}

		ctx->transfer_state.total_size = partitions[partition_id].objlist[object_id].size;
		ctx->transfer_state.transferred = 0;
		ctx->transfer_state.chunks_sent = 0;

		available_buf_len = ctx->max_packet_size - sizeof(struct mtp_header);

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

	int read = fs_read(&ctx->transfer_state.file, ctx->filebuf, data_len);

	if (read <= 0) {
		LOG_ERR("Failed to read file content %d", read);
	}
	net_buf_add_mem(buf, ctx->filebuf, read);

	ctx->transfer_state.transferred += read;
	ctx->transfer_state.chunks_sent++;

	LOG_DBG("Sent chunk: %u [%u of %u], %u bytes remaining", ctx->transfer_state.chunks_sent,
		ctx->transfer_state.transferred, ctx->transfer_state.total_size,
		(ctx->transfer_state.total_size - ctx->transfer_state.transferred));

	if (ctx->transfer_state.transferred >= ctx->transfer_state.total_size) {
		LOG_DBG("Done, Sending Response");
		fs_close(&ctx->transfer_state.file);
		memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		send_response_code(ctx, buf, MTP_RESP_OK);
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT_INFO)
{
	uint32_t dest_partition_id = 0;
	uint32_t dest_parent_id = 0;
	uint32_t new_obj_id = 0;
	uint16_t ObjectFormat = 0;
	uint8_t filename_len = 0;
	int32_t ret = 0;
	uint16_t err_code = MTP_RESP_OK;
	struct mtp_object_t *new_obj;
	struct fs_statvfs fs_stat;

	/* Process the request only after receiving the 2 packets.
	 * First packet: contains only destination storageID, ParentID.
	 * Second packet: contains the rest of the info.
	 * Reply to host only after the second packet
	 */
	if (ctx->phase == MTP_PHASE_REQUEST) {
		MTP_PARAMS_DEBUG(mtp_command, 2, "StorageID", "ParentObjHandle");

		ctx->op_state.args[0] = mtp_command->param[0];
		ctx->op_state.args[1] = mtp_command->param[1];

		set_mtp_phase(ctx, MTP_PHASE_DATA);
		return;
	}

	dest_partition_id = ((mtp_storage_id_t)(ctx->op_state.args[0])).id;
	dest_parent_id = ((mtp_object_handle_t)(ctx->op_state.args[1])).value;

	/* ParentID should be 0x00 when asked to store the new object at Root of the store */
	if (dest_parent_id == MTP_STORE_ROOT) {
		dest_parent_id = MTP_ROOT_OBJ_HANDLE;
	} else {
		dest_parent_id = ((mtp_object_handle_t)(ctx->op_state.args[1])).object_id;
	}

	/* Initial checks */
	if (ctx->phase != MTP_PHASE_DATA) {
		LOG_ERR("Invalid phase %d, expected %d", ctx->phase, MTP_PHASE_DATA);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	if (dest_partition_id == 0 || dest_partition_id >= ARRAY_SIZE(partitions)) {
		LOG_ERR("Unknown partition id %x", dest_partition_id);
		err_code = MTP_RESP_INVALID_STORAGE_ID;
		goto exit;
	}

	if (partitions[dest_partition_id].read_only) {
		LOG_ERR("Storage %u is read-only", dest_partition_id);
		err_code = MTP_RESP_STORE_READ_ONLY;
		goto exit;
	}

	if ((partitions[dest_partition_id].files_count + 1) >= MAX_FILES) {
		LOG_ERR("No file handle available [file count: %u]",
			partitions[dest_partition_id].files_count);
		err_code = MTP_RESP_STORAGE_FULL;
		goto exit;
	}

	new_obj_id = partitions[dest_partition_id].files_count;

	partitions[dest_partition_id].files_count++;

	new_obj = &partitions[dest_partition_id].objlist[new_obj_id];
	new_obj->handle.object_id = new_obj_id;
	new_obj->handle.type = FS_DIR_ENTRY_FILE;
	new_obj->handle.partition_id = dest_partition_id;
	new_obj->handle.parent_id = dest_parent_id;

	LOG_DBG("New ObjHandle: 0x%08x (PartID=%u, ParentID=%u, ObjID=%u)", new_obj->handle.value,
		new_obj->handle.partition_id, new_obj->handle.parent_id, new_obj->handle.object_id);

	net_buf_pull(payload, sizeof(struct mtp_header)); /* SKIP the header */
	net_buf_pull_le32(payload);                       /* StorageID, always 0 ignore */
	ObjectFormat = net_buf_pull_le16(payload);        /* ObjectFormat */

	if (ObjectFormat == MTP_FORMAT_ASSOCIATION) {
		new_obj->handle.type = FS_DIR_ENTRY_DIR;
	}

	net_buf_pull_le16(payload);                 /* ProtectionStatus */
	new_obj->size = net_buf_pull_le32(payload); /* ObjectCompressedSize */
	net_buf_pull_le16(payload);                 /* ThumbFormat */
	net_buf_pull_le32(payload);                 /* ThumbCompressedSize */
	net_buf_pull_le32(payload);                 /* ThumbPixWidth */
	net_buf_pull_le32(payload);                 /* ThumbPixHeight */
	net_buf_pull_le32(payload);                 /* ImagePixWidth */
	net_buf_pull_le32(payload);                 /* ImagePixHeight */
	net_buf_pull_le32(payload);                 /* ImageBitDepth */
	net_buf_pull_le32(payload);                 /* ParentObject (always 0, Ignore) */
	net_buf_pull_le16(payload);                 /* AssociationType */
	net_buf_pull_le32(payload);                 /* AssociationDesc */
	net_buf_pull_le32(payload);                 /* SequenceNumber */

	filename_len = net_buf_pull_u8(payload); /* FileNameLength */
	if (filename_len >= MAX_FILE_NAME) {
		LOG_ERR("filename is too long %u", filename_len);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	usb_buf_pull_utf16le(payload, new_obj->name, filename_len); /* FileName */
	/* Rest of props are ignored */

	ret = fs_statvfs(partitions[new_obj->handle.partition_id].mountpoint, &fs_stat);
	if (ret < 0) {
		LOG_ERR("Failed to statvfs %s (%d)",
			partitions[new_obj->handle.partition_id].mountpoint, ret);
		err_code = MTP_RESP_GENERAL_ERROR;
		goto exit;
	}

	if (new_obj->size > (fs_stat.f_bfree * fs_stat.f_frsize)) {
		LOG_ERR("Not enough space to store file %u > %lu", new_obj->size,
			(fs_stat.f_bfree * fs_stat.f_frsize));
		err_code = MTP_RESP_STORAGE_FULL;
		goto exit;
	}

	traverse_path(new_obj, ctx->transfer_state.filepath);

	if (new_obj->handle.type == FS_DIR_ENTRY_DIR) {
		ret = fs_mkdir(ctx->transfer_state.filepath);
		if (ret) {
			LOG_ERR("Failed to create directory %s (%d)", ctx->transfer_state.filepath,
				ret);
			err_code = MTP_RESP_GENERAL_ERROR;
		}
	} else {
		fs_file_t_init(&ctx->transfer_state.file);
		ret = fs_open(&ctx->transfer_state.file, ctx->transfer_state.filepath,
			      FS_O_CREATE | FS_O_WRITE);
		if (ret) {
			LOG_ERR("Open file failed, %d", ret);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto exit;
		}

		ctx->transfer_state.total_size = new_obj->size;
	}

	LOG_DBG("\n ObjFormat: %x, size: %d, parent: %x\n mnt: %s\n fname: %s\n path: %s "
		"Handle:%x\n parentID: %u",
		ObjectFormat, new_obj->size, dest_parent_id,
		partitions[new_obj->handle.partition_id].mountpoint, new_obj->name,
		ctx->transfer_state.filepath, new_obj->handle.value, new_obj->handle.parent_id);

exit:
	set_mtp_phase(ctx, MTP_PHASE_RESPONSE);

	if (new_obj && err_code == MTP_RESP_OK) {
		uint32_t params[3] = {0, 0, 0};

		params[0] = GENERATE_STORAGE_ID(new_obj->handle.partition_id),
		params[1] = GET_PARENT_HANDLE(new_obj);
		params[2] = new_obj->handle.value;

		send_response_with_params(ctx, buf, err_code, params, 3);

		LOG_DBG("Sent info:\n\tSID: %x\n\tPID: %x\n\tOID: %x", params[0], params[1],
			params[2]);
	} else {
		if (new_obj_id) {
			partitions[dest_partition_id].files_count--;
			memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
			memset(new_obj, 0x00, sizeof(struct mtp_object_t));
		}
		send_response_code(ctx, buf, err_code);
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT)
{
	int ret = 0;

	if (ctx->phase == MTP_PHASE_REQUEST) {
		if (mtp_command->hdr.type == MTP_CONTAINER_COMMAND) {
			LOG_DBG("COMMAND RECEIVED len: %u", payload->len);
			set_mtp_phase(ctx, MTP_PHASE_DATA);
			return;
		}
	}

	if (ctx->transfer_state.transferred == 0) {
		/* First data packet contains the header */
		LOG_DBG("DATA RECEIVED len: %u", payload->len);
		net_buf_pull_mem(payload, sizeof(struct mtp_header));
	}

	ret = fs_write(&ctx->transfer_state.file, payload->data, payload->len);
	if (ret < 0) {
		LOG_ERR("Failed to write data to file %s (%d)", ctx->transfer_state.filepath, ret);
		send_response_code(ctx, buf, MTP_RESP_STORE_NOT_AVAILABLE);
		return;
	}

	ctx->transfer_state.chunks_sent++;
	ctx->transfer_state.transferred += payload->len;
	LOG_DBG("SEND_OBJECT: Data len: %u out of %u", ctx->transfer_state.transferred,
		ctx->transfer_state.total_size);

	if (ctx->transfer_state.transferred >= ctx->transfer_state.total_size) {
		fs_close(&ctx->transfer_state.file);
		LOG_DBG("SEND_OBJECT: All data received (%u bytes), Sending Response",
			ctx->transfer_state.transferred);

		memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		set_mtp_phase(ctx, MTP_PHASE_RESPONSE);
		send_response_code(ctx, buf, MTP_RESP_OK);
	} else {
		set_mtp_phase(ctx, MTP_PHASE_DATA);
	}
}

MTP_CMD_HANDLER(MTP_OP_DELETE_OBJECT)
{
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[0];
	uint32_t partition_id = obj_handle.partition_id;
	uint32_t object_id = obj_handle.object_id;
	uint16_t err_code = MTP_RESP_OK;

	char path[MAX_OBJPATH_LEN];

	MTP_PARAMS_DEBUG(mtp_command, 1, "ObjHandle");

	err_code = validate_object_handle(obj_handle);
	if (err_code != MTP_RESP_OK) {
		send_response_code(ctx, buf, err_code);
		return;
	}

	if (obj_handle.value == MTP_ALLROOTOBJECTS) {
		LOG_ERR("Invalid object handle 0x%08x", obj_handle.value);
		err_code = MTP_RESP_INVALID_OBJECT_HANDLE;
	} else {
		memset(path, 0x00, MAX_OBJPATH_LEN);
		traverse_path(&partitions[partition_id].objlist[object_id], path);
		LOG_DBG("Traversed Path: %s", path);

		if (partitions[partition_id].read_only == false) {
			if (obj_handle.type == FS_DIR_ENTRY_DIR) {
				LOG_DBG("Deleting directory %s", path);
				dir_delete(path);
			} else {
				LOG_DBG("Deleting file %s", path);
				fs_unlink(path);
			}
#ifdef CONFIG_RECYCLE_OBJECT_HANDLES
			partitions[partition_id].files_count--;
#endif
			memset(&partitions[partition_id].objlist[object_id], 0x00,
			       sizeof(struct mtp_object_t));
		} else {
			LOG_WRN("Read only partition %u", partition_id);
			err_code = MTP_RESP_STORE_READ_ONLY;
		}
	}

	send_response_code(ctx, buf, err_code);
}

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
	struct mtp_container *mtp_command = (struct mtp_container *)buf_in->data;
	struct net_buf *payload = buf_in;

	if (buf_resp == NULL) {
		LOG_ERR("%s: NULL Buffer", __func__);
		return -EINVAL;
	}

	switch (ctx->phase) {
	case MTP_PHASE_REQUEST:
		ctx->op_state.code = mtp_command->hdr.code;
		ctx->transaction_id = mtp_command->hdr.transaction_id;

		if (ctx->session_opened == false) {
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

	LOG_DBG(GREEN "[%s]" RESET " %u", mtp_code_to_string(ctx->op_state.code),
		ctx->transaction_id);

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
	case MTP_OP_DELETE_OBJECT:
		MTP_CMD(MTP_OP_DELETE_OBJECT);
		break;
	case MTP_OP_SEND_OBJECT_INFO:
		MTP_CMD(MTP_OP_SEND_OBJECT_INFO);
		break;
	case MTP_OP_SEND_OBJECT:
		MTP_CMD(MTP_OP_SEND_OBJECT);
		break;
	default:
		LOG_ERR("Not supported cmd 0x%x!", mtp_command->hdr.code);
		send_response_code(ctx, buf_resp, MTP_RESP_OPERATION_NOT_SUPPORTED);
		break;
	}

	return buf_resp->len;
}

int mtp_control_to_host(struct mtp_context *ctx, uint8_t request, struct net_buf *const buf)
{
	struct mtp_device_status_t dev_status;

	if (request == MTP_REQUEST_GET_DEVICE_STATUS) {
		if (ctx->phase == MTP_PHASE_CANCELED) {
			LOG_DBG("Operation cancelled by Host, Sending Response");
			set_mtp_phase(ctx, MTP_PHASE_REQUEST);
			dev_status.len = sizeof(struct mtp_device_status_t);
			dev_status.code = MTP_RESP_TRANSACTION_CANCELLED;
			net_buf_add_mem(buf, &dev_status, dev_status.len);
		} else {
			LOG_DBG("Device status OK");
			dev_status.len = sizeof(uint16_t) * 2;
			dev_status.code = MTP_RESP_OK;
			net_buf_add_mem(buf, &dev_status, dev_status.len);
		}
	} else {
		LOG_ERR("Unknown Host request 0x%x!", request);
		errno = -ENOTSUP;
	}

	return 0;
}

int mtp_control_to_dev(struct mtp_context *ctx, uint8_t request, const struct net_buf *const buf)
{
	switch (request) {
	case MTP_REQUEST_CANCEL:
		set_mtp_phase(ctx, MTP_PHASE_CANCELED);
		LOG_DBG("Operation cancelled by Host, Closing incomplete file %s",
			ctx->transfer_state.filepath);
		if (strnlen(ctx->transfer_state.filepath, MAX_OBJPATH_LEN) > 0) {
			fs_close(&ctx->transfer_state.file);

			/* Delete the opened file only when downloading from Host */
			if (ctx->op_state.code == MTP_OP_SEND_OBJECT) {
				fs_unlink(ctx->transfer_state.filepath);
			}

			memset(ctx->filebuf, 0, sizeof(ctx->filebuf));
			memset(&ctx->transfer_state, 0, sizeof(ctx->transfer_state));
		}
		break;

	case MTP_REQUEST_DEVICE_RESET:
		LOG_WRN("MTP_REQUEST_DEVICE_RESET");
		mtp_reset(ctx);
		break;

	default:
		LOG_ERR("Unknown Dev request 0x%x!", request);
		errno = -ENOTSUP;
		break;
	}

	return 0;
}

void mtp_reset(struct mtp_context *ctx)
{
	LOG_DBG("%s", __func__);
	for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
		memset(partitions[i].objlist, 0, sizeof(partitions[i].objlist));
		partitions[i].files_count = 1; /* Reserve 0th obj for ROOT */
	}

	ctx->transaction_id = 0;
	ctx->session_opened = false;
	ctx->phase = MTP_PHASE_REQUEST;
	memset(&ctx->op_state, 0, sizeof(ctx->op_state));
	memset(&ctx->transfer_state, 0, sizeof(ctx->transfer_state));
}

int mtp_init(struct mtp_context *ctx, const char *manufacturer, const char *model,
	     const char *device_version, const char *serial_number)
{
	dev_info.manufacturer = manufacturer;
	dev_info.model = model;
	dev_info.device_version = device_version;

	/* Zephyr set Serial Number descriptor after MTP init, so for now use this one */
	dev_info.serial_number = "0123456789ABCDEF";

	mtp_reset(ctx);

	return 0;
}

#if CONFIG_SHELL
static int cmd_mtp_list(const struct shell *sh, size_t argc, char **argv)
{
	for (int partitionIdx = 1; partitionIdx < ARRAY_SIZE(partitions); partitionIdx++) {
		shell_print(sh, "File list Storage %s", partitions[partitionIdx].mountpoint);
		for (int i = 0; i < partitions[partitionIdx].files_count; i++) {
			shell_print(
				sh, "\tID: 0x%08x (S: %02x, P: %02x, O: %02x), T:%s, Size: %u : %s",
				partitions[partitionIdx].objlist[i].handle.value,
				partitions[partitionIdx].objlist[i].handle.partition_id,
				partitions[partitionIdx].objlist[i].handle.parent_id,
				partitions[partitionIdx].objlist[i].handle.object_id,
				partitions[partitionIdx].objlist[i].handle.type == FS_DIR_ENTRY_DIR
					? "DIR"
					: "FILE",
				partitions[partitionIdx].objlist[i].size,
				i == 0 ? "ROOT" : partitions[partitionIdx].objlist[i].name);
		}
		shell_print(sh, "\n\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mtp,
			       SHELL_CMD_ARG(list, NULL, "Create directory", cmd_mtp_list, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(mtp, &sub_mtp, "USB MTP commands", NULL);
#endif
