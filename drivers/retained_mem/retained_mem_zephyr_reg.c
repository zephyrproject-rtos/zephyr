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
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_zephyr_reg, CONFIG_RETAINED_MEM_LOG_LEVEL);

#define IS_ACCESS_4(inst) \
	+ IS_EQ(DT_INST_PROP(inst, reg_io_width), 4)

#define IS_ACCESS_8(inst) \
	+ IS_EQ(DT_INST_PROP(inst, reg_io_width), 8)

#define ANY_ACCESS_4() \
	(0 DT_INST_FOREACH_STATUS_OKAY(IS_ACCESS_4))

#define ANY_ACCESS_8() \
	(0 DT_INST_FOREACH_STATUS_OKAY(IS_ACCESS_8))

#define ANY_WIDE() (ANY_ACCESS_4() || ANY_ACCESS_8())

struct zephyr_retained_mem_reg_data {
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

struct zephyr_retained_mem_reg_config {
	mem_addr_t address;
	size_t size;
	void (*read)(uint8_t *dst, mem_addr_t src, size_t n);
	void (*write)(mem_addr_t dst, const uint8_t *src, size_t n);
	void (*set)(mem_addr_t dst, int c, size_t n);
	size_t reg_io_width;
};

static __maybe_unused void reg_read4(uint8_t *dst, mem_addr_t src, size_t n)
{
	/* Only the register side needs to be word aligned */
	__ASSERT_NO_MSG(((src | n) & 0x3) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint32_t)) {
		UNALIGNED_PUT(sys_read32(src + i), (uint32_t *)&dst[i]);
	}
}

static __maybe_unused void reg_write4(mem_addr_t dst, const uint8_t *src, size_t n)
{
	__ASSERT_NO_MSG(((dst | n) & 0x3) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint32_t)) {
		sys_write32(UNALIGNED_GET((const uint32_t *)&src[i]), dst + i);
	}
}

static __maybe_unused void reg_set4(mem_addr_t dst, int c, size_t n)
{
	uint32_t v = (uint8_t)c * 0x01010101U;

	__ASSERT_NO_MSG(((dst | n) & 0x3) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint32_t)) {
		sys_write32(v, dst + i);
	}
}

#ifdef CONFIG_64BIT

static __maybe_unused void reg_read8(uint8_t *dst, mem_addr_t src, size_t n)
{
	__ASSERT_NO_MSG(((src | n) & 0x7) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
		UNALIGNED_PUT(sys_read64(src + i), (uint64_t *)&dst[i]);
	}
}

static __maybe_unused void reg_write8(mem_addr_t dst, const uint8_t *src, size_t n)
{
	__ASSERT_NO_MSG(((dst | n) & 0x7) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
		sys_write64(UNALIGNED_GET((const uint64_t *)&src[i]), dst + i);
	}
}

static __maybe_unused void reg_set8(mem_addr_t dst, int c, size_t n)
{
	uint64_t v = (uint8_t)c * 0x0101010101010101ULL;

	__ASSERT_NO_MSG(((dst | n) & 0x7) == 0);

	for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
		sys_write64(v, dst + i);
	}
}

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

static void zephyr_retained_mem_reg_read_aligned(const struct device *dev,
						 off_t offset, uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;
	const off_t start = ROUND_DOWN(offset, config->reg_io_width);
	const off_t start_aligned = ROUND_UP(offset, config->reg_io_width);
	const off_t start_size = MIN(start_aligned - offset, (off_t)size);
	const off_t end = ROUND_UP((offset + size), config->reg_io_width);
	const off_t end_aligned = ROUND_DOWN((offset + size), config->reg_io_width);
	const off_t end_size = (offset + size) - end_aligned;
	uint8_t tmp_buf[sizeof(uint64_t)];

	if (start != start_aligned) {
		config->read(tmp_buf, config->address + start, config->reg_io_width);
		memcpy(buffer, &tmp_buf[offset - start], start_size);
	}

	if (end_aligned > start_aligned) {
		config->read(&buffer[start_size], config->address + start_aligned,
			     (end_aligned - start_aligned));
	}

	if (end != end_aligned && (end_aligned >= start_aligned)) {
		config->read(tmp_buf, config->address + end_aligned, config->reg_io_width);
		memcpy(&buffer[size - end_size], tmp_buf, end_size);
	}
}

static void zephyr_retained_mem_reg_write_aligned(const struct device *dev,
						  off_t offset, const uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;
	const off_t start = ROUND_DOWN(offset, config->reg_io_width);
	const off_t start_aligned = ROUND_UP(offset, config->reg_io_width);
	const off_t start_size = MIN(start_aligned - offset, (off_t)size);
	const off_t end = ROUND_UP((offset + size), config->reg_io_width);
	const off_t end_aligned = ROUND_DOWN((offset + size), config->reg_io_width);
	const off_t end_size = (offset + size) - end_aligned;
	uint8_t tmp_buf[sizeof(uint64_t)];

	if (start != start_aligned) {
		config->read(tmp_buf, config->address + start, config->reg_io_width);
		memcpy(&tmp_buf[offset - start], buffer, start_size);
		config->write(config->address + start, tmp_buf, config->reg_io_width);
	}

	if (end_aligned > start_aligned) {
		config->write(config->address + start_aligned, &buffer[start_size],
			      (end_aligned - start_aligned));
	}

	if (end != end_aligned && (end_aligned >= start_aligned)) {
		config->read(tmp_buf, config->address + end_aligned, config->reg_io_width);
		memcpy(tmp_buf, &buffer[size - end_size], end_size);
		config->write(config->address + end_aligned, tmp_buf, config->reg_io_width);
	}
}

static int zephyr_retained_mem_reg_read(const struct device *dev, off_t offset, uint8_t *buffer,
					size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	if (config->reg_io_width > 1 && ANY_WIDE()) {
		zephyr_retained_mem_reg_read_aligned(dev, offset, buffer, size);
	} else {
		memcpy(buffer, (const void *)(config->address + offset), size);
	}

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_write(const struct device *dev, off_t offset,
					 const uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	if (config->reg_io_width > 1 && ANY_WIDE()) {
		zephyr_retained_mem_reg_write_aligned(dev, offset, buffer, size);
	} else {
		memcpy((void *)(config->address + offset), buffer, size);
	}

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_clear(const struct device *dev)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	if (config->reg_io_width > 1 && ANY_WIDE()) {
		config->set(config->address, 0, config->size);
	} else {
		memset((void *)config->address, 0, config->size);
	}

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, zephyr_retained_mem_reg_api) = {
	.size = zephyr_retained_mem_reg_size,
	.read = zephyr_retained_mem_reg_read,
	.write = zephyr_retained_mem_reg_write,
	.clear = zephyr_retained_mem_reg_clear,
};

#define ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_READ(inst)						\
	COND_CASE_1(IS_EQ(DT_INST_PROP(inst, reg_io_width), 8), (reg_read8),			\
		    IS_EQ(DT_INST_PROP(inst, reg_io_width), 4), (reg_read4),			\
		    (NULL))

#define ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_WRITE(inst)						\
	COND_CASE_1(IS_EQ(DT_INST_PROP(inst, reg_io_width), 8), (reg_write8),			\
		    IS_EQ(DT_INST_PROP(inst, reg_io_width), 4), (reg_write4),			\
		    (NULL))

#define ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMSET(inst)						\
	COND_CASE_1(IS_EQ(DT_INST_PROP(inst, reg_io_width), 8), (reg_set8),			\
		    IS_EQ(DT_INST_PROP(inst, reg_io_width), 4), (reg_set4),			\
		    (NULL))

#define ZEPHYR_RETAINED_MEM_REG_DEVICE(inst)							\
	BUILD_ASSERT((DT_INST_REG_ADDR(inst) % DT_INST_PROP(inst, reg_io_width)) == 0		\
		     && (DT_INST_REG_SIZE(inst) % DT_INST_PROP(inst, reg_io_width)) == 0,	\
		     "retained-reg address/size must be multiples of reg-io-width");		\
	BUILD_ASSERT(IS_ENABLED(CONFIG_64BIT) || DT_INST_PROP(inst, reg_io_width) != 8,		\
		     "reg-io-width = <8> requires a 64-bit target");				\
	static struct zephyr_retained_mem_reg_data zephyr_retained_mem_reg_data_##inst;		\
	static const struct zephyr_retained_mem_reg_config					\
			zephyr_retained_mem_reg_config_##inst = {				\
		.address = (mem_addr_t)DT_INST_REG_ADDR(inst),					\
		.size = DT_INST_REG_SIZE(inst),							\
		.read = ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_READ(inst),				\
		.write = ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_WRITE(inst),			\
		.set = ZEPHYR_RETAINED_MEM_REG_DEVICE_GET_MEMSET(inst),				\
		.reg_io_width = DT_INST_PROP(inst, reg_io_width),				\
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
