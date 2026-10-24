/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBD_MTP_CLASS_H_
#define _USBD_MTP_CLASS_H_

#include <zephyr/fs/fs.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_mtp.h>

#define MAX_PATH_LEN    255U
#define MAX_OBJPATH_LEN (MAX_PATH_LEN + MAX_FILE_NAME + 1)
#define MTP_MAX_FILES   (CONFIG_USBD_MTP_MAX_HANDLES + 1U) /* +1 for root */

enum mtp_phase {
	MTP_PHASE_REQUEST = 0,
	MTP_PHASE_DATA,
	MTP_PHASE_RESPONSE,
	MTP_PHASE_CANCELED,
};

#define MTP_REQUEST_CANCEL            0x64U
#define MTP_REQUEST_DEVICE_RESET      0x66U
#define MTP_REQUEST_GET_DEVICE_STATUS 0x67U

struct mtp_op_state_t {
	uint16_t code;
	uint16_t err;
	uint32_t args[2];
};

struct mtp_device_status_t {
	uint16_t len;
	uint16_t code;
	uint8_t ep_in;
	uint8_t ep_out;
} __packed;

union mtp_object_handle {
	struct {
		uint8_t partition_id; /* ID of the partition containing the object */
		uint8_t parent_id;    /* ID of the parent object (0xFF for root) */
		uint8_t object_id;    /* Unique ID of the object within the storage */
		uint8_t type;         /* Type of the object (e.g., file, directory) */
	};
	uint32_t value;
};

struct mtp_object {
	union mtp_object_handle handle;
	uint32_t size;
	char name[MAX_FILE_NAME + 1];
};

struct mtp_partition {
	const char *mountpoint;
	struct mtp_object objlist[MTP_MAX_FILES];
	uint8_t files_count;
	bool read_only;
};

struct mtp_device_info {
	const char *manufacturer;
	const char *model;
	const char *device_version;
	const char *serial_number;
};

struct mtp_context {
	struct mtp_partition partitions[CONFIG_USBD_MTP_STORAGES_PER_INSTANCE + 1];
	uint8_t partitions_count;
	struct mtp_device_info dev_info;
	bool session_opened;
	uint32_t session_id;
	uint32_t transaction_id;
	uint16_t max_packet_size;
	enum mtp_phase phase;
	bool initialized;
	uint8_t filebuf[USBD_MAX_BULK_MPS];
	struct {
		struct fs_file_t file;
		char filepath[MAX_OBJPATH_LEN];
		uint32_t total_size;
		uint32_t transferred;
		uint32_t chunks_sent;
		uint8_t partition_id;
	} transfer_state;
	struct mtp_op_state_t op_state;
	struct mtp_device_status_t dev_status;
};

bool mtp_packet_pending(struct mtp_context *ctx);
int mtp_commands_handler(struct mtp_context *ctx, struct net_buf *buf_in, struct net_buf *buf);
int mtp_get_dev_status(struct mtp_context *ctx, struct net_buf *const buf);
int mtp_cancel(struct mtp_context *ctx, const struct net_buf *const buf);
int mtp_device_reset(struct mtp_context *ctx);
void mtp_reset(struct mtp_context *ctx);
int mtp_init(struct mtp_context *ctx, const struct usbd_mtp_instance *config);

#endif /* _USBD_MTP_CLASS_H_ */
