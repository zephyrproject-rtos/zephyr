# SPDX-License-Identifier: Apache-2.0

add_library(subsys__bluetooth INTERFACE)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_HCI host)
add_subdirectory_ifdef(CONFIG_BT_SHELL shell)
add_subdirectory_ifdef(CONFIG_BT_CONN services)
add_subdirectory_ifdef(CONFIG_BT_MESH mesh)

if(CONFIG_BT_CTLR)
  if(CONFIG_BT_LL_SW_LEGACY OR CONFIG_BT_LL_SW_SPLIT)
    add_subdirectory(controller)
  endif()
endif()
