# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build the QEMU hardware device tree ("hw-dtb") for the Xilinx arm-generic-fdt
# machine from the optional Xilinx/qemu-devicetrees west module, instead of
# vendoring the large, non-DTS-spec QEMU device trees in the Zephyr tree. The
# resulting DTB is passed to QEMU via -hw-dtb (see each board's board.cmake) and
# is never consumed by Zephyr's own devicetree build.
#
# A board includes this file from its CMakeLists.txt after setting:
#   QEMU_DT_NAME            - basename of the board ".dts" in the module (the
#                             standalone single-architecture model; the co-sim
#                             "*-cosim-*" variants need a second QEMU).
#   QEMU_CONSOLE_SERIAL_REG - optional; register base of the UART used as the
#                             Zephyr console, e.g. 0xf1920000. When set, that
#                             UART's chardev is rewired to "con" so QEMU connects
#                             it to stdio under -nographic. Leave unset for models
#                             whose UARTs carry no chardev property (e.g. the
#                             ZynqMP Cadence UART, which QEMU binds to -serial by
#                             index); the DTB is then used verbatim.
# The built DTB is written to ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb.

if(NOT DEFINED QEMU_DEVICETREES_DIR)
  set(QEMU_DEVICETREES_DIR "${WEST_TOPDIR}/modules/hal/qemu_devicetrees")
endif()

if(NOT EXISTS "${QEMU_DEVICETREES_DIR}/Makefile")
  return()
endif()

if(NOT DEFINED QEMU_DT_NAME)
  message(FATAL_ERROR "qemu_hw_dtb.cmake: QEMU_DT_NAME must be set")
endif()

# Build the device tree with the C preprocessor and dtc directly, rather than
# invoking the qemu-devicetrees Makefile, so the build does not depend on "make"
# and works under any CMake generator (e.g. Ninja). The preprocessor flags and
# the generated NPI fragments mirror that Makefile.
find_program(QEMU_DT_CPP NAMES gcc cc clang REQUIRED)
find_program(DTC_EXECUTABLE NAMES dtc REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(qemu_dt_outdir "${PROJECT_BINARY_DIR}/qemu_dt")
set(qemu_dt_gen "${qemu_dt_outdir}/gen")
set(qemu_dt_pre "${qemu_dt_outdir}/${QEMU_DT_NAME}.dts.pre")
set(qemu_dt_built "${qemu_dt_outdir}/${QEMU_DT_NAME}.dtb")
file(MAKE_DIRECTORY ${qemu_dt_outdir})

# The Versal PMC model includes NPI fragments that the upstream project
# generates (and git-ignores); reproduce them so the preprocessor can find them.
execute_process(
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/qemu_dt_npi_gen.py
          --outdir ${qemu_dt_gen}
  RESULT_VARIABLE qemu_dt_gen_result
)
if(NOT qemu_dt_gen_result EQUAL 0)
  message(FATAL_ERROR "Failed to generate QEMU NPI device-tree fragments")
endif()

execute_process(
  COMMAND ${QEMU_DT_CPP} -E -nostdinc
          -I${qemu_dt_gen}
          -I.
          -Iinclude/
          -Iinclude/versal-net
          -Iinclude/versal2/2ve/memmap
          -Iinclude/spartan-ultra-plus/memmap/
          -x assembler-with-cpp -o ${qemu_dt_pre} ${QEMU_DT_NAME}.dts
  WORKING_DIRECTORY ${QEMU_DEVICETREES_DIR}
  RESULT_VARIABLE qemu_dt_cpp_result
)
if(NOT qemu_dt_cpp_result EQUAL 0)
  message(FATAL_ERROR
    "Failed to preprocess QEMU device tree ${QEMU_DT_NAME} from ${QEMU_DEVICETREES_DIR}")
endif()

execute_process(
  COMMAND ${DTC_EXECUTABLE} -q -O dtb -@ -I dts -b 0 -o ${qemu_dt_built} ${qemu_dt_pre}
  RESULT_VARIABLE qemu_dt_result
)
if(NOT qemu_dt_result EQUAL 0)
  message(FATAL_ERROR "Failed to compile QEMU device tree ${QEMU_DT_NAME}")
endif()

# Models whose UARTs carry no chardev property are used verbatim.
if(NOT DEFINED QEMU_CONSOLE_SERIAL_REG)
  file(COPY_FILE ${qemu_dt_built} ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb)
  return()
endif()

# Rewire the console UART's chardev to "con" via a dtc round-trip.
set(qemu_dt_dts_in "${qemu_dt_outdir}/${BOARD}-qemu.in.dts")
set(qemu_dt_dts_out "${qemu_dt_outdir}/${BOARD}-qemu.dts")
execute_process(
  COMMAND ${DTC_EXECUTABLE} -q -I dtb -O dts -o ${qemu_dt_dts_in} ${qemu_dt_built}
  RESULT_VARIABLE qemu_dt_decompile_result
)
if(NOT qemu_dt_decompile_result EQUAL 0)
  message(FATAL_ERROR "Failed to decompile ${qemu_dt_built}")
endif()
execute_process(
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_LIST_DIR}/qemu_console_chardev.py
          --input ${qemu_dt_dts_in}
          --output ${qemu_dt_dts_out}
          --reg ${QEMU_CONSOLE_SERIAL_REG}
  RESULT_VARIABLE qemu_dt_patch_result
)
if(NOT qemu_dt_patch_result EQUAL 0)
  message(FATAL_ERROR "Failed to patch console chardev in ${qemu_dt_dts_in}")
endif()
execute_process(
  COMMAND ${DTC_EXECUTABLE} -q -I dts -O dtb
          -o ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb ${qemu_dt_dts_out}
  RESULT_VARIABLE qemu_dt_recompile_result
)
if(NOT qemu_dt_recompile_result EQUAL 0)
  message(FATAL_ERROR "Failed to recompile ${qemu_dt_dts_out}")
endif()
