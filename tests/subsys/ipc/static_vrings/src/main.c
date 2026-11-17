/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/ipc/ipc_static_vrings.h>
#include <zephyr/ztest.h>

#include "ipc_rpmsg_static_vrings.h"

#define TEST_REGION_SIZE DT_REG_SIZE(DT_NODELABEL(ordinary_memory))

static const struct metal_io_dt_spec single = METAL_IO_DT_SPEC_GET(DT_NODELABEL(single_mapping));
static const struct metal_io_dt_spec paged = METAL_IO_DT_SPEC_GET(DT_NODELABEL(paged_mapping));
static const struct metal_io_dt_spec remote = METAL_IO_DT_SPEC_GET(DT_NODELABEL(remote_mapping));

static const uintptr_t region_addrs[] = {
	DT_REG_ADDR(DT_NODELABEL(ordinary_memory)),
	DT_REG_ADDR(DT_NODELABEL(mapped_memory)),
	DT_REG_ADDR(DT_NODELABEL(paged_memory)),
	DT_REG_ADDR(DT_NODELABEL(remote_memory)),
};

static int seed_status(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(region_addrs); i++) {
		memset((void *)region_addrs[i], 0xa5, TEST_REGION_SIZE);
	}
	return 0;
}

/* Run before shared_memory_prepare(), including when reset is disabled. */
SYS_INIT(seed_status, PRE_KERNEL_1, 0);

static void *check_reset(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(region_addrs); i++) {
		const uint8_t *memory = (const uint8_t *)region_addrs[i];
		bool reset = IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_RPMSG_SHMEM_RESET) && i < 3;

		for (size_t offset = 0; offset < TEST_REGION_SIZE; offset++) {
			uint8_t expected = reset && offset < VDEV_STATUS_SIZE ? 0 : 0xa5;

			zassert_equal(memory[offset], expected, "region %zu offset %zu", i, offset);
		}
	}
	return NULL;
}

static void check_descriptors(const struct device *dev, uintptr_t local,
			      const struct metal_io_dt_spec *mapping)
{
	unsigned int count = optimal_num_desc(TEST_REGION_SIZE, RPMSG_BUFFER_SIZE);
	uintptr_t buffers = local + VDEV_STATUS_SIZE;
	uintptr_t rx = buffers + VRING_COUNT * vq_ring_size(count, RPMSG_BUFFER_SIZE);
	uintptr_t tx = ROUND_UP(rx + vring_size(count, MEM_ALIGNMENT), MEM_ALIGNMENT);
	const struct vring_desc *desc = (const struct vring_desc *)tx;

	zassert_true(device_is_ready(dev));
	/* Reopening also checks that libmetal does not retain temporary physmaps. */
	for (size_t attempt = 0; attempt < 2; attempt++) {
		memset((void *)local, 0, TEST_REGION_SIZE);
		zassert_ok(ipc_service_open_instance(dev));
		for (size_t i = 0; i < count; i++) {
			size_t offset = VDEV_STATUS_SIZE + i * RPMSG_BUFFER_SIZE;
			metal_phys_addr_t expected = local + offset;

			if (mapping != NULL) {
				if (mapping->page_shift == -1) {
					expected = mapping->phys_map[0] + offset;
				} else {
					expected =
						mapping->phys_map[offset >> 12] + (offset & 0xfff);
				}
			}
			zassert_equal(desc[i].addr, expected, "descriptor %zu", i);
			zassert_equal(desc[i].len, RPMSG_BUFFER_SIZE);
		}
		zassert_ok(ipc_service_close_instance(dev));
	}
}

ZTEST(static_vrings, test_ordinary_backend)
{
	check_descriptors(DEVICE_DT_GET(DT_NODELABEL(ordinary)), region_addrs[0], NULL);
}

ZTEST(static_vrings, test_translated_backend)
{
	check_descriptors(DEVICE_DT_GET(DT_NODELABEL(mapped)), single.region_addr, &single);
}

ZTEST(static_vrings, test_paged_backend)
{
	check_descriptors(DEVICE_DT_GET(DT_NODELABEL(paged)), paged.region_addr, &paged);
}

static void configure_vrings(struct ipc_static_vrings *vr, const struct metal_io_dt_spec *io)
{
	memset(vr, 0, sizeof(*vr));
	vr->io = io;
	vr->status_reg_addr = io->region_addr;
	vr->shm_addr = io->region_addr + VDEV_STATUS_SIZE;
	vr->shm_size = io->region_size - VDEV_STATUS_SIZE;
	vr->vring_size = 1;
}

ZTEST(static_vrings, test_peer_translation)
{
	struct ipc_static_vrings host_vr;
	struct ipc_static_vrings remote_vr;
	const size_t offsets[] = {0, VDEV_STATUS_SIZE, 4095, 4096, TEST_REGION_SIZE - 1};

	configure_vrings(&host_vr, &single);
	configure_vrings(&remote_vr, &remote);
	zassert_ok(ipc_static_vrings_init(&host_vr, ROLE_HOST));
	zassert_ok(ipc_static_vrings_init(&remote_vr, ROLE_REMOTE));
	zassert_not_equal(single.region_addr, remote.region_addr);
	zassert_equal(host_vr.shm_io.mem_flags, 0);
	zassert_equal(host_vr.shm_io.page_mask, METAL_BAD_PHYS);

	for (size_t i = 0; i < ARRAY_SIZE(offsets); i++) {
		void *host_addr = (void *)(single.region_addr + offsets[i]);
		void *remote_addr = (void *)(remote.region_addr + offsets[i]);
		metal_phys_addr_t address = metal_io_virt_to_phys(&host_vr.shm_io, host_addr);

		zassert_equal(address, single.phys_map[0] + offsets[i]);
		zassert_equal(metal_io_virt_to_phys(&remote_vr.shm_io, remote_addr), address);
		zassert_equal_ptr(metal_io_phys_to_virt(&remote_vr.shm_io, address), remote_addr);
		zassert_equal_ptr(metal_io_phys_to_virt(&host_vr.shm_io, address), host_addr);
	}
	zassert_is_null(metal_io_phys_to_virt(&host_vr.shm_io, single.phys_map[0] - 1));
	zassert_equal(metal_io_phys(&host_vr.shm_io, TEST_REGION_SIZE), METAL_BAD_PHYS);
	zassert_ok(ipc_static_vrings_deinit(&remote_vr, ROLE_REMOTE));
	zassert_ok(ipc_static_vrings_deinit(&host_vr, ROLE_HOST));
}

ZTEST(static_vrings, test_page_boundaries)
{
	struct metal_io_dt_spec partial = paged;
	struct ipc_static_vrings vr;
	const size_t offsets[] = {0, VDEV_STATUS_SIZE, 4095, 4096, TEST_REGION_SIZE - 1};

	configure_vrings(&vr, &paged);
	zassert_ok(ipc_static_vrings_init(&vr, ROLE_HOST));
	zassert_equal(vr.shm_io.mem_flags, 0x1234);
	zassert_equal(vr.shm_io.page_shift, 12);
	for (size_t i = 0; i < ARRAY_SIZE(offsets); i++) {
		metal_phys_addr_t expected = paged.phys_map[offsets[i] / 4096] + offsets[i] % 4096;
		void *local = (void *)(paged.region_addr + offsets[i]);

		zassert_equal(metal_io_virt_to_phys(&vr.shm_io, local), expected);
		zassert_equal_ptr(metal_io_phys_to_virt(&vr.shm_io, expected), local);
	}
	zassert_is_null(metal_io_phys_to_virt(&vr.shm_io, paged.phys_map[0] + 4096));
	zassert_equal(metal_io_phys(&vr.shm_io, TEST_REGION_SIZE), METAL_BAD_PHYS);
	zassert_ok(ipc_static_vrings_deinit(&vr, ROLE_HOST));

	partial.region_size = 4097;
	configure_vrings(&vr, &partial);
	zassert_ok(ipc_static_vrings_init(&vr, ROLE_HOST));
	zassert_equal(metal_io_phys(&vr.shm_io, 4096), paged.phys_map[1]);
	zassert_equal(metal_io_phys(&vr.shm_io, 4097), METAL_BAD_PHYS);
	zassert_is_null(metal_io_phys_to_virt(&vr.shm_io, paged.phys_map[1] + 1));
	zassert_ok(ipc_static_vrings_deinit(&vr, ROLE_HOST));
}

ZTEST(static_vrings, test_invalid_mapping)
{
	struct metal_io_dt_spec io = paged;
	struct ipc_static_vrings vr;
	metal_phys_addr_t bad_map[] = {0x90000001, 0xa0000000};

	configure_vrings(&vr, &io);
	io.phys_map = NULL;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = paged;
	io.region_size = 0;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = paged;
	io.phys_map_size = 1;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = paged;
	io.page_shift = sizeof(metal_phys_addr_t) * CHAR_BIT;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io.page_shift = -2;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = paged;
	io.phys_map = bad_map;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = single;
	io.phys_map_size = 2;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = single;
	bad_map[0] = METAL_BAD_PHYS - 1;
	io.phys_map = bad_map;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
	io = single;
	io.region_addr = UINTPTR_MAX - 1;
	zassert_equal(ipc_static_vrings_init(&vr, ROLE_HOST), -EINVAL);
}

ZTEST(static_vrings, test_identity_address_width)
{
	struct ipc_static_vrings vr = {
		.shm_addr = IS_ENABLED(CONFIG_64BIT) ? (uintptr_t)UINT64_C(0x100000040)
						     : (uintptr_t)0x90000040,
		.shm_size = 4096,
		.vring_size = 1,
	};

	/* Initialising the vring metadata does not dereference these addresses. */
	zassert_ok(ipc_static_vrings_init(&vr, ROLE_HOST));
	zassert_equal_ptr(vr.shm_io.physmap, vr.shm_physmap);
	zassert_equal(metal_io_phys(&vr.shm_io, 64), vr.shm_addr + 64);
	zassert_equal_ptr(metal_io_phys_to_virt(&vr.shm_io, vr.shm_addr + 64),
			  (void *)(vr.shm_addr + 64));
	zassert_ok(ipc_static_vrings_deinit(&vr, ROLE_HOST));
}

ZTEST_SUITE(static_vrings, NULL, check_reset, NULL, NULL, NULL);
