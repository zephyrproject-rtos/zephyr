set(BOARD lpcxpresso54114)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language(C ASM)

# Location of external dependencies:
set(LIBMETAL_INCLUDE_DIR "/home/kristian/libmetal/build-lpc/lib/include" CACHE PATH "Path to the libmetal header files")
set(LIBMETAL_LIBRARY     "/home/kristian/libmetal/build-lpc/lib/libmetal.a" CACHE FILEPATH "Path to the libmetal library")
set(OPENAMP_INCLUDE_DIR  "/home/kristian/openamp/lib/include" CACHE PATH "Path to the OpenAMP header files")
set(OPENAMP_LIBRARY      "/home/kristian/openamp/build-lpc/lib/libopen_amp.a" CACHE FILEPATH "Path to the OpenAMP library")

set(ZEPHYR_KERNEL_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zephyr/kernel/libkernel.a")
set(PLATFORM_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/openamp/remote/build/zephyr)
target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})

target_sources(app PRIVATE src/main_master.c ${PLATFORM_DIR}/platform.c ${PLATFORM_DIR}/platform_ops.c ${PLATFORM_DIR}/resource_table.c)
if(CONFIG_SOC_SERIES_LPC54XXX)
	target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/hal/nxp/mcux/devices/LPC54114/incbin.S)
endif()

target_link_libraries(app
	${OPENAMP_LIBRARY}
	${LIBMETAL_LIBRARY}
	${ZEPHYR_KERNEL_LIBRARY})

