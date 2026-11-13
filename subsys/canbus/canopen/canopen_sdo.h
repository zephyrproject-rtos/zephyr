/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_SDO_H_
#define ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_SDO_H_

#include <zephyr/sys/byteorder.h>

#include "canopen_internal.h"

#define SDO_ABORT        0x80U
#define SDO_FRAME_LEN    8U
#define SDO_SEGMENT_DATA 7U

enum sdo_state {
	SDO_IDLE,
	SDO_DOWNLOAD,
	SDO_UPLOAD,
};

static inline void canopen_sdo_put_mux(uint8_t *data, uint16_t index, uint8_t sub)
{
	sys_put_le16(index, &data[1]);
	data[3] = sub;
}

static inline uint32_t canopen_sdo_find(struct canopen_node *node, uint16_t index, uint8_t sub,
					bool write, struct canopen_od_ref *ref)
{
	uint32_t sdo_abort = canopen_od_find(node, index, sub, ref);

	if (sdo_abort != CANOPEN_SDO_ABORT_NONE) {
		return sdo_abort;
	}

	if (write && ((ref->attr & CANOPEN_OD_ATTR_WRITE) == 0U)) {
		return CANOPEN_SDO_ABORT_READ_ONLY;
	}

	if (!write && ((ref->attr & CANOPEN_OD_ATTR_READ) == 0U)) {
		return CANOPEN_SDO_ABORT_WRITE_ONLY;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static inline void canopen_sdo_rearm(struct canopen_sdo_server *sdo)
{
	sdo->deadline = sys_timepoint_calc(K_MSEC(CONFIG_CANOPEN_SDO_SERVER_TIMEOUT_MS));
}

void canopen_sdo_send(struct canopen_node *node, const uint8_t *data);
void canopen_sdo_abort(struct canopen_node *node, uint16_t index, uint8_t sub, uint32_t code);
void canopen_sdo_start(struct canopen_sdo_server *sdo, enum sdo_state state, uint16_t index,
		       uint8_t sub, size_t size);

#endif /* ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_SDO_H_ */
