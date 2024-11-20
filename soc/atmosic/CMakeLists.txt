# Copyright (c) 2021-2024 Atmosic
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
    include
)

add_subdirectory(${SOC_SERIES})

if(CONFIG_COVERAGE AND CONFIG_AUTO_TEST)
  # Reset coverage properties
  set_compiler_property(PROPERTY no_coverage -fno-profile-arcs -fno-test-coverage -finline)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,no_coverage>)

  # app target
  set_property(TARGET app APPEND PROPERTY COMPILE_OPTIONS -fno-profile-arcs -fno-test-coverage -finline)

  # library source files
  function(set_coverage_properties_for_files file_list patterns_list)
    foreach(pattern IN LISTS ${patterns_list})
        foreach(file IN LISTS ${file_list})
            get_filename_component(file_name ${file} NAME)
            if(${file_name} MATCHES ${pattern})
                set_source_files_properties(
                    ${file}
                    DIRECTORY ${ZEPHYR_BASE}
                    PROPERTIES COMPILE_OPTIONS $<TARGET_PROPERTY:compiler,coverage>
                )
            endif()
        endforeach()
    endforeach()
  endfunction()
endif()
