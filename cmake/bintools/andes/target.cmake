# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as GNU binutils for the Andes toolchains

include(${ZEPHYR_BASE}/cmake/bintools/gnu/target.cmake)

if(CONFIG_LLVM_USE_LLD)
  # LLD keeps empty output sections placed with AT> (e.g. nocache_load,
  # .ramfunc) loadable with an LMA in RAM. GNU objcopy --gap-fill then fills
  # the binary from that LMA up to ROM, producing an incorrect image.
  set_property(TARGET bintools PROPERTY elfconvert_flag_gapfill "")
endif()
