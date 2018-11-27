macro(toolchain_bintools_print_size)

  if(CONFIG_OUTPUT_PRINT_MEMORY_USAGE)
    # Use --print-memory-usage with the first link.
    #
    # Don't use this option with the second link because seeing it twice
    # could confuse users and using it on the second link would suppress
    # it when the first link has a ram/flash-usage issue.
    set(option ${LINKERFLAGPREFIX},--print-memory-usage)
    string(MAKE_C_IDENTIFIER check${option} check)

    # https://cmake.org/cmake/help/latest/module/CheckIncludeFile.html:
    #   CMAKE_REQUIRED_FLAGS is the string of compile command line flags
    set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})       # push
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}") #   modify
    zephyr_check_compiler_flag(C "" ${check})                     #   use
    set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})       # pop, undo mod

    target_link_libraries_ifdef(${check} zephyr_prebuilt ${option})
  endif()

endmacro()
