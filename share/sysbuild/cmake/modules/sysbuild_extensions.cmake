# Copyright (c) 2021-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Usage:
#   load_cache(IMAGE <image> BINARY_DIR <dir>)
#
# This function will load the CMakeCache.txt file from the binary directory
# given by the BINARY_DIR argument.
#
# All CMake cache variables are stored in a custom target which is identified by
# the name given as value to the IMAGE argument.
#
# IMAGE:      image name identifying the cache for later sysbuild_get() lookup calls.
# BINARY_DIR: binary directory (build dir) containing the CMakeCache.txt file to load.
function(load_cache)
  set(single_args IMAGE BINARY_DIR)
  cmake_parse_arguments(LOAD_CACHE "" "${single_args}" "" ${ARGN})

  if(NOT TARGET ${LOAD_CACHE_IMAGE}_cache)
    add_custom_target(${LOAD_CACHE_IMAGE}_cache)
  endif()
  file(STRINGS "${LOAD_CACHE_BINARY_DIR}/CMakeCache.txt" cache_strings)
  foreach(str ${cache_strings})
    # Using a regex for matching whole 'VAR_NAME:TYPE=VALUE' will strip semi-colons
    # thus resulting in lists to become strings.
    # Therefore we first fetch VAR_NAME and TYPE, and afterwards extract
    # remaining of string into a value that populates the property.
    # This method ensures that both quoted values and ;-separated list stays intact.
    string(REGEX MATCH "([^:]*):([^=]*)=" variable_identifier ${str})
    if(NOT "${variable_identifier}" STREQUAL "")
      string(LENGTH ${variable_identifier} variable_identifier_length)
      string(SUBSTRING "${str}" ${variable_identifier_length} -1 variable_value)
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache APPEND PROPERTY "CACHE:VARIABLES" "${CMAKE_MATCH_1}")
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache PROPERTY "${CMAKE_MATCH_1}:TYPE" "${CMAKE_MATCH_2}")
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache PROPERTY "${CMAKE_MATCH_1}" "${variable_value}")
    endif()
  endforeach()
endfunction()

# Usage:
#   sysbuild_get(<variable> IMAGE <image> [VAR <image-variable>] <KCONFIG|CACHE>)
#
# This function will return the variable found in the CMakeCache.txt file
# identified by image.
# If `VAR` is provided, the name given as parameter will be looked up, but if
# `VAR` is not given, the `<variable>` name provided will be used both for
# lookup and value return.
#
# The result will be returned in `<variable>`.
#
# Example use:
#   sysbuild_get(PROJECT_NAME IMAGE my_sample CACHE)
#     will lookup PROJECT_NAME from the CMakeCache identified by `my_sample` and
#     and return the value in the local variable `PROJECT_NAME`.
#
#   sysbuild_get(my_sample_PROJECT_NAME IMAGE my_sample VAR PROJECT_NAME CACHE)
#     will lookup PROJECT_NAME from the CMakeCache identified by `my_sample` and
#     and return the value in the local variable `my_sample_PROJECT_NAME`.
#
#   sysbuild_get(my_sample_CONFIG_FOO IMAGE my_sample VAR CONFIG_FOO KCONFIG)
#     will lookup CONFIG_FOO from the KConfig identified by `my_sample` and
#     and return the value in the local variable `my_sample_CONFIG_FOO`.
#
# <variable>: variable used for returning CMake cache value. Also used as lookup
#             variable if `VAR` is not provided.
# IMAGE:      image name identifying the cache to use for variable lookup.
# VAR:        name of the CMake cache variable name to lookup.
# KCONFIG:    Flag indicating that a Kconfig setting should be fetched.
# CACHE:      Flag indicating that a CMake cache variable should be fetched.
function(sysbuild_get variable)
  cmake_parse_arguments(GET_VAR "CACHE;KCONFIG" "IMAGE;VAR" "" ${ARGN})

  if(NOT DEFINED GET_VAR_IMAGE)
    message(FATAL_ERROR "sysbuild_get(...) requires IMAGE.")
  endif()

  if(DEFINED ${variable})
    message(WARNING "Return variable ${variable} already defined with a value. "
                    "sysbuild_get(${variable} ...) may overwrite existing value. "
		    "Please use sysbuild_get(<variable> ... VAR <image-variable>) "
		    "where <variable> is undefined."
    )
  endif()

  if(NOT DEFINED GET_VAR_VAR)
    set(GET_VAR_VAR ${variable})
  endif()

  if(GET_VAR_KCONFIG)
    set(variable_target ${GET_VAR_IMAGE})
  elseif(GET_VAR_CACHE)
    set(variable_target ${GET_VAR_IMAGE}_cache)
  else()
    message(WARNING "<CACHE> or <KCONFIG> not specified, defaulting to CACHE")
    set(variable_target ${GET_VAR_IMAGE}_cache)
  endif()

  get_property(${GET_VAR_IMAGE}_${GET_VAR_VAR} TARGET ${variable_target} PROPERTY ${GET_VAR_VAR})
  if(DEFINED ${GET_VAR_IMAGE}_${GET_VAR_VAR})
    set(${variable} ${${GET_VAR_IMAGE}_${GET_VAR_VAR}} PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   ExternalZephyrProject_Add(APPLICATION <name>
#                             SOURCE_DIR <dir>
#                             [BOARD <board> [BOARD_REVISION <revision>]]
#                             [MAIN_APP]
#   )
#
# This function includes a Zephyr based build system into the multiimage
# build system
#
# APPLICATION: <name>:       Name of the application, name will also be used for build
#                            folder of the application
# SOURCE_DIR <dir>:          Source directory of the application
# BOARD <board>:             Use <board> for application build instead user defined BOARD.
# BOARD_REVISION <revision>: Use <revision> of <board> for application (only valid if
#                            <board> is also supplied).
# MAIN_APP:                  Flag indicating this application is the main application
#                            and where user defined settings should be passed on as-is
#                            except for multi image build flags.
#                            For example, -DCONF_FILES=<files> will be passed on to the
#                            MAIN_APP unmodified.
#
function(ExternalZephyrProject_Add)
  cmake_parse_arguments(ZBUILD "MAIN_APP" "APPLICATION;BOARD;BOARD_REVISION;SOURCE_DIR" "" ${ARGN})

  if(ZBUILD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "ExternalZephyrProject_Add(${ARGV0} <val> ...) given unknown arguments:"
      " ${ZBUILD_UNPARSED_ARGUMENTS}"
    )
  endif()

  set(sysbuild_image_conf_dir ${APP_DIR}/sysbuild)
  set(sysbuild_image_name_conf_dir ${APP_DIR}/sysbuild/${ZBUILD_APPLICATION})
  # User defined `-D<image>_CONF_FILE=<file.conf>` takes precedence over anything else.
  if (NOT ${ZBUILD_APPLICATION}_CONF_FILE)
    if(EXISTS ${sysbuild_image_name_conf_dir})
      set(${ZBUILD_APPLICATION}_APPLICATION_CONFIG_DIR ${sysbuild_image_name_conf_dir}
          CACHE INTERNAL "Application configuration dir controlled by sysbuild"
      )
    endif()

     # Check for sysbuild related configuration fragments.
     # The contents of these are appended to the image existing configuration
     # when user is not specifying custom fragments.
    if(NOT "${CONF_FILE_BUILD_TYPE}" STREQUAL "")
      set(sysbuil_image_conf_fragment ${sysbuild_image_conf_dir}/${ZBUILD_APPLICATION}_${CONF_FILE_BUILD_TYPE}.conf)
    else()
      set(sysbuild_image_conf_fragment ${sysbuild_image_conf_dir}/${ZBUILD_APPLICATION}.conf)
    endif()

    if (NOT ${ZBUILD_APPLICATION}_OVERLAY_CONFIG AND EXISTS ${sysbuild_image_conf_fragment})
      set(${ZBUILD_APPLICATION}_OVERLAY_CONFIG ${sysbuild_image_conf_fragment}
          CACHE INTERNAL "Kconfig fragment defined by main application"
      )
    endif()

    # Check for overlay named <ZBUILD_APPLICATION>.overlay.
    set(sysbuild_image_dts_overlay ${sysbuild_image_conf_dir}/${ZBUILD_APPLICATION}.overlay)
    if (NOT ${ZBUILD_APPLICATION}_DTC_OVERLAY_FILE AND EXISTS ${sysbuild_image_dts_overlay})
      set(${ZBUILD_APPLICATION}_DTC_OVERLAY_FILE ${sysbuild_image_dts_overlay}
          CACHE INTERNAL "devicetree overlay file defined by main application"
      )
    endif()
  endif()
  # CMake variables which must be known by all Zephyr CMake build systems
  # Those are settings which controls the build and must be known to CMake at
  # invocation time, and thus cannot be passed though the sysbuild cache file.
  set(
    shared_cmake_variables_list
    CMAKE_BUILD_TYPE
    CMAKE_VERBOSE_MAKEFILE
  )

  set(sysbuild_cache_file ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}_sysbuild_cache.txt)

  set(shared_cmake_vars_argument)
  foreach(shared_var ${shared_cmake_variables_list})
    if(DEFINED CACHE{${ZBUILD_APPLICATION}_${shared_var}})
      get_property(var_type  CACHE ${ZBUILD_APPLICATION}_${shared_var} PROPERTY TYPE)
      list(APPEND shared_cmake_vars_argument
           "-D${shared_var}:${var_type}=$CACHE{${ZBUILD_APPLICATION}_${shared_var}}"
      )
    elseif(DEFINED CACHE{${shared_var}})
      get_property(var_type  CACHE ${shared_var} PROPERTY TYPE)
      list(APPEND shared_cmake_vars_argument
           "-D${shared_var}:${var_type}=$CACHE{${shared_var}}"
      )
    endif()
  endforeach()

  foreach(kconfig_target
      menuconfig
      hardenconfig
      guiconfig
      ${EXTRA_KCONFIG_TARGETS}
      )

    if(NOT ZBUILD_MAIN_APP)
      set(image_prefix "${ZBUILD_APPLICATION}_")
    endif()

    add_custom_target(${image_prefix}${kconfig_target}
      ${CMAKE_MAKE_PROGRAM} ${kconfig_target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}
      USES_TERMINAL
      )
  endforeach()
  include(ExternalProject)
  ExternalProject_Add(
    ${ZBUILD_APPLICATION}
    SOURCE_DIR ${ZBUILD_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}
    CONFIGURE_COMMAND ""
    CMAKE_ARGS -DSYSBUILD:BOOL=True
               -DSYSBUILD_CACHE:FILEPATH=${sysbuild_cache_file}
               ${shared_cmake_vars_argument}
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
  )
  set_target_properties(${ZBUILD_APPLICATION} PROPERTIES CACHE_FILE ${sysbuild_cache_file})
  if(ZBUILD_MAIN_APP)
    set_target_properties(${ZBUILD_APPLICATION} PROPERTIES MAIN_APP True)
  endif()

  if(DEFINED ZBUILD_BOARD)
    # Only set image specific board if provided.
    # The sysbuild BOARD is exported through sysbuild cache, and will be used
    # unless <image>_BOARD is defined.
    if(DEFINED ZBUILD_BOARD_REVISION)
      # Use provided board revision
      set_target_properties(${ZBUILD_APPLICATION} PROPERTIES BOARD ${ZBUILD_BOARD}@${ZBUILD_BOARD_REVISION})
    else()
      set_target_properties(${ZBUILD_APPLICATION} PROPERTIES BOARD ${ZBUILD_BOARD})
    endif()
  elseif(DEFINED ZBUILD_BOARD_REVISION)
    message(FATAL_ERROR
      "ExternalZephyrProject_Add(... BOARD_REVISION ${ZBUILD_BOARD_REVISION})"
      " requires BOARD."
    )
  elseif(DEFINED BOARD_REVISION)
    # Include build revision for target image
    set_target_properties(${ZBUILD_APPLICATION} PROPERTIES BOARD ${BOARD}@${BOARD_REVISION})
  endif()
endfunction()

# Usage:
#   ExternalZephyrProject_Cmake(APPLICATION <name>)
#
# This function invokes the CMake configure step on an external Zephyr project
# which has been added at an earlier stage using `ExternalZephyrProject_Add()`
#
# If the application is not due to ExternalZephyrProject_Add() being called,
# then an error is raised.
#
# APPLICATION: <name>: Name of the application.
#
function(ExternalZephyrProject_Cmake)
  cmake_parse_arguments(ZCMAKE "" "APPLICATION" "" ${ARGN})

  if(ZBUILD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "ExternalZephyrProject_Cmake(${ARGV0} <val> ...) given unknown arguments:"
      " ${ZBUILD_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(NOT DEFINED ZCMAKE_APPLICATION)
    message(FATAL_ERROR "Missing required argument: APPLICATION")
  endif()

  if(NOT TARGET ${ZCMAKE_APPLICATION})
    message(FATAL_ERROR
      "${ZCMAKE_APPLICATION} does not exists. Remember to call "
      "ExternalZephyrProject_Add(APPLICATION ${ZCMAKE_APPLICATION} ...) first."
    )
  endif()

  set(image_banner "* Running CMake for ${ZCMAKE_APPLICATION} *")
  string(LENGTH "${image_banner}" image_banner_width)
  string(REPEAT "*" ${image_banner_width} image_banner_header)
  message(STATUS "\n   ${image_banner_header}\n"
                 "   ${image_banner}\n"
                 "   ${image_banner_header}\n"
  )

  ExternalProject_Get_Property(${ZCMAKE_APPLICATION} SOURCE_DIR BINARY_DIR CMAKE_ARGS)
  get_target_property(${ZCMAKE_APPLICATION}_CACHE_FILE ${ZCMAKE_APPLICATION} CACHE_FILE)
  get_target_property(${ZCMAKE_APPLICATION}_BOARD      ${ZCMAKE_APPLICATION} BOARD)
  get_target_property(${ZCMAKE_APPLICATION}_MAIN_APP   ${ZCMAKE_APPLICATION} MAIN_APP)

  get_cmake_property(sysbuild_cache CACHE_VARIABLES)
  foreach(var_name ${sysbuild_cache})
    if(NOT "${var_name}" MATCHES "^CMAKE_.*")
      # Perform a dummy read to prevent a false warning about unused variables
      # being emitted due to a cmake bug: https://gitlab.kitware.com/cmake/cmake/-/issues/24555
      set(unused_tmp_var ${${var_name}})

      # We don't want to pass internal CMake variables.
      # Required CMake variable to be passed, like CMAKE_BUILD_TYPE must be
      # passed using `-D` on command invocation.
      get_property(var_type CACHE ${var_name} PROPERTY TYPE)
      set(cache_entry "${var_name}:${var_type}=$CACHE{${var_name}}")
      string(REPLACE ";" "\;" cache_entry "${cache_entry}")
      list(APPEND sysbuild_cache_strings "${cache_entry}\n")
    endif()
  endforeach()
  list(APPEND sysbuild_cache_strings "SYSBUILD_NAME:STRING=${ZCMAKE_APPLICATION}\n")

  if(${ZCMAKE_APPLICATION}_MAIN_APP)
    list(APPEND sysbuild_cache_strings "SYSBUILD_MAIN_APP:BOOL=True\n")
  endif()

  if(${ZCMAKE_APPLICATION}_BOARD)
    # Only set image specific board if provided.
    # The sysbuild BOARD is exported through sysbuild cache, and will be used
    # unless <image>_BOARD is defined.
    list(APPEND sysbuild_cache_strings
         "${ZCMAKE_APPLICATION}_BOARD:STRING=${${ZCMAKE_APPLICATION}_BOARD}\n"
    )
  endif()

  file(WRITE ${${ZCMAKE_APPLICATION}_CACHE_FILE}.tmp ${sysbuild_cache_strings})
  zephyr_file_copy(${${ZCMAKE_APPLICATION}_CACHE_FILE}.tmp
                   ${${ZCMAKE_APPLICATION}_CACHE_FILE} ONLY_IF_DIFFERENT
  )

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G${CMAKE_GENERATOR}
        ${CMAKE_ARGS}
      -B${BINARY_DIR}
      -S${SOURCE_DIR}
    RESULT_VARIABLE   return_val
    WORKING_DIRECTORY ${BINARY_DIR}
  )

  if(return_val)
    message(FATAL_ERROR
            "CMake configure failed for Zephyr project: ${ZCMAKE_APPLICATION}\n"
            "Location: ${SOURCE_DIR}"
    )
  endif()
  load_cache(IMAGE ${ZCMAKE_APPLICATION} BINARY_DIR ${BINARY_DIR})
  import_kconfig(CONFIG_ ${BINARY_DIR}/zephyr/.config TARGET ${ZCMAKE_APPLICATION})
endfunction()

# Usage:
#   sysbuild_module_call(<hook> MODULES <modules> [IMAGES <images>] [EXTRA_ARGS <arguments>])
#
# This function invokes the sysbuild hook provided as <hook> for <modules>.
#
# If `IMAGES` is passed, then the provided list of of images will be passed to
# the hook.
#
# `EXTRA_ARGS` can be used to pass extra arguments to the hook.
#
# Valid <hook> values:
# PRE_CMAKE   : Invoke pre-CMake call for modules before CMake configure is invoked for images
# POST_CMAKE  : Invoke post-CMake call for modules after CMake configure has been invoked for images
# PRE_DOMAINS : Invoke pre-domains call for modules before creating domains yaml.
# POST_DOMAINS: Invoke post-domains call for modules after creation of domains yaml.
#
function(sysbuild_module_call)
  set(options "PRE_CMAKE;POST_CMAKE;PRE_DOMAINS;POST_DOMAINS")
  set(multi_args "MODULES;IMAGES;EXTRA_ARGS")
  cmake_parse_arguments(SMC "${options}" "${test_args}" "${multi_args}" ${ARGN})

  zephyr_check_flags_required("sysbuild_module_call" SMC ${options})
  zephyr_check_flags_exclusive("sysbuild_module_call" SMC ${options})

  foreach(call ${options})
    if(SMC_${call})
      foreach(module ${SMC_MODULES})
        if(COMMAND ${module}_${call})
          cmake_language(CALL ${module}_${call} IMAGES ${SMC_IMAGES} ${SMC_EXTRA_ARGS})
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()

# Usage:
#   sysbuild_cache_set(VAR <variable> [APPEND [REMOVE_DUPLICATES]] <value>)
#
# This function will set the specified value of the sysbuild cache variable in
# the CMakeCache.txt file which can then be accessed by images.
# `VAR` specifies the variable name to set/update.
#
# The result will be returned in `<variable>`.
#
# Example use:
#   sysbuild_cache_set(VAR ATTRIBUTES APPEND REMOVE_DUPLICATES battery)
#     Will add the item `battery` to the `ATTRIBUTES` variable as a new element
#     in the list in the CMakeCache and remove any duplicates from the list.
#
# <variable>:        Name of variable in CMake cache.
# APPEND:            If specified then will append the supplied data to the
#                    existing value as a list.
# REMOVE_DUPLICATES: If specified then remove duplicate entries contained
#                    within the list before saving to the cache.
# <value>:           Value to set/update.
function(sysbuild_cache_set)
  cmake_parse_arguments(VARS "APPEND;REMOVE_DUPLICATES" "VAR" "" ${ARGN})

  zephyr_check_arguments_required(sysbuild_cache_set VARS VAR)

  if(NOT VARS_UNPARSED_ARGUMENTS AND VARS_APPEND)
    # Nothing to append so do nothing
    return()
  elseif(VARS_REMOVE_DUPLICATES AND NOT VARS_APPEND)
    message(FATAL_ERROR
            "sysbuild_set(VAR <var> APPEND REMOVE_DUPLICATES ...) missing required APPEND option")
  endif()

  get_property(var_type CACHE ${VARS_VAR} PROPERTY TYPE)
  get_property(var_help CACHE ${VARS_VAR} PROPERTY HELPSTRING)

  # If the variable type is not set, use UNINITIALIZED which will not apply any
  # specific formatting.
  if(NOT var_type)
    set(var_type "UNINITIALIZED")
  endif()

  if(VARS_APPEND)
    set(var_new "$CACHE{${VARS_VAR}}")

    # Search for these exact items in the existing value and prevent adding
    # them if they are already present which avoids issues with double addition
    # when cmake is reran.
    string(FIND "$CACHE{${VARS_VAR}}" "${VARS_UNPARSED_ARGUMENTS}" index)

    if(NOT ${index} EQUAL -1)
      return()
    endif()

    list(APPEND var_new "${VARS_UNPARSED_ARGUMENTS}")

    if(VARS_REMOVE_DUPLICATES)
      list(REMOVE_DUPLICATES var_new)
    endif()
  else()
    set(var_new "${VARS_UNPARSED_ARGUMENTS}")
  endif()

  set(${VARS_VAR} "${var_new}" CACHE "${var_type}" "${var_help}" FORCE)
endfunction()
