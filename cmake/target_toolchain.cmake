set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

set(BUILD_SHARED_LIBS OFF)

if(NOT (COMPILER STREQUAL "host-gcc"))
  include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}/target.cmake)
endif()

# The 'generic' compiler and the 'target' compiler might be different,
# so we unset the 'generic' one and thereby force the 'target' to
# re-set it.
unset(CMAKE_C_COMPILER)
unset(CMAKE_C_COMPILER CACHE)

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${ZEPHYR_BASE}/cmake/compiler/${COMPILER}/target.cmake OPTIONAL)

# Uniquely identify the toolchain wrt. it's capabilities.
#
# What we are looking for, is a signature definition that is defined
# like this:

# Toolchains with the same signature will always support the same set
# of flags.

# It is not clear how this signature should be constructed. The
# strategy chosen is to md5sum the CC binary.

file(MD5 ${CMAKE_C_COMPILER} CMAKE_C_COMPILER_MD5_SUM)
set(TOOLCHAIN_SIGNATURE ${CMAKE_C_COMPILER_MD5_SUM})
