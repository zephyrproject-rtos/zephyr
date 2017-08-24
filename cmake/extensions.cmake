include(CheckCCompilerFlag)

########################################################
# CMake-generic extensions
########################################################

# Conditionally executing CMake functions with oneliners
# e.g. if(x) sqrt(10); would be sqrt_ifdef(x 10)
#
# <function-name>_ifdef()
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
    target_cc_options(${target} ${scope} ${option})
  endif()
endfunction()

function(cc_option_ifdef feature_toggle option)
  if(${feature_toggle})
    cc_option(${option})
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

function(zephyr_library_compile_definitions_ifdef feature_toggle item)
  if(${${feature_toggle}})
    zephyr_library_compile_definitions(${item} ${ARGN})
  endif()
endfunction()

# <function-name>_ifndef()
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

# Utility functions for silently omitting a compiler flag when the
# compiler does not support it.
function(cc_option option)
  string(MAKE_C_IDENTIFIER check${option} check)
  check_c_compiler_flag(${option} ${check})
  add_compile_option_ifdef(${check} ${option})
endfunction()

function(target_cc_option target scope option)
  string(MAKE_C_IDENTIFIER check${option} check)
  check_c_compiler_flag(${option} ${check})
  target_compile_option_ifdef(${check} ${target} ${scope} ${option})
endfunction()

function(cc_disable_warning warning)
  string(MAKE_C_IDENTIFIER check_warning_${warning} check)
  check_c_compiler_flag(-W${warning} ${check})
  add_compile_option_ifdef(${check} -Wno-${warning})
endfunction()

function(ld_option option)
  # TODO: figure out how to test linker flags
  # string(MAKE_C_IDENTIFIER check${option} check)
  # check_c_compiler_flag(${option} ${check})
  # if(${check})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${option}" PARENT_SCOPE)
  # endif()
endfunction()

#
# Misc.
#
function(target_object_link_libraries target)
  foreach(library ${ARGN})
    get_property(target_options     TARGET ${library} PROPERTY INTERFACE_COMPILE_OPTIONS)
    get_property(target_definitions TARGET ${library} PROPERTY INTERFACE_COMPILE_DEFINITIONS)
    get_property(target_directories TARGET ${library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} PRIVATE ${target_directories})
    target_compile_options    (${target} PRIVATE ${target_options})
    target_compile_definitions(${target} PRIVATE ${target_definitions})
  endforeach()
endfunction()

########################################################
# Kconfig-aware extensions
########################################################
function(add_subdirectory_if_kconfig dir)
  string(TOUPPER config_${dir} UPPER_CASE_CONFIG)
  add_subdirectory_ifdef(${UPPER_CASE_CONFIG} ${dir})
endfunction()

function(target_sources_if_kconfig target scope item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  target_sources_ifdef(${UPPER_CASE_CONFIG} ${target} ${scope} ${item})
endfunction()

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
# Zephyr-aware extensions
########################################################

# Zephyr-library-aware extentions
#
# Zephyr libraries use CMake's library concept and a set of
# assumptions about how zephyr code is organized to cut down on
# boilerplate code.
#
# Zephyr libraries are created and modified by the below functions.
#

macro(zephyr_library)
  # Remove the prefix (/home/sebo/zephyr/driver/serial/CMakeLists.txt => driver/serial/CMakeLists.txt)
  file(RELATIVE_PATH name $ENV{ZEPHYR_BASE} ${CMAKE_CURRENT_LIST_FILE})

  # Remove the filename (driver/serial/CMakeLists.txt => driver/serial)
  get_filename_component(name ${name} DIRECTORY)

  # Replace / with __ (driver/serial => driver__serial)
  string(REGEX REPLACE "/" "__" name ${name})

  zephyr_library_named(${name})
endmacro()

macro(zephyr_library_named name)
  # This is a macro because we need add_library() to be executed
  # within the scope of the caller.
  set(ZEPHYR_CURRENT_LIBRARY ${name})
  add_library(${name} STATIC "")

  zephyr_append_cmake_library(${name})

  if(${name} STREQUAL "zephyr")
  else()
    target_link_libraries(${name} zephyr)
  endif()
endmacro()

#
# zephyr_library versions of normal CMake target_<func> functions
#
function(zephyr_library_sources source)
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(zephyr_library_include_directories scope item)
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} ${scope} ${item} ${ARGN})
endfunction()

function(zephyr_library_link_libraries item)
  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} ${item} ${ARGN})
endfunction()

function(zephyr_library_compile_definitions item)
  target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

# Add the existing CMake library 'library' to the global list of
# Zephyr CMake libraries. This done automatically by zephyr_library()
# but must called explicitly on CMake libraries that are not created
# by zephyr_library().
function(zephyr_append_cmake_library library)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_LIBS ${library})
endfunction()

# zephyr-aware extentions
#
# The following functions are for modifying the Zephyr library called
# "zephyr". zephyr is a catchall CMake library for source files that
# can be built purely with the include paths, defines, and other
# compiler flags that all zephyr source files use.
#

# Usage:
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
function(zephyr_sources)
  foreach(arg ${ARGV})
    target_sources(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
  endforeach()
endfunction()
