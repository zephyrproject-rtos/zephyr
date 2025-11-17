/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power timer driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_lp_timer_pdl

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer, CONFIG_KERNEL_LOG_LEVEL);

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */

/* Data structure */
struct ifx_cat1_lptimer_data {
	bool clear_int_mask;
	uint8_t isr_instruction;
};

/* Device config structure */
struct ifx_cat1_lptimer_config {
	MCWDT_STRUCT_Type *reg_addr;
	uint8_t irq_priority;
};

#define LPTIMER_FREQ (32768u)

#include "cy_mcwdt.h"

#if defined(CY_IP_MXS40SSRSS)
static const uint16_t LPTIMER_RESET_TIME_US = 93;
#else
static const uint16_t LPTIMER_RESET_TIME_US = 62;
#endif

static const uint16_t LPTIMER_SETMATCH_TIME_US;
static const cy_stc_mcwdt_config_t lptimer_default_cfg = {.c0Match = 0xFFFF,
							  .c1Match = 0xFFFF,
							  .c0Mode = CY_MCWDT_MODE_INT,
							  .c1Mode = CY_MCWDT_MODE_INT,
							  .c2Mode = CY_MCWDT_MODE_NONE,
							  .c2ToggleBit = 0,
							  .c0ClearOnMatch = false,
							  .c1ClearOnMatch = false,
							  .c0c1Cascade = true,
							  .c1c2Cascade = false};

static uint32_t last_lptimer_value;
static struct k_spinlock lock;

void lptimer_enable_event(const struct device *dev, bool enable)
{
#define LPTIMER_ISR_CALL_USER_CB_MASK (0x01)
	const struct ifx_cat1_lptimer_config *config = dev->config;
	struct ifx_cat1_lptimer_data *data = dev->data;

	data->isr_instruction &= ~LPTIMER_ISR_CALL_USER_CB_MASK;
	data->isr_instruction |= (uint8_t)enable;

	if (enable) {
		Cy_MCWDT_ClearInterrupt(config->reg_addr, CY_MCWDT_CTR1);
		Cy_MCWDT_SetInterruptMask(config->reg_addr, CY_MCWDT_CTR1);

	} else {
		Cy_MCWDT_ClearInterrupt(config->reg_addr, CY_MCWDT_CTR1);
		Cy_MCWDT_SetInterruptMask(config->reg_addr, 0);
	}
}

void lptimer_set_delay(const struct device *dev, uint32_t delay)
{
	const struct ifx_cat1_lptimer_config *config = dev->config;
	struct ifx_cat1_lptimer_data *data = dev->data;

	data->clear_int_mask = true;

#define LPTIMER_MIN_DELAY (3U) /* Minimum amount of lfclk cycles of that LPTIMER can delay for. */
#define LPTIMER_MAX_DELAY_TICKS                                                                    \
	(0xfff0ffffUL) /* ~36hours. Not set to 0xffffffff to avoid C0 and C1 both overflowing */

	if ((Cy_MCWDT_GetEnabledStatus(config->reg_addr, CY_MCWDT_COUNTER0) == 0UL) ||
	    (Cy_MCWDT_GetEnabledStatus(config->reg_addr, CY_MCWDT_COUNTER1) == 0UL) ||
	    (Cy_MCWDT_GetEnabledStatus(config->reg_addr, CY_MCWDT_COUNTER2) == 0UL)) {
		return;
	}

	/* - 16 bit Counter0 (C0) & Counter1 (C1) are cascaded to generated a 32 bit counter.
	 * - Counter2 (C2) is a free running counter.
	 * - C0 continues counting after reaching its match value. On PSoC™ 4 Counter1 is reset on
	 * match. On PSoC™ 6 it continues counting.
	 * - An interrupt is generated when C1 reaches the match value. On PSoC™ 4 this happens
	 * when the counter increments to the same value as match. On PSoC™ 6 this happens when it
	 * increments past the match value.
	 *
	 * EXAMPLE:
	 * Supposed T=C0=C1=0, and we need to trigger an interrupt at T=0x18000.
	 * We set C0_match to 0x8000 and C1 match to 2 on PSoC™ 4 and 1 on PSoC™ 6.
	 * At T = 0x8000, C0_value matches C0_match so C1 get incremented. C1/C0=0x18000.
	 * At T = 0x18000, C0_value matches C0_match again so C1 get incremented from 1 to 2.
	 * When C1 get incremented from 1 to 2 the interrupt is generated.
	 * At T = 0x18000, C1/C0 = 0x28000.
	 */

	if (delay <= LPTIMER_MIN_DELAY) {
		delay = LPTIMER_MIN_DELAY;
	}
	if (delay > LPTIMER_MAX_DELAY_TICKS) {
		delay = LPTIMER_MAX_DELAY_TICKS;
	}

	Cy_MCWDT_ClearInterrupt(config->reg_addr, CY_MCWDT_CTR1);
	uint16_t c0_old_match = (uint16_t)Cy_MCWDT_GetMatch(config->reg_addr, CY_MCWDT_COUNTER0);

	uint32_t critical_section = Cy_SysLib_EnterCriticalSection();

	/* Cascading from C0 match into C1 is queued and can take 1 full LF clk cycle.
	 * There are 3 cases:
	 * Case 1: if c0 = match0 then the cascade into C1 will happen 1 cycle from now. The value
	 * c1_current_ticks is 1 lower than expected. Case 2: if c0 = match0 -1 then cascade may or
	 * not happen before new match value would occur. Match occurs on rising clock edge.
	 *          Synchronizing match value occurs on falling edge. Wait until c0 = match0 to
	 * ensure cascade occurs. Case 3: everything works as expected.
	 *
	 * Note: timeout is needed here just in case the LFCLK source gives out. This avoids device
	 * lockup.
	 *
	 * ((2 * Cycles_LFClk) / Cycles_cpu_iteration) * (HFCLk_max / LFClk_min) =
	 * Iterations_required Typical case: (2 / 100) * ((150x10^6)/33576) = 89 iterations Worst
	 * case: (2 / 100) * ((150x10^6)/1) = 3x10^6 iterations Compromise: (2 / 100) *
	 * ((150x10^6)/0xFFFF iterations) = 45 Hz = LFClk_min
	 */
	const uint32_t DEFAULT_TIMEOUT = 0xFFFFUL;
	uint32_t timeout = DEFAULT_TIMEOUT;
	uint16_t c0_current_ticks =
		(uint16_t)Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER0);
	/* Wait until the cascade has definitively happened. It takes a clock cycle for the
	 *   cascade to happen, and potentially another a full LFCLK clock cycle for the
	 *   cascade to propagate up to the HFCLK-domain registers that the CPU reads.
	 */
	while (((((uint16_t)(c0_old_match - 1)) == c0_current_ticks) ||
		(c0_old_match == c0_current_ticks) ||
		(((uint16_t)(c0_old_match + 1)) == c0_current_ticks)) &&
	       (timeout != 0UL)) {
		c0_current_ticks = (uint16_t)Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER0);
		timeout--;
	}

	if (timeout == 0UL) {
		/* Timeout has occurred. There could have been a clock failure while waiting for the
		 * count value to update.
		 */
		Cy_SysLib_ExitCriticalSection(critical_section);
		return;
	}

	uint16_t c0_match = (uint16_t)(c0_current_ticks + delay);
	/* Changes can take up to 2 clk_lf cycles to propagate. If we set the match within this
	 * window of the current value, then it is nondeterministic whether the first cascade will
	 * trigger immediately or after 2^16 cycles. Wait until c0 is in a more predictable state.
	 */
	timeout = DEFAULT_TIMEOUT;
	uint32_t c0_new_ticks = c0_current_ticks;

	while (((c0_new_ticks == c0_match) || (c0_new_ticks == ((uint16_t)(c0_match + 1))) ||
		(c0_new_ticks == ((uint16_t)(c0_match + 2)))) &&
	       (timeout != 0UL)) {
		c0_new_ticks = (uint16_t)Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER0);
		timeout--;
	}

	delay -= (c0_new_ticks >= c0_current_ticks) ? (c0_new_ticks - c0_current_ticks)
						    : ((0xFFFFU - c0_current_ticks) + c0_new_ticks);

	c0_match = (uint16_t)(c0_current_ticks + delay);
	uint16_t c1_current_ticks =
		(uint16_t)Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER1);
	uint16_t c1_match = (uint16_t)(c1_current_ticks + (delay >> 16));

	Cy_MCWDT_SetMatch(config->reg_addr, CY_MCWDT_COUNTER0, c0_match, LPTIMER_SETMATCH_TIME_US);
	Cy_MCWDT_SetMatch(config->reg_addr, CY_MCWDT_COUNTER1, c1_match, LPTIMER_SETMATCH_TIME_US);

	Cy_SysLib_ExitCriticalSection(critical_section);
	Cy_MCWDT_SetInterruptMask(config->reg_addr, CY_MCWDT_CTR1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	const struct device *lptimer_dev =
		DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT));

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		/* Disable the LPTIMER events */
		lptimer_enable_event(lptimer_dev, false);
		return;
	}

	/* Configure and Enable the LPTIMER events */
	lptimer_enable_event(lptimer_dev, true);

	/* passing ticks==1 means "announce the next tick", ticks value of zero (or even negative)
	 * is legal and treated identically: it simply indicates the kernel would like the next
	 * tick announcement as soon as possible.
	 */
	if (ticks < 1) {
		ticks = 1;
	}

	uint32_t set_ticks = ((uint32_t)(ticks)*LPTIMER_FREQ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	/* Set the delay value for the next wakeup interrupt */
	lptimer_set_delay(lptimer_dev, set_ticks);
}

uint32_t sys_clock_elapsed(void)
{
	const struct device *lptimer_dev =
		DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT));
	const struct ifx_cat1_lptimer_config *config = lptimer_dev->config;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lptimer_value = Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER2);

	/* Gives the value of LPTIM counter (ms) since the previous 'announce' */
	uint64_t ret = (((uint64_t)(lptimer_value - last_lptimer_value)) *
			CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
		       LPTIMER_FREQ;

	k_spin_unlock(&lock, key);

	return (uint32_t)ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	const struct device *lptimer_dev =
		DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT));
	const struct ifx_cat1_lptimer_config *config = lptimer_dev->config;

	/* Gives the accumulated count in a number of hw cycles */
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lp_time = Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER2);

	/* Convert lptim count in a nb of hw cycles with precision */
	uint64_t ret = ((uint64_t)lp_time * sys_clock_hw_cycles_per_sec()) / LPTIMER_FREQ;

	k_spin_unlock(&lock, key);

	/* Convert in hw cycles (keeping 32bit value) */
	return (uint32_t)ret;
}

static void lptimer_isr(const struct device *dev)
{
	const struct ifx_cat1_lptimer_config *config = dev->config;
	const struct ifx_cat1_lptimer_data *data = dev->data;

	Cy_MCWDT_ClearInterrupt(config->reg_addr, (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2));

	/* Clear interrupt mask if set only from cyhal_lptimer_set_delay() function */
	if (data->clear_int_mask) {
		Cy_MCWDT_SetInterruptMask(config->reg_addr, 0);
	}

	if ((data->isr_instruction & LPTIMER_ISR_CALL_USER_CB_MASK) != 0) {
		/* announce the elapsed time in ms */
		uint32_t lptimer_value = Cy_MCWDT_GetCount(config->reg_addr, CY_MCWDT_COUNTER2);
		int32_t delta_ticks = ((uint64_t)(lptimer_value - last_lptimer_value) *
				       CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
				      LPTIMER_FREQ;
		sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks
								      : (delta_ticks > 0));
		last_lptimer_value = lptimer_value;
	}
}

static int lptimer_init(const struct device *dev)
{
#define LPTIMER_CTRL (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2)

	const struct ifx_cat1_lptimer_config *config = dev->config;
	struct ifx_cat1_lptimer_data *data = dev->data;
	cy_rslt_t rslt = CY_MCWDT_BAD_PARAM;
	cy_stc_mcwdt_config_t cfg = lptimer_default_cfg;

	data->clear_int_mask = false;
	data->isr_instruction = LPTIMER_ISR_CALL_USER_CB_MASK;

	rslt = (cy_rslt_t)Cy_MCWDT_Init(config->reg_addr, &cfg);
	if (rslt == CY_RSLT_SUCCESS) {
		Cy_MCWDT_Enable(config->reg_addr, LPTIMER_CTRL, LPTIMER_RESET_TIME_US);
	} else {
		Cy_MCWDT_Disable(config->reg_addr, LPTIMER_CTRL, LPTIMER_RESET_TIME_US);
		Cy_MCWDT_DeInit(config->reg_addr);
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), lptimer_isr, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

#define INFINEON_CAT1_LPTIMER_INIT(n)                                                              \
                                                                                                   \
	static struct ifx_cat1_lptimer_data ifx_cat1_lptimer_data##n;                              \
                                                                                                   \
	static const struct ifx_cat1_lptimer_config lptimer_cat1_cfg_##n = {                       \
		.reg_addr = (MCWDT_STRUCT_Type *)DT_INST_REG_ADDR(n),                              \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, lptimer_init, NULL, &ifx_cat1_lptimer_data##n,                    \
			      &lptimer_cat1_cfg_##n, PRE_KERNEL_2,                                 \
			      CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_LPTIMER_INIT)
