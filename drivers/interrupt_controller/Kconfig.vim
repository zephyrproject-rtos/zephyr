# Copyright (C) 2023 BeagleBoard.org Foundation
# Copyright (C) 2023 S Prashanth
#
# SPDX-License-Identifier: Apache-2.0

if CPU_CORTEX_R5

config VIM
	bool "TI Vectored Interrupt Manager"
	default y
	depends on DT_HAS_TI_VIM_ENABLED
	help
		The TI Vectored Interrupt Manager provides hardware assistance for prioritizing
		and aggregating the interrupt sources for ARM Cortex-R5 processor cores.

endif # CPU_CORTEX_R5
