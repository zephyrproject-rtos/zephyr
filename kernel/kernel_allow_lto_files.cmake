# Copyright (c) 2026 ITE Corporation. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

# Check if there is a permitted kernel file that can enable LTO
if(CONFIG_KERNEL_LTO_ALLOWLIST)
  set(KERNEL_LTO_ALLOWLIST ${CONFIG_KERNEL_LTO_ALLOWLIST})
  separate_arguments(KERNEL_LTO_ALLOWLIST)
endif()

# Default kernel source files that are allowed to use LTO
# (e.g. init.c, errno.c, fatal.c) and those that are less critical to be
# executed in RAM
set(DEFAULT_KERNEL_LTO_ALLOWLIST init.c errno.c fatal.c)
# Append LTO allowlist
list(APPEND DEFAULT_KERNEL_LTO_ALLOWLIST ${KERNEL_LTO_ALLOWLIST})

# Retrieve all source files from the kernel library
get_property(KERNEL_SRCS TARGET kernel PROPERTY SOURCES)

# Re-enable LTO only for allowlisted kernel source files
# All other kernel files remain non-LTO by default
foreach(src ${KERNEL_SRCS})
  get_filename_component(basename ${src} NAME)

  if(basename IN_LIST DEFAULT_KERNEL_LTO_ALLOWLIST)
    set_source_files_properties(
      ${src}
      PROPERTIES COMPILE_FLAGS "-flto"
    )
  endif()
endforeach()
