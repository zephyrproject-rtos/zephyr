# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0
#
# BK7258 SoC sources.
#
# zephyr_include_directories(.) is what puts this directory on the include
# path; nothing else does it, the HWMv2 SoC metadata (soc.yml) only
# registers the directory as a configure dependency. Both soc.c and the
# arch layer's cmsis_core_m.h (which includes <soc.h>) need it, and
# without it the build stops with "soc.h: No such file or directory".

zephyr_include_directories(.)
zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_BK7258_IPC_HEARTBEAT soc_ipc.c)

# ---------------------------------------------------------------------------
# Linker script
#
# The generic LINKER_SCRIPT choice looks for
# CONFIG_HAVE_CUSTOM_LINKER_SCRIPT, then BOARD_LINKER_SCRIPT, then
# SOC_LINKER_SCRIPT, and fails configuration when none of them is set.
# The linker template Zephyr ships for the architecture is enough here: the
# devicetree already declares zephyr,flash (the XIP window) and zephyr,sram,
# and the template lays the image out from those two regions.
# ---------------------------------------------------------------------------
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

# ---------------------------------------------------------------------------
# Flash images
#
# Two images end up in flash, and neither of them can be built by linking, as
# both have to be passed through bkimage.py first:
#
#   0x000000   first_stage.S, this directory  (only when Zephyr owns the chip)
#   0x165000   the application, linked at 0x02150000 by the board devicetree
#
# The first stage is plain assembly built with the same toolchain but outside
# the Zephyr build: it runs before the kernel exists and must not depend on it.
# It is built by its own custom command so that the build system sees its
# inputs, and merged into the application image in a post-build step.
# ---------------------------------------------------------------------------
if(CONFIG_BUILD_OUTPUT_BIN)
  set(BK_APP_CRC ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.crc.bin)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bkimage.py crc
            ${KERNEL_BIN_NAME} ${KERNEL_NAME}.crc.bin
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${BK_APP_CRC})

  if(CONFIG_SOC_BK7258_VENDOR_FIRMWARE)
    # Only the application is written; the rest of the flash is owned by the
    # firmware the device ships with.
    set_target_properties(runners_yaml_props_target PROPERTIES bin_file ${BK_APP_CRC})
  else()
    set(BK_FIRST_STAGE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/bk7258_first_stage.o)
    set(BK_FIRST_STAGE_ELF ${CMAKE_CURRENT_BINARY_DIR}/bk7258_first_stage.elf)
    set(BK_FIRST_STAGE_BIN ${CMAKE_CURRENT_BINARY_DIR}/bk7258_first_stage.bin)
    set(BK_FIRST_STAGE_CRC ${CMAKE_CURRENT_BINARY_DIR}/bk7258_first_stage.crc.bin)
    set(BK_FULL_BIN ${PROJECT_BINARY_DIR}/${KERNEL_NAME}_full.bin)

    # Cortex-M33 is the instruction set STAR-MC1 implements; no FPU is used, so
    # the soft-float ABI keeps the stub free of FPU setup.
    set(BK_FIRST_STAGE_FLAGS -mcpu=cortex-m33 -mthumb -mabi=aapcs -mfloat-abi=soft)

    # A custom command with declared inputs, so that touching the stub, its
    # linker script or the image tool rebuilds it. Built through a target the
    # library depends on, which puts it before the link and therefore before the
    # merge below runs.
    add_custom_command(
      OUTPUT ${BK_FIRST_STAGE_CRC}
      COMMAND ${CMAKE_C_COMPILER} ${BK_FIRST_STAGE_FLAGS} -c
              ${CMAKE_CURRENT_SOURCE_DIR}/first_stage.S -o ${BK_FIRST_STAGE_OBJ}
      COMMAND ${CMAKE_C_COMPILER} ${BK_FIRST_STAGE_FLAGS} -nostdlib -nostartfiles
              -T ${CMAKE_CURRENT_SOURCE_DIR}/first_stage.ld -Wl,--build-id=none
              -o ${BK_FIRST_STAGE_ELF} ${BK_FIRST_STAGE_OBJ}
      COMMAND ${CMAKE_OBJCOPY} -O binary ${BK_FIRST_STAGE_ELF} ${BK_FIRST_STAGE_BIN}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bkimage.py crc
              ${BK_FIRST_STAGE_BIN} ${BK_FIRST_STAGE_CRC}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/first_stage.S
              ${CMAKE_CURRENT_SOURCE_DIR}/first_stage.ld
              ${CMAKE_CURRENT_SOURCE_DIR}/bkimage.py
      BYPRODUCTS ${BK_FIRST_STAGE_OBJ} ${BK_FIRST_STAGE_ELF} ${BK_FIRST_STAGE_BIN}
      COMMENT "Building BK7258 first stage"
    )

    add_custom_target(bk7258_first_stage DEPENDS ${BK_FIRST_STAGE_CRC})
    add_dependencies(${ZEPHYR_CURRENT_LIBRARY} bk7258_first_stage)

    # The merge names both images as inputs, so it is redone whenever either of
    # them changes and not only when the application relinks.
    add_custom_command(
      OUTPUT ${BK_FULL_BIN}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bkimage.py merge
              --first-stage ${BK_FIRST_STAGE_CRC} --app ${BK_APP_CRC}
              --out ${BK_FULL_BIN}
      DEPENDS ${BK_FIRST_STAGE_CRC} ${BK_APP_CRC}
      COMMENT "Merging BK7258 flash image"
    )
    add_custom_target(bk7258_full_image ALL DEPENDS ${BK_FULL_BIN})

    # A complete image, so "west flash" programs the chip from nothing.
    set_target_properties(runners_yaml_props_target PROPERTIES bin_file ${BK_FULL_BIN})
  endif()
endif()
