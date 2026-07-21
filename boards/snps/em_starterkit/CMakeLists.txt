# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2017 Synopsys

if((CONFIG_BOARD_EM_STARTERKIT_EMSK_EM9D OR CONFIG_BOARD_EM_STARTERKIT_EMSK_EM11D) AND "${BOARD_REVISION}" STREQUAL "2.2")
  message(FATAL_ERROR "Board revision 2.2 is not supported for this SoC")
endif()

zephyr_sources(pmodmux.c)
zephyr_sources_ifdef(CONFIG_ARC_MPU_ENABLE arc_mpu_regions.c)
