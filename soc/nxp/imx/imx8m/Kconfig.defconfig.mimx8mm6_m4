# MIMX8MM6 M4 SoC defconfig

# Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

if SOC_MIMX8MM6_M4

config SYS_CLOCK_HW_CYCLES_PER_SEC
	int
	default 400000000

config IPM_IMX
	default y
	depends on IPM

config NUM_IRQS
	int
	# must be >= the highest interrupt number used
	default 127

config PINCTRL_IMX
	default y if HAS_MCUX_IOMUXC
	depends on PINCTRL

endif # SOC_MIMX8MM6_M4
