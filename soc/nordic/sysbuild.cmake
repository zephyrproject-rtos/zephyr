# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_VPR_LAUNCHER)
  set(launcher_core "cpuapp")
  string(REPLACE "/" ";" launcher_quals ${BOARD_QUALIFIERS})
  list(LENGTH launcher_quals launcher_quals_len)
  list(GET launcher_quals 1 launcher_soc)
  list(GET launcher_quals 2 launcher_vpr)

  string(REPLACE "cpu" "" launcher_vpr ${launcher_vpr})

  if(launcher_quals_len EQUAL 4)
    list(GET launcher_quals 3 launcher_variant)
    set(launcher_vpr ${launcher_vpr}-${launcher_variant})
  endif()

  string(CONCAT launcher_board ${BOARD} "/" ${launcher_soc} "/" ${launcher_core})

  set(image "vpr_launcher")

  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${ZEPHYR_BASE}/samples/basic/minimal
    BOARD ${launcher_board}
  )

  string(CONCAT launcher_snippet "nordic-" ${launcher_vpr})

  sysbuild_cache_set(VAR ${image}_SNIPPET APPEND REMOVE_DUPLICATES ${launcher_snippet})
endif()
