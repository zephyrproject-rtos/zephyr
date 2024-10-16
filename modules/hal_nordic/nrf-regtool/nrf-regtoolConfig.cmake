# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

function(nrf_regtool_generate_uicr generated_hex_file)
  string(REPEAT "-v;" ${CONFIG_NRF_REGTOOL_VERBOSITY} verbosity)
  execute_process(
    COMMAND
    ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
    ${NRF_REGTOOL} ${verbosity} uicr-compile
    --edt-pickle-file ${EDT_PICKLE}
    --product-name ${CONFIG_SOC}
    --output-file ${generated_hex_file}
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
  message(STATUS "Generated UICR hex file: ${generated_hex_file}")
endfunction()

function(nrf_regtool_generate_peripheral peripheral generated_hex_file)
  # Prepare common argument sub-lists.
  string(REPEAT "-v;" ${CONFIG_NRF_REGTOOL_VERBOSITY} verbosity)
  list(TRANSFORM CACHED_DTS_ROOT_BINDINGS PREPEND "--bindings-dir;" OUTPUT_VARIABLE bindings_dirs)

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
endfunction()

get_property(version GLOBAL PROPERTY nrf_regtool_version)

foreach(component IN LISTS ${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
  if(component STREQUAL "GENERATE:UICR")
    set(generated_hex_file ${PROJECT_BINARY_DIR}/uicr.hex)
    if(version VERSION_GREATER_EQUAL 7.0.0)
      nrf_regtool_generate_uicr(${generated_hex_file})
    else()
      nrf_regtool_generate_peripheral(UICR ${generated_hex_file})
    endif()

    # UICR must be flashed together with the Zephyr binary.
    set(merged_hex_file ${PROJECT_BINARY_DIR}/uicr_merged.hex)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${merged_hex_file}
      ${generated_hex_file}
      ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
    )
    set_property(TARGET runners_yaml_props_target PROPERTY hex_file ${merged_hex_file})

  else()
    message(FATAL_ERROR "Unrecognized package component: \"${component}\"")
  endif()
endforeach()
