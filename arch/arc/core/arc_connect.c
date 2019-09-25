/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 ARC CONNECT driver
 *
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <spinlock.h>


static struct k_spinlock arc_connect_spinlock;

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			!__i.key;					\
			k_spin_unlock(lck, __key), __i.key = 1)


/* Generate an inter-core interrupt to the target core */
void z_arc_connect_ici_generate(u32_t core)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_INTRPT_GENERATE_IRQ, core);
	}
}

/* Acknowledge the inter-core interrupt raised by core */
void z_arc_connect_ici_ack(u32_t core)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_INTRPT_GENERATE_ACK, core);
	}
}

/* Read inter-core interrupt status */
u32_t z_arc_connect_ici_read_status(u32_t core)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_INTRPT_READ_STATUS, core);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Check the source of inter-core interrupt */
u32_t z_arc_connect_ici_check_src(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_INTRPT_CHECK_SOURCE, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Clear the inter-core interrupt */
void z_arc_connect_ici_clear(void)
{
	u32_t cpu, c;

	LOCKED(&arc_connect_spinlock) {

		z_arc_connect_cmd(ARC_CONNECT_CMD_INTRPT_CHECK_SOURCE, 0);
		cpu = z_arc_connect_cmd_readback(); /* 1,2,4,8... */
	/*
	 * In rare case, multiple concurrent ICIs sent to same target can
	 * possibly be coalesced by MCIP into 1 asserted IRQ, so @cpu can be
	 * "vectored" (multiple bits sets) as opposed to typical single bit
	 */
		while (cpu) {
			c = find_lsb_set(cpu) - 1;
			z_arc_connect_cmd(
				ARC_CONNECT_CMD_INTRPT_GENERATE_ACK, c);
			cpu &= ~(1U << c);
		}
	}
}

/* Reset the cores in core_mask */
void z_arc_connect_debug_reset(u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_RESET,
			0, core_mask);
	}
}

/* Halt the cores in core_mask */
void z_arc_connect_debug_halt(u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_HALT,
			0, core_mask);
	}
}

/* Run the cores in core_mask */
void z_arc_connect_debug_run(u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_RUN,
			0, core_mask);
	}
}

/* Set core mask */
void z_arc_connect_debug_mask_set(u32_t core_mask, u32_t mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_SET_MASK,
			mask, core_mask);
	}
}

/* Read core mask */
u32_t z_arc_connect_debug_mask_read(u32_t core_mask)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_READ_MASK,
			0, core_mask);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/*
 * Select cores that should be halted if the core issuing the command is halted
 */
void z_arc_connect_debug_select_set(u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_DEBUG_SET_SELECT,
			0, core_mask);
	}
}

/* Read the select value */
u32_t z_arc_connect_debug_select_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_DEBUG_READ_SELECT, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Read the status, halt or run of all cores in the system */
u32_t z_arc_connect_debug_en_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_DEBUG_READ_EN, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Read the last command sent */
u32_t z_arc_connect_debug_cmd_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_DEBUG_READ_CMD, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Read the value of internal MCD_CORE register */
u32_t z_arc_connect_debug_core_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_DEBUG_READ_CORE, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Clear global free running counter */
void z_arc_connect_gfrc_clear(void)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_CLEAR, 0);
	}
}

/* Read total 64 bits of global free running counter */
u64_t z_arc_connect_gfrc_read(void)
{
	u32_t low;
	u32_t high;
	u32_t key;

	/*
	 * each core has its own arc connect interface, i.e.,
	 * CMD/READBACK. So several concurrent commands to ARC
	 * connect are of if they are trying to access different
	 * sub-components. For GFRC, HW allows simultaneously accessing to
	 * counters. So an irq lock is enough.
	 */
	key = z_arch_irq_lock();

	z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_READ_LO, 0);
	low = z_arc_connect_cmd_readback();

	z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_READ_HI, 0);
	high = z_arc_connect_cmd_readback();

	z_arch_irq_unlock(key);

	return (((u64_t)high) << 32) | low;
}

/* Enable global free running counter */
void z_arc_connect_gfrc_enable(void)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_ENABLE, 0);
	}
}

/* Disable global free running counter */
void z_arc_connect_gfrc_disable(void)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_DISABLE, 0);
	}
}

/* Disable global free running counter */
void z_arc_connect_gfrc_core_set(u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_GFRC_SET_CORE,
			0, core_mask);
	}
}

/* Set the relevant cores to halt global free running counter */
u32_t z_arc_connect_gfrc_halt_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_READ_HALT, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Read the internal CORE register */
u32_t z_arc_connect_gfrc_core_read(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_GFRC_READ_CORE, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Enable interrupt distribute unit */
void z_arc_connect_idu_enable(void)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_ENABLE, 0);
	}
}

/* Disable interrupt distribute unit */
void z_arc_connect_idu_disable(void)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_DISABLE, 0);
	}
}

/* Read enable status of interrupt distribute unit */
u32_t z_arc_connect_idu_read_enable(void)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_READ_ENABLE, 0);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/*
 * Set the triggering mode and distribution mode for the specified common
 * interrupt
 */
void z_arc_connect_idu_set_mode(u32_t irq_num,
	u16_t trigger_mode, u16_t distri_mode)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_IDU_SET_MODE,
			irq_num, (distri_mode | (trigger_mode << 4)));
	}
}

/* Read the internal MODE register of the specified common interrupt */
u32_t z_arc_connect_idu_read_mode(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_READ_MODE, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/*
 * Set the target cores to receive the specified common interrupt
 * when it is triggered
 */
void z_arc_connect_idu_set_dest(u32_t irq_num, u32_t core_mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_IDU_SET_DEST,
			irq_num, core_mask);
	}
}

/* Read the internal DEST register of the specified common interrupt */
u32_t z_arc_connect_idu_read_dest(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_READ_DEST, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Assert the specified common interrupt */
void z_arc_connect_idu_gen_cirq(u32_t irq_num)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_GEN_CIRQ, irq_num);
	}
}

/* Acknowledge the specified common interrupt */
void z_arc_connect_idu_ack_cirq(u32_t irq_num)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_ACK_CIRQ, irq_num);
	}
}

/* Read the internal STATUS register of the specified common interrupt */
u32_t z_arc_connect_idu_check_status(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_CHECK_STATUS, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Read the internal SOURCE register of the specified common interrupt */
u32_t z_arc_connect_idu_check_source(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_CHECK_SOURCE, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/* Mask or unmask the specified common interrupt */
void z_arc_connect_idu_set_mask(u32_t irq_num, u32_t mask)
{
	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd_data(ARC_CONNECT_CMD_IDU_SET_MASK,
			irq_num, mask);
	}
}

/* Read the internal MASK register of the specified common interrupt */
u32_t z_arc_connect_idu_read_mask(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_READ_MASK, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;
}

/*
 * Check if it is the first-acknowledging core to the common interrupt
 * if IDU is programmed in the first-acknowledged mode
 */
u32_t z_arc_connect_idu_check_first(u32_t irq_num)
{
	u32_t ret = 0;

	LOCKED(&arc_connect_spinlock) {
		z_arc_connect_cmd(ARC_CONNECT_CMD_IDU_CHECK_FIRST, irq_num);
		ret = z_arc_connect_cmd_readback();
	}

	return ret;

}
