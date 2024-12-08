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

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)

zephyr_library_sources_ifdef(CONFIG_PM power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

# get flash size to use in esptool as string
math(EXPR esptoolpy_flashsize "${CONFIG_FLASH_SIZE} / 0x100000")

# This includes CONFIG_MCUBOOT and CONFIG_ESP_SIMPLE_BOOT
if(NOT CONFIG_BOOTLOADER_MCUBOOT)

  if(CONFIG_BUILD_OUTPUT_BIN)

    set(ESPTOOL_PY ${ESP_IDF_PATH}/tools/esptool_py/esptool.py)
    message("esptool path: ${ESPTOOL_PY}")

    set(ELF2IMAGE_ARG "")
    if(NOT CONFIG_MCUBOOT)
      set(ELF2IMAGE_ARG "--ram-only-header")
    endif()

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ESPTOOL_PY}
      ARGS --chip esp32 elf2image ${ELF2IMAGE_ARG}
      --flash_mode dio --flash_freq 40m
      --flash_size ${esptoolpy_flashsize}MB
      -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf)
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

  # get code-partition boot address
  dt_nodelabel(dts_partition_path NODELABEL "boot_partition")
  dt_reg_addr(boot_off PATH ${dts_partition_path})

  # get code-partition slot0 address
  dt_nodelabel(dts_partition_path NODELABEL "slot0_partition")
  dt_reg_addr(img_0_off PATH ${dts_partition_path})

  if(CONFIG_BOOTLOADER_MCUBOOT)
    board_finalize_runner_args(esp32 "--esp-app-address=${img_0_off}")
  else()
    board_finalize_runner_args(esp32 "--esp-app-address=${boot_off}")
  endif()

endif()

if(CONFIG_MCUBOOT)
    # search from cross references between bootloader sections
    message("check_callgraph using: ${ESP_IDF_PATH}/tools/ci/check_callgraph.py")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND
        ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/ci/check_callgraph.py
      ARGS
        --rtl-dirs ${CMAKE_BINARY_DIR}/zephyr
	--elf-file ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
        find-refs --from-section=.iram0.iram_loader --to-section=.iram0.text
        --exit-code)
endif()

if(CONFIG_MCUBOOT)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_ESP32_APPCPU)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default_appcpu.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default.ld CACHE INTERNAL "")
endif()
