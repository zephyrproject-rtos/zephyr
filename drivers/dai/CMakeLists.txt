# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DAI_INTEL_SSP intel/ssp)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_ALH intel/alh)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_DMIC intel/dmic)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_HDA intel/hda)
add_subdirectory_ifdef(CONFIG_DAI_NXP_SAI nxp/sai)
add_subdirectory_ifdef(CONFIG_DAI_NXP_ESAI nxp/esai)
