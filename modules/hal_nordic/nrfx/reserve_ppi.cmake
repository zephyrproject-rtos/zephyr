# SPDX-License-Identifier: Apache-2.0

# Function to add PPI reservations
function(reserve_ppi module_name channels groups)
  # Since cmake doesn't support maps or list-of-lists, we make our own poor
  # man's version of a list-of-lists.
  #
  # We escape the ";" element delimiter with "," in order to later iterate on
  # the top-level list and extract the PPIs for each module.
  #
  # Similarly, the channel and group lists are separated using ":", and
  # extracted later with a regex.
  string(REPLACE ";" "," channels_escaped_list "${channels}")
  string(REPLACE ";" "," groups_escaped_list "${groups}")
  message(DEBUG "Reserve PPIs: chans [${channels_escaped_list}] groups [${groups_escaped_list}] for ${module_name}")

  set_property(GLOBAL APPEND PROPERTY PPI_MODULE_MAP "${module_name},${channels_escaped_list}:${groups_escaped_list}")
endfunction()

function(finalize_ppi_reservations)
  # Retrieve the concatenated list of modules and their reserved PPI channels/groups
  get_property(PPI_MODULE_MAP GLOBAL PROPERTY PPI_MODULE_MAP)

  set(PPI_CHANNELS_LIST "")
  set(PPI_GROUPS_LIST "")
  set(PPI_INFO_MESSAGE "")

  foreach(MODULE IN LISTS PPI_MODULE_MAP)
    string(REPLACE "," ";" MODULE_ENTRY "${MODULE}")

    # Pop the module's name from the list
    list(GET MODULE_ENTRY 0 MODULE_NAME)
    list(REMOVE_AT MODULE_ENTRY 0)

    # Extract channel and group lists
    string(REGEX MATCH ".*:" CHANNELS "${MODULE_ENTRY}")
    string(REPLACE ":" "" CHANNELS "${CHANNELS}")
    string(REGEX MATCH ":.*" GROUPS "${MODULE_ENTRY}")
    string(REPLACE ":" "" GROUPS "${GROUPS}")

    message(DEBUG "module: ${MODULE_NAME} chans ${CHANNELS} groups ${GROUPS}")

    # Detect if channels are not already reserved
    foreach(item IN LISTS CHANNELS)
      list(FIND PPI_CHANNELS_LIST ${item} index)
      if(${index} GREATER -1)
        message(STATUS "Reserved PPIs: ${PPI_INFO_MESSAGE}")
        message(FATAL_ERROR "${MODULE_NAME} takes already reserved channel ${item}")
      endif()
    endforeach()

    # Detect if groups are not already reserved
    foreach(item IN LISTS GROUPS)
      list(FIND PPI_GROUPS_LIST ${item} index)
      if(${index} GREATER -1)
        message(STATUS "Reserved PPIs: ${PPI_INFO_MESSAGE}")
        message(FATAL_ERROR "${MODULE_NAME} takes group ${item} but is already reserved")
      endif()
    endforeach()

    # Append the PPIs to the reserved list
    list(APPEND PPI_CHANNELS_LIST ${CHANNELS})
    list(APPEND PPI_GROUPS_LIST ${GROUPS})

    # Pretty-print the lists for user transparency
    string(REPLACE ";" ", " PRINT_CHANNELS "${CHANNELS}")
    string(REPLACE ";" ", " PRINT_GROUPS "${GROUPS}")
    set(PPI_INFO_MESSAGE
      "${PPI_INFO_MESSAGE} ${MODULE_NAME}: channels [${PRINT_CHANNELS}] groups [${PRINT_GROUPS}]")
  endforeach()

  # Display the info message
  message(STATUS "Reserved PPIs: ${PPI_INFO_MESSAGE}")

  # Convert the list of numbers to a bitfield expression
  set(PPI_CHANNELS_DEFINE "0 ")
  foreach(NUMBER IN LISTS PPI_CHANNELS_LIST)
    if(PPI_CHANNELS_DEFINE)
      set(PPI_CHANNELS_DEFINE "${PPI_CHANNELS_DEFINE} | ")
    endif()
    set(PPI_CHANNELS_DEFINE "${PPI_CHANNELS_DEFINE}BIT(${NUMBER})")
  endforeach()

  set(PPI_GROUPS_DEFINE "0 ")
  foreach(NUMBER IN LISTS PPI_GROUPS_LIST)
    if(PPI_GROUPS_DEFINE)
      set(PPI_GROUPS_DEFINE "${PPI_GROUPS_DEFINE} | ")
    endif()
    set(PPI_GROUPS_DEFINE "${PPI_GROUPS_DEFINE}BIT(${NUMBER})")
  endforeach()

  # Generate the header used by nrfx_glue.h
  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/include/generated/ppi_reserved.h "
    #pragma once

    #include <zephyr/sys/util_macro.h>

    #define CMAKE_GEN_PPI_RESERVED_CHANNELS (${PPI_CHANNELS_DEFINE})
    #define CMAKE_GEN_PPI_RESERVED_GROUPS (${PPI_GROUPS_DEFINE})
    ")
endfunction()

function(register_function_finalize_ppi_reservations)
  # Defer the call to finalize_ppi_reservations until the end of the configure stage
  # This should only be called once.
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL finalize_ppi_reservations)
endfunction()
