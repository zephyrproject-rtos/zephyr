# SPDX-License-Identifier: Apache-2.0

function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  ExternalProject_Get_Property(${DEFAULT_IMAGE} BINARY_DIR)
  import_kconfig(CONFIG_ ${BINARY_DIR}/zephyr/.config)

  if("${SB_CONFIG_TEST_FOO_VAL}" STREQUAL "${CONFIG_TEST_FOO_VAL}")
    message(FATAL_ERROR "Values match (sysbuild and app): ${SB_CONFIG_TEST_FOO_VAL} and ${CONFIG_TEST_FOO_VAL}")
  else()
    message(NOTICE "Values diverge (sysbuild and app)")
  endif()

  if("${SB_CONFIG_EXPECTED_SB_TEST_FOO_VAL}" STREQUAL "${SB_CONFIG_TEST_FOO_VAL}")
    message(NOTICE "Values match (sysbuild and snippet): ${SB_CONFIG_EXPECTED_SB_TEST_FOO_VAL} and ${SB_CONFIG_TEST_FOO_VAL}")
  else()
    message(FATAL_ERROR "Values diverge (sysbuild and snippet): ${SB_CONFIG_EXPECTED_SB_TEST_FOO_VAL} and ${SB_CONFIG_TEST_FOO_VAL}")
  endif()

  if("${SB_CONFIG_EXPECTED_APP_TEST_FOO_VAL}" STREQUAL "${CONFIG_TEST_FOO_VAL}")
    message(NOTICE "Values match (app and snippet): ${SB_CONFIG_EXPECTED_APP_TEST_FOO_VAL} and ${CONFIG_TEST_FOO_VAL}")
  else()
    message(FATAL_ERROR "Values diverge (app and snippet): ${SB_CONFIG_EXPECTED_APP_TEST_FOO_VAL} and ${CONFIG_TEST_FOO_VAL}")
  endif()
endfunction()
