#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if(SB_CONFIG_SLOT0_IMAGE_BUILD_ONLY)
  set_target_properties(${DEFAULT_IMAGE} PROPERTIES BUILD_ONLY y)
else()
  set_target_properties(${DEFAULT_IMAGE} PROPERTIES BUILD_ONLY n)
endif()
