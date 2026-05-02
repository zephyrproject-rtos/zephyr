# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025 Alex Fabre

find_program(CLANG_SCA_EXE NAMES analyze-build REQUIRED)
message(STATUS "Found SCA: clang static analyzer (${CLANG_SCA_EXE})")

# Get clang analyzer user options
zephyr_get(CLANG_SCA_OPTS)
zephyr_get(LLVM_TOOLCHAIN_PATH)

# Check analyzer extra options
if(DEFINED CLANG_SCA_OPTS)
  foreach(analyzer_option IN LISTS CLANG_SCA_OPTS)
    list(APPEND CLANG_SCA_EXTRA_OPTS ${analyzer_option})
  endforeach()
endif()

# clang analyzer uses the compile_commands.json as input
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create an output directory for clang analyzer results
set(output_dir ${CMAKE_BINARY_DIR}/sca/clang)
file(MAKE_DIRECTORY ${output_dir})

# Use a dummy file to let clang static analyzer know we can start analyzing
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
  ${CMAKE_COMMAND} -E touch ${output_dir}/clang-sca.ready)
set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
  ${output_dir}/clang-sca.ready)

# Add a cmake target to run the analyzer after the build is done
add_custom_target(clang-sca ALL
  COMMAND ${CLANG_SCA_EXE} --cdb ${CMAKE_BINARY_DIR}/compile_commands.json -o ${CMAKE_BINARY_DIR}/sca/clang/ --analyze-headers --use-analyzer ${LLVM_TOOLCHAIN_PATH}/bin/clang ${CLANG_SCA_EXTRA_OPTS}
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json ${output_dir}/clang-sca.ready
)

# Cleanup dummy file
add_custom_command(
  TARGET clang-sca POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm ${output_dir}/clang-sca.ready
)
