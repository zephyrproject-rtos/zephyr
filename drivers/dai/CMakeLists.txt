# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DAI_INTEL_SSP intel/ssp)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_ALH intel/alh)
add_subdirectory_ifdef(CONFIG_DAI_INTEL_DMIC intel/dmic)
