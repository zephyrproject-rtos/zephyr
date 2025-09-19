# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-mlongcalls)

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  fatal.c
  window_vectors.S
  xtensa_asm2_util.S
  irq_manage.c
  thread.c
  vector_handlers.c
  prep_c.c
  )

zephyr_library_sources_ifdef(CONFIG_XTENSA_USE_CORE_CRT1 crt1.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE xtensa_backtrace.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE debug_helpers_asm.S)
zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_MMU ptables.c mmu.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_MPU mpu.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S syscall_helper.c)
zephyr_library_sources_ifdef(CONFIG_LLEXT elf.c)
zephyr_library_sources_ifdef(CONFIG_SMP smp.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_HIFI_SHARING xtensa_hifi.S)

zephyr_library_sources_ifdef(
  CONFIG_KERNEL_VM_USE_CUSTOM_MEM_RANGE_CHECK
  mem_manage.c
)

if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "xcc")
  zephyr_library_sources(xcc_stubs.c)
endif()

# ...where to find core-isa.h for custom compilation commands below.
if(CONFIG_SOC_FAMILY_ESPRESSIF_ESP32)
  set(XTENSA_CONFIG_HAL_INCLUDE_DIR
      -I${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/xtensa/${CONFIG_SOC}/include
     )
else()
  set(XTENSA_CONFIG_HAL_INCLUDE_DIR
      -I${ZEPHYR_XTENSA_MODULE_DIR}/zephyr/soc/${CONFIG_SOC}
     )
endif()

add_subdirectory(startup)

# This produces a preprocessed and regenerated (in the sense of gcc
# -dM, supported by all Xtensa toolchains) core-isa.h file available
# as "core-isa-dM.h".  This can be easily parsed by non-C tooling.
#
# Note that this adds the SOC/HAL include directory explicitly, they
# are the official places where we find core-isa.h. (Also that we
# undefine __XCC_ because that compiler actually trips an error trying
# to build this file to protect against mismatched versions.)
set(CORE_ISA_DM ${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/core-isa-dM.h)
set(CORE_ISA_IN ${CMAKE_BINARY_DIR}/zephyr/include/generated/core-isa-dM.c)
file(WRITE ${CORE_ISA_IN} "#include <xtensa/config/core-isa.h>\n")
add_custom_command(OUTPUT ${CORE_ISA_DM}
  COMMAND ${CMAKE_C_COMPILER} -E -dM -U__XCC__ ${XTENSA_CORE_LOCAL_C_FLAG}
          ${XTENSA_CONFIG_HAL_INCLUDE_DIR}
          -I${SOC_FULL_DIR}
          ${CORE_ISA_IN} -o ${CORE_ISA_DM})

if(CONFIG_USERSPACE AND NOT CONFIG_THREAD_LOCAL_STORAGE)
  # It is possible that the SoC does not have THREADPTR.
  # This means that we cannot use THREADPTR as a shortcut to
  # in arch_is_user_context(). However, whether a SoC has
  # THREADPTR is in core-isa.h which can be parsed in gen_zsr.py.
  # There, if there is no THREADPTR, we need a scratch register
  # so we can do arch_is_user_context() via syscall.
  set(MAY_NEED_SYSCALL_SCRATCH_REG true)
else()
  # With thread local storage, the variable is_user_mode is
  # stored in the thread's TLS area. There is no need for
  # scratch register.
  set(MAY_NEED_SYSCALL_SCRATCH_REG false)
endif()

if(CONFIG_KERNEL_COHERENCE AND NOT CONFIG_SCHED_CPU_MASK_PIN_ONLY)
  set(NEED_FLUSH_SCRATCH_REG true)
else()
  set(NEED_FLUSH_SCRATCH_REG false)
endif()

if(CONFIG_SEMIHOST)
  zephyr_library_include_directories(${ZEPHYR_BASE}/arch/common/include)
endif()

# Generates a list of device-specific scratch register choices
set(ZSR_H ${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/zsr.h)
add_custom_command(OUTPUT ${ZSR_H} DEPENDS ${CORE_ISA_DM}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_zsr.py
                               $<$<BOOL:${CONFIG_XTENSA_MMU}>:--mmu>
                               $<$<BOOL:${MAY_NEED_SYSCALL_SCRATCH_REG}>:--syscall-scratch>
                               $<$<BOOL:${NEED_FLUSH_SCRATCH_REG}>:--flush-reg>
                               ${CORE_ISA_DM} ${ZSR_H})
add_custom_target(zsr_h DEPENDS ${ZSR_H})
add_dependencies(zephyr_interface zsr_h)

unset(MAY_NEED_SYSCALL_SCRATCH_REG)

# Auto-generate interrupt vector entry
set(VECS_LD ${CMAKE_BINARY_DIR}/zephyr/include/generated/xtensa_vectors.ld)
add_custom_command(OUTPUT ${VECS_LD} DEPENDS ${CORE_ISA_DM}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_vectors.py
          ${CORE_ISA_DM} > ${VECS_LD})
add_custom_target(xtensa_vectors_ld DEPENDS ${VECS_LD})
add_dependencies(zephyr_interface xtensa_vectors_ld)
