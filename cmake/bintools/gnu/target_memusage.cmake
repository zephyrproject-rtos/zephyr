# SPDX-License-Identifier: Apache-2.0

# Add and/or prepare print of memory usage report
#
# Usage:
#   bintools_print_mem_usage(
#     RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#     RESULT_BYPROD_LIST <List of command output byproducts>
#   )
#
macro(bintools_print_mem_usage)

  # Here we make use of the linkers ability to produce memory usage output
  #  and thus we have no need for the above provided arguments, but another
  #  toolchain with a different set of binary tools, most likely will...
  #
  # Use --print-memory-usage with the first link.
  #
  # Don't use this option with the second link because seeing it twice
  # could confuse users and using it on the second link would suppress
  # it when the first link has a ram/flash-usage issue.
  set(option ${LINKERFLAGPREFIX},--print-memory-usage)
  string(MAKE_C_IDENTIFIER check${option} check)

  set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
  zephyr_check_compiler_flag(C "" ${check})
  set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

  target_link_libraries_ifdef(${check} ${ZEPHYR_PREBUILT_EXECUTABLE} ${option})

endmacro(bintools_print_mem_usage)
