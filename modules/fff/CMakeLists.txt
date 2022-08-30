# Copyright (c) 2021 Google LLC
#
# SPDX-License-Identifier: Apache-2.0


if(CONFIG_FFF_TEST)
  # Add the FFF C test suites as ztest suites
  message("CONFIG_FFF_TEST_TYPE_C=${CONFIG_FFF_TEST_TYPE_C}")
  message("CONFIG_FFF_TEST_TYPE_GLOBAL_C=${CONFIG_FFF_TEST_TYPE_GLOBAL_C}")
  zephyr_library()
  zephyr_include_directories(
      include
      "${ZEPHYR_CURRENT_MODULE_DIR}"
      "${ZEPHYR_CURRENT_MODULE_DIR}/test"
  )
  if(DEFINED CONFIG_FFF_TEST_TYPE_C)
    zephyr_library_sources("${ZEPHYR_CURRENT_MODULE_DIR}/test/fff_test_c.c")
  elseif(DEFINED CONFIG_FFF_TEST_TYPE_GLOBAL_C)
    zephyr_library_sources(
        "${ZEPHYR_CURRENT_MODULE_DIR}/test/fff_test_global_c.c"
        "${ZEPHYR_CURRENT_MODULE_DIR}/test/global_fakes.c"
    )
  endif()
endif()
