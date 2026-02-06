/*
 * Copyright (c) 2025 Felipe Neves <ryukokki.felipe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_lvgl_gpu_vglite

#include <string.h>

#include <lvgl.h>
#include <libs/vg_lite_driver/inc/vg_lite.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_heap.h>

#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_kernel.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hal.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hw.h>

#define VGLITE_INST		0
#define VGLITE_NODE		DT_INST(VGLITE_INST, zephyr_lvgl_gpu_vglite)
#define VGLITE_GPU_NODE		DT_PHANDLE(VGLITE_NODE, gpu)
#define VGLITE_GPU_BASE		DT_REG_ADDR(VGLITE_GPU_NODE)
#define VGLITE_GPU_IRQN		DT_IRQN(VGLITE_GPU_NODE)

#define VGLITE_TESS_H		1280
#define VGLITE_TESS_W		128
#define VGLITE_COMMAND_BUF_SIZE (256 * 1024)

static char __nocache vg_lite_heap_mem[CONFIG_LV_Z_VGLITE_HEAP_SIZE] __aligned(32);
static struct sys_heap vg_lite_heap;
static struct k_spinlock vg_lite_heap_lock;

typedef void (*irq_config_func_t)(const struct device *dev);

struct vg_lite_device_runtime {
	uint32_t register_base; /* Always use physical for register access in RTOS. */
	volatile uint32_t int_flags;
	struct k_sem sync;
	void *device;
	vg_lite_gpu_execute_state_t gpu_execute_state;
};

struct z_vglite_config {
	uint32_t vglite_gpu_base_address;
	uint32_t vglite_gpu_irq_line;
	irq_config_func_t irq_config;
};

struct z_vglite_data {
	struct vg_lite_device_runtime *vglite_device;
};

static struct vg_lite_device_runtime *vglite_dev;

vg_lite_error_t vg_lite_hal_allocate(uint32_t size, void **memory)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;
	k_spinlock_key_t key;

	key = k_spin_lock(&vg_lite_heap_lock);
	*memory = sys_heap_alloc(&vg_lite_heap, size);
	k_spin_unlock(&vg_lite_heap_lock, key);
	if (*memory == NULL) {
		error = VG_LITE_OUT_OF_MEMORY;
	}

	return error;
}

vg_lite_error_t vg_lite_hal_free(void *memory)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&vg_lite_heap_lock);
	sys_heap_free(&vg_lite_heap, memory);
	k_spin_unlock(&vg_lite_heap_lock, key);

	return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_allocate_contiguous(unsigned long size,
						vg_lite_vidmem_pool_t pool,
						void **logical,
						void **klogical,
						uint32_t *physical,
						void **node)
{
	ARG_UNUSED(pool);
	ARG_UNUSED(node);

	/* Align the size to 64 bytes. */
	uint32_t aligned_size = (uint32_t)((size + 63U) & ~63U);
	vg_lite_error_t ret = vg_lite_hal_allocate(aligned_size, logical);

	if (ret != VG_LITE_SUCCESS) {
		return ret;
	}

	*klogical = *logical;
	*physical = (uint32_t)(uintptr_t)(*logical);

	return VG_LITE_SUCCESS;
}

void *vg_lite_os_malloc(uint32_t size)
{
	k_spinlock_key_t key;
	void *ret;

	key = k_spin_lock(&vg_lite_heap_lock);
	ret = sys_heap_alloc(&vg_lite_heap, size);
	k_spin_unlock(&vg_lite_heap_lock, key);

	return ret;
}

void vg_lite_os_free(void *memory)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&vg_lite_heap_lock);
	sys_heap_free(&vg_lite_heap, memory);
	k_spin_unlock(&vg_lite_heap_lock, key);
}

void vg_lite_hal_delay(uint32_t ms)
{
	k_msleep(ms);
}

uint32_t vg_lite_hal_peek(uint32_t address)
{
	return sys_read32((mem_addr_t)(vglite_dev->register_base + address));
}

void vg_lite_hal_poke(uint32_t address, uint32_t data)
{
	sys_write32(data, (mem_addr_t)(vglite_dev->register_base + address));
}

int32_t vg_lite_hal_wait_interrupt(uint32_t timeout, uint32_t mask, uint32_t *value)
{
	int ret = k_sem_take(&vglite_dev->sync, K_MSEC(timeout));

	if (!ret) {
		if (value != NULL) {
			*value = vglite_dev->int_flags & mask;
		}

		vglite_dev->int_flags = 0U;

		return 1;
	}

	return 0;
}

vg_lite_error_t vg_lite_hal_map_memory(vg_lite_kernel_map_memory_t *node)
{
	node->logical = (void *)(uintptr_t)node->physical;

	return VG_LITE_SUCCESS;
}

void vg_lite_set_gpu_execute_state(vg_lite_gpu_execute_state_t state)
{
	vglite_dev->gpu_execute_state = state;
}

static void z_vg_lite_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct z_vglite_data *dev_data = dev->data;
	struct vg_lite_device_runtime *vg_dev = dev_data->vglite_device;
	uint32_t flags = vg_lite_hal_peek(VG_LITE_INTR_STATUS);

	if (flags != 0U) {
		vg_dev->int_flags |= flags;
		k_sem_give(&vg_dev->sync);
	}
}

static void z_vglite_irq_config_func(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(VGLITE_GPU_IRQN, 0, z_vg_lite_isr,
		    DEVICE_DT_INST_GET(VGLITE_INST), 0);
	irq_enable(VGLITE_GPU_IRQN);
}

static int z_vg_lite_init(const struct device *dev)
{
	const struct z_vglite_config *cfg = dev->config;
	struct z_vglite_data *data = dev->data;
	vg_lite_error_t err;

	sys_heap_init(&vg_lite_heap, &vg_lite_heap_mem[0], CONFIG_LV_Z_VGLITE_HEAP_SIZE);

	err = vg_lite_hal_allocate(sizeof(*vglite_dev), (void **)&vglite_dev);
	if (err != VG_LITE_SUCCESS || vglite_dev == NULL) {
		return -ENOMEM;
	}

	memset(vglite_dev, 0, sizeof(*vglite_dev));

	vglite_dev->register_base = cfg->vglite_gpu_base_address;
	k_sem_init(&vglite_dev->sync, 0, 1);
	vglite_dev->int_flags = 0U;

	data->vglite_device = vglite_dev;

	cfg->irq_config(dev);

	vg_lite_init(VGLITE_TESS_W, VGLITE_TESS_H);
	vg_lite_set_command_buffer_size(VGLITE_COMMAND_BUF_SIZE);

	return 0;
}

static const struct z_vglite_config z_vglite_config_0 = {
	.vglite_gpu_base_address = VGLITE_GPU_BASE,
	.vglite_gpu_irq_line = VGLITE_GPU_IRQN,
	.irq_config = z_vglite_irq_config_func,
};

static struct z_vglite_data z_vglite_data_0;

DEVICE_DT_INST_DEFINE(0,
	      z_vg_lite_init,
	      NULL,
	      &z_vglite_data_0,
	      &z_vglite_config_0,
	      POST_KERNEL,
	      CONFIG_APPLICATION_INIT_PRIORITY,
	      NULL);
