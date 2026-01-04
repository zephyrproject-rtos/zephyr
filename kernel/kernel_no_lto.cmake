# Copyright (c) 2025 ITE Corporation. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

# Optional script to disable LTO for kernel files

# Retrieve all source files from the kernel library target
if(TARGET kernel)
  get_property(KERNEL_SRCS TARGET kernel PROPERTY SOURCES)
else()
  message(WARNING "[no-LTO] kernel target not found, skipping")
  return()
endif()

# Check if there is a permitted kernel file that can enable LTO
if(CONFIG_KERNEL_LTO_ALLOWLIST)
  set(ADDITIONAL_ALLOWLIST ${CONFIG_KERNEL_LTO_ALLOWLIST})
  separate_arguments(ADDITIONAL_ALLOWLIST)
endif()

# Default LTO allowlist
set(DEFAULT_LTO_ALLOWLIST init.c errno.c fatal.c)
# Append LTO allowlist
list(APPEND DEFAULT_LTO_ALLOWLIST ${ADDITIONAL_ALLOWLIST})
# Setting LTO allowlist
set(LTO_ALLOWLIST ${DEFAULT_LTO_ALLOWLIST})

# Apply -fno-lto to all C source files, except for some initialization files
# (e.g. init.c, errno.c, fatal.c) and those that are less critical to be
# executed in RAM. These files can be excluded from -fno-lto by using
# ADDITIONAL_ALLOWLIST.
foreach(src ${KERNEL_SRCS})
  if(src MATCHES "\\.c$")

    # Skip if filename matches any in allowlist
    set(skip FALSE)
    foreach(allow ${LTO_ALLOWLIST})
      get_filename_component(basename ${src} NAME)
      if("${basename}" STREQUAL "${allow}")
        set(skip TRUE)
        break()
      endif()
    endforeach()

    if(NOT skip)
      set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-fno-lto -g")
    endif()

  endif()
endforeach()
