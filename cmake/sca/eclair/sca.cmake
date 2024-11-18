# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, BUGSENG Srl

find_program(ECLAIR_ENV eclair_env REQUIRED)
message(STATUS "Found eclair_env: ${ECLAIR_ENV}")

find_program(ECLAIR_REPORT eclair_report REQUIRED)
message(STATUS "Found eclair_report: ${ECLAIR_REPORT}")

# Get eclair specific option file variables, also needed if invoked with sysbuild
zephyr_get(ECLAIR_OPTIONS_FILE)

if(ECLAIR_OPTIONS_FILE)
  if(IS_ABSOLUTE ${ECLAIR_OPTIONS_FILE})
    set(ECLAIR_OPTIONS ${ECLAIR_OPTIONS_FILE})
  else()
    set(ECLAIR_OPTIONS ${APPLICATION_CONFIG_DIR}/${ECLAIR_OPTIONS_FILE})
  endif()
  include(${ECLAIR_OPTIONS})
else()
  include(${CMAKE_CURRENT_LIST_DIR}/sca_options.cmake)
endif()

# ECLAIR Settings
set(ECLAIR_PROJECT_NAME "Zephyr-${BOARD}${BOARD_QUALIFIERS}")
set(ECLAIR_OUTPUT_DIR "${CMAKE_BINARY_DIR}/sca/eclair")
set(ECLAIR_ECL_DIR "${ZEPHYR_BASE}/cmake/sca/eclair/ECL")
set(ECLAIR_ANALYSIS_ECL_DIR "${ZEPHYR_BASE}/cmake/sca/eclair/ECL")
set(ECLAIR_DIAGNOSTICS_OUTPUT "${ECLAIR_OUTPUT_DIR}/DIAGNOSTIC.txt")
set(ECLAIR_ANALYSIS_DATA_DIR "${ECLAIR_OUTPUT_DIR}/analysis_data")
set(ECLAIR_PROJECT_ECD "${ECLAIR_OUTPUT_DIR}/PROJECT.ecd")
set(CC_ALIASES "${CMAKE_C_COMPILER}")
set(CXX_ALIASES "${CMAKE_CXX_COMPILER}")
set(AS_ALIASES "${CMAKE_AS}")
set(LD_ALIASES "${CMAKE_LINKER}")
set(AR_ALIASES "${CMAKE_ASM_COMPILER_AR} ${CMAKE_C_COMPILER_AR} ${CMAKE_CXX_COMPILER_AR}")

set(ECLAIR_ENV_ADDITIONAL_OPTIONS "")
set(ECLAIR_REPORT_ADDITIONAL_OPTIONS "")

# Default value
set(ECLAIR_RULESET first_analysis)

# ECLAIR env
if(ECLAIR_RULESET_FIRST_ANALYSIS)
  set(ECLAIR_RULESET first_analysis)
elseif(ECLAIR_RULESET_STU)
  set(ECLAIR_RULESET STU)
elseif(ECLAIR_RULESET_STU_HEAVY)
  set(ECLAIR_RULESET STU_heavy)
elseif(ECLAIR_RULESET_WP)
  set(ECLAIR_RULESET WP)
elseif(ECLAIR_RULESET_STD_LIB)
  set(ECLAIR_RULESET std_lib)
elseif(ECLAIR_RULESET_USER)
  set(ECLAIR_RULESET ${ECLAIR_USER_RULESET_NAME})
  if(IS_ABSOLUTE ${ECLAIR_USER_RULESET_PATH})
      set(ECLAIR_ANALYSIS_ECL_DIR ${ECLAIR_USER_RULESET_PATH})
  else()
    set(ECLAIR_ANALYSIS_ECL_DIR ${APPLICATION_CONFIG_DIR}/${ECLAIR_USER_RULESET_PATH})
  endif()
endif()

# ECLAIR report
if (ECLAIR_METRICS_TAB)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-metrics_tab=${ECLAIR_OUTPUT_DIR}/metrics")
endif()
if (ECLAIR_REPORTS_TAB)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-reports_tab=${ECLAIR_OUTPUT_DIR}/reports")
endif()
if (ECLAIR_REPORTS_SARIF)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-reports_sarif=${ECLAIR_OUTPUT_DIR}/reports.sarif")
endif()
if (ECLAIR_SUMMARY_TXT)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-summary_txt=${ECLAIR_OUTPUT_DIR}/summary_txt")
endif()
if (ECLAIR_SUMMARY_DOC)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-summary_doc=${ECLAIR_OUTPUT_DIR}/summary_doc")
endif()
if (ECLAIR_SUMMARY_ODT)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-summary_odt=${ECLAIR_OUTPUT_DIR}/summary_odt")
endif()
if (ECLAIR_FULL_TXT_ALL_AREAS)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-setq=report_areas,areas")
endif()
if (ECLAIR_FULL_TXT_FIRST_AREA)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-setq=report_areas,first_area")
endif()
if (ECLAIR_FULL_TXT)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-full_txt=${ECLAIR_OUTPUT_DIR}/report_full_txt")
endif()
if (ECLAIR_FULL_DOC_ALL_AREAS)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-setq=report_areas,areas")
endif()
if (ECLAIR_FULL_DOC_FIRST_AREA)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-setq=report_areas,first_area")
endif()
if (ECLAIR_FULL_DOC)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-full_doc=${ECLAIR_OUTPUT_DIR}/report_full_doc")
endif()
if (ECLAIR_FULL_ODT)
  list(APPEND ECLAIR_REPORT_ADDITIONAL_OPTIONS "-full_odt=${ECLAIR_OUTPUT_DIR}/report_full_odt")
endif()

message(STATUS "ECLAIR outputs have been written to: ${ECLAIR_OUTPUT_DIR}")
message(STATUS "ECLAIR ECB files have been written to: ${ECLAIR_ANALYSIS_DATA_DIR}")

add_custom_target(eclair_setup_analysis_dir ALL
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${ECLAIR_ANALYSIS_DATA_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ECLAIR_ANALYSIS_DATA_DIR}
  VERBATIM
  USES_TERMINAL
)

# configure the camke script which will be used to replace the compiler call with the eclair_env
# call which calls the compiler and to generate analysis files.
configure_file(${CMAKE_CURRENT_LIST_DIR}/eclair.template ${ECLAIR_OUTPUT_DIR}/eclair.cmake @ONLY)

set(launch_environment ${CMAKE_COMMAND} -P ${ECLAIR_OUTPUT_DIR}/eclair.cmake --)
set(CMAKE_C_COMPILER_LAUNCHER ${launch_environment} CACHE INTERNAL "")

# This target is used to generate the ECLAIR database when all the compilation is done and the
# elf file was generated with this we cane make sure that the analysis is completed.
add_custom_target(eclair_report ALL
  COMMAND ${CMAKE_COMMAND} -E env
    ECLAIR_DATA_DIR=${ECLAIR_ANALYSIS_DATA_DIR}
    ECLAIR_OUTPUT_DIR=${ECLAIR_OUTPUT_DIR}
    ECLAIR_PROJECT_ECD=${ECLAIR_PROJECT_ECD}
    ${ECLAIR_REPORT} -quiet -eval_file=${ECLAIR_ECL_DIR}/db_generation.ecl
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
  VERBATIM
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

# This command is used to generate the final reports from the database and print the overall results
add_custom_target(eclair_summary_print ALL
  COMMAND ${ECLAIR_REPORT}
    -db=${ECLAIR_PROJECT_ECD} ${ECLAIR_REPORT_ADDITIONAL_OPTIONS}
    -overall_txt=${ECLAIR_OUTPUT_DIR}/summary_overall.txt
  COMMAND ${CMAKE_COMMAND} -E cat ${ECLAIR_OUTPUT_DIR}/summary_overall.txt
)
add_dependencies(eclair_summary_print eclair_report)
