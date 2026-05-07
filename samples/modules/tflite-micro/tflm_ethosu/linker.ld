/*
 * SPDX-FileCopyrightText: <text>Copyright 2022, 2026 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * If a chosen handle is provided, use it.
 * Otherwise default to DDR4.
 */
#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_tflm_storage))
  #define SAMPLE_TFLM_REGION DT_STRING_UPPER_TOKEN(DT_CHOSEN(zephyr_tflm_storage), zephyr_memory_region)
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(ddr4), okay)
  #define SAMPLE_TFLM_REGION DDR4
#else
  #error "No TFLM storage region: set chosen zephyr,tflm-storage in an overlay or add a ddr4 node."
#endif

GROUP_START(SAMPLE_TFLM_REGION)
SECTION_DATA_PROLOGUE(_TFLM_STORAGE_REGION_SECTION_NAME,,SUBALIGN(16))
{
  . = ALIGN(16);
  *(tflm_model tflm_arena tflm_input tflm_output)
} GROUP_LINK_IN(SAMPLE_TFLM_REGION)
GROUP_END(SAMPLE_TFLM_REGION)

#undef SAMPLE_TFLM_REGION
