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

# Add a cmake target to run the analyzer after the build is done
add_custom_target(clang-sca ALL
  COMMAND ${CLANG_SCA_EXE} --cdb ${CMAKE_BINARY_DIR}/compile_commands.json -o ${CMAKE_BINARY_DIR}/sca/clang/ --analyze-headers --use-analyzer ${LLVM_TOOLCHAIN_PATH}/bin/clang ${CLANG_SCA_EXTRA_OPTS}
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
)

# Add a dependency on the final zephyr ELF target so the analyzer runs only
# after the build is complete. logical_target_for_zephyr_elf is not yet
# defined when this file is included, so defer the call until the end of
# the top-level CMakeLists.txt scope.
cmake_language(DEFER CALL add_dependencies clang-sca ${logical_target_for_zephyr_elf})
