/*
 * Copyright (c) 2025 Dan Collins
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 *
 * Takes inspiration from misc/nordic_vpr_launcher which is
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp2_cpu1

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
#include "hardware/structs/sio.h"
#include "hardware/structs/psm.h"

LOG_MODULE_REGISTER(rp2_cpu1_launcher, CONFIG_RP2_CPU1_LAUNCHER_LOG_LEVEL);

struct rpi_pico_mailbox_config {
	sio_hw_t *const sio_regs;
	psm_hw_t *const psm_regs;
};

static inline void rpi_pico_mailbox_put_blocking(sio_hw_t *const sio_regs, uint32_t value)
{
	while (!(sio_regs->fifo_st & SIO_FIFO_ST_RDY_BITS)) {
	}

	sio_regs->fifo_wr = value;

	/* Inform other CPU about FIFO update. */
	__SEV();
}

static inline uint32_t rpi_pico_mailbox_pop_blocking(sio_hw_t *const sio_regs)
{
	while (!(sio_regs->fifo_st & SIO_FIFO_ST_VLD_BITS)) {
		/* Wait for a message to be available in the FIFO. */
		/* Before IRQ is enables, this is signalled by an event */
		__WFE();
	}

	return sio_regs->fifo_rd;
}

#if DT_INST_NODE_HAS_PROP(0, execution_memory)
static int rpi_pico_reset_cpu1(sio_hw_t *const sio_regs, psm_hw_t *const psm_regs)
{
	uint32_t val;

	/* Power off, and wait for it to take effect. */
	hw_set_bits(&psm_regs->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	while (!(psm_regs->frce_off & PSM_FRCE_OFF_PROC1_BITS)) {
	}

	/* Power back on, and we can wait for a '0' in the FIFO to know
	 * that it has come back.
	 */
	hw_clear_bits(&psm_regs->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	val = rpi_pico_mailbox_pop_blocking(sio_regs);

	return val == 0 ? 0 : -EIO;
}

static void rpi_pico_boot_cpu1(sio_hw_t *const sio_regs, uint32_t vector_table_addr,
	uint32_t stack_ptr, uint32_t pc)
{
	/* We synchronise with CPU1 and then we can hand over the memory addresses. */
	uint32_t cmds[] = {0, 0, 1, vector_table_addr, stack_ptr, pc};
	uint32_t seq = 0;

	do {
		uint32_t cmd = cmds[seq], rsp;

		if (cmd == 0) {
			/* Flush the mailbox by reading all pending messages. */
			while (sio_regs->fifo_st & SIO_FIFO_ST_VLD_BITS) {
				(void)sio_regs->fifo_rd;
			}

			/* Signal readiness to CPU1 */
			__SEV();
		}

		rpi_pico_mailbox_put_blocking(sio_regs, cmd);
		rsp = rpi_pico_mailbox_pop_blocking(sio_regs);

		seq = (cmd == rsp) ? seq + 1 : 0;
	} while (seq < ARRAY_SIZE(cmds));
}

#if CONFIG_RP2_CPU1_LAUNCHER_LOG_LEVEL >= LOG_LEVEL_DBG
static inline bool address_in_range(uint32_t addr, uint32_t base, uint32_t size)
{
	return addr >= base && addr < base + size;
}

static inline int rpi_pico_validate_image(uint32_t cpu1_sp, uint32_t cpu1_pc)
{
#if DT_INST_NODE_HAS_PROP(0, memory_region)
	/* Make sure sp is within available RAM, and pc is within the image. */
	uint32_t sram_start = DT_REG_ADDR(DT_INST_PHANDLE(0, memory_region));
	uint32_t sram_size = DT_REG_SIZE(DT_INST_PHANDLE(0, memory_region));

	/* Stack pointer shall point within RAM assigned to the core. */
	if (!address_in_range(cpu1_sp, sram_start, sram_size)) {
		LOG_ERR("CPU1 stack pointer 0x%08x invalid.", cpu1_sp);
		return -EINVAL;
	}

	LOG_DBG("CPU1 stack pointer: 0x%08x", cpu1_sp);
#endif /* DT_INST_NODE_HAS_PROP(0, memory_region) */

	/* Initial program counter shall point to code within the CPU1 partition. */
	if (!address_in_range(cpu1_pc, DT_REG_ADDR(DT_INST_PHANDLE(0, execution_memory)),
			      DT_REG_SIZE(DT_INST_PHANDLE(0, execution_memory)))) {
		LOG_ERR("CPU1 reset pointer 0x%08x invalid.", cpu1_pc);
		return -EINVAL;
	}

	LOG_DBG("CPU1 reset pointer: 0x%08x", cpu1_pc);
	return 0;
}
#endif /* CONFIG_RP2_CPU1_LAUNCHER_LOG_LEVEL >= LOG_LEVEL_DBG */
#endif /* DT_INST_NODE_HAS_PROP(0, execution_memory) */

static int rpi_pico_cpu1_init(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, execution_memory)
#if DT_INST_NODE_HAS_PROP(0, image_source)
	BUILD_ASSERT((DT_REG_SIZE(DT_INST_PHANDLE(0, execution_memory)) >=
					DT_REG_SIZE(DT_INST_PHANDLE(0, image_source))),
					"Image size must not exceed execution memory size");

	BUILD_ASSERT(((DT_REG_ADDR(DT_INST_PHANDLE(0, execution_memory)) >=
					DT_REG_ADDR(DT_INST_PHANDLE(0, image_source)) +
					DT_REG_SIZE(DT_INST_PHANDLE(0, image_source))) ||
				(DT_REG_ADDR(DT_INST_PHANDLE(0, image_source)) >=
					DT_REG_ADDR(DT_INST_PHANDLE(0, execution_memory)) +
					DT_REG_SIZE(DT_INST_PHANDLE(0, execution_memory)))),
		"Image source memory must not overlap with execution memory");

	void *src_mem = (void *)DT_REG_ADDR(DT_INST_PHANDLE(0, image_source));
	void *exec_mem = (void *)DT_REG_ADDR(DT_INST_PHANDLE(0, execution_memory));

	LOG_DBG("Copying image from %p to %p", src_mem, exec_mem);

	memcpy(exec_mem, src_mem,
		   MIN(DT_REG_SIZE(DT_INST_PHANDLE(0, execution_memory)),
		   DT_REG_SIZE(DT_INST_PHANDLE(0, image_source))));
#endif /* DT_INST_NODE_HAS_PROP(0, image_source) */

	uint32_t cpu1_image_base = DT_REG_ADDR(DT_INST_PHANDLE(0, execution_memory));

	uint32_t *cpu1_vector_table = (void *)cpu1_image_base;
	uint32_t cpu1_sp = cpu1_vector_table[0];
	uint32_t cpu1_pc = cpu1_vector_table[1];

#if CONFIG_RP2_CPU1_LAUNCHER_LOG_LEVEL >= LOG_LEVEL_DBG
	if (rpi_pico_validate_image(cpu1_sp, cpu1_pc) != 0) {
		return -EINVAL;
	}

	LOG_DBG("Launching CPU1 with vector table at 0x%p", (void *)cpu1_vector_table);
#endif /* CONFIG_RP2_CPU1_LAUNCHER_LOG_LEVEL >= LOG_LEVEL_DBG */

	const struct rpi_pico_mailbox_config *config = dev->config;

	if (rpi_pico_reset_cpu1(config->sio_regs, config->psm_regs) != 0) {
		LOG_ERR("CPU1 reset failed.");
		return -EIO;
	}

	rpi_pico_boot_cpu1(config->sio_regs, (uint32_t)cpu1_vector_table, cpu1_sp, cpu1_pc);
#endif /* DT_INST_NODE_HAS_PROP(0, execution_memory) */

	return 0;
}

struct rpi_pico_mailbox_config rpi_pico_cpu1_config = {
	.sio_regs = (sio_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, sio),
	.psm_regs = (psm_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, psm),
};

DEVICE_DT_INST_DEFINE(0, &rpi_pico_cpu1_init, NULL, NULL,
			  &rpi_pico_cpu1_config, POST_KERNEL,
			  CONFIG_RP2_CPU1_LAUNCHER_INIT_PRIORITY, NULL);
