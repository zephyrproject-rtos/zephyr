# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/msbc.c
  ${CMAKE_CURRENT_LIST_DIR}/sco_hci.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR})
