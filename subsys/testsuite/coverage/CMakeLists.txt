# SPDX-License-Identifier: Apache-2.0

if(CONFIG_COVERAGE_GCOV)
  # clang --coverage calls the llvm_gcda_*() runtime instead of __gcov_init()
  if(CONFIG_COVERAGE_GCOV_LLVM)
    zephyr_sources(coverage_llvm.c)
  else()
    zephyr_sources(coverage.c)
  endif()
endif()

zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RAM_SECTIONS coverage_ram.ld)
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RODATA coverage_rodata.ld)
