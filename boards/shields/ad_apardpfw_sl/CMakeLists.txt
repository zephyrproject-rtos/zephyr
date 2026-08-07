# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_SHIELD_AD_APARDPFW_SL)
  message(WARNING "
    Warning [AD-APARDPFW-SL]: ADIN2111 is not enabled in the devicetree.
    A board-specific overlay defining the ADIN2111 on the correct SPI bus is required.
    See boards/apard32690_max32690_m4.overlay for an example.")
endif()
