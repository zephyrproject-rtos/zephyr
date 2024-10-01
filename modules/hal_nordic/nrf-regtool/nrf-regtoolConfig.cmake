# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

function(nrf_regtool_generate_hex_from_dts peripheral)
  string(TOLOWER "${peripheral}.hex" generated_hex_name)
  string(TOLOWER "${peripheral}_merged.hex" merged_hex_name)

  # Prepare common argument sub-lists.
  string(REPEAT "-v;" ${CONFIG_NRF_REGTOOL_VERBOSITY} verbosity)
  list(TRANSFORM CACHED_DTS_ROOT_BINDINGS PREPEND "--bindings-dir;" OUTPUT_VARIABLE bindings_dirs)
  separate_arguments(extra_args UNIX_COMMAND "${CONFIG_NRF_REGTOOL_EXTRA_GENERATE_ARGS}")

  set(generated_hex_file ${PROJECT_BINARY_DIR}/${generated_hex_name})
  execute_process(
    COMMAND
    ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
    ${NRF_REGTOOL} ${verbosity} generate
    --peripheral ${peripheral}
    --svd-file ${SOC_SVD_FILE}
    --dts-file ${ZEPHYR_DTS}
    ${bindings_dirs}
    --output-file ${generated_hex_file}
    ${extra_args}
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
  message(STATUS "Generated ${peripheral} hex file: ${generated_hex_file}")

  set(merged_hex_file ${PROJECT_BINARY_DIR}/${merged_hex_name})
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${merged_hex_file}
    ${generated_hex_file}
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
  )
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file ${merged_hex_file})
endfunction()


foreach(component IN LISTS ${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
  string(REGEX MATCH "(^.*):(.*$)" match ${component})
  set(operation  "${CMAKE_MATCH_1}")
  set(peripheral "${CMAKE_MATCH_2}")

  if(operation STREQUAL "GENERATE")
    nrf_regtool_generate_hex_from_dts(${peripheral})
  else()
    message(FATAL_ERROR "Unrecognized package component: \"${component}\"")
  endif()
endforeach()
