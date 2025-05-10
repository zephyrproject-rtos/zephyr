/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBD_MTP_CLASS_H_
#define _USBD_MTP_CLASS_H_

#include <zephyr/fs/fs.h>
#include <zephyr/usb/usbd.h>

#define MAX_PATH_LEN 128

struct mtp_context {
	bool session_opened;
	bool op_canceled;
	uint16_t op_code;
	uint16_t max_packet_size;
	uint8_t filebuf[USBD_MAX_BULK_MPS];

	bool more_data_needed;
	int (*extra_data_fn)(struct mtp_context *ctx, struct net_buf *buf_in);
	int (*pending_fn)(struct mtp_context *ctx, struct net_buf *buf);
	struct {
		struct fs_file_t file;
		char filepath[MAX_PATH_LEN];
		uint32_t total_size;
		uint32_t transferred;
		uint32_t chunks_sent;
		uint32_t storage_id;
	} filestate;
};

bool mtp_packet_pending(struct mtp_context *ctx);
bool mtp_needs_more_data(struct mtp_context *ctx, struct net_buf *buf);
int mtp_commands_handler(struct mtp_context *ctx, struct net_buf *buf_in, struct net_buf *buf);
int mtp_get_pending_packet(struct mtp_context *ctx, struct net_buf *buf);
int mtp_control_to_host(struct mtp_context *ctx, uint8_t request, struct net_buf *const buf);
int mtp_control_to_dev(struct mtp_context *ctx, uint8_t request, const struct net_buf *const buf);
void mtp_reset(struct mtp_context *ctx);
int mtp_init(struct mtp_context *ctx, const char *manufacturer, const char *model,
	     const char *device_version, const char *serial_number);

#endif /* _USBD_MTP_CLASS_H_ */
