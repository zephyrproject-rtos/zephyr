# Copyright (c) 2022-2023, Nordic Semiconductor ASA
# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

# Searches for installed Zephyr SDKs from the defined path.
# The find_package process is executed for each SDK found,
# and the relevant variables are set.
# If a positive value is specified for the macro argument list_sdks,
# a message will be displayed with the SDK path found from the search path.
macro(find_zephyr_sdk list_sdks)
  # Paths that are used to find installed Zephyr SDK versions
  SET(zephyr_sdk_search_paths
      /usr
      /usr/local
      /opt
      $ENV{HOME}
      $ENV{HOME}/.local
      $ENV{HOME}/.local/opt
      $ENV{HOME}/bin)

  # Search for Zephyr SDK version 0.0.0 which does not exist, this is needed to
  # return a list of compatible versions and find the best suited version that
  # is available.
  find_package(Zephyr-sdk 0.0.0 EXACT QUIET CONFIG PATHS ${zephyr_sdk_search_paths})

  # Remove duplicate entries and sort naturally in descending order.
  set(zephyr_sdk_found_versions ${Zephyr-sdk_CONSIDERED_VERSIONS})
  set(zephyr_sdk_found_configs ${Zephyr-sdk_CONSIDERED_CONFIGS})

  list(REMOVE_DUPLICATES Zephyr-sdk_CONSIDERED_VERSIONS)
  list(SORT Zephyr-sdk_CONSIDERED_VERSIONS COMPARE NATURAL ORDER DESCENDING)

  # Loop over each found Zephyr SDK version until one is found that is compatible.
  foreach(zephyr_sdk_candidate ${Zephyr-sdk_CONSIDERED_VERSIONS})
    if("${zephyr_sdk_candidate}" VERSION_GREATER_EQUAL "${Zephyr-sdk_FIND_VERSION}")
      # Find the path for the current version being checked and get the directory
      # of the Zephyr SDK so it can be checked.
      list(FIND zephyr_sdk_found_versions ${zephyr_sdk_candidate} zephyr_sdk_current_index)
      list(GET zephyr_sdk_found_configs ${zephyr_sdk_current_index} zephyr_sdk_current_check_path)
      get_filename_component(zephyr_sdk_current_check_path ${zephyr_sdk_current_check_path} DIRECTORY)

      # Then see if this version is compatible.
      find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION} QUIET CONFIG PATHS ${zephyr_sdk_current_check_path} NO_DEFAULT_PATH)

      if (${Zephyr-sdk_FOUND})
        if (NOT ${list_sdks})
          # A compatible version of the Zephyr SDK has been found which is the highest
          # supported version, exit.
          break()
        else()
          message(STATUS ${Zephyr-sdk_DIR})
        endif()
      endif()
    endif()
  endforeach()

  if (NOT ${Zephyr-sdk_FOUND})
    if (NOT ${list_sdks})
      # This means no compatible Zephyr SDK versions were found, set the version
      # back to the minimum version so that it is displayed in the error text.
      find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION} REQUIRED CONFIG PATHS ${zephyr_sdk_search_paths})
    endif()
  endif()
endmacro()
