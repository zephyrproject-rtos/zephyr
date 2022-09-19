# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

if(CONFIG_BUILD_OUTPUT_BIN)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/components/esptool_py/esptool/esptool.py
    ARGS --chip esp32 elf2image --flash_mode dio --flash_freq 40m
    -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
    ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/esp_bin2c_array.py
    ARGS -i ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
    -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.c
    -a "esp32_net_fw_array")
endif()
