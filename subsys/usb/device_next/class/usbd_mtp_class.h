/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBD_MTP_CLASS_H_
#define _USBD_MTP_CLASS_H_

#include <zephyr/fs/fs.h>

#define MAX_PATH_LEN             128


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
		uint8_t direction; 	/* 0: HOST_IN (Upload to Host), 1: HOST_OUT (Download from Host) */
	} filestate;

        bool more_data_needed;
        int (*extra_data_fn)(struct mtp_context* ctx, struct net_buf *buf,
                                        struct net_buf *buf_recv);
        int (*pending_fn)(struct mtp_context* ctx, struct net_buf *buf);
};


bool mtp_packet_pending(struct mtp_context* ctx);
bool mtp_needs_more_data(struct mtp_context* ctx, struct net_buf *buf);
int mtp_commands_handler(struct mtp_context* ctx, struct net_buf *buf_in, struct net_buf *buf);
int send_pending_packet(struct mtp_context* ctx, struct net_buf *buf);
int mtp_control_handler(struct mtp_context* ctx, uint8_t request, struct net_buf *buf);
int mtp_init(struct mtp_context* ctx, const char *manufacturer, const char *model, const char *device_version, const char *serial_number);

#endif /* _USBD_MTP_CLASS_H_ */
