add_custom_target(
  usage
  ${CMAKE_COMMAND}
  -DBOARD_ROOT=${BOARD_ROOT}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/usage.cmake
  )

add_custom_target(
  kconfig-usage
  ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/kconfig-usage.cmake
  )

# NB: The reason it is 'usage' and not help is that CMake already
# defines a target 'help'
