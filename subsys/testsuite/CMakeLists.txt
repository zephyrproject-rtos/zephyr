# SPDX-License-Identifier: Apache-2.0

add_subdirectory_if_kconfig(ztest)

zephyr_include_directories_ifdef(CONFIG_TEST
  $ENV{ZEPHYR_BASE}/subsys/testsuite/include
  )
add_subdirectory_ifdef(CONFIG_COVERAGE_GCOV coverage)
