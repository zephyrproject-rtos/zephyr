#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if("${CONFIG_SOC_FAMILY}" STREQUAL "nxp_imx")
  add_subdirectory(imx)
elseif("${CONFIG_SOC_FAMILY}" STREQUAL "nxp_kinetis")
  add_subdirectory(kinetis)
elseif("${CONFIG_SOC_FAMILY}" STREQUAL "nxp_lpc")
  add_subdirectory(lpc)
endif()
