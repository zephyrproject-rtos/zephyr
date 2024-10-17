# Copyright 2022 Meta
# SPDX-License-Identifier: Apache-2.0

find_program(THRIFT_EXECUTABLE thrift)
if(NOT THRIFT_EXECUTABLE)
  message(FATAL_ERROR "The 'thrift' command was not found")
endif()

function(thrift
    target          # CMake target (for dependencies / headers)
    lang            # The language for generated sources
    lang_opts       # Language options (e.g. ':no_skeleton')
    out_dir         # Output directory for generated files
                    # (do not include 'gen-cpp', etc)
    source_file     # The .thrift source file
    options         # Additional thrift options

                    # Generated files in ${ARGN}
    )
  file(MAKE_DIRECTORY ${out_dir})
  add_custom_command(
    OUTPUT ${ARGN}
    COMMAND
    ${THRIFT_EXECUTABLE}
    --gen ${lang}${lang_opts}
    -o ${out_dir}
    ${source_file}
    ${options}
    DEPENDS ${source_file}
    )

    target_include_directories(
      ${target} PRIVATE ${out_dir}/gen-${lang}
      ${target} PRIVATE ${ZEPHYR_BASE}/modules/thrift/src
      ${target} PRIVATE ${ZEPHYR_THRIFT_MODULE_DIR}/lib/cpp/src
    )
endfunction()
