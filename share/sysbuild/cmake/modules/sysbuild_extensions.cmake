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
#                             [GROUP <groups>]
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
# GROUP <groups>:            Add the application to each group in the `<groups>` list.
#                            Equivalent to: ExternalZephyrProject_Group(<g> INCLUDE <name>)
#                            for every `<g>` in `<groups>`.
# MAIN_APP:                  Flag indicating this application is the main application
#                            and where user defined settings should be passed on as-is
#                            except for multi image build flags.
#                            For example, -DCONF_FILES=<files> will be passed on to the
#                            MAIN_APP unmodified.
#
function(ExternalZephyrProject_Add)
  cmake_parse_arguments(ZBUILD "MAIN_APP" "APPLICATION;BOARD;BOARD_REVISION;SOURCE_DIR" "GROUP" ${ARGN})

  if(ZBUILD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "ExternalZephyrProject_Add(${ARGV0} <val> ...) given unknown arguments:"
      " ${ZBUILD_UNPARSED_ARGUMENTS}"
    )
  endif()

  set_property(GLOBAL APPEND PROPERTY sysbuild_images ${ZBUILD_APPLICATION})

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

  if(DEFINED ZBUILD_GROUP)
    foreach(group ${ZBUILD_GROUP})
      ExternalZephyrProject_Group(${group} INCLUDE ${ZBUILD_APPLICATION})
    endforeach()
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
#   ExternalZephyrProject_Group(<name>
#                               [INCLUDE <images>]
#                               [EXCLUDE <images>]
#                               [INCLUDE_FROM <groups>]
#                               [EXCLUDE_FROM <groups>]
#                               [IMMUTABLE]
#   )
#
# This function defines groups of external Zephyr projects. Images added using
# `ExternalZephyrProject_Add()` may inherit common properties of the group(s)
# they are assigned to.
#
# This function can either add a new group "<name>", or extend the definition of
# an existing group with that name. This lets multiple sources influence what a
# given group should represent in the multiimage build system. The final set of
# images/properties for every group is evaluated in `sysbuild_groups_resolve()`.
#
# <name>                 Name of the group.
# INCLUDE <images>:      Add a set of images to the group.
# EXCLUDE <images>:      Remove a set of images from the group.
# INCLUDE_FROM <groups>: INCLUDE all images belonging to each group in the list.
# EXCLUDE_FROM <groups>: EXCLUDE all images belonging to each group in the list.
# IMMUTABLE:             Disallow extending the definition of this group.
#                        Raises an error if the group already exists.
#
# Example use:
#   ExternalZephyrProject_Group(g_1
#                               INCLUDE i_1 i_2 i_3 i_4
#   )
#   ExternalZephyrProject_Group(g_2
#                               INCLUDE_FROM g_1
#                               EXCLUDE i_1 i_2
#   )
#   ExternalZephyrProject_Group(g_3
#                               INCLUDE_FROM g_1
#                               EXCLUDE_FROM g_2
#   )
#   ExternalZephyrProject_Group(g_4
#                               INCLUDE_FROM g_3
#                               EXCLUDE_FROM g_1
#   )
#
# In this example, the contents of each group will be as follows:
#   g_1: i_1, i_2, i_3, i_4
#   g_2: i_3, i_4
#   g_3: i_1, i_2
#   g_4: <empty>
#
function(ExternalZephyrProject_Group name)
  set(options IMMUTABLE)
  set(multi_args INCLUDE INCLUDE_FROM EXCLUDE EXCLUDE_FROM)
  cmake_parse_arguments(ZGROUP "${options}" "" "${multi_args}" ${ARGN})
  if(ZGROUP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "ExternalZephyrProject_Group(${ARGV0} <val> ...) given unknown arguments:"
      " ${ZGROUP_UNPARSED_ARGUMENTS}"
    )
  endif()

  set(target_name ${name}_group)
  if(NOT TARGET ${target_name})
    add_custom_target(${target_name})
    set_property(GLOBAL APPEND PROPERTY sysbuild_groups ${name})
  elseif(ZGROUP_IMMUTABLE)
    message(FATAL_ERROR
      "ExternalZephyrProject_Group(${ARGV0} <val> ...) failed to define "
      "group '${name}' as IMMUTABLE, because it already exists."
    )
  else()
    get_property(is_immutable TARGET ${target_name} PROPERTY IMMUTABLE)
    if(is_immutable)
      message(FATAL_ERROR
        "ExternalZephyrProject_Group(${ARGV0} <val> ...) group '${name}' is "
        "IMMUTABLE and cannot be extended."
      )
    endif()
  endif()

  set_property(TARGET ${target_name} PROPERTY IMMUTABLE "${ZGROUP_IMMUTABLE}")
  foreach(arg ${multi_args})
    if(DEFINED ZGROUP_${arg})
      set_property(TARGET ${target_name} APPEND PROPERTY ${arg} "${ZGROUP_${arg}}")
    endif()
  endforeach()
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

# Usage:
#   sysbuild_add_dependencies(<IMAGE | GROUP> <CONFIGURE | FLASH> <item> [<item-dependency> ...])
#
# This function makes an image or group depend on other images or groups in the
# configuration or flashing order.
#
# IMAGE:     Interpret <item> and every <item-dependency> as images, created
#            using `ExternalZephyrProject_Add()`. Each image "<item-dependency>"
#            will be ordered before the image named "<item>".
# GROUP:     Interpret <item> and every <item-dependency> as groups of images,
#            created using `ExternalZephyrProject_Group()`. For each group named
#            "<item-dependency>", its images will be ordered before the images
#            belonging to the group "<item>". If these groups have a set of
#            images in common, then those images will be placed in the middle.
#            In mock set notation:
#             - (<item-dependency> \ <item>) before (<item-dependency> & <item>)
#             - (<item-dependency> & <item>) before (<item> \ <item-dependency>)
# CONFIGURE: Add CMake configuration dependencies. This will determine the order
#            in which `ExternalZephyrProject_Cmake()` will be called, as well as
#            the order of images passed to all PRE_CMAKE and POST_CMAKE hooks.
# FLASH:     Add flashing dependencies. This will determine the order in which
#            all images will appear in `domains.yaml`, as well as the order of
#            images passed to all PRE_DOMAINS and POST_DOMAINS hooks.
#
# Note: specifying dependencies on non-existent images and groups is allowed.
# This makes it possible to define relationships between various images/groups
# in the build system, regardless of whether they are present in the current
# multiimage build.
#
function(sysbuild_add_dependencies item_type dependency_type image)
  set(valid_item_types IMAGE GROUP)
  if(NOT item_type IN_LIST valid_item_types)
    list(JOIN valid_item_types ", " valid_item_types)
    message(FATAL_ERROR "sysbuild_add_dependencies(...) item type "
                        "${item_type} must be one of the following: "
                        "${valid_item_types}"
    )
  endif()

  set(valid_dependency_types CONFIGURE FLASH)
  if(NOT dependency_type IN_LIST valid_dependency_types)
    list(JOIN valid_dependency_types ", " valid_dependency_types)
    message(FATAL_ERROR "sysbuild_add_dependencies(...) dependency type "
                        "${dependency_type} must be one of the following: "
                        "${valid_dependency_types}"
    )
  endif()

  set(property_name sysbuild_deps_${item_type}_${dependency_type}_${image})
  set_property(GLOBAL APPEND PROPERTY ${property_name} ${ARGN})
endfunction()

# Internal helper macro. Add edges between two sets of vertices in a dependency
# graph. Define "hint" strings explaining every edge in current scope.
macro(add_dependencies_to_graph)
  cmake_parse_arguments(DEP "" "HINT;OUTPUT_EDGES" "VERTICES_FROM;VERTICES_TO" ${ARGN})

  # Create three disjoint sets. Let MID be the intersection of FROM and TO.
  set(DEP_VERTICES_MID ${DEP_VERTICES_FROM})
  list(REMOVE_ITEM DEP_VERTICES_FROM ${DEP_VERTICES_TO})
  list(REMOVE_ITEM DEP_VERTICES_MID ${DEP_VERTICES_FROM})
  list(REMOVE_ITEM DEP_VERTICES_TO ${DEP_VERTICES_MID})

  foreach(from ${DEP_VERTICES_FROM})
    foreach(to ${DEP_VERTICES_MID} ${DEP_VERTICES_TO})
      list(APPEND ${DEP_OUTPUT_EDGES} ${from},${to})
      list(APPEND hint_${from}_${to} "${DEP_HINT}")
    endforeach()
  endforeach()
  foreach(from ${DEP_VERTICES_MID})
    foreach(to ${DEP_VERTICES_TO})
      list(APPEND ${DEP_OUTPUT_EDGES} ${from},${to})
      list(APPEND hint_${from}_${to} "${DEP_HINT}")
    endforeach()
  endforeach()
endmacro()

# Internal helper macro. If `<cycle>` is non-empty, then display the `<message>`
# with helpful information about the dependency graph where the cycle was found.
# There should be a `hint_<from>_<to>` variable for every "<from>,<to>" edge,
# defined in the current scope, containing helper strings.
macro(assert_no_cyclic_dependencies cycle message)
  if(cycle)
    list(LENGTH cycle cycle_length)
    math(EXPR cycle_length "${cycle_length} - 1")

    message("Found cyclic dependencies:\n")
    set(i 0)
    while(i LESS cycle_length)
      list(GET cycle ${i} image_0)
      math(EXPR i "${i} + 1")
      list(GET cycle ${i} image_1)

      message("${image_1} <- ${image_0}")
      foreach(hint ${hint_${image_0}_${image_1}})
        message("- ${hint}")
      endforeach()
      message("")
    endwhile()

    message(FATAL_ERROR "${message}")
  endif()
endmacro()

# Usage:
#   sysbuild_groups_resolve(GROUPS <groups> IMAGES <images>)
#
# This function evaluates which images belong to each group in `<groups>`.
#
# First, it resolves the INCLUDE, EXCLUDE, INCLUDE_FROM, EXCLUDE_FROM directives
# passed to `ExternalZephyrProject_Group()`. Then, it restricts every group to
# be a subset of all provided `<images>`.
#
function(sysbuild_groups_resolve)
  cmake_parse_arguments(SGR "" "" "GROUPS;IMAGES" ${ARGN})
  zephyr_check_arguments_required_all("sysbuild_groups_resolve" SGR GROUPS IMAGES)

  # Groups need to be sorted, so that if <a> INCLUDE_FROM/EXCLUDE_FROM <b>, then
  # the images belonging to group <b> will be known before resolving group <a>.
  set(V ${SGR_GROUPS})
  set(E)
  foreach(group ${V})
    if(NOT TARGET ${group}_group)
      message(FATAL_ERROR
        "sysbuild_groups_resolve(...) group ${group} does not exist. "
        "Remember to call ExternalZephyrProject_Group(${group} ...) first."
      )
    endif()
    get_property(${group}_include      TARGET ${group}_group PROPERTY INCLUDE)
    get_property(${group}_exclude      TARGET ${group}_group PROPERTY EXCLUDE)
    get_property(${group}_include_from TARGET ${group}_group PROPERTY INCLUDE_FROM)
    get_property(${group}_exclude_from TARGET ${group}_group PROPERTY EXCLUDE_FROM)

    foreach(dependent ${${group}_include_from})
      add_dependencies_to_graph(
        HINT "ExternalZephyrProject_Group(${group} ... INCLUDE_FROM ${dependent} ...)"
        VERTICES_FROM ${dependent}
        VERTICES_TO ${group}
        OUTPUT_EDGES E
      )
    endforeach()
    foreach(dependent ${${group}_exclude_from})
      add_dependencies_to_graph(
        HINT "ExternalZephyrProject_Group(${group} ... EXCLUDE_FROM ${dependent} ...)"
        VERTICES_FROM ${dependent}
        VERTICES_TO ${group}
        OUTPUT_EDGES E
      )
    endforeach()
  endforeach()

  zephyr_topological_sort(VERTICES "${V}" EDGES "${E}" OUTPUT_RESULT sorted OUTPUT_CYCLE cycle)
  assert_no_cyclic_dependencies(cycle "Failed to resolve sysbuild groups.")

  # Evaluate the images for each group in isolation.
  foreach(group ${sorted})
    # Apply inclusion filtering.
    set(images ${${group}_include})
    foreach(dependent ${${group}_include_from})
      if(dependent IN_LIST sorted)
        get_property(dependent_images TARGET ${dependent}_group PROPERTY IMAGES_RESOLVED)
        list(APPEND images ${dependent_images})
      endif()
    endforeach()

    # Apply exclusion filtering.
    list(REMOVE_ITEM images ${${group}_exclude})
    foreach(dependent ${${group}_exclude_from})
      if(dependent IN_LIST sorted)
        get_property(dependent_images TARGET ${dependent}_group PROPERTY IMAGES_RESOLVED)
        list(REMOVE_ITEM images ${dependent_images})
      endif()
    endforeach()

    # Remove non-existent images.
    set(images_diff ${images})
    list(REMOVE_ITEM images_diff ${SGR_IMAGES})
    list(REMOVE_ITEM images ${images_diff})

    list(REMOVE_DUPLICATES images)
    set_property(TARGET ${group}_group PROPERTY IMAGES_RESOLVED "${images}")
  endforeach()
endfunction()

# Usage:
#   sysbuild_images_list(<variable> GROUP <group>)
#
# This function will list all images belonging to a given `<group>`. The result
# will be returned in `<variable>`.
#
# It should not be called before `sysbuild_groups_resolve()` because group
# contents are unknown before that point. If `<group>` is unresolved or even
# undefined, then an empty list will be returned.
#
function(sysbuild_images_list variable)
  cmake_parse_arguments(SIL "" "GROUP" "" ${ARGN})
  zephyr_check_arguments_required_all("sysbuild_groups_resolve" SIL GROUP)

  set(output "")

  set(target_name ${SIL_GROUP}_group)
  if(TARGET ${target_name})
    get_property(is_resolved TARGET ${target_name} PROPERTY IMAGES_RESOLVED SET)
    if(NOT is_resolved)
      message(WARNING
        "sysbuild_images_list(...) group ${SIL_GROUP} exists, "
        "but its contents are not yet resolved."
      )
    endif()
    get_property(output TARGET ${target_name} PROPERTY IMAGES_RESOLVED)
  endif()

  set(${variable} ${output} PARENT_SCOPE)
endfunction()

# Usage:
#   sysbuild_images_order(<variable> <CONFIGURE | FLASH> IMAGES <images> [GROUPS <groups>])
#
# This function will sort the provided `<images>` to satisfy the dependencies
# specified using `sysbuild_add_dependencies()`. The result will be returned in
# `<variable>`.
#
# Note: if `<groups>` are given, then this function should not be called before
# `sysbuild_groups_resolve(GROUPS <groups>)`.
#
# Raises a fatal error if cyclic dependencies are found.
#
function(sysbuild_images_order variable dependency_type)
  cmake_parse_arguments(SIS "" "" "IMAGES;GROUPS" ${ARGN})
  zephyr_check_arguments_required_all("sysbuild_images_order" SIS IMAGES)

  set(valid_dependency_types CONFIGURE FLASH)
  if(NOT dependency_type IN_LIST valid_dependency_types)
    list(JOIN valid_dependency_types ", " valid_dependency_types)
    message(FATAL_ERROR "sysbuild_images_order(...) dependency type "
                        "${dependency_type} must be one of the following: "
                        "${valid_dependency_types}"
    )
  endif()

  # Generate dependency graph.
  set(V ${SIS_IMAGES})
  set(E)
  foreach(image ${V})
    set(property_name sysbuild_deps_IMAGE_${dependency_type}_${image})
    get_property(image_dependencies GLOBAL PROPERTY ${property_name})

    foreach(dependent ${image_dependencies})
      add_dependencies_to_graph(
        HINT "sysbuild_add_dependencies(IMAGE ${dependency_type} ${image} ... ${dependent} ...)"
        VERTICES_FROM ${dependent}
        VERTICES_TO ${image}
        OUTPUT_EDGES E
      )
    endforeach()
  endforeach()

  foreach(group ${SIS_GROUPS})
    sysbuild_images_list(images GROUP ${group})

    set(property_name sysbuild_deps_GROUP_${dependency_type}_${group})
    get_property(group_dependencies GLOBAL PROPERTY ${property_name})
    foreach(group_dependent ${group_dependencies})
      if(group_dependent IN_LIST SIS_GROUPS)
        sysbuild_images_list(dependents GROUP ${group_dependent})
        add_dependencies_to_graph(
          HINT "sysbuild_add_dependencies(GROUP ${dependency_type} ${group} ... ${group_dependent} ...)"
          VERTICES_FROM ${dependents}
          VERTICES_TO ${images}
          OUTPUT_EDGES E
        )
      endif()
    endforeach()
  endforeach()

  zephyr_topological_sort(VERTICES "${V}" EDGES "${E}" OUTPUT_RESULT sorted OUTPUT_CYCLE cycle)
  assert_no_cyclic_dependencies(cycle "Failed to produce ${dependency_type} ordering for sysbuild images.")

  set(${variable} ${sorted} PARENT_SCOPE)
endfunction()
