/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBD_MTP_CLASS_H_
#define _USBD_MTP_CLASS_H_

#include <zephyr/fs/fs.h>
#include <zephyr/usb/usbd.h>

#define MAX_PATH_LEN    255U
#define MAX_OBJPATH_LEN (MAX_PATH_LEN + MAX_FILE_NAME + 1)

enum mtp_phase {
	MTP_PHASE_REQUEST = 0,
	MTP_PHASE_DATA,
	MTP_PHASE_RESPONSE,
	MTP_PHASE_CANCELED,
};

struct mtp_op_state_t {
	uint16_t code;
	uint16_t err;
	uint32_t args[2];
};

struct mtp_device_status_t {
	uint16_t len;
	uint16_t code;
	uint8_t epIn;
	uint8_t epOut;
} __packed;

struct mtp_context {
	bool session_opened;
	uint32_t transaction_id;
	uint16_t max_packet_size;
	enum mtp_phase phase;
	uint8_t filebuf[USBD_MAX_BULK_MPS];
	struct {
		struct fs_file_t file;
		char filepath[MAX_OBJPATH_LEN];
		uint32_t total_size;
		uint32_t transferred;
		uint32_t chunks_sent;
		uint32_t storage_id;
	} transfer_state;
	struct mtp_op_state_t op_state;
	struct mtp_device_status_t dev_status;
};

bool mtp_packet_pending(struct mtp_context *ctx);
int mtp_commands_handler(struct mtp_context *ctx, struct net_buf *buf_in, struct net_buf *buf);
int mtp_control_to_host(struct mtp_context *ctx, uint8_t request, struct net_buf *const buf);
int mtp_control_to_dev(struct mtp_context *ctx, uint8_t request, const struct net_buf *const buf);
void mtp_reset(struct mtp_context *ctx);
int mtp_init(struct mtp_context *ctx, const char *manufacturer, const char *model,
	     const char *device_version, const char *serial_number);

#endif /* _USBD_MTP_CLASS_H_ */
