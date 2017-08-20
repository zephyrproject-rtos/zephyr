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

# Add the existing CMake library 'library' to the global list of
# Zephyr CMake libraries.
function(zephyr_library library)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_LIBS ${library})
endfunction()
