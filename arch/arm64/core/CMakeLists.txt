# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.S
  early_mem_funcs.S
  fatal.c
  irq_init.c
  irq_manage.c
  prep_c.c
  reset.S
  reset.c
  switch.S
  thread.c
  vector_table.S
)

# Use large code model for addresses larger than 32 bits,
# This is 10 characters long (0x12345678). We can't use a
# simple numeric comparison because these values may be
# beyond the numeric range of integers for cmake.

string(LENGTH "x${CONFIG_SRAM_BASE_ADDRESS}" SRAM_LENGTH)
string(LENGTH "x${CONFIG_KERNEL_VM_BASE}" KERNEL_VM_LENGTH)

if(${SRAM_LENGTH} GREATER 11 OR ${KERNEL_VM_LENGTH} GREATER 11)
  zephyr_cc_option(-mcmodel=large)
endif()

zephyr_library_sources_ifdef(CONFIG_FPU_SHARING fpu.c fpu.S)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu.c mmu.S)
zephyr_library_sources_ifdef(CONFIG_ARM_MPU cortex_r/arm_mpu.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_HAS_ARM_SMCCC smccc-call.S)
zephyr_library_sources_ifdef(CONFIG_AARCH64_IMAGE_HEADER header.S)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
if ((CONFIG_MP_MAX_NUM_CPUS GREATER 1) OR (CONFIG_SMP))
  zephyr_library_sources(smp.c)
endif ()

zephyr_cc_option_ifdef(CONFIG_USERSPACE -mno-outline-atomics)
zephyr_cc_option_ifdef(CONFIG_FRAME_POINTER -mno-omit-leaf-frame-pointer)

# GCC may generate ldp/stp instructions with the Advanced SIMD Qn registers for
# consecutive 32-byte loads and stores. Saving and restoring the Advanced SIMD
# context is very expensive, and it is preferable to keep it turned off by not
# emitting these instructions for better context switching performance.

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_cc_option(-moverride=tune=no_ldp_stp_qregs)
endif()

add_subdirectory_ifdef(CONFIG_XEN xen)

if(CONFIG_GEN_SW_ISR_TABLE)
  if(CONFIG_DYNAMIC_INTERRUPTS)
    zephyr_linker_sources(RWDATA swi_tables.ld)
  else()
    zephyr_linker_sources(RODATA swi_tables.ld)
  endif()
endif()
