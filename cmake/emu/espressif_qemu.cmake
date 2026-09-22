# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Espressif QEMU emu platform (SUPPORTED_EMU_PLATFORMS espressif_qemu).
#
# Boards opt in with CONFIG_ESPRESSIF_QEMU_TARGET. This file is included late by the
# top-level CMakeLists (after the Zephyr ELF exists), so flash-image merge and
# run_/debugserver_ targets are created here directly.
#
# Flash layout properties are published by soc/espressif/common/CMakeLists.txt.

# Per-SoC QEMU binary, machine type and extra flags
if(CONFIG_SOC_SERIES_ESP32)
  set(ESPRESSIF_QEMU_BIN_NAME qemu-system-xtensa)
  set(ESPRESSIF_QEMU_MACHINE esp32)
  set(ESPRESSIF_QEMU_EFUSE_DRIVER nvram.esp32.efuse)
  # Default QEMU efuses report ESP32 rev 0; Zephyr aborts below ECO3 unless
  # CONFIG_ESP32_USE_UNSUPPORTED_REVISION is set. Provide the ECO3 efuse
  # image documented by Espressif so the guest reports v3.0 automatically.
  # https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md#emulating-esp32-eco3
  set(ESPRESSIF_QEMU_EXTRA_FLAGS "")
elseif(CONFIG_SOC_SERIES_ESP32S3)
  set(ESPRESSIF_QEMU_BIN_NAME qemu-system-xtensa)
  set(ESPRESSIF_QEMU_MACHINE esp32s3)
  set(ESPRESSIF_QEMU_EFUSE_DRIVER nvram.esp32s3.efuse)
  set(ESPRESSIF_QEMU_EXTRA_FLAGS "")
elseif(CONFIG_SOC_SERIES_ESP32C3)
  set(ESPRESSIF_QEMU_BIN_NAME qemu-system-riscv32)
  set(ESPRESSIF_QEMU_MACHINE esp32c3)
  set(ESPRESSIF_QEMU_EFUSE_DRIVER nvram.esp32c3.efuse)
  # Free-running mode is not supported; 1<<3 = 8 ns/insn, about 125 MHz
  set(ESPRESSIF_QEMU_EXTRA_FLAGS "-icount;3")
elseif(CONFIG_SOC_SERIES_ESP32C6)
  set(ESPRESSIF_QEMU_BIN_NAME qemu-system-riscv32)
  set(ESPRESSIF_QEMU_MACHINE esp32c6)
  set(ESPRESSIF_QEMU_EFUSE_DRIVER nvram.esp32c6.efuse)
  set(ESPRESSIF_QEMU_EXTRA_FLAGS "-icount;3")
else()
  message(FATAL_ERROR "Espressif QEMU: unsupported SoC series")
endif()

# PSRAM: derive QEMU -m from CONFIG_ESP_SPIRAM_SIZE (board DT psram0 → Kconfig).
if(CONFIG_ESP_SPIRAM)
  if(CONFIG_ESP_SPIRAM_SIZE LESS_EQUAL 0)
    message(WARNING
      "Espressif QEMU: CONFIG_ESP_SPIRAM is set but CONFIG_ESP_SPIRAM_SIZE is 0; "
      "check psram0 size in the board device tree.")
  else()
    math(EXPR _psram_mb "${CONFIG_ESP_SPIRAM_SIZE} / 1048576")
    math(EXPR _psram_remainder "${CONFIG_ESP_SPIRAM_SIZE} % 1048576")
    set(_psram_valid_sizes 2 4)
    if(CONFIG_SOC_SERIES_ESP32S3)
      list(APPEND _psram_valid_sizes 8 16 32)
    endif()
    list(FIND _psram_valid_sizes "${_psram_mb}" _psram_size_index)
    if(NOT _psram_remainder EQUAL 0)
      message(FATAL_ERROR
        "Espressif QEMU: CONFIG_ESP_SPIRAM_SIZE=${CONFIG_ESP_SPIRAM_SIZE} is "
        "not a whole number of MiB.")
    elseif(_psram_size_index EQUAL -1)
      message(FATAL_ERROR
        "Espressif QEMU: PSRAM -m${_psram_mb}M is unsupported "
        "(QEMU documents ESP32: 2M/4M; ESP32-S3: 2M/4M/8M/16M/32M). "
        "Override psram0 size for the /qemu board or sample overlay "
        "(CONFIG_ESP_SPIRAM_SIZE=${CONFIG_ESP_SPIRAM_SIZE}).")
    endif()
    list(APPEND ESPRESSIF_QEMU_EXTRA_FLAGS "-m;${_psram_mb}M")
    message(STATUS
      "Espressif QEMU: PSRAM -m${_psram_mb}M "
      "(CONFIG_ESP_SPIRAM_SIZE=${CONFIG_ESP_SPIRAM_SIZE} bytes, from board DT psram0)")
  endif()
endif()

if(CONFIG_ESP_SPIRAM AND CONFIG_SOC_SERIES_ESP32S3 AND CONFIG_SPIRAM_MODE_OCT)
  list(APPEND ESPRESSIF_QEMU_EXTRA_FLAGS
    "-global;driver=ssi_psram,property=is_octal,value=true")
  message(STATUS "Espressif QEMU: octal PSRAM (CONFIG_SPIRAM_MODE_OCT)")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/espressif_qemu/efuse.cmake)

# Locate a QEMU that actually implements the target machine.
#
# find_program() alone is not enough: the Zephyr SDK ships its own
# qemu-system-xtensa / qemu-system-riscv32 in hosttools and those take
# precedence in CMake's search path, but they are upstream builds without the
# Espressif machines. Probe each candidate with '-machine help' and accept the
# first one that lists ${ESPRESSIF_QEMU_MACHINE}.
set(_esp_qemu_dirs "")
if(DEFINED ENV{ESPRESSIF_QEMU_PATH})
  file(TO_CMAKE_PATH "$ENV{ESPRESSIF_QEMU_PATH}" _esp_qemu_explicit_dirs)
  list(APPEND _esp_qemu_dirs ${_esp_qemu_explicit_dirs})
endif()
if(DEFINED ENV{QEMU_BIN_PATH})
  file(TO_CMAKE_PATH "$ENV{QEMU_BIN_PATH}" _esp_qemu_bin_dirs)
  list(APPEND _esp_qemu_dirs ${_esp_qemu_bin_dirs})
endif()
if(DEFINED ENV{PATH})
  file(TO_CMAKE_PATH "$ENV{PATH}" _esp_qemu_path_dirs)
  list(APPEND _esp_qemu_dirs ${_esp_qemu_path_dirs})
endif()
list(REMOVE_DUPLICATES _esp_qemu_dirs)

set(ESPRESSIF_QEMU_EXECUTABLE "")
set(_esp_qemu_rejected "")
foreach(_dir IN LISTS _esp_qemu_dirs)
  set(_cand "${_dir}/${ESPRESSIF_QEMU_BIN_NAME}${CMAKE_HOST_EXECUTABLE_SUFFIX}")
  if(NOT EXISTS "${_cand}")
    continue()
  endif()

  execute_process(
    COMMAND "${_cand}" -machine help
    OUTPUT_VARIABLE _cand_machines
    ERROR_VARIABLE _cand_stderr
    RESULT_VARIABLE _cand_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    TIMEOUT 10
  )
  if(NOT _cand_result EQUAL 0)
    continue()
  endif()

  if(_cand_machines MATCHES "(^|\n)${ESPRESSIF_QEMU_MACHINE}[ \t]")
    set(ESPRESSIF_QEMU_EXECUTABLE "${_cand}")
    break()
  endif()
  list(APPEND _esp_qemu_rejected "${_cand}")
endforeach()

if(ESPRESSIF_QEMU_EXECUTABLE)
  message(STATUS
    "Espressif QEMU: ${ESPRESSIF_QEMU_EXECUTABLE} (-machine ${ESPRESSIF_QEMU_MACHINE})")
else()
  set(_esp_qemu_hint "")
  if(_esp_qemu_rejected)
    string(REPLACE ";" "\n  " _esp_qemu_rejected_list "${_esp_qemu_rejected}")
    string(CONCAT _esp_qemu_hint
      "Found ${ESPRESSIF_QEMU_BIN_NAME} without "
      "'${ESPRESSIF_QEMU_MACHINE}' support at:\n"
      "  ${_esp_qemu_rejected_list}\n")
  endif()
  message(WARNING
    "Espressif QEMU: no ${ESPRESSIF_QEMU_BIN_NAME} supporting "
    "'-machine ${ESPRESSIF_QEMU_MACHINE}' was found.\n"
    "${_esp_qemu_hint}"
    "Install Espressif's fork (https://github.com/espressif/qemu/releases) and "
    "set ESPRESSIF_QEMU_PATH to its bin directory, or put it earlier on PATH.\n"
    "'west build -t run' will report this error instead of running."
  )
endif()

set(ESPRESSIF_FLASH_IMAGE ${CMAKE_BINARY_DIR}/zephyr/flash_image.bin)

# Assemble the merged SPI flash image QEMU boots from.
#
# Depend on the ELF target (not only an extra_post_build_commands entry): with
# MCUboot the signing command is appended later by cmake/mcuboot.cmake, so a
# merge registered earlier would run before zephyr.signed.bin exists.
function(espressif_qemu_add_image_target)
  get_property(_flash_mb GLOBAL PROPERTY espressif_qemu_flash_size_mb)
  if(NOT _flash_mb)
    message(WARNING
      "Espressif QEMU: flash layout not published by soc/espressif/common; "
      "no flash image will be generated.")
    return()
  endif()

  if(CONFIG_BOOTLOADER_MCUBOOT)
    get_property(_mcuboot_off GLOBAL PROPERTY espressif_qemu_mcuboot_off)
    get_property(_slot0_off GLOBAL PROPERTY espressif_qemu_slot0_off)
    # Sysbuild builds MCUboot as a sibling domain of the application. A manual
    # build has no bootloader in the tree: on hardware it is already flashed,
    # but the QEMU image has to carry it, so take it from
    # ESPRESSIF_QEMU_MCUBOOT_BIN.
    if(SYSBUILD)
      set(_mcuboot_bin ${APPLICATION_BINARY_DIR}/../mcuboot/zephyr/zephyr.bin)
    elseif(ESPRESSIF_QEMU_MCUBOOT_BIN)
      set(_mcuboot_bin ${ESPRESSIF_QEMU_MCUBOOT_BIN})
      # The merge command runs from the build tree, where a path relative to
      # the shell that invoked west would not resolve.
      if(NOT IS_ABSOLUTE "${_mcuboot_bin}")
        message(WARNING
          "Espressif QEMU: ESPRESSIF_QEMU_MCUBOOT_BIN must be an absolute "
          "path: ${_mcuboot_bin}. No flash image will be generated.")
        return()
      endif()
      if(NOT EXISTS "${_mcuboot_bin}")
        message(WARNING
          "Espressif QEMU: ESPRESSIF_QEMU_MCUBOOT_BIN does not exist: "
          "${_mcuboot_bin}. No flash image will be generated.")
        return()
      endif()
    else()
      message(WARNING
        "Espressif QEMU: MCUboot is enabled but this is not a sysbuild, so the "
        "bootloader binary is not in the build tree. QEMU starts from an empty "
        "flash and cannot use a previously flashed bootloader. Set "
        "-DESPRESSIF_QEMU_MCUBOOT_BIN=<path to mcuboot zephyr.bin> or build "
        "with --sysbuild. No flash image will be generated.")
      return()
    endif()
    set(_merge_parts
      ${_mcuboot_off} ${_mcuboot_bin}
      ${_slot0_off} ${BYPRODUCT_KERNEL_SIGNED_BIN_NAME}
    )
    set(_merge_desc "MCUboot@${_mcuboot_off} + signed app@${_slot0_off}")
    # Include the bootloader binary so editing/rebuilding MCUboot regenerates
    # flash_image.bin (file-level dep; sysbuild sibling or ESPRESSIF_QEMU_MCUBOOT_BIN).
    set(_merge_deps ${BYPRODUCT_KERNEL_SIGNED_BIN_NAME} ${_mcuboot_bin})
  else()
    get_property(_image_off GLOBAL PROPERTY espressif_qemu_image_off)
    set(_merge_parts ${_image_off} ${BYPRODUCT_KERNEL_BIN_NAME})
    set(_merge_desc "Simple Boot image@${_image_off}")
    set(_merge_deps ${BYPRODUCT_KERNEL_BIN_NAME})
  endif()

  add_custom_command(
    OUTPUT ${ESPRESSIF_FLASH_IMAGE}
    COMMAND ${ESPTOOL_EXECUTABLE} --chip ${CONFIG_SOC} merge-bin
      --pad-to-size ${_flash_mb}MB
      -o ${ESPRESSIF_FLASH_IMAGE}
      ${_merge_parts}
    DEPENDS ${logical_target_for_zephyr_elf} ${_merge_deps}
    COMMENT "Espressif QEMU: merging ${_merge_desc}"
    VERBATIM
  )
  add_custom_target(espressif_qemu_flash_image ALL
    DEPENDS ${ESPRESSIF_FLASH_IMAGE}
  )
endfunction()

if(NOT TARGET run_espressif_qemu)
  if(ESPRESSIF_QEMU_EXECUTABLE)
    set(_qemu_cmd
      ${ESPRESSIF_QEMU_EXECUTABLE}
      -nographic
      -machine ${ESPRESSIF_QEMU_MACHINE}
    )
    if(ESPRESSIF_QEMU_EXTRA_FLAGS)
      list(APPEND _qemu_cmd ${ESPRESSIF_QEMU_EXTRA_FLAGS})
    endif()
    # Allow callers to inject extra args (SD card, watchdog disable, GUI, ...)
    if(DEFINED ENV{QEMU_EXTRA_FLAGS} AND NOT "$ENV{QEMU_EXTRA_FLAGS}" STREQUAL "")
      separate_arguments(_qemu_extra_env UNIX_COMMAND "$ENV{QEMU_EXTRA_FLAGS}")
      list(APPEND _qemu_cmd ${_qemu_extra_env})
    endif()
    list(APPEND _qemu_cmd
      -drive file=${ESPRESSIF_FLASH_IMAGE},if=mtd,format=raw
    )

    add_custom_target(run_espressif_qemu
      COMMAND ${_qemu_cmd}
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      USES_TERMINAL
      COMMENT "Running Espressif QEMU (${ESPRESSIF_QEMU_MACHINE})"
    )

    add_custom_target(debugserver_espressif_qemu
      COMMAND ${_qemu_cmd} -s -S
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      USES_TERMINAL
      COMMENT "Espressif QEMU GDB server on :1234 (${ESPRESSIF_QEMU_MACHINE})"
    )
  else()
    include(${CMAKE_CURRENT_LIST_DIR}/espressif_qemu/missing.cmake)
  endif()
endif()

if(CONFIG_BUILD_OUTPUT_BIN AND NOT TARGET espressif_qemu_flash_image)
  if(NOT logical_target_for_zephyr_elf)
    message(WARNING
      "Espressif QEMU: logical_target_for_zephyr_elf unset; no flash image generated")
  else()
    espressif_qemu_add_image_target()
  endif()
endif()

if(TARGET espressif_qemu_flash_image AND ESPRESSIF_QEMU_EXECUTABLE)
  add_dependencies(run_espressif_qemu espressif_qemu_flash_image)
  add_dependencies(debugserver_espressif_qemu espressif_qemu_flash_image)
endif()
if(TARGET espressif_qemu_efuse_image AND ESPRESSIF_QEMU_EXECUTABLE)
  add_dependencies(run_espressif_qemu espressif_qemu_efuse_image)
  add_dependencies(debugserver_espressif_qemu espressif_qemu_efuse_image)
endif()
