set(BOARD lpcxpresso54114)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language(C ASM)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/master/src)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/remote/build/zephyr)

if(ZEPHYR_GCC_VARIANT STREQUAL "zephyr")
configure_file($ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/remote/build/zephyr/core1_image.bin $ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/master/build/core1_image.bin COPYONLY)
endif()

target_sources(app PRIVATE src/main_master.c)
if(CONFIG_SOC_SERIES_LPC54XXX)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/hal/nxp/mcux/devices/LPC54114/incbin.S)
endif()
