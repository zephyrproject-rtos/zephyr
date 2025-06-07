include(CMakeDependentOption)

option(ECLAIR_RULESET_FIRST_ANALYSIS "A tiny selection of the projects coding guideline rules to
                                      verify that everything is correctly working" ON)

option(ECLAIR_RULESET_STU "Selection of the projects coding guidelines, which can be verified
                           by analysing the single translation units independently." OFF)

option(ECLAIR_RULESET_STU_HEAVY "Selection of complex STU project coding guidelines that
                                require a significant amount of time" OFF)
option(ECLAIR_RULESET_WP "All whole program project coding guidelines ('system' in MISRA's
                         parlance)." OFF)
option(ECLAIR_RULESET_STD_LIB "Project coding guidelines about the C Standard Library" OFF)
option(ECLAIR_RULESET_ZEPHYR_GUIDELINES "Zephyr project Coding Guidelines" OFF)
option(ECLAIR_RULESET_USER "User defined ruleset" OFF)

option(ECLAIR_METRICS_TAB "Metrics in a spreadsheet format" OFF)
option(ECLAIR_REPORTS_TAB "Findings in a spreadsheet format" OFF)
option(ECLAIR_REPORTS_SARIF "Findings in sarif JSON format" ON)
option(ECLAIR_SUMMARY_TXT "Plain textual summary format" OFF)
option(ECLAIR_SUMMARY_DOC "DOC summary format" OFF)
option(ECLAIR_SUMMARY_ODT "ODT summary format" OFF)
option(ECLAIR_SUMMARY_HTML "HTML summary format" OFF)
option(ECLAIR_FULL_TXT "Detailed plain textual format" ON)
option(ECLAIR_FULL_DOC "Detailed DOC format" OFF)
option(ECLAIR_FULL_ODT "Detailed ODT format" OFF)
option(ECLAIR_FULL_HTML "Detailed HTML format" OFF)

cmake_dependent_option(ECLAIR_FULL_DOC_ALL_AREAS "Show all areas in a full doc report"
                       OFF "ECLAIR_FULL_DOC OR ECLAIR_FULL_ODT" OFF)
cmake_dependent_option(ECLAIR_FULL_DOC_FIRST_AREA "Show only the first area in a full doc report"
                       ON "ECLAIR_FULL_DOC OR ECLAIR_FULL_ODT" OFF)

cmake_dependent_option(ECLAIR_FULL_TXT_ALL_AREAS "Show all areas in a full text report"
                       OFF "ECLAIR_FULL_TXT" OFF)
cmake_dependent_option(ECLAIR_FULL_TXT_FIRST_AREA "Show only the first area in a full text report"
                       ON "ECLAIR_FULL_TXT" OFF)

# Ensure that exactly one ECLAIR_RULESET is selected
set(ECLAIR_RULESETS
    ECLAIR_RULESET_FIRST_ANALYSIS
    ECLAIR_RULESET_STU
    ECLAIR_RULESET_STU_HEAVY
    ECLAIR_RULESET_WP
    ECLAIR_RULESET_STD_LIB
    ECLAIR_RULESET_ZEPHYR_GUIDELINES
    ECLAIR_RULESET_USER
)

set(selected_rulesets "")
foreach(ruleset ${ECLAIR_RULESETS})
  if(${ruleset})
    list(APPEND selected_rulesets ${ruleset})
  endif()
endforeach()

list(LENGTH selected_rulesets selected_count)
if(selected_count EQUAL 0)
  message(FATAL_ERROR "No ECLAIR_RULESET is selected. Please select exactly one.")
elseif(selected_count GREATER 1)
  message(FATAL_ERROR "Only one ECLAIR_RULESET can be selected at a time.
                       Selected: ${selected_rulesets}")
endif()
