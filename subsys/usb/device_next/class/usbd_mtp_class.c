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

#define INTERNAL_STORAGE_ID(id)  (0x00010000 + id)
#define REMOVABLE_STORAGE_ID(id) (0x00020000 + id)
#define MTP_STR_LEN(str)         (strlen(str) + 1)

#define GET_OBJECT_ID(objID)  ((uint8_t)(objID & 0xFF))
#define GET_TYPE(objID)       ((uint8_t)((objID >> 8) & 0xFF))
#define GET_PARENT_ID(objID)  ((uint8_t)((objID >> 16) & 0xFF))
#define GET_STORAGE_ID(objID) ((uint8_t)((objID >> 24) & 0xFF))

#define MTP_CMD(opcode) mtp_##opcode(ctx, mtp_command, payload, buf_resp)

#define MTP_CMD_HANDLER(opcode)                                                                    \
	static void mtp_##opcode(struct mtp_context *ctx, struct mtp_container *mtp_command,       \
				 struct net_buf *payload, struct net_buf *buf)
#define PROCESS_FSTAB_ENTRY(node_id)                                                               \
	IF_ENABLED(DT_PROP(node_id, mtp_enabled),                                                  \
			({.mountpoint = DT_PROP(node_id, mount_point),	\
			.files_count = 1,				\
			.read_only = DT_PROP(node_id, read_only)},))

/* Used by MTP commands which reply to host over multiple packets
 * like GetObject (Sending file to host) or a command sending its result
 * in a separate packet
 */
#define DEV_TO_HOST_SET_PENDING_PACKET(ctx, fn) ctx->pending_fn = fn

/* Used by MTP commands which receive data from host over multiple packets
 * like SendObject (Receiving file from host) or a command receiving all required
 * info over multiple packets like SendObjectInfo
 */
#define HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, fn) ctx->extra_data_fn = fn

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

struct fs_object_t {
	union {
		struct {
			uint8_t object_id;
			uint8_t type;
			uint8_t parent_id;
			uint8_t storage_id;
		};
		uint32_t ID; /* Allows access to the entire ID as a single value */
	};
	uint32_t size;
	char name[MAX_FILE_NAME];
};

struct storage_t {
	const char mountpoint[10];
	struct fs_object_t filelist[CONFIG_USBD_MTP_MAX_HANDLES];
	uint16_t files_count;
	bool read_only;
};

struct dev_info_t {
	const char *manufacturer;
	const char *model;
	const char *device_version;
	const char *serial_number;
};

/* Local functions declarations */
static int mtp_send_confirmation(struct mtp_context *ctx, struct net_buf *buf);
static int continue_get_object(struct mtp_context *ctx, struct net_buf *buf);
static int continue_send_object(struct mtp_context *ctx, struct net_buf *buf_in);

/* Variables */
static struct storage_t storage[] = {{.mountpoint = "NULL"},
				     DT_FOREACH_CHILD(DT_PATH(fstab), PROCESS_FSTAB_ENTRY)};

static struct dev_info_t dev_info;

#define RESET   "\033[0m"
#define GREEN   "\033[32m" /* Green */
#define BLUE    "\033[34m" /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN    "\033[36m" /* Cyan */
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

int mtp_get_pending_packet(struct mtp_context *ctx, struct net_buf *buf)
{
	if (ctx->pending_fn) {
		return ctx->pending_fn(ctx, buf);
	} else {
		return -EINVAL;
	}
}

bool mtp_packet_pending(struct mtp_context *ctx)
{
	return (ctx->pending_fn != NULL);
}

static void usb_buf_add_utf16le(struct net_buf *buf, const char *str)
{
	uint16_t len = strlen(str) + 1; /* we need the null terminator */

	for (int i = 0; i < len; i++) {
		__ASSERT(str[i] > 0x1F && str[i] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB");
		net_buf_add_le16(buf, str[i]);
	}
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

static int dir_traverse(uint8_t storage_id, const char *root_path, uint32_t parent)
{
	char path[MAX_PATH_LEN + MAX_FILE_NAME];
	struct fs_dir_t dir;
	int err;
	struct storage_t *sstorage = &storage[storage_id];

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
		snprintf(path, sizeof(path), "%s/%s%s", root_path, entry.name,
			 (entry.type == FS_DIR_ENTRY_DIR) ? "/" : "");

		/* If it's a file, store the path in the array */
		if (sstorage->files_count < MAX_FILES) {
			strncpy(sstorage->filelist[sstorage->files_count].name, entry.name,
				MAX_FILE_NAME - 1);
			sstorage->filelist[sstorage->files_count].size = entry.size;
			sstorage->filelist[sstorage->files_count].type =
				entry.type; /* FS_DIR_ENTRY_FILE=0 or FS_DIR_ENTRY_DIR=1 */
			sstorage->filelist[sstorage->files_count].parent_id = parent;
			sstorage->filelist[sstorage->files_count].object_id = sstorage->files_count;
			sstorage->filelist[sstorage->files_count].storage_id = storage_id;
			sstorage->files_count++;

			if (entry.type == FS_DIR_ENTRY_DIR) {
				/* Recursive call to traverse subdirectory */
				err = dir_traverse(
					storage_id, path,
					sstorage->filelist[sstorage->files_count - 1].object_id);
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
		if (entry.type == FS_DIR_ENTRY_DIR) {
			snprintf(objpath, sizeof(objpath), "%s%s/", dirpath, entry.name);
			dir_delete(objpath);
		} else {
			snprintf(objpath, sizeof(objpath), "%s%s", dirpath, entry.name);
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

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.manufacturer)); /* manufacturer_len */
	usb_buf_add_utf16le(buf, dev_info.manufacturer);         /* manufacturer[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.model)); /* model_len; */
	usb_buf_add_utf16le(buf, dev_info.model);         /* model[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.device_version)); /* device_version_len; */
	usb_buf_add_utf16le(buf, dev_info.device_version);         /* device_version[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.serial_number)); /* serial_number_len; */
	usb_buf_add_utf16le(buf, dev_info.serial_number);         /* serial_number[] */

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
}

MTP_CMD_HANDLER(MTP_OP_OPEN_SESSION)
{
	uint16_t err_code = MTP_RESP_OK;

	if (ctx->session_opened == false) {
		for (int i = 1; i < ARRAY_SIZE(storage); i++) {
			if (dir_traverse(i, storage[i].mountpoint, 0xFFFFFFFF)) {
				LOG_ERR("Failed to traverse %s", storage[i].mountpoint);
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
	uint32_t requested_storage_id = mtp_command->param[0] & 0x0F;

	LOG_DBG("\n\t\tStorageID    : 0x%x\n", requested_storage_id);

	if (requested_storage_id == 0) {
		LOG_ERR("Unknown Storage ID %x", requested_storage_id);
	}

	struct fs_statvfs stat;
	int err = fs_statvfs(storage[requested_storage_id].mountpoint, &stat);

	if (err < 0) {
		LOG_ERR("Failed to statvfs %s (%d)", storage[requested_storage_id].mountpoint, err);
		return;
	}

	const char *storage_name = storage[requested_storage_id].mountpoint;

	if (storage_name[0] == '/') {
		/* skip the slash */
		storage_name++;
	}

	net_buf_add_le16(buf, STORAGE_TYPE_FIXED_RAM);       /* type */
	net_buf_add_le16(buf, FS_TYPE_GENERIC_HIERARCHICAL); /* fs_type */
	if (storage[requested_storage_id].read_only) {
		net_buf_add_le16(buf, OBJECT_PROTECTION_READ_ONLY); /* access_caps */
	} else {
		net_buf_add_le16(buf, OBJECT_PROTECTION_NO);
	}
	net_buf_add_le64(buf, (stat.f_blocks * stat.f_frsize)); /* max_capacity */
	net_buf_add_le64(buf, (stat.f_bfree * stat.f_frsize));  /* free_space */
	net_buf_add_le32(buf, 0xFFFFFFFF);                      /* free_space_obj */
	net_buf_add_u8(buf, MTP_STR_LEN(storage_name));         /* storage_desc_len */
	usb_buf_add_utf16le(buf, storage_name);                 /* storage_desc[] */
	net_buf_add_u8(buf, 0);                                 /* volume_id_len, Unused */

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
}

MTP_CMD_HANDLER(MTP_OP_GET_STORAGE_IDS)
{
	net_buf_add_le32(buf, ARRAY_SIZE(storage) - 1); /* Number of storage */
	for (int i = 1; i < ARRAY_SIZE(storage); i++) {
		net_buf_add_le32(
			buf, INTERNAL_STORAGE_ID(
				     i)); /* Use array index as Storage ID, 0x00 can't be used */
	}

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_HANDLES)
{
	uint32_t storage_id = mtp_command->param[0] & 0x0F;
	uint32_t obj_format_code = mtp_command->param[1];
	uint32_t obj_handle = mtp_command->param[2];

	LOG_DBG("\n\t\tStorageID    : 0x%x"
		"\n\t\tObjFormatCode: 0x%x"
		"\n\t\tObjHandle    : 0x%x\n",
		storage_id, obj_format_code, obj_handle);

	uint32_t found_files = 0;
	uint32_t parent_id = (obj_handle == 0xffffffff ? 0xff : GET_OBJECT_ID(obj_handle));

	for (int i = 0; i < storage[storage_id].files_count; ++i) {
		if (storage[storage_id].filelist[i].parent_id == parent_id) {
			net_buf_add_le32(buf, storage[storage_id].filelist[i].ID);
			found_files++;
		}
	}
	net_buf_push_mem(buf, &found_files, sizeof(uint32_t));

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT_INFO)
{
	uint32_t obj_handle = mtp_command->param[0];
	uint8_t storage_id = GET_STORAGE_ID(obj_handle);
	uint8_t object_id = GET_OBJECT_ID(obj_handle);

	LOG_DBG("\n\t\tObjHandle: 0x%x, SID: %x, OID: %x", mtp_command->param[0], storage_id,
		object_id);

	if (storage[storage_id].filelist[object_id].ID == obj_handle) {
		char *filename = storage[storage_id].filelist[object_id].name;
		char *data_created = "20250101T120000";
		char *data_modified = "20250101T120000";

		net_buf_add_le32(buf, INTERNAL_STORAGE_ID(storage_id)); /* StorageID */

		if (storage[storage_id].filelist[object_id].type == 1) {
			net_buf_add_le16(buf, MTP_FORMAT_ASSOCIATION); /* ObjectFormat */
		} else {
			net_buf_add_le16(buf, MTP_FORMAT_UNDEFINED); /* ObjectFormat */
		}

		net_buf_add_le16(buf, OBJECT_PROTECTION_NO); /* ProtectionStatus */

		if (storage[storage_id].filelist[object_id].type == 1) {
			net_buf_add_le32(buf, 0xFFFFFFFFUL); /* ObjectCompressedSize */
		} else {
			net_buf_add_le32(buf, storage[storage_id].filelist[object_id].size);
		}
		net_buf_add_le16(buf, 0); /* ThumbFormat */
		net_buf_add_le32(buf, 0); /* ThumbCompressedSize */
		net_buf_add_le32(buf, 0); /* ThumbPixWidth */
		net_buf_add_le32(buf, 0); /* ThumbPixHeight */
		net_buf_add_le32(buf, 0); /* ImagePixWidth */
		net_buf_add_le32(buf, 0); /* ImagePixHeight */
		net_buf_add_le32(buf, 0); /* ImageBitDepth */
		if (storage[storage_id].filelist[object_id].parent_id == 0xff) {
			LOG_DBG("%s in root", storage[storage_id].filelist[object_id].name);
			net_buf_add_le32(buf,
					 0xFFFFFFFFUL); /* ParentObject (0xFF if Object in Root) */
		} else {
			LOG_DBG("%s in parent %x", storage[storage_id].filelist[object_id].name,
				storage[storage_id].filelist[object_id].parent_id);
			net_buf_add_le32(buf, storage[storage_id]
						      .filelist[object_id]
						      .parent_id); /* ParentObject */
		}
		net_buf_add_le16(buf, 0x0001); /* AssociationType */
		net_buf_add_le32(buf, 0);      /* AssociationDesc */
		net_buf_add_le32(buf, 0);      /* SequenceNumber */

		net_buf_add_u8(buf, MTP_STR_LEN(filename)); /* FileNameLength */
		usb_buf_add_utf16le(buf, filename);         /* FileName */

		net_buf_add_u8(buf, MTP_STR_LEN(data_created)); /* DateCreatedLength */
		usb_buf_add_utf16le(buf, data_created);         /* DateCreated */

		net_buf_add_u8(buf, MTP_STR_LEN(data_modified)); /* DateModifiedLength */
		usb_buf_add_utf16le(buf, data_modified);         /* DateModified */

		net_buf_add_u8(buf, 0); /* KeywordsLength, always 0 unused */

	} else {
		LOG_ERR("Unknown ID %08x", obj_handle);
	}

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
}

static int continue_get_object(struct mtp_context *ctx, struct net_buf *buf)
{
	int len = 0;
	int total_chunks = (ctx->filestate.total_size / ctx->max_packet_size);

	memset(ctx->filebuf, 0x00, sizeof(ctx->filebuf));

	if (ctx->filestate.transferred < ctx->filestate.total_size) {
		len = MIN(ctx->max_packet_size,
			  (ctx->filestate.total_size - ctx->filestate.transferred));

		int read = fs_read(&ctx->filestate.file, ctx->filebuf, len);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, ctx->filebuf, read);

		ctx->filestate.transferred += len;
		ctx->filestate.chunks_sent++;
		LOG_DBG("sent [%u of %u]: %u, remaining %u", ctx->filestate.chunks_sent,
			total_chunks, ctx->filestate.transferred,
			(ctx->filestate.total_size - ctx->filestate.transferred));

		if (ctx->filestate.transferred >= ctx->filestate.total_size) {
			LOG_DBG("Done (%u), CONFIRMING", read);
			fs_close(&ctx->filestate.file);
			memset(&ctx->filestate, 0x00, sizeof(ctx->filestate));
			DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
		} else {
			LOG_DBG("Continue (%u) Next", read);
			DEV_TO_HOST_SET_PENDING_PACKET(ctx, continue_get_object);
		}
	} else {
		LOG_ERR("Shouldn't happen");
		__ASSERT_NO_MSG(false);
	}
	return 0;
}

static int traverse_path(struct fs_object_t *obj, uint8_t *buf)
{
	if (obj->parent_id != 0xff) {
		int off = traverse_path(&storage[obj->storage_id].filelist[obj->parent_id], buf);

		return snprintf(&buf[off], MAX_PATH_LEN, "%s", obj->name);
	} else {
		return snprintf(buf, MAX_PATH_LEN, "%s/%s%s", storage[obj->storage_id].mountpoint,
				obj->name, (obj->type == FS_DIR_ENTRY_DIR) ? "/" : "");
	}
}

MTP_CMD_HANDLER(MTP_OP_GET_OBJECT)
{
	LOG_DBG("\n\t\tParam0: 0x%x"
		"\n\t\tParam1: 0x%x"
		"\n\t\tParam2: 0x%x",
		mtp_command->param[0], mtp_command->param[1], mtp_command->param[2]);

	uint32_t obj_handle = mtp_command->param[0];
	uint8_t storage_id = GET_STORAGE_ID(obj_handle);
	uint8_t object_id = GET_OBJECT_ID(obj_handle);
	char path[MAX_PATH_LEN];
	int err = 0;

	memset(path, 0x00, MAX_PATH_LEN);
	traverse_path(&storage[storage_id].filelist[object_id], path);
	LOG_DBG(">>>> Traversed Path: %s", path);

	fs_file_t_init(&ctx->filestate.file);

	err = fs_open(&ctx->filestate.file, path, FS_O_READ);
	if (err) {
		LOG_ERR("Failed to open %s (%d)", path, err);
		return;
	}

	memcpy(ctx->filestate.filepath, path, MAX_PATH_LEN);

	uint32_t filesize = storage[storage_id].filelist[object_id].size;
	uint32_t available_buf_len = ctx->max_packet_size - sizeof(struct mtp_header);

	LOG_DBG("Sending file: %s size: %u", path, storage[storage_id].filelist[object_id].size);

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, storage[storage_id].filelist[object_id].size);

	if (storage[storage_id].filelist[object_id].size > available_buf_len) {
		int read = fs_read(&ctx->filestate.file, ctx->filebuf, available_buf_len);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, ctx->filebuf, read);

		ctx->filestate.total_size = filesize;
		ctx->filestate.transferred = read;
		DEV_TO_HOST_SET_PENDING_PACKET(ctx, continue_get_object);
	} else {
		int read = fs_read(&ctx->filestate.file, ctx->filebuf, filesize);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, ctx->filebuf, read);

		fs_close(&ctx->filestate.file);
		memset(&ctx->filestate, 0x00, sizeof(ctx->filestate));
		DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT_INFO)
{
	int ret = 0;
	static struct fs_object_t *fs_obj;

	/* first packet received from Host contains only:
	 * destination storageID and Destination ParentID
	 */
	if (fs_obj == NULL) {
		LOG_DBG("\n\t\tDest StorageID: 0x%x"
			"\n\t\tDest ParentHandle: 0x%x, resolved 0x%x (%s)",
			mtp_command->param[0], mtp_command->param[1],
			GET_OBJECT_ID(mtp_command->param[1]),
			storage[GET_STORAGE_ID(mtp_command->param[0])]
				.filelist[GET_OBJECT_ID(mtp_command->param[1])]
				.name);

		uint32_t dest_storage_id = mtp_command->param[0] & 0x0F;
		uint32_t dest_parent_handle = mtp_command->param[1];

		if (dest_storage_id != 0 && dest_storage_id < ARRAY_SIZE(storage)) {
			if ((storage[dest_storage_id].files_count + 1) < MAX_FILES) {
				uint32_t new_obj_id = storage[dest_storage_id].files_count++;

				fs_obj = &storage[dest_storage_id].filelist[new_obj_id];

				fs_obj->object_id = new_obj_id;
				fs_obj->type = 0;
				fs_obj->parent_id = (dest_parent_handle == 0xffffffff
							     ? 0xff
							     : GET_OBJECT_ID(dest_parent_handle));
				fs_obj->storage_id = dest_storage_id;

				LOG_DBG("New ObjID:  0x%08x", fs_obj->ID);
			} else {
				LOG_ERR("No file handle available %u",
					storage[dest_storage_id].files_count);
			}
		} else {
			LOG_ERR("Unknown storage id %x", dest_storage_id);
		}
	} else { /* Host sent more info */
		char filepath[MAX_PATH_LEN + MAX_FILE_NAME];
		char *filename = fs_obj->name;
		uint8_t str_len = 0;
		uint16_t err_code = MTP_RESP_OK;

		net_buf_pull(payload, sizeof(struct mtp_header));   /* SKIP the header */
		net_buf_pull_le32(payload);                         /* StorageID, always 0 ignore */
		uint16_t ObjectFormat = net_buf_pull_le16(payload); /* ObjectFormat */

		if (ObjectFormat == MTP_FORMAT_ASSOCIATION) {
			fs_obj->type = FS_DIR_ENTRY_DIR;
		}
		net_buf_pull_le16(payload);                /* ProtectionStatus */
		fs_obj->size = net_buf_pull_le32(payload); /* ObjectCompressedSize */

		net_buf_pull_le16(payload); /* ThumbFormat */
		net_buf_pull_le32(payload); /* ThumbCompressedSize */
		net_buf_pull_le32(payload); /* ThumbPixWidth */
		net_buf_pull_le32(payload); /* ThumbPixHeight */
		net_buf_pull_le32(payload); /* ImagePixWidth */
		net_buf_pull_le32(payload); /* ImagePixHeight */
		net_buf_pull_le32(payload); /* ImageBitDepth */
		uint32_t ParentObject =
			net_buf_pull_le32(payload); /* ParentObject (0xFFFF if Object in Root) */
		net_buf_pull_le16(payload);         /* AssociationType */
		net_buf_pull_le32(payload);         /* AssociationDesc */
		net_buf_pull_le32(payload);         /* SequenceNumber */
		str_len = net_buf_pull_u8(payload); /* FileNameLength */
		usb_buf_pull_utf16le(payload, filename, str_len); /* FileName */
		/* Rest of props are ignored */
		if (str_len > MAX_FILE_NAME) {
			LOG_ERR("Invalid filename length %u", str_len);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto fail;
		}

		struct fs_statvfs stat;

		if (storage[fs_obj->storage_id].read_only) {
			LOG_ERR("Storage %u is read-only", fs_obj->storage_id);
			err_code = MTP_RESP_STORE_READ_ONLY;
			goto fail;
		}

		ret = fs_statvfs(storage[fs_obj->storage_id].mountpoint, &stat);
		if (ret < 0) {
			LOG_ERR("Failed to statvfs %s (%d)", storage[fs_obj->storage_id].mountpoint,
				ret);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto fail;
		}

		if (fs_obj->size > (stat.f_bfree * stat.f_frsize)) {
			LOG_ERR("Not enough space to store file %u > %lu", fs_obj->size,
				(stat.f_bfree * stat.f_frsize));
			err_code = MTP_RESP_STORAGE_FULL;
			goto fail;
		}

		if (fs_obj->parent_id == 0xff) {
			snprintf(filepath, sizeof(filepath), "%s/%s",
				 storage[fs_obj->storage_id].mountpoint, filename);
		} else {
			snprintf(filepath, sizeof(filepath), "%s/%s/%s",
				 storage[fs_obj->storage_id].mountpoint,
				 storage[fs_obj->storage_id].filelist[fs_obj->parent_id].name,
				 filename);
		}

		LOG_DBG("\noFormat: %x, size: %d, parent: %x", ObjectFormat, fs_obj->size,
			ParentObject);
		LOG_DBG("mnt: %s", storage[fs_obj->storage_id].mountpoint);
		LOG_DBG("fname: %s", filename);
		LOG_DBG("path: %s ID:%x", filepath, fs_obj->ID);
		LOG_DBG("parentID: %u", fs_obj->parent_id);
		LOG_DBG("parentPath:%s",
			storage[fs_obj->storage_id].filelist[fs_obj->parent_id].name);

		if (fs_obj->type == FS_DIR_ENTRY_DIR) {
			ret = fs_mkdir(filepath);
			if (ret) {
				LOG_ERR("Failed to create directory %s (%d)", filepath, ret);
				err_code = MTP_RESP_GENERAL_ERROR;
			}
		} else {
			fs_file_t_init(&ctx->filestate.file);
			ret = fs_open(&ctx->filestate.file, filepath, FS_O_CREATE | FS_O_WRITE);
			if (ret) {
				LOG_ERR("Open file failed, %d", ret);
				err_code = MTP_RESP_GENERAL_ERROR;
			} else {
				ctx->filestate.total_size = fs_obj->size;
				memcpy(ctx->filestate.filepath, filepath, MAX_PATH_LEN);
			}
		}

fail:
		struct mtp_container mtp_response = {
			.hdr = {
					.length = 24,
					.type = MTP_CONTAINER_RESPONSE,
					.code = err_code,
					.transaction_id = mtp_command->hdr.transaction_id,
				},
			.param[0] = INTERNAL_STORAGE_ID(fs_obj->storage_id),
			.param[1] =
				(fs_obj->parent_id == 0xff ? 0xffffffff
							   : storage[fs_obj->storage_id]
								     .filelist[fs_obj->parent_id]
								     .ID),
			.param[2] = fs_obj->ID};

		LOG_DBG("Sent info:\n\tSID: %x\n\tPID: %x\n\tOID: %x", mtp_response.param[0],
			mtp_response.param[1], mtp_response.param[2]);

		net_buf_add_mem(buf, &mtp_response, 24);
		fs_obj = NULL;
	}
}

static int continue_send_object(struct mtp_context *ctx, struct net_buf *buf_in)
{
	ctx->filestate.transferred += buf_in->len;
	ctx->filestate.chunks_sent++;

	fs_write(&ctx->filestate.file, buf_in->data, buf_in->len);
	LOG_DBG("EXTRA: Data len: %u out of %u", ctx->filestate.transferred,
		ctx->filestate.total_size);

	if (ctx->filestate.transferred >= ctx->filestate.total_size) {
		fs_close(&ctx->filestate.file);
		LOG_DBG("Sending Confirmation after reciving data (Total len: %u)",
			ctx->filestate.transferred);

		HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, NULL);
		memset(&ctx->filestate, 0x00, sizeof(ctx->filestate));
		DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
		return 1;
	}

	HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, continue_send_object);
	return 0;
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT)
{
	if (mtp_command->hdr.type == MTP_CONTAINER_COMMAND) {
		LOG_DBG("COMMAND RECEIVED len: %u", payload->len);
	} else if (mtp_command->hdr.type == MTP_CONTAINER_DATA) {
		LOG_DBG("DATA RECEIVED len: %u", payload->len);
		net_buf_pull_mem(payload, sizeof(struct mtp_header)); /* SKIP The header */
		fs_write(&ctx->filestate.file, payload->data, payload->len);

		ctx->filestate.chunks_sent++;
		ctx->filestate.transferred += payload->len;
		LOG_DBG("SEND_OBJECT: Data len: %u out of %u", ctx->filestate.transferred,
			ctx->filestate.total_size);

		if (ctx->filestate.transferred >= ctx->filestate.total_size) {
			fs_close(&ctx->filestate.file);
			LOG_DBG("SEND_OBJECT: Sending Confirmation after reciving data (Total len: "
				"%u)",
				ctx->filestate.transferred);

			memset(&ctx->filestate, 0x00, sizeof(ctx->filestate));
			HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, NULL);
			mtp_send_confirmation(ctx, buf);
		} else {
			HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, continue_send_object);
		}
	}
}

MTP_CMD_HANDLER(MTP_OP_DELETE_OBJECT)
{
	uint32_t storage_id = GET_STORAGE_ID(mtp_command->param[0]);
	uint32_t object_id = GET_OBJECT_ID(mtp_command->param[0]);
	uint16_t err_code = MTP_RESP_OK;

	char path[MAX_PATH_LEN];

	memset(path, 0x00, MAX_PATH_LEN);
	traverse_path(&storage[storage_id].filelist[object_id], path);
	LOG_DBG("Traversed Path: %s", path);

	if (storage[storage_id].read_only == false) {
		if (GET_TYPE(mtp_command->param[0]) == FS_DIR_ENTRY_DIR) {
			LOG_DBG("Deleting directory %s", path);
			dir_delete(path);
		} else {
			LOG_DBG("Deleting file %s", path);
			fs_unlink(path);
		}
	} else {
		LOG_WRN("Read only srorage %u", storage_id);
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

	for (int i = 0; i < storage[1].files_count; i++) {
		if (storage[1].filelist[i].parent_id == 0xff) {
			objcount++;
			net_buf_add_le32(buf, storage[1].filelist[i].ID);
		}
	}

	net_buf_push_le32(buf, objcount);

	data_header_push(buf, mtp_command, buf->len);

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, mtp_send_confirmation);
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
	int ret = 0;

	if (buf_resp == NULL) {
		LOG_ERR("%s: NULL Buffer", __func__);
		return -EINVAL;
	}

	/* if extra_data_fn is set, it means a command is waiting for more data from host
	 * this needs to be checked and handled first
	 */
	if (ctx->extra_data_fn) {
		ret = ctx->extra_data_fn(ctx, buf_in);
		if (ret >= 0) {
			if (ret == 1) {
				ctx->pending_fn(ctx, buf_resp);
			}
			return buf_resp->len;
		}
		LOG_ERR("Failed to handle extra data");
		return -EINVAL;
	}

	ctx->op_code = mtp_command->hdr.code;
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
		LOG_WRN("MTP_REQUEST_GET_DEVICE_STATUS");
		if (ctx->op_canceled) {
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

		ctx->op_canceled = false;
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
		LOG_WRN("Closing incomplete file %s", ctx->filestate.filepath);
		if (strlen(ctx->filestate.filepath) > 0) {
			fs_close(&ctx->filestate.file);

			/* Delete the opened file only when downloading from Host */
			if (ctx->op_code == MTP_OP_SEND_OBJECT) {
				fs_unlink(ctx->filestate.filepath);
			}

			memset(ctx->filebuf, 0x00, sizeof(ctx->filebuf));
			memset(&ctx->filestate, 0x00, sizeof(ctx->filestate));
		}
		DEV_TO_HOST_SET_PENDING_PACKET(ctx, NULL);
		HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, NULL);

		ctx->op_canceled = true;
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

static int mtp_send_confirmation(struct mtp_context *ctx, struct net_buf *buf)
{
	if (buf == NULL) {
		LOG_ERR("%s: Null Buffer!", __func__);
		return -EINVAL;
	}
	LOG_DBG("Sending Confirmation");
	struct mtp_container *mtp_command = (struct mtp_container *)buf->data;
	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = MTP_RESP_OK,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
	if (buf->len <= 0) {
		LOG_ERR("Failed to send MTP confirmation");
	}

	DEV_TO_HOST_SET_PENDING_PACKET(ctx, NULL);
	return 0;
}

void mtp_reset(struct mtp_context *ctx)
{
	for (int i = 1; i < ARRAY_SIZE(storage); i++) {
		memset(storage[i].filelist, 0x00, sizeof(storage[i].filelist));
		storage[i].files_count = 0;
	}
	ctx->session_opened = false;
	ctx->op_code = 0;
	ctx->op_canceled = false;
	HOST_TO_DEV_SET_CONT_DATA_HANDLER(ctx, NULL);
	DEV_TO_HOST_SET_PENDING_PACKET(ctx, NULL);
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
	for (int storageIdx = 1; storageIdx < ARRAY_SIZE(storage); storageIdx++) {
		LOG_INF("File list Storage %s", storage[storageIdx].mountpoint);
		for (int i = 0; i < storage[storageIdx].files_count; i++) {
			LOG_INF("ID: 0x%08x S: %02x, P: %02x, T:%s, O: %02x : %s",
				storage[storageIdx].filelist[i].ID,
				storage[storageIdx].filelist[i].storage_id,
				storage[storageIdx].filelist[i].parent_id,
				(storage[storageIdx].filelist[i].type == 1 ? "D" : "f"),
				storage[storageIdx].filelist[i].object_id,
				storage[storageIdx].filelist[i].name);
		}
		LOG_INF("\n\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mtp,
			       SHELL_CMD_ARG(list, NULL, "Create directory", cmd_mtp_list, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(mtp, &sub_mtp, "USB MTP commands", NULL);
#endif
