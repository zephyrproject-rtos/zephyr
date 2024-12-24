# Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
# SPDX-License-Identifier: Apache-2.0

config CLOCK_CONTROL_TCC_CCU
	bool "Telechips clock control driver"
	default y
	depends on DT_HAS_TCC_CCU_ENABLED
	help
	  This option enables the clock driver for Telechips TCC70XX series SOC.
