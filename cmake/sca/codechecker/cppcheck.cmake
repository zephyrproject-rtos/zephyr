# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Martin Lampacher

# This CMake file ensures a proper configuration of `cppcheck` for Zephyr's CodeChecker integration.
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

# `clang` requires the `--target` configuration in case the `-mcpu` option is used. This flag
# is not part of the TOOLCHAIN_C_FLAGS and must be added explicitly.
set(cc_target)
if(DEFINED CMAKE_C_COMPILER_TARGET)
  set(cc_target "--target=${CMAKE_C_COMPILER_TARGET}")
endif()

# The following command generates a `cppcheck_cc.h` header file in the build folder containing
# definitions that are otherwise implicitly used by the C compiler. This header file can
# subsequently passed as an additional include to `cppcheck`.
set(CPPCHECK_CC_HEADER ${output_dir}/cppcheck_cc.h)
add_custom_command(
  OUTPUT ${CPPCHECK_CC_HEADER}
  COMMAND ${CMAKE_COMMAND}
    -E touch ${output_dir}/empty.c
  COMMAND ${CMAKE_C_COMPILER}
    ${TOOLCHAIN_C_FLAGS}
    ${cc_target}
    -E -dM
    -o ${CPPCHECK_CC_HEADER}
    ${output_dir}/empty.c
  COMMENT "Generating cppcheck_cc.h"
  VERBATIM
)

# User-defined options to include in the generated `cc-verbatim-args.txt`
zephyr_get(CPPCHECK_VERBATIM_EXTRA_ARGS)

# With CodeChecker it is only possible to pass additional includes to `cppcheck` using a file
# and the `--analyzer-config;cppcheck:cc-verbatim-args-file=<path/to/file>` option. The following
# generates the required options file in the build folder.
set(CPPCHECK_VERBATIM_ARGS
  "--include=${CPPCHECK_CC_HEADER}"
  "--include=${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h"
  ${CPPCHECK_VERBATIM_EXTRA_ARGS}
)
set(CPPCHECK_VERBATIM_ARGS_FILE ${output_dir}/cc-verbatim-args.txt)
list(JOIN CPPCHECK_VERBATIM_ARGS " " CPPCHECK_VERBATIM_ARGS_STR)
file(WRITE ${CPPCHECK_VERBATIM_ARGS_FILE} "${CPPCHECK_VERBATIM_ARGS_STR}")
