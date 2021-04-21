/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <zephyr/types.h>
#include <soc.h>
#include <toolchain.h>

#include <arch/arc/v2/secureshield/arc_secure.h>

#define IRQ_PRIO_MASK (0xffff << ARC_N_IRQ_START_LEVEL)
/*
 * @brief read secure auxiliary regs on behalf of normal mode
 *
 * @param aux_reg  address of aux reg
 *
 * Some aux regs require secure privilege, this function implements
 * an secure service to access secure aux regs. Check should be done
 * to decide whether the access is valid.
 */
static int32_t arc_s_aux_read(uint32_t aux_reg)
{
	return -1;
}

/*
 * @brief write secure auxiliary regs on behalf of normal mode
 *
 * @param aux_reg address of aux reg
 * @param val, the val to write
 *
 *  Some aux regs require secure privilege, this function implements
 * an secure service to access secure aux regs. Check should be done
 * to decide whether the access is valid.
 */
static int32_t arc_s_aux_write(uint32_t aux_reg, uint32_t val)
{
	if (aux_reg == _ARC_V2_AUX_IRQ_ACT) {
		/* 0 -> CONFIG_NUM_IRQ_PRIO_LEVELS allocated to secure world
		 * left prio levels allocated to normal world
		 */
		val &= IRQ_PRIO_MASK;
		z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_ACT, val |
		(z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT) &
			(~IRQ_PRIO_MASK)));

		return  0;
	}

	return -1;
}

/*
 * @brief allocate interrupt for normal world
 *
 * @param intno, the interrupt to be allocated to normal world
 *
 * By default, most interrupts are configured to be secure in initialization.
 * If normal world wants to use an interrupt, through this secure service to
 * apply one. Necessary check should be done to decide whether the apply is
 * valid
 */
static int32_t arc_s_irq_alloc(uint32_t intno)
{
	z_arc_v2_irq_uinit_secure_set(intno, 0);
	return 0;
}


/*
 * \todo, to access MPU from normal mode, secure mpu service should be
 * created. In the secure mpu service, the parameters should be checked
 * (e.g., not overwrite the mpu regions for secure world)that operations
 * are valid
 */


/*
 * \todo, how to add secure service easily
 */
const _arc_s_call_handler_t arc_s_call_table[ARC_S_CALL_LIMIT] = {
	[ARC_S_CALL_AUX_READ] = (_arc_s_call_handler_t)arc_s_aux_read,
	[ARC_S_CALL_AUX_WRITE] = (_arc_s_call_handler_t)arc_s_aux_write,
	[ARC_S_CALL_IRQ_ALLOC] = (_arc_s_call_handler_t)arc_s_irq_alloc,
};
