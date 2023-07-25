/*
 * Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
GROUP_START(DDR4)

	SECTION_DATA_PROLOGUE(_DDR4_SECTION_NAME,,SUBALIGN(16))
	{
		. = ALIGN(16);
		*(tflm_model tflm_arena tflm_input tflm_output)
	} GROUP_LINK_IN(DDR4)

GROUP_END(DDR4)
#endif
