cmake_minimum_required(VERSION 3.8.2)
if(BOARD STREQUAL "qemu_cortex_m3")
set(CONF_FILE qemu.conf)
else()
set(CONF_FILE default.conf)
endif()

LIST(APPEND QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-stack-tester)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

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

if(CONFIG_BT_MESH)
target_sources(app PRIVATE src/mesh.c)
endif()
