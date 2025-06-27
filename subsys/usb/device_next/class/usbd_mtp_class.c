/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <wctype.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/net_buf.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
#include "usbd_mtp_class.h"

LOG_MODULE_REGISTER(usb_mtp_class, CONFIG_USBD_MTP_LOG_LEVEL);

/* MTP Control Requests Codes */
#define MTP_REQUEST_CANCEL            0x64U
#define MTP_REQUEST_GET_DEVICE_STATUS 0x67U
#define MTP_REQUEST_DEVICE_RESET      0x66U

/* MTP Operation Codes */
#define MTP_OP_GET_DEVICE_INFO       0x1001
#define MTP_OP_OPEN_SESSION          0x1002
#define MTP_OP_CLOSE_SESSION         0x1003
#define MTP_OP_GET_STORAGE_IDS       0x1004
#define MTP_OP_GET_STORAGE_INFO      0x1005
#define MTP_OP_GET_OBJECT_HANDLES    0x1007
#define MTP_OP_GET_OBJECT_INFO       0x1008
#define MTP_OP_GET_OBJECT            0x1009
#define MTP_OP_DELETE_OBJECT         0x100B
#define MTP_OP_SEND_OBJECT_INFO      0x100C
#define MTP_OP_SEND_OBJECT           0x100D
#define MTP_OP_GET_OBJECT_REFERENCES 0x9810

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
#define MAX_FILES CONFIG_USBD_MTP_MAX_HANDLES

#define STORAGE_TYPE_INTERNAL   0x00010000
#define GENERATE_STORAGE_ID(id) (STORAGE_TYPE_INTERNAL + id)
#define MTP_STR_LEN(str)        (strlen(str) + 1)

#define MTP_CMD(opcode) mtp_##opcode(ctx, mtp_command, payload, buf_resp)

#define MTP_CMD_HANDLER(opcode)                                                                    \
	static void mtp_##opcode(struct mtp_context *ctx, struct mtp_container *mtp_command,       \
				 struct net_buf *payload, struct net_buf *buf)
#define PROCESS_FSTAB_ENTRY(node_id)                                                               \
	IF_ENABLED(DT_PROP(node_id, mtp_enabled),                                                  \
			({.mountpoint = DT_PROP(node_id, mount_point),	\
			.files_count = 1,				\
			.read_only = DT_PROP(node_id, read_only)},))

/* Types */
enum mtp_container_type {
	MTP_CONTAINER_UNDEFINED = 0x00,
	MTP_CONTAINER_COMMAND,
	MTP_CONTAINER_DATA,
	MTP_CONTAINER_RESPONSE,
	MTP_CONTAINER_EVENT,
};

struct mtp_device_status_t {
	uint16_t len;
	uint16_t code;
	uint8_t epIn;
	uint8_t epOut;
} __packed;

struct mtp_header {
	uint32_t length;         /* Total length of the response block */
	uint16_t type;           /* a value from \ref:mtp_container_type */
	uint16_t code;           /* MTP response code (e.g., MTP_RESP_OK) */
	uint32_t transaction_id; /* Transaction ID of the command being responded to */
} __packed;

struct mtp_container {
	struct mtp_header hdr;
	uint32_t param[5]; /* Optional Parameters (e.g., session ID) */
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
	char name[MAX_FILE_NAME];
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

static const uint16_t events_supported[] = {MTP_EVENT_OBJECT_ADDED, MTP_EVENT_OBJECT_REMOVED};

static const uint16_t playback_formats[] = {
	MTP_FORMAT_UNDEFINED,
	MTP_FORMAT_ASSOCIATION,
};

/* Local functions declarations */
static int mtp_send_response(struct mtp_context *ctx, struct net_buf *buf);

/* Variables */
static struct partition_t partitions[] = {{.mountpoint = "NULL"},
					  DT_FOREACH_CHILD(DT_PATH(fstab), PROCESS_FSTAB_ENTRY)};

static struct dev_info_t dev_info;

#define RESET "\033[0m"
#define GREEN "\033[32m" /* Green */
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
	case 0x2009:
		str = "InvalidObjectHandle";
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

static inline void mark_op_complete(struct mtp_context *ctx, uint16_t err_code)
{
	ctx->op_state.phase = MTP_PHASE_COMPLETE;
	ctx->op_state.err = err_code;
}

static void usb_buf_add_utf16le(struct net_buf *buf, const char *str)
{
	if (!str) {
		LOG_ERR("string is NULL");
		return;
	}

	uint16_t len = MTP_STR_LEN(str); /* we need the null terminator */

	for (int i = 0; i < len; i++) {
		__ASSERT(str[i] > 0x1F && str[i] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB");
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

static void data_header_push(struct net_buf *buf, struct mtp_container *mtp_command,
			     uint32_t data_len)
{
	/* DATA Block Header */
	struct mtp_header hdr;

	hdr.type = MTP_CONTAINER_DATA;
	hdr.code = mtp_command->hdr.code;
	hdr.transaction_id = mtp_command->hdr.transaction_id;
	hdr.length = (sizeof(struct mtp_header) + data_len);

	net_buf_push_mem(buf, &hdr, sizeof(struct mtp_header));
}

static int construct_path(char *buf, uint32_t max_len, const char *path, const char *name,
			  enum fs_dir_entry_type type)
{
	return snprintf(buf, max_len, "%s/%s%s", path, name, (type == FS_DIR_ENTRY_DIR) ? "/" : "");
}

static int traverse_path(struct mtp_object_t *obj, char *buf)
{
	if (obj->handle.parent_id != 0xff) {
		int off = traverse_path(
			&partitions[obj->handle.partition_id].objlist[obj->handle.parent_id], buf);
		return snprintf(&buf[off], MAX_PATH_LEN - off, "%s", obj->name);
	} else {
		return construct_path(buf, MAX_PATH_LEN,
				      partitions[obj->handle.partition_id].mountpoint, obj->name,
				      obj->handle.type);
	}
}

static int dir_traverse(uint8_t partition_id, const char *root_path, uint32_t parent)
{
	char path[MAX_PATH_LEN + MAX_FILE_NAME];
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
			if (strnlen(entry.name, MAX_PATH_LEN) >= MAX_FILE_NAME) {
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
	char objpath[MAX_PATH_LEN + MAX_FILE_NAME];

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

MTP_CMD_HANDLER(MTP_OP_GET_DEVICE_INFO)
{
	/* Device Info */
	net_buf_add_le16(buf, 100); /* standard_version = MTP version 1.00 */
	net_buf_add_le32(buf, 6); /* vendor_extension_id = MTP standard extension ID (Microsoft) */
	net_buf_add_le16(buf, 100); /* vendor_extension_version */

	/* No Vendor extension is supported */
	net_buf_add_u8(buf, 0);

	/* functional_mode; */
	net_buf_add_le16(buf, 0);

	/* operations supported */
	net_buf_add_le32(buf, ARRAY_SIZE(mtp_operations));            /* count */
	net_buf_add_mem(buf, mtp_operations, sizeof(mtp_operations)); /* operations_supported[] */

	/* events supported */
	net_buf_add_le32(buf, ARRAY_SIZE(events_supported));              /* count */
	net_buf_add_mem(buf, events_supported, sizeof(events_supported)); /* events_supported[] */

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
	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_OPEN_SESSION)
{
	uint16_t err_code = MTP_RESP_OK;

	if (ctx->session_opened == false) {
		for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
			if (dir_traverse(i, partitions[i].mountpoint, 0xFFFFFFFF)) {
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

	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = err_code,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
}

MTP_CMD_HANDLER(MTP_OP_CLOSE_SESSION)
{
	mtp_reset(ctx);
	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = MTP_RESP_OK,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_INFO)
{
	uint32_t partition_id = ((mtp_storage_id_t)(mtp_command->param[0])).id;

	LOG_DBG("\n\t\tStorageID    : 0x%x\n", mtp_command->param[0]);

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

	if (storage_name[0] == '/') {
		/* skip the slash */
		storage_name++;
	}

	net_buf_add_le16(buf, STORAGE_TYPE_FIXED_RAM);       /* type */
	net_buf_add_le16(buf, FS_TYPE_GENERIC_HIERARCHICAL); /* fs_type */
	if (partitions[partition_id].read_only) {
		net_buf_add_le16(buf, OBJECT_PROTECTION_READ_ONLY); /* access_caps */
	} else {
		net_buf_add_le16(buf, OBJECT_PROTECTION_NO);
	}
	net_buf_add_le64(buf, (stat.f_blocks * stat.f_frsize)); /* max_capacity */
	net_buf_add_le64(buf, (stat.f_bfree * stat.f_frsize));  /* free_space */
	net_buf_add_le32(buf, 0xFFFFFFFF);                      /* free_space_obj */
	mtp_buf_add_string(buf, storage_name);                  /* storage_desc[] */
	net_buf_add_u8(buf, 0);                                 /* volume_id_len, Unused */

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_IDS)
{
	net_buf_add_le32(buf, ARRAY_SIZE(partitions) - 1); /* Number of storage */
	for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
		net_buf_add_le32(
			buf, GENERATE_STORAGE_ID(
				     i)); /* Use array index as Storage ID, 0x00 can't be used */
	}

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_HANDLES)
{
	uint32_t partition_id = ((mtp_storage_id_t)(mtp_command->param[0])).id;
	uint32_t obj_format_code = mtp_command->param[1];
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[2];

	LOG_DBG("\n\t\tPartitionID    : 0x%x"
		"\n\t\tObjFormatCode: 0x%x"
		"\n\t\tObjHandle    : 0x%x\n",
		partition_id, obj_format_code, obj_handle.value);

	uint32_t found_files = 0;
	uint32_t parent_id = (obj_handle.value == 0xffffffff ? 0xff : obj_handle.object_id);

	for (int i = 0; i < partitions[partition_id].files_count; ++i) {
		if (partitions[partition_id].objlist[i].handle.parent_id == parent_id) {
			net_buf_add_le32(buf, partitions[partition_id].objlist[i].handle.value);
			found_files++;
		}
	}
	net_buf_push_mem(buf, &found_files, sizeof(uint32_t));

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_INFO)
{
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[0];
	uint8_t partition_id = obj_handle.partition_id;
	uint8_t object_id = obj_handle.object_id;

	LOG_DBG("\n\t\tObjHandle: 0x%x, SID: %x, OID: %x", mtp_command->param[0], partition_id,
		object_id);

	if (partitions[partition_id].objlist[object_id].handle.value == obj_handle.value) {
		char *filename = partitions[partition_id].objlist[object_id].name;
		char *data_created = "20250101T120000";
		char *data_modified = "20250101T120000";

		net_buf_add_le32(buf, GENERATE_STORAGE_ID(partition_id)); /* StorageID */

		if (partitions[partition_id].objlist[object_id].handle.type == FS_DIR_ENTRY_DIR) {
			net_buf_add_le16(buf, MTP_FORMAT_ASSOCIATION); /* ObjectFormat */
		} else {
			net_buf_add_le16(buf, MTP_FORMAT_UNDEFINED); /* ObjectFormat */
		}

		net_buf_add_le16(buf, OBJECT_PROTECTION_NO); /* ProtectionStatus */

		if (partitions[partition_id].objlist[object_id].handle.type == FS_DIR_ENTRY_DIR) {
			net_buf_add_le32(buf, 0xFFFFFFFFUL); /* ObjectCompressedSize */
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
		if (partitions[partition_id].objlist[object_id].handle.parent_id == 0xff) {
			LOG_DBG("%s in root", partitions[partition_id].objlist[object_id].name);
			net_buf_add_le32(buf,
					 0xFFFFFFFFUL); /* ParentObject (0xFF if Object in Root) */
		} else {
			LOG_DBG("%s in parent %x", partitions[partition_id].objlist[object_id].name,
				partitions[partition_id].objlist[object_id].handle.parent_id);
			net_buf_add_le32(buf, partitions[partition_id]
						      .objlist[object_id]
						      .handle.parent_id); /* ParentObject */
		}
		net_buf_add_le16(buf, 0x0001); /* AssociationType */
		net_buf_add_le32(buf, 0);      /* AssociationDesc */
		net_buf_add_le32(buf, 0);      /* SequenceNumber */

		mtp_buf_add_string(buf, filename);      /* FileName */
		mtp_buf_add_string(buf, data_created);  /* DateCreated */
		mtp_buf_add_string(buf, data_modified); /* DateModified */
		net_buf_add_u8(buf, 0);                 /* KeywordsLength, always 0 unused */

	} else {
		LOG_ERR("Unknown ID %08x", obj_handle.value);
	}

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT)
{
	mtp_object_handle_t obj_handle;
	uint8_t partition_id;
	uint8_t object_id;
	uint32_t available_buf_len = ctx->max_packet_size;
	int err = 0;

	if (ctx->op_state.phase == MTP_PHASE_INIT) {
		LOG_DBG("\n\t\tParam0: 0x%x"
			"\n\t\tParam1: 0x%x"
			"\n\t\tParam2: 0x%x",
			mtp_command->param[0], mtp_command->param[1], mtp_command->param[2]);

		obj_handle = (mtp_object_handle_t)mtp_command->param[0];
		partition_id = obj_handle.partition_id;
		object_id = obj_handle.object_id;

		if (partition_id == 0 || partition_id >= ARRAY_SIZE(partitions)) {
			LOG_ERR("Unknown storage id %x", partition_id);
			mark_op_complete(ctx, MTP_RESP_INVALID_STORAGE_ID);
		}

		if (object_id >= partitions[partition_id].files_count) {
			LOG_ERR("Unknown object id %x", object_id);
			mark_op_complete(ctx, MTP_RESP_INVALID_OBJECT_HANDLE);
		}

		memset(ctx->transfer_state.filepath, 0x00, MAX_PATH_LEN);
		traverse_path(&partitions[partition_id].objlist[object_id],
			      ctx->transfer_state.filepath);

		fs_file_t_init(&ctx->transfer_state.file);
		err = fs_open(&ctx->transfer_state.file, ctx->transfer_state.filepath, FS_O_READ);
		if (err) {
			LOG_ERR("Failed to open %s (%d)", ctx->transfer_state.filepath, err);
			return;
		}

		LOG_DBG("Traversed Path: %s", ctx->transfer_state.filepath);
		ctx->transfer_state.total_size = partitions[partition_id].objlist[object_id].size;
		ctx->transfer_state.transferred = 0;
		ctx->transfer_state.chunks_sent = 0;
		ctx->op_state.phase = MTP_PHASE_DATA;
	}

	if (ctx->op_state.phase != MTP_PHASE_DATA) {
		return;
	}

	if (ctx->transfer_state.transferred == 0) {
		available_buf_len = ctx->max_packet_size - sizeof(struct mtp_header);

		/* Add the Packet Header */
		data_header_push(buf, mtp_command, ctx->transfer_state.total_size);
	}

	LOG_DBG("Sending file: %s size: %u", ctx->transfer_state.filepath,
		ctx->transfer_state.total_size);

	int read = fs_read(&ctx->transfer_state.file, ctx->filebuf, available_buf_len);

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
		LOG_DBG("Done, CONFIRMING");
		fs_close(&ctx->transfer_state.file);
		memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		mark_op_complete(ctx, MTP_RESP_OK);
	} else {
		LOG_DBG("Continue Next");
		ctx->op_state.phase = MTP_PHASE_DATA;
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT_INFO)
{
	uint32_t dest_partition_id;
	uint32_t dest_parent_id;
	uint16_t ObjectFormat;
	uint8_t filename_len;
	struct mtp_object_t *new_obj;
	struct fs_statvfs fs_stat;
	uint16_t err_code = MTP_RESP_OK;
	int32_t ret = 0;

	/* Process the request only after receiving the 2 packets.
	 * First packet: contains only destination storageID, ParentID.
	 * Second packet: contains the rest of the info.
	 * Reply to host only after the second packet
	 */
	if (ctx->op_state.phase == MTP_PHASE_INIT) {
		LOG_DBG("\n\t\tDest StorageID: 0x%x"
			"\n\t\tDest ParentHandle: 0x%x",
			mtp_command->param[0], mtp_command->param[1]);

		ctx->op_state.args[0] = mtp_command->param[0];
		ctx->op_state.args[1] = mtp_command->param[1];

		ctx->op_state.phase = MTP_PHASE_WAITING_MORE_INFO;
		return;
	}

	dest_partition_id = ((mtp_storage_id_t)(ctx->op_state.args[0])).id;
	dest_parent_id = ((mtp_object_handle_t)(ctx->op_state.args[1])).object_id;

	/* Initial checks */
	if (ctx->op_state.phase != MTP_PHASE_WAITING_MORE_INFO) {
		LOG_ERR("Invalid phase %d, expected %d", ctx->op_state.phase,
			MTP_PHASE_WAITING_MORE_INFO);
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

	uint32_t new_obj_id = partitions[dest_partition_id].files_count++;

	new_obj = &partitions[dest_partition_id].objlist[new_obj_id];
	new_obj->handle.object_id = new_obj_id;
	new_obj->handle.type = FS_DIR_ENTRY_FILE;
	new_obj->handle.partition_id = dest_partition_id;
	new_obj->handle.parent_id = (dest_parent_id == 0xffffffff ? 0xff : dest_parent_id);

	LOG_DBG("New ObjHandle:  0x%08x", new_obj->handle.value);

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
		LOG_ERR("Invalid filename length %u", filename_len);
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
	struct mtp_container mtp_response = {
		.hdr = {
				.length = sizeof(struct mtp_header) +
					  (3 * sizeof(mtp_response.param[0])),
				.type = MTP_CONTAINER_RESPONSE,
				.code = err_code,
				.transaction_id = mtp_command->hdr.transaction_id,
			},
		.param = {0, 0, 0}};

	if (new_obj && err_code == MTP_RESP_OK) {
		mtp_response.param[0] = GENERATE_STORAGE_ID(new_obj->handle.partition_id),
		mtp_response.param[1] = (new_obj->handle.parent_id == 0xff
						 ? 0xffffffff
						 : partitions[new_obj->handle.partition_id]
							   .objlist[new_obj->handle.parent_id]
							   .handle.value),
		mtp_response.param[2] = new_obj->handle.value;
	}

	LOG_DBG("Sent info:\n\tSID: %x\n\tPID: %x\n\tOID: %x", mtp_response.param[0],
		mtp_response.param[1], mtp_response.param[2]);

	net_buf_add_mem(buf, &mtp_response, 24);
	memset(&ctx->op_state, 0x00, sizeof(ctx->op_state));
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT)
{
	int ret = 0;

	if (ctx->op_state.phase == MTP_PHASE_INIT) {
		if (mtp_command->hdr.type == MTP_CONTAINER_COMMAND) {
			LOG_DBG("COMMAND RECEIVED len: %u", payload->len);
			return;
		} else if (mtp_command->hdr.type == MTP_CONTAINER_DATA) {
			LOG_DBG("DATA RECEIVED len: %u", payload->len);
			net_buf_pull_mem(payload, sizeof(struct mtp_header)); /* SKIP The header */
		}
	}

	ret = fs_write(&ctx->transfer_state.file, payload->data, payload->len);
	if (ret < 0) {
		LOG_ERR("Failed to write data to file %s (%d)", ctx->transfer_state.filepath, ret);
		mark_op_complete(ctx, MTP_RESP_STORE_NOT_AVAILABLE);
		mtp_send_response(ctx, buf);
		return;
	}

	ctx->transfer_state.chunks_sent++;
	ctx->transfer_state.transferred += payload->len;
	LOG_DBG("SEND_OBJECT: Data len: %u out of %u", ctx->transfer_state.transferred,
		ctx->transfer_state.total_size);

	if (ctx->transfer_state.transferred >= ctx->transfer_state.total_size) {
		fs_close(&ctx->transfer_state.file);
		LOG_DBG("SEND_OBJECT: All data received (%u bytes), Sending Confirmation",
			ctx->transfer_state.transferred);

		memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		mark_op_complete(ctx, MTP_RESP_OK);
		mtp_send_response(ctx, buf);
	} else {
		ctx->op_state.phase = MTP_PHASE_DATA;
	}
}

MTP_CMD_HANDLER(MTP_OP_DELETE_OBJECT)
{
	mtp_object_handle_t obj_handle = (mtp_object_handle_t)mtp_command->param[0];
	uint32_t partition_id = obj_handle.partition_id;
	uint32_t object_id = obj_handle.object_id;
	uint16_t err_code = MTP_RESP_OK;

	char path[MAX_PATH_LEN];

	memset(path, 0x00, MAX_PATH_LEN);
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
		memset(&partitions[partition_id].objlist[object_id], 0x00,
		       sizeof(struct mtp_object_t));
#endif
	} else {
		LOG_WRN("Read only partition %u", partition_id);
		err_code = MTP_RESP_STORE_READ_ONLY;
	}

	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = err_code,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_REFERENCES)
{
	uint32_t objcount = 0;

	for (int i = 0; i < partitions[1].files_count; i++) {
		if (partitions[1].objlist[i].handle.parent_id == 0xff) {
			objcount++;
			net_buf_add_le32(buf, partitions[1].objlist[i].handle.value);
		}
	}

	net_buf_push_le32(buf, objcount);

	data_header_push(buf, mtp_command, buf->len);

	mark_op_complete(ctx, MTP_RESP_OK);
}

/* Check if the MTP context is in a state where it awaits more data or have a pending packet to be
 * sent to host.
 */
bool mtp_packet_pending(struct mtp_context *ctx)
{
	return (ctx->op_state.phase != MTP_PHASE_INIT);
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

	if (ctx->op_state.phase == MTP_PHASE_CANCELED) {
		LOG_ERR("Unexpected cmd while an operation cancelling is in progress");
		return -EBUSY;
	}

	if (ctx->op_state.phase == MTP_PHASE_DATA) {
		switch (ctx->op_state.code) {
		case MTP_OP_SEND_OBJECT:
			MTP_CMD(MTP_OP_SEND_OBJECT);
			return buf_resp->len;
		case MTP_OP_GET_OBJECT:
			MTP_CMD(MTP_OP_GET_OBJECT);
			return buf_resp->len;
		default:
			LOG_ERR("Invalid phase %d for op_code %d", ctx->op_state.phase,
				ctx->op_state.code);
			return -EINVAL;
		}
	}

	if (ctx->op_state.phase == MTP_PHASE_COMPLETE) {
		LOG_WRN("Confirmation sent, phase: %d", ctx->op_state.phase);
		mtp_send_response(ctx, buf_resp);
		return buf_resp->len;
	}

	ctx->op_state.code = mtp_command->hdr.code;
	LOG_DBG(GREEN "[%s]" RESET, mtp_code_to_string(mtp_command->hdr.code));

	switch (mtp_command->hdr.code) {
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
	case MTP_OP_GET_OBJECT_REFERENCES:
		MTP_CMD(MTP_OP_GET_OBJECT_REFERENCES);
		break;
	default:
		LOG_ERR("Not supported cmd 0x%x!", mtp_command->hdr.code);

		struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
						  .type = MTP_CONTAINER_RESPONSE,
						  .code = MTP_RESP_OPERATION_NOT_SUPPORTED,
						  .transaction_id =
							  mtp_command->hdr.transaction_id};

		net_buf_add_mem(buf_resp, &mtp_response, sizeof(struct mtp_header));
		break;
	}

	return buf_resp->len;
}

int mtp_control_to_host(struct mtp_context *ctx, uint8_t request, struct net_buf *const buf)
{
	struct mtp_device_status_t dev_status;

	switch (request) {
	case MTP_REQUEST_GET_DEVICE_STATUS:
		if (ctx->op_state.phase == MTP_PHASE_CANCELED) {
			dev_status.len = 6;
			dev_status.code = MTP_RESP_TRANSACTION_CANCELLED;
			dev_status.epIn = 0x81;
			dev_status.epOut = 0x01;
			net_buf_add_mem(buf, &dev_status, dev_status.len);
		} else {
			dev_status.len = 4;
			dev_status.code = MTP_RESP_OK;
			net_buf_add_mem(buf, &dev_status, dev_status.len);
		}

		ctx->op_state.phase = MTP_PHASE_INIT;
		break;
	default:
		LOG_ERR("Unknown Host request 0x%x!", request);
		break;
	}

	return 0;
}

int mtp_control_to_dev(struct mtp_context *ctx, uint8_t request, const struct net_buf *const buf)
{
	switch (request) {
	case MTP_REQUEST_CANCEL:
		ctx->op_state.phase = MTP_PHASE_CANCELED;
		LOG_WRN("Closing incomplete file %s", ctx->transfer_state.filepath);
		if (strnlen(ctx->transfer_state.filepath, MAX_PATH_LEN) > 0) {
			fs_close(&ctx->transfer_state.file);

			/* Delete the opened file only when downloading from Host */
			if (ctx->op_state.code == MTP_OP_SEND_OBJECT) {
				fs_unlink(ctx->transfer_state.filepath);
			}

			memset(ctx->filebuf, 0x00, sizeof(ctx->filebuf));
			memset(&ctx->transfer_state, 0x00, sizeof(ctx->transfer_state));
		}
		break;

	case MTP_REQUEST_DEVICE_RESET:
		LOG_WRN("MTP_REQUEST_DEVICE_RESET");
		mtp_reset(ctx);
		break;

	default:
		LOG_ERR("Unknown Dev request 0x%x!", request);
		break;
	}

	return 0;
}

static int mtp_send_response(struct mtp_context *ctx, struct net_buf *buf)
{
	if (buf == NULL) {
		LOG_ERR("%s: Null Buffer!", __func__);
		return -EINVAL;
	}

	LOG_DBG("Sending Confirmation (0x%x)", ctx->op_state.err);
	struct mtp_container *mtp_command = (struct mtp_container *)buf->data;
	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = ctx->op_state.err,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
	if (buf->len <= 0) {
		LOG_ERR("Failed to send MTP confirmation");
	}

	ctx->op_state.phase = MTP_PHASE_INIT;
	return 0;
}

void mtp_reset(struct mtp_context *ctx)
{
	for (int i = 1; i < ARRAY_SIZE(partitions); i++) {
		memset(partitions[i].objlist, 0x00, sizeof(partitions[i].objlist));
		partitions[i].files_count = 0;
	}
	ctx->session_opened = false;
	memset(&ctx->op_state, 0x00, sizeof(ctx->op_state));
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
				partitions[partitionIdx].objlist[i].name);
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
