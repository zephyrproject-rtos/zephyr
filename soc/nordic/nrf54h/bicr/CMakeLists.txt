if(CONFIG_SOC_NRF54H20_GENERATE_BICR)
  # The bicr.json file shall be placed in the board folder, named
  # <board_target including revision>_bicr.json. For example:
  #
  #   nrf54h20dk_nrf54h20_cpuapp_0_9_0_bicr.json
  zephyr_build_string(
    bicr_build_string
    BOARD ${BOARD}
    BOARD_QUALIFIERS ${BOARD_QUALIFIERS}
    BOARD_REVISION ${BOARD_REVISION}
  )

  set(bicr_json_file ${BOARD_DIR}/${bicr_build_string}_bicr.json)
  set(bicr_hex_file ${PROJECT_BINARY_DIR}/bicr.hex)
  set(svd_file ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx/mdk/nrf54h20_application.svd)

  set(bicr_depricated_json_file ${BOARD_DIR}/bicr.json)
  if(EXISTS ${bicr_depricated_json_file})
    message(
      WARNING
      "The common bicr.json file at ${bicr_depricated_json_file} has been "
      "deprecated in favor of board target specific <board_target>_bicr.json "
      "files. Please rename the file at ${bicr_depricated_json_file} "
      "${bicr_build_string}_bicr.json"
    )
    set(bicr_json_file ${bicr_depricated_json_file})
  endif()

  if(EXISTS ${bicr_json_file})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${bicr_json_file})

    execute_process(
      COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_LIST_DIR}/bicrgen.py
        --svd ${svd_file}
        --input ${bicr_json_file}
        --output ${bicr_hex_file}
      WORKING_DIRECTORY ${BOARD_DIR}
      COMMAND_ERROR_IS_FATAL ANY
    )
    message(STATUS "Generated BICR hex file: ${bicr_hex_file}")
  endif()
endif()
