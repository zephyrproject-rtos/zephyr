# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(user_cache)

# Dependencies on CMake modules from the CMake distribution.
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

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
# 4. Devicetree extensions
# 4.1 dt_*
# 4.2. *_if_dt_node
# 4.3  zephyr_dt_*
# 5. Zephyr linker functions
# 5.1. zephyr_linker*
# 6 Function helper macros

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
  target_include_directories(zephyr_interface INTERFACE ${ARGV})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(zephyr_system_include_directories)
  target_include_directories(zephyr_interface SYSTEM INTERFACE ${ARGV})
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

function(zephyr_libc_link_libraries)
  set_property(TARGET zephyr_interface APPEND PROPERTY LIBC_LINK_LIBRARIES ${ARGV})
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
  zephyr_get_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_system_include_directories_for_lang_as_string lang i)
  zephyr_get_system_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_definitions_for_lang_as_string lang i)
  zephyr_get_compile_definitions_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_options_for_lang_as_string lang i)
  zephyr_get_compile_options_for_lang(${lang} list_of_flags DELIMITER " ")

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(zephyr_get_include_directories_for_lang lang i)
  zephyr_get_parse_args(args ${ARGN})
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

  if(NOT ARGN)
    set(result_output_list "-I$<JOIN:${genexp_output_list},$<SEMICOLON>-I>")
  elseif(args_STRIP_PREFIX)
    # The list has no prefix, so don't add it.
    set(result_output_list ${output_list})
  elseif(args_DELIMITER)
    set(result_output_list "-I$<JOIN:${genexp_output_list},${args_DELIMITER}-I>")
  endif()
  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_system_include_directories_for_lang lang i)
  zephyr_get_parse_args(args ${ARGN})
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

  set_ifndef(args_DELIMITER "$<SEMICOLON>")
  set(result_output_list "$<$<BOOL:${genexp_output_list}>:-isystem$<JOIN:${genexp_output_list},${args_DELIMITER}-isystem>>")

  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_definitions_for_lang lang i)
  zephyr_get_parse_args(args ${ARGN})
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_DEFINITIONS)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

  set_ifndef(args_DELIMITER "$<SEMICOLON>")
  set(result_output_list "-D$<JOIN:${genexp_output_list},${args_DELIMITER}-D>")

  set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(zephyr_get_compile_options_for_lang lang i)
  zephyr_get_parse_args(args ${ARGN})
  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

  process_flags(${lang} flags output_list)
  string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

  set_ifndef(args_DELIMITER "$<SEMICOLON>")
  set(result_output_list "$<JOIN:${genexp_output_list},${args_DELIMITER}>")

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
    if(DEFINED single_argument)
      set(${single_argument} ${x} PARENT_SCOPE)
      unset(single_argument)
    else()
      if(x STREQUAL STRIP_PREFIX)
        set(${return_dict}_STRIP_PREFIX 1 PARENT_SCOPE)
      elseif(x STREQUAL NO_SPLIT)
        set(${return_dict}_NO_SPLIT 1 PARENT_SCOPE)
      elseif(x STREQUAL DELIMITER)
        set(single_argument ${return_dict}_DELIMITER)
      endif()
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
            # Nested generator expressions are used, just substitute `$<COMPILE_LANGUAGE:${l}>` to `1`
            string(REGEX REPLACE "\\\$<COMPILE_LANGUAGE:${l}>" "1" updated_flag ${flag})
          endif()
          list(APPEND tmp_list ${updated_flag})
          break()
        endif()
      endif()
    endforeach()

    if(NOT is_compile_lang_generator_expression)
      # SHELL is used to avoid de-duplication, but when process flags
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

  # Replace : with __ (C:/zephyrproject => C____zephyrproject)
  string(REGEX REPLACE ":" "__" name ${name})

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
# zephyr_library_sources(...)
#
# It is also possible to use generator expression when amending to Zephyr
# libraries.
#
# For example, in case it is required to expose the Zephyr library's folder as
# include path then the following is possible:
# zephyr_library_amend()
# zephyr_library_include_directories($<TARGET_PROPERTY:SOURCE_DIR>)
#
# See the CMake documentation for more target properties or generator
# expressions.
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
# Note, paths passed to this function must be relative in order
# to support the library relocation feature of zephyr_code_relocate
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

  string(MD5 uniqueness "${ARGV}")
  set(lib_name options_interface_lib_${uniqueness})

  if (NOT TARGET ${lib_name})
    # Create the unique target only if it doesn't exist.
    add_library(           ${lib_name} INTERFACE)
    target_compile_options(${lib_name} INTERFACE ${item} ${ARGN})
  endif()

  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${lib_name})
endfunction()

function(zephyr_library_cc_option)
  foreach(option ${ARGV})
    string(MAKE_C_IDENTIFIER check${option} check)
    zephyr_check_compiler_flag(C ${option} ${check})

    if(${${check}})
      zephyr_library_compile_options(${option})
    endif()
  endforeach()
endfunction()

# Add the existing CMake library 'library' to the global list of
# Zephyr CMake libraries. This is done automatically by the
# constructor but must be called explicitly on CMake libraries that do
# not use a zephyr library constructor.
function(zephyr_append_cmake_library library)
  if(TARGET zephyr_prebuilt)
    message(WARNING
      "zephyr_library() or zephyr_library_named() called in Zephyr CMake "
      "application mode. `${library}` will not be treated as a Zephyr library."
      "To create a Zephyr library in Zephyr CMake kernel mode consider "
      "creating a Zephyr module. See more here: "
      "https://docs.zephyrproject.org/latest/guides/modules.html"
    )
  endif()
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
# Note: Ensure the given partition has been defined using
#       K_APPMEM_PARTITION_DEFINE in source code.
function(zephyr_library_app_memory partition)
  set_property(TARGET zephyr_property_target
               APPEND PROPERTY COMPILE_OPTIONS
               "-l" $<TARGET_FILE_NAME:${ZEPHYR_CURRENT_LIBRARY}> "${partition}")
endfunction()

# Configure a Zephyr library specific property.
#
# Usage:
#   zephyr_library_property(<property> <value>)
#
# Current Zephyr library specific properties that are supported:
# ALLOW_EMPTY <TRUE:FALSE>: Allow a Zephyr library to be empty.
#                           An empty Zephyr library will generate a CMake
#                           configure time warning unless `ALLOW_EMPTY` is TRUE.
function(zephyr_library_property)
  set(single_args "ALLOW_EMPTY")
  cmake_parse_arguments(LIB_PROP "" "${single_args}" "" ${ARGN})

  if(LIB_PROP_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "zephyr_library_property(${ARGV0} ...) given unknown arguments: ${FILE_UNPARSED_ARGUMENTS}")
  endif()

  foreach(arg ${single_args})
    if(DEFINED LIB_PROP_${arg})
      set_property(TARGET ${ZEPHYR_CURRENT_LIBRARY} PROPERTY ${arg} ${LIB_PROP_${arg}})
    endif()
  endforeach()
endfunction()

# 1.2.1 zephyr_interface_library_*
#
# A Zephyr interface library is a thin wrapper over a CMake INTERFACE
# library. The most important responsibility of this abstraction is to
# ensure that when a user KConfig-enables a library then the header
# files of this library will be accessible to the 'app' library.
#
# This is done because when a user uses Kconfig to enable a library he
# expects to be able to include its header files and call its
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
    ${ZEPHYR_BASE}/scripts/build/file2hex.py
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
# This section is for extensions related to Zephyr board handling.
#
# Zephyr board extensions current contains:
# - Board runners
# - Board revision

# Zephyr board runners:
#   Zephyr board runner extension functions control Zephyr's board runners
#   from the build system. The Zephyr build system has targets for
#   flashing and debugging supported boards. These are wrappers around a
#   "runner" Python subpackage that is part of Zephyr's "west" tool.
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
    # next, so they take precedence over the common runner file.
    ${explicit}
    # Arguments given via the CMake cache come last of all. Users
    # can provide variables in this way from the CMake command line.
    ${BOARD_RUNNER_ARGS_${runner_id}}
    )

  # Add the finalized runner to the global property list.
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_RUNNERS ${runner})
endfunction()

function(board_set_rimage_target target)
  set(RIMAGE_TARGET ${target} CACHE STRING "rimage target")
  zephyr_check_cache(RIMAGE_TARGET)
endfunction()

# Zephyr board revision:
#
# This section provides a function for revision checking.

# Usage:
#   board_check_revision(FORMAT <LETTER | NUMBER | MAJOR.MINOR.PATCH>
#                        [EXACT]
#                        [DEFAULT_REVISION <revision>]
#                        [HIGHEST_REVISION <revision>]
#   )
#
# Zephyr board extension function.
#
# This function can be used in `boards/<board>/revision.cmake` to check a user
# requested revision against available board revisions.
#
# The function will check the revision from `-DBOARD=<board>@<revision>` that
# is provided by the user according to the arguments.
# When `EXACT` is not specified, this function will set the Zephyr build system
# variable `ACTIVE_BOARD_REVISION` with the selected revision.
#
# FORMAT <LETTER | NUMBER | MAJOR.MINOR.PATCH>: Specify the revision format.
#         LETTER:             Revision format is a single letter from A - Z.
#         NUMBER:             Revision format is a single integer number.
#         MAJOR.MINOR.PATCH:  Revision format is three numbers, separated by `.`,
#                             `x.y.z`. Trailing zeroes may be omitted on the
#                             command line, which means:
#                             1.0.0 == 1.0 == 1
#
# EXACT: Revision is required to be an exact match. As example, available revisions are:
#        0.1.0 and 0.3.0, and user provides 0.2.0, then an error is reported
#        when `EXACT` is given.
#        If `EXACT` is not provided, then closest lower revision will be selected
#        as the active revision, which in the example will be `0.1.0`.
#
# DEFAULT_REVISION: Provides a default revision to use when user has not selected
#                   a revision number. If no default revision is provided then
#                   user will be printed with an error if no revision is given
#                   on the command line.
#
# HIGHEST_REVISION: Allows to specify highest valid revision for a board.
#                   This can be used to ensure that a newer board cannot be used
#                   with an older Zephyr. As example, if current board supports
#                   revisions 0.x.0-0.99.99 and 1.0.0-1.99.99, and it is expected
#                   that current board implementation will not work with board
#                   revision 2.0.0, then HIGHEST_REVISION can be set to 1.99.99,
#                   and user will be printed with an error if using
#                   `<board>@2.0.0` or higher.
#                   This field is not needed when `EXACT` is used.
#
# VALID_REVISIONS:  A list of valid revisions for this board.
#                   If this argument is not provided, then each Kconfig fragment
#                   of the form ``<board>_<revision>.conf`` in the board folder
#                   will be used as a valid revision for the board.
#
function(board_check_revision)
  set(options EXACT)
  set(single_args FORMAT DEFAULT_REVISION HIGHEST_REVISION)
  set(multi_args  VALID_REVISIONS)
  cmake_parse_arguments(BOARD_REV "${options}" "${single_args}" "${multi_args}" ${ARGN})

  string(TOUPPER ${BOARD_REV_FORMAT} BOARD_REV_FORMAT)

  if(NOT DEFINED BOARD_REVISION)
    if(DEFINED BOARD_REV_DEFAULT_REVISION)
      set(BOARD_REVISION ${BOARD_REV_DEFAULT_REVISION})
      set(BOARD_REVISION ${BOARD_REVISION} PARENT_SCOPE)
    else()
      message(FATAL_ERROR "No board revision specified, Board: `${BOARD}` \
              requires a revision. Please use: `-DBOARD=${BOARD}@<revision>`")
    endif()
  endif()

  if(DEFINED BOARD_REV_HIGHEST_REVISION)
    if(((BOARD_REV_FORMAT STREQUAL LETTER) AND
        (BOARD_REVISION STRGREATER BOARD_REV_HIGHEST_REVISION)) OR
       ((BOARD_REV_FORMAT STREQUAL NUMBER) AND
       (BOARD_REVISION GREATER BOARD_REV_HIGHEST_REVISION)) OR
       ((BOARD_REV_FORMAT MATCHES "^MAJOR\.MINOR\.PATCH$") AND
        (BOARD_REVISION VERSION_GREATER BOARD_REV_HIGHEST_REVISION))
    )
      message(FATAL_ERROR "Board revision `${BOARD_REVISION}` greater than \
              highest supported revision `${BOARD_REV_HIGHEST_REVISION}`. \
              Please specify a valid board revision.")
    endif()
  endif()

  if(BOARD_REV_FORMAT STREQUAL LETTER)
    set(revision_regex "([A-Z])")
  elseif(BOARD_REV_FORMAT STREQUAL NUMBER)
    set(revision_regex "([0-9]+)")
  elseif(BOARD_REV_FORMAT MATCHES "^MAJOR\.MINOR\.PATCH$")
    set(revision_regex "((0|[1-9][0-9]*)(\.[0-9]+)(\.[0-9]+))")
    # We allow loose <board>@<revision> typing on command line.
    # so append missing zeroes.
    if(BOARD_REVISION MATCHES "((0|[1-9][0-9]*)(\.[0-9]+)?(\.[0-9]+)?)")
      if(NOT CMAKE_MATCH_3)
        set(BOARD_REVISION ${BOARD_REVISION}.0)
        set(BOARD_REVISION ${BOARD_REVISION} PARENT_SCOPE)
      endif()
      if(NOT CMAKE_MATCH_4)
        set(BOARD_REVISION ${BOARD_REVISION}.0)
        set(BOARD_REVISION ${BOARD_REVISION} PARENT_SCOPE)
      endif()
    endif()
  else()
    message(FATAL_ERROR "Invalid format specified for \
    `board_check_revision(FORMAT <LETTER | NUMBER | MAJOR.MINOR.PATCH>)`")
  endif()

  if(NOT (BOARD_REVISION MATCHES "^${revision_regex}$"))
    message(FATAL_ERROR "Invalid revision format used for `${BOARD_REVISION}`. \
            Board `${BOARD}` uses revision format: ${BOARD_REV_FORMAT}.")
  endif()

  if(NOT DEFINED BOARD_REV_VALID_REVISIONS)
    file(GLOB revision_candidates LIST_DIRECTORIES false RELATIVE ${BOARD_DIR}
         ${BOARD_DIR}/${BOARD}_*.conf
    )
    string(REPLACE "." "_" underscore_revision_regex ${revision_regex})
    set(file_revision_regex "${BOARD}_${underscore_revision_regex}.conf")
    foreach(candidate ${revision_candidates})
      if(${candidate} MATCHES "${file_revision_regex}")
        string(REPLACE "_" "." FOUND_BOARD_REVISION ${CMAKE_MATCH_1})
        list(APPEND BOARD_REV_VALID_REVISIONS ${FOUND_BOARD_REVISION})
      endif()
    endforeach()
  endif()

  if(${BOARD_REVISION} IN_LIST BOARD_REV_VALID_REVISIONS)
    # Found exact match.
    return()
  endif()

  if(NOT BOARD_REV_EXACT)
    foreach(TEST_REVISION ${BOARD_REV_VALID_REVISIONS})
      if((BOARD_REV_FORMAT MATCHES "^MAJOR\.MINOR\.PATCH$") AND
         (${BOARD_REVISION} VERSION_GREATER_EQUAL ${TEST_REVISION}) AND
         (${TEST_REVISION} VERSION_GREATER_EQUAL "${ACTIVE_BOARD_REVISION}")
      )
        set(ACTIVE_BOARD_REVISION ${TEST_REVISION})
      elseif((BOARD_REV_FORMAT STREQUAL LETTER) AND
             (${BOARD_REVISION} STRGREATER ${TEST_REVISION}) AND
             (${TEST_REVISION} STRGREATER "${ACTIVE_BOARD_REVISION}")
      )
        set(ACTIVE_BOARD_REVISION ${TEST_REVISION})
      elseif((BOARD_REV_FORMAT STREQUAL NUMBER) AND
             (${BOARD_REVISION} GREATER ${TEST_REVISION}) AND
             (${TEST_REVISION} GREATER "${ACTIVE_BOARD_REVISION}")
      )
        set(ACTIVE_BOARD_REVISION ${TEST_REVISION})
      endif()
    endforeach()
  endif()

  if(BOARD_REV_EXACT OR NOT DEFINED ACTIVE_BOARD_REVISION)
    message(FATAL_ERROR "Board revision `${BOARD_REVISION}` for board \
            `${BOARD}` not found. Please specify a valid board revision.")
  endif()

  set(ACTIVE_BOARD_REVISION ${ACTIVE_BOARD_REVISION} PARENT_SCOPE)
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
  zephyr_check_compiler_flag_hardcoded(${lang} "${option}" _${check} exists)
  if(exists)
    set(${check} ${_${check}} PARENT_SCOPE)
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

  string(MD5 key "${key_string}")

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
    string(REPLACE "-Wno-" "-W" possibly_translated_option "${option}")
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
    # Delete the cache, add the sleep above and run twister with a
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
  # the test.  Exclude them by toolchain-specific blocklist.
  if((${lang} STREQUAL CXX) AND ("${option}" IN_LIST CXX_EXCLUDED_OPTIONS))
    set(${check} 0 PARENT_SCOPE)
    set(${exists} 1 PARENT_SCOPE)
  else()
    # There does not exist a hardcoded check for this option.
    set(${exists} 0 PARENT_SCOPE)
  endif()
endfunction(zephyr_check_compiler_flag_hardcoded)

# zephyr_linker_sources(<location> [SORT_KEY <sort_key>] <files>)
#
# <files> is one or more .ld formatted files whose contents will be
#    copied/included verbatim into the given <location> in the global linker.ld.
#    Preprocessor directives work inside <files>. Relative paths are resolved
#    relative to the calling file, like zephyr_sources().
# <location> is one of
#    NOINIT        Inside the noinit output section.
#    RWDATA        Inside the data output section.
#    RODATA        Inside the rodata output section.
#    ROM_START     Inside the first output section of the image. This option is
#                  currently only available on ARM Cortex-M, ARM Cortex-R,
#                  x86, ARC, openisa_rv32m1, and RISC-V.
#    RAM_SECTIONS  Inside the RAMABLE_REGION GROUP, not initialized.
#    DATA_SECTIONS Inside the RAMABLE_REGION GROUP, initialized.
#    RAMFUNC_SECTION Inside the RAMFUNC RAMABLE_REGION GROUP, not initialized.
#    NOCACHE_SECTION Inside the NOCACHE section
#    SECTIONS      Near the end of the file. Don't use this when linking into
#                  RAMABLE_REGION, use RAM_SECTIONS instead.
#    PINNED_RODATA Similar to RODATA but pinned in memory.
#    PINNED_RAM_SECTIONS
#                  Similar to RAM_SECTIONS but pinned in memory.
#    PINNED_DATA_SECTIONS
#                  Similar to DATA_SECTIONS but pinned in memory.
# <sort_key> is an optional key to sort by inside of each location. The key must
#    be alphanumeric, and the keys are sorted alphabetically. If no key is
#    given, the key 'default' is used. Keys are case-sensitive.
#
# Use NOINIT, RWDATA, and RODATA unless they don't work for your use case.
#
# When placing into NOINIT, RWDATA, RODATA, ROM_START, RAMFUNC_SECTION,
# NOCACHE_SECTION the contents of the files will be placed inside
# an output section, so assume the section definition is already present, e.g.:
#    _mysection_start = .;
#    KEEP(*(.mysection));
#    _mysection_end = .;
#    _mysection_size = ABSOLUTE(_mysection_end - _mysection_start);
#
# When placing into SECTIONS, RAM_SECTIONS or DATA_SECTIONS, the files must
# instead define their own output sections to achieve the same thing:
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
  set(snippet_base       "${__build_dir}/include/generated")
  set(sections_path      "${snippet_base}/snippets-sections.ld")
  set(ram_sections_path  "${snippet_base}/snippets-ram-sections.ld")
  set(data_sections_path "${snippet_base}/snippets-data-sections.ld")
  set(rom_start_path     "${snippet_base}/snippets-rom-start.ld")
  set(noinit_path        "${snippet_base}/snippets-noinit.ld")
  set(rwdata_path        "${snippet_base}/snippets-rwdata.ld")
  set(rodata_path        "${snippet_base}/snippets-rodata.ld")
  set(ramfunc_path       "${snippet_base}/snippets-ramfunc-section.ld")
  set(nocache_path       "${snippet_base}/snippets-nocache-section.ld")

  set(pinned_ram_sections_path  "${snippet_base}/snippets-pinned-ram-sections.ld")
  set(pinned_data_sections_path "${snippet_base}/snippets-pinned-data-sections.ld")
  set(pinned_rodata_path        "${snippet_base}/snippets-pinned-rodata.ld")

  # Clear destination files if this is the first time the function is called.
  get_property(cleared GLOBAL PROPERTY snippet_files_cleared)
  if (NOT DEFINED cleared)
    file(WRITE ${sections_path} "")
    file(WRITE ${ram_sections_path} "")
    file(WRITE ${data_sections_path} "")
    file(WRITE ${rom_start_path} "")
    file(WRITE ${noinit_path} "")
    file(WRITE ${rwdata_path} "")
    file(WRITE ${rodata_path} "")
    file(WRITE ${ramfunc_path} "")
    file(WRITE ${nocache_path} "")
    file(WRITE ${pinned_ram_sections_path} "")
    file(WRITE ${pinned_data_sections_path} "")
    file(WRITE ${pinned_rodata_path} "")
    set_property(GLOBAL PROPERTY snippet_files_cleared true)
  endif()

  # Choose destination file, based on the <location> argument.
  if ("${location}" STREQUAL "SECTIONS")
    set(snippet_path "${sections_path}")
  elseif("${location}" STREQUAL "RAM_SECTIONS")
    set(snippet_path "${ram_sections_path}")
  elseif("${location}" STREQUAL "DATA_SECTIONS")
    set(snippet_path "${data_sections_path}")
  elseif("${location}" STREQUAL "ROM_START")
    set(snippet_path "${rom_start_path}")
  elseif("${location}" STREQUAL "NOINIT")
    set(snippet_path "${noinit_path}")
  elseif("${location}" STREQUAL "RWDATA")
    set(snippet_path "${rwdata_path}")
  elseif("${location}" STREQUAL "RODATA")
    set(snippet_path "${rodata_path}")
  elseif("${location}" STREQUAL "RAMFUNC_SECTION")
    set(snippet_path "${ramfunc_path}")
  elseif("${location}" STREQUAL "NOCACHE_SECTION")
    set(snippet_path "${nocache_path}")
  elseif("${location}" STREQUAL "PINNED_RAM_SECTIONS")
    set(snippet_path "${pinned_ram_sections_path}")
  elseif("${location}" STREQUAL "PINNED_DATA_SECTIONS")
    set(snippet_path "${pinned_data_sections_path}")
  elseif("${location}" STREQUAL "PINNED_RODATA")
    set(snippet_path "${pinned_rodata_path}")
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
# This function may either be invoked with a list of files, or a library
# name to relocate.
#
# The FILES directive will relocate a list of files (wildcards supported)
# This directive will relocate file1. and file2.c to SRAM:
# zephyr_code_relocate(FILES file1.c file2.c LOCATION SRAM)
# Note, files can also be passed as a comma separated list to support using
# cmake generator arguments
#
# The LIBRARY directive will relocate a library
# This directive will relocate the target my_lib to SRAM:
# zephyr_code_relocate(LIBRARY my_lib SRAM)
#
# The following optional arguments are supported:
# - NOCOPY: this flag indicates that the file data does not need to be copied
#   at boot time (For example, for flash XIP).
# - PHDR [program_header]: add program header. Used on Xtensa platforms.
function(zephyr_code_relocate)
  set(options NOCOPY)
  set(single_args LIBRARY LOCATION PHDR)
  set(multi_args FILES)
  cmake_parse_arguments(CODE_REL "${options}" "${single_args}"
    "${multi_args}" ${ARGN})
  # Argument validation
  if(CODE_REL_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_code_relocate(${ARGV0} ...) "
      "given unknown arguments: ${CODE_REL_UNPARSED_ARGUMENTS}")
  endif()
  if((NOT CODE_REL_FILES) AND (NOT CODE_REL_LIBRARY))
    message(FATAL_ERROR
      "zephyr_code_relocate() requires either FILES or LIBRARY be provided")
  endif()
  if(CODE_REL_FILES AND CODE_REL_LIBRARY)
    message(FATAL_ERROR "zephyr_code_relocate() only accepts "
      "one argument between FILES and LIBRARY")
  endif()
  if(NOT CODE_REL_LOCATION)
    message(FATAL_ERROR "zephyr_code_relocate() requires a LOCATION argument")
  endif()
  if(CODE_REL_LIBRARY)
    # Use cmake generator expression to convert library to file list
    set(genex_src_dir "$<TARGET_PROPERTY:${CODE_REL_LIBRARY},SOURCE_DIR>")
    set(genex_src_list "$<TARGET_PROPERTY:${CODE_REL_LIBRARY},SOURCES>")
    set(file_list
      "${genex_src_dir}/$<JOIN:${genex_src_list},$<SEMICOLON>${genex_src_dir}/>")
  else()
    # Check if CODE_REL_FILES is a generator expression, if so leave it
    # untouched.
    string(GENEX_STRIP "${CODE_REL_FILES}" no_genex)
    if(CODE_REL_FILES STREQUAL no_genex)
      # no generator expression in CODE_REL_FILES, check if list of files
      # is absolute
      foreach(file ${CODE_REL_FILES})
        if(NOT IS_ABSOLUTE ${file})
          set(file ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endif()
        list(APPEND file_list ${file})
      endforeach()
    else()
      # Generator expression is present in file list. Leave the list untouched.
      set(file_list ${CODE_REL_FILES})
    endif()
  endif()
  if(NOT CODE_REL_NOCOPY)
    set(copy_flag COPY)
  else()
    set(copy_flag NOCOPY)
  endif()
  if(CODE_REL_PHDR)
    set(CODE_REL_LOCATION "${CODE_REL_LOCATION}\ :${CODE_REL_PHDR}")
  endif()
  # We use the "|" character to separate code relocation directives instead
  # of using CMake lists. This way, the ";" character can be reserved for
  # generator expression file lists.
  get_property(code_rel_str TARGET code_data_relocation_target
    PROPERTY COMPILE_DEFINITIONS)
  set_property(TARGET code_data_relocation_target
    PROPERTY COMPILE_DEFINITIONS
    "${code_rel_str}|${CODE_REL_LOCATION}:${copy_flag}:${file_list}")
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

# Function to round number to next power of two.
#
# Usage:
#   pow2round(<variable>)
#
# Example:
# set(test 2)
# pow2round(test)
# # test is still 2
#
# set(test 5)
# pow2round(test)
# # test is now 8
#
# Arguments:
# n   = Variable containing the number to round
function(pow2round n)
  math(EXPR x "${${n}} & (${${n}} - 1)")
  if(${x} EQUAL 0)
    return()
  endif()

  math(EXPR ${n} "${${n}} | (${${n}} >> 1)")
  math(EXPR ${n} "${${n}} | (${${n}} >> 2)")
  math(EXPR ${n} "${${n}} | (${${n}} >> 4)")
  math(EXPR ${n} "${${n}} | (${${n}} >> 8)")
  math(EXPR ${n} "${${n}} | (${${n}} >> 16)")
  math(EXPR ${n} "${${n}} | (${${n}} >> 32)")
  math(EXPR ${n} "${${n}} + 1")
  set(${n} ${${n}} PARENT_SCOPE)
endfunction()

# Function to create a build string based on BOARD, BOARD_REVISION, and BUILD
# type.
#
# This is a common function to ensure that build strings are always created
# in a uniform way.
#
# Usage:
#   zephyr_build_string(<out-variable>
#                       BOARD <board>
#                       [BOARD_REVISION <revision>]
#                       [BUILD <type>]
#   )
#
# <out-variable>:            Output variable where the build string will be returned.
# BOARD <board>:             Board name to use when creating the build string.
# BOARD_REVISION <revision>: Board revision to use when creating the build string.
# BUILD <type>:              Build type to use when creating the build string.
#
# Examples
# calling
#   zephyr_build_string(build_string BOARD alpha BUILD debug)
# will return the string `alpha_debug` in `build_string` parameter.
#
# calling
#   zephyr_build_string(build_string BOARD alpha BOARD_REVISION 1.0.0 BUILD debug)
# will return the string `alpha_1_0_0_debug` in `build_string` parameter.
#
function(zephyr_build_string outvar)
  set(single_args BOARD BOARD_REVISION BUILD)

  cmake_parse_arguments(BUILD_STR "" "${single_args}" "" ${ARGN})
  if(BUILD_STR_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "zephyr_build_string(${ARGV0} <val> ...) given unknown arguments:"
      " ${BUILD_STR_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED BUILD_STR_BOARD_REVISION AND NOT BUILD_STR_BOARD)
    message(FATAL_ERROR
      "zephyr_build_string(${ARGV0} <list> BOARD_REVISION ${BUILD_STR_BOARD_REVISION} ...)"
      " given without BOARD argument, please specify BOARD"
    )
  endif()

  set(${outvar} ${BUILD_STR_BOARD})

  if(DEFINED BUILD_STR_BOARD_REVISION)
    string(REPLACE "." "_" revision_string ${BUILD_STR_BOARD_REVISION})
    set(${outvar} "${${outvar}}_${revision_string}")
  endif()

  if(BUILD_STR_BUILD)
    set(${outvar} "${${outvar}}_${BUILD_STR_BUILD}")
  endif()

  # This updates the provided outvar in parent scope (callers scope)
  set(${outvar} ${${outvar}} PARENT_SCOPE)
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
# import_kconfig(<prefix> <kconfig_fragment> [<keys>] [TARGET <target>])
#
# Parse a KConfig fragment (typically with extension .config) and
# introduce all the symbols that are prefixed with 'prefix' into the
# CMake namespace. List all created variable names in the 'keys'
# output variable if present.
#
# <prefix>          : symbol prefix of settings in the Kconfig fragment.
# <kconfig_fragment>: absolute path to the config fragment file.
# <keys>            : output variable which will be populated with variable
#                     names loaded from the kconfig fragment.
# TARGET <target>   : set all symbols on <target> instead of adding them to the
#                     CMake namespace.
function(import_kconfig prefix kconfig_fragment)
  cmake_parse_arguments(IMPORT_KCONFIG "" "TARGET" "" ${ARGN})
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

    if(DEFINED IMPORT_KCONFIG_TARGET)
      set_property(TARGET ${IMPORT_KCONFIG_TARGET} APPEND PROPERTY "kconfigs" "${CONF_VARIABLE_NAME}")
      set_property(TARGET ${IMPORT_KCONFIG_TARGET} PROPERTY "${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}")
    else()
      set("${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)
    endif()
    list(APPEND keys "${CONF_VARIABLE_NAME}")
  endforeach()

  list(LENGTH IMPORT_KCONFIG_UNPARSED_ARGUMENTS unparsed_length)
  if(unparsed_length GREATER 0)
    if(unparsed_length GREATER 1)
    # Two mandatory arguments and one optional, anything after that is an error.
      list(GET IMPORT_KCONFIG_UNPARSED_ARGUMENTS 1 first_invalid)
      message(FATAL_ERROR "Unexpected argument after '<keys>': import_kconfig(... ${first_invalid})")
    endif()
    set(${IMPORT_KCONFIG_UNPARSED_ARGUMENTS} "${keys}" PARENT_SCOPE)
  endif()
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
function(add_subdirectory_ifdef feature_toggle source_dir)
  if(${${feature_toggle}})
    add_subdirectory(${source_dir} ${ARGN})
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

function(zephyr_sources_ifdef feature_toggle)
  if(${${feature_toggle}})
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

function(zephyr_library_include_directories_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_library_include_directories(${ARGN})
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

function(add_subdirectory_ifndef feature_toggle source_dir)
  if(NOT ${feature_toggle})
    add_subdirectory(${source_dir} ${ARGN})
  endif()
endfunction()

function(target_sources_ifndef feature_toggle target scope item)
  if(NOT ${feature_toggle})
    target_sources(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_compile_definitions_ifndef feature_toggle target scope item)
  if(NOT ${feature_toggle})
    target_compile_definitions(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_include_directories_ifndef feature_toggle target scope item)
  if(NOT ${feature_toggle})
    target_include_directories(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_link_libraries_ifndef feature_toggle target item)
  if(NOT ${feature_toggle})
    target_link_libraries(${target} ${item} ${ARGN})
  endif()
endfunction()

function(add_compile_option_ifndef feature_toggle option)
  if(NOT ${feature_toggle})
    add_compile_options(${option})
  endif()
endfunction()

function(target_compile_option_ifndef feature_toggle target scope option)
  if(NOT ${feature_toggle})
    target_compile_options(${target} ${scope} ${option})
  endif()
endfunction()

function(target_cc_option_ifndef feature_toggle target scope option)
  if(NOT ${feature_toggle})
    target_cc_option(${target} ${scope} ${option})
  endif()
endfunction()

function(zephyr_library_sources_ifndef feature_toggle source)
  if(NOT ${feature_toggle})
    zephyr_library_sources(${source} ${ARGN})
  endif()
endfunction()

function(zephyr_sources_ifndef feature_toggle)
   if(NOT ${feature_toggle})
    zephyr_sources(${ARGN})
  endif()
endfunction()

function(zephyr_cc_option_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_cc_option(${ARGN})
  endif()
endfunction()

function(zephyr_ld_option_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_ld_options(${ARGN})
  endif()
endfunction()

function(zephyr_link_libraries_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_link_libraries(${ARGN})
  endif()
endfunction()

function(zephyr_compile_options_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_compile_options(${ARGN})
  endif()
endfunction()

function(zephyr_compile_definitions_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_compile_definitions(${ARGN})
  endif()
endfunction()

function(zephyr_include_directories_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_include_directories(${ARGN})
  endif()
endfunction()

function(zephyr_library_compile_definitions_ifndef feature_toggle item)
  if(NOT ${feature_toggle})
    zephyr_library_compile_definitions(${item} ${ARGN})
  endif()
endfunction()

function(zephyr_library_include_directories_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_library_include_directories(${ARGN})
  endif()
endfunction()

function(zephyr_library_compile_options_ifndef feature_toggle item)
  if(NOT ${feature_toggle})
    zephyr_library_compile_options(${item} ${ARGN})
  endif()
endfunction()

function(zephyr_link_interface_ifndef feature_toggle interface)
  if(NOT ${feature_toggle})
    target_link_libraries(${interface} INTERFACE zephyr_interface)
  endif()
endfunction()

function(zephyr_library_link_libraries_ifndef feature_toggle item)
  if(NOT ${feature_toggle})
     zephyr_library_link_libraries(${item})
  endif()
endfunction()

function(zephyr_linker_sources_ifndef feature_toggle)
  if(NOT ${feature_toggle})
    zephyr_linker_sources(${ARGN})
  endif()
endfunction()

macro(list_append_ifndef feature_toggle list)
  if(NOT ${feature_toggle})
    list(APPEND ${list} ${ARGN})
  endif()
endmacro()

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
    "check${option}_${lang}_${CMAKE_REQUIRED_FLAGS}"
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
  if(CONFIG_CPP)
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
  zephyr_get_parse_args(args ${ARGN})
  list(REMOVE_ITEM ARGN NO_SPLIT)

  foreach(option ${ARGN})
    if(args_NO_SPLIT)
      set(option ${ARGN})
    endif()
    string(JOIN "" check_identifier "check" ${option})
    string(MAKE_C_IDENTIFIER ${check_identifier} check)

    set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    string(JOIN " " CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} ${option})
    zephyr_check_compiler_flag(C "" ${check})
    set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

    target_link_libraries_ifdef(${check} ${target} ${scope} ${option})

    if(args_NO_SPLIT)
      break()
    endif()
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
  file(STRINGS ${input_file} input)

  # The file is formatted like this:
  # empty_file.o: misc/empty_file.c \
  # nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts \
  # nrf52840_qiaa.dtsi

  # The dep file will contain `\` for line continuation.
  # This results in `\;` which is then treated a the char `;` instead of
  # the element separator, so let's get the pure `;` back.
  string(REPLACE "\;" ";" input_as_list ${input})

  # Pop the first line and treat it specially
  list(POP_FRONT input_as_list first_input_line)
  string(FIND ${first_input_line} ": " index)
  math(EXPR j "${index} + 2")
  string(SUBSTRING ${first_input_line} ${j} -1 first_include_file)

  # Remove whitespace before and after filename and convert to CMake path.
  string(STRIP "${first_include_file}" first_include_file)
  file(TO_CMAKE_PATH "${first_include_file}" first_include_file)
  set(result "${first_include_file}")

  # Remove whitespace before and after filename and convert to CMake path.
  foreach(file ${input_as_list})
    string(STRIP "${file}" file)
    file(TO_CMAKE_PATH "${file}" file)
    list(APPEND result "${file}")
  endforeach()

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

  if(${${check}})
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
# To test flags together, such as '-Wformat -Wformat-security', an option group
# can be specified by using shell-like quoting along with a 'SHELL:' prefix.
# The 'SHELL:' prefix will be dropped before testing, so that
# '"SHELL:-Wformat -Wformat-security"' becomes '-Wformat -Wformat-security' for
# testing.
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
    if(${option} MATCHES "^SHELL:")
      string(REGEX REPLACE "^SHELL:" "" option ${option})
      separate_arguments(option UNIX_COMMAND ${option})
    endif()

    if(CONFIG_CPP)
      zephyr_check_compiler_flag(CXX "${option}" check)

      if(${check})
        set_property(TARGET compiler-cpp ${APPEND-CPP} PROPERTY ${property} ${option})
        set(APPEND-CPP "APPEND")
      endif()
    endif()

    zephyr_check_compiler_flag(C "${option}" check)

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

# 3.5. File system management
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
# This function currently supports the following <modes>
#
# APPLICATION_ROOT <path>: Check all paths in provided variable, and convert
#                          those paths that are defined with `-D<path>=<val>`
#                          to absolute path, relative from `APPLICATION_SOURCE_DIR`
#                          Issue an error for any relative path not specified
#                          by user with `-D<path>`
#
# returns an updated list of absolute paths
#
# CONF_FILES <path>: Find all configuration files in path and return them in a
#                    list. Configuration files will be:
#                    - DTS:       Overlay files (.overlay)
#                    - Kconfig:   Config fragments (.conf)
#                    The conf file search will return existing configuration
#                    files for the current board.
#                    CONF_FILES takes the following additional arguments:
#                    BOARD <board>:             Find configuration files for specified board.
#                    BOARD_REVISION <revision>: Find configuration files for specified board
#                                               revision. Requires BOARD to be specified.
#
#                                               If no board is given the current BOARD and
#                                               BOARD_REVISION will be used.
#
#                    DTS <list>:   List to append DTS overlay files in <path> to
#                    KCONF <list>: List to append Kconfig fragment files in <path> to
#                    BUILD <type>: Build type to include for search.
#                                  For example:
#                                  BUILD debug, will look for <board>_debug.conf
#                                  and <board>_debug.overlay, instead of <board>.conf
#
function(zephyr_file)
  set(file_options APPLICATION_ROOT CONF_FILES)
  if((ARGC EQUAL 0) OR (NOT (ARGV0 IN_LIST file_options)))
    message(FATAL_ERROR "No <mode> given to `zephyr_file(<mode> <args>...)` function,\n \
Please provide one of following: APPLICATION_ROOT, CONF_FILES")
  endif()

  if(${ARGV0} STREQUAL APPLICATION_ROOT)
    set(single_args APPLICATION_ROOT)
  elseif(${ARGV0} STREQUAL CONF_FILES)
    set(single_args CONF_FILES BOARD BOARD_REVISION DTS KCONF BUILD)
  endif()

  cmake_parse_arguments(FILE "" "${single_args}" "" ${ARGN})
  if(FILE_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "zephyr_file(${ARGV0} <val> ...) given unknown arguments: ${FILE_UNPARSED_ARGUMENTS}")
  endif()


  if(FILE_APPLICATION_ROOT)
    # Note: user can do: `-D<var>=<relative-path>` and app can at same
    # time specify `list(APPEND <var> <abs-path>)`
    # Thus need to check and update only CACHED variables (-D<var>).
    set(CACHED_PATH $CACHE{${FILE_APPLICATION_ROOT}})
    foreach(path ${CACHED_PATH})
      # The cached variable is relative path, i.e. provided by `-D<var>` or
      # `set(<var> CACHE)`, so let's update current scope variable to absolute
      # path from  `APPLICATION_SOURCE_DIR`.
      if(NOT IS_ABSOLUTE ${path})
        set(abs_path ${APPLICATION_SOURCE_DIR}/${path})
        list(FIND ${FILE_APPLICATION_ROOT} ${path} index)
        if(NOT ${index} LESS 0)
          list(REMOVE_AT ${FILE_APPLICATION_ROOT} ${index})
          list(INSERT ${FILE_APPLICATION_ROOT} ${index} ${abs_path})
        endif()
      endif()
    endforeach()

    # Now all cached relative paths has been updated.
    # Let's check if anyone uses relative path as scoped variable, and fail
    foreach(path ${${FILE_APPLICATION_ROOT}})
      if(NOT IS_ABSOLUTE ${path})
        message(FATAL_ERROR
"Relative path encountered in scoped variable: ${FILE_APPLICATION_ROOT}, value=${path}\n \
Please adjust any `set(${FILE_APPLICATION_ROOT} ${path})` or `list(APPEND ${FILE_APPLICATION_ROOT} ${path})`\n \
to absolute path using `\${CMAKE_CURRENT_SOURCE_DIR}/${path}` or similar. \n \
Relative paths are only allowed with `-D${ARGV1}=<path>`")
      endif()
    endforeach()

    # This updates the provided argument in parent scope (callers scope)
    set(${FILE_APPLICATION_ROOT} ${${FILE_APPLICATION_ROOT}} PARENT_SCOPE)
  endif()

  if(FILE_CONF_FILES)
    if(DEFINED FILE_BOARD_REVISION AND NOT FILE_BOARD)
        message(FATAL_ERROR
          "zephyr_file(${ARGV0} <path> BOARD_REVISION ${FILE_BOARD_REVISION} ...)"
          " given without BOARD argument, please specify BOARD"
        )
    endif()

    if(NOT DEFINED FILE_BOARD)
      # Defaulting to system wide settings when BOARD is not given as argument
      set(FILE_BOARD ${BOARD})
      if(DEFINED BOARD_REVISION)
        set(FILE_BOARD_REVISION ${BOARD_REVISION})
      endif()
    endif()

    zephyr_build_string(filename
                        BOARD ${FILE_BOARD}
                        BUILD ${FILE_BUILD}
    )
    set(filename_list ${filename})

    zephyr_build_string(filename
                        BOARD ${FILE_BOARD}
                        BOARD_REVISION ${FILE_BOARD_REVISION}
                        BUILD ${FILE_BUILD}
    )
    list(APPEND filename_list ${filename})
    list(REMOVE_DUPLICATES filename_list)

    if(FILE_DTS)
      foreach(filename ${filename_list})
        if(EXISTS ${FILE_CONF_FILES}/${filename}.overlay)
          list(APPEND ${FILE_DTS} ${FILE_CONF_FILES}/${filename}.overlay)
        endif()
      endforeach()

      # This updates the provided list in parent scope (callers scope)
      set(${FILE_DTS} ${${FILE_DTS}} PARENT_SCOPE)
    endif()

    if(FILE_KCONF)
      foreach(filename ${filename_list})
        if(EXISTS ${FILE_CONF_FILES}/${filename}.conf)
          list(APPEND ${FILE_KCONF} ${FILE_CONF_FILES}/${filename}.conf)
        endif()
      endforeach()

      # This updates the provided list in parent scope (callers scope)
      set(${FILE_KCONF} ${${FILE_KCONF}} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Usage:
#   zephyr_file_copy(<oldname> <newname> [ONLY_IF_DIFFERENT])
#
# Zephyr file copy extension.
# This function is similar to CMake function
# 'file(COPY_FILE <oldname> <newname> [ONLY_IF_DIFFERENT])'
# introduced with CMake 3.21.
#
# Because the minimal required CMake version with Zephyr is 3.20, this function
# is not guaranteed to be available.
#
# When using CMake version 3.21 or newer 'zephyr_file_copy()' simply calls
# 'file(COPY_FILE...)' directly.
# When using CMake version 3.20, the implementation will execute using CMake
# for running command line tool in a subprocess for identical functionality.
function(zephyr_file_copy oldname newname)
  set(options ONLY_IF_DIFFERENT)
  cmake_parse_arguments(ZEPHYR_FILE_COPY "${options}" "" "" ${ARGN})

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21.0)
    if(ZEPHYR_FILE_COPY_ONLY_IF_DIFFERENT)
      set(copy_file_options ONLY_IF_DIFFERENT)
    endif()
    file(COPY_FILE ${oldname} ${newname} ${copy_file_options})
  else()
    if(ZEPHYR_FILE_COPY_ONLY_IF_DIFFERENT)
      set(copy_file_command copy_if_different)
    else()
      set(copy_file_command copy)
    endif()
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E ${copy_file_command} ${oldname} ${newname}
    )
  endif()
endfunction()

# Usage:
#   zephyr_string(<mode> <out-var> <input> ...)
#
# Zephyr string function extension.
# This function extends the CMake string function by providing additional
# manipulation arguments to CMake string.
#
# SANITIZE: Ensure that the output string does not contain any special
#           characters. Special characters, such as -, +, =, $, etc. are
#           converted to underscores '_'.
#
# SANITIZE TOUPPER: Ensure that the output string does not contain any special
#                   characters. Special characters, such as -, +, =, $, etc. are
#                   converted to underscores '_'.
#                   The sanitized string will be returned in UPPER case.
#
# returns the updated string
function(zephyr_string)
  set(options SANITIZE TOUPPER)
  cmake_parse_arguments(ZEPHYR_STRING "${options}" "" "" ${ARGN})

  if (NOT ZEPHYR_STRING_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Function zephyr_string() called without a return variable")
  endif()

  list(GET ZEPHYR_STRING_UNPARSED_ARGUMENTS 0 return_arg)
  list(REMOVE_AT ZEPHYR_STRING_UNPARSED_ARGUMENTS 0)

  list(JOIN ZEPHYR_STRING_UNPARSED_ARGUMENTS "" work_string)

  if(ZEPHYR_STRING_SANITIZE)
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" work_string ${work_string})
  endif()

  if(ZEPHYR_STRING_TOUPPER)
    string(TOUPPER ${work_string} work_string)
  endif()

  set(${return_arg} ${work_string} PARENT_SCOPE)
endfunction()

# Usage:
#    zephyr_list(TRANSFORM <list> <ACTION>
#                [OUTPUT_VARIABLE <output variable])
#
# Example:
#
#    zephyr_list(TRANSFORM my_input_var NORMALIZE_PATHS
#                OUTPUT_VARIABLE my_input_as_list)
#
# Like CMake's list(TRANSFORM ...). This is intended as a placeholder
# for storing current and future Zephyr-related extensions for list
# processing.
#
# <ACTION>: This currently must be NORMALIZE_PATHS. This action
#           converts the argument list <list> to a ;-list with
#           CMake path names, after passing its contents through
#           a configure_file() transformation. The input list
#           may be whitespace- or semicolon-separated.
#
# OUTPUT_VARIABLE: the result is normally stored in place, but
#                  an alternative variable to store the result
#                  can be provided with this.
function(zephyr_list transform list_var action)
  # Parse arguments.
  if(NOT "${transform}" STREQUAL "TRANSFORM")
    message(FATAL_ERROR "the first argument must be TRANSFORM")
  endif()
  if(NOT "${action}" STREQUAL "NORMALIZE_PATHS")
    message(FATAL_ERROR "the third argument must be NORMALIZE_PATHS")
  endif()
  set(single_args OUTPUT_VARIABLE)
  cmake_parse_arguments(ZEPHYR_LIST "" "${single_args}" "" ${ARGN})
  if(DEFINED ZEPHYR_LIST_OUTPUT_VARIABLE)
    set(out_var ${ZEPHYR_LIST_OUTPUT_VARIABLE})
  else()
    set(out_var ${list_var})
  endif()
  set(input ${${list_var}})

  # Perform the transformation.
  set(ret)
  string(CONFIGURE "${input}" input_expanded)
  string(REPLACE " " ";" input_raw_list "${input_expanded}")
  foreach(file ${input_raw_list})
    file(TO_CMAKE_PATH "${file}" cmake_path_file)
    list(APPEND ret ${cmake_path_file})
  endforeach()

  set(${out_var} ${ret} PARENT_SCOPE)
endfunction()

# Usage:
#   zephyr_get(<variable>)
#   zephyr_get(<variable> SYSBUILD [LOCAL|GLOBAL])
#
# Return the value of <variable> as local scoped variable of same name.
#
# zephyr_get() is a common function to provide a uniform way of supporting
# build settings that can be set from sysbuild, CMakeLists.txt, CMake cache, or
# in environment.
#
# The order of precedence for variables defined in multiple scopes:
# - Sysbuild defined when sysbuild is used.
#   Sysbuild variables can be defined as global or local to specific image.
#   Examples:
#   - BOARD is considered a global sysbuild cache variable
#   - blinky_BOARD is considered a local sysbuild cache variable only for the
#     blinky image.
#   If no sysbuild scope is specified, GLOBAL is assumed.
# - CMake cache, set by `-D<var>=<value>` or `set(<var> <val> CACHE ...)
# - Environment
# - Locally in CMakeLists.txt before 'find_package(Zephyr)'
#
# For example, if ZEPHYR_TOOLCHAIN_VARIANT is set in environment but locally
# overridden by setting ZEPHYR_TOOLCHAIN_VARIANT directly in the CMake cache
# using `-DZEPHYR_TOOLCHAIN_VARIANT=<val>`, then the value from the cache is
# returned.
function(zephyr_get variable)
  cmake_parse_arguments(GET_VAR "" "SYSBUILD" "" ${ARGN})

  if(DEFINED GET_VAR_SYSBUILD)
    if(NOT (${GET_VAR_SYSBUILD} STREQUAL "GLOBAL" OR
            ${GET_VAR_SYSBUILD} STREQUAL "LOCAL")
    )
      message(FATAL_ERROR "zephyr_get(... SYSBUILD) requires GLOBAL or LOCAL.")
    endif()
  else()
    set(GET_VAR_SYSBUILD "GLOBAL")
  endif()

  if(SYSBUILD)
    get_property(sysbuild_name TARGET sysbuild_cache PROPERTY SYSBUILD_NAME)
    get_property(sysbuild_main_app TARGET sysbuild_cache PROPERTY SYSBUILD_MAIN_APP)
    get_property(sysbuild_${variable} TARGET sysbuild_cache PROPERTY ${sysbuild_name}_${variable})
    if(NOT DEFINED sysbuild_${variable} AND
       (${GET_VAR_SYSBUILD} STREQUAL "GLOBAL" OR sysbuild_main_app)
    )
      get_property(sysbuild_${variable} TARGET sysbuild_cache PROPERTY ${variable})
    endif()
  endif()

  if(DEFINED sysbuild_${variable})
    set(${variable} ${sysbuild_${variable}} PARENT_SCOPE)
  elseif(DEFINED CACHE{${variable}})
    set(${variable} $CACHE{${variable}} PARENT_SCOPE)
  elseif(DEFINED ENV{${variable}})
    set(${variable} $ENV{${variable}} PARENT_SCOPE)
    # Set the environment variable in CMake cache, so that a build invocation
    # triggering a CMake rerun doesn't rely on the environment variable still
    # being available / have identical value.
    set(${variable} $ENV{${variable}} CACHE INTERNAL "")

    if(DEFINED ${variable} AND NOT "${${variable}}" STREQUAL "$ENV{${variable}}")
      # Variable exists as a local scoped variable, defined in a CMakeLists.txt
      # file, however it is also set in environment.
      # This might be a surprise to the user, so warn about it.
      message(WARNING "environment variable '${variable}' is hiding local "
                      "variable of same name.\n"
                      "Environment value (in use): $ENV{${variable}}\n"
                      "Local scope value (hidden): ${${variable}}\n"
      )
    endif()
  endif()
endfunction(zephyr_get variable)

# Usage:
#   zephyr_check_cache(<variable> [REQUIRED])
#
# Check the current CMake cache for <variable> and warn the user if the value
# is being modified.
#
# This can be used to ensure the user does not accidentally try to change
# Zephyr build variables, such as:
# - BOARD
# - SHIELD
#
# variable: Name of <variable> to check and set, for example BOARD.
# REQUIRED: Optional flag. If specified, then an unset <variable> will be
#           treated as an error.
# WATCH: Optional flag. If specified, watch the variable and print a warning if
#        the variable is later being changed.
#
# Details:
#   <variable> can be set by 3 sources.
#   - Using CMake argument, -D<variable>
#   - Using an environment variable
#   - In the project CMakeLists.txt before `find_package(Zephyr)`.
#
#   CLI has the highest precedence, then comes environment variables,
#   and then finally CMakeLists.txt.
#
#   The value defined on the first CMake invocation will be stored in the CMake
#   cache as CACHED_<variable>. This allows the Zephyr build system to detect
#   when a user reconfigures a sticky variable.
#
#   A user can ignore all the precedence rules if the same source is always used
#   E.g. always specifies -D<variable>= on the command line,
#   always has an environment <variable> set, or always has a set(<variable> foo)
#   line in his CMakeLists.txt and avoids mixing sources.
#
#   The selected <variable> can be accessed through the variable '<variable>' in
#   following Zephyr CMake code.
#
#   If the user tries to change <variable> to a new value, then a warning will
#   be printed, and the previously cached value (CACHED_<variable>) will be
#   used, as it has precedence.
#
#   Together with the warning, user is informed that in order to change
#   <variable> the build directory must be cleaned.
#
function(zephyr_check_cache variable)
  cmake_parse_arguments(CACHE_VAR "REQUIRED;WATCH" "" "" ${ARGN})
  string(TOLOWER ${variable} variable_text)
  string(REPLACE "_" " " variable_text ${variable_text})

  get_property(cached_value CACHE ${variable} PROPERTY VALUE)

  # If the build has already been configured in an earlier CMake invocation,
  # then CACHED_${variable} is set. The CACHED_${variable} setting takes
  # precedence over any user or CMakeLists.txt input.
  # If we detect that user tries to change the setting, then print a warning
  # that a pristine build is needed.

  # If user uses -D<variable>=<new_value>, then cli_argument will hold the new
  # value, otherwise cli_argument will hold the existing (old) value.
  set(cli_argument ${cached_value})
  if(cli_argument STREQUAL CACHED_${variable})
    # The is no changes to the <variable> value.
    unset(cli_argument)
  endif()

  set(app_cmake_lists ${${variable}})
  if(cached_value STREQUAL ${variable})
    # The app build scripts did not set a default, The variable we are
    # reading is the cached value from the CLI
    unset(app_cmake_lists)
  endif()

  if(DEFINED CACHED_${variable})
    # Warn the user if it looks like he is trying to change the variable
    # without cleaning first
    if(cli_argument)
      if(NOT ((CACHED_${variable} STREQUAL cli_argument) OR (${variable}_DEPRECATED STREQUAL cli_argument)))
        message(WARNING "The build directory must be cleaned pristinely when "
                        "changing ${variable_text},\n"
                        "Current value=\"${CACHED_${variable}}\", "
                        "Ignored value=\"${cli_argument}\"")
      endif()
    endif()

    if(CACHED_${variable})
      set(${variable} ${CACHED_${variable}} PARENT_SCOPE)
      set(${variable} ${CACHED_${variable}})
      # This resets the user provided value with previous (working) value.
      set(${variable} ${CACHED_${variable}} CACHE STRING "Selected ${variable_text}" FORCE)
    else()
      unset(${variable} PARENT_SCOPE)
      unset(${variable} CACHE)
    endif()
  else()
    zephyr_get(${variable})
  endif()

  if(${CACHE_VAR_REQUIRED} AND NOT DEFINED ${variable})
    message(FATAL_ERROR "${variable} is not being defined on the CMake command-line,"
                        " in the environment or by the app."
    )
  endif()

  if(DEFINED ${variable})
    # Store the specified variable in parent scope and the cache
    set(${variable} ${${variable}} PARENT_SCOPE)
    set(${variable} ${${variable}} CACHE STRING "Selected ${variable_text}")
  endif()
  set(CACHED_${variable} ${${variable}} CACHE STRING "Selected ${variable_text}")

  if(CACHE_VAR_WATCH)
    # The variable is now set to its final value.
    zephyr_boilerplate_watch(${variable})
  endif()
endfunction(zephyr_check_cache variable)


# Usage:
#   zephyr_boilerplate_watch(SOME_BOILERPLATE_VAR)
#
# Inform the build system that SOME_BOILERPLATE_VAR, a variable
# handled in the Zephyr package's boilerplate code, is now fixed and
# should no longer be changed.
#
# This function uses variable_watch() to print a noisy warning
# if the variable is set after it returns.
function(zephyr_boilerplate_watch variable)
  variable_watch(${variable} zephyr_variable_set_too_late)
endfunction()

function(zephyr_variable_set_too_late variable access value current_list_file)
  if (access STREQUAL "MODIFIED_ACCESS")
    message(WARNING
"
   **********************************************************************
   *
   *                    WARNING
   *
   * CMake variable ${variable} set to \"${value}\" in:
   *     ${current_list_file}
   *
   * This is too late to make changes! The change was ignored.
   *
   * Hint: ${variable} must be set before calling find_package(Zephyr ...).
   *
   **********************************************************************
")
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

# Usage:
#   test_sysbuild([REQUIRED])
#
# Test that current sample is invoked through sysbuild.
#
# This function tests that current CMake configure was invoked through sysbuild.
# If CMake configure was not invoked through sysbuild, then a warning is printed
# to the user. The warning can be upgraded to an error by setting `REQUIRED` as
# argument the `test_sysbuild()`.
#
# This function allows samples that are multi-image samples by nature to ensure
# all samples are correctly built together.
function(test_sysbuild)
  cmake_parse_arguments(TEST_SYSBUILD "REQUIRED" "" "" ${ARGN})

  if(TEST_SYSBUILD_REQUIRED)
    set(message_mode FATAL_ERROR)
  else()
    set(message_mode WARNING)
  endif()

  if(NOT SYSBUILD)
    message(${message_mode}
            "Project '${PROJECT_NAME}' is designed for sysbuild.\n"
            "For correct user-experiences, please build '${PROJECT_NAME}' "
            "using sysbuild."
    )
  endif()
endfunction()

# Usage:
#   target_byproducts(TARGET <target> BYPRODUCTS <file> [<file>...])
#
# Specify additional BYPRODUCTS that this target produces.
#
# This function allows the build system to specify additional byproducts to
# target created with `add_executable()`. When linking an executable the linker
# may produce additional files, like map files. Those files are not known to the
# build system. This function makes it possible to describe such additional
# byproducts in an easy manner.
function(target_byproducts)
  cmake_parse_arguments(TB "" "TARGET" "BYPRODUCTS" ${ARGN})

  if(NOT DEFINED TB_TARGET)
    message(FATAL_ERROR "target_byproducts() missing parameter: TARGET <target>")
  endif()

  add_custom_command(TARGET ${TB_TARGET}
                     POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo ""
                     BYPRODUCTS ${TB_BYPRODUCTS}
                     COMMENT "Logical command for additional byproducts on target: ${TB_TARGET}"
  )
endfunction()

########################################################
# 4. Devicetree extensions
########################################################
# 4.1. dt_*
#
# The following methods are for retrieving devicetree information in CMake.
#
# Notes:
#
# - In CMake, we refer to the nodes using the node's path, therefore
#   there is no dt_path(...) function for obtaining a node identifier
#   like there is in the C devicetree.h API.
#
# - As another difference from the C API, you can generally use an
#   alias at the beginning of a path interchangeably with the full
#   path to the aliased node in these functions. The usage comments
#   will make this clear in each case.

# Usage:
#   dt_nodelabel(<var> NODELABEL <label>)
#
# Function for retrieving the node path for the node having nodelabel
# <label>.
#
# Example devicetree fragment:
#
#   / {
#           soc {
#                   nvic: interrupt-controller@e000e100  { ... };
#           };
#   };
#
# Example usage:
#
#   # Sets 'nvic_path' to "/soc/interrupt-controller@e000e100"
#   dt_nodelabel(nvic_path NODELABEL "nvic")
#
# The node's path will be returned in the <var> parameter.
# <var> will be undefined if node does not exist.
#
# <var>              : Return variable where the node path will be stored
# NODELABEL <label>  : Node label
function(dt_nodelabel var)
  set(req_single_args "NODELABEL")
  cmake_parse_arguments(DT_LABEL "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_nodelabel(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_LABEL_${arg})
      message(FATAL_ERROR "dt_nodelabel(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  get_target_property(${var} devicetree_target "DT_NODELABEL|${DT_LABEL_NODELABEL}")
  if(${${var}} STREQUAL ${var}-NOTFOUND)
    set(${var})
  endif()

  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# Usage:
#   dt_alias(<var> PROPERTY <prop>)
#
# Get a node path for an /aliases node property.
#
# Example usage:
#
#   # The full path to the 'led0' alias is returned in 'path'.
#   dt_alias(path PROPERTY "led0")
#
#   # The variable 'path' will be left undefined for a nonexistent
#   # alias "does-not-exist".
#   dt_alias(path PROPERTY "does-not-exist")
#
# The node's path will be returned in the <var> parameter. The
# variable will be left undefined if the alias does not exist.
#
# <var>           : Return variable where the node path will be stored
# PROPERTY <prop> : The alias to check
function(dt_alias var)
  set(req_single_args "PROPERTY")
  cmake_parse_arguments(DT_ALIAS "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_alias(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_ALIAS_${arg})
      message(FATAL_ERROR "dt_alias(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  get_target_property(${var} devicetree_target "DT_ALIAS|${DT_ALIAS_PROPERTY}")
  if(${${var}} STREQUAL ${var}-NOTFOUND)
    set(${var})
  endif()

  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# Usage:
#   dt_node_exists(<var> PATH <path>)
#
# Tests whether a node with path <path> exists in the devicetree.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# The result of the check, either TRUE or FALSE, will be returned in
# the <var> parameter.
#
# <var>       : Return variable where the check result will be returned
# PATH <path> : Node path
function(dt_node_exists var)
  set(req_single_args "PATH")
  cmake_parse_arguments(DT_NODE "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_node_exists(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_NODE_${arg})
      message(FATAL_ERROR "dt_node_exists(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  dt_path_internal(canonical "${DT_NODE_PATH}")
  if (DEFINED canonical)
    set(${var} TRUE PARENT_SCOPE)
  else()
    set(${var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   dt_node_has_status(<var> PATH <path> STATUS <status>)
#
# Tests whether <path> refers to a node which:
# - exists in the devicetree, and
# - has a status property matching the <status> argument
#   (a missing status or an “ok” status is treated as if it
#    were “okay” instead)
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# The result of the check, either TRUE or FALSE, will be returned in
# the <var> parameter.
#
# <var>           : Return variable where the check result will be returned
# PATH <path>     : Node path
# STATUS <status> : Status to check
function(dt_node_has_status var)
  set(req_single_args "PATH;STATUS")
  cmake_parse_arguments(DT_NODE "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_node_has_status(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_NODE_${arg})
      message(FATAL_ERROR "dt_node_has_status(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  dt_path_internal(canonical ${DT_NODE_PATH})
  if(NOT DEFINED canonical)
    set(${var} FALSE PARENT_SCOPE)
    return()
  endif()

  dt_prop(${var} PATH ${canonical} PROPERTY status)

  if(NOT DEFINED ${var} OR "${${var}}" STREQUAL ok)
    set(${var} okay)
  endif()

  if(${var} STREQUAL ${DT_NODE_STATUS})
    set(${var} TRUE PARENT_SCOPE)
  else()
    set(${var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#
#   dt_prop(<var> PATH <path> PROPERTY <prop> [INDEX <idx>])
#
# Get a devicetree property value. The value will be returned in the
# <var> parameter.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# This function currently only supports properties with the following
# devicetree binding types: string, int, boolean, array, uint8-array,
# string-array, path.
#
# For array valued properties (including uint8-array and
# string-array), the entire array is returned as a CMake list unless
# INDEX is given. If INDEX is given, just the array element at index
# <idx> is returned.
#
# The property value will be returned in the <var> parameter if the
# node exists and has a property <prop> with one of the above types.
# <var> will be undefined otherwise.
#
# To test if the property is defined before using it, use DEFINED on
# the return <var>, like this:
#
#   dt_prop(reserved_ranges PATH "/soc/gpio@deadbeef" PROPERTY "gpio-reserved-ranges")
#   if(DEFINED reserved_ranges)
#     # Node exists and has the "gpio-reserved-ranges" property.
#   endif()
#
# To distinguish a missing node from a missing property, combine
# dt_prop() and dt_node_exists(), like this:
#
#   dt_node_exists(node_exists PATH "/soc/gpio@deadbeef")
#   dt_prop(reserved_ranges    PATH "/soc/gpio@deadbeef" PROPERTY "gpio-reserved-ranges")
#   if(DEFINED reserved_ranges)
#     # Node "/soc/gpio@deadbeef" exists and has the "gpio-reserved-ranges" property
#   elseif(node_exists)
#     # Node exists, but doesn't have the property, or the property has an unsupported type.
#   endif()
#
# <var>          : Return variable where the property value will be stored
# PATH <path>    : Node path
# PROPERTY <prop>: Property for which a value should be returned, as it
#                  appears in the DTS source
# INDEX <idx>    : Optional index when retrieving a value in an array property
function(dt_prop var)
  set(req_single_args "PATH;PROPERTY")
  set(single_args "INDEX")
  cmake_parse_arguments(DT_PROP "" "${req_single_args};${single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_prop(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_PROP_${arg})
      message(FATAL_ERROR "dt_prop(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  dt_path_internal(canonical "${DT_PROP_PATH}")
  get_property(exists TARGET devicetree_target
      PROPERTY "DT_PROP|${canonical}|${DT_PROP_PROPERTY}"
      SET
  )

  if(NOT exists)
    set(${var} PARENT_SCOPE)
    return()
  endif()

  get_target_property(val devicetree_target
      "DT_PROP|${canonical}|${DT_PROP_PROPERTY}"
  )

  if(DEFINED DT_PROP_INDEX)
    list(GET val ${DT_PROP_INDEX} element)
    set(${var} "${element}" PARENT_SCOPE)
  else()
    set(${var} "${val}" PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#
#   dt_comp_path(<var> COMPATIBLE <compatible> [INDEX <idx>])
#
# Get a list of paths for the nodes with the given compatible. The value will
# be returned in the <var> parameter.
# <var> will be undefined if no such compatible exists.
#
# For details and considerations about the format of <path> and the returned
# parameter refer to dt_prop().
#
# <var>                  : Return variable where the property value will be stored
# COMPATIBLE <compatible>: Compatible for which the list of paths should be
#                          returned, as it appears in the DTS source
# INDEX <idx>            : Optional index when retrieving a value in an array property

function(dt_comp_path var)
  set(req_single_args "COMPATIBLE")
  set(single_args "INDEX")
  cmake_parse_arguments(DT_COMP "" "${req_single_args};${single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_comp_path(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_COMP_${arg})
      message(FATAL_ERROR "dt_comp_path(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  get_property(exists TARGET devicetree_target
      PROPERTY "DT_COMP|${DT_COMP_COMPATIBLE}"
      SET
  )

  if(NOT exists)
    set(${var} PARENT_SCOPE)
    return()
  endif()

  get_target_property(val devicetree_target
      "DT_COMP|${DT_COMP_COMPATIBLE}"
  )

  if(DEFINED DT_COMP_INDEX)
    list(GET val ${DT_COMP_INDEX} element)
    set(${var} "${element}" PARENT_SCOPE)
  else()
    set(${var} "${val}" PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   dt_num_regs(<var> PATH <path>)
#
# Get the number of register blocks in the node's reg property;
# this may be zero.
#
# The value will be returned in the <var> parameter.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# <var>          : Return variable where the property value will be stored
# PATH <path>    : Node path
function(dt_num_regs var)
  set(req_single_args "PATH")
  cmake_parse_arguments(DT_REG "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_num_regs(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_REG_${arg})
      message(FATAL_ERROR "dt_num_regs(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  dt_path_internal(canonical "${DT_REG_PATH}")
  get_target_property(${var} devicetree_target "DT_REG|${canonical}|NUM")

  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# Usage:
#   dt_reg_addr(<var> PATH <path> [INDEX <idx>] [NAME <name>])
#
# Get the base address of the register block at index <idx>, or with
# name <name>. If <idx> and <name> are both omitted, the value at
# index 0 will be returned. Do not give both <idx> and <name>.
#
# The value will be returned in the <var> parameter.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# Results can be:
# - The base address of the register block
# - <var> will be undefined if node does not exists or does not have a register
#   block at the requested index or with the requested name
#
# <var>          : Return variable where the address value will be stored
# PATH <path>    : Node path
# INDEX <idx>    : Register block index number
# NAME <name>    : Register block name
function(dt_reg_addr var)
  set(req_single_args "PATH")
  set(single_args "INDEX;NAME")
  cmake_parse_arguments(DT_REG "" "${req_single_args};${single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_reg_addr(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_REG_${arg})
      message(FATAL_ERROR "dt_reg_addr(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  if(DEFINED DT_REG_INDEX AND DEFINED DT_REG_NAME)
    message(FATAL_ERROR "dt_reg_addr(${ARGV0} ...) given both INDEX and NAME")
  elseif(NOT DEFINED DT_REG_INDEX AND NOT DEFINED DT_REG_NAME)
    set(DT_REG_INDEX 0)
  elseif(DEFINED DT_REG_NAME)
    dt_reg_index_private(DT_REG_INDEX "${DT_REG_PATH}" "${DT_REG_NAME}")
    if(DT_REG_INDEX EQUAL "-1")
      set(${var} PARENT_SCOPE)
      return()
    endif()
  endif()

  dt_path_internal(canonical "${DT_REG_PATH}")
  get_target_property(${var}_list devicetree_target "DT_REG|${canonical}|ADDR")

  list(GET ${var}_list ${DT_REG_INDEX} ${var})

  if("${var}" STREQUAL NONE)
    set(${var})
  endif()

  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# Usage:
#   dt_reg_size(<var> PATH <path> [INDEX <idx>] [NAME <name>])
#
# Get the size of the register block at index <idx>, or with
# name <name>. If <idx> and <name> are both omitted, the value at
# index 0 will be returned. Do not give both <idx> and <name>.
#
# The value will be returned in the <value> parameter.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# <var>          : Return variable where the size value will be stored
# PATH <path>    : Node path
# INDEX <idx>    : Register block index number
# NAME <name>    : Register block name
function(dt_reg_size var)
  set(req_single_args "PATH")
  set(single_args "INDEX;NAME")
  cmake_parse_arguments(DT_REG "" "${req_single_args};${single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_reg_size(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_REG_${arg})
      message(FATAL_ERROR "dt_reg_size(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  if(DEFINED DT_REG_INDEX AND DEFINED DT_REG_NAME)
    message(FATAL_ERROR "dt_reg_size(${ARGV0} ...) given both INDEX and NAME")
  elseif(NOT DEFINED DT_REG_INDEX AND NOT DEFINED DT_REG_NAME)
    set(DT_REG_INDEX 0)
  elseif(DEFINED DT_REG_NAME)
    dt_reg_index_private(DT_REG_INDEX "${DT_REG_PATH}" "${DT_REG_NAME}")
    if(DT_REG_INDEX EQUAL "-1")
      set(${var} PARENT_SCOPE)
      return()
    endif()
  endif()

  dt_path_internal(canonical "${DT_REG_PATH}")
  get_target_property(${var}_list devicetree_target "DT_REG|${canonical}|SIZE")

  list(GET ${var}_list ${DT_REG_INDEX} ${var})

  if("${var}" STREQUAL NONE)
    set(${var})
  endif()

  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# Internal helper for dt_reg_addr/dt_reg_size; not meant to be used directly
function(dt_reg_index_private var path name)
  dt_prop(reg_names PATH "${path}" PROPERTY "reg-names")
  if(NOT DEFINED reg_names)
    set(index "-1")
  else()
    list(FIND reg_names "${name}" index)
  endif()
  set(${var} "${index}" PARENT_SCOPE)
endfunction()

# Usage:
#   dt_has_chosen(<var> PROPERTY <prop>)
#
# Test if the devicetree's /chosen node has a given property
# <prop> which contains the path to a node.
#
# Example devicetree fragment:
#
#       chosen {
#               foo = &bar;
#       };
#
# Example usage:
#
#       # Sets 'result' to TRUE
#       dt_has_chosen(result PROPERTY "foo")
#
#       # Sets 'result' to FALSE
#       dt_has_chosen(result PROPERTY "baz")
#
# The result of the check, either TRUE or FALSE, will be stored in the
# <var> parameter.
#
# <var>           : Return variable
# PROPERTY <prop> : Chosen property
function(dt_has_chosen var)
  set(req_single_args "PROPERTY")
  cmake_parse_arguments(DT_CHOSEN "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_has_chosen(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_CHOSEN_${arg})
      message(FATAL_ERROR "dt_has_chosen(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  get_target_property(exists devicetree_target "DT_CHOSEN|${DT_CHOSEN_PROPERTY}")

  if(${exists} STREQUAL exists-NOTFOUND)
    set(${var} FALSE PARENT_SCOPE)
  else()
    set(${var} TRUE PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   dt_chosen(<var> PROPERTY <prop>)
#
# Get a node path for a /chosen node property.
#
# The node's path will be returned in the <var> parameter. The
# variable will be left undefined if the chosen node does not exist.
#
# <var>           : Return variable where the node path will be stored
# PROPERTY <prop> : Chosen property
function(dt_chosen var)
  set(req_single_args "PROPERTY")
  cmake_parse_arguments(DT_CHOSEN "" "${req_single_args}" "" ${ARGN})

  if(${ARGV0} IN_LIST req_single_args)
    message(FATAL_ERROR "dt_chosen(${ARGV0} ...) missing return parameter.")
  endif()

  foreach(arg ${req_single_args})
    if(NOT DEFINED DT_CHOSEN_${arg})
      message(FATAL_ERROR "dt_chosen(${ARGV0} ...) "
                          "missing required argument: ${arg}"
      )
    endif()
  endforeach()

  get_target_property(${var} devicetree_target "DT_CHOSEN|${DT_CHOSEN_PROPERTY}")

  if(${${var}} STREQUAL ${var}-NOTFOUND)
    set(${var} PARENT_SCOPE)
  else()
    set(${var} ${${var}} PARENT_SCOPE)
  endif()
endfunction()

# Internal helper. Canonicalizes a path 'path' into the output
# variable 'var'. This resolves aliases, if any. Child nodes may be
# accessed via alias as well. 'var' is left undefined if the path does
# not refer to an existing node.
#
# Example devicetree:
#
#   / {
#           foo {
#                   my-label: bar {
#                           baz {};
#                   };
#           };
#           aliases {
#                   my-alias = &my-label;
#           };
#   };
#
# Example usage:
#
#   dt_path_internal(ret "/foo/bar")     # sets ret to "/foo/bar"
#   dt_path_internal(ret "my-alias")     # sets ret to "/foo/bar"
#   dt_path_internal(ret "my-alias/baz") # sets ret to "/foo/bar/baz"
#   dt_path_internal(ret "/blub")        # ret is undefined
function(dt_path_internal var path)
  string(FIND "${path}" "/" slash_index)

  if("${slash_index}" EQUAL 0)
    # If the string starts with a slash, it should be an existing
    # canonical path.
    dt_path_internal_exists(check "${path}")
    if (check)
      set(${var} "${path}" PARENT_SCOPE)
      return()
    endif()
  else()
    # Otherwise, try to expand a leading alias.
    string(SUBSTRING "${path}" 0 "${slash_index}" alias_name)
    dt_alias(alias_path PROPERTY "${alias_name}")

    # If there is a leading alias, append the rest of the string
    # onto it and see if that's an existing node.
    if (DEFINED alias_path)
      set(rest)
      if (NOT "${slash_index}" EQUAL -1)
        string(SUBSTRING "${path}" "${slash_index}" -1 rest)
      endif()
      dt_path_internal_exists(expanded_path_exists "${alias_path}${rest}")
      if (expanded_path_exists)
        set(${var} "${alias_path}${rest}" PARENT_SCOPE)
        return()
      endif()
    endif()
  endif()

  # Failed search; ensure return variable is undefined.
  set(${var} PARENT_SCOPE)
endfunction()

# Internal helper. Set 'var' to TRUE if a canonical path 'path' refers
# to an existing node. Set it to FALSE otherwise. See
# dt_path_internal for a definition and examples of 'canonical' paths.
function(dt_path_internal_exists var path)
  get_target_property(path_prop devicetree_target "DT_NODE|${path}")
  if (path_prop)
    set(${var} TRUE PARENT_SCOPE)
  else()
    set(${var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# 4.2. *_if_dt_node
#
# This section is similar to the extensions named *_ifdef, except
# actions are performed if the devicetree contains some node.
# *_if_dt_node functions may be added as needed, or if they are likely
# to be useful for user applications.

# Add item(s) to a target's SOURCES list if a devicetree node exists.
#
# Example usage:
#
#   # If the devicetree alias "led0" refers to a node, this
#   # adds "blink_led.c" to the sources list for the "app" target.
#   target_sources_if_dt_node("led0" app PRIVATE blink_led.c)
#
#   # If the devicetree path "/soc/serial@4000" is a node, this
#   # adds "uart.c" to the sources list for the "lib" target,
#   target_sources_if_dt_node("/soc/serial@4000" lib PRIVATE uart.c)
#
# <path>    : Path to devicetree node to check
# <target>  : Build system target whose sources to add to
# <scope>   : Scope to add items to
# <item>    : Item (or items) to add to the target
function(target_sources_if_dt_node path target scope item)
  dt_node_exists(check PATH "${path}")
  if(${check})
    target_sources(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

########################################################
# 4.3 zephyr_dt_*
#
# The following methods are common code for dealing
# with devicetree related files in CMake.
#
# Note that functions related to accessing the
# *contents* of the devicetree belong in section 4.1.
# This section is just for DT file processing at
# configuration time.
########################################################

# Usage:
#   zephyr_dt_preprocess(CPP <path> [<argument...>]
#                        SOURCE_FILES <file...>
#                        OUT_FILE <file>
#                        [DEPS_FILE <file>]
#                        [EXTRA_CPPFLAGS <flag...>]
#                        [INCLUDE_DIRECTORIES <dir...>]
#                        [WORKING_DIRECTORY <dir>]
#
# Preprocess one or more devicetree source files. The preprocessor
# symbol __DTS__ will be defined. If the preprocessor command fails, a
# fatal error occurs.
#
# Mandatory arguments:
#
# CPP <path> [<argument...>]: path to C preprocessor, followed by any
#                             additional arguments
#
# SOURCE_FILES <file...>: The source files to run the preprocessor on.
#                         These will, in effect, be concatenated in order
#                         and used as the preprocessor input.
#
# OUT_FILE <file>: Where to store the preprocessor output.
#
# Optional arguments:
#
# DEPS_FILE <file>: If set, generate a dependency file here.
#
# EXTRA_CPPFLAGS <flag...>: Additional flags to pass the preprocessor.
#
# INCLUDE_DIRECTORIES <dir...>: Additional #include file directories.
#
# WORKING_DIRECTORY <dir>: where to run the preprocessor.
function(zephyr_dt_preprocess)
  set(req_single_args "OUT_FILE")
  set(single_args "DEPS_FILE;WORKING_DIRECTORY")
  set(req_multi_args "CPP;SOURCE_FILES")
  set(multi_args "EXTRA_CPPFLAGS;INCLUDE_DIRECTORIES")
  cmake_parse_arguments(DT_PREPROCESS "" "${req_single_args};${single_args}" "${req_multi_args};${multi_args}" ${ARGN})

  foreach(arg ${req_single_args} ${req_multi_args})
    if(NOT DEFINED DT_PREPROCESS_${arg})
      message(FATAL_ERROR "dt_preprocess() missing required argument: ${arg}")
    endif()
  endforeach()

  set(include_opts)
  foreach(dir ${DT_PREPROCESS_INCLUDE_DIRECTORIES})
    list(APPEND include_opts -isystem ${dir})
  endforeach()

  set(source_opts)
  foreach(file ${DT_PREPROCESS_SOURCE_FILES})
    list(APPEND source_opts -include ${file})
  endforeach()

  set(deps_opts)
  if(DEFINED DT_PREPROCESS_DEPS_FILE)
    list(APPEND deps_opts -MD -MF ${DT_PREPROCESS_DEPS_FILE})
  endif()

  set(workdir_opts)
  if(DEFINED DT_PREPROCESS_WORKING_DIRECTORY)
    list(APPEND workdir_opts WORKING_DIRECTORY ${DT_PREPROCESS_WORKING_DIRECTORY})
  endif()

  # We are leaving linemarker directives enabled on purpose. This tells
  # dtlib where each line actually came from, which improves error
  # reporting.
  set(preprocess_cmd ${DT_PREPROCESS_CPP}
    -x assembler-with-cpp
    -nostdinc
    ${include_opts}
    ${source_opts}
    ${NOSYSDEF_CFLAG}
    -D__DTS__
    ${DT_PREPROCESS_EXTRA_CPPFLAGS}
    -E   # Stop after preprocessing
    ${deps_opts}
    -o ${DT_PREPROCESS_OUT_FILE}
    ${ZEPHYR_BASE}/misc/empty_file.c
    ${workdir_opts})

  execute_process(COMMAND ${preprocess_cmd} RESULT_VARIABLE ret)
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "failed to preprocess devicetree files (error code ${ret}): ${DT_PREPROCESS_SOURCE_FILES}")
  endif()
endfunction()

########################################################
# 5. Zephyr linker functions
########################################################
# 5.1. zephyr_linker*
#
# The following methods are for defining linker structure using CMake functions.
#
# This allows Zephyr developers to define linker sections and their content and
# have this configuration rendered into an appropriate linker script based on
# the toolchain in use.
# For example:
# ld linker scripts with GNU ld
# ARM scatter files with ARM linker.
#
# Example usage:
# zephyr_linker_section(
#   NAME my_data
#   VMA  RAM
#   LMA  FLASH
# )
#
# and to configure special input sections for the section
# zephyr_linker_section_configure(
#   SECTION my_data
#   INPUT   "my_custom_data"
#   KEEP
# )


# Usage:
#   zephyr_linker([FORMAT <format>]
#     [ENTRY <entry symbol>]
#   )
#
# Zephyr linker general settings.
# This function specifies general settings for the linker script to be generated.
#
# FORMAT <format>:            The output format of the linked executable.
# ENTRY <entry symbolformat>: The code entry symbol.
#
function(zephyr_linker)
  set(single_args "ENTRY;FORMAT")
  cmake_parse_arguments(LINKER "" "${single_args}" "" ${ARGN})

  if(LINKER_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker(${ARGV0} ...) given unknown "
                        "arguments: ${LINKER_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED LINKER_FORMAT)
    get_property(format_defined TARGET linker PROPERTY FORMAT SET)
    if(format_defined)
      message(FATAL_ERROR "zephyr_linker(FORMAT ...) already configured.")
    else()
      set_property(TARGET linker PROPERTY FORMAT ${LINKER_FORMAT})
    endif()
  endif()

  if(DEFINED LINKER_ENTRY)
    get_property(entry_defined TARGET linker PROPERTY ENTRY SET)
    if(entry_defined)
      message(FATAL_ERROR "zephyr_linker(ENTRY ...) already configured.")
    else()
      set_property(TARGET linker PROPERTY ENTRY ${LINKER_ENTRY})
    endif()
  endif()
endfunction()

# Usage:
#   zephyr_linker_memory(NAME <name> START <address> SIZE <size> FLAGS <flags>)
#
# Zephyr linker memory.
# This function specifies a memory region for the platform in use.
#
# Note:
#   This function should generally be called with values obtained from
#   devicetree or Kconfig.
#
# NAME <name>    : Name of the memory region, for example FLASH.
# START <address>: Start address of the memory region.
#                  Start address can be given as decimal or hex value.
# SIZE <size>    : Size of the memory region.
#                  Size can be given as decimal value, hex value, or decimal with postfix k or m.
#                  All the following are valid values:
#                    1048576, 0x10000, 1024k, 1024K, 1m, and 1M.
# FLAGS <flags>  : Flags describing properties of the memory region.
#                  Currently supported:
#                  r: Read-only region
#                  w: Read-write region
#                  x: Executable region
#                  The flags r and x, or w and x may be combined like: rx, wx.
function(zephyr_linker_memory)
  set(single_args "FLAGS;NAME;SIZE;START")
  cmake_parse_arguments(MEMORY "" "${single_args}" "" ${ARGN})

  if(MEMORY_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker_memory(${ARGV0} ...) given unknown "
                        "arguments: ${MEMORY_UNPARSED_ARGUMENTS}"
    )
  endif()

  foreach(arg ${single_args})
    if(NOT DEFINED MEMORY_${arg})
      message(FATAL_ERROR "zephyr_linker_memory(${ARGV0} ...) missing required "
                          "argument: ${arg}"
      )
    endif()
  endforeach()

  set(MEMORY)
  zephyr_linker_arg_val_list(MEMORY "${single_args}")

  string(REPLACE ";" "\;" MEMORY "${MEMORY}")
  set_property(TARGET linker
               APPEND PROPERTY MEMORY_REGIONS "{${MEMORY}}"
  )
endfunction()

# Usage:
#   zephyr_linker_memory_ifdef(<setting> NAME <name> START <address> SIZE <size> FLAGS <flags>)
#
# Will create memory region if <setting> is enabled.
#
# <setting>: Setting to check for True value before invoking
#            zephyr_linker_memory()
#
# See zephyr_linker_memory() description for other supported arguments.
#
macro(zephyr_linker_memory_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_linker_memory(${ARGN})
  endif()
endmacro()

# Usage:
#   zephyr_linker_dts_section(PATH <path>)
#
# Zephyr linker devicetree memory section from path.
#
# This function specifies an output section for the platform in use based on its
# devicetree configuration.
#
# The section will only be defined if the devicetree exists and has status okay.
#
# PATH <path>      : Devicetree node path.
#
function(zephyr_linker_dts_section)
  set(single_args "PATH")
  cmake_parse_arguments(DTS_SECTION "" "${single_args}" "" ${ARGN})

  if(DTS_SECTION_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker_dts_section(${ARGV0} ...) given unknown "
	    "arguments: ${DTS_SECTION_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(NOT DEFINED DTS_SECTION_PATH)
    message(FATAL_ERROR "zephyr_linker_dts_section(${ARGV0} ...) missing "
                        "required argument: PATH"
    )
  endif()

  dt_node_exists(exists PATH ${DTS_SECTION_PATH})
  if(NOT ${exists})
    return()
  endif()

  dt_prop(name PATH ${DTS_SECTION_PATH} PROPERTY "zephyr,memory-region")
  if(NOT DEFINED name)
    message(FATAL_ERROR "zephyr_linker_dts_section(${ARGV0} ...) missing "
                        "\"zephyr,memory-region\" property"
    )
  endif()
  zephyr_string(SANITIZE name ${name})

  dt_reg_addr(addr PATH ${DTS_SECTION_PATH})

  zephyr_linker_section(NAME ${name} ADDRESS ${addr} VMA ${name} TYPE NOLOAD)

endfunction()

# Usage:
#   zephyr_linker_dts_memory(PATH <path> FLAGS <flags>)
#   zephyr_linker_dts_memory(NODELABEL <nodelabel> FLAGS <flags>)
#   zephyr_linker_dts_memory(CHOSEN <prop> FLAGS <flags>)
#
# Zephyr linker devicetree memory.
# This function specifies a memory region for the platform in use based on its
# devicetree configuration.
#
# The memory will only be defined if the devicetree node or a devicetree node
# matching the nodelabel exists and has status okay.
#
# Only one of PATH, NODELABEL, and CHOSEN parameters may be given.
#
# PATH <path>      : Devicetree node identifier.
# NODELABEL <label>: Node label
# CHOSEN <prop>    : Chosen property, add memory section described by the
#                    /chosen property if it exists.
# FLAGS <flags>  : Flags describing properties of the memory region.
#                  Currently supported:
#                  r: Read-only region
#                  w: Read-write region
#                  x: Executable region
#                  The flags r and x, or w and x may be combined like: rx, wx.
#
function(zephyr_linker_dts_memory)
  set(single_args "CHOSEN;FLAGS;PATH;NODELABEL")
  cmake_parse_arguments(DTS_MEMORY "" "${single_args}" "" ${ARGN})

  if(DTS_MEMORY_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker_dts_memory(${ARGV0} ...) given unknown "
                        "arguments: ${DTS_MEMORY_UNPARSED_ARGUMENTS}"
    )
  endif()

  if((DEFINED DTS_MEMORY_PATH AND (DEFINED DTS_MEMORY_NODELABEL OR DEFINED DTS_MEMORY_CHOSEN))
     OR (DEFINED DTS_MEMORY_NODELABEL AND DEFINED DTS_MEMORY_CHOSEN))
    message(FATAL_ERROR "zephyr_linker_dts_memory(${ARGV0} ...), only one of "
                        "PATH, NODELABEL, and CHOSEN is allowed."
    )
  endif()

  if(DEFINED DTS_MEMORY_NODELABEL)
    dt_nodelabel(DTS_MEMORY_PATH NODELABEL ${DTS_MEMORY_NODELABEL})
  endif()

  if(DEFINED DTS_MEMORY_CHOSEN)
    dt_chosen(DTS_MEMORY_PATH PROPERTY ${DTS_MEMORY_CHOSEN})
  endif()

  if(NOT DEFINED DTS_MEMORY_PATH)
    return()
  endif()

  dt_node_exists(exists PATH ${DTS_MEMORY_PATH})
  if(NOT ${exists})
    return()
  endif()

  dt_reg_addr(addr PATH ${DTS_MEMORY_PATH})
  dt_reg_size(size PATH ${DTS_MEMORY_PATH})
  dt_prop(name PATH ${DTS_MEMORY_PATH} PROPERTY "zephyr,memory-region")
  zephyr_string(SANITIZE name ${name})

  zephyr_linker_memory(
    NAME  ${name}
    START ${addr}
    SIZE  ${size}
    FLAGS ${DTS_MEMORY_FLAGS}
  )
endfunction()

# Usage:
#   zephyr_linker_group(NAME <name> [VMA <region|group>] [LMA <region|group>] [SYMBOL <SECTION>])
#   zephyr_linker_group(NAME <name> GROUP <group> [SYMBOL <SECTION>])
#
# Zephyr linker group.
# This function specifies a group inside a memory region or another group.
#
# The group ensures that all section inside the group are located together inside
# the specified group.
#
# This also allows for section placement inside a given group without the section
# itself needing the precise knowledge regarding the exact memory region this
# section will be placed in, as that will be determined by the group setup.
#
# Each group will define the following linker symbols:
# __<name>_start      : Start address of the group
# __<name>_end        : End address of the group
# __<name>_size       : Size of the group
#
# Note: <name> will be converted to lower casing for linker symbols definitions.
#
# NAME <name>         : Name of the group.
# VMA <region|group>  : VMA Memory region or group to be used for this group.
#                       If a group is used then the VMA region of that group will be used.
# LMA <region|group>  : Memory region or group to be used for this group.
# GROUP <group>       : Place the new group inside the existing group <group>
# SYMBOL <SECTION>    : Specify that start symbol of the region should be identical
#                       to the start address of the first section in the group.
#
# Note: VMA and LMA are mutual exclusive with GROUP
#
# Example:
#   zephyr_linker_memory(NAME memA START ... SIZE ... FLAGS ...)
#   zephyr_linker_group(NAME groupA LMA memA)
#   zephyr_linker_group(NAME groupB LMA groupA)
#
# will create two groups in same memory region as groupB will inherit the LMA
# from groupA:
#
# +-----------------+
# | memory region A |
# |                 |
# | +-------------+ |
# | | groupA      | |
# | +-------------+ |
# |                 |
# | +-------------+ |
# | | groupB      | |
# | +-------------+ |
# |                 |
# +-----------------+
#
# whereas
#   zephyr_linker_memory(NAME memA START ... SIZE ... FLAGS ...)
#   zephyr_linker_group(NAME groupA LMA memA)
#   zephyr_linker_group(NAME groupB GROUP groupA)
#
# will create groupB inside groupA:
#
# +---------------------+
# | memory region A     |
# |                     |
# | +-----------------+ |
# | | groupA          | |
# | |                 | |
# | | +-------------+ | |
# | | | groupB      | | |
# | | +-------------+ | |
# | |                 | |
# | +-----------------+ |
# |                     |
# +---------------------+
function(zephyr_linker_group)
  set(single_args "NAME;GROUP;LMA;SYMBOL;VMA")
  set(symbol_values SECTION)
  cmake_parse_arguments(GROUP "" "${single_args}" "" ${ARGN})

  if(GROUP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker_group(${ARGV0} ...) given unknown "
                        "arguments: ${GROUP_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED GROUP_GROUP AND (DEFINED GROUP_VMA OR DEFINED GROUP_LMA))
    message(FATAL_ERROR "zephyr_linker_group(GROUP ...) cannot be used with "
                        "VMA or LMA"
    )
  endif()

  if(DEFINED GROUP_SYMBOL)
    if(NOT ${GROUP_SYMBOL} IN_LIST symbol_values)
      message(FATAL_ERROR "zephyr_linker_group(SYMBOL ...) given unknown value")
    endif()
  endif()

  set(GROUP)
  zephyr_linker_arg_val_list(GROUP "${single_args}")

  string(REPLACE ";" "\;" GROUP "${GROUP}")
  set_property(TARGET linker
               APPEND PROPERTY GROUPS "{${GROUP}}"
  )
endfunction()

# Usage:
#   zephyr_linker_section(NAME <name> [GROUP <group>]
#                         [VMA <region|group>] [LMA <region|group>]
#                         [ADDRESS <address>] [ALIGN <alignment>]
#                         [SUBALIGN <alignment>] [FLAGS <flags>]
#                         [HIDDEN] [NOINPUT] [NOINIT]
#                         [PASS [NOT] <name>]
#   )
#
# Zephyr linker output section.
# This function specifies an output section for the linker.
#
# When using zephyr_linker_section(NAME <name>) an output section with <name>
# will be configured. This section will default include input sections of the
# same name, unless NOINPUT is specified.
# This means the section named `foo` will default include the sections matching
# `foo` and `foo.*`
# Each output section will define the following linker symbols:
# __<name>_start      : Start address of the section
# __<name>_end        : End address of the section
# __<name>_size       : Size of the section
# __<name>_load_start : Load address of the section, if VMA = LMA then this
#                       value will be identical to `__<name>_start`
#
# The location of the output section can be controlled using LMA, VMA, and
# address parameters
#
# NAME <name>         : Name of the output section.
# VMA <region|group>  : VMA Memory region or group where code / data is located runtime (VMA)
#                       If <group> is used here it means the section will use the
#                       same VMA memory region as <group> but will not be placed
#                       inside the group itself, see also GROUP argument.
# KVMA <region|group> : Kernel VMA Memory region or group where code / data is located runtime (VMA)
#                       When MMU is active and Kernel VM base and offset is different
#                       from SRAM base and offset, then the region defined by KVMA will
#                       be used as VMA.
#                       If <group> is used here it means the section will use the
#                       same VMA memory region as <group> but will not be placed
#                       inside the group itself, see also GROUP argument.
# LMA <region|group>  : Memory region or group where code / data is loaded (LMA)
#                       If VMA is different from LMA, the code / data will be loaded
#                       from LMA into VMA at bootup, this is usually the case for
#                       global or static variables that are loaded in rom and copied
#                       to ram at boot time.
#                       If <group> is used here it means the section will use the
#                       same LMA memory region as <group> but will not be placed
#                       inside the group itself, see also GROUP argument.
# GROUP <group>       : Place this section inside the group <group>
# ADDRESS <address>   : Specific address to use for this section.
# ALIGN_WITH_INPUT    : The alignment difference between VMA and LMA is kept
#                       intact for this section.
# ALIGN <alignment>   : Align the execution address with alignment.
# SUBALIGN <alignment>: Align input sections with alignment value.
# ENDALIGN <alignment>: Align the end so that next output section will start aligned.
#                       This only has effect on Scatter scripts.
#  Note: Regarding all alignment attributes. Not all linkers may handle alignment
#        in identical way. For example the Scatter file will align both load and
#        execution address (LMA and VMA) to be aligned when given the ALIGN attribute.
# NOINPUT             : No default input sections will be defined, to setup input
#                       sections for section <name>, the corresponding
#                       `zephyr_linker_section_configure()` must be used.
# PASS [NOT] <name>   : Linker pass iteration where this section should be active.
#                       Default a section will be present during all linker passes
#                       but in cases a section shall only be present at a specific
#                       pass, this argument can be used. For example to only have
#                       this section present on the `TEST` linker pass, use `PASS TEST`.
#                       It is possible to negate <name>, such as `PASS NOT <name>`.
#                       For example, `PASS NOT TEST` means the call is effective
#                       on all but the `TEST` linker pass iteration.
#
# Note: VMA and LMA are mutual exclusive with GROUP
#
function(zephyr_linker_section)
  set(options     "ALIGN_WITH_INPUT;HIDDEN;NOINIT;NOINPUT")
  set(single_args "ADDRESS;ALIGN;ENDALIGN;GROUP;KVMA;LMA;NAME;SUBALIGN;TYPE;VMA")
  set(multi_args  "PASS")
  cmake_parse_arguments(SECTION "${options}" "${single_args}" "${multi_args}" ${ARGN})

  if(SECTION_UNPARSED_ARGUMENTS)
    message(WARNING "zephyr_linker_section(${ARGV0} ...) given unknown "
                    "arguments: ${SECTION_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED SECTION_GROUP AND (DEFINED SECTION_VMA OR DEFINED SECTION_LMA))
    message(FATAL_ERROR "zephyr_linker_section(GROUP ...) cannot be used with "
                        "VMA or LMA"
    )
  endif()

  if(DEFINED SECTION_KVMA)
    # If KVMA is set and the Kernel virtual memory settings reqs are met, we
    # substitute the VMA setting with the specified KVMA value.
    if(CONFIG_MMU)
      math(EXPR KERNEL_MEM_VM_OFFSET
           "(${CONFIG_KERNEL_VM_BASE} + ${CONFIG_KERNEL_VM_OFFSET})\
            - (${CONFIG_SRAM_BASE_ADDRESS} + ${CONFIG_SRAM_OFFSET})"
      )

      if(NOT (${KERNEL_MEM_VM_OFFSET} EQUAL 0))
        set(SECTION_VMA ${SECTION_KVMA})
        set(SECTION_KVMA)
      endif()
    endif()
  endif()

  if(DEFINED SECTION_PASS)
    list(LENGTH SECTION_PASS pass_length)
    if(${pass_length} GREATER 1)
      list(GET SECTION_PASS 0 pass_elem_0)
      if((NOT (${pass_elem_0} STREQUAL "NOT")) OR (${pass_length} GREATER 2))
        message(FATAL_ERROR "zephyr_linker_section(PASS takes maximum "
          "a single argument of the form: '<pass name>' or 'NOT <pass_name>'.")
      endif()
    endif()
  endif()

  set(SECTION)
  zephyr_linker_arg_val_list(SECTION "${single_args}")
  zephyr_linker_arg_val_list(SECTION "${options}")
  zephyr_linker_arg_val_list(SECTION "${multi_args}")

  string(REPLACE ";" "\;" SECTION "${SECTION}")
  set_property(TARGET linker
               APPEND PROPERTY SECTIONS "{${SECTION}}"
  )
endfunction()

# Usage:
#   zephyr_linker_section_ifdef(<setting>
#                               NAME <name> [VMA <region>] [LMA <region>]
#                               [ADDRESS <address>] [ALIGN <alignment>]
#                               [SUBALIGN <alignment>] [FLAGS <flags>]
#                               [HIDDEN] [NOINPUT] [NOINIT]
#                               [PASS <no> [<no>...]
#   )
#
# Will create an output section if <setting> is enabled.
#
# <setting>: Setting to check for True value before invoking
#            zephyr_linker_section()
#
# See zephyr_linker_section() description for other supported arguments.
#
macro(zephyr_linker_section_ifdef feature_toggle)
  if(${${feature_toggle}})
    zephyr_linker_section(${ARGN})
  endif()
endmacro()

# Usage:
#   zephyr_iterable_section(NAME <name> [GROUP <group>]
#                           [VMA <region|group>] [LMA <region|group>]
#                           [ALIGN_WITH_INPUT] [SUBALIGN <alignment>]
#   )
#
#
# Define an output section which will set up an iterable area
# of equally-sized data structures. For use with STRUCT_SECTION_ITERABLE.
# Input sections will be sorted by name in lexicographical order.
#
# Each list for an input section will define the following linker symbols:
# _<name>_list_start: Start of the iterable list
# _<name>_list_end  : End of the iterable list
#
# The output section will be named `<name>_area` and define the following linker
# symbols:
# __<name>_area_start      : Start address of the section
# __<name>_area_end        : End address of the section
# __<name>_area_size       : Size of the section
# __<name>_area_load_start : Load address of the section, if VMA = LMA then this
#                            value will be identical to `__<name>_area_start`
#
# NAME <name>         : Name of the struct type, the output section be named
#                       accordingly as: <name>_area.
# VMA <region|group>  : VMA Memory region where code / data is located runtime (VMA)
# LMA <region|group>  : Memory region where code / data is loaded (LMA)
#                       If VMA is different from LMA, the code / data will be loaded
#                       from LMA into VMA at bootup, this is usually the case for
#                       global or static variables that are loaded in rom and copied
#                       to ram at boot time.
# GROUP <group>       : Place this section inside the group <group>
# ADDRESS <address>   : Specific address to use for this section.
# ALIGN_WITH_INPUT    : The alignment difference between VMA and LMA is kept
#                       intact for this section.
# SUBALIGN <alignment>: Force input alignment with size <alignment>
#  Note: Regarding all alignment attributes. Not all linkers may handle alignment
#        in identical way. For example the Scatter file will align both load and
#        execution address (LMA and VMA) to be aligned when given the ALIGN attribute.
#/
function(zephyr_iterable_section)
  # ToDo - Should we use ROM, RAM, etc as arguments ?
  set(options     "ALIGN_WITH_INPUT")
  set(single_args "GROUP;LMA;NAME;SUBALIGN;VMA")
  set(multi_args  "")
  set(align_input)
  cmake_parse_arguments(SECTION "${options}" "${single_args}" "${multi_args}" ${ARGN})

  if(NOT DEFINED SECTION_NAME)
    message(FATAL_ERROR "zephyr_iterable_section(${ARGV0} ...) missing "
                        "required argument: NAME"
    )
  endif()

  if(NOT DEFINED SECTION_SUBALIGN)
    message(FATAL_ERROR "zephyr_iterable_section(${ARGV0} ...) missing "
                        "required argument: SUBALIGN"
    )
  endif()

  if(SECTION_ALIGN_WITH_INPUT)
    set(align_input ALIGN_WITH_INPUT)
  endif()

  zephyr_linker_section(
    NAME ${SECTION_NAME}_area
    GROUP "${SECTION_GROUP}"
    VMA "${SECTION_VMA}" LMA "${SECTION_LMA}"
    NOINPUT ${align_input} SUBALIGN ${SECTION_SUBALIGN}
  )
  zephyr_linker_section_configure(
    SECTION ${SECTION_NAME}_area
    INPUT "._${SECTION_NAME}.static.*"
    SYMBOLS _${SECTION_NAME}_list_start _${SECTION_NAME}_list_end
    KEEP SORT NAME
  )
endfunction()

# Usage:
#   zephyr_linker_section_obj_level(SECTION <section> LEVEL <level>)
#
# generate a symbol to mark the start of the objects array for
# the specified object and level, then link all of those objects
# (sorted by priority). Ensure the objects aren't discarded if there is
# no direct reference to them.
#
# This is useful content such as struct devices.
#
# For example: zephyr_linker_section_obj_level(SECTION init LEVEL PRE_KERNEL_1)
# will create an input section matching `.z_init_PRE_KERNEL_1?_` and
# `.z_init_PRE_KERNEL_1??_`.
#
# SECTION <section>: Section in which the objects shall be placed
# LEVEL <level>    : Priority level, all input sections matching the level
#                    will be sorted.
#
function(zephyr_linker_section_obj_level)
  set(single_args "SECTION;LEVEL")
  cmake_parse_arguments(OBJ "" "${single_args}" "" ${ARGN})

  if(NOT DEFINED OBJ_SECTION)
    message(FATAL_ERROR "zephyr_linker_section_obj_level(${ARGV0} ...) "
                        "missing required argument: SECTION"
    )
  endif()

  if(NOT DEFINED OBJ_LEVEL)
    message(FATAL_ERROR "zephyr_linker_section_obj_level(${ARGV0} ...) "
                        "missing required argument: LEVEL"
    )
  endif()

  zephyr_linker_section_configure(
    SECTION ${OBJ_SECTION}
    INPUT ".z_${OBJ_SECTION}_${OBJ_LEVEL}?_"
    SYMBOLS __${OBJ_SECTION}_${OBJ_LEVEL}_start
    KEEP SORT NAME
  )
  zephyr_linker_section_configure(
    SECTION ${OBJ_SECTION}
    INPUT ".z_${OBJ_SECTION}_${OBJ_LEVEL}??_"
    KEEP SORT NAME
  )
endfunction()

# Usage:
#   zephyr_linker_section_configure(SECTION <section> [ALIGN <alignment>]
#                                   [PASS [NOT] <name>] [PRIO <no>] [SORT <sort>]
#                                   [ANY] [FIRST] [KEEP]
#   )
#
# Configure an output section with additional input sections.
# An output section can be configured with additional input sections besides its
# default section.
# For example, to add the input section `foo` to the output section bar, with KEEP
# attribute, call:
#   zephyr_linker_section_configure(SECTION bar INPUT foo KEEP)
#
# ALIGN <alignment>   : Will align the input section placement inside the load
#                       region with <alignment>
# FIRST               : The first input section in the list should be marked as
#                       first section in output.
# SORT <NAME>         : Sort the input sections according to <type>.
#                       Currently only `NAME` is supported.
# KEEP                : Do not eliminate input section during linking
# PRIO                : The priority of the input section. Per default, input
#                       sections order is not guaranteed by all linkers, but
#                       using priority Zephyr CMake linker will create sections
#                       such that order can be guaranteed. All unprioritized
#                       sections will internally be given a CMake process order
#                       priority counting from 100, so first unprioritized section
#                       is handled internal prio 100, next 101, and so on.
#                       To ensure a specific input section come before those,
#                       you may use `PRIO 50`, `PRIO 20` and so on.
#                       To ensure an input section is at the end, it is advised
#                       to use `PRIO 200` and above.
# PASS [NOT] <name>   : The call should only be considered for linker pass where
#                       <name> is defined. It is possible to negate <name>, such
#                       as `PASS NOT <name>.
#                       For example, `PASS TEST` means the call is only effective
#                       on the `TEST` linker pass iteration. `PASS NOT TEST` on
#                       all iterations the are not `TEST`.
# FLAGS <flags>       : Special section flags such as "+RO", +XO, "+ZI".
# ANY                 : ANY section flag in scatter file.
#                       The FLAGS and ANY arguments only has effect for scatter files.
#
function(zephyr_linker_section_configure)
  set(options     "ANY;FIRST;KEEP")
  set(single_args "ALIGN;OFFSET;PRIO;SECTION;SORT")
  set(multi_args  "FLAGS;INPUT;PASS;SYMBOLS")
  cmake_parse_arguments(SECTION "${options}" "${single_args}" "${multi_args}" ${ARGN})

  if(SECTION_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "zephyr_linker_section_configure(${ARGV0} ...) given unknown arguments: ${SECTION_UNPARSED_ARGUMENTS}")
  endif()

  if(DEFINED SECTION_SYMBOLS)
    list(LENGTH SECTION_SYMBOLS symbols_count)
    if(${symbols_count} GREATER 2)
      message(FATAL_ERROR "zephyr_linker_section_configure(SYMBOLS [start_sym [end_sym]]) takes maximum two symbol names (start and end).")

    endif()
  endif()

  if(DEFINED SECTION_PASS)
    list(LENGTH SECTION_PASS pass_length)
    if(${pass_length} GREATER 1)
      list(GET SECTION_PASS 0 pass_elem_0)
      if((NOT (${pass_elem_0} STREQUAL "NOT")) OR (${pass_length} GREATER 2))
        message(FATAL_ERROR "zephyr_linker_section_configure(PASS takes maximum "
          "a single argument of the form: '<pass name>' or 'NOT <pass_name>'.")
      endif()
    endif()
  endif()

  set(SECTION)
  zephyr_linker_arg_val_list(SECTION "${single_args}")
  zephyr_linker_arg_val_list(SECTION "${options}")
  zephyr_linker_arg_val_list(SECTION "${multi_args}")

  string(REPLACE ";" "\;" SECTION "${SECTION}")
  set_property(TARGET linker
               APPEND PROPERTY SECTION_SETTINGS "{${SECTION}}"
  )
endfunction()

# Usage:
#   zephyr_linker_symbol(SYMBOL <name> EXPR <expr>)
#
# Add additional user defined symbol to the generated linker script.
#
# SYMBOL <name>: Symbol name to be available.
# EXPR <expr>  : Expression that defines the symbol. Due to linker limitations
#                all expressions should only contain simple math, such as
#                `+, -, *` and similar. The expression will go directly into the
#                linker, and all `@<symbol>@` will be replaced with the referred
#                symbol.
#
# Example:
#   To create a new symbol `bar` pointing to the start VMA address of section
#   `foo` + 1024, one can write:
#     zephyr_linker_symbol(SYMBOL bar EXPR "(@foo@ + 1024)")
#
function(zephyr_linker_symbol)
  set(single_args "EXPR;SYMBOL")
  cmake_parse_arguments(SYMBOL "" "${single_args}" "" ${ARGN})

  if(SECTION_UNPARSED_ARGUMENTS)
    message(WARNING "zephyr_linker_symbol(${ARGV0} ...) given unknown "
                    "arguments: ${SECTION_UNPARSED_ARGUMENTS}"
    )
  endif()

  set(SYMBOL)
  zephyr_linker_arg_val_list(SYMBOL "${single_args}")

  string(REPLACE ";" "\;" SYMBOL "${SYMBOL}")
  set_property(TARGET linker
               APPEND PROPERTY SYMBOLS "{${SYMBOL}}"
  )
endfunction()

# Internal helper macro for zephyr_linker*() functions.
# The macro will create a list of argument-value pairs for defined arguments
# that can be passed on to linker script generators and processed as a CMake
# function call using cmake_parse_arguments.
#
# For example, having the following argument and value:
# FOO: bar
# BAZ: <undefined>
# QUX: option set
#
# will create a list as: "FOO;bar;QUX;TRUE" which can then be parsed as argument
# list later.
macro(zephyr_linker_arg_val_list list arguments)
  foreach(arg ${arguments})
    if(DEFINED ${list}_${arg})
      list(APPEND ${list} ${arg} "${${list}_${arg}}")
    endif()
  endforeach()
endmacro()

########################################################
# 6. Function helper macros
########################################################
#
# Set of CMake macros to facilitate argument processing when defining functions.
#

#
# Helper macro for verifying that at least one of the required arguments has
# been provided by the caller.
#
# A FATAL_ERROR will be raised if not one of the required arguments has been
# passed by the caller.
#
# Usage:
#   zephyr_check_arguments_required(<function_name> <prefix> <arg1> [<arg2> ...])
#
macro(zephyr_check_arguments_required function prefix)
  set(check_defined DEFINED)
  zephyr_check_flags_required(${function} ${prefix} ${ARGN})
  set(check_defined)
endmacro()

#
# Helper macro for verifying that at least one of the required flags has
# been provided by the caller.
#
# A FATAL_ERROR will be raised if not one of the required arguments has been
# passed by the caller.
#
# Usage:
#   zephyr_check_flags_required(<function_name> <prefix> <flag1> [<flag2> ...])
#
macro(zephyr_check_flags_required function prefix)
  set(required_found FALSE)
  foreach(required ${ARGN})
    if(${check_defined} ${prefix}_${required})
      set(required_found TRUE)
    endif()
  endforeach()

  if(NOT required_found)
    message(FATAL_ERROR "${function}(...) missing a required argument: ${ARGN}")
  endif()
endmacro()

#
# Helper macro for verifying that all the required arguments have been
# provided by the caller.
#
# A FATAL_ERROR will be raised if one of the required arguments is missing.
#
# Usage:
#   zephyr_check_arguments_required_all(<function_name> <prefix> <arg1> [<arg2> ...])
#
macro(zephyr_check_arguments_required_all function prefix)
  foreach(required ${ARGN})
    if(NOT DEFINED ${prefix}_${required})
      message(FATAL_ERROR "${function}(...) missing a required argument: ${required}")
    endif()
  endforeach()
endmacro()

#
# Helper macro for verifying that none of the mutual exclusive arguments are
# provided together.
#
# A FATAL_ERROR will be raised if any of the arguments are given together.
#
# Usage:
#   zephyr_check_arguments_exclusive(<function_name> <prefix> <arg1> <arg2> [<arg3> ...])
#
macro(zephyr_check_arguments_exclusive function prefix)
  set(check_defined DEFINED)
  zephyr_check_flags_exclusive(${function} ${prefix} ${ARGN})
  set(check_defined)
endmacro()

#
# Helper macro for verifying that none of the mutual exclusive flags are
# provided together.
#
# A FATAL_ERROR will be raised if any of the flags are given together.
#
# Usage:
#   zephyr_check_flags_exclusive(<function_name> <prefix> <flag1> <flag2> [<flag3> ...])
#
macro(zephyr_check_flags_exclusive function prefix)
  set(args_defined)
  foreach(arg ${ARGN})
    if(${check_defined} ${prefix}_${arg})
      list(APPEND args_defined ${arg})
    endif()
  endforeach()
  list(LENGTH args_defined exclusive_length)
  if(exclusive_length GREATER 1)
    list(POP_FRONT args_defined argument)
    message(FATAL_ERROR "${function}(${argument} ...) cannot be used with "
        "argument: ${args_defined}"
      )
  endif()
endmacro()
