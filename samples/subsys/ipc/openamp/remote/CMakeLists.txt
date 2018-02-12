set(BOARD lpcxpresso54114_m0)

# Location of external dependencies:
set(LIBMETAL_INCLUDE_DIR "/home/kristian/libmetal/build-lpc-m0/lib/include" CACHE PATH "Path to the libmetal header files")
set(LIBMETAL_LIBRARY     "/home/kristian/libmetal/build-lpc-m0/lib/libmetal.a" CACHE FILEPATH "Path to the libmetal library")
set(OPENAMP_INCLUDE_DIR  "/home/kristian/openamp/lib/include" CACHE PATH "Path to the OpenAMP header files")
set(OPENAMP_LIBRARY      "/home/kristian/openamp/build-lpc-m0/lib/libopen_amp.a" CACHE FILEPATH "Path to the OpenAMP library")

set(ZEPHYR_KERNEL_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zephyr/kernel/libkernel.a")
set(PLATFORM_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})

target_sources(app PRIVATE src/main_remote.c ${PLATFORM_DIR}/platform.c ${PLATFORM_DIR}/resource_table.c ${PLATFORM_DIR}/platform_ops.c)
target_link_libraries(app
	${OPENAMP_LIBRARY}
	${LIBMETAL_LIBRARY}
	${ZEPHYR_KERNEL_LIBRARY})

