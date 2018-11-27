macro(toolchain_cc_preprocess_linker_pass linker_output_name)

  set(linker_cmd_file_name ${linker_output_name}.cmd)

  if (${linker_output_name} MATCHES "^linker_pass_final$")
    set(LINKER_PASS_DEFINE -DLINKER_PASS2)
  else()
    set(LINKER_PASS_DEFINE "")
  endif()

  # Different generators deal with depfiles differently.
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # Note that the IMPLICIT_DEPENDS option is currently supported only
    # for Makefile generators and will be ignored by other generators.
    set(LINKER_SCRIPT_DEP IMPLICIT_DEPENDS C ${LINKER_SCRIPT})
  elseif(CMAKE_GENERATOR STREQUAL "Ninja")
    # Using DEPFILE with other generators than Ninja is an error.
    set(LINKER_SCRIPT_DEP DEPFILE ${PROJECT_BINARY_DIR}/${linker_cmd_file_name}.dep)
  else()
    # TODO: How would the linker script dependencies work for non-linker
    # script generators.
    message(STATUS "
      Warning; this generator is not well supported. The
      Linker script may not be regenerated when it should."
    )
    set(LINKER_SCRIPT_DEP "")
  endif()

  get_filename_component(BASE_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)

  # Specify the way to build ${linker_cmd_file_name}, with the side effect of
  # also building dependency-rule files
  add_custom_command(
    OUTPUT ${linker_cmd_file_name}
    DEPENDS ${LINKER_SCRIPT}
    ${LINKER_SCRIPT_DEP}
    COMMAND ${CMAKE_C_COMPILER}
    # gcc -x: Specify explicitly the language for the following input files
          -x assembler-with-cpp
    ${NOSTDINC_F}
    ${NOSYSDEF_CFLAG}
    # gcc -MD: Equivalent to -M -MF file, except that -E is not implied
          -MD
    # gcc -MF: When used with -M or -MM, specifies a file to write the dependencies to
          -MF ${linker_cmd_file_name}.dep
    # gcc -MT: Output a rule suitable for GNU make, describing the dependencies of source file
          -MT ${BASE_NAME}/${linker_cmd_file_name}
    ${ZEPHYR_INCLUDES}
    ${LINKER_SCRIPT_DEFINES}
    ${LINKER_PASS_DEFINE}
    # gcc -E: Stop after the preprocessing stage; do not run the compiler proper
          -E
    ${LINKER_SCRIPT}
    # gcc -P: Prevent generation of `#line' directives (linemarkers) from the preprocessor
          -P
    -o ${linker_cmd_file_name}
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

endmacro()
