/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_DMOP_H_
#define ZEPHYR_XEN_DMOP_H_

#include <xen/public/hvm/dm_op.h>

int dmop_create_ioreq_server(domid_t domid, uint8_t handle_bufioreq, ioservid_t *id);

int dmop_map_io_range_to_ioreq_server(domid_t domid, ioservid_t id, uint32_t type, uint64_t start,
				      uint64_t end);

int dmop_set_ioreq_server_state(domid_t domid, ioservid_t id, uint8_t enabled);

int dmop_nr_vcpus(domid_t domid);

int dmop_set_irq_level(domid_t domid, uint32_t irq, uint8_t level);

#endif /* ZEPHYR_XEN_DMOP_H_ */
