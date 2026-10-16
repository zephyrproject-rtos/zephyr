/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 * Copyright (c) 2023, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_retained_reg

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_zephyr_reg, CONFIG_RETAINED_MEM_LOG_LEVEL);

#define IS_ALIGN_4(inst) \
	+ IS_EQ(DT_INST_PROP(inst, alignment), 4)

#define IS_ALIGN_8(inst) \
	+ IS_EQ(DT_INST_PROP(inst, alignment), 8)

#define ANY_ALIGN_4() \
	0 DT_INST_FOREACH_STATUS_OKAY(IS_ALIGN_4)

#define ANY_ALIGN_8() \
	0 DT_INST_FOREACH_STATUS_OKAY(IS_ALIGN_8)

struct zephyr_retained_mem_reg_data {
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

struct zephyr_retained_mem_reg_config {
	uint8_t *address;
	size_t size;
	void *(*memcpy)(void *s1, const void *s2, size_t n);
	void *(*memset)(void *s, int c, size_t n);
	size_t alignment;
};

#if ANY_ALIGN_4()

static void *memcpy4(void *s1, const void *s2, size_t n)
{
	mem_addr_t d = (mem_addr_t)s1;
	mem_addr_t s = (mem_addr_t)s2;

	__ASSERT_NO_MSG(((d | s | n) & 0x3) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint32_t)) {
		sys_write32(sys_read32(s + i), d + i);
	}

	return s1;
}

static void *memset4(void *s, int c, size_t n)
{
	mem_addr_t d = (mem_addr_t)s;
	uint32_t v = (uint8_t)c * 0x01010101U;

	__ASSERT_NO_MSG(((d | n) & 0x3) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint32_t)) {
		sys_write32(v, d + i);
	}

	return s;
}

#endif

#if ANY_ALIGN_8()

#ifdef CONFIG_64BIT

static void *memcpy8(void *s1, const void *s2, size_t n)
{
	mem_addr_t d = (mem_addr_t)s1;
	mem_addr_t s = (mem_addr_t)s2;

	__ASSERT_NO_MSG(((d | s | n) & 0x7) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
		sys_write64(sys_read64(s + i), d + i);
	}

	return s1;
}

static void *memset8(void *s, int c, size_t n)
{
	mem_addr_t d = (mem_addr_t)s;
	uint64_t v = (uint8_t)c * 0x0101010101010101ULL;

	__ASSERT_NO_MSG(((d | n) & 0x7) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
		sys_write64(v, d + i);
	}

	return s;
}

#else

#error Invalid configuration

#endif

#endif

static inline void zephyr_retained_mem_reg_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void zephyr_retained_mem_reg_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int zephyr_retained_mem_reg_init(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	return 0;
}

static ssize_t zephyr_retained_mem_reg_size(const struct device *dev)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	return (ssize_t)config->size;
}

static int zephyr_retained_mem_reg_read(const struct device *dev, off_t offset, uint8_t *buffer,
					size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	if (size % config->alignment != 0) {
		return -EINVAL;
	}

	zephyr_retained_mem_reg_lock_take(dev);

	config->memcpy(buffer, (config->address + offset), size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_write(const struct device *dev, off_t offset,
					 const uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	if (size % config->alignment != 0) {
		return -EINVAL;
	}

	zephyr_retained_mem_reg_lock_take(dev);

	config->memcpy((config->address + offset), buffer, size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_clear(const struct device *dev)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	config->memset(config->address, 0, config->size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, zephyr_retained_mem_reg_api) = {
	.size = zephyr_retained_mem_reg_size,
	.read = zephyr_retained_mem_reg_read,
	.write = zephyr_retained_mem_reg_write,
	.clear = zephyr_retained_mem_reg_clear,
};

#define ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMCPY(inst)						\
	COND_CASE_1(IS_EQ(DT_INST_PROP(inst, alignment), 8), (memcpy8),				\
		    IS_EQ(DT_INST_PROP(inst, alignment), 4), (memcpy4),				\
		    (memcpy))

#define ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMSET(inst)						\
	COND_CASE_1(IS_EQ(DT_INST_PROP(inst, alignment), 8), (memset8),				\
		    IS_EQ(DT_INST_PROP(inst, alignment), 4), (memset4),				\
		    (memset))

#define ZEPHYR_RETAINED_MEM_REG_DEVICE(inst)							\
	static struct zephyr_retained_mem_reg_data zephyr_retained_mem_reg_data_##inst;		\
	static const struct zephyr_retained_mem_reg_config					\
			zephyr_retained_mem_reg_config_##inst = {				\
		.address = (uint8_t *)DT_INST_REG_ADDR(inst),					\
		.size = DT_INST_REG_SIZE(inst),							\
		.memcpy = ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMCPY(inst),			\
		.memset = ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMSET(inst),			\
		.alignment = DT_INST_PROP(inst, alignment)					\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &zephyr_retained_mem_reg_init,					\
			      NULL,								\
			      &zephyr_retained_mem_reg_data_##inst,				\
			      &zephyr_retained_mem_reg_config_##inst,				\
			      POST_KERNEL,							\
			      CONFIG_RETAINED_MEM_INIT_PRIORITY,				\
			      &zephyr_retained_mem_reg_api);

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_RETAINED_MEM_REG_DEVICE)
