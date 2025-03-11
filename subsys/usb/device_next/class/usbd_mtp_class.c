/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <wctype.h>
#include <stddef.h>

#include <zephyr/net_buf.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_mtp_impl, CONFIG_USBD_MTP_LOG_LEVEL);

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
#define MAX_FILES                CONFIG_USBD_MTP_MAX_HANDLES /* Define the maximum number of files to store */
#define MAX_PATH_LEN             128
#define MAX_PACKET_SIZE          512 /* Get it in a usb complaint way */
/* Helpers */
#define INTERNAL_STORAGE_ID(id)  (0x00010000 + id)
#define REMOVABLE_STORAGE_ID(id) (0x00020000 + id)
#define MTP_GB(x)                (x * 1ULL * 1024 * 1024 * 1024)
#define MTP_STR_LEN(str)         (strlen(str) + 1)

#define GET_OBJECT_ID(objID)  ((uint8_t)(objID & 0xFF))
#define GET_TYPE(objID)       ((uint8_t)((objID >> 8) & 0xFF))
#define GET_PARENT_ID(objID)  ((uint8_t)((objID >> 16) & 0xFF))
#define GET_STORAGE_ID(objID) ((uint8_t)((objID >> 24) & 0xFF))

/* Functions dec/def */
static void mtp_reset();
#define MTP_CMD(opcode) mtp_##opcode(mtp_command, payload, buf)

#define MTP_CMD_HANDLER(opcode)                                                                    \
	static void mtp_##opcode(struct mtp_container *mtp_command, struct net_buf *payload,       \
				 struct net_buf *buf)
/* DTS */
#define PROCESS_FSTAB_ENTRY(node_id)                                                               \
	IF_ENABLED(DT_PROP(node_id, mtp_enabled),                                                  \
		   ({.mountpoint = DT_PROP(node_id, mount_point), .files_count = 1},))

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
	MTP_OP_SEND_OBJECT_INFO, MTP_OP_SEND_OBJECT
};

static const uint16_t events_supported[] = {MTP_EVENT_OBJECT_ADDED, MTP_EVENT_OBJECT_REMOVED};

static const uint16_t playback_formats[] = {
	MTP_FORMAT_UNDEFINED,
	MTP_FORMAT_ASSOCIATION,
};

struct mtp_context {
	bool session_opened;
	uint8_t filebuf[512]; /* TODO: should match USB Packet size */
	struct {
		struct fs_file_t file;
		char filepath[MAX_PATH_LEN];
		uint32_t total_size;
		uint32_t transferred;
		uint32_t chunks_sent;
		uint32_t storage_id;
	} filestate;
} mtp_ctx = {
	.session_opened = false,
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
};

struct dev_info_t {
	const char *manufacturer;
	const char *model;
	const char *device_version;
	const char *serial_number;
};

static struct storage_t storage[] = {{.mountpoint = "NULL"},
				     DT_FOREACH_CHILD(DT_PATH(fstab), PROCESS_FSTAB_ENTRY)};

static struct dev_info_t dev_info;
/********************************************************************* */
#if (CONFIG_USBD_MTP_LOG_LEVEL == 4)
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
#endif

static void net_buf_add_utf16le(struct net_buf *buf, const char *str)
{
	uint16_t len = strlen(str) + 1; /* we need the null terminator */

	for (int i = 0; i < len; i++) {
		__ASSERT(ascii7_str[i] > 0x1F && ascii7_str[i] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		net_buf_add_le16(buf, str[i]);
	}
}

static void net_buf_pull_utf16le(struct net_buf *buf, char *strbuf, size_t len)
{
	for (int i = 0; i < len; ++i) {
		strbuf[i] = net_buf_pull_u8(buf);
		net_buf_pull_u8(buf);
	}
}

static int mtp_send_confirmation(struct net_buf *buf);
/* ================== Pending packet handling ================ */
typedef int(pending_fn_t)(struct net_buf *buf);
static pending_fn_t *pending_fn;

static void set_pending_packet(pending_fn_t *pend_fn)
{
	pending_fn = pend_fn;
}

int send_pending_packet(struct net_buf *buf)
{
	if (pending_fn) {
		pending_fn_t *lpfn = pending_fn;

		pending_fn = NULL;
		return lpfn(buf);
	} else {
		return -EINVAL;
	}
}

bool mtp_packet_pending(void)
{
	return (pending_fn != NULL);
}

/* ===================== Extra Data needed handling =============== */
typedef int(extra_data_fn_t)(struct net_buf *buf, struct net_buf *buf_recv);
static extra_data_fn_t *extra_data_fn;

static bool more_data_needed;
static void set_needs_more_data(bool more_data)
{
	more_data_needed = more_data;
}

bool mtp_needs_more_data(struct net_buf *buf)
{
	bool val = more_data_needed;

	more_data_needed = false;
	return val;
}

int handle_extra_data(struct net_buf *buf, struct net_buf *buf_recv)
{
	if (extra_data_fn) {
		extra_data_fn_t *lpfn = extra_data_fn;

		extra_data_fn = NULL;
		return lpfn(buf, buf_recv);
	} else {
		return -EINVAL;
	}
}

void data_header_push(struct net_buf *buf, struct mtp_container *mtp_command, uint32_t data_len)
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
	char path[MAX_PATH_LEN];
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
				MAX_PATH_LEN - 1);
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
	char objpath[MAX_PATH_LEN * 2];

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
	net_buf_add_le32(buf, 0);   /* no props are used */

	/* Capture formats count */
	net_buf_add_le32(buf, 0);

	/* Playback formats supported */
	net_buf_add_le32(buf, ARRAY_SIZE(playback_formats));              /* count */
	net_buf_add_mem(buf, playback_formats, sizeof(playback_formats)); /* playback_formats[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.manufacturer)); /* manufacturer_len */
	net_buf_add_utf16le(buf, dev_info.manufacturer);         /* manufacturer[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.model)); /* model_len; */
	net_buf_add_utf16le(buf, dev_info.model);         /* model[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.device_version)); /* device_version_len; */
	net_buf_add_utf16le(buf, dev_info.device_version);         /* device_version[] */

	net_buf_add_u8(buf, MTP_STR_LEN(dev_info.serial_number)); /* serial_number_len; */
	net_buf_add_utf16le(buf, dev_info.serial_number);         /* serial_number[] */

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	set_pending_packet(mtp_send_confirmation);
}

MTP_CMD_HANDLER(MTP_OP_OPEN_SESSION)
{
	uint16_t err_code = MTP_RESP_OK;

	if (mtp_ctx.session_opened == false) {
		for (int i = 1; i < ARRAY_SIZE(storage); i++) {
			if (dir_traverse(i, storage[i].mountpoint, 0xFFFFFFFF)) {
				LOG_ERR("Failed to traverse %s", storage[i].mountpoint);
				err_code = MTP_RESP_GENERAL_ERROR;
				break;
			}
			/* TODO: Fail the MTP command if dir_traverse fails */
			mtp_ctx.session_opened = true;
		}
	} else {
		LOG_ERR("Session already open");
		err_code = MTP_RESP_SESSION_ALREADY_OPEN;
	}

	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = err_code,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
}

MTP_CMD_HANDLER(MTP_OP_CLOSE_SESSION)
{
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

	net_buf_add_le16(buf, STORAGE_TYPE_FIXED_RAM);          /* type */
	net_buf_add_le16(buf, FS_TYPE_GENERIC_HIERARCHICAL);    /* fs_type */
	net_buf_add_le16(buf, OBJECT_PROTECTION_NO);            /* access_caps */
	net_buf_add_le64(buf, (stat.f_blocks * stat.f_frsize)); /* max_capacity */
	net_buf_add_le64(buf, (stat.f_bfree * stat.f_frsize));  /* free_space */
	net_buf_add_le32(buf, 0xFFFFFFFF);                      /* free_space_obj */
	net_buf_add_u8(buf, MTP_STR_LEN(storage_name));         /* storage_desc_len */
	net_buf_add_utf16le(buf, storage_name);                 /* storage_desc[] */
	net_buf_add_u8(buf, 0);                                 /* volume_id_len, Unused */

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	set_pending_packet(mtp_send_confirmation);
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

	set_pending_packet(mtp_send_confirmation);
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

	set_pending_packet(mtp_send_confirmation);
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
		net_buf_add_utf16le(buf, filename);         /* FileName */

		net_buf_add_u8(buf, MTP_STR_LEN(data_created)); /* DateCreatedLength */
		net_buf_add_utf16le(buf, data_created);         /* DateCreated */

		net_buf_add_u8(buf, MTP_STR_LEN(data_modified)); /* DateModifiedLength */
		net_buf_add_utf16le(buf, data_modified);         /* DateModified */

		net_buf_add_u8(buf, 0); /* KeywordsLength, always 0 unused */

	} else {
		LOG_ERR("Unknown Error ID %08x", obj_handle);
	}

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, buf->len);

	set_pending_packet(mtp_send_confirmation);
}

static int continue_get_object(struct net_buf *buf)
{
	int len = 0;
	int total_chunks = (mtp_ctx.filestate.total_size / MAX_PACKET_SIZE);

	memset(mtp_ctx.filebuf, 0x00, 512);

	if (mtp_ctx.filestate.transferred < mtp_ctx.filestate.total_size) {
		len = MIN(MAX_PACKET_SIZE,
			  (mtp_ctx.filestate.total_size - mtp_ctx.filestate.transferred));

		int read = fs_read(&mtp_ctx.filestate.file, mtp_ctx.filebuf, len);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, mtp_ctx.filebuf, read);

		mtp_ctx.filestate.transferred += len;
		mtp_ctx.filestate.chunks_sent++;
		LOG_DBG("sent [%u of %u]: %u, remaining %u", mtp_ctx.filestate.chunks_sent,
			total_chunks, mtp_ctx.filestate.transferred,
			(mtp_ctx.filestate.total_size - mtp_ctx.filestate.transferred));

		if (mtp_ctx.filestate.transferred >= mtp_ctx.filestate.total_size) {
			LOG_DBG("Done (%u), CONFIRMING", read);
			fs_close(&mtp_ctx.filestate.file);
			memset(&mtp_ctx.filestate, 0x00, sizeof(mtp_ctx.filestate));
			set_pending_packet(mtp_send_confirmation);
		} else {
			LOG_DBG("Continue (%u) Next", read);
			set_pending_packet(continue_get_object);
		}
	} else {
		LOG_ERR("shouldn't happen !");
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

	memset(path, 0x00, MAX_PATH_LEN);
	traverse_path(&storage[storage_id].filelist[object_id], path);
	LOG_DBG(">>>> Traversed Path: %s", path);

	fs_file_t_init(&mtp_ctx.filestate.file);
	int err = fs_open(&mtp_ctx.filestate.file, path, FS_O_READ);
	memcpy(mtp_ctx.filestate.filepath, path, MAX_PATH_LEN);

	if (err) {
		LOG_ERR("Failed to open %s (%d)", path, err);
		return;
	}

	uint32_t filesize = storage[storage_id].filelist[object_id].size;
	uint32_t available_buf_len = MAX_PACKET_SIZE - sizeof(struct mtp_header);

	LOG_DBG("Sending file: %s size: %u", path, storage[storage_id].filelist[object_id].size);

	/* Add the Packet Header */
	data_header_push(buf, mtp_command, storage[storage_id].filelist[object_id].size);

	if (storage[storage_id].filelist[object_id].size > available_buf_len) {
		int read = fs_read(&mtp_ctx.filestate.file, mtp_ctx.filebuf, available_buf_len);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, mtp_ctx.filebuf, read);

		mtp_ctx.filestate.total_size = filesize;
		mtp_ctx.filestate.transferred = read;
		set_pending_packet(continue_get_object);
	} else {
		int read = fs_read(&mtp_ctx.filestate.file, mtp_ctx.filebuf, filesize);

		if (read <= 0) {
			LOG_ERR("Failed to read file content %d", read);
		}
		net_buf_add_mem(buf, mtp_ctx.filebuf, read);
		fs_close(&mtp_ctx.filestate.file);
		memset(&mtp_ctx.filestate, 0x00, sizeof(mtp_ctx.filestate));
		set_pending_packet(mtp_send_confirmation);
	}
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT_INFO)
{
	int ret = 0;
	static struct fs_object_t *fs_obj;

	/* first packet received from Host contains only, destination storageID and
	 * Destination ParentID
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

				LOG_INF("New ObjID:  0x%08x", fs_obj->ID);
				set_needs_more_data(true);
			} else {
				LOG_ERR("No file handle avaiable %u",
					storage[dest_storage_id].files_count);
			}
		} else {
			LOG_ERR("Unkown storage id %x", dest_storage_id);
		}
	} else { /* Host sent more info */
		char filepath[MAX_PATH_LEN];
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
		net_buf_pull_utf16le(payload, filename, str_len); /* FileName */
		/* Rest of props are ignored */
		if (str_len > MAX_FILE_NAME) {
			LOG_ERR("Invalid filename length %u", str_len);
			err_code = MTP_RESP_GENERAL_ERROR;
			goto fail;
		}

		struct fs_statvfs stat;

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
			snprintf(filepath, MAX_PATH_LEN, "%s/%s",
				 storage[fs_obj->storage_id].mountpoint, filename);
		} else {
			snprintf(filepath, MAX_PATH_LEN, "%s/%s/%s",
				 storage[fs_obj->storage_id].mountpoint,
				 storage[fs_obj->storage_id].filelist[fs_obj->parent_id].name,
				 filename);
		}

		LOG_DBG("\noFormat: %x, size: %x, parent: %x\n", ObjectFormat, fs_obj->size,
			ParentObject);
		LOG_DBG("mnt: %s\n", storage[fs_obj->storage_id].mountpoint);
		LOG_DBG("fname: %s\n", filename);
		LOG_DBG("path: %s ID:%x\n", filepath, fs_obj->ID);
		LOG_DBG("parentID: %u\n", fs_obj->parent_id);
		LOG_DBG("parentPath:%s\n",
			storage[fs_obj->storage_id].filelist[fs_obj->parent_id].name);

		if (fs_obj->type == FS_DIR_ENTRY_DIR) {
			ret = fs_mkdir(filepath);
			if (ret) {
				LOG_ERR("Failed to create directory %s (%d)", filepath, ret);
				err_code = MTP_RESP_GENERAL_ERROR;
			}
		} else {
			fs_file_t_init(&mtp_ctx.filestate.file);
			ret = fs_open(&mtp_ctx.filestate.file, filepath, FS_O_CREATE | FS_O_WRITE);
			if (ret) {
				LOG_ERR("Open file failed, %d", ret);
				err_code = MTP_RESP_GENERAL_ERROR;
			} else {
				mtp_ctx.filestate.total_size = fs_obj->size;
				memcpy(mtp_ctx.filestate.filepath, filepath, MAX_PATH_LEN);
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

		LOG_INF("Sent info:\n\tSID: %x\n\tPID: %x\n\tOID: %x", mtp_response.param[0],
			mtp_response.param[1], mtp_response.param[2]);

		net_buf_add_mem(buf, &mtp_response, 24);
		fs_obj = NULL;
	}
}

int extra_data_handler(struct net_buf *buf, struct net_buf *buf_recv)
{
	mtp_ctx.filestate.transferred += buf_recv->len;
	mtp_ctx.filestate.chunks_sent++;

	fs_write(&mtp_ctx.filestate.file, buf_recv->data, buf_recv->len);
	LOG_INF("EXTRA: Data len: %u out of %u", mtp_ctx.filestate.transferred,
		mtp_ctx.filestate.total_size);
	if (mtp_ctx.filestate.transferred >= mtp_ctx.filestate.total_size) {
		fs_close(&mtp_ctx.filestate.file);
		LOG_INF("Sending Confirmation after reciving data (Total len: %u)",
			mtp_ctx.filestate.transferred);

		memset(&mtp_ctx.filestate, 0x00, sizeof(mtp_ctx.filestate));
		mtp_send_confirmation(buf);
	} else {
		extra_data_fn = extra_data_handler;
		set_needs_more_data(true);
	}
	return 0;
}

MTP_CMD_HANDLER(MTP_OP_SEND_OBJECT)
{
	if (mtp_command->hdr.type == MTP_CONTAINER_COMMAND) {
		LOG_INF("COMMAND RECEIVED len: %u", payload->len);
	} else if (mtp_command->hdr.type == MTP_CONTAINER_DATA) {
		LOG_INF("DATA RECEIVED len: %u", payload->len); /* SKIP The header */
		net_buf_pull_mem(payload, sizeof(struct mtp_header));
		fs_write(&mtp_ctx.filestate.file, payload->data, payload->len);

		mtp_ctx.filestate.chunks_sent++;
		mtp_ctx.filestate.transferred += payload->len;
		LOG_INF("SEND_OBJECT: Data len: %u out of %u", mtp_ctx.filestate.transferred,
			mtp_ctx.filestate.total_size);

		if (mtp_ctx.filestate.transferred >= mtp_ctx.filestate.total_size) {
			fs_close(&mtp_ctx.filestate.file);
			LOG_INF("SEND_OBJECT: Sending Confirmation after reciving data (Total len: "
				"%u)",
				mtp_ctx.filestate.transferred);
			LOG_INF("SEND_OBJECT: Old filecount %u", storage[1].files_count);

			memset(&mtp_ctx.filestate, 0x00, sizeof(mtp_ctx.filestate));
			mtp_send_confirmation(buf);
			if (buf->len <= 0) {
				LOG_ERR("Failed to send confirmation");
			}
		} else {
			extra_data_fn = extra_data_handler;
			set_needs_more_data(true);
		}
	}
}

MTP_CMD_HANDLER(MTP_OP_DELETE_OBJECT)
{
	uint32_t storage_id = GET_STORAGE_ID(mtp_command->param[0]);
	uint32_t object_id = GET_OBJECT_ID(mtp_command->param[0]);

	char path[MAX_PATH_LEN];

	memset(path, 0x00, MAX_PATH_LEN);
	traverse_path(&storage[storage_id].filelist[object_id], path);
	LOG_DBG(">>>> Traversed Path: %s", path);

	if (GET_TYPE(mtp_command->param[0]) == FS_DIR_ENTRY_DIR) {
		LOG_DBG("Deleting directory %s", path);
		dir_delete(path);
	} else {
		LOG_DBG("Deleting file %s", path);
		fs_unlink(path);
	}

	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = MTP_RESP_OK,
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

	set_pending_packet(mtp_send_confirmation);
}

int mtp_commands_handler(struct net_buf *buf_in, struct net_buf *buf)
{
	if (buf == NULL) {
		LOG_ERR("%s: NULL Buffer", __func__);
		return -EINVAL;
	}

	if (handle_extra_data(buf, buf_in) == 0) {
		return buf->len;
	}

	struct mtp_container *mtp_command = (struct mtp_container *)buf_in->data;
	struct net_buf *payload = buf_in;

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

		struct mtp_header mtp_response = {
			.length = sizeof(struct mtp_header),
			.type = MTP_CONTAINER_RESPONSE,
			.code = MTP_RESP_OPERATION_NOT_SUPPORTED,
			.transaction_id = mtp_command->hdr.transaction_id };

		net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));
		break;
	}

	return buf->len;
}

int mtp_control_handler(uint8_t request, struct net_buf *buf)
{
	struct mtp_device_status_t dev_status;
	static bool canceled = false;
	switch (request)
	{
		case MTP_REQUEST_GET_DEVICE_STATUS:
			LOG_WRN("MTP_REQUEST_GET_DEVICE_STATUS");
			if (canceled){
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
			canceled = false;
			break;

		case MTP_REQUEST_CANCEL:
			LOG_WRN("Closing incomplete file %s", mtp_ctx.filestate.filepath);
			if (strlen(mtp_ctx.filestate.filepath) > 0) {
				fs_close(&mtp_ctx.filestate.file);
				fs_unlink(mtp_ctx.filestate.filepath);

				memset(mtp_ctx.filebuf, 0x00, sizeof(mtp_ctx.filebuf));
				memset(&mtp_ctx.filestate, 0x00, sizeof(mtp_ctx.filestate));
			}
			canceled = true;
			//usbd_ep_set_halt(usbd_class_get_ctx(c_data), ep);
			return 1;
			break;

		case MTP_REQUEST_DEVICE_RESET:
			LOG_WRN("MTP_REQUEST_DEVICE_RESET");
			mtp_reset();
			break;

		default:
			LOG_ERR("Unknown request 0x%x!", request);
			break;
	}

	return 0;
}

static int mtp_send_confirmation(struct net_buf *buf)
{
	if (buf == NULL) {
		LOG_ERR("%s: Null Buffer!", __func__);
		return -EINVAL;
	}

	struct mtp_container *mtp_command = (struct mtp_container *)buf->data;
	struct mtp_header mtp_response = {.length = sizeof(struct mtp_header),
					  .type = MTP_CONTAINER_RESPONSE,
					  .code = MTP_RESP_OK,
					  .transaction_id = mtp_command->hdr.transaction_id};

	net_buf_add_mem(buf, &mtp_response, sizeof(struct mtp_header));

	return 0;
}

static void mtp_reset()
{
	for (int i = 1; i < ARRAY_SIZE(storage); i++) {
		memset(storage[i].filelist, 0x00, sizeof(storage[i].filelist));
		storage[i].files_count = 0;
	}
	mtp_ctx.session_opened = false;
}

int mtp_init(const char *manufacturer, const char *model, const char *device_version,
	     const char *serial_number)
{
	dev_info.manufacturer = manufacturer;
	dev_info.model = model;
	dev_info.device_version = device_version;

	/* Zephyr set Serial Number descriptor after MTP init, so for now use this one */
	dev_info.serial_number = "0123456789ABCDEF";

	mtp_reset();

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