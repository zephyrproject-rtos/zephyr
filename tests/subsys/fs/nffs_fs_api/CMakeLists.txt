include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

if(BOARD STREQUAL qemu_x86)
    zephyr_compile_definitions(
        -DTEST_FLASH_OFFSET=0
        -DFLASH_AREA_NFFS_OFFSET=0
        -DFLASH_AREA_NFFS_SIZE=1048576
    )
elseif(BOARD STREQUAL nrf51_pca10028)
    set(CONF_FILE nrf5x.conf)
    zephyr_compile_definitions(
        -DTEST_FLASH_OFFSET=0x20000
    )
elseif(BOARD STREQUAL nrf52_pca10040)
    set(CONF_FILE nrf5x.conf)
    zephyr_compile_definitions(
        -DTEST_FLASH_OFFSET=0x20000
    )
elseif(BOARD STREQUAL nrf52840_pca10056)
    set(CONF_FILE nrf5x.conf)
    zephyrt_compile_definitions(
        -DTEST_FLASH_OFFSET=0x20000
    )
endif()

if(NOT TEST)
    target_compile_definitions(app PRIVATE
        -DTEST_basic
	)
else()
    target_compile_definitions(app PRIVATE
        -DTEST_${TEST}
	)
endif()


target_link_libraries(app NFFS)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
