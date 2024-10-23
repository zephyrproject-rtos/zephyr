# Copyright (c) 2022 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(KCONFIG_ROOT ${CMAKE_CURRENT_LIST_DIR}/../Kconfig)

# Add the sources
list(APPEND test_sources src/main.cpp)
if(CONFIG_ZTEST_FAIL_TEST_ASSERT_AFTER)
  list(APPEND test_sources src/assert_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSERT_TEARDOWN)
  list(APPEND test_sources src/assert_teardown.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSUME_AFTER)
  list(APPEND test_sources src/assume_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSUME_TEARDOWN)
  list(APPEND test_sources src/assume_teardown.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_PASS_AFTER)
  list(APPEND test_sources src/pass_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_PASS_TEARDOWN)
  list(APPEND test_sources src/pass_teardown.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_UNEXPECTED_ASSUME)
  list(APPEND test_sources src/unexpected_assume.cpp)
endif()

if(BOARD STREQUAL unit_testing/unit_testing)
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)

  target_sources(testbinary PRIVATE ${test_sources})
  target_include_directories(testbinary PRIVATE include)
  install(TARGETS testbinary)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)

  target_sources(app PRIVATE ${test_sources})
  target_include_directories(app PRIVATE include)
  install(FILES ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_EXE_NAME}
    DESTINATION bin/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
      WORLD_EXECUTE WORLD_READ
  )
endif()
