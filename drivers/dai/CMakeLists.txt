# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_DAI_INTEL_ALH intel/alh)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_DMIC intel/dmic)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_HDA intel/hda)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_SSP intel/ssp)
add_subdirectory_ifdef(CONFIG_DAI_NXP_ESAI nxp/esai)
add_subdirectory_ifdef(CONFIG_DAI_NXP_MICFIL nxp/micfil)
add_subdirectory_ifdef(CONFIG_DAI_NXP_SAI nxp/sai)
# zephyr-keep-sorted-stop

if(CONFIG_DAI_USERSPACE)
  zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/dai.h)
  zephyr_library_sources(dai_handlers.c)
endif()
