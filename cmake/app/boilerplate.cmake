# SPDX-License-Identifier: Apache-2.0

# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications.
# Zephyr CMake package automatically includes this file when CMake function
# find_package() is used.
#
# To ensure this file is loaded in a Zephyr application it must start with
# one of those lines:
#
# find_package(Zephyr)
# find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#
# The `REQUIRED HINTS $ENV{ZEPHYR_BASE}` variant is required for any application
# inside the Zephyr repository.
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.

# CMake version 3.20 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.20.0)

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

set(ENV_ZEPHYR_BASE $ENV{ZEPHYR_BASE})
# This add support for old style boilerplate include.
if((NOT DEFINED ZEPHYR_BASE) AND (DEFINED ENV_ZEPHYR_BASE))
  set(ZEPHYR_BASE ${ENV_ZEPHYR_BASE} CACHE PATH "Zephyr base")
endif()

find_package(ZephyrBuildConfiguration
  QUIET NO_POLICY_SCOPE
  NAMES ZephyrBuild
  PATHS ${ZEPHYR_BASE}/../*
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_PACKAGE_REGISTRY
  NO_CMAKE_SYSTEM_PATH
  NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)
#
# Import more CMake functions and macros
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#

include(${ZEPHYR_BASE}/cmake/python.cmake)
include(${ZEPHYR_BASE}/cmake/west.cmake)
include(${ZEPHYR_BASE}/cmake/ccache.cmake)

include(${ZEPHYR_BASE}/cmake/root.cmake)
#
# Find Zephyr modules.
# Those may contain additional DTS, BOARD, SOC, ARCH ROOTs.
# Also create the Kconfig binary dir for generated Kconf files.
#
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

include(${ZEPHYR_BASE}/cmake/boards.cmake)
include(${ZEPHYR_BASE}/cmake/shields.cmake)
include(${ZEPHYR_BASE}/cmake/arch.cmake)
include(${ZEPHYR_BASE}/cmake/configuration_files.cmake)

include(${ZEPHYR_BASE}/cmake/user_cache.cmake)
include(${ZEPHYR_BASE}/cmake/verify-toolchain.cmake)
include(${ZEPHYR_BASE}/cmake/host-tools.cmake)

# Include board specific device-tree flags before parsing.
include(${BOARD_DIR}/pre_dt_board.cmake OPTIONAL)

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
#
# The DT system uses a C preprocessor for it's code generation needs.
# This creates an awkward chicken-and-egg problem, because we don't
# always know exactly which toolchain the user needs until we know
# more about the target, e.g. after DT and Kconfig.
#
# To resolve this we find "some" C toolchain, configure it generically
# with the minimal amount of configuration needed to have it
# preprocess DT sources, and then, after we have finished processing
# both DT and Kconfig we complete the target-specific configuration,
# and possibly change the toolchain.
include(${ZEPHYR_BASE}/cmake/generic_toolchain.cmake)
include(${ZEPHYR_BASE}/cmake/dts.cmake)
include(${ZEPHYR_BASE}/cmake/kconfig.cmake)
include(${ZEPHYR_BASE}/cmake/soc.cmake)
include(${ZEPHYR_BASE}/cmake/target_toolchain.cmake)
include(${ZEPHYR_BASE}/cmake/kernel.cmake)
