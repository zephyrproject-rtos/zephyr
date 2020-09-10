# SPDX-License-Identifier: Apache-2.0

########################################################
# Table of contents
########################################################
# 1. Zephyr-aware extensions
# 1.1. zephyr_*
# 1.2. zephyr_library_*
# 1.2.1 zephyr_interface_library_*
# 1.3. generate_inc_*
# 1.4. board_*
# 1.5. Misc.
# 2. Kconfig-aware extensions
# 2.1 Misc
# 3. CMake-generic extensions
# 3.1. *_ifdef
# 3.2. *_ifndef
# 3.3. *_option compiler compatibility checks
# 3.3.1 Toolchain integration
# 3.4. Debugging CMake
# 3.5. File system management

########################################################
# 1. Zephyr-aware extensions
########################################################
# 1.1. zephyr_*
#
# The following methods are for modifying the CMake library[0] called
# "zephyr". zephyr is a catch-all CMake library for source files that
# can be built purely with the include paths, defines, and other
# compiler flags that all zephyr source files use.
# [0] https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html
#
# Example usage:
# zephyr_sources(
#   random_esp32.c
#   utils.c
# )
#
# Is short for:
# target_sources(zephyr PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/random_esp32.c
#   ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
# )
#
# As a very high-level introduction here are two call graphs that are
# purposely minimalistic and incomplete.
#
#  zephyr_library_cc_option()
#           |
#           v
#  zephyr_library_compile_options()  -->  target_compile_options()
#
#
#  zephyr_cc_option()           --->  target_cc_option()
#                                          |
#                                          v
#  zephyr_cc_option_fallback()  --->  target_cc_option_fallback()
#                                          |
#                                          v
#  zephyr_compile_options()     --->  target_compile_options()
#


# https://cmake.org/cmake/help/latest/command/target_sources.html
function(zephyr_sources)
  foreach(arg ${ARGV})
    if(IS_DIRECTORY ${arg})
      message(FATAL_ERROR "zephyr_sources() was called on a directory")
    endif()
    target_sources(zephyr PRIVATE ${arg})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(zephyr_include_directories)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()
    target_include_directories(zephyr_interface INTERFACE ${path})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(zephyr_system_include_directories)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()
    target_include_directories(zephyr_interface SYSTEM INTERFACE ${path})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
function(zephyr_compile_definitions)
  target_compile_definitions(zephyr_interface INTERFACE ${ARGV})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_options.html
function(zephyr_compile_options)
  target_compile_options(zephyr_interface INTERFACE ${ARGV})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_link_libraries.html
function(zephyr_link_libraries)
  target_link_libraries(zephyr_interface INTERFACE ${ARGV})
endfunction()

# See this file section 3.1. target_cc_option
function(zephyr_cc_option)
  foreach(arg ${ARGV})
    target_cc_option(zephyr_interface INTERFACE ${arg})
  endforeach()
endfunction()

function(zephyr_cc_option_fallback option1 option2)
    target_cc_option_fallback(zephyr_interface INTERFACE ${option1} ${option2})
endfunction()

function(zephyr_ld_options)
    target_ld_options(zephyr_interface INTERFACE ${ARGV})
endfunction()

# Getter functions for extracting build information from
# zephyr_interface. Returning lists, and strings is supported, as is
# requesting specific categories of build information (defines,
# includes, options).
#
# The naming convention follows:
# zephyr_get_${build_information}_for_lang${format}(lang x [STRIP_PREFIX])
# Where
#  the argument 'x' is written with the result
# and
#  ${build_information} can be one of
#   - include_directories           # -I directories
#   - system_include_directories    # -isystem directories
#   - compile_definitions           # -D'efines
#   - compile_options               # misc. compiler flags
# and
#  ${format} can be
#   - the empty string '', signifying that it should be returned as a list
#   - _as_string signifying that it should be returned as a string
# and
#  ${lang} can be one of
#   - C
#   - CXX
#   - ASM
#
# STRIP_PREFIX
#
# By default the result will be returned ready to be passed directly
# to a compiler, e.g. prefixed with -D, or -I, but it is possible to
# omit this prefix by specifying 'STRIP_PREFIX' . This option has no
# effect for 'compile_options'.
#
# e.g.
# zephyr_get_include_directories_for_lang(ASM x)
# writes "-Isome_dir;-Isome/other/dir" to x

function(zephyr_get_include_directories_for_lang_as_string lang i)
  zephyr_get_include_directories_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_system_include_directories_for_lang_as_string lang i)
  zephyr_get_system_include_directories_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_definitions_for_lang_as_string lang i)
  zephyr_get_compile_definitions_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_options_for_lang_as_string lang i)
  zephyr_get_compile_options_for_lang(${lang} list_of_flags)

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_include_directories_for_lang lang i)
  zephyr_get_parse_args(args ${ARGN})
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

  if(NOT ARGN)
    set(result_output_list "-I$<JOIN:${genexp_output_list}, -I>")
  elseif(args_STRIP_PREFIX)
    # The list has no prefix, so don't add it.
    set(result_output_list ${output_list})
  else()
    set(result_output_list "-I$<JOIN:${genexp_output_list},${ARGN}-I>")
  endif()
  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_system_include_directories_for_lang lang i)
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")
  set(result_output_list "-isystem$<JOIN:${genexp_output_list}, -isystem>")

  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_definitions_for_lang lang i)
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_DEFINITIONS)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")
  set(result_output_list "-D$<JOIN:${genexp_output_list}, -D>")

  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_options_for_lang lang i)
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")
  set(result_output_list " $<JOIN:${genexp_output_list}, >")

  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

# This function writes a dict to it's output parameter
# 'return_dict'. The dict has information about the parsed arguments,
#
# Usage:
#   zephyr_get_parse_args(foo ${ARGN})
#   print(foo_STRIP_PREFIX) # foo_STRIP_PREFIX might be set to 1
function(zephyr_get_parse_args return_dict)
  foreach(x ${ARGN})
    if(x STREQUAL STRIP_PREFIX)
      set(${return_dict}_STRIP_PREFIX 1 PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

function(process_flags lang input output)
  # The flags might contains compile language generator expressions that
  # look like this:
  # $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  # $<$<COMPILE_LANGUAGE:CXX>:$<OTHER_EXPRESSION>>
  #
  # Flags that don't specify a language like this apply to all
  # languages.
  #
  # See COMPILE_LANGUAGE in
  # https://cmake.org/cmake/help/v3.3/manual/cmake-generator-expressions.7.html
  #
  # To deal with this, we apply a regex to extract the flag and also
  # to find out if the language matches.
  #
  # If this doesn't work out we might need to ban the use of
  # COMPILE_LANGUAGE and instead partition C, CXX, and ASM into
  # different libraries
  set(languages C CXX ASM)

  set(tmp_list "")

  foreach(flag ${${input}})
    set(is_compile_lang_generator_expression 0)
    foreach(l ${languages})
      if(flag MATCHES "<COMPILE_LANGUAGE:${l}>:([^>]+)>")
        set(updated_flag ${CMAKE_MATCH_1})
        set(is_compile_lang_generator_expression 1)
        if(${l} STREQUAL ${lang})
          # This test will match in case there are more generator expressions in the flag.
          # As example: $<$<COMPILE_LANGUAGE:C>:$<OTHER_EXPRESSION>>
          #             $<$<OTHER_EXPRESSION:$<COMPILE_LANGUAGE:C>:something>>
          string(REGEX MATCH "(\\\$<)[^\\\$]*(\\\$<)[^\\\$]*(\\\$<)" IGNORE_RESULT ${flag})
          if(CMAKE_MATCH_2)
            # Nested generator expressions are used, just substitue `$<COMPILE_LANGUAGE:${l}>` to `1`
            string(REGEX REPLACE "\\\$<COMPILE_LANGUAGE:${l}>" "1" updated_flag ${flag})
          endif()
          list(APPEND tmp_list ${updated_flag})
          break()
        endif()
      endif()
    endforeach()

    if(NOT is_compile_lang_generator_expression)
      # SHELL is used to avoid de-deplucation, but when process flags
      # then this tag must be removed to return real compile/linker flags.
      if(flag MATCHES "SHELL:[ ]*(.*)")
        separate_arguments(flag UNIX_COMMAND ${CMAKE_MATCH_1})
      endif()
      # Flags may be placed inside generator expression, therefore any flag
      # which is not already a generator expression must have commas converted.
      if(NOT flag MATCHES "\\\$<.*>")
        string(REPLACE "," "$<COMMA>" flag "${flag}")
      endif()
      list(APPEND tmp_list ${flag})
    endif()
  endforeach()

  set(${output} ${tmp_list} PARENT_SCOPE)
endfunction()

function(convert_list_of_flags_to_string_of_flags ptr_list_of_flags string_of_flags)
  # Convert the list to a string so we can do string replace
  # operations on it and replace the ";" list separators with a
  # whitespace so the flags are spaced out
  string(REPLACE ";"  " "  locally_scoped_string_of_flags "${${ptr_list_of_flags}}")

  # Set the output variable in the parent scope
  set(${string_of_flags} ${locally_scoped_string_of_flags} PARENT_SCOPE)
endfunction()

macro(get_property_and_add_prefix result target property prefix)
  zephyr_get_parse_args(args ${ARGN})

  if(args_STRIP_PREFIX)
    set(maybe_prefix "")
  else()
    set(maybe_prefix ${prefix})
  endif()

  get_property(target_property TARGET ${target} PROPERTY ${property})
  foreach(x ${target_property})
    list(APPEND ${result} ${maybe_prefix}${x})
  endforeach()
endmacro()

# 1.2 zephyr_library_*
#
# Zephyr libraries use CMake's library concept and a set of
# assumptions about how zephyr code is organized to cut down on
# boilerplate code.
#
# A Zephyr library can be constructed by the function zephyr_library
# or zephyr_library_named. The constructors create a CMake library
# with a name accessible through the variable ZEPHYR_CURRENT_LIBRARY.
#
# The variable ZEPHYR_CURRENT_LIBRARY should seldom be needed since
# the zephyr libraries have methods that modify the libraries. These
# methods have the signature: zephyr_library_<target-function>
#
# The methods are wrappers around the CMake target_* functions. See
# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html for
# documentation on the underlying target_* functions.
#
# The methods modify the CMake target_* API to reduce boilerplate;
#  PRIVATE is assumed
#  The target is assumed to be ZEPHYR_CURRENT_LIBRARY
#
# When a flag that is given through the zephyr_* API conflicts with
# the zephyr_library_* API then precedence will be given to the
# zephyr_library_* API. In other words, local configuration overrides
# global configuration.

# Constructor with a directory-inferred name
macro(zephyr_library)
  zephyr_library_get_current_dir_lib_name(${ZEPHYR_BASE} lib_name)
  zephyr_library_named(${lib_name})
endmacro()

# Determines what the current directory's lib name would be according to the
# provided base and writes it to the argument "lib_name"
macro(zephyr_library_get_current_dir_lib_name base lib_name)
  # Remove the prefix (/home/sebo/zephyr/driver/serial/CMakeLists.txt => driver/serial/CMakeLists.txt)
  file(RELATIVE_PATH name ${base} ${CMAKE_CURRENT_LIST_FILE})

  # Remove the filename (driver/serial/CMakeLists.txt => driver/serial)
  get_filename_component(name ${name} DIRECTORY)

  # Replace / with __ (driver/serial => driver__serial)
  string(REGEX REPLACE "/" "__" name ${name})

  set(${lib_name} ${name})
endmacro()

# Constructor with an explicitly given name.
macro(zephyr_library_named name)
  # This is a macro because we need add_library() to be executed
  # within the scope of the caller.
  set(ZEPHYR_CURRENT_LIBRARY ${name})
  add_library(${name} STATIC "")

  zephyr_append_cmake_library(${name})

  target_link_libraries(${name} PUBLIC zephyr_interface)
endmacro()

# Provides amend functionality to a Zephyr library for out-of-tree usage.
#
# When called from a Zephyr module, the corresponding zephyr library defined
# within Zephyr will be looked up.
#
# Note, in order to ensure correct library when amending, the folder structure in the
# Zephyr module must resemble the structure used in Zephyr, as example:
#
# Example: to amend the zephyr library created in
# ZEPHYR_BASE/drivers/entropy/CMakeLists.txt
# add the following file:
# ZEPHYR_MODULE/drivers/entropy/CMakeLists.txt
# with content:
# zephyr_library_amend()
# zephyr_libray_add_sources(...)
#
macro(zephyr_library_amend)
  # This is a macro because we need to ensure the ZEPHYR_CURRENT_LIBRARY and
  # following zephyr_library_* calls are executed within the scope of the
  # caller.
  if(NOT ZEPHYR_CURRENT_MODULE_DIR)
    message(FATAL_ERROR "Function only available for Zephyr modules.")
  endif()

  zephyr_library_get_current_dir_lib_name(${ZEPHYR_CURRENT_MODULE_DIR} lib_name)

  set(ZEPHYR_CURRENT_LIBRARY ${lib_name})
endmacro()

function(zephyr_link_interface interface)
  target_link_libraries(${interface} INTERFACE zephyr_interface)
endfunction()

#
# zephyr_library versions of normal CMake target_<func> functions
#
function(zephyr_library_sources source)
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(zephyr_library_include_directories)
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${ARGN})
endfunction()

function(zephyr_library_link_libraries item)
  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${item} ${ARGN})
endfunction()

function(zephyr_library_compile_definitions item)
  target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

function(zephyr_library_compile_options item)
  # The compiler is relied upon for sane behaviour when flags are in
  # conflict. Compilers generally give precedence to flags given late
  # on the command line. So to ensure that zephyr_library_* flags are
  # placed late on the command line we create a dummy interface
  # library and link with it to obtain the flags.
  #
  # Linking with a dummy interface library will place flags later on
  # the command line than the the flags from zephyr_interface because
  # zephyr_interface will be the first interface library that flags
  # are taken from.

  string(MD5 uniqueness ${item})
  set(lib_name options_interface_lib_${uniqueness})

  if (TARGET ${lib_name})
    # ${item} already added, ignoring duplicate just like CMake does
    return()
  endif()

  add_library(           ${lib_name} INTERFACE)
  target_compile_options(${lib_name} INTERFACE ${item} ${ARGN})

  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${lib_name})
endfunction()

function(zephyr_library_cc_option)
  foreach(option ${ARGV})
    string(MAKE_C_IDENTIFIER check${option} check)
    zephyr_check_compiler_flag(C ${option} ${check})

    if(${check})
      zephyr_library_compile_options(${option})
    endif()
  endforeach()
endfunction()

# Add the existing CMake library 'library' to the global list of
# Zephyr CMake libraries. This is done automatically by the
# constructor but must called explicitly on CMake libraries that do
# not use a zephyr library constructor.
function(zephyr_append_cmake_library library)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_LIBS ${library})
endfunction()

# Add the imported library 'library_name', located at 'library_path' to the
# global list of Zephyr CMake libraries.
function(zephyr_library_import library_name library_path)
  add_library(${library_name} STATIC IMPORTED GLOBAL)
  set_target_properties(${library_name}
    PROPERTIES IMPORTED_LOCATION
    ${library_path}
    )
  zephyr_append_cmake_library(${library_name})
endfunction()

# Place the current zephyr library in the application memory partition.
#
# The partition argument is the name of the partition where the library shall
# be placed.
#
# Note: Ensure the given partition has been define using
#       K_APPMEM_PARTITION_DEFINE in source code.
function(zephyr_library_app_memory partition)
  set_property(TARGET zephyr_property_target
               APPEND PROPERTY COMPILE_OPTIONS
               "-l" $<TARGET_FILE_NAME:${ZEPHYR_CURRENT_LIBRARY}> "${partition}")
endfunction()

# 1.2.1 zephyr_interface_library_*
#
# A Zephyr interface library is a thin wrapper over a CMake INTERFACE
# library. The most important responsibility of this abstraction is to
# ensure that when a user KConfig-enables a library then the header
# files of this library will be accessible to the 'app' library.
#
# This is done because when a user uses Kconfig to enable a library he
# expects to be able to include it's header files and call it's
# functions out-of-the box.
#
# A Zephyr interface library should be used when there exists some
# build information (include directories, defines, compiler flags,
# etc.) that should be applied to a set of Zephyr libraries and 'app'
# might be one of these libraries.
#
# Zephyr libraries must explicitly call
# zephyr_library_link_libraries(<interface_library>) to use this build
# information. 'app' is treated as a special case for usability
# reasons; a Kconfig option (CONFIG_APP_LINK_WITH_<interface_library>)
# should exist for each interface_library and will determine if 'app'
# links with the interface_library.
#
# This API has a constructor like the zephyr_library API has, but it
# does not have wrappers over the other cmake target functions.
macro(zephyr_interface_library_named name)
  add_library(${name} INTERFACE)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_INTERFACE_LIBS ${name})
endmacro()

# 1.3 generate_inc_*

# These functions are useful if there is a need to generate a file
# that can be included into the application at build time. The file
# can also be compressed automatically when embedding it.
#
# See tests/application_development/gen_inc_file for an example of
# usage.
function(generate_inc_file
    source_file    # The source file to be converted to hex
    generated_file # The generated file
    )
  add_custom_command(
    OUTPUT ${generated_file}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/file2hex.py
    ${ARGN} # Extra arguments are passed to file2hex.py
    --file ${source_file}
    > ${generated_file} # Does pipe redirection work on Windows?
    DEPENDS ${source_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()

function(generate_inc_file_for_gen_target
    target          # The cmake target that depends on the generated file
    source_file     # The source file to be converted to hex
    generated_file  # The generated file
    gen_target      # The generated file target we depend on
                    # Any additional arguments are passed on to file2hex.py
    )
  generate_inc_file(${source_file} ${generated_file} ${ARGN})

  # Ensure 'generated_file' is generated before 'target' by creating a
  # dependency between the two targets

  add_dependencies(${target} ${gen_target})
endfunction()

function(generate_inc_file_for_target
    target          # The cmake target that depends on the generated file
    source_file     # The source file to be converted to hex
    generated_file  # The generated file
                    # Any additional arguments are passed on to file2hex.py
    )
  # Ensure 'generated_file' is generated before 'target' by creating a
  # 'custom_target' for it and setting up a dependency between the two
  # targets

  # But first create a unique name for the custom target
  generate_unique_target_name_from_filename(${generated_file} generated_target_name)

  add_custom_target(${generated_target_name} DEPENDS ${generated_file})
  generate_inc_file_for_gen_target(${target} ${source_file} ${generated_file} ${generated_target_name} ${ARGN})
endfunction()

# 1.4. board_*
#
# This section is for extensions which control Zephyr's board runners
# from the build system. The Zephyr build system has targets for
# flashing and debugging supported boards. These are wrappers around a
# "runner" Python subpackage that is part of Zephyr's "west" tool.
#
# This section provides glue between CMake and the Python code that
# manages the runners.

function(_board_check_runner_type type) # private helper
  if (NOT (("${type}" STREQUAL "FLASH") OR ("${type}" STREQUAL "DEBUG")))
    message(FATAL_ERROR "invalid type ${type}; should be FLASH or DEBUG")
  endif()
endfunction()

# This function sets the runner for the board unconditionally.  It's
# meant to be used from application CMakeLists.txt files.
#
# NOTE: Usually board_set_xxx_ifnset() is best in board.cmake files.
#       This lets the user set the runner at cmake time, or in their
#       own application's CMakeLists.txt.
#
# Usage:
#   board_set_runner(FLASH pyocd)
#
# This would set the board's flash runner to "pyocd".
#
# In general, "type" is FLASH or DEBUG, and "runner" is the name of a
# runner.
function(board_set_runner type runner)
  _board_check_runner_type(${type})
  if (DEFINED BOARD_${type}_RUNNER)
    message(STATUS "overriding ${type} runner ${BOARD_${type}_RUNNER}; it's now ${runner}")
  endif()
  set(BOARD_${type}_RUNNER ${runner} PARENT_SCOPE)
endfunction()

# This macro is like board_set_runner(), but will only make a change
# if that runner is currently not set.
#
# See also board_set_flasher_ifnset() and board_set_debugger_ifnset().
macro(board_set_runner_ifnset type runner)
  _board_check_runner_type(${type})
  # This is a macro because set_ifndef() works at parent scope.
  # If this were a function, that would be this function's scope,
  # which wouldn't work.
  set_ifndef(BOARD_${type}_RUNNER ${runner})
endmacro()

# A convenience macro for board_set_runner(FLASH ${runner}).
macro(board_set_flasher runner)
  board_set_runner(FLASH ${runner})
endmacro()

# A convenience macro for board_set_runner(DEBUG ${runner}).
macro(board_set_debugger runner)
  board_set_runner(DEBUG ${runner})
endmacro()

# A convenience macro for board_set_runner_ifnset(FLASH ${runner}).
macro(board_set_flasher_ifnset runner)
  board_set_runner_ifnset(FLASH ${runner})
endmacro()

# A convenience macro for board_set_runner_ifnset(DEBUG ${runner}).
macro(board_set_debugger_ifnset runner)
  board_set_runner_ifnset(DEBUG ${runner})
endmacro()

# This function is intended for board.cmake files and application
# CMakeLists.txt files.
#
# Usage from board.cmake files:
#   board_runner_args(runner "--some-arg=val1" "--another-arg=val2")
#
# The build system will then ensure the command line used to
# create the runner contains:
#   --some-arg=val1 --another-arg=val2
#
# Within application CMakeLists.txt files, ensure that all calls to
# board_runner_args() are part of a macro named app_set_runner_args(),
# like this, which is defined before including the boilerplate file:
#   macro(app_set_runner_args)
#     board_runner_args(runner "--some-app-setting=value")
#   endmacro()
#
# The build system tests for the existence of the macro and will
# invoke it at the appropriate time if it is defined.
#
# Any explicitly provided settings given by this function override
# defaults provided by the build system.
function(board_runner_args runner)
  string(MAKE_C_IDENTIFIER ${runner} runner_id)
  # Note the "_EXPLICIT_" here, and see below.
  set_property(GLOBAL APPEND PROPERTY BOARD_RUNNER_ARGS_EXPLICIT_${runner_id} ${ARGN})
endfunction()

# This function is intended for internal use by
# boards/common/runner.board.cmake files.
#
# Basic usage:
#   board_finalize_runner_args(runner)
#
# This ensures the build system captures all arguments added in any
# board_runner_args() calls, and otherwise finishes registering a
# runner for use.
#
# Extended usage:
#   board_runner_args(runner "--some-arg=default-value")
#
# This provides common or default values for arguments. These are
# placed before board_runner_args() calls, so they generally take
# precedence, except for arguments which can be given multiple times
# (use these with caution).
function(board_finalize_runner_args runner)
  # If the application provided a macro to add additional runner
  # arguments, handle them.
  if(COMMAND app_set_runner_args)
    app_set_runner_args()
  endif()

  # Retrieve the list of explicitly set arguments.
  string(MAKE_C_IDENTIFIER ${runner} runner_id)
  get_property(explicit GLOBAL PROPERTY "BOARD_RUNNER_ARGS_EXPLICIT_${runner_id}")

  # Note no _EXPLICIT_ here. This property contains the final list.
  set_property(GLOBAL APPEND PROPERTY BOARD_RUNNER_ARGS_${runner_id}
    # Default arguments from the common runner file come first.
    ${ARGN}
    # Arguments explicitly given with board_runner_args() come
    # last, so they take precedence.
    ${explicit}
    )

  # Add the finalized runner to the global property list.
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_RUNNERS ${runner})
endfunction()

# 1.5. Misc.

# zephyr_check_compiler_flag is a part of Zephyr's toolchain
# infrastructure. It should be used when testing toolchain
# capabilities and it should normally be used in place of the
# functions:
#
# check_compiler_flag
# check_c_compiler_flag
# check_cxx_compiler_flag
#
# See check_compiler_flag() for API documentation as it has the same
# API.
#
# It is implemented as a wrapper on top of check_compiler_flag, which
# again wraps the CMake-builtin's check_c_compiler_flag and
# check_cxx_compiler_flag.
#
# It takes time to check for compatibility of flags against toolchains
# so we cache the capability test results in USER_CACHE_DIR (This
# caching comes in addition to the caching that CMake does in the
# build folder's CMakeCache.txt)
function(zephyr_check_compiler_flag lang option check)
  # Check if the option is covered by any hardcoded check before doing
  # an automated test.
  zephyr_check_compiler_flag_hardcoded(${lang} "${option}" check exists)
  if(exists)
    set(check ${check} PARENT_SCOPE)
    return()
  endif()

  # Locate the cache directory
  set_ifndef(
    ZEPHYR_TOOLCHAIN_CAPABILITY_CACHE_DIR
    ${USER_CACHE_DIR}/ToolchainCapabilityDatabase
    )

  # The toolchain capability database/cache is maintained as a
  # directory of files. The filenames in the directory are keys, and
  # the file contents are the values in this key-value store.

  # We need to create a unique key wrt. testing the toolchain
  # capability. This key must include everything that can affect the
  # toolchain test.
  #
  # Also, to fit the key into a filename we calculate the MD5 sum of
  # the key.

  # The 'cacheformat' must be bumped if a bug in the caching mechanism
  # is detected and all old keys must be invalidated.
  set(cacheformat 3)

  set(key_string "")
  set(key_string "${key_string}${cacheformat}_")
  set(key_string "${key_string}${TOOLCHAIN_SIGNATURE}_")
  set(key_string "${key_string}${lang}_")
  set(key_string "${key_string}${option}_")
  set(key_string "${key_string}${CMAKE_REQUIRED_FLAGS}_")

  string(MD5 key ${key_string})

  # Check the cache
  set(key_path ${ZEPHYR_TOOLCHAIN_CAPABILITY_CACHE_DIR}/${key})
  if(EXISTS ${key_path})
    file(READ
    ${key_path}   # File to be read
    key_value     # Output variable
    LIMIT 1       # Read at most 1 byte ('0' or '1')
    )

    set(${check} ${key_value} PARENT_SCOPE)
    return()
  endif()

  # Flags that start with -Wno-<warning> can not be tested by
  # check_compiler_flag, they will always pass, but -W<warning> can be
  # tested, so to test -Wno-<warning> flags we test -W<warning>
  # instead.
  if("${option}" MATCHES "-Wno-(.*)")
    set(possibly_translated_option -W${CMAKE_MATCH_1})
  else()
    set(possibly_translated_option ${option})
  endif()

  check_compiler_flag(${lang} "${possibly_translated_option}" inner_check)

  set(${check} ${inner_check} PARENT_SCOPE)

  # Populate the cache
  if(NOT (EXISTS ${key_path}))

    # This is racy. As often with race conditions, this one can easily be
    # made worse and demonstrated with a simple delay:
    #    execute_process(COMMAND "sleep" "5")
    # Delete the cache, add the sleep above and run sanitycheck with a
    # large number of JOBS. Once it's done look at the log.txt file
    # below and you will see that concurrent cmake processes created the
    # same files multiple times.

    # While there are a number of reasons why this race seems both very
    # unlikely and harmless, let's play it safe anyway and write to a
    # private, temporary file first. All modern filesystems seem to
    # support at least one atomic rename API and cmake's file(RENAME
    # ...) officially leverages that.
    string(RANDOM LENGTH 8 tempsuffix)

    file(
      WRITE
      "${key_path}_tmp_${tempsuffix}"
      ${inner_check}
      )
    file(
      RENAME
      "${key_path}_tmp_${tempsuffix}" "${key_path}"
      )

    # Populate a metadata file (only intended for trouble shooting)
    # with information about the hash, the toolchain capability
    # result, and the toolchain test.
    file(
      APPEND
      ${ZEPHYR_TOOLCHAIN_CAPABILITY_CACHE_DIR}/log.txt
      "${inner_check} ${key} ${key_string}\n"
      )
  endif()
endfunction()

function(zephyr_check_compiler_flag_hardcoded lang option check exists)
  # Various flags that are not supported for CXX may not be testable
  # because they would produce a warning instead of an error during
  # the test.  Exclude them by toolchain-specific blacklist.
  if((${lang} STREQUAL CXX) AND ("${option}" IN_LIST CXX_EXCLUDED_OPTIONS))
    set(check 0 PARENT_SCOPE)
    set(exists 1 PARENT_SCOPE)
  else()
    # There does not exist a hardcoded check for this option.
    set(exists 0 PARENT_SCOPE)
  endif()
endfunction(zephyr_check_compiler_flag_hardcoded)

# zephyr_linker_sources(<location> [SORT_KEY <sort_key>] <files>)
#
# <files> is one or more .ld formatted files whose contents will be
#    copied/included verbatim into the given <location> in the global linker.ld.
#    Preprocessor directives work inside <files>. Relative paths are resolved
#    relative to the calling file, like zephyr_sources().
# <location> is one of
#    NOINIT       Inside the noinit output section.
#    RWDATA       Inside the data output section.
#    RODATA       Inside the rodata output section.
#    ROM_START    Inside the first output section of the image. This option is
#                 currently only available on ARM Cortex-M, ARM Cortex-R,
#                 x86, ARC, and openisa_rv32m1.
#    RAM_SECTIONS Inside the RAMABLE_REGION GROUP.
#    SECTIONS     Near the end of the file. Don't use this when linking into
#                 RAMABLE_REGION, use RAM_SECTIONS instead.
# <sort_key> is an optional key to sort by inside of each location. The key must
#    be alphanumeric, and the keys are sorted alphabetically. If no key is
#    given, the key 'default' is used. Keys are case-sensitive.
#
# Use NOINIT, RWDATA, and RODATA unless they don't work for your use case.
#
# When placing into NOINIT, RWDATA, RODATA, ROM_START, the contents of the files
# will be placed inside an output section, so assume the section definition is
# already present, e.g.:
#    _mysection_start = .;
#    KEEP(*(.mysection));
#    _mysection_end = .;
#    _mysection_size = ABSOLUTE(_mysection_end - _mysection_start);
#
# When placing into SECTIONS or RAM_SECTIONS, the files must instead define
# their own output sections to achieve the same thing:
#    SECTION_PROLOGUE(.mysection,,)
#    {
#        _mysection_start = .;
#        KEEP(*(.mysection))
#        _mysection_end = .;
#    } GROUP_LINK_IN(ROMABLE_REGION)
#    _mysection_size = _mysection_end - _mysection_start;
#
# Note about the above examples: If the first example was used with RODATA, and
# the second with SECTIONS, the two examples do the same thing from a user
# perspective.
#
# Friendly reminder: Beware of the different ways the location counter ('.')
# behaves inside vs. outside section definitions.
function(zephyr_linker_sources location)
  # Set up the paths to the destination files. These files are #included inside
  # the global linker.ld.
  set(snippet_base      "${__build_dir}/include/generated")
  set(sections_path     "${snippet_base}/snippets-sections.ld")
  set(ram_sections_path "${snippet_base}/snippets-ram-sections.ld")
  set(rom_start_path    "${snippet_base}/snippets-rom-start.ld")
  set(noinit_path       "${snippet_base}/snippets-noinit.ld")
  set(rwdata_path       "${snippet_base}/snippets-rwdata.ld")
  set(rodata_path       "${snippet_base}/snippets-rodata.ld")

  # Clear destination files if this is the first time the function is called.
  get_property(cleared GLOBAL PROPERTY snippet_files_cleared)
  if (NOT DEFINED cleared)
    file(WRITE ${sections_path} "")
    file(WRITE ${ram_sections_path} "")
    file(WRITE ${rom_start_path} "")
    file(WRITE ${noinit_path} "")
    file(WRITE ${rwdata_path} "")
    file(WRITE ${rodata_path} "")
    set_property(GLOBAL PROPERTY snippet_files_cleared true)
  endif()

  # Choose destination file, based on the <location> argument.
  if ("${location}" STREQUAL "SECTIONS")
    set(snippet_path "${sections_path}")
  elseif("${location}" STREQUAL "RAM_SECTIONS")
    set(snippet_path "${ram_sections_path}")
  elseif("${location}" STREQUAL "ROM_START")
    set(snippet_path "${rom_start_path}")
  elseif("${location}" STREQUAL "NOINIT")
    set(snippet_path "${noinit_path}")
  elseif("${location}" STREQUAL "RWDATA")
    set(snippet_path "${rwdata_path}")
  elseif("${location}" STREQUAL "RODATA")
    set(snippet_path "${rodata_path}")
  else()
    message(fatal_error "Must choose valid location for linker snippet.")
  endif()

  cmake_parse_arguments(L "" "SORT_KEY" "" ${ARGN})
  set(SORT_KEY default)
  if(DEFINED L_SORT_KEY)
    set(SORT_KEY ${L_SORT_KEY})
  endif()

  foreach(file IN ITEMS ${L_UNPARSED_ARGUMENTS})
    # Resolve path.
    if(IS_ABSOLUTE ${file})
      set(path ${file})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    endif()

    if(IS_DIRECTORY ${path})
      message(FATAL_ERROR "zephyr_linker_sources() was called on a directory")
    endif()

    # Find the relative path to the linker file from the include folder.
    file(RELATIVE_PATH relpath ${ZEPHYR_BASE}/include ${path})

    # Create strings to be written into the file
    set (include_str "/* Sort key: \"${SORT_KEY}\" */#include \"${relpath}\"")

    # Add new line to existing lines, sort them, and write them back.
    file(STRINGS ${snippet_path} lines) # Get current lines (without newlines).
    list(APPEND lines ${include_str})
    list(SORT lines)
    string(REPLACE ";" "\n;" lines "${lines}") # Add newline to each line.
    file(WRITE ${snippet_path} ${lines} "\n")
  endforeach()
endfunction(zephyr_linker_sources)


# Helper function for CONFIG_CODE_DATA_RELOCATION
# Call this function with 2 arguments file and then memory location
function(zephyr_code_relocate file location)
  if(NOT IS_ABSOLUTE ${file})
    set(file ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endif()
  set_property(TARGET code_data_relocation_target
    APPEND PROPERTY COMPILE_DEFINITIONS
    "${location}:${file}")
endfunction()

# Usage:
#   check_dtc_flag("-Wtest" DTC_WARN_TEST)
#
# Writes 1 to the output variable 'ok' if
# the flag is supported, otherwise writes 0.
#
# using
function(check_dtc_flag flag ok)
  execute_process(
    COMMAND
    ${DTC} ${flag} -v
    ERROR_QUIET
    OUTPUT_QUIET
    RESULT_VARIABLE dtc_check_ret
  )
  if (dtc_check_ret EQUAL 0)
    set(${ok} 1 PARENT_SCOPE)
  else()
    set(${ok} 0 PARENT_SCOPE)
  endif()
endfunction()

########################################################
# 2. Kconfig-aware extensions
########################################################
#
# Kconfig is a configuration language developed for the Linux
# kernel. The below functions integrate CMake with Kconfig.
#

# 2.1 Misc
#
# import_kconfig(<prefix> <kconfig_fragment> [<keys>])
#
# Parse a KConfig fragment (typically with extension .config) and
# introduce all the symbols that are prefixed with 'prefix' into the
# CMake namespace. List all created variable names in the 'keys'
# output variable if present.
function(import_kconfig prefix kconfig_fragment)
  # Parse the lines prefixed with 'prefix' in ${kconfig_fragment}
  file(
    STRINGS
    ${kconfig_fragment}
    DOT_CONFIG_LIST
    REGEX "^${prefix}"
    ENCODING "UTF-8"
  )

  foreach (CONFIG ${DOT_CONFIG_LIST})
    # CONFIG could look like: CONFIG_NET_BUF=y

    # Match the first part, the variable name
    string(REGEX MATCH "[^=]+" CONF_VARIABLE_NAME ${CONFIG})

    # Match the second part, variable value
    string(REGEX MATCH "=(.+$)" CONF_VARIABLE_VALUE ${CONFIG})
    # The variable name match we just did included the '=' symbol. To just get the
    # part on the RHS we use match group 1
    set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

    if("${CONF_VARIABLE_VALUE}" MATCHES "^\"(.*)\"$") # Is surrounded by quotes
      set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})
    endif()

    set("${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)
    list(APPEND keys "${CONF_VARIABLE_NAME}")
  endforeach()

  foreach(outvar ${ARGN})
    set(${outvar} "${keys}" PARENT_SCOPE)
  endforeach()
endfunction()

########################################################
# 3. CMake-generic extensions
########################################################
#
# These functions extend the CMake API in a way that is not particular
# to Zephyr. Primarily they work around limitations in the CMake
# language to allow cleaner build scripts.

# 3.1. *_ifdef
#
# Functions for conditionally executing CMake functions with oneliners
# e.g.
#
# if(CONFIG_FFT)
#   zephyr_library_source(
#     fft_32.c
#     fft_utils.c
#     )
# endif()
#
# Becomes
#
# zephyr_source_ifdef(
#   CONFIG_FFT
#   fft_32.c
#   fft_utils.c
#   )
#
# More Generally
# "<function-name>_ifdef(CONDITION args)"
# Becomes
# """
# if(CONDITION)
#   <function-name>(args)
# endif()
# """
#
# ifdef functions are added on an as-need basis. See
# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html for
# a list of available functions.
function(add_subdirectory_ifdef feature_toggle dir)
  if(${${feature_toggle}})
    add_subdirectory(${dir})
  endif()
endfunction()

function(target_sources_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_sources(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_compile_definitions_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_compile_definitions(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_include_directories_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_include_directories(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_link_libraries_ifdef feature_toggle target item)
  if(${${feature_toggle}})
    target_link_libraries(${target} ${item} ${ARGN})
  endif()
endfunction()

function(add_compile_option_ifdef feature_toggle option)
  if(${${feature_toggle}})
    add_compile_options(${option})
  endif()
endfunction()

function(target_compile_option_ifdef feature_toggle target scope option)
  if(${feature_toggle})
    target_compile_options(${target} ${scope} ${option})
  endif()
endfunction()

function(target_cc_option_ifdef feature_toggle target scope option)
  if(${feature_toggle})
    target_cc_option(${target} ${scope} ${option})
  endif()
endfunction()

function(zephyr_library_sources_ifdef feature_toggle source)
  if(${${feature_toggle}})
    zephyr_library_sources(${source} ${ARGN})
  endif()
endfunction()

function(zephyr_library_sources_ifndef feature_toggle source)
  if(NOT ${feature_toggle})
    zephyr_library_sources(${source} ${ARGN})
  endif()
endfunction()

function(zephyr_sources_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_sources(${ARGN})
  endif()
endfunction()

function(zephyr_sources_ifndef feature_toggle)
   if(NOT ${feature_toggle})
    zephyr_sources(${ARGN})
  endif()
endfunction()

function(zephyr_cc_option_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_cc_option(${ARGN})
  endif()
endfunction()

function(zephyr_ld_option_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_ld_options(${ARGN})
  endif()
endfunction()

function(zephyr_link_libraries_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_link_libraries(${ARGN})
  endif()
endfunction()

function(zephyr_compile_options_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_compile_options(${ARGN})
  endif()
endfunction()

function(zephyr_compile_definitions_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_compile_definitions(${ARGN})
  endif()
endfunction()

function(zephyr_include_directories_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_include_directories(${ARGN})
  endif()
endfunction()

function(zephyr_library_compile_definitions_ifdef feature_toggle item)
  if(${${feature_toggle}})
    zephyr_library_compile_definitions(${item} ${ARGN})
  endif()
endfunction()

function(zephyr_library_compile_options_ifdef feature_toggle item)
  if(${${feature_toggle}})
    zephyr_library_compile_options(${item} ${ARGN})
  endif()
endfunction()

function(zephyr_link_interface_ifdef feature_toggle interface)
  if(${${feature_toggle}})
    target_link_libraries(${interface} INTERFACE zephyr_interface)
  endif()
endfunction()

function(zephyr_library_link_libraries_ifdef feature_toggle item)
  if(${${feature_toggle}})
     zephyr_library_link_libraries(${item})
  endif()
endfunction()

function(zephyr_linker_sources_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_linker_sources(${ARGN})
  endif()
endfunction()

macro(list_append_ifdef feature_toggle list)
  if(${${feature_toggle}})
    list(APPEND ${list} ${ARGN})
  endif()
endmacro()

# 3.2. *_ifndef
# See 3.1 *_ifdef
function(set_ifndef variable value)
  if(NOT ${variable})
    set(${variable} ${value} ${ARGN} PARENT_SCOPE)
  endif()
endfunction()

function(target_cc_option_ifndef feature_toggle target scope option)
  if(NOT ${feature_toggle})
    target_cc_option(${target} ${scope} ${option})
  endif()
endfunction()

function(zephyr_cc_option_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_cc_option(${ARGN})
  endif()
endfunction()

function(zephyr_compile_options_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_compile_options(${ARGN})
  endif()
endfunction()

# 3.3. *_option Compiler-compatibility checks
#
# Utility functions for silently omitting compiler flags when the
# compiler lacks support. *_cc_option was ported from KBuild, see
# cc-option in
# https://www.kernel.org/doc/Documentation/kbuild/makefiles.txt

# Writes 1 to the output variable 'ok' for the language 'lang' if
# the flag is supported, otherwise writes 0.
#
# lang must be C or CXX
#
# TODO: Support ASM
#
# Usage:
#
# check_compiler_flag(C "-Wall" my_check)
# print(my_check) # my_check is now 1
function(check_compiler_flag lang option ok)
  if(NOT DEFINED CMAKE_REQUIRED_QUIET)
    set(CMAKE_REQUIRED_QUIET 1)
  endif()

  string(MAKE_C_IDENTIFIER
    check${option}_${lang}_${CMAKE_REQUIRED_FLAGS}
    ${ok}
    )

  if(${lang} STREQUAL C)
    check_c_compiler_flag("${option}" ${${ok}})
  else()
    check_cxx_compiler_flag("${option}" ${${ok}})
  endif()

  if(${${${ok}}})
    set(ret 1)
  else()
    set(ret 0)
  endif()

  set(${ok} ${ret} PARENT_SCOPE)
endfunction()

function(target_cc_option target scope option)
  target_cc_option_fallback(${target} ${scope} ${option} "")
endfunction()

# Support an optional second option for when the first option is not
# supported.
function(target_cc_option_fallback target scope option1 option2)
  if(CONFIG_CPLUSPLUS)
    foreach(lang C CXX)
      # For now, we assume that all flags that apply to C/CXX also
      # apply to ASM.
      zephyr_check_compiler_flag(${lang} ${option1} check)
      if(${check})
        target_compile_options(${target} ${scope}
          $<$<COMPILE_LANGUAGE:${lang}>:${option1}>
          $<$<COMPILE_LANGUAGE:ASM>:${option1}>
          )
      elseif(option2)
        target_compile_options(${target} ${scope}
          $<$<COMPILE_LANGUAGE:${lang}>:${option2}>
          $<$<COMPILE_LANGUAGE:ASM>:${option2}>
          )
      endif()
    endforeach()
  else()
    zephyr_check_compiler_flag(C ${option1} check)
    if(${check})
      target_compile_options(${target} ${scope} ${option1})
    elseif(option2)
      target_compile_options(${target} ${scope} ${option2})
    endif()
  endif()
endfunction()

function(target_ld_options target scope)
  foreach(option ${ARGN})
    string(MAKE_C_IDENTIFIER check${option} check)

    set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
    zephyr_check_compiler_flag(C "" ${check})
    set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

    target_link_libraries_ifdef(${check} ${target} ${scope} ${option})
  endforeach()
endfunction()

# 3.3.1 Toolchain integration
#
# 'toolchain_parse_make_rule' is a function that parses the output of
# 'gcc -M'.
#
# The argument 'input_file' is in input parameter with the path to the
# file with the dependency information.
#
# The argument 'include_files' is an output parameter with the result
# of parsing the include files.
function(toolchain_parse_make_rule input_file include_files)
  file(READ ${input_file} input)

  # The file is formatted like this:
  # empty_file.o: misc/empty_file.c \
  # nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts \
  # nrf52840_qiaa.dtsi

  # Get rid of the backslashes
  string(REPLACE "\\" ";" input_as_list ${input})

  # Pop the first line and treat it specially
  list(GET input_as_list 0 first_input_line)
  string(FIND ${first_input_line} ": " index)
  math(EXPR j "${index} + 2")
  string(SUBSTRING ${first_input_line} ${j} -1 first_include_file)
  list(REMOVE_AT input_as_list 0)

  list(APPEND result ${first_include_file})

  # Add the other lines
  list(APPEND result ${input_as_list})

  # Strip away the newlines and whitespaces
  list(TRANSFORM result STRIP)

  set(${include_files} ${result} PARENT_SCOPE)
endfunction()

# 'check_set_linker_property' is a function that check the provided linker
# flag and only set the linker property if the check succeeds
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will check that the linker supports the flag before
# setting the property.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# TARGET: Name of target on which to add the property (commonly: linker)
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(check_set_linker_property)
  set(options APPEND)
  set(single_args TARGET)
  set(multi_args  PROPERTY)
  cmake_parse_arguments(LINKER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})

  if(LINKER_PROPERTY_APPEND)
   set(APPEND "APPEND")
  endif()

  list(GET LINKER_PROPERTY_PROPERTY 0 property)
  list(REMOVE_AT LINKER_PROPERTY_PROPERTY 0)
  set(option ${LINKER_PROPERTY_PROPERTY})

  string(MAKE_C_IDENTIFIER check${option} check)

  set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
  zephyr_check_compiler_flag(C "" ${check})
  set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

  if(${check})
    set_property(TARGET ${LINKER_PROPERTY_TARGET} ${APPEND} PROPERTY ${property} ${option})
  endif()
endfunction()

# 'set_compiler_property' is a function that sets the property for the C and
# C++ property targets used for toolchain abstraction.
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will set the property on both the compile and
# compiler-cpp targets.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(set_compiler_property)
  set(options APPEND)
  set(multi_args  PROPERTY)
  cmake_parse_arguments(COMPILER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})
  if(COMPILER_PROPERTY_APPEND)
   set(APPEND "APPEND")
   set(APPEND-CPP "APPEND")
  endif()

  set_property(TARGET compiler ${APPEND} PROPERTY ${COMPILER_PROPERTY_PROPERTY})
  set_property(TARGET compiler-cpp ${APPEND} PROPERTY ${COMPILER_PROPERTY_PROPERTY})
endfunction()

# 'check_set_compiler_property' is a function that check the provided compiler
# flag and only set the compiler or compiler-cpp property if the check succeeds
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will check that the compiler supports the flag
# before setting the property on compiler or compiler-cpp targets.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(check_set_compiler_property)
  set(options APPEND)
  set(multi_args  PROPERTY)
  cmake_parse_arguments(COMPILER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})
  if(COMPILER_PROPERTY_APPEND)
   set(APPEND "APPEND")
   set(APPEND-CPP "APPEND")
  endif()

  list(GET COMPILER_PROPERTY_PROPERTY 0 property)
  list(REMOVE_AT COMPILER_PROPERTY_PROPERTY 0)

  foreach(option ${COMPILER_PROPERTY_PROPERTY})
    if(CONFIG_CPLUSPLUS)
      zephyr_check_compiler_flag(CXX ${option} check)

      if(${check})
        set_property(TARGET compiler-cpp ${APPEND-CPP} PROPERTY ${property} ${option})
        set(APPEND-CPP "APPEND")
      endif()
    endif()

    zephyr_check_compiler_flag(C ${option} check)

    if(${check})
      set_property(TARGET compiler ${APPEND} PROPERTY ${property} ${option})
      set(APPEND "APPEND")
    endif()
  endforeach()
endfunction()


# 3.4. Debugging CMake

# Usage:
#   print(BOARD)
#
# will print: "BOARD: nrf52dk_nrf52832"
function(print arg)
  message(STATUS "${arg}: ${${arg}}")
endfunction()

# Usage:
#   assert(ZEPHYR_TOOLCHAIN_VARIANT "ZEPHYR_TOOLCHAIN_VARIANT not set.")
#
# will cause a FATAL_ERROR and print an error message if the first
# expression is false
macro(assert test comment)
  if(NOT ${test})
    message(FATAL_ERROR "Assertion failed: ${comment}")
  endif()
endmacro()

# Usage:
#   assert_not(OBSOLETE_VAR "OBSOLETE_VAR has been removed; use NEW_VAR instead")
#
# will cause a FATAL_ERROR and print an error message if the first
# expression is true
macro(assert_not test comment)
  if(${test})
    message(FATAL_ERROR "Assertion failed: ${comment}")
  endif()
endmacro()

# Usage:
#   assert_exists(CMAKE_READELF)
#
# will cause a FATAL_ERROR if there is no file or directory behind the
# variable
macro(assert_exists var)
  if(NOT EXISTS ${${var}})
    message(FATAL_ERROR "No such file or directory: ${var}: '${${var}}'")
  endif()
endmacro()

function(print_usage)
  if(NOT CMAKE_MAKE_PROGRAM)
    # Create dummy project, in order to obtain make program for correct usage printing.
    project(dummy_print_usage)
  endif()
  message("see usage:")
  string(REPLACE ";" " " BOARD_ROOT_SPACE_SEPARATED "${BOARD_ROOT}")
  string(REPLACE ";" " " SHIELD_LIST_SPACE_SEPARATED "${SHIELD_LIST}")
  execute_process(
    COMMAND
    ${CMAKE_COMMAND}
    -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DBOARD_ROOT_SPACE_SEPARATED=${BOARD_ROOT_SPACE_SEPARATED}
    -DSHIELD_LIST_SPACE_SEPARATED=${SHIELD_LIST_SPACE_SEPARATED}
    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
    -P ${ZEPHYR_BASE}/cmake/usage/usage.cmake
    )
endfunction()

# 3.5. File system management
function(check_if_directory_is_writeable dir ok)
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/dir_is_writeable.py
    ${dir}
    RESULT_VARIABLE ret
    )

  if("${ret}" STREQUAL "0")
    # The directory is write-able
    set(${ok} 1 PARENT_SCOPE)
  else()
    set(${ok} 0 PARENT_SCOPE)
  endif()
endfunction()

function(find_appropriate_cache_directory dir)
  set(env_suffix_LOCALAPPDATA   .cache)

  if(CMAKE_HOST_APPLE)
    # On macOS, ~/Library/Caches is the preferred cache directory.
    set(env_suffix_HOME Library/Caches)
  else()
    set(env_suffix_HOME .cache)
  endif()

  # Determine which env vars should be checked
  if(CMAKE_HOST_APPLE)
    set(dirs HOME)
  elseif(CMAKE_HOST_WIN32)
    set(dirs LOCALAPPDATA)
  else()
    # Assume Linux when we did not detect 'mac' or 'win'
    #
    # On Linux, freedesktop.org recommends using $XDG_CACHE_HOME if
    # that is defined and defaulting to $HOME/.cache otherwise.
    set(dirs
      XDG_CACHE_HOME
      HOME
      )
  endif()

  foreach(env_var ${dirs})
    if(DEFINED ENV{${env_var}})
      set(env_dir $ENV{${env_var}})

      set(test_user_dir ${env_dir}/${env_suffix_${env_var}})

      check_if_directory_is_writeable(${test_user_dir} ok)
      if(${ok})
        # The directory is write-able
        set(user_dir ${test_user_dir})
        break()
      else()
        # The directory was not writeable, keep looking for a suitable
        # directory
      endif()
    endif()
  endforeach()

  # Populate local_dir with a suitable directory for caching
  # files. Prefer a directory outside of the git repository because it
  # is good practice to have clean git repositories.
  if(DEFINED user_dir)
    # Zephyr's cache files go in the "zephyr" subdirectory of the
    # user's cache directory.
    set(local_dir ${user_dir}/zephyr)
  else()
    set(local_dir ${ZEPHYR_BASE}/.cache)
  endif()

  set(${dir} ${local_dir} PARENT_SCOPE)
endfunction()

function(generate_unique_target_name_from_filename filename target_name)
  get_filename_component(basename ${filename} NAME)
  string(REPLACE "." "_" x ${basename})
  string(REPLACE "@" "_" x ${x})

  string(MD5 unique_chars ${filename})

  set(${target_name} gen_${x}_${unique_chars} PARENT_SCOPE)
endfunction()

# Usage:
#   zephyr_file(<mode> <arg> ...)
#
# Zephyr file function extension.
# This function currently support the following <modes>
#
# APPLICATION_ROOT <path>: Check all paths in provided variable, and convert
#                          those paths that are defined with `-D<path>=<val>`
#                          to absolute path, relative from `APPLICATION_SOURCE_DIR`
#                          Issue an error for any relative path not specified
#                          by user with `-D<path>`
#
# returns an updated list of absolute paths
function(zephyr_file)
  set(options APPLICATION_ROOT)
  cmake_parse_arguments(FILE "${options}" "" "" ${ARGN})
  if(NOT FILE_APPLICATION_ROOT)
    message(FATAL_ERROR "No <mode> given to `zephyr_file(<mode> <args>...)` function,\n \
Please provide one of following: APPLICATION_ROOT")
  endif()

  if(FILE_APPLICATION_ROOT)
    if(NOT (${ARGC} EQUAL 2))
      math(EXPR ARGC "${ARGC} - 1")
      message(FATAL_ERROR "zephyr_file(APPLICATION_ROOT <path-variable>) takes exactly 1 argument, ${ARGC} were given")
    endif()

    # Note: user can do: `-D<var>=<relative-path>` and app can at same
    # time specify `list(APPEND <var> <abs-path>)`
    # Thus need to check and update only CACHED variables (-D<var>).
    set(CACHED_PATH $CACHE{${ARGV1}})
    foreach(path ${CACHED_PATH})
      # The cached variable is relative path, i.e. provided by `-D<var>` or
      # `set(<var> CACHE)`, so let's update current scope variable to absolute
      # path from  `APPLICATION_SOURCE_DIR`.
      if(NOT IS_ABSOLUTE ${path})
        set(abs_path ${APPLICATION_SOURCE_DIR}/${path})
        list(FIND ${ARGV1} ${path} index)
        if(NOT ${index} LESS 0)
          list(REMOVE_AT ${ARGV1} ${index})
          list(INSERT ${ARGV1} ${index} ${abs_path})
        endif()
      endif()
    endforeach()

    # Now all cached relative paths has been updated.
    # Let's check if anyone uses relative path as scoped variable, and fail
    foreach(path ${${ARGV1}})
      if(NOT IS_ABSOLUTE ${path})
        message(FATAL_ERROR
"Relative path encountered in scoped variable: ${ARGV1}, value=${path}\n \
Please adjust any `set(${ARGV1} ${path})` or `list(APPEND ${ARGV1} ${path})`\n \
to absolute path using `\${CMAKE_CURRENT_SOURCE_DIR}/${path}` or similar. \n \
Relative paths are only allowed with `-D${ARGV1}=<path>`")
      endif()
    endforeach()

    # This updates the provided argument in parent scope (callers scope)
    set(${ARGV1} ${${ARGV1}} PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   zephyr_get_targets(<directory> <types> <targets>)
#
# Get build targets for a given directory and sub-directories.
#
# This functions will traverse the build tree, starting from <directory>.
# It will read the `BUILDSYSTEM_TARGETS` for each directory in the build tree
# and return the build types matching the <types> list.
# Example of types: OBJECT_LIBRARY, STATIC_LIBRARY, INTERFACE_LIBRARY, UTILITY.
#
# returns a list of targets in <targets> matching the required <types>.
function(zephyr_get_targets directory types targets)
    get_property(sub_directories DIRECTORY ${directory} PROPERTY SUBDIRECTORIES)
    get_property(dir_targets DIRECTORY ${directory} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(dir_target ${dir_targets})
      get_property(target_type TARGET ${dir_target} PROPERTY TYPE)
      if(${target_type} IN_LIST types)
        list(APPEND ${targets} ${dir_target})
      endif()
    endforeach()

    foreach(directory ${sub_directories})
        zephyr_get_targets(${directory} "${types}" ${targets})
    endforeach()
    set(${targets} ${${targets}} PARENT_SCOPE)
endfunction()
