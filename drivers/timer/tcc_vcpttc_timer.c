/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tcc_ttcvcp

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/dt-bindings/interrupt-controller/tcc-tic.h>
#include <zephyr/drivers/interrupt_controller/intc_tic.h>
#include <soc.h>

#include "tcc_vcpttc_timer.h"
#include <string.h>

#define VCP_CPU_TIMER_ID CONFIG_TCC_VCPTTC_TIMER_INDEX /* HW Timer Resource for OS sheduling */
#define VCP_TICK_RATE_HZ ((uint32_t)1000UL)

#define VCP_TIMER_IRQ_NUM   DT_INST_IRQN(0)
#define VCP_TIMER_IRQ_PRIO  DT_INST_IRQ(0, priority)
#define VCP_TIMER_IRQ_FLAGS DT_INST_IRQ(0, flags)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_INST(0, tcc_ttcvcp));
#endif

#ifdef CONFIG_TICKLESS_KERNEL
static uint32_t last_cycles;
#endif
static bool flag_timer_initialized;
static struct timer_resource_table vcp_timer_resource[TIMER_CH_MAX];

uint32_t vcp_timer_irq_clear(enum timer_channel channel);

static uint32_t read_count(void)
{
	/* Read current counter value */
	return sys_read32(TIMER_BASE_ADDR + TMR_MAIN_CNT);
}

#ifdef CONFIG_TICKLESS_KERNEL
static void update_match(uint32_t cycles, uint32_t match)
{
	uint32_t delta = match - cycles;

	/* Ensure that the match value meets the minimum timing requirements */
	if (delta < CYCLES_NEXT_MIN) {
		match += CYCLES_NEXT_MIN - delta;
	}

	/* Write counter match value for interrupt generation */
	sys_write32(match, TIMER_BASE_ADDR + TMR_CMP_VALUE0);
}
#endif

static void ttc_timer_compare_isr(const void *arg)
{
	cpu_irq_disable();

	tic_cpu_if->cpu_pri_mask = (uint32_t)(MAX_API_CALL_INTERRUPT_PRIORITY << PRIORITY_SHIFT);

	__asm volatile("dsb\n"
		       "isb\n");

	cpu_irq_enable();

	sys_clock_announce(1);

	/* Ensure all interrupt priorities are active again. */
	vcp_timer_irq_clear(VCP_CPU_TIMER_ID);
	clear_interrupt_mask();
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles, next_cycles;

	/* Read counter value */
	cycles = read_count();

	/* Calculate timeout counter value */
	if (ticks == K_TICKS_FOREVER) {
		next_cycles = cycles + CYCLES_NEXT_MAX;
	} else {
		next_cycles = cycles + ((uint32_t)ticks * CYCLES_PER_TICK);
	}

	/* Set match value for the next interrupt */
	update_match(cycles, next_cycles);
#endif
}

uint32_t sys_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles;

	/* Read counter value */
	cycles = read_count();

	/* Return the number of ticks since last announcement */
	return (cycles - last_cycle) / CYCLES_PER_TICK;
#else
	return 0;
#endif
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* Return the current counter value */
	return read_count();
}

uint32_t vcp_timer_irq_clear(enum timer_channel channel)
{
	uint32_t reg, clr_ctl;

	reg = TIMER_BASE_ADDR + TMR_IRQ_CTRL;
	clr_ctl = sys_read32(reg);

	if ((clr_ctl & TMR_IRQ_CLR_CTRL_WRITE) != 0U) {
		sys_write32(clr_ctl | TMR_IRQ_MASK_ALL, reg);
	} else {
		sys_read32(reg);
	}

	return 0;
}

static void vcp_timer_handler(void *arg_ptr)
{
	struct timer_resource_table *timer = TCC_NULL_PTR;
	uint32_t reg;

	memcpy(&timer, (const void *)&arg_ptr, sizeof(struct timer_resource_table *));

	if (timer != TCC_NULL_PTR) {
		reg = TIMER_BASE_ADDR + TMR_IRQ_CTRL;

		if (((sys_read32(reg) & TMR_IRQ_CTRL_IRQ_ALLEN) != 0UL) &&
		    (timer->rsc_table_used == true)) {
			vcp_timer_irq_clear(timer->rsc_table_channel);

			if (timer->rsc_table_handler != TCC_NULL_PTR) {
				timer->rsc_table_handler(timer->rsc_table_channel,
							 timer->rsc_table_arg);
			}
		}

		z_tic_irq_eoi(TIC_TIMER_0);
	}
}

static void vcp_timer_set_enable_core_reg(const struct vcp_timer_config *cfg_ptr,
					  const uint32_t cmp0_val, const uint32_t cmp1_val,
					  uint32_t config_val, const uint32_t irq_val)
{
	uint32_t mainval;
	uint32_t tmpval;
	uint32_t reg = TIMER_BASE_ADDR;
	uint32_t rate_factor = (TMR_CLK_RATE / 1000UL) / ((TMR_PRESCALE + 1UL) * 1000UL);

	mainval = (cfg_ptr->cfg_main_val_usec == 0UL)
			  ? 0xFFFFFFFFUL
			  : ((cfg_ptr->cfg_main_val_usec * rate_factor) - 1UL);

	sys_write32(mainval, (uint32_t)(reg + TMR_MAIN_CNT_LVD));
	sys_write32(cmp0_val, (uint32_t)(reg + TMR_CMP_VALUE0));
	sys_write32(cmp1_val, (uint32_t)(reg + TMR_CMP_VALUE1));

	config_val |= (TMR_PRESCALE | TMR_OP_EN_CFG_CNT_EN |
		       ((uint32_t)cfg_ptr->cfg_start_mode << TMR_OP_EN_CFG_LDZERO_OFFSET));

	if (cfg_ptr->cfg_op_mode == TIMER_OP_ONESHOT) {
		config_val |= TMR_OP_EN_CFG_OPMODE_ONE_SHOT;
	}

	tmpval = sys_read32((uint32_t)(reg + TMR_IRQ_CTRL));

	sys_write32(config_val, (uint32_t)(reg + TMR_OP_EN_CFG));
	sys_write32((tmpval | irq_val), (uint32_t)(reg + TMR_IRQ_CTRL));
}

static int32_t vcp_timer_enable_comp0(const struct vcp_timer_config *cfg_ptr)
{
	uint32_t tmpval, rate_factor;
	uint32_t mainval, cmpval0;
	uint32_t cmpval1 = 0x0UL;
	uint32_t reg_cfgval;
	uint32_t reg_irqval = TMR_IRQ_CTRL_IRQ_EN2;
	int32_t ret = 0;

	rate_factor = (TMR_CLK_RATE / 1000UL) / ((TMR_PRESCALE + 1UL) * 1000UL);

	if ((VCP_MAX_INT_VAL / rate_factor) < cfg_ptr->cfg_cmp0_val_usec) {
		ret = -EINVAL;
	} else {
		mainval = (cfg_ptr->cfg_main_val_usec == 0UL)
				  ? 0xFFFFFFFFUL
				  : ((cfg_ptr->cfg_main_val_usec * rate_factor) - 1UL);
		tmpval = (cfg_ptr->cfg_cmp0_val_usec * rate_factor) - 1UL;

		if ((cfg_ptr->cfg_start_mode == TIMER_START_MAINCNT) &&
		    (((0xFFFFFFFFUL - tmpval) == 0xFFFFFFFFUL) ||
		     (mainval > (0xFFFFFFFFUL - tmpval)))) {
			ret = -EINVAL;
		} else {
			cmpval0 = (cfg_ptr->cfg_start_mode == TIMER_START_ZERO)
					  ? tmpval
					  : (mainval + tmpval);
			reg_cfgval = TMR_OP_EN_CFG_LDM0_ON;
			reg_irqval |= TMR_IRQ_CTRL_IRQ_EN0;

			vcp_timer_set_enable_core_reg(cfg_ptr, cmpval0, cmpval1, reg_cfgval,
						      reg_irqval);
		}
	}

	return ret;
}

static int32_t vcp_timer_enable_comp1(const struct vcp_timer_config *cfg_ptr)
{
	uint32_t tmpval;
	uint32_t rate_factor, mainval;
	uint32_t cmpval0 = 0x0UL;
	uint32_t cmpval1, reg_cfgval;
	uint32_t reg_irqval = TMR_IRQ_CTRL_IRQ_EN2;
	int32_t ret = 0;

	rate_factor = (TMR_CLK_RATE / 1000UL) / ((TMR_PRESCALE + 1UL) * 1000UL);

	if ((VCP_MAX_INT_VAL / rate_factor) < cfg_ptr->cfg_cmp1_val_usec) {
		ret = -EINVAL;
	} else {
		mainval = (cfg_ptr->cfg_main_val_usec == 0UL)
				  ? 0xFFFFFFFFUL
				  : ((cfg_ptr->cfg_main_val_usec * rate_factor) - 1UL);
		tmpval = (cfg_ptr->cfg_cmp1_val_usec * rate_factor) - 1UL;

		if ((cfg_ptr->cfg_start_mode == TIMER_START_MAINCNT) &&
		    (((0xFFFFFFFFUL - tmpval) == 0xFFFFFFFFUL) ||
		     (mainval > (0xFFFFFFFFUL - tmpval)))) {
			ret = -EINVAL;
		} else {
			cmpval1 = (cfg_ptr->cfg_start_mode == TIMER_START_ZERO)
					  ? tmpval
					  : (mainval + tmpval);
			reg_cfgval = TMR_OP_EN_CFG_LDM1_ON;
			reg_irqval |= TMR_IRQ_CTRL_IRQ_EN1;

			vcp_timer_set_enable_core_reg(cfg_ptr, cmpval0, cmpval1, reg_cfgval,
						      reg_irqval);
		}
	}

	return ret;
}

static int32_t vcp_timer_enable_small_comp(const struct vcp_timer_config *cfg_ptr)
{
	uint32_t rate_factor, tmpval0, tmpval1, mainval;
	uint32_t cmpval0, cmpval1, reg_cfgval;
	uint32_t reg_irqval = TMR_IRQ_CTRL_IRQ_EN2;
	int32_t ret = 0;

	rate_factor = (TMR_CLK_RATE / 1000UL) / ((TMR_PRESCALE + 1UL) * 1000UL);
	mainval = (cfg_ptr->cfg_main_val_usec == 0UL)
			  ? 0xFFFFFFFFUL
			  : ((cfg_ptr->cfg_main_val_usec * rate_factor) - 1UL);

	if ((VCP_MAX_INT_VAL / rate_factor) < cfg_ptr->cfg_cmp0_val_usec) {
		ret = -EINVAL;
	} else if ((VCP_MAX_INT_VAL / rate_factor) < cfg_ptr->cfg_cmp1_val_usec) {
		ret = -EINVAL;
	} else {
		tmpval0 = (cfg_ptr->cfg_cmp0_val_usec * rate_factor) - 1UL;
		tmpval1 = (cfg_ptr->cfg_cmp1_val_usec * rate_factor) - 1UL;

		if (cfg_ptr->cfg_start_mode == TIMER_START_MAINCNT) {
			if (tmpval0 <= tmpval1) {
				if ((VCP_MAX_INT_VAL - mainval) <= tmpval0) {
					ret = -EINVAL;
				} else {
					cmpval0 = mainval + tmpval0;
					cmpval1 = VCP_MAX_INT_VAL;
				}
			} else {
				if ((VCP_MAX_INT_VAL - mainval) <= tmpval1) {
					ret = -EINVAL;
				} else {
					cmpval0 = VCP_MAX_INT_VAL;
					cmpval1 = mainval + tmpval1;
				}
			}
		} else {
			cmpval0 = tmpval0;
			cmpval1 = tmpval1;
		}

		if (ret == 0) {
			reg_cfgval = (TMR_OP_EN_CFG_LDM0_ON | TMR_OP_EN_CFG_LDM1_ON);
			reg_irqval |= (TMR_IRQ_CTRL_IRQ_EN0 | TMR_IRQ_CTRL_IRQ_EN1);

			vcp_timer_set_enable_core_reg(cfg_ptr, cmpval0, cmpval1, reg_cfgval,
						      reg_irqval);
		}
	}

	return ret;
}

static int32_t vcp_timer_enable_mode(const struct vcp_timer_config *cfg_ptr)
{
	int32_t ret = 0;

	switch (cfg_ptr->cfg_counter_mode) {
	case TIMER_COUNTER_COMP0:
		ret = vcp_timer_enable_comp0(cfg_ptr);
		break;
	case TIMER_COUNTER_COMP1:
		ret = vcp_timer_enable_comp1(cfg_ptr);
		break;
	case TIMER_COUNTER_SMALL_COMP:
		ret = vcp_timer_enable_small_comp(cfg_ptr);
		break;
	default:
		vcp_timer_set_enable_core_reg(cfg_ptr, 0x0UL, 0x0UL, 0x0UL, TMR_IRQ_CTRL_IRQ_EN2);
		break;
	}

	return ret;
}

static int32_t vcp_timer_enable_with_config(const struct vcp_timer_config *cfg_ptr)
{
	int32_t ret = 0;

	if (flag_timer_initialized == false) {
		return -EIO;
	}

	if ((cfg_ptr == TCC_NULL_PTR) || (TIMER_CH_MAX <= cfg_ptr->cfg_channel)) {
		return -EINVAL;
	}

	ret = vcp_timer_enable_mode(cfg_ptr);

	if (ret == 0) {
		vcp_timer_resource[cfg_ptr->cfg_channel].rsc_table_used = true;
		vcp_timer_resource[cfg_ptr->cfg_channel].rsc_table_handler = cfg_ptr->handler_fn;
		vcp_timer_resource[cfg_ptr->cfg_channel].rsc_table_arg = cfg_ptr->arg_ptr;

		if (cfg_ptr->cfg_channel != VCP_CPU_TIMER_ID) {
			tic_irq_vector_set((uint32_t)TIC_TIMER_0 + (uint32_t)cfg_ptr->cfg_channel,
					   TIC_IRQ_DEFAULT_PRIORITY, TIC_INT_TYPE_LEVEL_HIGH,
					   (tic_isr_func)&vcp_timer_handler,
					   (void *)&vcp_timer_resource[cfg_ptr->cfg_channel]);
		}
	}

	return ret;
}

static int32_t vcp_timer_enable_with_mode(enum timer_channel channel, uint32_t u_sec,
					  enum vcp_timer_op_mode op_mode,
					  vcp_timer_handler_fn handler_fn, void *arg_ptr)
{
	struct vcp_timer_config cfg;

	cfg.cfg_channel = channel;
	cfg.cfg_start_mode = TIMER_START_ZERO;
	cfg.cfg_op_mode = op_mode;
	cfg.cfg_counter_mode = TIMER_COUNTER_COMP0;
	cfg.cfg_main_val_usec = 0;
	cfg.cfg_cmp0_val_usec = u_sec;
	cfg.cfg_cmp1_val_usec = 0;
	cfg.handler_fn = handler_fn;
	cfg.arg_ptr = arg_ptr;

	return vcp_timer_enable_with_config(&cfg);
}

static int32_t vcp_timer_enable(uint32_t channel, uint32_t u_sec, vcp_timer_handler_fn handler_fn,
				void *arg_ptr)
{
	return vcp_timer_enable_with_mode(channel, u_sec, TIMER_OP_FREERUN, handler_fn, arg_ptr);
}

static int32_t vcp_timer_init(void)
{
	uint32_t reg, resIndex, reg_val;
	int32_t ret;

	ret = 0;

	if (flag_timer_initialized == true) {
		return ret;
	}

	for (resIndex = 0; resIndex < (uint32_t)TIMER_CH_MAX; resIndex++) {
		reg = MCU_BSP_TIMER_BASE + (resIndex * 0x100UL);

		vcp_timer_resource[resIndex].rsc_table_channel = (enum timer_channel)resIndex;
		vcp_timer_resource[resIndex].rsc_table_used = false;
		vcp_timer_resource[resIndex].rsc_table_handler = TCC_NULL_PTR;
		vcp_timer_resource[resIndex].rsc_table_arg = TCC_NULL_PTR;

		sys_write32(0x7FFFU, (uint32_t)(reg + TMR_OP_EN_CFG));
		sys_write32(0x0U, (uint32_t)(reg + TMR_MAIN_CNT_LVD));
		sys_write32(0x0U, (uint32_t)(reg + TMR_CMP_VALUE0));
		sys_write32(0x0U, (uint32_t)(reg + TMR_CMP_VALUE1));

		reg_val = TMR_IRQ_CLR_CTRL_WRITE | TMR_IRQ_MASK_ALL;

		sys_write32(reg_val, (uint32_t)(reg + TMR_IRQ_CTRL));
	}

	flag_timer_initialized = true;

	return ret;
}

static int sys_clock_driver_init(void)
{
	uint32_t timer_channel, tic_to_sec;

	vcp_timer_init();

	timer_channel = ((TIMER_BASE_ADDR - MCU_BSP_TIMER_BASE) / 0x100);
	if (timer_channel >= TIMER_CH_MAX) {
		return -EINVAL;
	}

	if (timer_channel != VCP_CPU_TIMER_ID) {
		return -EINVAL;
	}

#ifdef CONFIG_TICKLESS_KERNEL
	/* Initialise internal states */
	last_cycles = 0;
#endif

	tic_irq_vector_set(TIMER_IRQ, TIC_IRQ_DEFAULT_PRIORITY, TIC_INT_TYPE_LEVEL_HIGH,
			   (tic_isr_func)ttc_timer_compare_isr, NULL);

	tic_to_sec = (uint32_t)(((uint32_t)1000 * (uint32_t)1000) / VCP_TICK_RATE_HZ);

	IRQ_CONNECT(VCP_TIMER_IRQ_NUM, VCP_TIMER_IRQ_PRIO, ttc_timer_compare_isr, NULL, 0);

	z_tic_irq_enable(TIC_TIMER_0 + VCP_CPU_TIMER_ID);

	vcp_timer_enable((enum timer_channel)timer_channel, tic_to_sec, 0, 0);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
