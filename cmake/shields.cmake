if(CMAKE_SCRIPT_MODE_FILE AND NOT CMAKE_PARENT_LIST_FILE)
  # This file was invoked as a script directly with -P:
  # cmake -P shields.cmake
  #
  # Unlike boards.cmake, this takes no OUTPUT_FILE option, but
  # SHIELD_LIST_SPACE_SEPARATED is required.
  list(SORT SHIELD_LIST)
  foreach(shield ${SHIELD_LIST})
    message("${shield}")
  endforeach()
else()
  # This file was included into usage.cmake.
  set(sorted_shield_list ${SHIELD_LIST})
  list(SORT sorted_shield_list)
  foreach(shield ${sorted_shield_list})
    list(APPEND sorted_shield_cmds COMMAND ${CMAKE_COMMAND} -E echo "${shield}")
  endforeach()
endif()
