# We are just defining available libraries created in the Bluetooth subsystem in order to provide
# easy understanding of what is available. Mesh is part of host in this example.

add_library(subsys_bluetooth STATIC "")

include_relative(common/CMakeLists.txt)
include_relative_ifdef(CONFIG_BT_HCI host/CMakeLists.txt)
include_relative_ifdef(CONFIG_BT_CONN services/CMakeLists.txt)

if(CONFIG_BT_CTLR)
  if(CONFIG_BT_LL_SW)
    include_relative(controller/CMakeLists.txt)
  endif()
endif()

if(CONFIG_BT_SHELL)
  include_relative(shell/CMakeLists.txt)
endif()

target_link_libraries(subsys_bluetooth PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE subsys_bluetooth)

zephyr_set_property(TARGET subsys_bluetooth MEMORY_SPACE kernel)
