# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-mlongcalls)

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  fatal.c
  window_vectors.S
  xtensa-asm2-util.S
  xtensa-asm2.c
  irq_manage.c
  )

zephyr_library_sources_ifdef(CONFIG_XTENSA_USE_CORE_CRT1 crt1.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE xtensa_backtrace.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE debug_helpers_asm.S)
zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)

if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "xcc")
  zephyr_library_sources(xcc_stubs.c)
endif()

zephyr_library_include_directories(include)

add_subdirectory(startup)

# This produces a preprocessed and regenerated (in the sense of gcc
# -dM, supported by all Xtensa toolchains) core-isa.h file available
# as "core-isa-dM.h".  This can be easily parsed by non-C tooling.
#
# Note that this adds the SOC/HAL include directory explicitly, they
# are the official places where we find core-isa.h. (Also that we
# undefine __XCC_ because that compiler actually trips an error trying
# to build this file to protect against mismatched versions.)
set(CORE_ISA_DM ${CMAKE_BINARY_DIR}/zephyr/include/generated/core-isa-dM.h)
set(CORE_ISA_IN ${CMAKE_BINARY_DIR}/zephyr/include/generated/core-isa-dM.c)
file(WRITE ${CORE_ISA_IN} "#include <xtensa/config/core-isa.h>\n")
add_custom_command(OUTPUT ${CORE_ISA_DM}
  COMMAND ${CMAKE_C_COMPILER} -E -dM -U__XCC__
          -I${ZEPHYR_XTENSA_MODULE_DIR}/zephyr/soc/${CONFIG_SOC}
          -I${SOC_DIR}/${ARCH}/${SOC_PATH}
          ${CORE_ISA_IN} -o ${CORE_ISA_DM})

# Generates a list of device-specific scratch register choices
set(ZSR_H ${CMAKE_BINARY_DIR}/zephyr/include/generated/zsr.h)
add_custom_command(OUTPUT ${ZSR_H} DEPENDS ${CORE_ISA_DM}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_zsr.py
                               ${CORE_ISA_DM} ${ZSR_H})
add_custom_target(zsr_h DEPENDS ${ZSR_H})
add_dependencies(zephyr_interface zsr_h)
