/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_INTERNAL_H_
#define ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_INTERNAL_H_

#include <zephyr/canbus/canopen.h>

#define CANOPEN_COB_NMT     0x000U
#define CANOPEN_COB_SYNC    0x080U
#define CANOPEN_COB_SDO_TX  0x580U
#define CANOPEN_COB_SDO_RX  0x600U
#define CANOPEN_COB_NMT_ERR 0x700U

#define CANOPEN_OD_GUARD_TIME  0x100CU
#define CANOPEN_OD_LIFE_FACTOR 0x100DU
#define CANOPEN_OD_TIME        0x1012U
#define CANOPEN_OD_HB_CONSUMER 0x1016U

#define CANOPEN_ERROR_BEHAVIOR_PRE_OPERATIONAL 0U
#define CANOPEN_ERROR_BEHAVIOR_NO_CHANGE       1U
#define CANOPEN_ERROR_BEHAVIOR_STOPPED         2U

#define CANOPEN_COB_ID_INVALID  BIT(31)
#define CANOPEN_COB_ID_BIT30    BIT(30)
#define CANOPEN_COB_ID_FRAME    BIT(29)
#define CANOPEN_COB_ID_RESERVED GENMASK(28, 11)

#define CANOPEN_FLAG_RX_OVERRUN 0
#define CANOPEN_FLAG_SYNC_TICK  1
#define CANOPEN_FLAG_CAN_STATE  2

struct canopen_od_ref {
	const struct canopen_od_entry *entry;
	const void *src;
	void *dst;
	uint16_t size;
	uint16_t index;
	uint8_t sub;
	uint8_t type;
	uint8_t attr;
};

uint32_t canopen_od_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			 struct canopen_od_ref *ref);
uint32_t canopen_od_read_wire(const struct canopen_od_ref *ref, uint8_t *buf, size_t max,
			      size_t *len);
uint32_t canopen_od_write_wire(struct canopen_node *node, const struct canopen_od_ref *ref,
			       const uint8_t *buf, size_t len);
size_t canopen_od_wire_size(const struct canopen_od_ref *ref);
size_t canopen_od_type_size(uint8_t type);
int canopen_od_validate(const struct canopen_od *od);

void canopen_od_comm_ref(struct canopen_od_ref *ref, uint16_t index, uint8_t sub, uint8_t type,
			 uint8_t attr, const void *src, void *dst, uint16_t size);
uint32_t canopen_od_comm_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			      struct canopen_od_ref *ref);
uint32_t canopen_od_comm_write(struct canopen_node *node, const struct canopen_od_ref *ref,
			       uint32_t value);
void canopen_od_comm_reset(struct canopen_node *node);

int canopen_send(struct canopen_node *node, const struct can_frame *frame);
int canopen_filter_set(struct canopen_node *node, int *filter, uint32_t cob_id);
bool canopen_cob_id_restricted(uint32_t cob_id);

void canopen_nmt_rx(struct canopen_node *node, const struct can_frame *frame);
int canopen_nmt_apply(struct canopen_node *node, enum canopen_nmt_cmd cmd);
void canopen_nmt_boot(struct canopen_node *node, bool reset_app);
void canopen_nmt_hb_restart(struct canopen_node *node);
void canopen_nmt_process(struct canopen_node *node);
void canopen_nmt_comm_error(struct canopen_node *node);
k_timepoint_t canopen_nmt_deadline(struct canopen_node *node);

void canopen_sdo_server_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_sdo_server_reset(struct canopen_node *node);
void canopen_sdo_server_process(struct canopen_node *node);
k_timepoint_t canopen_sdo_server_deadline(struct canopen_node *node);

int canopen_pdo_init(struct canopen_node *node);
void canopen_pdo_reset(struct canopen_node *node);
void canopen_pdo_filters(struct canopen_node *node);
uint32_t canopen_pdo_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			  struct canopen_od_ref *ref);
uint32_t canopen_pdo_write(struct canopen_node *node, uint16_t index, uint8_t sub,
			   uint32_t value);
void canopen_pdo_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_pdo_sync(struct canopen_node *node, uint8_t counter);
void canopen_pdo_state(struct canopen_node *node);
void canopen_pdo_process(struct canopen_node *node);
k_timepoint_t canopen_pdo_deadline(struct canopen_node *node);

void canopen_sync_init(struct canopen_node *node);
void canopen_sync_reset(struct canopen_node *node);
void canopen_sync_update(struct canopen_node *node);
uint32_t canopen_sync_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			   struct canopen_od_ref *ref);
uint32_t canopen_sync_write(struct canopen_node *node, uint16_t index, uint32_t value);
bool canopen_sync_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_sync_state(struct canopen_node *node, enum canopen_nmt_state old);
void canopen_sync_process(struct canopen_node *node);
bool canopen_sync_window_open(struct canopen_node *node);
k_timepoint_t canopen_sync_deadline(struct canopen_node *node);

int canopen_leds_init(struct canopen_node *node);
void canopen_leds_process(struct canopen_node *node);
k_timepoint_t canopen_leds_deadline(struct canopen_node *node);

void canopen_emcy_reset(struct canopen_node *node, bool app);
void canopen_emcy_signal(struct canopen_node *node, uint16_t code, uint8_t reg);
int canopen_emcy_set(struct canopen_node *node, uint16_t code, uint8_t reg, const uint8_t *msef);
int canopen_emcy_unset(struct canopen_node *node, uint16_t code);
bool canopen_emcy_active(struct canopen_node *node, uint16_t code);
void canopen_emcy_can_state(struct canopen_node *node, enum can_state state);
uint32_t canopen_emcy_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			   struct canopen_od_ref *ref);
uint32_t canopen_emcy_write(struct canopen_node *node, uint16_t index, uint32_t value);
void canopen_emcy_process(struct canopen_node *node);
k_timepoint_t canopen_emcy_deadline(struct canopen_node *node);

void canopen_storage_load(struct canopen_node *node, bool app);
uint32_t canopen_storage_find(uint16_t index, uint8_t sub, struct canopen_od_ref *ref);
uint32_t canopen_storage_write(struct canopen_node *node, uint16_t index, uint8_t sub,
			       uint32_t value);

void canopen_sdo_client_init(struct canopen_node *node);
void canopen_sdo_client_reset(struct canopen_node *node);
uint32_t canopen_sdo_client_find(struct canopen_node *node, uint8_t sub,
				 struct canopen_od_ref *ref);
uint32_t canopen_sdo_client_write(struct canopen_node *node, uint8_t sub, uint32_t value);
bool canopen_sdo_client_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_sdo_client_process(struct canopen_node *node);
k_timepoint_t canopen_sdo_client_deadline(struct canopen_node *node);

int canopen_hb_init(struct canopen_node *node);
void canopen_hb_reset(struct canopen_node *node);
void canopen_hb_filters(struct canopen_node *node);
uint32_t canopen_hb_find(struct canopen_node *node, uint8_t sub, struct canopen_od_ref *ref);
uint32_t canopen_hb_write(struct canopen_node *node, uint8_t sub, uint32_t value);
bool canopen_hb_rx(struct canopen_node *node, const struct can_frame *frame);
bool canopen_hb_timed_out(struct canopen_node *node);

void canopen_shell_register(struct canopen_node *node);

void canopen_guard_init(struct canopen_node *node);
void canopen_guard_reset(struct canopen_node *node);
void canopen_guard_filter(struct canopen_node *node);
bool canopen_guard_timed_out(struct canopen_node *node);
uint32_t canopen_guard_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			    struct canopen_od_ref *ref);
uint32_t canopen_guard_write(struct canopen_node *node, uint16_t index, uint32_t value);
void canopen_guard_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_guard_process(struct canopen_node *node);
k_timepoint_t canopen_guard_deadline(struct canopen_node *node);

void canopen_time_init(struct canopen_node *node);
void canopen_time_reset(struct canopen_node *node);
void canopen_time_filter(struct canopen_node *node);
uint32_t canopen_time_find(struct canopen_node *node, uint8_t sub, struct canopen_od_ref *ref);
uint32_t canopen_time_write(struct canopen_node *node, uint32_t value);
bool canopen_time_rx(struct canopen_node *node, const struct can_frame *frame);
void canopen_hb_process(struct canopen_node *node);
k_timepoint_t canopen_hb_deadline(struct canopen_node *node);

#endif /* ZEPHYR_SUBSYS_CANBUS_CANOPEN_CANOPEN_INTERNAL_H_ */
