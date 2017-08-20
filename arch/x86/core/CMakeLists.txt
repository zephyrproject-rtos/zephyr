if (COMPILER STREQUAL "clang")
  # We rely on GAS for assembling, so don't use the integrated assembler
  target_compile_options_ifndef(CONFIG_X86_IAMCU zephyr PUBLIC $<$<COMPILE_LANGUAGE:ASM>:-no-integrated-as>)
endif()

target_compile_options(zephyr PUBLIC $<$<COMPILE_LANGUAGE:ASM>:-Wa,--divide>)

add_subdirectory_ifdef(CONFIG_DEBUG_INFO debug)

target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/cache.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cache_s.S
  ${CMAKE_CURRENT_SOURCE_DIR}/cpuhalt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crt0.S
  ${CMAKE_CURRENT_SOURCE_DIR}/excstub.S
  ${CMAKE_CURRENT_SOURCE_DIR}/intstub.S
  ${CMAKE_CURRENT_SOURCE_DIR}/irq_manage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/msr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/swap.S
  ${CMAKE_CURRENT_SOURCE_DIR}/sys_fatal_error_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thread.c
  )

target_sources_if_kconfig(             zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/irq_offload.c)
target_sources_if_kconfig(             zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/x86_mmu.c)
target_sources_if_kconfig(             zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/reboot_rst_cnt.c)
target_sources_ifdef(CONFIG_FP_SHARING zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/float.c)

# Last since we declare default exception handlers here
target_sources(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fatal.c)
