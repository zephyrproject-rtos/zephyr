# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Martin Lampacher

# This CMake file ensures a proper configuration of `cppcheck` with CodeChecker. It can also be
# included by the application in case `cppcheck` is used without CodeChecker, but it is mainly
# targeted at Zephyr's CodeChecker integration.
#
# (1) Implicit compiler definitions
#
# `cppcheck` has its own preprocessor and therefore does not know about definitions (macros) that
# are implicitly used by a compiler. E.g., `__ARM_ARCH_PROFILE` in a GCC compiler targeting an ARM
# MCU. This integration dumps implicit definitions into a header file which is then passed to
# `cppcheck`, resolving macros that would otherwise end up in #error directives.
#
# **Known limitation:**
# It is assumed that the definitions in `TOOLCHAIN_C_FLAGS` (plus some extra-definitions)
# are the same for C and C++ compilation units and that all compilation units use the same set of
# compilation flags, i.e., it is not possible to create a header file per compilation unit for
# the analysis with `cppcheck`.
#
# (2) Kconfig
#
# The generated file `autoconf.h`, containing the Kconfig definitions, also needs to be passed
# to `cppcheck` in order for the analysis to work correctly. Therefore, `autoconf.h` is passed
# to `cppcheck` in the same way as the generated compiler header file.

set(output_dir ${CMAKE_BINARY_DIR}/sca/cppcheck)
file(MAKE_DIRECTORY ${output_dir})

# An empty .c file is used to extract the compiler-specific definitions
if(NOT EXISTS ${output_dir}/empty.c)
  file(WRITE ${output_dir}/empty.c "")
endif()

# `clang` requires the `--target` configuration in case the `-mcpu` option is used. This flag
# is not part of the TOOLCHAIN_C_FLAGS and must be added explicitly.
set(cc_target "")
if(DEFINED CMAKE_C_COMPILER_TARGET)
  set(cc_target "--target=${CMAKE_C_COMPILER_TARGET}")
endif()

# The following target generates a `cppcheck_cc.h` header file in the build folder containing
# definitions that are otherwise implicitly used by the C compiler. This header file can
# subsequently passed as an additional include to `cppcheck`.
add_custom_target(cppcheck_generate_compiler_header ALL
  COMMAND ${CMAKE_C_COMPILER}
    ${TOOLCHAIN_C_FLAGS}
    ${cc_target}
    -E -dM
    ${output_dir}/empty.c > ${output_dir}/cppcheck_cc.h
  DEPENDS ${output_dir}/empty.c
  COMMENT "Generating cppcheck_cc.h"
  BYPRODUCTS ${output_dir}/cppcheck_cc.h
  VERBATIM
)

# User-defined options to include in the generated `cc-verbatim-args.txt`
zephyr_get(CPPCHECK_VERBATIM_EXTRA_ARGS)

# With CodeChecker it is only possible to pass additional includes to `cppcheck` using a file
# and the `--analyzer-config;cppcheck:cc-verbatim-args-file=<path/to/file>` option. The following
# generates the required options file in the build folder.
#
# _Without_ CodeChecker the generated file can also be used when running `cppcheck` directly.
# E.g., assuming that this file is included after `find_package(Zephyr ...)` by the application's
# `CMakeLists.txt`, the following allows executing `cppcheck` with the required integration for
# Zephyr:
#
# $ cppcheck
#     $(cat build/sca/cppcheck/cc-verbatim-args.txt)
#     --project=build/compile_commands.json
#     --enable=style
#
set(CPPCHECK_VERBATIM_ARGS
  "--include=${output_dir}/cppcheck_cc.h"
  "--include=${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h"
  ${CPPCHECK_VERBATIM_EXTRA_ARGS}
)
set(CPPCHECK_VERBATIM_ARGS_FILE ${output_dir}/cc-verbatim-args.txt)
list(JOIN CPPCHECK_VERBATIM_ARGS " " CPPCHECK_VERBATIM_ARGS_STR)
file(WRITE ${CPPCHECK_VERBATIM_ARGS_FILE} "${CPPCHECK_VERBATIM_ARGS_STR}")
