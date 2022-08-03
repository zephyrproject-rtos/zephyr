# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ZTEST ztest)
add_subdirectory_ifdef(CONFIG_UNITY unity)
add_subdirectory_ifdef(CONFIG_CMOCK cmock)

if(CONFIG_TEST)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)
  zephyr_include_directories_ifdef(
      CONFIG_LEGACY_INCLUDE_PATH ${ZEPHYR_BASE}/subsys/testsuite/include/zephyr)
endif()
add_subdirectory_ifdef(CONFIG_COVERAGE_GCOV coverage)

zephyr_library_sources_ifdef(CONFIG_TEST_BUSY_SIM busy_sim/busy_sim.c)
