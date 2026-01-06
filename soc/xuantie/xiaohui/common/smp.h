/*
 * Copyright (C) 2018-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_XUANTIE_SMP_H
#define ZEPHYR_SOC_XUANTIE_SMP_H

static struct xuantie_regs_struct {
	unsigned long pmpaddr0;
	unsigned long pmpaddr1;
	unsigned long pmpaddr2;
	unsigned long pmpaddr3;
	unsigned long pmpaddr4;
	unsigned long pmpaddr5;
	unsigned long pmpaddr6;
	unsigned long pmpaddr7;
	unsigned long pmpcfg0;
	unsigned long mcor;
	unsigned long mhcr;
	unsigned long mccr2;
	unsigned long mhint;
	unsigned long msmpr;
	unsigned long mie;
	unsigned long mstatus;
	unsigned long mxstatus;
	unsigned long mtvec;
	unsigned long plic_base_addr;
	unsigned long clint_base_addr;
} xuantie_regs;

#endif /* ZEPHYR_SOC_XUANTIE_SMP_H */
