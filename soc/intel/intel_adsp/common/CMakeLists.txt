# Intel CAVS SoC family CMake file
#
# Copyright (c) 2020-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(INTEL_ADSP_COMMON)

zephyr_library_named(intel_adsp_common)
zephyr_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources_ifdef(CONFIG_INTEL_ADSP_IPC ipc.c)

zephyr_library_sources(
    rimage_modules.c
    boot.c
    soc.c
    mem_window.c
    boot_complete.c
    )

zephyr_library_sources_ifdef(CONFIG_ADSP_CLOCK clk.c)

if(CONFIG_SMP OR CONFIG_MP_MAX_NUM_CPUS GREATER 1)
  zephyr_library_sources(multiprocessing.c)
endif()

zephyr_library_link_libraries(INTEL_ADSP_COMMON)

target_include_directories(INTEL_ADSP_COMMON INTERFACE include)
target_link_libraries(INTEL_ADSP_COMMON INTERFACE intel_adsp_common)

set(KERNEL_REMAPPED ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}-remapped.elf)
set(EXTMAN ${CMAKE_BINARY_DIR}/zephyr/extman.bin)

if(${CMAKE_HOST_WIN32})
    set(NULL_FILE nul)
elseif(${CMAKE_HOST_UNIX})
    set(NULL_FILE /dev/null)
endif()

# Generate rimage modules from the base kernel ELF file.  Note the
# warning squashing on the objcopy steps.  Binutils has a misfeature
# where if the copy removes all the sections from an input ELF program
# header (our link generates lots of phdrs because of the disjoint
# cacheability addresses), it will warn about an "empty" segment even
# though it was TOLD to drop the contents!
#
# Also note that rimage is picky with section flags: it will try to
# include a section in the output data (even its own metadata in
# .module!) if it has any of ALLOC, WRITABLE or EXEC flags in the ELF
# file.  The GNU linker will set these automatically based on the
# flags of the sections coming out of C code, so this is fragile and
# breaks easily.  Set noload flags explicitly here.
add_custom_target(
  gen_modules ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/boot.mod ${CMAKE_BINARY_DIR}/zephyr/main.mod
)

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/zephyr/boot.mod ${CMAKE_BINARY_DIR}/zephyr/main.mod
  COMMENT "Extracting .mod(ule) files for rimage"
  DEPENDS ${ZEPHYR_FINAL_EXECUTABLE} ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}

  # The .fw_metadata section may not be present (xcc's older linker
  # will remove it if empty).  Extract it here (which will create an
  # empty file if not present) and add it back when we generate the
  # main.mod file below.
  COMMAND ${CMAKE_OBJCOPY} -O binary --only-section=.fw_metadata
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf ${EXTMAN}

  # Remap uncached section addresses so they appear contiguous
  COMMAND ${CMAKE_COMMAND} -E
      copy ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf ${KERNEL_REMAPPED}

  # Extract modules for rimage
  COMMAND ${CMAKE_OBJCOPY}
      --only-section .imr
      --only-section .imrdata
      --only-section .module.boot
      --set-section-flags .module.boot=noload,readonly
      --rename-section .module.boot=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/boot.mod 2>${NULL_FILE}

  # Remove .fw_metadata here...
  COMMAND ${CMAKE_OBJCOPY}
      --remove-section .imr
      --remove-section .imrdata
      --remove-section .module.boot
      --remove-section .fw_metadata
      --set-section-flags .module.main=noload,readonly
      --set-section-flags .static_uuid_entries=noload,readonly
      --set-section-flags .static_log_entries=noload,readonly
      --rename-section .module.main=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/main.mod 2>${NULL_FILE}

  # ...and copy it back in
  COMMAND ${CMAKE_OBJCOPY}
      --add-section .fw_metadata=${EXTMAN}
      --set-section-flags .fw_metadata=noload,readonly
      ${CMAKE_BINARY_DIR}/zephyr/main.mod
      ${CMAKE_BINARY_DIR}/zephyr/main.mod 2>${NULL_FILE}
)

if(CONFIG_BUILD_OUTPUT_STRIPPED)
add_custom_command(
  COMMENT "strip main.mod"
  APPEND OUTPUT ${CMAKE_BINARY_DIR}/zephyr/main.mod
    COMMAND $<TARGET_PROPERTY:bintools,strip_command>
            $<TARGET_PROPERTY:bintools,strip_flag>
            $<TARGET_PROPERTY:bintools,strip_flag_all>
            $<TARGET_PROPERTY:bintools,strip_flag_infile>${CMAKE_BINARY_DIR}/zephyr/main.mod
            $<TARGET_PROPERTY:bintools,strip_flag_outfile>${CMAKE_BINARY_DIR}/zephyr/main-stripped.mod
            $<TARGET_PROPERTY:bintools,strip_flag_final>
)
endif()


#  west sign
add_custom_target(zephyr.ri ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
)

# If some of your board(s) need to override default rimage parameters
# then you can define WEST_SIGN_OPTS in  boards/my/board/board.cmake.
# Example:
#
#   set(WEST_SIGN_OPTS -- -c "/home/sweet  home/rimage/config/abc.toml" -i 4)

# Parameters after the double dash -- are passed through to rimage.  For
# other ways to override default rimage parameters check
# boards/intel/adsp/doc/intel_adsp_generic.rst

# Warning: because `west sign` can also be used interactively, using
# ${WEST_SIGN_OPTS} like this has _higher_ precedence than `west config
# rimage.extra-args`! Avoid overriding default rimage parameters in
# multiple places to avoid unexpected precedence rules.
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  COMMENT "west sign --if-tool-available --tool rimage ..."
  # Use --if-tool-available so we don't force every CI to install
  # rimage. We don't want to break build-only and other tests that don't
  # require signing. When rimage is missing, `west flash` fails with a
  # clear "zephyr.ri missing" error with an "rimage not found" warning
  # from west sign immediately before it.
  COMMAND  west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR} ${WEST_SIGN_OPTS}
  DEPENDS gen_modules
          ${CMAKE_BINARY_DIR}/zephyr/boot.mod ${CMAKE_BINARY_DIR}/zephyr/main.mod
)
