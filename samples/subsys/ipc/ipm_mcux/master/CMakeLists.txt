set(BOARD lpcxpresso54114)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language(C ASM)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/master/src)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/ipm_mcux/remote/build/zephyr)
target_sources(app PRIVATE src/main_master.c)
if(CONFIG_SOC_SERIES_LPC54XXX)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/hal/nxp/mcux/devices/LPC54114/incbin.S)
endif()
