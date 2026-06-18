# SPDX-License-Identifier: Apache-2.0

if(CMAKE_SCRIPT_MODE_FILE)
  set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../../..)
endif()

set(FORMAT armlink)
set(ENTRY __start)
list(APPEND MEMORY_REGIONS "{NAME\;FLASH\;START\;0x0\;SIZE\;64K}")
list(APPEND SECTIONS "{NAME\;.text\;GROUP\;FLASH}")
list(APPEND SECTIONS "{NAME\;.empty_noinput\;GROUP\;FLASH\;NOINPUT\;TRUE}")

include(${ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake)

set(expected_empty_noinput "empty_noinput +0 EMPTY 0x0\n  {\n  }")
string(FIND "${OUT}" "${expected_empty_noinput}" empty_noinput_pos)
if(empty_noinput_pos EQUAL -1)
  message(FATAL_ERROR
    "Empty NOINPUT section did not generate a valid empty scatter block.\n"
    "Expected to find:\n${expected_empty_noinput}\n"
    "Generated scatter output:\n${OUT}"
  )
endif()
