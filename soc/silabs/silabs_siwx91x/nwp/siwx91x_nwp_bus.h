/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_NWP_BUS_H_
#define SIWX91X_NWP_BUS_H_
#include <stdint.h>
#include <stddef.h>

struct device;
struct net_buf;

/* Send the frame even if the queues are locked. */
#define SIWX91X_FRAME_FLAG_NO_LOCK              BIT(0)
/* Enqueue the frame and return immediately. Note the reference counter for the net_buf is
 * incremented, so the caller can safely call net_buf_unref().
 */
#define SIWX91X_FRAME_FLAG_ASYNC                BIT(1)
/* Request to siwx91x_nwp_send_frame() to not erase the 16 first bytes */
#define SIWX91X_FRAME_FLAG_NO_HDR_RESET         BIT(2)
/* On Tx, shift the payload (the part after the descriptor) by one byte. This is required by
 * Bluetooth frame because the HCI command has to be placed in the descriptor part which is in
 * another fragment
 */
#define SIWX91X_FRAME_FLAG_SHIFT_PAYLOAD_1_BYTE BIT(3)

struct net_buf *siwx91x_nwp_send_frame(const struct device *dev, struct net_buf *buf,
				       uint16_t command, int queue_id, uint8_t flags);
void siwx91x_nwp_tx_flush_lock(const struct device *dev);
void siwx91x_nwp_tx_unlock(const struct device *dev);

void siwx91x_nwp_isr(const struct device *dev);
void siwx91x_nwp_thread(void *arg1, void *arg2, void *arg3);

#endif
