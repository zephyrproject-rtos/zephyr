# SPDX-License-Identifier: Apache-2.0
#
# SPDX-FileCopyrightText: Copyright (c) 2025 GARDENA GmbH

include(ProcessorCount)
processorcount(CPPCHECK_JOBS_COUNT)
if(N EQUAL 0)
    set(CPPCHECK_JOBS_COUNT 1)
endif()

find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED)
message(STATUS "Found SCA: cppcheck (${CPPCHECK_EXE})")

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
        --library=zephyr.cfg # Enable zephyr specific knowledge
        --quiet # Suppress progress reporting
        --include=${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h
        --include=${output_dir}/compiler-macros.h # Use exact compiler macros
        -j ${CPPCHECK_JOBS_COUNT} # Parallelize analysis
        -D__cppcheck__ # As recommended by the Cppcheck manual
        --check-level=exhaustive # Prevent normalCheckLevelMaxBranches "findings"
        # --premium=safety-off # Premium only: Prevent non-zero exit code when issues found
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
