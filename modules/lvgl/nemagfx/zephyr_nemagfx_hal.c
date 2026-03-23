/* Copyright (c) 2026 Silicon Signals Pvt. Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lv_conf_internal.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <lvgl.h>
#include <nema_core.h>
#include <nema_sys_defs.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include "tsi_malloc.h"

/********** Devicetree **********/

#define GPU2D_NODE		DT_INST(0, st_neochrom_gpu2d)
#define GPU2D_IRQN		DT_IRQ_BY_IDX(GPU2D_NODE, 0, irq)
#define GPU2D_IRQ_PRIO		DT_IRQ_BY_IDX(GPU2D_NODE, 0, priority)
#define GPU2D_ERR_IRQN		DT_IRQ_BY_IDX(GPU2D_NODE, 1, irq)
#define GPU2D_ERR_IRQ_PRIO	DT_IRQ_BY_IDX(GPU2D_NODE, 1, priority)

/********** Defines **********/

#define RING_SIZE	CONFIG_LV_NEMA_HAL_RING_SIZE

/* NemaGFX byte pool size in bytes.
 * One byte per pixel for masking/stenciling plus 10240 for
 * additional allocations.
 */
#if defined(LV_NEMA_GFX_MAX_RESX) && defined(LV_NEMA_GFX_MAX_RESY)
	#define NEMAGFX_MEM_POOL_SIZE (LV_NEMA_GFX_MAX_RESX * LV_NEMA_GFX_MAX_RESY + 10240)
#else
	/* LV_USE_NEMA_VG is 0 so masking/stenciling memory is not needed. */
	#define NEMAGFX_MEM_POOL_SIZE	10240
#endif

/********** Runtime **********/

struct zephyr_nemagfx_runtime {
	mem_addr_t register_base;
	struct k_sem sync;
	volatile uint32_t last_cl_id;
	GPU2D_HandleTypeDef hgpu2d;
	bool initialized;
};

/********** Static Variables **********/

static uint8_t nemagfx_pool_mem[NEMAGFX_MEM_POOL_SIZE];
static nema_ringbuffer_t ring_buffer_str;

static struct zephyr_nemagfx_runtime gpu2d_rt;

/********** IRQ / HAL **********/

static void zephyr_nemagfx_gpu2d_isr(const void *arg)
{
	ARG_UNUSED(arg);

	HAL_GPU2D_IRQHandler(&gpu2d_rt.hgpu2d);
	k_sem_give(&gpu2d_rt.sync);
}

static void zephyr_nemagfx_gpu2d_error_isr(const void *arg)
{
	ARG_UNUSED(arg);

	HAL_GPU2D_ER_IRQHandler(&gpu2d_rt.hgpu2d);
}

static int zephyr_nemagfx_gpu2d_init(void)
{
	const struct device *gpu2d_dev = DEVICE_DT_GET(GPU2D_NODE);

	if (gpu2d_rt.initialized) {
		return 0;
	}

	if (!device_is_ready(gpu2d_dev)) {
		return -ENODEV;
	}

	k_sem_init(&gpu2d_rt.sync, 0, 1);
	gpu2d_rt.last_cl_id = 0U;
	gpu2d_rt.register_base = DT_REG_ADDR(GPU2D_NODE);
	gpu2d_rt.hgpu2d.Instance = DT_REG_ADDR(GPU2D_NODE);

	IRQ_CONNECT(GPU2D_IRQN, GPU2D_IRQ_PRIO, zephyr_nemagfx_gpu2d_isr, NULL, 0);
	irq_enable(GPU2D_IRQN);

	IRQ_CONNECT(GPU2D_ERR_IRQN, GPU2D_ERR_IRQ_PRIO, zephyr_nemagfx_gpu2d_error_isr, NULL, 0);
	irq_enable(GPU2D_ERR_IRQN);

	gpu2d_rt.initialized = true;

	return 0;
}

/********** Callback **********/
void HAL_GPU2D_CommandListCpltCallback(GPU2D_HandleTypeDef *hgpu2d, uint32_t CmdListID)
{
	LV_UNUSED(hgpu2d);

	gpu2d_rt.last_cl_id = CmdListID;
}

/********** Nema Sys **********/

int32_t nema_sys_init(void)
{
	int error_code;
	int ret;

	ret = zephyr_nemagfx_gpu2d_init();
	if (ret < 0) {
		LV_LOG_ERROR("Unable to initialize GPU2D HAL support");
		return ret;
	}

	error_code = tsi_malloc_init_pool_aligned(0,
						  (void *)nemagfx_pool_mem,
						  (uintptr_t)nemagfx_pool_mem,
						  NEMAGFX_MEM_POOL_SIZE,
						  1, 8);

	if (error_code < 0) {
		LV_LOG_ERROR("Unable to initialize NemaGFX pool memory");
		return error_code;
	}

	ring_buffer_str.bo = nema_buffer_create(RING_SIZE);

	if (ring_buffer_str.bo.base_virt == NULL) {
		LV_LOG_ERROR("Unable to allocate ring buffer memory");
		return -ENOMEM;
	}

	return nema_rb_init(&ring_buffer_str, 1);
}

uint32_t nema_reg_read(uint32_t reg)
{
	return sys_read32(gpu2d_rt.register_base + reg);
}

void nema_reg_write(uint32_t reg, uint32_t value)
{
	sys_write32(value, gpu2d_rt.register_base + reg);
}

int nema_wait_irq(void)
{
	return k_sem_take(&gpu2d_rt.sync, K_FOREVER);
}

int nema_wait_irq_cl(int cl_id)
{
	while (gpu2d_rt.last_cl_id < (uint32_t)cl_id) {
		(void)nema_wait_irq();
	}

	return 0;
}

/********** Memory **********/

void nema_host_free(void *ptr)
{
	tsi_free(ptr);
}

void *nema_host_malloc(unsigned int size)
{
	return tsi_malloc(size);
}

nema_buffer_t nema_buffer_create(int size)
{
	nema_buffer_t bo = { 0 };

	bo.base_virt = tsi_malloc(size);
	bo.base_phys = (uintptr_t)bo.base_virt;
	bo.size = size;
	bo.fd = -1;

	LV_ASSERT_MSG(bo.base_virt != NULL, "Unable to allocate memory");

	return bo;
}

nema_buffer_t nema_buffer_create_pool(int pool, int size)
{
	LV_UNUSED(pool);

	return nema_buffer_create(size);
}

void nema_buffer_flush(nema_buffer_t *bo)
{
	LV_UNUSED(bo);

	sys_cache_data_flush_and_invd_all();
}

void *nema_buffer_map(nema_buffer_t *bo)
{
	return bo->base_virt;
}

void nema_buffer_unmap(nema_buffer_t *bo)
{
	LV_UNUSED(bo);
}

void nema_buffer_destroy(nema_buffer_t *bo)
{
	if (bo->base_virt == NULL) {
		return;
	}

	tsi_free(bo->base_virt);

	bo->base_virt = NULL;
	bo->base_phys = 0U;
	bo->size = 0;
	bo->fd = -1;
}

uintptr_t nema_buffer_phys(nema_buffer_t *bo)
{
	return bo->base_phys;
}

/********** Mutex **********/

int nema_mutex_lock(int mutex_id)
{
	LV_UNUSED(mutex_id);

	return 0;
}

int nema_mutex_unlock(int mutex_id)
{
	LV_UNUSED(mutex_id);

	return 0;
}
