# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOC_ESP32S3_APPCPU)

  zephyr_sources(
    soc_appcpu.c
    )

else()

  zephyr_sources(
    soc.c
    soc_cache.c
    esp32s3-mp.c
    ../common/loader.c
    )

endif()

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

# Power Management
zephyr_library_sources_ifdef(CONFIG_PM power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

# Get flash size to use in esptool as string
math(EXPR esptoolpy_flashsize "${CONFIG_FLASH_SIZE} / 0x100000")

# Get UART baudrate from DT
dt_chosen(dts_shell_uart PROPERTY "zephyr,shell-uart")
if(${dts_shell_uart})
  dt_prop(monitor_baud PATH ${dts_shell_uart} PROPERTY "current-speed")
endif()

board_runner_args(esp32 "--esp-monitor-baud=${monitor_baud}")

message("-- Espressif HAL path: ${ESP_IDF_PATH}")

# Select image processing

if(CONFIG_ESP_SIMPLE_BOOT OR CONFIG_MCUBOOT)

  if(CONFIG_BUILD_OUTPUT_BIN)

    set(ESPTOOL_PY ${ESP_IDF_PATH}/tools/esptool_py/esptool.py)
    message("-- Use the esptool.py: ${ESPTOOL_PY}")

    set(ELF2IMAGE_ARG "")
    if(NOT CONFIG_MCUBOOT)
      set(ELF2IMAGE_ARG "--ram-only-header")
    endif()

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ESPTOOL_PY}
      ARGS --chip ${CONFIG_SOC} elf2image ${ELF2IMAGE_ARG}
      --flash_mode dio --flash_freq 40m
      --flash_size ${esptoolpy_flashsize}MB
      -o ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf)

  endif()

endif()

set_property(TARGET bintools PROPERTY disassembly_flag_inline_source)

# Select the image origin depending on the boot configuration
if(CONFIG_SOC_ESP32S3_APPCPU)
  dt_nodelabel(dts_partition_path NODELABEL "slot0_appcpu_partition")
elseif(CONFIG_MCUBOOT)
  dt_nodelabel(dts_partition_path NODELABEL "boot_partition")
elseif(CONFIG_ESP_SIMPLE_BOOT)
  dt_nodelabel(dts_partition_path NODELABEL "boot_partition")
else()
  dt_nodelabel(dts_partition_path NODELABEL "slot0_partition")
endif()

dt_reg_addr(image_off PATH ${dts_partition_path})
board_finalize_runner_args(esp32 "--esp-app-address=${image_off}")

message("-- Image partition ${dts_partition_path}")

# Look for cross references between bootloader sections
if(CONFIG_MCUBOOT)

    message("check_callgraph using: ${ESP_IDF_PATH}/tools/ci/check_callgraph.py")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND
        ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/ci/check_callgraph.py
      ARGS
        --rtl-dirs ${CMAKE_BINARY_DIR}/zephyr
        --elf-file ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
        find-refs
        --from-section='.iram0.loader_text'
        --to-section='.iram0.text'
        --exit-code)
endif()

if(CONFIG_MCUBOOT)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_ESP32S3_APPCPU)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default_appcpu.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/default.ld CACHE INTERNAL "")
endif()
