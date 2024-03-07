# MIMX8MQ6 M4 SoC defconfig

# Copyright (c) 2021, Kwon Tae-young <tykwon@m2i.co.kr>
# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

if SOC_MIMX8MQ6_M4

config SYS_CLOCK_HW_CYCLES_PER_SEC
	int
	default 266000000

config PINCTRL_IMX
	default y if HAS_MCUX_IOMUXC
	depends on PINCTRL

config NUM_IRQS
	int
	# must be >= the highest interrupt number used
	default 127

endif # SOC_MIMX8MQ6_M4
