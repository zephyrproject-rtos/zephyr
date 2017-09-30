if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD arduino_101)
endif()

set(CONF_FILE default.conf)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

LIST(APPEND QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-stack-tester)
LIST(APPEND QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-server-bredr)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
target_sources(app PRIVATE
    src/main.c
    src/bttester.c
    src/gap.c
    src/gatt.c
    )
if(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
target_sources(app PRIVATE src/l2cap.c)
endif()
