set(CONF_FILE default.conf)

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
