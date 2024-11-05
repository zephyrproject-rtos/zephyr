# SPDX-License-Identifier: Apache-2.0
#
# SPDX-FileCopyrightText: Copyright (c) 2025 GARDENA GmbH

# If not specified manually, let Cppcheck to use all available (logical) CPU
# cores.
if(NOT DEFINED CPPCHECK_JOBS_COUNT)
  cmake_host_system_information(RESULT CPPCHECK_JOBS_COUNT
                                QUERY NUMBER_OF_LOGICAL_CORES)
endif()

find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED)
message(STATUS "Found SCA: cppcheck (${CPPCHECK_EXE})")

# Defaulting to exhaustive to prevent normalCheckLevelMaxBranches "findings"
set(CPPCHECK_CHECK_LEVEL "exhaustive" CACHE STRING
    "Value of the --check-level argument passed to cppcheck")

# The premium edition exits with a non-zero exit code whenever it finds a bug,
# causing the SCA infrastructure of Zephyr to quit with an error too.
# We can prevent this by adding the "--premium=safety-off" argument. However,
# this is known only the the Premium edition of Cppcheck.
if(NOT DEFINED CPPCHECK_IS_PREMIUM)
  execute_process(COMMAND "${CPPCHECK_EXE}" "--version"
                  OUTPUT_VARIABLE CPPCHECK_VERSION_STRING
                  COMMAND_ERROR_IS_FATAL ANY)
  if("${CPPCHECK_VERSION_STRING}" MATCHES "Premium")
    set(CPPCHECK_IS_PREMIUM 1)
  endif()
endif()
option(CPPCHECK_IS_PREMIUM "Whether we run Cppcheck premium" 0)
if(CPPCHECK_IS_PREMIUM)
  set(CPPCHECK_PREMIUM_ARGUMENT "--premium=safety-off")
  message(STATUS "Using the Premium version of Cppcheck")
else()
  set(CPPCHECK_PREMIUM_ARGUMENT "")
  message(STATUS "Using the Open Source version of Cppcheck")
endif()

# Cppcheck uses the compile_commands.json as input
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create an output directory for our tool
set(output_dir ${CMAKE_BINARY_DIR}/sca/cppcheck)
file(MAKE_DIRECTORY ${output_dir})

# Help Cppcheck parse the Zephyr headers, reduce runtime by ensuring all
# relevant configs defined
set_property(
    GLOBAL
    APPEND
    PROPERTY
        extra_post_build_commands
            COMMAND
            ${CMAKE_COMMAND}
            ARGS
            -E
            touch
            ${output_dir}/compiler-macros.c
)
set_property(
    GLOBAL
    APPEND
    PROPERTY extra_post_build_byproducts ${output_dir}/compiler-macros.c
)

# Note: This works only for GCC and compatibles. Please add support for other
# compilers!
set_property(
    GLOBAL
    APPEND
    PROPERTY
        extra_post_build_commands
            COMMAND
            ${CMAKE_C_COMPILER}
            ARGS
            -dM
            -E
            ${output_dir}/compiler-macros.c
            -o
            ${output_dir}/compiler-macros.h
)

set_property(
    GLOBAL
    APPEND
    PROPERTY extra_post_build_byproducts ${output_dir}/compiler-macros.h
)

add_custom_target(
    cppcheck
    ALL
    COMMAND
        ${CPPCHECK_EXE} --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --xml # Write results in XML format
        --output-file=${output_dir}/cppcheck.xml # write output to file, not stderr
        --library=zephyr.cfg # Enable Zephyr specific knowledge
        --quiet # Suppress progress reporting
        --include=${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h
        --include=${output_dir}/compiler-macros.h # Use exact compiler macros
        -j ${CPPCHECK_JOBS_COUNT} # Parallelize analysis
        -D__cppcheck__ # As recommended by the Cppcheck manual
        --check-level=${CPPCHECK_CHECK_LEVEL}
        ${CPPCHECK_PREMIUM_ARGUMENT}
        ${CODECHECKER_ANALYZE_OPTS}
    DEPENDS
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${output_dir}/compiler-macros.h
    BYPRODUCTS ${output_dir}/cppcheck.xml
    VERBATIM
)

# Cleanup helper files
add_custom_command(
    TARGET cppcheck
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E rm ${output_dir}/compiler-macros.c ${output_dir}/compiler-macros.h
)

find_program(CPPCHECK_HTMLREPORT_EXE NAMES cppcheck-htmlreport REQUIRED)
message(STATUS "Found SCA: cppcheck-htmlreport (${CPPCHECK_HTMLREPORT_EXE})")
add_custom_command(
    TARGET cppcheck
    POST_BUILD
    COMMAND
        ${CPPCHECK_HTMLREPORT_EXE} --title=Zephyr
        --file=${output_dir}/cppcheck.xml # Read in file created by Cppcheck
        --report-dir=${output_dir} # Set output directory
        ${CPPCHECK_HTMLREPORT_OPTS}
    BYPRODUCTS
        ${output_dir}/index.html
        ${output_dir}/stats.html
        ${output_dir}/style.css
    VERBATIM
)
