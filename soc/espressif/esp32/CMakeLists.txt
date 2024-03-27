# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOC_ESP32_APPCPU)
  zephyr_sources(soc_appcpu.c)
else()
  zephyr_sources(
    soc.c
    esp32-mp.c
    ../common/loader.c
    )
endif()

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)

zephyr_library_sources_ifdef(CONFIG_PM power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

# get flash size to use in esptool as string
math(EXPR esptoolpy_flashsize "${CONFIG_FLASH_SIZE} / 0x100000")

if(CONFIG_BOOTLOADER_ESP_IDF)

  set(bootloader_dir "${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/zephyr/blobs/lib/${CONFIG_SOC_SERIES}")

  if(EXISTS "${bootloader_dir}/bootloader-${CONFIG_SOC_SERIES}.bin")
    file(COPY "${bootloader_dir}/bootloader-${CONFIG_SOC_SERIES}.bin" DESTINATION ${CMAKE_BINARY_DIR})
    file(RENAME "${CMAKE_BINARY_DIR}/bootloader-${CONFIG_SOC_SERIES}.bin" "${CMAKE_BINARY_DIR}/bootloader.bin")
  endif()

  if(EXISTS "${bootloader_dir}/partition-table-${CONFIG_SOC_SERIES}.bin")
    file(COPY "${bootloader_dir}/partition-table-${CONFIG_SOC_SERIES}.bin" DESTINATION ${CMAKE_BINARY_DIR})
    file(RENAME "${CMAKE_BINARY_DIR}/partition-table-${CONFIG_SOC_SERIES}.bin" "${CMAKE_BINARY_DIR}/partition-table.bin")
  endif()
  board_finalize_runner_args(esp32 "--esp-flash-bootloader=${CMAKE_BINARY_DIR}/bootloader.bin")

  board_finalize_runner_args(esp32 "--esp-flash-partition_table=${CMAKE_BINARY_DIR}/partition-table.bin")

  board_finalize_runner_args(esp32 "--esp-partition-table-address=0x8000")

endif()

if(CONFIG_MCUBOOT OR CONFIG_BOOTLOADER_ESP_IDF)

  if(CONFIG_BUILD_OUTPUT_BIN)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/esptool_py/esptool.py
      ARGS --chip esp32 elf2image --flash_mode dio --flash_freq 40m --flash_size ${esptoolpy_flashsize}MB
      -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf)
  endif()

  if(CONFIG_MCUBOOT)
    board_finalize_runner_args(esp32 "--esp-flash-bootloader=${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin")
  endif()

endif()

## When building for APPCPU
if (CONFIG_SOC_ESP32_APPCPU)

  if(CONFIG_BUILD_OUTPUT_BIN)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/esp_bin2c_array.py
      ARGS -i ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.c
      -a "esp32_appcpu_fw_array")
  endif()

else()
  set_property(TARGET bintools PROPERTY disassembly_flag_inline_source)

  # get code-partition slot0 address
  dt_nodelabel(dts_partition_path NODELABEL "slot0_partition")
  dt_reg_addr(img_0_off PATH ${dts_partition_path})

  # get code-partition boot address
  dt_nodelabel(dts_partition_path NODELABEL "boot_partition")
  dt_reg_addr(boot_off PATH ${dts_partition_path})

  board_finalize_runner_args(esp32 "--esp-boot-address=${boot_off}")
  board_finalize_runner_args(esp32 "--esp-app-address=${img_0_off}")
endif()

if(CONFIG_MCUBOOT)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_ESP32_APPCPU)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default_appcpu.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default.ld CACHE INTERNAL "")
endif()
