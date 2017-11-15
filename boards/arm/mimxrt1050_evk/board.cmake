#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(DEBUG_SCRIPT jlink.sh)
endif()

set(JLINK_DEVICE Cortex-M7)

set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS
  JLINK_DEVICE
  )
