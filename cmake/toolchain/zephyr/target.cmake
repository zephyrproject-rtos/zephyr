# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/${SDK_VERSION}/target.cmake)


# For toolchains without a deterministic 'ar' option, see
# less ideal but tested "strip-nondeterminism" alternative at
#  https://github.com/marc-hb/zephyr/commits/strip-nondeterminism

# TODO: build future versions of the Zephyr SDK with
#      --enable-deterministic-archives?

# Ideally we'd want to _append_ -D to the list of flags and operations
# that cmake already uses by default. However as of version 3.14 cmake
# doesn't support addition, one can only replace the complete AR command.

# To see what the default flags are and find where they're defined
# in cmake's code:
# - comment out the end of the foreach line like this:
#      foreach(lang ) # ASM C CXX
# - build any sample with:
#       2>cmake.log  cmake --trace-expand ...
# - Search cmake.log for:
#      CMAKE_.*_ARCHIVE and CMAKE_.*CREATE_STATIC

# CMake's documentation is at node: CMAKE_LANG_CREATE_STATIC_LIBRARY

# At least one .a file needs 'ASM': arch/xtensa/core/startup/ because
# there's no C file there.
foreach(lang ASM C CXX)
  # GNU ar always updates the index: no need for CMAKE_RANLIB
  SET(CMAKE_${lang}_CREATE_STATIC_LIBRARY
      "<CMAKE_AR> qcD <TARGET> <LINK_FLAGS> <OBJECTS>")
endforeach()
