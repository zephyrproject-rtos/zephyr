# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

add_subdirectory(core)

# Add hexagon-specific include directory
zephyr_include_directories(include)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littlehexagon)

# Hexagon clang does not support -mno-global-merge; clear the property
# so CONFIG_USERSPACE builds do not inject an unsupported flag.
set_compiler_property(PROPERTY no_global_merge "")

# Prevent the clang driver from injecting picolibc's CRT startup files
# (crt0-semihost.o) and default linker script (picolibc.ld).  Zephyr provides
# its own entry point (__start in hvm_event_vectors.S) and linker script.
#
# --no-default-config is needed for BOTH compile and link.  Without it, clang
# auto-discovers hexagon-unknown-none-elf.cfg which injects -nostdlibinc and
# -isystem <picolibc> before the command-line flags.  This causes picolibc
# headers to be searched before Zephyr's minimal libc headers, breaking
# CONFIG_MINIMAL_LIBC builds (ZRESTRICT undefined, conflicting types).
zephyr_compile_options(--no-default-config)
zephyr_link_libraries(
  -nostartfiles
  --no-default-config
)

# With the default config suppressed, we must supply -nostdlibinc and the
# picolibc include path explicitly so the toolchain's C library headers are
# found.  See https://github.com/llvm/llvm-project/pull/188824 for the
# upstream fix (clang >= 23 no longer leaks host /usr/include).
if(DEFINED LLVM_TOOLCHAIN_PATH AND TOOLCHAIN_HAS_PICOLIBC)
  set(_hex_picolibc_inc
    "${LLVM_TOOLCHAIN_PATH}/target/picolibc/hexagon-unknown-none-elf/include"
  )
  if(EXISTS "${_hex_picolibc_inc}")
    zephyr_compile_options(-nostdlibinc)
    zephyr_compile_options(-isystem ${_hex_picolibc_inc})
  endif()
endif()

# With --no-default-config, the clang driver no longer injects the picolibc
# library path.  Add it explicitly so -lc resolves to the baremetal picolibc.
if(DEFINED LLVM_TOOLCHAIN_PATH)
  file(GLOB _hex_picolibc_dir
    "${LLVM_TOOLCHAIN_PATH}/target/picolibc/hexagon-unknown-none-elf/lib/*/G0"
  )
  if(_hex_picolibc_dir)
    list(GET _hex_picolibc_dir 0 _hex_picolibc_dir)
    zephyr_link_libraries(-L${_hex_picolibc_dir})
  endif()
endif()
