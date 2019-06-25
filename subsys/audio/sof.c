/* sof.c - Sound Open Firmware handling */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sof, CONFIG_SOF_LOG_LEVEL);

#include "sof/sof.h"
#include "sof/ipc.h"

/* SRAM window 0 FW "registers" */
#define SRAM_REG_ROM_STATUS                     0x0
#define SRAM_REG_FW_STATUS                      0x4
#define SRAM_REG_FW_TRACEP                      0x8
#define SRAM_REG_FW_IPC_RECEIVED_COUNT          0xc
#define SRAM_REG_FW_IPC_PROCESSED_COUNT         0x10
#define SRAM_REG_FW_END                         0x14

static struct sof sof;

#define CASE(x) case TRACE_CLASS_##x: return #x

/* look up subsystem class name from table */
char *get_trace_class(uint32_t trace_class)
{
	switch (trace_class) {
		CASE(IRQ);
		CASE(IPC);
		CASE(PIPE);
		CASE(HOST);
		CASE(DAI);
		CASE(DMA);
		CASE(SSP);
		CASE(COMP);
		CASE(WAIT);
		CASE(LOCK);
		CASE(MEM);
		CASE(MIXER);
		CASE(BUFFER);
		CASE(VOLUME);
		CASE(SWITCH);
		CASE(MUX);
		CASE(SRC);
		CASE(TONE);
		CASE(EQ_FIR);
		CASE(EQ_IIR);
		CASE(SA);
		CASE(DMIC);
		CASE(POWER);
	default: return "unknown";
	}
}

/* print debug messages */
void debug_print(char *message)
{
	LOG_DBG("%s", message);
}

/* FIXME: The following definitions are to satisfy linker errors */
struct dai dai;

/* Make use of dai_install to register DAI drivers, there maybe common code
 * from i2s_cavs.c and codec.h */
struct dai *dai_get(uint32_t type, uint32_t index, uint32_t flags)
{
	return &dai;
}

void dai_put(struct dai *dai)
{
}

struct dma dma;

struct dma *dma_get(uint32_t dir, uint32_t caps, uint32_t dev, uint32_t flags)
{
	return &dma;
}

void dma_put(struct dma *dma)
{
}

int dma_sg_alloc(struct dma_sg_elem_array *elem_array,
		 int zone,
		 uint32_t direction,
		 uint32_t buffer_count, uint32_t buffer_bytes,
		 uintptr_t dma_buffer_addr, uintptr_t external_addr)
{
	return 0;
}

void dma_sg_free(struct dma_sg_elem_array *elem_array)
{
}

static void sys_module_init(void)
{
	Z_STRUCT_SECTION_FOREACH(_sof_module, mod) {
		mod->init();
		LOG_INF("module %p init", mod);
	}
}

static int sof_init(struct device *unused)
{
	int ret;

	/* init components */
	sys_comp_init();

	/* init static modules */
	sys_module_init();

	/* init IPC */
	ret = ipc_init(&sof);
	if (ret < 0) {
		LOG_ERR("IPC init: %d", ret);
		return ret;
	}

	LOG_INF("IPC initialized");

	/* init scheduler */
	ret = scheduler_init();
	if (ret < 0) {
		LOG_ERR("scheduler init: %d", ret);
		return ret;
	}

	LOG_INF("scheduler initialized");

#if defined(CONFIG_SOF_STATIC_PIPELINE)
	/* init static pipeline */
	ret = init_static_pipeline(sof.ipc);
	if (ret < 0) {
		LOG_ERR("static pipeline init: %d", ret);
		return ret;
	}

	LOG_INF("pipeline initialized");
#endif /* CONFIG_SOF_STATIC_PIPELINE */

	mailbox_sw_reg_write(SRAM_REG_ROM_STATUS, 0xabbac0fe);

	return 0;
}

SYS_INIT(sof_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
