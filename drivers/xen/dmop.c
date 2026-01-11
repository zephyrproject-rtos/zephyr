/*
 * Copyright 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/xen/dmop.h>
#include <zephyr/arch/arm64/hypercall.h>

int dmop_create_ioreq_server(domid_t domid, uint8_t handle_bufioreq, ioservid_t *id)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_create_ioreq_server;
	dm_op.u.create_ioreq_server.handle_bufioreq = handle_bufioreq;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, ARRAY_SIZE(bufs), bufs);
	if (err) {
		return err;
	}

	*id = dm_op.u.create_ioreq_server.id;

	return 0;
}

int dmop_destroy_ioreq_server(domid_t domid, ioservid_t id)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_destroy_ioreq_server;
	dm_op.u.destroy_ioreq_server.id = id;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, ARRAY_SIZE(bufs), bufs);
	if (err) {
		return err;
	}

	return 0;
}

int dmop_map_io_range_to_ioreq_server(domid_t domid, ioservid_t id, uint32_t type, uint64_t start,
				      uint64_t end)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_map_io_range_to_ioreq_server;
	dm_op.u.map_io_range_to_ioreq_server.id = id;
	dm_op.u.map_io_range_to_ioreq_server.type = type;
	dm_op.u.map_io_range_to_ioreq_server.start = start;
	dm_op.u.map_io_range_to_ioreq_server.end = end;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, ARRAY_SIZE(bufs), bufs);
	if (err < 0) {
		return err;
	}

	return 0;
}

int dmop_unmap_io_range_from_ioreq_server(domid_t domid, ioservid_t id, uint32_t type,
					  uint64_t start, uint64_t end)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_unmap_io_range_from_ioreq_server;
	dm_op.u.unmap_io_range_from_ioreq_server.id = id;
	dm_op.u.unmap_io_range_from_ioreq_server.type = type;
	dm_op.u.unmap_io_range_from_ioreq_server.start = start;
	dm_op.u.unmap_io_range_from_ioreq_server.end = end;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, ARRAY_SIZE(bufs), bufs);
	if (err < 0) {
		return err;
	}

	return 0;
}

int dmop_set_ioreq_server_state(domid_t domid, ioservid_t id, uint8_t enabled)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_set_ioreq_server_state;
	dm_op.u.set_ioreq_server_state.id = id;
	dm_op.u.set_ioreq_server_state.enabled = enabled;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, 1, bufs);
	if (err) {
		return err;
	}

	return 0;
}

int dmop_nr_vcpus(domid_t domid)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_nr_vcpus;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, 1, bufs);
	if (err < 0) {
		return err;
	}

	return dm_op.u.nr_vcpus.vcpus;
}

int dmop_set_irq_level(domid_t domid, uint32_t irq, uint8_t level)
{
	struct xen_dm_op_buf bufs[1] = {0};
	struct xen_dm_op dm_op = {0};
	int err;

	dm_op.op = XEN_DMOP_set_irq_level;
	dm_op.u.set_irq_level.irq = irq;
	dm_op.u.set_irq_level.level = level;

	set_xen_guest_handle(bufs[0].h, &dm_op);
	bufs[0].size = sizeof(struct xen_dm_op);

	err = HYPERVISOR_dm_op(domid, 1, bufs);

	return err;
}
