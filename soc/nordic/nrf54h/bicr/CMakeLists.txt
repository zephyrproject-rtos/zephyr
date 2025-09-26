if(CONFIG_SOC_NRF54H20_GENERATE_BICR)
  set(bicr_json_file ${BOARD_DIR}/${CONFIG_SOC_NRF54H20_BICR_NAME})
  set(bicr_hex_file ${PROJECT_BINARY_DIR}/bicr.hex)
  set(svd_file ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx/mdk/nrf54h20_application.svd)

  set(app_bicr_json_file ${APPLICATION_SOURCE_DIR}/${CONFIG_SOC_NRF54H20_BICR_NAME})
  if(EXISTS ${app_bicr_json_file})
    set(bicr_json_file ${app_bicr_json_file})
  endif()

  message(STATUS "Found BICR: ${bicr_json_file}")
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
