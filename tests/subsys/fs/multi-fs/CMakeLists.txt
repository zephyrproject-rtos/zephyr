include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

zephyr_compile_definitions(
  -DTEST_FLASH_OFFSET=0
  -DFLASH_AREA_STORAGE_OFFSET=0
  -DFLASH_AREA_STORAGE_SIZE=1048576
)

target_link_libraries(app ELMFAT)
target_link_libraries(app NFFS)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
