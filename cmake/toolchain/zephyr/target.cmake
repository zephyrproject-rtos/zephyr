# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/${SDK_VERSION}/target.cmake)


# For toolchains without a deterministic 'ar' option, see
# less ideal but tested "strip-nondeterminism" alternative at
#  https://github.com/marc-hb/zephyr/commits/strip-nondeterminism

# TODO: build future versions of the Zephyr SDK with
#      --enable-deterministic-archives?
# https://github.com/zephyrproject-rtos/sdk-ng/issues/81

# Ideally we'd want to _append_ -D to the list of flags and operations
# that cmake already uses by default. However as of version 3.14 cmake
# doesn't support addition, one can only replace the complete AR command.

# Quoting https://gitlab.kitware.com/cmake/cmake/issues/19474
#
#   The rule variables are actually pseudo-public as described here:
#   projects can set them but are responsible for keeping up with
#   changes to CMake.  The main variable(s) documentation could use
#   updates for this.

# To see what the default flags are and find where they're defined
# in cmake's code:
# - comment out the end of the foreach line like this:
#      foreach(lang ) # ASM C CXX
# - build any sample with:
#       2>cmake.log  cmake --trace-expand ...
# - Search cmake.log for:
#      CMAKE_.*_ARCHIVE and CMAKE_.*CREATE_STATIC

# CMake's documentation is at node: CMAKE_LANG_ARCHIVE_CREATE

# Unlike CMAKE_ASM_CREATE_STATIC_LIBRARY (which has another issue, see
# revert 2371679528f8), CMAKE_ASM_ARCHIVE_* do not seem to work, at
# least not with cmake version 3.14.5. Workaround: add one empty.c file
# which turns "Linking ASM static library" into "Linking C static
# library", see example at arch/xtensa/core/startup/empty.c

foreach(lang ASM C CXX)
  SET(CMAKE_${lang}_ARCHIVE_CREATE
      "<CMAKE_AR> qcD <TARGET> <LINK_FLAGS> <OBJECTS>")
  SET(CMAKE_${lang}_ARCHIVE_APPEND # just to be on the safe side
      "<CMAKE_AR> qD  <TARGET> <LINK_FLAGS> <OBJECTS>")
  SET(CMAKE_${lang}_ARCHIVE_FINISH
      "<CMAKE_RANLIB> -D <TARGET>")
endforeach()
