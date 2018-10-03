#
# Copyright (c) 2018 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

if SOC_NSIM_EM

config CPU_EM4_DMIPS
	def_bool y

config NUM_IRQ_PRIO_LEVELS
	# This processor supports 4 priority levels:
	# 0 for Fast Interrupts (FIRQs) and 1-3 for Regular Interrupts (IRQs).
	default 4

config NUM_IRQS
	# must be > the highest interrupt number used
	default 20

config ARC_MPU_VER
	default 2

config RGF_NUM_BANKS
	default 2

config SYS_CLOCK_HW_CYCLES_PER_SEC
	default 10000000

config HARVARD
	def_bool y

config ARC_FIRQ
	def_bool y

config CACHE_FLUSHING
	def_bool y

if (ARC_MPU_VER = 2)

config MAIN_STACK_SIZE
	default 2048

config IDLE_STACK_SIZE
	default 2048

if ZTEST

config ZTEST_STACKSIZE
	default 2048

endif # ZTEST

endif # ARC_MPU_VER

endif #SOC_NSIM_EM
