# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

if(NOT CONFIG_MCUBOOT AND NOT CONFIG_SOC_ESP32_APPCPU AND NOT CONFIG_SOC_ESP32S3_APPCPU)
  zephyr_sources_ifdef(CONFIG_ESP_SPIRAM esp_psram.c)
endif()

# Get flash size to use in esptool as string
math(EXPR esptoolpy_flashsize "${CONFIG_FLASH_SIZE} / 0x100000")

# Get UART baudrate from DT
dt_chosen(dts_shell_uart PROPERTY "zephyr,shell-uart")
if(DEFINED dts_shell_uart AND dts_shell_uart)
  dt_prop(monitor_baud PATH ${dts_shell_uart} PROPERTY "current-speed")
endif()

message("-- Espressif HAL path: ${ESP_IDF_PATH}")

if((CONFIG_ESP_SIMPLE_BOOT OR CONFIG_MCUBOOT) AND NOT CONFIG_SOC_ESP32C6_LPCORE)
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
      --flash_mode dio
      --flash_freq ${CONFIG_ESPTOOLPY_FLASHFREQ}
      --flash_size ${esptoolpy_flashsize}MB
      -o ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME})
  endif()
endif()

set_property(TARGET bintools PROPERTY disassembly_flag_inline_source)

# Select the image origin depending on the boot configuration
if(CONFIG_SOC_ESP32_APPCPU OR CONFIG_SOC_ESP32S3_APPCPU)
  dt_nodelabel(dts_partition_path NODELABEL "slot0_appcpu_partition")
elseif(CONFIG_SOC_ESP32C6_LPCORE)
  dt_nodelabel(dts_partition_path NODELABEL "slot0_lpcore_partition")
elseif(CONFIG_MCUBOOT OR CONFIG_ESP_SIMPLE_BOOT)
  dt_nodelabel(dts_partition_path NODELABEL "boot_partition")
else()
  dt_nodelabel(dts_partition_path NODELABEL "slot0_partition")
endif()

dt_reg_addr(image_off PATH ${dts_partition_path})
board_runner_args(esp32 "--esp-app-address=${image_off}")
board_runner_args(esp32 "--esp-flash-size=${esptoolpy_flashsize}MB")
board_runner_args(esp32 "--esp-flash-freq=${CONFIG_ESPTOOLPY_FLASHFREQ}")
board_runner_args(esp32 "--esp-flash-mode=${CONFIG_ESPTOOLPY_FLASHMODE}")
board_finalize_runner_args(esp32 "--esp-monitor-baud=${monitor_baud}")

message("-- Image partition ${dts_partition_path}")

# Look for cross references between bootloader sections
if(CONFIG_MCUBOOT)
    message("check_callgraph using: ${ESP_IDF_PATH}/tools/ci/check_callgraph.py")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND
        ${PYTHON_EXECUTABLE} ${ESP_IDF_PATH}/tools/ci/check_callgraph.py
      ARGS
        --rtl-dirs ${CMAKE_BINARY_DIR}/zephyr
        --elf-file ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
        find-refs
        --from-section='.iram0.loader_text'
        --to-section='.iram0.text'
        --exit-code)
endif()

if(CONFIG_MCUBOOT)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../${CONFIG_SOC}/mcuboot.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_ESP32_APPCPU OR CONFIG_SOC_ESP32S3_APPCPU)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../${CONFIG_SOC}/default_appcpu.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_ESP32C6_LPCORE)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../${CONFIG_SOC}/default_lpcore.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../${CONFIG_SOC}/default.ld CACHE INTERNAL "")
endif()
