macro(toolchain_cc_cpp)

  # Dialects of C++, corresponding to the multiple published ISO standards.
  # Which standard it implements can be selected using the -std= command-line option.
  set_ifndef(DIALECT_STD_CPP98 "c++98")
  set_ifndef(DIALECT_STD_CPP11 "c++11")
  set_ifndef(DIALECT_STD_CPP14 "c++14")
  set_ifndef(DIALECT_STD_CPP17 "c++17")
  set_ifndef(DIALECT_STD_CPP2A "c++2a")

  if(CONFIG_STD_CPP98)
    set(STD_CPP_DIALECT ${DIALECT_STD_CPP98})
  elseif(CONFIG_STD_CPP11)
    set(STD_CPP_DIALECT ${DIALECT_STD_CPP11}) # Default
  elseif(CONFIG_STD_CPP14)
    set(STD_CPP_DIALECT ${DIALECT_STD_CPP14})
  elseif(CONFIG_STD_CPP17)
    set(STD_CPP_DIALECT ${DIALECT_STD_CPP17})
  elseif(CONFIG_STD_CPP2A)
    set(STD_CPP_DIALECT ${DIALECT_STD_CPP2A})
  else()
    assert(0 "Unreachable code. Expected C++ standard to have been chosen. See misc/Kconfig.")
  endif()

  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-std=${STD_CPP_DIALECT}>
    $<$<COMPILE_LANGUAGE:CXX>:-fcheck-new>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>

    $<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp>
    $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
  )

  if(NOT CONFIG_RTTI)
    zephyr_compile_options(
      $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    )
  endif()

  if(NOT CONFIG_EXCEPTIONS)
    zephyr_compile_options(
      $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    )
  endif()

endmacro()
