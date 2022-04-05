# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ZTEST ztest)

zephyr_include_directories_ifdef(CONFIG_TEST
  ${ZEPHYR_BASE}/subsys/testsuite/include
  )
add_subdirectory_ifdef(CONFIG_COVERAGE_GCOV coverage)

zephyr_library_sources_ifdef(CONFIG_TEST_BUSY_SIM busy_sim/busy_sim.c)

if(NOT BOARD STREQUAL unit_testing)
  zephyr_linker_sources(RODATA include/ztest.ld)
endif()
