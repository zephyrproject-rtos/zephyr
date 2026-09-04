/*
 * Copyright (c) 2025 Dan Collins <dan@collinsnz.com>
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <errno.h>

#include <hardware/structs/sio.h>
#include <hardware/structs/psm.h>
#ifdef CONFIG_SOC_SERIES_RP2350
#include <hardware/regs/otp.h>
#include <hardware/structs/otp.h>
#endif

#include <zephyr/drivers/misc/mbox_rpi_pico/mbox_rpi_pico.h>

LOG_MODULE_REGISTER(soc_rpi_pico_cpu1, CONFIG_SOC_LOG_LEVEL);

#define CPU1_NODE DT_NODELABEL(cpu1)

/* Verify CPU1's enable-method value matches exactly one node's compatible */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_STRING_TOKEN(CPU1_NODE, enable_method)) == 1,
	     "exactly one enabled node must match cpu1's enable-method compatible");
#define CPU1_LAUNCHER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(DT_STRING_TOKEN(CPU1_NODE, enable_method))

BUILD_ASSERT(DT_SAME_NODE(DT_PARENT(CPU1_LAUNCHER_NODE), DT_ROOT),
	     "The cpu enable node must be at the top level of the devicetree");

#define CPU1_LAUNCHER_HAS_SOURCE_MEM DT_NODE_HAS_PROP(CPU1_LAUNCHER_NODE, source_memory)
#if CPU1_LAUNCHER_HAS_SOURCE_MEM
BUILD_ASSERT((DT_PARTITION_EXISTS(DT_PHANDLE(CPU1_LAUNCHER_NODE, source_memory))),
	     "Image source memory must be in a flash partition");
#define CPU1_SRC_ADDR DT_PARTITION_ADDR(DT_PHANDLE(CPU1_LAUNCHER_NODE, source_memory))
#define CPU1_SRC_SIZE DT_REG_SIZE(DT_PHANDLE(CPU1_LAUNCHER_NODE, source_memory))
#endif /* CPU1_LAUNCHER_HAS_SOURCE_MEM */

#define CPU1_LAUNCHER_EXEC_MEM_IN_FLASH                                                            \
	DT_PARTITION_EXISTS(DT_PHANDLE(CPU1_LAUNCHER_NODE, execution_memory))
#if CPU1_LAUNCHER_EXEC_MEM_IN_FLASH
#define CPU1_EXEC_ADDR DT_PARTITION_ADDR(DT_PHANDLE(CPU1_LAUNCHER_NODE, execution_memory))
#else
#define CPU1_EXEC_ADDR DT_REG_ADDR(DT_PHANDLE(CPU1_LAUNCHER_NODE, execution_memory))
#endif /* CPU1_LAUNCHER_EXEC_MEM_IN_FLASH */
#define CPU1_EXEC_SIZE DT_REG_SIZE(DT_PHANDLE(CPU1_LAUNCHER_NODE, execution_memory))

#define CPU1_STACK_ADDR DT_REG_ADDR(DT_PHANDLE(CPU1_LAUNCHER_NODE, stack_memory))
#define CPU1_STACK_SIZE DT_REG_SIZE(DT_PHANDLE(CPU1_LAUNCHER_NODE, stack_memory))
#define CPU1_ARCH_RISCV DT_NODE_HAS_COMPAT(CPU1_NODE, riscv)

static int rpi_pico_mailbox_put_timeout(sio_hw_t *const sio_regs, uint32_t value)
{
	for (uint32_t waited = 0; !rpi_pico_mbox_write_ready(sio_regs); waited++) {
		if (waited == CONFIG_SOC_RPI_PICO_CPU1_ENABLE_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	rpi_pico_mbox_write(sio_regs, value);

	/* Inform other CPU about FIFO update. */
	__SEV();
	return 0;
}

static int rpi_pico_mailbox_pop_timeout(sio_hw_t *const sio_regs, uint32_t *value)
{
	for (uint32_t waited = 0; !rpi_pico_mbox_read_valid(sio_regs); waited++) {
		if (waited == CONFIG_SOC_RPI_PICO_CPU1_ENABLE_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	*value = rpi_pico_mbox_read(sio_regs);
	return 0;
}

#if CPU1_LAUNCHER_HAS_SOURCE_MEM
static void rpi_pico_load_cpu1_image(void)
{
	BUILD_ASSERT((CPU1_EXEC_SIZE >= CPU1_SRC_SIZE),
		     "Image size must not exceed execution memory size");

	BUILD_ASSERT(((CPU1_EXEC_ADDR >= CPU1_SRC_ADDR + CPU1_SRC_SIZE) ||
		      (CPU1_SRC_ADDR >= CPU1_EXEC_ADDR + CPU1_EXEC_SIZE)),
		     "Image source memory must not overlap with execution memory");

	void *src_mem = (void *)CPU1_SRC_ADDR;
	void *exec_mem = (void *)CPU1_EXEC_ADDR;

	LOG_DBG("Copying image from %p to %p", src_mem, exec_mem);

	memcpy(exec_mem, src_mem, MIN(CPU1_EXEC_SIZE, CPU1_SRC_SIZE));
}
#endif /* CPU1_LAUNCHER_HAS_SOURCE_MEM */

#ifdef CONFIG_SOC_RPI_PICO_CPU1_ENABLE_CHECK_VTOR
static inline bool address_in_range(uint32_t addr, uint32_t base, uint32_t size)
{
	return addr >= base && addr - base < size;
}

static int rpi_pico_validate_image(uint32_t cpu1_vector, uint32_t cpu1_sp, uint32_t cpu1_pc)
{
	if (cpu1_sp <= CPU1_STACK_ADDR || cpu1_sp - CPU1_STACK_ADDR > CPU1_STACK_SIZE ||
	    !IS_ALIGNED(cpu1_sp, 8)) {
		LOG_ERR("CPU1 stack pointer 0x%08x invalid.", cpu1_sp);
		return -EINVAL;
	}

	LOG_DBG("CPU1 stack pointer: 0x%08x", cpu1_sp);

#if CPU1_ARCH_RISCV
	if (!IS_ALIGNED(cpu1_vector, 4) ||
	    !address_in_range(cpu1_vector, CPU1_EXEC_ADDR, CPU1_EXEC_SIZE) ||
	    !IS_ALIGNED(cpu1_pc, 4) ||
	    !address_in_range(cpu1_pc, CPU1_EXEC_ADDR, CPU1_EXEC_SIZE)) {
		LOG_ERR("CPU1 RISC-V vectors or entry point are invalid.");
		return -EINVAL;
	}
	LOG_DBG("CPU1 RISC-V entry point: 0x%08x", cpu1_pc);
#else
	if (!IS_ALIGNED(cpu1_vector, 128)) {
		LOG_ERR("CPU1 vector table 0x%08x invalid.", cpu1_vector);
		return -EINVAL;
	}
	if ((cpu1_pc & 1U) == 0U ||
	    !address_in_range(cpu1_pc & ~1U, CPU1_EXEC_ADDR, CPU1_EXEC_SIZE)) {
		LOG_ERR("CPU1 reset pointer 0x%08x invalid.", cpu1_pc);
		return -EINVAL;
	}

	LOG_DBG("CPU1 reset pointer: 0x%08x", cpu1_pc);
#endif
	return 0;
}
#endif /* CONFIG_SOC_RPI_PICO_CPU1_ENABLE_CHECK_VTOR */

static int rpi_pico_select_cpu1_architecture(bool riscv)
{
#ifdef CONFIG_SOC_SERIES_RP2350
	if (riscv) {
		hw_set_bits(&otp_hw->archsel, OTP_ARCHSEL_CORE1_BITS);
	} else {
		hw_clear_bits(&otp_hw->archsel, OTP_ARCHSEL_CORE1_BITS);
	}

	if (!!(otp_hw->archsel & OTP_ARCHSEL_CORE1_BITS) != riscv) {
		LOG_ERR("CPU1 architecture selection is prohibited by OTP boot policy.");
		return -EPERM;
	}
#else
	if (riscv) {
		return -ENOTSUP;
	}
#endif
	return 0;
}

static int rpi_pico_reset_cpu1(sio_hw_t *const sio_regs, psm_hw_t *const psm_regs)
{
	uint32_t val;

	/* Power off, and wait for it to take effect. */
	hw_set_bits(&psm_regs->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	for (uint32_t waited = 0; !(psm_regs->frce_off & PSM_FRCE_OFF_PROC1_BITS); waited++) {
		if (waited == CONFIG_SOC_RPI_PICO_CPU1_ENABLE_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}
	if (rpi_pico_select_cpu1_architecture(CPU1_ARCH_RISCV) != 0) {
		return -EPERM;
	}

	/*
	 * Power back on, and we can wait for a '0' in the FIFO to know
	 * that it has come back.
	 */
	hw_clear_bits(&psm_regs->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	for (uint32_t waited = 0; psm_regs->frce_off & PSM_FRCE_OFF_PROC1_BITS; waited++) {
		if (waited == CONFIG_SOC_RPI_PICO_CPU1_ENABLE_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}
	if (rpi_pico_mailbox_pop_timeout(sio_regs, &val) != 0) {
		return -ETIMEDOUT;
	}

#ifdef CONFIG_SOC_SERIES_RP2350
	if (!!(otp_hw->archsel_status & OTP_ARCHSEL_STATUS_CORE1_BITS) != CPU1_ARCH_RISCV) {
		LOG_ERR("CPU1 started with the wrong architecture.");
		return -EIO;
	}
#endif

	return val == 0 ? 0 : -EIO;
}

static int rpi_pico_boot_cpu1(sio_hw_t *const sio_regs, uint32_t vector_table_addr,
			      uint32_t stack_ptr, uint32_t pc)
{
	/* We synchronise with CPU1 and then we can hand over the memory addresses. */
	uint32_t cmds[] = {0, 0, 1, vector_table_addr, stack_ptr, pc};
	uint32_t seq = 0;
	int64_t deadline = k_uptime_get() +
			   DIV_ROUND_UP(CONFIG_SOC_RPI_PICO_CPU1_ENABLE_TIMEOUT_US, USEC_PER_MSEC);

	do {
		uint32_t cmd = cmds[seq], rsp;

		if (cmd == 0) {
			/* Flush the mailbox by reading all pending messages. */
			rpi_pico_mbox_drain(sio_regs);

			/* Signal readiness to CPU1 */
			__SEV();
		}

		if (rpi_pico_mailbox_put_timeout(sio_regs, cmd) != 0 ||
		    rpi_pico_mailbox_pop_timeout(sio_regs, &rsp) != 0) {
			return -ETIMEDOUT;
		}

		seq = (cmd == rsp) ? seq + 1 : 0;
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
	} while (seq < ARRAY_SIZE(cmds));

	return 0;
}

void soc_late_init_hook(void)
{
#if CPU1_LAUNCHER_HAS_SOURCE_MEM
	rpi_pico_load_cpu1_image();
#endif /* CPU1_LAUNCHER_HAS_SOURCE_MEM */
	uint32_t cpu1_image_base = CPU1_EXEC_ADDR;

#if CPU1_ARCH_RISCV
	/* RISC-V images establish their own interrupt stack in __initialize. */
	uint32_t cpu1_vector = cpu1_image_base;
	uint32_t cpu1_sp = CPU1_STACK_ADDR + CPU1_STACK_SIZE;
	uint32_t cpu1_pc = cpu1_image_base;
#else
	uint32_t *cpu1_vector_table = (void *)cpu1_image_base;
	uint32_t cpu1_sp = cpu1_vector_table[0];
	uint32_t cpu1_pc = cpu1_vector_table[1];
	uint32_t cpu1_vector = (uint32_t)cpu1_vector_table;
#endif

#ifdef CONFIG_SOC_RPI_PICO_CPU1_ENABLE_CHECK_VTOR
	if (rpi_pico_validate_image(cpu1_vector, cpu1_sp, cpu1_pc) != 0) {
		return;
	}
#endif /* CONFIG_SOC_RPI_PICO_CPU1_ENABLE_CHECK_VTOR */

	LOG_DBG("Launching CPU1 with vector table at 0x%08x", cpu1_vector);

	if (rpi_pico_reset_cpu1(sio_hw, psm_hw) != 0) {
		LOG_ERR("CPU1 reset failed.");
		return;
	}

	int ret = rpi_pico_boot_cpu1(sio_hw, cpu1_vector, cpu1_sp, cpu1_pc);

	if (ret != 0) {
		LOG_ERR("CPU1 boot handshake failed (%d).", ret);
	}
}
