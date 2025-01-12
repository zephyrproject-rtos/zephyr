# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

# Load all drivers
mcux_load_all_cmakelists_in_directory(${SdkRootDirPath}/drivers)

if(NOT CONFIG_ASSERT OR CONFIG_FORCE_NO_ASSERT)
  zephyr_compile_definitions(NDEBUG) # squelch fsl_flexcan.c warning
endif()

zephyr_compile_definitions_ifdef(
  CONFIG_PTP_CLOCK_MCUX
  ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
)

zephyr_compile_definitions_ifdef(
  CONFIG_PTP_CLOCK_NXP_ENET
  ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
)

zephyr_library_compile_definitions_ifdef(
  CONFIG_HAS_MCUX_CACHE
  FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL
)

zephyr_library_compile_definitions_ifdef(
  CONFIG_DAI_NXP_SAI
  MCUX_SDK_SAI_ALLOW_NULL_FIFO_WATERMARK=1
)

zephyr_library_compile_definitions_ifdef(
  CONFIG_DAI_NXP_SAI
  MCUX_SDK_SAI_DISABLE_IMPLICIT_CHAN_CONFIG=1
)
