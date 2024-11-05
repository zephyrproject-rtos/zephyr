# SPDX-License-Identifier: Apache-2.0
#
# SPDX-FileCopyrightText: Copyright (c) 2025 GARDENA GmbH

find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED)
message(STATUS "Found SCA: cppcheck (${CPPCHECK_EXE})")

# Cppcheck uses the compile_commands.json as input
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create an output directory for our tool
set(output_dir ${CMAKE_BINARY_DIR}/sca/cppcheck)
file(MAKE_DIRECTORY ${output_dir})

# Workaround to help Cppcheck parse the Zephyr headers
# Note: This will probably work only for GCC, mabe also for Clang
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
            ${output_dir}/cppcheck_dummy.c
)
set_property(
    GLOBAL
    APPEND
    PROPERTY extra_post_build_byproducts ${output_dir}/cppcheck_dummy.c
)

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
            ${output_dir}/cppcheck_dummy.c
            -o
            ${output_dir}/cppcheck_dummy.h
)
set_property(
    GLOBAL
    APPEND
    PROPERTY extra_post_build_byproducts ${output_dir}/cppcheck_dummy.h
)

add_custom_target(
    cppcheck
    ALL
    COMMAND
        ${CPPCHECK_EXE} --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --xml # Write results in XML format
        --output-file="${output_dir}/cppcheck.xml" # write output to file, not stderr
        --library=zephyr.cfg # Enable zephyr specific knowledge
        --quiet # Suppress progress reporting
        --include="${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h"
        --include="${output_dir}/cppcheck_dummy.h" -j
        12 # XXX: Works on my machine
        -D__cppcheck__ # As recommended by the Cppcheck manual
        # --premium=safety-off # Premium only: Prevent non-zero exit code when issues found
        ${CODECHECKER_ANALYZE_OPTS}
    DEPENDS
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${output_dir}/cppcheck_dummy.h
    BYPRODUCTS ${output_dir}/cppcheck.xml
    VERBATIM
)

# Cleanup workaround files
add_custom_command(
    TARGET cppcheck
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND}
    ARGS
        -E
        rm
        ${output_dir}/cppcheck_dummy.c
        ${output_dir}/cppcheck_dummy.h
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
