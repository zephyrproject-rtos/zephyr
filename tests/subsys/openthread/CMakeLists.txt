# Boilerplate code, which pulls in the Zephyr build system.
cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(openthread_tests)

zephyr_library_include_directories(
	${ZEPHYR_BASE}/../modules/lib/openthread/include/
	${ZEPHYR_BASE}/../modules/lib/openthread/src/posix/platform/
	${ZEPHYR_BASE}/subsys/net/lib/openthread/platform/
	${ZEPHYR_BASE}/../modules/lib/openthread/examples/platforms/
	)

# Add your source file to the "app" target. This must come after
# the boilerplate code, which defines the target.
target_sources(app PRIVATE radio_test.c radio_stub.c)
