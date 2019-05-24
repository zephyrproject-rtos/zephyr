# SPDX-License-Identifier: Apache-2.0

#.rst:
# version.cmake
# -------------
#
# Inputs:
#
#   ``*VERSION*`` and other constants set by
#   maintainers in ``${ZEPHYR_BASE}/VERSION``
#
# Outputs with examples::
#
#   PROJECT_VERSION           1.14.99.07
#   KERNEL_VERSION_STRING    "1.14.99-extraver"
#
#   KERNEL_VERSION_MAJOR       1
#   KERNEL_VERSION_MINOR        14
#   KERNEL_PATCHLEVEL             99
#   KERNELVERSION            0x10E6307
#   KERNEL_VERSION_NUMBER    0x10E63
#   ZEPHYR_VERSION_CODE        69219
#
# Most outputs are converted to C macros, see ``version.h.in``
#
# See also: independent and more dynamic ``BUILD_VERSION`` in
# ``git.cmake``.


include(${ZEPHYR_BASE}/cmake/hex.cmake)
file(READ ${ZEPHYR_BASE}/VERSION ver)

string(REGEX MATCH "VERSION_MAJOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_1})

string(REGEX MATCH "PATCHLEVEL = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_TWEAK = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_TWEAK ${CMAKE_MATCH_1})

string(REGEX MATCH "EXTRAVERSION = ([a-z0-9]*)" _ ${ver})
set(PROJECT_VERSION_EXTRA ${CMAKE_MATCH_1})

# Temporary convenience variable
set(PROJECT_VERSION_WITHOUT_TWEAK ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

if(PROJECT_VERSION_TWEAK)
  set(PROJECT_VERSION ${PROJECT_VERSION_WITHOUT_TWEAK}.${PROJECT_VERSION_TWEAK})
else()
  set(PROJECT_VERSION ${PROJECT_VERSION_WITHOUT_TWEAK})
endif()

message("Zephyr version: ${PROJECT_VERSION}")

set(MAJOR ${PROJECT_VERSION_MAJOR}) # Temporary convenience variable
set(MINOR ${PROJECT_VERSION_MINOR}) # Temporary convenience variable
set(PATCH ${PROJECT_VERSION_PATCH}) # Temporary convenience variable

math(EXPR KERNEL_VERSION_NUMBER_INT "(${MAJOR} << 16) + (${MINOR} << 8)  + (${PATCH})")
math(EXPR KERNELVERSION_INT         "(${MAJOR} << 24) + (${MINOR} << 16) + (${PATCH} << 8) + (${PROJECT_VERSION_TWEAK})")

to_hex(${KERNEL_VERSION_NUMBER_INT} KERNEL_VERSION_NUMBER)
to_hex(${KERNELVERSION_INT}         KERNELVERSION)

set(KERNEL_VERSION_MAJOR      ${PROJECT_VERSION_MAJOR})
set(KERNEL_VERSION_MINOR      ${PROJECT_VERSION_MINOR})
set(KERNEL_PATCHLEVEL         ${PROJECT_VERSION_PATCH})

if(PROJECT_VERSION_EXTRA)
  set(KERNEL_VERSION_STRING     "\"${PROJECT_VERSION_WITHOUT_TWEAK}-${PROJECT_VERSION_EXTRA}\"")
else()
  set(KERNEL_VERSION_STRING     "\"${PROJECT_VERSION_WITHOUT_TWEAK}\"")
endif()

set(ZEPHYR_VERSION_CODE ${KERNEL_VERSION_NUMBER_INT})

# Cleanup convenience variables
unset(MAJOR)
unset(MINOR)
unset(PATCH)
unset(PROJECT_VERSION_WITHOUT_TWEAK)
