# SPDX-License-Identifier: Apache-2.0

add_library(subsys__bluetooth INTERFACE)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_HCI host)
add_subdirectory_ifdef(CONFIG_BT_SHELL shell)
add_subdirectory_ifdef(CONFIG_BT_CONN services)
add_subdirectory_ifdef(CONFIG_BT_MESH mesh)
add_subdirectory_ifdef(CONFIG_BT_AUDIO audio)
add_subdirectory_ifdef(CONFIG_BT_CRYPTO crypto)

if(CONFIG_BT_CTLR AND CONFIG_BT_LL_SW_SPLIT)
  include(${ZEPHYR_BASE}/modules/hal_nordic/nrfx/reserve_ppi.cmake)

  set(MPSL_PPIS 4 20)
  reserve_ppi("MPSL" "${MPSL_PPIS}" "")

  add_subdirectory(controller)
endif()

zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth)

add_subdirectory(lib)
