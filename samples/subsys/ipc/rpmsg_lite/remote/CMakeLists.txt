cmake_minimum_required(VERSION 3.13.1)

set(BOARD lpcxpresso54114_m0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/rpmsg_lite)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)

if(CONFIG_SOC_SERIES_LPC54XXX)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/include/platform/lpc5411x)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/porting/platform/lpc5411x/rpmsg_platform_zephyr_ipm.c)
endif()

target_sources(app PRIVATE src/main_remote.c)
