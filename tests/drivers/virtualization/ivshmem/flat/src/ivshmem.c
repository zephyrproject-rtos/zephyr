/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <cmsis_core.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/ztest.h>

#define FLAT0             DT_NODELABEL(flat0)
#define FLAT1             DT_NODELABEL(flat1)
#define NO_IRQ            DT_NODELABEL(flat_no_irq)
#define MMR(node)         DT_REG_ADDR_BY_NAME(node, mmr)
#define MASK_SENTINEL     0xa5a55a5aU
#define DOORBELL_SENTINEL 0x12345678U

static void prepare(void *fixture)
{
	ARG_UNUSED(fixture);

	const struct device *const devices[] = {
		DEVICE_DT_GET(FLAT0),
		DEVICE_DT_GET(FLAT1),
		DEVICE_DT_GET(NO_IRQ),
	};
	const uintptr_t registers[] = {MMR(FLAT0), MMR(FLAT1), MMR(NO_IRQ)};

	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]));
		if (ivshmem_get_vectors(devices[i]) > 0) {
			zassert_ok(ivshmem_register_handler(devices[i], NULL, 0));
		}
		sys_write32(sys_cpu_to_le32(MASK_SENTINEL), registers[i]);
		sys_write32(0, registers[i] + 4);
		sys_write32(sys_cpu_to_le32(17 + i), registers[i] + 8);
		sys_write32(sys_cpu_to_le32(DOORBELL_SENTINEL), registers[i] + 12);
	}
}

ZTEST(ivshmem_flat, test_memory_and_id)
{
	const struct device *dev = DEVICE_DT_GET(FLAT0);
	uintptr_t mem;

	zassert_equal(ivshmem_get_mem(dev, &mem), DT_REG_SIZE_BY_NAME(FLAT0, shmem));
	zassert_equal(mem, DT_REG_ADDR_BY_NAME(FLAT0, shmem));

	sys_write32(0xfeedface, mem);
	zassert_equal(sys_read32(DT_REG_ADDR_BY_NAME(FLAT0, shmem)), 0xfeedface);

	zassert_equal(ivshmem_get_id(dev), 17);

	sys_write32(sys_cpu_to_le32(65535), MMR(FLAT0) + 8);
	zassert_equal(ivshmem_get_id(dev), 65535);
	zassert_equal(ivshmem_get_id(DEVICE_DT_GET(FLAT1)), 18);
}

ZTEST(ivshmem_flat, test_vectors_and_doorbell)
{
	const struct device *dev = DEVICE_DT_GET(FLAT0);

	zassert_equal(ivshmem_get_vectors(dev),
		      IS_ENABLED(CONFIG_IVSHMEM_DOORBELL) && DT_IRQ_HAS_IDX(FLAT0, 0));
	if (!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		zassert_equal(ivshmem_int_peer(dev, 1, 0), -ENOSYS);
		zassert_equal(ivshmem_register_handler(dev, NULL, 0), -ENOSYS);
		zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0) + 12)), DOORBELL_SENTINEL);
		return;
	}

	zassert_ok(ivshmem_int_peer(dev, 0x1234, 0));
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0) + 12)), 0x12340000);

	zassert_ok(ivshmem_int_peer(dev, UINT16_MAX, 0));
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0) + 12)), 0xffff0000);

	zassert_equal(ivshmem_int_peer(dev, 1, 1), -EINVAL);
	zassert_equal(ivshmem_int_peer(dev, 1, UINT16_MAX), -EINVAL);
	zassert_equal(ivshmem_int_peer(dev, 0x10000, 0), -EINVAL);
	zassert_equal(ivshmem_register_handler(dev, NULL, 1),
		      DT_IRQ_HAS_IDX(FLAT0, 0) ? -EINVAL : -ENOSYS);

	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0) + 12)), 0xffff0000);
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT1) + 12)), DOORBELL_SENTINEL);
}

ZTEST(ivshmem_flat, test_no_irq)
{
	const struct device *dev = DEVICE_DT_GET(NO_IRQ);
	struct k_poll_signal signal;
	uintptr_t mem;

	zassert_equal(ivshmem_get_vectors(dev), 0);
	zassert_equal(ivshmem_get_id(dev), 19);
	zassert_equal(ivshmem_get_mem(dev, &mem), DT_REG_SIZE_BY_NAME(NO_IRQ, shmem));
	zassert_equal(mem, DT_REG_ADDR_BY_NAME(NO_IRQ, shmem));
	sys_write32(0xfeedface, mem);
	zassert_equal(sys_read32(DT_REG_ADDR_BY_NAME(NO_IRQ, shmem)), 0xfeedface);

	k_poll_signal_init(&signal);
	zassert_equal(ivshmem_register_handler(dev, &signal, 0), -ENOSYS);
	zassert_equal(ivshmem_register_handler(dev, NULL, 0), -ENOSYS);
	if (!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		zassert_equal(ivshmem_int_peer(dev, 1, 0), -ENOSYS);
		zassert_equal(sys_le32_to_cpu(sys_read32(MMR(NO_IRQ) + 12)), DOORBELL_SENTINEL);
		return;
	}

	zassert_ok(ivshmem_int_peer(dev, 0x1234, 0));
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(NO_IRQ) + 12)), 0x12340000);
	zassert_equal(ivshmem_int_peer(dev, 1, 1), -EINVAL);
	zassert_equal(ivshmem_int_peer(dev, 1, UINT16_MAX), -EINVAL);
	zassert_equal(ivshmem_int_peer(dev, 0x10000, 0), -EINVAL);
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(NO_IRQ) + 12)), 0x12340000);
}

ZTEST(ivshmem_flat, test_reserved_registers)
{
	const struct device *dev = DEVICE_DT_GET(FLAT0);
	uintptr_t mem;

	(void)ivshmem_get_mem(dev, &mem);
	(void)ivshmem_get_id(dev);
	(void)ivshmem_get_vectors(dev);
	if (IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		zassert_ok(ivshmem_int_peer(dev, 1, 0));
	}
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0))), MASK_SENTINEL);
	zassert_equal(sys_read32(MMR(FLAT0) + 4), 0);
}

#if defined(CONFIG_IVSHMEM_DOORBELL) && DT_IRQ_HAS_IDX(FLAT0, 0) && DT_IRQ_HAS_IDX(FLAT1, 0)
ZTEST(ivshmem_flat, test_signal_lifecycle_and_isolation)
{
	const struct device *dev = DEVICE_DT_GET(FLAT0);
	const struct device *other = DEVICE_DT_GET(FLAT1);

	struct k_poll_signal first;
	struct k_poll_signal second;

	struct k_poll_event event =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &first);
	unsigned int signaled;
	int result;

	k_poll_signal_init(&first);
	k_poll_signal_init(&second);

	zassert_ok(ivshmem_register_handler(dev, &first, 0));
	zassert_ok(ivshmem_register_handler(other, &second, 0));
	zassert_equal(ivshmem_register_handler(dev, &second, 1), -EINVAL);

	NVIC_SetPendingIRQ((IRQn_Type)DT_IRQN(FLAT0));
	zassert_ok(k_poll(&event, 1, K_MSEC(100)));
	k_poll_signal_check(&first, &signaled, &result);
	zassert_equal(signaled, 1);
	zassert_equal(result, 0);

	k_poll_signal_check(&second, &signaled, &result);
	zassert_equal(signaled, 0);
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0))), MASK_SENTINEL);
	zassert_equal(sys_read32(MMR(FLAT0) + 4), 0);

	k_poll_signal_reset(&first);
	event.state = K_POLL_STATE_NOT_READY;
	sys_write32(sys_cpu_to_le32(0x87654321), MMR(FLAT0) + 4);
	zassert_ok(ivshmem_register_handler(dev, &second, 0));
	NVIC_SetPendingIRQ((IRQn_Type)DT_IRQN(FLAT0));
	event.signal = &second;
	zassert_ok(k_poll(&event, 1, K_MSEC(100)));
	k_poll_signal_check(&first, &signaled, &result);
	zassert_equal(signaled, 0);
	zassert_equal(sys_le32_to_cpu(sys_read32(MMR(FLAT0) + 4)), 0x87654321);

	zassert_ok(ivshmem_register_handler(dev, NULL, 0));
	k_poll_signal_reset(&second);
	event.state = K_POLL_STATE_NOT_READY;
	NVIC_SetPendingIRQ((IRQn_Type)DT_IRQN(FLAT0));
	zassert_equal(k_poll(&event, 1, K_MSEC(20)), -EAGAIN);
	NVIC_SetPendingIRQ((IRQn_Type)DT_IRQN(FLAT1));
	zassert_ok(k_poll(&event, 1, K_MSEC(100)));
	zassert_ok(ivshmem_register_handler(other, NULL, 0));
}
#endif

ZTEST_SUITE(ivshmem_flat, NULL, NULL, prepare, NULL, NULL);
