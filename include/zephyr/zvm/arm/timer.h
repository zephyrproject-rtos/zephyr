/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/zvm/vm_irq.h>
#include <../../kernel/include/timeout_q.h>

/**
 * @brief Virtual timer context for this vcpu.
 * Describes only two elements, one is a virtual timer, the other is physical.
 */
struct virt_timer_context {
	/* virtual timer irq number */
	uint32_t virt_virq;
	uint32_t virt_pirq;
	/* control register */
	uint32_t cntv_ctl;
	uint32_t cntp_ctl;
	/* virtual count compare register */
	uint64_t cntv_cval;
	uint64_t cntp_cval;
	/* virtual count value register */
	uint64_t cntv_tval;
	uint64_t cntp_tval;
	/* timeout for softirq */
	struct _timeout vtimer_timeout;
	struct _timeout ptimer_timeout;
	/* vcpu timer offset value, value is cycle */
	uint64_t timer_offset;
	void *vcpu;
	bool enable_flag;
};

/**
 * @brief Simulate cntp_tval_el0 register
 */
void simulate_timer_cntp_tval(struct z_vcpu *vcpu, int read, uint64_t *value);

/**
 * @brief Simulate cntp_cval_el0 register
 */
void simulate_timer_cntp_cval(struct z_vcpu *vcpu, int read, uint64_t *value);

/**
 * @brief Simulate cntp_ctl register
 */
void simulate_timer_cntp_ctl(struct z_vcpu *vcpu, int read, uint64_t *value);

int arch_vcpu_timer_init(struct z_vcpu *vcpu);
int arch_vcpu_timer_deinit(struct z_vcpu *vcpu);

/**
 * @brief Init zvm arch timer.
 */
int zvm_arch_vtimer_init(void);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_ */
