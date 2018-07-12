
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  # We rely on GAS for assembling, so don't use the integrated assembler
  zephyr_compile_options_ifndef(CONFIG_X86_IAMCU $<$<COMPILE_LANGUAGE:ASM>:-no-integrated-as>)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-Wa,--divide>)
endif()

# Last since we declare default exception handlers here
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            cache.c
            cache_s.S
            cpuhalt.c
            crt0.S
            excstub.S
            intstub.S
            irq_manage.c
            swap.S
            sys_fatal_error_handler.c
            thread.c
            IFDEF:${CONFIG_DISABLE_SSBD} spec_ctrl.c
            IFDEF:${CONFIG_FP_SHARING} float.c
            IFDEF:${CONFIG_X86_USERSPACE} userspace.S
            IF_KCONFIG irq_offload.c
            IF_KCONFIG x86_mmu.c
            IF_KCONFIG reboot_rst_cnt.c
)

target_sources(arch_x86 PRIVATE ${PRIVATE_SOURCES})

# Last since we declare default exception handlers here
target_sources(arch_x86 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/fatal.c)

