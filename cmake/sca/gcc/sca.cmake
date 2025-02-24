# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Intel Corporation
# Copyright (c) 2025 Alex Fabre

# Check GCC version
# GCC static analyzer requires GCC version >= 10.0.0
if("${GCC_COMPILER_VERSION}" VERSION_LESS 10.0.0)
  message(FATAL_ERROR "GCC static analyzer requires GCC >= v10.0")
endif()


# Enable GCC static analyzer
list(APPEND TOOLCHAIN_C_FLAGS -fanalyzer)

# Add GCC analyzer user options
zephyr_get(GCC_SCA_OPTS)
zephyr_get(USE_CCACHE)

if(DEFINED GCC_SCA_OPTS)
  foreach(analyzer_option IN LISTS GCC_SCA_OPTS)

    if(analyzer_option STREQUAL "-fdiagnostics-format=sarif-file" OR analyzer_option STREQUAL "-fdiagnostics-format=json-file")
      # Beginning with GCC 14, users can use the -fdiagnostics-format option
      # to output analyzer diagnostics as SARIF or JSON files.
      # This option encounters a specific issue[1] when used with ccache.
      # Therefore, currently, the build process is halted if 'ccache' is enabled.
      # [1] https://github.com/ccache/ccache/issues/1466
      if(NOT USE_CCACHE STREQUAL "0")
        message(FATAL_ERROR "GCC SCA requires 'ccache' to be disabled for output file generation. Disable 'ccache' by setting USE_CCACHE=0.")
      endif()
    endif()

    list(APPEND TOOLCHAIN_C_FLAGS ${analyzer_option})
  endforeach()
endif()
