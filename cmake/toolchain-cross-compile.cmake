# CROSS_COMPILE is a KBuild mechanism for specifying an external
# toolchain with a single environment variable.
#
# It is a legacy mechanism that will in Zephyr translate to
# specififying ZEPHYR_TOOLCHAIN_VARIANT to 'cross-compile' with the location
# 'CROSS_COMPILE'.
#
# New users should set the env var 'ZEPHYR_TOOLCHAIN_VARIANT' to
# 'cross-compile' and the 'CROSS_COMPILE' env var to the toolchain
# prefix. This interface is consisent with the other non-"Zephyr SDK"
# toolchains.
#
# It can be set from three sources, the KConfig option
# CONFIG_CROSS_COMPILE, the CMake var CROSS_COMPILE, or the env var
# CROSS_COMPILE.
#
# The env var has the lowest precedence.
#
# It is not clear what should have the highest precedence of a Kconfig
# option and a CMake var so an error is triggered if they are both
# defined, but are defined with different values.

if(CONFIG_CROSS_COMPILE AND CROSS_COMPILE)
  if(NOT (${CONFIG_CROSS_COMPILE} STREQUAL ${CROSS_COMPILE}))
    message(FATAL_ERROR "The Kconfig and CMake var's must match when defined simultaneously:
CONFIG_CROSS_COMPILE: ${CONFIG_CROSS_COMPILE}
CROSS_COMPILE: ${CROSS_COMPILE}")
  endif()
endif()

if(CONFIG_CROSS_COMPILE)
  set(CROSS_COMPILE ${CONFIG_CROSS_COMPILE})
elseif(DEFINED ENV{CROSS_COMPILE})
  set(CROSS_COMPILE $ENV{CROSS_COMPILE})
endif()

set(   CROSS_COMPILE ${CROSS_COMPILE} CACHE PATH "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

set(COMPILER gcc)
