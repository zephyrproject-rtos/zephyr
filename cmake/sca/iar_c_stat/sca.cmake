# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025, IAR Systems AB.

cmake_minimum_required(VERSION 4.1.0)

include(extensions)
include(west)

# Get IAR C-STAT
cmake_path(GET CMAKE_C_COMPILER PARENT_PATH IAR_COMPILER_DIR)
find_program(IAR_CSTAT icstat
  HINTS ${IAR_COMPILER_DIR}
  REQUIRED
)
message(STATUS "Found SCA: IAR C-STAT Static Analysis (${IAR_CSTAT})")
find_program(IAR_CHECKS ichecks
  HINTS ${IAR_COMPILER_DIR}
  REQUIRED
)

zephyr_get(CSTAT_RULESET)
zephyr_get(CSTAT_ANALYZE_THREADS)
zephyr_get(CSTAT_ANALYZE_OPTS)
zephyr_get(CSTAT_DB)
zephyr_get(CSTAT_CLEANUP)

# Create an output directory for IAR C-STAT
set(output_dir ${CMAKE_BINARY_DIR}/sca/iar_c_stat)
file(MAKE_DIRECTORY ${output_dir})

# Set the IAR C-STAT ruleset
set(iar_checks_arg --output=${output_dir}/cstat_sel_checks.txt)
if(CSTAT_RULESET MATCHES "^(cert|security|misrac2004|misrac\\+\\+2008|misrac2012)")
  set(iar_checks_arg ${iar_checks_arg} --default=${CSTAT_RULESET})
elseif(CSTAT_RULESET MATCHES "^all")
  set(iar_checks_arg ${iar_checks_arg} --all)
else()
  set(iar_checks_arg ${iar_checks_arg} --default=stdchecks)
endif()
execute_process(COMMAND ${IAR_CHECKS} ${iar_checks_arg})

# Forwards the ruleset manifest file to icstat
set(output_arg --checks=${output_dir}/cstat_sel_checks.txt)

# Analsys parallelization
if(CSTAT_ANALYZE_THREADS)
  set(output_arg ${output_arg};--parallel=${CSTAT_ANALYZE_THREADS})
endif()

# Entrypoint for additional C-STAT options
if(CSTAT_ANALYZE_OPTS)
  set(output_arg ${output_arg};${CSTAT_ANALYZE_OPTS})
endif()

# Full path to the C-STAT SQLite database
if(CSTAT_DB)
  set(CSTAT_DB_PATH ${CSTAT_DB})
else()
  set(CSTAT_DB_PATH ${output_dir}/cstat.db)
endif()
set(output_arg ${output_arg};--db=${CSTAT_DB_PATH})

# Clean-up C-STAT SQLite database
if(CSTAT_CLEANUP)
  execute_process(COMMAND ${IAR_CSTAT} clear --db=${CSTAT_DB_PATH})
endif()

# Enable IAR C-STAT Static Analysis (requires CMake v4.1+)
set(CMAKE_C_ICSTAT ${IAR_CSTAT};${output_arg} CACHE INTERNAL "")
set(CMAKE_CXX_ICSTAT ${IAR_CSTAT};${output_arg} CACHE INTERNAL "")
