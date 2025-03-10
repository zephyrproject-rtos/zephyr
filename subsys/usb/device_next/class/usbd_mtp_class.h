/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBD_MTP_CLASS_H_
#define _USBD_MTP_CLASS_H_

bool mtp_packet_pending(void);
bool mtp_packet_pending(void);
bool mtp_needs_more_data(struct net_buf *buf);
int mtp_commands_handler(struct net_buf *buf_in, struct net_buf *buf);
int send_pending_packet(struct net_buf *buf);
int mtp_control_handler(uint8_t request, struct net_buf *buf);
int mtp_init(const char *manufacturer, const char *model, const char *device_version, const char *serial_number);

#endif /* _USBD_MTP_CLASS_H_ */
