include(CheckCCompilerFlag)

########################################################
# Table of contents
########################################################
# 1. Zephyr-aware extensions
# 1.1. zephyr_*
# 1.2. zephyr_library_*
# 1.3. generate_inc_*
# 2. Kconfig-aware extensions
# 2.1 *_if_kconfig
# 2.2 Misc
# 3. CMake-generic extensions
# 3.1. *_ifdef
# 3.2. *_ifndef
# 3.3. *_option compiler compatibility checks
# 3.4. Debugging CMake

########################################################
# 1. Zephyr-aware extensions
########################################################
# 1.1. zephyr_*
#
# The following methods are for modifying the CMake library[0] called
# "zephyr". zephyr is a catchall CMake library for source files that
# can be built purely with the include paths, defines, and other
# compiler flags that all zephyr source files use.
# [0] https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html
#
# Example usage:
# zephyr_sources(
#   random_esp32.c
#   utils.c
#   )
#
# Is short for:
# target_sources(zephyr PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/random_esp32.c
#   ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
#  )

# https://cmake.org/cmake/help/latest/command/target_sources.html
function(zephyr_sources)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()
    target_sources(zephyr PRIVATE ${path})
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
# zephyr_get_${build_information}${format}(x)
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
#  the empty string '', signifying that it should be returned as a list
#  _as_string signifying that it should be returned as a string
#
# e.g.
# zephyr_get_include_directories(x)
# writes "-Isome_dir;-Isome/other/dir" to x

# Utility macro used by the below macros.
macro(get_property_and_add_prefix result target property prefix)
  get_property(target_property TARGET ${target} PROPERTY ${property})
  foreach(x ${target_property})
    list(APPEND ${result} ${prefix}${x})
  endforeach()
endmacro()

macro(zephyr_get_include_directories i)
  get_property_and_add_prefix(${i} zephyr_interface INTERFACE_INCLUDE_DIRECTORIES -I)
endmacro()

macro(zephyr_get_system_include_directories i)
  get_property_and_add_prefix(${i} zephyr_interface INTERFACE_SYSTEM_INCLUDE_DIRECTORIES -isystem)
endmacro()

macro(zephyr_get_compile_definitions i)
  get_property_and_add_prefix(${i} zephyr_interface INTERFACE_COMPILE_DEFINITIONS -D)
endmacro()

macro(zephyr_get_compile_options i)
  get_property(${i} TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)
endmacro()

macro(zephyr_get_include_directories_as_string i)
  zephyr_get_include_directories(${i})

  string(REPLACE ";"  " "   ${i} ${${i}})
  string(REPLACE "-I" " -I" ${i} ${${i}})
endmacro()

macro(zephyr_get_system_include_directories_as_string i)
  get_property_and_add_prefix(${i} zephyr_interface INTERFACE_SYSTEM_INCLUDE_DIRECTORIES -isystem)

  string(REPLACE ";"  " "               ${i} ${${i}})
  string(REPLACE "-isystem" " -isystem" ${i} ${${i}})
endmacro()

macro(zephyr_get_compile_definitions_as_string i)
  get_property_and_add_prefix(${i} zephyr_interface INTERFACE_COMPILE_DEFINITIONS -D)

  string(REPLACE ";"  " "   ${i} ${${i}})
  string(REPLACE "-D" " -D" ${i} ${${i}})
endmacro()

macro(zephyr_get_compile_options_as_string i)
  zephyr_get_compile_options(j)

  foreach(__opt__ ${j})
    if(__opt__ MATCHES "<COMPILE_LANGUAGE:")
      # TODO: Support COMPILE_LANGUAGE generator expressions
      continue()
    endif()
    set(${i} "${${i}} ${__opt__}")
  endforeach()
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
    $ENV{ZEPHYR_BASE}/scripts/file2hex.py
    ${ARGN} # Extra arguments are passed to file2hex.py
    --file ${source_file}
    > ${generated_file} # Does pipe redirection work on Windows?
    DEPENDS ${source_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()

function(generate_inc_file_for_target
    target          # The cmake target that depends on the generated file
    source_file     # The source file to be converted to hex
    generated_file  # The generated file
                    # Any additional arguments are passed on to file2hex.py
    )
  generate_inc_file(${source_file} ${generated_file} ${ARGN})

  # Ensure 'generated_file' is generated before 'target' by creating a
  # 'custom_target' for it and setting up a dependency between the two
  # targets

  # But first create a unique name for the custom target
  # Replace / with _ (driver/serial => driver_serial) and . with _
  set(generated_target_name ${generated_file})

  string(REPLACE "/" "_" generated_target_name ${generated_target_name})
  string(REPLACE "." "_" generated_target_name ${generated_target_name})

  add_custom_target(${generated_target_name} DEPENDS ${generated_file})
  add_dependencies(${target} ${generated_target_name})
endfunction()

# 2.1 zephyr_library_*
#
# Zephyr libraries use CMake's library concept and a set of
# assumptions about how zephyr code is organized to cut down on
# boilerplate code.
#
# A Zephyr library can be constructed by the function zephyr_library
# or zephyr_library_named. The constructors create a CMake library
# with a name accessible through the variable ZEPHYR_CURRENT_LIBRARY.
#
# The variable ZEPHYR_CURRENT_LIBRARY should seldomly be needed since
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

# Constructor with a directory-inferred name
macro(zephyr_library)
  zephyr_library_get_current_dir_lib_name(lib_name)
  zephyr_library_named(${lib_name})
endmacro()

# Determines what the current directory's lib name would be and writes
# it to the argument "lib_name"
macro(zephyr_library_get_current_dir_lib_name lib_name)
  # Remove the prefix (/home/sebo/zephyr/driver/serial/CMakeLists.txt => driver/serial/CMakeLists.txt)
  file(RELATIVE_PATH name $ENV{ZEPHYR_BASE} ${CMAKE_CURRENT_LIST_FILE})

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

  target_link_libraries(${name} zephyr_interface)
endmacro()


function(zephyr_link_interface interface)
  target_link_libraries(interface INTERFACE zephyr_interface)
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
  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} ${item} ${ARGN})
endfunction()

function(zephyr_library_compile_definitions item)
  target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

function(zephyr_library_cc_option)
  foreach(arg ${ARGV})
     target_cc_option(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${arg})
  endforeach()
endfunction()

# Add the existing CMake library 'library' to the global list of
# Zephyr CMake libraries. This is done automatically by the
# constructor but must called explicitly on CMake libraries that do
# not use a zephyr library constructor.
function(zephyr_append_cmake_library library)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_LIBS ${library})
endfunction()

########################################################
# 2. Kconfig-aware extensions
########################################################
#
# Kconfig is a configuration language developed for the Linux
# kernel. The below functions integrate CMake with Kconfig.
#
# 2.1 *_if_kconfig
#
# Functions for conditionally including directories and source files
# that have matching KConfig values.
#
# zephyr_library_sources_if_kconfig(fft.c)
# is the same as
# zephyr_library_sources_ifdef(CONFIG_FFT fft.c)
#
# add_subdirectory_if_kconfig(serial)
# is the same as
# add_subdirectory_ifdef(CONFIG_SERIAL serial)
function(add_subdirectory_if_kconfig dir)
  string(TOUPPER config_${dir} UPPER_CASE_CONFIG)
  add_subdirectory_ifdef(${UPPER_CASE_CONFIG} ${dir})
endfunction()

function(target_sources_if_kconfig target scope item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  target_sources_ifdef(${UPPER_CASE_CONFIG} ${target} ${scope} ${item})
endfunction()

function(zephyr_library_sources_if_kconfig item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  zephyr_library_sources_ifdef(${UPPER_CASE_CONFIG} ${item})
endfunction()

function(zephyr_sources_if_kconfig item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  zephyr_sources_ifdef(${UPPER_CASE_CONFIG} ${item})
endfunction()

# 2.2 Misc
#
# Parse a KConfig formatted file (typically named *.config) and
# introduce all the CONF_ variables into the CMake namespace
function(import_kconfig config_file)
  # Parse the lines prefixed with CONFIG_ in ${config_file}
  file(
    STRINGS
    ${config_file}
    DOT_CONFIG_LIST
    REGEX "^CONFIG_"
  )

  foreach (CONFIG ${DOT_CONFIG_LIST})
    # CONFIG looks like: CONFIG_NET_BUF=y

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


# 3.2. *_ifndef
# See 3.1 *_ifdef
function(set_ifndef variable value)
  if(NOT ${variable})
    set(${variable} ${value} PARENT_SCOPE)
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

# 3.2. *_option Compiler-compatibility checks
#
# Utility functions for silently omitting compiler flags when the
# compiler lacks support. *_cc_option was ported from KBuild, see
# cc-option in
# https://www.kernel.org/doc/Documentation/kbuild/makefiles.txt
#
function(target_cc_option target scope option)
  string(MAKE_C_IDENTIFIER check${option} check)
  check_c_compiler_flag(${option} ${check})
  target_compile_option_ifdef(${check} ${target} ${scope} ${option})
endfunction()

# Support an optional second option for when the first option is
# not supported.
function(target_cc_option_fallback target scope option1 option2)
  string(MAKE_C_IDENTIFIER check${option1} check)
  check_c_compiler_flag(${option1} ${check})
  if(${check})
    target_compile_options(${target} ${scope} ${option1})
  else()
    target_compile_options(${target} ${scope} ${option2})
  endif()
endfunction()

function(target_ld_options target scope)
  foreach(option ${ARGN})
    string(MAKE_C_IDENTIFIER check${option} check)
    check_c_compiler_flag(${option} ${check})
    target_link_libraries_ifdef(${check} ${target} ${scope} ${option})
  endforeach()
endfunction()

# 3.4. Debugging CMake

# Usage:
#   print(BOARD)
#
# will print: "BOARD: nrf52_pca10040"
function(print arg)
  message(STATUS "${arg}: ${${arg}}")
endfunction()

# Usage:
#   assert(ZEPHYR_GCC_VARIANT "ZEPHYR_GCC_VARIANT not set.")
#
# will cause a FATAL_ERROR and print an error message if the first
# expression is false
macro(assert test comment)
  if(NOT ${test})
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

# Usage:
#   Same as CMake's built-in execute_process() function except that
#   RESULT_VARIABLE may not be set.
#
#   https://cmake.org/cmake/help/v3.8/command/execute_process.html
#
# Will wrap execute_process() and ensure that if the command fails
# this causes the build to fail as well. As opposed to CMake's default
# behaviour of silently ignoring the failure.
function(execute_process_safely)
  execute_process(
    ${ARGN}
    RESULT_VARIABLE ret
  )
  if(ret)
    message(FATAL_ERROR "Executing cmd: '${ARGN}' resulted in error: '${ret}'")
  endif()
endfunction()
